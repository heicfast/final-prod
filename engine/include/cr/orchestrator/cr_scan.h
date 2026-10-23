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
#include "../storage/cr_retry.h"
#include "../table/cr_ldm.h"
#include "../tree/cr_node.h"
#include "../session/cr_crsf.h"
#include "../session/cr_records.h"
#include "cr/lost/cr_lostpart.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

namespace cr {
namespace orchestrator {

enum class FsFamily : u8 {
    Unknown = 0,
    Ntfs,
    Fat,
    Exfat,
    Ext4,
    /* 5 reserved (was CarvedOnly - never produced by probe_family nor
     * matched by the dispatch; the name is removed, the value is NOT
     * renumbered because serialized fs_kind depends on it - FB-L03). */
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

    /* Volume label for the session VolumeInfo (G-14). Supplied by the
     * caller (the CLI/win layer reads it from the volume entry; engines
     * never own it). UTF-8, NUL-padded into the session record. */
    char volume_label[32] = {0};

    /* Device identity card (SESS-2/GAP-W3-9a): model/serial from the disk
     * enumeration, UTF-8 NUL-padded. Informational - the volume identity
     * proper is the boot-sector hash the orchestrator computes itself. */
    char volume_model[48] = {0};
    char volume_serial[32] = {0};

    /* ---- auto-checkpoint (G-13, issue #9) ----
     * Invoked by the orchestrator at STAGE BOUNDARIES with a freshly
     * serialized session (state + volume + contexts + tree + error map -
     * the same record set save_session emits). The caller provides the
     * IO triple (the file-backed sink writes <path>.tmp + flush +
     * rename: atomic per the platform); a failed sink write ABORTS the
     * scan with its error code (never a silent checkpoint gap). The
     * serialized mask is truthful (actual completion), so resume() from
     * the last checkpoint skips finished stages by construction. */
    struct CheckpointSink {
        u32 (*write)(void* user, const void* bytes, u32 len) = nullptr;
        u64 (*tell)(void* user) = nullptr;
        u32 (*seek)(void* user, u64 absolute) = nullptr;
        void* user = nullptr;
        explicit operator bool() const { return write && tell && seek; }
    };
    CheckpointSink checkpoint;

    /* Cross-disk LDM members (audit row 30 / LDM audit Bug 5): when a
     * whole-disk scan finds an LDM database, spanned/striped/mirrored/
     * RAID-5 volumes may reference member disks other than the scanned
     * one. The platform layer installs a GUID -> IStorageDevice resolver
     * built from every disk it can open (each device must outlive the
     * scan). Empty resolver = single-disk semantics (only the scanned
     * disk's volumes resolve). */
    table::LdmDiskResolver ldm_resolver;
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

/* ---- live scan observability (G-04/G-05/G-06) ----
 *
 * Per-stage progress with honest units: `units` are whatever the underlying
 * engine counts for that stage (MFT records, dentries, inodes, tree nodes,
 * carve windows); the carve stage counts BYTES. ETA is derived from
 * permille + elapsed - `bytes_scanned` remains carve-true-bytes only
 * (structured stages cannot report honest per-byte coverage without
 * fabricating it). All fields are snapshots; overall_permille is clamped
 * monotonic. */
struct StageTelemetry {
    u32 permille = 0;    /* 0..1000 within the stage, monotonic */
    u64 units_done = 0;  /* engine-reported work units so far */
    u64 units_total = 0; /* engine-reported total (0 = unknown) */
    u64 bytes_done = 0;  /* bytes counted by stages that know them (carve) */
    u64 elapsed_ms = 0;  /* stage wall-time, pause-compensated */
    bool completed = false;
};

struct ScanTelemetrySnapshot {
    u32 overall_permille = 0; /* weighted, monotonic */
    StageTelemetry quick, deep, carve;
    u64 files_found = 0;
    u64 deleted_found = 0;
    u64 carved_found = 0;
    u64 lost_partitions = 0;
    u64 bytes_scanned = 0; /* carve-true-bytes (honest fill-rate) */
    ReadErrorMap errors;   /* live bad-sector map snapshot (G-06) */
    u64 elapsed_ms = 0;    /* pause-compensated run time so far */
    bool paused = false;
    bool running = false;
    bool cancelled = false;
};

/* Monotonic weighted progress: per-stage permille -> overall 0..1000. */
u32 weighted_progress(const StageWeights& w, u32 quick_pm, u32 deep_pm, u32 carve_pm);

/* stage_done_mask bit assignments (SessionState + VolumeInfo.flags share
 * these; G-07 resume consumes them). */
constexpr u32 STAGE_QUICK = 1;
constexpr u32 STAGE_DEEP = 2;
constexpr u32 STAGE_CARVE = 4;

/*
 * ScanController - runs the staged pipeline over ONE slice (a partition or
 * whole-disk view), producing one merged ResultTree.
 */
class ScanController {
public:
    /* Fresh scan: pre-creates the standard roots the configured stages
     * will emit into. */
    ScanController(tree::ResultTree& tree, ScanConfig cfg);

    /* Resume path (G-07): `resumed` carries the saved session's completion
     * state - stages completed there keep their results (roots already in
     * the loaded tree; internal completion flags restored so run()
     * continues where the session stopped). cfg must already have the
     * completed stages cleared (see load_session_summary). */
    ScanController(tree::ResultTree& tree, ScanConfig cfg,
                   const session::VolumeSessionInfo& resumed);

    /* probe the slice's FS family from the boot sector (OEM fields). */
    static FsFamily probe_family(IStorageDevice* slice);

    u32 run(IStorageDevice* slice, CancelToken* cancel,
            bool (*progress)(u32 overall_permille, void*), void* progress_user, ScanStats& stats);

    /* Session integration: emit the standard record set. */
    u32 save_session(session::Writer& w, const ScanStats& stats) const;
    static u32 load_session(session::Reader& r, tree::ResultTree& tree,
                            ScanStats& stats /*summary only*/);

    /* ---- resume support (G-07) ----
     * Full flow: load the session (tree + stats + summary), validate the
     * stored completion mask against the stored configured-stages flags,
     * clear completed stages from the original config, and construct a
     * controller that continues where the session stopped. The caller
     * keeps `out` and calls run() with the SAME slice the original scan
     * used. Errors: ERR_SESSION_* from the container; ERR_SESSION_FORMAT
     * on mask integrity violations (never a silent partial resume).
     * Note (documented truth): sessions written before the mask-truth fix
     * stamped the CONFIGURED mask as done, so resuming them yields a
     * no-op run - they claim completion by construction. */
    static u32 resume(session::Reader& r, tree::ResultTree& tree, const ScanConfig& original_cfg,
                      ScanStats& stats, session::VolumeSessionInfo& info,
                      std::unique_ptr<ScanController>& out);

    /* Load with the full restored summary (G-14/G-23): tree + stats +
     * volume info + lost candidates + completion masks, with the same
     * integrity validation as resume(). Use for session-info queries
     * without continuing the scan. */
    static u32 load_session_summary(session::Reader& r, tree::ResultTree& tree, ScanStats& stats,
                                    session::VolumeSessionInfo& info);

    /* ---- live telemetry (G-04/G-05/G-06) ----
     * Thread-safe snapshot of the running scan's state; callable from any
     * thread while run() executes (and after it returns). Includes the live
     * bad-sector map merged from the retry pipe. */
    ScanTelemetrySnapshot telemetry() const;

    /* G-13: checkpoints emitted so far this run (0 when no sink). */
    u64 checkpoint_count() const { return checkpoint_count_; }

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
     * the double MFT walk and FAT double emission in one move).
     * bridge_stage/covers_both steer telemetry: the pass reports its engine
     * progress into stage `bridge_stage` (0=quick, 1=deep); when the pass
     * carries deep semantics for a quick+deep run it also feeds deep. */
    u32 run_engine_pass(IStorageDevice* slice, bool deep_mode, CancelToken* cancel,
                        bool (*progress)(u32, void*), void* pu, ScanStats& stats, u32 bridge_stage,
                        bool covers_both);
    u32 scan_candidate(IStorageDevice* cand_slice, tree::FullNode* croot, CancelToken* cancel,
                       u64& files_out, u64& deleted_out, bool (*progress)(u64, u64, void*),
                       void* progress_user);

    /* ---- telemetry internals (G-04/G-05/G-06) ---- */
    struct TelemetryState {
        std::mutex m;
        ScanTelemetrySnapshot snap;
        std::chrono::steady_clock::time_point run_t0{};
        std::chrono::steady_clock::time_point stage_t0{};
        u64 stage_pause_base = 0; /* run-wide paused_ms() at stage start */
    };
    /* Engine->stage progress bridge context (raw fn-pointer trampoline). */
    struct StageBridge {
        ScanController* self;
        bool (*overall)(u32, void*);
        void* user;
        u32 stage;        /* 0 quick, 1 deep, 2 carve */
        bool covers_both; /* the pass runs deep semantics for quick+deep */
        const CancelToken* cancel;
    };
    static bool engine_progress_bridge_(u64 done, u64 total, void* user);
    void tel_stage_begin_(u32 stage, const CancelToken* cancel);
    void tel_stage_end_(u32 stage, const CancelToken* cancel);
    void tel_publish_(u32 stage, u64 done, u64 total, bool bytes_units, const CancelToken* cancel);
    void tel_counts_(const ScanStats& stats, const CancelToken* cancel);
    /* Post-stamp overall permille (monotonic truth) for the boundary
     * progress calls - NEVER a hardcoded weighted() guess, which could
     * regress below what the bridge already forwarded mid-stage. */
    u32 tel_overall_() const;
    /* G-13: emit one checkpoint through cfg_.checkpoint (a full session
     * record set). Returns the sink's error on failure (the caller aborts
     * the scan - no silent checkpoint gaps). */
    u32 emit_checkpoint_() const;

    tree::ResultTree& tree_;
    ScanConfig cfg_;
    tree::FullNode* root_found_ = nullptr;
    tree::FullNode* root_deleted_ = nullptr;
    tree::FullNode* root_lost_ = nullptr;
    tree::FullNode* root_carved_ = nullptr;
    bool quick_done_ = false; /* quick stage completed its engine pass (ARCH-7) */
    /* actual stage completion for stage_done_mask truth (G-07 resume) */
    bool deep_extras_done_ = false; /* remnant sweep + lost discovery done */
    bool carve_done_ = false;
    /* telemetry plumbing */
    mutable TelemetryState tel_;
    mutable std::atomic<u32> tel_stage_ = 0; /* current stage for worker-side publishes */
    mutable std::atomic<bool> tel_running_ = false;
    std::atomic<RetryingDevice*> tel_retry_ = nullptr;
    /* the caller's token, for live paused()/paused_ms() refresh in
     * telemetry() while run() executes (cleared under tel_.m at teardown) */
    std::atomic<const CancelToken*> tel_cancel_ = nullptr;
    /* stage completion truth (G-07 resume; save_session stamps the mask
     * from ACTUAL completion, not the config) */
    bool pass_deep_done_ = false; /* deep-semantics engine pass completed */
    void adopt_resume_state_(const session::VolumeSessionInfo& resumed);
    /* volume facts recorded during run() for the session VolumeInfo
     * (G-14) and the original configured mask for resume truth */
    u64 vol_offset_ = 0;
    u64 vol_bytes_ = 0;
    u32 vol_logical_sector_ = 0;
    u32 vol_physical_sector_ = 0;
    u32 vol_boot_hash_ = 0; /* SESS-2: CRC32 of the slice's first 512 bytes */
    u32 session_configured_mask_ = 0;
    /* G-13: the run's stats reference for checkpoint serialization +
     * emitted-checkpoint count (diagnostics) */
    mutable ScanStats* checkpoint_stats_ = nullptr;
    mutable u64 checkpoint_count_ = 0;
};

} // namespace orchestrator
} // namespace cr

#endif // CR_SCAN_H_
