/* cr_lfn.cpp - LFN assembly + 8.3 name conversion (R5.1).
 *
 * Corpus: VFAT slots assembled before the 8.3 entry; checksum ties them.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/fat/cr_fat_engine.h"
#include <algorithm>

namespace cr {
namespace fat {

u8 lfn_checksum(const u8 short_name[11]) {
    u32 sum = 0;
    for (int i = 0; i < 11; ++i)
        sum = ((sum & 1) << 7) + (sum >> 1) + u32(short_name[i]);
    return u8(sum & 0xFF);
}

bool lfn_assemble(const std::vector<const LfnEntry*>& slots, u8 checksum, std::u16string& out) {
    if (slots.empty())
        return false;
    /* slots arrive in DISK order (usually last-fragment first). Live VFAT
     * entries are stored in DESCENDING sequence order (last fragment
     * physically first); the sequence byte orders them.
     *
     * FAT-1 (AUDIT-FAT, P1): DELETED slots have the sequence byte
     * overwritten with 0xE5 - every key under & 0x3F collapses to 0x25, the
     * sort degenerates to disk order, and the fragments concatenate in
     * REVERSE (garbage names for every deleted-LFN file; the per-slot
     * checksum still matches the 8.3 alias, so the mangled name won over
     * the correct 8.3 fallback). The physical DESCENDING layout survives
     * deletion untouched - reverse the disk order for deleted slots. The
     * 0x40 last-fragment bit is destroyed with the byte, so no other
     * ordering signal exists. */
    std::vector<const LfnEntry*> ordered = slots;
    const bool deleted_slots = ordered.front()->sequence >= 0xE0;
    if (deleted_slots) {
        std::reverse(ordered.begin(), ordered.end());
    } else {
        std::sort(ordered.begin(), ordered.end(), [](const LfnEntry* a, const LfnEntry* b) {
            return (a->sequence & 0x3F) < (b->sequence & 0x3F);
        });
    }
    if (ordered.front()->checksum != checksum)
        return false; /* corrupt: 8.3 fallback */
    out.clear();
    for (const LfnEntry* e : ordered) {
        if (e->checksum != checksum)
            return false;
        /* 13 chars per slot: 5 + 6 + 2. N-018/UBSan: the on-disk LFN
         * layout puts name1 at an ODD offset (+1); loading through u16*
         * members is unaligned UB (fine on x86, trapped by UBSan, wrong
         * on strict-alignment targets). Byte-wise le16 loads - the
         * compiler folds them to the same instructions on x86. */
        const u8* base = reinterpret_cast<const u8*>(e);
        static constexpr size_t kNameOff[3] = {1, 14, 28}; /* name1/2/3 */
        static constexpr int kNameCnt[3] = {5, 6, 2};
        for (int pi = 0; pi < 3; ++pi) {
            for (int ci = 0; ci < kNameCnt[pi]; ++ci) {
                u16 ch = u16(base[kNameOff[pi] + 2 * size_t(ci)]) |
                         u16(base[kNameOff[pi] + 2 * size_t(ci) + 1]) << 8;
                if (ch == 0x0000) { /* end of name */
                    return !out.empty();
                }
                if (ch == 0xFFFF)
                    continue; /* padding */
                out.push_back(ch);
            }
        }
    }
    return !out.empty();
}

bool lfn_deleted_checksum_recover(const std::vector<const LfnEntry*>& slots,
                                  const u8 short_name_raw[11], u8& out_first_byte,
                                  u8& out_checksum) {
    out_first_byte = 0;
    out_checksum = 0;
    if (slots.empty())
        return false;
    /* all slots must agree on the stored checksum (a torn/mixed set has no
     * oracle); FAT-10 already filtered junk 0x0F entries. */
    const u8 stored = slots.front()->checksum;
    for (const LfnEntry* e : slots)
        if (e->checksum != stored)
            return false;
    /* brute-force the destroyed first byte: the checksum folds byte 0 in
     * first and rotates it 10 more times, so its contribution to the final
     * sum is ROR^10(b0) - a bijection over the 256 candidates. Exactly one
     * candidate can match `stored`; zero matches means the set is corrupt. */
    u8 candidate = 0;
    bool found = false;
    for (u32 c = 0; c <= 0xFF; ++c) {
        u8 name[11];
        std::memcpy(name, short_name_raw, 11);
        name[0] = u8(c);
        if (lfn_checksum(name) == stored) {
            if (found && candidate != u8(c))
                return false; /* unreachable for this checksum family */
            candidate = u8(c);
            found = true;
        }
    }
    if (!found)
        return false;
    out_first_byte = candidate;
    out_checksum = stored;
    return true;
}

/* N-087: the OEM-codepage decode for 8.3 short names. The og persists
 * the raw OEM bytes (ifileproperties-part1:121 - control-byte names
 * from deleted entries survive verbatim; the mounting system's
 * codepage supplies the glyphs). The core's name carrier is UTF-16, so
 * the faithful boundary conversion is OEM -> Unicode at the name-build
 * site; CP437 is the corpus-observable default (the US-market boot
 * tables; the og's exact UI codepage is not decoded in part1 - the
 * open-question note in the finding). 0x20..0x7E pass through (ASCII
 * names are byte-identical); the high half maps through the table; the
 * 0x05-first-byte KANJI escape and the 0xE5 deleted marker are
 * consumed by the CALLER before this point (cr_fat_quick.cpp:202-203),
 * so 0xE5 arriving here is a genuine name byte (CP437 alpha). */
static const u16 kCp437[128] = {
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, /* 80-87 */
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, /* 88-8F */
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, /* 90-97 */
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, /* 98-9F */
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, /* A0-A7 */
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, /* A8-AF */
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, /* B0-B7 */
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, /* B8-BF */
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255A, /* C0-C7 */
    0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, 0x2568, /* C8-CF */
    0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, /* D0-D7 */
    0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 0x03A9, /* D8-DF */
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, /* E0-E7 */
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x2205, 0x2208, 0x2204, /* E8-EF */
    0x2264, 0x2265, 0x03C0, 0x2260, 0x00A4, 0x03B1, 0x2248, 0x222B, /* F0-F7 */
    0x00A0, 0x00B7, 0x221A, 0x0192, 0x2248, 0x2202, 0x25A0, 0x00A0, /* F8-FF */
};

namespace {
/* N-175 (diff-to-fix): the og's 8.3 display-case fold - a four-way
 * EXACT-VALUE switch on the NT reserved byte (FATSearchDLL
 * :44191-44216, re-read first-hand):
 *   NTRes 0x00 -> base UPPER / ext UPPER
 *   NTRes 0x08 -> base lower / ext UPPER
 *   NTRes 0x10 -> base UPPER / ext lower
 *   NTRes 0x18 -> base lower / ext lower
 *   anything else -> the raw on-disk bytes (the og's switch is an
 *     exact compare, NOT a mask - NTRes bits 0x01/0x02, Windows'
 *     "already permanently lowercase" markers, take the raw path).
 * Windows stores the display case of NT-created 8.3 names in these
 * two bits (bit 3 = base lowercase, bit 4 = extension lowercase);
 * the on-disk bytes are conventionally uppercase. The fold is
 * ASCII-only (the og's _strupr/_strlwr are ANSI CRT calls; CP437
 * high-half glyphs are untouched, matching the locale-safe og). */
void ntres_fold_u16(std::u16string& s, bool lower) {
    for (char16_t& c : s) {
        if (lower && c >= u'A' && c <= u'Z')
            c = char16_t(c + 0x20);
        else if (!lower && c >= u'a' && c <= u'z')
            c = char16_t(c - 0x20);
    }
}
} // namespace

std::u16string short_name_to_u16(const u8 name[11], u8 nt_reserved) {
    std::u16string s;
    int base_end = 8;
    while (base_end > 0 && name[base_end - 1] == ' ')
        --base_end;
    for (int i = 0; i < base_end; ++i) {
        const u8 c = name[i];
        s.push_back(c >= 0x80 ? kCp437[c - 0x80] /* N-087: OEM -> Unicode */ : u16(c));
    }
    int ext_end = 3;
    while (ext_end > 0 && name[8 + ext_end - 1] == ' ')
        --ext_end;
    std::u16string ext;
    if (ext_end > 0) {
        for (int i = 0; i < ext_end; ++i) {
            const u8 c = name[8 + i];
            ext.push_back(c >= 0x80 ? kCp437[c - 0x80] : u16(c));
        }
    }
    switch (nt_reserved) {
    case 0x00:
        ntres_fold_u16(s, false);
        ntres_fold_u16(ext, false);
        break;
    case 0x08:
        ntres_fold_u16(s, true);
        ntres_fold_u16(ext, false);
        break;
    case 0x10:
        ntres_fold_u16(s, false);
        ntres_fold_u16(ext, true);
        break;
    case 0x18:
        ntres_fold_u16(s, true);
        ntres_fold_u16(ext, true);
        break;
    default:
        break; /* raw bytes (the og's exact-match switch, not a mask) */
    }
    if (!ext.empty()) {
        s.push_back(u'.');
        s += ext;
    }
    return s;
}

u64 fat_date_to_unix(u16 date, u16 time, u8 tenths) {
    /* FAT date: bits 15-9 = year-1980, 8-5 month, 4-0 day.
     * FAT time: bits 15-11 hour, 10-5 minute, 4-0 second/2.
     * tenths: 10ms units on the creation field (ignored for seconds math). */
    (void)tenths;
    /* N-173: the u32 clamp is GONE - the og's DosDateTimeToFileTime
     * (FATSearchDLL :32135/:32144/:32151) converts the FULL FAT year
     * field (<= 2107, year 127) with no 2106 cutoff, and the
     * FileTimes contract is u64 (cr_types.h) - the stale "u32 props
     * contract" comment described a contract that never existed. The
     * shared core (W4 dedup, AUDIT-FAT §6 row 20) still returns 0 for
     * the INVALID-date class (month 0/13, day 0/32 - the N-021 zero-
     * FileTime semantics preserved exactly); the exFAT sibling
     * (exfat_time_to_unix100) already had no clamp - the two FAT-family
     * decoders now agree. */
    return fat_datetime_to_unix_seconds(date, time);
}

} // namespace fat
} // namespace cr
