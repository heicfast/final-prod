/*
 * cr_mftwalk.h - the MFT record walker: records -> FileProperties -> tree.
 *
 * Corpus mapping (algorithms-v2 section 1 - the deleted-record batch loop +
 * deep-dive section 5):
 *   - magic 'FILE', mandatory fixup, header sanity, not-BAAD
 *   - filters: no $FILE_NAME -> damaged; name[0]=='$' -> meta skip;
 *     dos_flags & 4 (SYSTEM) -> skip (corpus "flags & 4")
 *   - namespace preference POSIX > Win32+Dos > Win32 > Dos
 *   - in-use bit -> active vs deleted; deleted non-dir with size >= 0x3FF
 *     gets the viability walk (corpus gate), graded per fix F5
 *   - hard links: node per parent ref, shared props (corpus refcount model)
 *   - named $DATA -> ADS child nodes
 *   - orphan parents -> "Lost Files" root (post-format salvage, R2.5)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_MFTWALK_H_
#define CR_MFTWALK_H_

#include "cr_bootstrap.h"
#include "cr_bitmap.h"
#include "../tree/cr_node.h"
#include <memory>

namespace cr {
namespace ntfs {

/* props-internal flags (corpus mapping: bit4 = in-use; we add bit for dir). */
constexpr u16 PFLAG_IN_USE = 0x0010;
constexpr u16 PFLAG_IS_DIR = 0x0002; /* mirrors record flag 0x02 */

struct WalkerFlags {
    bool skip_system_files = true;  /* corpus: dos_flags & 4 -> skip */
    bool keep_overwritten = true;   /* fix F5: grade instead of destroy */
    bool emit_ads = true;           /* named $DATA -> child nodes */
    u64 viability_min_size = 0x3FF; /* corpus gate (files < 1 KiB fast-path) */
};

struct WalkStats {
    u64 examined = 0;
    u64 emitted = 0;
    u64 meta_skipped = 0;
    u64 system_skipped = 0;
    u64 damaged = 0;
    u64 deleted = 0;
    u64 active = 0;
    u64 orphans = 0;
    u64 hardlinks = 0;
    u64 ads = 0;
};

/* Progress callback: (done, total) with 0..10000 scale. Return false to
 * request cancel (same as CancelToken). */
using WalkProgress = bool (*)(u64 done, u64 total, void* user);

/*
 * walk_mft - the single-pass record walk.
 *   vol: opened volume
 *   bitmap: may be null (viability grading degrades to Unknown)
 *   tree/roots: result graph; requires root_active, root_deleted, root_lost
 * Returns OK / ERR_CANCELLED / first hard error.
 */
u32 walk_mft(NtfsVolume* vol, VolumeBitmap* bitmap, const WalkerFlags& flags, CancelToken* cancel,
             WalkProgress progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             WalkStats& stats);

/*
 * scan_mft_remnants - post-format / severe-damage pass (R2.5): stride the
 * whole slice at record_size, validate 'FILE' records outside the live MFT
 * stream. Emits metadata-only nodes under root_lost. Used by the complete
 * engine and by lost-partition candidates.
 */
u32 scan_mft_remnants(IStorageDevice* slice, u32 sector_bytes, u32 cluster_bytes, u64 record_size,
                      CancelToken* cancel, WalkProgress progress, void* progress_user,
                      tree::ResultTree& tree, tree::FullNode* root_lost, WalkStats& stats);

} // namespace ntfs
} // namespace cr

#endif // CR_MFTWALK_H_
