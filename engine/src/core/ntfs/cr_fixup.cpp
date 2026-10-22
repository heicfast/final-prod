/* cr_fixup.cpp - USA fixup validate/apply (corpus FUN_180041bb0 chain).
 *
 * Rules (corpus ground-truth contracts section 3):
 *   - record_bytes >= 0x400 (min parse size)
 *   - usa_offset != 0 and usa_offset % 2 == 0; usa_count = sectors + 1
 *   - every sector i: u16 at (i * sector_size - 2) must equal usa[i]
 *   - apply: that trailing u16 is replaced by usa[i + 1]
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_fixup.h"

namespace cr {
namespace ntfs {

namespace {
constexpr u64 MIN_PARSE_BYTES = 0x400; /* corpus: fixup min size gate */
} // namespace

u32 fixup_validate(const u8 *record, u64 record_bytes, u32 usa_offset, u32 usa_count,
                   u32 sector_size) {
    if (record == nullptr || record_bytes < MIN_PARSE_BYTES) return ERR_BAD_PARAMETER;
    if (sector_size < 256 || sector_size > 65536) return ERR_BAD_PARAMETER;
    if (usa_offset == 0 || usa_offset & 1) return ERR_BAD_PARAMETER; /* corpus: nonzero */
    if (usa_count < 2) return ERR_BAD_PARAMETER;
    const u32 sectors = usa_count - 1;
    if (u64(sectors) * sector_size > record_bytes) return ERR_BAD_PARAMETER;
    if (u64(usa_offset) + u64(usa_count) * 2 > record_bytes) return ERR_BAD_PARAMETER;

    /* USA[0] is the single USN that ALL sector tails must carry */
    const u16 usn = u16_from_bytes_le(record + usa_offset);
    for (u32 i = 0; i < sectors; ++i) {
        const u8 *tail = record + u64(i) * sector_size + sector_size - 2;
        if (u16_from_bytes_le(tail) != usn) return ERR_FIXUP_FAILED;
    }
    return OK;
}

u32 fixup_apply(u8 *record, u64 record_bytes, u32 usa_offset, u32 usa_count, u32 sector_size) {
    /* validate first - apply cannot fail afterwards, so no rollback path
     * and no partially-applied state is ever observable */
    u32 rc = fixup_validate(record, record_bytes, usa_offset, usa_count, sector_size);
    if (rc != OK) return rc;
    const u32 sectors = usa_count - 1;
    for (u32 i = 0; i < sectors; ++i) {
        u8 *tail = record + u64(i) * sector_size + sector_size - 2;
        u16 replace = u16_from_bytes_le(record + usa_offset + u32(i + 1) * 2);
        tail[0] = static_cast<u8>(replace);
        tail[1] = static_cast<u8>(replace >> 8);
    }
    return OK;
}

} // namespace ntfs
} // namespace cr
