/*
 * cr_zfs_engine.h - ZFS scan engine (single-vdev pools).
 *
 * Corpus: ZfsSearchDLL (zfs_struct:: namespace: UberData, VdevLoc/VdevDisk,
 * objset, ZplNode/ZplNodeFile; uberblock walk decompile-verified: magic
 * 0x00BAB10C, rings walked with txg newest-wins).
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
    u64 unsupported_vdevs = 0;  /* D-09's registered posture (N-148 re-pinned):
                                 * mirror/raidz multi-vdev: REPORT-ONLY; gang
                                 * blocks ARE assembled (cr_zfs_engine.cpp:594-621);
                                 * indirect vdevs unsupported */
    u64 txg = 0;                /* the selected uberblock's txg */
    u64 embedded_files = 0;     /* files with embedded-bp content */
    u64 compressed_extents = 0; /* extents carrying a codec */
    u64 foreign_vdev_reads = 0; /* M-019: DVAs naming another vdev id,
                                 * served via the og's first-entry
                                 * fallback (the scanned disk) */
    u64 checksum_drops = 0;     /* M-019: blocks dropped after fletcher4
                                 * mismatch on every DVA copy */
    u64 ring_states = 0;        /* S-034: distinct txg states opened from
                                 * the uberblock rings (live + recall) */
    u64 label_retries = 0;      /* N-072: delay-execution label re-sweeps
                                 * (live handles only; the og's polling
                                 * import, remaining-engines artifact :86) */
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
