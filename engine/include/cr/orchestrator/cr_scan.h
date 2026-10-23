/*
 * cr_scan.h - the scan orchestrator (R6.1, R6.2, R6.4).
 *
 * Stages: quick (structured metadata walk) -> deep (deleted/complete
 * behaviors + lost-partition candidates) -> carve (grammar carving with
 * dedup against structured results). Weighted monotonic progress,
 * cancellation, resumable sessions (CRSF).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SCAN_H_
#define CR_SCAN_H_

#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include "../session/cr_crsf.h"
#include "../session/cr_records.h"
#include "cr/lost/cr_lostpart.h"
#include <memory>
#include <vector>

namespace cr {
namespace orchestrator {

enum class FsFamily : u8 {
    Unknown = 0,
    Ntfs,
    Fat,
    Exfat,
    Ext4,
    CarvedOnly,
    /* Values 6..9 appended after v0.1.0: the serialized fs_kind numbers
     * 0..5 keep their meaning, so old sessions stay readable. */
    Hfsplus,
    Btrfs,
    Zfs,
    Refs
};

struct StageWeights {
    u32 quick = 40; /* percent-of-total per stage (monotonic weighted) */
    u32 deep = 35;
    u32 carve = 25;
};

struct ScanConfig {
    bool run_quick = true;
    bool run_deep = true;
    bool run_carve = true;
    bool whole_disk = false;              /* enables lost-partition discovery */
    FsFamily fs_hint = FsFamily::Unknown; /* auto-probed when Unknown */
    StageWeights weights;
    u32 threads = 0; /* 0 = auto (hw-1, clamp 1..8); 1 = single-threaded */
};

struct ScanStats {
    u64 files_found = 0;
    u64 deleted_found = 0;
    u64 carved_found = 0;
    u64 lost_partitions = 0;
    u64 bytes_scanned = 0;
    u64 elapsed_ms = 0;
    u64 cached_hits = 0; /* R1.6 pipe counters (diagnostics) */
    u64 cached_misses = 0;
    ReadErrorMap errors;
    std::vector<lost::Candidate> lost_candidates;
};

/* Monotonic weighted progress: per-stage permille -> overall 0..1000. */
u32 weighted_progress(const StageWeights& w, u32 quick_pm, u32 deep_pm, u32 carve_pm);

/*
 * ScanController - runs the staged pipeline over ONE slice (a partition or
 * whole-disk view), producing one merged ResultTree.
 */
class ScanController {
public:
    ScanController(tree::ResultTree& tree, ScanConfig cfg);

    /* probe the slice's FS family from the boot sector (OEM fields). */
    static FsFamily probe_family(IStorageDevice* slice);

    u32 run(IStorageDevice* slice, CancelToken* cancel,
            bool (*progress)(u32 overall_permille, void*), void* progress_user, ScanStats& stats);

    /* Session integration: emit the standard record set. */
    u32 save_session(session::Writer& w, const ScanStats& stats) const;
    static u32 load_session(session::Reader& r, tree::ResultTree& tree,
                            ScanStats& stats /*summary only*/);

    const ScanConfig& config() const { return cfg_; }

private:
    u32 run_quick(IStorageDevice* slice, CancelToken* cancel, bool (*progress)(u32, void*),
                  void* pu, ScanStats& stats);
    u32 run_deep(IStorageDevice* slice, CancelToken* cancel, bool (*progress)(u32, void*), void* pu,
                 ScanStats& stats);
    u32 run_carve_stage(IStorageDevice* slice, CancelToken* cancel, bool (*progress)(u32, void*),
                        void* pu, ScanStats& stats);
    /* One structured engine pass (single walk; deep adds the extra passes).
     * Used by run_quick; run_deep skips it when quick already ran (fixes
     * the double MFT walk and FAT double emission in one move). */
    u32 run_engine_pass(IStorageDevice* slice, bool deep_mode, CancelToken* cancel,
                        bool (*progress)(u32, void*), void* pu, ScanStats& stats);
    u32 scan_candidate(IStorageDevice* cand_slice, tree::FullNode* croot, CancelToken* cancel,
                       u64& files_out, u64& deleted_out);
    tree::ResultTree& tree_;
    ScanConfig cfg_;
    tree::FullNode* root_found_ = nullptr;
    tree::FullNode* root_deleted_ = nullptr;
    tree::FullNode* root_lost_ = nullptr;
    tree::FullNode* root_carved_ = nullptr;
    u32 quick_done_ = false; /* quick stage completed its engine pass */
};

} // namespace orchestrator
} // namespace cr

#endif // CR_SCAN_H_
