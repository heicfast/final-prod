/*
 * cr_btrfs_engine.h - Btrfs scan engine.
 *
 * Corpus: BtrfsSearchDLL (CBtrfsQuickSearch/CSearchedBtrfsPartitionInfo;
 * superblock gate decompile-verified: magic "_BHRfS_M"@+0x40, newest
 * generation wins across mirror superblocks, sectorsize pow2@+0x90,
 * nodesize band 0x86..0x10000@+0x94, nodesize%sectorsize==0; sys chunk
 * array @+0x32B with size @+0xA0, entries [disk_key 17B][chunk 48+32*n]).
 *
 * Walks: mirror superblock select (max generation) -> sys chunk map ->
 * chunk tree walk (full logical->physical mapping) -> root-tree discovery
 * (self-validating: header.bytenr self-reference + chunk-tree-uuid match +
 * ROOT_ITEM(CHUNK_TREE) cross-check) -> per-subvolume FS trees: inode,
 * dir_item/dir_index, extent_data (regular + inline), subvolumes as roots.
 *
 * Deleted-file story (parity decision, AUDIT.md row 20 + corpus
 * SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md section 2): only the
 * NEWEST-generation tree is walked - the og does the same (max-generation
 * superblock select, FUN @ corpus line 10290; no old-generation walk in the
 * decompile) and lets RAW carving cover deleted content. We do the same
 * (contrast ZFS D-05's identical stance; PARITY-DELTAS D-06).
 *
 * The root/chunk-tree pointer fields vary across on-disk revisions; instead
 * of trusting one offset, candidates are validated against the node's own
 * self-referential bytenr and the fsid - a wrong candidate can never
 * validate. Fallback: bounded scan of the first metadata chunk.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BTRFS_ENGINE_H_
#define CR_BTRFS_ENGINE_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace btrfs {

using WalkProgressLike = bool (*)(u64 done, u64 total, void* user);

struct BtrfsScanStats {
    u64 files = 0;
    u64 dirs = 0;
    u64 subvolumes = 0;
    u64 chunks = 0;
    u64 inline_files = 0;       /* EXTENT_DATA inline: content in resident_data */
    u64 compressed_extents = 0; /* BTR-4: zlib/zstd/lzo extents seen */
    u64 lzo_extents = 0;        /* lzo extents: no codec - counted, unmapped */
    u64 nodes_read = 0;
    u64 damaged_nodes = 0;
};

/* One chunk mapping: [logical_start, +logical_len) -> device bytes. */
struct ChunkMap {
    u64 logical_start;
    u64 logical_len;
    u64 physical_start; /* slice-relative */
    u64 type;           /* DATA 1 / SYSTEM 2 / METADATA 4 / RAID bits */
};

/* Superblock (kernel fs/btrfs/ctree.h struct btrfs_super_block). */
struct SuperBlock {
    u64 generation = 0;
    u32 sectorsize = 0;
    u32 nodesize = 0;
    u64 total_bytes = 0;
    u8 fsid[16] = {0};
    /* BTR-2: chunk_tree_uuid is NOT a superblock field (SB+0x60 is
     * log_root) - it is adopted from the chunk-root node header. */
    std::vector<ChunkMap> sys_chunks;
    bool valid = false;
};

bool validate_superblock(const u8 sb[4096], SuperBlock& out);

u32 scan_btrfs(IStorageDevice* slice, CancelToken* cancel, WalkProgressLike progress,
               void* progress_user, tree::ResultTree& tree, tree::FullNode* root_active,
               tree::FullNode* root_deleted, tree::FullNode* root_lost, BtrfsScanStats& stats);

/* unix sec+nsec -> 100ns/1601 units: shared fold in cr_types.h
 * (unix_to_filetime_ns; W4 dedup, AUDIT-LINUX-FS DC-13). */

} // namespace btrfs
} // namespace cr

#endif // CR_BTRFS_ENGINE_H_
