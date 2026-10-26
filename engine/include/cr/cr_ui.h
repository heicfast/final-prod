/*
 * cr_ui.h - the cr_ui C-ABI shim: the flat-C boundary a UI host binds to
 * (GAP G-01, issue #1; docs/UI-READINESS-PLAN.md #1).
 *
 * ARCHITECTURE LINEAGE (why this layer exists): the reference product binds
 * its UI to the engines IN-PROCESS through C-export factories - DRWUI.exe
 * LoadLibrary's the *SearchDLL.dll engines and GetProcAddress'es
 * "EUCreate<FS>Searcher" / "EUCreate<FS>QuickSearcher", which return
 * controller interface pointers (corpus: searchdll-family--architecture.md
 * section 1; DRWUI-exe--main-ui-architecture.md section 9.2, decompiled
 * loader FUN_14056c0d0 + factory map FUN_140577d40). Partition ops go
 * through MOM (.mo modules, GetModule/CreateObject exports). There is NO
 * CLI subprocess and NO IPC on the UI<->engine leg of the reference.
 *
 * This header is our equivalent of that factory surface, adapted to the
 * core's contract: a C11 flat ABI over cr_core (+ the cr_win platform
 * layer on Windows) so a UI (or any host - the C consumer target is the
 * proof) can drive the full scan/recover lifecycle in-process without
 * linking C++ classes: opaque handles stand in for the controller
 * interfaces, cr_ui_* factories stand in for EUCreate*Searcher, and C
 * callbacks stand in for the engine event hooks.
 *
 * ABI RULES
 *   - C11, no C++ types cross the boundary, no exception ever escapes
 *     (every entry has a catch-all that returns CR_UI_ERR_INTERNAL).
 *   - Every function returns a u32 error code from the SAME numeric space
 *     as cr_error.h (0 = success; e.g. 0x50020005 = ERR_CANCELLED).
 *   - Strings in: NUL-terminated UTF-8. Strings out: caller buffers
 *     (two-call size pattern) or heap strings freed with cr_ui_string_free.
 *   - Zero-initialized config structs are ALWAYS safe: numeric fields use
 *     0 = "default" everywhere (never 0 = "nothing matches").
 *   - Handles are opaque; close every handle you open. Lifetime rules are
 *     documented per family below.
 *   - Thread safety: scan/recovery run on the caller's thread; pause/
 *     cancel/telemetry and snapshot capture are safe from any thread.
 *     The progress callbacks fire on the worker thread only, must not
 *     block, and carry NO cancellation semantics (the token functions are
 *     the cancel path - mirroring the reference's separate abort event).
 *   - ABI VERSIONING: cr_ui_abi_version() returns CR_UI_ABI_VERSION.
 *     Compatibility policy (docs/ABI-CONTRACT.md section 9 - the operative
 *     policy; this comment was corrected to match it in the core-api
 *     session after the two documents disagreed): ADDING exports, APPENDING
 *     struct fields, and adding event `type` values are COMPATIBLE and do
 *     NOT bump (consumers detect features by export presence / zero-init
 *     defaults in appended fields). Breaking changes (removing/renaming an
 *     export, changing a field's type/meaning/order, changing an existing
 *     symbol's ownership or threading contract) bump the version. Fields
 *     are never reordered or removed within a version.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_UI_H_
#define CR_UI_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- export macro ------------------------------------------------------- */

#if defined(_WIN32)
#if defined(CR_UI_BUILD)
#define CR_UI_API __declspec(dllexport)
#else
#define CR_UI_API __declspec(dllimport)
#endif
#else
#define CR_UI_API __attribute__((visibility("default")))
#endif

#define CR_UI_ABI_VERSION 2u

/* Selected error codes (full table: cr_error.h; same numeric space). */
#define CR_UI_OK 0x00000000u
#define CR_UI_ERR_INTERNAL 0x50090001u
#define CR_UI_ERR_BAD_PARAMETER 0x50030003u
#define CR_UI_ERR_CANCELLED 0x50020005u
#define CR_UI_ERR_TRUNCATED 0x50030004u
#define CR_UI_ERR_UNSUPPORTED 0x50030008u /* e.g. win-layer surface on Linux */
#define CR_UI_ERR_NO_MEMORY 0x50030009u
#define CR_UI_ERR_BAD_CHECKSUM 0x50030002u /* identity mismatch (see verify) */

/* Opaque handles. */
typedef struct cr_ui_device cr_ui_device;
typedef struct cr_ui_scan cr_ui_scan;
typedef struct cr_ui_session cr_ui_session;
typedef struct cr_ui_snapshot cr_ui_snapshot;
typedef struct cr_ui_preview cr_ui_preview;
typedef struct cr_ui_recovery cr_ui_recovery;

/* ---- version & diagnostics ---------------------------------------------- */

/* Returns CR_UI_ABI_VERSION of this library. Call first; refuse to run on
 * a mismatch. */
CR_UI_API uint32_t cr_ui_abi_version(void);

/* Thread-local diagnostic text for the LAST failing cr_ui_* call on this
 * thread ("no error" when the last call succeeded). Stable until the next
 * cr_ui_* call on the same thread. Never NULL. */
CR_UI_API const char* cr_ui_last_error(void);

/* Frees a heap string returned by this API (also valid to call free()). */
CR_UI_API void cr_ui_string_free(char* s);

/* ---- progress callbacks --------------------------------------------------
 * Fired on the worker thread only. Must not block. NO cancellation
 * semantics - returning is the only contract (cancel via the token
 * functions; the reference keeps the same separation between progress
 * events and its scan-abort event). */

/* stage: 0 quick, 1 deep, 2 carve, 0xFFFFFFFF none/unknown.
 * stage_permille: 0..1000 within the stage. overall_permille: weighted
 * 0..1000 monotonic. bytes_* only meaningful where a stage counts bytes
 * (carve); error_ranges = live read-failure range count so far. */
typedef void (*cr_ui_progress_fn)(void* user, uint32_t stage, uint32_t stage_permille,
                                  uint32_t overall_permille, uint64_t bytes_done,
                                  uint64_t bytes_total, uint32_t error_ranges);

/* Recovery: cumulative bytes done / total across the queue. */
typedef void (*cr_ui_recovery_progress_fn)(void* user, uint64_t bytes_done, uint64_t bytes_total);

/* ---- typed event stream (GAP-S6-10; docs/ABI-CONTRACT.md section 6) ----
 * Corpus lineage: the reference pipes engine events to the UI through a
 * typed message hub (INotifyObserver::Notify -> CMessageProxy ->
 * doDispatchMessage(int, void*, int&) fan-out). This is the equivalent
 * consumer surface: stage transitions, progress samples, files-found
 * counts, error-map deltas, checkpoints, lost-partition candidates -
 * delivered incrementally, no polling.
 *
 * Contract: POD payload (no pointers to free - a transport bridge can
 * serialize it byte-for-byte); events may fire from ENGINE WORKER
 * THREADS or the shim's HEARTBEAT WATCHDOG thread (N-043: the 5 s
 * liveness watchdog fires CR_EVT_HEARTBEAT from its own thread; the
 * shim does NOT serialize event deliveries - single-flight
 * is this callback's own discipline if it needs it); must not block; no
 * cancellation semantics (the token functions are the cancel path);
 * stage transitions fire in scan order; progress samples coalesce by
 * overall permille; files-found/error events are stage-boundary samples
 * (the engines do not publish per-file increments); unknown type values
 * MUST be ignored (forward compatibility). */
#define CR_EVT_SCAN_STAGE 1u     /* stage transition */
#define CR_EVT_SCAN_PROGRESS 2u  /* coalesced progress sample */
#define CR_EVT_FILES_FOUND 3u    /* files-found counters (cumulative) */
#define CR_EVT_ERROR_RANGE 4u    /* read-failure range appended (delta) */
#define CR_EVT_CHECKPOINT 5u     /* a G-13 checkpoint was written */
#define CR_EVT_LOST_CANDIDATE 6u /* lost-partition candidate found */
#define CR_EVT_REPAIR                                                                              \
    7u /* pre-scan repair lifecycle (S-008):                                                       \
        * outcome 0 plan ready for consent,                                                        \
        * 1 applied, 2 consent denied,                                                             \
        * 3 failed/unavailable; repair_actions                                                     \
        * carries the planned/applied count. */
#define CR_EVT_HEARTBEAT 8u /* N-043 liveness heartbeat: fired from the shim's watchdog thread    \
                             * every 5000 ms while run() is in flight and no                    \
                             * CR_EVT_SCAN_PROGRESS arrived in the window (the og's 5 s         \
                             * watchdog, DRWUI_decompiled.c:910565-910585 + CreateThread        \
                             * :910864) - the HONEST variant: the payload carries the           \
                             * standing telemetry truth, never a synthetic permille             \
                             * advance (the og's watchdog bumps its coarse counter +1           \
                             * instead). stage = 0xFFFFFFFF; bytes_total = 0 (unknown);         \
                             * overall_permille / stage_permille = the standing values;         \
                             * timestamp_ms = elapsed_ms; files counters = standing. */
#define CR_EVT_SAVE_PROGRESS 9u /* N-060 save-path progress: fired from the thread running  \
                                 * cr_ui_scan_save_session while the session serializes -  \
                                 * coalesced (first tick, permille steps, terminal);      \
                                 * stage = 0xFFFFFFFF; stage_permille = the save's own    \
                                 * 0..1000; overall_permille = 1000 (standing - the scan  \
                                 * is complete); bytes_done/bytes_total = nodes           \
                                 * serialized / total nodes. Cancellation: the SAVE-     \
                                 * SCOPED flag (cr_ui_scan_save_cancel during the save    \
                                 * aborts it with CR_UI_ERR_CANCELLED and the final path  \
                                 * keeps its previous content - the save is atomic        \
                                 * temp-then-rename; the og's per-child IsCancelled is    \
                                 * the save progress object's own state, NOT the scan     \
                                 * flag - save-on-scan-cancel stays intact). */

typedef struct cr_ui_event {
    uint32_t type;         /* CR_EVT_* above */
    uint32_t stage;        /* 0 quick, 1 deep, 2 carve, 0xFFFFFFFF none */
    uint64_t timestamp_ms; /* run-elapsed, pause-compensated */
    /* progress sample (CR_EVT_SCAN_PROGRESS) */
    uint64_t bytes_done, bytes_total; /* stage-true bytes where counted */
    uint32_t stage_permille, overall_permille;
    /* counters (CR_EVT_FILES_FOUND; cumulative, not deltas) */
    uint64_t files_found, deleted_found, carved_found;
    /* error-map delta (CR_EVT_ERROR_RANGE): the newly-failed span */
    uint64_t err_start_bytes, err_len_bytes;
    uint32_t error_ranges; /* cumulative count after the append */
    /* checkpoint (CR_EVT_CHECKPOINT) */
    uint64_t checkpoint_index;
    /* lost candidate (CR_EVT_LOST_CANDIDATE) */
    uint64_t candidate_lba;
    /* pre-scan repair (CR_EVT_REPAIR, S-008; v1-era append - consumes the
     * reserved slots, layout unchanged; no 1.2 marker ever shipped, see
     * ABI-CONTRACT section 9): planned/applied action count and
     * the outcome code documented at CR_EVT_REPAIR. */
    uint32_t repair_actions;
    uint32_t repair_outcome;
} cr_ui_event;

typedef void (*cr_ui_event_fn)(void* user, const cr_ui_event* evt);

/* Subscribe BEFORE run() (the sink is read at scan start; calling it while
 * run() executes is ERR_BAD_PARAMETER). Pass fn == NULL to unsubscribe.
 * One sink per scan handle. */
CR_UI_API uint32_t cr_ui_scan_set_event_fn(cr_ui_scan* s, cr_ui_event_fn fn, void* user);

/* ---- devices -------------------------------------------------------------
 * A device is a READ view handed to scans, previews and recovery. Sources:
 * a raw image FILE (portable - the ABI-check/test surface), a physical
 * disk or a volume letter (Windows platform layer: RDA-driver first with
 * direct-handle fallback, the reference Device.mo duality).
 * Lifetime: a device must outlive every scan/preview/recovery using it
 * (they borrow, never own); close order is the caller's responsibility. */

CR_UI_API uint32_t cr_ui_open_image(const char* utf8_path, uint64_t offset, uint64_t bytes,
                                    cr_ui_device** out);
/* bytes == 0: from offset to end of file. Exposed as a 512-byte-sector
 * read-only device bounded to [offset, offset+bytes). */

/* Windows only (platform layer): ERR_UNSUPPORTED elsewhere. open_disk
 * prefers the RDA driver transport, falling back to \\.\PhysicalDriveN
 * (the reference duality; the chosen transport is reported by
 * cr_ui_device_transport). open_volume mirrors the CLI: BitLocker state
 * is probed first - a locked volume is refused with CR_UI_ERR_FVE_LOCKED
 * (0x50060001) unless auto-unlock succeeds; the returned device is the
 * volume's slice on its parent disk (kept alive internally). */
CR_UI_API uint32_t cr_ui_open_disk(uint32_t disk, cr_ui_device** out);
CR_UI_API uint32_t cr_ui_open_volume(char letter, cr_ui_device** out);

/* M-033 (additive export - compatible per the ABI policy, no version bump):
 * locked-volume open with a user-supplied FVEK - the in-stack XTS-AES
 * decrypt path the reference implements below its engines (FVE keys ->
 * XTS decrypt; corpus architecture doc section 8 + BitLockerLib artifact).
 * utf8_hex_fvek: 64 hex chars (AES-128-XTS) or 128 hex chars (AES-256-XTS)
 * - the full-volume-encryption-key bytes (key||tweak-key concatenated,
 * the GetFullVolumeEncryptionKey output shape). ERR_BAD_PARAMETER on
 * malformed/weak key material; ERR_UNSUPPORTED off-Windows; the
 * no-key refusal contract of cr_ui_open_volume is unchanged (locked
 * without a key stays CR_UI_ERR_FVE_LOCKED - legitimate unlock only). */
CR_UI_API uint32_t cr_ui_open_volume_with_key(char letter, const char* utf8_hex_fvek,
                                               cr_ui_device** out);

CR_UI_API void cr_ui_device_close(cr_ui_device* dev);

typedef struct cr_ui_geometry {
    uint64_t total_bytes;
    uint32_t logical_sector;
    uint32_t physical_sector;
    uint64_t sector_count;
} cr_ui_geometry;

CR_UI_API uint32_t cr_ui_device_geometry(cr_ui_device* dev, cr_ui_geometry* out);

/* Transport truth (the CLI's "[transport]" lines): "rda", "direct" or
 * "image", NUL-terminated into buf (cap bytes incl. NUL). Two-call: pass
 * buf==NULL to learn the needed size from *needed. */
CR_UI_API uint32_t cr_ui_device_transport(cr_ui_device* dev, char* buf, uint32_t cap,
                                          uint32_t* needed);

/* ---- enumeration (Windows platform layer; ERR_UNSUPPORTED elsewhere) ---- */

typedef struct cr_ui_disk_info {
    uint32_t number;
    uint64_t size_bytes;
    uint32_t logical_sector;
    uint32_t physical_sector;
    uint32_t removable; /* 1 = USB/SD bus */
    uint32_t layout;    /* 0 none, 1 MBR, 2 GPT, 3 EBR (N-057 append - extended-chain tables; 0..2 frozen) */
    uint32_t table_ok;
    char model[128];  /* UTF-8, may be empty */
    char serial[128]; /* UTF-8, may be empty */
} cr_ui_disk_info;

typedef struct cr_ui_volume_info {
    char letter; /* 'C' style; 0 when absent */
    uint32_t disk;
    uint64_t offset_on_disk;
    uint64_t bytes;
    char fs_name[16];   /* "NTFS", ...; may be empty */
    char label[64];     /* UTF-8, may be empty */
    uint32_t bitlocker; /* 1 = FVE detected */
} cr_ui_volume_info;

/* Two-call pattern for both: *count as IN capacity when out != NULL, as
 * OUT needed when out == NULL. Enumeration failure returns its error code
 * (never a silent empty list - CLI G-20 parity). */
CR_UI_API uint32_t cr_ui_enumerate_disks(cr_ui_disk_info* out, uint32_t* count);
CR_UI_API uint32_t cr_ui_enumerate_volumes(cr_ui_volume_info* out, uint32_t* count);

/* ---- scan lifecycle ------------------------------------------------------
 * One cr_ui_scan = one staged pipeline over one attached device view,
 * mirroring the CLI `scan` flow exactly (identity card, LDM cross-disk
 * resolver, checkpoint sink, save-on-demand). run() is synchronous on the
 * CALLER's thread (call it from a UI worker thread, as the reference UI
 * calls its controllers); pause/cancel/telemetry are safe from any thread
 * while it runs. Cancellation makes run() return CR_UI_ERR_CANCELLED with
 * the tree intact - saving the session afterwards is the caller's explicit
 * choice (cr_ui_scan_save_session), matching the CLI's --out policy. */

/* fs_hint values (orchestrator::FsFamily, serialized values): 0 auto-probe,
 * 1 NTFS, 2 FAT, 3 exFAT, 4 ext4, 6 HFS+, 7 Btrfs, 8 ZFS, 9 ReFS, 11 UDF
 * (M-044; classification + carve-only - no structured engine exists on
 * either side) (5 reserved on the wire - never produced; 10 BitLocker is
 * probe-only, never a valid hint). */
/* ---- pre-scan repair consent (S-008; v1-era append - no 1.2 marker ----
 * ever shipped, ABI-CONTRACT section 9) -----------------------------------
 * Corpus lineage: the reference runs RawFixer PRE-SCAN with a UI consent
 * prompt (rawfixer-fs-repair.md: invoked during scan preparation; the
 * DRWUI "Check" phase classes pair with the fixers). This is the
 * equivalent surface: when cr_ui_scan_config.pre_scan_repair is 1, the
 * shim runs the read-only repair analysis before the stages and calls
 * repair_consent ONCE (on the run thread, before any engine read) with
 * a POD summary whenever a repairable plan exists. Nonzero return =
 * execute the transactional repair through the shim-opened write view
 * (image files: the same file reopened read-write at the image offset;
 * Windows disks and volumes: the platform RW transport mapped through
 * the device's slice). Zero, a NULL callback, or no available write view
 * = the plan is surfaced via CR_EVT_REPAIR events and stats and NEVER
 * executed (default-deny). The undo log is retrievable after run() via
 * cr_ui_scan_repair_save_undo. */
typedef struct cr_ui_repair_plan {
    uint32_t detected;       /* family identified at all */
    char fs_family[8];       /* "ntfs"/"fat"/"exfat", or "" when unknown */
    uint32_t action_count;   /* planned write actions (0 = clean) */
    uint32_t refused;        /* 1 = analysis refused (no safe repair) */
    char reason[160];        /* refusal reason, or the first action's
                              * reason; NUL-truncated */
    uint32_t advisory_count; /* report-only findings (see stats/events) */
} cr_ui_repair_plan;

typedef uint32_t (*cr_ui_repair_consent_fn)(void* user, const cr_ui_repair_plan* plan);

typedef struct cr_ui_scan_config {
    uint32_t stage_mask;   /* 0 = default (quick|deep|carve = 7); else bits
                            * 1 quick, 2 deep, 4 carve */
    uint32_t whole_disk;   /* 1 = lost-partition discovery (needs a disk view) */
    uint32_t fs_hint;      /* 0 = auto-probe */
    uint32_t quick_weight; /* 0 = defaults 40/35/25 (percent-of-total) */
    uint32_t deep_weight;
    uint32_t carve_weight;
    uint32_t threads;              /* 0 = auto */
    const char* volume_label_utf8; /* optional; NULL/"" = none; truncated
                                    * to 31 chars (session card field) */
    const char* volume_model_utf8; /* optional identity-card overrides; when
                                    * absent the shim fills them from the
                                    * disk enumeration (Windows, CLI parity) */
    const char* volume_serial_utf8;
    const char* checkpoint_path_utf8; /* optional: stage-boundary auto-
                                       * checkpoints written to this file
                                       * (overwrite-in-place, CLI --out) */
    cr_ui_progress_fn progress;       /* optional live progress */
    void* progress_user;
    uint32_t pre_scan_repair;               /* v1-era append (S-008; no 1.2
                                             * marker ever shipped - section 9):
                                             * 1 = analyze + consent gate
                                             * before the stages */
    cr_ui_repair_consent_fn repair_consent; /* NULL = never execute */
    void* repair_consent_user;
    uint32_t memory_warn_mb;                /* v2-era append (N-062): the
                                             * memory monitor's threshold in
                                             * MiB; 0 = OFF (the conservative
                                             * default). While run() is in
                                             * flight the shim's watchdog
                                             * samples the process working
                                             * set every 5 s and emits a
                                             * Warn line through the cr::log
                                             * seam at/above the threshold.
                                             * Zero-init safe (0 = off). */
} cr_ui_scan_config;

CR_UI_API uint32_t cr_ui_scan_create(const cr_ui_scan_config* cfg, cr_ui_scan** out);
CR_UI_API void cr_ui_scan_close(cr_ui_scan* s);

/* The device view to scan (borrowed; must outlive the scan). Attach before
 * run(). On Windows the identity card is auto-filled from the disk
 * enumeration here unless the config supplied one (CLI parity). */
CR_UI_API uint32_t cr_ui_scan_attach_device(cr_ui_scan* s, cr_ui_device* dev);

/* Runs the pipeline to completion on the calling thread. Returns 0, an
 * engine error, or CR_UI_ERR_CANCELLED (tree stays browsable - take a
 * snapshot or save the session). */
CR_UI_API uint32_t cr_ui_scan_run(cr_ui_scan* s);

/* Token control (any thread; the reference's pause/abort-event split). */
CR_UI_API uint32_t cr_ui_scan_pause(cr_ui_scan* s);  /* blocks engines cooperatively */
CR_UI_API uint32_t cr_ui_scan_resume(cr_ui_scan* s); /* releases a pause */
CR_UI_API uint32_t cr_ui_scan_cancel(cr_ui_scan* s); /* run() -> ERR_CANCELLED */

/* N-075 (v2-era append): synchronous state queries — the og's
 * reference-out query signals (sigIsBusy(bool&) x14, sigIsStopping(bool&)
 * x8 -> isBusy(bool&) relays — corpus UIUX/25-connect-wiring-graph.md
 * :125-144) surfaced as ABI ops a host can call at any time. busy: run()
 * is executing on some thread (the LIVE execution state; telemetry's
 * running field mirrors it through the snapshot). stopping: cr_ui_scan_
 * cancel was called and the run is winding down — the live cancel
 * REQUEST on the token, distinct from cr_ui_telemetry's cancelled flag
 * (the post-run state). Synchronous by construction (the stronger
 * contract vs the og's Qt-connection-dependent answer edge). Any thread,
 * any time; *out is 0 or 1. */
CR_UI_API uint32_t cr_ui_scan_busy(cr_ui_scan* s, uint32_t* out);
CR_UI_API uint32_t cr_ui_scan_stopping(cr_ui_scan* s, uint32_t* out);

typedef struct cr_ui_stage_tel {
    uint32_t permille;    /* 0..1000 within the stage, monotonic */
    uint64_t units_done;  /* engine work units (records/dentries/...) */
    uint64_t units_total; /* 0 = unknown */
    uint64_t bytes_done;  /* bytes where the stage counts them (carve) */
    uint64_t elapsed_ms;  /* pause-compensated */
    uint32_t completed;
} cr_ui_stage_tel;

typedef struct cr_ui_telemetry {
    uint32_t overall_permille; /* weighted, monotonic */
    cr_ui_stage_tel quick, deep, carve;
    uint64_t files_found, deleted_found, carved_found, lost_partitions;
    uint64_t bytes_scanned; /* carve-true-bytes (honest fill-rate) */
    uint64_t elapsed_ms;    /* pause-compensated run time */
    uint32_t error_ranges;  /* live read-failure count */
    uint32_t paused, running, cancelled;
} cr_ui_telemetry;

/* Live snapshot of the running (or finished) scan; any thread, any time. */
CR_UI_API uint32_t cr_ui_scan_telemetry(cr_ui_scan* s, cr_ui_telemetry* out);

typedef struct cr_ui_scan_stats {
    uint64_t files_found, deleted_found, carved_found, lost_partitions;
    uint64_t bytes_scanned, elapsed_ms;
    uint64_t cached_hits, cached_misses;
    uint32_t error_ranges; /* final count (full detail via the session) */
    /* v1-era append (S-008 pre-scan repair outcome; no 1.2 marker ever
     * shipped - ABI-CONTRACT section 9): */
    uint32_t repair_ran;     /* analysis executed at all */
    uint32_t repair_planned; /* surfaced action count */
    uint32_t repair_applied; /* transactionally written (all-or-rolled-back) */
    uint32_t repair_denied;  /* consent denied or unavailable (nothing written) */
} cr_ui_scan_stats;

CR_UI_API uint32_t cr_ui_scan_get_stats(cr_ui_scan* s, cr_ui_scan_stats* out);

/* ---- S-023: post-scan grading (the og's CRecoverability model) -------
 * The grading pass runs at scan completion (all stages done); grades are
 * stored on the tree nodes (monotonic, grade-2 sticky), persist with the
 * session, and are re-derivable per selection. The statistics mirror the
 * og's RecoveryAssistant block. */

typedef struct cr_ui_assistant_stats {
    uint64_t total_file_count;
    uint64_t raw_count;
    uint64_t lost_partition_exist_file_count;
    uint64_t lost_partition_delete_file_count;
    uint64_t quick_scan_exist_file_count;
    uint64_t quick_scan_delete_file_count;
    uint64_t deep_scan_file_count;
    uint64_t quick_scan_delete_extend;
    uint64_t category_counts[8]; /* image video audio document archive
                                  * other email bookmarks */
    uint64_t matched, total;     /* the recoverability gauge; when total < 100
                                  * the og forces matched to 0 (small-result
                                  * heuristic - the gauge never shows) */
    uint32_t small_result;
    uint64_t graded_count;
} cr_ui_assistant_stats;

/* The scan-completion grading statistics. Valid after run() returns OK
 * (or after resume of a completed session; the counters re-derive from
 * the loaded tree). */
CR_UI_API uint32_t cr_ui_scan_assistant(cr_ui_scan* s, cr_ui_assistant_stats* out);

/* Per-selection re-grade (the og's FUN_1401d7110 semantics): re-run the
 * grading rules on the given session-tree nodes. Only grade-1 (average)
 * nodes are eligible - grade 0 and 2 verdicts are final (sticky).
 * Returns OK with *regarded set to the count actually re-graded. */
CR_UI_API uint32_t cr_ui_session_regrade(cr_ui_session* sess, const uint64_t* node_ids,
                                         uint32_t count, uint32_t* regraded);
CR_UI_API uint32_t cr_ui_scan_checkpoint_count(cr_ui_scan* s, uint64_t* out);

/* S-008 (v1-era append - no 1.2 marker ever shipped, section 9): persist
 * the pre-scan repair's undo log to a
 * file (the durable CRC-checked container). Valid after run() when the
 * repair executed; returns 0 on write. Returns CR_UI_ERR_BAD_PARAMETER
 * when nothing was applied (no undo log exists) or the scan has not
 * run. The operator restores with the CLI's
 * `rrecov fix --disk N --undo-apply <file>`. */
CR_UI_API uint32_t cr_ui_scan_repair_save_undo(cr_ui_scan* s, const char* utf8_path);

/* ---- S-029: lost-partition APPLY (consent-gated; ABI-since 1.3) --------
 * Corpus lineage: the og's RecoveryModule exposes the apply as its own
 * scenario object (CreateRecoveryPartitionObject vs the scan's
 * CreateScanLostPartitionObject); PartitionRecovery.dll's rebuild path
 * (FUN_1800566d0/FUN_180038b10/FUN_1800392f0) executes the IOCTL chain
 * quoted in cr_apply.h. This is the equivalent surface: plan (read-only)
 * -> consent callback -> transactional write -> undo. Default-deny: a
 * NULL consent callback or a zero return writes NOTHING. */
typedef struct cr_ui_apply_candidate {
    uint64_t start_lba; /* absolute LBA on the disk */
    uint64_t sector_count;
    uint32_t fs; /* lost::CandidateFs serialized values: 0 NTFS, 1 FAT,
                  * 2 exFAT, 4 GPT-table (refused by the planner), 5
                  * unknown (refused), 6 ext2, 7 HFS+, 8 Btrfs, 9 ZFS,
                  * 10 ReFS, 11 BitLocker */
} cr_ui_apply_candidate;

typedef struct cr_ui_apply_plan {
    uint32_t refused; /* 1 = planner refused; see reason */
    uint32_t scope;   /* 0 single-entry, 1 rebuild-table */
    uint64_t start_lba, sector_count;
    uint32_t mbr_type;             /* the entry type byte to write */
    uint32_t slot;                 /* target MBR slot 0..3 */
    uint32_t regenerate_signature; /* 1 = the rebuild stamps a fresh 32-bit
                                    * disk signature (corpus GPT variant) */
    uint32_t signature;            /* effective signature (LE value) */
    char reason[160];              /* refusal reason, NUL-truncated */
} cr_ui_apply_plan;

typedef uint32_t (*cr_ui_apply_consent_fn)(void* user, const cr_ui_apply_plan* plan);

typedef struct cr_ui_apply_result {
    uint32_t applied;      /* 1 = entry written + verified + refreshed */
    uint32_t outcome;      /* 0 applied; 1 consent denied; 2 refused; 3
                            * table changed since plan; 4 applied but the
                            * refresh tail failed (undo valid); 5 error
                            * (see the return code) */
    uint64_t undo_entries; /* undo log entries (0 = nothing written) */
    char reason[160];      /* refusal / failure detail, NUL-truncated */
} cr_ui_apply_result;

/* Plans + consents + executes one lost-partition apply on a WHOLE-DISK
 * view (an image opened at offset 0, or a Windows disk device; volume
 * slices are refused - the table lives at the disk's LBA 0). The consent
 * callback fires exactly once, with the full POD plan, before any write;
 * zero/NULL = denied, nothing written (outcome 1). The undo log of the
 * last apply on this device is retained: cr_ui_apply_undo_last restores
 * it, cr_ui_apply_save_undo persists it. */
CR_UI_API uint32_t cr_ui_apply_lost_partition(cr_ui_device* dev, const cr_ui_apply_candidate* cand,
                                              uint32_t rebuild_table,
                                              cr_ui_apply_consent_fn consent, void* consent_user,
                                              cr_ui_apply_result* out);

/* Restores the last apply's undo log on this device (byte-exact sector
 * restore + properties refresh). Idempotent on bytes. */
CR_UI_API uint32_t cr_ui_apply_undo_last(cr_ui_device* dev, cr_ui_apply_result* out);

/* S-029 (ABI-since 1.3): persist the last apply's undo log to a file
 * (the durable CRC-checked container, loadable by `rrecov fix
 * --undo-apply <file>`). CR_UI_ERR_BAD_PARAMETER when no undo log exists. */
CR_UI_API uint32_t cr_ui_apply_save_undo(cr_ui_device* dev, const char* utf8_path);

/* Writes the full truthful session (.crsf) - the same record set the CLI
 * emits, loadable by cr_ui_session_load / `rrecov tree`. Valid after run()
 * (including a cancelled one) or after a session-resumed run. */
CR_UI_API uint32_t cr_ui_scan_save_session(cr_ui_scan* s, const char* utf8_path);

/* N-060: cancel an in-flight cr_ui_scan_save_session (the og's per-child
 * IUIProgressInfo::IsCancelled teardown - rsf-format--scan-session.md
 * flow section). SAVE-SCOPED: never touches the scan's cancel token, so
 * the save-on-scan-cancel flow (persisting a partial session after a
 * cancelled scan) is unaffected. The aborted save returns
 * CR_UI_ERR_CANCELLED; the final path keeps its previous content and no
 * staging .tmp remains. A save not in flight is a no-op. Safe from any
 * thread. */
CR_UI_API uint32_t cr_ui_scan_save_cancel(cr_ui_scan* scan);

/* Immutable value snapshot of the result tree (G-02). Callable from any
 * thread WHILE run() executes (partial=1 is stamped - mid-scan truth) and
 * after it returns (partial=0). The UI refreshes at stage boundaries or on
 * demand, exactly the model the reference UI uses for its live tree. */
CR_UI_API uint32_t cr_ui_scan_snapshot(cr_ui_scan* s, cr_ui_snapshot** out);

/* Volume-identity check for RESUMED scans (the CLI's boot-hash warning):
 * compares the attached device's first 512 bytes against the session's
 * stored identity hash. Returns CR_UI_ERR_BAD_CHECKSUM on mismatch (the
 * session came from a different volume), 0 on match or unverifiable
 * (legacy session with no hash). Call after cr_ui_session_resume +
 * attach, before run(). */
CR_UI_API uint32_t cr_ui_scan_verify_identity(cr_ui_scan* s);

/* ---- sessions ------------------------------------------------------------
 * A loaded .crsf: browsable tree + stats + volume info. The tree is
 * STABLE (no engine running against it), so queries, previews and
 * recovery enqueue directly - this is the post-scan surface the results
 * screen is built on. */

CR_UI_API uint32_t cr_ui_session_load(const char* utf8_path, cr_ui_session** out);
CR_UI_API void cr_ui_session_close(cr_ui_session* sess);

typedef struct cr_ui_session_info {
    uint32_t fs_family;       /* orchestrator::FsFamily value (D-1 truth) */
    uint32_t stage_done_mask; /* actual completion (quick|deep|carve bits) */
    uint32_t configured_mask; /* stages the original run configured */
    uint64_t slice_offset, slice_bytes;
    uint32_t logical_sector, physical_sector;
    char label[32]; /* UTF-8 */
    uint64_t scanned_bytes, elapsed_ms;
    char model[48]; /* UTF-8 identity card; empty on v1 sessions */
    char serial[32];
    uint32_t boot_hash; /* 0 = unverifiable (v1 session) */
} cr_ui_session_info;

CR_UI_API uint32_t cr_ui_session_get_info(cr_ui_session* sess, cr_ui_session_info* out);
CR_UI_API uint32_t cr_ui_session_stats(cr_ui_session* sess, cr_ui_scan_stats* out);
CR_UI_API uint32_t cr_ui_session_snapshot(cr_ui_session* sess, cr_ui_snapshot** out);

/* Resume flow (the CLI's `scan --resume`): loads the session into a FRESH
 * tree, validates the completion mask, and returns a scan handle whose
 * controller continues where the session stopped. Attach the ORIGINAL
 * target device, optionally cr_ui_scan_verify_identity, then run().
 * original_cfg mirrors the config of the interrupted run. */
CR_UI_API uint32_t cr_ui_session_resume(cr_ui_session* sess, const cr_ui_scan_config* original_cfg,
                                        cr_ui_scan** out);

/* ---- snapshot enumeration -------------------------------------------------
 * Flat, index-addressed rows (parent/child/sibling links as indices;
 * CR_UI_SNAP_NONE when absent). This is the mid-scan browse surface; the
 * UI filters client-side (reference parity: the og has no core-side
 * query on the live scan - its results screens filter in the UI). */

#define CR_UI_SNAP_NONE 0xFFFFFFFFu

typedef struct cr_ui_node_info {
    uint32_t parent, first_child, next_sibling, prev_sibling; /* indices */
    uint64_t node_id;    /* stable id; recovery/preview addressing */
    uint32_t fs_kind;    /* FsKindCode value (cr_types.h) */
    uint64_t record_ref; /* NTFS record (hard-link grouping); 0 carved */
    uint32_t origin;     /* tree::Origin: 0 structured, 1 carved, 2 lost, 3 virtual */
    uint32_t deleted;    /* tree::DeletedState: 0 active, 1 deleted, 2 partial, 3 damaged */
    uint32_t confidence; /* tree::Confidence ordinal 0..4 */
    uint32_t grade;      /* tree::Grade: 0 low, 1 average, 2 high (S-023;
                          * the og's CRecoverability verdict, computed by
                          * the post-scan grading pass) */
    uint32_t overlap;    /* 1 when the twin/overlap rule flagged this node
                          * (og bit 2 of node+0x73) */
    uint32_t is_full;    /* roots/virtuals carry role + stats */
    uint64_t logical_size, allocated_size;
    uint64_t created, modified, changed, accessed; /* 100ns since 1601 */
    uint32_t attributes;                           /* DOS attributes (0x10 = directory) */
    uint32_t flags;
    uint32_t extent_count; /* info only - the writer resolves extents */
    uint32_t resident_bytes;
    uint32_t carve_format, carve_confidence; /* carved provenance */
    uint32_t hash_valid;
    uint8_t sha128[16]; /* truncated content hash when hash_valid */
    uint32_t name_len;  /* name bytes excluding NUL (UTF-8) */
} cr_ui_node_info;

CR_UI_API void cr_ui_snapshot_close(cr_ui_snapshot* snap);
CR_UI_API uint32_t cr_ui_snapshot_node_count(cr_ui_snapshot* snap, uint32_t* out);
CR_UI_API uint32_t cr_ui_snapshot_partial(cr_ui_snapshot* snap, uint32_t* out);

/* Row `index` into *out. name_buf: NULL -> *out->name_len = needed size;
 * non-NULL -> copies min(cap-1, name_len) bytes + NUL and sets name_len to
 * the FULL length (compare with cap-1 to detect truncation; a capped copy
 * still returns 0 when everything fit, CR_UI_ERR_TRUNCATED when not). */
CR_UI_API uint32_t cr_ui_snapshot_node(cr_ui_snapshot* snap, uint32_t index, cr_ui_node_info* out,
                                       char* name_buf, uint32_t name_cap);

/* Depth-first emission order (byte-parity with the live serializer).
 * Two-call: indices==NULL -> *count = needed; else *count as IN capacity,
 * OUT filled count; capacity < needed -> CR_UI_ERR_TRUNCATED. */
CR_UI_API uint32_t cr_ui_snapshot_dfs(cr_ui_snapshot* snap, uint32_t* indices, uint32_t* count);

/* ---- query (session trees) ------------------------------------------------
 * Core-side filter/sort/paging over a loaded session (G-12) - the
 * improvement beyond the reference (whose filtering is client-side).
 * Works on the STABLE session tree; mid-scan filtering is snapshot
 * enumeration + client-side predicates (see above). */

/* sort: 0 none (emission order), 1 name, 2 size, 3 modified, 4 confidence,
 * 5 path, 6 grade (S-023: the CRecoverability verdict; desc = High first,
 * the og's results comparator order), 7 type (M-029: the og's results-view
 * Type column - category in the FileFilter.xml iconIndex order, then
 * extension, then name). dir: 0 asc, 1 desc. category bits:
 * 1 image, 2 video, 3 audio, 4 document, 5 archive, 6 other, 7 email,
 * 8 bookmarks. Zero-initialized struct = match-all.
 * Paging: offset+limit is a HOST-side model (default limit 100) - the og
 * has NO page-size constant (its results view virtualizes at the UI
 * layer, ~1000-item incremental batches, sigResetStartIdx); the core
 * claims no paging parity against it (M-029's registered deliberate
 * difference). */
typedef struct cr_ui_query {
    uint32_t category_mask;              /* 0 = all categories */
    uint64_t size_min, size_max;         /* 0 = unbounded */
    uint64_t modified_from, modified_to; /* 0 = unbounded; 100ns-1601 units */
    uint32_t confidence_min;             /* ordinal floor 0..4 */
    const char* const* extensions;       /* lowercased WITH dot; NULL = none */
    uint32_t extension_count;
    const char* name_substring; /* UTF-8, ASCII-case-insensitive */
    const char* path_prefix;    /* ABI-since core-api: full-path prefix filter
                                 * (og AdvanceFilterCondition path group),
                                 * ASCII-case-insensitive; NULL = no filter */
    uint32_t deleted_only;
    uint32_t origin_mask; /* 0 = all origins */
    uint32_t sort, dir;
    uint64_t offset;
    uint32_t limit; /* 0 = default 100 */
    uint32_t count_only;
} cr_ui_query;

typedef struct cr_ui_query_row {
    uint64_t node_id; /* feeds preview/recovery addressing */
    char* name;       /* UTF-8 heap (cr_ui_query_rows_free) */
    char* path;       /* UTF-8 heap, '/'-joined */
    uint64_t size, modified;
    uint32_t origin, deleted, confidence, category;
    uint32_t grade; /* S-023: tree::Grade 0..2 (rows carry the verdict so
                     * parity UIs can sort grade-2 first like the og) */
} cr_ui_query_row;

CR_UI_API uint32_t cr_ui_query_run(cr_ui_session* sess, const cr_ui_query* q,
                                   cr_ui_query_row** rows, uint32_t* row_count,
                                   uint64_t* total_matched);
CR_UI_API void cr_ui_query_rows_free(cr_ui_query_row* rows, uint32_t row_count);

/* ---- carve-format registry (ops #7-#8 parity) ------------------------------
 * The og exposes its analyser-registry views through RAWSearchDLL exports
 * (EUGetAnalyserList / EUGetAnalyserMap / EUGetAnalyserMultimap) and the
 * per-extension preference map (GetRawUserInfoMap) so its UI can build
 * format-config surfaces (corpus rawsearch-carving--deep-dive.md section 1).
 * Our equivalent: read-only enumeration of every format the carve stages
 * can surface - BOTH layers (the grammar-validator registry and the
 * raw-sign prefilter table). No handle: safe from any thread; the tables
 * are immutable for the library's lifetime. */

typedef struct cr_ui_carve_format {
    uint32_t layer;           /* 0 grammar validator, 1 raw-sign prefilter */
    char ext[24];             /* lowercase, no dot ("jpg"); NUL-padded */
    uint64_t min_size;        /* bytes (raw-sign layer: KiB-converted once, S-015) */
    uint64_t max_size;        /* bytes; 0 = unbounded (grammar layer) */
    uint16_t base_confidence; /* 0..1000 before validator deltas */
    uint32_t magic_len;       /* 0..16; grammar layer only */
    uint32_t magic_offset;    /* magic position within the file start */
    uint8_t magic[16];        /* header magic, grammar layer only */
} cr_ui_carve_format;

/* Total enumerable formats: grammar registry rows first (FormatId order),
 * then raw-sign format rows (kFormats order). Deterministic, stable order
 * across releases (both tables only ever APPEND). */
CR_UI_API uint32_t cr_ui_carve_format_count(void);

/* Row `index` [0, count()) into *out. BAD_PARAMETER on out-of-range. */
CR_UI_API uint32_t cr_ui_carve_format_info(uint32_t index, cr_ui_carve_format* out);

/* ---- disk map (GAP-S6-11; docs/ABI-CONTRACT.md section 7) ------------------
 * The visualization DATA PRODUCER (corpus lineage: AnalysisGridWidget
 * sector grid + the engines' IBuildVolumeBitmap interface): geometry +
 * per-cell overlays a front-end paints. Exactly ONE of sess/snap.
 *
 * want (optional for sessions, REQUIRED for snapshots): caller geometry
 * hints - start_byte/total_bytes select the mapped view (snapshots are
 * value objects and carry no view; sessions default to the scanned
 * slice), cell_bytes = 0 picks the engine cluster size or the 64 KiB
 * default. got always reports the geometry ACTUALLY used.
 *
 * Overlays (each optional, at least one required): heap arrays of
 * got->cell_count bytes, freed with cr_ui_disk_map_free:
 *   allocation: 0 unknown, 1 used (active structured files - the only
 *               structured claim a session proves), 2 free (S-009: the
 *               engine allocation bitmap's truth - NTFS $Bitmap /
 *               FAT / exFAT table; only where no extent and no error
 *               span claims the cell), 3 bad (retry-layer error spans)
 *   coverage:   0 unscanned, 1 scanned (carve completion = all cells)
 *   found:      distinct found files covering the cell (capped 255)
 *   confidence: max Confidence ordinal 0..4 of covering content
 * bad_ranges: caller buffer of u64 PAIRS [start,len] (absolute bytes),
 * bad_cap the pair capacity; *bad_count returns the needed pair count
 * either way; bad_cap < needed -> CR_UI_ERR_TRUNCATED after filling.
 *
 * S-009 free overlay: the session path feeds the persisted REC_ALLOC_MAP
 * (downsampled to the map's own cell granularity, budget-bounded); the
 * snapshot path feeds the value-captured map (session snapshots) or the
 * completed scan's map (scan snapshots, !running captures only - a live
 * mid-scan map would be a racy read and stays absent). Sessions saved
 * before the record existed degrade to unknown cells, never a guess. */

typedef struct cr_ui_disk_map_geometry {
    uint64_t start_byte;    /* in: view base (snapshot path); out: used base */
    uint64_t total_bytes;   /* in: view extent; out: used extent */
    uint64_t cell_bytes;    /* in: 0 = auto; out: the granularity used */
    uint64_t cell_count;    /* out: the overlay array lengths */
    uint32_t cluster_bytes; /* out: 0 when no cluster model was known */
    uint32_t fs_family;     /* out: orchestrator::FsFamily value */
} cr_ui_disk_map_geometry;

CR_UI_API uint32_t cr_ui_disk_map(cr_ui_session* sess, cr_ui_snapshot* snap,
                                  const cr_ui_disk_map_geometry* want, cr_ui_disk_map_geometry* got,
                                  uint8_t** allocation, uint8_t** coverage, uint8_t** found,
                                  uint8_t** confidence, uint64_t* bad_ranges, uint32_t bad_cap,
                                  uint32_t* bad_count);

/* Frees an overlay array returned by cr_ui_disk_map (each array
 * separately; NULL is a no-op). */
CR_UI_API void cr_ui_disk_map_free(uint8_t* overlay);

/* ---- preview ---------------------------------------------------------------
 * Bounded, chunked reads of one node's logical content with NO recovery
 * pass (G-10 - the reference's IInWindowStream preview contract: range
 * reads under explicit caps). The reader owns a private copy of the
 * node's properties, so the source session/snapshot may be closed while
 * a preview lives; the DEVICE must outlive it (it supplies the bytes). */

/* node addressing: snapshot row index, or session node_id. */
CR_UI_API uint32_t cr_ui_preview_open_snapshot(cr_ui_device* dev, cr_ui_snapshot* snap,
                                               uint32_t index, cr_ui_preview** out);
CR_UI_API uint32_t cr_ui_preview_open_session(cr_ui_device* dev, cr_ui_session* sess,
                                              uint64_t node_id, cr_ui_preview** out);
CR_UI_API void cr_ui_preview_close(cr_ui_preview* pv);

CR_UI_API uint32_t cr_ui_preview_size(cr_ui_preview* pv, uint64_t* out);

/* Reads [offset, offset+len) logical bytes into buf. *placed = bytes
 * actually assembled (short only at logical EOF; len is capped at 4 MiB
 * per call - the preview window). Unreadable device spans are zero-filled
 * AND reported (see below) - never silently swallowed. */
CR_UI_API uint32_t cr_ui_preview_read(cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t len,
                                      uint32_t* placed);

/* Bounded metadata extraction. Fixed-capacity UTF-8 fields (truncation is
 * reported via `truncated`, never silent). kind: image|video|audio|
 * document|archive|text|unsupported. */
typedef struct cr_ui_metadata {
    char kind[24];
    char format[24];
    uint32_t width, height;
    char orientation[8]; /* EXIF "1".."8"; "" absent */
    char datetime[40];   /* EXIF DateTimeOriginal */
    uint64_t duration_ms;
    char codec[32];
    char text_snippet[256];
    uint32_t truncated; /* extraction hit a cap (re-run is allowed) */
} cr_ui_metadata;

CR_UI_API uint32_t cr_ui_preview_metadata(cr_ui_preview* pv, cr_ui_metadata* out);

/* Truthful failure map: device spans that could not be read and were
 * zero-filled. *total_bytes = sum of span lengths. */
CR_UI_API uint32_t cr_ui_preview_unreadable(cr_ui_preview* pv, uint64_t* total_bytes,
                                            uint32_t* range_count);

/* ---- recovery ----------------------------------------------------------------
 * The CLI `recover` flow in-process: SafeWriter over an injected DestIo
 * (Win32 atomic temp+rename on Windows; a portable stdio equivalent on
 * other hosts for the ABI-check surface), destination-overlap refusal
 * (F12), per-item evidence and the machine-readable JSON report (G-09).
 * The writer runs on the CALLER's thread in process(); cancel() is safe
 * from any thread. */

typedef struct cr_ui_recovery_config {
    uint64_t read_chunk;           /* 0 = 1 MiB */
    uint32_t rename_attempts;      /* 0 = 64 */
    uint32_t skip_hash_verify;     /* 0 = verify ON (default) */
    uint32_t skip_dir_reconstruct; /* 0 = reconstruct ON (default, G-08) */
    uint32_t conflict;             /* 0 rename, 1 skip, 2 overwrite */
    cr_ui_recovery_progress_fn progress;
    void* progress_user;
} cr_ui_recovery_config;

CR_UI_API uint32_t cr_ui_recovery_create(cr_ui_device* source, const cr_ui_recovery_config* cfg,
                                         cr_ui_recovery** out);
CR_UI_API void cr_ui_recovery_close(cr_ui_recovery* rec);

/* Destination root (UTF-8). On Windows a drive-letter destination also
 * arms the overlap refusal against disk-backed sources (the CLI's [F12]
 * gate); image-file sources have file-internal extents, which cannot
 * overlap a volume (documented truth). Normalized with a trailing
 * separator internally. */
CR_UI_API uint32_t cr_ui_recovery_set_destination(cr_ui_recovery* rec, const char* utf8_path);

/* Enqueue by session node_id: one node (a file, or a directory as a
 * structure item), or a whole subtree (the CLI's recursive walk: every
 * directory on the path becomes a mkdir item so empty leaves materialize,
 * then every file). Returns per-call errors (e.g. 0x50050001
 * ERR_DEST_OVERLAP - count refusals by return code). */
CR_UI_API uint32_t cr_ui_recovery_enqueue_node(cr_ui_recovery* rec, cr_ui_session* sess,
                                               uint64_t node_id);
CR_UI_API uint32_t cr_ui_recovery_enqueue_subtree(cr_ui_recovery* rec, cr_ui_session* sess,
                                                  uint64_t node_id);
/* Snapshot-based enqueue (G-02b value path - mid-scan recovery without
 * touching the live tree; race-free by construction). */
CR_UI_API uint32_t cr_ui_recovery_enqueue_snapshot(cr_ui_recovery* rec, cr_ui_snapshot* snap,
                                                   uint32_t index);
CR_UI_API uint32_t cr_ui_recovery_enqueue_subtree_snapshot(cr_ui_recovery* rec,
                                                           cr_ui_snapshot* snap, uint32_t index);

CR_UI_API uint32_t cr_ui_recovery_queue_count(cr_ui_recovery* rec, uint32_t* out);

/* Processes the queue to completion on the calling thread (read source
 * extents -> temp -> verify -> atomic commit). CR_UI_ERR_CANCELLED on
 * cancel() - already-committed items stay committed; the report tells the
 * truth about the rest (pending items). */
CR_UI_API uint32_t cr_ui_recovery_process(cr_ui_recovery* rec);
CR_UI_API uint32_t cr_ui_recovery_cancel(cr_ui_recovery* rec);

typedef struct cr_ui_recovery_stats {
    uint64_t queued, done, refused, errors, hash_mismatch, resumed, salvaged, dirs;
    uint64_t nothing_to_recover; /* GAP-S6-7: metadata-only items skipped before
                                  * any write (no file created; v1-era append,
                                  * no 1.1 marker ever shipped - section 9;
                                  * appended field - zero-init safe) */
} cr_ui_recovery_stats;

CR_UI_API uint32_t cr_ui_recovery_get_stats(cr_ui_recovery* rec, cr_ui_recovery_stats* out);

/* The G-09 JSON report (schema: docs/recovery-report-schema.md) as a heap
 * string (cr_ui_string_free). Valid any time - unprocessed items report
 * "pending" truthfully. source_utf8 identifies the source in the report. */
CR_UI_API uint32_t cr_ui_recovery_report_json(cr_ui_recovery* rec, const char* source_utf8,
                                              char** json_out);

/* ---- health (Windows platform layer; ERR_UNSUPPORTED elsewhere) -------------
 * The CLI `health` flow: SMART via ATA passthrough, NVMe health log,
 * SSD heuristic and TRIM state, one flat struct. Read-only. */

typedef struct cr_ui_smart_attr {
    uint32_t id;
    uint32_t value, worst;
    uint64_t raw; /* 6 vendor bytes */
    uint32_t failing;
    char name[40];
} cr_ui_smart_attr;

typedef struct cr_ui_smart_report {
    uint32_t available; /* SMART table decoded */
    char transport[24]; /* "ata-passthrough" / "nvme" / "legacy" */
    uint32_t ata_version, smart_version;
    uint32_t attribute_count; /* needed size for attrs */
    uint32_t failing_now;
    uint64_t reallocated_sectors, pending_sectors, uncorrectable;
    uint64_t power_on_hours, spin_retry, ssd_wear_level, temperature_c;
    /* NVMe health log */
    uint32_t nvme_available;
    uint32_t temperature_c_x10; /* Kelvin -> Celsius, tenths */
    uint32_t available_spare, available_spare_threshold;
    uint64_t percentage_used;
    uint64_t data_units_read, data_units_written, power_on_hours_nvme;
    uint64_t unsafe_shutdowns, media_errors;
    uint32_t critical_warning;
    /* media class + TRIM */
    uint32_t ssd_like; /* seek-penalty heuristic */
    uint32_t trim_known, trim_enabled;
    /* ABI v2 (M-036): the og DiskHealth's full NVMe row set - host
     * r/w, controller busy time, error-log count - and the data-unit
     * -> byte conversion (x 512,000, DiskHealth_decompiled.c
     * :5589-5614). Appended fields; see ABI-CONTRACT.md section 9. */
    uint64_t host_reads, host_writes, controller_busy_time;
    uint64_t num_error_log_entries;
    uint64_t data_bytes_read, data_bytes_written;
} cr_ui_smart_report;

/* One call fills the report AND (when attrs != NULL) up to attr_cap
 * attribute rows; *attr_count returns the needed size either way. */
CR_UI_API uint32_t cr_ui_health_read(uint32_t disk, cr_ui_smart_report* rep,
                                     cr_ui_smart_attr* attrs, uint32_t attr_cap,
                                     uint32_t* attr_count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_UI_H_ */
