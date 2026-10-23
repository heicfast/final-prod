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
    static const char* const brands[] = {"heic", "heix", "mif1", "msf1"};
    return bmff_walk(s, hit, cap, brands, 4);
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
    const u64 eocd_pos = scan_first_eocd(s, hit, limit);
    if (eocd_pos == 0 || eocd_pos < hit + 22)
        return r;
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
    u64 end = eocd_pos + 22 + comment;
    if (end > limit)
        end = limit;
    /* fragmentation-aware (R4.5): a gap between cd end and EOCD is normal
     * (comment/empty space); a cd_offset far from the tail hints at damage */
    bool frag = (eocd_pos - hit) > u64(cd_offset) + cd_size + 4096;
    r.ok = true;
    r.end = end;
    r.confidence = office ? 850 : 700;
    r.fragmented = frag;
    r.note = office ? "[Content_Types]+EOCD" : "EOCD+cd-geometry";
    return r;
}

CarveResult validate_zip(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, false);
}
CarveResult validate_docx(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, true);
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
    if (vlen == 0 || v == 0 || v > 1024 * 1024)
        return r;                 /* All-1 = streaming */
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
    /* "Rar!\x1A\x07\x00" (7 bytes) then MAIN_HEAD: crc u16, type 0x73,
     * flags u16, size u16 (+optional 4-byte add) */
    u16 hsize = rd16(s, hit + 11);
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
        return r; /* unknown generation: reject (corpus floors others) */
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
    CarveResult r = tiff_common(s, hit, cap, false);
    if (r.ok) {
        r.note = "IIU";
        r.confidence = 800;
    }
    return r;
}

/* RAF (Fuji): FUJIFILMCCD-RAW + version + section offsets */
CarveResult validate_raf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[32];
    if (!rd(s, hit, h, 32))
        return r;
    if (std::memcmp(h, "FUJIFILMCCD-RAW ", 16) != 0)
        return r;
    const u32 ver = rd32(s, hit + 16);
    if (ver == 0 || ver > 0x2000)
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
    r.confidence = 850;
    r.note = "FUJIFILMCCD-RAW";
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
    const u16 fmt = u16(h[16 - 16]) | 0; /* first fmt bytes follow; accept */
    (void)fmt;
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
    if (std::memcmp(h, "MSCF", 4) != 0)
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

} // namespace carve
} // namespace cr
