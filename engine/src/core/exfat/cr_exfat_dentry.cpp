/* cr_exfat_dentry.cpp - checksums + boot region validation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/exfat/cr_exfat_engine.h"
#include <cstring>

namespace cr {
namespace exfat {

u32 checksum32(const u8* p, u32 n, u32 seed) {
    u32 sum = seed;
    for (u32 i = 0; i < n; ++i) {
        sum = ((sum & 1) ? 0x80000000u : 0u) + (sum >> 1) + p[i];
    }
    return sum;
}

/* Boot-region checksum over sectors 0..10, spec-exact skip semantics: the
 * bytes of VolumeFlags (offsets 106, 107) and PercentInUse (offset 112) in
 * the boot sector are skipped ENTIRELY - no rotation step for them
 * (exFAT spec 3.1.14.1 reference BootChecksum(); Linux kernel
 * exfat_calc_chksum32 CS_BOOT_SECTOR agrees). Shared with the repair
 * engine so the fixer and the scanner can never disagree. */
u32 boot_region_checksum(const u8* region, u32 sector_bytes) {
    u32 chk = 0;
    for (u32 s = 0; s <= 10; ++s) {
        const u8* sec = region + u64(s) * sector_bytes;
        for (u32 i = 0; i < sector_bytes; ++i) {
            if (s == 0 && ((i >= 106 && i < 108) || i == 112))
                continue;
            chk = ((chk & 1) ? 0x80000000u : 0u) + (chk >> 1) + sec[i];
        }
    }
    return chk;
}

u16 checksum16(const u8* p, u32 n, u16 seed) {
    u16 sum = seed;
    for (u32 i = 0; i < n; ++i) {
        sum = static_cast<u16>(((sum & 1) ? 0x8000u : 0u) + (sum >> 1) + p[i]);
    }
    return sum;
}

u16 entry_set_checksum(const u8* entries, u32 entry_count) {
    /* whole set byte-by-byte, excluding ONLY the 2 SetCheckSum bytes (file+2).
     * Composed from checksum16 (W4 dedup, AUDIT-FAT §6 row 19 - the rotate
     * was re-implemented inline here before): entry 0 bytes 0..1, skip the
     * SetCheckSum field, entry 0 bytes 4..31, then every following entry. */
    u16 sum = checksum16(entries, 2, 0);
    sum = checksum16(entries + 4, 28, sum);
    if (entry_count > 1)
        sum = checksum16(entries + 32, (entry_count - 1) * 32, sum);
    return sum;
}

/* M-013 (diff-to-fix): the og's DUAL-FORM entry-set checksum
 * (exFATSearchDLL FUN_18001eba0 :25153-25198, re-verified line-by-line
 * against the corpus): TWO ROL15 rotate-add sums over the set bytes
 * (skipping indices 2..3, the stored field itself):
 *   - the RAW form: the on-disk bytes as they are;
 *   - the INUSE-RESTORED form, computed ONLY when the set's first byte
 *     is 0x05 (the File entry's InUse bit cleared - a deleted set):
 *     byte 0 substituted with 0x85 (File | InUse), byte 0x20 (the
 *     Stream entry's type) with 0xC0, and every other 32-byte entry
 *     boundary with 0xC1 (File Name | InUse) - exactly the og's
 *     uVar5 ladder at :25173-25186.
 * A set is checksum-VALID iff EITHER form equals the stored field
 * (entries+2): a live set matches the raw form; a properly deleted set
 * (InUse bits cleared after the checksum was stamped over the live
 * bytes) matches the restored form. Used as the deleted-set accept/
 * reject gate and as the reconstruction oracle's acceptance test
 * (FUN_18001fef0 :26287-26292 / FUN_1800203a0 :26642-26664). */
SetChecksumForms entry_set_checksum_dual(const u8* entries, u32 entry_count) {
    SetChecksumForms out{0, 0};
    const u32 bytes = entry_count * 32;
    const bool deleted_set = entries[0] == 0x05;
    for (u32 i = 0; i < bytes; ++i) {
        if (i == 2 || i == 3)
            continue; /* the stored SetChecksum field */
        out.raw = static_cast<u16>(((out.raw & 1) ? 0x8000u : 0u) + (out.raw >> 1) + entries[i]);
        if (deleted_set) {
            u16 sub = entries[i];
            if (i == 0)
                sub = 0x85; /* File | InUse */
            else if (i == 0x20)
                sub = 0xC0; /* Stream | InUse */
            else if ((i & 0x1F) == 0)
                sub = 0xC1; /* File Name | InUse */
            out.restored =
                static_cast<u16>(((out.restored & 1) ? 0x8000u : 0u) + (out.restored >> 1) + sub);
        }
    }
    return out;
}

bool entry_set_checksum_valid(const u8* entries, u32 entry_count, u16 stored) {
    const SetChecksumForms forms = entry_set_checksum_dual(entries, entry_count);
    return forms.raw == stored || forms.restored == stored;
}

u32 validate_boot_region(IStorageDevice* dev, ExfatParams& out) {
    if (dev == nullptr || dev->geometry().logical_sector > 4096)
        return ERR_BAD_PARAMETER;
    const u32 bps = dev->geometry().logical_sector;
    if (u64(BOOT_REGION_SECTORS) * bps > dev->size())
        return ERR_RANGE_OUT_OF_BOUNDS;

    std::vector<u8> region(BOOT_REGION_SECTORS * bps);
    if (dev->read_at(0, region.data(), u32(region.size())) != OK)
        return ERR_READ_FAILED;

    /* main boot sector checks */
    const BootSectorMain* main_b = reinterpret_cast<const BootSectorMain*>(region.data());
    /* EXFAT-4 (AUDIT-FAT): og boot gates we lacked (FUN_180021e80,
     * exFATSearchDLL_decompiled.c:28180-28208) - now a helper so the
     * S-066 ladder re-runs the IDENTICAL gate set on the backup boot
     * (the og does exactly that: FUN_180001630 :357-359 feeds the +12
     * sector back through FUN_180021e80):
     *   (a) OEM + 55AA signature;
     *   (b) bps shift in [9,12] and matching the device;
     *   (c) spc shift bound;
     *   (d) MustBeZero sweep +0x0B..+0x3F (the 53-byte reserved region
     *       after JumpBoot/OEM - 28183-28196);
     *   (e) fat_count in [1,2] (28200);
     *   (f) fat_length != 0 - og rejects "no-FAT" boot sectors (28200);
     *   (g) cluster-heap fit (28206-28208; the volume-relative form,
     *       applied to the main boot below with the device bounds). */
    const auto structural_gates = [bps](const BootSectorMain* b) -> bool {
        if (!oem_is_exfat(*b))
            return false;
        if (b->signature[0] != 0x55 || b->signature[1] != 0xAA)
            return false;
        if (b->bytes_per_sector_shift < 9 || b->bytes_per_sector_shift > 12)
            return false;
        if ((1u << b->bytes_per_sector_shift) != bps)
            return false;
        if (b->sectors_per_cluster_shift > 25 - b->bytes_per_sector_shift)
            return false;
        for (int i = 0; i < 53; ++i)
            if (b->must_be_zero[i] != 0)
                return false;
        if (b->fat_count < 1 || b->fat_count > 2)
            return false;
        if (b->fat_length == 0)
            return false;
        /* the heap-fit gate in its volume-relative form (the full
         * device-bounds form runs on the main boot below) */
        if (b->cluster_count == 0 || b->cluster_count > 0xFFFFFFF5)
            return false;
        if (((u64(b->cluster_count) - 1) << b->sectors_per_cluster_shift) +
                u64(b->cluster_heap_offset) >
            b->volume_length)
            return false;
        return true;
    };
    /* the original error mapping preserved: OEM/signature failures are
     * BAD_MAGIC, the shape-gate failures are BAD_PARAMETER (the
     * refactor must not re-classify the rejects) */
    if (!oem_is_exfat(*main_b))
        return ERR_BAD_MAGIC;
    if (main_b->signature[0] != 0x55 || main_b->signature[1] != 0xAA)
        return ERR_BAD_MAGIC;
    if (!structural_gates(main_b))
        return ERR_BAD_PARAMETER;

    /* ---- S-066: the recognition re-base ladder (FUN_180001630
     * :341-420, re-read first-hand this session) runs BEFORE the volume
     * gates: the shift feeds every downstream offset and the
     * device-bound check. When the backup boot at +12 FAILS the full
     * structural gates, the anchor is presumed to BE the backup boot of
     * a volume whose primary is destroyed (exFAT places the backup
     * exactly 12 sectors after the primary); the FAT media words then
     * decide which base is true:
     *   - FAT[0..1] == {0xFFFFFFF8, 0xFFFFFFFF} at base+fat_offset ->
     *     the anchor is the primary: keep the base (damaged backup).
     *   - mismatch there BUT the pair matches at (base-12)+fat_offset
     *     and base >= 12 -> re-base the volume to base-12
     *     (:377-378 `base += -0xc`).
     * The re-base is params-local: the fat/heap offsets shift -12
     * sectors, which is exactly equivalent to re-basing the volume view
     * (the engine reads nothing below (fat_offset-12) >= 12 sectors
     * into the slice, and slice-relative extents + the slice's absolute
     * base anchor at the TRUE disk positions). The volume-relative
     * gates below run on the RAW (unshifted) fields - the boot region
     * they reason about lives at the TRUE base. The og's base>=0xc gate
     * maps to the slice's absolute base (a whole-device open at 0
     * cannot re-base below the device start). */
    const BootSectorMain* bak =
        reinterpret_cast<const BootSectorMain*>(region.data() + BACKUP_BOOT_SECTOR * bps);
    u16 base_shift = 0;
    if (!structural_gates(bak)) {
        auto fat_media_words_ok = [&](u64 sector_off) {
            u8 fat8[8];
            if (dev->read_at(sector_off * bps, fat8, 8) != OK)
                return false;
            return u32_from_bytes_le(fat8) == 0xFFFFFFF8u &&
                   u32_from_bytes_le(fat8 + 4) == 0xFFFFFFFFu;
        };
        const bool in_place = fat_media_words_ok(main_b->fat_offset);
        if (!in_place && dev->absolute_base() >= u64(12) * bps) {
            if (fat_media_words_ok(u64(main_b->fat_offset) - 12))
                base_shift = 12;
        }
    }

    /* volume-relative gates on the RAW fields (see the ladder note) */
    const u64 volume_length = main_b->volume_length; /* full u64 (audit row 10) */
    if (volume_length < 2048 || volume_length > (~u64(0)) / bps)
        return ERR_BAD_PARAMETER; /* overflow-safe bounds */
    /* the device bound is shift-aware: a re-based volume's true span
     * starts 12 sectors BEFORE the slice, and the engine never reads
     * below (fat_offset-12) - the og's whole-disk view covers the same
     * span without a bound at all */
    if (volume_length * bps > dev->size() + u64(base_shift) * bps)
        return ERR_BAD_PARAMETER;
    const u32 fat_offset_raw = main_b->fat_offset;
    const u32 fat_length = main_b->fat_length;
    const u32 heap_offset_raw = main_b->cluster_heap_offset;
    const u32 cluster_count = main_b->cluster_count;
    if (cluster_count == 0 || cluster_count > 0xFFFFFFF5)
        return ERR_BAD_PARAMETER;
    if (main_b->first_cluster_of_root < 2 || main_b->first_cluster_of_root >= cluster_count + 2)
        return ERR_BAD_PARAMETER;
    if (fat_offset_raw >= volume_length || heap_offset_raw >= volume_length)
        return ERR_BAD_PARAMETER;
    if (fat_offset_raw < BOOT_REGION_SECTORS)
        return ERR_BAD_PARAMETER;
    if (u64(heap_offset_raw) < u64(fat_offset_raw) + u64(fat_length))
        return ERR_BAD_PARAMETER; /* heap must start at/after the FAT area (sectors) */
    /* (d) heap fit (sectors, overflow-safe) */
    if (((u64(cluster_count) - 1) << main_b->sectors_per_cluster_shift) + u64(heap_offset_raw) >
        volume_length)
        return ERR_BAD_PARAMETER;

    ExfatParams p;
    p.bytes_per_sector_shift = main_b->bytes_per_sector_shift;
    p.bytes_per_sector = 1u << p.bytes_per_sector_shift;
    p.sectors_per_cluster_shift = main_b->sectors_per_cluster_shift;
    p.cluster_bytes = p.bytes_per_sector << p.sectors_per_cluster_shift;
    p.volume_length = volume_length;
    p.fat_offset = fat_offset_raw - base_shift;
    p.fat_length = fat_length;
    p.cluster_heap_offset = heap_offset_raw - base_shift;
    p.cluster_count = cluster_count;
    p.first_cluster_of_root = main_b->first_cluster_of_root;
    p.base_shift_sectors = base_shift;
    p.boot_main_ok = true;
    p.boot_backup_ok = oem_is_exfat(*bak) && bak->signature[0] == 0x55 && bak->signature[1] == 0xAA;

    /* boot-region checksum: the value is stored FILL-REPEATED in sector 11
     * (main side) and sector 23 (backup side); computed over sectors 0..10
     * with VolumeFlags (106..107) and PercentInUse (112) of the boot sector
     * SKIPPED entirely (spec 3.1.14.1; see boot_region_checksum). */
    {
        const u32 expect = u32_from_bytes_le(region.data() + 11 * bps);
        const u32 chk = boot_region_checksum(region.data(), bps);
        /* the fill-repeated pattern must be consistent in sector 11 */
        bool pattern_ok = true;
        for (u32 k = 0; k < bps / 4; ++k)
            if (u32_from_bytes_le(region.data() + 11 * bps + k * 4) != expect) {
                pattern_ok = false;
                break;
            }
        p.boot_checksum_ok = (chk == expect) && pattern_ok;
    }
    out = p;
    return OK;
}

} // namespace exfat
} // namespace cr
