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

enum class FsFamily : u8 { Unknown = 0, Ntfs, Fat, Exfat, CarvedOnly };

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
};

struct ScanStats {
    u64 files_found = 0;
    u64 deleted_found = 0;
    u64 carved_found = 0;
    u64 lost_partitions = 0;
    u64 bytes_scanned = 0;
    u64 elapsed_ms = 0;
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
    tree::ResultTree& tree_;
    ScanConfig cfg_;
    tree::FullNode* root_found_ = nullptr;
    tree::FullNode* root_deleted_ = nullptr;
    tree::FullNode* root_lost_ = nullptr;
    tree::FullNode* root_carved_ = nullptr;
};

} // namespace orchestrator
} // namespace cr

#endif // CR_SCAN_H_
