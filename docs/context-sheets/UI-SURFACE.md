# UI-SURFACE — recovery-wizard-ui-new exhaustive context sheet

> Produced by Agent B (task 2-b) from a line-by-line read of
> `/home/z/my-project/work/recovery-wizard-ui-new` pinned at commit
> `f4a8f8aebab625e9048d833cd9c3501741d7c436` (branch `v3`, tag state v0.3.3).
> READ-ONLY analysis; nothing in the repo was modified.
> Purpose: single source of truth for ripping out the simulated backend and
> wiring a real C++ core behind the existing seam. Every claim cites a file
> path. Paths are relative to the repo root unless absolute.

---

## 1. Identity

| Fact | Value | Source |
| --- | --- | --- |
| Product name | "Recovery Wizard" (Tauri productName) / npm package `recovery-wizard-ui` | `src-tauri/tauri.conf.json`, `package.json` |
| App display name in title bar / i18n | `app.name` = **"EaseUS Data Recovery Wizard Free"** (en) — deliberately reference-branded copy | `src/i18n/en.ts:6` |
| Version | **0.3.3**, kept in lockstep across `package.json` (v0.3.3), `src-tauri/tauri.conf.json` (v0.3.3), `src-tauri/Cargo.toml` (v0.3.3) | those files |
| Framework | **React 19.3.1** + **Tauri 2** (tauri crate v2, `@tauri-apps/api` 2.12.4, CLI 2.12.4) + TypeScript 5.9.3 (strict) + **Vite 8.3.5** | `package.json`, `src-tauri/Cargo.toml` |
| Router | React Router **7.18.4**, data mode, `createHashRouter` (ADR-001: hash routing works identically under WebView2 `tauri://localhost` and browser dev/preview) | `src/app/router.tsx`, `docs/adr/001-router-choice.md` |
| State | TanStack Query 5.104.5 (server state) + Zustand 5.0.16 (UI state) | `package.json` |
| Styling | Tailwind CSS **4.3.3** (v4 `@theme inline` architecture) + tw-animate-css; two-layer adopted theme (ADR-007) | `src/styles/index.css` |
| UI kit | shadcn/ui-v4-style primitives restyled in `src/components/ui/` (Radix: checkbox, context-menu, dialog, dropdown-menu, label, popover, progress, radio-group, slot, switch, tooltip) + Lucide icons 1.61.0 + sonner 2.0.8 | `package.json`, `src/components/ui/*` |
| Compiler | React Compiler enabled via `react({ compiler: true })` (oxc-transform-react Rust port, ADR-002) | `vite.config.ts:13`, `docs/adr/002-react-compiler.md` |
| i18n | i18next 26.5.6 + react-i18next 17.1.4; locales `en` + `zh-CN` only (`SUPPORTED_LOCALES = ['en', 'zh-CN']`); key surface 1:1, governed by `scripts/i18n-sweep.mjs` + `tests/i18n-sweep.test.ts` (missing=0, unused=0) | `src/i18n/index.ts`, `docs/TESTING.md` |
| Node / pnpm | Node >= 24, pnpm 12.15.0 (build approvals in `pnpm-workspace.yaml`: @ast-grep/cli, unrs-resolver, esbuild, @tauri-apps/cli) | `package.json`, `pnpm-workspace.yaml` |
| Dev commands | `pnpm dev` (Vite dev server, port 1420 or 3000 w/ `DISABLE_HMR=true`; `TAURI_DEV_HOST` for `tauri dev` HMR on 1421), `pnpm build`, `pnpm preview`, `pnpm test` (145 tests / 22 files), `pnpm test:coverage`, `pnpm e2e` (12 Playwright tests against `vite preview` :4173), `pnpm tauri dev`, `pnpm tauri build`, `pnpm icons`, `pnpm arch:lint`, `pnpm knip`, `pnpm jscpd`, `pnpm bundle:check`, `pnpm visual:compare` | `package.json` scripts, `playwright.config.ts` |
| Quality gates at HEAD | tsc strict clean; ESLint 0 errors (`eslint.config.js`, recommendedTypeChecked + react-hooks v7 compiler rules); prettier; arch-lint A1–A6; ast-grep 2 rules; knip 0 findings; jscpd ~0.03%; 145/145 vitest; coverage thresholds 70/60/70/70; bundle budget main 124.6 KB / 200 KB gzip, total 309.2 KB / 500 KB; 12/12 e2e; live axe 0 violations | `README.md`, `docs/TESTING.md` |

The repository is a **clean-room re-implementation** of the EaseUS Data Recovery
Wizard visual/interaction language, driven entirely by a **Mock Connector**
simulating the recovery engine; the real IPC adapter is a deliberate loud stub
(`src/connector/tauri/tauri-connector.ts`).

---

## 2. Complete feature inventory

Routes (`src/app/router.tsx`, all under `WizardShell` + `ErrorBoundary`):

| Route | Component | What it does |
| --- | --- | --- |
| `/` | `HomePage` (`src/features/device-selection/home-page.tsx`) | Device selection. Internal-drives grid (radiogroup `DeviceCard`s: 3D `Drive3dIcon` w/ BitLocker lock state, capacity bar, health chip, lost-partition badge, per-volume scan chips), "Specify a location" quick targets (**Recycle Bin** → `startScan({targetId:'vol-recycle-bin'})`, **Desktop** → `'vol-desktop'`, **Select Folder** → volume picker mini-dialog → `pickDirectory({volumeId})` then `startScan({targetId, pathScope})`), recent-sessions resume list (`loadSession`), permission-denied / disconnected banners (device OR volume level), refresh devices, bottom "Search for Lost Data" CTA. `?media` shows removable group. **No scan-configuration step** (ADR-008): every entry starts the scan directly and navigates to `/scan/:targetId/live?scanId=…`. |
| `/sd-card` | `SdCardPage` | SD card landing: SD/MMC/MS PRO row, "Show all external devices" expansion (each with Scan button), SDR/SSR/DVR feature cards, "Search for Lost Data" CTA → direct scan of `kind === 'sd' \|\| 'usb'` device. |
| `/tools` | legacy alias → `HealthPage` | (route kept for back-compat) |
| `/health` | `HealthPage` (`src/features/health/health-page.tsx`) | Disk Health: physical-disk selector cards, hero card w/ circular health-score gauge derived from `DiskHealth.status`+`wearPercent`, KPI tiles (temperature, power-on hours, SSD/HDD class, TRIM/wear), **S.M.A.R.T. table** (ID/Attribute/Current/Worst/Threshold/Raw/Status), scan-this-drive CTA. |
| `/backup` | `BackupPage` | "Scan Snapshot": marketing-ish feature points + **saved recovery checkpoints** = `sessions.listSessions()` rows with Open → `loadSession(fileName)` → `/results/:scanId`. |
| `/scan/:deviceId/live` | `ScanProgressPage` (`src/features/scan-progress/scan-progress-page.tsx`) | Live scan INSIDE the 3-pane results surface (`<ResultsExplorer liveMode>`). Bottom cluster: `ProgressGauge` (overall %), status line "Quick/Advanced Scanning X% , Files Found: N (size)" (`scan.quickScanning`/`scan.advancedScanning`/`scan.filesFoundWithSize`), unreadable-areas badge (`errors` count + `badSectorMap` tooltip), line 2: pause/resume/stop buttons + "Remaining time" + "Reading sector: X/Y" (sectors = bytes/512), full-width "Searching: \<path\>" ticker (`currentPath`, deep-walk/carve only), **Recover All CTA** (disabled until `recoverableDuringScan`), stop-confirm dialog (stats tiles, Stop/Continue), device-lost non-dismissable reconnect modal (`deviceConnected === false && status === 'paused'` → resume = reconnect), completion **celebration modal** (stats grid, "Go to Recover", Esc blocked). |
| `/results/:scanId` | `ResultsRoute` + `ResultsExplorer` | Guarded results surface: `getScanState(scanId)` gate, partial banner for `status === 'cancelled'` w/ "Continue searching" re-scan preserving `pathScope`. Explorer: toolbar (Home/Back/Up cluster, breadcrumb heading "Search results for …" or tree node label + item count, details⇄icons view toggle, Filter button, search input), FilterBar, 3-pane layout (ResultsTree | VirtualFileList | PreviewPane), 6-item context menu (Recover, Preview, Open location (disabled in mock w/ truthful tooltip), Select not-yet-recovered, Smart select best copy, Clear selection), tri-state select-all over the FULL filtered count, smart-select (best-per-name by origin+confidence), bottom status bar "Scan Completed, Files Found: N (size)" + Recover-N/Recover-All CTA + Save-this-search (`saveSession`). |
| `/recover/:scanId` | `RecoveryPage` (`src/features/recovery-queue/recovery-page.tsx`) | "Choose a destination path…" dialog card: Local/Cloud tabs (Cloud = truthful unavailable), drive radio rows from `listMountPoints` (free/total GB, overlap warning, low-space warning), Choose-folder → `pickDirectory({excludeDeviceId: sourceTargetId})`, collapsible destination path input + collision policy radio (rename/overwrite/skip), **live `validateDestination`** on selection/blur with hard refusal (overlap → red alert + hint; out-of-space → Insufficient Disk Space modal gate with Continue/Reselect), "Enable file repair" switch (`RecoveryOptions.fileRepair`), Save/Recover button → `startRecovery`. Then live queue: overall progress bar, current file, counts, verifying indicator, ETA, item list with per-state badges, cancel-with-confirm, and final report card (success/partial/failed, processedTotal exact, truncated-lists notice, failed-reasons list, **JSON report export** via Blob download `recovery-report-<jobId>.json`). |
| `/repair` | `RepairPage` | File Repair 2-step direct flow: dropzone → queue (static sample files `SAMPLE_REPAIR_FILES`; "Repair All" is client-side demo). **Advanced Repair toggle** reveals `VolumeRepairWorkbench` — the real connector-driven RawFixer surface: device chips, `dryRunRepair` findings list (severity badges), typed **REPAIR consent** (checkbox + type-REPAIR input → `applyRepair(deviceId, {consentToken:'REPAIR'})`), undo-log list + `undoRepairs` rollback. |
| `/lost-partition` | `LostPartitionPage` | 2-step flow: select-disk (static `RESCUE_DISK_SPECS` Disk 2/0/1 cards with partition segment bars + BitLocker locks; "Search for Lost Partition" → `startScan` on a real device id) → scan-completed (found-partitions table, "Try Data Recovery" → real scan). |
| `/settings` | `SettingsPage` | Appearance (7-mode theme picker: system/light/dark/ember/tide/blossom/high-contrast; UI Scale 100/150/200% via `document.documentElement.style.zoom`; density comfortable/compact; language en/zh-CN), Diagnostics (log ring buffer + copy), About table (version from `getAppInfo`, connector-mode badge, engine status from `getDriverStatus`, licenses). |
| `*` | `ResultsPageFallback` | Empty-state fallback for scan/results routes without a session. |

Title-bar chrome (`src/app/shell.tsx`): briefcase emblem + app name, orange
**Buy Now pill** + key icon (both open `ActivationDialog` — license entry UI,
activation is truthful "provider unavailable" toast; no backend), standalone
language dropdown, three-lines Menu (Settings / Theme quick toggle / About),
`WindowControls` (Tauri-only). Sidebar (hidden in explorer flow): 3 sections —
Data Recovery (Hardware Disk, SD Card, Disk Health), Advanced Features (File
Repair, Lost Partition Rescue), File Backup (Scan Snapshot) + collapse toggle;
in results flow shows "Choose another device" + active-scan pulse link.

Deliberately excluded surfaces (documented divergence, `docs/UI-MAP.md` §0):
retention/license gates (except the harmless Buy-Now chrome), AI assistant,
Cloud/NAS/WeChat/iCloud targets (truthful unavailable), auto-recover, the
reference's separate backup product.

---

## 3. THE BACKEND SEAM (most critical)

### 3.1 The interface

**`RecoveryConnector`** — defined in **`src/connector/contract.ts:631-641`**:

```ts
export interface RecoveryConnector {
  readonly devices: DevicesDomain;
  readonly scan: ScanDomain;
  readonly results: ResultsDomain;
  readonly preview: PreviewDomain;
  readonly sessions: SessionsDomain;
  readonly recovery: RecoveryDomain;
  readonly health: HealthDomain;
  readonly repair: RepairDomain;
  readonly system: SystemDomain;
}
```

Nine domains. Every domain interface, every type, and the free function
`scanPhaseOf(stage)` live in the same file `src/connector/contract.ts` (663
lines) — the single source of truth ("The UI talks ONLY to this interface").
The human-readable companion is `docs/CONNECTOR-CONTRACT.md`.

There is also a mock-only control surface (NOT part of RecoveryConnector):
`ScenarioControl { setScenario, getScenario, setSeed, reset }` over
`MockScenarioId` (`contract.ts:644-662`).

### 3.2 Complete method list (36 RecoveryConnector methods)

Exact signatures from `src/connector/contract.ts`:

**DevicesDomain** (lines 128-137):
- `listDevices(): Promise<Device[]>`
- `getDeviceInfo(deviceId: string): Promise<Device>` — id may be a device OR volume id; connector resolves volume → owning device
- `refreshDevices(): Promise<Device[]>`

**ScanDomain** (lines 215-223):
- `startScan(params: ScanParams): Promise<{ scanId: string }>` where `ScanParams = { readonly targetId: string; readonly pathScope?: string | null }`
- `pauseScan(scanId: string): Promise<void>`
- `resumeScan(scanId: string): Promise<void>`
- `cancelScan(scanId: string): Promise<void>`
- `getScanState(scanId: string): Promise<ScanState>`
- `onScanProgress(scanId: string, cb: (event: ScanProgressEvent) => void): () => void` — subscription; returns unsubscribe

**ResultsDomain** (lines 308-322):
- `getResultTree(scanId: string, parentId: string | null, mode: ResultTreeMode): Promise<TreePage>`
- `queryResults(scanId: string, filter: ResultsFilter, sort: { field: SortField; direction: SortDirection }, page: { offset: number; limit: number }): Promise<ResultsPage>`
- `getNodeDetail(scanId: string, nodeId: string): Promise<NodeDetail>`
- `countResults(scanId: string, filter: ResultsFilter): Promise<number>`

**PreviewDomain** (lines 371-377):
- `requestPreview(scanId: string, nodeId: string): Promise<PreviewPayload>` — the CONNECTOR decides the preview kind (ADR-008); no `kind` argument
- `getThumbnail(scanId: string, nodeId: string): Promise<{ dataUrl: string } | null>`

**SessionsDomain** (lines 393-398):
- `saveSession(scanId: string, path: string | null): Promise<SessionMeta>`
- `listSessions(): Promise<SessionMeta[]>`
- `loadSession(path: string): Promise<{ scanId: string }>`
- `getSessionMeta(path: string): Promise<SessionMeta>`

**RecoveryDomain** (lines 474-489):
- `startRecovery(scanId: string, selection: RecoverySelection, destination: string, options: RecoveryOptions): Promise<{ jobId: string }>`
- `cancelRecovery(jobId: string): Promise<void>`
- `getRecoveryState(jobId: string): Promise<RecoveryJob>`
- `getRecoveryItems(jobId: string, page: { offset: number; limit: number }): Promise<readonly RecoveryItem[]>`
- `onRecoveryProgress(jobId: string, cb: (event: RecoveryProgressEvent) => void): () => void`
- `getRecoveryReport(jobId: string): Promise<RecoveryReport>`

**HealthDomain** (lines 520-522):
- `getHealth(deviceId: string): Promise<DiskHealth>`

**RepairDomain** (lines 559-567):
- `dryRunRepair(deviceId: string): Promise<RepairDryRun>`
- `applyRepair(deviceId: string, options: { consentToken: string }): Promise<{ applied: number }>`
- `getUndoLog(deviceId: string): Promise<readonly RepairUndoEntry[]>`
- `undoRepairs(deviceId: string): Promise<{ rolledBack: number }>`

**SystemDomain** (lines 604-625):
- `getDriverStatus(): Promise<DriverStatus>`
- `getAppInfo(): Promise<AppInfo>`
- `validateDestination(path: string, sourceDeviceId: string, neededBytes: number): Promise<DestinationValidation>`
- `pickDirectory(hint?: { readonly volumeId?: string | null; readonly excludeDeviceId?: string | null }): Promise<string | null>`
- `listMountPoints(): Promise<readonly MountPoint[]>`
- `openPath(path: string): Promise<void>`

### 3.3 How the factory selects the backend — GATING

`src/connector/index.ts` (51 lines) is **the only place** that decides the
backend:

- **Env var `VITE_CONNECTOR`** (build-time, read via `import.meta.env.VITE_CONNECTOR`).
  `connectorMode()` returns `'tauri'` only when the value is exactly `'tauri'`; **anything else (including unset) → `'mock'`**. Mock is the DEFAULT.
- **Static import**, not dynamic: `import { MockConnector } from './mock/mock-connector'; import { createTauriConnector } from './tauri/tauri-connector';` — both implementations are always in the bundle; the branch is a build-time constant fold (`if (connectorMode() === 'tauri')`).
- The singleton is created once at module load: `const backend = createBackend()` → `export const connector: RecoveryConnector = backend.connector`.
- `export function scenarioControl(): ScenarioControl | null` returns the concrete `MockConnector`'s control surface when in mock mode, `null` in tauri mode (the dev panel hides scenario controls accordingly).
- `AppInfo.connectorMode: 'mock' | 'tauri'` is surfaced to the UI via `getAppInfo()`; Settings and About display it; the results context-menu uses it to disable "Open location" in mock.

CI/release gating: `.github/workflows/ui-smoke.yml`, `visual-compare.yml`,
and `release.yml` all explicitly set `env: VITE_CONNECTOR: mock` when building.
`.env.example` contains only AI-Studio leftovers (GEMINI_API_KEY/APP_URL) —
**the connector env var is documented in README.md, not .env.example**.

### 3.4 The simulated backend (Mock Connector) — exact files

The mock backend consists of exactly 5 files under `src/connector/`:

| File | Lines | Role |
| --- | --- | --- |
| `src/connector/contract.ts` | 663 | The typed contract (shared by both backends and importable type-only everywhere) |
| `src/connector/index.ts` | 51 | Factory + env gating + `scenarioControl()` |
| `src/connector/mock/mock-connector.ts` | 1603 | **`class MockConnector`** — the simulator: all 9 domains + scenario control, in-memory worlds, timers |
| `src/connector/mock/fixtures.ts` | 569 | Per-scenario device/volume fixtures + name/extension word pools |
| `src/connector/mock/node-space.ts` | 945 | **`class NodeSpace`** — procedural deterministic result index (ADR-004) |
| `src/connector/mock/rng.ts` | 28 | `mulberry32` seeded PRNG + `randInt`, `pick`, `randBell` |

Real backend (stub): `src/connector/tauri/tauri-connector.ts` (36 lines,
`createTauriConnector()` returns a Proxy per domain where every method throws)
+ `src/connector/tauri/not-implemented.ts` (`connNotImplemented` error with
`code = 'not-implemented'` and a message pointing at CONNECTOR-CONTRACT.md).

**There are ZERO `invoke()` calls anywhere in `src/`** (verified by grep; also
enforced by rule `rules/no-raw-invoke.yml` + arch-lint A2 + CI ast-grep step).
The Tauri shell's three Rust commands (`reveal_in_explorer`, `path_exists`,
`app_environment` in `src-tauri/src/commands/system.rs`) are registered in
`src-tauri/src/lib.rs` `invoke_handler` but never called from the frontend —
they are staging for the future adapter.

#### How fake data is generated

**Fixtures** (`fixtures.ts`): `fixturesFor(scenario)` returns per-scenario
`DeviceFixture[]` (device minus volumes + volumes + `nodeSeed` + `fileCount` +
`directoryCount`), always appended with two **virtual location targets**
(`LOCATION_FIXTURES`): `dev-recycle-bin`/`vol-recycle-bin` (900 files, seed 5501)
and `dev-desktop`/`vol-desktop` (420 files, seed 6602), kind `'virtual'`,
`canScanWholeDisk: false`, excluded from health/repair pickers via
`usePhysicalDevices` (`d.kind !== 'virtual'`).

Scenarios (`MockScenarioId`, `contract.ts:651-662`) → device worlds:
- `standard`: `dev-sys` Samsung 970 EVO 500GB "Windows (C:)" NTFS 149.45 GB free 31.16 GB (180k files / 1.6k dirs, seed 6607; volume `bitlocker: 'unlocked'`), `dev-data` WD Blue 2TB "Temporary Storage (D:)" 150 GB / 146.12 GB free NTFS **`bitlocker: 'locked'`** (340k files / 2.4k dirs, seed 7708), `dev-usb` SanDisk Ultra 64GB exFAT F: (21k files, seed 8809)
- `large-ntfs`: `dev-large` 2 TB NTFS "Data Disk (D:)" with **1,200,000 files / 5,400 dirs (seed 4405)** + sys volume (210k files)
- `sd-card`: `dev-sdcard` 64 GB exFAT E: (24k files, seed 1102)
- `btrfs-disk`: `dev-btrfs` 4 TB Btrfs (480k files, seed 2203)
- `lost-partition`: `dev-lost` 1 TB Seagate with `vol-lost-main` carrying `lostPartition: { id:'lost-part-1', startByte: 660 GB, sizeBytes: 330 GB, probableFs:'ntfs' }` (96k files, seed 3304)
- behavior-injecting scenarios (`permission-denied`, `disconnect-mid-scan`, `bad-sectors`, `failing-disk`, `out-of-space`, `corrupt-session`) reuse the standard world and flip behavior in the connector (see below).

Shared pools: `CATEGORY_BY_EXT` (8 categories: graphics/video/audio/document/
email/archive/bookmarks/other with explicit extension lists), `EXT_WEIGHTS`
(weighted extension pool — jpg 18, png 12, pdf 8 …), `NAME_STEMS` (57 stems:
report, notes, photo…), `DIR_NAMES` (48 dir names: Documents, Downloads…).

**NodeSpace** (`node-space.ts`, ADR-004): constructed per volume as
`new NodeSpace(volumeId, fixtureSeed ^ connectorSeed, directoryCount, fileCount,
capacity.totalBytes * 0.985)` (found-data budget scaling — corpus: 147 GB found
on a 149 GB disk). Structure:
- A materialized directory skeleton: root `root` + `d1…dN` random-walk tree (climb depth 0-3, names from `DIR_NAMES`), with two-pass precomputed subtree aggregates (direct sums, then bottom-up propagation).
- Typed-array file records (NOT objects): `fDir: Uint32Array`, `fCat: Uint8Array`, `fSize: Float64Array`, `fDay: Uint16Array` (days since 2019-01-01), `fConf: Uint8Array`, `fOrigin: Uint8Array`, `fExt: Uint8Array`. Names derived on demand in `fileName(i)` by re-seeding mulberry32 from `seed ^ (i+1)*2654435761` — no storage.
- Origin mix (corpus-weighted): 62% `existing`, 26% `deleted`, 12% `lost-path`; the last 14% of indices (`i >= 86%`) are `raw` with reduced confidence (poor/fair). **Zero-byte rule**: every 97th file is 0 bytes and forced `confidence: 'poor'` (per `CRecoverability::GetEntryRecoverability`).
- Category from extension, size from per-category bell curves (graphics ~3.2 MB, video ~840 MB, audio ~9 MB, other ~240 KB), aggregate scaled to the volume budget.
- Tree semantics: path-mode top level = 3 disjoint branches `root` ("Existing files" = existing+deleted w/ paths), `lost-paths` (special `'lost-path'`), `raw-all` (special `'raw-found'`); `lost-*`/`recon-*` prefixed subtree ids render the lost/raw hierarchies at 20%/30% scaling; type-mode = 8 category nodes `c0..c7` + `ext:<cat>:<ext>` leaves; raw-mode = single `raw-all` group.
- Folder scope: `resolveScope(path)` builds a `Uint8Array` dir mask (BFS from the scope root); scoped queries filter by mask; the scoped path-tree presents the scanned folder itself as root; `suggestedScopePath()` for picker hints.
- Filtering (`matches()`): scope mask, categories, origins, `hideEmptyFiles`, size range, date range, extensions, path/type/raw tree-node scoping (incl. `lost-paths`/`raw-all`/`lost-root`/`recon-root` special ids), and case-insensitive search (with an alpha-only fast path that avoids materializing names).
- `query()` returns `{nodes, total, totalBytes, elapsedMs}` with lexicographic sort (no localeCompare — perf); `count()`, `queryNodes()` (index stream for recover-all), `searchingPathAt(tick)` (deterministic "Searching:" ticker item), `stats(scope)`.

**MockConnector** (`mock-connector.ts`): holds `scenario` (default `'standard'`),
`seed` (default **42**), `deviceList`, `scans: Map<scanId, ScanRuntime>`,
`nodeSpaces: Map<volumeId, NodeSpace>`, `sessionStore`, `sessionPathToScan`,
`jobs: Map<jobId, JobRuntime>`, `repairUndoLogs`, `repairApplied`, monotonic
`nextScanId`/`nextJobId`/`nextSessionId`, `pickIndex`.

- `setScenario`/`setSeed`/`reset` → `rebuildDevices()` (rebuilds device list + NodeSpaces with `nodeSeed ^ seed`). `permission-denied` flips volume C: status; `bad-sectors` forces `quickScanAvailable: false` on all volumes.
- **Scan simulation**: automatic stage chain `STAGE_ORDER = ['volume-analysis','quick-index','deep-walk','carve','tree-build','done']` with `STAGE_WEIGHTS = { volume-analysis: 0.04, quick-index: 0.14, deep-walk: 0.42, carve: 0.24, tree-build: 0.16 }` (normalized over the active stages; quick-index omitted when `!quickScanAvailable`). `startScan` validates permission, resolves target (device or volume id), resolves `pathScope` (hard `scan-scope-invalid` + reasonKey `errors.scanScopeInvalid` on failure), seeds `badSectorMap` in the `bad-sectors` scenario (14 ranges from `mulberry32(seed ^ 0xbad5e)`), and starts `setInterval(scanTick, 55 + (scanId.length % 3) * 25)` ms ≈ 55–105 ms ticks.
- `scanTick`: per-tick `stageProgress += 0.014..0.02` (0.22 in test mode `import.meta.env.MODE === 'test'`); **disconnect injection** — at `overall > 0.38` in the `disconnect-mid-scan` scenario it one-shot pauses and sets `deviceConnected: false` (resume clears it = "user reconnected"); bad-sector error counter ticks every 9th tick; bytes/throughput/ETA derived from `totalTicks = 96 (+60 for >500k files)`; overall = weighted normalized stage progress; on completion: exact `filesFound`/`dataFound` from `space.stats(scope)` and **auto-save session** (`auto_<volumeId>_<timestamp>.rsf`); found-file curve during the run is `1 - exp(-3.4 * overall)`; `currentPath` ticker assigned during deep-walk/carve only.
- **Progress events**: `onScanProgress` is a `setInterval(150 ms)` poll that pushes `toProgress(rt)` snapshots (NOT a push stream — the mock is poll-shaped; the real bridge is expected to map Tauri `emit`/`listen` onto the same subscription shape).
- `pauseScan`/`resumeScan`/`cancelScan` flip status + clear the timer with simulated latency (~180-220 ms; contract budget: pause/resume < 600 ms, cancel ≤ 300 ms).
- **Results during scan**: `getResultTree`/`queryResults`/`countResults` scale by `visibleFraction(rt)` (`1 - exp(-3.4 * overall)`, min 0.05; exactly 1 once completed/cancelled) and apply the scope mask.
- **Preview** (`requestPreview`): connector-side kind dispatch `previewKindOf(node)` (pdf → image exts → av exts → office exts → text exts → hex default); image = deterministic **synthetic SVG data URL** (gradient + 7 random circles + name label, `mulberry32(idx ^ 0x5eed)`); hex = 64 KiB random bytes (`idx ^ 0xbeef`) with `preview-too-large` over 64 GiB cap; text = 220 generated lines (UTF-8 / UTF-16LE on `idx % 7`), truncated flag over 16 KB; pdf = 1-3 synthetic A4 page SVGs; av = metadata + thumbnail; office = metadata + extracted text. **Injected failures**: `.crypt` ext → `preview-encrypted` (`preview.reason.encrypted`), every 97th image → `preview-damaged`. `getThumbnail` returns the synthetic image only when `node.thumbnailAvailable` (graphics/video categories).
- **Sessions**: in-memory map keyed by filename; `loadSession` throws `session-corrupt` for `auto_*` files in the `corrupt-session` scenario, else `session-not-found` for unknown paths. `getSessionMeta` synthesizes meta from the stored scan.
- **Recovery jobs** (`startRecovery`): explicit `nodes` selection materializes items immediately; `all` selection = **real lazy job** — `jobTick` (120 ms interval) materializes matching node indices in a lookahead window (`max(2048, batch*2)`) via `space.queryNodes`, advances a scale-aware batch (`batch = ceil(totalCount / 200 ticks)`; 25 ticks in test mode), per-item outcome rolls (4% failed `recovery-item-failed`, collision-skip > 0.92, rename suffix " (1)" > 0.88), verifying pass when `verifyOnWrite`, ETA from batch cadence. `getRecoveryReport` caps per-item lists at **500** with exact `processedTotal` and `truncatedItemLists` flag.
- **Health** (`getHealth`): deterministic SMART table from `mulberry32(seed ^ serial.length ^ 0xea17)`; SSD detection by model string (`ssd`/`evo`) or usb kind; `caution` for `dev-data` or bad-sectors scenario; `bad` for `failing-disk` (reallocated 1200-6400, pending 800-3200); attrs 5/9/10/12/194/196/197/198 + SSD-only 177/178/231.
- **Repair**: dry-run returns 3 canned findings (`boot-sector-mismatch` @ LBA 0, `backup-dbr-stale` @ LBA 32, `fat-table-damaged` @ FAT region 1) for `bad-sectors` scenario or `dev-data`, else none; `applyRepair` **hard-requires `consentToken === 'REPAIR'`** (else `repair-consent-required` + reasonKey `repair.consentRequired`), appends 3 undo entries (512B/131072B/512B snapshots); `undoRepairs` clears the log and returns the count.
- **System**: `getDriverStatus` → `{installed: true, version: 'mock-1.0.0', running: true}`; `getAppInfo` → `{name: 'Recovery Wizard', version: appVersion (imported from package.json!), connectorMode: 'mock', engineVersion: null, platform: navigator.platform}`; `validateDestination` resolves source device-or-volume → letters, **hard overlap refusal** (`reason: 'overlap'`, reasonKey `recovery.overlapReason`) for same-letter destinations, `invalid` for paths not under a mount point, `out-of-space` in that scenario; `pickDirectory(hint)` — `{volumeId}` roots at the hinted volume + `suggestedScopePath()`, `{excludeDeviceId}` avoids the source device's volumes, else cycles mount points (`pickIndex++`) returning `<drive>:\Recovered Files`; `listMountPoints` from volumes with letters (`C:\` style paths); **`openPath` honestly throws `not-implemented`** with reasonKey `errors.openPathMock` ("the mock has no OS access").
- **Simulated latency**: every method wraps `await latency(meanMs, jitterMs)` — e.g. listDevices 180±60, startScan 420±120, refreshDevices 700±200, queryResults 90±50, requestPreview 240±160, getHealth 280±120, dryRunRepair 600±200, applyRepair 900±300 (min 20 ms floor). Full budget table in §6.

#### Every UI call site through the seam

All 34 `connector.*` invocation sites live in **`src/queries/`** (verified by
grep; enforced by arch-lint A1 + ast-grep rule + eslint). Exact list:

| Query file:line | Call | Consumed by (components) |
| --- | --- | --- |
| `src/queries/devices.ts:8` `listDevices` | `useDevices()` | HomePage, SdCardPage, LostPartitionPage, usePhysicalDevices (health/repair), ScanProgress sidebar n/a |
| `src/queries/devices.ts:15` `refreshDevices` | `useRefreshDevices()` | HomePage, SdCardPage, LostPartitionPage |
| `src/queries/scan.ts:91` `startScan` | `useStartScan()` | HomePage, SdCardPage, HealthPage, LostPartitionPage, ResultsRoute PartialBanner |
| `src/queries/scan.ts:102` `getScanState` | `useScanState(scanId)` (poll 500 ms while running/starting) | ScanProgressPage, ResultsRoute, RecoveryPage |
| `src/queries/scan.ts:31` `onScanProgress` | external store in `useScanProgress()` | ScanProgressPage (gauge/status/ticker), also invalidates tree/results/count/session queries on status change & 10%-buckets |
| `src/queries/scan.ts:137` `pauseScan` / `:145` `resumeScan` / `:154` `cancelScan` | `usePauseScan`/`useResumeScan`/`useCancelScan` | ScanProgressPage (pause/resume/stop-confirm; resume doubles as reconnect) |
| `src/queries/results.ts:28` `getResultTree` | `useResultTree()` | ResultsTree (root), results-explorer breadcrumb |
| `src/queries/results.ts:44` `queryResults` | `useResultsPages()` (infinite, `RESULTS_PAGE_SIZE = 200`) | ResultsExplorer/VirtualFileList |
| `src/queries/results.ts:62` `countResults` | `useCountResults()` | ResultsExplorer (select-all tri-state sizing, smart-select compare) |
| `src/queries/results.ts:71` `getNodeDetail` | `useNodeDetail()` | PreviewPane, ConfidenceBadge (reason tooltip) |
| `src/queries/results.ts:79` `preview.getThumbnail` | `useThumbnail()` | PreviewPane Thumbnail |
| `src/queries/preview.ts:10` `requestPreview` | `usePreview()` (retry: false — terminal diagnostics) | PreviewPane modal body |
| `src/queries/sessions.ts:8` `listSessions` | `useSessions()` | HomePage recent list, BackupPage |
| `src/queries/sessions.ts:15` `saveSession` | `useSaveSession(scanId)` | ResultsExplorer "Save this search" |
| `src/queries/sessions.ts:23` `loadSession` | `useLoadSession()` | HomePage resume, BackupPage open |
| `src/queries/recovery.ts:74` `startRecovery` | `useStartRecovery(scanId)` | RecoveryPage Save button |
| `src/queries/recovery.ts:34` `onRecoveryProgress` | external store in `useRecoveryProgress()` | RecoveryPage QueueProgress; invalidates job/items/report on status change |
| `src/queries/recovery.ts:98` `getRecoveryState` | `useRecoveryJob()` | RecoveryPage (running check) |
| `src/queries/recovery.ts:106` `getRecoveryItems` | `useRecoveryItems()` (page 0/500) | RecoveryPage RecoveryQueue list |
| `src/queries/recovery.ts:114` `cancelRecovery` | `useCancelRecovery()` | RecoveryPage CancelRecoveryButton (confirm dialog) |
| `src/queries/recovery.ts:125` `getRecoveryReport` | `useRecoveryReport()` | RecoveryPage QueueReport |
| `src/queries/health.ts:8` `getHealth` | `useHealth()` (staleTime 60 s, retry false) | HealthPage |
| `src/queries/repair.ts:8` `dryRunRepair` / `:18` `applyRepair` / `:28` `getUndoLog` / `:36` `undoRepairs` | `useRepairDryRun`/`useApplyRepair`/`useUndoLog`/`useUndoRepairs` | RepairPage VolumeRepairWorkbench |
| `src/queries/system.ts:8` `getAppInfo` | `useAppInfo()` | SettingsPage, AboutDialog |
| `src/queries/system.ts:16` `getDriverStatus` | `useDriverStatus()` | SettingsPage, AboutDialog |
| `src/queries/system.ts:24` `listMountPoints` | `useMountPoints()` (staleTime 30 s) | RecoveryPage destination rows |
| `src/queries/system.ts:35` `validateDestination` | `useValidateDestination(sourceTargetId)` mutation | RecoveryPage live validation |
| `src/queries/system.ts:42` `pickDirectory` | `usePickDirectory()` mutation | HomePage (folder scope, hint volumeId), RecoveryPage (excludeDeviceId) |
| `src/queries/dev.ts:9` `getAppInfo` | `useConnectorMode()` | ResultsExplorer (mock ⇒ disable Open-location context item) |

**Contract methods NOT called by the UI (3):** `devices.getDeviceInfo`,
`sessions.getSessionMeta`, `system.openPath` — implemented in the mock (and
documented) but no query hook or component invokes them today. The real bridge
may still implement them; nothing depends on them.

**Mock-only control call sites:** `useScenarioControl()` (`src/queries/dev.ts`)
→ `scenarioControl()` → consumed by `DevPanel`
(`src/features/dev-panel/dev-panel.tsx`, DEV builds only: mounted under
`import.meta.env.DEV` in `WizardShell`, visibility gated by `ui.devPanelOpen`).
Changing the scenario calls `qc.invalidateQueries()` (whole cache).

### 3.5 The import rule / enforcement (the seam's guard rails)

Four independent layers, all in CI (`.github/workflows/ci.yml`):

1. **`rules/no-connector-import-outside-queries.yml`** (ast-grep, run with negation globs `!src/queries/** !src/connector/**`): pattern `import $$$ from '@/connector'` — only queries/ and connector/ may import the factory.
2. **`rules/no-raw-invoke.yml`** (ast-grep, `!src/connector/**`): pattern `invoke($$$)` — raw Tauri invoke only inside the connector layer.
3. **`scripts/arch-lint.mjs`** — A1 (factory/implementation imports only in queries/ + connector/; **type-only imports of `@/connector/contract` legal everywhere**), A2 (no invoke outside connector), A3 (components ↛ features), A4 (app layer imports features only in shell/router), A5 (feature isolation; allowed compositions: scan-progress→results-explorer, results-explorer→preview), A6 (no `any` in connector exported types).
4. **`eslint.config.js` `import-x/no-restricted-paths`** zones mirroring A3/A5 (features cannot import each other; components cannot import features).

`docs/ARCHITECTURE.md` records the contract: "Only `src/queries/**` and
`src/connector/**` import the connector factory/implementations. Type-only
imports of `@/connector/contract` are legal everywhere."

### 3.6 Swapping procedure (what the next agent must do)

From `docs/CONNECTOR-CONTRACT.md` §"Swapping in the real bridge" and
`docs/ROADMAP.md` §"Next: engine integration session":
1. Implement `RecoveryConnector` in `src/connector/tauri/tauri-connector.ts` delegating to `invoke()` on thin Rust commands in `src-tauri/src/commands/` (reveal/path/environment helpers already exist); map engine progress callbacks to the two event subscriptions via Tauri `emit`/`listen`.
2. Delete the loud stub; set `VITE_CONNECTOR=tauri` at build time. **No component or view changes** — all consumption is through `src/queries/*`.
3. Replace JS-side `validateDestination` with engine volume checks (same-letter + raw-volume-open semantics).
4. Re-measure perf grid #34–44 on real hardware.

---

## 4. State management

**Server state** — TanStack Query, single `QueryClient` in
`src/queries/client.ts` (retry 1, staleTime 5 s, gcTime 5 min,
refetchOnWindowFocus false; mutations retry 0). Query-key registry
`queryKeys` (devices, device(id), scanState, scanProgress, tree, results,
count, nodeDetail, thumbnail, preview, sessions, job, jobItems, report,
health, appInfo, driverStatus, mounts, repairDryRun, undoLog).

**Live streams** — module-level external stores over the two event
subscriptions (`src/queries/scan.ts` `getScanStore`, `src/queries/recovery.ts`
`getJobStore`), consumed via `useSyncExternalStore` (React-hooks-v7-clean).
The scan store ALSO drives cache invalidation: on status change it invalidates
scanState (+ tree/results/count/sessions on terminal status); while running it
re-invalidates tree/results/count every 10% progress bucket — this is how live
scan results flow into the 3-pane explorer. Unsubscribe + map cleanup when the
last listener detaches. Initial prime from `getScanState` async.

**Zustand stores (4 files):**
- `src/stores/ui-store.ts` — `viewMode: 'details' | 'icons'`, `previewPaneWidth` (240–560, default 320), `sidebarCollapsed` (persisted `localStorage['rw.ui.sidebarCollapsed']`), `devPanelOpen`, `perfOverlayVisible` + setters.
- `src/stores/scan-session-store.ts` — `active: {scanId, targetId} | null`, `known: Record<scanId, {scanId,targetId}>`, `celebrationSeenFor` (one-shot celebration modal); setters `setActiveScan` (called by `useStartScan` onSuccess), `markScanStopped` (cancel), `setActiveStatus`, `markCelebrationSeen`.
- `src/stores/filter-store.ts` — `ResultsFilterUi` (treeMode, treeNodeId, search/searchApplied debounced, categories, datePreset+from/to, sizePreset+min/max, extensionInput/extensions, existingOnly, hideEmptyFiles) + `useSortStore` (field: name|modified|type|size|confidence, direction) + **`resolveFilter()`** which produces the connector `ResultsFilter` payload (incl. preset→ISO-range mapping, MB→bytes, `existingOnly` → `origins: ['existing']`).
- `src/stores/selection-store.ts` — two modes: `ids: Set<string>` or `all` (filter snapshot `allFilter` + `allCount` + per-row `exclusions: Set`) mirroring `RecoverySelection`; `selectedCountOf()` computes the visible count. Selection survives into RecoveryPage via `?all=1`/`?ids=1` params + store.

**Data flow summary:** connector → queries (hooks/stores) → feature components
→ design-system components. Nothing skips the queries layer.

**localStorage keys:** `rw.ui.theme` (theme mode), `rw.ui.density`,
`rw.ui.locale`, `rw.ui.sidebarCollapsed`, `rw.ui.scale` (settings page),
`diskgenie.theme` (adopted themes.ts legacy helper — the ThemeProvider itself
uses `rw.ui.theme`).

---

## 5. Data shapes the UI expects (contract types, exact fields)

All from `src/connector/contract.ts` (line refs in parens). **All fields are
`readonly`.**

Primitives: `Iso8601 = string` (20); `FileCategoryId = 'graphics' | 'video' |
'audio' | 'document' | 'email' | 'archive' | 'bookmarks' | 'other'` (22);
`ConfidenceGrade = 'excellent' | 'good' | 'fair' | 'poor' | 'unknown'` (26);
`DeviceKind = 'internal' | 'usb' | 'sd' | 'optical' | 'virtual'` (28);
`VolumeFs = 'ntfs' | 'fat32' | 'exfat' | 'ext4' | 'btrfs' | 'refs' | 'hfsplus'
| 'zfs' | 'raw' | 'unknown'` (29); `DeviceStatus = 'ok' | 'permission-denied'
| 'disconnected' | 'locked'` (41); `Capacity = { totalBytes, usedBytes,
freeBytes }` (43).

**Errors**: `ConnectorError extends Error { code: ConnectorErrorCode;
reasonKey?: string; detail?: Record<string, unknown> }` (49). Codes (56-80):
`device-gone, device-permission, scan-not-found, scan-not-paused,
scan-not-running, scan-scope-invalid, session-corrupt, session-not-found,
destination-overlap, destination-invalid, destination-unwritable,
out-of-space, preview-unsupported, preview-too-large, preview-damaged,
preview-encrypted, preview-timeout, preview-missing-data,
recovery-item-failed, repair-consent-required, repair-failed,
health-unavailable, not-implemented, unknown`. `reasonKey` is an i18n key under
`errors.` or `preview.reason.`.

**Device/Volume** (86-126): `Device { id, kind, model, label, serial,
capacity, status, isSourceOfActiveScan?, volumes, canScanWholeDisk }`;
`Volume { id, deviceId, label, letter: string|null, fs, capacity, status,
lostPartition?: LostPartitionHint, quickScanAvailable, sectorSize?, bitlocker?:
boolean | 'unlocked' | 'locked' }`; `LostPartitionHint { id, startByte,
sizeBytes, probableFs, note? }`.

**Scan** (143-223): `ScanStatus = 'idle' | 'starting' | 'running' | 'paused' |
'pausing' | 'resuming' | 'stopping' | 'completed' | 'cancelled' | 'failed'`;
`ScanStage = 'volume-analysis' | 'quick-index' | 'deep-walk' | 'carve' |
'tree-build' | 'done'`; `ScanPhase = 'quick' | 'advanced' | 'organizing'` with
free fn `scanPhaseOf(stage)` (quick: volume-analysis+quick-index; advanced:
deep-walk+carve; else organizing); `ScanParams { targetId, pathScope? }`;
`ScanProgressEvent { scanId, status, stage, stageProgress (0..1 per stage,
may reset), overallProgress (0..1 weighted, MONOTONIC), bytesScanned,
bytesTotal, filesFound, dataFoundBytes, errors, badSectorMap:
BadSectorRange[], etaSeconds: number|null,
throughputBytesPerSecond, recoverableDuringScan, deviceConnected,
currentPath: string|null }`; `BadSectorRange { startByte, sizeBytes }`;
`ScanState { scanId, params, target: Device|Volume, status, startedAt,
completedAt, progress, failure?: {code, reasonKey?} }`.

**Results** (229-322): `ResultNodeKind = 'file' | 'directory' | 'group'`;
`ResultOrigin = 'existing' | 'deleted' | 'raw' | 'lost-path' | 'unsaved'`;
`ResultNode { id, scanId, kind, name, path: string|null, sizeBytes,
modifiedAt: Iso8601|null, extension: string|null, category:
FileCategoryId|null, confidence, origin, childCount: number|null,
thumbnailAvailable, parentId: string|null, attributes? }` — **node ids are
`f<index>` for files, `d<index>` for dirs (the mock parses
`nodeId.replace(/^f/,'')`)**; `ResultTreeMode = 'path' | 'type' | 'raw'`;
`TreeNode { id, parentId, label, icon: 'directory'|'category'|'group'|'device',
childCount, fileCount, sizeBytes, origin, special?: 'deleted'|'lost-path'|
'raw-found'|'unsaved'|'existing' }`; `TreePage { nodes, parentHasMore }`;
`SortField = 'name'|'modified'|'type'|'size'|'confidence'`; `SortDirection`;
`ResultsFilter { nodeId, treeMode, search, categories, dateFrom, dateTo,
sizeMinBytes, sizeMaxBytes, extensions, origins, hideEmptyFiles }` (note:
**no `confidence` field** — removed in ADR-008); `ResultsPage { nodes, total,
totalBytes, offset, pageSize, elapsedMs }`; `NodeDetail { node,
fullPhysicalPath, createdAt, accessedAt, confidenceReason, badSectorsOverlap,
clusterCount }`.

**Preview** (328-377): `PreviewPayload` discriminated union on `kind`:
- `'image'`: `{ dataUrl, width, height, bounded }`
- `'hex'`: `{ bytes: Uint8Array, totalSize, baseOffset }`
- `'text'`: `{ text, encoding, totalSize, truncated }`
- `'pdf'`: `{ pages: { dataUrl, width, height }[], pageCount }`
- `'av'`: `{ metadata: Record<string,string>, thumbnailDataUrl: string|null, durationSeconds: number|null }`
- `'office'`: `{ metadata, extractedText: string|null, truncated }`

**Sessions** (383-398): `SessionMeta { id, fileName, savedAt, targetLabel,
filesFound, dataFoundBytes, completed }`.

**Recovery** (404-489): `CollisionPolicy = 'rename'|'overwrite'|'skip'`;
`RecoveryOptions { collision, rebuildFolders, keepEmptyFolders, fileRepair,
verifyOnWrite }`; `RecoveryJob { jobId, scanId, nodeIds, destination, options,
status }`; `RecoveryJobStatus = 'validating'|'running'|'completed'|
'completed-partial'|'cancelled'|'failed'`; `RecoveryItem { nodeId, name,
sizeBytes, state: 'pending'|'running'|'done'|'failed'|'skipped'|'verifying',
error?: {code, reasonKey?}, savedAs? }`; `RecoveryProgressEvent { jobId,
status, overallProgress, currentFile, currentFileProgress, completedCount,
failedCount, skippedCount, totalCount, throughputBytesPerSecond, verifying,
etaSeconds }`; `RecoveryReport { jobId, status, startedAt, finishedAt,
succeeded, failed, skipped (each RecoveryItem[]), totalBytesWritten, verified,
processedTotal, truncatedItemLists }`; `RecoverySelection = { kind: 'nodes',
nodeIds } | { kind: 'all', filter: ResultsFilter }`.

**Health** (495-522): `HealthStatus = 'good'|'caution'|'bad'`;
`SmartAttribute { id, name, current: number|null, worst: number|null,
threshold: number|null, raw, status: 'ok'|'failing'|'unknown' }`;
`DiskHealth { deviceId, status, temperatureC, powerOnHours, diskClass:
'ssd'|'hdd'|'unknown', wearPercent, trimEnabled, reallocatedSectors,
pendingSectors, attributes }`.

**Repair** (528-567): `RepairFindingKind = 'boot-sector-mismatch' |
'backup-dbr-stale' | 'fat-table-damaged' | 'mft-record-error' |
'sectors-per-cluster-corrupt' | 'checksum-mismatch'`; `RepairFinding { id,
kind, where, descriptionKey (i18n), fixable, severity: 'info'|'warning'|
'critical' }`; `RepairDryRun { deviceId, findings, canApply }`;
`RepairUndoEntry { index, action, where, at, snapshotBytes }`.

**System** (573-625): `DriverStatus { installed, version, running, note? }`;
`AppInfo { name, version, connectorMode: 'mock'|'tauri', engineVersion:
string|null, platform }`; `MountPoint { id, label, path (e.g. `C:\`),
freeBytes, totalBytes }`; `DestinationValidation { allowed, reason?:
'overlap'|'unwritable'|'invalid'|'out-of-space', reasonKey?, freeBytes,
neededBytes }`.

**Mock-only**: `MockScenarioId = 'standard' | 'large-ntfs' | 'sd-card' |
'btrfs-disk' | 'lost-partition' | 'permission-denied' | 'disconnect-mid-scan'
| 'bad-sectors' | 'failing-disk' | 'out-of-space' | 'corrupt-session'`.

---

## 6. Threading / lifecycle / timing assumptions

The UI's assumptions about backend behavior (all asserted in
`tests/connector-mock.test.ts` and documented in `docs/CONNECTOR-CONTRACT.md`):

- **Progress cadence**: mock ticks the simulator at ~55–105 ms and emits
  progress snapshots at a **150 ms subscription cadence** ("100–250 ms
  progress ticks" per README); the UI renders whatever arrives, with no
  built-in throttling. The scan-state query additionally polls at 500 ms
  while running (defensive; the event stream is primary).
- **Latency budgets** (mock `latency(mean, jitter)`): devices list 180±60,
  device info 90±40, refresh 700±200, startScan 420±120, pause ~180,
  resume ~200, cancel ~220, getScanState 40±20, tree 60±30, queryResults
  90±50, nodeDetail 50±25, count 60±30, preview 240±160, thumbnail 120±60,
  saveSession 320±120, listSessions 120±60, loadSession 420±180, startRecovery
  300±100, cancelRecovery ~200, report 40, health 280±120, dryRun 600±200,
  applyRepair 900±300, undoRepairs 700±250, validateDestination 180±80,
  pickDirectory ~150, mounts 100±40, driver 60, appInfo 40 (ms).
- **Contract timing budgets enforced by tests**: pause/resume < 600 ms; cancel
  ≤ 300 ms; filter query < 200 ms on the standard scenario
  (`ResultsPage.elapsedMs`); recover-all over 340k items completes in bounded
  wall time (mock batches); preview < 500 ms target (ADR-003).
- **Monotonicity**: `overallProgress` never regresses; `stageProgress` resets
  per stage; completion reports exactly 1.0 and exact `filesFound`/
  `dataFoundBytes`.
- **Cancellation**: YES, both scan and recovery have cancel buttons.
  - Scan stop button → confirm dialog → `cancelScan(scanId)` → mock clears the
    timer, status `'cancelled'`, ~220 ms. On success the UI `markStopped()`
    (scan-session store), invalidates sessions, navigates to
    `/results/:scanId` — cancelled scans keep browsable partial results with
    the truthful "Search was stopped early" banner (ResultsRoute) offering a
    re-scan that preserves `pathScope`.
  - Recovery cancel → confirm dialog → `cancelRecovery(jobId)` → status
    `'cancelled'`; already-copied files stay (copy: "Files already copied
    will remain in the destination").
- **Pause/resume**: scan only; resume-after-disconnect models the user having
  re-plugged the device (`disconnectFired` cleared on resume; UI shows the
  non-dismissable reconnect modal when `deviceConnected === false && paused`).
- **RecoverableDuringScan**: the "Recover All" CTA is disabled mid-scan until
  the connector says extents are safe (mock: `overall > 0.4`, or `> 0.15`
  during deep-walk/tree-build); tooltip explains.
- **Events are subscription-shaped** (`onScanProgress`/`onRecoveryProgress`
  return unsubscribe fns). The real bridge must map engine callbacks / Tauri
  `emit`-`listen` to this shape; the queries layer wraps them in
  `useSyncExternalStore` stores that also invalidate the query cache on
  status transitions and 10%-progress buckets.
- **Bounded payloads** principle (ADR-003): hex capped 64 KiB, text capped,
  PDF/Office as bounded rendered artifacts, AV first-frame only. The UI never
  requests unbounded data.
- **Sector math**: the scan page derives "Reading sector: X/Y" by dividing
  `bytesScanned`/`bytesTotal` by 512 — a real engine must keep byte units.
- **IDs**: scanIds are opaque strings (`scan-1`, `job-1` in the mock); node
  ids `f<index>` / `d<index>`; the mock's `getNodeDetail`/`requestPreview`/
  `startRecovery` parse the numeric suffix — **a real backend can keep opaque
  ids as long as `getNodeDetail(scanId, nodeId)` resolves them** (only the
  mock's internals assume the `f`-prefix).

---

## 7. Error handling

- **Error surface**: `src/app/error-boundary.tsx` — class `ErrorBoundary`
  (logs via structured logger `ui-crash`) + `ErrorScreen` (`role="alert"`,
  icon, `errors.title`, `error.message || errors.unknown`, Home + Retry
  buttons). Wraps the whole `WizardShell` in the router.
- **Connector errors carry `code` + optional `reasonKey`**; UI maps reasonKey
  through `t()` — never string-matches messages. Examples:
  - HomePage `beginScan` onError → `toast.error(reasonKey ? t(reasonKey) : err.message)`.
  - Session resume onError → `t(reasonKey)` or `errors.sessionCorrupt` fallback.
  - PreviewPane error state → `FileWarningIcon` + `preview.failedTitle` + `t(reasonKey) ?? message` (retry disabled: `usePreview` has `retry: false` — preview failures are terminal diagnostics).
  - RecoveryPage queue item badges show `t(item.error.reasonKey)` as tooltip; failed report rows show `t(f.error.reasonKey)`.
  - Repair consent failure surfaces via toast (`err.message`).
- **Query-level surfaces**: per-page `EmptyState` with retry/refetch for
  isError (devices, health, repair dry-run); `Alert` for validation refusals;
  Skeletons for loading.
- **Toasts**: sonner stack, top-right below title bar, 4 s, hover-pause,
  aria-live polite (`src/components/ui/toaster.tsx`).
- **Hard gates**: destination overlap refusal (Save disabled + alert + row
  hint), insufficient-space modal, scan-scope-invalid blocks the scan start,
  repair consent (`REPAIR` typed token) blocks apply.
- **Structured logger** (`src/lib/logger.ts`): ring buffer (500 entries)
  rendered in Settings → Diagnostics; `exportLogText()` copy; console output
  dev-only; eslint `no-console: error` except logger. Long-task watchdog
  (PerformanceObserver, dev only) feeds `PerfOverlay`.
- **Stub truthfulness**: tauri-mode misconfiguration throws loud
  `not-implemented` on every method (message points at CONNECTOR-CONTRACT.md);
  test `tests/connector-mock.test.ts` asserts it.

---

## 8. App shell, theming, i18n, packaging

**Window / shell** (`src-tauri/tauri.conf.json`): productName "Recovery
Wizard", identifier `com.heicfast.recoverywizard`, single window `main` titled
"Recovery Wizard" **1240×840, min 800×600, opens centered + MAXIMIZED,
`decorations: false`** (custom chrome). `withGlobalTauri: false`, CSP `null`,
devUrl `http://localhost:1420` (beforeDevCommand `pnpm dev`), frontendDist
`../dist` (beforeBuildCommand `pnpm build`).

**Custom chrome**: `WizardShell` header is the drag region
(`data-tauri-drag-region`, dblclick toggle-maximize Tauri-only); `WindowControls`
renders only when `isTauriRuntime()` (`'__TAURI_INTERNALS__' in window`,
`src/lib/runtime.ts`) — browser gets host chrome. Capabilities
(`src-tauri/capabilities/default.json`): `core:default`, `dialog:default`,
`opener:default`, `core:window:allow-{minimize, toggle-maximize, is-maximized,
close, start-dragging}`. Plugins: `tauri-plugin-dialog`, `tauri-plugin-opener`
(NOT used by any UI code yet — staged for pickDirectory/openPath).

**Theming** (ADR-007): two-layer adopted system — `src/theme/adopted/tokens.css`
(immutable raw palettes: light, dark, ember, tide, blossom + HC-derivation) +
`src/theme/adopted/themes.ts` (`THEME_IDS`, `familyOf`, `applyThemeAttributes`
writes `data-theme` + `data-scheme` on `<html>`) → semantic mapping
`src/styles/tokens.css` (app names ← adopted names, a11y-adjusted accents) →
Tailwind `@theme inline` in `src/styles/index.css` (dark: variant =
`[data-scheme='dark']`). `ThemeProvider` (`src/app/theme.tsx`) adds `system`
(matchMedia) + `high-contrast` (adds `.hc` class) modes, density
(`data-density`), forced-colors tracking; persisted in localStorage.

**i18n** (`src/i18n/index.ts`, `en.ts` 467 lines, `zh-CN.ts` 455): auto-detect
(navigator.language zh* → zh-CN else en, persisted `rw.ui.locale`), fallback en.
~330 leaf keys across namespaces: app, about, activation, menu, categories,
common, chrome, nav, sidebar, home, sdCard, tools, backup, lostPartition,
scan, results (column/confidence/tree/context/selection/status/views/session),
preview (reason.*), recovery (queueItem, collisions…), health (smartCol…),
repair (finding.*, consent…), settings, errors, a11y, devPanel. Plural pairs
`X`/`X_other` (i18next). Governance: `scripts/i18n-sweep.mjs` (paren-balanced
`t()` extraction, template-family expansion against contract unions, labelKey/
reasonKey/descriptionKey indirection) + tests (missing=0, unused=0, parity).

**Packaging / bundle** (`tauri.conf.json` bundle): `active: true`,
**`targets: ["nsis"]`** only; icons: 32x32, 128x128, 128x128@2x, icon.ico,
icon.png (+ full Square* set + icon.icns in `src-tauri/icons/`, generated by
`pnpm icons` from `src-tauri/app-icon.png`). Windows NSIS:
`installerIcon: icons/icon.ico`, `installMode: "both"` (per-user + per-machine),
`languages: ["English", "SimpChinese"]`, `displayLanguageSelector: true`;
webviewInstallMode `downloadBootstrapper`. `nsis-header.bmp` present.
Release workflow builds unsigned NSIS + SHA-256, release notes disclose
no code signing; version-match gate tag `v*` == conf version. **No updater
config** (no `plugins.updater` — none exists).

**Build artifacts**: `dist/` (Vite, target chrome110, minified, no sourcemap,
manual chunks: vendor-i18n, vendor-query (@tanstack+zustand), vendor-react);
budget-gated by `scripts/bundle-check.mjs` (BUDGET_MAIN_KB=200,
BUDGET_TOTAL_KB=500 env-overridable). Tauri release profile: codegen-units 1,
lto, opt-level "s", panic abort, strip.

---

## 9. Test infrastructure

**Unit/contract/component** — Vitest 4.1.12, jsdom, setup
`tests/setup.ts` (jest-dom, matchMedia mock, i18n import, RTL cleanup). 22
files / 145 tests (`pnpm test`), coverage v8 with thresholds 70/60/70/70
covering connector, stores, lib, theme, shell, device-selection, about-dialog,
and a listed set of ui components (`vite.config.ts` coverage.include).

Key files:
- `tests/connector-mock.test.ts` (570 lines) — THE contract suite: determinism, quick-scan availability, virtual targets, direct-start automatic chain (full stage order), quick→advanced phase mapping + monotonic overall, collapsed-quick (bad-sectors) reaches 100% + exact counts, ticker stage-gating, Recycle Bin/Desktop scans, pause/resume <600 ms, cancel ≤300 ms, disconnect injection + resume-reconnect, deterministic paging + category filter, lost-path origins, hide-empty filter, filter <200 ms (`elapsedMs`), preview kind determinism, destination-exclusion picking, `scan-scope-invalid` rejection + absolute/relative scope forms + scoped tree root, overlap refusal (device OR volume id), explicit-selection job, **recover-all real job** (processedTotal == count, truncated lists, ≤500 items), SMART field set, failing-disk 'bad', REPAIR consent + undo round-trip, openPath not-implemented, tauri stub throws.
- `tests/node-space.test.ts` — NodeSpace determinism, paging, filters, search, scope masks, aggregates.
- `tests/stores.test.ts`, `tests/scan-session-store.test.ts`, `tests/ui-store.test.ts`, `tests/filter… via stores`, `tests/theme-persist.test.ts`, `tests/theme-snapshot.test.ts` (token fixture pin `tests/fixtures/adopted-tokens.snapshot.json`), `tests/i18n-parity.test.ts`, `tests/i18n-sweep.test.ts`, `tests/i18n-sweep-extraction.test.ts`, component tests (`tests/components.test.tsx`, `home-states`, `sidebar`, `window-controls`, `dialog-choreography`, `press-states`, `toaster`, `route-fade`, `confidence-pill`, `reference-surfaces`, `format`, `logger`).

**How tests fake the backend**: they don't mock it — unit/contract tests
instantiate the real `MockConnector` / `NodeSpace` directly (import from
`@/connector/mock/*`; legal because tests aren't restricted by A1). The mock
detects `import.meta.env.MODE === 'test'` to accelerate scan/job ticks. jsdom
shortfalls: matchMedia mocked, hex Worker falls back to inline
(`hex-worker-client.ts`), localStorage real.

**E2E** — Playwright 1.64.1, `playwright.config.ts` (testDir `./e2e`,
serial workers 1, viewport 1280×832, `webServer: pnpm preview --port 4173`,
baseURL http://localhost:4173, SMOKE_BASE_URL override, trace retain-on-
failure). **E2E runs the REAL mock connector inside the built bundle** — no
page mocking at all. 12 tests:
- `e2e/smoke.spec.ts` (6): golden path (home → direct scan → progress lines →
  completion modal → results grid → filter bar → search breadcrumb → preview
  pane confidence → checkbox → Recover 1 item → destination → report → export
  toast; console-error zero assertion; step screenshots 05/10/20/30/40/50),
  Recycle-Bin direct scan, missing-scan fallback, browser-mode window chrome
  absence, sidebar collapse + persistence + keyboard, dialog choreography
  (outside-click swallow, ESC, focus return).
- `e2e/responsive.spec.ts` (6): 800×600 / 1024×768 / 1920×1080 layout
  invariants, 150% DPI (dsf 1.5), 3-pane at min size, keyboard flow
  (link→Enter→scan starts), focus-on-`#main-content` per route.

**Fixtures**: `tests/fixtures/adopted-tokens.snapshot.json` (theme token pin).
E2E artifacts: `test-results/05-scan-live.png` … `60-sidebar-collapsed.png`
verified by `scripts/visual-compare.mjs` (PNG ≥1018×716, >10 KB) via
`.github/workflows/visual-compare.yml` (workflow_dispatch).

**Governance scripts**: `scripts/arch-lint.mjs` (A1-A6), ast-grep rules
(`rules/*.yml`), `scripts/bundle-check.mjs`, `scripts/i18n-sweep.mjs` (+
`i18n-sweep.d.mts` types), `scripts/visual-compare.mjs`. jscpd (min-tokens 70,
ignores `src/theme/adopted/**`). knip config `knip.json` (entries
tests/**, ignores adopted theme, `ignoreExportsUsedInFile`).

---

## 10. Build & packaging summary

- Dev: `pnpm dev` (mock connector default) or `pnpm tauri dev` (Vite on 1420 +
  Rust shell; watch ignores src-tauri).
- Build: `pnpm build` → `dist/`; `VITE_CONNECTOR=tauri` folds the factory to
  the stub adapter (every method throws until the integration session lands).
- Desktop bundle: `pnpm tauri build` → `src-tauri/target/release/bundle/nsis/
  Recovery.Wizard_<version>_x64-setup.exe` + `.sha256`; unsigned (disclosed).
- Rust shell: crate `recovery-wizard-ui` v0.3.3, edition 2021, deps: tauri 2,
  tauri-plugin-dialog 2, tauri-plugin-opener 2, serde; build.rs = tauri_build.
  CI runs `cargo check` + `clippy -D warnings` on windows-latest.
- CI matrix (`.github/workflows/`): `ci.yml` (typecheck→lint→format→arch-lint
  →ast-grep×2→knip→jscpd→test:coverage→build→bundle:check→cargo→clippy,
  windows-latest, Node 24 + pnpm 12.15, KNIP_DISABLE_RAW_TRANSFER=1),
  `ui-smoke.yml` (Playwright 12 + step screenshots + tauri-debug-build
  `--debug --no-bundle`, both VITE_CONNECTOR=mock), `visual-compare.yml`
  (dispatch), `release.yml` (tag v*, version match, NSIS + sha256 + notes).
- Icons: `pnpm icons` regenerates the set from `src-tauri/app-icon.png`.

**public/ assets** (`public/assets/aistudio/`, 11 PNGs): reference-screenshot
comparators only (landing-*, preview-panel-live, recovery-*) — not shipped in
the UI; content icons are inline SVG / Lucide; the only runtime-imported
binary assets are the 32 chrome SVGs under `src/assets/interim/chrome/{light,
dark}/` (+ `manifest.json` mapping each to its corpus source, per
`docs/INTERIM-ASSETS.md` §A — the documented replacement debt).

---

## 11. Dead code / gaps / stubs (what's incomplete)

From `docs/IMPLEMENTATION-STATUS.md` (all surfaces COMPLETE), `docs/AUDIT-REPORT.md`
(all 13 axes MATCH/CLEAN post-fix, no open rows), `docs/ROADMAP.md`, and code:

1. **The real backend is a stub by design** — `createTauriConnector()` throws
   `not-implemented` on every method. This is THE gap the integration session
   fills (ROADMAP item 1-4). Zero `invoke()` in `src/` today.
2. **3 contract methods unconsumed by the UI**: `devices.getDeviceInfo`,
   `sessions.getSessionMeta`, `system.openPath` (implemented in mock; no call
   sites). `openPath`'s UI affordance (context-menu "Open location") is
   present but disabled in mock mode with a truthful tooltip; wiring it is
   part of the tauri adapter work.
3. **Client-side demo surfaces (not connector-backed)**:
   - RepairPage file queue: `SAMPLE_REPAIR_FILES` static entries; "Repair All"
     just flips statuses locally; Browse button toasts the path. Only the
     Advanced-Repaid `VolumeRepairWorkbench` is connector-driven.
   - LostPartitionPage: `RESCUE_DISK_SPECS` static disk/partition display
     (Disk 2/0/1 with hardcoded LBA/size strings); the actual scan uses real
     device ids. The mock's `LostPartitionHint` (contract) is surfaced only as
     the Home badge; the lost-partition page does not read `v.lostPartition`.
   - SdCardPage's hero SD row ("SD/MMC/MS PRO … Disk2", "58 GB") is static
     display; real devices appear under "Show all external devices".
   - ActivationDialog: license UI with truthful "provider unavailable"
     (no licensing backend exists; buy → toast).
   - RecoveryPage Cloud tab: truthful unavailable text (`tools.providerBody`).
4. **`confirm exit during scan` guard** (UI-MAP dialog inventory) is NOT
   implemented — no close-guard while scanning exists in code (UI-MAP lists
   it; the window close button has no scan-state hook).
5. **Windows-hardware PARTIALs** (ACCEPTANCE.md): cold-start number, 125/175%
   fractional-DPI capture, RTL — tracked, not blocking.
6. **Interim assets debt**: chrome SVGs are corpus-derived
   (`src/assets/interim/chrome/`, manifest enumerates homegrown replacements);
   content icons are Lucide placeholders (INTERIM-ASSETS.md; icon set v2 is a
   roadmap item).
7. **Code signing** pending (release notes disclose).
8. **`unsaved` origin** and `'pausing' | 'resuming' | 'stopping'` scan
   statuses, `preview-unsupported`/`preview-timeout`/`preview-missing-data`/
   `destination-unwritable`/`repair-failed`/`scan-not-running` codes,
   `ResultNodeKind` 'directory'/'group', `TreeNode.special` 'deleted'/'unsaved',
   `DriverStatus.note`, `AppInfo.engineVersion`, `RepairFinding` kinds beyond
   the three mock ones, `SessionMeta` via real `.rsf` files — all defined by
   the contract but exercised only partially/not at all by the mock; the UI
   handles them generically where they surface (codes → errors/reasons).
9. **Mock poll-shaped events**: `onScanProgress`/`onRecoveryProgress` are
   150 ms `setInterval` snapshots. A real push bridge (Tauri listen) must
   simply call the callback; unsubscribe semantics identical.
10. **Removed on purpose (ADR-008)**: scan configuration surface
    (`/scan/:deviceId` + ScanConfigPage — deleted; scan modes/categories/
    sectorSize/badSectorPolicy/retries are engine-internal), `deleteSession`,
    `ResultsFilter.confidence`, `SessionMeta.mode`, preview `kind` argument.

---

## 12. Quick reference — the seam in one paragraph

`src/connector/contract.ts` defines `RecoveryConnector` (9 domains, 36
methods + 2 event subscriptions) and every payload type.
`src/connector/index.ts` picks the implementation via the **build-time env var
`VITE_CONNECTOR`** (`'tauri'` → stub, anything else → **`MockConnector`**,
the default) and exports the singleton `connector` plus `scenarioControl()`.
The mock lives in `src/connector/mock/{mock-connector.ts, fixtures.ts,
node-space.ts, rng.ts}` — a deterministic, mulberry32-seeded simulator (seed
42, 11 scenarios via the DEV-only dev panel) that fakes devices/results/
previews/jobs with typed-array procedural indexes, simulated 20–900 ms
latencies, 55–105 ms scan ticks, 150 ms event polls, and error injection.
Only `src/queries/**` may import the connector (arch-lint A1, ast-grep
`rules/no-connector-import-outside-queries.yml`, eslint zones); raw Tauri
`invoke()` may only appear in `src/connector/**` (`rules/no-raw-invoke.yml`).
33 of the 36 methods are consumed by 34 call sites across 10 query modules.
Swapping in a real C++ core = implement `RecoveryConnector` in
`src/connector/tauri/tauri-connector.ts` over `invoke()`/`listen()`, delete
the stub, set `VITE_CONNECTOR=tauri` — zero component changes expected.
