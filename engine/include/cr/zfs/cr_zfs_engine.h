/*
 * cr_zfs_engine.h - ZFS scan engine (single-vdev pools).
 *
 * Corpus: ZfsSearchDLL (zfs_struct:: namespace: UberData, VdevLoc/VdevDisk,
 * objset, ZplNode/ZplNodeFile; uberblock walk decompile-verified: magic
 * 0x00BAB10C). S-034: the og's quick searcher iterates EVERY uberblock
 * slot in the label-0 ring (FUN_180006b90, 1 KiB stride, txg read at
 * slot+0x10) and walks each txg state's tree through the per-txg UberData
 * registry (FUN_180003150 "addBlock" -> FUN_180017310, consecutive-txg
 * memo, tree-open FUN_180016c50, unbounded from_txg=0 descent
 * FUN_18001ac60/FUN_18001d9a0 - the birth-txg gate `(from_txg < txg) ||
 * from_txg == 0` passes every node at the top level). This engine walks
 * every distinct valid ring txg the same way: newest state first (live
 * tree), then each older state (deleted view, cross-state dedup so the
 * newest state of each object wins and in-window deletions surface as
 * Deleted). Deliberate delta D-89: the og emits per-state duplicates at
 * the engine boundary (the quick searcher's visitor is a bare
 * push_back, FUN_180006b40); we dedup newest-wins - the recovered
 * object UNION is identical.
 *
 * Walks: vdev labels (4 per disk: 0, 256K, end-512K, end-256K) -> nvlist
 * config at label+16K (XDR/big-endian - vdev_label.c packs labels with
 * NV_ENCODE_XDR; pool name, ashift, vdev geometry) -> uberblock ring
 * (label+128K; slot size 1<<MAX(ashift,10), geometry auto-probed) -> MOS
 * objset -> object directory ZAP -> dataset directory chain (dsl_dir
 * bonus: head_dataset_obj@8, child_dir_zapobj@24) -> per-dataset
 * objset_phys -> master-node ZAP -> ZPL directories (micro/macro ZAP,
 * any nlevels) -> dnode bonus buffers (SA layout 1 in zfs_attr_table
 * order, and legacy znode_phys per zfs_sa.h; mode/size/parent/links/times).
 * DVAs resolve through the vdev offset from the label config (vdev id =
 * bits 32..55 of DVA word 0); file data walks the full indirect-block
 * ladder (any nlevels), embedded block pointers decode via the blkptr.c
 * payload algorithm, gang blocks assemble through the 512-byte gang
 * header (3 blkptrs + 32-byte self-checksum tail), and compressed
 * blocks (lzjb/lz4/zle/gzip/zstd) are tagged on the extent for
 * writer-time decode. Big-endian pools are detected and refused with
 * ERR_UNSUPPORTED. Damaged-block reads are counted, never silently
 * zero-filled.
 *
 * Divergence note: the og probes the config nvlist at a 0x1C000 window
 * that is internally inconsistent with its own limit; we follow the
 * OpenZFS-spec label layout (AUDIT.md sec 3.4).
 *
 * Single-device pools (the overwhelmingly common external-drive case);
 * multi-vdev pools are detected and reported as unsupported at full depth.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ZFS_ENGINE_H_
#define CR_ZFS_ENGINE_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace zfs {

using WalkProgressLike = bool (*)(u64 done, u64 total, void* user);

struct ZfsScanStats {
    u64 datasets = 0;
    u64 files = 0;
    u64 dirs = 0;
    u64 microzap_entries = 0;
    u64 macrozap_entries = 0;
    u64 damaged_blocks = 0;
    u64 unsupported_vdevs = 0;  /* gang/indirect/mirror/raid multi-vdev */
    u64 txg = 0;                /* the newest uberblock's txg */
    u64 embedded_files = 0;     /* files with embedded-bp content */
    u64 compressed_extents = 0; /* extents carrying a codec */
    u64 foreign_vdev_reads = 0; /* M-019: DVAs naming another vdev id,
                                 * served via the og's first-entry
                                 * fallback (the scanned disk) */
    u64 checksum_drops = 0;     /* M-019: blocks dropped after fletcher4
                                 * mismatch on every DVA copy */
    u64 ring_states = 0;        /* S-034: distinct valid txg states found
                                 * in the rings (pre-dedup walk set) */
    u64 deleted_objects = 0;    /* S-034: objects surfaced only from older
                                 * ring states (in-window deletions) */
};

/* Parsed label config (nvlist subset we need). */
struct LabelConfig {
    std::string pool_name;
    u64 ashift = 9; /* sector size = 1 << ashift */
    u64 asize = 0;  /* vdev allocatable size */
    u64 offset = 0; /* vdev data start offset */
    u64 guid = 0;
    std::string vdev_type;
    bool valid = false;
};

u32 scan_zfs(IStorageDevice* slice, CancelToken* cancel, WalkProgressLike progress,
             void* progress_user, tree::ResultTree& tree, tree::FullNode* root_active,
             tree::FullNode* root_deleted, tree::FullNode* root_lost, ZfsScanStats& stats);

/* unix sec -> 100ns/1601: shared fold in cr_types.h (W4 dedup, DC-13). */

} // namespace zfs
} // namespace cr

#endif // CR_ZFS_ENGINE_H_
