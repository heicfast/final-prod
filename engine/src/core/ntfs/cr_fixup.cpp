/* cr_fixup.cpp - USA fixup validate/apply (corpus FUN_1800411f0 + FUN_180041290).
 *
 * Rules (corpus ground-truth contracts section 3 + decompile):
 *   - record_bytes >= 0x400 (min parse size)
 *   - usa_offset != 0 and even; usa_count = sectors + 1
 *   - VALIDATION checks only the FIRST TWO sector tails against USA[0]
 *     (og FUN_1800411f0 loop bound 'local_18 < 2', decompiled line 52129:
 *     salvage tolerance — records with a bad 3rd+ tail are still accepted;
 *     identical for 1 KiB records, differs only for >= 2 KiB records)
 *   - apply: every sector tail is replaced by usa[i + 1] (FUN_180041290
 *     walks all sectors, bounded by the record size)
 *   - sector size is parameterized (4Kn-aware; og hardcodes 0x200 — fix F1
 *     family, documented delta)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_fixup.h"

#include <cstring>

namespace cr {
namespace ntfs {

namespace {
constexpr u64 MIN_PARSE_BYTES = 0x400; /* corpus: fixup min size gate */
constexpr u32 VALIDATE_TAIL_COUNT = 2; /* og FUN_1800411f0: first two tails */
/* N-213 (the og's record-header pre-gate band, re-read first-hand — the
 * og keys the floor on the record family): FUN_180046160 :55187-55220
 * gates FILE records to usa_offset ∈ [0x2A, 0x100] with usa_count ≤ 16
 * (the pre-gate before the fixup chain, FUN_180041bb0); FUN_180045510
 * :54704-54745 gates INDX blocks to usa_offset ∈ [0x28, 0x100] with the
 * same count cap (the INDX header's shorter tail lowers the floor by
 * two bytes). Real volumes carry 0x30 (1 KiB records) / 0x48 (4 KiB
 * records) / 0x28 (INDX), all inside; the bands reject the torn/random
 * residue whose offset lands below the family floor or above 0x100.
 * The og's alloc_size-set half of the FILE pre-gate is remnant-path-only
 * (see cr_mftwalk.cpp) - on the main walk the volume's declared record
 * size makes it redundant, and the >4 KiB half is the registered
 * deliberate D-143 superset. */
constexpr u32 USA_OFFSET_MIN_FILE = 0x2A;
constexpr u32 USA_OFFSET_MIN_INDX = 0x28;
constexpr u32 USA_OFFSET_MAX = 0x100;
constexpr u32 USA_COUNT_MAX = 0x10;
} // namespace

u32 fixup_validate(const u8* record, u64 record_bytes, u32 usa_offset, u32 usa_count,
                   u32 sector_size) {
    if (record == nullptr || record_bytes < MIN_PARSE_BYTES)
        return ERR_BAD_PARAMETER;
    if (sector_size < 256 || sector_size > 65536)
        return ERR_BAD_PARAMETER;
    if (usa_offset == 0 || usa_offset & 1)
        return ERR_BAD_PARAMETER; /* corpus: nonzero */
    /* N-213: the family-keyed band (FILE: < 0x2A or > 0x100 fails,
     * FUN_180046160 :55208-55216; INDX: < 0x28 or > 0x100 fails,
     * FUN_180045510 :54733-54742; count >= 0x11 fails in both) */
    const u32 floor =
        (std::memcmp(record, "INDX", 4) == 0) ? USA_OFFSET_MIN_INDX : USA_OFFSET_MIN_FILE;
    if (usa_offset < floor || usa_offset > USA_OFFSET_MAX)
        return ERR_BAD_PARAMETER;
    if (usa_count > USA_COUNT_MAX)
        return ERR_BAD_PARAMETER;
    if (usa_count < 2)
        return ERR_BAD_PARAMETER;
    const u32 sectors = usa_count - 1;
    if (u64(sectors) * sector_size > record_bytes)
        return ERR_BAD_PARAMETER;
    if (u64(usa_offset) + u64(usa_count) * 2 > record_bytes)
        return ERR_BAD_PARAMETER;

    /* USA[0] is the single USN that the FIRST TWO sector tails must carry
     * (og parity: FUN_1800411f0 stops after 2 — a mismatching 3rd+ tail
     * does not reject the record; salvage over strictness) */
    const u16 usn = u16_from_bytes_le(record + usa_offset);
    const u32 check = sectors < VALIDATE_TAIL_COUNT ? sectors : VALIDATE_TAIL_COUNT;
    for (u32 i = 0; i < check; ++i) {
        const u8* tail = record + u64(i) * sector_size + sector_size - 2;
        if (u16_from_bytes_le(tail) != usn)
            return ERR_FIXUP_FAILED;
    }
    return OK;
}

u32 fixup_apply(u8* record, u64 record_bytes, u32 usa_offset, u32 usa_count, u32 sector_size) {
    /* validate first - apply cannot fail afterwards, so no rollback path
     * and no partially-applied state is ever observable */
    u32 rc = fixup_validate(record, record_bytes, usa_offset, usa_count, sector_size);
    if (rc != OK)
        return rc;
    const u32 sectors = usa_count - 1;
    for (u32 i = 0; i < sectors; ++i) {
        u8* tail = record + u64(i) * sector_size + sector_size - 2;
        u16 replace = u16_from_bytes_le(record + usa_offset + u32(i + 1) * 2);
        tail[0] = static_cast<u8>(replace);
        tail[1] = static_cast<u8>(replace >> 8);
    }
    return OK;
}

} // namespace ntfs
} // namespace cr
