/* cr_exfat_dentry.cpp - checksums + boot region validation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/exfat/cr_exfat_engine.h"
#include <cstring>

namespace cr {
namespace exfat {

u32 checksum32(const u8 *p, u32 n, u32 seed) {
    u32 sum = seed;
    for (u32 i = 0; i < n; ++i) {
        sum = ((sum & 1) ? 0x80000000u : 0u) + (sum >> 1) + p[i];
    }
    return sum;
}

u16 checksum16(const u8 *p, u32 n, u16 seed) {
    u16 sum = seed;
    for (u32 i = 0; i < n; ++i) {
        sum = static_cast<u16>(((sum & 1) ? 0x8000u : 0u) + (sum >> 1) + p[i]);
    }
    return sum;
}

u16 entry_set_checksum(const u8 *entries, u32 entry_count) {
    /* whole set byte-by-byte, excluding ONLY the 2 SetCheckSum bytes (file+2) */
    u16 sum = 0;
    for (u32 e = 0; e < entry_count; ++e) {
        const u8 *d = entries + e * 32;
        for (int i = 0; i < 32; ++i) {
            if (e == 0 && i >= 2 && i < 4) continue; /* SetCheckSum field */
            sum = static_cast<u16>(((sum & 1) ? 0x8000u : 0u) + (sum >> 1) + d[i]);
        }
    }
    return sum;
}

u32 validate_boot_region(IStorageDevice *dev, ExfatParams &out) {
    if (dev == nullptr || dev->geometry().logical_sector > 4096) return ERR_BAD_PARAMETER;
    const u32 bps = dev->geometry().logical_sector;
    if (u64(BOOT_REGION_SECTORS) * bps > dev->size()) return ERR_RANGE_OUT_OF_BOUNDS;

    std::vector<u8> region(BOOT_REGION_SECTORS * bps);
    if (dev->read_at(0, region.data(), u32(region.size())) != OK) return ERR_READ_FAILED;

    /* main boot sector checks */
    const BootSectorMain *main_b = reinterpret_cast<const BootSectorMain *>(region.data());
    if (!oem_is_exfat(*main_b)) return ERR_BAD_MAGIC;
    if (main_b->signature[0] != 0x55 || main_b->signature[1] != 0xAA) return ERR_BAD_MAGIC;
    if (main_b->bytes_per_sector_shift < 9 || main_b->bytes_per_sector_shift > 12)
        return ERR_BAD_PARAMETER;
    if ((1u << main_b->bytes_per_sector_shift) != bps) return ERR_BAD_PARAMETER;
    if (main_b->sectors_per_cluster_shift > 25 - main_b->bytes_per_sector_shift)
        return ERR_BAD_PARAMETER;

    ExfatParams p;
    p.bytes_per_sector_shift = main_b->bytes_per_sector_shift;
    p.bytes_per_sector = 1u << p.bytes_per_sector_shift;
    p.sectors_per_cluster_shift = main_b->sectors_per_cluster_shift;
    p.cluster_bytes = p.bytes_per_sector << p.sectors_per_cluster_shift;
    p.volume_length = u32(main_b->volume_length); /* u64 field; u32 cast safe: < 2^32 sectors */
    if (p.volume_length < 2048 || u64(p.volume_length) * bps > dev->size())
        return ERR_BAD_PARAMETER;
    p.fat_offset = main_b->fat_offset;
    p.fat_length = main_b->fat_length;
    p.cluster_heap_offset = main_b->cluster_heap_offset;
    p.cluster_count = main_b->cluster_count;
    p.first_cluster_of_root = main_b->first_cluster_of_root;
    p.volume_flags = main_b->volume_flags;
    if (p.cluster_count == 0 || p.cluster_count > 0xFFFFFFF5) return ERR_BAD_PARAMETER;
    if (p.first_cluster_of_root < 2 || p.first_cluster_of_root >= p.cluster_count + 2)
        return ERR_BAD_PARAMETER;
    if (p.fat_offset >= p.volume_length || p.cluster_heap_offset >= p.volume_length)
        return ERR_BAD_PARAMETER;
    if (p.fat_offset < BOOT_REGION_SECTORS) return ERR_BAD_PARAMETER;
    if (p.cluster_heap_offset < p.fat_offset + p.fat_length)
        return ERR_BAD_PARAMETER; /* heap must start at/after the FAT area (sectors) */
    p.boot_main_ok = true;

    /* backup boot sector */
    const BootSectorMain *bak =
        reinterpret_cast<const BootSectorMain *>(region.data() + BACKUP_BOOT_SECTOR * bps);
    p.boot_backup_ok = oem_is_exfat(*bak) && bak->signature[0] == 0x55 &&
                        bak->signature[1] == 0xAA;

    /* boot-region checksum (elm-chan/spec): the value is stored FILL-
     * REPEATED in sector 11 (main side) and sector 23 (backup side); computed
     * over sectors 0..10 (resp. 12..22) as a byte array, with VolumeFlags
     * (106..107) and PercentInUse (112) of the BOOT SECTOR treated as zero. */
    {
        const u32 expect = u32_from_bytes_le(region.data() + 11 * bps);
        u32 chk = 0;
        for (u32 s = 0; s <= 10; ++s) {
            const u8 *sec = region.data() + u64(s) * bps;
            for (u32 i = 0; i < bps; ++i) {
                u8 v = sec[i];
                if (s == 0) {
                    if (i >= 106 && i < 108) v = 0; /* VolumeFlags */
                    if (i == 112) v = 0;           /* PercentInUse */
                }
                chk = ((chk & 1) ? 0x80000000u : 0u) + (chk >> 1) + v;
            }
        }
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
