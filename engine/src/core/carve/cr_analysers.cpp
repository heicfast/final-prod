/* cr_analysers.cpp - grammar validators (R4.3, F8: never header-only).
 *
 * Every validator reads through the slice with bounded reads, caps by the
 * entry max_size, and returns ok=false on any grammar break.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_signatures.h"
#include "cr/compress/cr_decomp.h" /* N-115: the mimetype content inflate */
#include "cr/storage/cr_device.h"
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <ctime>
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
u8 rd8(IStorageDevice* s, u64 off) {
    u8 b = 0;
    rd(s, off, &b, 1);
    return b;
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

/* ---- S-049: the content-derived naming-prefix producers ----------------
 *
 * The og's engine-side analysers supply a prefix string that DRWUI's
 * naming layer composes as "<prefix>_<%06d>.<EXT>"
 * (DRWUI_decompiled.c:1878330-81; the prefix-extraction and WxH-dims
 * branches at the caller :1877538-64; the per-format counter map at
 * :1877584-90 — all first-hand this session). The real og sessions pin
 * the per-format prefix families (the verbatim table in
 * evidence/executions/s10b-real-rsf-session-mining.md + this session's
 * first-hand re-mining: JPG_192x144 / GT-S7562 / Adobe Photoshop 7.0,
 * MP4_08m07s / MOV_10s, <ID3 title>, <Title/Creator+date>, <docProps
 * created>, <first CD entry>). */

/* Content bytes -> the u16 prefix. The og's analysers read raw bytes and
 * widen them; ASCII prefixes dominate the real sessions. Try UTF-8
 * first (EXIF MP/ID3v2 UTF-8 strings), fall back to byte promotion
 * (latin-1) so arbitrary tag bytes still surface. */
std::u16string prefix_from_bytes(const char* p, size_t n) {
    if (p == nullptr || n == 0)
        return {};
    /* cheap UTF-8 validity walk */
    bool utf8_ok = true;
    for (size_t i = 0; i < n;) {
        const unsigned char c = static_cast<unsigned char>(p[i]);
        size_t need;
        if (c < 0x80)
            need = 1;
        else if ((c & 0xE0) == 0xC0)
            need = 2;
        else if ((c & 0xF0) == 0xE0)
            need = 3;
        else if ((c & 0xF8) == 0xF0)
            need = 4;
        else {
            utf8_ok = false;
            break;
        }
        if (i + need > n) {
            utf8_ok = false;
            break;
        }
        i += need;
    }
    std::u16string out;
    out.reserve(n);
    if (utf8_ok) {
        for (size_t i = 0; i < n;) {
            const unsigned char c = static_cast<unsigned char>(p[i]);
            if (c < 0x80) {
                out.push_back(u16(c));
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                out.push_back(u16(((u32(c) & 0x1F) << 6) | (u32(p[i + 1]) & 0x3F)));
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                out.push_back(u16(((u32(c) & 0x0F) << 12) | ((u32(p[i + 1]) & 0x3F) << 6) |
                                  (u32(p[i + 2]) & 0x3F)));
                i += 3;
            } else {
                /* a 4-byte sequence outside the BMP: the og's wide string
                 * holds it as the surrogate pair; both halves promote */
                const u32 cp = ((u32(c) & 0x07) << 18) | ((u32(p[i + 1]) & 0x3F) << 12) |
                               ((u32(p[i + 2]) & 0x3F) << 6) | (u32(p[i + 3]) & 0x3F);
                out.push_back(u16(0xD800 + ((cp - 0x10000) >> 10)));
                out.push_back(u16(0xDC00 + ((cp - 0x10000) & 0x3FF)));
                i += 4;
            }
        }
        return out;
    }
    for (size_t i = 0; i < n; ++i)
        out.push_back(u16(u8(p[i])));
    return out;
}

std::u16string prefix_from_bytes(const std::string& s) {
    return prefix_from_bytes(s.data(), s.size());
}

/* The og's date/duration composition forms (real-session-pinned):
 * the date "2020-05-23 23-23-23" (PDF/DOCX - colons are Windows-illegal,
 * the og writes dashes), the MP4/MOV duration "08m07s" / "14s" / "01m"
 * (%02u-padded minutes/seconds). */
std::u16string duration_form(u64 total_seconds) {
    char buf[32];
    const u64 m = total_seconds / 60;
    const u64 sec = total_seconds % 60;
    int n;
    if (m == 0)
        n = snprintf(buf, sizeof(buf), "%02us", unsigned(sec));
    else if (sec == 0)
        n = snprintf(buf, sizeof(buf), "%02um", unsigned(m));
    else
        n = snprintf(buf, sizeof(buf), "%02um%02us", unsigned(m), unsigned(sec));
    if (n <= 0)
        return {};
    return prefix_from_bytes(buf, size_t(n));
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
/* S-049: the PDF naming prefix — real-session-pinned model (first-hand
 * re-mining of all three og sessions this session):
 *   - a PDF with an info-dict /Title prefixes by the Title (truncated at
 *     25 chars: " TeX output 1998.06.28_17" and "LaTeX with hyperref
 *     packa" are both exactly-25 truncations of 29/27-char source
 *     strings — two independent pins);
 *   - otherwise (Creator, else Producer) + the /CreationDate formatted
 *     "YYYY-MM-DD HH-MM-SS" (colons are Windows-illegal; the og writes
 *     dashes): "TeX2022-03-03 21-11-38", "GPL Ghostscript
 *     9.56.12022-08-17 15-05-58", "groff version 1.23.02023-12-19
 *     21-25-45";
 *   - the date parse reads "D:" + at least 14 plain digits (matplotlib's
 *     dashed "D:2022-08-17 15:05:58+02'00'" form never produced a date
 *     prefix in any session — rejected naturally by the 14-digit read);
 *   - string reads cap at 25 chars; a date-only form exists ("2020-05-23
 *     23-23-23" — no Title/Creator/Producer found in the bounded scan).
 * Open nuance recorded in S-049's Resolution: which of Creator/Producer
 * the og prefers when both are present beyond the read bound — the
 * sessions never show a form that separates them (every observed
 * non-Title string matches its file's Creator; gs-distilled files carry
 * the Producer only). Creator-first is the implemented reading. */
static void pdf_name_prefix(IStorageDevice* s, u64 hit, u64 end, CarveResult& r) {
    enum Key { K_TITLE, K_CREATOR, K_PRODUCER, K_DATE };
    static const size_t kStrCap = 25;
    static const u64 kScanCap = 256ull << 10;
    const u64 limit = end - hit > kScanCap ? hit + kScanCap : end;
    std::vector<u8> buf(kProbeBlock);
    std::string title, creator, producer, date;
    u64 block = hit;
    while (block < limit &&
           (title.empty() || (creator.empty() && producer.empty()) || date.empty())) {
        const u64 remain = limit - block;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < 16)
            break;
        if (!rd(s, block, buf.data(), take))
            break;
        /* candidate keys: /Title /Creator /Producer /CreationDate */
        for (u32 i = 0; i + 6 <= take; ++i) {
            if (buf[i] != '/')
                continue;
            Key key;
            size_t klen;
            if (std::memcmp(buf.data() + i, "/Title", 6) == 0) {
                key = K_TITLE;
                klen = 6;
            } else if (std::memcmp(buf.data() + i, "/Creator", 8) == 0 && i + 8 <= take) {
                key = K_CREATOR;
                klen = 8;
            } else if (std::memcmp(buf.data() + i, "/Producer", 9) == 0 && i + 9 <= take) {
                key = K_PRODUCER;
                klen = 9;
            } else if (std::memcmp(buf.data() + i, "/CreationDate", 13) == 0 && i + 13 <= take) {
                key = K_DATE;
                klen = 13;
            } else {
                continue;
            }
            /* the first key occurrence wins (per key); skip the string
             * walk when this key is already filled */
            const std::string* have = key == K_TITLE      ? &title
                                      : key == K_CREATOR  ? &creator
                                      : key == K_PRODUCER ? &producer
                                                          : &date;
            if (!have->empty())
                continue;
            /* optional whitespace then a literal ( string */
            u64 p = block + i + klen;
            u32 ws = 0;
            while (p + ws < limit && ws < 8) {
                u8 c = 0;
                if (!rd(s, p + ws, &c, 1))
                    break;
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == 0) {
                    ++ws;
                    continue;
                }
                break;
            }
            u8 open = 0;
            if (p + ws >= limit || !rd(s, p + ws, &open, 1) || open != '(')
                continue;
            p = p + ws + 1;
            if (key == K_DATE) {
                /* D: + at least 14 plain digits (the dashed matplotlib
                 * "D:2022-08-17 15:05:58+02'00'" form never produced a
                 * date prefix in any session - it fails this read
                 * naturally: the first dash breaks the digit run) */
                u8 pre[2] = {0, 0};
                if (!rd(s, p, pre, 2) || pre[0] != 'D' || pre[1] != ':')
                    continue;
                char digits[16] = {};
                u32 nd = 0;
                while (p + 2 + nd < limit && nd < 16) {
                    u8 c = 0;
                    if (!rd(s, p + 2 + nd, &c, 1))
                        break;
                    if (c < '0' || c > '9')
                        break;
                    digits[nd] = char(c);
                    ++nd;
                }
                if (nd < 14)
                    continue; /* not the plain D:YYYYMMDDHHMMSS form */
                /* the fixed 14-digit read; an over-long digit run keeps
                 * the first 14 (the suffix grammar belongs past them) */
                digits[14] = 0;
                char out[24];
                const int n =
                    snprintf(out, sizeof(out), "%.4s-%.2s-%.2s %.2s-%.2s-%.2s", digits, digits + 4,
                             digits + 6, digits + 8, digits + 10, digits + 12);
                if (n == 19)
                    date.assign(out, size_t(n));
                continue;
            }
            /* literal string: read up to the cap, honoring \( \) \\ */
            std::string v;
            v.reserve(kStrCap);
            u8 c = 0;
            u64 q = p;
            while (q < limit && v.size() < kStrCap) {
                if (!rd(s, q, &c, 1))
                    break;
                if (c == ')')
                    break;
                if (c == '\\') {
                    u8 e = 0;
                    if (q + 1 < limit && rd(s, q + 1, &e, 1) &&
                        (e == '(' || e == ')' || e == '\\')) {
                        v.push_back(char(e));
                        q += 2;
                        continue;
                    }
                    v.push_back('\\');
                    ++q;
                    continue;
                }
                v.push_back(char(c));
                ++q;
            }
            while (!v.empty() && (v.back() == 0 || v.back() == ' '))
                v.pop_back();
            if (!v.empty()) {
                if (key == K_TITLE)
                    title = std::move(v);
                else if (key == K_CREATOR)
                    creator = std::move(v);
                else
                    producer = std::move(v);
            }
        }
        if (block + take >= limit)
            break;
        block += u64(take) - 8; /* 9-byte key overlap seam */
    }
    if (!title.empty()) {
        r.name_prefix = prefix_from_bytes(title);
        r.name_counter_key = title; /* the per-string counter key (the
                                     * H-session census: TeX 1..453,
                                     * GPL Ghostscript 9.56.1 1..158,
                                     * LaTeX-with-hyperref-packa 1..46) */
        return;
    }
    std::u16string out;
    if (!creator.empty()) {
        out = prefix_from_bytes(creator);
        r.name_counter_key = creator;
    } else if (!producer.empty()) {
        out = prefix_from_bytes(producer);
        r.name_counter_key = producer;
    }
    if (!date.empty())
        out += prefix_from_bytes(date); /* the date rides the display
                                         * prefix, NOT the counter key
                                         * (TeX-dated and TeX-plain
                                         * share one counter) */
    if (!out.empty())
        r.name_prefix = std::move(out);
}

static CarveResult carve_pdf_done(IStorageDevice* s, CarveResult r, u64 hit, u64 end) {
    r.ok = true;
    r.end = end;
    r.confidence = 900;
    r.note = "%%EOF(last-before-next-PDF,1MiB-blocks)";
    pdf_name_prefix(s, hit, end, r); /* S-049 */
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

/* ---- JPEG: SOI + marker walk to EOI (F4-class grammar, corpus SOF0) ----
 *
 * S-018 RE-FILED (s31, OQ-S3-JPEG resolved): the og's JPEG chain is
 * two-generation — the EOI-gated modern BruteForce::JpegAnalyzer (S-018's
 * original INVALID evidence, the backward FF D9 scan) AND the legacy
 * FileJPGAnalyser fallback (dispatcher FUN_18004b170 → JFIF/Exif/generic
 * sub-analysers FUN_18004bbd0/b3b0/c1e0; the census-documented chain: one
 * generation accepting is enough). The legacy tier SALVAGES tail-truncated
 * JPEGs: its header walk is SOS-gated (SOF0 precision-8 checked, SOF2 dims
 * fill-if-empty), and the post-SOS walker FUN_18004c480
 * (RAWSearchDLL_decompiled.c:57942-57991) scans for FF D9 up to the limit
 * h*w*3 (dims from SOF) or 20,000,000 (the 200,000,000 global cap at
 * FUN_180001350+0x70, /10) measured FROM THE CANDIDATE START — and on
 * reaching the limit (or the device end via the sliding-window getter
 * FUN_180042930) it ACCEPTS anyway: the 0x23 format stamp + size = the
 * offset reached (:57958-59 `if (limit <= off || get() == NULL) goto
 * ACCEPT`). The core previously required EOI unconditionally — the
 * canonical damaged-disk photo (tail destroyed) was dropped where the og
 * salvages it. The salvage below reproduces the legacy tier's sizing;
 * the deliberate delta: the og stamps salvaged files identically to
 * complete ones (no grade distinction found in the legacy path), the
 * core grades them PartialRisk (600 < the 850 Excellent band) — recover
 * more, grade honestly (registered in PARITY-DELTAS). */
/* S-049: the JPEG naming prefix — EXIF Model, then EXIF Software, then
 * the SOF dimensions (the dims branch the DRWUI naming caller gates on
 * W,H in [1, 0x2000] — :1877540-46), else empty (the emit site's plain
 * uppercase-ext default). Real-session forms: GT-S7562_000123.JPG,
 * Adobe Photoshop 7.0_N.JPG, JPG_192x144_000001.JPG. The dims prefix
 * composes the format name: "JPG_<W>x<H>". */
static std::u16string jpeg_name_prefix(IStorageDevice* s, u64 hit, u64 cap, u64 sof_w, u64 sof_h,
                                       std::string* counter_key = nullptr,
                                       std::string* tag_group = nullptr) {
    /* EXIF APP1: walk the JPEG segments for APP1 "Exif\0\0", then the
     * TIFF IFD0 for Model (0x0110) / Software (0x0131). Bounded: the
     * first 64 KiB of the file (EXIF lives in the header; the APP1
     * segment length is u16-bounded anyway). */
    const u64 limit = hit + (cap < 65536 ? cap : 65536);
    u64 pos = hit + 2;
    std::string model, software;
    int markers = 0;
    while (pos + 4 <= limit) {
        if (++markers > 256)
            break;
        u8 m[4];
        if (!rd(s, pos, m, 2) || m[0] != 0xFF)
            break;
        const u8 mk = m[1];
        if (mk == 0xD8 || (mk >= 0xD0 && mk <= 0xD7) || mk == 0x01) {
            pos += 2;
            continue;
        }
        if (mk == 0xD9 || mk == 0xDA)
            break; /* header walk ends at the entropy data */
        const u16 seglen = rdbe16(s, pos + 2);
        if (seglen < 2)
            break;
        if (mk == 0xE1 && seglen >= 10) {
            u8 exif_hdr[6];
            if (rd(s, pos + 4, exif_hdr, 6) == true && std::memcmp(exif_hdr, "Exif\0\0", 6) == 0) {
                /* TIFF header at pos+10: byte order II/MM (0x4942/0x4D4D),
                 * magic 42, IFD0 offset */
                const u64 tiff = pos + 10;
                u8 th[8];
                if (rd(s, tiff, th, 8) == true) {
                    const bool le = th[0] == 0x49 && th[1] == 0x49;
                    const bool be = th[0] == 0x4D && th[1] == 0x4D;
                    if (le || be) {
                        const auto rd16e = [&](u64 o) {
                            u8 b[2];
                            if (!rd(s, o, b, 2))
                                return u16(0);
                            return le ? u16(u32(b[0]) | (u32(b[1]) << 8))
                                      : u16((u32(b[0]) << 8) | b[1]);
                        };
                        const auto rd32e = [&](u64 o) {
                            u8 b[4];
                            if (!rd(s, o, b, 4))
                                return u32(0);
                            return le ? (u32(b[0]) | (u32(b[1]) << 8) | (u32(b[2]) << 16) |
                                         (u32(b[3]) << 24))
                                      : ((u32(b[0]) << 24) | (u32(b[1]) << 16) | (u32(b[2]) << 8) |
                                         b[3]);
                        };
                        const u16 magic = rd16e(tiff + 2);
                        const u32 ifd_off = rd32e(tiff + 4);
                        if (magic == 42 && ifd_off != 0 && ifd_off < 65536 &&
                            tiff + ifd_off + 2 <= limit) {
                            const u64 ifd = tiff + ifd_off;
                            const u16 count = rd16e(ifd);
                            if (count > 0 && count <= 512) {
                                for (u16 e = 0; e < count; ++e) {
                                    const u64 entry = ifd + 2 + u64(e) * 12;
                                    if (entry + 12 > limit)
                                        break;
                                    const u16 tag = rd16e(entry);
                                    if (tag != 0x0110 && tag != 0x0131)
                                        continue;
                                    const u16 typ = rd16e(entry + 2);
                                    const u32 cnt = rd32e(entry + 4);
                                    /* ASCII type only; bounded to a sane
                                     * tag-string length */
                                    if (typ != 2 || cnt == 0 || cnt > 256)
                                        continue;
                                    u64 val_off = entry + 8;
                                    if (cnt > 4) {
                                        const u32 vo = rd32e(entry + 8);
                                        if (vo == 0 || vo >= 65536)
                                            continue;
                                        val_off = tiff + vo;
                                    }
                                    if (val_off + cnt > limit)
                                        continue;
                                    std::string v(size_t(cnt), '\0');
                                    if (!rd(s, val_off, &v[0], cnt))
                                        continue;
                                    while (!v.empty() && (v.back() == 0 || v.back() == ' '))
                                        v.pop_back(); /* NUL/space trim */
                                    if (v.empty())
                                        continue;
                                    /* N-209 (renumbered from the born-collided N-190 at the S57
                                     * CI-red repair): the og's strncpy(dst, src, 0x27) caps the tag
                                     * value at 39 chars (RAWSearchDLL:40388-40389 the Software
                                     * 0x131 branch, :40483-40484 the Model
                                     * 0x110 twin - the 39th char KEPT, no NUL
                                     * at exactly 39); the pre-fix core
                                     * consumed the whole tag string into the
                                     * prefix and the per-string counter key */
                                    if (v.size() > 39)
                                        v.resize(39);
                                    if (tag == 0x0110 && model.empty())
                                        model = v;
                                    else if (tag == 0x0131 && software.empty())
                                        software = v;
                                }
                            }
                        }
                    }
                }
            }
        }
        pos += 2 + u64(seglen);
    }
    if (!model.empty()) {
        if (counter_key != nullptr)
            *counter_key = model; /* the per-string counter key */
        /* M-101: the Tags Extracted branch's family value — EXIF Model
         * first, Software second (the same priority the prefix derives;
         * the og's branch groups by the extracted tag itself). */
        if (tag_group != nullptr)
            *tag_group = model;
        return prefix_from_bytes(model);
    }
    if (!software.empty()) {
        if (counter_key != nullptr)
            *counter_key = software;
        if (tag_group != nullptr)
            *tag_group = software; /* M-101: the Software fallback family */
        return prefix_from_bytes(software);
    }
    /* the dims branch: W,H in [1, 0x2000] (the DRWUI caller's sanity
     * gate, :1877540-46); the prefix composes the format name. The
     * dims_form flag routes the emit site's counter to the branch-1
     * per-format key (the plain+dims families share ONE counter - the
     * H-session JPG census: the union 1..952 at 96.7% density; the
     * string families carry their own per-string counters). */
    if (sof_w >= 1 && sof_w <= 0x2000 && sof_h >= 1 && sof_h <= 0x2000) {
        char buf[32];
        const int n = snprintf(buf, sizeof(buf), "JPG_%ux%u", unsigned(sof_w), unsigned(sof_h));
        if (n > 0)
            return prefix_from_bytes(buf, size_t(n)); /* derived: the
                                                       * per-format key */
    }
    return {};
}

CarveResult validate_jpeg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u64 pos = hit + 2; /* skip FFD8 */
    bool saw_sof = false, saw_sos = false;
    /* the og fallback's salvage inputs: SOF dimensions (last good SOF0
     * wins, SOF2 fills-if-empty — FUN_18004bbd0 case -0x40/-0x3e) */
    u64 sof_estimate = 0;     /* h*w*3; 0 = dims unknown/unreadable */
    u64 sof_w = 0, sof_h = 0; /* S-049: the dims prefix inputs */
    /* S-049: compose the naming prefix on every accept path (the og's
     * prefix is analyser-supplied regardless of the accept tier) */
    const auto set_prefix = [&]() {
        r.name_counter_key.clear();
        r.name_prefix =
            jpeg_name_prefix(s, hit, cap, sof_w, sof_h, &r.name_counter_key, &r.tag_group);
    };
    int markers = 0;
    /* the legacy tier's accept-at-limit: anchored at the candidate start,
     * end = min(hit + (h*w*3 | 20,000,000), bound) where bound is the
     * furthest offset the scan could reach (probe_limit, or the I/O
     * failure point — the og's getter NULL accepts at the current
     * offset, :57959); SOS is the gate (no SOS in the header walk = the
     * og's buffer-end reject) */
    const auto salvage = [&](u64 bound) -> CarveResult {
        if (!saw_sos)
            return r; /* og: the walk never reached SOS → no stamp */
        const u64 og_limit = sof_estimate != 0 ? sof_estimate : 20000000ull;
        u64 end = hit + og_limit;
        if (end > bound)
            end = bound;
        if (end <= hit + 2)
            return r;
        r.ok = true;
        r.end = end;
        r.confidence = 600;
        r.note = sof_estimate != 0 ? "fallback-salvage(no-EOI,SOF-estimate)"
                                   : "fallback-salvage(no-EOI,cap/10)";
        set_prefix(); /* S-049: the prefix rides the salvage tier too */
        return r;
    };
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
            if (r.ok)
                set_prefix();
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
        /* S-018 salvage inputs — the og fallback's dimension reads
         * (FUN_18004bbd0 :57749-58/:57762-66): SOF0 re-reads dims
         * unconditionally after the precision gate (precision BE at
         * pos+4, height BE at pos+5, width BE at pos+7); SOF2 fills
         * them only when still unknown (`uVar17 * uVar16 == 0`). */
        if (mk == 0xC0 || mk == 0xC2) {
            u8 prec[1];
            const bool prec_ok = mk == 0xC2 || (rd(s, pos + 4, prec, 1) && prec[0] == 8);
            if (prec_ok) {
                const u64 h = rdbe16(s, pos + 5);
                const u64 w = rdbe16(s, pos + 7);
                if (h != 0 && w != 0 && (mk == 0xC0 || sof_estimate == 0)) {
                    sof_estimate = h * w * 3;
                    if (mk == 0xC0 || (sof_w == 0 && sof_h == 0)) {
                        sof_w = w;
                        sof_h = h;
                    }
                }
            }
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
                    return salvage(p); /* og: getter NULL mid-scan accepts
                                        * at the current offset */
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
                return salvage(elim); /* cap reached without a closing
                                       * marker — the og legacy tier
                                       * accepts at its limit */
            continue;
        }
        pos += 2 + u64(len);
    }
    return salvage(probe_limit(s, hit, cap)); /* cap reached without EOI —
                                               * same legacy-tier accept */
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
                /* M-056: the value field is an offset RELATIVE to the
                 * TIFF base (hit) - the data end feeds end_data, which is
                 * later re-based by r.end = hit + end_data. The old
                 * `data_at = hit + val` produced end_data in ABSOLUTE
                 * coordinates, so every out-of-line entry (all real
                 * multi-strip TIFFs: StripOffsets/StripByteCounts) added
                 * the candidate offset to its own size - on real disks
                 * the 256 MiB cap then swallowed the neighborhood. */
                u64 data_at = u64(val);
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
/* ---- S-049: the MP4/MOV duration naming prefix ------------------------
 * Real-session evidence (first-hand re-mining this session; the verbatim
 * table in evidence/executions/s10b-real-rsf-session-mining.md): the og's
 * carved MP4/MOV rows are named "<EXT>_<duration>_<%06d>.<EXT>" —
 * MP4_14s_000003.MP4, MP4_08m07s_000030.MP4, MP4_01m_000006.MP4,
 * MOV_10s_000009.MOV, MOV_01m01s_000015.MOV — 57/57 real MP4/MOV carve
 * rows across the three og sessions, ZERO "Lost Name File (...)_EaseUS"
 * template names. The N-063 engine-side decode (RAWSearchDLL :33312 —
 * the analyser SetName template) is real ENGINE code, but the DRWUI
 * tree-naming layer names every unnamed carve record through the
 * "%s_%06d" contract (FUN_140c44770) and the engine template does not
 * survive it in the shipped product's persisted sessions. This landing
 * supersedes N-063's carve-tree application: the prefix is the mvhd
 * DURATION (timescale+duration), composed with the format name.
 *
 * mvhd (ISO/IEC 14496-12): v0 — timescale u32@box+20, duration u32@box+24;
 * v1 — timescale u32@box+28, duration u64@box+32. duration_seconds =
 * duration / timescale (timescale 0 = no prefix). */
static std::u16string mp4_duration_prefix(IStorageDevice* s, u64 hit, u64 cap,
                                          const char* fmt_name) {
    /* walk top-level boxes for moov, then moov's children for mvhd */
    u64 pos = hit;
    u64 mvhd = 0;
    int boxes = 0;
    while (pos + 8 <= hit + cap && mvhd == 0) {
        if (++boxes > 4096)
            break;
        const u32 sz = rdbe32(s, pos);
        u8 type[4];
        if (!rd(s, pos + 4, type, 4))
            break;
        u64 box_len = sz;
        if (sz == 1) {
            const u64 big = (u64(rdbe32(s, pos + 8)) << 32) | rdbe32(s, pos + 12);
            if (big < 16 || pos + big > hit + cap)
                break;
            box_len = big;
        } else if (sz == 0) {
            box_len = (hit + cap) - pos; /* to-EOF box */
        }
        if (pos + box_len > hit + cap)
            box_len = (hit + cap) - pos;
        if (std::memcmp(type, "moov", 4) == 0 && box_len >= 8) {
            /* walk moov's children */
            u64 ipos = pos + 8;
            const u64 iend = pos + box_len;
            int inner = 0;
            while (ipos + 8 <= iend) {
                if (++inner > 1024)
                    break;
                const u32 isz = rdbe32(s, ipos);
                u8 itype[4];
                if (!rd(s, ipos + 4, itype, 4))
                    break;
                u64 ibox = isz;
                if (isz == 1) {
                    const u64 big2 = (u64(rdbe32(s, ipos + 8)) << 32) | rdbe32(s, ipos + 12);
                    if (big2 < 16 || ipos + big2 > iend)
                        break;
                    ibox = big2;
                } else if (isz == 0) {
                    ibox = iend - ipos;
                }
                if (std::memcmp(itype, "mvhd", 4) == 0 && ibox >= 20) {
                    mvhd = ipos;
                    break;
                }
                if (ibox < 8)
                    break;
                ipos += ibox;
            }
        }
        if (box_len < 8)
            break;
        pos += box_len;
    }
    if (mvhd == 0)
        return {}; /* no mvhd: the emit site falls back to the plain
                      uppercase-ext default prefix */
    const u8 version = rd8(s, mvhd + 8);
    u32 timescale;
    u64 duration;
    if (version == 1) {
        if (mvhd + 40 > hit + cap)
            return {};
        timescale = rdbe32(s, mvhd + 28);
        duration = rdbe64(s, mvhd + 32);
    } else {
        if (mvhd + 28 > hit + cap)
            return {};
        timescale = rdbe32(s, mvhd + 20);
        duration = rdbe32(s, mvhd + 24);
    }
    if (timescale == 0 || duration == 0)
        return {};
    const u64 secs = duration / timescale;
    if (secs == 0)
        return {}; /* sub-second: the og's %02us floor would print 00s —
                      no observed sub-second sample; the plain default is
                      the honest form */
    const std::u16string dur = duration_form(secs);
    if (dur.empty())
        return {};
    char head[16];
    const int n = snprintf(head, sizeof(head), "%s_", fmt_name);
    if (n <= 0)
        return {};
    std::u16string out = prefix_from_bytes(head, size_t(n));
    out += dur;
    return out;
}

CarveResult validate_mp4(IStorageDevice* s, u64 hit, u64 cap) {
    static const char* const brands[] = {"isom", "mp42", "mp41", "iso2", "avc1", "dash"};
    CarveResult r = bmff_walk(s, hit, cap, brands, 6);
    if (r.ok) {
        r.confidence = 780;
        r.name_prefix = mp4_duration_prefix(s, hit, cap, "MP4"); /* S-049 */
    }
    return r;
}
CarveResult validate_mov(IStorageDevice* s, u64 hit, u64 cap) {
    static const char* const brands[] = {"qt  "};
    CarveResult r = bmff_walk(s, hit, cap, brands, 1);
    if (r.ok) {
        r.confidence = 720;
        r.name_prefix = mp4_duration_prefix(s, hit, cap, "MOV"); /* S-049 */
    }
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
    r.name_prefix = mp4_duration_prefix(s, hit, cap, "MOV"); /* S-049 */
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
                return best != 0 ? carve_pdf_done(s, r, hit, best) : r;
            }
        }
        if (block + take >= limit)
            break;
        block += u64(take) - 4; /* 5-byte magics: (mlen-1) seam */
    }
    return best != 0 ? carve_pdf_done(s, r, hit, best) : r;
}

/* ---- S-049: the archive naming prefixes ---------------------------------
 * ZIP: the first central-directory entry name, path separators -> '_'
 * (real-session pins: "META-INF/" -> META-INF__000001.ZIP, "classes.dex"
 * -> classes.dex_000001.ZIP, "mimetype" -> mimetype_N.ZIP, "-" ->
 * -_000001.ZIP, "027.jpg" keeps its dots - the finding's dot-translation
 * example is corrected by the classes.dex/027.jpg families: dots are
 * preserved, only the path separator maps). */
static std::u16string zip_first_entry_prefix(std::string nm, std::string* counter_key = nullptr) {
    if (counter_key != nullptr)
        *counter_key = nm; /* the raw entry name keys the counter (the
                            * census: every ZIP family 1-based dense) */
    for (char& c : nm)
        if (c == '/' || c == '\\')
            c = '_';
    return prefix_from_bytes(nm);
}

static std::u16string zip_first_entry_prefix(const std::vector<CarveEntry>& entries,
                                             std::string* counter_key = nullptr) {
    if (entries.empty())
        return {};
    return zip_first_entry_prefix(entries.front().name, counter_key);
}

/* DOCX: the docProps/core.xml dc:creator + created date, composed
 * "<creator>_<YYYY-MM-DD HH-MM-SS>" (real-session pins - the S42 lane's
 * first-hand G/H census: jacko_2014-09-10 20-43-00_N x6,
 * KIS2_2022-05-25 13-14-00_N, Wei Chen_2017-05-25 19-28-00_N x2,
 * python-docx_2013-12-23 23-15-00_N - and the creator-less form
 * 2006-07-25 02-16-00_N.DOCX x24, 2006-10-24 12-00-00, 2021-10-27
 * 10-50-00: the W3CDTF T/Z punctuation maps to space/dashes; colons are
 * Windows-illegal). The creator rides the og's office metadata walk
 * (RAWSearchDLL :53530-53666: dc:creator from cp:coreProperties). The
 * core.xml entry is located through the S-032 inventory
 * (data_offset/data_len) and inflated through the shared zlib path when
 * deflate-compressed. */
static std::u16string docx_created_prefix(IStorageDevice* s, const std::vector<CarveEntry>& entries,
                                          std::string* counter_key = nullptr) {
    const CarveEntry* core_xml = nullptr;
    for (const CarveEntry& e : entries)
        if (e.name == "docProps/core.xml" && e.data_len > 0 && e.data_len < (1u << 20)) {
            core_xml = &e;
            break;
        }
    if (core_xml == nullptr)
        return {};
    std::vector<u8> raw(size_t(core_xml->data_len));
    if (!rd(s, core_xml->data_offset, raw.data(), u32(raw.size())))
        return {};
    /* core.xml is deflate-compressed in practice (Office writes method
     * 8); inflate through the RFC-1950 tolerant wrapper (a zlib header
     * is present for method-8 entries; stored members fall through to
     * the raw bytes). Bounded: the inflated member is at most 256 KiB
     * (docProps/core.xml is a few KiB in every real Office file). */
    std::vector<u8> out(256 * 1024);
    std::string xml;
    {
        z_stream zs = {};
        if (inflateInit2(&zs, 15) == Z_OK) { /* +15: RFC 1950 wrapper */
            zs.next_in = const_cast<Bytef*>(raw.data());
            zs.avail_in = uInt(raw.size());
            zs.next_out = out.data();
            zs.avail_out = uInt(out.size());
            if (inflate(&zs, Z_FINISH) == Z_STREAM_END)
                xml.assign(reinterpret_cast<const char*>(out.data()), size_t(zs.total_out));
            inflateEnd(&zs);
        }
    }
    if (xml.empty()) {
        /* stored (method 0) member: the raw bytes are the XML */
        xml.assign(reinterpret_cast<const char*>(raw.data()), raw.size());
    }
    /* the dc:creator text (the og's office walk extracts it from
     * cp:coreProperties, :53542; the real creator-prefixed families:
     * jacko_/KIS2_/Wei Chen_/python-docx_) - bounded to 39 chars (the
     * EXIF-family strncpy cap class) */
    std::u16string creator;
    {
        const size_t cpos = xml.find("dc:creator");
        if (cpos != std::string::npos) {
            const size_t cgt = xml.find('>', cpos);
            const size_t clt = xml.find('<', cgt == std::string::npos ? 0 : cgt + 1);
            if (cgt != std::string::npos && clt != std::string::npos && clt > cgt + 1 &&
                clt - (cgt + 1) <= 39) {
                creator = prefix_from_bytes(xml.data() + cgt + 1, clt - (cgt + 1));
                /* the creator keys the per-string counter (the census:
                 * jacko 1..6 across six different dates, KIS2 1..2,
                 * Wei Chen 1..2 - the DATE never joins the key) */
                if (counter_key != nullptr)
                    *counter_key = xml.substr(cgt + 1, clt - (cgt + 1));
            }
        }
    }
    /* the created date: <dcterms:created ...>YYYY-MM-DDThh:mm:ss...< */
    const size_t pos = xml.find("dcterms:created");
    if (pos == std::string::npos)
        return creator; /* creator alone: no date row observed, but the
                         * composition degrades to the string family */
    const size_t gt = xml.find('>', pos);
    if (gt == std::string::npos)
        return creator;
    const size_t lt = xml.find('<', gt + 1);
    if (lt == std::string::npos)
        return creator;
    const size_t ilen = lt - (gt + 1);
    if (ilen != 19 && ilen != 20) /* 19: no zone; 20: the trailing Z form */
        return creator;
    const std::string iso = xml.substr(gt + 1, 19);
    if (iso[4] != '-' || iso[7] != '-' || (iso[10] != 'T' && iso[10] != ' ') || iso[13] != ':' ||
        iso[16] != ':')
        return creator;
    /* the W3CDTF punctuation maps to the og's form: T -> space, the time
     * colons -> dashes (Windows-illegal) - "2006-07-25 02-16-00" */
    char buf[20];
    const int n =
        snprintf(buf, sizeof(buf), "%.4s-%.2s-%.2s %.2s-%.2s-%.2s", iso.data() + 0, iso.data() + 5,
                 iso.data() + 8, iso.data() + 11, iso.data() + 14, iso.data() + 17);
    if (n != 19)
        return creator;
    if (!creator.empty())
        return creator + prefix_from_bytes("_", 1) + prefix_from_bytes(buf, 19);
    return prefix_from_bytes(buf, 19);
}

/* ---- ZIP family: local header + EOCD; DOCX adds [Content_Types].xml ---- */
/* N-158: the og's XMIND content probe (RAWSearchDLL FUN_180045110,
 * :52469-52715, disassembly-verified at 0x180045110-0x180045666).
 * The og builds a 300-byte RAW window at record+30+strlen(name) - the
 * entry's data start with the extra field skipped (the og quirk: it
 * reads past elen; well-formed archives have elen 0 there, and the
 * window is the entry's data either way) - and types XMIND when
 * EITHER needle hits:
 *   - "xmlns=\"urn:xmind" (16 ASCII bytes - the stored-XML default
 *     namespace declaration; FUN_18000f820 = the std::string find,
 *     disasm 0x180045616)
 *   - the 14-byte binary constant at .data RVA 0x93980 (f4 4a c6 40
 *     1a bc a7 76 cf e1 a0 d1 cc 9d - the deflated-stream prolog
 *     fingerprint; the second find FUN_180016fc0 len 0xe, disasm
 *     0x180045629-0x18004564f, non-NULL return stamps via cmovne)
 * Both searches cover the same 300 bytes; the probe runs per entry
 * named content.xml or meta.xml. The window clamps to the candidate's
 * probe limit (the og reads its in-memory carve buffer unbounded; a
 * short-tail window only ever fails to match, never false-matches). */
static bool xmind_content_probe(IStorageDevice* s, u64 pos, u16 nlen, u64 limit) {
    static const char kNeedleXmlns[] = "xmlns=\"urn:xmind"; /* 16 bytes */
    static const u8 kNeedleProlog[14] = {0xf4, 0x4a, 0xc6, 0x40, 0x1a, 0xbc, 0xa7,
                                         0x76, 0xcf, 0xe1, 0xa0, 0xd1, 0xcc, 0x9d};
    const u64 base = pos + 30 + u64(nlen); /* the og's window base (elen skipped) */
    if (base >= limit)
        return false;
    u8 win[300];
    const u64 avail64 = limit - base;
    const size_t avail = avail64 < sizeof(win) ? size_t(avail64) : sizeof(win);
    if (avail == 0 || !rd(s, base, win, u32(avail))) /* u32: rd's length param (MSVC C4267) */
        return false;
    for (size_t i = 0; i + sizeof(kNeedleXmlns) - 1 <= avail; ++i)
        if (std::memcmp(win + i, kNeedleXmlns, sizeof(kNeedleXmlns) - 1) == 0)
            return true;
    for (size_t i = 0; i + sizeof(kNeedleProlog) <= avail; ++i)
        if (std::memcmp(win + i, kNeedleProlog, sizeof(kNeedleProlog)) == 0)
            return true;
    return false;
}

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
        /* N-115: the og's OfficeTagFromXML typing pass (RAWSearchDLL
         * :53164-53194 + the singleton walker :53202-53300). The old
         * gate required [Content_Types].xml as the LITERAL FIRST entry;
         * the og walks the entry inventory matching names ANYWHERE (a
         * damaged/reordered zip still types) and decompresses entries
         * up to 10 KiB (0x2800) for the content tags. The rels-path
         * map (matched on the ENTRY NAME with the office flag):
         *   word/_rels/document.xml.rels     -> 0x49 DOCX
         *   xl/_rels/workbook.xml.rels       -> 0x4a XLSX
         *   ppt/_rels/presentation.xml.rels  -> 0x4b PPTX
         *   a >=5-char name starting "word" -> 0x49 (the fallback)
         * An office-shaped zip ([Content_Types].xml seen) that the map
         * cannot refine stays DOCX (the og's dispatch analyser id).
         * The wider tag corpus (the floss strings :6699-6729): the
         * mimetype entry's content types the ODF trio / EPUB / HWP
         * (stored or deflated <= 10 KiB), and name tags carry iWork
         * (the Index/.iwa family, index.apxl), EPUB (OPS/chapter1.html) and
         * CorelDRAW (content/riffData.cdr). The walk is bounded (32
         * entries / 256 KiB - the og's window bounds it similarly; the
         * typed names live in the first entries of real files). */
        u32 typed = 0;
        bool content_types = false;
        u8 nmbuf[512];
        u64 pos = hit;
        const u64 wl = probe_limit(s, hit, cap);
        const u64 wb = hit + (u64(256) << 10);
        const u64 walk_limit = wb < wl ? wb : wl;
        for (int ent = 0; ent < 32 && pos + 30 <= walk_limit; ++ent) {
            u8 lfh[30];
            if (!rd(s, pos, lfh, 30) || std::memcmp(lfh, "PK\x03\x04", 4) != 0)
                break;
            const u16 method = u16(lfh[8]) | (u16(lfh[9]) << 8);
            const u16 nlen = u16(lfh[26]) | (u16(lfh[27]) << 8);
            const u16 elen = u16(lfh[28]) | (u16(lfh[29]) << 8);
            u32 csize =
                u32(lfh[18]) | (u32(lfh[19]) << 8) | (u32(lfh[20]) << 16) | (u32(lfh[21]) << 24);
            u32 usize =
                u32(lfh[22]) | (u32(lfh[23]) << 8) | (u32(lfh[24]) << 16) | (u32(lfh[25]) << 24);
            if (nlen == 0 || nlen > sizeof(nmbuf))
                break; /* not a name-bearing entry the walk understands */
            if (!rd(s, pos + 30, nmbuf, nlen))
                break;
            nmbuf[nlen] = 0;
            const char* nm = reinterpret_cast<const char*>(nmbuf);
            const bool is_mimetype = std::strcmp(nm, "mimetype") == 0;
            const bool is_ct = std::strcmp(nm, "[Content_Types].xml") == 0;
            if (is_ct)
                content_types = true;
            /* the rels-path map (entry-name matches, office context) */
            if (std::strcmp(nm, "word/_rels/document.xml.rels") == 0)
                typed = FMT_DOCX;
            else if (std::strcmp(nm, "xl/_rels/workbook.xml.rels") == 0)
                typed = FMT_XLSX;
            else if (std::strcmp(nm, "ppt/_rels/presentation.xml.rels") == 0)
                typed = FMT_PPTX;
            else if (nlen >= 5 && std::memcmp(nm, "word", 4) == 0 && typed == 0 && content_types)
                typed = FMT_DOCX; /* the og's word-prefix fallback */
            /* the name-tag set (no decompression needed) */
            else if (std::strcmp(nm, "content/riffData.cdr") == 0)
                typed = FMT_CDR;
            else if ((nlen > 6 && std::memcmp(nm, "Index/", 6) == 0 && nlen > 10 &&
                      std::memcmp(nm + nlen - 4, ".iwa", 4) == 0) ||
                     std::strcmp(nm, "index.apxl") == 0)
                typed = FMT_IWORK;
            else if (std::strcmp(nm, "OPS/chapter1.html") == 0)
                typed = FMT_EPUB;
            /* N-158: the og's five-branch typing tail (the LFH-handler
             * walk :53025-53100, re-read first-hand + disassembly-
             * verified). Per-entry sequence: the XMIND content probe
             * (content.xml/meta.xml only), then the four entry-name
             * compares. Each match OVERWRITES the id - the og's chain
             * walk (:18411-18450, the do/while over every record with
             * the shared id slot) has NO typing stop; the N-115-era
             * "the first decisive tag wins" comment was an erratum,
             * corrected here (last-match-wins within the walk bounds).
             * No office-context gate: the og's LFH handler runs these
             * compares unconditionally, and the office rows' validator
             * runs at every LFH hit through the contest. */
            if ((std::strcmp(nm, "content.xml") == 0 || std::strcmp(nm, "meta.xml") == 0) &&
                xmind_content_probe(s, pos, nlen, wl))
                typed = FMT_XMIND;
            else if (std::strcmp(nm, "xsd/MindManagerApplication.xsd") == 0)
                typed = FMT_MMAP;
            else if (std::strcmp(nm, "rels/_rels.xml") == 0 ||
                     std::strcmp(nm, "rels/page1_rels.xml") == 0)
                typed = FMT_EDDX;
            else if (std::strcmp(nm, "ProjectFolder/Anon/") == 0 ||
                     std::strcmp(nm, "ProjectFolder/project_info.json") == 0)
                typed = FMT_WFP;
            else if (std::strcmp(nm, "MediaPool/Master/MpFolder.xml") == 0)
                typed = FMT_DRP;
            /* the content tags: the mimetype entry (ODF/EPUB/HWP) and
             * [Content_Types].xml (HWP), <= 10 KiB, stored or deflated */
            if (typed == 0 && (is_mimetype || (is_ct && csize <= 0x2800))) {
                u8 content[0x2800 + 1];
                size_t got = 0;
                const u64 dpos = pos + 30 + nlen + elen;
                if (method == 0 && csize <= sizeof(content)) {
                    if (csize != 0 && rd(s, dpos, content, csize))
                        got = csize;
                } else if (method == 8 && csize <= 0x2800 && usize != 0 && usize <= 0x2800 &&
                           csize != 0xFFFFFFFFu) {
                    u8 comp[0x2800];
                    if (rd(s, dpos, comp, csize) &&
                        compress::decompress_block(compress::kGzip, comp, csize, content, usize))
                        got = usize;
                }
                content[got] = 0;
                if (got != 0) {
                    const char* text = reinterpret_cast<const char*>(content);
                    const size_t tlen = got;
                    auto has = [&](const char* needle) {
                        const size_t nl = std::strlen(needle);
                        if (nl == 0 || nl > tlen)
                            return false;
                        for (size_t i = 0; i + nl <= tlen; ++i)
                            if (std::memcmp(text + i, needle, nl) == 0)
                                return true;
                        return false;
                    };
                    if (has("application/vnd.oasis.opendocument.text"))
                        typed = FMT_ODT;
                    else if (has("application/vnd.oasis.opendocument.spreadsheet"))
                        typed = FMT_ODS;
                    else if (has("application/vnd.oasis.opendocument.presentation"))
                        typed = FMT_ODP;
                    else if (has("application/epub+zip"))
                        typed = FMT_EPUB;
                    else if (has("application/hwp+zip"))
                        typed = FMT_HWP;
                }
            }
            /* N-158: NO typing stop - the og's chain walk
             * (:18411-18450, the do/while dispatching every record
             * with the shared id slot) never stops on a typed id;
             * later records overwrite earlier stamps (last-match-wins
             * within the 32-entry/256 KiB walk bounds). The old
             * "first decisive tag wins (the og's map stop)" stop was
             * an N-115-era misreading of the CEN singleton walker
             * (FUN_180046120 returns after one record - the walk
             * itself lives in the caller), corrected here. */
            const u64 next = pos + 30 + u64(nlen) + elen + csize;
            if (next <= pos)
                break;
            pos = next;
        }
        if (typed == 0 && !content_types)
            return r; /* neither office-shaped nor typed: not this row */
        if (typed != 0 && typed != FMT_DOCX)
            r.format_override = typed; /* the og stamps the id at the probe */
        /* typed == 0 && content_types: the unrefinable office zip stays
         * the dispatch row (DOCX, the og's 0x49) - no override needed */
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
            u32 csize =
                u32(lfh[18]) | (u32(lfh[19]) << 8) | (u32(lfh[20]) << 16) | (u32(lfh[21]) << 24);
            u32 usize =
                u32(lfh[22]) | (u32(lfh[23]) << 8) | (u32(lfh[24]) << 16) | (u32(lfh[25]) << 24);
            if (csize == 0xFFFFFFFFu || usize == 0xFFFFFFFFu) {
                /* N-008: ZIP64 sizes live in the LFH extra field (header
                 * id 0x0001; spec order usize, csize, header-offset,
                 * each present only when its LFH slot carries the
                 * 0xFFFFFFFF sentinel). The og's chain walk continues
                 * via the parsed sizes (:18462-500 family) - the old
                 * break truncated ZIP64-era indexless archives. */
                u64 epos = pos + 30 + nlen;
                const u64 eend = epos + elen;
                bool fixed = false;
                while (epos + 4 <= eend && epos + 4 <= limit) {
                    const u16 id = rd16(s, epos);
                    const u16 dsz = rd16(s, epos + 2);
                    if (id == 0x0001) {
                        u64 p = epos + 4;
                        u64 z_usize = usize, z_csize = csize;
                        if (usize == 0xFFFFFFFFu && p + 8 <= eend) {
                            z_usize = u64(rd32(s, p)) | (u64(rd32(s, p + 4)) << 32);
                            p += 8;
                        }
                        if (csize == 0xFFFFFFFFu && p + 8 <= eend) {
                            z_csize = u64(rd32(s, p)) | (u64(rd32(s, p + 4)) << 32);
                            p += 8;
                        }
                        if (z_csize == 0xFFFFFFFFull || z_csize > 0xFFFFFFFFull)
                            break; /* chain math needs a real u32 csize */
                        if (z_usize <= 0xFFFFFFFFull)
                            usize = u32(z_usize); /* inventory truth */
                        csize = u32(z_csize);
                        fixed = true;
                        break;
                    }
                    if (dsz > eend - (epos + 4))
                        break; /* hostile size: end the walk */
                    epos += 4 + dsz;
                }
                if (!fixed)
                    break; /* no ZIP64 record: no deterministic chain */
            }
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
            ir.name_prefix = zip_first_entry_prefix(ir.entries); /* S-049 */
            /* N-008 (decompile :18492-18508): the og's salvage extension -
             * after a BROKEN record chain, absorb data-bearing trailing
             * sectors. Entry: the window at [end, end+sec) carries a
             * non-zero tail dword; then step sec at a time while the
             * window's trailing 16 bytes are NOT all zero; a zero-tailed
             * window ends the absorption (never absorbed). Total
             * extension capped at 0x500001 bytes (the og's loop cap -
             * the old code's 5,242,880 was off by one). CLEAN archives are
             * never inflated (N-027d: the og never extends past a
             * complete EOCD; the old 16-byte zero-absorption here was
             * the invented behavior - removed). */
            const u32 sec =
                s->geometry().logical_sector >= 512 ? s->geometry().logical_sector : 512;
            if (ir.end + sec <= limit && rd32(s, ir.end + sec - 4) != 0) {
                u64 w = ir.end;
                while (w + sec <= limit && w + sec - last_end <= 0x500001ull) {
                    u8 tail[16];
                    if (!rd(s, w + sec - 16, tail, 16))
                        break;
                    bool zeros = true;
                    for (u8 b : tail)
                        if (b) {
                            zeros = false;
                            break;
                        }
                    if (zeros)
                        break; /* stop BEFORE the empty window */
                    w += sec;
                    ir.end = w;
                }
            }
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
    /* N-008/N-027d: CLEAN archives are never inflated - the og's padding
     * extension belongs to the BROKEN-chain salvage path only
     * (decompile :18492-18508, absorbed into the indexless walk above).
     * The old 16-byte-granular zero-absorption here inflated intact
     * ZIPs with trailing zero slack - removed. */
    if (end > limit)
        end = limit;
    r.ok = true;
    r.end = end;
    r.confidence = office ? 850 : 700;
    r.note = office ? "[Content_Types]+EOCD" : "EOCD+cd-geometry";
    /* S-049: DOCX prefixes by the docProps created date (real sessions:
     * 2006-07-25 02-16-00_N.DOCX - never the [Content_Types].xml first
     * entry); plain ZIP prefixes by the first central-directory entry
     * name (META-INF__N.ZIP, classes.dex_N.ZIP) */
    r.name_prefix = office ? docx_created_prefix(s, r.entries, &r.name_counter_key)
                           : zip_first_entry_prefix(r.entries, &r.name_counter_key);
    return r;
}

CarveResult validate_zip(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, false);
}
CarveResult validate_docx(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, true);
}

/* N-115: the typed rows (XLSX/PPTX/ODF/EPUB/iWork/HWP/CDR) share the
 * office walk - the FMT_DOCX row wins the registry-order tie at a shared
 * LFH hit and the override carries the true id; these rows exist so the
 * override resolves the effective ext/name (the M-093/M-111 mechanism). */
CarveResult validate_office_zip(IStorageDevice* s, u64 hit, u64 cap) {
    return zip_common(s, hit, cap, true);
}

/* CARVE-2: central-directory-anchored carving - the og's PREFERRED
 * anchor (PK\x01\x02): when the head of the archive is gone but the CD
 * survives, the entry still surfaces (names live in the CD). Scan
 * forward for the EOCD to size the span; without one, take a bounded
 * CD-sized window. */
/* S-049: the CD record at pos - its entry name (bounded, 512) */
static std::string cd_entry_name(IStorageDevice* s, u64 pos, u16 nlen) {
    if (nlen == 0 || nlen > 512)
        return {};
    std::string nm(size_t(nlen), '\0');
    if (!rd(s, pos + 46, &nm[0], nlen))
        return {};
    return nm;
}

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
            r.name_prefix =
                zip_first_entry_prefix(cd_entry_name(s, hit, nlen), &r.name_counter_key);
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
    r.name_prefix = zip_first_entry_prefix(cd_entry_name(s, hit, nlen), &r.name_counter_key);
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
    /* A-011: every multi-byte SQLite header field is BIG-ENDIAN (the
     * file-format spec; the og composes the page size explicitly BE -
     * FUN_18002e2b0's `v8 | (256 * v5 & 0xff00)`, RetDec :45142-45156).
     * The pre-A-011 LE reads rejected every real database (a 4096-page
     * DB stored 10 00 at +16 read as 16 -> the < 512 gate) - real files
     * only surfaced through the raw-sign layer with degraded extents. */
    const u16 raw_ps = rdbe16(s, hit + 16);
    const u32 page_size = (raw_ps == 1) ? 65536u : u32(raw_ps); /* field: 1 = 64KiB */
    if (page_size < 512 || (page_size & (page_size - 1)) != 0 || page_size > 65536)
        return r;
    /* the text encoding at +56 (BE u32): 1 = UTF-8, 2 = UTF-16LE,
     * 3 = UTF-16BE; the og gates the high byte < 4 (the same field,
     * FUN_18002e2b0's `v7 >> 24 < 4`) */
    const u32 encoding = rdbe32(s, hit + 56);
    if (encoding < 1 || encoding > 3)
        return r;
    /* A-011's second half: the pre-fix `pages > 2ull << 30 / page_size`
     * parsed as `2ull << (30 / page_size)` - for 4096-byte pages the
     * limit was 2ull << 0 = 2, rejecting every real multi-page database
     * (the same never-exercised path as the endianness). */
    u32 pages = rdbe32(s, hit + 28);
    if (pages == 0 || pages > (2ull << 30) / page_size)
        return r;
    u64 end = u64(pages) * page_size;
    if (end > cap)
        end = cap;
    /* freelist + schema sanity-lite: version-valid-for at @18, reader ver @19 */
    r.ok = true;
    r.end = hit + end;
    r.confidence = 860;
    r.note = "page-math";
    /* N-089: the og's CFCPEvt deep walk (FUN_18002de40, the RetDec export
     * :44847-45100 - missing from the Ghidra export) relabels FCP event
     * databases to the subtype id 0x75 (from the generic 0x73) when a name
     * read from the DB content - a 100-byte UTF-16 buffer loaded from the
     * schema walk's position, WideCharToMultiByte'd, exactly 17 converted
     * chars - equals "tableactiveactive" (the .rdata literal at 0x6a0c0,
     * rabin2 :7968). The marker rides the DB's text encoding (the +56
     * field above); table names live in the sqlite_master schema region
     * (page 1). Bounded equivalent: scan the first 4 pages for the marker
     * in the header-declared encoding; a hit relabels the note to the
     * FCP-event-database subtype. */
    {
        static const char kMarker[17] = {'t', 'a', 'b', 'l', 'e', 'a', 'c', 't', 'i',
                                         'v', 'e', 'a', 'c', 't', 'i', 'v', 'e'};
        u8 encoded[34];
        size_t enc_len = 0;
        if (encoding == 1) {
            std::memcpy(encoded, kMarker, sizeof(kMarker));
            enc_len = sizeof(kMarker);
        } else {
            for (size_t i = 0; i < sizeof(kMarker); ++i) {
                if (encoding == 2) { /* UTF-16LE */
                    encoded[2 * i] = u8(kMarker[i]);
                    encoded[2 * i + 1] = 0;
                } else { /* UTF-16BE */
                    encoded[2 * i] = 0;
                    encoded[2 * i + 1] = u8(kMarker[i]);
                }
            }
            enc_len = 2 * sizeof(kMarker);
        }
        const u64 scan_pages = 4;
        const u64 scan_bytes = scan_pages * page_size < end ? scan_pages * page_size : end;
        std::vector<u8> page(page_size);
        for (u64 off = 0; off + enc_len <= scan_bytes; off += page_size) {
            const u64 chunk = scan_bytes - off < page_size ? scan_bytes - off : page_size;
            if (!rd(s, hit + off, page.data(), u32(chunk)) || chunk < enc_len)
                break;
            for (u64 p = 0; p + enc_len <= chunk; ++p) {
                if (std::memcmp(page.data() + p, encoded, enc_len) == 0) {
                    r.note = "page-math+fcpevent";
                    off = scan_bytes; /* break the outer loop */
                    break;
                }
            }
        }
    }
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
    /* The og-style "reject > 4 GiB" ceiling is structurally unreachable
     * here: the RIFF size field is a u32, whose maximum (4 GiB - 1) is
     * already below the 4ull << 30 threshold the earlier `size >`
     * compared against - that comparison was always false and is dropped
     * (the size < 16 sanity floor stays). No accepted/rejected input set
     * changes. */
    if (size < 16)
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

/* S-049: the MP3 naming prefix - the ID3v2 title (TIT2 for v2.3/v2.4;
 * real-session pins: "AREE DEEWANO MUJHE PECHANO_000123.MP3", "Ek Lo Ek
 * Muft_N.MP3", "02 Prem Ratan Dhan Payo (SongsMp3.Com)_N.MP3" - spaces,
 * dots and parentheses are preserved verbatim). v2.4 frame sizes are
 * syncsafe; v2.3 plain BE u32. The text encoding byte leads the payload
 * (0 latin-1, 1 UTF-16/BOM, 2 UTF-16BE, 3 UTF-8); ASCII dominates the
 * observed titles. */
static std::u16string id3_title_prefix(IStorageDevice* s, u64 hit, u64 audio_at, u8 ver,
                                       std::string* counter_key = nullptr) {
    u64 pos = hit + 10; /* skip the ID3 header */
    int frames = 0;
    while (pos + 10 <= audio_at && frames < 512) {
        ++frames;
        u8 fh[10];
        if (!rd(s, pos, fh, 10))
            return {};
        if (fh[0] == 0)
            return {}; /* padding: no title found */
        char fid[5] = {char(fh[0]), char(fh[1]), char(fh[2]), char(fh[3]), 0};
        u32 fsize;
        if (ver == 4)
            fsize = (u32(fh[4]) << 21) | (u32(fh[5]) << 14) | (u32(fh[6]) << 7) | fh[7];
        else
            fsize = (u32(fh[4]) << 24) | (u32(fh[5]) << 16) | (u32(fh[6]) << 8) | fh[7];
        if (fsize == 0 || fsize > 4096)
            return {}; /* hostile/empty frame: no sane title walk */
        if (std::memcmp(fid, "TIT2", 4) == 0) {
            /* encoding byte + text, NUL-terminated */
            const u64 body = pos + 10 + 1;
            const u64 avail = audio_at > body ? audio_at - body : 0;
            const size_t want = size_t(std::min<u64>(avail, fsize - 1));
            if (want == 0)
                return {};
            std::string v(want, '\0');
            if (!rd(s, body, &v[0], u32(want)))
                return {};
            const size_t nul = v.find('\0');
            if (nul != std::string::npos)
                v.resize(nul);
            while (!v.empty() && v.back() == ' ')
                v.pop_back();
            if (v.empty())
                return {};
            if (counter_key != nullptr)
                *counter_key = v; /* the per-string counter key */
            return prefix_from_bytes(v);
        }
        pos += 10 + fsize;
    }
    return {};
}

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
    r.name_prefix = id3_title_prefix(s, hit, audio_at, ver_major, &r.name_counter_key); /* S-049 */
    return r;
}

/* ---- WAV: RIFF + WAVE + fmt chunk ---- */
/* S-049: the WAV naming prefix - the LIST/INFO INAM title (real-session
 * pins: "BIRD_N.WAV" x90, "RAIN_N.WAV" x60, "CAR_N.WAV" x50 in the I
 * session - game/audio-library INFO titles; 348 of 548 WAV rows carry no
 * INAM and take the plain WAV default, consistent with INAM being
 * optional metadata). RIFF chunks from hit+12: id(4) size(4 LE) data;
 * LIST with type INFO carries INAM subchunks. */
static std::u16string wav_inam_prefix(IStorageDevice* s, u64 hit, u64 end,
                                      std::string* counter_key = nullptr) {
    u64 pos = hit + 12;
    int chunks = 0;
    while (pos + 8 <= end && chunks < 512) {
        ++chunks;
        u8 ch[8];
        if (!rd(s, pos, ch, 8))
            return {};
        const u32 csz = u32(ch[4]) | (u32(ch[5]) << 8) | (u32(ch[6]) << 16) | (u32(ch[7]) << 24);
        if (csz > 0x7FFFFFFFu)
            return {}; /* hostile size */
        if (std::memcmp(ch, "LIST", 4) == 0 && csz >= 4 && pos + 12 <= end) {
            u8 lt[4];
            if (!rd(s, pos + 8, lt, 4))
                return {};
            if (std::memcmp(lt, "INFO", 4) == 0) {
                /* walk the INFO subchunks for INAM */
                u64 sub = pos + 12;
                const u64 send = std::min(pos + 8 + u64(csz), end);
                int subs = 0;
                while (sub + 8 <= send && subs < 64) {
                    ++subs;
                    u8 sc[8];
                    if (!rd(s, sub, sc, 8))
                        return {};
                    const u32 ssz =
                        u32(sc[4]) | (u32(sc[5]) << 8) | (u32(sc[6]) << 16) | (u32(sc[7]) << 24);
                    if (ssz > 4096)
                        return {};
                    if (std::memcmp(sc, "INAM", 4) == 0 && ssz > 0 && sub + 8 + ssz <= end) {
                        std::string v(size_t(ssz), '\0');
                        if (!rd(s, sub + 8, &v[0], ssz))
                            return {};
                        const size_t nul = v.find('\0');
                        if (nul != std::string::npos)
                            v.resize(nul);
                        while (!v.empty() && v.back() == ' ')
                            v.pop_back();
                        if (!v.empty()) {
                            if (counter_key != nullptr)
                                *counter_key = v; /* the per-string counter key */
                            return prefix_from_bytes(v);
                        }
                    }
                    sub += 8 + u64(ssz) + (ssz & 1); /* chunks are word-aligned */
                }
            }
        }
        pos += 8 + u64(csz) + (csz & 1); /* word-aligned stride */
    }
    return {};
}

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
    r.name_prefix = wav_inam_prefix(s, hit, r.end, &r.name_counter_key); /* S-049 */
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

/* ---- ASF/WMV: the BruteForce-family object walk (A-010) ----
 *
 * The og's BruteForce::CAsfAnalyser is a forward-walking structural
 * validator: it "walk[s] forward to EOF validating structure at every
 * step... keeps consuming until the grammar breaks, then reports the
 * last valid position as the file size... no premature truncation"
 * (rawsearch-carving--deep-dive.md:87-91; registry binding key
 * 0x75b22630 -> BruteForce::CAsfAnalyser, RAWSearchDLL decompile
 * :1914-1923; CAsfRawFileInfo's header sub-object GUID vocabulary
 * :41950-41965). The entry predicate stays the ASF Header Object GUID
 * + its gates (below); the END is now the last structurally-valid
 * object boundary - the pre-fix code ended the extent at the Header
 * Object's own size field, so every carved WMV/WMA/ASF was a
 * header-only fragment with none of the media payload (A-010). */
CarveResult validate_asf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    static const u8 asf_guid[16] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                    0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    u8 g[16];
    if (!rd(s, hit, g, 16))
        return r;
    if (std::memcmp(g, asf_guid, 16) != 0)
        return r;
    /* N-212: tier-1 is ONLY the 16-byte header GUID compare (FUN_180037030,
     * raw-asm verified: two sub/jne qword pairs, sete al - no other field is
     * read). The header object-count field (+0x18..+0x1B) is NEVER read by
     * the og's analyser family (verified: no +0x1c-indexed read in the ctor
     * :41948-41980, tier-1 :41927-41946, or walker :41994-42110) - the old
     * count==0/count>128 rejection refused the torn-header population the og
     * emits (recovery loss). The size field only BOUNDS the walk: a torn/
     * zero size emits the og's 30-byte salvage candidate (the header's
     * minimum structural footprint); a garbage-huge size is a walk cap, not
     * a rejection (the og's effective bound is the 100 GB product clamp -
     * 2 GiB is the core-scale stand-in at the 4 GiB row max). */
    const u64 obj_size = rd64(s, hit + 16);
    const u64 window_end = std::min<u64>(hit + cap, s->size());
    if (obj_size < 30) {
        r.ok = true;
        r.confidence = 650; /* salvage-class: GUID-only + torn size */
        r.end = std::min<u64>(hit + 30, window_end);
        r.note = "ASF-GUID-torn-size-salvage";
        return r;
    }
    const u64 header_end = hit + std::min<u64>(obj_size, 2ull << 30);
    r.ok = true;
    r.confidence = 850;

    /* N-212 phase 1: walk the header's children (from +0x1E to the header's
     * declared size, <= 400 children - the og's cap) capturing the File
     * Properties child's declared total file size. The og compares each
     * child's GUID against its precomputed pair at +0x174/+0x17c (ctor
     * :41970-41971: A1 DC AB 8C 47 A9 CF 11 8E E4 00 C0 0C 20 53 65) and
     * extracts the qword at child+0x28 via a 32-byte AVX load
     * (:42057-42058, stored at this+0x128) - the ASF spec's file_size
     * position (GUID 16 + size 8 + File ID 16 + File Size 8), the offset
     * cross-checked against the load shape. Unknown children are counted
     * and walked THROUGH (never a break - the og's +0x140 counter). */
    static const u8 fileprops_guid[16] = {0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11,
                                          0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
    u64 declared = 0;
    {
        const u64 phase1_end = std::min(header_end, window_end);
        u64 pos1 = hit + 30;
        u32 children = 0;
        while (pos1 + 24 <= phase1_end && children < 400) {
            if (!rd(s, pos1, g, 16))
                break;
            const u64 csz = rd64(s, pos1 + 16);
            if (csz < 24 || pos1 + csz > phase1_end)
                break; /* undersized / torn child ends the walk */
            if (csz >= 0x30 && std::memcmp(g, fileprops_guid, 16) == 0)
                declared = rd64(s, pos1 + 0x28);
            pos1 += csz;
            ++children;
        }
    }

    /* The top-level grammar: the ASF specification's five object types
     * after the header (Data, Simple Index, Index, Media Object Index x2
     * spellings - plus the Header itself for degenerate repeats). An
     * object is structurally valid when its GUID is one of these AND its
     * size is sane (>= its own 24-byte header, fits the window AND the
     * device - a torn tail ends the walk at the last COMPLETE object,
     * the family's salvage semantics). Anything else is the grammar
     * break: stop and report the last valid position. */
    static const u8 data_guid[16] = {0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                     0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    static const u8 simpidx_guid[16] = {0x90, 0x08, 0x00, 0x33, 0xB1, 0xE5, 0xCF, 0x11,
                                        0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB};
    static const u8 index_guid[16] = {0xD3, 0x29, 0xE2, 0xD6, 0xDA, 0x35, 0xD1, 0x11,
                                      0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE};
    static const u8 mediaidx1_guid[16] = {0xF8, 0x03, 0xB1, 0xFE, 0xBE, 0x57, 0xD2, 0x11,
                                          0xBB, 0x5B, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE};
    static const u8 mediaidx2_guid[16] = {0xAD, 0x3B, 0x20, 0x6B, 0x11, 0x3F, 0xE4, 0x48,
                                          0xAC, 0xA8, 0xD7, 0x61, 0x3D, 0xE2, 0xCF, 0xA7};
    u64 pos = std::min(header_end, window_end); /* first byte past the Header */
    while (pos + 24 <= window_end) {
        if (!rd(s, pos, g, 16))
            break;
        const u64 sz = rd64(s, pos + 16);
        if (sz < 24 || pos + sz > window_end)
            break; /* undersized / torn / non-fitting = grammar break */
        if (std::memcmp(g, asf_guid, 16) != 0 && std::memcmp(g, data_guid, 16) != 0 &&
            std::memcmp(g, simpidx_guid, 16) != 0 && std::memcmp(g, index_guid, 16) != 0 &&
            std::memcmp(g, mediaidx1_guid, 16) != 0 && std::memcmp(g, mediaidx2_guid, 16) != 0)
            break; /* unknown top-level object = grammar break */
        pos += sz;
    }
    /* N-212 extent finalization (the og's :42076-42086): the FIRST
     * non-family top-level object (or a torn tail / the window end)
     * finalizes the extent as max(FileProperties-declared size, the last
     * walked top-level position), then clamps - the og's product clamp is
     * global(+0x70) x 500 (the 200 MB default x 500 = 100 GB); the core's
     * stand-ins are the row max (already in `cap`) and the device end
     * (window_end). For intact files the declared size IS the walked end -
     * the two sources coincide; for padded/torn files the declared size
     * wins when the walk stopped early. */
    u64 end = pos;
    if (declared > 0)
        end = std::max<u64>(end, hit + declared);
    end = std::min<u64>(end, window_end);
    if (end <= hit)
        end = std::min<u64>(hit + 30, window_end); /* never a zero-extent emission */
    r.end = end;
    r.note = declared > 0 ? "ASF-GUID+object-walk+declared" : "ASF-GUID+object-walk";
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
        if (code == 0xBB) {                   /* system header: length follows */
            const u32 len = rdbe16(s, q + 4); /* N-155: BE u16 (the MPEG-PS wire form) */
            if (len < 6 || q + 6 + len > limit)
                break;
            q += 6 + len;
            ++elements;
            continue;
        }
        if ((code & 0xE0) == 0xE0 || (code & 0xC0) == 0xC0 || code == 0xBD || code == 0xBE) {
            /* stream packets + padding: PES length field */
            const u32 len = rdbe16(s, q + 4); /* N-155: BE u16 (the MPEG-PS wire form) */
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

/* ---- MPEG-TS / M2TS (S-047): the og's two list-layer sync scanners ----
 * BruteForce::CMTSAnalyser (id 0x76, registration :3143-3149) and
 * CM2TSAnalyser (id 0x74, :3173-3179) - no magic key in the og's
 * registries (the "numeric key 284" census row was the FreeHand-11
 * mis-lead, erratum E54); the analysers generate their own candidates
 * by chunked scanning. Core port: the candidate generation rides the
 * keyed dispatch's 1-byte 0x47 magic - FMT_TS anchors the sync at +0
 * (188-byte packets), FMT_M2TS at +4 (192-byte BDAV packets, the 4-byte
 * TP_extra_header in front). That mirrors the og's auto-detect order
 * (the chunk handler FUN_18002e430 :35578-35677 checks the sync at +4
 * FIRST, then at +0), and each family's grammar rejects the other's
 * streams within two packets (188- and 192-strides drift apart
 * immediately), so the N-070 best-pick resolves shared positions.
 *
 * The packet walk (FUN_180032460 :38457-38536, decoded first-hand):
 * stride the detected size, verify 0x47 at every packet boundary
 * (+0 in 188-mode, +4 in 192-mode), tolerate all-0xFF null packets
 * (bytes +8..+15 all 0xFF mark the null-packet flag - the run does NOT
 * end), end the run at the first sync loss committing the accumulated
 * extent, minimum extent 0x5000 (20 KiB) or size=0/reject, cap = the
 * controller global x 10 (the row's max_size), extension "M2TS" for
 * the 192-byte family (the og stamps L"M2TS" when the analyser id is
 * 0x74, :38531-38536). */
static CarveResult validate_ts_common(IStorageDevice* s, u64 hit, u64 cap, u32 stride, u32 sync_off,
                                      u16 conf, const char* note, const char* null_note) {
    CarveResult r;
    /* the og's detection gate: the byte AFTER the sync must be in
     * {0x40, 0x41, 'P', 0x10} (FUN_18002e430's byte-set) */
    u8 h[8];
    if (!rd(s, hit + sync_off, h, sizeof(h)))
        return r;
    if (h[0] != 0x47)
        return r;
    if (h[1] != 0x40 && h[1] != 0x41 && h[1] != 0x50 && h[1] != 0x10)
        return r;
    /* the block-buffered packet walk (the P1-12 discipline: probe
     * windows, boundary-straddling packets re-anchored at the next
     * block; each packet costs one sync-byte compare at its boundary).
     * Progressive reads: the FIRST block is 2 KiB - a 1-byte magic
     * means every 0x47+follow-byte pair in the window reaches this
     * walk, and noise candidates (no second synced packet) must die
     * for 2 KiB of traffic, never a full 1 MiB probe block. Only a
     * candidate whose first ~10 packets all sync graduates to the
     * kProbeBlock ladder (the run needs 109+ packets to clear 0x5000,
     * so the small block can never accept - it only cheap-rejects). */
    constexpr u32 kTSFirstBlock = 2048;
    const u64 limit = probe_limit(s, hit, cap);
    u64 verified_end = hit; /* end of the last fully verified packet */
    u64 pos = hit;          /* next unverified packet boundary (absolute) */
    bool null_packets = false;
    std::vector<u8> buf(kProbeBlock);
    bool first_block = true;
    while (pos + stride <= limit) {
        const u64 remain = limit - pos;
        const u32 take = first_block ? u32(std::min<u64>(remain, kTSFirstBlock))
                                     : u32(std::min<u64>(remain, kProbeBlock));
        if (take < stride)
            break; /* the tail cannot hold another packet */
        if (!rd(s, pos, buf.data(), take))
            break; /* unreadable: the run ends at the last verified packet */
        u32 off = 0;
        while (off + stride <= take) {
            if (buf[off + sync_off] != 0x47)
                break; /* sync loss: the run ends at the last verified packet */
            /* null-packet tolerance: the og checks the packet's bytes
             * +8..+15 (the second u64) for all-0xFF and marks (but does
             * not end) the run - FUN_180032460's
             * ((u64*)(iVar8+param_2))[1] == -1 */
            if (off + 16 <= take) {
                bool all_ff = true;
                for (u32 b = 8; b < 16; ++b)
                    if (buf[off + b] != 0xFF) {
                        all_ff = false;
                        break;
                    }
                if (all_ff)
                    null_packets = true;
            }
            off += stride;
        }
        verified_end = pos + off;
        if (off + stride <= take)
            break;  /* sync loss inside this block */
        pos += off; /* the straddling packet anchors the next block */
        first_block = false;
    }
    const u64 run = verified_end - hit;
    if (run < 0x5000)
        return r; /* the og's minimum extent: size = 0 / reject */
    r.ok = true;
    r.end = verified_end;
    r.confidence = conf;
    /* the null-packet flag (the og's +0x11c marker, FUN_180032460
     * :38472-75) rides the evidence note - the N-089 channel - with a
     * "+null" marker; its og consumer is undecoded, so it changes no
     * acceptance/size semantics */
    r.note = null_packets ? null_note : note;
    return r;
}

/* ---- HTML (M-092): the og's four u16 keys route one analyser chain ----
 * Registrations: u16 `<h` 0x683c / `<H` 0x483c / `<!` 0x213c (:2849/:2858/
 * :2867) + the UTF-16LE BOM chain 0xfeff (:2876, census erratum E59) ->
 * RawSpeed::HTMLAnalyser + FileHTMAnalyser, id 0x21 (:14263-14281).
 * Probe (FUN_180012270 :14307-14380): ASCII - case-insensitive
 * "<HTML" (5 bytes) or "<!DOCTYPE HTML" (14 bytes); BOM chain -
 * dword@0 == 0x003CFEFF (FF FE 3C 00) AND (the wide "<HTML>" at +2
 * (6 chars, case-insensitive) OR the wide char at +4 == '!').
 * Size (FUN_180042e60 :50845-50884): the chunked "</html>" scan - a
 * 7-byte ASCII needle ONLY (no wide needle exists anywhere in the DLL:
 * grep for the wide-form constants returns zero sites; a UTF-16 file
 * without an ASCII needle falls to the cap), cap 0x100000, then sector
 * round-up; no marker within the cap -> the bounded cap (the
 * RTF/slab-end family, the batch-1 convention). FileInfo bounds
 * {20, 0x100000} (:14395-14396). */
static bool ci_ascii(const u8* p, const char* want, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        char c = char(p[i]);
        if (c >= 'a' && c <= 'z')
            c = char(c - 'a' + 'A');
        if (c != want[i])
            return false;
    }
    return true;
}

static bool ci_wide(const u8* p, const wchar_t* want, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        char c = char(p[2 * i]);
        if (c >= 'a' && c <= 'z')
            c = char(c - 'a' + 'A');
        if (c != char(want[i]) || p[2 * i + 1] != 0)
            return false;
    }
    return true;
}

/* The og's shared end-marker scan (FUN_180054470 :64131-64160) is
 * TWO-VARIANT case-aware: it lowercases the needle (_mbslwr_s) and
 * scans, then uppercases it (_mbsupr_s) and scans again - it finds the
 * all-lowercase and the all-uppercase forms, never mixed case. The
 * chunked walk: probe blocks, needle-1 overlap so a marker straddling
 * a block edge is found. `needle` arrives lowercase; the upper twin is
 * derived here. Returns the offset just past the FIRST matching
 * variant, 0 when none is found within the limit. */
static u64 scan_tag_two_case(IStorageDevice* s, u64 start, u64 limit, const char* needle) {
    const size_t n = std::strlen(needle);
    if (n == 0 || limit <= start + n)
        return 0;
    char up[16];
    if (n > sizeof(up) - 1)
        return 0; /* the family's needles are short; guard anyway */
    for (size_t i = 0; i < n; ++i)
        up[i] = (needle[i] >= 'a' && needle[i] <= 'z') ? char(needle[i] - 'a' + 'A') : needle[i];
    up[n] = 0;
    std::vector<u8> buf(kProbeBlock);
    u64 block = start;
    while (block < limit) {
        const u64 remain = limit - block;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < n)
            break;
        if (!rd(s, block, buf.data(), take))
            break;
        for (u32 i = 0; i + n <= take; ++i) {
            bool hit_l = true, hit_u = true;
            for (u32 b = 0; b < n; ++b) {
                if (buf[i + b] != u8(needle[b]))
                    hit_l = false;
                if (buf[i + b] != u8(up[b]))
                    hit_u = false;
                if (!hit_l && !hit_u)
                    break;
            }
            if (hit_l || hit_u)
                return block + i + n; /* just past the marker */
        }
        if (block + take >= limit)
            break;
        block += u64(take) - (n - 1); /* needle-1 overlap */
    }
    return 0; /* not found within the cap */
}

CarveResult validate_html(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[16];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    bool wide = false;
    if (ci_ascii(h, "<HTML", 5) || ci_ascii(h, "<!DOCTYPE HTML", 14)) {
        wide = false;
    } else if (u32_from_bytes_le(h) == 0x003CFEFF &&
               (ci_wide(h + 2, L"<HTML>", 6) || (h[4] == 0x21 && h[5] == 0))) {
        wide = true; /* the UTF-16LE BOM chain */
    } else {
        return r; /* the probe's three acceptance forms (plus the wide-'!' form) */
    }
    const u64 limit = probe_limit(s, hit, cap); /* cap = 1 MiB */
    const u64 marker_end = scan_tag_two_case(s, hit, limit, "</html>");
    /* FUN_180042e60's sector round-up (:50880-50884): the size rounds up
     * to the storage cell's logical sector (512 when the device reports
     * none) - applied to both the marker walk and the cap fallback,
     * clamped to the slab end (the core's node extents stay inside the
     * device) */
    const u64 sec = s->geometry().logical_sector >= 1 ? u64(s->geometry().logical_sector) : 512;
    u64 end = (marker_end != 0) ? marker_end : limit;
    if (sec > 1)
        end = ((end + sec - 1) / sec) * sec;
    if (end > limit)
        end = limit;
    if (marker_end != 0) {
        r.ok = true;
        r.end = end;
        r.confidence = 700;
        r.note = wide ? "html-endtag-walk+utf16" : "html-endtag-walk";
        return r;
    }
    /* no marker within the cap: the bounded fallback (the og's shared
     * chunk scan leaves the size at the cap) with the heuristic note */
    r.ok = true;
    r.end = end;
    r.confidence = 500;
    r.note = wide ? "html-bounded-cap+utf16" : "html-bounded-cap";
    return r;
}

/* ---- XML / SVG (M-093): one og chain, four u32 keys, two probe ids ----
 * The four registrations (:1806-1837) all construct the
 * Brance::SVGAnalyser chain: "<?xm" 0x6d783f3c, the UTF-8 BOM+'<'
 * 0x3cbfbbef (EF BB BF 3C), the numeric key 0x000001cd (bytes CD 01 00
 * 00 - decoded from .data DAT_180093940 this session; the census's
 * "undecoded semantics" resolved: it is a raw dword magic whose files
 * stamp SVG directly), and "<svg" 0x6776733c. The probe
 * (FUN_180041670 :49622-49703): a CASE-SENSITIVE declaration gate
 * ("<?xml version=\"" 15 bytes, at +0 or +3 for the BOM form), then
 * the first 512 bytes are strstr'd for the six <svg needles (decoded
 * from .data: "<svg " / "<svg\n " / "<svg\r " / "<svg> " / "<svg>\n"
 * / "<svg>\r") -> SVG id 0x5b; else the "csl" marker (DAT_180093968 =
 * the Corel symbol-library string) -> generic XML id 0x13. Direct key
 * paths (no declaration): dword@0 == 0x1cd, or "<svg " (dword "<svg"
 * + byte ' ') -> SVG. SVGRawFileInfo bounds {0, 0x100000} (:49721+
 * FUN_180041880). Size: the </svg> family end-tag walk (the
 * .data 0x180093950 string) - the two-case scan, 1 MiB cap, NO sector
 * round-up (FUN_180041950's tail sets the size verbatim; the HTML twin
 * FUN_180042e60 is the round-up exception). The walker's 0x44c branch
 * (the CD01-key leg's own size field, dword@0x44c + 0x480 within the
 * cap) LANDED with M-112 - see the branch below. */
CarveResult validate_xmlsvg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 win[513];
    u64 n = 0;
    if (s->size() > hit) {
        n = std::min<u64>(512, s->size() - hit);
        if (!rd(s, hit, win, u32(n)))
            return r;
    }
    win[n] = 0;
    bool svg = false, generic_xml = false;
    const bool decl0 = n >= 15 && std::memcmp(win, "<?xml version=\"", 15) == 0;
    const bool decl3 = n >= 18 && std::memcmp(win + 3, "<?xml version=\"", 15) == 0;
    if (decl0 || decl3) {
        static const char* const kSvgNeedles[6] = {
            "<svg ", "<svg\n ", "<svg\r ", "<svg> ", "<svg>\n", "<svg>\r",
        };
        for (const char* nd : kSvgNeedles) {
            if (std::strstr(reinterpret_cast<const char*>(win), nd) != nullptr) {
                svg = true;
                break;
            }
        }
        if (!svg && std::strstr(reinterpret_cast<const char*>(win), "csl") != nullptr)
            generic_xml = true; /* the Corel marker -> id 0x13 */
    } else if (n >= 4 && u32_from_bytes_le(win) == 0x000001cdu) {
        svg = true; /* the numeric key's direct path */
    } else if (n >= 5 && std::memcmp(win, "<svg", 4) == 0 && win[4] == ' ') {
        svg = true; /* the "<svg " direct path */
    }
    if (!svg && !generic_xml)
        return r;
    /* M-112: the og's CSL size branch (FUN_180041950 :49749-49828, the
     * Brance::SVGAnalyser walker's CD 01 00 00 leg - the DAT_180093918
     * latch, first-hand + binary-dumped this session: the constant IS
     * 0x1CD): when the file spans offset 0x44c with the full dword
     * in-window, the LE dword there + 0x480 IS the extent (assigned
     * verbatim at +0x10), gated by the family cap (the SVGRawFileInfo
     * ctor's +0x118 = 0x100000, FUN_180041880 :17291-17306 - the
     * finding's open question closed by the ctor read). FINDING-TEXT
     * CORRECTION: the branch belongs to the CD 01 00 00 KEY leg, which
     * the PROBE stamps 0x5b (DAT_180093940 = 0x1CD, the same dword) -
     * NOT to the "csl"-content strstr leg (id 0x13, whose first dword
     * is "<?xm" and never fires the walker's latch). The override keeps
     * the probe's stamp. M-140: Declination (short file / out-of-cap /
     * zero result) DROPS the candidate - the og's +0x120 latch
     * (FUN_180041950 :49766-49771, re-read first-hand) permanently binds
     * CD01-key candidates to the 0x44c sizing; every decline arm goto
     * EXIT with the size slot (+0x10) unset and NO retry through the
     * end-tag branch (which serves the OTHER SVG legs only). The M-112
     * landing's "falls to the end-tag walk" claim is corrected here and
     * in the M-112 Resolution's erratum note. */
    if (n >= 4 && u32_from_bytes_le(win) == 0x000001cdu) {
        const u64 avail = s->size() > hit ? s->size() - hit : 0;
        if (avail >= 0x450) {
            u8 f[4];
            if (rd(s, hit + 0x44c, f, sizeof(f))) {
                const u64 field_size = u64(u32_from_bytes_le(f)) + 0x480;
                if (field_size != 0 && field_size <= cap) {
                    const u64 limit0 = probe_limit(s, hit, cap);
                    r.ok = true;
                    r.end = std::min(hit + field_size, limit0);
                    r.format_override = u32(FMT_SVG); /* the probe's stamp for the CD01 key: 0x5b */
                    r.confidence = 700;
                    r.note = "csl-sizefield@0x44c";
                    return r;
                }
            }
        }
        /* M-140: the declined read ends the candidate - no end-tag walk,
         * no bounded fallback (the og's latch semantics: unsized
         * candidates are dropped by the pending model). */
        return r;
    }
    const u64 limit = probe_limit(s, hit, cap); /* cap = 1 MiB */
    const u64 marker_end = scan_tag_two_case(s, hit, limit, "</svg>");
    /* the og stamps the FORMAT ID at the probe (id 0x5b vs 0x13 depends
     * on CONTENT, not on the dispatch key) - carried via the override */
    r.format_override = svg ? u32(FMT_SVG) : u32(FMT_XML);
    if (marker_end != 0) {
        r.ok = true;
        r.end = marker_end;
        r.confidence = 700;
        r.note = svg ? "svg-endtag-walk" : "xml-csl-endtag-walk";
        return r;
    }
    /* no marker within the cap: the bounded fallback (the family
     * convention), the heuristic note */
    r.ok = true;
    r.end = limit;
    r.confidence = 500;
    r.note = svg ? "svg-bounded-cap" : "xml-bounded-cap";
    return r;
}

/* ---- MOBI (M-096): the og's full-salvage PalmDB gate ----
 * MOBIAnalyser (id 0x85), list-registered behind the RawConfig "MOBI"
 * token (:3600-3615 - the SHIPPED RawConfig.ini carries MOBI=1, the
 * config-files artifact's [Filter] decode: enabled by default). The
 * probe (FUN_180025bc0 :29369-29431) validates ONLY the PalmDB
 * type/creator pair: BE32 "BOOK" at +0x3C and BE32 "MOBI" at +0x40 -
 * the 0x3C bytes before it are NEVER examined (a deliberately weak,
 * full-salvage gate: fragmented or header-damaged books still carve;
 * ported EXACTLY - strengthening it would diverge). Size: the
 * CalculateSize_EndTag walker (the ctor descriptor at :29436-29485:
 * value 0xe98e0d0a LE-serialized = the bytes 0A 0D 8E E9, length 4,
 * step 0x200, cap 0x1f400000 = 500 MB) - the needle is checked at
 * 512-byte steps from the file start; no needle within the cap -> the
 * bounded fallback (the family convention) with the heuristic note. */
CarveResult validate_mobi(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 pair[8];
    if (!rd(s, hit + 0x3C, pair, sizeof(pair)))
        return r;
    if (std::memcmp(pair, "BOOKMOBI", 8) != 0)
        return r;                               /* the PalmDB type/creator pair - the ONLY gate */
    const u64 limit = probe_limit(s, hit, cap); /* cap = 500 MB */
    /* the 512-step end-tag walk, block-buffered (kProbeBlock is an
     * exact multiple of the step, so every block starts step-aligned
     * and no step position straddles) */
    constexpr u64 kStep = 0x200;
    std::vector<u8> buf(kProbeBlock);
    u64 pos = hit;
    u64 marker_end = 0;
    while (pos + 4 <= limit) {
        const u64 remain = limit - pos;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < 4)
            break;
        if (!rd(s, pos, buf.data(), take))
            break;
        for (u64 off = 0; off + 4 <= take; off += kStep) {
            if (buf[off] == 0x0A && buf[off + 1] == 0x0D && buf[off + 2] == 0x8E &&
                buf[off + 3] == 0xE9) {
                marker_end = pos + off + 4;
                break;
            }
        }
        if (marker_end != 0)
            break;
        pos += (u64(take) / kStep) * kStep; /* whole steps only */
    }
    if (marker_end != 0) {
        r.ok = true;
        r.end = marker_end;
        r.confidence = 700;
        r.note = "mobi-endtag-walk";
        return r;
    }
    /* no end tag within the cap: the bounded fallback */
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "mobi-bounded-cap";
    return r;
}

/* ---- JP2 family (N-111): the og's JP2Analyser, the full decode ----
 * JP2Analyser (id 0x80, the RawConfig "JP2" token, list-layer
 * :3544-3559). The probe (FUN_18001c380 :21812-21936) gates exactly:
 * BE u32@+4 == 0x6a502020 ("jP  " - the signature-box TYPE; the box
 * length u32@+0 is never read - the corrupt-length salvage posture),
 * BE u32@+8 == 0x0d0a870a (the signature-box payload), BE u32@+0x10 ==
 * 0x66747970 ("ftyp" - the file-type box must directly follow). The
 * major brand BE u32@+0x14 then walks a std::map built once per
 * process (:21886-21936 - lower_bound + key<=probe, i.e. EXACT match):
 * "jp2 " -> id 0x80, "jpx " -> 0x81, "jpm " -> 0x82; every other brand
 * leaves the walk unmatched and the probe RETURNS 0 (reject).
 *
 * The size machinery (FUN_18001c760/c820 :21979-22115, decoded
 * first-hand this session - the finding's own text had recorded it as
 * the undecoded follow-up; no stand-in needed):
 *   phase 1 - the box walk: read BE u32 length@+0 and BE u32 type@+4;
 *     len==1 -> the u64 extended length at +8 (16-byte box header);
 *     len==0 (literal or extended) -> phase 2 from box_start+4;
 *     length with bit63 set -> size 0 (the defensive reject);
 *     the box type "jp2c" (the expected-end map, FUN_18001ca60: all
 *     three ids map to "jp2c") latches the codestream flag, and the
 *     FIRST NON-jp2c-typed box after it ends the extent (the
 *     jp2c-consecutive rule - the og's end detector for the common
 *     jp2c-final files).
 *   phase 2 - the codestream byte scan (the unbounded len==0 form):
 *     from box_start+4, scan bytes for the FF D9 EOC u16 (the og reads
 *     *(u16*)pos == 0xD9FF - little-endian FF D9); a run of 16+
 *     consecutive zero bytes latches a back-off size (run_start) exactly
 *     once - the trailing-pad heuristic; the absolute guard 0x1F3FFFFF
 *     finalizes; EOC beats the latch.
 * Single-pass adaptation: the walk runs to the slice cap; no
 * terminator -> the latched back-off size if one fired, else the
 * bounded fallback (the family convention) with the heuristic note. */
namespace {
CarveResult jp2_eoc_scan(IStorageDevice* s, u64 hit, u64 start, u64 limit, CarveResult r) {
    /* the og's phase 2, block-buffered (one extra byte per block so
     * every position's u16 successor is in-buffer - no straddle miss) */
    std::vector<u8> buf(kProbeBlock + 1);
    u64 pos = start;
    u32 zero_run = 0;
    bool latched = false;
    u64 latched_end = 0;
    while (pos < limit) {
        const u64 remain = limit - pos;
        const u32 take = u32(std::min<u64>(remain, kProbeBlock));
        if (take < 2)
            break;
        /* one extra byte past the block so every position's u16 successor
         * is in-buffer (at the slice edge the successor is unreadable and
         * that final position's u16 check is skipped - the og's own
         * window-edge behavior); the extra byte re-reads as the next
         * block's buf[0], so the zero-run never double-counts */
        const u32 want = u32(std::min<u64>(remain, u64(take) + 1));
        if (!rd(s, pos, buf.data(), want))
            break;
        for (u32 i = 0; i < take; ++i) {
            const u64 abs = pos + i;
            if (abs - hit > 0x1F3FFFFFull) {
                /* the og's absolute guard: finalize with whatever the
                 * scan has recorded */
                if (latched) {
                    r.ok = true;
                    r.end = latched_end;
                    r.confidence = 550;
                    r.note = "jp2-eoc-zeros-backoff";
                    return r;
                }
                r.ok = true;
                r.end = abs;
                r.confidence = 450;
                r.note = "jp2-bounded-cap";
                return r;
            }
            const u8 b = buf[i];
            zero_run = (b != 0) ? 0 : zero_run + 1;
            if (zero_run > 15 && !latched) {
                latched = true;
                latched_end = abs - 15; /* the run's first zero - the og's
                                         * lVar5 - 0xf, recorded once */
            }
            if (i + 1 < want && b == 0xFF && buf[i + 1] == 0xD9) {
                r.ok = true;
                r.end = abs + 2;
                r.confidence = 700;
                r.note = "jp2-eoc-scan";
                return r;
            }
        }
        pos += take;
    }
    /* no EOC by the cap: the latched back-off if any, else the family
     * fallback */
    if (latched) {
        r.ok = true;
        r.end = latched_end;
        r.confidence = 550;
        r.note = "jp2-eoc-zeros-backoff";
        return r;
    }
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "jp2-bounded-cap";
    return r;
}
} // namespace

CarveResult validate_jp2(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 hdr[0x18];
    if (!rd(s, hit, hdr, sizeof(hdr)))
        return r;
    const u32 sig = (u32(hdr[4]) << 24) | (u32(hdr[5]) << 16) | (u32(hdr[6]) << 8) | u32(hdr[7]);
    const u32 payload =
        (u32(hdr[8]) << 24) | (u32(hdr[9]) << 16) | (u32(hdr[10]) << 8) | u32(hdr[11]);
    const u32 ftyp =
        (u32(hdr[0x10]) << 24) | (u32(hdr[0x11]) << 16) | (u32(hdr[0x12]) << 8) | u32(hdr[0x13]);
    if (sig != 0x6a502020u || payload != 0x0d0a870au || ftyp != 0x66747970u)
        return r; /* the three probe gates, ported exactly */
    const u32 brand =
        (u32(hdr[0x14]) << 24) | (u32(hdr[0x15]) << 16) | (u32(hdr[0x16]) << 8) | u32(hdr[0x17]);
    u32 fmt = 0xFFFFFFFFu;
    if (brand == 0x6a703220u)
        fmt = u32(FMT_JP2); /* "jp2 " -> the og's id 0x80 */
    else if (brand == 0x6a707820u)
        fmt = u32(FMT_JPX); /* "jpx " -> 0x81 */
    else if (brand == 0x6a706d20u)
        fmt = u32(FMT_JPM); /* "jpm " -> 0x82 */
    if (fmt == 0xFFFFFFFFu)
        return r; /* the exact-match map walk rejects unknown brands */
    r.format_override = fmt;
    const u64 limit = probe_limit(s, hit, cap);
    constexpr u32 kJp2c = 0x6a703263u; /* "jp2c" - the codestream box */
    u64 pos = hit;
    bool seen_jp2c = false;
    while (pos + 8 <= limit) {
        const u32 len = rdbe32(s, pos);
        const u32 type = rdbe32(s, pos + 4);
        if (seen_jp2c && type != kJp2c) {
            /* the jp2c-consecutive rule: the first non-codestream box
             * after a jp2c box ends the extent */
            r.ok = true;
            r.end = pos;
            r.confidence = 700;
            r.note = "jp2-box-walk";
            return r;
        }
        u64 blen = len;
        if (len == 1) {
            if (pos + 16 > limit)
                break; /* the extended form is unreadable - the walk ends */
            blen = rdbe64(s, pos + 8);
        }
        if (blen == 0)
            return jp2_eoc_scan(s, hit, pos + 4, limit, r); /* the og's +=4 scan start */
        if ((blen >> 63) != 0) {
            r.ok = true;
            r.end = hit; /* size 0 - the og's defensive reject shape */
            r.confidence = 300;
            r.note = "jp2-invalid-box-length";
            return r;
        }
        if (type == kJp2c)
            seen_jp2c = true;
        pos += blen;
    }
    /* the walk ran to the cap without a terminator (a jp2c-final file at
     * the slice edge takes this path - the fallback end IS the file end
     * there): the bounded fallback, the family convention */
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "jp2-bounded-cap";
    return r;
}

/* ---- LZH family (N-112): the og's entry-chain walk, the full decode ----
 * Brance::LZHAnalyser + FileLZHAnalyser (id 0x24, LIST-layer; erratum
 * E53 - no u32 key, the E4 52 5C 7B census row binds OneNote). The
 * probe (FUN_18003db20 :46937-46973) gates the level-1 method field:
 * *(u32*)(buf+2) in {"-lh0" 0x30686c2d, "-lh5" 0x35686c2d, "-lhd"
 * 0x64686c2d, "-lh7" 0x37686c2d} AND buf[6] == '-' (the registry magic
 * carries the 5-byte "-lh?-" form of this gate).
 *
 * The size walk (FUN_18003dd70 :47045-47138, decoded first-hand; the
 * .data method constants dumped from the binary at RVA 0x9388c -
 * "-lh0"/"-", "-lh5"/"-", "-lhd"/"-", "-lh7"/"-"):
 *   -lh0 / -lhd : pos += hdr_u8@0 + comp_u32@7 + 2 (the CRC; the +2
 *                 suppressed once the ext flag latched), CRC flag set
 *   -lh5        : pos += hdr_u8@0 ONLY, ext flag latched
 *   -lh7        : pos += hdr_u8@0 + comp_u32@7, ext flag latched
 *   no match    : the -2/+2 step-back dance (a latched CRC flag steps
 *                 pos back 2 and re-examines; the restore path adds it
 *                 back), then the chain ends with size = pos + 1 (the
 *                 single 0x00 end-marker byte - the level-1 archive
 *                 terminator)
 *   0x401 iterations per window (the counter is a local - reset per
 *   call; the walk state persists across the og's scan windows); the
 *   FileInfo ctor bound 0xC800000 (200 MiB).
 * The per-method strides are the og's EXACT behavior as decoded -
 * including the -lh5 hdr-only stride (a deliberate port-not-fix: the
 * og's walk lands on the CRC field after a -lh5 entry in a level-1
 * archive and breaks the chain there; parity means porting it, and the
 * LzhLh5HeaderOnlyStridePinned test pins the quirk against future
 * "fixes"). Single-pass adaptation: the chain runs to the slice cap; a
 * chain that never breaks takes the bounded fallback (the family
 * convention); the iteration bound is the defensive 2M (the og's own
 * total is bounded only by the 200 MiB cap amortized across windows -
 * no real archive approaches either bound; the guard exists so
 * degenerate crafted strides cannot spin the single-pass walk). */
CarveResult validate_lzh(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    const u64 limit = probe_limit(s, hit, cap); /* cap = 200 MiB, the og ctor bound */
    /* block-buffered entry reads: one 64 KiB window covers ~6K entries;
     * refill when the 11-byte entry header is not fully in-buffer (the
     * step-back dance may re-read behind the buffer - refill handles it) */
    constexpr u32 kEntryHdr = 11; /* hdr u8@0, method@2..6, comp u32@7 */
    std::vector<u8> buf(1u << 16);
    u64 buf_base = ~(u64)0;
    u32 buf_len = 0;
    auto entry_at = [&](u64 pos, u8* out) -> bool {
        if (pos + kEntryHdr > limit)
            return false;
        if (pos < buf_base || pos + kEntryHdr > buf_base + buf_len) {
            const u64 want = std::min<u64>(limit - pos, buf.size());
            if (want < kEntryHdr)
                return false;
            if (!rd(s, pos, buf.data(), u32(want)))
                return false;
            buf_base = pos;
            buf_len = u32(want);
        }
        std::memcpy(out, buf.data() + (pos - buf_base), kEntryHdr);
        return true;
    };
    u64 pos = hit;
    bool ext = false;  /* the og's flag +0x120: latched by -lh5/-lh7 */
    bool crcf = false; /* the og's flag +0x124: the +2 stride latched */
    bool back = false; /* the og's bVar3: a step-back happened */
    u64 iters = 0;
    constexpr u32 kLh0 = 0x30686c2du, kLh5 = 0x35686c2du, kLhd = 0x64686c2du, kLh7 = 0x37686c2du;
    u8 e[kEntryHdr];
    while (true) {
        if (++iters > 2000000ull)
            break; /* the defensive bound (see the header comment) */
        if (!entry_at(pos, e))
            break; /* the chain runs off the cap/slice edge - the fallback */
        const u32 method = u32_from_bytes_le(e + 2);
        const u32 comp = u32_from_bytes_le(e + 7);
        const u32 hdr = e[0];
        /* the probe's second gate (buf[6]=='-') re-run per entry, then the
         * per-method dispatch: 1 = -lh0/-lhd, 2 = -lh5, 3 = -lh7 */
        int kind = 0;
        if (e[6] == '-') {
            if (method == kLh0 || method == kLhd)
                kind = 1;
            else if (method == kLh5)
                kind = 2;
            else if (method == kLh7)
                kind = 3;
        }
        if (kind == 0) {
            /* the chain-break dance (the og's else branch) */
            if (crcf) {
                /* the step-back: the latched +2 may have overshot the last
                 * entry's CRC - re-examine 2 bytes earlier */
                pos -= 2;
                crcf = false;
                back = true;
                continue;
            }
            if (back)
                pos += 2; /* the restore */
            r.ok = true;
            r.end = pos + 1; /* the end-marker byte included */
            r.confidence = 700;
            r.note = "lzh-entry-chain";
            return r;
        }
        if (kind == 1) {
            if (!ext) {
                crcf = true;
                pos += u64(hdr) + comp + 2;
            } else {
                pos += u64(hdr) + comp;
            }
        } else if (kind == 2) {
            pos += hdr;
            ext = true;
        } else { /* kind == 3 */
            pos += u64(hdr) + comp;
            ext = true;
        }
        if (pos > limit) {
            /* the chain passed the cap mid-walk (the og's per-window
             * return-1 path): the bounded fallback */
            break;
        }
    }
    /* no chain break: the bounded fallback (the family convention) */
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "lzh-chain-cap";
    return r;
}

/* ---- N-113: the text school - the og's small text-format list probes ----
 * Five RawConfig-token-gated list analysers, every probe + walk re-verified
 * first-hand this session (the corpus line cites in cr_signatures.h's
 * N-113 block). Shared shape: a chunked needle scan; the found extent per
 * the decoded formula; the family's bounded fallback at the cap.
 *
 * The og's 0x200-chunk finds are PER-CHUNK (each 512-byte block is copied
 * into a fixed buffer and std::string-searched independently - a needle
 * straddling a 512-byte boundary is MISSED by the og); ported exactly
 * here: chunk_find searches within one chunk only. VCF's loop over-reads
 * 10 bytes past its 0x400 advance (the u64 compare chain reads
 * [chunk, chunk+0x40A]) - also ported. The og's 64 KiB sliding-window
 * trailing-NUL trim (the >16-zero tail check that detects the device end)
 * is adapted as the bounded fallback at the slice cap (the M-092/M-093
 * convention for this school's walks). */
namespace {
/* case-insensitive ASCII compare (the og's _memicmp) */
bool ci_eq(const u8* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        u8 x = a[i], y = u8(b[i]);
        if (x >= 'A' && x <= 'Z')
            x = u8(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z')
            y = u8(y - 'A' + 'a');
        if (x != y)
            return false;
    }
    return true;
}

/* the per-chunk needle search: returns the offset of the needle WITHIN
 * [chunk_start, chunk_start + chunk + tail) if fully inside one chunk
 * (tail > 0 only for VCF's over-read form), or npos */
constexpr u64 kNpos = ~0ull;
u64 chunk_find(IStorageDevice* s, u64 chunk_start, u64 chunk, u64 tail, const char* needle,
               u32 needle_len, u8* buf, u32 buf_cap) {
    const u64 span = chunk + tail;
    if (span > buf_cap || span < needle_len)
        return kNpos;
    const u64 readable = std::min<u64>(span, s->size() - chunk_start);
    if (readable < needle_len)
        return kNpos;
    const u32 take = u32(std::min<u64>(readable, span));
    if (!rd(s, chunk_start, buf, take))
        return kNpos;
    const u64 limit = take >= needle_len ? take - needle_len + 1 : 0;
    for (u64 i = 0; i < limit; ++i)
        if (std::memcmp(buf + i, needle, needle_len) == 0)
            return i;
    return kNpos;
}
} // namespace

CarveResult validate_vcf(IStorageDevice* s, u64 hit, u64 cap) {
    /* CVCFAnalyser (id 0x88): the probe's BEGIN:VCARD gate is carried by
     * the row magic. The walk (FUN_18003ba40 :45363-45452): scan 0x400
     * chunks (the u64/u16/byte compare chain over-reads 10 bytes past the
     * advance) for the exact-case "END:VCARD\r\n"; on find: extent += find
     * + 11, then the multi-card continuation - the next 11 bytes
     * memicmp-compared against "BEGIN:VCARD" (case-insensitive): a match
     * resumes the walk at the new extent; otherwise the walk ends. Cap
     * 0x1400000 (the loop bound 0x1400001's floor value). */
    CarveResult r;
    const u64 limit = probe_limit(s, hit, cap); /* cap = 20 MiB */
    std::vector<u8> buf(0x40A + 16);
    u64 size = 0;
    while (size < limit) {
        const u64 chunk_abs = hit + size;
        const u64 rel =
            chunk_find(s, chunk_abs, 0x400, 0x0A, "END:VCARD\r\n", 11, buf.data(), u32(buf.size()));
        if (rel != kNpos) {
            size += rel + 11;
            /* the multi-card continuation: BEGIN:VCARD at the new extent
             * (the og's memicmp) */
            u8 next[11];
            if (size + 11 <= limit && rd(s, hit + size, next, 11) && ci_eq(next, "BEGIN:VCARD", 11))
                continue; /* the next card - keep walking */
            r.ok = true;
            r.end = hit + size;
            r.confidence = 700;
            r.note = "vcf-endtag-walk";
            return r;
        }
        size += 0x400;
    }
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "vcf-bounded-cap";
    return r;
}

CarveResult validate_gpx(IStorageDevice* s, u64 hit, u64 cap) {
    /* CGPXAnalyser (id 0x97): the probe (FUN_18001b0a0 :20860-20936)
     * gates the 15-byte declaration "<?xml version=\"" at 0 or +3 (the
     * UTF-8 BOM form - the two registry rows), then scans for the u32
     * "<gpx" in [+0x16, +0x216). The walk (FUN_18001b220): 0x200 chunks,
     * "</gpx>" needle, extent = find + 6; cap 0x6400000 (the id != 0xa0
     * branch). */
    CarveResult r;
    u8 head[0x21A];
    const u64 avail = std::min<u64>(sizeof(head), s->size() > hit ? s->size() - hit : 0);
    if (avail < 15)
        return r;
    if (!rd(s, hit, head, u32(avail)))
        return r;
    if (std::memcmp(head, "<?xml version=\"", 15) != 0 &&
        !(avail >= 18 && std::memcmp(head + 3, "<?xml version=\"", 15) == 0))
        return r; /* the declaration gate: direct or UTF-8-BOM form */
    /* the <gpx window: u32 0x7870673c at [+0x16, +0x216) */
    bool gpx = false;
    for (u64 i = 0x16; i + 4 <= avail && i < 0x216; ++i) {
        if (u32_from_bytes_le(head + i) == 0x7870673cu) {
            gpx = true;
            break;
        }
    }
    if (!gpx)
        return r;
    const u64 limit = probe_limit(s, hit, cap); /* cap = 100 MiB */
    std::vector<u8> buf(0x200 + 16);
    u64 size = 0;
    while (size < limit) {
        const u64 rel =
            chunk_find(s, hit + size, 0x200, 0, "</gpx>", 6, buf.data(), u32(buf.size()));
        if (rel != kNpos) {
            r.ok = true;
            r.end = hit + size + rel + 6;
            r.confidence = 700;
            r.note = "gpx-endtag-walk";
            return r;
        }
        size += 0x200;
    }
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "gpx-bounded-cap";
    return r;
}

CarveResult validate_osp(IStorageDevice* s, u64 hit, u64 cap) {
    /* COSPAnalyser (id 0x8f): the probe's 10-byte prefix gate ('{\r\n
     * "di": ') is carried by the row magic. The walk (FUN_1800421c0
     * :50159-50317): 0x200 chunks for "\"libopenshot\":" within 2 MiB;
     * extent = find + 14 + 0xF; the needle NOT FOUND -> the size field is
     * set to 0 -> the candidate is DROPPED (the family's only
     * reject-on-missing - ported as ok=false). */
    CarveResult r;
    const u64 limit = probe_limit(s, hit, cap); /* cap = 2 MiB */
    std::vector<u8> buf(0x200 + 16);
    u64 size = 0;
    while (size < limit) {
        const u64 rel = chunk_find(s, hit + size, 0x200, 0, "\"libopenshot\":", 14, buf.data(),
                                   u32(buf.size()));
        if (rel != kNpos) {
            r.ok = true;
            r.end = hit + size + rel + 14 + 0xF;
            r.confidence = 700;
            r.note = "osp-needle-walk";
            return r;
        }
        size += 0x200;
    }
    /* the og's reject-on-missing: size = 0 (FUN_1800421c0's exit path) */
    r.ok = false;
    return r;
}

CarveResult validate_pos(IStorageDevice* s, u64 hit, u64 cap) {
    /* CPOSAnalyser (id 0x8b): the probe's '{"diagram":' gate is carried by
     * the row magic (the og's _memicmp - the canonical case here). The
     * walk needle (the finding's open question, settled first-hand from
     * FUN_18001a470 :20250-20445): "ProcessOn Schema File" (21 bytes);
     * 0x200 chunks; extent = find + 21 + 0x13; cap 0x6400000. */
    CarveResult r;
    const u64 limit = probe_limit(s, hit, cap); /* cap = 100 MiB */
    std::vector<u8> buf(0x200 + 16);
    u64 size = 0;
    while (size < limit) {
        const u64 rel = chunk_find(s, hit + size, 0x200, 0, "ProcessOn Schema File", 21, buf.data(),
                                   u32(buf.size()));
        if (rel != kNpos) {
            r.ok = true;
            r.end = hit + size + rel + 21 + 0x13;
            r.confidence = 700;
            r.note = "pos-needle-walk";
            return r;
        }
        size += 0x200;
    }
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "pos-bounded-cap";
    return r;
}

CarveResult validate_chfp(IStorageDevice* s, u64 hit, u64 cap) {
    /* CHFPAnalyser (id 0x90): the probe (FUN_1800199xx :19939-19996)
     * gates the 15-byte XML declaration (case-SENSITIVE memcmp - carried
     * by the "<?xm" magic + re-verified) AND _memicmp(buf+0x28,
     * "<BiffProject", 12) == 0 (case-insensitive). The walk
     * (:20001-20184): 0x200 chunks for "</BiffProject>", extent =
     * find + 14; cap 0x6400000. */
    CarveResult r;
    u8 head[0x34];
    const u64 avail = std::min<u64>(sizeof(head), s->size() > hit ? s->size() - hit : 0);
    if (avail < 0x34)
        return r;
    if (!rd(s, hit, head, u32(avail)))
        return r;
    if (std::memcmp(head, "<?xml version=\"", 15) != 0)
        return r; /* the case-sensitive declaration gate */
    if (!ci_eq(head + 0x28, "<BiffProject", 12))
        return r;                               /* the case-insensitive root gate */
    const u64 limit = probe_limit(s, hit, cap); /* cap = 100 MiB */
    std::vector<u8> buf(0x200 + 16);
    u64 size = 0;
    while (size < limit) {
        const u64 rel =
            chunk_find(s, hit + size, 0x200, 0, "</BiffProject>", 14, buf.data(), u32(buf.size()));
        if (rel != kNpos) {
            r.ok = true;
            r.end = hit + size + rel + 14;
            r.confidence = 700;
            r.note = "chfp-endtag-walk";
            return r;
        }
        size += 0x200;
    }
    r.ok = true;
    r.end = limit;
    r.confidence = 450;
    r.note = "chfp-bounded-cap";
    return r;
}

CarveResult validate_ts(IStorageDevice* s, u64 hit, u64 cap) {
    /* 188-byte MPEG transport stream packets (CMTS, id 0x76) */
    return validate_ts_common(s, hit, cap, 188, 0, 700, "ts-packet-walk-188",
                              "ts-packet-walk-188+null");
}

CarveResult validate_m2ts(IStorageDevice* s, u64 hit, u64 cap) {
    /* 192-byte BDAV/M2TS packets - the 4-byte TP_extra_header precedes
     * every 0x47 sync (CM2TS, id 0x74; the og stamps the M2TS extension
     * for this family) */
    return validate_ts_common(s, hit, cap, 192, 4, 700, "m2ts-packet-walk-192",
                              "m2ts-packet-walk-192+null");
}

/* M-107 member 3 (DWG): the og's THREE-GENERATION size walk, decoded
 * first-hand from the raw asm this session (objdump over RAWSearchDLL.dll;
 * evidence/s85-lane-sheets/s85-l1-dwg-section-walk-raw-asm-decode.md):
 * the "AC10" registry binding at 0x1800036bf -> the node (vtable
 * 0x18006bc00, idx 0x18) -> the tier-1 nine-version gate FUN_18002fe30
 * {12,13,14,15,18,21,24,27,32} -> the factory FUN_18002ff20 (the 0x508
 * CDWGRawInfo child, cursor 0x15, extent 0) -> the child's +0x20 walk
 * FUN_18002f320, with NO secondary (node+0x40 = the return-0 stub).
 *
 * Gen-1 (AC1012/13/14/15 = R13/R14/R2000): the section-locator walk -
 * count = u32 @ 0x15 (the ctor's cursor, raw-asm 0x18002ff59), then
 * 9-byte records from 0x19 {number:u8 == index, seeker:u32 @+1, size:u32
 * @+5}; a number mismatch zeroes the extent (the decline, :36379-83);
 * extent = max(seeker + size) (the running max, :36384-86); the feed
 * stopping at the device end keeps the running max (the walked-prefix
 * salvage class). Gen-2 (AC1018/24/27/32 = R2004/R2010/R2013/R2018):
 * FUN_18002f630 XORs the 0x6c struct at +0x20 with the MSVC rand() LCG
 * stream (seed 1: state = state*0x343fd + 0x269ec3, byte = state>>16);
 * the gates on the DECODED struct (u32@0x0b==0, u32@0x10==0x6c,
 * u32@0x14==4, u32@0x44==0x20, u32@0x48==0x80, u32@0x4c==0x40); all
 * pass -> extent = u64@0x34 + 0x6c; any fail -> extent stays 0 (the
 * decline). Gen-3 (AC1021 = R2007): the 0x3d8-byte accumulate + the
 * 3x239 stride-3 de-interleave + FUN_18002ef50's record chain - NOT
 * decoded to landing quality; the bounded extent stays for that
 * generation, recorded as the finding's honest remainder (the fix
 * plan's own point 1(c) split). The walk cap: singleton+0x70
 * (0xbebc200 = 200 MB) * 100 = 0x4A817C800 (raw-asm 0x18002f5ee
 * `imul $0x64`) - the row max carries it. */
CarveResult validate_dwg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 v[6];
    if (!rd(s, hit, v, 6))
        return r;
    if (std::memcmp(v, "AC10", 4) != 0)
        return r;
    /* the og's tier-1 table (FUN_18002fe30 :36766-36794): exactly these
     * nine. The S-007 landing carried "30" (AC1030 - not a real DWG
     * release) and lacked "13" (AC1013 = R13) - corrected here. The og
     * has NO sentinel check (the version table is the whole gate). */
    static const char* kVersions[] = {"12", "13", "14", "15", "18", "21", "24", "27", "32"};
    int ver = -1;
    for (int i = 0; i < 9; ++i)
        if (v[4] == u8(kVersions[i][0]) && v[5] == u8(kVersions[i][1])) {
            ver = i;
            break;
        }
    if (ver < 0)
        return r;
    /* the generation routing (the og's own three tables): gen-1 =
     * {12,13,14,15} (:36331-46), gen-2 = {18,24,27,32} (:36394-408),
     * gen-3 = {21} (:36408-17) - routed by VERSION STRING, not table
     * index (the og's tables are not index-contiguous) */
    const char ver_str[3] = {char(v[4]), char(v[5]), 0};
    const bool gen1 = std::memcmp(ver_str, "12", 2) == 0 || std::memcmp(ver_str, "13", 2) == 0 ||
                      std::memcmp(ver_str, "14", 2) == 0 || std::memcmp(ver_str, "15", 2) == 0;
    const bool gen2 = std::memcmp(ver_str, "18", 2) == 0 || std::memcmp(ver_str, "24", 2) == 0 ||
                      std::memcmp(ver_str, "27", 2) == 0 || std::memcmp(ver_str, "32", 2) == 0;
    (void)ver;
    const u64 limit = probe_limit(s, hit, cap);
    const u64 rel_limit = limit - hit;

    if (gen1) {
        /* ---- gen-1: the section-locator walk (R13/R14/R2000) ---- */
        u8 hdr[0x19];
        if (!rd(s, hit, hdr, sizeof(hdr)))
            return r;
        const u32 count = u32_from_bytes_le(hdr + 0x15);
        u64 extent = 0; /* the base ctor's zero */
        bool walked = false;
        bool torn = false;
        /* the records: 9 bytes each from 0x19, walked through 64 KiB
         * windows (the og's chunk feed steps sector-sized windows; the
         * model divergence is the registered class - the semantics, the
         * record gates and the running max, are the og's own) */
        std::vector<u8> win(64 * 1024);
        u64 rec_off = 0x19; /* relative to the hit */
        u64 win_base = ~u64(0);
        u64 win_len = 0; /* the FILLED length (never win.size()) */
        for (u32 i = 0; i < count; ++i) {
            if (rec_off < win_base || rec_off + 9 > win_base + win_len) {
                if (rec_off + 9 > rel_limit) {
                    torn = true; /* the feed ran out mid-walk */
                    break;
                }
                win_base = rec_off;
                win_len = std::min<u64>(win.size(), rel_limit - win_base);
                if (win_len < 9 || !rd(s, hit + win_base, win.data(), u32(win_len))) {
                    torn = true;
                    break;
                }
            }
            const u8* rec = win.data() + (rec_off - win_base);
            if (rec[0] != u8(i)) {
                /* the og's only structural gate: number != index ->
                 * extent 0 (the decline, :36379-83) */
                return r;
            }
            const u32 seeker = u32_from_bytes_le(rec + 1);
            const u32 size = u32_from_bytes_le(rec + 5);
            const u64 rec_end = u64(seeker) + u64(size);
            if (rec_end > extent)
                extent = rec_end; /* the running max, :36384-86 */
            walked = true;
            rec_off += 9;
        }
        if (!walked)
            return r; /* nothing walked: the ctor's extent 0 */
        u64 end = hit + extent;
        if (end > limit)
            end = limit;
        if (end - hit < 512)
            return r;
        r.ok = true;
        r.end = end;
        r.confidence = torn ? 550 : 650; /* the torn salvage: reduced */
        r.note = torn ? "dwg-locator-walk,torn-salvage" : "dwg-locator-walk";
        return r;
    }

    if (gen2) {
        /* ---- gen-2: the rand-XOR struct walk (R2004+) ---- */
        u8 enc[0x6c];
        /* the encrypted struct sits at +0x80 BYTES (raw-asm 0x18002f5a0:
         * `lea rdx,[rdi+0x80]` with rdi = the chunk base, set once at
         * 0x18002f342 and never reloaded - the misaligned `mov edi` at
         * 0x18002f596 is the tail byte of `lea rcx,[rbp-0x41]`; the Ghidra
         * decompile's `param_2 + 0x20` is INT* arithmetic = 0x80 bytes,
         * the unscaled port repaired against the asm; evidence:
         * evidence/s84-lane-sheets/s84-l1-dwg-walk-decode.md §4) */
        if (!rd(s, hit + 0x80, enc, sizeof(enc)))
            return r; /* the struct never completes: the extent stays 0 */
        u8 dec[0x6c];
        u32 st = 1; /* the MSVC rand() LCG, seed 1 (FUN_18002f630) */
        for (size_t i = 0; i < sizeof(dec); ++i) {
            st = st * 0x343fd + 0x269ec3;
            dec[i] = u8(enc[i] ^ u8(st >> 16));
        }
        if (u32_from_bytes_le(dec + 0x0b) != 0 || u32_from_bytes_le(dec + 0x10) != 0x6c ||
            u32_from_bytes_le(dec + 0x14) != 4 || u32_from_bytes_le(dec + 0x44) != 0x20 ||
            u32_from_bytes_le(dec + 0x48) != 0x80 || u32_from_bytes_le(dec + 0x4c) != 0x40)
            return r; /* a failed gate leaves the extent at the ctor's 0 */
        const u64 extent = u64_from_bytes_le(dec + 0x34) + 0x6c;
        u64 end = hit + extent;
        if (end > limit)
            end = limit;
        r.ok = true;
        r.end = end;
        r.confidence = 650;
        r.note = "dwg-r2004-rand-struct-walk";
        return r;
    }

    /* ---- gen-3 (AC1021 = R2007): the undecoded de-interleave chain -
     * the bounded extent stays, the finding's recorded remainder ---- */
    u64 end = hit + (64ull << 20);
    if (end > limit)
        end = limit;
    if (end - hit < 512)
        return r;
    r.ok = true;
    r.end = end;
    r.confidence = 620; /* bounded-extent honesty: raw-sign class */
    r.note = "dwg-r2007-bounded-extent";
    return r;
}

/* ---- S-037: FreeHand 11 (BruteForce::CFH11Analyser, key 0x11C) ----
 * The og's Analyse is a pure constant-predicate (RAWSearchDLL
 * FUN_180029f30, decompile:32612-32633): registry key 0x11C hit
 * (bytes 1C 01 00 00 at the stride), then
 *   u64 @ +4  == 0x020014011c040002
 *   u32 @ +0xC == 0x011C1400
 *   u16 @ +0x22 == 0x3131 (ASCII "11" - the FreeHand 11 marker)
 * The og returns a CZeroEndRawFileInfo with type id 0x1d and NO size
 * walk (the zero-end result family; the engine's generic extension
 * semantics carry it). Our row mirrors the predicate exactly and sets a
 * bounded extent (the zero-end family has no size grammar to walk) -
 * the og id 0x1d is display-side only (the core's registry does not
 * carry og ids, the S-037 row note). */
CarveResult validate_fh11(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0x24];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    const u64 v8 = u64(h[4]) | (u64(h[5]) << 8) | (u64(h[6]) << 16) | (u64(h[7]) << 24) |
                   (u64(h[8]) << 32) | (u64(h[9]) << 40) | (u64(h[10]) << 48) | (u64(h[11]) << 56);
    if (v8 != 0x020014011C040002ULL)
        return r;
    const u32 v4 = u32(h[0xC]) | (u32(h[0xD]) << 8) | (u32(h[0xE]) << 16) | (u32(h[0xF]) << 24);
    if (v4 != 0x011C1400)
        return r;
    const u16 v2 = u16(h[0x22]) | (u16(h[0x23]) << 8);
    if (v2 != 0x3131)
        return r; /* "11" */
    r.ok = true;
    r.confidence = 700;
    /* CZeroEndRawFileInfo: no size grammar - bounded extent (the
     * family's salvage posture over the zero-end semantics). N-146: the
     * stand-in is REGISTERED (M-107's enumeration, the twelfth site - the
     * zero-end class; the D-101 stateless-architecture decline covers the
     * og's pending-object zero-end metric) and PINNED
     * (Carve.Fh11BoundedExtentPinned holds the exact 32 MiB). */
    const u64 limit = probe_limit(s, hit, cap);
    u64 end = hit + (32ull << 20); /* FH11 documents are small-to-medium */
    if (end > limit)
        end = limit;
    if (end - hit < 0x24)
        return r;
    r.end = end;
    r.note = "fh11-key+const-gate";
    return r;
}

/* ---- S-037: Corel PhotoPaint CPT7/8/9 (RawSpeed::CPTAnalyser +
 * FileCPTAnalyser fallback, keys 0x37545043/0x38545043/0x39545043) ----
 * The og's Analyse (FUN_180043630, decompile:51102-51130) after the key
 * hit: u32 @ +4 == 0x454C4946 ("FILE") and u32 @ +0x34 == 0x13C ->
 * type id 0x11, size deferred to the walk. The walk (FUN_1800436e0,
 * decompile:51134-51204) is a pointer-chase through the container:
 *   tab_off = u32 @ +0x34 (absolute data offset)
 *   n       = u32 @ +0x28; last entry = u32 @ tab_off + (n-1)*8
 *   base    = last + 0x3c; if u32 @ (last+0x20) != 0 and
 *             u32 @ (last+0x3c) == it: base = last + 0x3c + that value
 *   if u32 @ base != 0 and u32 @ (base+4) != 0:
 *       size = u32 @ (u32@base - 8) + u32 @ (u32@base - 4)
 *   else size = last
 * Every read failure anywhere rejects (the og returns null on any
 * window miss). The og id 0x11 is display-side only. */
CarveResult validate_cpt(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0x38];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    const u32 file_magic = u32(h[4]) | (u32(h[5]) << 8) | (u32(h[6]) << 16) | (u32(h[7]) << 24);
    if (file_magic != 0x454C4946) /* "FILE" */
        return r;
    const u32 slot34 =
        u32(h[0x34]) | (u32(h[0x35]) << 8) | (u32(h[0x36]) << 16) | (u32(h[0x37]) << 24);
    if (slot34 != 0x13C)
        return r; /* the version gate == the table offset (same slot) */
    const u32 n = u32(h[0x28]) | (u32(h[0x29]) << 8) | (u32(h[0x2A]) << 16) | (u32(h[0x2B]) << 24);
    if (n == 0 || n > 0x10000)
        return r; /* no pointer table / hostile count */
    const u64 limit = probe_limit(s, hit, cap);
    /* window-read helper: the og's FUN_180042930 returns NULL on any
     * read miss and the walk rejects - mirror that with a success flag */
    u8 raw[4];
    auto rd32chk = [&](u64 rel, u32& out) -> bool {
        if (hit + rel + 4 > limit)
            return false;
        if (!rd(s, hit + rel, raw, 4))
            return false;
        out = u32(raw[0]) | (u32(raw[1]) << 8) | (u32(raw[2]) << 16) | (u32(raw[3]) << 24);
        return true;
    };
    /* the walk: table pointer = the same +0x34 slot (0x13C) */
    u32 last = 0;
    if (!rd32chk(u64(slot34) + (u64(n) - 1) * 8, last))
        return r;
    u64 base = u64(last) + 0x3c;
    u32 tail_chk = 0;
    if (rd32chk(u64(last) + 0x20, tail_chk) && tail_chk != 0) {
        u32 at3c = 0;
        if (rd32chk(u64(last) + 0x3c, at3c) && at3c == tail_chk)
            base = u64(last) + 0x3c + at3c;
    }
    u32 t2 = 0, t3 = 0;
    if (!rd32chk(base, t2) || !rd32chk(base + 4, t3))
        return r;
    u64 size = last;
    if (t2 != 0 && t3 != 0) {
        if (t2 < 8)
            return r; /* the og's window read at (t2-8) fails -> reject */
        u32 a = 0, b = 0;
        if (!rd32chk(u64(t2) - 8, a) || !rd32chk(u64(t2) - 4, b))
            return r;
        size = u64(a) + u64(b);
    }
    if (size < 0x38 || hit + size > limit)
        return r; /* degenerate / non-fitting */
    r.ok = true;
    r.end = hit + size;
    r.confidence = 700;
    r.note = "cpt-file-magic+pointer-chase";
    return r;
}

/* ---- S-007 niche batch 1 (SCAN-2f) --------------------------------- *
 * Every key below was re-read first-hand at its RAWSearchDLL registration
 * site before the row landed (retdec static initializer :5450-6600, u16
 * layer :6205-6590, analyser bodies :20515-22690). Walk semantics follow
 * the documented format grammar (the S-007 primary batch's rule: "the
 * format grammar is the model, og behavior the anchor"); formats whose
 * og size-walk is undecoded use the DWG-pattern bounded extent heuristic
 * (the sanctioned honest cap, conf <= 650, note names the heuristic). */

/* MID: "MThd" key 0x6468544d (:5735) -> MIDAnalyser+FileMID chain idx 38.
 * Header: MThd + BE u32 len (6) + u16 format (0..2) + u16 ntrks + u16
 * division; then MTrk chunks (BE u32 length each). Accept needs >= 1
 * complete track; a torn tail salvages at the last complete track. */
CarveResult validate_mid(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[14];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "MThd", 4) != 0)
        return r;
    const u32 hlen = (u32(h[4]) << 24) | (u32(h[5]) << 16) | (u32(h[6]) << 8) | u32(h[7]);
    if (hlen != 6)
        return r; /* header length is 6 in every SMF file */
    const u32 fmt = (u32(h[8]) << 8) | h[9];
    const u32 ntrks = (u32(h[10]) << 8) | h[11];
    const u32 division = (u32(h[12]) << 8) | h[13];
    if (fmt > 2 || ntrks == 0 || division == 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit + 14;
    u64 last_end = 0;
    u32 complete = 0;
    for (u32 guard = 0; guard < 65536; ++guard) {
        u8 th[8];
        if (!rd(s, q, th, 8) || q + 8 > limit)
            break;
        if (std::memcmp(th, "MTrk", 4) != 0)
            break;
        const u64 len = (u64(th[4]) << 24) | (u64(th[5]) << 16) | (u64(th[6]) << 8) | u64(th[7]);
        if (len > limit - q)
            break; /* torn track: salvage what walked clean */
        q += 8 + len;
        ++complete;
        last_end = q;
    }
    if (complete == 0 || last_end == 0)
        return r; /* bare header is the degenerate the key would accept */
    r.ok = true;
    r.end = last_end;
    r.confidence = complete >= ntrks ? 800 : 650;
    r.note = complete >= ntrks ? "midi-track-walk" : "midi-partial-tracks";
    return r;
}

/* OGG: "OggS" key 0x5367674f (:5583) -> OGGAnalyser+CFileOgg idx 82.
 * Page: "OggS" + ver 0 + header_type (first page 0x00/0x02) + granule u64
 * + serial/seq/CRC u32 + seg-count u8 + segment table; walk to the first
 * page break; EOS page (type 0x04) closes a clean stream. */
CarveResult validate_ogg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[27];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "OggS", 4) != 0 || h[4] != 0)
        return r;
    if (h[5] != 0x00 && h[5] != 0x02)
        return r; /* first page: fresh or BOS */
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit;
    u64 last_end = 0;
    u32 pages = 0;
    bool saw_eos = false;
    for (u32 guard = 0; guard < 65536; ++guard) {
        u8 p[27];
        if (!rd(s, q, p, 27) || q + 27 > limit)
            break;
        if (std::memcmp(p, "OggS", 4) != 0 || p[4] != 0 || (p[5] & 0xF8))
            break;
        u64 seg_total = 0;
        u8 segs[255];
        if (p[26] != 0 && !rd(s, q + 27, segs, p[26]))
            break;
        for (int i = 0; i < p[26]; ++i)
            seg_total += segs[i];
        if (q + 27 + u64(p[26]) + seg_total > limit)
            break; /* torn page: salvage the walked pages */
        q += 27 + u64(p[26]) + seg_total;
        ++pages;
        last_end = q;
        if (p[5] & 0x04) {
            saw_eos = true;
            break;
        }
    }
    if (pages == 0)
        return r;
    r.ok = true;
    r.end = last_end;
    r.confidence = saw_eos ? 800 : 650;
    r.note = saw_eos ? "ogg-pages-to-eos" : "ogg-pages-torn-tail";
    return r;
}

/* RM: ".RMF" key 0x464d522e (:5603) -> RM_RMVB chain idx 94. The RMF
 * lead-in is 0x10 bytes (both documented layouts accepted: version @+4
 * (== 1) or uniform-chunk total-size @+4 (== 0x10)); the body is a chunk
 * sequence {4CC id + BE u32 total-size (>= 8)} walked BruteForce-style
 * to the first grammar break (the ASF/A-010 family contract). */
CarveResult validate_rm(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[16];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, ".RMF", 4) != 0)
        return r;
    const u32 v = (u32(h[4]) << 24) | (u32(h[5]) << 16) | (u32(h[6]) << 8) | u32(h[7]);
    if (v != 1 && v != 0x10)
        return r; /* version-1 or uniform-chunk-size RMF layouts */
    const u32 tail = (u32(h[8]) << 24) | (u32(h[9]) << 16) | (u32(h[10]) << 8) | u32(h[11]);
    if (tail == 0 || tail > 0x20000000u)
        return r; /* n-headers / size sanity */
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit + 0x10;
    u64 last_end = 0;
    u32 chunks = 0;
    for (u32 guard = 0; guard < 65536; ++guard) {
        u8 ch[8];
        if (!rd(s, q, ch, 8) || q + 8 > limit)
            break;
        const u32 sz = (u32(ch[4]) << 24) | (u32(ch[5]) << 16) | (u32(ch[6]) << 8) | u32(ch[7]);
        if (sz < 8 || q + sz > limit)
            break;
        q += sz;
        ++chunks;
        last_end = q;
    }
    if (chunks == 0)
        return r;
    r.ok = true;
    r.end = last_end;
    r.confidence = 700;
    r.note = "rm-chunk-walk";
    return r;
}

/* MXF: 06 0E 2B 34 key 0x342b0e06 (:5616) -> MXFAnalyser idx 120. KLV
 * items: 16-byte SMPTE UL key (each item re-keys 06 0E 2B 34) + BER
 * length (0x00-0x7F short / 0x81-0x88 long form); walk to break. */
CarveResult validate_mxf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 k[16];
    if (!rd(s, hit, k, sizeof(k)))
        return r;
    if (std::memcmp(k, "\x06\x0E\x2B\x34", 4) != 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit;
    u64 last_end = 0;
    u32 items = 0;
    for (u32 guard = 0; guard < 65536; ++guard) {
        if (!rd(s, q, k, 16) || q + 16 > limit)
            break;
        if (std::memcmp(k, "\x06\x0E\x2B\x34", 4) != 0)
            break;
        u8 lb = 0;
        if (!rd(s, q + 16, &lb, 1))
            break;
        u64 len = 0;
        u32 hdr = 17; /* key + first length byte */
        if ((lb & 0x80) == 0) {
            len = lb;
        } else if (lb >= 0x81 && lb <= 0x88) {
            const u32 n = lb & 0x7F;
            u8 lenb[8];
            if (!rd(s, q + 17, lenb, n))
                break;
            for (u32 i = 0; i < n; ++i)
                len = (len << 8) | lenb[i];
            hdr += n;
        } else {
            break; /* 0x80 indefinite + reserved forms: grammar break */
        }
        if (len > limit - q || u64(hdr) > limit - q - len)
            break; /* torn / overflowing item: salvage the walked set */
        q += hdr + len;
        ++items;
        last_end = q;
    }
    if (items < 2)
        return r; /* a single KLV is noise the 4-byte key would accept */
    r.ok = true;
    r.end = last_end;
    r.confidence = 700;
    r.note = "mxf-klv-walk";
    return r;
}

/* DMG: "BZh1" key 0x31685a42 (:5485) -> DMG chain idx 113 - the og keys
 * the LEVEL-1 bzip2 stream byte-exactly. Gates: the 48-bit block magic
 * 0x314159265359 at +4; the stream footer 0x177245385090 ends a clean
 * carve (+4-byte combined CRC); a missing footer salvages to the cap. */
CarveResult validate_dmg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[10];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "BZh1", 4) != 0)
        return r;
    if (std::memcmp(h + 4, "\x31\x41\x59\x26\x53\x59", 6) != 0)
        return r; /* bzip2 block header magic (BCD pi) */
    const u64 limit = probe_limit(s, hit, cap);
    /* M-090: the og walks the "koly" UDIF trailer, not the bzip2 EOS.
     * RAWSearchDLL_decompiled.c:63782-63868 (FUN_180053de0, first-hand):
     * 512-byte chunk steps from the stream start; a chunk whose LAST 8
     * bytes are zero gates both probes - "koly" at the chunk start
     * (aligned, size = chunk base + 0x200, the mandatory 512-byte UDIF
     * footer), and past 0x2800 a full search of the PREVIOUS chunk
     * (unaligned salvage, size = koly offset + 0x200). The bzip2 EOS
     * magic survives only as the no-koly fallback (the batch-1
     * bounded-extent convention, now the documented heuristic). */
    {
        std::vector<u8> chunk(512), prev(512);
        bool have_prev = false;
        u64 p = hit;
        while (p + 512 <= limit) {
            if (!rd(s, p, chunk.data(), 512))
                break;
            bool zero_tail = true;
            for (int i = 504; i < 512; ++i)
                if (chunk[size_t(i)] != 0) {
                    zero_tail = false;
                    break;
                }
            if (zero_tail) {
                if (std::memcmp(chunk.data(), "koly", 4) == 0) {
                    r.ok = true;
                    r.end = std::min(p + 0x200, limit);
                    r.confidence = 700;
                    r.note = "dmg-koly-trailer";
                    return r;
                }
                const u64 rel = p - hit;
                if (rel > 0x2800 && rel > 512 && have_prev) {
                    for (u32 i = 0; i + 4 <= 512; ++i) {
                        if (std::memcmp(prev.data() + i, "koly", 4) == 0) {
                            r.ok = true;
                            r.end = std::min(p - 512 + i + 0x200, limit);
                            r.confidence = 700;
                            r.note = "dmg-koly-trailer-unaligned";
                            return r;
                        }
                    }
                }
            }
            std::memcpy(prev.data(), chunk.data(), 512);
            have_prev = true;
            p += 512;
        }
    }
    const u8 kFooter[] = {0x17, 0x72, 0x45, 0x38, 0x50, 0x90};
    std::vector<u8> blk(kProbeBlock);
    u64 pos = hit + 10;
    while (pos + 6 <= limit) {
        const u64 n = std::min<u64>(u64(kProbeBlock), limit - pos);
        if (!rd(s, pos, blk.data(), u32(n)))
            break;
        for (u64 i = 0; i + 6 <= n; ++i) {
            if (std::memcmp(blk.data() + i, kFooter, 6) == 0) {
                r.ok = true;
                r.end = std::min(pos + i + 10, limit); /* footer + combined CRC */
                r.confidence = 650;
                r.note = "bzh1-stream-to-eos-no-koly";
                return r;
            }
        }
        if (n <= 6)
            break;    /* nothing more to advance past the 5-byte seam */
        pos += n - 5; /* seam: the footer may span two blocks */
    }
    r.ok = true;
    r.end = limit; /* no footer inside the cap: truncated stream salvage */
    r.confidence = 550;
    r.note = "bzh1-truncated-no-footer";
    return r;
}

/* X3F: "FOVb" key 0x62564f46 (:5504) -> X3F chain idx 108. M-107's X3F
 * member, the M-156 correction on top of the S71/M-155/S81/S82 chain of
 * landings (the racing lanes' legacy-only semantics kept their ASCII
 * version gate, which REJECTS EVERY REAL SIGMA X3F FILE - real files
 * carry a BINARY u32 version 0x00020001-style, and the gate demanded an
 * ASCII digit + point; that live bug is what this correction fixes
 * first). The full raw-asm decode, keyed to instruction addresses:
 * diff-to-fix/evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md.
 * The og's X3F surface is a TWO-GENERATION chain (the registry's
 * construct-then-insert pairing at 0x180003174-0x1800031f4: the 0x18-byte
 * modern Brance::X3FAnalyser node, vtable 0x18006cbc0, with the 0x10-byte
 * legacy FileX3FAnalyser child at node+0x10, vtable 0x18006e198):
 *
 *   - the MODERN tier-1 (FUN_180040080, node vtable +0x10): the version
 *     field must be a BINARY u32 - bytes +5/+7 == 0. The magic-key
 *     dispatch (FUN_18000cf60's map path at 0x18000d1e4-0x18000d2c9)
 *     looks the node up, calls node->vt[+0x40], and - BYTE-EXACT: the
 *     X3F node's +0x40 slot is the return-0 STUB (0x18000bae0, read
 *     straight from .rdata) - so the dispatcher's fallback consult NEVER
 *     FIRES for X3F (only 8 other node classes - the embedded-fallback
 *     shape, e.g. the 0xd302020 row - override +0x40 with the real
 *     getter FUN_180013700 `lea rax,[rcx+0x10]`). The node's own +0x10
 *     binary gate runs; on acceptance the invocation core
 *     (0x18000ca8d-0x18000ccfc) calls the node's factory (+0x38 =
 *     FUN_1800400a0: the 0x120-byte walker, cap [obj+0x118] =
 *     0x6400000 = 100 MiB) and registers it for the feeding protocol
 *     (walker vtable +0x20 = FUN_180040160 -> FUN_1800404a0): extent =
 *     SECd + 16 + count*12 (count = u32@SECd+8, clamped to the cap),
 *     no directory -> the extent field is never written (the modern
 *     declines).
 *   - the LEGACY walk (FUN_1800535b0, the child's vtable +0x08): the
 *     sector-stepped reads with the string search and the 2-sector
 *     window; extent = SECd + 0x4C in all three found branches; the
 *     loop-exit SALVAGES the walked prefix (never a refusal). Its
 *     tier-1 (FUN_180053590) accepts any readable header - everything
 *     the modern's binary gate refuses lands here.
 *
 * The correction's routing (the honest reading of the verified wiring):
 * a BINARY-version file takes the modern count-table walk - the
 * provably-wired magic-key path (the map returns the modern node, the
 * factory creates the count-walker, the walker registers for feeding);
 * anything else falls to the legacy fixed-tail walk (the racing lanes'
 * convergent semantics, preserved); a torn file with no directory lands
 * at the legacy's walked-prefix salvage at reduced confidence. The
 * version GATE itself is gone: the chain's net acceptance is the union
 * (the modern's binary form + the legacy's permissive read) - the og's
 * registry keys on FOVb alone and no tier-1 pair ever jointly rejects a
 * readable header. Open questions (registered in M-156): the engine's
 * feed loop driving walker->vt[+0x20] was not located this session (the
 * state machine's return-2 "feed me more" protocol is the evidence it
 * exists); the deeper dispatch layers (the map projections / the
 * full-registry sweep at 0x18000d2f3) were not fully decoded. */
CarveResult validate_x3f(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[8];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "FOVb", 4) != 0)
        return r;
    /* the chain's version router (M-156): the modern generation's tier-1
     * gate is the BINARY u32 form (FUN_180040080: bytes +5/+7 == 0);
     * every other shape falls to the legacy generation, whose tier-1
     * (FUN_180053590) accepts any readable header. NO version gate
     * survives: the union of the two generations' gates is "readable
     * header" - the old ASCII gate (S71/S78/M-155/S81/S82's landed code)
     * rejected every real binary-version Sigma file. */
    const bool modern_generation = (h[5] == 0 && h[7] == 0);
    r.ok = true;
    const u64 limit = probe_limit(s, hit, cap); /* absolute device bound */
    const u64 rel_limit = limit - hit;          /* the walk's relative bound */
    constexpr u64 X3F_DIR_TAIL = 0x4C;          /* the legacy generation's fixed tail */
    /* the S80 convergent-rider hardening: the 256 KiB window is a heap
     * vector, not a stack array - validate_x3f runs on the carve fan-out's
     * worker threads, and a 256 KiB stack frame is an overflow risk on
     * small-stack hosts the file's other validators avoid by convention
     * (the kProbeBlock vectors above) */
    constexpr u32 X3F_WIN = 256 * 1024;
    std::vector<u8> win(X3F_WIN);
    bool found = false;
    u64 sec_at = 0;
    /* the og's walk starts at the hit (uVar7 = 0) and steps forward in
     * overlapping windows to its cap; all coordinates here are relative
     * to the hit, the probe limit converted once on entry */
    for (u64 off = 0; off + 4 <= rel_limit && !found; off += X3F_WIN - 3) {
        const u64 want = std::min<u64>(X3F_WIN, rel_limit - off);
        if (!rd(s, hit + off, win.data(), u32(want)))
            break;
        for (u64 i = 0; i + 4 <= want; ++i) {
            if (win[size_t(i)] == 'S' && win[size_t(i) + 1] == 'E' && win[size_t(i) + 2] == 'C' &&
                win[size_t(i) + 3] == 'd') {
                sec_at = off + i;
                found = true;
                break;
            }
        }
        if (off + want >= rel_limit)
            break; /* the last window - no more data */
    }
    if (found) {
        if (modern_generation) {
            /* the modern count-table walk (FUN_1800404a0's extent math,
             * raw-asm at 0x180040541-0x180040571 and the half-back retry
             * 0x180040596-0x180040610): SECd + 16 + count*12, clamped to
             * the row bound + device end (the og's own cmova clamp). A
             * directory whose count field is unreadable (the magic at the
             * very end of the window) is the og's chunk-tail shape
             * (0x1800404f9-0x180040501): the extent ends at the window
             * end. */
            if (sec_at + 12 > rel_limit) {
                r.end = limit;
            } else {
                u8 d[4];
                u32 count = 0;
                if (rd(s, hit + sec_at + 8, d, sizeof(d)))
                    count = u32_from_bytes_le(d);
                u64 end = hit + sec_at + 16 + u64(count) * 12;
                if (end > limit)
                    end = limit; /* the og's clamp */
                r.end = end;
            }
            r.confidence = 620;
            r.note = "x3f-section-directory-walk";
        } else {
            /* the legacy fixed-tail walk (FUN_1800535b0): SECd + 0x4C in
             * every found branch - the count is never read on this path */
            r.end = hit + std::min<u64>(sec_at + X3F_DIR_TAIL, rel_limit);
            r.confidence = 620;
            r.note = "x3f-legacy-directory-tail-walk";
        }
    } else {
        /* torn: the walked prefix (bounded by the row cap) - the og's
         * loop-exit salvage shape */
        r.end = limit;
        r.confidence = 550;
        r.note = "x3f-torn-no-section-directory";
    }
    return r;
}

/* M-081 (SCAN-4): II\x1A\x00 / MM\x1A\x00 are CANON CRW keys - Brance::
 * CRWAnalyser -> FileCRWAnalyser, id 0x12 (registration RAWSearchDLL
 * :1746-1761, first-hand). Grammar FUN_180052000 :62333-62353 (first-hand):
 * bytes +4..+7 == 00 00 'H' 'E' (u32 0x45480000), u16@+8 == "AP" (completes
 * "HEAP" at +6..+9), record-type u32@+0xa in {CCDR (0x52444343), TIFP
 * (0x50464954), ARCH (0x48435241)}, version byte@+0xe == 2 - the real Canon
 * CIFF header `II/MM 1A 00 00 00 HEAP CCDR/TIFP/ARCH 02`. The og's CIFF heap
 * size-walk (FUN_180052160) was the recorded follow-up when this grammar
 * gate landed - it landed as M-107's windowed entry search (the block
 * below, the decode-then-land order); this sentence is corrected to
 * match (Session-84: the old "DWG-pattern bounded extent stands in"
 * claim survived the M-107 landing that removed the stand-in itself). */
/* M-107 (the CRW member, the finding's decode-then-land order, member 2
 * of 11): the og's CIFF heap size walk, decoded first-hand from the raw
 * asm this session (objdump over RAWSearchDLL.dll; the walk = the CRW
 * child's vtable 0x18006e008 slot +0x08 = FUN_180052150 -> the body
 * FUN_180052160): sector-stepped from the file start with the walker's
 * OWN cap 0x3200000 = 50 MiB (raw-asm 0x18005219d `mov r15d,0x3200000`);
 * non-zero-tailed sectors are skipped (the streaming chunk-tail gate -
 * the registered model divergence, same stance as X3F), zero-tailed
 * sectors are searched for CIFF directory entries of the shape
 * {u16 type == 0x2807, u32 length in {0xa8, 0xb2, 0xe4}} (the three
 * 6-byte signatures built at 0x1800521a3-0x1800521c4: 07 28 XX 00 00
 * 00); a hit stamps extent = entry_position + the length's tail
 * constant from the walker's own table ({0xa8 -> 0x46, 0xb2 -> 0x3c,
 * 0xe4 -> 0x32} - the map built at 0x1800521dc-0x18005234b, the values
 * written at 0x18005224f/0x1800522cb/0x18005234b); no entry anywhere in
 * range -> the loop-exit stamps the CAP itself (the 50 MiB walked bound
 * - raw-asm 0x180052641's r15 reset and the exit at 0x1800526d7). The
 * og's backup-80 re-probe (0x18005264d: the prefix parse at pos-0x50 ->
 * extent = pos + sector_size) is the streaming model's straddle
 * artifact - the unconditional windowed search subsumes it (the same
 * registered divergence class). The old blind 64 MiB stand-in and its
 * comment are GONE. */
CarveResult validate_crw(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0xF];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "II\x1A\x00", 4) != 0 && std::memcmp(h, "MM\x1A\x00", 4) != 0)
        return r; /* the registry keys, re-checked */
    if (u32_from_bytes_le(h + 4) != 0x45480000u)
        return r; /* 00 00 'H' 'E' */
    if (u16_from_bytes_le(h + 8) != 0x5041u)
        return r; /* "AP" */
    const u32 fourcc = u32_from_bytes_le(h + 0xa);
    if (fourcc != 0x52444343u /* CCDR */ && fourcc != 0x50464954u /* TIFP */ &&
        fourcc != 0x48435241u /* ARCH */)
        return r;
    if (h[0xe] != 2)
        return r; /* version byte 2 */
    r.ok = true;
    const u64 limit = probe_limit(s, hit, cap);
    const u64 rel_limit = limit - hit;
    /* the og walker's own cap constant (raw-asm 0x18005219d) */
    constexpr u64 CRW_WALK_CAP = 0x3200000;
    const u64 bound = std::min<u64>(rel_limit, CRW_WALK_CAP);
    /* the CIFF directory-entry search: {u16 0x2807, u32 len} with the
     * og's own three known lengths and their tail constants (the map
     * built at 0x1800521dc-0x18005234b). Windowed with the straddle
     * overlap; the search covers from the file start. */
    struct CrwTail {
        u32 len;
        u64 tail;
    };
    constexpr CrwTail CRW_TAILS[] = {{0xa8, 0x46}, {0xb2, 0x3c}, {0xe4, 0x32}};
    std::vector<u8> win(kProbeBlock);
    u64 entry_at = 0;
    u64 entry_tail = 0;
    bool found = false;
    u64 pos = 0;
    while (pos + 6 <= bound) {
        const u64 chunk = std::min<u64>(win.size(), bound - pos);
        if (chunk < 6)
            break;
        if (!rd(s, hit + pos, win.data(), u32(chunk)))
            break; /* unreadable window: the walked prefix ends here */
        for (u64 i = 0; i + 6 <= chunk; ++i) {
            if (u16_from_bytes_le(win.data() + i) != 0x2807)
                continue;
            const u32 len = u32_from_bytes_le(win.data() + i + 2);
            for (const CrwTail& t : CRW_TAILS) {
                if (len == t.len) {
                    entry_at = pos + i;
                    entry_tail = t.tail;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
        if (found)
            break;
        if (chunk < win.size())
            break;        /* short final read: the window is exhausted */
        pos += chunk - 5; /* straddle overlap */
    }
    if (found) {
        u64 end = hit + entry_at + entry_tail;
        if (end > limit)
            end = limit;
        r.end = end;
        r.confidence = 650;
        r.note = "crw-ciff-directory-walk";
    } else {
        /* torn: the og's loop-exit stamps the walked bound (the cap or
         * the device end, whichever clamps) - never a refusal */
        r.end = hit + bound;
        r.confidence = 550;
        r.note = "crw-torn-no-directory";
    }
    return r;
}

/* MRW (M-081): the ONLY true Minolta key is \x00MRM (0x004d524d,
 * :2500-2510) -> FileMRWAnalyser id 0x66. The og validator
 * (FUN_180012100 :14233-14258, first-hand) sets NO structural gate
 * beyond the key and computes size = BE32(u32@+4) + 8 +
 * ceil(u8@0x20 * BE16@+0x1a * BE16@+0x18 / 8). The u64 product
 * replaces the og's wrapping signed multiply (identical for every
 * non-UB input; the D-68 numeric-hygiene precedent). */
CarveResult validate_mrw(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0x21];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\x00MRM", 4) != 0)
        return r; /* the registry key, re-checked */
    const u64 base = (u64(h[4]) << 24) | (u64(h[5]) << 16) | (u64(h[6]) << 8) | u64(h[7]);
    const u64 dim_w = (u64(h[0x18]) << 8) | u64(h[0x19]);
    const u64 dim_h = (u64(h[0x1a]) << 8) | u64(h[0x1b]);
    const u64 pixels = u64(h[0x20]) * dim_w * dim_h;
    const u64 size = base + 8 + (pixels + 7) / 8;
    r.ok = true;
    r.end = std::min(hit + size, probe_limit(s, hit, cap));
    r.confidence = 600;
    r.note = "mrm-og-size-formula";
    return r;
}

/* SCAN-4 (the M-081 twin): DB A5 2D 00 / 94 A6 2E 00 are
 * FileLowDocAnalyser keys (id 0x17, :2353/:2365, first-hand) - the
 * old-Word .doc family. The og validator (FUN_18001b650 :21167-21188,
 * first-hand) is a SIZE FORMULA ONLY (no structural gate beyond the
 * registry key): OLE branch (first u32 == D0 CF 11 E0) -> size =
 * (u32@0x30 + 5) * 0x200 (kept og-verbatim; unreachable through the
 * keyed rows - the key IS the first u32); else size = (u32@0x124 +
 * u16@0x128 + 0x1FF) & ~0x1FF. */
CarveResult validate_lowdoc(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 k[4];
    if (!rd(s, hit, k, sizeof(k)))
        return r;
    if (std::memcmp(k, "\xDB\xA5\x2D\x00", 4) != 0 && std::memcmp(k, "\x94\xA6\x2E\x00", 4) != 0)
        return r; /* the registry keys, re-checked */
    u64 size;
    if (u32_from_bytes_le(k) == 0xE011CFD0u) {
        u8 b[4];
        if (!rd(s, hit + 0x30, b, sizeof(b)))
            return r;
        size = (u64(u32_from_bytes_le(b)) + 5) * 0x200;
    } else {
        u8 b[6];
        if (!rd(s, hit + 0x124, b, sizeof(b)))
            return r;
        size = (u64(u32_from_bytes_le(b)) + u16_from_bytes_le(b + 4) + 0x1FF) & ~u64(0x1FF);
    }
    r.ok = true;
    r.end = std::min(hit + size, probe_limit(s, hit, cap));
    r.confidence = 600;
    r.note = "lowdoc-og-size-formula";
    return r;
}

/* M-107 member 5 (WPG): the og's ZERO-TAIL size walk, decoded
 * first-hand this session. The chain: the "wpg\0" tier-1 FUN_180014030
 * (:15660-15683: the magic re-check, id 0x45), the factory
 * FUN_180014090 (the 0x120 WPGRawFileInfo child: min 20, max 100 MB),
 * and the child's +0x20 processor FUN_180014160 (:15721-15785, raw asm
 * 0x180014160-0x1800142b0): extent = chunk_offset + zero_tail(chunk)
 * where zero_tail is FUN_180042b30 (:50703-50750) - a binary search for
 * the FIRST position whose remainder is all zeros (the whole buffer
 * zero -> 0; no zeros -> the buffer length), then +1. The processor
 * walks the FIRST fed chunk and RETURNS 1 (done) - the driver
 * (FUN_18000d670's +0x20 dispatch at 0x18000d7ae) passes the DEVICE
 * SECTOR SIZE as the chunk length (ctx+0xF0, the storage device's
 * sector query at :9192-9195, default 0x200). The model translation
 * (the registered divergence class): the core reads the hit's own
 * 512-byte window and stamps the trailing-zero-run position - the
 * og's semantic (the extent ends at the file's last non-zero byte
 * within the first sector window), without the streaming chunk
 * artifacts. The deferral residue (the processor's ctx gate
 * [result+0x14] >= 0x10 - undecoded; if it defers the walk to a later
 * sector the extent shifts by multiples of 512) is recorded in the
 * finding. */
CarveResult validate_wpg(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[20];
    if (!rd(s, hit, h, sizeof(h)))
        return r; /* the og FileInfo floor is 20 bytes */
    if (std::memcmp(h, "wpg\x00", 4) != 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    const u64 rel_limit = limit - hit;
    /* the og's chunk = the device sector (0x200); clamp to the row
     * bound + the device end */
    constexpr u64 WPG_WINDOW = 0x200;
    u8 win[WPG_WINDOW];
    const u64 avail = std::min<u64>(WPG_WINDOW, rel_limit);
    if (avail < 20)
        return r; /* the FileInfo floor */
    if (!rd(s, hit, win, u32(avail)))
        return r;
    /* FUN_180042b30's zero-tail: the first position whose remainder is
     * all zeros (+1); the whole window zero -> 0 (the og's extent 0 =
     * the decline class); no zeros -> the window length */
    u64 zt = 0;
    bool all_zero = true;
    for (u64 i = 0; i < avail; ++i)
        if (win[i] != 0) {
            all_zero = false;
            break;
        }
    if (!all_zero) {
        u64 lo = 0, hi = avail; /* hi = the first all-zero tail candidate */
        while (lo < hi - 1) {
            const u64 mid = (lo + hi) / 2;
            bool zeros = true;
            for (u64 i = mid; i < avail; ++i)
                if (win[i] != 0) {
                    zeros = false;
                    break;
                }
            if (zeros)
                hi = mid;
            else
                lo = mid;
        }
        zt = hi; /* the boundary: [hi, avail) is the zero run */
    }
    if (zt < 20)
        return r; /* the walked extent under the FileInfo floor */
    r.ok = true;
    r.end = hit + zt;
    r.confidence = 620;
    r.note = "wpg-zero-tail-walk";
    return r;
}

/* WPD: \xFF" "WPC key 0x435057ff (:5937) -> FileWPDAnalyser idx 68. The
 * document stream walk is undecoded; the 4-byte key + a nonzero
 * product-version word at +4 carry the gate, bounded extent stands in. */
CarveResult validate_wpd(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[8];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h,
                    "\xFF"
                    "WPC",
                    4) != 0)
        return r;
    if (h[4] == 0 && h[5] == 0)
        return r; /* zero product word: degenerate */
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 600;
    r.note = "wpd-magic+bounded-extent";
    return r;
}

/* M-107 member 4 (PSP): the og's tier-1 + the ~BK block-chain walk,
 * decoded first-hand this session (RAWSearchDLL FUN_180016390
 * :17347-17368 + the child FUN_180016510 :17410-17482, the ctor
 * FUN_180016430 :17366-17384: min 0x24, max 20000000, the walked
 * position starts at 0x24). The tier-1 is the EXACT 32-byte signature
 * (memcmp 0x20): "Paint Shop Pro Image File" + 0x0a + 0x1a + 5 NULs.
 * The walk: from 0x24, each block is {u32 magic 0x004b427e ("~BK\\0"),
 * u32 length @ +6}; the position advances 10 + length; the extent is
 * the walked END - the first non-~BK block, the 20 MB row bound, or
 * the device end all stop the walk there (never a refusal once the
 * signature held). */
CarveResult validate_psp(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[32];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    /* the og's exact 32-byte compare (FUN_180016390's seven locals) */
    static const u8 kSig[32] = {'P', 'a', 'i', 'n',  't',  ' ', 'S', 'h', 'o', 'p', ' ',
                                'P', 'r', 'o', ' ',  'I',  'm', 'a', 'g', 'e', ' ', 'F',
                                'i', 'l', 'e', 0x0a, 0x1a, 0,   0,   0,   0,   0};
    if (std::memcmp(h, kSig, sizeof(kSig)) != 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    const u64 rel_limit = limit - hit;
    /* the ~BK chain from 0x24 (the ctor's walked-position start) */
    u64 pos = 0x24;
    bool torn = false;
    std::vector<u8> blk(10);
    while (pos + 10 <= rel_limit) {
        if (!rd(s, hit + pos, blk.data(), 10)) {
            torn = true; /* the straddling header: the walked end salvages */
            break;
        }
        if (u32_from_bytes_le(blk.data()) != 0x004b427eu) /* "~BK\\0" */
            break; /* the non-~BK terminator: the extent is the walked end */
        const u32 len = u32_from_bytes_le(blk.data() + 6);
        pos += 10 + u64(len);
    }
    if (pos > rel_limit)
        pos = rel_limit; /* the 20 MB row bound / the device end clamp */
    r.ok = true;
    r.end = hit + pos;
    r.confidence = torn ? 550 : 650;
    r.note = torn ? "psp-bk-chain-walk,torn-salvage" : "psp-bk-chain-walk";
    return r;
}

/* SIT: "Stuf" key 0x66757453 (:5840) -> SIT chain idx 93 (index-first
 * archive family). The 7-byte "StuffIt" covenant is the strong form of
 * the key; bounded extent stands in for the undecoded index walk. */
CarveResult validate_sit(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[7];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "Stuf", 4) != 0 || std::memcmp(h + 4, "fIt", 3) != 0)
        return r;
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 620;
    r.note = "sit-magic+bounded-extent";
    return r;
}

/* GZ: u16 key 0x8B1F (:6481) -> GZAnalyzer+FileGZ chain idx 39. The
 * modern analyser is header-validation; the core walks the optional
 * FLG fields to the DEFLATE start and bounds the extent (the trailer
 * is only findable by inflating). */
CarveResult validate_gz(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[10];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (h[0] != 0x1F || h[1] != 0x8B)
        return r;
    if (h[2] != 8)
        return r; /* CM = deflate (8 in every shipped gzip) */
    if (h[3] & 0xE0)
        return r; /* reserved FLG bits must be clear */
    const u32 os = h[9];
    if (os > 13 && os != 0xFF)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit + 10;
    const u32 flg = h[3];
    if (flg & 0x04) { /* FEXTRA */
        u8 x[2];
        if (!rd(s, q, x, 2))
            return r;
        const u32 xlen = u32(x[0]) | (u32(x[1]) << 8);
        if (xlen == 0 || xlen > 0x10000)
            return r;
        q += 2 + xlen;
    }
    /* FNAME / FCOMMENT: NUL-terminated, bounded scan */
    for (int which = 0; which < 2; ++which) {
        if (!(flg & (which == 0 ? 0x08 : 0x10)))
            continue;
        bool term = false;
        for (u32 i = 0; i < 0x10000; ++i) {
            u8 b = 0;
            if (!rd(s, q, &b, 1))
                return r;
            ++q;
            if (b == 0) {
                term = true;
                break;
            }
        }
        if (!term)
            return r;
    }
    if (flg & 0x02)
        q += 2; /* FHCRC */
    if (q >= limit)
        return r;
    r.ok = true;
    r.end = limit; /* the DEFLATE stream runs to the member's trailer */
    r.confidence = 600;
    r.note = "gz-header-fields+deflate-bounded";
    return r;
}

/* APE: "MAC " key 0x2043414d (:5653) -> CAPEAnalyser idx 81 (frame-sync
 * school, no index). Version is the BE u32 at +4 (3.9x releases carry
 * 3920-3990); bounded extent stands in for the undecoded frame walk. */
CarveResult validate_ape(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[8];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "MAC ", 4) != 0)
        return r;
    const u32 v = (u32(h[4]) << 24) | (u32(h[5]) << 16) | (u32(h[6]) << 8) | u32(h[7]);
    if (v < 300 || v > 5000)
        return r; /* release integers 3.90-4.x */
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 620;
    r.note = "ape-version-gate+bounded-extent";
    return r;
}

/* AU: ".snd" key 0x646e732e (:6155) -> FileAUAnalyser idx 9. Sun/NeXT
 * audio: BE data-offset (>= 24), BE size (0xFFFFFFFF = unknown), BE
 * encoding 1..27, BE sample-rate and channel-count sanity. */
CarveResult validate_au(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[24];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, ".snd", 4) != 0)
        return r;
    const u32 off = (u32(h[4]) << 24) | (u32(h[5]) << 16) | (u32(h[6]) << 8) | u32(h[7]);
    const u32 size = (u32(h[8]) << 24) | (u32(h[9]) << 16) | (u32(h[10]) << 8) | u32(h[11]);
    const u32 enc = (u32(h[12]) << 24) | (u32(h[13]) << 16) | (u32(h[14]) << 8) | u32(h[15]);
    const u32 rate = (u32(h[16]) << 24) | (u32(h[17]) << 16) | (u32(h[18]) << 8) | u32(h[19]);
    const u32 ch = (u32(h[20]) << 24) | (u32(h[21]) << 16) | (u32(h[22]) << 8) | u32(h[23]);
    if (off < 24 || u64(off) > (256ull << 20))
        return r;
    if (enc < 1 || enc > 27)
        return r; /* documented encodings 1..27 */
    if (rate < 1000 || rate > 2000000)
        return r;
    if (ch < 1 || ch > 64)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    if (size != 0xFFFFFFFF) {
        if (size < 4)
            return r;
        r.ok = true;
        r.end = std::min(hit + u64(off) + u64(size), limit);
        r.confidence = 800;
        r.note = "sun-audio-fields";
        return r;
    }
    r.ok = true;
    r.end = limit; /* unknown size: the data runs out */
    r.confidence = 650;
    r.note = "sun-audio-fields+unknown-size";
    return r;
}

/* AIFF: "FORM" key 0x4d524f46 (:6086) -> CFileAIFAnalyzer idx 80 (the
 * IFF family, AIFF/ILBM...). Grammar: FORM + BE u32 size + 4-byte
 * printable form-type; extent = 8 + size. */
CarveResult validate_aiff(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[12];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "FORM", 4) != 0)
        return r;
    const u32 size = (u32(h[4]) << 24) | (u32(h[5]) << 16) | (u32(h[6]) << 8) | u32(h[7]);
    if (size < 4 || u64(size) > cap)
        return r;
    for (int i = 8; i < 12; ++i)
        if (h[i] < 0x20 || h[i] > 0x7E)
            return r; /* form-type must be printable ASCII */
    const u64 limit = probe_limit(s, hit, cap);
    r.ok = true;
    r.end = std::min(hit + 8 + u64(size), limit);
    r.confidence = 750;
    r.note = "iff-form-grammar";
    return r;
}

/* HLP (SCAN-4): 3F 5F 03 00 ONLY (idx 32, :2401, first-hand; id 0x20 -
 * E49). The og validator (FUN_180041c50 :49883-49895, first-hand) gates
 * u32@0xc > 0x1FF && u32@+4 <= u32@0xc and takes size = u32@0xc. */
CarveResult validate_hlp(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0x10];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\x3F\x5F\x03\x00", 4) != 0)
        return r; /* the registry key, re-checked */
    const u32 first = u32_from_bytes_le(h + 4);
    const u32 size = u32_from_bytes_le(h + 0xc);
    if (size <= 0x1FF || first > size)
        return r; /* the og's own two gates */
    r.ok = true;
    r.end = std::min(hit + u64(size), probe_limit(s, hit, cap));
    r.confidence = 600;
    r.note = "hlp-og-size-gate";
    return r;
}

/* ICO/CUR: 00 00 01 00 / 00 00 02 00 keys 0x00010000/0x00020000
 * (:6053-6065) -> FileICOAnalyser idx 34 (one analyser, both dir
 * types). ICONDIR: reserved u16 (0) + type u16 (1/2) + count u16; each
 * 16-byte entry carries a data extent - the walk ends at the last
 * entry's end. */
CarveResult validate_ico(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[6];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (h[0] != 0 || h[1] != 0)
        return r;
    const u32 type = u32(h[2]) | (u32(h[3]) << 8);
    const u32 count = u32(h[4]) | (u32(h[5]) << 8);
    if (type != 1 && type != 2)
        return r;
    if (count == 0 || count > 0x1000)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 end = 6 + 16ull * count;
    for (u32 i = 0; i < count; ++i) {
        u8 e[16];
        if (!rd(s, hit + 6 + 16ull * i, e, sizeof(e)))
            return r;
        const u32 planes = u32(e[4]) | (u32(e[5]) << 8);
        const u32 bpp = u32(e[6]) | (u32(e[7]) << 8);
        const u32 bytes = u32(e[8]) | (u32(e[9]) << 8) | (u32(e[10]) << 16) | (u32(e[11]) << 24);
        const u32 data_off =
            u32(e[12]) | (u32(e[13]) << 8) | (u32(e[14]) << 16) | (u32(e[15]) << 24);
        if (type == 1 && (planes < 1 || planes > 4))
            return r; /* icon entries: sane plane/bpp counts */
        if (type == 1 &&
            (bpp != 0 && bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32))
            return r;
        if (bytes < 16)
            return r;
        if (u64(data_off) + u64(bytes) > end)
            end = u64(data_off) + u64(bytes);
    }
    if (hit + end > limit)
        return r; /* the declared data does not fit the slab */
    r.ok = true;
    r.end = hit + end;
    r.confidence = 700;
    r.note = "ico-dir-walk";
    return r;
}

/* WMF (placeable): D7 CD C6 9A key 0x9ac6cdd7 -> the WMF node (format
 * code 0x46, shared with the standard branch). Og grammar decoded at
 * RAWSearchDLL FUN_1800264d0 :29807-29845 (S-042, triple-verified:
 * ghidra + retdec + capstone across three audit lanes): hmf word@+4 == 0;
 * the reserved dword @+16..19 == 0 (NO inch gate - the og never reads
 * the inch field); the checksum word stored @+20 == XOR(words 0..9)
 * (words 8..9 are the zeroed reserved dword, so the XOR equals the XOR
 * of words 0..7, but the range is 0..9 as decoded); the standard header
 * 01 00 09 00 follows at +22; its params word @+38 == 0; extent = the
 * standard header's Size dword@+28 * 2 + 22 - the exact file length,
 * not a heuristic. */
CarveResult validate_wmf_placeable(IStorageDevice* s, u64 hit, u64 /*cap*/) {
    CarveResult r;
    u8 h[40];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\xD7\xCD\xC6\x9A", 4) != 0)
        return r;
    if (u16_from_bytes_le(h + 4) != 0)
        return r; /* hmf word == 0 */
    if (h[16] != 0 || h[17] != 0 || h[18] != 0 || h[19] != 0)
        return r; /* reserved dword @+16..19 == 0 */
    /* the placeable checksum per the og: the stored word at +20 equals
     * the XOR of the TEN preceding words (0..9) - the S-007 batch-1
     * landing's "stored@+18 == XOR(words 0..8)" convention read the
     * wrong offset with the wrong range and was refuted by every lane
     * that re-read the decompile (S-042). */
    u16 x = 0;
    for (int i = 0; i < 10; ++i)
        x = u16(x ^ u16_from_bytes_le(h + 2 * i));
    if (x != u16_from_bytes_le(h + 20))
        return r; /* placeable-header checksum */
    if (std::memcmp(h + 22, "\x01\x00\x09\x00", 4) != 0)
        return r; /* the standard header follows at +22 */
    if (u16_from_bytes_le(h + 38) != 0)
        return r; /* the standard header's params word == 0 */
    const u32 std_words = u32_from_bytes_le(h + 28);
    r.ok = true;
    r.end = hit + u64(std_words) * 2 + 22;

    r.confidence = 750;
    r.note = "wmf-placeable-og-grammar";
    return r;
}

/* WMF (standard): 01 00 09 00 key 0x00090001 -> the WMF node (format
 * code 0x46, shared with the placeable branch). Og grammar (S-042
 * addendum 1, FUN_1800264d0 :29809-29815): the ONLY gates are the key
 * and the params word@+16 == 0; extent = the Size dword@+6 * 2
 * verbatim. The old core-only gates (version 0x0100/0x0300, objects
 * <= 256, maxrec <= words, words >= 9) were stricter than the og and
 * rejected og-accepted files - removed for parity. Size sanity is the
 * engine's job (min_size/max_size bounds apply to the returned
 * extent), exactly as the og's controller caps its verbatim size. */
CarveResult validate_wmf_std(IStorageDevice* s, u64 hit, u64 /*cap*/) {
    CarveResult r;
    u8 h[18];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\x01\x00\x09\x00", 4) != 0)
        return r;
    if (u16_from_bytes_le(h + 16) != 0)
        return r; /* params word == 0 - the og's only second gate */
    const u32 words = u32_from_bytes_le(h + 6);
    r.ok = true;
    r.end = hit + u64(words) * 2;
    r.confidence = 700;
    r.note = "wmf-standard-og-gates";
    return r;
}

/* TTF: 00 01 00 00 (sfnt 1.0) - the u16-key 0x0100 chain shared with
 * MDB (census multimap). The sfnt table directory is the gate:
 * numTables 1..128, and every entry's offset+length must fit inside the
 * walk's maximum. */
CarveResult validate_ttf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[12];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\x00\x01\x00\x00", 4) != 0)
        return r;
    const u32 n = (u32(h[4]) << 8) | h[5];
    if (n == 0 || n > 128)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 end = 12 + 16ull * n;
    for (u32 i = 0; i < n; ++i) {
        u8 e[16];
        if (!rd(s, hit + 12 + 16ull * i, e, sizeof(e)))
            return r;
        const u32 off = (u32(e[8]) << 24) | (u32(e[9]) << 16) | (u32(e[10]) << 8) | u32(e[11]);
        const u32 len = (u32(e[12]) << 24) | (u32(e[13]) << 16) | (u32(e[14]) << 8) | u32(e[15]);
        if (u64(off) + u64(len) > end)
            end = u64(off) + u64(len);
        if (e[8] == 0 && e[9] == 0 && e[10] == 0 && e[11] == 0 && e[12] == 0 && e[13] == 0 &&
            e[14] == 0 && e[15] == 0)
            return r; /* all-zero offset+length: not a table entry */
    }
    if (hit + end > limit)
        return r;
    r.ok = true;
    r.end = hit + end;
    r.confidence = 700;
    r.note = "ttf-table-directory";
    return r;
}

/* MDB family (M-084 + M-091): the u16-key 0x0100 chain -> CMDBAnalyser,
 * both signature branches re-read first-hand (FUN_180015260 :16497-16529:
 * key 00 01 00 00 + "Standard Jet DB\0" at +4 -> type 0x27 (39);
 * "Standard ACE DB\0" -> type 0x4C (76) - the Access 2007+ generation
 * the batch-1 landing dropped; FileInfo bounds {0x1000, 0xBEBC200}
 * :16532-16546). M-091: the size walk is the page-chain walk
 * FUN_1800447c0 (:52001-52039, read verbatim): 0x1000 strides from the
 * first page, the page-type u16 at each page start must be nonzero and
 * <= 0x901 (the Jet b-tree/data page-type family), the run ends at the
 * first breaking page, cap 0x6400000 (the 0x63fffff inner check). */
CarveResult validate_mdb_common(IStorageDevice* s, u64 hit, u64 cap, bool ace) {
    CarveResult r;
    u8 h[20];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\x00\x01\x00\x00", 4) != 0)
        return r;
    if (std::memcmp(h + 4, ace ? "Standard ACE DB\x00" : "Standard Jet DB\x00", 16) != 0)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 off = hit + 0x1000;
    while (off <= hit + 0xBEBC200ull && off + 2 <= limit) {
        const u16 word = rd16(s, off);
        if (word == 0)
            break; /* the while condition's zero page ends the run */
        if (word > 0x901)
            break; /* the Jet page-type family gate */
        off += 0x1000;
        if (off > hit + 0x63fffffull)
            break; /* the walk cap: final size <= 0x6400000 */
    }
    r.ok = true;
    r.end = std::min(off, limit);
    r.confidence = 650;
    r.note = ace ? "ace-db-signature+page-chain-walk" : "jet-db-signature+page-chain-walk";
    return r;
}

CarveResult validate_mdb(IStorageDevice* s, u64 hit, u64 cap) {
    return validate_mdb_common(s, hit, cap, false);
}

/* M-084: the ACE-generation signature - id 0x4C, same bounds, same
 * page-chain walk (the og's one validator, two branches). */
CarveResult validate_accdb(IStorageDevice* s, u64 hit, u64 cap) {
    return validate_mdb_common(s, hit, cap, true);
}

/* TAR: the list-layer probe - "ustar" via strncmp at +257 (TARAnalyser,
 * :63657/:20300). The classic tar header checksum (all header bytes
 * with the chksum field read as spaces) is the exact gate; the block
 * walk advances header+data in 512-byte units to the first break/zero
 * block. min_size 1024 keeps the F8 late-magic rule (257+32). */
CarveResult validate_tar(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    const u64 limit = probe_limit(s, hit, cap);
    u64 q = hit;
    u64 last_end = 0;
    u32 blocks = 0;
    for (u32 guard = 0; guard < (1u << 20); ++guard) {
        u8 b[512];
        if (!rd(s, q, b, sizeof(b)) || q + 512 > limit)
            break;
        bool all_zero = true;
        for (int i = 0; i < 512; ++i)
            if (b[i] != 0) {
                all_zero = false;
                break;
            }
        if (all_zero) {
            if (blocks > 0)
                last_end = q; /* the EOF zero block closes the archive */
            break;
        }
        if (std::memcmp(b + 257, "ustar", 5) != 0)
            break; /* not a header block: end of the walked region */
        /* checksum: sum with the 8 chksum bytes replaced by spaces */
        u64 sum = 0;
        for (int i = 0; i < 512; ++i)
            sum += (i >= 148 && i < 156) ? u64(' ') : u64(b[i]);
        u64 stored = 0;
        bool digits = false;
        for (int i = 148; i < 156; ++i) {
            if (b[i] >= '0' && b[i] <= '7') {
                stored = stored * 8 + u64(b[i] - '0');
                digits = true;
            }
        }
        if (!digits || sum != stored)
            break; /* header fails the tar checksum: grammar break */
        /* size field: 12 octal digits at +124 */
        u64 size = 0;
        for (int i = 124; i < 136; ++i)
            if (b[i] >= '0' && b[i] <= '7')
                size = size * 8 + u64(b[i] - '0');
        const u64 data = (size + 511) / 512 * 512;
        if (q + 512 + data > limit)
            break; /* torn member: salvage the walked blocks */
        q += 512 + data;
        ++blocks;
        last_end = q;
    }
    if (blocks == 0)
        return r;
    r.ok = true;
    r.end = last_end;
    r.confidence = 750;
    r.note = "tar-checksum-verified-blocks";
    return r;
}

/* EPS: u16 key "%!" 0x2125 (:6205/:6219, the CEPS-family PostScript
 * chain). The canonical PS covenant "%!PS" is the analyser-side gate
 * (every shipped PostScript carries it); extent is bounded (the PS
 * trailer scan is the recorded follow-up). */
CarveResult validate_eps(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[4];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (h[0] != '%' || h[1] != '!' || h[2] != 'P' || h[3] != 'S')
        return r;
    r.ok = true;
    /* N-148: 64 MiB - the M-107-verified stand-in shape (the registry
     * row and the finding's "all eleven at 64 MiB"; the 16 MiB drift
     * made the validator 4x tighter than its own row) */
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 620;
    r.note = "postscript-covenant+bounded-extent";
    return r;
}

/* AI: C5 D0 D3 C6 key 0xc6d3d0c5 (:5690) -> CEPSAnalyser idx 27 (the
 * session-3 correction: Adobe Illustrator AI/EPS - NO dBase analyser
 * exists in the og). Version word at +4 gates; bounded extent stands
 * in for the undecoded PS-body walk. */
CarveResult validate_ai(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[8];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\xC5\xD0\xD3\xC6", 4) != 0)
        return r;
    const u32 v = u32_from_bytes_le(h + 4);
    if (v < 1 || v > 64)
        return r; /* AI format versions 2..10 */
    r.ok = true;
    /* N-148: 64 MiB - the M-107-verified stand-in shape (the row
     * re-pinned with it; was 16 MiB) */
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 620;
    r.note = "ai-version-gate+bounded-extent";
    return r;
}

/* DXF: "  0\r" key 0x0d302020 (:5555) -> CDXF chain (g146+g144, idx
 * 25) - first-hand: DXF is a KEYED u32 format (the census's list-layer
 * label corrected). The ASCII group-code covenant "  0\r\nSECTION"
 * extends the key; bounded extent stands in for the text walk.
 * N-152: the og's probe (FUN_1800376f0 :42240-42290, re-read first-hand)
 * ALSO enters through the u16 "99" key 0x3939 (the 999-comment entry -
 * :2722-2733's registry binding, the same CDXF chain idx 0x19): a file
 * starting "999\n" or "999\r\n" (the DXF group-code-999 free-form
 * comment convention) is accepted when the section pattern follows -
 * the LF form "  0\nSECTION\n  " (14 bytes) after "999\n", the CRLF
 * form "  0\r\nSECTION\r\n  " (16 bytes) after "999\r\n". The scan
 * walks forward from the comment (the og's byte-by-byte memcmp loop
 * :42277-42293) and breaks at the first NUL byte (the og's loop
 * guard). The scan window here is bounded at 4 KiB (the comment-to-
 * SECTION span in real CAD exports is tens of bytes; the og scans its
 * full probe window - the bound is this port's documented posture). */
CarveResult validate_dxf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    /* N-208 (renumbered from the born-collided N-189 at the S57 CI-red repair): the canonical gate
     * is the og's FULL 16-byte two-qword compare "  0\r\nSECTION\r\n  " (RAWSearchDLL FUN_1800376f0
     * :42258-42275: *p == 0x4345530a0d302020 && p[1] ==
     * 0x20200a0d4e4f4954) - the 8-byte prefix alone is a collision
     * class the og refuses; the pre-fix core carved it. A header
     * truncated under 16 readable bytes falls to the 999 leg exactly
     * as the og's short-buffer probe does. */
    u8 h[16];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    bool ok = std::memcmp(h, "  0\r\nSECTION\r\n  ", 16) == 0;
    const char* note = "dxf-group-code-covenant+bounded-extent";
    if (!ok) {
        /* the 999-comment entry key's branch */
        u8 c[5];
        if (!rd(s, hit, c, sizeof(c)))
            return r;
        const bool lf_comment = c[0] == '9' && c[1] == '9' && c[2] == '9' && c[3] == '\n';
        const bool crlf_comment =
            c[0] == '9' && c[1] == '9' && c[2] == '9' && c[3] == '\r' && c[4] == '\n';
        if (lf_comment || crlf_comment) {
            static const char kPatLf[] = "  0\nSECTION\n  ";
            static const char kPatCrLf[] = "  0\r\nSECTION\r\n  ";
            const char* pat = lf_comment ? kPatLf : kPatCrLf;
            const size_t plen = lf_comment ? sizeof(kPatLf) - 1 : sizeof(kPatCrLf) - 1;
            constexpr u64 kScan = 4096;
            u8 win[kScan];
            const u64 avail = std::min<u64>(kScan, s->size() > hit ? s->size() - hit : 0);
            if (avail >= plen && rd(s, hit, win, u32(avail))) {
                for (u64 i = 0; i + plen <= avail; ++i) {
                    if (win[i] == 0)
                        break; /* the og's NUL guard */
                    if (std::memcmp(win + i, pat, plen) == 0) {
                        ok = true;
                        break;
                    }
                }
            }
            if (ok)
                note = "dxf-999-comment-entry+section-scan";
        }
    }
    if (!ok)
        return r;
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 620;
    r.note = note;
    return r;
}

/* 3GP: the og's File3GP row - the ftyp family's 3gp brands (the MP4
 * validator's brand set does not carry them). Shares the BMFF top-box
 * walk semantics: brand at +8, boxes to the first break. */
CarveResult validate_3gp(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[12];
    if (!rd(s, hit + 4, h, sizeof(h)))
        return r; /* ftyp sits at +4 */
    if (std::memcmp(h, "ftyp", 4) != 0)
        return r;
    static const char* kBrands[] = {"3gp4", "3gp5", "3gp6", "3gr6", "3gs6", "3ge6", "3gg6"};
    bool brand_ok = false;
    for (const char* b : kBrands)
        if (std::memcmp(h + 4, b, 4) == 0) /* major_brand sits at h+4 */
            brand_ok = true;
    if (!brand_ok)
        return r;
    const u64 limit = probe_limit(s, hit, cap);
    /* top-level box walk (the bmff family grammar) */
    u64 q = hit;
    u64 last_end = 0;
    for (u32 guard = 0; guard < 4096; ++guard) {
        u8 bh[8];
        if (!rd(s, q, bh, 8) || q + 8 > limit)
            break;
        u64 sz = (u64(bh[0]) << 24) | (u64(bh[1]) << 16) | (u64(bh[2]) << 8) | u64(bh[3]);
        if (sz == 1) {
            u8 lb[8];
            if (!rd(s, q + 8, lb, 8))
                break;
            u64 big = 0;
            for (int i = 0; i < 8; ++i)
                big = (big << 8) | lb[i];
            sz = big;
        } else if (sz == 0) {
            last_end = limit; /* to-EOF box */
            break;
        }
        if (sz < 8 || sz > limit - q)
            break; /* torn box: salvage the walked set */
        q += sz;
        last_end = q;
    }
    if (last_end == 0)
        return r;
    r.ok = true;
    r.end = last_end;
    r.confidence = 780;
    r.note = "bmff-3gp-brand+box-walk";
    return r;
}

/* ---- S-041 niche batch 2, school 1 (SCAN-4): every grammar below
 * re-verified first-hand at its analyser body before the row landed
 * (the session-5 decode table in SCAN-ENGINE-DELTA is the record). ---- */

/* CHM: ITSF key -> FileCHMAnalyser id 0x0e (registration :2488-2499,
 * first-hand). The og validator (FUN_180016b40 :17705-17740, first-hand):
 * ITSF@0 + u32@8==0x60 + both ITSF GUIDs as qword pairs (@0x18 ==
 * 0x11d07baa7c01fd10, @0x20 == 0xece622c9a0000c9e, @0x28 ==
 * 0x11d07baa7c01fd11, @0x30 == 0xece622c9a0000c9e) + u32@0x60==0x1FE
 * (the directory-header entry magic) + u32@0x70==0 + u32@0x74==0 ->
 * size = u64@0x68. The prior ":22635 pin" was std::_Tree rebalance code. */
CarveResult validate_chm(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0x78];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "ITSF", 4) != 0)
        return r; /* the registry key, re-checked */
    if (u32_from_bytes_le(h + 8) != 0x60u)
        return r;
    if (u64_from_bytes_le(h + 0x18) != 0x11d07baa7c01fd10ull ||
        u64_from_bytes_le(h + 0x20) != 0xece622c9a0000c9eull ||
        u64_from_bytes_le(h + 0x28) != 0x11d07baa7c01fd11ull ||
        u64_from_bytes_le(h + 0x30) != 0xece622c9a0000c9eull)
        return r; /* the two ITSF GUIDs */
    if (u32_from_bytes_le(h + 0x60) != 0x1FEu || u32_from_bytes_le(h + 0x70) != 0 ||
        u32_from_bytes_le(h + 0x74) != 0)
        return r;
    const u64 size = u64_from_bytes_le(h + 0x68);
    r.ok = true;
    r.end = std::min(hit + size, probe_limit(s, hit, cap));
    r.confidence = 750;
    r.note = "chm-itsf-guid-grammar";
    return r;
}

/* ONE: E4 52 5C 7B key -> RawSpeed::OneAnalyser/FileONE id 0x31
 * (registration :2156-2177). The og validator (FUN_180044db0
 * :52287-52311, first-hand) completes the OneNote GUID: qword@+4 ==
 * 0x7853b1ae4da7d88c and dword@0xc == 0xd32969d0. The FileInfo bounds
 * {20, 500,000,000} are the og ctor's own (FUN_180016080 :17191-17209).
 * The chunk-chain size walk (FUN_180044e30) is the recorded follow-up;
 * the DWG-pattern bounded extent stands in. */
CarveResult validate_one(IStorageDevice* s, u64 hit, u64 cap) {
    /* M-106: the chunk-chain size walk (FUN_180044e30 :52316-52358,
     * decoded in the trail's own evidence - s8-batch2-formats.md:29).
     * From the hit, iterate chunks: each re-checks the GUID pair
     * (u64@+4 == 0x7853B1AE4DA7D88C, u32@+0xC == 0xD39629D0); the
     * chunk size is the u64@+0xC4 (fallback the u64@+0x64 when the
     * primary is 0); a size of 0 or a size > 2x the device breaks the
     * chain; the cursor advances by the size.
     * S-062 exit table (the comma-operator decode, re-read first-hand
     * this session; the retdec cross-check agrees): the og's published
     * size (uVar5) is set to the CURSOR at each iteration head, and to
     * 2x-DEVICE at the moment the primary size field is read - so:
     *   - both size fields 0, or size > 2x-device -> publish 2x-device
     *     (the flat extent; the pre-fix core published the last-valid-
     *     chunk end, 0 when the FIRST chunk declines);
     *   - window miss / GUID mismatch / normal exhaustion -> the cursor
     *     (the end of the last fully-valid chunk);
     * the FileInfo bound is 500,000,000 (FUN_180016080 :17203 - the
     * row's own cap). Replaces the DWG-pattern 64 MiB stand-in (the
     * M-090/M-091 truncation class on the school-1 formats). */
    CarveResult r;
    u8 h[0x10];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\xE4\x52\x5C\x7B", 4) != 0)
        return r; /* the registry key, re-checked */
    if (u64_from_bytes_le(h + 4) != 0x7853b1ae4da7d88cull ||
        u32_from_bytes_le(h + 0xc) != 0xd32969d0u)
        return r;                   /* the GUID tail */
    const u64 dev2 = s->size() * 2; /* the og's 2x-device chunk-size bound */
    const u64 limit = probe_limit(s, hit, cap);
    u64 pos = 0;  /* the relative cursor */
    u64 size = 0; /* the published size: the cursor, or 2x-device on the
                   * size-gate breaks (the S-062 exit table) */
    u8 c[0xcc];   /* the chunk head: the fields live at +4/+0xC/+0x64/+0xC4 */
    while (pos <= dev2) {
        if (!rd(s, hit + pos, c, sizeof(c)))
            break; /* the window miss: the size keeps (the og's exit) */
        if (u64_from_bytes_le(c + 4) != 0x7853b1ae4da7d88cull ||
            u32_from_bytes_le(c + 0xc) != 0xd32969d0u)
            break; /* the per-chunk GUID re-check */
        u64 sz = u64_from_bytes_le(c + 0xc4);
        if (sz == 0)
            sz = u64_from_bytes_le(c + 0x64); /* the fallback field */
        if (sz == 0 || sz > dev2) {
            /* S-062: the og's uVar5 was set to 2x-device the moment the
             * primary size field was read (:52335's comma expression) -
             * both size-gate exits publish the flat 2x-device extent,
             * never the valid-prefix cursor */
            size = dev2;
            break; /* the 0<size<=2x-device gate */
        }
        pos += sz;
        size = pos; /* the end of this valid chunk - the cursor exits keep it */
    }
    if (size > 500000000ull)
        size = 500000000ull; /* the FileInfo ctor's own max */
    r.ok = true;
    r.end = std::min(hit + size, limit);
    r.confidence = 620;
    r.note = "one-guid+chunk-chain-walk";
    return r;
}

/* MNY: u16 'MS' key (:2909) -> FileMNYAnalyser id 0x28. The og
 * validator (FUN_180041dd0 :49956-49981, first-hand) is a single
 * 15-byte memcmp "MSISAM Database" @+4 -> claimed size 0x100000. */
CarveResult validate_mny(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[19];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "MS", 2) != 0)
        return r; /* the registry key, re-checked */
    if (std::memcmp(h + 4, "MSISAM Database", 15) != 0)
        return r; /* the og's whole follow-up gate */
    r.ok = true;
    r.end = std::min(hit + u64(0x100000), probe_limit(s, hit, cap));
    r.confidence = 650;
    r.note = "mny-msisam-memcmp";
    return r;
}

/* EMF: list-only, id 0x1a (:3194-3222). The og validator
 * (FUN_180012080 :14204-14214, first-hand): " EMF"@+0x28 (u32
 * 0x464d4520) + u16@0x3a==0 + nBytes u32@0x30!=0 -> size = nBytes.
 * NO iType-1 check exists in the og (the census phrasing corrected). */
CarveResult validate_emf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[0x3C];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h + 0x28, " EMF", 4) != 0)
        return r; /* the list-layer probe (re-checked at +0x28) */
    if (u16_from_bytes_le(h + 0x3A) != 0)
        return r; /* the reserved word must be zero */
    const u32 nbytes = u32_from_bytes_le(h + 0x30);
    if (nbytes == 0)
        return r;
    r.ok = true;
    r.end = std::min(hit + u64(nbytes), probe_limit(s, hit, cap));
    r.confidence = 700;
    r.note = "emf-signature+reserved+nbytes";
    return r;
}

/* SWF: list-only FWS/CWS (:3058-3086) -> FileSWFAnalyser id 0x3e. The
 * og validator (FUN_1800268a0 :29970-29996 + the deep gate
 * FUN_180026940 :30029-30031, first-hand): FWS/CWS only - ZWS is NOT
 * accepted; version byte@3 in [3,20]; FileLength u32@4 > 8 -> size =
 * FileLength. The RECT bit-walk past byte 8 is the recorded follow-up. */
CarveResult validate_swf(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[8];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if ((h[0] != 'F' && h[0] != 'C') || h[1] != 'W' || h[2] != 'S')
        return r; /* FWS | CWS only (the registry keys, re-checked) */
    if (h[3] < 3 || h[3] > 20)
        return r; /* the og's version gate */
    const u32 len = u32_from_bytes_le(h + 4);
    if (len <= 8)
        return r; /* the og's FileLength gate */
    r.ok = true;
    r.end = std::min(hit + u64(len), probe_limit(s, hit, cap));
    r.confidence = 700;
    r.note = "swf-sig+version+filelength";
    return r;
}

/* 3DS: u16 'MM' key (:2885) -> RawSpeed::New3DSAnalyser/File3DS id 1
 * (ctor FUN_1800126c0 :14491-14510). The og validator (FUN_180042fa0
 * :50906-50916, first-hand): 'MM'@0 AND '=='@+0x10, OR the alt branch
 * u32@0x100 == 0x4453434e ('NCSD' - extracted from the og DLL's .data,
 * VA 0x180093978 / file offset 0x91778) AND u32@0x104 == 0x200000.
 * The section size-walk (FUN_180042ff0) is the recorded follow-up; the
 * DWG-pattern bounded extent stands in. Row max = S-041's census
 * bound (200 MB). */
CarveResult validate_3ds(IStorageDevice* s, u64 hit, u64 cap) {
    /* M-106: the MM branch carries the section walk (FUN_180042ff0
     * :50921-50971, decoded in s8-batch2-formats.md:31): per chunk at
     * the cursor - the 0x58 SHORTCUT when the u16 at chunk+0x58 is
     * 0x4D4D (the next chunk's MM marker); else the chunk must start
     * MM (u16@0), the u16@+0x58 must be non-zero, and the stride is
     * the u16@+2 (non-zero); the cursor advances by the stride; the
     * walk caps at 5x the device (the og's while bound) and sizes at
     * the cursor (the row's 200 MB cap is the trial-loop bound). The
     * NCSD/CCI branch KEEPS the bounded stand-in: the walk's chunk
     * grammar is MM-marked, and the CCI form's first 0x100 bytes (the
     * CCI header before the NCSD magic) are not MM-marked - applying
     * the MM walk there would reject every CCI file (a regression the
     * finding's "both stand-ins" step did not intend; the CCI-leg
     * stand-in is registered here). */
    CarveResult r;
    u8 h[0x12];
    if (rd(s, hit, h, sizeof(h)) && std::memcmp(h, "MM", 2) == 0 &&
        u16_from_bytes_le(h + 0x10) == 0x3D3Du) {
        const u64 dev5 = s->size() * 5; /* the og's 5x-device walk cap */
        const u64 limit = probe_limit(s, hit, cap);
        u64 pos = 0; /* the relative cursor */
        u8 c[0x5a];  /* the chunk head: the fields live at +0/+2/+0x58 */
        for (;;) {
            if (!rd(s, hit + pos, c, sizeof(c)))
                return r; /* the og's fetch miss: the walk returns 0 (no size) */
            u64 stride = 0;
            if (u16_from_bytes_le(c + 0x58) == 0x4D4Du) {
                stride = 0x58; /* the shortcut: the next chunk's MM marker */
            } else {
                if (u16_from_bytes_le(c) != 0x4D4Du)
                    break; /* the chunk must start MM */
                if (u16_from_bytes_le(c + 0x58) == 0)
                    break; /* the non-zero next-marker gate */
                stride = u16_from_bytes_le(c + 2);
                if (stride == 0)
                    break; /* the degenerate stride */
            }
            pos += stride;
            if (pos > dev5)
                break; /* the 5x-device cap */
        }
        r.ok = true;
        r.end = std::min(hit + pos, limit);
        r.confidence = 620;
        r.note = "3ds-mm-chunk-gate+section-walk";
        return r;
    }
    u8 a[8];
    if (rd(s, hit + 0x100, a, sizeof(a)) && std::memcmp(a, "NCSD", 4) == 0 &&
        u32_from_bytes_le(a + 4) == 0x200000u) {
        r.ok = true;
        r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
        r.confidence = 620;
        r.note = "3ds-ncsd-gate+bounded-extent";
        return r;
    }
    return r;
}

/* 3DS-NCSD: the same id-0x01 chain's LIST-layer registration
 * (:3223-3243, first-hand) - the og tries the analyser at probed
 * positions WITHOUT the MM key, which is how a CCI file whose NCSD
 * header sits at +0x100 (junk before it) ever reaches the validator.
 * The core's equivalent: this late-magic row; the grammar is the
 * branch-2 pair re-checked ('NCSD'@+0x100 + u32@0x104 == 0x200000). */
CarveResult validate_3ds_ncsd(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 a[8];
    if (!rd(s, hit + 0x100, a, sizeof(a)))
        return r;
    if (std::memcmp(a, "NCSD", 4) != 0 || u32_from_bytes_le(a + 4) != 0x200000u)
        return r;
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 620;
    r.note = "3ds-ncsd-list-layer+bounded-extent";
    return r;
}

/* ---- M-121: Outlook Express Dbx (RawSpeed::DbxAnalyser, id 0x15=21; the
 * renumber chain M-117 -> M-118 -> M-121 per SA-9) ----
 * The og registers THREE magic keys (the static initializer
 * RAWSearchDLL_decompiled.c:2192-2232, each node stamped idx 0x15):
 *   "JMF6" 0x36464d4a / CF AD 12 FE 0xfe12adcf / F7 89 34 2C 0x2c3489f7.
 * The identity probe (FUN_180016740 :17509-17562) stamps id 0x15 iff
 * the 16-byte window is one of:
 *   - the JMF6 versioned form: first u64 == 0x0100003_36464d4a
 *     ("JMF6" + 03 00 10 00);
 *   - one of FOUR Outlook Express store GUIDs (the stack table
 *     :17529-17541): {fe12adcf, 6f74fdc5|c6|c7, 11d1e366, c0004e9a}
 *     (three generations) or {fe12adcf, 26fe9d30, 11d21a8f, 6000bfaa};
 *   - the marked 16-byte pattern {2c3489f7, 6deef0f7, d18922af,
 *     b0f0ff00}.
 * The size pass (FUN_180016950, disassembly-verified at
 * 0x180016950-0x180016ac0): the FIRST Analyse call re-checks the marked
 * pattern and latches it (the ctor's u16@+0x118 = 0x100 = the probe-once
 * flag pair); the plain forms size from u32@+0x7C (`mov 0x7c(%rdx),%eax`);
 * the marked form (gate: >8 valid window bytes) walks
 * size = (cur_sector - match_sector) * sector + FUN_180042b30(window,
 * sector) - the pending-object recompute adapted to the core's one-shot
 * validator as the walk below (D-101's registered stateless convention).
 * FUN_180042b30 (Ghidra + retdec + simulation cross-pinned this session):
 * 0 on an all-zero window, else the non-zero prefix ROUNDED UP TO EVEN
 * (prefix 1->2, 2->2, 3->4 ... 511->512; the bisection's mid+1 landing).
 * The marked form's 272-byte name buffer is memset-0-then-copied
 * (disasm 0x180016a16 memset -> 0x180016a1b walk): the embedded name is
 * EMPTY - the og emits no content prefix, and the S-049 default names
 * the row (DBX_%06d.DBX). */
CarveResult validate_dbx(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[16];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    const u64 lo = u64_from_bytes_le(h);
    const u64 hi = u64_from_bytes_le(h + 8);
    enum Form { FORM_NONE, FORM_JMF6, FORM_PLAIN, FORM_MARKED } form = FORM_NONE;
    if (lo == 0x0010000336464d4aULL) { /* "JMF6"+03 00 10 00 (disasm 0x180016766/0x18001676f: movl
                                          $0x36464d4a + movl $0x00100003 - the parallel lane's
                                          reading, confirmed; my first landing's 03 00 01 00 was a
                                          literal-expansion error) */
        form = FORM_JMF6;              /* "JMF6" + 03 00 10 00 */
    } else {
        /* the four OE store GUIDs (the probe's stack table) */
        static const u64 kGuids[4][2] = {
            {0x6f74fdc5fe12adcfULL, 0xc0004e9a11d1e366ULL},
            {0x6f74fdc6fe12adcfULL, 0xc0004e9a11d1e366ULL},
            {0x6f74fdc7fe12adcfULL, 0xc0004e9a11d1e366ULL},
            {0x26fe9d30fe12adcfULL, 0x6000bfaa11d21a8fULL},
        };
        for (const auto& g : kGuids) {
            if (lo == g[0] && hi == g[1]) {
                form = FORM_PLAIN;
                break;
            }
        }
        if (form == FORM_NONE && lo == 0x6deef0f72c3489f7ULL && hi == 0xb0f0ff00d18922afULL)
            form = FORM_MARKED;
    }
    if (form == FORM_NONE)
        return r; /* magic without a table identity: the probe rejects */
    if (form == FORM_JMF6 || form == FORM_PLAIN) {
        /* size = the DBX's own self-declared u32 @ +0x7C */
        u8 sz[4];
        if (!rd(s, hit + 0x7C, sz, sizeof(sz)))
            return r; /* the og's >=512-byte window always covers it */
        u64 size = u64(u32_from_bytes_le(sz));
        if (size < 0x80)
            return r; /* below the size field's own read bound */
        const u64 limit = probe_limit(s, hit, cap);
        if (size > limit - hit)
            size = limit - hit; /* the PST family precedent: clamp, not drop */
        r.ok = true;
        r.end = hit + size;
        r.confidence = form == FORM_JMF6 ? 780 : 800;
        r.note = form == FORM_JMF6 ? "dbx-jmf6+size@0x7c" : "dbx-guid+size@0x7c";
        return r;
    }
    /* the marked form: FUN_180016950's walk, one-shot form. The og
     * recomputes size per scan window; the terminal value is the
     * distance to the first all-zero sector (FUN_180042b30 -> 0), or
     * distance + the even-rounded non-zero prefix while content runs.
     * Walk sector windows from the hit; keep the last computed extent;
     * bound by the row cap (probe_limit). */
    const u64 ssz = s->geometry().logical_sector >= 512 ? s->geometry().logical_sector : 512;
    const u64 limit = probe_limit(s, hit, cap);
    std::vector<u8> win(size_t(ssz), 0);
    u64 end = hit + 16; /* the pattern itself: the first window's floor */
    u64 p = hit;
    while (p + ssz <= limit) {
        if (!rd(s, p, win.data(), u32(ssz)))
            break; /* keep the last computed extent (the og's window miss) */
        /* FUN_180042b30: all-zero window -> 0; else the non-zero prefix
         * rounded up to even (the bisection's mid+1 landing) */
        size_t last = 0;
        bool any = false;
        for (size_t i = 0; i < win.size(); ++i)
            if (win[i] != 0) {
                any = true;
                last = i;
            }
        if (!any) {
            end = p; /* the all-zero window: size = the distance alone */
            break;
        }
        size_t ext = last + 1;
        ext += ext & 1; /* round up to even (FUN_180042b30's landing) */
        if (ext > win.size())
            ext = win.size();
        end = p + ext;
        p += ssz;
    }
    if (end <= hit)
        return r;
    r.ok = true;
    r.end = std::min(end, limit);
    r.confidence = 700;
    r.note = "dbx-marked-zero-walk";
    return r;
}

/* ---- M-098: HIK (Hikvision surveillance, CHIKAnalyser id 0x92) ----
 * The og's list-layer probe (FUN_1800272a0 :30412-30484, the RawConfig
 * "HIK" token - shipped HIK=1/enabled) discriminates the vendor's
 * private_stream_2 PES inside an MPEG-PS pack (M-139: the discriminator
 * constant is -0x43ff0000 = 0xBC010000 = LE bytes 00 00 01 BC at BOTH
 * probe sites :30438/:30452, grep-verified against the retdec twin -
 * the M-098 landing's "private_stream_1"/BD reading was a transcription
 * error; 00 00 01 BD would be -0x42ff0000, which appears nowhere):
 *   - the 00 00 01 BA pack header; the pack length = (buf[0xD]&7)+0xE;
 *   - the SYSTEM-HEADER path: 00 00 01 BB at the pack end, the LE-u32
 *     skip length at +4 bounded < 0x40000001 with the total < 0x400001,
 *     then 00 00 01 BC at the skip end - the private_stream_2 PES;
 *   - or the DIRECT path: 00 00 01 BC straight after the pack;
 *   - the size starts at the PES's BE-u16 length at +4: pos + 6 + len
 *     (pos = the BC position, relative to the candidate start);
 *   - the walk (FUN_180027480 :30489-30558) continues the PS structure
 *     from the probe's size: a 00 00 01 BA pack advances 0x1A + the
 *     BE-u16 at +0x18 when low16(u32@+0x14)==0 && byte@+0x16==1 (else
 *     +0 only); any other 00 00 01 start code advances 6 + the BE-u16
 *     PES length at +4; the first non-start-code position (or an
 *     unreadable window) ends the walk; the cap is 0x40000000 (1 GiB)
 *     - on the cap exit the size is the cap EXACT (FileInfo+0x18 =
 *     0x40000000), on the break exit the last advanced cursor.
 * The analyser names its files L"HIK" (DAT_18006b428, dumped from the
 * og binary: 48 00 49 00 4B 00 - the UTF-16LE form; the direct-BD
 * branch's std::string length 3 confirms the 3-char read). */
CarveResult validate_hik(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[14];
    if (!rd(s, hit, h, sizeof(h)))
        return r; /* the pack header: the stuffing byte at +0xD needs 14 */
    if (h[0] != 0 || h[1] != 0 || h[2] != 1 || h[3] != 0xBA)
        return r; /* the registry key re-checked */
    const u64 pack_len = (h[0xD] & 7) + 0xE;
    u8 sc[6];
    if (!rd(s, hit + pack_len, sc, sizeof(sc)))
        return r; /* the start code after the pack + the length head */
    u64 pos = 0;  /* the BD position, relative to the candidate start */
    if (sc[0] == 0 && sc[1] == 0 && sc[2] == 1 && sc[3] == 0xBB) {
        /* the system-header path: the LE-u32 skip length at +4 */
        u8 lb[4];
        if (!rd(s, hit + pack_len + 4, lb, sizeof(lb)))
            return r;
        const u32 len = u32_from_bytes_le(lb);
        if (len >= 0x40000001u)
            return r; /* the og's first length bound */
        const u64 p = pack_len + 6 + len;
        if (p >= 0x400001u)
            return r; /* the og's total bound */
        u8 bd[6];
        if (!rd(s, hit + p, bd, sizeof(bd)))
            return r;
        if (bd[0] != 0 || bd[1] != 0 || bd[2] != 1 || bd[3] != 0xBC)
            return r; /* private_stream_2 required at the skip end (M-139) */
        pos = p;
    } else if (sc[0] == 0 && sc[1] == 0 && sc[2] == 1 && sc[3] == 0xBC) {
        pos = pack_len; /* the direct private_stream_2 after the pack */
    } else {
        return r; /* neither the system-header nor the direct form */
    }
    const u16 pes_len = rdbe16(s, hit + pos + 4);
    u64 cursor = pos + 6 + pes_len; /* the probe's size */
    /* the structural walk: packs advance 0x1A (+ the BE-u16 at +0x18
     * under the +0x14/+0x16 gate), other PES advance 6 + BE-u16@+4 */
    const u64 limit = probe_limit(s, hit, cap);
    while (cursor < 0x40000000ull) {
        u8 w[0x1A];
        if (!rd(s, hit + cursor, w, 4))
            break; /* the unreadable window ends the walk */
        if (w[0] != 0 || w[1] != 0 || w[2] != 1)
            break; /* the first non-start-code position ends the walk */
        if (w[3] == 0xBA) {
            u64 extra = 0;
            if (rd(s, hit + cursor, w, sizeof(w))) {
                const u32 w14 = u32_from_bytes_le(w + 0x14);
                if ((w14 & 0xFFFFu) == 0 && w[0x16] == 1)
                    extra = rdbe16(s, hit + cursor + 0x18);
            }
            cursor += 0x1A + extra;
        } else {
            cursor += 6 + rdbe16(s, hit + cursor + 4);
        }
    }
    if (cursor > 0x40000000ull)
        cursor = 0x40000000ull; /* the cap exit: the size is the cap exact */
    if (hit + cursor > limit)
        cursor = limit - hit; /* the slice edge clamp (the engine's window) */
    r.ok = true;
    r.end = hit + cursor;
    r.confidence = 650;
    r.note = "hik-ps-private-stream-2";
    r.name_prefix = u"HIK"; /* DAT_18006b428: L"HIK" - the analyser's own name */
    return r;
}

/* ---- M-099: AEP (After Effects project, AEPAnalyser id 0x9c) ----
 * The og's probe (FUN_18000efa0 :11550-11598, the RawConfig "AEP"
 * token - shipped AEP=1/enabled) accepts BARE RIFX (0x58464952,
 * big-endian RIFF) with NO body check - the og's own deliberate weak
 * grammar: any RIFX passes. Ported AS-IS: strengthening it with an
 * "Egg!" gate the og lacks would diverge. The analyser names its
 * files L"AEP" (DAT_180067008, dumped from the og binary:
 * 41 00 45 00 50 00). The size walk (FUN_18000f0d0 :11603-11840):
 *   - size = the BE-u32 chunk length at +4 (the byteswapped read
 *     :11690-11695 - the RIFX container's own length field);
 *   - len >= 0x1e00001 -> the 0x1e00000 (30 MiB) cap EXACT;
 *   - else the chunked scan: 0x200-byte windows from size+8 (the
 *     first window's own +8 offset, :11688), the size counter
 *     advancing 0x200 per miss with the next window fetched AT the
 *     counter (the og's 8-byte first-window overlap, :11784-11785),
 *     while size < 0x1e00000:
 *       - the 13-byte "<?xpacket end" XMP terminator found at offset
 *         o in the window at base b -> size = b + o + 6 (the og's
 *         +6 - six bytes past the marker START, :11761);
 *       - a window whose LAST 8 bytes are all zero triggers the
 *         zero-run walk (:11769-11840): the first 8 consecutive
 *         zero bytes at offset d in the window (d+7 < 0x1ff) ->
 *         size = size + d (the zero-tail trim);
 *       - the cap exit sets 0x1e00000 exact;
 *   - an unreadable window keeps the current size (the og's window
 *     miss exits with the counter's value, :11782). */
CarveResult validate_aep(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[8];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (h[0] != 'R' || h[1] != 'I' || h[2] != 'F' || h[3] != 'X')
        return r; /* the registry key re-checked (the WEAK probe: no body gate) */
    const u64 limit = probe_limit(s, hit, cap);
    u64 size = rdbe32(s, hit + 4); /* the BE-u32 chunk length (byteswapped read) */
    if (size >= 0x1e00001ull) {
        size = 0x1e00000ull; /* the 30 MiB cap, exact */
    } else {
        /* the chunked XMP-terminator scan: the first window at
         * size+8, then the 0x200 stepping (the size counter and the
         * window base move together after the first window) */
        static const char kNeedle[14] = "<?xpacket end"; /* 13 chars + NUL */
        std::vector<u8> w(0x200);
        bool first = true;
        while (size < 0x1e00000ull) {
            const u64 wbase = first ? size + 8 : size;
            first = false;
            const u64 avail = limit > hit + wbase ? limit - hit - wbase : 0;
            if (avail == 0)
                break; /* the unreadable window keeps the size */
            const u32 take = u32(std::min<u64>(0x200, avail));
            if (!rd(s, hit + wbase, w.data(), take))
                break; /* the og's window miss: the size keeps */
            /* the XMP terminator within the window (the marker may
             * straddle the window tail - the og's find is per-window,
             * a straddling marker is missed: ported exactly) */
            u64 found = UINT64_MAX;
            for (u32 i = 0; i + 13 <= take; ++i) {
                if (std::memcmp(w.data() + i, kNeedle, 13) == 0) {
                    found = i;
                    break;
                }
            }
            if (found != UINT64_MAX) {
                size = wbase + found + 6; /* the og's +6 past the marker start */
                break;
            }
            /* the window's last 8 bytes all zero: the zero-run walk */
            bool tail_zero = take >= 8;
            for (u32 i = take - 8; i < take; ++i)
                if (w[i] != 0) {
                    tail_zero = false;
                    break;
                }
            if (tail_zero) {
                /* the first 8 consecutive zero bytes in the window */
                for (u32 d = 0; d + 8 <= take; ++d) {
                    bool zeros = true;
                    for (u32 i = 0; i < 8; ++i)
                        if (w[d + i] != 0) {
                            zeros = false;
                            break;
                        }
                    if (zeros && d + 7 < 0x1ff) {
                        size = size + d;
                        goto zero_trim_done;
                    }
                }
                /* no qualifying run: the og's scan falls through the
                 * window and the counter advances */
            }
            size += 0x200;
        }
        if (size > 0x1e00000ull)
            size = 0x1e00000ull; /* the cap exit: the cap exact */
    }
zero_trim_done:
    r.ok = true;
    r.end = std::min(hit + size, limit);
    r.confidence = 620;
    r.note = "aep-rifx-weak+xmp-end";
    r.name_prefix = u"AEP"; /* DAT_180067008: L"AEP" - the analyser's own name */
    return r;
}

} // namespace carve
} // namespace cr
