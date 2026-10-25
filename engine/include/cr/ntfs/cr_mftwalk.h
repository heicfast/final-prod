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

/* props-internal flags. CORRECTED (AUDIT-NTFS NTFS-5): the og props word
 * is FILE_ATTRIBUTE-style - 0x10 = DIRECTORY (FUN_180006db0 :5327-5331
 * via $FILE_NAME flags & 0x10000000), 0x20 = deleted, 0x04 = system. The
 * old "bit4 = in-use" comment misread the corpus; our DeletedState is
 * carried on the node, not this word. */
constexpr u16 PFLAG_IN_USE = 0x0010;
constexpr u16 PFLAG_IS_DIR = 0x0002; /* mirrors record flag 0x02 */

struct WalkerFlags {
    bool skip_system_files = true; /* corpus: dos_flags & 4 -> skip */
    bool emit_ads = true;          /* named $DATA -> child nodes */
    bool harvest_usnjrnl = true;   /* $Extend/$UsnJrnl:$J deleted-name harvest */
    /* S-001 landed the og's destroy rule in the WRONG pass (S-012). The og's
     * main MFT walk (pass 1, FUN_180017390) emits EVERY parsed record
     * unconditionally - deleted included, no viability grading, no destroy
     * (emit at :18630; the og's viability/destroy machinery lives in the
     * pass-2 GUID-dump record parser FUN_18001acc0 :21208-21264). The core's
     * in-walk grading stays (informational superset, D15 documented), but
     * destroy-in-pass-1 removed records the og shows - membership must be
     * pass-1-parity first. Default OFF = og membership. The flag keeps the
     * opt-in destroy for honest-recovery scans that want it; pass 2 applies
     * the og's real destroy rules regardless of this flag. */
    bool destroy_fully_overwritten = false;
    /* S-012 pass 2: after the main walk, collect LIVE files named
     * "{...}{3808876b-c176-4e48-b7ae-04046e6cc752}" (0x4C chars, collector
     * NTFSSearchDLL :18584-18625) and parse their $DATA contents as stale
     * MFT-record dumps (FUN_18001a980 chunk walk + FUN_18001acc0 record
     * gates). The og's deep-scan second source for records whose live MFT
     * slots were reused after deletion. */
    bool guid_dump_pass = true;
    /* viability gate boundary (NTFS-1): og is STRICT - `0x3ff < size`
     * (Dec 21208): files of exactly 0x3FF bytes take the fast-path with
     * no bitmap check; larger files get the $Bitmap grading. */
    u64 viability_min_size = 0x3FF;
};

/* USN reasons (winioctl.h USN_REASON_*): the journal records we mine for
 * deleted-name evidence. FILE_CREATE is test-fixture material only - the
 * engine filters on DELETE alone (AUDIT-NTFS §4 row 5); its value lives in
 * the USN fixtures (tests/unit/test_ntfs_volume.cpp). */
constexpr u32 USN_REASON_FILE_DELETE = 0x00020000;

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
    u64 usn_records = 0;           /* journal records parsed */
    u64 usn_names = 0;             /* deleted names the MFT pass missed */
    u64 usn_damaged_runs = 0;      /* $J runs that failed to read (zeros kept) */
    u64 recycle_info = 0;          /* $Recycle.Bin $I metadata files parsed */
    u64 recycle_remapped = 0;      /* $R records mapped to their original name */
    u64 compressed_blocks = 0;     /* GAP-P1-1: expanded NTFS compression blocks */
    u64 compressed_failed = 0;     /* files whose envelope failed (zero-filled) */
    u64 destroyed_overwritten = 0; /* S-001: deleted files dropped by the og's
                                     destroy rule (all probe clusters
                                     re-allocated; never emitted). S-012: the
                                     rule belongs to pass 2 - pass 1 keeps
                                     every deleted record (og membership). */
    u64 guid_dump_files = 0;       /* S-012: live GUID-template files collected */
    u64 guid_dump_records = 0;     /* S-012: dump records parsed (post-gates) */
    u64 guid_dump_destroyed = 0;   /* S-012: dump records destroyed (og rules:
                                     size <= 0x3ff or all run-STARTs claimed) */
    u64 guid_dump_emitted = 0;     /* S-012: dump records emitted into the tree */
};

/* Progress callback: (done, total) in walk units (records for the MFT
 * walk). The orchestrator's telemetry bridge derives the permille
 * (done*1000/total, monotonic per stage) — there is no 0..10000 scale
 * (PARITY-AUDIT F-B-13: that claim was stale). Return false to request
 * cancel (same as CancelToken). */
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
 * stream. GAP-S6-4: records carrying an unnamed $DATA attribute are emitted
 * WITH their content (resident stream or runlist extents, PartialRisk);
 * records without recoverable content (or with compressed streams - the
 * honest refusal, never raw LZNT1 as content) stay metadata-only under
 * root_lost. Used by the complete engine and by lost-partition candidates.
 * Block-buffered (4 MiB reads, record-size overlap); threads > 1 fans
 * chunk ranges over a worker pool.
 */
u32 scan_mft_remnants(IStorageDevice* slice, u32 sector_bytes, u32 cluster_bytes, u64 record_size,
                      u64 total_clusters, CancelToken* cancel, WalkProgress progress,
                      void* progress_user, tree::ResultTree& tree, tree::FullNode* root_lost,
                      WalkStats& stats, u32 threads = 1);

} // namespace ntfs
} // namespace cr

#endif // CR_MFTWALK_H_
