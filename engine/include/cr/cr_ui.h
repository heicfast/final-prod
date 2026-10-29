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

/* v3 = M-067 (the S-039 convergence residue): the cr_ui_user_sign field
 * resize (bytes 16 -> 48, mime 64 -> 81, sign_len/offset ranges) - the
 * first struct FIELD change under the stability policy
 * (docs/ABI-CONTRACT.md section 9). The S-039 caps fix (b07ee9d) landed
 * without this half: a 73-char MIME memcpy'd into mime[64] left the
 * field non-NUL-terminated (a host read past the field). */
#define CR_UI_ABI_VERSION 3u

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
#define CR_EVT_HEARTBEAT                                                                           \
    8u /* N-043 liveness heartbeat: fired from the shim's watchdog thread                          \
        * every 5000 ms while run() is in flight and no                                            \
        * CR_EVT_SCAN_PROGRESS arrived in the window (the og's 5 s                                 \
        * watchdog, DRWUI_decompiled.c:910565-910585 + CreateThread                                \
        * :910864) - the HONEST variant: the payload carries the                                   \
        * standing telemetry truth, never a synthetic permille                                     \
        * advance (the og's watchdog bumps its coarse counter +1                                   \
        * instead). stage = 0xFFFFFFFF; bytes_total = 0 (unknown);                                 \
        * overall_permille / stage_permille = the standing values;                                 \
        * timestamp_ms = elapsed_ms; files counters = standing. */
#define CR_EVT_SAVE_PROGRESS                                                                       \
    9u /* N-060 save-path progress: fired from the thread running                                  \
        * cr_ui_scan_save_session while the session serializes -                                   \
        * coalesced (first tick, permille steps, terminal);                                        \
        * stage = 0xFFFFFFFF; stage_permille = the save's own                                      \
        * 0..1000; overall_permille = 1000 (standing - the scan                                    \
        * is complete); bytes_done/bytes_total = nodes                                             \
        * serialized / total nodes. Cancellation: the SAVE-                                        \
        * SCOPED flag (cr_ui_scan_save_cancel during the save                                      \
        * aborts it with CR_UI_ERR_CANCELLED and the final path                                    \
        * keeps its previous content - the save is atomic                                          \
        * temp-then-rename; the og's per-child IsCancelled is                                      \
        * the save progress object's own state, NOT the scan                                       \
        * flag - save-on-scan-cancel stays intact). */
#define CR_EVT_FS_CHECK                                                                            \
    10u /* M-041 pre-scan damage-audit phase done (the og's                                        \
         * FileSystemCheck): fired ONCE per scan after the check phase                             \
         * completes - check_errors carries the reported issue count                               \
         * (0x1001..0x100e taxonomy) and check_family the detected                                 \
         * family byte; per-error detail rides the session record.                                 \
         * The report NEVER aborts the scan. */
#define CR_EVT_SESSION_LOAD_PROGRESS                                                               \
    11u /* W1b (R-54) load-path progress: fired from the thread running                            \
         * cr_ui_session_load_with_events while the session tree                                   \
         * deserializes - coalesced (first tick, permille steps,                                   \
         * terminal), the CR_EVT_SAVE_PROGRESS mirror. stage =                                     \
         * 0xFFFFFFFF; stage_permille = the load's own 0..1000;                                    \
         * overall_permille = 0xFFFFFFFF (no scan context);                                        \
         * bytes_done/bytes_total = nodes deserialized / persisted                                 \
         * node count. Cancellation: the CALLER-OWNED cancel flag                                  \
         * (cr_ui_session_load_with_events' cancel_flag argument,                                  \
         * polled per node - the og's IUIProgressInfo IsCancelled                                  \
         * vtable+0x20 polled per child, rsf-format--scan-session.md                               \
         * flow section) aborts with CR_UI_ERR_CANCELLED - no                                      \
         * session is returned and the partial tree is discarded                                   \
         * (a torn load never half-claims state). */
#define CR_EVT_AUTOSAVED                                                                           \
    12u /* W1c (R-52) completion auto-save outcome: fired ONCE after                               \
         * run() completes and ScanConfig.autosave_dir is set -                                    \
         * autosave_rc = 0 (saved) or the write error code. The                                    \
         * session lands as <autosave_dir>/auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf                   \
         * (session::auto_session_name, the N-061 corpus convention                                \
         * with the name-forbidden-char sanitizer; the og's completion                             \
         * SaveScan flow, rsf-format section 5.5). A CANCELLED run never                           \
         * auto-saves and never fires this event. S-044: an EMPTY run                              \
         * (no files, no lost partitions), a RESUMED run and a run whose                           \
         * session was already saved explicitly (cr_ui_scan_save_session)                          \
         * never auto-save either - the og's trigger-site gates                                    \
         * (FUN_1402857c0 call sites: results-found / not-import /                                 \
         * not-already-saved). */

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
    /* error context (CR_EVT_ERROR_RANGE; M-049 v2-era append): the
     * module/operation/retry-state triple the og's error-object
     * architecture carries (engine-side subset - the MOM-bus
     * manager/track-manager layer stays out of core scope). Module =
     * CR_ERRMOD_* of the sub-pass that was reading (per-range
     * attribution recorded at failure time; kNone for ranges reloaded
     * from pre-M-049 sessions); operation = CR_ERROP_*; attempts =
     * retries spent before the range was given up. Zero on every other
     * event type. The trailing pad keeps the POD an 8-byte multiple
     * (transport-bridge alignment; always zero). */
    uint32_t err_module;
    uint32_t err_operation;
    uint32_t err_attempts;
    uint32_t err_context_pad;
    /* v2-era append (M-041, CR_EVT_FS_CHECK): the damage-audit phase's
     * summary - reported issue count (the 0x1001..0x100e set; the full
     * detail with codes/params/paths rides the session record + stats)
     * and the detected family byte ('f' fat / 'n' ntfs / 0 undetected).
     * stage = 0xFFFFFFFF (pre-scan). */
    uint64_t check_errors;
    uint32_t check_family;
    uint32_t check_pad; /* keeps the POD an 8-byte multiple (always 0) */
    /* W1c (R-52) append: the auto-save attempt's rc (CR_EVT_AUTOSAVED
     * only; 0 on every other event type). */
    uint64_t autosave_rc;
} cr_ui_event;

typedef void (*cr_ui_event_fn)(void* user, const cr_ui_event* evt);

/* ---- error-context constants (M-049; CR_EVT_ERROR_RANGE payload) ------
 * The module ids mirror the core's cr_errmod.h attribution set (the
 * engine-side subset of the og's per-module error registrars - Error.md
 * census; CErrorManager/CErrorTrackManager's MOM UI bus stays out of core
 * scope). Numeric values are CORE-INTERNAL (the D-68 value-space policy)
 * and pinned to the core enum by static_asserts in cr_ui.cpp; append-only.
 * Unknown values must be treated as "unattributed" by consumers. */
#define CR_ERRMOD_NONE 0u         /* unattributed (pre-M-049 session reloads) */
#define CR_ERRMOD_ORCHESTRATOR 1u /* the scan layer's own reads (probe/table) */
#define CR_ERRMOD_NTFS 2u
#define CR_ERRMOD_FAT 3u
#define CR_ERRMOD_EXFAT 4u
#define CR_ERRMOD_EXT4 5u
#define CR_ERRMOD_BTRFS 6u
#define CR_ERRMOD_HFSPLUS 7u
#define CR_ERRMOD_REFS 8u
#define CR_ERRMOD_ZFS 9u
#define CR_ERRMOD_CARVE 10u /* the carve stage's grammar sweep */
#define CR_ERRMOD_LOST 11u  /* lost-partition discovery */
#define CR_ERRMOD_LDM 12u   /* dynamic-disk (LDM) database parse + volumes */
/* Operations (the error-range family is read failures today). */
#define CR_ERROP_NONE 0u
#define CR_ERROP_READ 1u

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

/* Windows only (platform layer): ERR_UNSUPPORTED elsewhere. DRIVER-ONLY
 * (docs/DRIVER-REQUIRED.md): open_disk/open_volume/open_volume_with_key
 * REFUSE with the classified ERR_DRIVER_* code (0x500B0001..0x500B0005)
 * when the RDA driver is absent, not started, unauthorized,
 * version-incompatible, or errored - there is NO direct-handle fallback
 * (the reference duality was removed by deliberate divergence,
 * PLAN-DRIVER-ONLY; the chosen transport is always "rda" and is reported
 * by cr_ui_device_transport). A failure with a Ready driver is a target
 * error (ERR_OPEN_FAILED et al). open_volume mirrors the CLI: BitLocker
 * state is probed first - a locked volume is refused with
 * ERR_FVE_LOCKED (0x50060001) unless auto-unlock succeeds; the
 * returned device is the volume's slice on its parent disk (kept alive
 * internally). */
CR_UI_API uint32_t cr_ui_open_disk(uint32_t disk, cr_ui_device** out);
CR_UI_API uint32_t cr_ui_open_volume(char letter, cr_ui_device** out);

/* M-030 (diff-to-fix; ABI-since 2.2, additive - compatible per the ABI
 * policy, no version bump): the transport-open diagnostic. With the
 * driver-only architecture (docs/DRIVER-REQUIRED.md) this carries the
 * classified ERR_DRIVER_* refusal code from the LAST open call (or the
 * surviving target error): the presentation contract for hosts that
 * surface transport diagnostics. Advisory only: never gates an open's
 * own return code; never clears; process-global. */
CR_UI_API uint32_t cr_ui_last_transport_error(void);

/* ---- The driver-status surface (docs/DRIVER-REQUIRED.md; ABI-since 2.x,
 * additive - compatible per the ABI policy, no version bump) ----
 *
 * The single synchronous query answering every mandated facet: present /
 * loaded / started / authorized / healthy / version. Registered as a
 * deliberate product-scope divergence (PARITY-DELTAS): the reference
 * product exposes NO driver-status surface to its UI (the service-start
 * result is discarded; failed driver opens are silent NULL devices -
 * REFERENCE-DRIVER-BEHAVIOR section 2-3); this core refuses silently-
 * degrading instead, and the status surface is how a host shows the user
 * exactly which prerequisite is unmet.
 *
 * Facet -> mechanism (DRIVER-CONTRACT section 8; the driver's entire
 * user-mode surface is the control-device open + GET_INFO):
 *   service_present  SCM OpenService        (installed)
 *   service_running  SCM QueryServiceStatus (loaded)
 *   device_openable  CreateFile on the control device (started)
 *   authorized       the open is not ERROR_ACCESS_DENIED (device DACL)
 *   info_valid       GET_INFO + GET_VERSION consistent round-trip
 *                    (healthy) - there is no health/heartbeat IOCTL
 *   major/minor      GET_INFO, cross-checked against GET_VERSION
 *
 * Threading: synchronous, thread-safe, stateless between calls. Lifecycle:
 * none (a standalone query). Errors: 0 on a completed query (the state is
 * in *out - including non-Ready states); CR_UI_ERR_BAD_PARAMETER on null;
 * CR_UI_ERR_UNSUPPORTED off-Windows. state == CR_UI_DRIVER_STATE_READY
 * means every driver-requiring operation may proceed; each other state
 * maps 1:1 to the ERR_DRIVER_* family (driver_state_error in
 * cr/win/cr_driver_status.h). */
#define CR_UI_DRIVER_STATE_READY 0u
#define CR_UI_DRIVER_STATE_ABSENT 1u
#define CR_UI_DRIVER_STATE_NOT_STARTED 2u
#define CR_UI_DRIVER_STATE_UNAUTHORIZED 3u
#define CR_UI_DRIVER_STATE_VERSION_MISMATCH 4u
#define CR_UI_DRIVER_STATE_ERRORED 5u

typedef struct cr_ui_driver_status_t {
    uint32_t state;           /* CR_UI_DRIVER_STATE_* */
    uint32_t service_present; /* 0/1 */
    uint32_t service_running; /* 0/1 */
    uint32_t device_openable; /* 0/1 */
    uint32_t authorized;      /* 0/1 */
    uint32_t info_valid;      /* 0/1 (healthy) */
    uint32_t major;           /* driver wire version, from GET_INFO */
    uint32_t minor;
    uint32_t required_major; /* the core's compatibility requirement */
    uint32_t flags;          /* DriverInfo capability echo */
    uint32_t max_disks;
    uint32_t ioctl_base;
    uint32_t last_gle; /* diagnostic: last Win32 error from the
                        * probe that failed (0 when green) */
} cr_ui_driver_status_t;

CR_UI_API uint32_t cr_ui_driver_status(cr_ui_driver_status_t* out);

/* M-033 (additive export - compatible per the ABI policy, no version bump):
 * locked-volume open with a user-supplied FVEK - the in-stack XTS-AES
 * decrypt path the reference implements below its engines (FVE keys ->
 * XTS decrypt; corpus architecture doc section 8 + BitLockerLib artifact).
 * utf8_hex_fvek: 64 hex chars (AES-128-XTS) or 128 hex chars (AES-256-XTS)
 * - the full-volume-encryption-key bytes (key||tweak-key concatenated,
 * the GetFullVolumeEncryptionKey output shape). ERR_BAD_PARAMETER on
 * malformed/weak key material; ERR_UNSUPPORTED off-Windows; the
 * no-key refusal contract of cr_ui_open_volume is unchanged (locked
 * without a key stays ERR_FVE_LOCKED - legitimate unlock only). */
CR_UI_API uint32_t cr_ui_open_volume_with_key(char letter, const char* utf8_hex_fvek,
                                              cr_ui_device** out);

/* N-064 (additive export - compatible per the ABI policy, no version bump):
 * the password-dictionary unlock aid, the og's Dict flow (228-entry shipped
 * common-password dictionary, config-data-reference.md :20) applied through
 * the same PowerShell -Password / manage-bde -Password plumbing as a
 * user-typed password. THE EXPLICIT CONSENT ACTION: the host calls it only
 * from its user-initiated unlock surface (the og's BitlockerUnlockDialog
 * analog); cr_ui_open_volume NEVER tries passwords by itself - a locked
 * volume stays ERR_FVE_LOCKED until the user acts.
 * utf8_user_passwords: optional extra candidates tried AFTER the shipped
 * set (may be NULL with count 0). Returns OK when a candidate unlocked the
 * volume, ERR_FVE_LOCKED when the whole set failed, ERR_UNSUPPORTED
 * off-Windows. Attempt lines carry the candidate index only (never the
 * password) through the engine log seam. */
CR_UI_API uint32_t cr_ui_unlock_volume_with_dictionary(char letter,
                                                       const char* const* utf8_user_passwords,
                                                       uint32_t count);

CR_UI_API void cr_ui_device_close(cr_ui_device* dev);

typedef struct cr_ui_geometry {
    uint64_t total_bytes;
    uint32_t logical_sector;
    uint32_t physical_sector;
    uint64_t sector_count;
} cr_ui_geometry;

CR_UI_API uint32_t cr_ui_device_geometry(cr_ui_device* dev, cr_ui_geometry* out);

/* Transport truth (the CLI's "[transport]" lines): "rda" or "image",
 * NUL-terminated into buf (cap bytes incl. NUL) — the driver-only
 * architecture has no "direct" value (PLAN-DRIVER-ONLY removed the
 * fallback; the value never appears in code). Two-call: pass
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
    uint32_t layout;    /* 0 none, 1 MBR, 2 GPT, 3 EBR (N-057 append - extended-chain tables; 0..2
                           frozen) */
    uint32_t table_ok;
    char model[128];  /* UTF-8, may be empty */
    char serial[128]; /* UTF-8, may be empty */
    /* M-076 (append-only): the per-row TRIM datum behind the og's home-page
     * "TRIM ON"/"TRIM OFF" chip (UIUX/07:17, :46) - the SSD-aware
     * recovery-chance indicator. zero-init = unknown: the fields are only
     * filled by the opt-in _ex enumeration (CR_ENUM_FLAG_TRIM below), so
     * pre-existing hosts and the plain two-call caller see trim_known == 0
     * (unknown) and are unaffected. The datum is the OS-global TRIM state
     * (fsutil behavior query disabledeletenotify - the same primitive the
     * health facet reads); every row carries the same value, exactly as
     * every SSD row of the og's page renders the same chip state. */
    uint32_t trim_known; /* 0 = not queried/unknown (the flag was off, or the query failed); 1 = the
                            value below is valid */
    uint32_t trim_on;    /* valid only when trim_known == 1: 1 = TRIM enabled (recovery-chance
                            warning applies on SSDs), 0 = disabled */
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

/* M-076: the enumeration opt-in flags (the _ex call below only - the plain
 * two-call exports never take flags). Unknown bits are refused with
 * CR_UI_ERR_BAD_PARAMETER - a host passing a flag this build does not know
 * must fail loudly, not silently ignore. */
#define CR_ENUM_FLAG_TRIM 0x00000001u

/* Two-call pattern for both: *count as IN capacity when out != NULL, as
 * OUT needed when out == NULL. Enumeration failure returns its error code
 * (never a silent empty list - CLI G-20 parity).
 *
 * Refresh contract (R-86's pull-model equivalence, N-098): each call
 * RE-QUERIES the OS - there is no cache, no memoization, no retained
 * state anywhere in the path (a fresh PhysicalDiskList is constructed
 * per invocation). Calling this again IS the refresh - the og's modal
 * refresh flow re-runs the same enumeration machinery, and the modal
 * chrome is host-side. The update-disk-metadata case (making the OS
 * itself see a changed disk) is the C++-only win::refresh_disk_properties
 * (IOCTL_DISK_UPDATE_PROPERTIES, S-029), documented as such at
 * REFERENCE-API-GAP R-86. */
CR_UI_API uint32_t cr_ui_enumerate_disks(cr_ui_disk_info* out, uint32_t* count);
CR_UI_API uint32_t cr_ui_enumerate_disks_ex(cr_ui_disk_info* out, uint32_t* count, uint32_t flags);
CR_UI_API uint32_t cr_ui_enumerate_volumes(cr_ui_volume_info* out, uint32_t* count);

/* M-076 opt-in contract (cr_ui_enumerate_disks_ex above): the TRIM chip
 * datum is NOT free - the underlying fsutil query spawns a process per
 * invocation. cr_ui_enumerate_disks (and _ex with flags == 0) never pays
 * it; a host that wants the og's per-row chip opts in with
 * CR_ENUM_FLAG_TRIM and accepts the latency (the og itself queries the
 * same primitive for its home page - the chip is worth the cost there,
 * and the caller decides, per the opt-in design recorded in the M-076
 * resolution). The query is issued ONCE per invocation (the datum is
 * OS-global) and stamped on every row; a failed query leaves trim_known
 * == 0 on every row - never a guessed value. */

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
                                    * to 99 chars - the og's record-2
                                    * volumeName[100] width (N-090; the
                                    * legacy session card keeps the
                                    * 31-char prefix view) */
    const char* volume_model_utf8; /* optional identity-card overrides; when
                                    * absent the shim fills them from the
                                    * disk enumeration (Windows, CLI parity) */
    const char* volume_serial_utf8;
    /* N-066 (appended tail, zero-init safe - hosts compiled before this
     * field pass zeros and the axes stay absent): the og record-3
     * identity axes; the shim fills them from the enumeration like
     * model/serial. */
    const char* volume_firmware_utf8;
    const char* volume_wwn_utf8;
    const char* volume_interface_utf8;
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
    uint32_t memory_warn_mb;            /* v2-era append (N-062): the
                                         * memory monitor's threshold in
                                         * MiB; 0 = OFF (the conservative
                                         * default). While run() is in
                                         * flight the shim's watchdog
                                         * samples the process working
                                         * set every 5 s and emits a
                                         * Warn line through the cr::log
                                         * seam at/above the threshold.
                                         * Zero-init safe (0 = off). */
    const char* raw_category_name_utf8; /* v2-era append (M-029): the
                                         * display name for the raw/carve
                                         * category root - the og's
                                         * per-scan "filter_generalraw"
                                         * naming surface. NULL/"" = the
                                         * default "Other Deleted Files
                                         * (Carved)" role. */
    uint32_t pre_scan_check;            /* v2-era append (M-041): 1 = run the
                                         * read-only damage-audit check phase
                                         * (the og's FileSystemCheck error set,
                                         * codes 0x1001..0x100e) after any
                                         * pre-scan repair and before the
                                         * engines. The report NEVER aborts the
                                         * scan; it surfaces through
                                         * cr_ui_scan_stats' check_* fields, the
                                         * FsCheck event and the session record.
                                         * Zero-init safe (0 = off). */
    const char* autosave_dir_utf8;      /* W1c (R-52) append: the completion
                                         * auto-save DIRECTORY (the og's
                                         * auto_<vol>_<ts> SaveScan flow,
                                         * rsf-format section 5.5). NULL/""
                                         * = OFF (zero-init safe). On run()
                                         * COMPLETION - and, per S-057/N-211,
                                         * on a user-STOPPED run with results
                                         * (the og's stop path runs its own
                                         * organize into the shared
                                         * completion handler - DRWUI
                                         * :1549399-421 -> :339076-79 ->
                                         * FUN_140395e50, gates :612571-587
                                         * with no stopped-check) - the shim
                                         * saves the
                                         * standard record set there under
                                         * session::auto_session_name (the
                                         * N-061 corpus naming + sanitizer),
                                         * fires CR_EVT_AUTOSAVED once and
                                         * sets stats.autosaved. S-044: an
                                         * empty run (nothing found), a
                                         * resumed run and an already-
                                         * explicitly-saved run never
                                         * auto-save either (the og's
                                         * trigger-site gates). Other
                                         * failures (device loss, read
                                         * refusal) do not reach the
                                         * completion handler and do not
                                         * auto-save. */
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
    /* v2-era append (M-041 damage-audit report summary; the full issue
     * list with codes/params/paths rides the session record + the core's
     * ScanStats): */
    uint32_t check_ran;      /* the check phase executed at all */
    uint32_t check_errors;   /* reported issue count (the 0x1001..0x100e set) */
    uint32_t check_detected; /* family identified (fat/ntfs) */
    /* driver-only session append (the fake-scan fix; read-truth
     * accounting - docs/DRIVER-REQUIRED.md section 7): the host must be
     * able to present "these regions were NOT scanned" and "family
     * unknown because the header was unreadable" without reading the
     * log. bytes_unreadable = the error map's coverage (Skip-mode
     * zero-fill hides failures from engines, never from this field);
     * a scan that could read (essentially) nothing REFUSES with
     * ERR_READ_FAILED instead of fake-completing. */
    uint64_t bytes_unreadable;
    uint32_t probe_unreadable; /* sector 0 read failed (family=Unknown is
                                * BLIND, not filesystem-absent) */
    /* W1c (R-52) append: the completion auto-save outcome (the og's
     * auto_<vol>_<ts> SaveScan flow). 1 = the auto session landed in
     * ScanConfig.autosave_dir under session::auto_session_name; 0 =
     * off (the zero-init default), the run did not complete, the run
     * found nothing, the run was resumed, or the session was already
     * saved explicitly (S-044 - the og's trigger-site gates). The
     * per-attempt rc rides CR_EVT_AUTOSAVED. */
    uint32_t autosaved;
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
    /* M-137 append-only tail: the og's found-partition row context
     * (UIUX/08 §2: Disk / Basic / Dynamic / MBR / GPT / File System).
     * disk_layout: 0 none/unknown, 1 MBR, 2 EBR, 3 GPT, 4 GPT-damaged
     * (table::TableLayout). disk_flags bit 0 = the scan target is
     * dynamic (LDM database found), bit 1 = the target's table parsed.
     * Hosts compiled against the pre-M-137 header may keep passing
     * 24-byte structs - zero-initialize the whole struct before filling
     * (the planner never reads these fields; they are presentation
     * truth for the host's listing). */
    uint32_t disk_layout;
    uint32_t disk_flags;
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
    char label[32]; /* UTF-8 (the legacy 31-char prefix view) */
    /* N-090 (ABI append): the complete label at the og's
     * volumeName[100] width; == label on v1..v5 sessions. */
    char label_full[100]; /* UTF-8, NUL padded */
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

/* ---- session list (W1a / R-56) --------------------------------------------
 * Directory-level summary listing - the og's LastScanRecordView surface
 * (UIUX/07-home-phase-deep-dive.md:21 + its Continue-last banner): every
 * *.crsf file in utf8_dir yields ONE row decoded from the container's
 * LIGHT records only (SESSION_STATE + VOLUME_INFO); the heavy payloads
 * (the 36 MB-class tree record among them) are skipped BY TYPE before
 * any payload I/O (ScanController::read_summary + Reader::peek_record).
 *
 * Two-call size pattern (same contract as cr_ui_snapshot_dfs):
 *   rows == NULL -> *count = the number of *.crsf candidates (capacity
 *                   probe; nothing is decoded)
 *   rows != NULL -> *count is the IN capacity; rows fill NEWEST-FIRST by
 *                   the embedded save time (the og's LastScanRecordView
 *                   order - its record-2 SYSTEMTIME comparator sorts
 *                   newest first, which is what makes the og's
 *                   "Continue last scanning" banner pick the right file);
 *                   equal stamps and zero-stamp rows (pre-v5 sessions,
 *                   unreadable files) break ties by path bytes so the
 *                   order is deterministic; capacity < needed ->
 *                   CR_UI_ERR_TRUNCATED with *count = the number filled
 *                   (the time-ordered prefix - the NEWEST rows)
 * An unreadable file (bad magic, torn container, open failure) still
 * gets a row: valid = 0 + error = the decode/open error - a listing
 * never hides damage. A valid row carries error = CR_UI_OK unless the
 * PATH itself needed truncation (>= sizeof(path) bytes): then error =
 * CR_UI_ERR_TRUNCATED and path[] holds the 1023-byte prefix while the
 * summary fields stay decoded.
 * save_unix is the EMBEDDED save time (v5+ sessions; the og record-2
 * SYSTEMTIME scanTime); 0 = a pre-v5 session - display the file mtime
 * then, never a fabricated time. Errors: CR_UI_ERR_BAD_PARAMETER
 * (null/empty args), ERR_OPEN_FAILED (the directory itself cannot be
 * read). Threading: no shared state; any thread may call. */

#define CR_UI_SESSION_ROW_PATH 1024

typedef struct cr_ui_session_row {
    uint32_t valid;           /* 1 = summary decoded */
    uint32_t error;           /* CR_UI_OK, or the decode/truncation error */
    uint32_t fs_family;       /* orchestrator FsFamily as stored; 0 unknown */
    uint32_t stage_done_mask; /* quick|deep|carve bits actually done */
    uint32_t configured_mask; /* stages the original run configured */
    uint64_t files_found, deleted_found, carved_found;
    uint64_t bytes_scanned, elapsed_ms;
    uint64_t save_unix;                /* embedded save time; 0 = pre-v5 session */
    char label[32];                    /* UTF-8 volume label; empty on legacy files */
    char path[CR_UI_SESSION_ROW_PATH]; /* UTF-8 full path, NUL padded */
    /* N-090 (ABI append - the tail-append pattern, no version bump):
     * the COMPLETE volume label at the og's record-2 volumeName[100]
     * width (strncpy(...,99)); label[32] above stays the legacy
     * 31-char prefix view so old callers see exactly what they always
     * saw. == label on v1..v5 sessions (the uniform degrade). */
    char label_full[100]; /* UTF-8, NUL padded */
} cr_ui_session_row;

CR_UI_API uint32_t cr_ui_session_list(const char* utf8_dir, cr_ui_session_row* rows,
                                      uint32_t* count);

/* ---- session load progress + cancel (W1b / R-54) ---------------------------
 * The load-side twin of the N-060 save machinery. The og's RESTORE
 * reports progress inside the load window (40+50*done/(total+done),
 * rsf-format--scan-session.md flow section) and polls IsCancelled
 * after EVERY child. This variant of cr_ui_session_load takes:
 *   fn          - optional event sink; CR_EVT_SESSION_LOAD_PROGRESS
 *                 fires coalesced (first tick, permille steps,
 *                 terminal) from the CALLING thread
 *   cancel_flag - optional CALLER-OWNED flag (zero-init safe; set it
 *                 nonzero FROM ANY THREAD to abort the load), the
 *                 og's IUIProgressInfo IsCancelled mirror. Polled
 *                 per node EVEN WHEN fn is NULL (a silent host can
 *                 still cancel).
 * On cancel: returns CR_UI_ERR_CANCELLED, *out untouched (no
 * half-claimed session - a torn load never hands out partial state).
 * With fn = NULL and cancel_flag = NULL this is exactly the plain
 * cr_ui_session_load. */
CR_UI_API uint32_t cr_ui_session_load_with_events(const char* utf8_path, cr_ui_event_fn fn,
                                                  void* user, volatile uint32_t* cancel_flag,
                                                  cr_ui_session** out);

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
    const char* path_substring; /* M-072 (ABI-appended): the path group's
                                 * substring form; NULL = no filter */
    uint32_t state_mask;        /* M-072 (ABI-appended): tree::DeletedState
                                 * bits (bit0 Active, bit1 Deleted, bit2
                                 * Partial, bit3 Damaged); 0 = all states */
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

/* M-029 (v2-era added export): translate the og filter-bar's Date/Size
 * bucket IDs into this query's range fields, in place.
 *
 * date_bucket / size_bucket are the og's stable action IDs (the
 * CTimeFilterBtn/CSizeFilterBtn menus): Date 0x100 Today, 0x101 Yesterday,
 * 0x102 Last 7 days, 0x103 Last 1 month, 0x105 Custom; Size 0x1000 < 512KB,
 * 0x1001 512KB~1MB, 0x1002 1MB~512MB, 0x1003 512MB~1GB, 0x1004 > 1GB,
 * 0x1006 Custom. 0 leaves that axis untouched. The skipped IDs (0x104,
 * 0x1005) and any unknown value are REJECTED with ERR_BAD_PARAMETER
 * - the core never invents semantics the og's menu does not have.
 *
 * Date windows are computed from the host clock as day-truncated LOCAL
 * time exactly like the og (its cached today/yesterday/7d/30d bounds);
 * Custom (0x105) uses date_custom_from/to as FILETIMEs, both ends
 * inclusive after local-day truncation. Size buckets use the og's exact
 * byte boundaries; Custom (0x1006) is inclusive on [size_custom_min,
 * size_custom_max]. */
CR_UI_API uint32_t cr_ui_query_apply_buckets(cr_ui_query* q, uint32_t date_bucket,
                                             uint32_t size_bucket, uint64_t date_custom_from,
                                             uint64_t date_custom_to, uint64_t size_custom_min,
                                             uint64_t size_custom_max);

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

/* ---- N-140: the per-format carve tally (the og's GetRawUserInfoMap) ------
 * The og serves its per-scan carve-statistics map<extension,count> through
 * the GetRawUserInfoMap export (RAWSearchDLL :72213-72221; FUN_18000ed00's
 * insert-or-increment feeds it from every analyser detection; DRWUI's
 * post-scan report renders the "found N .jpg / M .png" breakdown,
 * :227538-227625). The core's equivalent: per-slot counts over the SAME
 * unified format-id space cr_ui_carve_format_count() enumerates (grammar
 * rows first, then raw-sign rows), plus ONE aggregate user-sign slot at
 * the end (index == cr_ui_carve_format_count(); the og's map keys user
 * signs by their dynamic extension strings - unbounded - so the aggregate
 * is the honest fixed-table projection).
 *
 * cr_ui_scan_carve_tally_count: the live slot bound (0 = no carve stage
 * ran or the session predates the tally). cr_ui_scan_carve_tally: the
 * count at `index` into *out (BAD_PARAMETER on null/out-of-range). The
 * format identity for index < cr_ui_carve_format_count() is
 * cr_ui_carve_format_info(index, ...); the last slot is the user-sign
 * aggregate. No tree walk required. */
CR_UI_API uint32_t cr_ui_scan_carve_tally_count(cr_ui_scan* s);
CR_UI_API uint32_t cr_ui_scan_carve_tally(cr_ui_scan* s, uint32_t index, uint64_t* out);

/* ---- M-048: the user-extensible signature database (ABI-since 2.1) ---------
 * The og's EUUserData surface: the host registers user-taught file
 * signatures (the og's JSON shape verbatim:
 *   { "<ext>": { "signs": ["<offset>,<HEX>"], "mime": "<type>" }, ... }
 * ). Registered signs are probed by every carve pass at the raw-sign
 * stride (built-in layers first; a user sign fires only where the
 * built-ins did not). Re-registration REPLACES the previous set (the
 * og's config-reload posture - EUUserData is re-read per scan).
 *
 * cr_ui_register_user_signs: parse + validate + install atomically.
 * Returns OK or BAD_PARAMETER (null/empty or a grammar violation - the
 * registration is atomic, never a silent partial load).
 * The set is process-global; thread-safe.
 *
 * cr_ui_user_sign_count / cr_ui_user_sign_info: enumeration for hosts
 * that render the active set (the og's UI lists user formats). */
CR_UI_API uint32_t cr_ui_register_user_signs(const char* json_bytes, uint32_t json_len);

typedef struct cr_ui_user_sign {
    char ext[9];       /* lowercased, NUL-terminated */
    char mime[81];     /* NUL-terminated; empty when the entry had none (v3/M-067:
                          the og's longest MIME is 73 chars - the office family) */
    uint32_t offset;   /* 0..1024 (v3/M-067: the og's max offset is 526) */
    uint32_t sign_len; /* 1..48 (v3/M-067: the og's longest sign is 44 B) */
    uint8_t bytes[48]; /* the signature bytes (v3/M-067: was 16) */
} cr_ui_user_sign;

CR_UI_API uint32_t cr_ui_user_sign_count(void);
CR_UI_API uint32_t cr_ui_user_sign_info(uint32_t index, cr_ui_user_sign* out);

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
 * pass (G-10 - a preview-true bounded reader: range reads under explicit
 * caps; NOT an og-named contract - M-028 proved the corpus's
 * IInWindowStream is the 7-Zip/LZMA SDK input interface, not a preview
 * surface). The reader owns a private copy of the
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

/* ---- archive inventory (M-116: the engine's browse-inside ABI half) -----
 * The engine extracts a bounded archive-content inventory at preview time
 * (the M-066 landing: the central-directory walk, the damaged-zip partial
 * salvage, the 512-entry hostile bound) - before these exports it was
 * unreachable from every C surface (cr_ui_metadata has no entries field
 * and the bridge discarded Metadata.entries; the M-100 retirement's
 * "reachable through cr_ui_preview_metadata" residue claim was false).
 *
 * Two-call pattern (the M-102 house shape): count probe, then per-index
 * fill. The inventory is extracted at the FIRST archive call on the
 * handle and cached for the handle's lifetime - a stable listing (the
 * og's ArchivePreviewModel serves one model per browse view); a fresh
 * preview handle re-extracts. Non-archive nodes report count 0 (never an
 * error). `name_truncated` flags a name cut at the fixed 260-byte
 * capacity (names are archive-native UTF-8 pass-through, never
 * re-encoded). */
typedef struct cr_ui_archive_entry {
    char name[260];       /* archive-native bytes, NUL-terminated */
    uint64_t size;        /* uncompressed size */
    uint64_t compressed;  /* compressed size */
    uint32_t crc_present; /* CD CRC-32 != 0 (the damaged-entry marker) */
    uint32_t name_truncated;
} cr_ui_archive_entry;

CR_UI_API uint32_t cr_ui_archive_entry_count(cr_ui_preview* pv, uint32_t* out);
CR_UI_API uint32_t cr_ui_archive_entry_at(cr_ui_preview* pv, uint32_t index,
                                          cr_ui_archive_entry* out);

/* Truthful failure map: device spans that could not be read and were
 * zero-filled. *total_bytes = sum of span lengths. */
CR_UI_API uint32_t cr_ui_preview_unreadable(cr_ui_preview* pv, uint64_t* total_bytes,
                                            uint32_t* range_count);

/* ---- thumbnail (M-102: the engine's ABI half; I-028 superset register) -----
 * The og's results page fetches a 256-px thumbnail per image row through
 * a dedicated worker (UIUX/10 §2; DRWUI :652395-652410) - host-side, but
 * backed by engine-readable pixels. This is the equivalent surface: one
 * bounded generation per node, synchronous (the async worker is host-side
 * by the in-process doctrine, I-028). Output is DISPLAY-ORIENTED RGBA8
 * (S-043: the EXIF orientation transform runs before the downscale).
 *
 * M-083: a successful generation promotes the node's grade to 2 ("high")
 * - the og's grading side-effect (FUN_1402636e0 :422498-518: grade != 2
 * -> FUN_140089630(node, 2)). The promotion never downgrades and is a
 * no-op for already-high nodes (the setter's monotonic-max + sticky-2
 * semantics are the node's own, og-exact). */

/* Caps block mirroring the engine's ThumbnailCaps. NULL caps (or any
 * zero field) = that default; nonzero values are validated by the same
 * ranges the engine enforces (max_edge 1..8192, max_pixels/max_read/
 * max_work nonzero, allow_embedded 0 or 1 - values above 1 are refused
 * with ERR_BAD_PARAMETER). allow_embedded NOTE (N-137): the embedded
 * fast path is ALWAYS preferred (og parity - the og's thumbnail worker
 * takes the embedded preview whenever present); 0 = default-ON and
 * 1 = explicit-ON. There is no off position by design. */
typedef struct cr_ui_thumbnail_caps {
    uint32_t max_edge;   /* 0 = 256 (the og's GDI+ convention) */
    uint64_t max_pixels; /* 0 = 64 MP (decode-memory bomb guard) */
    uint32_t max_read;   /* 0 = 200 MB / 0xC800000 (the og's still-image eligibility gate, M-109) */
    uint32_t max_work;   /* 0 = 4 Mi fetch units (decoder work bound) */
    uint32_t allow_embedded; /* 0 = ON (the EXIF fast path, M-080/D-105) */
} cr_ui_thumbnail_caps;

typedef struct cr_ui_thumbnail_info {
    uint32_t width; /* output dims (post-orientation, post-downscale) */
    uint32_t height;
    uint32_t bytes_needed;  /* width*height*4 - the two-call probe's answer */
    char source_format[16]; /* jpeg|png|bmp|gif|tiff|psd|pnm|hdr|raw */
    uint8_t from_embedded;  /* the EXIF fast path served it */
    uint8_t truncated;      /* decoder-tolerated short stream (disclosed,
                             * never silent - the engine's Ok-but-truncated
                             * contract crosses the ABI verbatim) */
    uint8_t _pad[2];
} cr_ui_thumbnail_info;

/* Generate one node's thumbnail. TWO-CALL pattern: rgba == NULL is the
 * SIZE PROBE (the generation runs - dims depend on the decode - and
 * info->bytes_needed carries the answer; pixels are discarded); then the
 * caller allocates and calls again. rgba != NULL with rgba_cap too small
 * returns 0x50030009 ERR_NO_MEMORY with bytes_needed set (nothing is
 * partially written). Status mapping is the engine's verbatim:
 * 0 = OK (info + pixels); 0x50030004 ERR_TRUNCATED is returned for a
 * decode REFUSAL (a damaged/short stream the strict decoders reject -
 * ABI-CONTRACT 7i's mapping; the refusal branch also sets
 * info->truncated); in-band truncation (a decoder-tolerated short
 * stream) is NOT an error - it crosses via info->truncated on Ok;
 * 0x50030008 ERR_UNSUPPORTED for the named phase-2 refusals
 * (info->source_format carries the family), other codes as the error
 * table. caps == NULL = the defaults above. The device is the scan's
 * device (the same pairing rule as cr_ui_preview_open_session).
 *
 * M-115 (grading side effects, decoded at DRWUI :422507-514 + :421400-402):
 * (1) nodes of size <= 20 (0x14) are refused 0x50030008 - the og's
 * dispatcher never creates work items that small, so no attempt and NO
 * grade promotion happen; (2) a decode REFUSAL (0x50030004) whose reads
 * were clean still promotes the node's grade to 2 - the og promotes on
 * READ evidence (readVerdict==0 with payload, decode-independent, the
 * decode's failure never retracts) - while span-damaged, budget-exhausted
 * and unsupported attempts never promote. */
CR_UI_API uint32_t cr_ui_thumbnail_generate(cr_ui_device* dev, cr_ui_session* sess,
                                            uint64_t node_id, const cr_ui_thumbnail_caps* caps,
                                            uint8_t* rgba, uint32_t rgba_cap,
                                            cr_ui_thumbnail_info* info);

/* The format-support probe (the engine's documented v1 matrix + the
 * S-045 raw family): true for jpeg/png/bmp/gif/tiff/psd/pnm/hdr/raw. */
CR_UI_API uint32_t cr_ui_thumbnail_format_supported(const char* utf8_format);

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

/* S-040 (R-93): create a flat raw disk image of physical disk N at
 * utf8_path (the og home page's "Create Image" producer - the safe-scan
 * posture's first step, UIUX/07:36-37 + UIUX/11:17; raw-flat only,
 * VHD/VHDX writers out of scope - PARITY-DELTAS D-147).
 *
 * Classification (a) driver-required (DRIVER-REQUIRED.md): the call
 * requires the RDA driver Ready like every disk-data-path verb, and
 * refuses with the ERR_DRIVER_* class otherwise (the same gate as
 * cr_ui_open_disk).
 *
 * progress (may be NULL) is the recovery-style cumulative-bytes callback.
 * Disposition: streamed to `<path>.crimg-tmp`, atomically renamed on
 * completion + a `<path>.sha256` sidecar (the standard "<hex>  <name>"
 * form). On cancel or failure the partial image is KEPT as
 * `<path>.partial` - reported, never silently passed off as complete.
 * Win-backed: on non-Windows lanes returns ERR_UNSUPPORTED (the honest
 * platform answer). */
CR_UI_API uint32_t cr_ui_image_create(uint32_t disk, const char* utf8_path,
                                      cr_ui_recovery_progress_fn progress, void* user);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_UI_H_ */
