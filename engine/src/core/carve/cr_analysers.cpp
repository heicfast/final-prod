/* cr_analysers.cpp - grammar validators (R4.3, F8: never header-only).
 *
 * Every validator reads through the slice with bounded reads, caps by the
 * entry max_size, and returns ok=false on any grammar break.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_signatures.h"
#include "cr/storage/cr_device.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace cr {
namespace carve {

namespace {

bool rd(IStorageDevice* s, u64 off, void* buf, u32 len) {
    return s->read_at(off, buf, len) == OK;
}

u32 rd32(IStorageDevice* s, u64 off) {
    u8 b[4];
    if (!rd(s, off, b, 4))
        return 0;
    return u32_from_bytes_le(b);
}
u32 rdbe32(IStorageDevice* s, u64 off) {
    u8 b[4];
    if (!rd(s, off, b, 4))
        return 0;
    return (u32(b[0]) << 24) | (u32(b[1]) << 16) | (u32(b[2]) << 8) | u32(b[3]);
}
u16 rd16(IStorageDevice* s, u64 off) {
    u8 b[2];
    if (!rd(s, off, b, 2))
        return 0;
    return u16_from_bytes_le(b);
}
u16 rdbe16(IStorageDevice* s, u64 off) {
    u8 b[2];
    if (!rd(s, off, b, 2))
        return 0;
    return u16((u32(b[0]) << 8) | b[1]);
}
u64 rd64(IStorageDevice* s, u64 off) {
    u8 b[8];
    if (!rd(s, off, b, 8))
        return 0;
    return u64_from_bytes_le(b);
}
u64 rdbe64(IStorageDevice* s, u64 off) {
    u8 b[8];
    if (!rd(s, off, b, 8))
        return 0;
    u64 v = 0;
    for (int i = 0; i < 8; ++i)
        v = (v << 8) | b[i];
    return v;
}

/* EBML vint: returns length in bytes (1..8), value in *v; 0 = invalid */
u32 ebml_vint(IStorageDevice* s, u64 off, u64* v) {
    u8 first = 0;
    if (!rd(s, off, &first, 1))
        return 0;
    u32 len = 0;
    u8 mask = 0x80;
    while (mask != 0) {
        ++len;
        if (first & mask)
            break;
        mask >>= 1;
    }
    if (len > 8)
        return 0;
    u8 raw[8];
    if (!rd(s, off, raw, len))
        return 0;
    u64 val = first & (mask - 1);
    for (u32 i = 1; i < len; ++i)
        val = (val << 8) | raw[i];
    *v = val;
    return len;
}

/* N-051: ebml_vint over an in-memory buffer (the EBML header payload is
 * bounded <= 0x200 by the og's element-bounds gate - read once, walk
 * locally). Same vint algorithm; 0 = invalid. */
u32 ebml_vint_local(const u8* buf, u64 buf_len, u64 off, u64* v) {
    if (off >= buf_len)
        return 0;
    const u8 first = buf[off];
    u32 len = 0;
    u8 mask = 0x80;
    while (mask != 0) {
        ++len;
        if (first & mask)
            break;
        mask >>= 1;
    }
    if (len > 8 || off + len > buf_len)
        return 0;
    u64 value = u64(first & u8(mask - 1));
    for (u32 i = 1; i < len; ++i)
        value = (value << 8) | buf[off + i];
    if (v)
        *v = value;
    return len;
}

/* ---- P1-12 shared block-scan helpers ------------------------------- *
 * The forward tail-marker searches (PDF %%EOF, ZIP EOCD), the JPEG SOS
 * entropy scan and the MP3 frame walk used to step in 0.5-1 KiB reads;
 * on an uncached transport that is one read_at syscall per step - up to
 * 2,097,152 calls for a single ZIP candidate against the 2 GiB cap
 * (multi-minute stalls per hit, hours per disk). All of them now read
 * kProbeBlock (1 MiB) windows: 1024x fewer syscalls while keeping the
 * FULL scan distance (no result truncation), each block clamped to the
 * slice end and seamed with (magic_len - 1) overlap bytes so markers
 * spanning a block boundary are always found. Per-call buffers keep the
 * helpers thread-safe (validators run inside parallel carve workers). */

constexpr u32 kProbeBlock = 1u << 20; /* 1 MiB probe window */

u64 probe_limit(IStorageDevice* s, u64 hit, u64 cap) {
    return std::min(hit + cap, s->size());
}

/* finalize a validated PDF (end = offset just past the effective %%EOF) */
static CarveResult carve_pdf_done(CarveResult r, u64 end) {
    r.ok = true;
    r.end = end;
    r.confidence = 900;
    r.note = "%%EOF(last-before-next-PDF,1MiB-blocks)";
    return r;
}

/* FIRST geometry-consistent PK\x05\x06 in [start, limit) wins. The old
 * last-wins rule attributed the EOCD of every LATER archive in the window
 * to this candidate - cross-file extent corruption (observed in the carve
 * fixtures: plant_zip validated to the DOCX plant's EOCD). A candidate
 * EOCD is accepted when its central directory fits inside the file:
 * entries > 0, cd_offset >= 30 (never before the first local header) and
 * cd_offset + cd_size <= eocd_rel. Returns the absolute EOCD position, or
 * 0 when none passes. */
u64 scan_first_eocd(IStorageDevice* s, u64 start, u64 limit) {
    if (limit <= start + 22)
        return 0;
    std::vector<u8> buf(kProbeBlock);
    u64 block = start;
    while (block < limit) {
        const u64 remain = limit - block;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < 22)
            break;
        if (!rd(s, block, buf.data(), take))
            break;
        for (u32 i = 0; i + 22 <= take; ++i) {
            if (std::memcmp(buf.data() + i, "PK\x05\x06", 4) != 0)
                continue;
            const u64 eocd_pos = block + i;
            if (eocd_pos + 22 > limit)
                return 0; /* record truncated by the slice end - stop */
            /* the 22-byte record may straddle the block edge: reread it
             * whole (single small read, once per candidate EOCD) */
            u8 eocd[22];
            if (!rd(s, eocd_pos, eocd, 22))
                return 0;
            const u16 entries = u16_from_bytes_le(eocd + 10);
            const u32 cd_size = u32_from_bytes_le(eocd + 12);
            const u32 cd_offset = u32_from_bytes_le(eocd + 16);
            const u64 eocd_rel = eocd_pos - start;
            if (entries == 0)
                continue;
            if (cd_offset < 30)
                continue; /* CD before the first possible local header */
            if (u64(cd_offset) + cd_size > eocd_rel)
                continue; /* CD must fit inside the file, before the EOCD */
            return eocd_pos;
        }
        if (block + take >= limit)
            break;
        block += u64(take) - 3; /* 4-byte magic: (mlen-1) seam */
    }
    return 0;
}

/* CARVE-2: nearest EOCD that OWNS the central-directory entry we anchor
 * on - the anchor must lie inside the EOCD's implied CD region
 * [file_start+cd_offset, file_start+cd_offset+cd_size). Geometry is
 * relative to the implied ARCHIVE start (scan_first_eocd's geometry is
 * LFH-relative and mis-attributes a later archive's EOCD to a CD anchor). */
static u64 scan_eocd_for_cd_anchor(IStorageDevice* s, u64 anchor, u64 limit) {
    const u64 scan_end = std::min<u64>(limit, anchor + (1ull << 20));
    if (scan_end <= anchor + 22)
        return 0;
    std::vector<u8> buf(kProbeBlock);
    u64 block = anchor;
    while (block < scan_end) {
        const u64 remain = scan_end - block;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < 22)
            break;
        if (!rd(s, block, buf.data(), take))
            break;
        for (u32 i = 0; i + 22 <= take; ++i) {
            if (std::memcmp(buf.data() + i, "PK\x05\x06", 4) != 0)
                continue;
            const u64 eocd_pos = block + i;
            u8 eocd[22];
            if (!rd(s, eocd_pos, eocd, 22))
                return 0;
            const u16 entries = u16_from_bytes_le(eocd + 10);
            const u32 cd_size = u32_from_bytes_le(eocd + 12);
            const u32 cd_offset = u32_from_bytes_le(eocd + 16);
            if (entries == 0 || cd_offset < 30)
                continue;
            const u64 span = u64(cd_offset) + u64(cd_size); /* head+CD bytes */
            if (eocd_pos < span)
                continue; /* implied archive start underflows */
            const u64 file_start = eocd_pos - span;
            if (anchor < file_start + cd_offset || anchor >= file_start + span)
                continue; /* this EOCD's CD region does not contain us */
            return eocd_pos;
        }
        if (block + take >= scan_end)
            break;
        block += u64(take) - 3;
    }
    return 0;
}

} // namespace

/* ---- JPEG: SOI + marker walk to EOI (F4-class grammar, corpus SOF0) ---- */
CarveResult validate_jpeg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u64 pos = hit + 2; /* skip FFD8 */
    bool saw_sof = false, saw_sos = false;
    int markers = 0;
    while (pos + 4 <= hit + cap) {
        if (++markers > 65536)
            return r; /* pathological */
        u8 m[4];
        if (!rd(s, pos, m, 2))
            return r;
        if (m[0] != 0xFF)
            return r; /* desync */
        const u8 mk = m[1];
        if (mk == 0xD8 || (mk >= 0xD0 && mk <= 0xD7) || mk == 0x01) {
            pos += 2; /* standalone */
            continue;
        }
        if (mk == 0xD9) { /* EOI */
            r.ok = saw_sof && saw_sos;
            r.end = pos + 2;
            r.confidence = r.ok ? 900 : 400;
            r.note = r.ok ? "marker-walk+EOI" : "no-SOF/SOS";
            return r;
        }
        if (mk == 0xDA)
            saw_sos = true;
        if (mk == 0xC0 || mk == 0xC1 || mk == 0xC2 || mk == 0xC3)
            saw_sof = true;
        /* CARVE-6 (AUDIT-PIPELINE, F-A3-8): og gates SOF0 on precision 8
         * (`bVar1==0xc0 && pbVar25[4]==8`, corpus 39653) - a corrupted
         * SOF0 with a nonsense precision byte must not validate. */
        if (mk == 0xC0) {
            u8 prec[1];
            if (rd(s, pos + 4, prec, 1) && prec[0] != 8)
                saw_sof = false;
        }
        /* JPEG segment lengths are BIG-endian (wire-correct read; the
         * original LE read desynced on every real-world file) */
        u16 len = rdbe16(s, pos + 2);
        if (len < 2)
            return r;
        if (mk == 0xDA) {
            /* entropy data: scan for the next marker (FF xx non-zero).
             * P1-12: 1 MiB block reads with a 1-byte seam - the 512-byte
             * stepping this replaced issued up to 196,608 read_at calls
             * per file against the 96 MiB cap. */
            u64 p = pos + 2 + u64(len);
            const u64 elim = probe_limit(s, hit, cap);
            std::vector<u8> buf(kProbeBlock);
            bool found = false;
            while (p + 2 <= elim) {
                const u64 remain = elim - p;
                const u32 take = u32(std::min<u64>(remain, kProbeBlock));
                if (take < 2)
                    break;
                if (!rd(s, p, buf.data(), take))
                    return r;
                for (u32 i = 0; i + 1 < take; ++i) {
                    if (buf[i] == 0xFF && buf[i + 1] != 0 &&
                        !(buf[i + 1] >= 0xD0 && buf[i + 1] <= 0xD7)) {
                        pos = p + i;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
                p += take - 1; /* 1-byte seam: boundary markers */
            }
            if (!found)
                return r; /* cap reached without a closing marker */
            continue;
        }
        pos += 2 + u64(len);
    }
    return r; /* cap reached without EOI */
}

/* ---- PNG: magic + IHDR dims + IEND ---- */
CarveResult validate_png(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* IHDR at hit+8: length(4, BIG-endian)=13, type(4) */
    if (rdbe32(s, hit + 8) != 13)
        return r;
    u8 ihdr[4];
    if (!rd(s, hit + 12, ihdr, 4))
        return r;
    if (std::memcmp(ihdr, "IHDR", 4) != 0)
        return r;
    u32 w = rdbe32(s, hit + 16), h = rdbe32(s, hit + 20);
    if (w == 0 || h == 0 || w > 8000 * 8000 || h > 8000 * 8000)
        return r;
    /* walk chunks to IEND */
    u64 pos = hit + 8;
    int chunks = 0;
    while (pos + 12 <= hit + cap) {
        if (++chunks > 4096)
            return r;
        u32 clen = rdbe32(s, pos);
        u8 type[4];
        if (!rd(s, pos + 4, type, 4))
            return r;
        if (std::memcmp(type, "IEND", 4) == 0) {
            r.ok = true;
            r.end = pos + 12; /* len+type+crc */
            r.confidence = 950;
            r.note = "IHDR+IEND";
            return r;
        }
        if (clen > 64ull * 1024 * 1024)
            return r; /* chunk cap */
        pos += 12 + u64(clen);
    }
    return r;
}

/* ---- TIFF family: IFD0 walk + last-strip extent (fragmentation-aware) ---- */
static CarveResult tiff_common(IStorageDevice* s, u64 hit, u64 cap, bool cr2) {
    CarveResult r;
    u8 bo[2];
    if (!rd(s, hit, bo, 2))
        return r;
    const bool le = bo[0] == 'I';
    u64 ifd_off = le ? u64(rd16(s, hit + 4)) : 0;
    if (!le) {
        u8 b2[2];
        if (!rd(s, hit + 4, b2, 2))
            return r;
        ifd_off = (u32(b2[0]) << 8) | b2[1];
    }
    u64 end_data = 0;
    u64 pos = hit + ifd_off;
    int ifds = 0;
    while (ifd_off != 0 && ifds < 8) {
        ++ifds;
        if (pos + 2 > hit + cap)
            return r;
        /* entry count: byte-order aware (II=LE, MM=BE) */
        u16 n = le ? rd16(s, pos) : rdbe16(s, pos);
        if (n == 0 || n > 512)
            return r;
        u64 max_off = 0;
        for (int e = 0; e < n; ++e) {
            u64 ep = pos + 2 + u64(e) * 12;
            u16 tag = le ? rd16(s, ep) : rdbe16(s, ep);
            u16 type = le ? rd16(s, ep + 2) : rdbe16(s, ep + 2);
            u32 count = le ? rd32(s, ep + 4) : rdbe32(s, ep + 4);
            (void)tag;
            (void)type;
            /* value: either inline (4 bytes at ep+8) or offset at ep+8 */
            u32 val = le ? rd32(s, ep + 8) : rdbe32(s, ep + 8);
            if (count > 4) {
                u64 data_at = hit + u64(val);
                u64 sz =
                    u64(count) *
                    (type == 3 ? 2
                               : (type == 4 || type == 9 ? 4 : (type == 5 || type == 10 ? 8 : 1)));
                if (data_at + sz > max_off)
                    max_off = data_at + sz;
            } else if (u64(val) + count > max_off) {
                max_off = u64(val) + count;
            }
        }
        if (max_off > end_data)
            end_data = max_off;
        u32 next = le ? rd32(s, pos + 2 + u64(n) * 12) : rdbe32(s, pos + 2 + u64(n) * 12);
        ifd_off = next;
        pos = hit + next;
    }
    if (end_data == 0)
        return r;
    r.ok = true;
    r.end = hit + (end_data > 512 ? end_data : 512);
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = cr2 ? 850 : 800;
    r.note = "IFD-walk";
    return r;
}

CarveResult validate_tiff(IStorageDevice* s, u64 hit, u64 cap) {
    return tiff_common(s, hit, cap, false);
}

CarveResult validate_cr2(IStorageDevice* s, u64 hit, u64 cap) {
    /* CR2: TIFF magic + "CR" sub-magic at +8 (Canon) */
    u8 cr[2];
    if (!rd(s, hit + 8, cr, 2))
        return CarveResult{};
    if (cr[0] != 'C' || cr[1] != 'R')
        return CarveResult{};
    return tiff_common(s, hit, cap, true);
}

/* ---- WebP: RIFF + size + chunk fourCC ---- */
CarveResult validate_webp(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 webp[4];
    if (!rd(s, hit + 8, webp, 4))
        return r;
    if (std::memcmp(webp, "WEBP", 4) != 0)
        return r;
    u32 size = rd32(s, hit + 4);
    if (size < 12 || size > 128ull * 1024 * 1024)
        return r;
    u8 chunk[4];
    if (!rd(s, hit + 12, chunk, 4))
        return r;
    if (std::memcmp(chunk, "VP8 ", 4) != 0 && std::memcmp(chunk, "VP8L", 4) != 0 &&
        std::memcmp(chunk, "VP8X", 4) != 0)
        return r;
    r.ok = true;
    r.end = hit + 8 + u64(size);
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = 800;
    r.note = "RIFF+WEBP+chunk";
    return r;
}

/* ---- HEIC: ISO-BMFF ftyp heic/heix/mif1 + box walk ---- */
static CarveResult bmff_walk(IStorageDevice* s, u64 hit, u64 cap, const char* const* brands,
                             size_t brand_count) {
    CarveResult r;
    u8 brand[5] = {0};
    if (!rd(s, hit + 8, brand, 4))
        return r;
    bool brand_ok = false;
    for (size_t i = 0; i < brand_count; ++i)
        if (std::memcmp(brand, brands[i], 4) == 0)
            brand_ok = true;
    if (!brand_ok)
        return r;
    /* walk top-level boxes */
    u64 pos = hit;
    u64 last_end = hit;
    int boxes = 0;
    while (pos + 8 <= hit + cap) {
        if (++boxes > 4096)
            break;
        u32 sz = rdbe32(s, pos);
        u8 type[4];
        if (!rd(s, pos + 4, type, 4))
            break;
        if (sz == 1) {
            u64 big = rdbe64(s, pos + 8); /* largesize: BIG-endian */
            if (big < 16 || pos + big > hit + cap)
                break;
            pos += big;
        } else if (sz == 0) {
            pos = hit + cap; /* to-EOF box */
        } else {
            if (pos + sz > hit + cap) {
                pos = hit + cap;
                break;
            }
            pos += sz;
        }
        last_end = pos;
        if (pos >= hit + cap)
            break;
    }
    if (last_end <= hit)
        return r;
    r.ok = true;
    r.end = last_end;
    r.confidence = 750;
    r.note = "ftyp+box-walk";
    return r;
}

CarveResult validate_heic(IStorageDevice* s, u64 hit, u64 cap) {
    /* N-052: heic-PROPER only (the og: brand 0x68656963 -> label L"HEIC",
     * type-id 0x7C). The HEIF-family siblings (heix/mif1/msf1) moved to
     * validate_heif, which carries the og's rewritten display label. */
    static const char* const brands[] = {"heic"};
    CarveResult r = bmff_walk(s, hit, cap, brands, 1);
    if (r.ok)
        r.note = "brand=heic";
    return r;
}

CarveResult validate_heif(IStorageDevice* s, u64 hit, u64 cap) {
    /* N-052: HEIF-family siblings only (mif1/heix/msf1). The og's analyser
     * relabels these L"HEIF" (the brand slot rewrite) - our registry row
     * carries that display label; heic-proper stays on FMT_HEIC. */
    static const char* const brands[] = {"heix", "mif1", "msf1"};
    CarveResult r = bmff_walk(s, hit, cap, brands, 3);
    if (r.ok)
        r.note = "HEIF-family (og display label: HEIF)";
    return r;
}
CarveResult validate_mp4(IStorageDevice* s, u64 hit, u64 cap) {
    static const char* const brands[] = {"isom", "mp42", "mp41", "iso2", "avc1", "dash"};
    CarveResult r = bmff_walk(s, hit, cap, brands, 6);
    if (r.ok)
        r.confidence = 780;
    return r;
}
CarveResult validate_mov(IStorageDevice* s, u64 hit, u64 cap) {
    static const char* const brands[] = {"qt  "};
    CarveResult r = bmff_walk(s, hit, cap, brands, 1);
    if (r.ok)
        r.confidence = 720;
    return r;
}

CarveResult validate_mov_naked(IStorageDevice* s, u64 hit, u64 cap) {
    /* S-017: conservative mirror of the og's list-layer MovAnalyzer for
     * ftyp-destroyed files (RAWSearchDLL FUN_180004fb0 :2919-4208 - the
     * live registration path; the u16 key at :2748 is dead under the
     * zero-header guard :9954). Anchored on a top-level "moov" box: the
     * walk must consume well-formed boxes to the box end; accept with
     * raw-sign-class confidence. The og's exact accept depth is not
     * decoded (pending-continuation model :10114) - the conservative
     * accept here is documented in S-017's Resolution as the follow-up
     * surface. */
    CarveResult r;
    u64 pos = hit; /* hit = moov box start (size field) */
    u32 sz = rdbe32(s, pos);
    if (sz == 0 || sz == 1)
        return r; /* to-EOF/largesize moov: not a sane salvage anchor */
    if (u64(sz) < 8 || u64(sz) > cap)
        return r;
    /* the moov box must itself be well-formed: size covers the type */
    u8 type[4];
    if (!rd(s, pos + 4, type, 4) || std::memcmp(type, "moov", 4) != 0)
        return r;
    /* walk the top-level boxes that follow; require at least one more
     * well-formed box (mdat/free/wide) so a lone 4-byte moov string in
     * noise does not accept */
    u64 next = pos + sz;
    u32 second_sz = rdbe32(s, next);
    u8 second_type[4];
    if (!rd(s, next + 4, second_type, 4))
        return r;
    if (second_sz == 0 || second_sz == 1)
        return r; /* keep the conservative anchor: concrete sizes only */
    if (u64(second_sz) < 8 || next + u64(second_sz) > hit + cap)
        return r;
    r.ok = true;
    r.end = next + second_sz;
    r.confidence = 620;
    r.note = "naked-moov+box-walk";
    return r;
}

/* ---- PDF: %PDF-1.x + %%EOF ---- */
CarveResult validate_pdf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 v[2];
    if (!rd(s, hit + 5, v, 2))
        return r;
    if (v[0] < '1' || v[0] > '9' || v[1] != '.')
        return r;
    /* The LAST %%EOF before the NEXT '%PDF-' occurrence wins: incremental
     * updates append revisions at the tail (multi-%%EOF is legitimate),
     * but the old unbounded last-wins attributed every LATER pdf's %%EOF
     * to this hit - cross-file absorption (runner benchmark evidence:
     * every carved pdf came out ~300 MiB, the distance to the volume's
     * last pdf). The next '%PDF-' is a hard boundary (standard carver
     * behavior; a '%PDF-' literal inside content is vanishingly rare and
     * truncates early - far cheaper than absorbing whole later files).
     * P1-12: 1 MiB block reads throughout. */
    const u64 limit = probe_limit(s, hit, cap);
    std::vector<u8> buf(kProbeBlock);
    u64 best = 0;         /* offset just past the last %%EOF before the boundary */
    u64 block = hit + 16; /* skip the own header (+version digits) */
    while (block < limit) {
        const u64 remain = limit - block;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < 16)
            break;
        if (!rd(s, block, buf.data(), take))
            break;
        /* ordered scan: positions of both magics, ascending */
        for (u32 i = 0; i + 5 <= take; ++i) {
            if (std::memcmp(buf.data() + i, "%%EOF", 5) == 0) {
                best = block + i + 5;
                continue;
            }
            if (std::memcmp(buf.data() + i, "%PDF-", 5) == 0) {
                /* next file's header: this candidate ends at its last %%EOF */
                return best != 0 ? carve_pdf_done(r, best) : r;
            }
        }
        if (block + take >= limit)
            break;
        block += u64(take) - 4; /* 5-byte magics: (mlen-1) seam */
    }
    return best != 0 ? carve_pdf_done(r, best) : r;
}

/* ---- ZIP family: local header + EOCD; DOCX adds [Content_Types].xml ---- */
static CarveResult zip_common(IStorageDevice* s, u64 hit, u64 cap, bool office) {
    CarveResult r;
    /* N-068: the og's LFH candidate sanity gates (decompile :18385-18388,
     * byte-exact: reject when u16@+8 > 10 OR when (u16@+0x1a - 1) > 0x102).
     * The artifact (rawsearchdll--zip-analyser-carving.md :13-21) names
     * the +8 field "version_needed"; in the spec LFH layout +8 is the
     * compression method - the og gates the BYTES, and so do we. The
     * second gate's accept window is [1, 0x103]: the artifact's
     * "[1, 0x102]" range text is off-by-one vs the decompile's
     * `0x102 < field - 1` (erratum filed). Cheap FP suppressors: they
     * reject the PK\x03\x04 lookalikes the og rejects before any walk. */
    if (rd16(s, hit + 8) > 10)
        return r;
    {
        const unsigned raw = unsigned(rd16(s, hit + 0x1a)) - 1u;
        if (raw > 0x102u) /* wrap: 0 - 1 = 0xFFFFFFFF > 0x102 -> reject */
            return r;
    }
    if (office) {
        /* first local entry name must be "[Content_Types].xml" */
        u16 name_len = rd16(s, hit + 26);
        if (name_len != 19)
            return r;
        u8 nm[19];
        if (!rd(s, hit + 30, nm, 19))
            return r;
        if (std::memcmp(nm, "[Content_Types].xml", 19) != 0)
            return r;
    }
    /* FIRST geometry-consistent EOCD in the window wins: last-wins (the
     * old rule) absorbed the EOCD of every later archive in the window -
     * cross-file extent corruption. P1-12: 1 MiB block scan - the 1 KiB
     * stepping this replaced issued up to 2,097,152 read_at calls per
     * PK\x03\x04 hit against the 2 GiB cap. */
    const u64 limit = probe_limit(s, hit, cap);
    /* CARVE-2 (AUDIT-PIPELINE, F-A3-3): indexless salvage - the og carves
     * archives whose central directory is destroyed (analyser status 0xd)
     * by walking the LOCAL header chain: each LFH carries name/extra
     * lengths + compressed size, so the next entry follows deterministically.
     * Walk until a non-LFH byte, a bogus geometry, or the og's 5.24 MB
     * padding cap; flag low confidence. */
    auto indexless = [&]() -> CarveResult {
        CarveResult ir;
        u64 pos = hit;
        u64 last_end = hit;
        u32 n = 0;
        const u64 cap5 = hit + (5242880ull); /* og padding cap */
        while (pos + 30 <= limit && pos < cap5 && n < 65535) {
            u8 lfh[30];
            if (!rd(s, pos, lfh, 30) || std::memcmp(lfh, "PK\x03\x04", 4) != 0)
                break;
            const u16 nlen = u16(lfh[26]) | (u16(lfh[27]) << 8);
            const u16 elen = u16(lfh[28]) | (u16(lfh[29]) << 8);
            const u32 csize =
                u32(lfh[18]) | (u32(lfh[19]) << 8) | (u32(lfh[20]) << 16) | (u32(lfh[21]) << 24);
            const u32 usize =
                u32(lfh[22]) | (u32(lfh[23]) << 8) | (u32(lfh[24]) << 16) | (u32(lfh[25]) << 24);
            if (csize == 0xFFFFFFFFu)
                break; /* ZIP64: no deterministic chain without the CD */
            const u64 next = pos + 30 + nlen + elen + csize;
            if (next > limit || next <= pos)
                break;
            /* S-032: the LFH chain carries the inner inventory (name +
             * sizes + the data span) - the og's indexless walk reads the
             * same fields for its entry list */
            if (nlen > 0 && nlen <= 512 && ir.entries.size() < 4096) {
                CarveEntry e;
                std::string nm(size_t(nlen), '\0');
                if (rd(s, pos + 30, &nm[0], nlen)) {
                    e.name = std::move(nm);
                    e.size = usize;
                    e.data_offset = pos + 30 + nlen + elen;
                    e.data_len = csize;
                    ir.entries.push_back(std::move(e));
                }
            }
            pos = next;
            last_end = next;
            ++n;
        }
        if (n >= 1 && last_end > hit + 64) {
            ir.ok = true;
            ir.end = last_end;
            ir.confidence = 550;
            ir.note = "indexless-LFH-chain";
        }
        return ir;
    };
    const u64 eocd_pos = scan_first_eocd(s, hit, limit);
    if (eocd_pos == 0 || eocd_pos < hit + 22)
        return indexless();
    /* full EOCD record at eocd_pos (22 bytes) */
    u8 eocd[22];
    if (!rd(s, eocd_pos, eocd, 22))
        return r;
    if (std::memcmp(eocd, "PK\x05\x06", 4) != 0)
        return r;
    const u32 cd_size = u32_from_bytes_le(eocd + 12);
    const u32 cd_offset = u32_from_bytes_le(eocd + 16);
    const u16 entries = u16_from_bytes_le(eocd + 10);
    const u16 comment = u16_from_bytes_le(eocd + 20);
    if (entries == 0)
        return r;
    /* central directory must live inside the file, before the EOCD
     * (cd_offset is file-relative: never before the first local header) */
    if (cd_offset < 30)
        return r;
    if (u64(cd_offset) + cd_size > (eocd_pos - hit))
        return r;
    /* CARVE-2 ownership: the central directory must actually START at
     * hit+cd_offset. A LATER archive's EOCD can be geometry-consistent
     * relative to our hit by coincidence while its CD lands elsewhere -
     * without this check a destroyed-CD archive absorbs the next archive's
     * EOCD (cross-file extent corruption, the original F-A3-3 shape). A
     * failed ownership test means OUR CD is gone: indexless salvage. */
    {
        u8 cdm[4];
        if (!rd(s, hit + cd_offset, cdm, 4) || std::memcmp(cdm, "PK\x01\x02", 4) != 0)
            return indexless();
    }
    /* S-032: walk the central directory for the inner-entry inventory
     * (the og's per-entry parser FUN_180045690 + the name-merge
     * FUN_180007060: "the analyser returns the complete inner file
     * inventory"). Each CD record: nameLen u16@+28, extraLen@+30,
     * commentLen@+32, csize u32@+20, usize u32@+24, LFH offset u32@+42,
     * name bytes at +46. Bounded: entries <= the EOCD count, name bytes
     * sane, whole CD capped at 4 MiB. */
    {
        u64 cpos = hit + cd_offset;
        const u64 cd_end = hit + cd_offset + (cd_size < (4u << 20) ? cd_size : (4u << 20));
        u32 collected = 0;
        while (cpos + 46 <= cd_end && collected < entries && collected < 4096) {
            u8 cd[46];
            if (!rd(s, cpos, cd, 46) || std::memcmp(cd, "PK\x01\x02", 4) != 0)
                break;
            const u16 nlen = u16(cd[28]) | (u16(cd[29]) << 8);
            const u16 xlen = u16(cd[30]) | (u16(cd[31]) << 8);
            const u16 clen = u16(cd[32]) | (u16(cd[33]) << 8);
            const u32 csize = u32_from_bytes_le(cd + 20);
            const u32 usize = u32_from_bytes_le(cd + 24);
            const u32 lfh_off = u32_from_bytes_le(cd + 42);
            if (nlen > 0 && nlen <= 512) {
                CarveEntry e;
                std::string nm(size_t(nlen), '\0');
                if (rd(s, cpos + 46, &nm[0], nlen)) {
                    e.name = std::move(nm);
                    e.size = usize;
                    if (csize != 0xFFFFFFFFu && lfh_off != 0xFFFFFFFFu) {
                        /* localize the data span through the LFH's own
                         * name/extra lengths (the CD's copy can differ) */
                        u8 lfh[30];
                        if (rd(s, hit + lfh_off, lfh, 30) &&
                            std::memcmp(lfh, "PK\x03\x04", 4) == 0) {
                            const u16 ln = u16(lfh[26]) | (u16(lfh[27]) << 8);
                            const u16 lx = u16(lfh[28]) | (u16(lfh[29]) << 8);
                            e.data_offset = hit + lfh_off + 30 + ln + lx;
                            e.data_len = csize;
                        }
                    }
                    r.entries.push_back(std::move(e));
                }
            }
            const u64 adv = u64(46) + nlen + xlen + clen;
            if (adv == 0 || cpos + adv >= cd_end + 1)
                break;
            cpos += adv;
            ++collected;
        }
    }
    u64 end = eocd_pos + 22 + comment;
    /* CARVE-2: og zero-padding end extension - trailing all-zero 16-byte
     * blocks are absorbed into the archive (up to the 5.24 MB cap; the
     * og's last-16-bytes-zero test). */
    {
        const u64 pad_cap = end + 5242880ull;
        while (end + 16 <= limit && end < pad_cap) {
            u8 z[16];
            if (!rd(s, end, z, 16))
                break;
            bool zero = true;
            for (u8 b : z)
                if (b) {
                    zero = false;
                    break;
                }
            if (!zero)
                break;
            end += 16;
        }
    }
    if (end > limit)
        end = limit;
    r.ok = true;
    r.end = end;
    r.confidence = office ? 850 : 700;
    r.note = office ? "[Content_Types]+EOCD" : "EOCD+cd-geometry";
    return r;
}

CarveResult validate_zip(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, false);
}
CarveResult validate_docx(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, true);
}

/* CARVE-2: central-directory-anchored carving - the og's PREFERRED
 * anchor (PK\x01\x02): when the head of the archive is gone but the CD
 * survives, the entry still surfaces (names live in the CD). Scan
 * forward for the EOCD to size the span; without one, take a bounded
 * CD-sized window. */
CarveResult validate_zip_cd(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 cd[46];
    if (!rd(s, hit, cd, 46))
        return r;
    if (std::memcmp(cd, "PK\x01\x02", 4) != 0)
        return r;
    const u16 nlen = u16(cd[28]) | (u16(cd[29]) << 8);
    const u16 elen = u16(cd[30]) | (u16(cd[31]) << 8);
    const u16 clen = u16(cd[32]) | (u16(cd[33]) << 8);
    if (u64(nlen) > 512 || u64(elen) > 1024 || u64(clen) > 1024)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    const u64 eocd_pos = scan_eocd_for_cd_anchor(s, hit, limit);
    if (eocd_pos > hit + 46) {
        u8 eocd[22];
        if (rd(s, eocd_pos, eocd, 22) && std::memcmp(eocd, "PK\x05\x06", 4) == 0) {
            /* og analyser reconciliation: when the archive head is INTACT
             * (a live local header exactly at the implied file start), the
             * LFH anchor owns the carve and the CD anchor stands down - no
             * double emission. Only a destroyed head (the recovery case
             * this anchor exists for) carves here. */
            const u32 cd_size2 = u32_from_bytes_le(eocd + 12);
            const u32 cd_offset2 = u32_from_bytes_le(eocd + 16);
            const u64 file_start = eocd_pos - u64(cd_offset2) - cd_size2;
            {
                u8 lfh[4];
                if (rd(s, file_start, lfh, 4) && std::memcmp(lfh, "PK\x03\x04", 4) == 0)
                    return r; /* intact head: LFH anchor carves this */
            }
            const u16 comment = u16_from_bytes_le(eocd + 20);
            r.ok = true;
            r.end = eocd_pos + 22 + comment;
            if (r.end > limit)
                r.end = limit;
            r.confidence = 600;
            r.note = "cd-anchor+EOCD";
            return r;
        }
    }
    /* no EOCD: bounded CD-region window (the names alone recover) */
    r.ok = true;
    r.end = hit + 46 + nlen + elen + clen + 512;
    if (r.end > limit)
        r.end = limit;
    r.confidence = 550;
    r.note = "cd-anchor";
    return r;
}

/* ---- 7z: StartHeader offsets ---- */
CarveResult validate_7z(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* signature 7z BC AF 27 1C at 0..5; version 2 bytes; 4 CRC; then
     * StartHeader: next-header-offset u64 @12, next-header-size u64 @20,
     * next-header-crc u32 @28 */
    u64 nh_off = rd64(s, hit + 12);
    u64 nh_size = rd64(s, hit + 20);
    if (nh_off > cap || nh_size > cap)
        return r;
    u64 end = 32 + nh_off + nh_size;
    if (end > cap)
        return r;
    r.ok = true;
    r.end = hit + end;
    r.confidence = 880;
    r.note = "StartHeader";
    return r;
}

/* ---- SQLite: header fields + page math ---- */
CarveResult validate_sqlite(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* magic is 16 bytes "SQLite format 3\0" - registry matches first 12 */
    u8 tail[4];
    if (!rd(s, hit + 12, tail, 4))
        return r;
    if (std::memcmp(tail, "t 3", 3) != 0 || tail[3] != 0)
        return r;
    const u16 raw_ps = rd16(s, hit + 16);
    const u32 page_size = (raw_ps == 1) ? 65536u : u32(raw_ps); /* field: 1 = 64KiB */
    if (page_size < 512 || (page_size & (page_size - 1)) != 0 || page_size > 65536)
        return r;
    u32 pages = rd32(s, hit + 28);
    if (pages == 0 || pages > 2ull << 30 / page_size)
        return r;
    u64 end = u64(pages) * page_size;
    if (end > cap)
        end = cap;
    /* freelist + schema sanity-lite: version-valid-for at @18, reader ver @19 */
    r.ok = true;
    r.end = hit + end;
    r.confidence = 860;
    r.note = "page-math";
    return r;
}

/* ---- AVI: RIFF + AVI list ---- */
CarveResult validate_avi(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 av[4];
    if (!rd(s, hit + 8, av, 4))
        return r;
    if (std::memcmp(av, "AVI ", 4) != 0)
        return r;
    u32 size = rd32(s, hit + 4);
    if (size < 16 || size > 4ull << 30)
        return r;
    /* hdrl LIST at +12 */
    u8 list[4];
    if (!rd(s, hit + 16, list, 4))
        return r;
    if (std::memcmp(list, "hdrl", 4) != 0)
        return r;
    r.ok = true;
    r.end = hit + 8 + u64(size);
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = 720;
    r.note = "RIFF+AVI+hdrl";
    return r;
}

/* ---- MKV: EBML header + element walk ---- */
CarveResult validate_mkv(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* EBML header: 1A45DFA3 then size vint, then payload */
    u64 v = 0;
    u32 vlen = ebml_vint(s, hit + 4, &v);
    if (vlen == 0 || v == 0)
        return r; /* All-1 = streaming */
    /* N-051: the og's element-bounds gate (analyser-granular-batch1 :43-66,
     * FileMKVAnalyser FUN_180016c70): header element total
     * id_len + size_len + payload + 4 <= 0x200, else reject. */
    if (u64(vlen) + v + 4 > 0x200)
        return r;
    /* N-051: the og's DocType discrimination - the EBML header payload
     * carries the DocType element (ID 0x4282); accept webm (0x6D626577)
     * and matroska, reject every other DocType (an exotic EBML use is
     * rejected by the og's analyser and now by ours). */
    {
        u8 hdr[512];                           /* <= 0x200 by the bounds gate above */
        const u64 hdr_len = 4 + u64(vlen) + v; /* id(4) + size vint + payload */
        if (hdr_len > sizeof(hdr) || !rd(s, hit, hdr, u32(hdr_len)))
            return r;
        u64 p = 4 + vlen; /* first child inside the header payload */
        bool doctype_ok = false;
        while (p + 2 <= hdr_len) {
            u64 cid = 0, csz = 0;
            const u32 cid_len = ebml_vint_local(hdr, hdr_len, p, &cid);
            if (cid_len == 0)
                break;
            const u32 csz_len = ebml_vint_local(hdr, hdr_len, p + cid_len, &csz);
            if (csz_len == 0)
                break;
            const u64 cend = p + cid_len + csz_len + csz;
            if (cend > hdr_len)
                break;
            if (cid == 0x0282) { /* DocType - full EBML ID 0x4282;
                                  * ebml_vint strips the ID marker bit
                                  * (0x4000 for the 2-byte length class) */
                const u8* dt = hdr + p + cid_len + csz_len;
                const u64 dn = csz;
                if (dn == 4 && std::memcmp(dt, "webm", 4) == 0)
                    doctype_ok = true; /* the og's webm constant 0x6D626577 */
                else if (dn == 8 && std::memcmp(dt, "matroska", 8) == 0)
                    doctype_ok = true;
                break; /* the DocType decides; no need to walk further */
            }
            p = cend;
        }
        if (!doctype_ok)
            return r; /* no DocType, or an exotic one: the og rejects */
    }
    u64 pos = hit + 4 + vlen + v; /* end of EBML header */
    u64 last_end = pos;
    int elements = 0;
    while (pos + 2 <= hit + cap) {
        if (++elements > 4096)
            break;
        u64 id_v = 0;
        u32 id_len = ebml_vint(s, pos, &id_v);
        if (id_len == 0)
            break;
        u64 sz_v = 0;
        u32 sz_len = ebml_vint(s, pos + id_len, &sz_v);
        if (sz_len == 0)
            break;
        u64 sz = sz_v;
        if (sz == 0x7F)
            break; /* unknown size: streaming element */
        u64 total = u64(id_len) + sz_len + sz;
        if (sz > cap || pos + total > hit + cap) {
            pos = hit + cap;
            break;
        }
        pos += total;
        last_end = pos;
    }
    if (last_end <= hit + 16)
        return r;
    r.ok = true;
    r.end = last_end;
    r.confidence = 700;
    r.note = "EBML-walk";
    return r;
}

/* ---- RAR4: marker + archive header + block walk to END ---- */
CarveResult validate_rar4(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* "Rar!\x1A\x07\x00" (7 bytes) then MAIN_HEAD: HEAD_CRC u16@+7,
     * HEAD_TYPE u8@+9 (0x73), HEAD_FLAGS u16@+10, HEAD_SIZE u16@+12
     * (+optional HIGH_POS_SIZE u32 when flags&0x100). S-032 CORRECTED:
     * the old code read HEAD_SIZE at +11 (flags-high | size-low as LE) -
     * off by one against the real RAR4 archive layout; it slipped through
     * because no positive real-layout fixture existed (only the RAR5
     * rejection test). Real MAIN_HEADs (HEAD_SIZE=13, flags=0) now parse
     * correctly. */
    u16 hsize = rd16(s, hit + 12);
    if (hsize < 13 || hsize > 4096)
        return r;
    u64 pos = hit + 7 + hsize;
    int blocks = 0;
    while (pos + 7 <= hit + cap) {
        if (++blocks > 65536)
            return r;
        u8 hdr[7];
        if (!rd(s, pos, hdr, 7))
            return r;
        const u8 type = hdr[2];
        u16 flags = u16(hdr[3]) | (u16(hdr[4]) << 8);
        u16 size = u16(hdr[5]) | (u16(hdr[6]) << 8);
        if (size < 7)
            return r;
        if (type == 0x7B) { /* END block: no data */
            r.ok = true;
            r.end = pos + size;
            r.confidence = 820;
            r.note = "block-walk+END";
            return r;
        }
        if (type == 0x74 && r.entries.size() < 4096) {
            /* S-032: FILE block - the og's walker FUN_180012d00 extracts
             * entry names with a 272-byte buffer. Classic FILE_HEAD after
             * the 7-byte base: [PACK_SIZE u32 if flags&0x8000] UNP_SIZE
             * u32, HOST_OS u8, FILE_CRC u32, FTIME u32, UNP_VER u8,
             * METHOD u8, NAME_SIZE u16, ATTR u32, [HIGH sizes 2xu32 if
             * flags&0x100], NAME bytes, [SALT if 0x4000, EXTTIME if
             * 0x200 - both inside HEAD_SIZE]. Data follows the block. */
            u64 off = 7;
            u32 pack = 0;
            if (flags & 0x8000) {
                pack = rd32(s, pos + off);
                off += 4;
            }
            const u32 unp = rd32(s, pos + off);
            off += 4 + 1 + 4 + 4 + 1 + 1; /* host_os, file_crc, ftime, unp_ver, method */
            const u16 name_size = rd16(s, pos + off);
            off += 2 + 4; /* name_size, attr */
            if (flags & 0x100)
                off += 8; /* high pack + high unp */
            if (name_size > 0 && name_size <= 272 && pos + off + name_size <= hit + cap) {
                CarveEntry e;
                std::string nm(size_t(name_size), '\0');
                if (rd(s, pos + off, &nm[0], name_size)) {
                    e.name = std::move(nm);
                    e.size = unp;
                    /* the data follows the whole block header (HEAD_SIZE
                     * covers name/salt/exttime); extent = [pos+size, pack) */
                    if (pack != 0 && pos + size <= hit + cap)
                        e.data_offset = pos + size, e.data_len = pack;
                    r.entries.push_back(std::move(e));
                }
            }
        }
        u64 adv = size;
        if (flags & 0x8000)
            adv += rd32(s, pos + 7); /* ADD_SIZE data */
        if (adv == 0 || pos + adv > hit + cap)
            return r;
        pos += adv;
    }
    return r;
}

/* ---- PST/OST: !BDN + version byte + NDB structure (corpus FilePSTAnalyser,
 * validator FUN_180041eb0: version@+0x0A 0x0E=ANSI size@+0xA8, 0x17=Unicode
 * size@+0xB8, floor 1 MiB; hardened with the wMagicClient gate and an
 * allocation-map readability probe - PST carries Outlook mail) ---- */
CarveResult validate_pst(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 hdr[0x100];
    if (!rd(s, hit, hdr, 0x100))
        return r;
    if (std::memcmp(hdr, "!BDN", 4) != 0)
        return r;
    const u16 magic_client = u16(hdr[4]) | (u16(hdr[5]) << 8);
    if (magic_client != 0x534D) /* "MS" */
        return r;
    const u8 version = hdr[0x0A];
    u64 size = 0;
    if (version == 0x0E) {
        size = rd32(s, hit + 0xA8); /* ANSI PST: u32 size */
    } else if (version == 0x17) {
        size = rd32(s, hit + 0xB8); /* Unicode PST/OST */
    } else {
        /* CARVE-4 (AUDIT-PIPELINE, F-A3-5): og FUN_180041eb0 ACCEPTS
         * unknown generations at the 1 MiB floor (returns 1 with the
         * size staying 0x100000) - the old reject had the og's polarity
         * inverted. Future/unknown PST generations still surface. */
        size = 1ull << 20;
    }
    if (size < (64ull << 10))
        size = 1ull << 20; /* damaged header: corpus 1 MiB floor */
    if (size > cap || hit + size > s->size())
        size = s->size() > hit ? s->size() - hit : 0;
    /* structural probe: the first allocation-map page (0x1000) must be
     * readable and non-trivial (all-zero AMaps do not occur on real PSTs) */
    u8 amap[512];
    if (!rd(s, hit + 0x1000, amap, sizeof(amap)))
        return r;
    bool nonzero = false;
    for (u8 b : amap)
        if (b) {
            nonzero = true;
            break;
        }
    if (!nonzero)
        return r;
    r.ok = true;
    r.end = hit + size;
    r.confidence = 850;
    r.note = "!BDN+NDB+AMAP";
    return r;
}

/* ---- RAR5: signature + vint header walk to ENDARC (the og validators
 * reject RAR5 entirely - clean-room improvement) ---- */
namespace {
bool rar5_vint(IStorageDevice* s, u64 at, u64& value, u64& consumed) {
    value = 0;
    consumed = 0;
    for (u32 i = 0; i < 10; ++i) {
        u8 b = 0;
        if (!rd(s, at + i, &b, 1))
            return false;
        value |= u64(b & 0x7F) << (7 * i);
        ++consumed;
        if ((b & 0x80) == 0)
            return true;
    }
    return false;
}
} // namespace

CarveResult validate_rar5(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 sig[8];
    if (!rd(s, hit, sig, 8))
        return r;
    const u8 want[8] = {'R', 'a', 'r', '!', 0x1A, 0x07, 0x01, 0x00};
    if (std::memcmp(sig, want, 8) != 0)
        return r;
    /* RAR5 block: crc32(4) | hdr_size vint | header (hdr_size bytes:
     * type, flags, [extra-size][data-size], type fields, extra area) |
     * data area. Flags: 0x0001 extra area, 0x0002 data area. */
    u64 pos = hit + 8;
    u32 blocks = 0;
    u64 files = 0;
    bool ended = false;
    while (pos < hit + cap && blocks < 100000) {
        u64 v = 0, used = 0;
        u64 p2 = pos + 4; /* crc32 */
        if (!rar5_vint(s, p2, v, used))
            break;
        const u64 vlen = used;
        const u64 hdr_size = v;
        p2 += used;
        u64 hdr_type = 0;
        if (!rar5_vint(s, p2, hdr_type, used))
            break;
        p2 += used;
        u64 flags = 0;
        if (!rar5_vint(s, p2, flags, used))
            break;
        p2 += used;
        u64 data_size = 0;
        if ((flags & 0x0001) != 0) { /* extra area size field */
            if (!rar5_vint(s, p2, v, used))
                break;
            p2 += used; /* extra content lives inside hdr_size */
        }
        if ((flags & 0x0002) != 0) { /* data area size field */
            if (!rar5_vint(s, p2, data_size, used))
                break;
            p2 += used;
        }
        if (hdr_type == 2)
            ++files;         /* file header */
        if (hdr_type == 5) { /* ENDARC */
            ended = true;
            pos = pos + 4 + vlen + hdr_size;
            break;
        }
        if (hdr_size == 0)
            break; /* degenerate */
        pos = pos + 4 + vlen + hdr_size + data_size;
        ++blocks;
    }
    if (!ended && files == 0)
        return r;
    r.ok = true;
    r.end = pos > hit + cap ? hit + cap : pos;
    if (r.end <= hit)
        r.end = hit + 128;
    r.confidence = ended ? 880 : 700;
    r.note = ended ? "sig+header-walk+ENDARC" : "sig+file-headers";
    return r;
}

/* ---- TIFF-family extensions ---- */

/* DNG: TIFF magic + DNGVersion tag (0xC612) present in IFD0 */
CarveResult validate_dng(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r = tiff_common(s, hit, cap, false);
    if (!r.ok)
        return r;
    const u8 bo[2] = {0, 0};
    if (!rd(s, hit, const_cast<u8*>(bo), 2))
        return r;
    const bool le = bo[0] == 'I';
    u32 ifd = le ? rd32(s, hit + 4) : rdbe32(s, hit + 4);
    if (ifd == 0 || ifd > 4096)
        return r;
    const u16 n = le ? rd16(s, hit + ifd) : rdbe16(s, hit + ifd);
    if (n == 0 || n > 512)
        return r;
    for (u32 e = 0; e < n; ++e) {
        const u64 ep = hit + ifd + 2 + u64(e) * 12;
        const u16 tag = le ? rd16(s, ep) : rdbe16(s, ep);
        if (tag == 0xC612) { /* DNGVersion */
            r.note = "TIFF+DNGVersion";
            r.confidence = 880;
            return r;
        }
    }
    r.ok = false;
    return r;
}

/* ORF (Olympus): IIRO magic + TIFF-like structure */
CarveResult validate_orf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r = tiff_common(s, hit, cap, false);
    if (r.ok) {
        r.note = "IIRO";
        r.confidence = 800;
    }
    return r;
}

/* RW2 (Panasonic): IIU\0 magic + IFD walk */
CarveResult validate_rw2(IStorageDevice* s, u64 hit, u64 cap) {
    /* N-053: the og binds IIU\0 to the TIFF chain (census :53) - that is
     * FMT_TIFF_EP's job now; this row fires only after the TIFF/EP grammar
     * REJECTS. Keep the TIFF grammar first (an IIU file that somehow
     * reaches this row and validates as TIFF still carries the note), and
     * add the IIU-salvage fallback: a Panasonic-shaped header (IIU + a
     * plausible IFD offset) whose IFD walk failed. Registered superset
     * (D-63): the og carves nothing here - we salvage corrupt TIFF/EPs. */
    CarveResult r = tiff_common(s, hit, cap, false);
    if (r.ok) {
        r.note = "IIU";
        r.confidence = 800;
        return r;
    }
    const u16 ifd_off = u16(rd32(s, hit + 4) & 0xFFFF);
    if (ifd_off != 0) { /* u16 by construction: always < 0x10000 */
        r.ok = true;
        r.end = hit + 4096;
        if (r.end > hit + cap)
            r.end = hit + cap;
        r.confidence = 600;
        r.note = "IIU-salvage (TIFF/EP rejected; Panasonic RAW fallback)";
    }
    return r;
}

/* RAF (Fuji): FUJIFILMCCD-RAW + version + section offsets */
CarveResult validate_raf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[32];
    if (!rd(s, hit, h, 32))
        return r;
    /* N-054: the og keys RAF on the 4-byte FUJI prefix (census :55) and
     * the camera-RAW chain validates further. The 16-byte string is the
     * PRIMARY path; a variant header (bytes 4..15 differ - older/variant
     * RAF writers) is accepted when the version + section table are
     * sane. The registry key is FUJI so both paths are reachable. */
    const bool canonical = std::memcmp(h, "FUJIFILMCCD-RAW ", 16) == 0;
    if (!canonical && std::memcmp(h, "FUJI", 4) != 0)
        return r; /* the registry guarantees FUJI; belt-and-braces */
    /* N-076 (NEW-FINDING, DFX12 session-12 - discovered during N-054's
     * fixture work): real-world RAF files carry the version at +16 as a
     * 4-byte ASCII string ("0100"/"0201"; exiftool FujiFilm tags, the
     * kladd/raw FormatVersion[4] header struct, freedesktop libopenraw)
     * - read LE those are 0x3031..-style values far above the old
     * binary cap, so this gate rejected every real RAF and the row was
     * dead for real files since introduction (zero fixture coverage).
     * Canonical acceptance: 4 ASCII digits OR a small binary value. */
    const u32 ver = rd32(s, hit + 16);
    const bool ascii_ver = h[16] >= '0' && h[16] <= '9' && h[17] >= '0' && h[17] <= '9' &&
                           h[18] >= '0' && h[18] <= '9' && h[19] >= '0' && h[19] <= '9';
    if (ver == 0 || (!ascii_ver && ver > 0x2000))
        return r;
    u64 max_end = 0;
    for (int i = 0; i < 4; ++i) { /* section offset/len pairs from +0x54 */
        const u64 off = rd32(s, hit + 0x54 + u64(i) * 8);
        const u64 len = rd32(s, hit + 0x54 + u64(i) * 8 + 4);
        if (off + len > max_end && off < (64ull << 20))
            max_end = off + len;
    }
    if (max_end == 0)
        max_end = 4096;
    r.ok = true;
    r.end = hit + (max_end < 512 ? 512 : max_end);
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = canonical ? 850 : 700; /* N-054: variant headers carry
                                           * lower confidence, still carved */
    r.note = canonical ? "FUJIFILMCCD-RAW" : "FUJI variant header (og 4-byte key)";
    return r;
}

/* ---- Adobe PSD: 8BPS + version 1 + geometry ---- */
CarveResult validate_psd(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[26];
    if (!rd(s, hit, h, 26))
        return r;
    if (std::memcmp(h, "8BPS", 4) != 0)
        return r;
    const u16 ver = (u16(h[4]) << 8) | u16(h[5]); /* BE */
    if (ver != 1)
        return r;
    const u16 channels = (u16(h[12]) << 8) | u16(h[13]);
    if (channels == 0 || channels > 56)
        return r;
    const u32 height = (u32(h[14]) << 24) | (u32(h[15]) << 16) | (u32(h[16]) << 8) | u32(h[17]);
    const u32 width = (u32(h[18]) << 24) | (u32(h[19]) << 16) | (u32(h[20]) << 8) | u32(h[21]);
    if (height == 0 || width == 0 || height > 300000 || width > 300000)
        return r;
    const u16 depth = (u16(h[22]) << 8) | u16(h[23]);
    if (depth != 1 && depth != 8 && depth != 16 && depth != 32)
        return r;
    r.ok = true;
    /* layer+image sections follow: size unknown without full parse; use
     * the color-mode + image-resource + layer + image length fields */
    u64 end = hit + 26;
    for (int i = 0; i < 3; ++i) { /* color mode len, image res len, layer len */
        const u64 len = rdbe32(s, end);
        if (len == 0xFFFFFFFFULL)
            break;
        end += 4 + len;
    }
    end += 2; /* compression */
    r.end = end > hit + cap ? hit + cap : end;
    if (r.end <= hit + 64)
        r.end = hit + 64;
    r.confidence = 870;
    r.note = "8BPS+v1+geometry";
    return r;
}

/* ---- MP3: ID3v2 + strict MPEG frame walk ----
 * AUDIT Wave-3: the fabricated 32 MiB ceiling is gone. The extent is the
 * end of the last VALID frame (walk stops at the first broken header);
 * minimum 2 frames; every frame must lie fully inside the slice. */
namespace {
/* bitrate tables (kbps), index 1..14 - 0 = free (rejected: needs 2-frame
 * lookback parsing), 15 = reserved */
const u16 kBrV1L1[] = {32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448};
const u16 kBrV1L2[] = {32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384};
const u16 kBrV1L3[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
const u16 kBrV2L1[] = {32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256};
const u16 kBrV2L23[] = {8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};

/* frame size in bytes from a 4-byte header; 0 = invalid */
u32 mp3_frame_size(const u8* f) {
    if (f[0] != 0xFF || (f[1] & 0xE0) != 0xE0)
        return 0;                     /* 11-bit sync */
    const u8 ver = (f[1] >> 3) & 3;   /* 0=MPEG2.5 1=reserved 2=MPEG2 3=MPEG1 */
    const u8 layer = (f[1] >> 1) & 3; /* 0=reserved 1=III 2=II 3=I */
    if (ver == 1 || layer == 0)
        return 0;
    const u8 bri = f[2] >> 4;
    const u8 sri = (f[2] >> 2) & 3;
    if (bri == 0 || bri > 14 || sri == 3)
        return 0; /* free / reserved samplerate */
    u32 sr;
    switch (ver) {
    case 3:
        sr = (sri == 0) ? 44100 : (sri == 1 ? 48000 : 32000);
        break; /* MPEG1 */
    case 2:
        sr = (sri == 0) ? 22050 : (sri == 1 ? 24000 : 16000);
        break; /* MPEG2 */
    default:
        sr = (sri == 0) ? 11025 : (sri == 1 ? 12000 : 8000);
        break; /* MPEG2.5 */
    }
    const u16* table;
    if (ver == 3)
        table = (layer == 3) ? kBrV1L1 : (layer == 2 ? kBrV1L2 : kBrV1L3);
    else
        table = (layer == 3) ? kBrV2L1 : kBrV2L23;
    const u32 kbps = table[bri - 1];
    const u32 pad = (f[2] >> 1) & 1;
    u32 size;
    if (layer == 3) {
        /* Layer I: 12 samples/frame, 4-byte slots */
        size = (12u * kbps * 1000u / sr + pad) * 4u;
    } else if (layer == 2) {
        /* Layer II (all versions): 1152 samples/frame */
        size = 144u * kbps * 1000u / sr + pad;
    } else {
        /* Layer III: 1152 samples @MPEG1, 576 @MPEG2/2.5 */
        size = (ver == 3 ? 144u : 72u) * kbps * 1000u / sr + pad;
    }
    /* real-world frame bounds: 8 kbps @ 8 kHz MPEG2.5 L3 ~ 72 B floor;
     * 320 kbps @ 32 kHz MPEG1 L3 ~ 1440 B ceiling class */
    if (size < 24 || size > 4096)
        return 0;
    return size;
}
} // namespace

CarveResult validate_mp3(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[10];
    if (!rd(s, hit, h, 10))
        return r;
    if (std::memcmp(h, "ID3", 3) != 0)
        return r;
    const u8 ver_major = h[3];
    /* v2.3 / v2.4 only: v2.2's 3-byte size field has no canonical
     * syncsafe interpretation - mis-parsing it would corrupt the frame
     * start, so it is rejected rather than guessed */
    if (ver_major != 3 && ver_major != 4)
        return r;
    /* syncsafe 28-bit size (excludes the 10-byte header; includes any
     * extended header); a v2.4 footer (flag 0x10) adds 10 more bytes */
    const u32 size = (u32(h[6]) << 21) | (u32(h[7]) << 14) | (u32(h[8]) << 7) | u32(h[9]);
    const bool footer = (ver_major == 4) && (h[5] & 0x10) != 0;
    const u64 audio_at = hit + 10 + size + (footer ? 10 : 0);

    /* frame walk: 1 MiB block reads (P1-12 - per-frame 4-byte reads would
     * re-create the amplification); stop at the first invalid header; a
     * frame counts only when it lies fully inside the slice */
    const u64 limit = probe_limit(s, hit, cap);
    if (audio_at + 4 > limit)
        return r;
    std::vector<u8> buf(kProbeBlock);
    u64 fill_at = 0;  /* absolute offset of buf[0] */
    u32 fill_len = 0; /* valid bytes from fill_at */
    auto ensure4 = [&](u64 q) -> const u8* {
        if (q < fill_at || q + 4 > fill_at + fill_len) {
            const u64 remain = limit - q;
            const u32 take = u32(std::min<u64>(remain, kProbeBlock));
            if (take < 4)
                return nullptr;
            if (!rd(s, q, buf.data(), take))
                return nullptr;
            fill_at = q;
            fill_len = take;
        }
        return buf.data() + (q - fill_at);
    };
    u64 q = audio_at;
    u32 nframes = 0;
    u64 frames_end = 0;
    while (q + 4 <= limit) {
        const u8* fh = ensure4(q);
        if (!fh)
            break;
        const u32 fsize = mp3_frame_size(fh);
        if (fsize == 0)
            break; /* first invalid header ends the stream */
        if (q + fsize > limit)
            break; /* frame not fully present in the slice */
        q += fsize;
        ++nframes;
        frames_end = q;
    }
    if (nframes < 2)
        return r; /* AUDIT Wave-3: minimum 2 frames */
    r.ok = true;
    r.end = frames_end; /* <= limit by construction */
    r.confidence = 820;
    r.note = "ID3+frame-walk";
    return r;
}

/* ---- WAV: RIFF + WAVE + fmt chunk ---- */
CarveResult validate_wav(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[16];
    if (!rd(s, hit, h, 16))
        return r;
    if (std::memcmp(h, "RIFF", 4) != 0 || std::memcmp(h + 8, "WAVE", 4) != 0)
        return r;
    const u32 riff_len = (u32(h[4]) | (u32(h[5]) << 8) | (u32(h[6]) << 16) | (u32(h[7]) << 24));
    if (riff_len < 36)
        return r;
    if (std::memcmp(h + 12, "fmt ", 4) != 0)
        return r;
    r.ok = true;
    r.end = hit + 8 + riff_len;
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = 900;
    r.note = "RIFF+WAVE+fmt";
    return r;
}

/* ---- M4A: ftyp + M4A brand ---- */
CarveResult validate_m4a(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    static const char* brands[] = {"M4A ", "M4B ", "M4P "};
    for (const char* brand : brands) {
        CarveResult t = bmff_walk(s, hit, cap, &brand, 1);
        if (t.ok) {
            r = t;
            r.confidence = 800;
            r.note = "ftyp+M4A";
            return r;
        }
    }
    return r;
}

/* ---- ASF/WMV: ASF header object GUID ---- */
CarveResult validate_asf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    static const u8 asf_guid[16] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                    0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    u8 g[16];
    if (!rd(s, hit, g, 16))
        return r;
    if (std::memcmp(g, asf_guid, 16) != 0)
        return r;
    const u64 obj_size = rd64(s, hit + 16);
    if (obj_size < 30 || obj_size > (2ull << 30))
        return r;
    const u32 count = rd32(s, hit + 24); /* header object count */
    if (count == 0 || count > 128)
        return r;
    r.ok = true;
    r.end = hit + obj_size;
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = 850;
    r.note = "ASF-GUID+object";
    return r;
}

/* ---- FLV: signature + version + tag walk ---- */
CarveResult validate_flv(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[13];
    if (!rd(s, hit, h, 13))
        return r;
    if (std::memcmp(h, "FLV", 3) != 0)
        return r;
    if (h[3] == 0 || h[3] > 2)
        return r;
    const u8 flags = h[4];
    if ((flags & 0xFA) != 0)
        return r; /* only audio(4)/video(1) bits legal */
    const u32 offset =
        (u32(h[5]) << 24) | (u32(h[6]) << 16) | (u32(h[7]) << 8) | u32(h[8]); /* BE DataOffset */
    if (offset != 9 && offset != 0)
        return r;
    r.ok = true;
    /* walk tags: PreviousTagSize0(4) + tags {type(1)+size(3 BE)+ts+...} */
    u64 pos = hit + 13 + 4;
    int tags = 0;
    while (pos + 11 < hit + cap && tags < 65536) {
        u8 th[11];
        if (!rd(s, pos, th, 11))
            break;
        const u32 tsize = (u32(th[1]) << 16) | (u32(th[2]) << 8) | u32(th[3]);
        if (th[0] == 0 || th[0] > 9)
            break; /* unknown tag type: end */
        pos += 11 + tsize + 4;
        ++tags;
    }
    r.end = pos > hit + cap ? hit + cap : pos;
    if (r.end <= hit + 64)
        r.end = hit + 64;
    r.confidence = 820;
    r.note = "FLV+tag-walk";
    return r;
}

/* ---- CAB: MSCF + header fields ---- */
CarveResult validate_cab(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[36];
    if (!rd(s, hit, h, 36))
        return r;
    /* N-069: the og registers TWO magic keys for this case-insensitive
     * format - "MSCF" and "mscf" (registry census :94, duplicate-key
     * policy :127). The validator serves both rows, so it re-accepts
     * exactly the two registered spellings (mixed case never anchors -
     * no registry row keys it). */
    if (std::memcmp(h, "MSCF", 4) != 0 && std::memcmp(h, "mscf", 4) != 0)
        return r;
    const u32 cbCab = rd32(s, hit + 8);
    const u16 cFolders = u16(h[26]) | (u16(h[27]) << 8);
    const u16 cFiles = u16(h[28]) | (u16(h[29]) << 8);
    if (cFolders == 0 || cFolders > 1000 || cFiles == 0 || cFiles > 60000)
        return r;
    if (cbCab < 36 || cbCab > (2ull << 30))
        return r;
    r.ok = true;
    r.end = hit + cbCab;
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = 870;
    r.note = "MSCF+counts";
    return r;
}

/* ---- ISO9660: CD001 at +0x8001 (primary volume descriptor) ---- */
CarveResult validate_iso(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* hit = ISO start; the primary volume descriptor sits 32 KiB in
     * (sector 16), "CD001" at hit+0x8001 (= header_offset) */
    u8 pvd[2048];
    if (!rd(s, hit + 0x8000, pvd, 2048))
        return r;
    if (pvd[0] != 1) /* type 1 = primary */
        return r;
    if (std::memcmp(pvd + 1, "CD001", 5) != 0)
        return r;
    /* volume space size: LE u32 at PVD+80, in 2048-byte blocks */
    const u32 blocks = rd32(s, hit + 0x8000 + 80);
    if (blocks == 0 || blocks > (1ull << 32 >> 11))
        return r;
    r.ok = true;
    r.end = hit + u64(blocks) * 2048;
    if (r.end > hit + cap)
        r.end = hit + cap;
    r.confidence = 900;
    r.note = "CD001+volume-size";
    return r;
}

/* ---- S-007 (diff-to-fix): the og's hardcoded common-format set ---------- */
/*
 * Corpus: RAWSearchDLL_decompiled.c registrations - GIF8 u32 0x38464947
 * (:2009), BM u16 0x4d42 (:2898), headerless MP3 MPEG-sync family
 * FF E2..FF E7 / FF F2..FF F7 / FF FA..FF FF (u16 keys :2565-2721), AAC
 * ADTS FF F1 / FF F9 (:2761/:2775, the 0xf9ff key decompiler-rendered as
 * decimal 63999), RTF u32 0x74725c7b (:2087), OLE D0CF11E0 (:2341), MPEG
 * 00 00 01 B3 / 00 00 01 BA (:1843/:1851), DWG "AC10" u32 0x30314341
 * (:1997). Validators are reimplemented from the format grammar (never
 * pasted from the decompile); the og's BruteForce frame-walkers are the
 * behavioral model for the headerless audio family.
 */

CarveResult validate_gif(IStorageDevice* s, u64 hit, u64 cap) {
    /* GIF87a/GIF89a: Logical Screen Descriptor (+ optional global color
     * table), then block stream (extensions 0x21 with sub-blocks, image
     * descriptors 0x2C with LZW-coded sub-blocks) to the trailer 0x3B.
     * Accept requires at least ONE image descriptor - a bare
     * header+trailer is a degenerate the 4-byte "GIF8" key would accept
     * on pure noise. */
    CarveResult r;
    u8 h[7];
    if (!rd(s, hit, h, 7))
        return r;
    if (std::memcmp(h, "GIF8", 4) != 0)
        return r;
    if (std::memcmp(h + 4, "7a", 2) != 0 && std::memcmp(h + 4, "9a", 2) != 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit + 6;
    u8 lsd[7];
    if (!rd(s, q, lsd, 7))
        return r;
    if (lsd[4] & 0x80)
        q += u64(3) << ((lsd[4] & 7) + 1); /* global color table */
    q += 7;
    bool saw_image = false;
    u64 images_end = 0;
    /* sub-block walker: returns false on slice break */
    auto skip_subblocks = [&](u64& p) -> bool {
        for (;;) {
            u8 bl = 0;
            if (!rd(s, p, &bl, 1) || p + 1 > limit)
                return false;
            ++p;
            if (bl == 0)
                return true;
            if (p + bl > limit)
                return false;
            p += bl;
        }
    };
    for (int guard = 0; guard < 65536; ++guard) {
        u8 b = 0;
        if (!rd(s, q, &b, 1) || q >= limit)
            break;
        if (b == 0x3B) { /* trailer */
            if (!saw_image)
                return r;
            r.ok = true;
            r.end = q + 1;
            r.confidence = 840;
            r.note = "GIF-blocks-to-trailer";
            return r;
        }
        if (b == 0x21) { /* extension: label + sub-blocks */
            q += 2;
            if (!skip_subblocks(q))
                break;
            continue;
        }
        if (b == 0x2C) { /* image descriptor */
            u8 idesc[10];
            if (!rd(s, q + 1, idesc, 9))
                break;
            q += 10;
            if (idesc[8] & 0x80)
                q += u64(3) << ((idesc[8] & 7) + 1); /* local color table */
            u8 lzw_min = 0;
            if (!rd(s, q, &lzw_min, 1) || lzw_min == 0 || lzw_min > 11)
                break;
            ++q;
            if (!skip_subblocks(q))
                break;
            saw_image = true;
            images_end = q;
            continue;
        }
        break; /* unknown block: grammar break */
    }
    if (saw_image && images_end > hit) {
        /* truncated before the trailer: salvage the walked extent (the
         * og's BruteForce school accepts partial frames the same way) */
        r.ok = true;
        r.end = std::min(images_end, limit);
        r.confidence = 600;
        r.note = "GIF-truncated-after-images";
        return r;
    }
    return r;
}

CarveResult validate_bmp(IStorageDevice* s, u64 hit, u64 cap) {
    /* BITMAPFILEHEADER (14) + DIB header. Extent = bfOffBits + the
     * computed image size for the common uncompressed forms (S-007 fix
     * plan); floor 512 per the plan. The 2-byte "BM" key matches noise
     * constantly - the DIB gates are the discriminator. */
    CarveResult r;
    u8 fh[18];
    if (!rd(s, hit, fh, 18))
        return r;
    if (fh[0] != 'B' || fh[1] != 'M')
        return r;
    const u32 bf_size = u32_from_bytes_le(fh + 2);
    const u32 off_bits = u32_from_bytes_le(fh + 10);
    const u32 dib = u32_from_bytes_le(fh + 14);
    if (dib != 12 && dib != 16 && dib != 40 && dib != 52 && dib != 56 && dib != 64 && dib != 68 &&
        dib != 108 && dib != 124)
        return r; /* known DIB sizes only (S-007: 12..124 family) */
    if (off_bits < 14 + dib || off_bits > 512ull << 20)
        return r;
    u64 image_bytes = 0;
    if (dib == 12) {                       /* BITMAPCOREHEADER */
        const u32 w = fh[18 - 18 + 4 + 0]; /* read fresh below instead */
        (void)w;
        u8 core[10];
        if (!rd(s, hit + 18, core, 10))
            return r;
        const u32 width = core[0] | (u32(core[1]) << 8);
        const u32 height = core[2] | (u32(core[3]) << 8);
        const u32 bpp = core[8] | (u32(core[9]) << 8);
        if (width == 0 || height == 0 || bpp == 0 || bpp > 24)
            return r;
        image_bytes = (u64(height) * ((u64(width) * bpp + 31) / 32 * 4));
    } else {
        u8 info[40];
        if (!rd(s, hit + 14, info, 40))
            return r;
        const i32 width = i32(u32_from_bytes_le(info + 4));
        const i32 height = i32(u32_from_bytes_le(info + 8));
        const u32 bpp = u32_from_bytes_le(info + 14) & 0xFFFF;
        const u32 compression = u32_from_bytes_le(info + 16);
        if (width <= 0 || height == 0 || bpp == 0 || bpp > 32)
            return r;
        if (compression == 0) { /* BI_RGB */
            image_bytes =
                u64(u32(height < 0 ? -height : height)) * ((u64(u32(width)) * bpp + 31) / 32 * 4);
        } else if (compression == 3 && (bpp == 16 || bpp == 32)) { /* BITFIELDS */
            image_bytes = u64(u32(height < 0 ? -height : height)) * u64(u32(width)) * (bpp / 8);
        } else {
            /* compressed / unknown: trust the declared bfSize when sane */
            if (bf_size > off_bits && bf_size <= 512ull << 20)
                image_bytes = bf_size - off_bits;
            else
                return r; /* no honest extent */
        }
    }
    const u64 floor = 512;
    u64 end = hit + off_bits + image_bytes;
    if (end - hit < floor)
        end = hit + floor;
    const u64 limit = probe_limit(s, hit, cap);
    if (end > limit)
        end = limit; /* slab-truncated extent */
    if (end - hit < 64)
        return r;
    r.ok = true;
    r.end = end;
    r.confidence = 780;
    r.note = "BM+DIB+offBits-extent";
    return r;
}

CarveResult validate_mp3_raw(IStorageDevice* s, u64 hit, u64 cap) {
    /* S-007: headerless MPEG audio - the og's BruteForce frame walkers
     * (:2565-2721 registrations; the 2-consecutive-frame grammar
     * :37152-92 is the og gate). The core's scan is byte-granular (the
     * og probes sector-aligned only, I-009), so the frame gate is raised
     * to 10 to hold the false-positive rate at the finer grid (the
     * finding's fix plan). */
    CarveResult r;
    const u64 limit = probe_limit(s, hit, cap);
    std::vector<u8> buf(kProbeBlock);
    u64 fill_at = 0;
    u32 fill_len = 0;
    auto ensure4 = [&](u64 q) -> const u8* {
        if (q < fill_at || q + 4 > fill_at + fill_len) {
            const u64 remain = limit - q;
            const u32 take = u32(std::min<u64>(remain, kProbeBlock));
            if (take < 4)
                return nullptr;
            if (!rd(s, q, buf.data(), take))
                return nullptr;
            fill_at = q;
            fill_len = take;
        }
        return buf.data() + (q - fill_at);
    };
    u64 q = hit;
    u32 nframes = 0;
    u64 frames_end = 0;
    while (q + 4 <= limit) {
        const u8* fh = ensure4(q);
        if (!fh)
            break;
        const u32 fsize = mp3_frame_size(fh);
        if (fsize == 0)
            break;
        if (q + fsize > limit)
            break;
        q += fsize;
        ++nframes;
        frames_end = q;
        if (nframes > (1u << 20))
            break; /* bounded walk */
    }
    if (nframes < 10)
        return r; /* headerless gate: 10 consecutive frames (S-007) */
    r.ok = true;
    r.end = frames_end;
    r.confidence = 700;
    r.note = "mpeg-audio-10-frame-sync";
    return r;
}

CarveResult validate_aac_adts(IStorageDevice* s, u64 hit, u64 cap) {
    /* ADTS: 12-bit sync (FFF), layer 00, profile != reserved(3),
     * sampling index != 15; 13-bit frame length in the fixed header;
     * 10-frame gate (same byte-granular rationale as validate_mp3_raw). */
    CarveResult r;
    u8 h[7];
    if (!rd(s, hit, h, 7))
        return r;
    if (h[0] != 0xFF || (h[1] & 0xF0) != 0xF0)
        return r;
    if (((h[1] >> 1) & 3) != 0)
        return r; /* layer must be 00 */
    const u8 profile = (h[2] >> 6) & 3;
    const u8 sr_idx = (h[2] >> 2) & 0xF;
    const u8 chan = (u8((h[2] & 1) << 2)) | u8(h[3] >> 6); /* 3 bits: b2[0] + b3[7:6] */
    if (profile == 3 || sr_idx == 15 || chan > 7)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    std::vector<u8> buf(kProbeBlock);
    u64 fill_at = 0;
    u32 fill_len = 0;
    auto ensure7 = [&](u64 q) -> const u8* {
        if (q < fill_at || q + 7 > fill_at + fill_len) {
            const u64 remain = limit - q;
            const u32 take = u32(std::min<u64>(remain, kProbeBlock));
            if (take < 7)
                return nullptr;
            if (!rd(s, q, buf.data(), take))
                return nullptr;
            fill_at = q;
            fill_len = take;
        }
        return buf.data() + (q - fill_at);
    };
    u64 q = hit;
    u32 nframes = 0;
    u64 frames_end = 0;
    while (q + 7 <= limit) {
        const u8* fh = ensure7(q);
        if (!fh)
            break;
        if (fh[0] != 0xFF || (fh[1] & 0xF0) != 0xF0 || ((fh[1] >> 1) & 3) != 0)
            break;
        const u32 flen = (u32(fh[3] & 3) << 11) | (u32(fh[4]) << 3) | (u32(fh[5]) >> 5);
        if (flen < 7 || flen > 8192)
            break;
        if (q + flen > limit)
            break;
        q += flen;
        ++nframes;
        frames_end = q;
        if (nframes > (1u << 20))
            break;
    }
    if (nframes < 10)
        return r;
    r.ok = true;
    r.end = frames_end;
    r.confidence = 700;
    r.note = "adts-10-frame-sync";
    return r;
}

CarveResult validate_rtf(IStorageDevice* s, u64 hit, u64 cap) {
    /* "{\rtf" + brace-group walk to the matching close. Backslash escapes
     * the following character (\{ \} are literals); the walk accepts a
     * truncated stream (never-closed groups) at degraded confidence -
     * the same partial-salvage stance as the MP3 frame walker. */
    CarveResult r;
    u8 h[6];
    if (!rd(s, hit, h, 6))
        return r;
    if (std::memcmp(h, "{\\rtf", 5) != 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 depth = 1; /* the opening group */
    std::vector<u8> buf(kProbeBlock);
    u64 p = hit + 5;
    u64 close_at = 0;
    while (p < limit) {
        const u64 remain = limit - p;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take == 0)
            break;
        if (!rd(s, p, buf.data(), take))
            break;
        u64 i = 0;
        for (; i < take; ++i) {
            const u8 c = buf[i];
            if (c == '\\') {
                ++i; /* escaped char: skip it (\'hh, \\, \{, \}) */
                continue;
            }
            if (c == '{')
                ++depth;
            else if (c == '}') {
                --depth;
                if (depth == 0) {
                    close_at = p + i + 1;
                    break;
                }
                if (depth == 0)
                    break;
            }
        }
        if (close_at != 0)
            break;
        if (i < take)
            break; /* closed inside this block - handled above */
        p += take;
    }
    if (close_at != 0 && depth == 0) {
        r.ok = true;
        r.end = close_at;
        r.confidence = 800;
        r.note = "rtf-brace-close";
        return r;
    }
    if (depth >= 1 && p >= limit) { /* truncated groups: salvage the walk */
        r.ok = true;
        r.end = limit;
        r.confidence = 600;
        r.note = "rtf-truncated-groups";
        return r;
    }
    return r;
}

CarveResult validate_ole(IStorageDevice* s, u64 hit, u64 cap) {
    /* Compound File (legacy .doc/.xls/.ppt + MSI): D0CF11E0A1B11AE1,
     * major version 3/4, sector shift 9/12, byte order 0xFFFE. Extent =
     * the last sector referenced by the DIFAT FAT-sector array and the
     * root directory chain, x sector size (S-007 fix plan). */
    CarveResult r;

    u8 h[80];
    if (!rd(s, hit, h, 80))
        return r;
    static const u8 kMagic[8] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
    if (std::memcmp(h, kMagic, 8) != 0)
        return r;
    const u16 minor = u16_from_bytes_le(h + 24);
    const u16 major = u16_from_bytes_le(h + 26);
    const u16 order = u16_from_bytes_le(h + 28);
    const u16 shift = u16_from_bytes_le(h + 30);
    const u16 mini_shift = u16_from_bytes_le(h + 32);
    (void)minor;
    if (major != 3 && major != 4)
        return r;
    if (order != 0xFFFE)
        return r;
    if ((shift != 9 && shift != 12) || mini_shift != 6)
        return r;
    const u64 ssz = u64(1) << shift;
    const u32 num_fat = u32_from_bytes_le(h + 44);
    const u32 first_dir = u32_from_bytes_le(h + 48);
    const u32 num_difat = u32_from_bytes_le(h + 72);
    if (num_fat == 0 || num_fat > (1u << 20))
        return r;
    if (first_dir == 0xFFFFFFFF || first_dir == 0xFFFFFFFE)
        return r;
    if (num_difat > (1u << 16))
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 max_sector = first_dir;
    /* DIFAT head: 109 sector ids at +76 */
    for (u32 i = 0; i < 109 && i < num_fat; ++i) {
        const u32 fs = u32_from_bytes_le(h + 76 + i * 4);
        if (fs == 0xFFFFFFFF || fs == 0xFFFFFFFE)
            continue;
        if (fs > max_sector)
            max_sector = fs;
    }
    /* the directory chain through the FIRST FAT sector's entries */
    if (first_dir != 0xFFFFFFFF) {
        u32 sector = first_dir;
        for (u32 hop = 0; hop < 4096; ++hop) {
            if (sector > max_sector)
                max_sector = sector;
            if (sector == 0xFFFFFFFE)
                break; /* ENDOFCHAIN */
            /* FAT entry lookup bounded to the header DIFAT head */
            const u32 fat_idx = sector >> (shift - 2);
            if (fat_idx >= 109 || fat_idx >= num_fat)
                break;
            const u32 fat_sector = u32_from_bytes_le(h + 76 + fat_idx * 4);
            if (fat_sector == 0xFFFFFFFF || fat_sector == 0xFFFFFFFE)
                break;
            /* CFB sector N's data lives at (N+1)*ssz: the header occupies
             * the first sector slot (512 bytes at shift 9, 4096 at 12) */
            const u64 ent_at = hit + u64(fat_sector + 1) * ssz + u64(sector) * 4;
            if (ent_at + 4 > limit)
                break;
            const u32 next = rd32(s, ent_at);
            if (next == 0xFFFFFFFF || next == 0xFFFFFFFE)
                break;
            sector = next;
        }
    }
    const u64 end = hit + (max_sector + 2) * ssz; /* +2: header slot + last sector */
    if (end <= hit + 1024)
        return r; /* degenerate: header + nothing referenced */
    if (end > limit) {
        /* slab-truncated: floor at the header + the referenced sectors that
         * fit; never below header + one sector */
        const u64 floored = std::max(hit + 1024, limit);
        r.ok = true;
        r.end = floored;
        r.confidence = 700;
        r.note = "ole-difat-extent-truncated";
        return r;
    }
    r.ok = true;
    r.end = end;
    r.confidence = 850;
    r.note = "ole-difat-extent";
    return r;
}

CarveResult validate_mpegps(IStorageDevice* s, u64 hit, u64 cap) {
    /* MPEG program stream: 00 00 01 BA pack header (MPEG-1 12 bytes /
     * MPEG-2 14 bytes - the SCR marker nibble discriminates), then the
     * start-code packet walk (lengths for packet codes, fixed sizes for
     * pack/system headers) to the first invalid position (S-007 plan,
     * BruteForce-style). */
    CarveResult r;
    u8 h[14];
    if (!rd(s, hit, h, 14))
        return r;
    if (h[0] != 0 || h[1] != 0 || h[2] != 1 || h[3] != 0xBA)
        return r;
    const bool mpeg2 = (h[4] & 0xC4) == 0x44; /* marker bits 01..10 in SCR */
    u64 q = hit + (mpeg2 ? 14 : 12);
    const u64 limit = probe_limit(s, hit, cap);
    u32 packs = 1, elements = 0;
    for (u32 guard = 0; guard < (1u << 20); ++guard) {
        u8 sc[4];
        if (q + 4 > limit || !rd(s, q, sc, 4))
            break;
        if (sc[0] != 0 || sc[1] != 0 || sc[2] != 1)
            break; /* first invalid start code ends the stream */
        const u8 code = sc[3];
        if (code == 0xB9) { /* end code */
            q += 4;
            ++elements;
            break;
        }
        if (code == 0xBA) {
            u8 pk[14];
            if (!rd(s, q, pk, 14))
                break;
            q += (pk[4] & 0xC4) == 0x44 ? 14 : 12;
            ++packs;
            ++elements;
            continue;
        }
        if (code == 0xBB) { /* system header: length follows */
            const u32 len = rd16(s, q + 4);
            if (len < 6 || q + 6 + len > limit)
                break;
            q += 6 + len;
            ++elements;
            continue;
        }
        if ((code & 0xE0) == 0xE0 || (code & 0xC0) == 0xC0 || code == 0xBD || code == 0xBE) {
            /* stream packets + padding: PES length field */
            const u32 len = rd16(s, q + 4);
            if (q + 6 + len > limit)
                break;
            q += 6 + len;
            ++elements;
            continue;
        }
        break; /* unknown start code: stop (conservative end) */
    }
    if (packs < 1 || elements < 2)
        return r; /* a lone pack header is noise-class for the 4-byte key */
    r.ok = true;
    r.end = std::min(q, limit);
    r.confidence = 650;
    r.note = "mpeg-ps-startcode-walk";
    return r;
}

CarveResult validate_mpeges(IStorageDevice* s, u64 hit, u64 cap) {
    /* MPEG video elementary stream anchored on a picture start code
     * (00 00 01 B3): walk picture/slice/group codes to the first break.
     * Accept requires at least one slice after the picture header (the
     * 4-byte key alone is weak). */
    CarveResult r;
    u8 h[10];
    if (!rd(s, hit, h, 10))
        return r;
    if (h[0] != 0 || h[1] != 0 || h[2] != 1 || h[3] != 0xB3)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit + 10; /* temporal reference(2) + code(1) + vbv(4) + ... */
    u32 pictures = 1, slices = 0;
    for (u32 guard = 0; guard < (1u << 20); ++guard) {
        u8 sc[4];
        if (q + 4 > limit || !rd(s, q, sc, 4))
            break;
        if (sc[0] != 0 || sc[1] != 0 || sc[2] != 1)
            break;
        const u8 code = sc[3];
        if (code == 0x00) { /* picture start */
            q += 4 + 8;     /* ref(2)+code(1)+vbv delay(3)+stuffing */
            ++pictures;
            continue;
        }
        if (code >= 0x01 && code <= 0xAF) { /* slice */
            u32 len = 0;
            /* walk the slice bytes to the next start-code prefix */
            u64 p = q + 4;
            std::vector<u8> buf(kProbeBlock);
            while (p + 4 <= limit) {
                const u64 remain = limit - p;
                const u32 take = u32(std::min<u64>(remain, kProbeBlock - 3));
                if (take == 0 || !rd(s, p, buf.data(), take))
                    break;
                u64 i = 0;
                for (; i + 3 < take; ++i) {
                    if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1)
                        break;
                }
                p += i;
                if (i + 3 < take)
                    break; /* found the next start code */
            }
            len = 0;
            (void)len;
            q = p;
            ++slices;
            continue;
        }
        if (code == 0xB8) { /* GOP header: fixed 8 bytes after the code */
            q += 4 + 8;
            ++slices; /* structure element */
            continue;
        }
        break;
    }
    if (pictures < 1 || slices < 1)
        return r;
    r.ok = true;
    r.end = std::min(q, limit);
    r.confidence = 600;
    r.note = "mpeg-es-slice-walk";
    return r;
}

CarveResult validate_dwg(IStorageDevice* s, u64 hit, u64 cap) {
    /* AutoCAD DWG: "AC10xx" version gate over the known release set
     * (r13..R2018), then the bounded extent (the finding's sanctioned
     * heuristic: the R13-R2000 section-locator map is version-dependent
     * and the R2004+ map is compressed - a full section walk is a
     * follow-up; the version gate + bounded extent is the honest floor). */
    CarveResult r;
    u8 v[6];
    if (!rd(s, hit, v, 6))
        return r;
    if (std::memcmp(v, "AC10", 4) != 0)
        return r;
    static const char* kVersions[] = {"12", "14", "15", "18", "21", "24", "27", "30", "32"};
    bool known = false;
    for (const char* ver : kVersions)
        if (v[4] == ver[0] && v[5] == ver[1])
            known = true;
    if (!known)
        return r;
    /* the R13-R2000 sentinel follows the version string: 0D 0A 0D 0A 00 */
    u8 sent[5];
    if (rd(s, hit + 6, sent, 5) && sent[0] == 0x0D && sent[1] == 0x0A && sent[2] == 0x0D &&
        sent[3] == 0x0A && sent[4] == 0x00) {
        known = true; /* strengthens the classification */
    }
    const u64 limit = probe_limit(s, hit, cap);
    u64 end = hit + (64ull << 20); /* the finding's 64 MiB cap */
    if (end > limit)
        end = limit;
    if (end - hit < 512)
        return r;
    r.ok = true;
    r.end = end;
    r.confidence = 620; /* bounded-extent honesty: raw-sign class */
    r.note = "dwg-version-gate,bounded-extent";
    return r;
}

} // namespace carve
} // namespace cr
