# ABI-CONTRACT — the `cr_ui` C ABI: ownership, threading, errors, events, stability

> **Status: normative.** This document is the contract for every consumer that
> binds `cr_ui.dll` / `libcr_ui.so` (G-01, issue #1). The header
> `include/cr/cr_ui.h` is the machine surface; this file is the human contract
> behind every symbol: ownership rules, threading rules, the error model, the
> typed event stream (§6), the disk-map geometry model (§7), the carve-format
> registry (§7a), and the stability policy (§9). Where a statement here and
> in the header disagree, the HEADER wins and this file must be fixed in the
> same change — never the reverse.
>
> Architecture lineage (corpus): the reference binds its UI to the engines
> IN-PROCESS via C-export factories — DRWUI.exe `LoadLibrary`'s each
> `*SearchDLL.dll` and `GetProcAddress`'es `EUCreate<FS>Searcher` /
> `EUCreate<FS>QuickSearcher`, which return controller interface pointers
> (corpus `searchdll-family--architecture.md` §1; `DRWUI-exe--main-ui-architecture.md`
> §9.2, loader FUN_14056c0d0 + factory map FUN_140577d40). This ABI is our
> equivalent of that factory surface: opaque handles stand in for the
> controller interfaces, `cr_ui_*` factories stand in for `EUCreate*Searcher`,
> C callbacks stand in for the engine event hooks.

## 1. Conformance basics

- **C11**, no C++ types cross the boundary, no exception ever escapes (every
  entry point has a catch-all that returns `CR_UI_ERR_INTERNAL`).
- Every function returns a `uint32_t` error code from the SAME numeric space
  as `cr_error.h` (`0` = success; e.g. `0x50020005` = ERR_CANCELLED).
- Strings in: NUL-terminated UTF-8. Strings out: caller buffers (two-call
  size pattern) or heap strings freed with `cr_ui_string_free` (calling
  `free()` is also valid).
- **Zero-initialized config structs are ALWAYS safe.** Numeric fields use
  sentinel values that mean "default" (typically 0); pointer fields may be
  NULL. A config that was memset to zero produces the documented default
  behavior — never undefined behavior.
- Structs are plain-old-data, C-layout stable, and only ever APPENDED to
  (see §9). The caller compiles against `CR_UI_ABI_VERSION` and checks
  `cr_ui_abi_version()` before any other call.

## 2. Ownership model

| Object | Created by | Freed by | Borrows |
|---|---|---|---|
| `cr_ui_device` | `cr_ui_open_image` / `cr_ui_open_disk` / `cr_ui_open_volume` / `cr_ui_open_volume_with_key` (M-033: locked-volume open with a user-supplied FVEK — the in-stack XTS decrypt opt-in; hex string in, same lifetime contract) | `cr_ui_device_close` | — |
| `cr_ui_scan` | `cr_ui_scan_create` / `cr_ui_session_resume` | `cr_ui_scan_close` | the attached device (caller keeps it alive until after close) |
| `cr_ui_session` | `cr_ui_session_load` | `cr_ui_session_close` | — |
| `cr_ui_snapshot` | `cr_ui_scan_snapshot` / `cr_ui_session_snapshot` | `cr_ui_snapshot_close` | the scan/session it came from (values are copied; the source may close after the LAST row has been READ, not while rows are being read) |
| `cr_ui_preview` | `cr_ui_preview_open_*` | `cr_ui_preview_close` | the DEVICE (must outlive the preview); the source session/snapshot may close — properties are private copies |
| `cr_ui_recovery` | `cr_ui_recovery_create` | `cr_ui_recovery_close` | the source device + every session/snapshot whose nodes were enqueued |
| heap strings / query rows | any `char**` / `cr_ui_query_row*` out-param | `cr_ui_string_free` / `cr_ui_query_rows_free` | — |

Rule of thumb: **close order is the caller's responsibility; every object
documents what it borrows and the borrow must outlive the object.** No handle
is internally reference-counted across threads — see §3 for the thread rules.

## 3. Threading model

- `cr_ui_scan_run` and `cr_ui_recovery_process` run SYNCHRONOUSLY on the
  CALLER's thread (the reference UI calls its controllers from worker
  threads the same way — corpus `DRWUI-exe--thread-architecture.md`).
- `cr_ui_scan_pause` / `cr_ui_scan_resume` / `cr_ui_scan_cancel` /
  `cr_ui_recovery_cancel` / `cr_ui_scan_telemetry` / `cr_ui_scan_snapshot`
  are safe from ANY thread while run()/process() executes.
- One handle, one controlling thread: create/run/close a scan on a single
  thread; the token functions above are the ONLY cross-thread entries.
- Snapshots are immutable value copies (G-02): readers never block writers,
  writers never block readers. A snapshot taken mid-scan is stamped
  `partial=1` and is safe to browse AND to recover from (value semantics).
- All other entry points: call from one thread at a time per handle.
- The library never calls back on a thread the caller didn't hand it:
  progress callbacks fire on the thread running `run()`/`process()`;
  EVENT callbacks (`cr_ui_scan_set_event_fn`) may additionally fire from
  engine worker threads and the heartbeat watchdog thread (§6; the
  header's contract block is the thread-provisioning reference — a host
  reading only this section should still provision for worker-thread
  event delivery).

## 4. Error model

Single numeric space, shared with `cr_error.h`. Selected codes:

| Code | Symbol | Meaning |
|---|---|---|
| 0x00000000 | OK | success |
| 0x50020005 | ERR_CANCELLED | cooperative cancel completed; tree/queue state preserved |
| 0x50030001 | ERR_BAD_MAGIC | volume/boot/signature mismatch |
| 0x50030002 | CR_UI_ERR_BAD_CHECKSUM | identity mismatch on resume (`cr_ui_scan_verify_identity`) |
| 0x50030003 | ERR_BAD_PARAMETER | invalid argument |
| 0x50030004 | ERR_TRUNCATED | output buffer too small (two-call pattern) / torn read |
| 0x50030008 | ERR_UNSUPPORTED | surface exists only on the Windows platform layer |
| 0x50030009 | ERR_NO_MEMORY | allocation failure |
| 0x50050001 | ERR_DEST_OVERLAP | destination overlaps the source (F12 refusal) |
| 0x50060001 | ERR_FVE_LOCKED | BitLocker volume locked (legitimate unlock failed) |
| 0x500B0001 | ERR_DRIVER_ABSENT | driver service not installed (prerequisite gate, DRIVER-REQUIRED.md §4) |
| 0x500B0002 | ERR_DRIVER_NOT_STARTED | driver installed, service not RUNNING |
| 0x500B0003 | ERR_DRIVER_UNAUTHORIZED | device DACL denied the control-device open |
| 0x500B0004 | ERR_DRIVER_VERSION_MISMATCH | GET_INFO (0x6002440) wire major != required |
| 0x500B0005 | ERR_DRIVER_ERRORED | service RUNNING but unusable (open/GET_INFO/GET_VERSION failure class) |
| 0x50090001 | CR_UI_ERR_INTERNAL | internal invariant failure (bug — report it) |

Diagnostics: `cr_ui_last_error()` returns thread-local text for the LAST
failing call on that thread; stable until the next `cr_ui_*` call on the same
thread. Never NULL.

**N-011e (diff-to-fix) — og error-facility families (registration).** The og
carries structured facility codes the core's `cr::Err` space does not model
verbatim (decoded: NTFSSearchDLL `0x2980005`/`0x2a30006`/`0x2a90006`/
`0x3a90001`/`0x17c0004` at :18908-27/:18966/:17897, plus the HFS+ family
`0x50LLxxxx00000nnn`). They are an INTERNAL classification surface of the og
engines (never consumer-visible exit codes); the core's engine errors map by
*category* (bad-magic/bad-parameter/truncated/unsupported) instead of by
facility number. Mapping table for parity tooling that reads both sides'
logs:

| og facility | Category | Core code |
|---|---|---|
| 0x2980005 (ntfs open fail) | bad parameter/magic | 0x50030001 / 0x50030003 |
| 0x2a30006 (ntfs walk fail) | read/parse | 0x50020002 / 0x50030004 |
| 0x2a90006 (ntfs deep-scan fail) | read/parse | 0x50020002 / 0x50030004 |
| 0x3a90001 (ntfs journal/aux fail) | unsupported/damaged | 0x50030008 |
| 0x17c0004 (ntfs volume-state) | volume state | 0x50030003 |
| 0x50LLxxxx00000nnn (HFS+ family) | engine-internal | per-case category |

Registration only — the core does not adopt the og's numeric facility space
(consumers depend on `cr_error.h`'s stable codes).

**Error philosophy:** failures are surfaced, never masked. A scan that cannot
open the volume returns its engine error; an empty result is `OK` with zero
files, not an error; a torn session record is repaired (F10 salvage) or
refused with `0x50040001` — never silently truncated.

## 5. Lifecycle flows (canonical order)

```
scan:        open_* -> scan_create -> scan_attach_device -> [scan_run]
             -> scan_snapshot* / scan_save_session -> scan_close -> device_close
resume:      session_load -> session_resume -> scan_attach_device
             -> scan_verify_identity -> scan_run -> ... -> scan_close
browse:      session_load -> session_snapshot -> snapshot_node/dfs
             -> preview_open_snapshot -> preview_read/metadata -> preview_close
             -> snapshot_close -> session_close
archive:     preview_open_session/snapshot -> archive_entry_count
             -> archive_entry_at × count -> preview_close (M-116: the
             browse-inside inventory's two-call pattern; the extraction
             runs once at the first archive call and is cached for the
             handle's lifetime - one stable listing per preview, the
             og's one-model-per-browse-view shape; non-archive nodes
             report count 0, never an error; out-of-range indices
             refuse with ERR_RANGE_OUT_OF_BOUNDS)
device rows: cr_ui_enumerate_disks[_ex] (the census; the _ex form's
             CR_ENUM_FLAG_TRIM opt-in adds the per-row TRIM chip datum —
             trim_known/trim_on appended to cr_ui_disk_info, the og's
             home-page SSD recovery-chance indicator UIUX/07:17; one fresh
             fsutil read per invocation, the OS-global datum stamped on
             every row, a failed query leaves trim_known == 0 — never a
             guessed value; unknown flag bits are refused before the
             platform check so the grammar is lane-uniform) — M-076
recover:     recovery_create -> recovery_set_destination
             -> recovery_enqueue_[node|subtree][_snapshot]* -> recovery_process
             -> recovery_report_json -> recovery_close
events:      (any time after create) scan_set_event_fn — see §6
state query: (any time after create, any thread) scan_busy / scan_stopping —
             the og's sigIsBusy(bool&) ×14 / sigIsStopping(bool&) ×8
             reference-out query signals (corpus UIUX/25-connect-wiring-graph.md
             :125-144) as synchronous ops; busy = run() in flight, stopping =
             the live cancel request (persists after the one-shot run ends —
             a cancel WAS requested on this handle; telemetry's cancelled flag
             is the post-run state, busy/stopping are the live answers)
carve formats: carve_format_count / carve_format_info — see §7a
disk map:    cr_ui_disk_map (session OR snapshot) — see §7
```

**The LDM basis (S-052).** Nodes from a dynamic-volume scan carry
LAYOUT-RELATIVE extents: their tree root stamps a basis id (persisted
with the session beside the REC_LDM_LAYOUTS layouts record), and every
content consumer resolves it — recovery enqueues route the SafeWriter's
reads through the session's (lazily re-armed) virtual volume devices,
and preview/thumbnail open the same device, so preview bytes == recovery
bytes == the bytes the scan read. The re-arm resolves member disks by
their PRIVHEAD GUIDs: on Windows every disk is enumerated; elsewhere
only the scan device itself resolves (a cross-disk member reads fail
honestly per-extent — the documented missing-member semantics, never
wrong bytes from a wrong disk). The destination-overlap refusal maps
LDM-basis extents to their PHYSICAL member spans before comparing
against the destination (mirrored/RAID-5 spans are conservative: every
copy/column — any may be read). Snapshots carry their source
session/scan pointer for the same routing; the source must outlive
recoveries enqueued from it (the borrowed-nodes contract the writer's
queue already carries).

**Recovery-progress mapping (N-075 / D-78).** The og's typed
`IUIProgressInfo` per-item recovery-progress interface
(`RecoverProgressUISink::sigUpdateProgress(const IUIProgressInfo*)` —
corpus UIUX/20-moc-surface-sink-architecture.md :145) maps onto surfaces
this ABI already ships, deliberately (D-78: no parallel interface):
`cr_ui_recovery_progress_fn` is the progress edge (cumulative bytes
done/total across the queue), `cr_ui_recovery_queue_count` the live queue
depth, `cr_ui_recovery_get_stats` the per-outcome counters (done, refused,
errors, hash_mismatch, resumed, salvaged, dirs, nothing_to_recover), and
`cr_ui_recovery_report_json` the per-item truth (every item reports its
state — recovered/pending/failed-with-reason — valid at any time, not
just at completion). A host porting the og's per-item progress reads
report_json on its own cadence, exactly as the og's 38 timer consumers
poll `onGet*Progress`.

Cancellation semantics: `cancel()` makes `run()`/`process()` return
`ERR_CANCELLED` with all completed work intact (tree browsable, committed
items committed, report truthful about the rest). Save-on-cancel is the
caller's explicit choice (`cr_ui_scan_save_session` works on a cancelled
scan) — matching the CLI's `--out` policy.

**Session-save contract (N-060).** The save is ATOMIC (staging
`<path>.tmp`, commit-by-rename): a torn save (cancel, failure, crash
between truncate and finalize) never destroys the previous good session
file at the final path; the staging file is removed on cancel/failure.
While the session serializes, the library emits `CR_EVT_SAVE_PROGRESS`
(coalesced: first tick, permille steps, terminal — fired from the thread
running `cr_ui_scan_save_session`). `cr_ui_scan_save_cancel` sets the
SAVE-SCOPED cancel flag — the og's per-child `IUIProgressInfo::
IsCancelled` teardown (rsf-format--scan-session.md flow section: checked
after every child, writer @186644-186674), which is the save progress
object's OWN state, not the scan flag — so save-on-scan-cancel is
unaffected. The aborted save returns `CR_UI_ERR_CANCELLED`; the final
path keeps its previous content. The CLI's save path mirrors the atomic
discipline (`<path>.tmp` + rename).

## 6. Typed event stream (GAP-S6-10)

**Corpus lineage.** The reference pipes engine events to the UI through a
typed message hub: engine callbacks → `INotifyObserver::Notify` → the
`CMessageProxy` bridge (a QObject+INotifyObserver multiple-inheritance
singleton whose dual connect marshals cross-thread emissions onto the UI
thread — the wiring graph's only true thread-marshaling adapter, corpus
`mom-modules--ui-bridge-and-dependency-matrix.md` §4) → `CDetailPage::
doDispatchMessage(int msgType, void* payload, int& ref)` fan-out to page
components (`ScaningRecoverMessage` status text, the `AnalysisGridWidget`
sector map, file-count milestone telemetry; corpus `13-ui-core-wiring-state.md`
§3).

Our equivalent: a **flat typed event callback** on the scan handle. The event
is a POD; the payload is embedded (no pointers to free); delivery is
"at-least-once per stage transition, latest-wins per sample" — a transport
bridge (streaming channel, queue, FFI pipe) can serialize the struct
byte-for-byte. The core does NOT assume a transport; it emits the clean
typed stream.

```c
#define CR_EVT_SCAN_STAGE       1u  /* stage transition */
#define CR_EVT_SCAN_PROGRESS    2u  /* periodic progress sample */
#define CR_EVT_FILES_FOUND      3u  /* files-found counters changed */
#define CR_EVT_ERROR_RANGE      4u  /* read-failure range appended (delta) */
#define CR_EVT_CHECKPOINT       5u  /* a checkpoint was written */
#define CR_EVT_LOST_CANDIDATE   6u  /* lost-partition candidate found */
#define CR_EVT_REPAIR           7u  /* pre-scan repair lifecycle (S-008) */
#define CR_EVT_HEARTBEAT        8u  /* N-043 liveness heartbeat (5 s watchdog) */
#define CR_EVT_SAVE_PROGRESS    9u  /* N-060 save-path progress (coalesced) */
#define CR_EVT_FS_CHECK         10u /* M-041 pre-scan damage-audit phase done (fired ONCE per scan; check_errors = reported issue count 0x1001..0x100e, check_family = detected family byte; the report NEVER aborts the scan) */
#define CR_EVT_AUTOSAVED 12u /* W1c (R-52) completion auto-save outcome (fired ONCE after run() completes with ScanConfig.autosave_dir set; autosave_rc = 0 saved or the write error; a cancelled run never auto-saves) */
#define CR_EVT_SESSION_LOAD_PROGRESS 11u /* W1b (R-54) load-path progress: coalesced (first tick, permille steps, terminal) while cr_ui_session_load_with_events deserializes; bytes_done/bytes_total = nodes deserialized / persisted node count; cancel via the CALLER-OWNED cancel_flag -> CR_UI_ERR_CANCELLED, no session out (the og's per-child IsCancelled restore teardown) */

typedef struct cr_ui_event {
    uint32_t type;            /* CR_EVT_* above */
    uint32_t stage;           /* 0 quick, 1 deep, 2 carve, 0xFFFFFFFF none */
    uint64_t timestamp_ms;    /* monotonic, pause-compensated */
    /* progress sample (CR_EVT_SCAN_PROGRESS) */
    uint64_t bytes_done, bytes_total;   /* stage-true bytes where counted */
    uint32_t stage_permille, overall_permille;
    /* counters (CR_EVT_FILES_FOUND; cumulative, not deltas) */
    uint64_t files_found, deleted_found, carved_found;
    /* error-map delta (CR_EVT_ERROR_RANGE): the newly-failed span */
    uint64_t err_start_bytes, err_len_bytes;
    uint32_t error_ranges;             /* cumulative count after append */
    /* checkpoint (CR_EVT_CHECKPOINT) */
    uint64_t checkpoint_index;
    /* lost candidate (CR_EVT_LOST_CANDIDATE) */
    uint64_t candidate_lba;
    /* pre-scan repair (CR_EVT_REPAIR; v1-era append - no 1.2 marker ever
       shipped, section 9 - the former reserved
     * slots, layout unchanged): planned/applied action count + outcome
     * (0 plan ready for consent, 1 applied, 2 consent denied,
     * 3 failed or writer unavailable). stage stays 0xFFFFFFFF. */
    uint32_t repair_actions;
    uint32_t repair_outcome;
    /* error context (CR_EVT_ERROR_RANGE; M-049 v2-era append): the
     * module/operation/retry-state triple - the engine-side subset of
     * the og's error-object architecture (per-module registrars; the
     * MOM-bus manager/track-manager layer stays out of core scope).
     * Module ids: CR_ERRMOD_* below (per-range attribution recorded at
     * failure time - WHO was reading when the sector died; kNone for
     * ranges reloaded from pre-M-049 sessions). The trailing pad keeps
     * the POD an 8-byte multiple (transport bridges); always zero. */
    uint32_t err_module;
    uint32_t err_operation;  /* CR_ERROP_* (READ for this family) */
    uint32_t err_attempts;   /* retries spent before the range was given up */
    uint32_t err_context_pad;
    /* v2-era append (M-041, CR_EVT_FS_CHECK): the damage-audit phase's
       summary - reported issue count (the 0x1001..0x100e set; full
       detail rides the session record + stats) and the detected
       family byte ('f' fat / 'n' ntfs / 0 undetected). stage stays
       0xFFFFFFFF (pre-scan). check_pad keeps the POD an 8-byte
       multiple; always zero. */
    uint64_t check_errors;
    uint32_t check_family;
    uint32_t check_pad;
    /* (CR_EVT_HEARTBEAT reuses the payload fields above: the standing
       permille/counters + timestamp_ms = elapsed_ms; stage = 0xFFFFFFFF
       when no stage is in flight; bytes_total = 0 = unknown.) */
} cr_ui_event;

typedef void (*cr_ui_event_fn)(void* user, const cr_ui_event* evt);

CR_UI_API uint32_t cr_ui_scan_set_event_fn(cr_ui_scan* s, cr_ui_event_fn fn,
                                           void* user);
```

Error-context constants (M-049; `CR_EVT_ERROR_RANGE` payload) — the
module ids mirror the core's `cr_errmod.h` attribution set (engine-side
subset of the og's per-module error registrars, Error.md census); the
numeric values are core-internal per the D-68 value-space policy,
append-only, pinned to the core enum by compile-time asserts in
cr_ui.cpp:

```c
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
#define CR_ERROP_NONE 0u
#define CR_ERROP_READ 1u
```

Contract:
- Fired on engine worker threads, the thread running `run()`, or the
  shim's heartbeat watchdog thread (all three are possible — the engine's
  own scan workers emit stage/sample events; the header's rule is
  authoritative). Must not block (same rule as the
  progress callback). No cancellation semantics — cancel via the token
  functions.
- `CR_EVT_HEARTBEAT` (N-043, the og's 5 s liveness watchdog — corpus
  DRWUI_decompiled.c:910565-910585, CreateThread :910864 — in the HONEST
  variant) fires from the shim's watchdog thread every 5000 ms while
  `run()` is in flight and no `CR_EVT_SCAN_PROGRESS` arrived in the
  window (a pause-parked or I/O-stalled scan). It carries the STANDING
  telemetry truth — permille NEVER advances without work (the og's
  watchdog bumps its coarse counter +0xe8 instead, fabricating movement;
  that is the registered divergence). A host distinguishes "alive,
  stalled" from "hung" without ever being shown fake progress.
- Ordering: stage transitions are delivered exactly once in scan order;
  progress/files/error events are samples (coalesced to the same cadence the
  engine progress uses — per work unit batch, never per byte).
- `CR_EVT_ERROR_RANGE` carries the DELTA span (what just failed); the
  cumulative count rides the same event. The full map is queryable via
  telemetry + the session (ReadErrorMap is serialized — `cr_device.h`).
- Zero-init `cr_ui_event` is invalid (type is mandatory); unknown `type`
  values MUST be ignored by consumers (forward compatibility).
- `CR_EVT_REPAIR` (ABI-since 1.2) fires only when
  `cr_ui_scan_config.pre_scan_repair` was set: once with outcome 0 when
  the pre-scan analysis produced a non-empty non-refused plan (the consent
  callback decides), and once more with the final outcome (applied /
  denied / failed). It NEVER carries a pointer; the plan summary reaches
  the UI through the `cr_ui_repair_consent_fn` callback (see the header's
  S-008 block), and the transactional result through
  `cr_ui_scan_stats` + `cr_ui_scan_repair_save_undo`.
- Pre-scan repair SAFETY (S-010): the engine acquires the write device's
  exclusive volume lock BEFORE any repair write (the corpus RecoveryModule
  sequence: FSCTL_LOCK_VOLUME -> DeleteVolumeMountPoint -> FlushFileBuffers ->
  settle -> verify-mounted -> FSCTL_DISMOUNT_VOLUME, per mounted volume on the
  repaired disk) and releases it when the writer closes. Concurrent access is
  REFUSED (the refusal surfaces through the repair outcome), never raced; a
  refused lock means zero writes. Image-file devices (tests, offline images)
  have no OS volume to lock.

## 7. Disk-map geometry (GAP-S6-11) — the visualization data model

**Corpus lineage.** The reference renders a sector-grid disk map during the
scan (`AnalysisGridWidget`, 54 v-methods, RTTI-verified — corpus
`08-scan-phase-deep-dive.md` §1) fed by the same engine events (§6) and the
engines' shared `IBuildVolumeBitmap` interface ("which clusters are
free/used" — corpus `searchdll-family--architecture.md` §2). The widget is
UI-side; the DATA MODEL is what core must produce: geometry + per-cell
allocation state + scan coverage + confidence grading + bad-sector overlay.

Our surface (session 6, GAP-S6-11): a POD geometry + per-cell byte arrays,
computable from a loaded session OR a live snapshot:

```c
typedef struct cr_ui_disk_map_geometry {
    uint64_t start_byte;       /* slice/volume base (absolute on disk) */
    uint64_t total_bytes;      /* map extent */
    uint64_t cell_bytes;       /* granularity: cluster size, or a power-of-2
                                  bucket when cluster math is unavailable */
    uint64_t cell_count;       /* ceil(total/cell) — the array lengths below */
    uint32_t cluster_bytes;    /* 0 when the engine had no cluster model */
    uint32_t fs_family;        /* orchestrator::FsFamily value */
} cr_ui_disk_map_geometry;

/* Per-cell overlays, one byte per cell, allocated by the library: */
/*  allocation: 0 unknown, 1 used (active structured files), 2 free */
/*              (S-009: engine bitmap truth - NTFS $Bitmap / FAT /    */
/*               exFAT table; never overriding used or bad),           */
/*              3 bad (retry-layer error spans - NOT FS-marked)        */
/*  coverage:   0 unscanned, 1 scanned */
/*  found:      0 none, else count of found-file cells capped at 255 */
/*  confidence: 0 none, else the max Confidence ordinal (0..4) of content
                whose extents cover the cell, capped at 4 */
CR_UI_API uint32_t cr_ui_disk_map(cr_ui_session* sess, cr_ui_snapshot* snap,
                                  const cr_ui_disk_map_geometry* want,
                                  cr_ui_disk_map_geometry* got,
                                  uint8_t** allocation, uint8_t** coverage,
                                  uint8_t** found, uint8_t** confidence,
                                  uint64_t* bad_ranges, uint32_t bad_cap,
                                  uint32_t* bad_count);
CR_UI_API void cr_ui_disk_map_free(uint8_t* overlay);
```

Semantics:
- `want->cell_bytes == 0` → the engine's cluster size (or 64 KiB default).
- The four overlays are independent arrays (a consumer may pass NULL for any
  it does not want; at least one must be non-NULL).
- `bad_ranges` are [start,len] u64 pairs in ABSOLUTE disk bytes — the
  ReadErrorMap spans, the same truth the error-map serialization carries.
- The map is a DATA PRODUCER: no rendering, no colors, no UI semantics. The
  consumer paints (the og's grid animates scan coverage — corpus UIUX/04
  §110).
- Memory bound: cell_count is capped so total overlay bytes stay ≤ 64 MiB
  (cell_bytes grows by powers of two to honor the cap); `got` reports the
  actual granularity chosen.

## 7a. Carve-format registry (ops #7-#8 parity)

**Corpus lineage.** The reference exposes its RAW analyser registry to the UI
through RAWSearchDLL exports — `EUGetAnalyserList` / `EUGetAnalyserMap` /
`EUGetAnalyserMultimap` — and the per-extension preference table through
`GetRawUserInfoMap` (tied to `config/RawConfig.ini`, 34 shipped toggles;
corpus `rawsearch-carving--deep-dive.md` §1; `config-files/filtering-priority--config-layer.md` §4).
A host uses these to build format-config surfaces and to know what the carve
stages can surface.

Our equivalent (core-api session, added-export rule §9):

```c
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

CR_UI_API uint32_t cr_ui_carve_format_count(void);
CR_UI_API uint32_t cr_ui_carve_format_info(uint32_t index, cr_ui_carve_format* out);
```

Contract:
- No handle, no lifecycle: the registry is compile-time immutable; both
  functions are safe from any thread, any time (including before any other
  call — no init ordering).
- Deterministic order: grammar-registry rows first (FormatId order), then
  raw-sign rows (`kFormats` order). Both tables only ever APPEND, so indices
  are stable across releases; a consumer may persist them.
- `cr_ui_carve_format_info` returns `ERR_BAD_PARAMETER` on out-of-range
  index or null `out`; the out-of-range message carries the valid count.
- `base_confidence` for raw-sign rows is the format's STRONGEST header-sign
  confidence (the table stores confidence per sign; the max is the honest
  per-format projection).
- The og's RawConfig.ini per-format carve TOGGLES are host-side policy: the
  core carves the full registry (matching the og engine, whose toggles are
  applied by its UI when it configures the searcher); a host filters this
  enumeration to build the same config surface.
- **CLI projection (N-033's close):** `rrecov list-carve-formats` prints the
  full enumeration (one row per format: layer, ext, min/max, base
  confidence, grammar magic) — the CLI shape of the og host's
  EUGetAnalyserList-driven "formats we can recover" surface. Exit 0 only on
  a complete walk; an empty registry or a failed row is exit 1 (the FB-K01
  family: scripts must never mistake a broken table for "nothing to
  carve").
- The og's FS engines re-clone the RAWSearchDLL registry per scan
  (FATSearchDLL :13872-13977 LoadLibrary/GetProcAddress + the per-scan
  re-fetch :29290-29361); the core's registry is compile-time immutable
  and read directly — PARITY-DELTAS D-100 registers the difference (same
  content by construction; the clone pattern exists only because the og
  is multi-DLL).

- Per-scan refresh divergence (N-033, registered): the og's FS engines
  RE-CLONE the RAWSearchDLL registry per scan
  (FATSearchDLL_decompiled.c :29290-29361 — LoadLibrary +
  GetProcAddress + std-container copy on every search) because the master
  registry lives in another DLL and may have been reconfigured between
  scans. The core's registry is compile-time immutable (both tables
  append-only, §7a), so there is nothing to re-fetch: the enumeration is
  valid for the library's lifetime and no refresh hook exists. If a host
  ever ships runtime-configurable carve tables, the refresh seam becomes
  a versioned ABI question (a bumped count between calls), not a silent
  re-clone.
- CLI rendering: `rrecov --list-carve-formats` prints the whole table
  (layer, ext, size bounds, base confidence, grammar-layer magic) — the
  parity-tooling surface (N-033's step 2).

### 7f. The session-list surface (W1a / R-56; PLAN-API-EXPANSION Wave 1; N-090 scope/width registration)

`cr_ui_session_list(const char* utf8_dir, cr_ui_session_row* rows, uint32_t* count)`
(cr_ui.h:865-914) — the og's LastScanRecordView surface
(UIUX/07-home-phase-deep-dive.md:21 + the Continue-last banner): a
directory-level summary listing where every `*.crsf` file yields ONE row
decoded from the container's LIGHT records only (`SESSION_STATE` +
`VOLUME_INFO`; the heavy payloads — the 36 MB-class tree record among them
— are skipped BY TYPE before any payload I/O:
`orchestrator::ScanController::read_summary` + `session::Reader::
peek_record`, the same light-read shape as the og's record-2 read,
rsf-format--scan-session.md §5.2).

Contract (ABI RULES-conformant):

- **Two-call size pattern** (same as `cr_ui_snapshot_dfs`): `rows == NULL`
  → `*count` = the number of `*.crsf` candidates, nothing decoded;
  `rows != NULL` → `*count` is the IN capacity, rows fill NEWEST-FIRST by
  the embedded save time (the og's LastScanRecordView order — its record-2
  SYSTEMTIME comparator FUN_140273550 sorts newest first, which is what
  makes the og's "Continue last scanning" banner pick the right file);
  equal stamps and zero-stamp rows (pre-v5 sessions, unreadable files)
  break ties by path bytes (deterministic), capacity < needed →
  `CR_UI_ERR_TRUNCATED` with `*count` = the number filled (the
  time-ordered prefix — the NEWEST rows). Caller-owned memory — rows are
  PODs, no free export.
- **Row truth**: `valid=0` + `error` = the decode/open error for unreadable
  files (bad magic, torn container) — a listing never hides damage; a
  `valid=1` row carries `error=CR_UI_OK` unless the PATH itself needed
  truncation (≥ `sizeof(path)` bytes) → `error=CR_UI_ERR_TRUNCATED`, the
  1023-byte prefix in `path[]`, summary intact.
- **Fields**: `fs_family`/`stage_done_mask`/`configured_mask`/counts/
  `label` mirror `cr_ui_session_get_info` + `cr_ui_session_stats` exactly
  (the C-consumer gate cross-checks); `save_unix` is the EMBEDDED save
  time (the v5 VOLUME_INFO tail; 0 = a pre-v5 session — display the file
  mtime then, never a fabricated time). N-090 registrations: (a)
  **SCOPE** — the listing enumerates EVERY `*.crsf` in the directory,
  the REGISTERED SUPERSET over the og's home list (the og keeps only
  names containing BOTH `auto_` and `.rsf` — FUN_140273940 at
  DRWUI:432326-432333 + rsf-format §5.5; PARITY-DELTAS D-129 [renumbered from the collided D-135 at N-207]): a manual
  save is a resumable session too, and a host wanting the og's exact
  home view filters client-side by name; (b) **LABEL WIDTH** —
  `label_full[100]` (the ABI tail append, no version bump per §9)
  carries the COMPLETE volume label at the og's record-2
  `volumeName[100]` width (strncpy(…,99)); `label[32]` stays the
  legacy 31-char prefix view so old callers see exactly what they
  always saw; `label_full == label` on v1..v5 sessions (the uniform
  degrade); (c) **SAVE-TIME RESOLUTION** — `save_unix` carries SECONDS
  (the v5 tail contract); the og's record-2 carries local SYSTEMTIME
  with milliseconds — the newest-first comparator needs second
  resolution only (the og's own auto-save FILE NAMES are
  second-granular: `auto_..._<HH>_<MM>_<SS>.rsf`), and equal-second
  ties break by path bytes deterministically (N-116); (d)
  **DEVICE TYPE** — the og's record-2 `deviceType` u32 (2 → "Desktop",
  3 → "Recycle Bin", else the disk-info name at +0x120 — a UI display
  classification) is NOT carried on this row; a host needing it
  composes it client-side from its own disk enumeration (no core
  consumer exists today; re-open with evidence when one does).
- **Errors**: `CR_UI_ERR_BAD_PARAMETER` (null/empty args),
  `ERR_OPEN_FAILED` (the directory itself cannot be read — fail loud,
  never a silent empty list).
- **Threading**: no shared state; any thread may call. No handle is
  created — nothing to close.
- **ABI stability**: append-only export + new POD struct = NO version
  bump (§9 policy; `CR_UI_ABI_VERSION` stays 3).
- **CLI rendering**: `rrecov sessions [--dir D] [--json]` (the three-site
  verb-list pin governs its doc sites).

### 7g. The session load progress/cancel surface (W1b / R-54; PLAN-API-EXPANSION Wave 1)

`cr_ui_session_load_with_events(const char* utf8_path, cr_ui_event_fn fn, void* user,
volatile uint32_t* cancel_flag, cr_ui_session** out)` (cr_ui.h:926-944, the
comment block + the decl at :943-944 — N-135: the old 916-944 range was 9 lines
loose at the audit tree and drifted further since; re-pinned at the current
tree) — the
load-side twin of the N-060 save machinery. The og's RESTORE reports
progress inside the load window (`40+50*done/(total+done)`,
rsf-format--scan-session.md flow section) and polls `IUIProgressInfo::
IsCancelled` after EVERY child, tearing down on cancel; our mirror:

- **Events**: `fn` (optional) receives `CR_EVT_SESSION_LOAD_PROGRESS`
  (type 11) from the CALLING thread — coalesced (first tick, permille
  steps, terminal — the save event cadence). `stage = 0xFFFFFFFF`;
  `stage_permille` = the load's own 0..1000; `overall_permille =
  0xFFFFFFFF` (no scan context); `bytes_done/bytes_total` = nodes
  deserialized / the persisted node count.
- **Cancel**: `cancel_flag` (optional, CALLER-OWNED, zero-init safe) is
  polled per node — EVEN WHEN `fn` is NULL (a silent host can still
  cancel, the same rule as the save side). Any thread may set it
  nonzero; the load aborts with `CR_UI_ERR_CANCELLED` and `*out` is
  NEVER touched (a torn load never half-claims state — the load-side
  twin of the save-side "a torn save never destroys the previous good
  file").
- **Degenerate contract**: with `fn = NULL` and `cancel_flag = NULL`
  this is exactly the plain `cr_ui_session_load`.
- **Core seam**: `session::DeserializeProgress` (cr_records.h:47-62),
  threaded through `load_session` / `load_session_summary` / `resume`;
  ERR_CANCELLED propagates from `deserialize_tree` through the
  container layer (never flattened into ERR_SESSION_FORMAT).
- **ABI stability**: append-only export + append-only event type = NO
  version bump (§9; `CR_UI_ABI_VERSION` stays 3). No CLI surface (the
  og has no CLI; load progress is a host concern).

### 7h. The completion auto-save (W1c / R-52; PLAN-API-EXPANSION Wave 1)

The og's completion SaveScan flow (rsf-format--scan-session.md section 5.5 +
UIUX/30:64-67): on run() COMPLETION the session is auto-saved to
`ScanConfig.autosave_dir` (the ABI field `cr_ui_scan_config::
autosave_dir_utf8`, zero-init = OFF) under `session::auto_session_name` (the
N-061 corpus naming `auto_<label>_<M>_<D>_<H>_<m>_<s>` with the
name-forbidden-char sanitizer) + `.crsf`. The write rides the shared atomic
path (`write_session_atomic`: tmp + finalize + rename - the N-014
discipline; a torn auto-save never destroys a previous good file).

Contract:
- A CANCELLED run never auto-saves (the og's flow is the completion
  window; save-on-cancel is the explicit `cr_ui_scan_save_session`
  surface).
- Surfacing: ONE `CR_EVT_AUTOSAVED` (type 12) event with
  `autosave_rc` (0 = saved, else the write error code - never a silent
  failure) and `cr_ui_scan_stats.autosaved` (0/1, live runs; loaded
  sessions honestly report 0).
- Pure core owns no file I/O: the WRITE is host-side by architecture -
  the cr_ui shim honors the field on run() completion; the CLI's
  `scan --autosave-dir D` sets the same core field and runs its own
  completion leg through its existing save path (the shared naming
  truth is core's `session::auto_session_name`).
- ABI stability: appended config field + appended stats field + appended
  event type + appended event-struct tail = NO version bump (§9).

### 7i. The thumbnail surface (M-102; M-083's grade promotion)

The og's results page fetches a 256-px thumbnail per image row through a
dedicated worker (UIUX/10 §2; the GDI+ width-derived 256-px convention,
DRWUI :652395-652410). This is the engine's equivalent: one bounded,
synchronous generation per node (`cr_ui_thumbnail_generate`), display-
oriented RGBA8 output (S-043: the EXIF orientation transform runs before
the downscale), the format probe (`cr_ui_thumbnail_format_supported`).

Contract:
- TWO-CALL pattern: `rgba == NULL` is the size probe — the generation
  runs (output dims depend on the decode), `cr_ui_thumbnail_info::
  bytes_needed` carries the answer, pixels are discarded. A too-small
  buffer returns 0x50030009 ERR_NO_MEMORY with `bytes_needed` set and
  nothing partially written.
- Caps: `NULL` (or any zero field) = the engine default for that field;
  nonzero values ride the engine's own validation. Defaults: max_edge
  256, max_pixels 64 MP, max_read 200 MB (0xC800000, M-109), max_work 1<<22,
  allow_embedded ON.
- Truncation is DATA, not an error: the engine's Ok-but-truncated
  contract crosses the ABI verbatim via `info.truncated` (stb's JPEG
  zero-fill leniency is the one Ok-with-short-input family; the strict
  decoders' Ok proves completeness — the post-parse EOF probe is parse
  noise, never reported).
- Status mapping is verbatim the engine's names onto the shared numeric
  space: Unsupported (the named phase-2 families — `info.source_format`
  carries the family), DecodeFailed → 0x50030004 ERR_TRUNCATED,
  UnreadableSpan → ERR_READ_FAILED, the caps refusals → ERR_UNSUPPORTED.
- M-083 (the og's grading side-effect, FUN_1402636e0 :422498-518): a
  successful generation promotes the node's grade to 2 ("high") — never
  downgrades, no-op for already-high nodes (the node setter's
  monotonic-max + sticky-2 semantics are the og's own, FUN_140089630).
  Failed generations never promote. The promotion is observable through
  `cr_ui_query_run`'s `grade` row field and persists through session
  save/load (the S-023 serialization).
- No version bump: appended exports + new structs, no existing shape
  touched (§9).

### 7j. The image-create producer (S-040 / R-93; PLAN-API-EXPANSION Wave 4)

The og home page's "Create Image" flow (UIUX/07:36-37 + UIUX/06:27;
the imaging capability lives in ToolsModule.dll, mom-module-system :87)
and the product's damage posture - scan the produced image instead of
the live disk (UIUX/11:17). The reader half is S-030
(`cr_ui_open_image`); this is the producer the core wholly lacked.

Contract:
- `cr_ui_image_create(disk, utf8_path, progress, user)` — one-shot,
  class (a) driver-required (DRIVER-REQUIRED.md): a non-Ready driver
  state is a classified ERR_DRIVER_* refusal, the same gate as
  `cr_ui_open_disk`.
- Progress: the recovery-style cumulative-bytes callback (bytes done /
  bytes total, once per 64 KiB chunk); may be NULL.
- Truth contract (the finding's acceptance): streamed to
  `<path>.crimg-tmp`, atomically renamed on completion, plus the
  `<path>.sha256` sidecar (the standard `<hex>  <basename>` form). On
  cancel or failure the partial image is KEPT as `<path>.partial` -
  reported through the return code, never silently passed off as
  complete. A sidecar write failure is reported while the image itself
  stays committed.
- Flat raw only: VHD/VHDX writers are out of scope (registered
  divergence D-147); the og's own reader accepts flat and the corpus
  does not decode the og's writer format beyond the libvmdk link.
- The copy loop is portable core (`storage::create_image_stream`,
  cr_image.cpp: 64 KiB envelope reads, SHA-256 over the stream, cancel
  at chunk heads); the platform layer owns the file disposition
  (`win::create_image_file`) and the device open (`win::create_disk_image`
  over `win::open_disk`). Non-Windows lanes return ERR_UNSUPPORTED
  (the honest platform answer).
- No version bump: one appended export, no existing shape touched (§9).

## 8. SQLite session index — REJECTED-BY-REFERENCE (closed GAP-S6-12)

**This section previously specified a queryable SQLite sidecar index
(`<session>.crsf` → `<session>.crsidx`) and claimed `cr_ui_scan_save_session`
wrote it. That feature was never implemented (zero sqlite references in
src/), and — decisive for the parity mandate — the reference has no such
surface either: the og's durable session store IS the `.rsf` container
(records 1-4, chunked zlib; corpus `rsf-format--scan-session.md` §2-§5);
there is no sidecar index anywhere in the reference's corpus.** Per the
mandate ("do not invent operations the reference does not have";
"removing an invented path the reference does not have is required"), the
claim is cut and GAP-S6-12 is closed REJECTED-BY-REFERENCE (corpus cite
above). Query/filter/sort/paging answers from the in-memory session tree
(`cr_ui_query_run`), exactly as shipped; sessions persist as CRSF only.
No ABI surface ever referenced the sidecar, so this is a documentation-only
closure — no consumer impact.

## 9. Stability policy

- `CR_UI_ABI_VERSION` is `3` (v3 = M-067, the S-039 convergence residue: the `cr_ui_user_sign` field resize — `bytes` 16 → 48, `mime` 64 → 81, `sign_len` 1..16 → 1..48, `offset` bounded 0..1024 — the first struct FIELD change under this policy; hosts must recompile against v3 or `cr_ui_user_sign_info` would leave a v2 host's `mime[]` non-NUL-terminated on a 73-char MIME (the S-039 caps fix landed first at `b07ee9d` without this half — the two halves converged this session). v2 = M-036: six appended `cr_ui_smart_report` fields — host_reads, host_writes, controller_busy_time, num_error_log_entries, data_bytes_read, data_bytes_written — at the struct tail; every pre-v2 field and offset unchanged). It bumps on ANY of: removing/renaming an
  export, changing a struct FIELD (type/meaning/order — an in-struct array
  resize included, which is why v3 exists), changing ownership
  or threading contracts of an existing symbol, renumbering the error-space
  symbols a consumer must handle.
- ADDING exports, APPENDING struct fields, adding event `type` values,
  adding overlay kinds: **no bump REQUIRED** — consumers must already ignore
  unknowns (§6/§7 rules). The carve-format registry (§7a) is an added
  export under this rule. A bump is still PERMITTED as a deliberate
  feature-batch marker when a consumer-probeable presence signal is worth
  it: v2 is exactly that (M-036's six fields could have shipped without a
  bump under the append rule; the bump was issued so a host can branch on
  `cr_ui_abi_version() >= 2` instead of probing field behavior). No other
  bump has occurred: there is no v1.1 or v1.2 — the historical
  "ABI-since 1.1/1.2" header annotations were doc defects (N-040 D4)
  and have been rewritten to the truthful era labels ("v1-era append
  with the change id"; every such field shipped while the macro read `1`
  and is present in all v2 builds).
- **The header's ABI-VERSIONING comment was corrected in the core-api
  session to match this section** (it previously claimed new exports and
  appended fields bump the version — contradicting both this section and
  the repo's actual practice of append-only growth; see
  docs/API-SURFACE-AUDIT.md A-ABI-1).
- Structs are never reordered; new fields append; the header comments each
  appended field with its landing (change id + era), never a version
  number that was never shipped.
- The C consumer test (`tests/ui_abi_check.c`, MSVC `/W4 /WX` on the Windows
  lane) exercises the full lifecycle on every push — it is the executable
  form of this contract.
- Versioning of the shared library follows semver at the PROJECT level; the
  ABI version is the compatibility unit the UI team compiles against.

## 10. What this ABI deliberately does NOT do

- No UI logic in the shim: no widgets, no strings beyond diagnostics and
  report/JSON outputs, no policy beyond the documented safety gates
  (destination-overlap refusal F12, BitLocker legitimate-unlock-only F13,
  RawFixer consent).
- No subprocess spawning: the UI binds in-process exactly like the og's
  factory surface (the CLI is a peer consumer, not the bus).
- No remote/cloud/NAS surfaces (standing scope exclusion, roadmap).
- No cancellation through callbacks: the reference separates progress events
  from its scan-abort event; we keep the same separation (§6).

### Query paging + the Type sort (M-029, session 11)

The `cr_ui_query` paging model (offset + limit, default 100) is a HOST-side
convenience: the reference has NO page-size constant — its results view
virtualizes at the UI layer (~1000-item incremental tree batches,
`sigResetStartIdx`; corpus DRWUI decode). The core claims no paging parity
against it; this is a registered deliberate difference. Sort code 7 (Type)
mirrors the og's results-view Type column: category in the FileFilter.xml
iconIndex order (Graphic/Document/Email/Audio/Video/Other/Archive/
Bookmarks, Unsaved - N-126's tenth bit: the og's office unsaved-document
category, the .asd/.wbk autosave family), then extension, then name.

### Query bucket translation + the raw-category name (M-029, session 32)

`cr_ui_query_apply_buckets` (added export, §9 no-bump) translates the og
filter-bar's stable bucket IDs — the exact action-ID set the DRWUI menus
build (CTimeFilterBtn/CSizeFilterBtn): Date 0x100/0x101/0x102/0x103 +
0x105 Custom, Size 0x1000..0x1004 + 0x1006 Custom — into the query's
`modified_from/to` and `size_min/max` range fields, in place. Date
windows are day-truncated LOCAL windows derived from the host clock
exactly like the og's cached bounds (today / yesterday / (today−7d,
today] / (today−30d, today], COleDateTimeSpan day arithmetic); Custom is
inclusive on local-day-truncated bounds. Size buckets use the og's exact
byte boundaries (≤512 KiB, ≤1 MiB, ≤512 MiB, ≤1 GiB, >1 GiB); Custom is
inclusive. The SKIPPED IDs (0x104, 0x1005) and any unknown value are
rejected with `CR_UI_ERR_BAD_PARAMETER` — the core never invents menu
semantics the og does not have.

`cr_ui_scan_config.raw_category_name_utf8` (tail append) is the og's
per-scan "filter_generalraw" display-name surface (corpus
s5-oq-s4-2-raw-naming-hook.md): the raw/carve category root's role name.
NULL/"" = the default "Reconstructed" (S-048: the og's own root name -
DRWUI's type==7 branch; resume also recognizes the legacy "Other Deleted
Files (Carved)" so old sessions never duplicate the root). Resume recognizes
both the default and the configured name, so a re-run never spawns a
duplicate raw root.


### 7d. The user-signature registration surface (M-048; caps S-039, widths v3/M-067)

`cr_ui_register_user_signs(json, len)` installs the og's EUUserData-shaped
signature database atomically (parse + validate + install; a bad payload
leaves the previous set). `cr_ui_user_sign_count/info` enumerate the
active set. Registered signs ride every carve pass at the raw-sign
stride, after the built-in layers (built-in-wins). The tag space for
user-sign records is 0xFFFE0000 (distinct from raw-sign 0xFFFF0000+fmt).
Re-registration replaces — the og's config-reload posture (EUUserData is
re-read per scan, never session state).

**S-039 (caps)**: the accepted grammar grew to the og's own shipped file's
extremes — signs up to 48 bytes (og max 44), up to 12 signs per entry
(og max 11), offsets up to 1024 (og max 526), MIME up to 80 chars (og max
73). The og's actual 111-entry EUUserData parses whole (190 signs); the
pre-fix caps rejected 15 of its 111 entries atomically (zero loaded).

**v3 (M-067, the convergence residue)**: `cr_ui_user_sign` carries the
full widths since v3 — `bytes[48]`, `mime[81]`, `sign_len` 1..48, `offset`
0..1024. The caps fix alone (v2 struct) left `mime[64]` receiving a
memcpy of a 73-char MIME with no terminator inside the field — a host
string read ran past it; the v3 resize is the completing half.

### 7e. The transport-open diagnostic (M-030; driver-only since PLAN-DRIVER-ONLY)

`cr_ui_last_transport_error()` reports WHY the last `cr_ui_open_disk` /
`cr_ui_open_volume`-family call ended as it did. The advisory is
process-global, advisory-only (never gates an open's own return code),
never cleared by reads — the open calls' own return codes stay
authoritative:

- `0` — the last open succeeded (or no open was attempted yet; the
  unsupported-platform stub records the clean state).
- `ERR_DRIVER_*` (`0x500B0001..0x500B0005`) — the last open refused at
  the driver-prerequisite gate: the driver state was not Ready
  (`absent` / `not_started` / `unauthorized` / `version_mismatch` /
  `errored`), and the returned code equals the advisory
  (`cr_ui.cpp` sets both from the same `driver_state_error()` value).
- Any other `cr_error` — the driver was **Ready** but the TARGET would
  not open (`ERR_OPEN_FAILED` etc.): a target error, never a silent
  transport switch. There is no fallback leg anymore — the driver-only
  architecture removed the RDA→direct fallback and the WinPE gate
  entirely (DRIVER-REQUIRED.md; PLAN-DRIVER-ONLY R4/R12), so the
  advisory never reports "the surviving leg" — it reports the one leg
  there is.
