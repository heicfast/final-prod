/* cr_analysers.cpp - grammar validators (R4.3, F8: never header-only).
 *
 * Every validator reads through the slice with bounded reads, caps by the
 * entry max_size, and returns ok=false on any grammar break.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_signatures.h"
#include "cr/storage/cr_device.h"
#include <cstring>

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
            /* entropy data: scan for next marker (FF xx non-zero) */
            u64 p = pos + 2 + u64(len);
            u8 buf[512];
            while (p + 2 <= hit + cap) {
                u32 take = 512;
                if (p + take > hit + cap)
                    take = u32(hit + cap - p);
                if (!rd(s, p, buf, take))
                    return r;
                bool found = false;
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
                p += take - 1; /* overlap 1 byte for boundary markers */
            }
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
    /* search for %%EOF from the tail: spec - within the last 1024 bytes of
     * the doc; we probe with binary search over plausible doc sizes using
     * the trailer convention: scan backwards from cap in 1KiB steps */
    u8 buf[1024];
    u64 best = 0;
    for (u64 probe = hit + 1024; probe <= hit + cap; probe += 1024) {
        u64 lo = probe - 1024;
        u32 take = 1024;
        if (lo + take > hit + cap)
            take = u32(hit + cap - lo);
        if (take < 16)
            break;
        if (!rd(s, lo, buf, take))
            break;
        for (int i = int(take) - 5; i >= 0; --i) {
            if (std::memcmp(buf + i, "%%EOF", 5) == 0) {
                best = lo + u64(i) + 5;
            }
        }
        if (best != 0 && probe >= hit + cap)
            break;
        if (best != 0) {
            /* keep scanning: the LAST %%EOF wins (incremental updates) */
        }
    }
    if (best == 0)
        return r;
    r.ok = true;
    r.end = best;
    r.confidence = 900;
    r.note = "%%EOF";
    return r;
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
    /* last PK\x05\x06 within cap wins (incremental updates append EOCDs) */
    u8 buf[1024];
    u64 eocd_pos = 0;
    for (u64 probe = hit + 1024; probe <= hit + cap; probe += 1024) {
        u64 lo = probe - 1024;
        u32 take = 1024;
        if (lo + take > hit + cap)
            take = u32(hit + cap - lo);
        if (take < 22)
            break;
        if (!rd(s, lo, buf, take))
            break;
        for (int i = int(take) - 4; i >= 0; --i)
            if (std::memcmp(buf + i, "PK\x05\x06", 4) == 0)
                eocd_pos = lo + u64(i);
    }
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
    /* central directory must live inside the file, before the EOCD */
    if (u64(cd_offset) + cd_size > (eocd_pos - hit))
        return r;
    if (cd_offset + 30 < hit - hit)
        return r; /* cd_offset is file-relative */
    u64 end = eocd_pos + 22 + comment;
    if (end > hit + cap)
        end = hit + cap;
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

} // namespace carve
} // namespace cr
