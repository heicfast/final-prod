/*
 * cr_zfs_engine.h - ZFS scan engine (single-vdev pools).
 *
 * Corpus: ZfsSearchDLL (zfs_struct:: namespace: UberData, VdevLoc/VdevDisk,
 * objset, ZplNode/ZplNodeFile; uberblock walk decompile-verified: magic
 * 0x00BAB10C, 128-slot x 1 KiB rings walked with txg newest-wins).
 *
 * Walks: vdev labels (4 per disk) -> nvlist config (pool name, ashift, vdev
 * geometry) -> uberblock ring (max txg, checksum-checked ub_rootbp) -> MOS
 * objset -> object directory ZAP -> dataset directory chain -> per-dataset
 * objset_phys -> master-node ZAP -> ZPL directories (micro/macro ZAP) ->
 * znode bonus buffers (mode/size/times). DVAs resolve through the vdev's
 * ashift/qshift math; gang blocks and indirect vdevs are detected and
 * reported rather than mis-read.
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
    u64 unsupported_vdevs = 0; /* gang/indirect/mirror/raid multi-vdev */
    u64 txg = 0;               /* the selected uberblock's txg */
};

struct ZfsScanFlags {
    bool keep_deleted = true;
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

u32 scan_zfs(IStorageDevice* slice, const ZfsScanFlags& flags, CancelToken* cancel,
             WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             ZfsScanStats& stats);

/* unix sec -> 100ns/1601 */
u64 unix_to_filetime(i64 sec);

} // namespace zfs
} // namespace cr

#endif // CR_ZFS_ENGINE_H_
