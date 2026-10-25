# API-SURFACE-AUDIT — the reference's API operations vs. the core's C ABI

> Session: core-api (2026-10-26). Baselines: core `e6d5b01`, driver `7e88e38`,
> corpus `03ad46c`. Method: four parallel read-legs covered the corpus
> (driver usage, API-facing behavior), the driver repo, and the core repo;
> every load-bearing claim below was then re-verified first-hand in code or
> corpus artifact. Citations: `path:line` / `path §section`. Nothing here is
> projected from memory.

## 1. What "the reference API" is

The og (EaseUS DRW) has no C ABI. Its product surface at the UI↔engine
boundary is:

1. **In-proc engine factories** — `EUCreate<FS>QuickSearcher` /
   `EUCreate<FS>Searcher` exports from 9 SearchDLLs + RAWSearchDLL, returning
   `IQuick/ICompleteFileSystemSearchController*` through a single out-param
   (corpus `DRWUI-exe--main-ui-architecture.md` §9.2; decompiled dispatch
   `DRWUI_decompiled.c:905316-905340`, complete `:917077-917100`).
2. **The MOM module system** — `GetModule`/`CreateObject` factories, the
   `IObject` COM-like family, `CCallBackOperate` callback registration
   (corpus `mom-module-system--architecture.md` §1.1).
3. **The event bus** — `INotifyObserver` → `CMessageSend` → 15 UI subscriber
   classes; thread-affinity-marshaled (corpus
   `mom-modules--ui-bridge-and-dependency-matrix.md` §1).
4. **The `.rsf` session container** — records 1-4, chunked zlib
   (corpus `rsf-format--scan-session.md`).
5. **`CRecoveryMgr` / ExportThread** — the recovery/writer contract
   (corpus `architecture/04-recovery-workflow.md` §7).

The core's equivalent product surface is the **`cr_ui` C ABI** (`cr_ui.dll` /
`libcr_ui.so`): 61 exports, `CR_UI_ABI_VERSION = 1`, opaque handles, `cr_error`
returns, UTF-8 strings with documented ownership, no exceptions across the
boundary (verified: `include/cr/cr_ui.h:69-705`, all `CR_UI_API` decls; the
in-proc single-library binding model is the documented equivalent of the og's
in-proc factory model — `docs/architecture.md`, "the shim binds, not hosts").

**Audit rule** (mandate §2): every operation the reference exposes gets an
equivalent with the same semantics, format, response shape, and error model.
Where the corpus does not document a contract detail, it is marked
**NOT IN CORPUS** — never guessed.

## 2. Reference operation inventory → core status

37 distinct operations were decoded from the corpus (full evidence in the
session worklog, reading-wave leg 4). Verdicts: **MATCH / MISSING / WRONG /
PARTIAL / N/A-by-layer** (operation lives in the og's UI process, not its
engine boundary — a C host composes it from existing exports).

### 2.1 Engine loading & construction

| # | Reference operation | Core equivalent | Verdict | Evidence / notes |
|---|---|---|---|---|
| 1 | `LoadEngineModule` — 10 DLLs LoadLibraryW at startup | single cr_ui.dll, engines statically linked | **MATCH** (binding-model equivalent) | og loads in-proc factories (DRWUI §9.2); core binds in-proc without per-engine DLLs — documented `docs/architecture.md`; the observable contract (engines available, no cross-DLL lifetime hazards) is preserved |
| 2 | `CreateQuickSearcher(FS)` | `cr_ui_scan_create` with `stage_mask` quick bit | **MATCH** | stage_mask bits 1/2/4 = quick/deep/carve (cr_ui.h:290-314); unknown bits → `ERR_BAD_PARAMETER` |
| 3 | `CreateCompleteSearcher(FS list)` — full searcher set, ZFS gated on config, RAW gated on settings | `cr_ui_scan_create` deep stage; engines dispatched by `fs_hint` | **MATCH** | fs_hint 0=auto dispatches the engine matrix (cr_ui.cpp:799-827; 5 reserved/invalid); ZFS/ReFS engines present |
| 4 | `CreateRAWSearcher` | carve stage bit | **MATCH** | |
| 5 | `RegisterCarvedFileNamer` — UI registers `std::function<string(string,string)>` at RAW vt+0x48 (naming/filter policy at discovery) | not exposed; carve names synthesized from signature registry (`CarvedNNNN` + format) | **PARTIAL** | og: `DRWUI_decompiled.c:917273-917279`. Core: names + `carve_format`/`carve_confidence` are queryable per node (cr_ui.h node_info), so a host can re-derive/rename post-hoc; the extension point itself is not exported. Registered as a deliberate divergence (host-side naming) — see §5 D-API-1 |
| 6 | `SetScanMode(int)` on last searcher | `whole_disk` + stage weights in scan config | **MATCH** | semantics folded into config |
| 7 | `EnumerateAnalysers` — `EUGetAnalyserList/Map/Multimap` (RAWSearchDLL registry views) | **MISSING** | **MISSING** | og: RAWSearchDLL exports (corpus `rawsearch-carving--deep-dive.md` §1). Core keeps the 139-format signature registry internally (cr_rawsigns_data.cpp) but exports no enumeration. **Completion: add `cr_ui_carve_format_count()` + `cr_ui_carve_format_info()` (name/extension/magic/offset/min/max) — small, testable, no ABI break (appended exports).** |
| 8 | `GetRawUserInfoMap` — per-extension preference table (RawConfig.ini, 34 toggles) | **MISSING** | **MISSING** | og: RawConfig.ini-driven preference map exposed as a DLL export. Core's format set is compile-time. With #7 the same surface can expose the registry rows a host needs for config UIs. Same completion item. |

### 2.2 Scan lifecycle

| # | Reference operation | Core equivalent | Verdict | Notes |
|---|---|---|---|---|
| 9 | `QuickSearchFiles` | `cr_ui_scan_run` (quick stage) | **MATCH** | error contract: og returns HRESULT-style `0x5002xxxx` (in-proc C++); core returns `cr_error` u32s — no interop boundary exists, error CLASSES map 1:1 (null/invalid device → BAD_PARAMETER; reader failure → READ_FAILED family). Verified first-hand: cr_ui.cpp:1206-1334 |
| 10 | `CompleteSearchFiles` (full set + RAW) | deep + carve stages | **MATCH** | |
| 11 | MOM `LoadModule/CreateObject` | n/a — internal plumbing of the og's process | **N/A-by-layer** | the og's module loader is in-process infrastructure, not a host-facing operation |
| 12 | `StartScan` (orchestrator) | `cr_ui_scan_create` + `attach_device` + `run` | **MATCH** | run is synchronous on caller thread (ABI-CONTRACT §3); tree intact on error/cancel |
| 13 | `StopScan` — manual-reset event + cancel flag + reason | `cr_ui_scan_cancel` (PausableToken) | **MATCH** | og: `DRWUI_decompiled.c:315560-315600` (SetEvent path); core: cooperative cancel, `ERR_CANCELLED`, tree intact (cr_device.h:101-124). Bounded response = one loop iteration + one in-flight read (honest scope, cr_device.h:101-106) |
| 14 | Pause/Resume — **engine-interface semantics NOT IN CORPUS** (UI-level only) | `cr_ui_scan_pause` / `_resume` | **MATCH+** | corpus: no pause vtable slot documented anywhere; core exposes pause/resume with paused-ms telemetry — a documented superset (D-18 register) |
| 15 | Engine→UI event bus `(int msg, void* data, int&)` | `cr_ui_scan_set_event_fn` + 7 typed events | **MATCH** | og: MOM bus with Qt marshaling; core: typed POD events on worker threads, unknown types ignored (cr_ui.h:120-148). Event-shape superset (typed vs. untyped int/void*) documented ABI-CONTRACT §6 |

### 2.3 Progress, query, results

| # | Reference operation | Core equivalent | Verdict | Notes |
|---|---|---|---|---|
| 16 | Progress — pull-polled (38 `timeout()` consumers; only push = `sigPercentChange`) | `cr_ui_progress_fn` push + `cr_ui_scan_telemetry` pull | **MATCH+** | core offers both models; progress monotonicity enforced shim-side (cr_ui.cpp:687-735). og's engine emission throttle NOT IN CORPUS (milestone-based counts documented) — core's permille-CAS throttle is a documented superset |
| 17 | Recovery progress (typed `IUIProgressInfo`) | `cr_ui_recovery_config.progress` + `get_stats` | **MATCH** | og's `IUIProgressInfo` field layout NOT IN CORPUS (percent via vt+8 double documented); core's progress callback carries permille + counts |
| 18 | Tree node metadata (`IFileProperties` 23 slots) | `cr_ui_snapshot_node` → `cr_ui_node_info` | **MATCH** | name/sizes/4 timestamps/attributes/flags/extents/deleted/origin/confidence/carve format/hash — field set ⊇ og's persisted set (cr_ui.h:392-430) |
| 19 | Classify results — FileFilter.xml 8 categories / 1,481 extensions | 6 hard-coded categories / ~90 extensions | **WRONG** | og: `config-files/filtering-priority--config-layer.md` §2 + CORRECTIONS.md #5. Core query enum: 1..6 (cr_ui.h:483-485). Open finding **M-023** (filed session-3). The category taxonomy is engine-parity scope, tracked there; the API-side plan is §5 item 3 |
| 20 | `ApplyFilter` — 0x48-B `AdvanceFilterCondition`, 7 groups (type/time/size/state/tag/path/search) | `cr_ui_query` | **PARTIAL** | core covers type/time/size/state(deleted)/search + origin/confidence extras; missing **tag** and **path** groups (og: `31-filterbar-family-decode.md` §3-§6). Push-down divergence D-29 registered (client-side filtering). Plan: §5 item 4 |
| 21 | Search results (query + type dropdown) | `name_substring` filter | **MATCH** | ASCII-case-insensitive substring, documented |
| 22 | Sort | query sort 0-5 × dir | **MATCH** | og sorts async via `CSortNodeThread`; core sorts at query time (session trees immutable) — mechanism documented D-29 |
| 23 | Select/Deselect nodes | host-side (enqueue set = selection) | **N/A-by-layer** | the og's selection is UI widget state; recovery enqueue is the semantic equivalent |
| 24 | `CheckRecoverability` (grades 0/2) | scan-time four-tier confidence grading | **MATCH+** | core grades during the walk (D-14 EXCEEDS); og re-fires a TimeCritical background thread |

### 2.4 Preview, recovery, sessions

| # | Reference operation | Core equivalent | Verdict | Notes |
|---|---|---|---|---|
| 25 | Preview open (per-type stubs, tri-state) | `cr_ui_preview_open_session/_snapshot` | **MATCH** | private-props copy; device must outlive (F-1 basis rule verified cr_ui.cpp:2071-2106) |
| 26 | Preview stream read (`IInWindowStream`) | `cr_ui_preview_read` | **MATCH** | 4 MiB/call cap, short only at EOF, unreadable spans reported (cr_preview.cpp:19-25). og's 100 MB/15 s caps are UI-policy (host-side), NOT IN CORPUS as engine limits |
| 27 | Media thumbnail IPC (TCP 20171) | n/a — UI-process internals | **N/A-by-layer** | og's video-preview child-process IPC; not an engine-boundary operation |
| 28 | Choose destination (source-volume hard block) | `cr_ui_recovery_set_destination` + F12 overlap refusal | **MATCH+** | og blocks same-volume by raw `\\.\X:` open (04-recovery-workflow.md §7.3); core refuses overlap deterministically via `volume_extent` (ERR_DEST_OVERLAP) — registered superset |
| 29 | Recover/export (per-target loop, ACL restore, failure log) | `cr_ui_recovery_create/enqueue_*/process/get_stats/report_json` | **MATCH** | og's per-file overwrite policy NOT IN CORPUS; core exposes conflict modes (rename/skip/overwrite) + rename_attempts=64 default. ACL restoration = N-009d (open, tracked). Report: G-09 schema-versioned JSON |
| 30 | Free-quota gate / upsell | n/a — monetization | **N/A-by-layer** | licensing/product policy, out of recovery-API scope by decision |
| 31 | Auto-Recover (scheduled) | host-side composition (scan + enqueue + process) | **N/A-by-layer** | og wires it as UI state machine over the same engine ops |
| 32 | Save session (.rsf) | `cr_ui_scan_save_session` (.crsf) | **MATCH** | container semantics equivalent (chunked-zlib records, D-01/F10 hardening supersets); field set audited PARITY-CORE-REC |
| 33 | Load session | `cr_ui_session_load` | **MATCH** | corrupt records skipped (F10), legacy extent normalization |
| 34 | Auto-save on completion (`auto_*.rsf`) | host-side (`save_session` after `run`) | **N/A-by-layer** | og's auto-save is UI orchestration (Freeze→save→Unfreeze); the CLI does the same; documented |
| 35 | Lost-partition ops (candidates → scoring → scan targets) | `whole_disk` mode + `CR_EVT_LOST_CANDIDATE` + lostpart engine | **MATCH** | scoring parity = M-017 (open, engine scope); API surface complete |
| 36 | Wizard navigation (4-page stack) | n/a — UI layer | **N/A-by-layer** | |
| 37 | Report generation | `cr_ui_recovery_report_json` | **MATCH** | og's recovery-log FILE format NOT IN CORPUS (strings only); core's JSON schema is the documented clean-room equivalent (G-09) |

### 2.5 Core-only surfaces (no og equivalent — supersets, all registered)

- `cr_ui_health_read` (SMART/NVMe) — the og ships a separate DiskHealth app;
  function-equivalent, unified into the ABI.
- `cr_ui_disk_map` (GAP-S6-11) — og's `IBuildVolumeBitmap` internal surface.
- `cr_ui_scan_verify_identity` — og's record-3 boot-sector snapshot enables
  the same check; core exposes it as an operation.
- BitLocker gate (`ERR_FVE_LOCKED` + auto-unlock attempt) — D-16 subset
  (legitimate unlock only — deliberate).
- Pre-scan repair consent + undo (S-008) — og's 33-method pre-scan repair is
  engine scope (S-008 open); the API-side consent/undo surface exists.

## 3. Infrastructure findings (the ABI's own contract)

| ID | Finding | Severity | Fix |
|---|---|---|---|
| A-ABI-1 | **Versioning policy contradiction**: `cr_ui.h:40-42` says "new functions and appended struct fields bump [ABI version] too"; `docs/ABI-CONTRACT.md` §9 says adding exports = no bump. Header wins by ABI-CONTRACT's own preamble — but the header is the wrong side. | HIGH (blocks every future export decision, incl. §2.1 #7-#8) | adopt the ABI-CONTRACT §9 policy (append-only = compatible = no bump; remove/rename/re-semanticize = bump); correct the header comment. This matches the og's practice: the og added exports across versions without a versioned ABI at all |
| A-ABI-2 | **SQLite session sidecar documented-but-absent**: ABI-CONTRACT §8 + §5 ("scan_save_session also writes the sidecar index") describe a feature with zero implementation (grep: no sqlite anywhere in src/). GAP-S6-12 register row says MISSING. | HIGH (a normative contract claim that is false) | **CUT the claim.** The reference has no sidecar index — .rsf records ARE the session store (corpus rsf-format §2-§5). Per mandate ("do not invent operations the reference does not have"; "removing an invented degradation path… is required"), the doc is corrected and GAP-S6-12 is closed as REJECTED-BY-REFERENCE with corpus cite |
| A-ABI-3 | Export-count drift: docs say "57 exports" (UI-READINESS-PLAN.md:48, RELEASE-NOTES.md:17); actual 61 | LOW | refresh counts |
| A-ABI-4 | ABI-CONTRACT §5 names nonexistent functions (`session_disk_map`, `scan_disk_map`) | LOW | fix names to `cr_ui_disk_map` |
| A-ABI-5 | ABI-CONTRACT §6 claims events fire "on the thread running run() only" vs. header truth (worker threads allowed) | MEDIUM (threading contract is load-bearing for hosts) | header wins; fix doc |
| A-ABI-6 | `codeql` lane disabled (`if: false`) in ci.yml:174-175 while RELEASE-NOTES.md:71 claims "CodeQL clean" | MEDIUM | re-enable or re-word the claim honestly |

## 4. C consumer test coverage (the `/W4 /WX` gate)

`tests/ui_abi_check.c` (563 lines) exercises: ABI gate, image open/geometry/
transport, zero-init scan + progress monotonicity, stats/telemetry, double-run
refusal, S-008 consent + undo, save/load/stats/info, snapshot node/dfs/TRUNCATED,
query (count/limit/deleted/substring), preview (session + snapshot + metadata),
recovery (create/destination/enqueue/process/stats/report + on-disk bytes),
platform-truth (`ERR_UNSUPPORTED`), 2 BAD_PARAMETER probes.

**Not exercised by any C consumer today**: pause/resume/cancel,
`session_resume`, `verify_identity`, `disk_map`, event subscription,
subtree/snapshot enqueue variants, `recovery_cancel`, `checkpoint_count`,
Windows-only surfaces (open_disk/open_volume/enumerate/health).
C++ gtest covers most of these (test_cr_ui.cpp 14 tests) but the **C-consumer
gate is the ABI's product-surface proof** — it must exercise the full
lifecycle. Completion: §5 item 6.

## 5. Completion plan (ordered)

1. **Carve-format enumeration** (ops #7-#8): `cr_ui_carve_format_count()` +
   `cr_ui_carve_format_info()` over the internal registry; contract test
   asserting count>0, stable ordering, fields; C-consumer additions.
2. **A-ABI-1** versioning policy fix (header comment) — must land BEFORE the
   new exports so the policy is coherent at review time.
3. **A-ABI-2** cut the SQLite sidecar claim; close GAP-S6-12 as
   REJECTED-BY-REFERENCE; audit-trail in CORE-GAP-LIST.
4. **Query filter parity** (op #20): add `path_prefix` filter group
   (tag has no core-side analog — the og's tag data comes from its Results
   UI; document as NOT-EXPOSED with rationale). Contract tests.
5. **Doc drifts** A-ABI-3/4/5/6.
6. **C-consumer expansion**: pause/resume/cancel, resume, verify_identity,
   disk_map, events, recovery_cancel, checkpoint_count (Windows-only
   surfaces stay on the Windows lane).
7. M-023 (category taxonomy) remains engine-scope, tracked in
   diff-to-fix — not blocking this session's API completion.

Each item lands with fail-before/pass-after tests and corpus cites in the
commit message.
