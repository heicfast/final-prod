# ABI-CONTRACT — the `cr_ui` C ABI: ownership, threading, errors, events, stability

> **Status: normative.** This document is the contract for every consumer that
> binds `cr_ui.dll` / `libcr_ui.so` (G-01, issue #1). The header
> `include/cr/cr_ui.h` is the machine surface; this file is the human contract
> behind every symbol: ownership rules, threading rules, the error model, the
> typed event stream (§6), the disk-map geometry model (§7), the SQLite-backed
> session index (§8), and the stability policy (§9). Where a statement here and
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
| `cr_ui_device` | `cr_ui_open_image` / `cr_ui_open_disk` / `cr_ui_open_volume` | `cr_ui_device_close` | — |
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
  progress callbacks fire on the thread running `run()`/`process()`.

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
| 0x50090001 | CR_UI_ERR_INTERNAL | internal invariant failure (bug — report it) |

Diagnostics: `cr_ui_last_error()` returns thread-local text for the LAST
failing call on that thread; stable until the next `cr_ui_*` call on the same
thread. Never NULL.

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
recover:     recovery_create -> recovery_set_destination
             -> recovery_enqueue_[node|subtree][_snapshot]* -> recovery_process
             -> recovery_report_json -> recovery_close
events:      (any time after create) scan_set_event_fn — see §6
disk map:    session_disk_map / scan_disk_map — see §7
index:       scan_save_session also writes the sidecar index (see §8)
```

Cancellation semantics: `cancel()` makes `run()`/`process()` return
`ERR_CANCELLED` with all completed work intact (tree browsable, committed
items committed, report truthful about the rest). Save-on-cancel is the
caller's explicit choice (`cr_ui_scan_save_session` works on a cancelled
scan) — matching the CLI's `--out` policy.

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
    /* pre-scan repair (CR_EVT_REPAIR; ABI-since 1.2 - the former reserved
     * slots, layout unchanged): planned/applied action count + outcome
     * (0 plan ready for consent, 1 applied, 2 consent denied,
     * 3 failed or writer unavailable). stage stays 0xFFFFFFFF. */
    uint32_t repair_actions;
    uint32_t repair_outcome;
} cr_ui_event;

typedef void (*cr_ui_event_fn)(void* user, const cr_ui_event* evt);

CR_UI_API uint32_t cr_ui_scan_set_event_fn(cr_ui_scan* s, cr_ui_event_fn fn,
                                           void* user);
```

Contract:
- Fired on the thread running `run()` only. Must not block (same rule as the
  progress callback). No cancellation semantics — cancel via the token
  functions.
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
/*  allocation: 0 unknown, 1 used, 2 free, 3 bad (marked by the FS) */
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

## 8. SQLite session index (GAP-S6-12)

The durable scan-session format stays the CRSF container (D-01 — the .rsf
proven chunked-zlib scheme). The SQLite layer is a **queryable index
sidecar** written next to it (`<session>.crsf` → `<session>.crsidx`):

- One table per concern: `nodes` (node_id, parent_id, name, path, size,
  times, origin, deleted, confidence, category, extent_count), `extents`
  (node_id, ordinal, offset, len), `errors` (start, len), `session_info`
  (identity card + stats). Row shape mirrors the query surface.
- **Every connection** opens with `PRAGMA journal_mode=WAL`,
  `busy_timeout = 15000`, `synchronous = NORMAL`, `cache_size = -8000`
  (8 MiB bounded). Schema versioned (`user_version`); opening an older
  schema upgrades in place (append-only migrations).
- Query/filter/sort/paging for LOADED sessions is query-driven: the session
  tree hydrates into the index once, and `cr_ui_query_run` may answer from
  SQL (the in-memory tree remains the source of truth — the index is
  rebuildable at any time and never silently diverges: a checksum of row
  count + content hash is stored in `session_info`).
- The index is a sidecar, never a dependency: if the file is missing or
  corrupt, the session still loads and the index is rebuilt on demand.

## 9. Stability policy

- `CR_UI_ABI_VERSION` is `1`. It bumps on ANY of: removing/renaming an
  export, changing a struct FIELD (type/meaning/order), changing ownership
  or threading contracts of an existing symbol, renumbering the error-space
  symbols a consumer must handle.
- ADDING exports, APPENDING struct fields, adding event `type` values,
  adding overlay kinds, adding pragma-tuned DB fields: **no bump** —
  consumers must already ignore unknowns (§6/§7 rules; the DB schema migrates).
- Structs are never reordered; new fields append; the header comments each
  struct with its ABI-since version.
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
