# UI-MAP — Recovery Wizard frontend (Tauri 2 + React 19 + Vite), Agent B (T2-B)

Ground-truth map of every screen, wire, store, OS touch, theme surface and error path in
`ui/src` + `ui/src-tauri/src`. Every claim carries file:line. Companion sheets:
`worklog/RELEASE-VALIDATION/UI-WIRING.md`, `worklog/context-sheets/UI-SURFACE.md`.

## 1. Shell & route inventory

Router: `ui/src/app/router.tsx:22-45` — React Router v7 **hash** router (WebView2-proof, router.tsx:18-21).
Single `WizardShell` layout wrapped in `ErrorBoundary` (router.tsx:26); `<Outlet/>` at shell.tsx:351.

Routes (router.tsx:31-42): `/` Home · `/sd-card` · `/tools` (aliases HealthPage, router.tsx:33) ·
`/backup` · `/scan/:deviceId/live` ScanProgress · `/results/:scanId` ResultsRoute ·
`/recover/:scanId` Recovery · `/health` · `/repair` · `/settings` · `/lost-partition` ·
`*` ResultsPageFallback.

Shell (`ui/src/app/shell.tsx`): custom title bar (shell.tsx:132-229) with brand logo, orange
"Buy Now" pill → ActivationDialog (shell.tsx:150-157), key icon (shell.tsx:160-170), language
menu (172-190), app menu (192-225), `WindowControls` (227). Sidebar: 3 groups — Data Recovery
(Home/SD/Disk Health), Advanced (Repair/Lost Partition), File Backup (Scan Snapshot)
(shell.tsx:56-100); collapses to "Back to devices" + live-scan pulse link in results flow
(shell.tsx:244-301, navigates `/scan/${active.targetId}/live` at 282 — **note: drops the
`?scanId=` param**, see §8). Global overlays mounted here: `ElevationBanner` (shell.tsx:350),
`ConsentModal` (357), `AboutDialog`/`ActivationDialog` (358-359), dev-only `DevPanel`+`PerfOverlay`
(356, 364-371). Main card is white rounded-tl on `#EBF2FF` frame (shell.tsx:130, 334-344).

## 2. Screens (purpose · states · controls · wires)

**HomePage** (`features/device-selection/home-page.tsx`, route `/`): "Select a location…" device
picker. States: skeleton (147-158), error EmptyState+retry/refresh (160-178), empty-drives
EmptyState (180-192), success grid. Controls: refresh button (217-226), DeviceCard radio grid
(248-263) with volume chips per-volume scan (647-664) + BitLocker unlock link (601-610), quick
targets Recycle Bin/Desktop/Select Folder (269-288; virtual ids `vol-recycle-bin`/`vol-desktop`
home-page.tsx:54-55, mapped to system-drive scopes bridge-side devices.rs:252-259), folder popup
Dialog (368-497), recent-sessions list (290-320) + ContinueLastBanner (699-722), footer CTA
(356-364). Wires: `useDevices`/`useRefreshDevices` (61-62), `useSessions`/`useLoadSession`/
`useResumeLastSession` (63-64), `useStartScan` (65) → navigates `/scan/<target>/live?scanId=…`
(94), `usePickDirectory` (66, 117-126). **Filter risk:** shows only `kind==='internal'` by default
(home-page.tsx:80-83) — the bridge only ever emits `internal|usb` (devices.rs:215,382), so any
disk the engine flags removable disappears from Home (defect-3 candidate; SD page likewise: sd-card-page.tsx:30).

**SdCardPage** (`features/device-selection/sd-card-page.tsx`, `/sd-card`): stylized single-card
picker + SDR/SSR/DVR feature card. Default selection is the **mock id** `dev-sd`
(sd-card-page.tsx:27); CTA `beginScan(sdDevice?.id ?? 'dev-sd')` (37-48). `useDevices`,
`useRefreshDevices`, `useStartScan`. Static i18n card metadata (`sdCard.cardSize` '58 GB',
i18n/en.ts:126) — decorative, not data-driven.

**HealthPage** (`features/health/health-page.tsx`, `/health`): SMART telemetry. Device radio
cards (138-208), health hero card with **UI-synthesized score** (100-108: good→`max(88,100-wear)`,
caution→72, bad→38 — a client-side fabrication the contract forbids, contract.ts:25-26 "the UI
never re-grades"), KPI tiles, SMART table. States: skeleton (69-84), empty (86-98), health
loading/error (210-222: error → EmptyState with `err.message`), success. Controls: refresh
(123-134, refetches devices+health), "Search for Lost Data" start-scan CTA (315-323).
Wires: `usePhysicalDevices` (hooks/use-physical-devices.ts:11-37 — filters `kind!=='virtual'`,
`?device=` param selection), `useHealth(deviceId)` (health-page.tsx:51), `useStartScan` (42).
`health_read` reaches the engine SMART layer whose TRIM probe `_popen("fsutil …")`
(engine/src/win/cr_health.cpp:306) is a known console-popup source (defect-2; no
`CREATE_NO_WINDOW` anywhere in the shell — §5).

**ScanProgressPage** (`features/scan-progress/scan-progress-page.tsx`, `/scan/:deviceId/live`):
the live 3-pane surface + status cluster. scanId = `?scanId=` ?? active store (64); no id →
fallback (120-122). Renders ResultsExplorer in liveMode (139-143), "Searching: path" ticker
(148-157 — hidden because the bridge always sends `currentPath: null`, events.rs:193), bottom
cluster: gauge, "Quick/Advanced Scanning %, Files Found: N (size)" (168-197), pause/resume/stop
(200-268), "Reading sector: X/Y" (257-262), Recover-All CTA gated on `recoverableDuringScan`
(273-289). Dialogs: device-lost reconnect (294-324), stop confirm (326-384), completion
celebration (389-447). Wires: `useScanProgress`/`useScanState`/`useConsentBridge`/pause/resume/
cancel (66-73); 1 s elapsed ticker (107-111); completion side-effect syncs store status (93-101).
Phase label from `scanPhaseOf(stage)` (83, contract.ts:171-175) — `deep-walk|carve` →
"Advanced Scanning" (i18n/en.ts:155).

**ResultsRoute + ResultsExplorer** (`features/results-explorer/results-route.tsx`,
`results-explorer.tsx`, `/results/:scanId`): route guard loads scan state; `cancelled` →
PartialBanner with rescan CTA (results-route.tsx:45-59,62-102). Explorer: toolbar (home/back/up,
breadcrumb with count, details/icons Segmented, Filter button, search Input, 203-347), left tree
with path/type Segmented (354-383), center virtualized list w/ 6-item context menu (387-510),
right PreviewPane on active row (514-528) with drag resizer (615-650). Bottom bar (non-live):
"Scan Completed, Files Found: N (size)", Save Session (+cancel save 579-589, onError toast
571-573), Recover CTA (536-604). States: rows loading/empty-filtered/empty (390-413), infinite
pages via `fetchNextPage` (451). Wires: `useResultsPages`/`useResultTree`/`useCountResults`
(95-97), `useSaveSession`/`useCancelSave` (78-79), `useConnectorMode` (80; mock disables
"Open location" 477-484). Smart-select wand = pure client ranking over loaded rows (166-188).

**RecoveryPage** (`features/recovery-queue/recovery-page.tsx`, `/recover/:scanId?all|ids`):
destination dialog card. Local/Cloud tabs (206-232; Cloud is a stub body 353-357), mount radio
rows w/ overlap & low-space warnings (235-352), Choose-folder row (320-351), advanced path +
collision radios (359-410), validation Alert (418-444), file-repair Switch (446-460), start
button (462-495), insufficient-space modal (501-548). Then per-job: RecoveryQueue with
QueueProgress (external-store progress, 637-677), item list w/ state badges + reasonKey tooltip
(592-635, 804-843), cancel w/ confirm (553-590), QueueReport success/partial/failed + JSON
export via Blob download (679-802). Wires: `useMountPoints`, `useStartRecovery`, `useValidateDestination`
(neededBytes estimated from scanState avg file size, 75-82), `usePickDirectory` (exclude source,
143-155), `useRecoveryJob/Items/Progress/Report/Cancel`.

**BackupPage** (`features/backup/backup-page.tsx`, `/backup` — "Scan Snapshot"): session list
(load → results, resume → live scan route with setActive, 25-53), feature blurb + CTA to Home.
Wires: `useSessions`/`useLoadSession`/`useResumeSession`. The resume fix (v0.8.2, 41-52) keeps
`/scan/<targetId>/live?scanId=` correct here.

**LostPartitionPage** (`features/lost-partition/lost-partition-page.tsx`, `/lost-partition`):
disk picker (279-351) → whole-disk scan (onStartSearch 92-103) → candidate table from
`scanState.lostCandidates.rows` (77-78; LBA/sectors/size×512/FS map 44-56) → Apply/Undo/SaveUndo
(233-273). Consent feed: `useApplyConsentBridge` + `useApplyConsentPending` poll (74-90).
Toast-verbatim outcomes (105-139).

**RepairPage** (`features/repair/repair-page.tsx`, `/repair`): **split brain** — decorative
dropzone/queue driven by hardcoded `SAMPLE_REPAIR_FILES` (35-54) with a fake `handleRepairAll`
that just flips local state + toast (121-125); the real surface lives in an "Advanced volume
repair" panel using `useRepairDryRun/useApplyRepair/useUndoLog/useUndoRepairs` (71-74, 448-565)
with a type-REPAIR consent gate (509-530).

**SettingsPage** (`features/settings-about/settings-page.tsx`, `/settings`): theme picker
(7 modes incl. ember/tide/blossom/HC, 24-42,114), scale via `document.documentElement.zoom`
(69-83), density, language, user-signs import (file input → content mutation, 237-275),
diagnostics copy-log (171-183), About table w/ connector-mode badge (186-229). Wires:
`useTheme`, `useAppInfo`, `useLoadUserSigns`, `getLogBuffer/exportLogText` (lib/logger).

**Dialogs**: AboutDialog (settings-about/about-dialog.tsx:30-49 — version/connector/engine ABI/
platform only). ActivationDialog (activation-dialog.tsx) — clipboard paste w/ **sample-key
fallback** (54) and `activation.providerUnavailable` status (63): **no activation backend exists**
(queries/system.ts has no activation surface). ConsentModal (consent/consent-modal.tsx) — global,
non-dismissable (75-81), renders repair plan rows (93-108) or apply-plan rows (110-126), deny/
approve gated on refused/actionCount (130-146); answers via `consent_respond` (48-65).
UnlockVolumeDialog (device-selection/unlock-volume-dialog.tsx) — password→dictionary unlock
(37-53); **FVEK field is dead UI**: state captured (35) but never sent — `beginScan` never passes
`bitlockerFvek` either (home-page.tsx:90-91), though the bridge supports it (scan.rs:169-184).
ElevationBanner (consent/elevation-banner.tsx) — see §7. DevPanel/PerfOverlay
(dev-panel/dev-panel.tsx): scenario/seed/controls, `qc.invalidateQueries()` on scenario swap
(81), rAF FPS meter (159-206).

**PreviewPane** (`features/preview/preview-pane.tsx`): thumbnail card + metadata stack + modal
zoom; per-kind payloads (image/hex→worker/text/pdf/av/office/archive) rendered by subcomponents;
failures surface as reason-diagnostics (uses `useNodeDetail`/`useThumbnail`/`usePreview`,
46-48). **FilterBar** (`results-explorer/filter-bar.tsx`): pure filter-store controls (search,
categories, date/size presets, extensions, Existing-only, Hide-0KB) — no queries of its own.
**PageFallback** (results-explorer/page-fallback.tsx:8-22): EmptyState → Home.

## 3. Connector layer

`connector/contract.ts` is the single source of truth (contract.ts:1-13). Key shapes:
`Device` (88-102: id/kind/model/label/serial/capacity/status/volumes/canScanWholeDisk),
`Volume` (104-120: letter, fs, quickScanAvailable, bitlocker tri-state, sectorSize),
`ScanParams` (177-190: targetId, pathScope, bitlockerFvek), `ScanProgressEvent` (192-213:
status/stage/stageProgress/overallProgress/bytes*/filesFound/dataFoundBytes/errors/badSectorMap/
eta/throughput/recoverableDuringScan/deviceConnected/currentPath), `ScanState` (220-242:
+ pendingConsent, lostCandidates, autosaved), `ConsentRequestEvent` (245-257),
`ResultNode`/`TreeNode`/`ResultsFilter` (299-352), `PreviewPayload` union (394-448),
`RecoveryProgressEvent` (537-550), `DiskHealth` (609-620), `ApplyConsentEvent` (676-691),
`SystemDomain.getSecurityPosture/relaunchElevated` (753-786). Domain methods: Devices 130-147,
Scan 272-289, Results 374-388, Preview 450-463, Sessions 481-500, Recovery 576-591, Health
622-624, Repair 661-669, LostPartition 703-721. Error model: `ConnectorErrorCode` (56-82) +
`reasonKey` i18n suffix (49-54).

Backend choice: `connector/index.ts:23-26` — `VITE_CONNECTOR==='mock'` else **tauri default**
(shipped builds never carry the simulator, index.ts:4-11). `isTauriRuntime()`
(lib/runtime.ts:18-20) gates shell-only affordances (window chrome), not the connector.
Mock-only extras: `__rwInjectConsent` e2e hook (index.ts:50-65), `scenarioControl()` (71-80).

Real adapter `connector/tauri/tauri-connector.ts`: every method a literal `invoke('<cmd>')`:
`devices_list/refresh`, `volume_unlock_dictionary` (189-204); `scan_start/pause/resume/cancel/
state`, `consent_respond` (208-259); `results_tree/query/count`, `node_detail` (261-281);
`preview_open/thumbnail` (283-300); `session_save`, `scan_save_cancel`, `sessions_list`,
`session_load/meta/resume_last/resume` (302-322); `recovery_start/cancel/state/items/report`
(324-358); `health_read` (360-363); `repair_dry_run/apply/undo_log/undo` (365-381);
`lost_partition_apply/undo/save_undo`, `apply_consent_pending`, `consent_respond` (383-420);
`system_app_info/security_posture/relaunch_elevated/load_user_signs/validate_destination/
pick_directory/list_mounts/open_path` (422-451). Event subscriptions: `scan-progress:<scanId>`
(233), `consent-request:<scanId>` (249), `recovery-progress:<jobId>` (348),
`apply-consent-request:<deviceId>` (407) — each `listen()` with unlisten-on-unsubscribe
(236-239 etc.). Errors: bridgeCode→code, else numeric engine table `ENGINE_CODE_MAP`
(92-132; ERR_ELEVATION_REQUIRED 0x500C0001→'elevation-required' at 131), rendered via
`REASON_KEYS` (62-89) — the UI never string-matches (toConnectorError 134-153).

Mock (`connector/mock/mock-connector.ts`, 1916 lines): deterministic fixtures per scenario
(`fixtures.ts:4-12`), stage chain with weights volume-analysis .04/quick .14/deep .42/carve .24/
tree .16 (mock-connector.ts:143-157), 55-130 ms `setInterval` ticks (452,474), files/data found
grown from the procedural node space; `getSecurityPosture` = `scenario!=='permission-denied'`
(1748-1754), `relaunchElevated` honest not-implemented (1756-1767), `openPath` mock refusal
(1882-1891). Mock synthesizes `dataFoundBytes` live; **the real bridge does not** (see §8).

## 4. Queries & stores (lifecycle · risks)

`queries/client.ts`: shared QueryClient, retry 1, staleTime 5 s, gc 5 min, no refetch-on-focus
(4-16); central `queryKeys` (18-42).

- **devices.ts** — `useDevices` (list), `useRefreshDevices` (mutation writes cache 17),
  `useUnlockVolume` (invalidates devices). No polling — stale until manual refresh; device
  arrival/disappearance mid-session is only seen after refresh (risk: stale device list).
- **scan.ts** — the event pump. `getScanStore` (22-84): one external store per scanId; subscribes
  `onScanProgress`, primes from `getScanState` with **silently-swallowed failure**
  (`.catch(() => undefined)` scan.ts:68); invalidates scanState on status change and
  tree/results/count on every 10 % bucket (37-56); **unsubscribes + deletes the store only when
  the last listener leaves** (75-79) — correct cleanup. `useScanState` polls 500 ms while
  running/starting/stopping, hard-stops otherwise (105-121; 'stopping' fix documented 106-118).
  `useScanProgress` via `useSyncExternalStore` (129-142). Mutations pause/resume/cancel
  invalidate state+sessions (148-175). `useConsentBridge` (183-199): event + `pendingConsent`
  re-hydration — unmount-safe (`return connector...` cleanup 188-191).
- **results.ts** — tree (staleTime 10 s), infinite pages (RESULTS_PAGE_SIZE 200, 41-55), count
  (5 s), nodeDetail, thumbnail (staleTime Infinity, 76-83). Live-mode refresh comes ONLY from
  the scan-store bucket invalidations; if `overallProgress` stalls, result views go stale too.
- **sessions.ts** — list/load/save/cancelSave/resume/resumeLast mutations; loadSession
  invalidates `['scan', scanId]` prefix (25). No polling.
- **recovery.ts** — job-store mirror of scan.ts (22-61, 86-93): `onRecoveryProgress`
  subscription, invalidations on status change, unsubscribe on last listener (52-56). Items page
  fixed 0..500 (103-109). Report query enabled whenever jobId set — fine.
- **health.ts** — `useHealth(deviceId)`, staleTime 60 s, retry false (5-13). One-shot; no
  polling, refresh is the manual button.
- **system.ts** — appInfo (Infinity), mounts (30 s), validateDestination/pickDirectory
  mutations, `useSecurityPosture` (staleTime 60 s, retry **false**, 42-51), `useRelaunchElevated`
  mutation, `useLoadUserSigns`. **Risk:** posture `isError → banner renders null**
  (elevation-banner.tsx:21) — a failed probe silently hides the elevation affordance entirely.
- **preview.ts** — requestPreview query, retry false (terminal diagnostics) (7-15).
- **repair.ts** — dryRun/undoLog queries + apply/undo mutations (invalidates undo log).
- **lost-partition.ts** — apply/undo/save mutations; `useApplyConsentPending` **polls every
  4 s** while the page holds a deviceId (41-50) — stops on unmount via react-query key change;
  `useApplyConsentBridge` event subscription with cleanup (54-62).
- **dev.ts** — connectorMode from appInfo; scenarioControl passthrough (mock-only).

Stores (`stores/`): **scan-session-store** (active/known scans + celebrationSeenFor;
`setActiveStatus` clears active on terminal states, 38-47) — in-memory only, lost on reload
(fallback = `?scanId=` param). **selection-store** (ids/all+exclusions tri-state model,
`selectedCountOf` 85-92; `allFilter` snapshot must be captured at select-all time — stale if
filters change afterwards). **ui-store** (viewMode, previewPaneWidth 240-560 clamp, sidebar
persisted `rw.ui.sidebarCollapsed`, dev flags; 41-75). **filter-store** (+sort store +
`resolveFilter()` 199-216; date/size preset math 71-121; `hasActiveFilters` 161-172) — global,
**never reset on scan change** (only manual reset/clear) — filters bleed across sessions (risk).
**consent-store** (repair/apply events + clear-by-requestId, 24-33).

## 5. OS / process touches (UI + shell side)

- `system_relaunch_elevated` — `ui/src-tauri/src/commands/system.rs:85-124`:
  `ShellExecuteW(null,"runas",<own exe>,…,SW_SHOWNORMAL)` (95-102) then `app.exit(0)` (113).
  This is the ONLY elevation path; triggered from ElevationBanner (elevation-banner.tsx:36-41).
- `system_pick_directory` — system.rs:369-388, `tauri_plugin_dialog` `blocking_pick_folder()`
  on the blocking pool. Plugin registered lib.rs:30.
- `system_open_path` — system.rs:391-412, `tauri_plugin_opener` `open_path` (dirs) /
  `reveal_item_in_dir` (files). Plugin registered lib.rs:31. Reached from results context
  menu (currently disabled in mock — results-explorer.tsx:477-484) and QueueReport export is
  pure webview Blob download (recovery-page.tsx:785-791).
- `system_validate_destination` — system.rs:294-338: `GetDiskFreeSpaceExW` (218-239) +
  **real write probe** creating/deleting `.rw-probe-<pid>` (323-326).
- Window controls — `getCurrentWindow().toggleMaximize` (shell.tsx:126) and
  `components/ui/window-controls.tsx` (min/max/close via `@tauri-apps/api/window`, runtime-gated).
- Device/volume probes — devices.rs:130 `cr_ui_open_disk`, 151 `cr_ui_open_volume` (fresh open
  probe per device on every list), GetDiskFreeSpaceExW per letter (54-75).
- Health — commands/health.rs:35,64 `cr_ui_health_read` → engine `cr_health.cpp:306`
  `_popen("fsutil behavior query disabledeletenotify 2>nul","r")` — **console child process;
  the reported popup when opening Disk Health originates here** (engine-side; no CREATE_NO_WINDOW
  / creation_flags anywhere in ui/src-tauri — grep clean).
- Threads (Rust, not processes): scan run thread scan.rs:362, emitter events.rs:250 & 418,
  recovery process recovery.rs:318, logging.rs:233. No `Command::new`/`powershell`/`cmd`/`wmic`
  in the Rust shell (grep clean). Webview OS touch: `navigator.clipboard`
  (activation-dialog.tsx:43), `document.documentElement.zoom` (settings-page.tsx:71).

## 6. Visual identity

- **Layer stack** (styles/index.css:1-15): adopted raw tokens (`src/theme/adopted/tokens.css`,
  immutable, DiskGenie palettes) → app semantic mapping (`src/styles/tokens.css`) → Tailwind v4
  `@theme inline` (index.css:18-86; **no tailwind.config file — CSS-first v4**). Dark variant is
  attribute-based `@custom-variant dark ([data-scheme='dark'] *)` (index.css:15).
- **Palettes** (adopted/tokens.css): light `:root` — white/`#f5f5f7` panels, text `#1d1d1f`,
  brand ink coral `#ff6b4a` (51), status `#ff3b30/#34c759/#f59e0b` (60-63,104); dark family +
  theme blocks `dark|ember|tide|blossom` (themes.ts:33-52, two-layer `data-theme`+`data-scheme`).
  App semantic layer: `--accent #b83410` (AA-darkened coral, tokens.css:52), **blue CTA gradient
  `#1f7dff→#1677ff→#0066ff`** (tokens.css:60-62), dark lifts (148-169), blossom (170-179),
  derived high-contrast `.hc` (185-278).
- **Chrome**: `WindowControls` swaps 24 SVG glyph sets (light/dark) — corpus-derived **interim**
  assets, `src/assets/interim/chrome/manifest.json:1-231`, imports window-controls.tsx:7-31.
- **Off-token / hardcoded color inventory** (grep `#[0-9A-F]{6}`): health-page.tsx (71 hits,
  incl. status pills `#DCFCE7/#FEF3C7/#FEE2E2`, ring `#10B981/#F59E0B/#EF4444` 255-261),
  brand-icons.tsx (56), repair-page.tsx (42), home-page.tsx (42 — incl. CTA `bg-[#1677FF]`
  hover `#0958D9` at 361, 491), sd-card-page.tsx (40), recovery-page.tsx (37), scan-progress-page.tsx
  (31 — mixed blues: `#1677FF` at 183/221/276 vs `#0066FF/#0052CC` at 377/429/437), results-tree,
  preview-pane, backup, lost-partition, activation-dialog, virtual-file-list, progress, button,
  window-controls, checkbox. Shell chrome bakes the ice-blue identity (`#EBF2FF` shell.tsx:130,
  `#D8E5FA` 135, `#DCE8FF`/`#1677FF` active rail 427) with `dark:` fallbacks only — **custom
  light-family palettes (blossom) and ember/tide never re-skin these; only system light/dark
  look right** (systemic theme risk).
- **The off-theme "restart as administrator" surface** = ElevationBanner:
  elevation-banner.tsx:27 `bg-amber-50 border-amber-300 text-amber-900` (+amber-400/100 button
  34, dark ambers 27) — raw Tailwind amber, not `--warning`/`warning-muted` tokens (tokens.css:71-73);
  label `elevation.relaunch` = 'Restart as administrator' (i18n/en.ts:488). Also amber:
  consent-modal.tsx:85 `text-amber-500` icon (off-token vs `text-warning`).

## 7. Localization & error surfaces

- i18n: `i18n/index.ts` (localStorage `rw.ui.locale`, navigator zh detection, fallback en);
  26 namespaces in en.ts:4-559, mirrored zh-CN.ts:6-542 — **full parity incl. elevation (472),
  unlock, consent, lostPartition, devPanel**; dynamic keys verified present
  (`repair.collisionRename/Overwrite/Skip` en.ts:330-332, `results.context.openLocationMock`
  245, `scan.quickScanning/advancedScanning/filesFoundWithSize/readingSector` 153-157).
- Error surfaces: (a) sonner toasts — every feature (`toast.error(...)` e.g. home-page.tsx:98,
  scan-progress-page.tsx:315, consent-modal.tsx:61); (b) route ErrorBoundary → designed
  ErrorScreen (app/error-boundary.tsx:37-75, logs via logger 21-26); (c) inline EmptyState
  errors (home 160-178, health 215-222, repair 450-455); (d) non-dismissable Dialogs for
  consent/device-lost; (e) reasonKey → `t(reasonKey)` translation ONLY in home beginScan
  (home-page.tsx:97-98) and QueueReport/QueueStateBadge (recovery-page.tsx:743,837) — **most
  call sites surface raw `err.message` (English engine text)**: sd-card-page.tsx:45,
  health-page.tsx:63, lost-partition 100/119/136/263, backup 28/51, elevation-banner 39.
- Silent swallowing: queries/scan.ts:68 (state-prime catch → undefined); ContinueLastBanner
  returns empty fragment on error (home-page.tsx:703 — intended fallback); **ElevationBanner
  returns null on posture.isError (elevation-banner.tsx:21) — a failed probe is
  indistinguishable from "elevated"** (defect-1 relevance); mock `unlockVolume` refusal is
  surfaced, but the engine's per-letter `bitlocker` probe failures degrade to status strings
  without any toast (devices.rs:159-165).

## 8. Wiring risks mapped to the four reported defects

1. **No elevation request at launch.** Banner visibility = `useSecurityPosture().data.
   administrator===false` (elevation-banner.tsx:21-22). Hidden when: query fails (retry:false,
   system.ts:49), or the app IS elevated, or VITE_CONNECTOR=mock with a non-permission-denied
   scenario (mock-connector.ts:1753). There is **no launch-time UAC** by design — the product
   is user-mode (system.rs:3-9); the only elevation is the banner's ShellExecuteW "runas"
   (system.rs:95). If the banner never shows on an unelevated machine, suspect the posture
   query error-null path or the dev/browser mock default.
2. **Console popups on Disk Health.** UI path HealthPage → `useHealth` → `health_read`
   (health.rs:19) → engine `_popen("fsutil …")` (cr_health.cpp:306) — an unsuppressed console
   child process on the engine side; the Rust shell itself spawns nothing (§5). HealthPage
   additionally **fabricates a 88-100 % "good" score client-side** (health-page.tsx:100-108).
3. **Only 1 of 3 disks.** Home filters `kind==='internal'` (home-page.tsx:80-83) and the bridge
   collapses every non-removable/ removable disk to `internal|usb` from one `removable` flag
   (devices.rs:215, devices.rs:382 — BF-02: no bus_type). Any disk the engine enumerates as
   removable (USB-attached NVMe enclosures, SD via reader) vanishes from Home (SD page only
   shows kind `sd|usb` — which the bridge **never emits** as 'sd', so sd-card-page.tsx:30
   falls back to `devices[0]`). Engine-side `enumerate_disks` (util.rs:156-169 → cr_ui) is the
   other half to check.
4. **Scan stuck at 75 % "Advanced Scanning", Files Found 21 (0 B), sector counter frozen.**
   Real-bridge truth: `overallProgress` only advances on `CR_EVT_SCAN_PROGRESS` and is clamped
   monotonic (events.rs:266-272); `stage` becomes `deep-walk|carve` from `CR_EVT_SCAN_STAGE`
   (events.rs:256-264) → phase 'advanced' (contract.ts:171-175). **`dataFoundBytes` is never
   updated while running**: `live.data_found_bytes` starts 0 (state.rs:136) and is only set in
   the run-thread tail after `cr_ui_scan_run` returns (scan.rs:419-443) — so "N (0 B)" is the
   *expected* live rendering of the real bridge, and a stalled run (no engine progress events)
   freezes gauge + sector counter + result invalidations together (bucket invalidation
   queries/scan.ts:51-56 never fires when overall stops). The mock behaves differently
   (computes dataFound live, mock-connector.ts:143-157), masking this in dev. `bytesTotal`
   comes from geometry (scan.rs:212-216) — sector line divides by hard-coded 512
   (scan-progress-page.tsx:124-125).
   Related risks: sidebar live-scan link drops `?scanId=` (shell.tsx:282); selection/filter
   stores persist across scans; `currentPath` ticker permanently null (events.rs:193).

— End of map. Cross-check engine-side truth in `engine/worklog/ACCESS-LAYER-CURRENT.md` and
bridge contracts at repo root `contracts/` (command-surface.json / error-codes.json).
