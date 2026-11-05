# UI-INTEGRATION-SURFACE — final-prod's wired UI↔core boundary (the reference implementation)

> Produced by Agent B (task T2-b), consolidation session. READ-ONLY analysis of
> `/home/z/my-project/final-prod` (main @ a6cdead9; ui/ merged from
> recovery-wizard-ui-new v3 @ 86bc3168, then wired to the real engine in
> 02d153af..e7cec9bd). Every claim cites a file path. Purpose: the complete
> factual record of the ONE existing REAL (non-mock) UI↔cr_ui integration, so
> it can be re-implemented against the current core (core_recovery, ABI v4,
> 95 exports) on a fresh UI repo. Companion: docs/context-sheets/UI-SURFACE.md
> (the PRE-wiring mock-era snapshot), docs/context-sheets/CORE-SURFACE.md.

---

## 1. Architecture of the boundary

### 1.1 The layer stack

```
React 19 + TS (ui/src, Vite, hash router)
  │  features import ONLY src/queries/* — enforced by ui/rules/no-raw-invoke.yml
  │  (no @tauri-apps/api outside the connector) + no-connector-import-outside-queries.yml
  ▼
connector layer (ui/src/connector)
  │  contract.ts = the TS interface (9 domains, 686 lines); tauri/tauri-connector.ts
  │  = the ONE real adapter: invoke('<cmd>', args) + listen('<event>:<id>')
  ▼
Tauri 2 IPC — commands registered in ui/src-tauri/src/lib.rs:32-77
  ▼
Rust command modules (ui/src-tauri/src/commands/{devices,scan,results,preview,sessions,
  recovery,health,repair,system}.rs) — thin: argument decode, registry access,
  JSON payload composition. Domain logic stays in the engine.
  ▼
bridge (ui/src-tauri/src/bridge/)
  │  abi.rs FFI surface (#[repr(C)] mirrors) · error.rs BridgeError · util.rs two-call
  │  helpers + FILETIME→ISO + SendPtr + OwnedCrString · state.rs registries + NodeMeta
  │  cache · events.rs sinks→channel→emitter→emit · consent.rs the gate · logging.rs
  │  NG-15 app.log · filefilter.rs GENERATED extension→category mirror (1,481 exts)
  ▼
cr_ui C ABI — engine cr_ui.dll, ABI version 2
   (engine/include/cr/cr_ui.h; implemented engine/src/ui/cr_ui.cpp)
```

The shell is deliberately thin: "All recovery domain logic lives in the engine behind the
bridge; the frontend talks only to the connector contract" (ui/src-tauri/src/lib.rs:1-7).

### 1.2 How the engine DLL is located/loaded

**Static import linking — no LoadLibrary, no libloading.** The bridge imports `cr_ui.dll`
through the classic import-library contract:

- ui/src-tauri/build.rs:21-41 — on Windows: `cargo:rustc-link-lib=dylib=cr_ui` with search order
  (1) `$CR_UI_LIB_DIR` (set by build.ps1's ui lane), then (2) `<manifest>/target/{release,debug}`
  — "the directories build.ps1 stages the engine kit into (cr_ui.lib beside where cargo will emit
  the exe, so the DLLs also load from there by default loader order)". Off Windows the directives
  are skipped (cargo check/clippy don't link). build.rs:3-7 records the decision: "a libloading
  dynamic path is deliberately NOT used — static import matches the release bundle's import-lib
  contract and avoids dual definitions". Consequence: the DLL resolves via the normal Windows
  loader (exe dir first) at process start; no runtime path probing in Rust.

### 1.3 ABI-version check (cr_ui_abi_version)

Checked in **two places** against the compile-time constant `CR_UI_ABI_VERSION: u32 = 2`
(bridge/abi.rs:41):

1. **Startup gate** — ui/src-tauri/src/lib.rs:84-98 (`setup`): calls `cr_ui_abi_version()`; on
   mismatch logs to the operational log + eprintln (does NOT abort — the frontend's getAppInfo
   "also surfaces the state truthfully").
2. **Per-scan gate** — commands/scan.rs:109-115 (`scan_start_blocking`): refuses to bind on
   mismatch with a typed bridge error. The hard refusal — a stale kit cannot start a scan.

### 1.4 Calling convention and struct marshalling

- All externs are `extern "C"` declarations (bridge/abi.rs:475-565); every function returns a
  `u32` error code from the SAME numeric space as cr_error.h (0 = OK; e.g. 0x50020005 =
  ERR_CANCELLED). Header rules (cr_ui.h:23-37): C11, no C++ types, no exception escapes, strings
  in are NUL-terminated UTF-8, strings out are caller buffers (two-call size pattern) or heap
  strings freed with `cr_ui_string_free`, zero-init configs are safe.
- Structs are mirrored **1:1 in declaration order** under `#[repr(C)]` — "natural alignment
  reproduces the MSVC layout for both sides, so no manual padding is ever written here"
  (abi.rs:1-9). The file is pinned to the whole header surface so it stays diffable
  (abi.rs:17-24).
- **Who allocates / who frees** (util.rs:1-7; cr_ui.h): heap results are freed by the owning
  side — `cr_ui_string_free` for `char*` out-params (RAII-wrapped as `OwnedCrString`,
  util.rs:228-258, used for `cr_ui_recovery_report_json`); `_close` per handle family
  (`cr_ui_device_close`, `cr_ui_scan_close`, `cr_ui_session_close`, `cr_ui_snapshot_close`,
  `cr_ui_preview_close`, `cr_ui_recovery_close`). Close ORDER is enforced in `ScanEntry::Drop`
  (state.rs:252-282): event-fn unsubscribed FIRST, then the boxed sink sender freed, then snapshot
  cache (own Drop), then scan/session, then the borrowed device last. Preview handles are
  transient — opened and closed inside a single command (preview.rs:293). Caller-allocated
  buffers: two-call enumerations (`cr_ui_enumerate_disks/_volumes`, `cr_ui_snapshot_dfs`)
  size-first then fill (util.rs:156-200); `cr_ui_snapshot_node` uses a 1024-byte name buffer with
  an exact-size retry on `CR_UI_ERR_TRUNCATED` via `name_len` (util.rs:204-225).
- Raw FFI handles are carried inside registries as `SendPtr<T>` — an explicit unsafe Send/Sync
  wrapper (util.rs:12-31). Time conversion happens in ONE place: FILETIME-shaped 100ns-since-1601
  → ISO-8601 via `iso_from_filetime` (util.rs:65-123), producing the exact
  `YYYY-MM-DDTHH:MM:SS.sssZ` shape `Date.toISOString()` makes.

### 1.5 Threading model (the engine contract the bridge honors)

From bridge/abi.rs:11-15 (quoting cr_ui.h ABI RULES) and contracts/event-mapping.md:

- `cr_ui_scan_run` / `cr_ui_recovery_process` are **synchronous on the caller's thread** → the
  bridge runs each on a dedicated `std::thread::spawn` thread (scan.rs:300, recovery.rs:283),
  never on a Tauri IPC worker.
- pause/cancel/telemetry/busy/stopping and snapshot capture are **any-thread safe** → token
  commands call them from `spawn_blocking`.
- Progress/consent callbacks fire on **engine worker threads**: must not block, must not lock
  the registry, must not call back into the engine. The sinks do exactly one `try_send` of a POD
  copy into a bounded `sync_channel` (512 events for scans, scan.rs:204; 64 samples for recovery,
  recovery.rs:104); a full channel means the UI is slow and the emitter coalesces
  (events.rs:31-56).
- Every `#[tauri::command]` is `async` and wraps its blocking core in
  `tauri::async_runtime::spawn_blocking` (exceptions: `consent_respond` — "registry-only — no
  engine call... the lock is held for one map lookup + a condvar notify", consent.rs:163-166; the
  four `repair_*` gap stubs; and `system_app_info`, which only reads package info).

### 1.6 Bridge state and registries

`Arc<BridgeState>` is Tauri-managed state (lib.rs:31). BridgeState (state.rs:381-405) holds:
`scans: Mutex<HashMap<String, Arc<ScanEntry>>>`, `jobs: Mutex<HashMap<String, Arc<JobEntry>>>`,
`consents: Mutex<HashMap<String, PendingConsent>>`, AtomicU64 id mints (`scan-N`, `job-N`,
`consent-N`), the sessions directory (ProgramData vs beside-exe portable, §7), and the
sessions-list mtime cache. Entries are Arc'd so long passes hold only the entry's snapshot
mutex, never the registry lock (state.rs:1-8). A `ScanEntry` is either `Live { scan, device }`
or `Session { session, device: Option<..> }` (state.rs:36-49) — ONE registry serves both
mid-scan browsing and loaded sessions, so the results/preview/recovery surface is identical for
both.

---

## 2. The command catalog (exhaustive)

**Count: 34 commands** registered in `tauri::generate_handler!` (ui/src-tauri/src/lib.rs:32-77)
and mirrored 1:1 in `contracts/command-surface.json` (`commands` array length = 34). The
README's "33 Tauri commands" (ui/README.md:35) is off by one — trust the registration and the
manifest. Commit 02d153af called it "the 33-command bridge" and `consent_respond` was re-added
afterwards (G-1 (a), commit f47463ec) — exactly how the drift happened.

Legend: **threading** — "SB" = body runs inside `tauri::async_runtime::spawn_blocking`; "sync" =
runs on the async runtime directly. **Cancel** = how a user cancels the underlying operation.
All commands return `Result<T, BridgeError>` serialized as `{ code: number, bridgeCode:
string|null, message: string }` (error.rs:42-51).

| # | Command | Module | TS args → Rust args | Return shape | cr_ui ABI calls | Error mapping | Threading | Cancel |
|---|---------|--------|--------------------|--------------|-----------------|---------------|-----------|--------|
| 1 | `devices_list` | devices.rs:423 | – | `Device[]` (camelCase JSON rows) | `cr_ui_enumerate_disks`, `cr_ui_enumerate_volumes`, `cr_ui_open_disk` (probe), `cr_ui_open_volume` (FVE probe), `cr_ui_device_close` | engine rc via `check()`; probe rc refines `status`/`bitlocker` | SB | n/a |
| 2 | `devices_refresh` | devices.rs:433 | – | `Device[]` (same composer) | same as #1 | same | SB | n/a |
| 3 | `scan_start` | scan.rs:79 | `targetId: string, pathScope?: string\|null` → `target_id: String, path_scope: Option<String>` | `{ scanId }` | `cr_ui_abi_version`, `cr_ui_open_volume`/`cr_ui_open_disk`, `cr_ui_device_geometry`, `cr_ui_scan_create`, `cr_ui_scan_attach_device`, `cr_ui_scan_set_event_fn` (+ `cr_ui_enumerate_*` via target resolve) | ABI mismatch → bridge UNKNOWN; FVE locked (0x50060001) → bridge `device-permission`; bad scope → `scan-scope-invalid`; device absent → `device-gone` | SB prep, then dedicated run thread | `scan_cancel` |
| 4 | `scan_pause` | scan.rs:434 | `scanId` | `()` | `cr_ui_scan_pause`, `cr_ui_scan_busy`, `cr_ui_scan_telemetry` (flags) | `scan-not-running` if not busy/terminal; engine rc | SB | – |
| 5 | `scan_resume` | scan.rs:449 | `scanId` | `()` | `cr_ui_scan_resume` + flags | `scan-not-running` / `scan-not-paused` | SB | – |
| 6 | `scan_cancel` | scan.rs:483 | `scanId` | `()` | `cr_ui_scan_cancel` (+ `close_scan_consents` registry force-deny FIRST) | `scan-not-running` if terminal; engine rc | SB | is the cancel |
| 7 | `scan_state` | scan.rs:512 | `scanId` | `ScanState` JSON (params, target, status, timestamps, progress, failure, pendingConsent, lostCandidates) | `cr_ui_scan_busy`, `cr_ui_scan_stopping`, `cr_ui_scan_telemetry` | `scan-not-found` | SB | – |
| 8 | `scan_close` | scan.rs:572 | `scanId` | `()` | (registry remove; `ScanEntry::Drop` closes handles) | `scan-not-found`; refuses while busy or jobs live (typed UNKNOWN) | SB | must cancel first |
| 9 | `results_tree` | results.rs:838 | `scanId, parentId?: string\|null, mode: 'path'\|'type'\|'raw'` | `{ nodes: TreeNode[], parentHasMore: false }` | `cr_ui_scan_snapshot`/`cr_ui_session_snapshot`, `cr_ui_snapshot_node_count`, `cr_ui_snapshot_partial`, `cr_ui_snapshot_dfs`, `cr_ui_snapshot_node` (via state::refresh/ensure_metas) | `scan-not-found`; bad node id → typed UNKNOWN | SB | n/a |
| 10 | `results_query` | results.rs:441 | `scanId, filter: ResultsFilter, sort {field,direction}, page {offset,limit}` | `{ nodes: ResultNode[], total, totalBytes, offset, pageSize, elapsedMs }` | same snapshot/metadata path (bridge-side filter, G-7) | `scan-not-found`; bad node id | SB | n/a |
| 11 | `results_count` | results.rs:464 | `scanId, filter` | `number` | same, `count_only` | same | SB | n/a |
| 12 | `node_detail` | results.rs:855 | `scanId, nodeId` | `{ node, fullPhysicalPath, createdAt, accessedAt, confidenceReason, badSectorsOverlap, clusterCount }` | metadata cache only | bad `f<idx>` id → typed UNKNOWN | SB | n/a |
| 13 | `preview_open` | preview.rs:298 | `scanId, nodeId` | `PreviewPayload` (image/hex/text/av/office JSON) | `cr_ui_preview_open_snapshot` (live) / `cr_ui_preview_open_session` (session), `cr_ui_preview_size`, `cr_ui_preview_metadata`, `cr_ui_preview_unreadable`, `cr_ui_preview_read`, `cr_ui_preview_close` | `preview-too-large` (>16 MiB), `preview-missing-data` (size 0), `preview-damaged` (unreadable>0), `preview-unsupported`, `device-gone` (no source) | SB | n/a |
| 14 | `preview_thumbnail` | preview.rs:310 | `scanId, nodeId` | `{ dataUrl } \| null` (null when >8 MiB — OQ-3) | same as #13 | same | SB | n/a |
| 15 | `session_save` | sessions.rs:69 | `scanId, path?: string\|null` | `SessionMeta` | `cr_ui_scan_save_session`, `cr_ui_scan_get_stats` | `scan-not-running` if busy (must finish/cancel first); engine rc | SB | n/a |
| 16 | `sessions_list` | sessions.rs:165 | – | `SessionMeta[]` | per file (lazy): `cr_ui_session_load`, `cr_ui_session_get_info`, `cr_ui_session_stats`, `cr_ui_session_close` | missing dir → typed UNKNOWN | SB | n/a |
| 17 | `session_load` | sessions.rs:274 | `path: string` | `{ scanId }` (registers a Session-kind scan entry) | `cr_ui_session_load`, `cr_ui_session_get_info`, `cr_ui_session_stats`, (+ `cr_ui_open_volume`/`cr_ui_open_disk` best-effort re-attach) | missing file → `session-not-found`; engine rc (corrupt codes pass through) | SB | n/a |
| 18 | `recovery_start` | recovery.rs:56 | `scanId, selection {kind:'nodes'\|'all', nodeIds?/filter?}, destination, options` | `{ jobId }` | `cr_ui_recovery_create`, `cr_ui_recovery_set_destination`, `cr_ui_recovery_enqueue_node`/`_subtree` (sessions) / `_snapshot`/`_subtree_snapshot` (live), `cr_ui_recovery_queue_count` (+ paged `query_page` for recover-all) | `device-gone` (no source); engine rc at enqueue (overlap 0x50050001 etc.) | SB prep, then dedicated process thread | `recovery_cancel` |
| 19 | `recovery_cancel` | recovery.rs:331 | `jobId` | `()` | `cr_ui_recovery_cancel` | terminal → truthful no-op Ok | SB | is the cancel |
| 20 | `recovery_state` | recovery.rs:348 | `jobId` | `RecoveryJob` JSON | none (registry) | unknown job → typed UNKNOWN | SB | – |
| 21 | `recovery_items` | recovery.rs:398 | `jobId, page {offset,limit}` | `RecoveryItem[]` (state done/skipped/failed/pending) | `cr_ui_recovery_report_json` (parsed each call) | engine rc | SB | – |
| 22 | `recovery_report` | recovery.rs:444 | `jobId` | `RecoveryReport` (succeeded/failed/skipped capped 500, totals, verified, truncatedItemLists) | `cr_ui_recovery_report_json` | engine rc | SB | – |
| 23 | `health_read` | health.rs:17 | `deviceId: 'disk-N'` | `DiskHealth` JSON (status, attributes with thresholds, NVMe fields) | `cr_ui_health_read_ex2` (two-call; base `cr_ui_health_read` NOT used) | `device-gone` (bad id), `health-unavailable` (0x50030008/0x50070001/no sources) | SB | n/a |
| 24 | `consent_respond` | repair.rs:32 | `requestId, allow: boolean` | `()` | none (registry + Condvar) | `consent-not-found` (unknown/stale/double) | sync | – |
| 25 | `repair_dry_run` | repair.rs:41 | `deviceId` (ignored) | – | none | typed `not-implemented` (BridgeError::gap) | sync | – |
| 26 | `repair_apply` | repair.rs:46 | `deviceId, options` (ignored) | – | none | typed `not-implemented` | sync | – |
| 27 | `repair_undo_log` | repair.rs:57 | `deviceId` (ignored) | – | none | typed `not-implemented` | sync | – |
| 28 | `repair_undo` | repair.rs:62 | `deviceId` (ignored) | – | none | typed `not-implemented` | sync | – |
| 29 | `system_driver_status` | system.rs:123 | – | `{ installed, version, running, note }` | none — Win32: OpenSCManagerA/OpenServiceA/QueryServiceStatus (service `RDA`), CreateFileW `\\.\RDA`, DeviceIoControl `IOCTL_RDA_GET_VERSION` (0x60002448) | never errors (truthful probe result) | SB | n/a |
| 30 | `system_app_info` | system.rs:142 | – | `{ name, version, engineVersion: null, platform }` | none (G-6: no ABI engine-version string exists) | – | sync | n/a |
| 31 | `system_validate_destination` | system.rs:229 | `path, sourceDeviceId, neededBytes` | `{ allowed, reason?, reasonKey?, freeBytes, neededBytes }` | `cr_ui_enumerate_volumes` (letter set + mount check) | reason ∈ overlap/invalid/out-of-space/unwritable (never an Err) | SB | n/a |
| 32 | `system_pick_directory` | system.rs:296 | `hint?: {volumeId?, excludeDeviceId?}` (currently unused Rust-side) | `string \| null` | none — tauri-plugin-dialog `blocking_pick_folder()` | join errors → UNKNOWN | SB (picker blocks) | n/a |
| 33 | `system_list_mounts` | system.rs:268 | – | `MountPoint[]` (lettered volumes with free/total) | `cr_ui_enumerate_volumes` + GetDiskFreeSpaceExW | engine rc | SB | n/a |
| 34 | `system_open_path` | system.rs:318 | `path` | `()` | none — tauri-plugin-opener: dirs `open_path`, files `reveal_item_in_dir` | UNKNOWN when path missing | sync | n/a |

### Group semantics & tricky details

**devices (1-2).** `list_devices` (devices.rs:100-228) composes contract-shaped `Device` rows:
disk id `disk-<n>`, volume ids `vol-<L>` / `vol-<disk>-<offset>` (devices.rs:20-32); usage bytes
from Win32 `GetDiskFreeSpaceExW` per lettered volume (engine reports totals only); status from a
fresh open-and-close probe; BitLocker tri-state (false / "locked" on 0x50060001 / "unlocked");
`kind` from the `bus_type` tail field — sd for bus 12/13 (SD/MMC), usb for bus 7 or removable,
else internal (devices.rs:209-211); `isSourceOfActiveScan` from the scan registry;
`quickScanAvailable` always true (no per-volume gate — G-10). `resolve_target`
(devices.rs:242-317) maps targetIds back to rows, including the virtual quick-targets
`vol-recycle-bin` (implicit scope `<SystemDrive>:\$Recycle.Bin`) and `vol-desktop`
(`%USERPROFILE%\Desktop`).

**scan (3-8).** Lifecycle (scan.rs:1-7): ABI gate → resolve target → open → `scan_create`
(zero-init config = documented defaults: stage mask 7, weights 40/35/25, threads auto) → attach
→ **subscribe BEFORE run** → dedicated run thread. `whole_disk` = 1 for disk targets;
`pre_scan_repair`/`pre_scan_check` always armed (scan.rs:184-187) with the consent callback
wired (§3). G-11 pathScope: the engine has NO folder-scoped scan — the bridge scans the whole
volume/disk and applies a normalized lowercase `'/'`-joined prefix (scan.rs:40-67) to every
result view; scan duration reflects the whole volume. Locked volumes refuse with bridge
`device-permission` (unlock surfaces are a documented gap). The run thread (scan.rs:300-405)
latches the terminal state from the run rc (OK→completed; 0x50020005→cancelled-as-STATUS; repair
codes 0x5008xxxx/0x50030005→failure `repair-failed`; 0x50060001→`device-permission`), saves the
repair undo log via `cr_ui_scan_repair_save_undo` when `stats.repair_applied > 0` (to
`<sessions>/repair-undo/<scanId>.undo`), freezes a final snapshot, computes the exact
found-bytes sum (G-3) from NodeMeta file rows, and emits the terminal events itself.
Pause/resume set a transient status word ('pausing'/'resuming', 2500 ms expiry cleared when
telemetry catches up — state.rs:82-84, events.rs:307-314). `scan_close` refuses while busy or
while recovery jobs still reference the scan.

**results (9-12).** ONE filter implementation serves both live scans and loaded sessions (G-7):
`cr_ui_query_run` cannot express the contract's 'existing' atom (no deleted_never knob), so the
bridge filters its own metadata cache for both (results.rs:1-17). The cache: one DFS pre-order
pass over `cr_ui_snapshot_node` rows assembles paths in O(total path bytes) + a reverse pass
folding subtree file_count/tree_size (state.rs:616-695). Live scans re-snapshot at
SNAPSHOT_REFRESH_MIN_MS = 1500 (state.rs:546); terminal scans freeze; sessions take ONE stable
snapshot. Node ids are bridge-minted and opaque: files `f<idx>`, dirs `d<idx>`, plus navigation
ids `root`/`lost-paths`/`raw-all`/`lost-<idx>`/`c<idx>`/`ext:<cat>:<ext>` (results.rs:4-8).
Three tree modes: path (dir tree + synthetic `lost-paths`/ `raw-all` groups), type (8 category
chips → per-extension leaves), raw. `results_query` sorts name/modified/size/confidence/type,
pages (default limit 200), returns `elapsedMs`; `node_detail` composes `fullPhysicalPath`
(structured rows on lettered volumes only) + `confidenceReason` from real evidence
(results.rs:906-931). T14 tests (#[ignore]d) pin filter cost and the NodeMeta memory bound (<1
KiB/node) at 10^5/10^6 scales.

**preview (13-14).** Kind dispatch is bridge-side from ENGINE metadata (ADR-008) — the UI never
classifies extensions. Bounds (ADR-003): image 16 MiB, thumbnail 8 MiB (oversized → null
thumbnail, OQ-3), hex 64 KiB; engine caps reads at 4 MiB/call so the bridge chunks
(preview.rs:110-128). Any unreadable span → typed `preview-damaged` refusal, never zero-filled
content as intact (preview.rs:163-172). Handles are transient — opened, read, closed inside the
command. Video/audio → 'av' metadata + null thumbnail (no renderer — G-4); documents/archives →
'office' metadata, no extracted text.

**sessions (15-17).** Store: `%ProgramData%\HeicFast\Recovery Wizard\sessions\` installed,
`.\sessions\` portable (marker probe, state.rs:510-540). Format `.crsf` (CRSFMT01); auto-name
`auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf` (UTC, sessions.rs:56-66). `session_save` refuses while
busy. `sessions_list` is stat-first against the directory mtime, labels/counts filled lazily via
ONE load+get_info+stats+close pass per file, cached in the bridge (OQ-4 — no sidecar index).
`session_load` pre-checks existence (bridge `session-not-found`), then best-effort re-attaches
the source by model+serial pinned by slice offset+bytes (sessions.rs:374-419) — when
absent/ambiguous, browsing keeps working; preview/recovery fail truthfully with `device-gone`.

**recovery (18-22).** Flow (recovery.rs:1-8): create (collision→conflict,
verifyOnWrite=false→skip_hash_verify, rebuildFolders=false→ skip_dir_reconstruct;
read_chunk/rename_attempts stay engine-default) → set_destination (engine normalizes + arms the
same-letter overlap refusal, backstop 0x50050001) → enqueue → process thread. Selection `nodes`
(sessions enqueue by engine node_id via the metadata cache; live scans by snapshot index —
race-free against the CURRENT generation, G-02b) or `all` (the bridge pages its own filtered
query at 500/page and enqueues every matching FILE — directory structure materializes through
the writer's dir reconstruction, G-08). Item truth comes from the engine report JSON:
done/salvaged→done, refused/resumed/nothing_to_recover→skipped, error/hash_mismatch→failed
(recovery.rs:46-53). Per-list cap 500 with `truncatedItemLists`; `processedTotal` exact.
Terminal status derivation (recovery.rs:287-305): cancelled / failed / completed /
completed-partial.

**health (23).** Uses ONLY `cr_ui_health_read_ex2` (NG-9 threshold-carrying append; the base
struct is an array element whose stride is ABI-frozen). Availability truth (NG-6/NG-9):
`available` is the ATA-table flag ONLY — `nvme_available == 1` is valid data; a
readable-but-EMPTY ATA table grades "unknown" with `tableEmpty: true`, never a fabricated
"good"; attribute 5/0xC5/0xC6-derived counters are null when the table is empty. Grading:
failing-now/critical-warning→bad; failing attributes, wear ≥ 80, or low spare→caution; data
present and clean→good; no data basis→unknown. No aggregate percentage is synthesized anywhere.

**repair (24-28).** The REAL repair path is the scan's pre-scan analysis with the consent
callback armed; `consent_respond` is the only decision crossing back. The standalone workbench
commands are typed not-implemented refusals (`BridgeError::gap`, error.rs:93-105) — ABI v2
exposes no standalone repair surface (RawFixer crosses the ABI only as pre-scan repair inside a
scan; repair.rs:1-17). Undo is CLI-only (`rrecov fix --undo-apply <file>`); the bridge persists
the undo log after an approved repair lands.

**system (29-34).** `system_driver_status` probes the SCM service `RDA` (OQ-5: the engine
transport's default service name) then `\\.\RDA` + IOCTL 0x60002448; the note carries the
test-signing posture (§5). `system_validate_destination`: same-letter source → 'overlap';
non-mount → 'invalid'; free < needed → 'out-of-space'; write probe (create+delete temp file)
failing → 'unwritable' — reasons are RESULT values, never errors. `system_pick_directory` runs
`blocking_pick_folder` on the blocking pool (never the main thread); the native picker cannot
filter by device, so the connector validates the result. `system_open_path` absorbed
reveal_in_explorer (plan §4.7): files revealed with selection, folders opened.

---

## 3. The event & callback model

### 3.1 The pipeline (bridge/events.rs + contracts/event-mapping.md)

```
engine worker thread        bridge                                        TS
────────────────────        ──────                                        ──
cr_ui_event_fn /       →    sink (extern "C"): ONE try_send of a POD   →   emitter thread     →   listen() in
cr_ui_recovery_progress_fn  copy into a bounded sync_channel (512/64)      (per handle):         tauri-connector.ts
   (NEVER blocks)           mpsc ─────────────────────────────────────►    drain, coalesce,     (onScanProgress /
                                                                          compose payload        onRecoveryProgress /
                                                                          ── AppHandle::emit ──► onConsentRequest)
```

Marshaling rules (events.rs:1-15, event-mapping.md): (1) the sink runs on an engine worker
thread — no blocking, no registry locks, no engine re-entry; (2) the sink does exactly one
`try_send`; (3) a single emitter thread per handle drains, coalesces superseded samples, and
emits; (4) unknown event types are ignored (forward compat); (5) HEARTBEAT never synthesizes
progress. **Terminal events are NOT emitter-produced**: the run/process threads latch the
terminal state first, then emit the final progress sample + `scan-finished:` /
`recovery-finished:` themselves (single-writer rule).

### 3.2 Event names and payload shapes

All events are per-handle scoped Tauri events (contracts/command-surface.json `events` array):
| Event | Source | Payload (camelCase JSON) |
|---|---|---|
| `scan-progress:<scanId>` | `cr_ui_event` via channel→emit | `ScanProgressEvent` (contract.ts:176-197): scanId, status, stage, stageProgress, overallProgress, bytesScanned, bytesTotal, filesFound, dataFoundBytes, errors, badSectorMap[{startByte,sizeBytes}], etaSeconds, throughputBytesPerSecond, recoverableDuringScan, deviceConnected, currentPath (always null — G-2) |
| `scan-finished:<scanId>` | run() return code | `{ status, failure }` |
| `save-progress:<scanId>` | CR_EVT_SAVE_PROGRESS | `{ fraction: 0..1 }` |
| `lost-candidate:<scanId>` | CR_EVT_LOST_CANDIDATE (NG-11) | `{ lba, count }` |
| `consent-request:<scanId>` | `cr_ui_repair_consent_fn` | `ConsentRequestEvent` (contract.ts:242-246): scanId, requestId, plan{detected, fsFamily, actionCount, refused, reason, advisoryCount} |
| `recovery-progress:<jobId>` | `cr_ui_recovery_progress_fn` + stats/report drain | `RecoveryProgressEvent` (contract.ts:476-489): jobId, status, overallProgress, currentFile, currentFileProgress (always 0 — bytes-level truth), completed/failed/skipped/totalCount, throughputBytesPerSecond, verifying, etaSeconds |
| `recovery-finished:<jobId>` | process() return code | `{ status }` |

Engine event-type handling in the scan emitter (events.rs:209-281): SCAN_STAGE maps stage 0/1/2
→ 'quick-index'/'deep-walk'/'carve' (initial 'volume-analysis'); SCAN_PROGRESS clamps overall
monotonically (`permille/1000`); FILES_FOUND sums files+deleted+carved; ERROR_RANGE appends to
the bad-sector map (bounded `BAD_SECTOR_MAP_CAP` = 4096; count stays exact); CHECKPOINT
internal-only; LOST_CANDIDATE increments the truthful counter + emits its own event; REPAIR
latches a summary line; HEARTBEAT only refreshes `last_heartbeat_ms`; SAVE_PROGRESS emits the
save event; FS_CHECK keeps a damage count. On every emitted sample the emitter takes a
**coalesced telemetry snapshot** (`cr_ui_scan_telemetry`) for bytes_scanned (carve-true bytes
the event cannot carry) and computes throughput (EMA 0.6/0.4) + ETA.

`deviceConnected` (G-8, events.rs:93-106): heartbeat-fresh within `DISCONNECT_AFTER_MS` = 10 000
(2× the engine's 5 s watchdog), OR a progress sample in that window, OR terminal/pre-start —
never fabricated beyond liveness evidence. Status word (events.rs:63-91): terminal status →
transient ('pausing'/'resuming') within expiry → stopping → paused → busy→running → else
'starting'.

The job emitter (events.rs:339-436) loops on `recv_timeout(250 ms)`, keeps the freshest sample,
derives throughput/ETA, refreshes live counters from `cr_ui_recovery_get_stats` (done+salvaged /
errors+hash_mismatch / refused+resumed+nothing_to_recover / queued), and polls the report tail
(`cr_ui_recovery_report_json`) every `JOB_REPORT_POLL_MS` = 2000 for currentFile + the
verify-phase signal. It stops once the process thread sets `finished`; the process thread emits
the terminal payload (events.rs:486-507).

### 3.3 The consent flow (bridge/consent.rs) — how a request blocks and is answered

Engine contract (consent.rs:1-9, cr_ui.h pre-scan repair section): when `cfg.pre_scan_repair =
1`, the shim runs the read-only repair analysis before the stages and — when a repairable plan
exists — calls `repair_consent` ONCE, ON THE RUN THREAD, before any engine read. Nonzero return
= execute the transactional repair; zero = the plan is surfaced via CR_EVT_REPAIR events and
stats and NEVER executed (default-deny).

Bridge wiring (the G-1 follow-up (a) design):
1. `scan_start` boxes + leaks a `ConsentPipe { scan_id, app, state }` and arms
   `cfg.repair_consent = Some(consent_sink)` (scan.rs:188-194). The pipe is the run thread's
   exclusive property (the callback can only fire inside run) — freed in the run tail after run()
   returns (scan.rs:314-316).
2. The sink (consent.rs:121-159) mints a `requestId` (`consent-N` — an echo token, "never
   derived from anything the UI could guess into authority"), registers a `PendingConsent {
   payload, gate }` in the state registry (**the registry lock is taken and RELEASED before
   blocking** — the run thread never sleeps holding a lock; plan §4.5 deadlock rule), emits
   `consent-request:<scanId>` with the plan POD as JSON, then **blocks the run thread on the gate
   with NO user-facing timeout** — "a non-answer is a user decision; the prompt stays open until
   answered, the scan is cancelled, or the process exits".
3. `ConsentGate` = `Mutex<Option<bool>>` + `Condvar` (consent.rs:51-93): `respond()` returns
   false on double-respond (→ typed consent-not-found), `close()` force-denies idempotently,
   `wait_for_decision()` parks the run thread. The answer crosses back ONLY through the
   `consent_respond` command → `consent_respond_impl` (registry-only, no engine call).
4. Default-deny on every teardown path: `scan_cancel` force-resolves the scan's pending gates
   FIRST (scan.rs:496-501) — a blocked run thread must unblock before the engine can observe the
   cancel token — and the run tail closes any survivor after run() returns (scan.rs:317).
5. **Re-hydration**: `scan_state` returns `pendingConsent` (the stored payload) so the prompt
   re-renders when the event fired before the page subscribed, or after navigating away and back
   (scan.rs:548-561; contract.ts:214-217). The UI-side store primes from state but the prime only
   FILLS (a pre-event poll's null must never race away a just-arrived request,
   queries/scan.ts:104-140).

The scan page renders a **non-dismissable modal** (Escape/outside-click prevented, no close
button — scan-progress-page.tsx:336-405): Approve is disabled when the plan was refused or has
zero actions; stale-id errors clear the store so the prompt dismisses. e2e/consent.spec.ts
covers deny-completes, re-hydration after navigate-away, and the stale double-respond path.

### 3.4 Log lines — NOT events

Engine diagnostic lines do NOT cross as Tauri events. NG-15 (logging.rs): a dedicated writer
thread + channel writes to `%ProgramData%\HeicFast\Recovery Wizard\logs\app.log` (portable:
`.\logs\`), fed by two producers — the ENGINE's lines through the registered
`cr_ui_set_log_sink` (levels 0/1/2 = Error/Warn/Info; fires on ANY engine thread; must be cheap)
and the BRIDGE's lifecycle facts. Rotation at 1 MiB to `app.log.old` (one generation). `init()`
is called FIRST in `run()` (lib.rs:20-26) — before the ABI gate — so every later failure leaves
evidence.

### 3.5 How the UI unsubscribes

`onScanProgress`/`onConsentRequest`/`onRecoveryProgress` return a closure that resolves the
pending `listen()` promise and calls Tauri's unlisten (tauri-connector.ts:215-241, 325-335). The
query-layer external stores ref- count: when the last React subscriber unsubscribes, the store
unsubscribes the connector and deletes itself from the module-level map (queries/scan.ts:84-98,
queries/recovery.ts:46-58).

---

## 4. The TS connector layer (ui/src/connector/)

Files:
- **contract.ts** (686 lines) — the single source of truth: shared primitives (`Iso8601`,
  `FileCategoryId`, `ConfidenceGrade`, `DeviceKind`, `VolumeFs`, `DeviceStatus`, `Capacity`),
  `ConnectorError`/`ConnectorErrorCode` (27 members, contract.ts:55-81), and 9 domain interfaces
  composing `RecoveryConnector` (contract.ts:676-686): devices, scan, results, preview, sessions,
  recovery, health, repair, system. Principles (contract.ts:4-12): the UI renders and requests; no
  UI-side intelligence that belongs in the backend; errors carry machine codes + reasonKeys — the
  UI never string-matches error text.
- **index.ts** (17 lines) — the import point: `export const connector: RecoveryConnector =
  createTauriConnector();` — **no env branch exists**. The mock/simulated backend is GONE (commit
  02d153af; package.json:5's "mock-connector driven" description is stale copy). **VITE_CONNECTOR
  does not exist in final-prod's ui/** — zero grep hits. Test-mode selection is a Vite ALIAS, not
  a runtime flag: `vite.config.ts:16-26` swaps exactly `@/connector` for
  `tests/fakes/test-connector.ts` when `process.env.VITEST !== undefined || mode === 'test'`
  (exact-match regex so `@/connector/contract` keeps resolving to the real source) — "production
  builds can never reach the double".
- **tauri/tauri-connector.ts** (390 lines) — the ONE real adapter. Every method is a literal
  `invoke('<command>', {args})` so `contracts/validate.mjs` phase 2 can cross-check both
  directions against the manifest.

**Error model.** No Error subclass — `ConnectorError` is `Error` augmented via `Object.assign`
(tauri-connector.ts:124-143): `{ code, ConnectorErrorCode, reasonKey, detail?: { code: number }
}`. The bridge rejects with `{ code, bridgeCode, message }`; `bridgeCode` (when a member of
`REASON_KEYS`, the 27-entry code→i18n-key table, tauri-connector.ts:57-84) maps directly;
otherwise the numeric `code` maps through `ENGINE_CODE_MAP` (34-entry cr_error.h table,
tauri-connector.ts:87-122) and the raw number rides `detail.code`. Non-payload rejections
degrade to `unknown`. ERR_CANCELLED never crosses as an error — the bridge already translated it
to status 'cancelled'. **Preview normalization**: hex payload `bytes` arrive as a JSON array and
are converted to `Uint8Array` client-side (tauri-connector.ts:164-170); a null preview payload
throws `preview-missing-data`.

**Cancellation / retry / poll.** No AbortSignal anywhere. Cancellation is explicit domain
commands (`cancelScan`, `cancelRecovery`) plus event-unsubscribe closures. No connector retry
logic; retries live in TanStack Query defaults (queries retry 1, mutations retry 0 —
queries/client.ts:4-16). The only polling: `useScanState` refetchInterval 500 ms while
running/starting (queries/scan.ts:216-220).

**State stores consuming the connector.**
- **TanStack Query v5** for all request/response (src/queries/*, 9 files; keys centralized in
  client.ts:18-42). Results list uses `useInfiniteQuery` with `RESULTS_PAGE_SIZE = 200`
  (queries/results.ts:16-56); count drives select-all semantics and recover-all sizing.
- **zustand** for client state: `scan-session-store` (active scan + known scans +
  celebration-seen, stores/scan-session-store.ts), `selection-store` (mode 'ids'|'all',
  exclusions, anchor — select-all over the full filtered total, not just loaded rows),
  `filter-store` (the filter-bar UI state: presets, debounced search, category chips, extensions,
  existingOnly, hideEmptyFiles), `ui-store` (viewMode, previewPaneWidth, sidebar collapsed
  persisted to localStorage, dev panel flags).
- **Event streams** are `useSyncExternalStore` external stores keyed by scanId/jobId
  (queries/scan.ts:19-99, queries/recovery.ts:15-61): the store subscribes the connector once per
  id, drives query-cache invalidations on status changes and (NG-2) on 10%-bucket jumps OR a 15 s
  time floor while running, and self-destructs with the last subscriber.

**Lifecycle binding.** Mount/unmount cleanup through the store ref-counting above; navigation
uses React Router v7 hash routing (router.tsx; ADR-001 — hash history behaves identically in
WebView2's custom protocol). There are no router navigation guards; in-flight work is bounded by
cancel commands and the bridge's own close refusals. Errors surface as toasts via sonner with
the i18n reasonKey (e.g. home-page.tsx:91-95 `toast.error(reasonKey ? t(reasonKey) :
err.message)`).

**The seam test**: tests/connector-tauri.test.ts mocks `@tauri-apps/api/core` + `/event` at
module boundary and asserts every contract method invokes the exact manifest command with exact
args, rejections surface the mapped ConnectorError, and subscriptions return the contract
unsubscribe shape.

---

## 5. Driver-era references in the UI (complete inventory)

`git grep -ni "driver|rda|elevat|administrator"` over ui/src, ui/src-tauri, ui/tests, ui/e2e
yields EXACTLY the following (CSS `surface-elevated` class names excluded — unrelated):

1. **commands/system.rs:20-135 — `system_driver_status` + `driver_probe`**. The whole command
   is driver-specific: SCM service `RDA` (OpenSCManagerA/OpenServiceA/QueryServiceStatus), device
   `\\.\RDA` (CreateFileW), `IOCTL_RDA_GET_VERSION` = 0x60002448 (u64 (Major<<32)|Minor),
   `DRIVER_REQUIRED_NOTE` (system.rs:23-27): "required component - the engine refuses raw-disk
   access without it; this build is development test-signed (docs/DEV-SIGNING.md): stock
   Secure-Boot machines must enable test signing (bcdedit /set testsigning on) or disable Secure
   Boot to load it". Non-Windows: truthful "the raw-disk driver ships Windows-only". Cargo.toml
   carries `Win32_System_Services` in windows-sys specifically for this probe.
2. **ConnectorErrorCode `'driver-required'`** — contract.ts:58; REASON_KEYS →
   `'errors.driverRequired'` (tauri-connector.ts:60); ENGINE_CODE_MAP `0x50020007:
   'driver-required' // ERR_DRIVER_REQUIRED (mandatory transport; docs/PARITY-DELTAS.md D-FALL)`
   (tauri-connector.ts:93). Added by commit b9d1a366 "ERR_DRIVER_REQUIRED flows through the
   boundary truthfully".
3. **i18n strings**: en.ts:12-16 `about.driver/driverOk/driverRunning/
   driverStopped/driverMissing` ("Disk driver", "Installed ({{version}}) — {{running}}", "Required
   — not installed"); en.ts:464-465 `errors.driverRequired`: "The RDA disk driver is required and
   not running. Install or start it, then retry — the product has no fallback mode."; en.ts:156
   `lostPartition.deepBody` mentions "reads the WHOLE disk through the recovery driver".
   zh-CN.ts:14-18, 448-449 mirror these.
4. **Consumers**: about-dialog.tsx:27-46 (About row) and settings-page.tsx:64, 197-198 (`✓/✕
   <version> (idle)` line) render `useDriverStatus()` (queries/system.ts:13-19, staleTime
   Infinity; key client.ts:38).
5. **Tests**: connector-tauri.test.ts:285-292 asserts `getDriverStatus` invokes
   `system_driver_status`; tests/fakes/test-connector.ts:938-943 returns fixture status. e2e specs
   carry no driver references.
6. **Elevation/administrator**: exactly ONE UI string — `home.permissionDenied`: "Access needs
   administrator approval" (en.ts:101; zh-CN.ts:102 "需要管理员权限"). Rendered as a warning banner on
   Home when any physical device's status is 'permission-denied' (home-page.tsx:180-184, 215-220),
   as a per-card hint (home-page.tsx:463), as a card badge (home-page.tsx:511), and as an
   aria-label on the lost-partition page (lost-partition-page.tsx:163). **There is NO
   retry-as-admin flow, NO runas/elevation relaunch, NO install-driver prompt anywhere in the
   UI.** The `driver-required` error surfaces only as a toast when a scan-start fails (home-page
   beginScan onError → `toast.error(t(reasonKey))`). **What happens to each when the driver is
   gone and the core requires elevation (ERR_ELEVATION_REQUIRED):** `system_driver_status` (1)
   must be REMOVED or repurposed — the SCM probe, `\\.\RDA` IOCTL, the test-signing note, and the
   windows-sys Services dependency are all driver-era; the About/Settings rows (4) lose their data
   source (drop the row or replace with an am-I-elevated probe). The `'driver-required'`
   vocabulary + `0x50020007` mapping (2, 3) must become an elevation-required vocabulary keyed to
   the v4 core's ERR_ELEVATION_REQUIRED code, with copy changing from "install or start the
   driver" to "relaunch as administrator" — a surface the old UI never had to build. The
   permissionDenied banner (6) remains VALID and becomes more important: the v4 core's user-mode
   raw-disk access needs elevation, so 'permission-denied' device statuses and scan-start failures
   will be the primary not-elevated signal, and the copy already fits. Item 5 follows whatever
   replaces the command.

---

## 6. Capability coverage map

### 6.1 Export counts (measured, not guessed)

- final-prod's v2 header declares **80 `CR_UI_API` function exports**
  (engine/include/cr/cr_ui.h; 83 occurrences of the token counting the three `#define CR_UI_API`
  macro lines at cr_ui.h:67/69/72 — the "83 exports" figure in the root worklog's session-start
  table and the session brief counts those macro lines; the distinct-function count is 80).
  `#define CR_UI_ABI_VERSION 2u` at cr_ui.h:75.
- bridge/abi.rs declares **68** of them as externs (its header comment says "the 78-export
  surface" — stale count from when it was written; 02d153af era).
- The bridge actually **calls 55**.

### 6.2 v2 exports the bridge USES (55)

version/diagnostics (4): `cr_ui_abi_version`, `cr_ui_last_error`, `cr_ui_string_free`,
`cr_ui_set_log_sink`. devices/enumeration (6): `cr_ui_open_disk`, `cr_ui_open_volume`,
`cr_ui_device_close`, `cr_ui_device_geometry`, `cr_ui_enumerate_disks`,
`cr_ui_enumerate_volumes`. scan (15): `cr_ui_scan_create`, `_close`, `_attach_device`, `_run`,
`_pause`, `_resume`, `_cancel`, `_busy`, `_stopping`, `_telemetry`, `_get_stats`,
`_set_event_fn`, `_save_session`, `_snapshot`, `_repair_save_undo` (all cr_ui_scan_*). sessions
(5): `cr_ui_session_load`, `_close`, `_get_info`, `_stats`, `_snapshot`. snapshot (5):
`cr_ui_snapshot_close`, `_node_count`, `_partial`, `_node`, `_dfs`. preview (7):
`cr_ui_preview_open_snapshot`, `_open_session`, `_close`, `_size`, `_read`, `_metadata`,
`_unreadable`. recovery (12): `cr_ui_recovery_create`, `_close`, `_set_destination`,
`_enqueue_node`, `_enqueue_subtree`, `_enqueue_snapshot`, `_enqueue_subtree_snapshot`,
`_queue_count`, `_process`, `_cancel`, `_get_stats`, `_report_json`. health (1):
`cr_ui_health_read_ex2` — ONLY the ex2 variant; the base `cr_ui_health_read` is declared but
never called.

### 6.3 v2 exports UNUSED by the UI (25 = 13 declared-but-uncalled + 12 never declared)

Declared in abi.rs but never called (13) — "exports the product has no surface for yet"
(abi.rs:17-24): **image opening** `cr_ui_open_image`; **BitLocker unlock aids**
`cr_ui_open_volume_with_key`, `cr_ui_unlock_volume_with_dictionary` (locked volumes truthfully
refuse — "unlock surfaces are a documented gap", scan.rs:149-166); **transport diagnostics**
`cr_ui_device_transport`, `cr_ui_last_transport_error`; **the engine query family**
`cr_ui_query_run`, `cr_ui_query_rows_free` (replaced by the bridge-side filter, G-7); **stats
extras** `cr_ui_scan_assistant`, `cr_ui_scan_checkpoint_count`, `cr_ui_scan_save_cancel`
(SAVE_PROGRESS events are consumed but save-cancel never invoked), `cr_ui_scan_verify_identity`;
**session resume** `cr_ui_session_resume` (the UI only browses/recovers loaded sessions);
**health base** `cr_ui_health_read` (superseded by ex2).

Never declared in abi.rs (12) — in the v2 header but invisible to the bridge: **lost-partition
apply family** `cr_ui_apply_lost_partition`, `cr_ui_apply_undo_last`, `cr_ui_apply_save_undo`
(cr_ui.h:673-690 — the lost-partition page can only SCAN for candidates; applying a found
partition is engine-capable but unwired); **carve format catalog** `cr_ui_carve_format_count`,
`cr_ui_carve_format_info` (cr_ui.h:898-901); **user-defined carve signatures**
`cr_ui_register_user_signs`, `cr_ui_user_sign_count`, `cr_ui_user_sign_info` (cr_ui.h:919-930);
**disk map overlay** `cr_ui_disk_map`, `cr_ui_disk_map_free` (cr_ui.h:973-981); **query date
buckets** `cr_ui_query_apply_buckets` (cr_ui.h:869); **regrading** `cr_ui_session_regrade`
(cr_ui.h:612).

These 25 are the candidate capabilities a fresh UI could surface (subject to what v4
kept/changed — cross-check against core_recovery's header).

### 6.4 Screens and the commands each drives

UI routes (ui/src/app/router.tsx; feature hook usage verified per file):

| Screen (route) | Feature file | Commands/events consumed |
|---|---|---|
| Home `/` (device selection) | features/device-selection/home-page.tsx | `devices_list`, `devices_refresh`, `scan_start` (device/volume cards + quick targets Recycle Bin/Desktop/Select Folder), `system_pick_directory`, `sessions_list`, `session_load`; toast on failure |
| SD Card `/sd-card` | device-selection/sd-card-page.tsx | `devices_list`, `scan_start` |
| Scan progress `/scan/:deviceId/live` | scan-progress/scan-progress-page.tsx | `scan_state` (500 ms poll), `scan-progress` events, `scan_pause/resume/cancel`, `consent_respond` + `consent-request` events, live results (below); renders the non-dismissable consent modal, stop-confirm dialog, celebration modal |
| Results `/results/:scanId` | results-explorer/results-explorer.tsx (+results-route, filter-bar) | `results_tree` (3 modes), `results_query` (200/page infinite), `results_count`, `node_detail`, `preview_open`/`preview_thumbnail`, `session_save`, `system_open_path` (open-location), `scan_state`; `save-progress` events drive the save spinner |
| Recovery `/recover/:scanId` | recovery-queue/recovery-page.tsx | `system_validate_destination`, `system_pick_directory`, `system_list_mounts`, `recovery_start`, `recovery_state`, `recovery_items`, `recovery_report`, `recovery_cancel`; `recovery-progress`/`recovery-finished` events |
| Health `/health`, `/tools` | health/health-page.tsx | `health_read`, `devices_list`, `scan_start` |
| Repair `/repair` | repair/repair-page.tsx | NONE — static truthful content (the real repair is the in-scan consent; workbench trio not-implemented) |
| Lost partition (sidebar flow) | lost-partition/lost-partition-page.tsx | `devices_list`, `scan_start` (whole-disk), `scan_state.lostCandidates` + `lost-candidate` events |
| Settings/About `/settings` | settings-about/settings-page.tsx + about-dialog.tsx | `system_app_info`, `system_driver_status` |
| Backup `/backup` | backup/backup-page.tsx | `sessions_list`, `session_load` |

Not wired to any screen: `preview`-related internals are query-layer only; `consent_respond`
appears only on the scan page; the dev-panel (dev-panel.tsx) uses only theme/ui stores.

---

## 7. Lessons for the re-implementation (v4 on the fresh UI repo)

Distilled from `git -C final-prod log --oneline 86bc3168..HEAD -- ui/` (40 commits: 1efb8d32 FFI
pin → 02d153af the 33-command bridge + real connector → 728b0baa consumers + test double →
f47463ec consent flow → 6e12f167 health truth → 6c156321 scan liveness → 1dfb358d real surfaces
+ bus-type ABI → a5455985 NG-15 log seam → 8a48b1cb NG-21/22 → a4dbcfc1 NG-18 → e7cec9bd
v0.6.0-rc2), the worklog.md tail, and the code.

### What the old wiring got right — preserve in the v4 rebuild

1. **The marshaling spine**: sinks that only `try_send` a POD copy into a bounded channel; one
   emitter thread per handle; single-writer terminal latching from the run/process threads;
   unknown-event-type tolerance. This is the only pattern that satisfies the engine's
   worker-thread callback contract (events.rs:1-15). Bounded capacities (512 scan / 64 recovery)
   are deliberate — keep the "channel-full = UI slow" posture.
2. **The consent machinery** exactly as built: echo-token requestIds, gate registered with the
   registry lock RELEASED before the run thread blocks, no user-facing timeout, cancel
   force-denies FIRST, run-tail hygiene close, re-hydration through scan-state. The S3 forensic
   incident (28-minute stall at 0% with the modal unanswered, run 40470191039; the probe now
   answers Deny) is the proof this flow must be airtight — NG-21's engine-side fix (per-partition
   repair scope + partition-table guard) removed the false offer, but the UI must keep the
   non-dismissable modal and force-deny paths. **The repair unit is the PARTITION, never the whole
   disk.**
3. **The metadata-cache results design (G-7)**: one DFS pre-order pass assembling paths in
   O(total path bytes) + reverse subtree folds; ONE filter implementation for live scans AND
   loaded sessions; refresh cadence 1500 ms live / freeze on terminal / single snapshot for
   sessions. If v4's query family now covers the 'existing' atom, engine-side queries can replace
   it — but repeat the T14 cost measurements.
4. **Truth rules that took incidents to learn**: ERR_CANCELLED is a STATUS, never an error
   (error.rs:14-16); no fabricated progress from heartbeats; `currentPath` stays null because no
   ABI event carries it (G-2); found bytes latch once at completion (G-3); empty SMART tables
   grade "unknown" not "good" (NG-9); preview damage is a typed refusal, never zero-filled
   content; `engineVersion` is null — the ABI number is a contract number, not a product version
   (G-6).
5. **The logging seam (NG-15)**: init the app log FIRST — before the ABI gate — because "the
   shipped app previously wrote NOTHING (eprintln to a lost stderr) — an installed-app failure
   left no evidence trail" (lib.rs:20-26). Engine diagnostics ride `cr_ui_set_log_sink` through a
   channel to a writer thread (engine sinks fire on ANY thread and must be cheap). Rotation 1 MiB
   / one generation.
6. **The portable-mode probe (NG-18)**: never probe the `ProgramData` env var — it is ALWAYS
   set on Windows; use the marker file `recovery-wizard.portable` beside the exe (a writability
   probe would misdetect elevated installs).
7. **Bus-type ABI (G-9/NG-14)**: `CrUiDiskInfo.bus_type` is a raw STORAGE_BUS_TYPE tail-append;
   SD/MMC (12/13) → 'sd', USB (7) or removable → 'usb', else 'internal' (devices.rs:209-211).
   Verify v4 kept the encoding.
8. **Struct-append discipline**: when the header gains an appended field, abi.rs gains it in
   the same position; array-element structs (SMART attrs) cannot grow in place — the engine
   appended `cr_ui_health_read_ex2` with a wrapper struct (NG-9). Expect v4 to have done more of
   this; diff the header, don't assume.
9. **Two-call protocol handling** with TRUNCATED exact-size retry (util.rs:204-225) — "IN
   capacity when out != NULL, OUT needed when out == NULL" (cr_ui.h). **The generated filefilter
   mirror**: bridge classification must match the engine's category_mask bit-for-bit; GENERATED
   from engine/src/core/query/cr_filefilter_data.cpp by ui/scripts/gen-filefilter.mjs with a
   `--check` CI freshness gate (1,481 extensions). Regenerate against the v4 engine's table —
   never hand-edit.
10. **Test-mode discipline**: the labeled double lives under tests/fakes, selected by an
    exact-match Vite alias in test mode only — "production builds can never reach the double: no
    env branch exists" (index.ts:8-10, vite.config.ts:10-26). The fresh repo still ships
    src/connector/mock/; final-prod deleted it — decide deliberately.
11. **The manifest contract**: contracts/command-surface.json is machine-checked against BOTH
    the Rust registration and every TS invoke (validate.mjs phase 2); removed[] documents dropped
    commands (preview_read/preview_close became bridge-internal helpers). Rebuild for v4 — it
    catches real drift (README's "33" vs the actual 34; the 78/83 export-count drift is the same
    error class).
12. **Close-order/lifetime discipline**: event-fn unsubscribe → free sink box → snapshot cache
    → scan/session → device; scan_close refuses while busy or jobs live; JobEntry::Drop does NOT
    close the device (state.rs:252-349). **Session re-attach is best-effort** by model+serial
    pinned by slice offset+bytes — sessions carry no disk number (sessions.rs:374-419); browsing
    survives a missing source, preview/recovery fail truthfully.
13. **Recover-all pages the bridge's own query** (500/page) and enqueues files; directory
    structure materializes through the writer's dir reconstruction (G-08). Dirs use subtree
    enqueue; sessions address by node_id, live scans by snapshot index (G-02b race-free).
    **Destination overlap is double-gated**: bridge-side same-letter check + engine-side
    0x50050001 (F12). Keep both.
14. **UX wiring facts**: scan starts DIRECTLY from device selection (no config step — ADR-008;
    the reference exposes no mode selection); live-results refetch needs the 15 s floor + bucket
    jumps (NG-2) or the deep-stage band goes stale; sector counts use the target's real
    sectorSize, not 512 (NG-13b); the native folder picker is called directly with no fake
    intermediate tree (NG-13a); the tree root label is the scan target's REAL label, never a
    hard-coded 'Windows (C:)' (NG-22).

### What was driver-specific — must NOT carry over

1. `system_driver_status` and its entire SCM/`\\.\RDA`/IOCTL probe (system.rs:20-135), the
   `Win32_System_Services` windows-sys feature, the test-signing note, and the About/Settings
   driver rows. The v4 core has no driver and no service.
2. The `'driver-required'` ConnectorErrorCode + `0x50020007` mapping + the "no fallback mode"
   copy. The v4 failure mode is **ERR_ELEVATION_REQUIRED** (core_recovery README: user-mode
   `\\.\PhysicalDriveN` access, GENERIC_READ|GENERIC_WRITE, 64 KiB read envelope). The fresh UI
   needs a first-class elevation surface the old one never had: detect not-elevated early (device
   status / a startup probe), explain it, and offer a relaunch-as-admin action — the old UI's only
   elevation-adjacent behavior was a passive warning banner (`home.permissionDenied`) and a toast
   on failure.
3. The driver-mandatory narrative in i18n (en.ts:464-465, zh-CN.ts:448-449, the lostPartition
   deepBody "through the recovery driver" copy).
4. WOW64 concerns were installer-side only (System32\drivers copy, docs/INSTALLER.md:43-45) —
   nothing in ui/ ever dealt with WOW64; do not import any of it.
5. The dev-signing posture guidance (testsigning/bcdedit) — obsolete with the driver gone.

### Net-new work the old UI never did (v4 deltas to expect)

- An elevation-aware startup/home state and relaunch flow (new).
- Wiring the v4 export growth (80→95): diff the new header against §6.3's unused list — the
  apply/unlock/query-buckets/carve-catalog/user-signs/ disk-map/session-regrade families are the
  growth areas to evaluate.
- The fresh repo's contract (recovery-wizard-ui-new @ dd2782c, src/connector/contract.ts, 662
  lines) has the same 9 domains and nearly the same method set but is MISSING the consent surface
  (`onConsentRequest` / `respondToConsent` were final-prod's G-1(a) additions) and still carries
  mock scenario control (`getScenario`/`setScenario`/`setSeed`/`reset`) plus src/connector/mock/ +
  tauri/not-implemented.ts. Porting the consent surface, the error-vocabulary, and the event-store
  patterns from this sheet is the core of the rewiring.

---

*Sources: ui/src-tauri/src/{lib,main}.rs; bridge/*; commands/*; build.rs; Cargo.toml;
tauri.conf.json; capabilities/default.json; ui/src/connector/*; ui/src/queries/*;
ui/src/stores/*; ui/src/features/*; ui/src/app/router.tsx; vite.config.ts; ui/tests/*; ui/e2e/*;
ui/i18n/*; ui/README.md; ui/scripts/gen-filefilter.mjs; contracts/*; engine/include/cr/cr_ui.h;
worklog.md (tail); git log 86bc3168..HEAD -- ui/.*
