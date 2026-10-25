/*
 * cr_hfsplus_engine.h - HFS+/HFSX scan engine.
 *
 * Corpus: HFSPlusSearchDLL (hfspluslib port: volume/btree/btnode/catalog/
 * extents/attribute/journal/rawfile; volume gate decompile-verified:
 * "H+"(0x482B)/"HX"(0x4858) signatures, blockSize % 512 == 0, version < 17,
 * fork block counts clamped to totalBlocks).
 *
 * Walks: volume header @1024 (BE) -> catalog B-tree (CNID 4) leaf chain ->
 * file/folder records with cnid-parent hierarchy, data + resource forks ->
 * extents-overflow B-tree (CNID 3) for fragmented forks -> deleted pass:
 * freed catalog nodes scanned for stale file records (orphaned cnids,
 * graded against the allocation bitmap), since HFS+ overwrites catalog
 * records on delete.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_HFSPLUS_ENGINE_H_
#define CR_HFSPLUS_ENGINE_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace hfsplus {

using WalkProgressLike = bool (*)(u64 done, u64 total, void* user);

struct HfsScanStats {
    u64 files = 0;
    u64 dirs = 0;
    u64 leaf_records = 0;
    u64 orphans = 0;          /* stale records in freed catalog nodes */
    u64 overflow_extents = 0; /* extents resolved from the overflow tree */
    u64 bad_nodes = 0;
    /* S-002 (diff-to-fix) */
    u64 attr_records = 0;     /* inline attribute records collected */
    u64 attr_unsupported = 0; /* non-0x10 record types skipped (og-parity) */
    u64 decmpfs_files = 0;    /* files whose content resolved from decmpfs */
    u64 named_forks = 0;      /* attribute children emitted */
    /* N-028/N-038 (diff-to-fix, minors session) */
    u64 sink_filtered = 0;    /* dotfiles + 'HFS+ Private Data' (og sink) */
    u64 prf_folderized = 0;   /* 'prdf' file records folderized (og gate) */
    u64 rsrc_marked_files = 0;/* ownerFlags 0x20: rsrc-fork content path */
};

struct HfsScanFlags {
    bool keep_deleted = true;
    bool scan_freed_nodes = true; /* orphan sweep in free catalog nodes */
    bool resource_forks = true;   /* emit ..namedfork/rsrc children */
    bool resolve_decmpfs = true;  /* S-002: com.apple.decmpfs content via
                                   * codec extents (both storage forms) */
    bool named_forks = true;      /* S-002: attributes as ..namedfork/<name> */
};

/* Volume header (BE fields decoded). */
struct VolumeHeader {
    u16 signature = 0; /* 0x482B H+ / 0x4858 HX */
    u16 version = 0;
    u32 block_size = 0;
    u32 total_blocks = 0;
    u32 free_blocks = 0;
    u32 file_count = 0;
    u32 folder_count = 0;
    u32 next_catalog_id = 0;
    u32 journal_info_block = 0;
    bool valid = false;
};

bool validate_volume_header(const u8 vh[512], VolumeHeader& out);

/* One fork extent resolved to slice-relative bytes. */
struct ForkExtent {
    u64 offset;
    u64 length;
};

u32 scan_hfsplus(IStorageDevice* slice, const HfsScanFlags& flags, CancelToken* cancel,
                 WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
                 tree::FullNode* root_active, tree::FullNode* root_deleted,
                 tree::FullNode* root_lost, HfsScanStats& stats);

/* Mac epoch (1904) seconds -> 100ns/1601 filetime units. */
u64 mac_to_filetime(u32 mac_sec);

} // namespace hfsplus
} // namespace cr

#endif // CR_HFSPLUS_ENGINE_H_
