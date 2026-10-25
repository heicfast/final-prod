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
#include "../storage/cr_freemap.h"
#include "../tree/cr_node.h"
#include "cr_exfat_wire.h"
#include <memory>
#include <vector>

namespace cr {
namespace exfat {

u32 checksum32(const u8* p, u32 n, u32 seed); /* ROL31 + add */
u16 checksum16(const u8* p, u32 n, u16 seed);
/* Boot-region checksum over sectors 0..10 (skip bytes 106/107/112 of
 * sector 0 entirely - spec 3.1.14.1). Shared with the repair engine. */
u32 boot_region_checksum(const u8* region, u32 sector_bytes);

struct ExfatParams {
    u32 bytes_per_sector = 512; /* 1 << shift, shift in 9..12 */
    u8 bytes_per_sector_shift = 9;
    u8 sectors_per_cluster_shift = 0;
    u32 cluster_bytes = 0; /* bytes_per_sector << spc_shift */
    u64 volume_length = 0; /* sectors (spec u64 @+0x48; the old u32 cast capped at 2 TiB@512/8
                              TiB@4Kn - audit row 10) */
    u32 fat_offset = 0;
    u32 fat_length = 0; /* 0 legal? spec: >=1 typically; 0 = no FAT at all (contiguous-only) */
    u32 cluster_heap_offset = 0;
    u32 cluster_count = 0;
    u32 first_cluster_of_root = 0;
    bool boot_main_ok = false;
    bool boot_backup_ok = false;
    bool boot_checksum_ok = false;
};

/* Validate main + backup boot regions (checksum per rules above). */
u32 validate_boot_region(IStorageDevice* dev, ExfatParams& out);

/* Entry-set checksum over raw dentry bytes (file+stream+names), skipping
 * the 2 checksum bytes at file_dentry+8. */
u16 entry_set_checksum(const u8* entries, u32 entry_count);

/* ---- scan ---- */
struct ExfatScanStats {
    u64 files = 0;
    u64 dirs = 0;
    u64 deleted_entries = 0;
    u64 sets_damaged = 0; /* checksum mismatch (kept, marked damaged) */
    u64 no_fat_chains = 0;
    u64 chains_broken = 0;
    u64 assumed_contiguous = 0; /* EXFAT-2: FREE-entry continuation salvage */
    u64 fat_window_refills = 0; /* S-013: windowed-FAT cache refills (big-FAT volumes) */
    /* N-067 (diff-to-fix): the og's volume analyzer validates the upcase
     * table as part of volume integrity (boot + upcase checks,
     * CExFatVolumeAnalyer - fat-exfat-engine-internals.md:63-64). Grade,
     * not reject: a corrupt table flags the volume's integrity signal
     * but never stops the scan (the og's corrupt-volume posture, the
     * EXFAT-2 salvage lineage). */
    u64 upcase_validated =
        0; /* in-use upcase dentries whose structure passed the og's shape gates */
    u64 upcase_mismatches = 0; /* checksum/length/read integrity failures (0 = clean or absent) */
};

struct ExfatScanFlags {
    /* N-003 (diff-to-fix): the og exFAT engine has NO entry-set checksum
     * loop anywhere (verified-absence search over 1,299 decompiled
     * functions, B-exfat 3.4) - it grades live sets purely structurally.
     * DEFAULT FALSE = og parity: a live file with a torn SetChecksum
     * grades normally instead of PartialRisk. TRUE = the honest-recovery
     * strictness (keep, never drop: the file is still emitted, only
     * downgraded) - a reviewed deliberate divergence while enabled
     * (registered in docs/PARITY-DELTAS.md). */
    bool validate_entry_sets = false;
};

/* S-009 (free overlay): free_out, when non-null, receives the
 * cluster-granular free map from the volume allocation bitmap (bit
 * set = cluster allocated per the exFAT spec; the map inverts to
 * free). Optional so existing callers are untouched. */
u32 scan_exfat(IStorageDevice* slice, const ExfatScanFlags& flags, CancelToken* cancel,
               bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
               tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
               ExfatScanStats& stats, storage::FreeMap* free_out = nullptr);

} // namespace exfat
} // namespace cr

#endif // CR_EXFAT_ENGINE_H_
