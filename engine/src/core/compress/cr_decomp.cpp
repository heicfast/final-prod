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

/* OpenZFS module/zfs/lzjb.c - byte-exact port of zfs_lzjb_decompress_buf. */
bool lzjb_decompress(const u8* src, size_t, u8* dst, size_t d_len) {
    constexpr unsigned MATCH_BITS = 6;
    constexpr unsigned MATCH_MIN = 3;
    constexpr unsigned OFFSET_MASK = (1u << (16 - MATCH_BITS)) - 1; /* 0x3FF */
    const u8* s = src;
    u8* d = dst;
    const u8* d_end = dst + d_len;
    u8 copymap = 0;
    unsigned copymask = 1u << 7; /* NBBY - 1 */

    while (d < d_end) {
        if ((copymask <<= 1) == (1u << 8)) {
            copymask = 1;
            copymap = *s++;
        }
        if (copymap & copymask) {
            const int mlen = (s[0] >> (8 - MATCH_BITS)) + MATCH_MIN;
            const int offset = ((s[0] << 8) | s[1]) & int(OFFSET_MASK);
            s += 2;
            const u8* cpy = d - offset;
            if (cpy < dst)
                return false; /* reference before output start */
            for (int k = 0; k < mlen && d < d_end; ++k)
                *d++ = *cpy++;
        } else {
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
