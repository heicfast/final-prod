/* cr_decomp.cpp - block decompression layer.
 *
 * Algorithms verified against OpenZFS master sources this session:
 *   - lzjb: module/zfs/lzjb.c zfs_lzjb_decompress_buf (MATCH_BITS=6,
 *     MATCH_MIN=3, OFFSET_MASK=0x3FF, control-byte tag scheme)
 *   - zle: module/zfs/zle.c zfs_zle_decompress_buf (n = SPA_MINBLOCKSHIFT
 *     = 9: run byte b -> 1+b literal bytes when <= 9, else b+1-9 zeros)
 *   - gzip-1..9: one raw-deflate family (zlib inflate, windowBits -15)
 *   - lz4: reference LZ4 block format (ZFS lz4.c calls LZ4_decompress_safe)
 *   - zstd: standard frames; ZFS may prefix a skippable frame which
 *     ZSTD_decompress skips on its own
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/compress/cr_decomp.h"
#include <zlib.h>
#if defined(CR_HAVE_LZ4)
#include <lz4.h>
#endif
#if defined(CR_HAVE_ZSTD)
#include <zstd.h>
#endif

namespace cr {
namespace compress {

namespace {

/* OpenZFS module/zfs/lzjb.c - byte-exact port of zfs_lzjb_decompress_buf
 * with one hardening divergence (GAP-P1-2): upstream trusts the block
 * checksum to bound the source; our engine parses corrupt pools with no
 * checksum gate, so every source read is bounded against src+psize and
 * any over-read returns false (the caller zero-fills + counts the span
 * unreadable - never silently wrong bytes). */
bool lzjb_decompress(const u8* src, size_t s_len, u8* dst, size_t d_len) {
    constexpr unsigned MATCH_BITS = 6;
    constexpr unsigned MATCH_MIN = 3;
    constexpr unsigned OFFSET_MASK = (1u << (16 - MATCH_BITS)) - 1; /* 0x3FF */
    const u8* s = src;
    const u8* s_end = src + s_len;
    u8* d = dst;
    const u8* d_end = dst + d_len;
    u8 copymap = 0;
    unsigned copymask = 1u << 7; /* NBBY - 1 */

    while (d < d_end) {
        if ((copymask <<= 1) == (1u << 8)) {
            copymask = 1;
            if (s >= s_end)
                return false; /* truncated control stream */
            copymap = *s++;
        }
        if (copymap & copymask) {
            if (s + 2 > s_end)
                return false; /* truncated match header (need s[0], s[1]) */
            const int mlen = (s[0] >> (8 - MATCH_BITS)) + MATCH_MIN;
            const int offset = ((s[0] << 8) | s[1]) & int(OFFSET_MASK);
            s += 2;
            const u8* cpy = d - offset;
            if (cpy < dst)
                return false; /* reference before output start */
            for (int k = 0; k < mlen && d < d_end; ++k)
                *d++ = *cpy++;
        } else {
            if (s >= s_end)
                return false; /* truncated literal stream */
            *d++ = *s++;
        }
    }
    return true;
}

/* OpenZFS module/zfs/zle.c - byte-exact port of zfs_zle_decompress_buf
 * with n = SPA_MINBLOCKSHIFT (9). */
bool zle_decompress(const u8* src, size_t s_len, u8* dst, size_t d_len) {
    const u8* s = src;
    const u8* s_end = src + s_len;
    u8* d = dst;
    const u8* d_end = dst + d_len;
    constexpr unsigned N = 9;

    while (s < s_end && d < d_end) {
        int len = 1 + *s++;
        if (len <= int(N)) {
            if (s + len > s_end || d + len > d_end)
                return false;
            while (len-- != 0)
                *d++ = *s++;
        } else {
            len -= int(N);
            if (d + len > d_end)
                return false;
            while (len-- != 0)
                *d++ = 0;
        }
    }
    return d == d_end;
}

bool gzip_decompress(const u8* src, size_t psize, u8* dst, size_t lsize) {
    z_stream zs = {};
    if (inflateInit2(&zs, -15) != Z_OK) /* raw deflate */
        return false;
    zs.next_in = const_cast<Bytef*>(src);
    zs.avail_in = uInt(psize);
    zs.next_out = dst;
    zs.avail_out = uInt(lsize);
    const int rc = inflate(&zs, Z_FINISH);
    const bool ok = rc == Z_STREAM_END && zs.total_out == lsize;
    inflateEnd(&zs);
    return ok;
}

/* MS-XCA LZNT1 tagged-group payload (GAP-P1-1). The NTFS compression-block
 * envelope (per-cluster u16 header) is stripped by the ENGINE; this decodes
 * the chunk payload proper: a tag byte precedes every 8 values - a 0 bit is
 * one literal byte, a 1 bit a 16-bit LE tuple {back-reference, length} with
 * a DYNAMIC SPLIT between the tuple's offset and length halves that widens
 * as the output grows (MS-XCA 3.3: 4-bit displacement at output<16 through
 * 12-bit at 4096 - the split-tracking shape is shared with the wimboot
 * reference decoder and libyal's documented bit math):
 *   split starts at 12, threshold 16; after each value, while out > threshold
 *   { split--; threshold <<= 1; }
 *   length  = (tuple & ((1 << split) - 1)) + 3
 *   backoff = (tuple >> split) + 1     (relative to the current output end)
 * Bounds: every source read is checked against psize and every output write
 * against lsize (a corrupt tuple length or a back-reference before the
 * output start rejects the block - the caller zero-fills + marks it
 * unreadable; never wrong bytes). */
bool lznt1_payload_decode(const u8* src, size_t s_len, u8* dst, size_t d_len) {
    const u8* s = src;
    const u8* s_end = src + s_len;
    u8* d = dst;
    const u8* d_end = dst + d_len;
    unsigned split = 12;
    unsigned next_threshold = 16;
    unsigned tag_bits = 0;
    unsigned tag = 0;

    while (d < d_end) {
        if (tag_bits == 0) {
            if (s >= s_end)
                return false; /* truncated: tag byte missing */
            tag = *s++;
            tag_bits = 8;
        }
        if (tag & 1) {
            if (s + 2 > s_end)
                return false; /* truncated tuple */
            const u16 tuple = u16(*s) | (u16(s[1]) << 8);
            s += 2;
            const size_t len = size_t(tuple & ((1u << split) - 1)) + 3;
            const size_t back = size_t(tuple >> split) + 1;
            if (back > size_t(d - dst))
                return false; /* reference before the output start */
            if (len > size_t(d_end - d))
                return false; /* corrupt length overruns the block */
            const u8* cpy = d - back;
            for (size_t k = 0; k < len; ++k)
                d[k] = cpy[k];
            d += len;
        } else {
            if (s >= s_end)
                return false; /* truncated literal */
            *d++ = *s++;
        }
        tag >>= 1;
        --tag_bits;
        while (size_t(d - dst) > next_threshold) {
            if (split <= 4)
                return false; /* output exceeded the 4096 chunk max */
            --split;
            next_threshold <<= 1;
        }
    }
    return true;
}

} // namespace

u32 codec_from_zfs(u32 zc) {
    /* OpenZFS enum zio_compress: 0 inherit, 1 on, 2 off, 3 lzjb, 4 empty,
     * 5..13 gzip_1..9, 14 zle, 15 lz4, 16 zstd. */
    switch (zc) {
    case 3:
        return kLzjb;
    case 4:
        return kRaw; /* empty payload: all-zero block */
    case 14:
        return kZle;
    case 15:
        return kLz4;
    case 16:
        return kZstd;
    default:
        if (zc >= 5 && zc <= 13)
            return kGzip;
        return kRaw;
    }
}

bool codec_available(u32 codec) {
    switch (codec) {
    case kRaw:
    case kLzjb:
    case kZle:
    case kGzip:
    case kLznt1:
        return true;
    case kLz4:
#if defined(CR_HAVE_LZ4)
        return true;
#else
        return false;
#endif
    case kZstd:
#if defined(CR_HAVE_ZSTD)
        return true;
#else
        return false;
#endif
    default:
        return false;
    }
}

bool decompress_block(u32 codec, const u8* src, size_t psize, u8* dst, size_t lsize) {
    if (src == nullptr && psize != 0)
        return false;
    if (dst == nullptr && lsize != 0)
        return false;
    switch (codec) {
    case kRaw:
        if (psize != lsize)
            return false;
        for (size_t i = 0; i < lsize; ++i)
            dst[i] = src[i];
        return true;
    case kLzjb:
        return lzjb_decompress(src, psize, dst, lsize);
    case kZle:
        return zle_decompress(src, psize, dst, lsize);
    case kGzip:
        return gzip_decompress(src, psize, dst, lsize);
    case kLznt1:
        return lznt1_payload_decode(src, psize, dst, lsize);
    case kLz4:
#if defined(CR_HAVE_LZ4)
        return LZ4_decompress_safe(reinterpret_cast<const char*>(src), reinterpret_cast<char*>(dst),
                                   int(psize), int(lsize)) == int(lsize);
#else
        return false;
#endif
    case kZstd:
#if defined(CR_HAVE_ZSTD)
        return ZSTD_decompress(dst, lsize, src, psize) == lsize;
#else
        return false;
#endif
    default:
        return false;
    }
}

} // namespace compress
} // namespace cr
