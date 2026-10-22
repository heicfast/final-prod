/*
 * cr_exfat_engine.h - exFAT engine (R5.2).
 *
 * Verified algorithms:
 *   - Checksum32: chk = ROL31(chk) + byte (elm-chan/spec v1.00)
 *   - boot region: 24 sectors; checksum value at +0x1FC of every sector;
 *     computing: zero bytes 508..511 in all sectors; zero VolumeFlags
 *     (106..107) and PercentInUse (112) in the main AND backup boot sectors
 *   - entry-set checksum: 16-bit, chk = ROL15(chk) + byte over the whole set
 *     (file dentry + stream + name fragments), excluding the 2-byte
 *     SetChecksum field in the file dentry
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_EXFAT_ENGINE_H_
#define CR_EXFAT_ENGINE_H_

#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include "cr_exfat_wire.h"
#include <memory>
#include <vector>

namespace cr {
namespace exfat {

u32 checksum32(const u8 *p, u32 n, u32 seed); /* ROL31 + add */
u16 checksum16(const u8 *p, u32 n, u16 seed);

struct ExfatParams {
    u32 bytes_per_sector = 512;     /* 1 << shift, shift in 9..12 */
    u8 bytes_per_sector_shift = 9;
    u8 sectors_per_cluster_shift = 0;
    u32 cluster_bytes = 0;          /* bytes_per_sector << spc_shift */
    u32 volume_length = 0;          /* sectors */
    u32 fat_offset = 0;
    u32 fat_length = 0;             /* 0 legal? spec: >=1 typically; 0 = no FAT at all (contiguous-only) */
    u32 cluster_heap_offset = 0;
    u32 cluster_count = 0;
    u32 first_cluster_of_root = 0;
    u16 volume_flags = 0;
    bool boot_main_ok = false;
    bool boot_backup_ok = false;
    bool boot_checksum_ok = false;
};

/* Validate main + backup boot regions (checksum per rules above). */
u32 validate_boot_region(IStorageDevice *dev, ExfatParams &out);

/* Entry-set checksum over raw dentry bytes (file+stream+names), skipping
 * the 2 checksum bytes at file_dentry+8. */
u16 entry_set_checksum(const u8 *entries, u32 entry_count);

/* ---- scan ---- */
struct ExfatScanStats {
    u64 files = 0;
    u64 dirs = 0;
    u64 deleted_entries = 0;
    u64 sets_damaged = 0;   /* checksum mismatch (kept, marked damaged) */
    u64 no_fat_chains = 0;
    u64 chains_broken = 0;
};

struct ExfatScanFlags {
    bool validate_entry_sets = true;
    bool keep_deleted = true;
};

u32 scan_exfat(IStorageDevice *slice, const ExfatScanFlags &flags, CancelToken *cancel,
               bool (*progress)(u64, u64, void *), void *progress_user,
               tree::ResultTree &tree, tree::FullNode *root_active,
               tree::FullNode *root_deleted, tree::FullNode *root_lost,
               ExfatScanStats &stats);

} // namespace exfat
} // namespace cr

#endif // CR_EXFAT_ENGINE_H_
