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
        out.raw = static_cast<u16>(((out.raw & 1) ? 0x8000u : 0u) + (out.raw >> 1) +
                                   entries[i]);
        if (deleted_set) {
            u16 sub = entries[i];
            if (i == 0)
                sub = 0x85; /* File | InUse */
            else if (i == 0x20)
                sub = 0xC0; /* Stream | InUse */
            else if ((i & 0x1F) == 0)
                sub = 0xC1; /* File Name | InUse */
            out.restored = static_cast<u16>(((out.restored & 1) ? 0x8000u : 0u) +
                                            (out.restored >> 1) + sub);
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
    if (!oem_is_exfat(*main_b))
        return ERR_BAD_MAGIC;
    if (main_b->signature[0] != 0x55 || main_b->signature[1] != 0xAA)
        return ERR_BAD_MAGIC;
    if (main_b->bytes_per_sector_shift < 9 || main_b->bytes_per_sector_shift > 12)
        return ERR_BAD_PARAMETER;
    if ((1u << main_b->bytes_per_sector_shift) != bps)
        return ERR_BAD_PARAMETER;
    if (main_b->sectors_per_cluster_shift > 25 - main_b->bytes_per_sector_shift)
        return ERR_BAD_PARAMETER;
    /* EXFAT-4 (AUDIT-FAT): og boot gates we lacked (FUN_180021e80,
     * exFATSearchDLL_decompiled.c:28180-28208):
     *   (a) MustBeZero sweep +0x0B..+0x3F (the 53-byte reserved region
     *       after JumpBoot/OEM - 28183-28196);
     *   (b) fat_count in [1,2] (28200);
     *   (c) fat_length != 0 - og rejects "no-FAT" boot sectors (28200);
     *   (d) cluster-heap fit: the last cluster's last sector must be
     *       inside the volume: (cluster_count-1)<<spc_shift + heap_offset
     *       <= volume_length, in sectors (28206-28208). */
    for (int i = 0; i < 53; ++i)
        if (main_b->must_be_zero[i] != 0)
            return ERR_BAD_PARAMETER;
    if (main_b->fat_count < 1 || main_b->fat_count > 2)
        return ERR_BAD_PARAMETER;
    if (main_b->fat_length == 0)
        return ERR_BAD_PARAMETER;

    ExfatParams p;
    p.bytes_per_sector_shift = main_b->bytes_per_sector_shift;
    p.bytes_per_sector = 1u << p.bytes_per_sector_shift;
    p.sectors_per_cluster_shift = main_b->sectors_per_cluster_shift;
    p.cluster_bytes = p.bytes_per_sector << p.sectors_per_cluster_shift;
    p.volume_length = main_b->volume_length; /* full u64 (audit row 10) */
    if (p.volume_length < 2048 || p.volume_length > (~u64(0)) / bps ||
        p.volume_length * bps > dev->size())
        return ERR_BAD_PARAMETER; /* overflow-safe bounds */
    p.fat_offset = main_b->fat_offset;
    p.fat_length = main_b->fat_length;
    p.cluster_heap_offset = main_b->cluster_heap_offset;
    p.cluster_count = main_b->cluster_count;
    p.first_cluster_of_root = main_b->first_cluster_of_root;
    if (p.cluster_count == 0 || p.cluster_count > 0xFFFFFFF5)
        return ERR_BAD_PARAMETER;
    if (p.first_cluster_of_root < 2 || p.first_cluster_of_root >= p.cluster_count + 2)
        return ERR_BAD_PARAMETER;
    if (p.fat_offset >= p.volume_length || p.cluster_heap_offset >= p.volume_length)
        return ERR_BAD_PARAMETER;
    if (p.fat_offset < BOOT_REGION_SECTORS)
        return ERR_BAD_PARAMETER;
    if (u64(p.cluster_heap_offset) < u64(p.fat_offset) + u64(p.fat_length))
        return ERR_BAD_PARAMETER; /* heap must start at/after the FAT area (sectors) */
    /* (d) heap fit (sectors, overflow-safe) */
    if (((u64(p.cluster_count) - 1) << p.sectors_per_cluster_shift) + u64(p.cluster_heap_offset) >
        p.volume_length)
        return ERR_BAD_PARAMETER;
    p.boot_main_ok = true;

    /* backup boot sector */
    const BootSectorMain* bak =
        reinterpret_cast<const BootSectorMain*>(region.data() + BACKUP_BOOT_SECTOR * bps);
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
