# INTEGRATION-PLAN.md — final-prod consolidation gate document

> Author: Agent E (consolidation draft, Task 2-e), reviewed and **APPROVED by the integrator**
> at the gate review on 2026-10-26. This is the gate document: nothing is implemented before this
> plan is committed and pushed (Task T3). Completion state of every item is recorded at the
> bottom of this file when the session closes.
>
> **Integrator sign-off decisions (2026-10-26):**
> - **§7.4 elevation model — DECIDED (a):** the app ships with a `requireAdministrator` manifest
>   (reference parity; both raw-disk transport legs are admin-only, evidence in §7.4; recorded as
>   PARITY-DELTA D-I3 *matched-to-reference*). The **installer** honors the mandate's
>   surfaced-elevation model: Tauri NSIS `installMode: "both"` — the installer launches
>   asInvoker; UAC elevation is requested only when the user chooses the per-machine install
>   (Program Files + HKLM) and/or the driver component; per-user mode installs app+engine only
>   with the driver explicitly unavailable and truthfully explained.
> - **OQ-5 driver service name — DECIDED:** `ServiceName = RDA` pinned by the installer (device
>   `\\.RDA`); to be re-verified against `engine/include/cr/win/cr_transport.h` at T7 before the
>   installer step lands (T11), and the pin recorded in `contracts/README.md`.
> - **OQ-6 Win7 floor — DECIDED:** engine + driver keep their Windows 7 SP1 contract (their CI
>   proves it); the **UI product floor is Windows 10+** (WebView2 Evergreen dropped Win7).
>   Recorded as a PARITY-DELTA row with rationale; `rrecov.exe` CLI and the engine kit remain
>   Win7-true.
> - **Unified product version — DECIDED:** root `VERSION` = **0.5.0** (already committed at the
>   repo root): the consolidation milestone. 1.0.0 is reached when the `docs/INTEGRATION-GAPS.md`
>   follow-ups close. The RC tag for T18 is `v0.5.0-rc1`.
> - **OQ-1..OQ-4:** implementation-detail pins, resolved during T5/T7 from `engine/include/cr/cr_ui.h`
>   (they are recorded in contracts/type-maps.md when pinned — no design change).
> - **Merge executed (T4):** the three-way merge described in §2 was executed on 2026-10-26 in
>   the order engine → ui → driver (sequential, `--allow-unrelated-histories`); 1,099 commits on
>   `main`; the §2.4 verification checklist is recorded in the session worklog and in
>   `docs/INTEGRATION-VALIDATION.md` as it completes.
> Inputs (read completely, line by line):
> - `work/context/CORE-SURFACE.md` — core_recovery @ `a574ecf` (C++20 engine, 78 cr_ui exports, CR_UI_ABI_VERSION=2) — committed at `docs/context-sheets/CORE-SURFACE.md` — cited **[CORE §n]**
> - `work/context/UI-SURFACE.md` — recovery-wizard-ui-new @ `f4a8f8a` (React 19 + Tauri 2, 36-method RecoveryConnector, MockConnector) — committed at `docs/context-sheets/UI-SURFACE.md` — cited **[UI §n]**
> - `work/context/DRIVER-DEPLOYMENT.md` — rda-driver @ `da7efb9` (WDM rda.sys, unsigned by design) — committed at `docs/context-sheets/DRIVER-DEPLOYMENT.md` — cited **[DRIVER §n]**
> - `work/context/REFERENCE-LAYOUT.md` — EaseUS DRW installed tree (og-codebase/, 1,470 files) — committed at `docs/context-sheets/REFERENCE-LAYOUT.md` — cited **[LAYOUT §n]**
> - `work/context/REFERENCE-INSTALLER.md` — DRW installer/registry/service evidence (Inno Setup 5.5.x) — committed at `docs/context-sheets/REFERENCE-INSTALLER.md` — cited **[INSTALLER §n]**
> Citations include the underlying repo file the sheet cites. Items that could not be verified from the
> sheets are marked **OPEN-QUESTION** (disposition: see the integrator sign-off decisions in the
> header). Where a sheet's evidence contradicted a proposed decision, the plan says so explicitly
> (§7.4 elevation — resolved by sign-off decision (a)).

---

## 1. Executive summary

**What is being merged** — three git repositories into one, history preserved, no squash
**[DECIDED]**, via `git-filter-repo --to-subdirectory-filter`:

| Sub-tree | Source repo | Pinned commit | Size |
|---|---|---|---|
| `engine/` | core_recovery (branch main) | `a574ecf6cc06a374e1ee0f893e574efef4904639` | 990 commits, 769 files |
| `ui/` | recovery-wizard-ui-new (branch v3) | `f4a8f8aebab625e9048d833cd9c3501741d7c436` | 62 commits, 385 files |
| `driver/` | rda-driver (branch main) | `da7efb9e0c94046893c62b25695bb95edf519f8b` | 43 commits, 66 files |

The research corpus (easeus-drw-research @ `03ad46c`) is **NOT merged**; it remains the external
evidence authority and is cited from docs **[DECIDED]**.

**What is being removed** — the entire simulated backend: `MockConnector`
(`ui/src/connector/mock/mock-connector.ts`), `fixtures.ts`, `node-space.ts`, `rng.ts`, the
`ScenarioControl` dev-panel surface, the throwing `tauri-connector.ts` stub, the
`VITE_CONNECTOR` build gate, and every CI pin of it (§5). Full inventory in §5; zero mock in
shipped code [UI §3.4, session-prompt Final reminder].

**What is being wired** — a real, in-process engine leg. The Tauri Rust shell
(`ui/src-tauri`) loads `cr_ui.dll` (the documented consumption contract: in-process flat-C ABI,
NO subprocess, NO IPC on the UI↔engine leg — `engine/include/cr/cr_ui.h:5-21`,
`engine/docs/ABI-CONTRACT.md:504-511` **[CORE §3]**) and exposes ~30 Tauri commands; a new real
`TauriConnector` in `ui/src/connector/tauri/tauri-connector.ts` implements `RecoveryConnector`
(all 36 methods) over `invoke()`/`listen()` [UI §3.6]. Shared contracts live in a top-level
`contracts/` directory: the error-code mapping, the event mapping, the Tauri command-surface
manifest — the bridge truth CI validates both sides against **[DECIDED]**. The engine keeps
`cr_ui.h`; the UI keeps `contract.ts`; **no other duplication**.

**What is being packaged** — one Windows product: `Recovery Wizard` (Tauri productName,
identifier `com.heicfast.recoverywizard` [UI §8]), app + engine DLLs + optional `rda.sys`
kernel driver, installed per-machine by **NSIS** (Tauri's native bundler; the reference's
Inno Setup 5.5.x is a documented PARITY-DELTA **[DECIDED]**, §7.6). Install layout: Program
Files (code, read-only) + ProgramData (sessions/logs) + System32\drivers (driver file),
surfaced elevation for the driver step, uninstall/update flows, plus a portable zip.

**Single product identity** — repo `github.com/heicfast/final-prod`; one root `VERSION` file
consumed by engine CMake, ui Cargo/tauri.conf/package.json, driver rc/INF, and the NSIS
installer **[DECIDED]**. Display naming stays exactly as the UI ships it today
[UI §1]: app name "Recovery Wizard" / i18n display name "EaseUS Data Recovery Wizard Free"
(`ui/src/i18n/en.ts:6`). Recommended first unified version: **1.0.0** (component lineage
0.4.0-parity engine / 0.3.3 UI / 1.0 driver recorded in release notes).

**Resulting behavior** — the real core's behavior replaces the simulator's, operation by
operation. 36 connector operations: **7 clean, 26 adapted, 3 integration gaps** (§3). The mock
was poll-shaped (150 ms `setInterval`); the real bridge is push-shaped over the cr_ui event
stream on engine worker threads, marshaled to Tauri `emit` — the UI's subscription-shaped
contract is unchanged [UI §6, §3.4].

---

## 2. Repository structure

### 2.1 Target tree (every top-level entry + origin)

```
final-prod/                                (new; github.com/heicfast/final-prod)
├── VERSION                    (NEW — single version source of truth, §6)
├── README.md                  (NEW product README; sub-tree READMEs remain as history)
├── LICENSE                    (NEW — MIT, "heicfast / final-prod contributors";
│                               engine+driver already MIT; ui repo has NO LICENSE today
│                               [verified via git ls-files] — root LICENSE fixes that)
├── worklog.md                 (NEW unified session worklog; per-tree worklogs archived §2.3)
├── .gitignore                 (NEW root ignore for cross-tree build outputs)
├── build.ps1                  (NEW single build entry point, §8; build.sh = Linux subset)
├── contracts/                 (NEW shared contract truth, §2.4 — the ONLY cross-tree contract)
│   ├── error-codes.ts/.md     (cr_error numeric space 0x50020001..0x500A0001 ↔ ConnectorErrorCode)
│   ├── event-mapping.ts/.md   (CR_EVT_* 1..10 → ScanProgressEvent/RecoveryProgressEvent fields)
│   ├── command-surface.json   (Tauri command manifest: name ↔ ABI refs ↔ TS call sites)
│   ├── type-maps.md           (Device/ScanState/TreePage/… ↔ cr_ui_* struct field maps, §3)
│   ├── validate.mjs           (CI gate: mapping completeness + both-side consistency, §9)
│   └── README.md
├── installer/                 (NEW NSIS project + portable-zip script, §7)
├── docs/                      (NEW product docs: ARCHITECTURE/BUILD/RELEASE/TESTING/
│                               INTEGRATION-PLAN/INTEGRATION-VALIDATION/INTEGRATION-GAPS/
│                               PARITY-DELTAS, §10)
├── .github/workflows/         (NEW consolidated workflow set, §8.3)
├── engine/                    ← core_recovery @ a574ecf (untouched tree: include/cr, src/,
│                               docs/, tests/, bench/, fuzz/, tools/, scripts/, diff-to-fix/,
│                               worklog/, .github/ (workflows deleted at T4, §2.3))
├── ui/                        ← recovery-wizard-ui-new @ f4a8f8a (src/, src-tauri/, tests/,
│                               e2e/, docs/, rules/, scripts/, public/, worklog/)
└── driver/                    ← rda-driver @ da7efb9 (src/, docs/, scripts/, signing/, fuzz/,
                                tools/, tests/, winpe reg file, worklog/)
```

### 2.2 Merge commands, order, strategy — EXECUTED 2026-10-26 (evidence: merge commits on `main`)

Tool: `git-filter-repo --to-subdirectory-filter` per source **[DECIDED]**, then **sequential**
merges (NOT octopus). Octopus merges of three rewritten roots are legal, but sequential gives
one reviewable merge per repo, a per-merge verification gate, and trivial bisection; with
disjoint sub-trees there are zero content conflicts either way. **Executed order: engine → ui →
driver** (functionally equivalent to the engine → driver → ui order sketched below; merges are
disjoint, so order affects only merge-commit topology, not content):

```powershell
# 0. workspace (fresh clones, never rewrite the originals)
git clone github.com/heicfast/core_recovery        engine-src
git clone github.com/heicfast/recovery-wizard-ui-new ui-src
git clone github.com/heicfast/rda-driver            driver-src

# 1. rewrite each into its sub-directory (full history kept, no squash)
cd engine-src  ; git filter-repo --to-subdirectory-filter engine
cd ../ui-src   ; git filter-repo --to-subdirectory-filter ui
cd ../driver-src; git filter-repo --to-subdirectory-filter driver

# 2. fresh target repo
git init final-prod && cd final-prod
#    seed root: VERSION, LICENSE, README.md, worklog.md, .gitignore  (T4 skeleton)
git remote add engine   ../engine-src   ; git fetch engine
git remote add driver   ../driver-src   ; git fetch driver
git remote add ui       ../ui-src       ; git fetch ui

# 3. merge order: engine → driver → ui
git merge --allow-unrelated-histories -m "Merge core_recovery @ a574ecf as engine/ (history preserved)" engine/main
git merge --allow-unrelated-histories -m "Merge rda-driver @ da7efb9 as driver/ (history preserved)"   driver/main
git merge --allow-unrelated-histories -m "Merge recovery-wizard-ui-new @ f4a8f8a as ui/ (history preserved)" ui/v3
git remote remove engine; git remote remove driver; git remote remove ui
```

Rationale for order: engine first (largest, defines the ABI the other work targets), driver
second (engine documents it as its transport [CORE §10]), ui last (its bridge work in T7+
sits on top of both). All three merge without conflicts because `--to-subdirectory-filter`
keeps every path disjoint.

### 2.3 Duplicated files that exist in MORE THAN ONE tree — enumeration + policy

Enumerated from `git ls-files` over all three pinned trees (verified this session) and the
sheets [CORE §1, UI §1, DRIVER §1]:

| Duplicated name | Present in | Policy |
|---|---|---|
| `worklog.md` | engine (17,143 L), ui (923 L), driver (1 entry) | **Keep all three** as frozen per-tree session archives at `engine/worklog.md`, `ui/worklog.md`, `driver/worklog.md`; the NEW root `worklog.md` continues the session worklog. Each per-tree log documents that repo's own history — deleting it would erase history-linked evidence. |
| `README.md` | engine, ui, driver | Keep per-tree (retitled as sub-project READMEs with a banner "part of final-prod; see root README"); NEW root README is the product entry. |
| `.gitignore` | engine, ui, driver | Keep per-tree (they scope subtree build outputs); NEW root `.gitignore` for cross-tree artifacts (`node_modules/`, `target/`, `dist/`, `build/`, `*.msix`, `bundle/`). |
| `.clang-format` | engine, ui? — **engine ✓, driver ✓** (ui has none) | Keep both (engine is C++20, driver is C17; formats may differ legitimately). |
| `LICENSE` | engine ✓ MIT, driver ✓ MIT, **ui has NO LICENSE file** | Keep per-tree MIT files; NEW root `LICENSE` (MIT, final-prod contributors). Note in INTEGRATION-GAPS that ui had none (fixed by root). |
| `.github/workflows/ci.yml` | engine, ui, driver | GitHub Actions only reads the **root** `.github/workflows/`; sub-tree copies are inert → **delete** all sub-tree `.github/workflows/*` in the T4 skeleton commit (history preserved in git; consolidated set rebuilt at root, §8.3). |
| `.github/workflows/release.yml` | engine, ui, driver | Same — delete; one root release workflow (§8.3). |
| `.github/workflows/{api-benchmark.yml, comparative-bench.yml}` | engine only | Delete; consolidated as root `engine-bench.yml` with `paths: engine/**`. |
| `.github/workflows/{ui-smoke.yml, visual-compare.yml}` | ui only | Delete; consolidated at root (§8.3). |
| `.github/workflows/benchmark.yml` | driver only | Delete; consolidated as root `driver-bench.yml` (the cross-driver comparison workflow, §8.3). |
| `docs/architecture.md`, `docs/build.md`, `docs/ground-truth-contracts.md`, `docs/requirements.md`, `docs/testing.md` | engine ✓ AND driver ✓ (near-pairs, each citing its own repo's evidence) | Keep both per-tree (different scopes: engine vs driver ground truth — do NOT merge; each cites its own verified evidence). Root `docs/` is product-level and namespaced (`docs/ARCHITECTURE.md` etc.), so there is no path collision (all sub-tree docs live under `engine/docs/`, `ui/docs/`, `driver/docs/`). |
| `docs/ARCHITECTURE.md`, `docs/TESTING.md` | ui only | Keep (UI-scoped); root docs are separate files with distinct scope, referenced from ui docs at T16. |
| `worklog/` (session-prompt archives) | engine (47 files), ui (4 files), driver | Keep — historical session prompts, evidence trail. |
| `tools/stamp_osversion.py` (driver) vs `tools/stamp_osversion_um.py` (engine); `tools/verify_pe.py` (driver) vs `tools/verify_pe_um.py` (engine) | engine + driver | **Keep both pairs** — they are deliberate near-twins: kernel (NATIVE subsystem) vs user-mode (UM) PE gates [DRIVER §9, CORE §1]. Document in `docs/ARCHITECTURE.md` to prevent a future "dedup" mistake. |
| `scripts/` directories | engine (33 files), ui (5), driver (3) | Contents are disjoint; keep per-tree. |

No file collides at merge time (disjoint sub-trees). All deletions above happen in a dedicated
**T4 skeleton commit after the three merges**, so the merge commits themselves are verifiable
against the source trees (see §2.4).

### 2.4 History-preservation verification (gate for T4)

1. `git ls-files engine | wc -l` = 769; `git ls-files ui | wc -l` = 385;
   `git ls-files driver | wc -l` = 66 — exact at the merge commits (before the T4 skeleton
   commit's deletions; driver tracked binaries noted [DRIVER §12: fuzz/selftest + unit_asan are
   tracked ELF binaries — left as-is, flagged in INTEGRATION-GAPS as repo hygiene]).
2. `git log --oneline -- engine | wc -l` ≥ 990 (plus merge commits); same for ui ≥ 62,
   driver ≥ 43.
3. `git log --follow -- engine/include/cr/cr_ui.h` reaches the file's creation in core_recovery
   history (rename-aware). Same for `--follow -- ui/src/connector/contract.ts`,
   `--follow -- driver/src/driver/rda_control.c`, `--follow -- driver/scripts/install.ps1`.
4. Pinned source commits reachable: `git merge-base --is-ancestor a574ecf… <merge1>`;
   `f4a8f8a… <merge3>`; `da7efb9… <merge2>`.
5. Byte-identity spot checks: `git show a574ecf:include/cr/cr_ui.h | git hash-object --stdin`
   equals `git hash-object engine/include/cr/cr_ui.h` (same for `ui/package.json`,
   `driver/rda.inf`).
6. No stray root files: `git ls-files | grep -v /` lists exactly the T4 root set
   (VERSION, README.md, LICENSE, worklog.md, .gitignore, build.ps1, build.sh).

---

## 3. Operation-by-operation wiring table — THE CORE

Legend: **CLEAN** = direct ABI call, no semantics delta beyond mechanical mapping.
**ADAPTED** = real ABI equivalent exists; UI or bridge changes listed. **GAP** = no cr_ui
equivalent → disabled-surface plan + `docs/INTEGRATION-GAPS.md` entry (G-n).
Threading: `run`-like calls are synchronous-on-caller-thread in the ABI → the Rust command
spawns a dedicated thread and joins it; `pause`/`resume`/`cancel`/telemetry/state queries are
any-thread token/state functions [CORE §6, `cr_ui.h:498-517`].

### 3.1 DevicesDomain (3 methods)

**1. `listDevices(): Promise<Device[]>` — ADAPTED.**
ABI: `cr_ui_enumerate_disks` (two-call, ≤200 disks) + `cr_ui_enumerate_volumes`
(`cr_ui_disk_info{number,size_bytes,logical_sector,physical_sector,removable,layout,table_ok,model[128],serial[128]}`,
`cr_ui_volume_info{letter,disk,offset_on_disk,bytes,fs_name[16],label[64],bitlocker}`)
[CORE §2.1]. Bridge groups volumes by `disk`. Field maps: `Device.capacity.totalBytes` ←
`size_bytes`; `usedBytes`/`freeBytes` ← Win32 `GetDiskFreeSpaceExW` per lettered volume (core
gives totals only). `Volume.bitlocker` ← `cr_ui_volume_info.bitlocker` / probe
`cr_ui_open_volume` → `CR_UI_ERR_FVE_LOCKED` (0x50060001) ⇒ `'locked'` [CORE §2.1]. Deltas:
- `DeviceKind` ('internal'|'usb'|'sd'|'optical'|'virtual'): `cr_ui_disk_info` has only a
  `removable` bool, **no bus type** → map removable→'usb', else→'internal'. **'sd' and
  'optical' cannot be distinguished → G-9.**
- `quickScanAvailable`: always `true` (core has no per-volume quick-scan gate; the mock's
  bad-sectors flip was simulation-only).
- `Volume.lostPartition` (pre-scan `LostPartitionHint`): **no pre-scan source** — lost
  candidates are scan-time `CR_EVT_LOST_CANDIDATE` events [CORE §2.1] → **G-10**; the field
  stays optional/undefined; LostPartitionPage's flow uses real scans instead (its static
  `RESCUE_DISK_SPECS` display is already decoupled [UI §11.3]).
- Virtual Recycle Bin/Desktop targets (`'vol-recycle-bin'`/`'vol-desktop'`, HomePage quick
  targets [UI §3.4 fixtures]): bridge maps them to volume `C:` + `pathScope`
  (`C:\$Recycle.Bin`, `%USERPROFILE%\Desktop`) — depends on the G-11 scope adaptation.
- `Device.status`: 'ok' default; 'locked' via 0x50060001; 'permission-denied' on open/access
  failure (`cr_ui_last_transport_error`, `cr_ui_last_error`); 'disconnected' is synthesized on
  re-enumeration absence (no hot-plug events in the ABI → G-8).

**2. `getDeviceInfo(deviceId)` — ADAPTED (unconsumed by UI [UI §3.4]);** implement as
enumerate + match (id may be device or volume id; connector resolves volume→owning device).

**3. `refreshDevices(): Promise<Device[]>` — CLEAN** (same enumerate calls, fresh pass).

### 3.2 ScanDomain (6 methods)

**4. `startScan(params: {targetId, pathScope?})` — ADAPTED (heaviest).**
Sequence (the documented binding lifecycle [CORE §3, `docs/CONNECTION-GUIDE.md:39-60`]):
`cr_ui_abi_version()` gate (=2) → resolve targetId → `cr_ui_open_disk(disk)` /
`cr_ui_open_volume(letter)` (locked ⇒ consent paths `cr_ui_open_volume_with_key` /
`cr_ui_unlock_volume_with_dictionary`, CORE §2.1; UI surfaces the three-choice prompt) →
`cr_ui_scan_create` with zero-init `cr_ui_scan_config` (memset-0 = documented defaults:
stage_mask 7, weights 40/35/25, threads auto 1..8 [CORE §7]) → `cr_ui_scan_attach_device`
(device borrowed, must outlive scan) → `cr_ui_scan_set_event_fn` **before** run →
spawn dedicated OS thread → `cr_ui_scan_run` (synchronous; returns 0 / engine error /
`CR_UI_ERR_CANCELLED` with tree intact). Bridge mints `scanId` ("scan-N") and registers
scanId → `{cr_ui_scan*, device, session?}` in the handle registry (§4.2).
Adaptations & deltas:
- **pathScope — G-11**: `cr_ui_scan_config` has NO folder-scope parameter [CORE §2.1]. The
  bridge runs the whole-volume/whole-disk scan and applies `cr_ui_query.path_prefix`-style
  scoping to every result view (tree, query, count, recovery). Semantics delta (documented in
  INTEGRATION-VALIDATION): scan duration and progress reflect the WHOLE volume; only results
  are scoped. The mock's `scan-scope-invalid` hard error remains bridge-generated for invalid
  paths. Future engine-side folder-scope capability goes to INTEGRATION-GAPS.
- Stage naming map (mock STAGE_ORDER → core stages): 'volume-analysis' ← bridge-synthesized
  open/probe phase; 'quick-index' ← quick stage; 'deep-walk' ← deep stage; 'carve' ← carve
  stage; 'tree-build' ← bridge-synthesized snapshot/finalize phase; 'done' ← run() return.
  `scanPhaseOf` mapping (quick: volume-analysis+quick-index; advanced: deep-walk+carve;
  organizing: rest) is unchanged [UI §5].
- Weights: mock 0.04/0.14/0.42/0.24/0.16 [UI §3.4] → core quick/deep/carve 40/35/25
  [CORE §7]. Bridge normalizes into `overallProgress` = telemetry `overall_permille`/1000,
  **clamped monotonic** (UI asserts monotonic overallProgress [UI §6]).
- `ScanProgressEvent` field map (from `cr_ui_scan_telemetry` + events, CORE §2.1):
  `bytesScanned` ← `bytes_scanned`; `bytesTotal` ← device geometry (or slice bytes);
  `filesFound` ← `files_found + deleted_found + carved_found` (documented sum);
  `errors` ← `error_ranges` count; `badSectorMap` ← `CR_EVT_ERROR_RANGE` ranges
  {start,len}→{startByte,sizeBytes} (+ M-049 module/op/attempts kept for logs);
  `etaSeconds`/`throughputBytesPerSecond` ← bridge-derived from bytes/elapsed;
  `stage`/`stageProgress` ← per-stage permilles (stage_progress resets per stage — matches
  UI contract); `recoverableDuringScan` ← true once a snapshot exists
  (`cr_ui_scan_snapshot`, partial=1, G-02 mid-scan value path [CORE §2.1]) — the mock's 0.4
  threshold was simulation; `deviceConnected` ← synthesized (G-8); `currentPath` ← **G-2**
  (no ABI event carries a current path → null → UI hides the ticker); `dataFoundBytes` ←
  **G-3** (no live found-bytes counter in telemetry → 0/null live, computed at completion).
- **Open question OQ-1** (resolved in T7 from `cr_ui.h`, not invented here): the exact
  recovery progress-callback struct fields (`cr_ui_recovery_progress_fn` payload) for
  per-item state.

**5. `pauseScan(scanId)` — CLEAN.** ABI `cr_ui_scan_pause` — any-thread token function
[CORE §6]; engine parks threads on a condvar (`PausableToken`), pause excluded from elapsed.
Budget <600 ms met structurally (CancelPoolWatcher polls the token every 50 ms).

**6. `resumeScan(scanId)` — CLEAN.** ABI `cr_ui_scan_resume`. The UI's
resume-as-reconnect affordance (reconnect modal) degrades per G-8.

**7. `cancelScan(scanId)` — CLEAN.** ABI `cr_ui_scan_cancel` (any thread); run() returns
`ERR_CANCELLED` (0x50020005) with the tree intact and browsable — exactly the contract the
mock emulated (cancelled scans keep browsable partial results + truthful banner [UI §6]).
Budget ≤300 ms structurally (50 ms poll + wind-down; measured in T14).

**8. `getScanState(scanId)` — ADAPTED.** ABI `cr_ui_scan_busy` / `cr_ui_scan_stopping` /
`cr_ui_scan_telemetry` (paused flag) [CORE §2.1]. Status map: busy&&!paused→'running';
paused→'paused'; stopping→'stopping'; bridge-synthesized 'starting' before run begins,
'pausing'/'resuming' as transients (token calls return immediately; the engine's pause is
observable via telemetry within the 500 ms UI poll [UI §3.4]); terminal
'completed'/'cancelled'/'failed' latched by the bridge from run()'s return code.
`startedAt`/`completedAt` bridge-tracked; `failure.code` via §3.9 error map.
`target` ← registry's stored Device/Volume.

**9. `onScanProgress(scanId, cb): () => void` — ADAPTED (events).**
Source: `cr_ui_scan_set_event_fn` — ONE sink per handle, subscribe BEFORE run, events are POD
`cr_ui_event` fired on **engine worker threads** (heartbeat on the shim watchdog thread,
save-progress on the save thread) [CORE §2.1, §6]. Bridge contract: sink must not block →
push into an mpsc channel; a dedicated emitter thread drains it and calls
`app_handle.emit("scan-progress:<scanId>", payload)`. TS side: `listen("scan-progress:<scanId>")`
→ callback → unsubscribe shape unchanged [UI §3.4, §6]. Event map (contracts/event-mapping):
CR_EVT_SCAN_STAGE(1)→stage change; SCAN_PROGRESS(2)→stage/overall permilles; FILES_FOUND(3)
→filesFound bump; ERROR_RANGE(4)→errors/badSectorMap; CHECKPOINT(5)→internal (no UI);
LOST_CANDIDATE(6)→lost-partition count/badge (future page wiring); REPAIR(7)→repair outcome
toast/consent result; HEARTBEAT(8)→liveness only (never synthetic progress); SAVE_PROGRESS(9)
→saveSession progress UI; FS_CHECK(10)→damage-audit count for the scan page. Unknown types
ignored (forward compat) [CORE §2.1]. The UI's 10%-bucket cache invalidation stays TS-side.

### 3.3 ResultsDomain (4 methods)

Node-id scheme **[load-bearing]**: the mock's ids are `'f<idx>'`/`'d<idx>'` and only mock
internals parse the prefix; the contract keeps ids opaque [UI §5, §6]. Bridge adopts the same
visible scheme over `cr_ui_snapshot` **indices**: `f<index>`/`d<index>` for files/dirs, with
registry entry storing (index → `node_id` u64, snapshot ref) so recovery enqueues the right
handle. `getNodeDetail`/`requestPreview`/`startRecovery` resolve ids through the registry —
zero UI change.

**10. `getResultTree(scanId, parentId, mode): Promise<TreePage>` — ADAPTED.**
ABI: mid-scan `cr_ui_scan_snapshot` (immutable value copy; partial=1; ~150–300 ms at 1M
nodes — bridge re-snapshots at a bounded cadence and caches) + `cr_ui_snapshot_node`
(parent/first_child/next/prev index navigation) + `cr_ui_snapshot_dfs`. Post-scan: session
load + same snapshot path (`cr_ui_session_snapshot`). Modes:
- 'path': existing+deleted structured nodes under root; **virtual grouping nodes are
  bridge-synthesized**: `'lost-paths'` ← origin==2 (lost) nodes; `'raw-all'`/`'raw-found'` ←
  origin==1 (carved) [CORE §2.1 node.origin: 0 structured/1 carved/2 lost/3 virtual]. This
  reproduces the mock's 3-branch path-mode tree [UI §3.4] from real node origins.
- 'type': 8 category nodes + `ext:<cat>:<ext>` leaves — bridge aggregates via
  `cr_ui_query_run` count_only per category (or snapshot pass) over the session.
- 'raw': single raw-found group.
`TreeNode.childCount/fileCount/sizeBytes` ← bridge subtree aggregates (snapshot pass);
`special` flags from deleted state/origin. `TreePage.parentHasMore` ← index-window paging
(bridge-side page size).
- `NodeDetail.clusterCount` ← `node.extent_count`; `createdAt`/`accessedAt` ← node 100ns-1601
  timestamps → ISO; `confidence` ← grade 0..4 → 'poor'/'fair'/'good'/'excellent' (+ 'unknown')
  per contracts/type-maps; `badSectorsOverlap` ← bridge intersects node extents with the
  session error map; `confidenceReason` ← bridge composes i18n key from grade/carve_format/
  hash_valid/sha128 evidence.

**11. `queryResults(scanId, filter, sort, page): Promise<ResultsPage>` — ADAPTED.**
ABI: `cr_ui_query_run(cr_ui_session*, …)` — **session trees only** (stable trees) [CORE §2.1].
Post-scan/completed/cancelled: direct. Mid-scan (live 3-pane browsing — a core UI feature
[UI §2 route `/scan/:deviceId/live`]): **no session handle exists yet** → the bridge
implements the filter/sort/page semantics over the cached `cr_ui_snapshot` in Rust (G-7; the
mock's `visibleFraction` scaling is replaced by the REAL partial tree — snapshot partial=1).
Maps: `search`→`name_substring`; `extensions`→`extensions[]` (lowercased with dot);
`categories`→`category_mask` (**OQ-2**: exact bit assignments beyond "1 image … 8 bookmarks"
[CORE §2.1] pinned from `cr_ui.h` into contracts/type-maps at T5); `sizeMin/MaxBytes`→
`size_min/max`; `dateFrom/To`→`modified_from/to`; `origins`→origin_mask + deleted filter
(existing←structured,deleted==0; deleted←structured,deleted≠0; raw←carved; lost-path←lost;
unsaved←virtual/special, bridge-mapped); `hideEmptyFiles`→`size_min=1`; sort
name/modified/type/size/confidence → core sort 1/3/7/2/4, dir asc/desc→0/1; paging via
`offset/limit` (UI `RESULTS_PAGE_SIZE=200` [UI §3.4]). `ResultsPage.elapsedMs` ← real
measurement (budget: core's own contract is filter+count over 1M nodes <200 ms [CORE §8] —
matches the UI's <200 ms budget).

**12. `countResults(scanId, filter)` — ADAPTED** — same path as 11 with `count_only=1`.

**13. `getNodeDetail(scanId, nodeId): Promise<NodeDetail>` — ADAPTED.** Snapshot node row +
path reconstruction (bridge walks parent indices; query rows carry '/'-joined paths when a
query already ran). See field maps in 10/11.

### 3.4 PreviewDomain (2 methods)

**14. `requestPreview(scanId, nodeId): Promise<PreviewPayload>` — ADAPTED (with G-4).**
ABI: `cr_ui_preview_open_snapshot`/`_open_session` (device must outlive preview) →
`cr_ui_preview_size` / `_metadata` (`kind[24]`: image|video|audio|document|archive|text|
unsupported; format; width/height; duration_ms; codec; text_snippet[256]; truncated)
/ `cr_ui_preview_read` (**len capped 4 MiB/call**, short only at EOF; unreadable spans
zero-filled AND reported via `cr_ui_preview_unreadable`) [CORE §2.1].
Kind dispatch (connector decides, ADR-008 [UI §3.2]):
- 'hex' (default): `preview_read` 64 KiB → `{bytes: Uint8Array, totalSize, baseOffset}` —
  bounded payload principle ADR-003 [UI §6] matches the core's 4 MiB cap + 4 MiB work cap
  [CORE §7].
- 'text': text_snippet (256 B) + bounded read → `{text, encoding, totalSize, truncated}`.
- 'image': raw file bytes → base64 `dataUrl` (mime from `metadata.format`).
- 'pdf'/'av'/'office': **G-4** — the core provides bytes+metadata only, no renderer. v1:
  metadata + hex fallback (truthful "preview not rendered"); rendered PDF pages / AV
  thumbnails / office extracted-text are INTEGRATION-GAPS (candidates: none in scope — no
  renderer component exists on any side; do not invent).
- Error mapping: `metadata.kind == 'unsupported'` / encrypted-ext files →
  `preview-unsupported`/`preview-encrypted` analogues; unreadable spans → `preview-damaged`
  analogues — **all from real core signals** (zero-filled-span report), not injected.

**15. `getThumbnail(scanId, nodeId)` — ADAPTED.** Bridge decodes image bytes (only when
`thumbnailAvailable` from category) to a small PNG dataUrl, bounded by the 4 MiB preview
window (**OQ-3**: decode cost policy for huge RAW images — cap decode to header-adjacent
window, else null). Non-image thumbnails → null (G-4).

### 3.5 SessionsDomain (4 methods)

Session store: `%ProgramData%\HeicFast\Recovery Wizard\sessions\` (§7.2; the core itself
writes ONLY caller-supplied paths [CORE §4.3] — the bridge supplies this directory).
Format `.crsf` (CRSFMT01, 9 record types, atomic temp-then-rename) [CORE §4.1] replaces the
reference `.rsf` — the point of the product; auto-naming parity N-061
`auto_<label sanitized>_<M>_<D>_<H>_<m>_<s>` [CORE §4.1] matches the reference's
`auto_<volume>_<date>` convention [LAYOUT §7].

**16. `saveSession(scanId, path|null) — ADAPTED.** ABI `cr_ui_scan_save_session` (atomic;
valid after run incl. cancelled) + `cr_ui_scan_save_cancel` (SAVE-SCOPED cancel token)
+ `CR_EVT_SAVE_PROGRESS` for the save spinner [CORE §2.1]. `path=null` → bridge composes the
ProgramData auto-name. `SessionMeta.filesFound` ← `cr_ui_scan_get_stats`;
`dataFoundBytes` ← computed once via snapshot sum (G-3; O(n) once at save is acceptable).
`completed` ← run status.

**17. `listSessions() — ADAPTED.** No ABI enumerate exists; bridge enumerates the sessions
directory (`*.crsf`), stat()s for fileName/savedAt, and lazily `cr_ui_session_load` +
`cr_ui_session_get_info` + `cr_ui_session_stats` for targetLabel/filesFound/dataFoundBytes —
**OQ-4**: listing cost vs. loading every session (e.g. the corpus reference sessions are
7–52 MB [LAYOUT §7]); decide cache-on-save vs load-on-list at T7. No sidecar index file
(avoid new persisted formats).

**18. `loadSession(path): Promise<{scanId}> — ADAPTED.** ABI `cr_ui_session_load` →
registry entry (browsable session for results/recovery) → `cr_ui_session_get_info` /
`_stats`; resume-to-continue-scan = `cr_ui_session_resume` + `cr_ui_scan_verify_identity`
(BAD_CHECKSUM 0x50030002 → `session-corrupt`) [CORE §2.1, Appendix A step 10]. Corrupt
container: torn record → repaired-or-refused 0x50040001 [CORE §5] → `session-corrupt`.
The HomePage/BackupPage flows keep their shape [UI §3.4 call sites].

**19. `getSessionMeta(path) — ADAPTED (unconsumed by UI)** — same meta read as 16/17.

### 3.6 RecoveryDomain (6 methods)

**20. `startRecovery(scanId, selection, destination, options): Promise<{jobId}> — ADAPTED.**
ABI sequence: `cr_ui_recovery_create(source_device, config)` → `cr_ui_recovery_set_destination`
(drive-letter destinations arm the F12 overlap refusal → `ERR_DEST_OVERLAP` 0x50050001
[CORE §2.1]) → enqueue → spawn thread → `cr_ui_recovery_process` (synchronous; committed
items stay committed on cancel) [CORE §6].
Config map [CORE §7 `cr_ui_recovery_config`]: read_chunk default 1 MiB; rename_attempts 64;
`collision` 'rename'|'skip'|'overwrite' → conflict 0|1|2; `verifyOnWrite` →
`skip_hash_verify` (verify ON default); `rebuildFolders=false` → `skip_dir_reconstruct=1`;
`keepEmptyFolders` → **G-5** (no core equivalent; option hidden/disabled with truthful
note); `fileRepair` → **G-5** (core repair is pre-scan RawFixer, not during-recovery; the
switch is disabled with truthful tooltip).
Selection: 'nodes' → per node `cr_ui_recovery_enqueue_node(recovery, session, node_id)` /
dirs `cr_ui_recovery_enqueue_subtree` (mid-scan: `_snapshot`/`_subtree_snapshot` variants,
G-02b race-free [CORE §2.1]); 'all'+filter → bridge pages the filtered query (same filter
path as 11) and enqueues every row (files + dirs). Per-call enqueue errors (e.g.
0x50050001) surface as item-level failures. jobId registry → `{cr_ui_recovery*, items ring,
report}`.

**21. `cancelRecovery(jobId) — CLEAN.** ABI `cr_ui_recovery_cancel` (any thread);
`process` returns `CR_UI_ERR_CANCELLED`; already-copied files remain (matches the UI's
confirm-copy [UI §6]).

**22. `getRecoveryState(jobId): Promise<RecoveryJob> — ADAPTED.** ABI
`cr_ui_recovery_get_stats` `{queued, done, refused, errors, hash_mismatch, resumed,
salvaged, dirs, nothing_to_recover}` + queue_count [CORE §2.1]. Status map: 'validating'
(bridge enqueue phase) → 'running' (process in flight) → 'completed' (done>0 &&
errors==0&&refused==0&&hash_mismatch==0) / 'completed-partial' (done>0 with
errors/refused/hash_mismatch) / 'cancelled' / 'failed' (fatal pre-stats error).
`nothing_to_recover` → completed with truthful empty report.

**23. `getRecoveryItems(jobId, page): Promise<readonly RecoveryItem[]> — ADAPTED.**
Per-item truth: engine `cr_ui_recovery_report_json` items[] (outcome/reason/duration/sha256)
[CORE §4.2] + live per-item progress callback (OQ-1 pins the callback struct). Bridge keeps a
bounded item ring (last N, mirroring the report's 500-item cap the UI already renders with
`truncatedItemLists` [UI §3.4]); state map done/refused→'skipped'/error→'failed'/
hash_mismatch→'failed'(reason)/pending→'pending'/running→'running'.

**24. `onRecoveryProgress(jobId, cb) — ADAPTED (events).** ABI progress fires on worker
threads, must not block [CORE §6] → same channel→emit marshaling as 9
(`recovery-progress:<jobId>`). `RecoveryProgressEvent` fields: completedCount/failedCount/
skippedCount/totalCount ← live counters (callback + stats); currentFile/currentFileProgress
← callback (OQ-1); verifying ← hash-verify phase signal; throughput/eta ← bridge-derived.

**25. `getRecoveryReport(jobId): Promise<RecoveryReport> — ADAPTED (cleanest map).**
ABI `cr_ui_recovery_report_json` (schema_version 1; keys totals{queued,done,dirs,refused,
errors,hash_mismatch,resumed,salvaged,nothing_to_recover,pending,bytes} + items[{item_id,
path,name,kind,outcome,reason,bytes,duration_ms,sha256,hash_status}]) [CORE §4.2] — bridge
parses (serde) and maps: succeeded/failed/skipped lists (≤500 + truncatedItemLists, exact
`processedTotal` — same truncation contract the mock shipped [UI §3.4]);
`totalBytesWritten` ← totals.bytes; `verified` ← count(hash_status=='verified');
`status` from 22. The report JSON export flow (Blob download `recovery-report-<jobId>.json`)
now exports the ENGINE's report (validated by `engine/tools/validate_report.py` upstream).

### 3.7 HealthDomain (1 method)

**26. `getHealth(deviceId): Promise<DiskHealth> — ADAPTED.** ABI `cr_ui_health_read(disk,
rep, attrs, cap, count)` — one call; Windows-only (else `ERR_HEALTH_UNAVAILABLE`
0x50070001 → `health-unavailable`) [CORE §2.1]. Maps: status ← 4-status grading
Unknown/Good/Caution/Bad [CORE §2.1 health grading] → 'good'|'caution'|'bad' (Unknown →
`health-unavailable`-style truthful state, keyed in contracts); temperatureC ←
`temperature_c` (ATA) / `temperature_c_x10` (NVMe); powerOnHours ← `power_on_hours` /
`power_on_hours_nvme`; diskClass ← `ssd_like` → 'ssd'|'hdd'|'unknown'; wearPercent ←
`percentage_used` (NVMe) / `ssd_wear_level`; trimEnabled ← `trim_known ? trim_enabled :
false`; reallocated/pendingSectors ← report fields; `SmartAttribute{id,name,current,worst,
threshold,raw,status}` ← attr row `{id,value,worst,raw(6 vendor bytes),failing,name[40]}`:
current←value, worst←worst, **threshold←null (core has no threshold field)**, raw←u48,
status←failing?'failing':'ok'.

### 3.8 RepairDomain (4 methods) — the one true capability GAP cluster

Evidence: the 78 cr_ui exports contain **no standalone repair surface**. RawFixer
analyze/execute/undo exist only in the C++ engine API (`cr::repair::analyze/execute/undo`
[CORE §2.3]) and are exposed through the ABI **only as pre-scan repair inside a scan**:
`cr_ui_scan_config.pre_scan_repair` + `repair_consent` callback (default-deny), outcomes via
`CR_EVT_REPAIR` (0 plan-ready/1 applied/2 denied/3 failed), undo log via
`cr_ui_scan_repair_save_undo` (CRUNDO01) [CORE §2.1, §7].

**27. `dryRunRepair(deviceId): Promise<RepairDryRun>` — GAP (G-1).** No analyze export.
Disabled-surface plan: `VolumeRepairWorkbench` findings list renders a truthful
"engine dry-run analysis is not exposed by the current engine ABI" state; the REAL repair
path offered instead is the scan-integrated flow: start scan with `pre_scan_repair=1` +
consent UI → findings arrive as `CR_EVT_REPAIR` plan-ready events.
`docs/INTEGRATION-GAPS.md` entry G-1 records the proposed append-only ABI additions
(`cr_ui_repair_analyze/apply/undo` — version policy allows appends without an ABI bump
[CORE §2.1]).

**28. `applyRepair(deviceId, {consentToken:'REPAIR'}) — ADAPTED via the scan path.**
The typed-'REPAIR' consent UI is kept and wired to `cr_ui_repair_consent_fn` inside the scan
flow (default-deny honored: NULL/never-called consent writes NOTHING [CORE §7]); the
workbench's standalone apply button is disabled per G-1 until the ABI exports exist.

**29. `getUndoLog(deviceId) — GAP (G-1).** No ABI read of undo entries. The bridge surfaces
undo-log FILE existence/size (saved via `cr_ui_scan_repair_save_undo` at a ProgramData
path); the entry list UI shows the truthful unavailable state.

**30. `undoRepairs(deviceId) — GAP (G-1).** Undo is CLI-only today (`rrecov fix
--undo-apply` [CORE §2.2]) — no cr_ui export. Disabled with truthful note; recorded in G-1.
(Lost-partition apply undo DOES exist — `cr_ui_apply_undo_last`/`_save_undo` [CORE §2.1] —
and is wired when the lost-partition surface lands; not part of the 36.)

### 3.9 SystemDomain (6 methods)

**31. `getDriverStatus(): Promise<DriverStatus> — ADAPTED.** Bridge probes the RDA device
(bare CreateFile, same probe semantics as the core's `rda_available` [CORE §10]) and, if
open, `IOCTL_RDA_GET_INFO` (0x60002440 → `{MajorVersion, MinorVersion, Flags, MaxDisks,
IoctlBase}`) / `IOCTL_RDA_GET_VERSION` (0x60002448 → (Major<<32)|Minor) [DRIVER §2.3] →
`{installed, version: "1.0", running, note}`. installed = SCM service exists;
note carries the truthful unsigned/test-signing posture [DRIVER §7].
**OQ-5 (load-bearing)**: the core probes `\\.\RDA` [CORE §10] while the driver derives its
device name from the SERVICE name (`\\.\{UPPER(SN)}`, `rdacli` hardcodes EPRDTDRV)
[DRIVER §2.1, §10]. The installer therefore pins **ServiceName = `RDA`**
(unless `engine/include/cr/win/cr_transport.h` proves a different constant — verify at T7;
the driver's WinPE profile already uses service name `rda` [DRIVER §4.4]).

**32. `getAppInfo(): Promise<AppInfo> — ADAPTED.** `{name, version, engineVersion,
platform}`: name ← Tauri productName; version ← root VERSION (§6); platform ← OS info.
**Contract change (T5):** `connectorMode: 'mock'|'tauri'` is REMOVED from
`AppInfo` (and from `contract.ts`) — with the mock gone the mode is always the real
connector; Settings/About connector-mode badge + `useConnectorMode` + the mock-check in
ResultsExplorer ("Open location" disable) are deleted [UI §3.3, §3.4 dev.ts, §11.2].
`engineVersion` ← **G-6**: no cr_ui export carries an engine version string
(`cr_ui_abi_version` is the ABI number 2, not a product version [CORE §2.1]) → null in v1;
G-6 proposes an append-only `cr_ui_engine_version` export.

**33. `validateDestination(path, sourceDeviceId, neededBytes) — ADAPTED.** Rust-side:
resolve source device/volume letters; same-letter destination → `destination-overlap`
(hard refusal, reasonKey `recovery.overlapReason` — parity with the mock's rule [UI §3.4]);
path not under a mount point → 'invalid'; `GetDiskFreeSpaceExW` freeBytes < neededBytes →
'out-of-space'; writability probe → 'unwritable'. The core backstops at
`cr_ui_recovery_set_destination` (ERR_DEST_OVERLAP/DEST_UNWRITABLE/DEST_FULL 0x5005000x)
[CORE §5] — defense in depth, same codes.

**34. `pickDirectory(hint) — CLEAN** (no core involvement): `tauri-plugin-dialog`
(already a dependency, staged but unused [UI §8]) with volume hint root /
`excludeDeviceId` filtering.

**35. `listMountPoints(): Promise<readonly MountPoint[]> — ADAPTED.**
`cr_ui_enumerate_volumes` letters + `GetDiskFreeSpaceExW` → `{id, label, path, freeBytes,
totalBytes}`.

**36. `openPath(path) — CLEAN.** `tauri-plugin-opener` (or the staged
`reveal_in_explorer` command) — this WIRES the currently-disabled "Open location" context
item [UI §11.2] and retires the mock's honest `errors.openPathMock` refusal.

### 3.10 Tally

| Classification | Count | Methods |
|---|---|---|
| CLEAN | 7 | refreshDevices, pauseScan, resumeScan, cancelScan, cancelRecovery, pickDirectory, openPath |
| ADAPTED | 26 | listDevices, getDeviceInfo, startScan, getScanState, onScanProgress, getResultTree, queryResults, countResults, getNodeDetail, requestPreview, getThumbnail, saveSession, listSessions, loadSession, getSessionMeta, startRecovery, getRecoveryState, getRecoveryItems, onRecoveryProgress, getRecoveryReport, getHealth, applyRepair (scan-path), getDriverStatus, getAppInfo, validateDestination, listMountPoints |
| GAP | 3 | dryRunRepair, getUndoLog, undoRepairs |

### 3.11 Integration gaps register (→ `docs/INTEGRATION-GAPS.md`)

- **G-1 Standalone repair workbench** (dryRunRepair/getUndoLog/undoRepairs): no ABI exports;
  scan-integrated repair flow ships instead; append-only ABI additions proposed.
- **G-2 `currentPath` ticker**: no cr_ui event carries the file being scanned; UI hides the
  "Searching: <path>" line (stage label remains).
- **G-3 `dataFoundBytes` live counter**: no aggregate found-bytes in telemetry/stats; live
  value 0/null, exact value computed at completion via snapshot sum.
- **G-4 Rendered previews (pdf pages / av thumbnail / office extractedText)**: core gives
  bytes+metadata only; image/text/hex previews are real; others show metadata+hex fallback.
- **G-5 Recovery options `keepEmptyFolders` + `fileRepair`**: no core equivalents; disabled
  with truthful UI notes.
- **G-6 Engine version string**: no ABI export; `AppInfo.engineVersion` null v1; propose
  `cr_ui_engine_version` (append-only compatible).
- **G-7 Mid-scan query**: `cr_ui_query_run` is session-only; bridge implements snapshot-side
  filtering in Rust for live browsing (real partial tree replaces the mock's
  visibleFraction); a future ABI snapshot-query would retire bridge logic.
- **G-8 Device disconnect mid-scan**: no device-arrival/gone events in the ABI;
  `deviceConnected` synthesized from I/O error patterns; the reconnect modal degrades to
  pause+resume (resume re-probes the device).
- **G-9 Device bus kinds**: `cr_ui_disk_info` has no bus type — 'sd'/'optical' cannot be
  distinguished from 'usb'; SdCardPage hero remains static display (already is [UI §11.3]).
- **G-10 Pre-scan `Volume.lostPartition` hint**: lost candidates are scan-time events; the
  Home badge uses post-scan knowledge only; LostPartitionPage scans for real.
- **G-11 Folder `pathScope`**: engine lacks folder-scoped scan; scope = whole-volume scan +
  path-filtered views (semantics delta: scan duration is whole-volume).
- (Hygiene, non-behavioral: driver repo ships tracked ELF fuzz binaries [DRIVER §12];
  ui repo had no LICENSE [§2.3]; `rda.cat` declared in INF but never built [DRIVER §3].)

---

## 4. Rust bridge design (`ui/src-tauri`)

### 4.1 Module layout (new code inside the ui sub-tree — no engine files touched)

```
ui/src-tauri/src/
├── main.rs, lib.rs            (invoke_handler registers the bridge command set, §4.3)
├── bridge/
│   ├── abi.rs                 (cr_ui FFI declarations — generated/pinned from engine/include/cr/cr_ui.h;
│   │                           static link against cr_ui.lib; runtime gate cr_ui_abi_version()==2)
│   ├── state.rs               (handle registries, §4.2)
│   ├── events.rs              (channel + emitter thread; event→payload translation)
│   ├── consent.rs             (consent request/response plumbing, §4.5)
│   ├── error.rs               (cr u32 → BridgeError{code, message, cr_last_error text})
│   └── util.rs                (two-call helpers, string/rows freeing, time conversion)
└── commands/
    ├── mod.rs, system.rs      (existing commands/system.rs retained where reused, §4.6)
    ├── devices.rs, scan.rs, results.rs, preview.rs, sessions.rs, recovery.rs, health.rs
```

### 4.2 State management

- `BridgeState` (Tauri managed state): `Mutex<HashMap<String /*scanId*/, ScanEntry>>`,
  `Mutex<HashMap<String /*jobId*/, JobEntry>>`, consent pending map (§4.5).
- `ScanEntry { scan: *mut cr_ui_scan, device: *mut cr_ui_device, session: Option<*mut
  cr_ui_session>, snapshot: Option<*mut cr_ui_snapshot>, last_snapshot_ms, node_index_map,
  started_at, terminal: Option<ScanTerminal> }` — scanId "scan-N" minted monotonically.
- `JobEntry { recovery: *mut cr_ui_recovery, source_device, items_ring, report_cache, ... }`.
- Lifetime ordering enforced on drop/close (per contract): preview closes before
  session/snapshot; session/snapshot before scan; scan before device; device last
  (`cr_ui_preview_*` device-must-outlive; scan device borrowed [CORE §2.1]).
- One controlling thread per run-handle: run/process execute on dedicated spawned threads;
  all token/state queries are any-thread safe [CORE §6].

### 4.3 Command surface (manifest mirrored in `contracts/command-surface.json`)

`devices_list`, `devices_refresh`, `device_info` · `scan_start`, `scan_pause`, `scan_resume`,
`scan_cancel`, `scan_state` · `results_tree`, `results_query`, `results_count`, `node_detail` ·
`preview_open`, `preview_read`, `preview_close`, `preview_thumbnail` · `session_save`,
`sessions_list`, `session_load`, `session_meta` · `recovery_start`, `recovery_cancel`,
`recovery_state`, `recovery_items`, `recovery_report` · `health_read` ·
`consent_respond(requestId, allow)` · `system_driver_status`, `system_app_info`,
`system_validate_destination`, `system_pick_directory`, `system_list_mounts`,
`system_open_path`.
Events emitted: `scan-progress:<scanId>`, `recovery-progress:<jobId>`,
`consent-request:<consentId>`, `save-progress:<scanId>`.

### 4.4 Thread marshaling & memory rules

- Synchronous engine calls (`cr_ui_scan_run`, `cr_ui_recovery_process`) run on dedicated
  `std::thread`s; the Tauri command spawns and returns immediately (state via events/state
  queries) — the UI's promise-shaped contract (start → poll/subscribe) is preserved.
- Event sinks (worker threads, heartbeat watchdog thread, save thread [CORE §6]) only
  `try_send` into an mpsc channel — never block, never lock the registry in the sink.
  A single emitter thread translates `cr_ui_event` → JSON payload →
  `app_handle.emit(...)`. Unknown event types ignored [CORE §2.1].
- Two-call patterns (enumerate_disks/volumes, snapshot_dfs, device_transport) implemented in
  `util.rs`. Heap results freed by the owning side: `cr_ui_query_rows_free` for query rows;
  `cr_ui_string_free` for report JSON and any `char*`; `cr_ui_disk_map_free` per overlay
  array; handles via their `_close` exports [CORE §2.1]. Strings cross to TS as UTF-8
  (serde); timestamps convert 100ns-1601 → ISO-8601 in the bridge (single place).

### 4.5 Consent wiring (default-deny honored)

`cr_ui_repair_consent_fn` / `cr_ui_apply_consent_fn` are answered from the UI: the callback
(ON THE ENGINE THREAD) registers a pending consent `(consentId, description, scope)` and
blocks on a oneshot channel; the emitter thread emits `consent-request:<consentId>`; the UI
shows the typed-consent prompt; the user's answer arrives via the `consent_respond`
command → the oneshot resolves true/false. Deadlock analysis: the engine thread blocks only
on the oneshot (no registry locks held); the emitter thread never joins the engine thread;
UI non-response = the prompt stays open (a user decision, not a timeout) — **default-deny**
comes from the engine contract itself (NULL/never-called consent = deny [CORE §7]) plus a
bridge-side deny-once-confirm safety when the window/subscription is gone.

### 4.6 Error propagation

Every command returns `Result<T, BridgeError>` where
`BridgeError { code: u32 /*cr numeric space*/, cr_message: String /*cr_ui_last_error*/,
bridge_code: Option<String> }`. TS side (`tauri-connector.ts`) maps through
`contracts/error-codes.ts`: 0x50020001 open-failed→'device-permission'|'unknown';
0x50020005→scan status 'cancelled' (not an error surface); 0x50030002/0x50040001-3→
'session-corrupt'; 0x50030008→'not-implemented'|'preview-unsupported'; 0x50050001→
'destination-overlap'; 0x50050002→'destination-unwritable'; 0x50050004→'out-of-space';
0x50060001→volume 'locked' + consent prompt; 0x50070001→'health-unavailable';
0x50080004→'repair-consent-required'; 0x50090001→'unknown'. Bridge-generated codes keep the
UI vocabulary: 'scan-not-found', 'scan-scope-invalid', 'preview-*', 'recovery-item-failed'.
Codes never string-matched in UI (reasonKey i18n only [UI §7]).

### 4.7 Replacement of the 3 staged Rust commands

`src-tauri/src/commands/system.rs` today registers `reveal_in_explorer`, `path_exists`,
`app_environment` with **zero** frontend callers [UI §3.4]. Disposition: `reveal_in_explorer`
→ absorbed into `system_open_path` (tauri-plugin-opener; behavior kept);
`path_exists` → folded into `system_validate_destination`; `app_environment` → folded into
`system_app_info`. The old names are removed from `invoke_handler` at T7 (no call sites
break — zero invoke() exists today [UI §3.4]).

### 4.8 ABI binding

Static import (link `cr_ui.lib`, deploy `cr_ui.dll` + vcpkg runtimes `z.dll/lz4.dll/zstd.dll`
+ app-local VC143 CRT + UCRT beside the exe — the complete host-binding kit [CORE §9]).
Startup gate: `cr_ui_abi_version() == 2` else the shell fails fast with a surfaced error
(refuse to bind on mismatch [CORE §2.1]). A `libloading` dynamic path is NOT used (static
import is simpler, matches the release bundle's import-lib contract, and avoids dual
definitions). `build.rs`/`Cargo.toml` gain the engine artifacts via the build script (§8).

---

## 5. Simulated-backend removal inventory

### 5.1 Files DELETED (from `ui/`)

| File | Lines | Note |
|---|---|---|
| `src/connector/mock/mock-connector.ts` | 1603 | the simulator [UI §3.4] |
| `src/connector/mock/fixtures.ts` | 569 | 11-scenario device worlds + virtual targets [UI §3.4] |
| `src/connector/mock/node-space.ts` | 945 | procedural typed-array index (ADR-004) [UI §3.4] |
| `src/connector/mock/rng.ts` | 28 | mulberry32 PRNG [UI §3.4] |
| `src/connector/tauri/not-implemented.ts` | — | loud-stub helper [UI §3.4] |
| `src/queries/dev.ts` (scenario part) | — | `useScenarioControl()` [UI §3.4] |
| `src/features/dev-panel/dev-panel.tsx` scenario section | — | ScenarioControl UI (DEV builds) [UI §3.4] |
| `tests/connector-mock.test.ts` | 570 | re-authored (§5.4), then deleted |
| `tests/node-space.test.ts` | — | tests the deleted NodeSpace |
| `.env`/docs references to `VITE_CONNECTOR` | — | README.md, docs [UI §3.3] |
| `.github/workflows/{ui-smoke,visual-compare,release}.yml` `VITE_CONNECTOR: mock` env pins | — | sub-tree workflows already deleted at T4; root workflows never set it [UI §3.3] |

`src/connector/tauri/tauri-connector.ts` (36-line throwing Proxy) is **replaced** by the real
implementation (not merely deleted). `src/connector/index.ts` (51 L) is rewritten: no env
gate, no `scenarioControl()`; it exports only `connector` (the TauriConnector singleton) —
the factory still exists as THE import point (arch-lint A1 unchanged).

### 5.2 Contract changes (in `src/connector/contract.ts`)

- Remove `ScenarioControl`, `MockScenarioId` (contract.ts:644-662) [UI §3.1].
- Remove `AppInfo.connectorMode` (§3.9 #32). All other 36 signatures + payload types
  UNCHANGED — the seam survives intact (that is the whole design).
- i18n sweep: remove `devPanel.scenario*`, `settings.connectorMode` keys; `scripts/i18n-sweep.mjs`
  must stay missing=0/unused=0 [UI §1, §9].

### 5.3 AppInfo / mode displays

Settings/About connector-mode badge removed; About shows `engineVersion` (or "—" per G-6) +
driver status (already present). ResultsExplorer's mock-gated "Open location" disable is
removed — the item becomes always-enabled (real `openPath`) [UI §3.4, §11.2].

### 5.4 Tests re-authored (which tests used the mock's scenario control)

- `tests/connector-mock.test.ts` (570 L) — the entire contract suite ran the real MockConnector
  [UI §9]: **split** into (a) `tests/connector-tauri.test.ts` — the same behavioral
  assertions (stage order, monotonic overall, pause/resume <600 ms, cancel ≤300 ms, filter
  <200 ms, preview kind dispatch, overlap refusal, REPAIR consent, report truncation)
  executed against the REAL bridge in a Tauri test harness (`#[tauri::test]`/mock window +
  real engine) — runs in CI on windows; (b) UI-logic tests (component/query behavior) re-based
  on a **labeled test double** (§5.5).
- Component tests that instantiated the mock (`tests/components.test.tsx`, home-states,
  reference-surfaces, dialog-choreography, format/logger tests are mock-free) — re-based on
  the double [UI §9].
- E2E: `e2e/smoke.spec.ts` + `e2e/responsive.spec.ts` (12 tests) run the real mock inside the
  built bundle via `vite preview` today [UI §9] → the **test-mode alias** (§5.5) keeps them
  running in the browser with the double; a second, smaller bridge-e2e lane exercises the
  real app under `tauri dev`/WebDriver where feasible (stretch, §9).
- `visual-compare` baselines re-captured under the test double (labeled as such in
  INTEGRATION-VALIDATION; the previous mock screenshots are retired).
- Surviving unchanged: i18n tests, theme tests, store tests (pure), `ui-store`, `filter-store`
  mapping tests, logger/format tests, `tests/fixtures/adopted-tokens.snapshot.json` [UI §9].

### 5.5 Clearly-labeled test-scope doubles that remain

```
ui/tests/fakes/
├── test-connector.ts     (implements RecoveryConnector; deterministic fixture device set;
│                          labeled "TEST DOUBLE — never shipped"; no scenario injection beyond
│                          what tests need; no rng exported to src/)
└── fixtures.ts           (small static device/volume/node fixtures)
```
Wired ONLY via a Vite resolve-alias override in the `test` mode
(`ui/vite.config.ts` modes: production alias `@/connector` → `src/connector`; mode `test`
aliases it to `../tests/fakes/test-connector`). Production `pnpm build` cannot reach the
double (no env branch in src/, no dynamic import). Arch-lint gains **A7: no import of
`tests/fakes/**` from `src/**`**; ast-grep rule added alongside the existing pair
[UI §3.5]. knip keeps `tests/**` entries [UI §9].

### 5.6 VITE_CONNECTOR removal consequences

- Build-time constant fold in `index.ts` gone; `connectorMode()` deleted.
- CI env pins gone (all four workflows [UI §3.3]); e2e/visual jobs build with
  `vite build --mode test` (alias switch) — production/release builds pin nothing.
- `.env.example` loses nothing (it never had the var [UI §3.3]); README/docs updated (§10).

---

## 6. Configuration reconciliation

| Setting | Single source of truth | Consumed by | Persists where |
|---|---|---|---|
| Product version | root `VERSION` (e.g. `1.0.0`) | engine CMake `project(… VERSION)` [CORE §1]; ui `package.json` + `tauri.conf.json` + `Cargo.toml` (kept in lockstep [UI §1]); driver `rda_version.h` FILEVERSION + `rda.inf` DriverVer [DRIVER §1]; NSIS `!define PRODUCT_VERSION` | `VERSION` file; `build.ps1` gate fails on any mismatch (§8.2) |
| Scan tuning (stage_mask, weights, threads, fs_hint, raw_category_name, memory_warn_mb, pre_scan_check) | engine zero-init defaults [CORE §7] — the UI deliberately exposes NO scan config (ADR-008 [UI §2]) | bridge passes zero-init config | none (per-scan, in-memory) |
| Repair consent | `cr_ui_repair_consent_fn` wiring (default-deny) [CORE §7] | bridge §4.5 | none |
| Recovery options (collision, verifyOnWrite, rebuildFolders) | UI `RecoveryOptions` → mapped into `cr_ui_recovery_config` per `contracts/type-maps.md` | bridge | none (per-job) |
| keepEmptyFolders / fileRepair | no core equivalent → disabled (G-5) | UI truthful notes | — |
| Sessions directory | `%ProgramData%\HeicFast\Recovery Wizard\sessions\` — defined ONCE in `contracts/README.md` + installer; bridge reads it from app config | bridge (`session_save/list/load`), NSIS creates + ACLs | filesystem |
| Logs directory | `%ProgramData%\HeicFast\Recovery Wizard\logs\` (Rust shell log; the ENGINE writes no implicit files [CORE §4.3]) | bridge logger | filesystem |
| Driver service name / Parameters (IoctlBase, MaxDisks, DebugLevel, SessionsEnabled, Sddl, FilterMode, VolSnapAttach) | installer + `driver/scripts/install.ps1` / `rda.inf` [DRIVER §4] — app never writes them | NSIS driver step | HKLM Services (installer-owned) |
| UI prefs (theme, density, locale, scale, sidebar, previewPaneWidth, viewMode) | unchanged — localStorage `rw.ui.*` keys [UI §4] | React app | per-user localStorage |
| Filters/sort | unchanged (Zustand filter-store, `resolveFilter()` [UI §4]) | UI | per-scan memory |
| BitLocker unlock aids | engine compiled 228-entry dictionary + optional user dictionary [CORE §7]; FVEK/dictionary = consent actions | bridge §3.2 #4 | none |

No config file exists on the engine side to reconcile (zero-init structs + CLI flags only
[CORE §4.3]); the UI has no server-side config; the only cross-tree constants are VERSION,
the two ProgramData paths, and the driver service name — all defined once in
`contracts/` + installer. (Reference parity note: the og's config/ INI/XML family
[RawConfig.ini, FileFilter.xml, config.ini …] is host-side policy the rebuilt core folds
into engine tables [CORE §4.3] — nothing to import.)

---

## 7. Installer project plan (NSIS)

### 7.1 Install layout (component → path → privilege → evidence)

| # | Component | Target path | Privilege | Evidence |
|---|---|---|---|---|
| 1 | App: `RecoveryWizard.exe` + WebView2 bootstrapper | `%ProgramFiles%\HeicFast\Recovery Wizard\` | per-machine write (admin at install only) | reference installs under `C:\Program Files\EaseUS` [LAYOUT §3]; Tauri NSIS perMachine [UI §8] |
| 2 | Engine kit: `cr_ui.dll`, `z.dll`, `lz4.dll`, `zstd.dll`, app-local VC143 CRT + UCRT + api-ms-win-crt-* | `<app>\engine\` | same | the complete host-binding kit [CORE §9]; Win7 RTM ships no UCRT — bundle carries it |
| 3 | Driver: `rda.sys`, `rda.inf`, `install.ps1`, `uninstall.ps1` | staged at `<app>\driver\`; COPIED at the driver step to `%SystemRoot%\System32\drivers\RDA.sys` (renamed to service name) + `sc.exe create RDA binPath= "<abs>" type= kernel start= demand error= normal group= "Boot File System"` + Parameters {IoctlBase=0, MaxDisks=200, DebugLevel=1, SessionsEnabled=1} + `sc.exe start` | **surfaced elevation, driver step only** [DECIDED] | [DRIVER §5.1, §13]; reference: eprdtdrv.sys runtime-copied to `System32\Drivers\%s.sys`, group "Boot File System" [LAYOUT §4, INSTALLER §4.2] |
| 4 | rdacli.exe (diagnostic smoke tool) | `<app>\driver\` | same | [DRIVER §10, §13 step 7] |
| 5 | Shared state: sessions, logs | `%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}\` | per-machine write, standard-user-writable ACL | deliberate deviation D-I2 (§7.6) |
| 6 | Registry: product key + uninstall key | `HKLM\Software\HeicFast\Recovery Wizard\{Version, InstallDir}`; `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Recovery Wizard` (NSIS standard format) | admin | NSIS native; deviation D-I4 from Inno `{AppId}_is1` [INSTALLER §3.1] |
| 7 | Shortcuts (Start Menu + Desktop optional) | standard shell folders | NSIS | reference shortcut creation not corpus-decoded [INSTALLER §10.11] — industry-standard choice |
| 8 | Per-user prefs | localStorage (none at install) | user | [UI §4] |

No AUTO_START service is installed (D-I5: the reference's DRWService "Read FVE Server"
[INSTALLER §4.1] has no counterpart — BitLocker unlock is in-process via cr_ui with consent).

### 7.2 NSIS script structure (`installer/recoverywizard.nsi`)

- Built on Tauri's NSIS bundler output with a custom template + hook page set
  (`tauri.conf.json` bundle > windows > nsis customizations; languages English/SimpChinese,
  perUser+perMachine retained [UI §8]).
- Pages: Welcome → License → InstallDir (per-machine default Program Files) →
  **Components**: [x] Application (required) / [ ] Raw-disk driver (recommended) →
  **Driver page (surfaced, elevated)**: explains why (raw sector reads; unsigned → test-sign
  note; app runs degraded without it), checkbox default ON, runs `install.ps1` via the
  elevated installer context (installer already admin in perMachine mode) →
  Install → Finish (launch option).
- Elevation timing: the INSTALLER elevates once (perMachine, standard NSIS
  RequestExecutionLevel admin); the driver step needs no second elevation inside the
  installer. **App runtime elevation — flagged decision conflict, see §7.4.**
- Per-user mode without driver: perUser install = app + engine only, driver step hidden;
  app then runs in degraded transport (direct handles) — truthful note on the page.
- Uninstall completeness: remove app dir, ProgramData subtree (with an optional
  "keep my scan sessions" prompt — default keep), registry keys, shortcuts; driver:
  `sc.exe stop RDA` → `sc.exe delete RDA` → delete `System32\drivers\RDA.sys`
  [DRIVER §5.2]; in-use file → NSIS retry/`Delete /REBOOTOK` (Inno's MoveFileEx analog
  [INSTALLER §5.1]).
- Update: NSIS in-place upgrade (same uninstall key; version from VERSION); driver updated
  via stop/delete/create (the `%s%d` hot-upgrade scheme [DRIVER §5.4, INSTALLER §4.2] is
  documented as an option, not v1); ProgramData sessions/logs preserved.
- Repair: reinstall over (NSIS standard); no maintenance mode (the reference has none
  documented either [INSTALLER §7]).

### 7.3 MSIX feasibility verdict — **NOT FEASIBLE for this product; dropped**

Truthful record: a Windows kernel driver **cannot** be installed by MSIX — MSIX packages run
AppContainer-isolated and cannot create SCM services, write `System32\drivers`, or register
kernel modules; kernel drivers require INF/Hardware-Dev-Center signing flows. Additionally
the app itself requires raw-disk handles (`\\.\RDA` / `\\.\PhysicalDriveN`) which an
AppContainer cannot open. MSIX would force a driver-less, volume-less shell — the product
has no meaningful MSIX shape. **Decision: MSIX dropped entirely** (recorded in
`docs/PARITY-DELTAS.md` + INTEGRATION-GAPS context; T12 produces the documented verdict, no
packaging project). The portable zip (§7.5) serves the "no-installer" need instead.

### 7.4 Driver signing posture + the elevation-model evidence conflict

- `rda.sys` ships **unsigned by design** (SignMode=Off; CI asserts cert-table RVA == 0)
  [DRIVER §7]. It will NOT load on stock Secure-Boot Win10/11. Test-sign path documented and
  proven in-repo: `bcdedit /set testsigning on`, or the benchmark workflow's no-reboot
  self-signed-cert flow (New-SelfSignedCertificate CN=RDA-BENCH-TEST → LocalMachine
  Root+TrustedPublisher → signtool /fd SHA256) [DRIVER §7]. The installer's driver page and
  README state this truthfully; the app functions without the driver (direct-transport
  fallback, observable via `cr_ui_device_transport` [CORE §10]).
- **Evidence conflict with the [DECIDED] elevation model** ("surfaced elevation only for
  driver step"): BOTH transport legs are administrator-only on stock Windows — the driver
  device DACL is `D:P(A;;GA;;;BA)(A;;GA;;;SY)` (admin/SYSTEM only; standard users cannot
  open it [DRIVER §2.7, §6]) and direct `\\.\PhysicalDriveN` raw reads require admin (the
  reference's no-admin fallback is volume-level `\\.\HarddiskVolumeN` only [INSTALLER §8]).
  The reference solves this with **requireAdministrator manifests on DRW.exe AND DRWUI.exe**
  — whole chain elevated at every launch [INSTALLER §8]. Options:
  (a) app manifest requireAdministrator (reference parity; honest; simplest);
  (b) app asInvoker + UAC relaunch-elevated prompt when raw access is needed (closer to the
  decision's spirit; new UX surface).
  **Recommendation (a) for v1 with the deviation documented (D-I3); the installer
  elevation stays scoped as decided.** — **RESOLVED BY INTEGRATOR SIGN-OFF: decision (a)
  ADOPTED.** The app manifest is `requireAdministrator` (reference parity, honest about the
  admin-only transport legs). The installer honors surfaced elevation via Tauri NSIS
  `installMode: "both"` (launches asInvoker; elevation only for per-machine/driver choices).
  Portable build ships the same manifest — truthfully documented ("run as administrator").

### 7.5 Portable build

`installer/portable.ps1` produces `RecoveryWizard-<version>-portable.zip`: app + engine kit +
driver staging folder (NOT installed). Portable mode: sessions/logs written beside the exe
(`.\sessions`, `.\logs`) — this matches the reference's module-relative model [LAYOUT §5-§8]
and is the ONE mode where install-dir-relative writes are kept (documented). Bridge selects
ProgramData vs local via an app-dir-writability probe at startup.

### 7.6 Deviation table vs reference (PARITY-DELTA candidates → `docs/PARITY-DELTAS.md`)

| ID | Reference behavior (evidence) | Our behavior | Rationale |
|---|---|---|---|
| D-I1 | Inno Setup 5.5.x installer + unins000 trio [INSTALLER §1] | NSIS (Tauri-native) | installer tech is not user-visible behavior; mandate names NSIS [DECIDED] |
| D-I2 | ALL writable data inside the install dir (log\, SaveScan\, DRWCfg.bin) — install-dir-writable-world [LAYOUT §5, §8] | Program Files (RO) + %ProgramData% sessions/logs + localStorage per-user | industry standard; deliberate [DECIDED] |
| D-I3 | requireAdministrator whole chain [INSTALLER §8] | installer elevation scoped to install+driver step; app manifest decision flagged §7.4 (recommend requireAdministrator for v1) | deliberate, better; evidence conflict recorded |
| D-I4 | Inno `{AppId}_is1` uninstall key [INSTALLER §3.1] | NSIS standard uninstall key format | mechanical consequence of D-I1 |
| D-I5 | DRWService "Read FVE Server" AUTO_START LocalSystem [INSTALLER §4.1] | no service; BitLocker in-process via cr_ui + consent | our engine has the capability in-process [CORE §2.1]; no key-broker needed |
| D-I6 | eprdtdrv installed on demand at first raw scan by setupdrvx64.exe [INSTALLER §4.2] | driver installed by a surfaced installer step (demand start, Boot File System group, System32\drivers\<SN>.sys — same SCM semantics) | closer to mandate's surfaced-elevation model; same wire contract |
| D-I7 | unins000 shared-file refcount prompts + RestartManager interplay [INSTALLER §5.1] | NSIS standard in-use handling (/REBOOTOK) | mechanical |
| D-I8 | CVersionCtrl/AutoUpdate/EuDownloader network update chain [INSTALLER §6] | no auto-updater (no plugins.updater configured [UI §8]); releases via GitHub + manual download | deliberate v1 scope cut; documented |
| D-I9 | Sessions `.rsf` in `<installdir>\SaveScan\` [LAYOUT §7] | `.crsf` (CRSFMT01) in ProgramData sessions\ | the format IS the product; auto-naming preserved (N-061 parity [CORE §4.1]) |
| D-I10 | OS gate via SetupUE 16-OS table + install telemetry [INSTALLER §1-2] | NSIS OsVersionCheck Win7 SP1+ floor; no install telemetry | privacy + simplicity; documented |
| D-I11 | Driver SDDL absent on reference (PID token only) [INSTALLER §4.2 vs DRIVER §2.7] | our driver adds admin/SYSTEM SDDL | deliberate divergence already kept in the driver [CORE §10] — carried, not re-litigated |

---

## 8. Build system plan

### 8.1 Top-level entry point — **`build.ps1`** (with a Linux-subset `build.sh`)

Justification: the full product is Windows-only (driver = WDK/msbuild [DRIVER §9]; installer
= NSIS; ui bundle = Tauri Windows; engine production target = MSVC [CORE §11]). PowerShell is
native to every step; a Makefile would shell out to msbuild/nuget/pnpm/cargo anyway. `build.sh`
runs only the Linux-subset (engine linux presets + contracts validation) for dev loops.
`build.ps1` modes: `-Target all|engine|driver|ui|installer|portable`, `-Configuration Release`.

### 8.2 Sub-build sequencing

```
VERSION (read once, validate against all consumers — FAIL on mismatch)
├── [parallel lane A] driver:  nuget restore packages.config → msbuild rda.sln -p:Configuration=Release -p:Platform=x64
│                                → stamp_osversion.py → verify_pe.py (+ unsigned assertion)   [DRIVER §9]
├── [parallel lane B] engine:  cmake --preset win-x64-release (vcpkg manifest: zlib/gtest/lz4/zstd)
│                                → cr_ui.dll + cr_ui.lib + rrecov.exe + vcpkg runtimes + CRT/UCRT
│                                → stamp_osversion_um.py + verify_pe_um.py gates               [CORE §1, §9]
└── [after B] ui:              pnpm install → pnpm build (Vite) → cargo build (tauri) linking cr_ui.lib;
                               stage engine kit beside the exe → pnpm tauri build → NSIS bundle
└── [after A+B+ui] installer:  makensis installer/recoverywizard.nsi (consumes Tauri bundle +
                               driver dist + VERSION) → portable.zip → SHA256SUMS
```

### 8.3 CI: what runs per platform (consolidated workflow set at root; paths-filtered)

| Workflow | Runner | Jobs (all inherited from the sheets' existing lanes) |
|---|---|---|
| `engine-ci.yml` (paths `engine/**`, `contracts/**`) | ubuntu + windows | lint; linux-gcc ASan; linux-tsan; linux-ubsan (926 tests); windows MSVC + PE stamp/import gates; fuzz; ui_abi_check.c C consumer [CORE §1, §8] |
| `driver-ci.yml` (paths `driver/**`) | ubuntu + windows-2022 | lint (clang-format + md); fuzz (libFuzzer + selftest + ASan/UBSan); unit (test_compat.c); build matrix Debug/Release (NuGet WDK → msbuild → stamp → PE gate → unsigned assertion) [DRIVER §9] |
| `ui-ci.yml` (paths `ui/**`, `contracts/**`) | windows-latest (Node 24, pnpm 12.15) | typecheck → lint → format → arch-lint (A1–A7) → ast-grep×3 → knip → jscpd → test:coverage (thresholds 70/60/70/70) → build → bundle:check → cargo check + clippy -D warnings; bridge contract tests (`tests/connector-tauri.test.ts`, windows-only job with real cr_ui.dll) [UI §1, §9] |
| `contracts-ci.yml` (paths `contracts/**`) | ubuntu | `contracts/validate.mjs`: error-code map covers the full cr enum; event map covers types 1–10; command manifest ↔ Rust `invoke_handler` ↔ TS invoke sites cross-check |
| `ui-e2e.yml` (paths `ui/**`) | windows-latest | Playwright 12 (mode=test alias double) + step screenshots + tauri debug build `--no-bundle`; `visual-compare.yml` (dispatch) with re-baselined screenshots [UI §9, §10] |
| `engine-bench.yml`, `driver-bench.yml` (dispatch/paths) | windows-2022 | existing api-benchmark + comparative-bench lanes [CORE §1]; driver benchmark incl. cross-driver byte-equality [DRIVER §9] |
| `release.yml` (tags `v*`) | windows matrix + ubuntu | full build.ps1 → NSIS + portable.zip + engine SDK bundle + driver dist → SHA256SUMS → GitHub release with truthful unsigned/no-signing notes + version-match gate tag==VERSION [UI §10, DRIVER §3] |

---

## 9. Test plan

**Survive unchanged**: engine's 926-test suite + ui_abi_check.c + 15 fuzz harnesses [CORE §1];
driver's fuzz/selftest/unit/verify_pe gates [DRIVER §9]; UI tests with no backend coupling —
i18n parity/sweep, theme persist/snapshot, ui-store/scan-session-store/filter-store,
logger, format, window-controls, route-fade, confidence-pill, toaster [UI §9].

**Re-authored for the real core** (per §5.4): the connector contract suite (against the real
bridge in a windows CI job), component tests (against `tests/fakes`), e2e smoke/responsive
(test-mode alias), visual-compare baselines.

**New bridge contract tests**: `tests/connector-tauri.test.ts` asserting the timing budgets
against the real engine fixture — cancel ≤300 ms, pause/resume <600 ms, filter <200 ms (the
core's own query contract is 1M nodes <200 ms [CORE §8] — same budget), preview <500 ms target;
monotonic overallProgress; cancellation leaves browsable results; report truncation at 500;
overlap refusal; consent default-deny. Plus Rust-side unit tests for the registries/consent
oneshot/event channel, and `contracts-ci` validation (§8.3).

**Integration validation measurements** (→ `docs/INTEGRATION-VALIDATION.md`, T14): a
mock-baseline vs real-bridge table using the mock's documented latency budget table [UI §6]
(listDevices 180±60, startScan 420±120, queryResults 90±50, preview 240±160, health 280±120,
dryRun 600±200 …) as the comparison baseline; plus engine-native numbers (cr_ui cheap-op
latencies 10.7–166.3 ns/op, query 31–36 ms in-suite [CORE §8]); memory bounds (snapshot
O(n) copy, disk-map ≤64 MiB, writer queues [CORE §8]); event-marshal latency (worker-thread →
emit → render). E2E where feasible (bridge e2e lane), labeled unit doubles where the real
engine is too heavy for jsdom (per §5.5).

---

## 10. Documentation plan

**Created (root)**: `README.md` (product: what it is, quickstart, layout map),
`docs/ARCHITECTURE.md` (three components + bridge + contracts + threading model + the
UM/kernel PE-tool near-twin note §2.3), `docs/BUILD.md` (build.ps1 lanes + toolchain
prereqs: WDK NuGet 10.0.26100.6584, vcpkg, Node 24/pnpm 12, Rust, NSIS),
`docs/RELEASE.md` (tag flow, artifacts, signing truth), `docs/TESTING.md` (suite map §9),
`docs/INTEGRATION-PLAN.md` (this document, approved), `docs/INTEGRATION-VALIDATION.md`,
`docs/INTEGRATION-GAPS.md` (G-1..G-11 + hygiene items), `docs/PARITY-DELTAS.md`
(imports the engine's 97-row register [CORE §12] + installer rows D-I1..D-I11),
`worklog.md` (unified continuation).

**Updated**: `ui/docs/CONNECTOR-CONTRACT.md` — mock sections ("Swapping in the real
bridge", mock semantics) rewritten to document the REAL bridge + the test double;
`ui/docs/IMPLEMENTATION-STATUS.md`, `ROADMAP.md` (engine-integration session closed),
`AUDIT-REPORT.md` (append integration-session verdicts); `ui/docs/ARCHITECTURE.md` +
`TESTING.md` cross-link the root docs. `engine/README.md`, `driver/README.md` get the
"part of final-prod" banner (§2.3). `contracts/README.md` (§6 constants: VERSION rule,
ProgramData paths, driver service name).

**Deleted/cleaned**: every stale reference — old repo URLs in sub-READMEs; `VITE_CONNECTOR`
in `ui/README.md` + `.env.example` docs; mock-backend doc sections; the dev-panel scenario
docs; per-tree `.github/workflows` (T4, §2.3). Gate at T16/T19: `rg -i "mock-connector|VITE_CONNECTOR|ScenarioControl"` over shipped paths (`ui/src`, `ui/e2e`, `installer`, root `docs`) returns zero; `rg "recovery-wizard-ui-new|core_recovery|rda-driver"` URL references updated to sub-tree paths.

---

## 11. Risk register (top risks + mitigations)

| # | Risk | Evidence | Mitigation |
|---|---|---|---|
| R1 | **Bridge threading deadlock**: event sinks must not block; consent callback blocks an engine thread; run/process on dedicated threads | [CORE §6] | channel-only sinks (try_send), consent oneshot with no registry lock held (§4.5), dedicated run threads, T8 timing + stress tests; the core's own progress bridge serializes under `progress_mu` [CORE §6] — the Rust bridge adds no locks on engine threads |
| R2 | **Win7 SP1 floor vs Tauri 2/WebView2**: engine+driver are Win7SP1-hardened (PE 6.1 stamps [CORE §1, DRIVER §11]) but WebView2 Evergreen dropped Win7; Rust/Tauri min-OS is unverified for our stack | [CORE §1], [DRIVER §11] | **OPEN-QUESTION OQ-6**: verify Tauri 2 + WebView2 Fixed-Version on Win7SP1 at T14; if unsupported, ship UI floor = Win10 while engine/driver keep the Win7 contract (documented in PARITY-DELTAS); portable engine/CLI (rrecov) remains Win7-true |
| R3 | **Unsigned driver on modern Windows → degraded product**: rda.sys cannot load on stock Secure-Boot Win10/11; direct fallback ALSO needs admin (SDDL + raw handles) | [DRIVER §7], §7.4 | truthful driver page + `getDriverStatus` note + test-sign path documented; elevation-model decision resolved at gate (§7.4); OSSign/Hardware-Dev-Center track carried from driver docs [DRIVER §7] |
| R4 | **CI toolchain availability**: WDK NuGet + vcpkg + pnpm + Rust + NSIS in one release runner; MSVC /tmp quirk history | [DRIVER §9], [CORE §1] | keep per-component workflows paths-filtered (§8.3) so a driver break doesn't red the UI; release workflow assembled from proven lanes (all four repos have green pipelines today); NSIS comes with the Tauri bundler (`targets: ["nsis"]` already proven [UI §8]) |
| R5 | **History-merge pitfalls**: filter-repo rewrites + merge order + lost files | session practice | §2.4 verification gate (counts, --follow, byte-identity) executed at T4 before any deletion commit; deletions (workflows) in a separate commit after verification |
| R6 | **Semantics regressions vs mock**: monotonic overallProgress, exact terminal counts, report truncation, timing budgets | [UI §6] | bridge clamps monotonic; terminal counts from real stats (exact by construction); contract tests assert all budgets against the real engine (§9) |
| R7 | **Mid-scan live-query cost**: snapshot O(n) copy ~150–300 ms @1M nodes; Rust-side filtering | [CORE §6, §8] | bounded snapshot cadence + snapshot cache + query over the cached copy; measured in T14; G-7 records the ABI ask |
| R8 | **Service-name mismatch**: core probes `\\.\RDA` vs driver's service-name-derived device | [CORE §10], [DRIVER §2.1] | OQ-5: pin `cr_transport.h` constant at T7; installer pins ServiceName=RDA until then; WinPE profile already uses `rda` [DRIVER §4.4] |

---

## 12. Sequencing — T4→T20 mapped to plan sections, each with its push gate

| Task | Plan section | Deliverable | Push gate (must be green to push) |
|---|---|---|---|
| T3 | all | this plan committed as `docs/INTEGRATION-PLAN.md` | integrator approval of this draft (resolve OQ-1..OQ-6, esp. §7.4) |
| T4 | §2 | merges + skeleton (root files, sub-tree workflow deletion commit) | §2.4 checklist: file counts 769/385/66, `log --follow` ×4, byte-identity ×3, ancestor checks ×3 |
| T5 | §2.1 contracts/, §3 tables | `contracts/` (error-codes, event-mapping, command-surface, type-maps, validate.mjs) | `contracts/validate.mjs` green; every cr enum value + event type mapped; zero holes |
| T6 | §8 | `build.ps1` + `build.sh` | one command builds driver + engine + ui app (stub connector acceptable) on a Windows runner; VERSION mismatch gate active |
| T7 | §3 (row by row), §4 | bridge + TauriConnector per operation | per-operation push: that operation's contract test green; `pnpm arch:lint` + tsc + cargo clippy green |
| T8 | §4, §3 threading column | threading/cancellation/lifecycle adaptation | timing budgets on real engine fixture: cancel ≤300 ms, pause/resume <600 ms, filter <200 ms; no deadlock under pause/cancel/dismount stress |
| T9 | §6 | config reconciliation | single-source audit: VERSION gate + paths/service-name defined once; no divergent constants (rg gates) |
| T10 | §5 | mock removal | zero references in shipped paths (§10 rg gate); tests re-authored green; workflows carry no `VITE_CONNECTOR`; knip/jscpd/i18n-sweep 0 findings |
| T11 | §7 | NSIS project | installer builds; §7.1 layout table verified against built script; per-user mode works without driver |
| T12 | §7.3 | MSIX verdict document | truthful kernel-driver impossibility recorded; MSIX dropped (no packaging project) |
| T13 | §8.3 release.yml | tag-triggered release workflow | dry-run on a test tag produces NSIS + portable.zip + SHA256SUMS + truthful notes |
| T14 | §9 | full validation run | INTEGRATION-VALIDATION.md filled with measured evidence (budgets table + mock-baseline comparison) |
| T15 | §7.2 | clean-runner install/uninstall/update/repair evidence | uninstall removes everything (§7.2 list); update preserves ProgramData sessions; repair = reinstall; evidence committed |
| T16 | §10 | docs sweep | all root docs created; stale-reference rg gates zero; CONNECTOR-CONTRACT mock sections rewritten |
| T17 | §8.3 | full CI sweep | every root workflow green for the correct reason (paths filters spot-checked, logs reviewed) |
| T18 | §8.3, §13-in-release | RC tag `v1.0.0-rc1` | artifacts install + run on a clean Windows runner; T13 flow green end-to-end |
| T19 | §10, §11 | final sweep | worklog complete; G-1..G-11 + D-I1..D-I11 documented with rationale; no TODO in shipped code; `main` green |
| T20 | §12 | handoff | worklog entry with exact commit; INTEGRATION-GAPS next actions (ABI appends: repair surface, engine version, snapshot query, folder scope); final clean-machine verification |

**OPEN-QUESTIONS register (to resolve at gate review / early tasks)**: OQ-1 recovery
progress-callback struct fields (T7, from `cr_ui.h`); OQ-2 category_mask bit assignments
(T5, from `cr_ui.h`); OQ-3 thumbnail decode cost policy (T7); OQ-4 listSessions load-vs-stat
caching (T7); OQ-5 core RDA symlink constant / service name pin (T7, from `cr_transport.h`);
OQ-6 Tauri/WebView2 on Win7SP1 (T14); plus §7.4 elevation-model sign-off.

— end of draft —

---

## COMPLETION RECORD — this plan is EXECUTED

Marked complete at commit **6fb8f658 (the `v0.5.0` release cut, 2026-10-27T00:09Z;
release run 4 GREEN — the artifacts are live on the release)** by the
integration sessions recorded in `worklog.md`. State of the §12 task table:

- T3–T6: done (the gate plan itself, the three history-preserving merges,
  `contracts/` with validate.mjs phase 1+2 green on every push since
  `a790cf9b`, `build.ps1`/`build.sh` with the VERSION gate).
- T7–T10: done — the Rust bridge (34 commands) + TS connector wired to the
  REAL core; the simulated backend removed with zero references in shipped
  paths (the T19 sweep above re-verified); the test double is test-scope
  only (arch-lint A7, bundle assertions both ways).
- T11–T12: done — `installer/recoverywizard.nsi` (the combined product
  installer; compile-validated + runtime-validated) + `installer/portable.ps1`;
  MSIX truthfully assessed impossible (docs/MSIX-ASSESSMENT.md) and dropped.
- T13: done — `release.yml` tag-triggered, proven end to end by release run
  40418458268 (tag `v0.5.0-rc3`, GREEN, artifacts + SHA256SUMS + truthful
  unsigned notes live on the release).
- T14: done — `docs/INTEGRATION-VALIDATION/t14-measurements/` (three
  consecutive green runs; every budget row measured or honestly recorded
  as a virtual-runner limit).
- T15: done — `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/` (runs 16 AND
  17 fully green back to back: install / update / repair / uninstall on a
  clean Windows runner, the installer's own lifecycle facts file committed
  as evidence).
- T16–T17: done — the root docs set reflects the shipped reality; every CI
  lane green for the correct reason (contracts, ui, ui-e2e, driver, engine,
  release, validation-evidence).
- T18: done — the RC discipline's three gates were satisfied in order
  (rc1/rc2 failures root-caused and fixed; rc3's release run GREEN with
  every artifact confirmed; the clean-runner install/run/uninstall
  evidence committed as `t15-runtime-matrix/`), and the FINAL `v0.5.0`
  tag was cut at `6fb8f658` on the validated tree per the session's
  release instruction — release run 4 GREEN. The stricter
  operator-acceptance-on-real-hardware step this lane originally
  preferred is preserved as the FIRST recorded follow-up (test v0.5.0 on
  a real machine; any finding rides the standard patch flow to v0.5.1).
- T19: done — the sweep above (zero TODO/FIXME in shipped paths, zero mock
  references; G-1..G-11 + D-I1..I11/D-B1..B5 documented with rationale).
- T20: this completion record + the worklog handoff entry.

The plan text above stays frozen as the historical gate document; the
worklog and docs/INTEGRATION-VALIDATION.md carry the shipped truth.

---

## Post-completion amendment — 2026-10-27 (recorded, plan text above stays frozen)

The 2026-10-27 operator directive superseded two postures this plan
recorded, after the v0.5.0 completion above:

1. **The driver is MANDATORY** — this plan's transport sections described
   the corpus-observed EPRDTDRV-first / direct-fallback duality to
   preserve. The directive removed every fallback (`PARITY-DELTAS.md`
   D-FALL); `ERR_DRIVER_REQUIRED` (0x50020007) is the driver-absent
   truth. §7.4's per-user install mode is retired with it (a per-user
   install cannot install the required driver).
2. **Signing posture** — this plan recorded "unsigned by default,
   signing disabled." The directive replaced it with DEVELOPMENT
   TEST-SIGNING (docs/DEV-SIGNING.md): the ephemeral build certificate
   embed-signs rda.sys + the engine kit + the installer; the public
   certificate ships as a release asset; production signing (attestation)
   remains pending.

The shipped truth lives in `docs/INTEGRATION-VALIDATION.md` (run
40442056802 evidence) and `worklog.md`. The v0.6.0-rc1 release is the
first cut under these postures.
