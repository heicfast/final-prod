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

std::u16string short_name_to_u16(const u8 name[11]) {
    std::u16string s;
    int base_end = 8;
    while (base_end > 0 && name[base_end - 1] == ' ')
        --base_end;
    for (int i = 0; i < base_end; ++i)
        s.push_back(u16(name[i]));
    int ext_end = 3;
    while (ext_end > 0 && name[8 + ext_end - 1] == ' ')
        --ext_end;
    if (ext_end > 0) {
        s.push_back(u'.');
        for (int i = 0; i < ext_end; ++i)
            s.push_back(u16(name[8 + i]));
    }
    return s;
}

u32 fat_date_to_unix(u16 date, u16 time, u8 tenths) {
    /* FAT date: bits 15-9 = year-1980, 8-5 month, 4-0 day.
     * FAT time: bits 15-11 hour, 10-5 minute, 4-0 second/2.
     * tenths: 10ms units on the creation field (ignored for seconds math). */
    (void)tenths;
    /* shared FAT-family datetime core (W4 dedup, AUDIT-FAT §6 row 20);
     * clamped to the u32 props contract - dates past 2106 saturate to 0 */
    const u64 secs = fat_datetime_to_unix_seconds(date, time);
    return secs > 0xFFFFFFFFULL ? 0 : u32(secs);
}

} // namespace fat
} // namespace cr
