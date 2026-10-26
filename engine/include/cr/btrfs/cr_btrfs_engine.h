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
 * M-020 chunk profiles (BtrfsSearchDLL_decompiled.c :2498-2543, first-hand):
 * DUP(&0x20)/RAID1(&0x10)/SINGLE map through the og's rotating mirror
 * index (per-chunk u16 state, index = (index+1) % num_stripes per access,
 * :2534-2540); RAID10 geometry gate at parse (:2162-2168: sub_stripes==0
 * or num_stripes<sub_stripes rejects the chunk); RAID0/RAID10 stripe math
 * is the core's REGISTERED SUPERSET (the og's read path fails both classes
 * - class 8/0x40 goto LAB_1800043fe, verified :2521-2583); classes &0x80/
 * &0x100 are refused exactly like the og. Default-subvolume pick
 * (FUN_1800061f0 :3975-4060): the root tree's DIR_ITEM
 * (ROOT_TREE_DIR_OBJECTID 6, type 0x54, name-hash 0x8dbfc2d2) named
 * "default" (len 7) whose located key is a ROOT_ITEM (0x84) selects the
 * default; fallback root 5; the default's children render at the volume
 * root (ReFS convention), other subvolumes stay named containers.
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

struct BtrfsCacheConfig; /* N-072: defined in cr_btrfs_cache.h */

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
    /* S-028: xattr surfaces */
    u64 xattr_records = 0;              /* user.* xattrs emitted as records */
    u64 compression_property_files = 0; /* files carrying btrfs.compression
                                         * (the og's +0x88 codec tag; the
                                         * per-extent truth stays on the
                                         * extents - I-014 B4) */
    /* M-020: extents the profile machinery could not map (unsupported
     * profile classes, compressed extents spanning a stripe boundary on
     * striped profiles, extents crossing past the chunk end) - counted,
     * never silently emitted with wrong bytes. */
    u64 unmappable_extents = 0;
    u64 mirror_rotations = 0; /* 2-copy accesses served by the non-first stripe */
    /* N-072: the re-scan metadata cache (OFF by default - cr_btrfs_cache.h) */
    u64 cache_hits = 0;   /* chunk map + roots adopted from a valid blob */
    u64 cache_writes = 0; /* blob persisted for the next scan */
};

/* One chunk stripe (kernel btrfs_stripe: devid u64, offset u64, uuid[16]). */
struct ChunkStripe {
    u64 devid = 0;
    u64 offset = 0; /* slice-relative physical offset */
};

/* One chunk mapping: [logical_start, +logical_len) -> device bytes.
 * M-020: the full stripe array is retained; the profile-aware mapping
 * lives in the engine (rotating mirror for 2-copy, stripe math for
 * RAID0/RAID10 - see the header comment). physical_start remains the
 * stripe-0 offset for diagnostics. */
struct ChunkMap {
    u64 logical_start = 0;
    u64 logical_len = 0;
    u64 physical_start = 0; /* stripe-0 offset (diagnostics) */
    u64 type = 0;           /* DATA 1 / SYSTEM 2 / METADATA 4 / RAID bits */
    u64 stripe_len = 0;     /* kernel chunk stripe_len (RAID0/RAID10 math) */
    u16 num_stripes = 0;
    u16 sub_stripes = 0;    /* RAID10 mirror-set size */
    u16 mirror_index = 0;   /* og rotating-mirror state (:2534-2537) */
    std::vector<ChunkStripe> stripes;
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
               tree::FullNode* root_deleted, tree::FullNode* root_lost, BtrfsScanStats& stats,
               /* N-072: the optional re-scan metadata cache (null/empty dir =
                * disabled, the default). See cr_btrfs_cache.h. */
               const BtrfsCacheConfig* cache = nullptr);

/* unix sec+nsec -> 100ns/1601 units: shared fold in cr_types.h
 * (unix_to_filetime_ns; W4 dedup, AUDIT-LINUX-FS DC-13). */

} // namespace btrfs
} // namespace cr

#endif // CR_BTRFS_ENGINE_H_
