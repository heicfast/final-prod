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
    /* slots arrive in DISK order (usually last-fragment first): sort by
     * sequence number; bit 0x40 marks the LAST fragment (highest seq). */
    std::vector<const LfnEntry*> ordered = slots;
    std::sort(ordered.begin(), ordered.end(), [](const LfnEntry* a, const LfnEntry* b) {
        return (a->sequence & 0x3F) < (b->sequence & 0x3F);
    });
    if (ordered.front()->checksum != checksum)
        return false; /* corrupt: 8.3 fallback */
    out.clear();
    for (const LfnEntry* e : ordered) {
        if (e->checksum != checksum)
            return false;
        /* 13 chars per slot: 5 + 6 + 2 */
        const u16* parts[3] = {e->name1, e->name2, e->name3};
        for (int pi = 0; pi < 3; ++pi) {
            for (int ci = 0; ci < (pi == 0 ? 5 : (pi == 1 ? 6 : 2)); ++ci) {
                u16 ch = parts[pi][ci];
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
    const u32 year = 1980u + ((date >> 9) & 0x7F);
    const u32 month = (date >> 5) & 0x0F;
    const u32 day = date & 0x1F;
    const u32 hour = (time >> 11) & 0x1F;
    const u32 minute = (time >> 5) & 0x3F;
    const u32 second = (time & 0x1F) * 2;
    (void)tenths;
    if (month < 1 || month > 12 || day < 1 || day > 31)
        return 0;
    /* days since 1970-01-01 (Howard Hinnant civil algorithm) */
    const i64 y = i64(year) - (month <= 2 ? 1 : 0);
    const i64 era = (y >= 0 ? y : y - 399) / 400;
    const u64 yoe = u64(y - era * 400);
    const u64 doy = (153u * ((month > 2 ? month - 3 : month + 9)) + 2) / 5 + day - 1;
    const u64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const i64 days = era * 146097 + i64(doe) - 719468;
    if (days < 0)
        return 0;
    const u64 secs = u64(days) * 86400 + u64(hour) * 3600 + u64(minute) * 60 + second;
    return secs > 0xFFFFFFFFULL ? 0 : u32(secs);
}

} // namespace fat
} // namespace cr
