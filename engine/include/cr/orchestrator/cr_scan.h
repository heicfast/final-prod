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

#include "cr/tree/cr_grading.h"
#include "../storage/cr_device.h"
#include "../storage/cr_cached.h"
#include "../storage/cr_freemap.h"
#include "../storage/cr_retry.h"
#include "../table/cr_ldm.h"
#include "../tree/cr_node.h"
#include "../session/cr_crsf.h"
#include "../session/cr_records.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/check/cr_fscheck.h"
#include "cr/repair/cr_rawfixer.h"
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
     * renumbered because serialized fs_kind depends on it - FB-L03).
     * N-076: the anchor was MISSING since the Hfsplus landing - the
     * auto-numbering had silently consumed value 5 (Hfsplus=5,
     * BitLocker=9), contradicting every downstream contract that quotes
     * the documented values (cr_ui's fs_hint cases 6..9, S-019's
     * "BitLocker appended (value 10)", this very comment). The explicit
     * anchor below restores the documented numbering; sessions written
     * by the intermediate builds (fs_kind 5..9 for Hfsplus..BitLocker)
     * predate any release and reload with an Unknown family at worst -
     * never a wrong-family read (see the N-076 finding for the matrix). */
    Reserved5 = 5, /* FB-L03 / N-076: held, never produced */
    /* Values 6..9 appended after v0.1.0: the serialized fs_kind numbers
     * 0..5 keep their meaning, so old sessions stay readable. */
    Hfsplus = 6,
    Btrfs = 7,
    Zfs = 8,
    Refs = 9,
    /* 10: S-019 - lost BitLocker (FVE) volumes; no walk engine (locked
     * encrypted content), the candidate surfaces as a locked volume. */
    BitLocker = 10,
    /* 11: M-044 - UDF (the og's analyser result 0xc, "(UDF)" in DRWUI's
     * label map). Recognition + classification only: the og has NO UDF
     * structured engine (the 9-engine SearchDLL set), UDF content
     * recovers through the RAW/carve family exactly like the og. */
    Udf = 11
};

struct StageWeights {
    u32 quick = 40; /* percent-of-total per stage (monotonic weighted) */
    u32 deep = 35;
    u32 carve = 25;
};

/* ---- M-058: the scan read pipe, as a testable unit -----------------------
 * run()'s inline construction had a defensive-path use-after-free: when
 * CachedReader::create fails it DESTROYS the retry device it took by
 * value, so the old fallback `pipe = retry_raw` dereferenced freed memory
 * (and contradicted its own comment, which documented the bare-slice
 * fallback). Extracted so the fallback is a pinned, tested contract:
 * cache-creation failure degrades to the BARE SLICE (retry isolation
 * lost on that path - unreachable with run()'s constant arguments, but
 * memory-safe and truthful when taken). */
struct ReadPipe {
    std::unique_ptr<CachedReader> chain; /* owns the cache when built; null
                                            on the degraded bare-slice path */
    IStorageDevice* pipe = nullptr;      /* what engines read through (the
                                            slice itself on the degraded path) */
    RetryingDevice* retry = nullptr;     /* live error-map source (the cache's
                                            inner device; null on degrade) */
};
/* cache_bytes/block_bytes parameterized for the regression test; run()
 * passes (8 MiB, 256 KiB). slice must outlive the returned pipe. */
ReadPipe build_read_pipe(IStorageDevice* slice, u64 cache_bytes, u32 block_bytes);

/* ---- typed event stream (GAP-S6-10; ABI-CONTRACT.md section 6) ----
 *
 * Corpus lineage: the reference pipes engine events to the UI through a
 * typed message hub (INotifyObserver::Notify -> CMessageProxy thread
 * marshaling -> doDispatchMessage(int, void*, int&) fan-out - MOM-modules
 * section 4, UIUX/13 section 3). This is the core's equivalent: a flat
 * typed event a consumer subscribes to incrementally - stage transitions,
 * progress samples, files-found counts, error-map deltas, checkpoints,
 * lost-partition candidates. Transport-agnostic by construction (a POD
 * with no pointers - a bridge can serialize it byte-for-byte).
 *
 * Contract:
 *  - Events may fire from ENGINE WORKER THREADS (the same threads the
 *    progress callback uses); the core does not serialize deliveries
 *    (single-flight is the bridge's job, as the cr_ui shim does).
 *  - Must not block; no cancellation semantics (the token is).
 *  - Stage transitions fire in scan order, exactly once each. Progress
 *    samples fire when the overall permille advances (coalesced, <= 1000
 *    per scan plus stage flips). Files-found/error events are STAGE-BOUNDARY
 *    samples (the engines do not publish per-file increments); the
 *    error-map delta carries the newest appended range plus the
 *    cumulative count. Checkpoint fires per G-13 emission. Lost-candidate
 *    events fire per discovered candidate (deep stage, whole-disk scans).
 *  - Unknown `type` values MUST be ignored by consumers (forward compat). */
namespace event {
enum class Type : u32 {
    ScanStage = 1,     /* a stage transition (quick/deep/carve) */
    ScanProgress = 2,  /* a coalesced progress sample */
    FilesFound = 3,    /* files-found counters advanced (stage boundary) */
    ErrorRange = 4,    /* a read-failure range was appended (delta) */
    Checkpoint = 5,    /* a G-13 checkpoint was written */
    LostCandidate = 6, /* a lost-partition candidate was found */
    Repair = 7,        /* S-008: pre-scan repair lifecycle (see Repair fields) */
    /* NOTE: ABI wire values 8 (CR_EVT_HEARTBEAT) and 9
     * (CR_EVT_SAVE_PROGRESS) are SHIM-side events (the watchdog thread
     * and the save path) - the core never emits them. The core's next
     * event type therefore starts at 10. */
    FsCheck = 10 /* M-041: pre-scan damage-audit phase done (see check
                  * fields; fires ONCE per scan with the final report
                  * summary — per-error detail rides the session record
                  * and stats) */
};

struct Event {
    Type type = Type::ScanStage;
    u32 stage = 0xFFFFFFFFu; /* 0 quick, 1 deep, 2 carve, 0xFFFFFFFF none */
    u64 timestamp_ms = 0;    /* run-elapsed, pause-compensated */
    /* ScanProgress */
    u64 bytes_done = 0, bytes_total = 0; /* stage-true bytes where counted (carve) */
    u32 stage_permille = 0, overall_permille = 0;
    /* FilesFound (cumulative counters, not deltas) */
    u64 files_found = 0, deleted_found = 0, carved_found = 0;
    /* ErrorRange: the newest appended span (absolute device bytes) */
    u64 err_start_bytes = 0, err_len_bytes = 0;
    u32 error_ranges = 0; /* cumulative count after the append */
    /* ErrorRange context (M-049 - the engine-side subset of the og's
     * error objects): the module that was reading (cr_errmod.h ids; the
     * newest range's attribution, exact for every orchestrator-stamped
     * sub-pass), the operation (read - this family is read failures) and
     * the retry state (attempts spent before giving up). Zero on other
     * event types. */
    u32 err_module = 0;
    u32 err_operation = 0;
    u32 err_attempts = 0;
    /* Checkpoint */
    u64 checkpoint_index = 0;
    /* LostCandidate */
    u64 candidate_lba = 0;
    /* Repair (Type::Repair; S-008). Fires (a) when analyze() produced a
     * non-empty, non-refused plan - repair_outcome 0 ("ready"),
     * repair_actions = planned action count - and (b) once the outcome is
     * known: 1 applied (repair_actions = actions actually written),
     * 2 consent denied, 3 execution failed or writer unavailable.
     * stage stays 0xFFFFFFFF (pre-scan). */
    u64 repair_actions = 0;
    u32 repair_outcome = 0;
    /* FsCheck (Type::FsCheck; M-041). check_errors = the report's issue
     * count; check_family = the detected family byte ('f'/'n'/0); the
     * full report (codes, params, paths) persists as REC_FS_CHECK and
     * surfaces in ScanStats. stage stays 0xFFFFFFFF (pre-scan). */
    u64 check_errors = 0;
    u32 check_family = 0;
};

using EventFn = void (*)(const Event& evt, void* user);
} // namespace event

struct ScanConfig {
    bool run_quick = true;
    bool run_deep = true;
    bool run_carve = true;
    bool whole_disk = false;              /* enables lost-partition discovery */
    FsFamily fs_hint = FsFamily::Unknown; /* auto-probed when Unknown */
    StageWeights weights;
    u32 threads = 0; /* 0 = auto (hw-1, clamp 1..8); 1 = single-threaded */

    /* ---- pre-scan repair (S-008; corpus RawFixer pre-scan position:
     * repair, then scan) ----
     * OFF by default (CLI default; the reference UI flow enables it with
     * a consent prompt). When ON, run() executes repair::analyze() on the
     * caller's slice BEFORE any engine/probe reads, surfaces the plan
     * (event stream + stats), and executes the transactional writes ONLY
     * when the consent hook returns true AND a write view can be opened.
     * No hooks = analysis surfaced, nothing ever written (S1). A failed or
     * denied repair NEVER aborts the scan - the og falls through to
     * scanning/caving the unrepaired volume the same way. */
    bool pre_scan_repair = false;
    struct RepairHooks {
        /* Called once per scan when analyze() produced a non-empty,
         * non-refused plan, before any write. Called on the scan thread
         * (run()'s caller thread) BEFORE the stages start; may prompt a
         * user. NULL = consent never granted (plans surface, never
         * execute). */
        bool (*consent)(const repair::RepairPlan& plan, void* user) = nullptr;
        /* Open a WRITE view of the same bytes the scan reads (the platform
         * layer's RW handle at the same slice offset). NULL return =
         * repair unavailable this scan (recorded, scan proceeds). */
        repair::IWriteDevice* (*open_writer)(void* user) = nullptr;
        /* Close the writer exactly once per successful open (NULL device
         * allowed), before run() proceeds. */
        void (*close_writer)(repair::IWriteDevice* dev, void* user) = nullptr;
        void* user = nullptr;
    } repair;

    /* ---- M-041 pre-scan damage-audit check phase (the og's
     * FileSystemCheck: CCheckFAT + CCheckNTFSVolume, corpus
     * ghidra-mo/FileSystemCheck_decompiled.c) ----
     * OFF by default (like pre_scan_repair: the og runs the check from
     * its dedicated check/repair flows, not every scan). When ON, run()
     * executes check::run_fscheck() on the caller's slice AFTER the
     * repair stage (the og's order — repair writes first, the audit
     * sees the repaired bytes) and BEFORE any engine/probe read. The
     * report NEVER aborts the scan (the og's escalation stance: the
     * audit informs, the scan proceeds — rawfixer-fs-repair.md:59-65).
     * Surfacing: ScanStats.check_* + event::Type::FsCheck + the
     * REC_FS_CHECK session record. */
    bool pre_scan_check = false;

    /* Volume label for the session VolumeInfo (G-14). Supplied by the
     * caller (the CLI/win layer reads it from the volume entry; engines
     * never own it). UTF-8, NUL-padded into the session record. */
    char volume_label[32] = {0};

    /* Device identity card (SESS-2/GAP-W3-9a): model/serial from the disk
     * enumeration, UTF-8 NUL-padded. Informational - the volume identity
     * proper is the boot-sector hash the orchestrator computes itself. */
    char volume_model[48] = {0};
    char volume_serial[32] = {0};

    /* N-066 (the og record-3 identity axes): firmware revision, WWN /
     * serial-2, and the interface description from the disk enumeration
     * (UTF-8 NUL-padded; zero-init = absent - pre-N-066 hosts and image
     * targets leave them empty and the v4 card fields stay zeros). */
    char volume_firmware[32] = {0};
    char volume_wwn[32] = {0};
    char volume_interface[32] = {0};

    /* S-030 (image targets): the image FILE path when the scan target is
     * a disk image (the og's import flow keeps the file identity for the
     * device list and the resume flow). UTF-8, NUL-padded; empty for
     * --disk/--volume targets and pre-v3 sessions. */
    char volume_image[256] = {0};

    /* M-029 (the narrowed "server-side push", session-5 correction): the
     * display name for the carve/raw category root. The og names its
     * general-RAW category node through a one-shot per-scan config fetch
     * (the "filter_generalraw" key through the controller's vtable+0x48
     * hook - localized/remote-config-driven; corpus
     * evidence/decompiled-quotes/s5-oq-s4-2-raw-naming-hook.md). Empty =
     * the default "Other Deleted Files (Carved)" role. UTF-8, NUL-padded;
     * the resume path recognizes BOTH the default and the configured name
     * so a re-run never spawns a duplicate raw root. */
    char raw_category_name[64] = {0};

    /* ---- auto-checkpoint (G-13, issue #9) ----
     * Invoked by the orchestrator at STAGE BOUNDARIES with a freshly
     * serialized session (state + volume + contexts + tree + error map -
     * the same record set save_session emits). The caller provides the
     * IO triple + optional commit hook (the file-backed sink writes
     * <path>.tmp + flush + rename: atomic per the platform, N-014);
     * a failed sink write ABORTS the
     * scan with its error code (never a silent checkpoint gap). The
     * serialized mask is truthful (actual completion), so resume() from
     * the last checkpoint skips finished stages by construction. */
    struct CheckpointSink {
        u32 (*write)(void* user, const void* bytes, u32 len) = nullptr;
        u64 (*tell)(void* user) = nullptr;
        u32 (*seek)(void* user, u64 absolute) = nullptr;
        /* N-014 (diff-to-fix): invoked after EVERY successful finalize()
         * - the atomic commit. File-backed sinks fflush + close + rename
         * <tmp> -> <final> + reopen the tmp for the next emit, so a
         * crash mid-emit leaves the FINAL path holding the previous good
         * checkpoint (the og's fresh-file-per-save crash-safety,
         * rsf-format--scan-session.md §5.5) and a torn .tmp that the
         * next run simply overwrites. Null = not file-backed (in-memory
         * sinks: behavior unchanged). A failed commit aborts the scan
         * with its error code - same contract as a failed write. */
        u32 (*commit)(void* user) = nullptr;
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

    /* ---- typed event stream (GAP-S6-10) ----
     * Optional; null = no events. May fire from engine worker threads
     * (see the event:: namespace contract above). Zero-init safe. */
    event::EventFn event_fn = nullptr;
    void* event_user = nullptr;
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
    /* M-005: used-cluster bytes the carve dispatch never offered (the
     * og's block-scan bitmap gate). 0 when no bitmap was available. */
    u64 carve_bitmap_skipped_bytes = 0;
    /* N-031: all-zero 512-byte strides whose raw-sign probe was skipped
     * (the og's zero-sector fast-skip analog; diagnostics/telemetry). */
    u64 carve_zero_strides = 0;
    /* M-039: carve positions suppressed by the occupancy interval gate
     * (the cross-engine FS-accounted-extent skip - the og's per-volume
     * bitmap every FS engine fills; both carve layers count). */
    u64 carve_occupancy_skipped = 0;
    /* M-018: nodes the FilterFolder pass flagged (the og's
     * PushFolderFilter hidden set - OS droppings, metadata noise,
     * localized "New Folder" names, SVI, BitLocker artifacts). */
    u64 junk_filtered = 0;

    /* M-035: lost candidates the engine-side content-consistency scorer
     * rejected (the og's exFAT-only FUN_180025390 rule - majority
     * checkable + match ratio < 0.3 + at least one checkable file, or a
     * structurally impossible data start). The candidate stays listed
     * (the og's per-hit persistence keeps the find; M-042) but its walked
     * files are detached from the tree and its counts report zero. */
    u64 lost_content_rejected = 0;
    ReadErrorMap errors;
    std::vector<lost::Candidate> lost_candidates;

    /* ---- S-009: the engine allocation bitmap (the og's shared
     * IBuildVolumeBitmap truth - "which clusters are free/used",
     * searchdll-family--architecture.md section 2). Filled by the main
     * engine pass from the filesystem allocation structures (NTFS
     * $Bitmap, FAT/exFAT tables); ABSOLUTE device bytes (rebased like
     * the tree extents - F-1). Empty for engines without a bitmap and
     * for sessions saved before the record existed; the disk-map free
     * overlay degrades to unknown in that case, never to a guess. ---- */
    storage::FreeMap free_map;

    /* ---- S-008 pre-scan repair outcome (filled when cfg.pre_scan_repair) ----
     * repair_status: OK (clean, applied, or surfaced-but-denied), or the
     * analyze/execute error code. repair_undo is populated only when
     * actions were applied; the CALLER persists it (the CLI writes the
     * durable container next to the session). */
    bool repair_ran = false;
    bool repair_detected = false;
    char repair_family[8] = {0}; /* "ntfs"/"fat"/"exfat"/"" */
    u32 repair_status = 0;
    u32 repair_actions_planned = 0;
    u32 repair_actions_applied = 0;
    bool repair_consent_denied = false;
    bool repair_writer_unavailable = false;
    std::vector<std::string> repair_advisories;
    repair::UndoLog repair_undo;

    /* ---- M-041 pre-scan damage-audit report (filled when
     * cfg.pre_scan_check) ----
     * The full issue list (the og's 0x1001..0x100e taxonomy with
     * cluster/LCN/version params and path hints) + the summary counters.
     * Persisted as REC_FS_CHECK; the event stream carries the summary. */
    bool check_ran = false;
    bool check_detected = false;
    u32 check_status = 0; /* run_fscheck's return (OK even on findings) */
    check::FsCheckReport check_report;

    /* ---- S-023: post-scan grading (the og's CRecoverability model) ----
     * Filled by the grading pass at scan completion; the per-node grades
     * live on the tree nodes (tree::Grade) and persist with the session. */
    tree::grading::AssistantStats assistant;

    /* ---- M-042: resume-time disk-truth signals ----
     * layout_changed: the persisted REC_DISK_LAYOUT snapshot differs from
     * the freshly enumerated table at run start (the og's partinfo
     * "disk changed" flag, PRD:20998-21039). Surfaced, never enforced.
     * revalidated_candidates / dropped_stale_candidates: the per-record
     * re-validation bookkeeping (the og re-reads every persisted record
     * and drops stale ones before the UI sees them).
     * persisted_layout: the session's REC_DISK_LAYOUT entries (empty on
     * sessions saved before the record existed). Filled by
     * load_session_summary; consumed (and cleared) by the run-start
     * compare — a per-search-start signal, exactly the og's timing. */
    bool layout_changed = false;
    u64 revalidated_candidates = 0;
    u64 dropped_stale_candidates = 0;
    std::vector<session::DiskLayoutEntry> persisted_layout;
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
    /* N-060: optional save-path progress + cancel hook (per-node; false
     * return aborts with ERR_CANCELLED - the og's per-child IsCancelled
     * save teardown). NULL = the silent save (pre-N-060 behavior). */
    u32 save_session(session::Writer& w, const ScanStats& stats,
                     session::SerializeProgress progress = nullptr,
                     void* progress_user = nullptr) const;
    /* M-042: revalidate_disk, when non-null, is the CURRENT disk the
     * session is being used against - every restored lost candidate is
     * re-read and re-classified at its start LBA through it, and records
     * the disk no longer confirms are DROPPED (the og's resume
     * re-validation, PRD:20748-20842). nullptr = device-less listing
     * (the og has no device-less surface; candidates restore as-is -
     * the drop is a search-start behavior, not a listing one). */
    static u32 load_session(session::Reader& r, tree::ResultTree& tree,
                            ScanStats& stats /*summary only*/,
                            IStorageDevice* revalidate_disk = nullptr);

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
                      std::unique_ptr<ScanController>& out,
                      IStorageDevice* revalidate_disk = nullptr);

    /* Load with the full restored summary (G-14/G-23): tree + stats +
     * volume info + lost candidates + completion masks, with the same
     * integrity validation as resume(). Use for session-info queries
     * without continuing the scan. M-042: revalidate_disk (nullable) is
     * the device-bearing resume/listing surfaces' current disk - the
     * per-record re-validation runs when the caller has one, matching
     * the og's at-search-start drop of stale records. */
    static u32 load_session_summary(session::Reader& r, tree::ResultTree& tree, ScanStats& stats,
                                    session::VolumeSessionInfo& info,
                                    IStorageDevice* revalidate_disk = nullptr);

    /* ---- live telemetry (G-04/G-05/G-06) ----
     * Thread-safe snapshot of the running scan's state; callable from any
     * thread while run() executes (and after it returns). Includes the live
     * bad-sector map merged from the retry pipe. */
    ScanTelemetrySnapshot telemetry() const;

    /* G-13: checkpoints emitted so far this run (0 when no sink). */
    u64 checkpoint_count() const { return checkpoint_count_; }

    /* GAP-S6-10: subscribe/replace the typed event sink. PRE-RUN only
     * (returns ERR_BAD_PARAMETER once run() has started - a mid-run sink
     * swap could double-deliver or drop events). NULL unsubscribes. The
     * initial sink comes from ScanConfig (construction time); this setter
     * exists so the RESUMED flow (controller built inside resume()) can
     * subscribe after the handle exists, before run(). */
    u32 set_event_sink(event::EventFn fn, void* user);

    const ScanConfig& config() const { return cfg_; }

private:
    /* M-029: the raw/carve category root's role string - the configured
     * display name (cfg_.raw_category_name, the og's "filter_generalraw"
     * surface) when set, the default role otherwise. */
    std::u16string raw_root_name_() const;
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
                       void* progress_user, u64* content_rejected = nullptr);

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

    /* ---- typed event stream (GAP-S6-10) ----
     * emit_event_ fills the common fields (stage/timestamp) and calls the
     * configured sink; emit_progress_event_ coalesces by overall permille
     * (an atomic last-emitted tracker); emit_boundary_events_ fires
     * FilesFound + ErrorRange deltas at the stage-boundary/stamp merge
     * points (where the counters + error map actually advance). */
    void emit_event_(event::Type type, u32 stage) const;
    void emit_progress_event_(u32 stage) const;
    void emit_boundary_events_(u32 stage) const;
    void emit_lost_candidate_(const lost::Candidate& cand) const;
    /* M-049: publish the sub-pass owner module on the retry device (the
     * fallback engine-internal worker pools attribute to). No-op before
     * the pipe exists (tel_retry_ null). */
    void publish_owner_module_(u32 module) const {
        if (RetryingDevice* rd = tel_retry_.load(std::memory_order_acquire))
            rd->set_owner_module(module);
    }
    mutable std::atomic<u32> evt_last_overall_{0xFFFFFFFFu};
    mutable std::atomic<u32> evt_last_errors_{0};
    /* the live sink (constructed from cfg_; overridable pre-run so the
     * resumed flow - whose controller is built inside resume() - can
     * subscribe after the handle exists) */
    event::EventFn event_fn_ = nullptr;
    void* event_user_ = nullptr;

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
    /* N-066: the full 512-byte boot-sector image behind vol_boot_hash_
     * (the og record-3's +0x518 snapshot - mismatch diagnostics and the
     * offline re-materialization datum). valid=false when the read failed
     * or the session was resumed pre-run. */
    u8 vol_boot_snapshot_[512] = {0};
    bool vol_boot_snapshot_valid_ = false;
    u32 session_configured_mask_ = 0;
    /* G-13: the run's stats reference for checkpoint serialization +
     * emitted-checkpoint count (diagnostics) */
    mutable ScanStats* checkpoint_stats_ = nullptr;
    mutable u64 checkpoint_count_ = 0;
    /* M-042: the live whole-disk table captured at run() start (saved as
     * REC_DISK_LAYOUT at save/checkpoint time - the og's partinfo.dat
     * semantics; empty for volume-basis views where parse_disk_table has
     * no whole-disk table). resumed_whole_disk_: the session being
     * resumed ran on a whole-disk view (slice_offset == 0) - the run-start
     * candidate re-validation runs only then (the persisted LBAs are
     * disk-relative; a volume-basis re-read would be the wrong arbiter). */
    std::vector<session::DiskLayoutEntry> disk_layout_;
    bool resumed_whole_disk_ = false;
};

} // namespace orchestrator
} // namespace cr

#endif // CR_SCAN_H_
