# UI-MAP — Phase 0 Audit, Agent B (read-only)

Repo: `/home/z/my-project/final-prod`. UI = React 19 + Tauri 2, frontend `ui/src`, Rust shell `ui/src-tauri`.
All citations `file:line` relative to repo root unless noted. Rust bridge/engine citations are included where the UI rendering chain terminates there.

---

## Screens

Router: `ui/src/app/router.tsx:22-45` — `createHashRouter`, `WizardShell` (`ui/src/app/shell.tsx:102`) wraps all routes inside `ErrorBoundary` (`ui/src/app/error-boundary.tsx:14`). Shell chrome: custom title bar + 3-group sidebar + `ElevationBanner` above `<Outlet/>` (`shell.tsx:350`), global `ConsentModal`/`AboutDialog`/`ActivationDialog` (`shell.tsx:357-359`), `DevPanel`+`PerfOverlay` only under `import.meta.env.DEV` (`shell.tsx:356`).

| Route | Feature dir | Component |
|---|---|---|
| `/` | `features/device-selection` | `home-page.tsx:57` |
| `/sd-card` | `features/device-selection` | `sd-card-page.tsx:20` |
| `/tools`, `/health` | `features/health` | `health-page.tsx:39` (both routes map to the same page, router.tsx:33,38) |
| `/backup` | `features/backup` | `backup-page.tsx:17` |
| `/scan/:deviceId/live` | `features/scan-progress` | `scan-progress-page.tsx:50` |
| `/results/:scanId` | `features/results-explorer` | `results-route.tsx:22` → `results-explorer.tsx:63` |
| `/recover/:scanId` | `features/recovery-queue` | `recovery-page.tsx:58` |
| `/repair` | `features/repair` | `repair-page.tsx` |
| `/settings` | `features/settings-about` | `settings-page.tsx` |
| `/lost-partition` | `features/lost-partition` | `lost-partition-page.tsx` |
| `*` | `features/results-explorer` | `page-fallback.tsx:8` |

### Home / device list (`features/device-selection/home-page.tsx`)
- States: loading skeletons (`:147-158`); device-list error EmptyState (`:160-178`); no-physical-devices EmptyState (`:180-192`); permission-denied banner (`:229-234`); disconnected banner (`:235-240`); success = internal drive cards + quick targets + sessions.
- **Device visibility filter**: `physical = kind !== 'virtual'` (`:80`), `internal = kind === 'internal'` (`:81`), `visible = showRemovable ? removable : internal` (`:83`) where `showRemovable = params.has('media')` (`:77`). **Nothing in the app ever navigates to `/?media`** (grep: zero matches; the SD-card page's "show all external devices" toggle is local state only, `sd-card-page.tsx:26,176`). So Home lists *only* `kind === 'internal'` rows. The bridge classifies every removable disk as `'usb'` (`ui/src-tauri/src/commands/devices.rs:215` `kind: removable ? "usb" : "internal"`). **"Only 1 of 3 disks shown" is explained at the UI layer whenever 2 of 3 disks enumerate as removable** (or by engine-side enumeration loss — Agent C's sheet). Note the 'locked' volume still renders with an unlock dialog (`unlock-volume-dialog.tsx:23`).
- Ops: `listDevices` (`devices_list`), `refreshDevices`, `startScan`, `pickDirectory` (`:66`), `listSessions`, `loadSession`, `resumeLastSession` (`:64,19`). Quick targets hardcode mock-era volume ids `vol-recycle-bin`/`vol-desktop` (`:54-55`) — the bridge resolves them via implicit pathScope (`devices.rs:252-258`).
- Sessions list shows `filesFound` count (`:310`); `ContinueLastBanner` renders when sessions exist (`:292`).

### Disk health (`features/health/health-page.tsx`)
- Device picker via `usePhysicalDevices` (`hooks/use-physical-devices.ts:11-37`, `?device=` param, default first physical).
- States: loading (`:69-84`), empty (`:86-98`), health loading (`:210-214`), health error EmptyState showing the raw bridge `error.message` — English, untranslated (`:215-222`), success = gauge + KPI tiles + SMART table.
- Health score is **synthesized UI-side**: `status==='good' ? max(88, 100−wearPercent) : caution→72 : 38` (`:101-106`) — the bridge truthfully refuses to invent a percentage (`commands/health.rs:9-10,142`) but the UI invents one anyway.
- Ops: `getHealth` → `health_read` (`commands/health.rs:19`), `listDevices`, `startScan` (`:56-66`).
- **The console-popup symptom**: health reads trigger the engine's TRIM probe `cr_health.cpp:306` `_popen("fsutil behavior query disabledeletenotify 2>nul","r")` — `_popen` from a GUI-subsystem process flashes a **visible cmd.exe console** per call. (Other engine subprocesses use `CREATE_NO_WINDOW`: `cr_fsck.cpp:50-51`, `cr_bitlocker.cpp:63-64`.) See "OS/subprocess touches".

### Scan live progress (`features/scan-progress/scan-progress-page.tsx`)
- Full state machine below in "The scan-progress chain". States: no scanId → `ResultsPageFallback` (`:120-122`); starting/running/paused/stopping (footer cluster); completed → celebration modal (`:389-447`); deviceLost (paused + `deviceConnected===false`) → non-dismissable reconnect modal (`:294-324`); stop-confirm dialog (`:327-384`).
- Renders `ResultsExplorer` in `liveMode` (browse-while-scan, `:139-143`).
- The "Searching: <path>" ticker (`:148-157`) is **dead on the real backend**: the bridge never fabricates `currentPath` (`bridge/events.rs:191-193` G-2 comment — always `null`), so the strip never renders outside the mock.

### Scan results (`features/results-explorer/results-route.tsx:22`)
- Guard: `getScanState` fails/missing → `ResultsPageFallback` (`:41-43`); `status === 'cancelled'` → partial-results banner with rescan CTA (`:45-54,62-102`).
- `results-explorer.tsx:63` — 3-pane (tree / virtualized list / preview); ops: `getResultTree`, `queryResults` (infinite, 200/page, `queries/results.ts:11,41-55`), `countResults`, `getNodeDetail`, `requestPreview`, `getThumbnail`, `saveSession`/`cancelSave`.
- Dead surface: context-menu "Open location" item has **no onClick** (`results-explorer.tsx:476-484`) — the `openPath` contract op (`contract.ts:785`) is never invoked by any component; its enable-gate `connectorMode === 'mock'` reads `data?.connectorMode` which the real `system_app_info` never returns (`commands/system.rs:131-147` — no `connectorMode` key) so the value is stuck at `'loading'` (`queries/dev.ts:12`).

### Recovery (`features/recovery-queue/recovery-page.tsx:58`)
- Destination card + validation + live queue. Ops: `getScanState`, `listMountPoints`, `startRecovery`, `getRecoveryState/Items/Report`, `onRecoveryProgress`, `cancelRecovery`, `validateDestination`, `pickDirectory` (`:84-90,114,555,594,639,683`).
- **Downstream casualty of the 0 B gap**: `avgFileBytes = dataFoundBytes/filesFound` else 2,400,000 (`:75-78`) — during a *live* scan `dataFoundBytes` is always 0 (see chain), so `neededBytes` collapses to 1 (`:79-82`) and the out-of-space gate can never trip from live-scan state.
- Hardcoded English placeholder `"D:\\Recovered Files"` (`:382`).

### Sessions / backup (`features/backup/backup-page.tsx:17`)
- Ops: `listSessions`, `loadSession`, `resumeSession` (scan resume with targetId handoff `:47-49`). States: loading skeletons (`:123-127`), empty (`:128-140`), success list.

### Settings (`features/settings-about/settings-page.tsx`)
- Theme/density controls (`:63`), diagnostics log copy (in-memory buffer, `lib/logger.ts`), user-signs import via hidden `<input type=file>` → `loadUserSigns` **passes the file NAME, not content** (`:253` `loadUserSigns.mutate(file.name…)`) while the bridge's `system_load_user_signs` expects the document **text** (`commands/system.rs:198-210` — the comment explicitly says "the webview's file input hands over the DOCUMENT text"). The import is therefore broken end-to-end (reads a path as JSON).
- "Data source" badge (`:204-217`): on the real backend `appInfo.data?.connectorMode` is `undefined` → badge renders **"tauri" with success styling regardless** (`'mock' ? warning : success`), and the About dialog's mirrored logic (`about-dialog.tsx:35-39`) renders **"mock" with warning styling** because it tests `=== 'tauri'` on the same undefined field. Two contradictory indicators of the same missing field.

### Repair (`features/repair/repair-page.tsx`)
- Device picker + dry-run card + apply/undo trio (`:69-74`). Ops: `repair_dry_run`, `repair_apply`, `repair_undo_log`, `repair_undo`. States: loading (`:408-413`), dry-run loading/error/success (`:448-452`), no-findings EmptyState (`:536`).

### Lost partition (`features/lost-partition/lost-partition-page.tsx`)
- Ops: `listDevices`, `startScan`, `applyLostPartition`, `undoLastApply`, `saveApplyUndo`, `apply_consent_pending` (4 s poll, `queries/lost-partition.ts:48`), `onApplyConsent` bridge (`:87`), `getScanState` (`:77`).
- Live candidate rows come from the scan event stream (`ScanState.lostCandidates`, `contract.ts:236-242`; bridge join in `events.rs:287-313`).

### Banner/dialog surfaces (global)
- `ElevationBanner` — `features/consent/elevation-banner.tsx:16` (see dedicated section).
- `ConsentModal` — `features/consent/consent-modal.tsx:30`: non-dismissable (Escape/outside prevented `:79-81`), both consent families, "Decline/Approve". Uses `text-amber-500` raw Tailwind (`:85`) — another off-token amber.
- `ActivationDialog` (`settings-about/activation-dialog.tsx`) + orange "Buy Now" pill (`shell.tsx:150-157`).
- Toasts: `sonner` via `components/ui/toaster.tsx`.

---

## State & query map

**Stores (`ui/src/stores`, zustand):**
- `scan-session-store.ts:20` — active/known scans, celebration-seen flag. Sidebar "scanning…" link keyed off `active` (`shell.tsx:279-299`).
- `consent-store.ts:24` — global pending repair/apply consent events (navigation-safe).
- `selection-store.ts` — results selection (all/ids/exclusions).
- `filter-store.ts` — results filter/sort.
- `ui-store.ts:41` — view mode, preview width, sidebar collapse (persisted `rw.ui.sidebarCollapsed`).

**QueryClient defaults** (`queries/client.ts:4-16`): retry 1, staleTime 5 s, gcTime 5 min, **`refetchOnWindowFocus: false`**.

**Queries (`ui/src/queries`)** — keys in `client.ts:18-42`:
| Query | Hook | Poll/refresh |
|---|---|---|
| devices | `devices.ts:5` | none (staleTime 5 s default) |
| scanState | `scan.ts:100-123` | **500 ms poll while running/starting/stopping** (`:118-120`); the 'stopping' poll is load-bearing (v0.8.0 forensic comment `:107-117`) |
| scanProgress | `scan.ts:129-142` | **event stream, no polling** — `useSyncExternalStore` over `connector.scan.onScanProgress` (store per scanId, `scan.ts:22-84`) |
| tree/results/count | `results.ts:21,34,58` | invalidated by the progress store every 10 % bucket (`scan.ts:37,51-56`) and on terminal status (`scan.ts:41-50`) |
| preview/thumbnail | `preview.ts:7`, `results.ts:76` | staleTime 60 s / ∞ |
| sessions | `sessions.ts:5` | default |
| job/jobItems/report | `recovery.ts:95,103,122` | event-driven invalidation (`recovery.ts:34-45`) |
| health | `health.ts:5` | staleTime 60 s, `retry: false` |
| securityPosture | `system.ts:42-51` | staleTime 60 s, `retry: false`; comment claims refetch-on-focus keeps it truthful but `client.ts:10` disables focus refetches — **the comment is wrong; posture never refreshes after the first read** |
| appInfo | `system.ts:5` | staleTime ∞ |
| mounts | `system.ts:13` | staleTime 30 s |
| repairDryRun/undoLog | `repair.ts:5,25` | default |
| apply-consent pending | `lost-partition.ts:41-50` | 4 s poll |

**Stale/divergence notes:**
- The scan-progress external store is ref-counted and torn down when the last subscriber leaves (`scan.ts:75-79`); progress events that fire while nobody is subscribed are lost, but `getScanState` priming (`scan.ts:60-68`) re-seeds one snapshot on remount.
- `useScanState`'s 500 ms poll coexists with the event store; terminal transitions invalidate tree/results/count/sessions (`scan.ts:41-50`).
- Device rows carry a fresh open-probe status each `devices_list` (`commands/devices.rs:128-134`) — an unelevated launch marks raw disks `permission-denied`, which feeds Home's amber banner (`home-page.tsx:229-234`) but does **not** hide the disks.
- The found-files counters rendered during a live scan come exclusively from the event/telemetry path (below); they are exact engine counters, not UI-estimated.

---

## The scan-progress chain (75 % / "Files Found: 21 (0 B)")

**Contract types**: `ScanProgressEvent` `ui/src/connector/contract.ts:192-213` — `stage: ScanStage` (`:165-166`), `stageProgress`, `overallProgress` ("weighted across stages; monotonic", `:198-199`), `bytesScanned/bytesTotal`, `filesFound`, `dataFoundBytes`, `etaSeconds`, `currentPath`.

**Full chain, engine → pixel:**

1. **Engine** computes the overall permille as `weighted_progress(w, quick_pm, deep_pm, carve_pm)` = `(40·quick + 35·deep + 25·carve)/100` (`engine/src/core/orchestrator/cr_scan.cpp:131-146`; weights 40/35/25 set by the bridge's zero-init scan config, `ui/src-tauri/src/commands/scan.rs:5-6` doc + `cr_scan.cpp` stage weights). Stage permilles are clamped monotonic (`cr_scan.cpp:494-520` `tel_publish_`, `:464-482` `tel_stage_end_`). **When quick=1000 and deep=1000 and carve=0, overall is exactly 750 permille = 75 %** — the instant deep-walk ends, overall locks at 75 % until carve starts publishing. A stuck-at-75 % display therefore means the engine's carve stage is not advancing its permille (or the scan genuinely sits at the deep→carve boundary) — the UI merely renders it.
2. **Bridge** consumes `CR_EVT_SCAN_PROGRESS` (`ui/src-tauri/src/bridge/events.rs:266-272`): `live.overall = evt.overall_permille/1000`, clamped monotonic (`:269-270`); `CR_EVT_SCAN_STAGE` maps engine stage ids `0→quick-index, 1→deep-walk, 2→carve` (`:256-262`); `CR_EVT_FILES_FOUND` sets `live.files_found = files+deleted+carved` (`:274-277`); telemetry snapshot overwrites counters at emit time (`:357-377`, `bytes_scanned` at `:360`, files at `:361`, throughput/ETA at `:362-376`).
   **`live.data_found_bytes` is NEVER updated during the scan** — it is zero-initialized (`bridge/state.rs:136`) and only filled when `run()` returns, by walking the frozen snapshot's metas (`commands/scan.rs:416-443`; grep for `data_found` shows no other writer). The ABI event struct carries no found-bytes field (`bridge/abi.rs:125-150`). **So every live progress event says `dataFoundBytes: 0` → the UI renders "(0 B)" for the whole scan.** (The engine itself tracks `stats.bytes_found` internally, `cr_scan.cpp: tel_counts_` — it just never crosses the ABI during the run.)
3. **Compose/emit**: `compose_scan_progress` (`events.rs:155-195`) emits `scan-progress:{scanId}` (`events.rs:234,398`) with `"overallProgress": overall` (`:180`), `"stage"` (`:178`), `"bytesScanned"` (`:181`), `"filesFound"` (`:183`), `"dataFoundBytes"` (`:184`), `"currentPath": null` always (`:193`).
4. **TS connector**: `ui/src/connector/tauri/tauri-connector.ts:229-239` — `listen('scan-progress:${scanId}')`.
5. **React layer**: `queries/scan.ts:22-84` external store; `useScanProgress` (`:129-142`).
6. **Render** (`features/scan-progress/scan-progress-page.tsx`):
   - Percentage driver: `const overall = progress?.overallProgress ?? 0` (`:80`) → `ProgressGauge value={overall}` (`:161-166`, circle + text at `components/ui/progress.tsx:50-108`, hardcodes `#1677FF`/`#D0E2FF`) **and** the label `t('scan.advancedScanning', { percent: formatPercent(overall) })` (`:179-181`; `formatPercent` at `lib/format.ts:127-131`; string `'Advanced Scanning {{percent}}'` at `i18n/en.ts:155`, zh `'高级扫描 {{percent}}'` `zh-CN.ts:153`).
   - Stage-label driver: `const phase = scanPhaseOf(progress?.stage ?? 'volume-analysis')` (`:83`) — `scanPhaseOf` (`contract.ts:171-175`): `deep-walk|carve → 'advanced'`. So "Advanced Scanning 75 %" requires `stage ∈ {deep-walk, carve}` and `overallProgress = 0.75`.
   - **"Files Found: 21 (0 B)"**: `t('scan.filesFoundWithSize', { count: progress?.filesFound ?? 0, size: formatBytes(progress?.dataFoundBytes ?? 0) })` (`:183-188`; string at `en.ts:153`). `filesFound=21` is genuine engine counter data (files+deleted+carved, `events.rs:275/361`); `(0 B)` is the bridge gap in step 2. `formatBytes(0)` → `"0 B"` (`lib/format.ts:13-31`).
   - **Sector progress**: `sectorsScanned = floor(bytesScanned/512)`, `sectorsTotal = floor(bytesTotal/512)` (`:124-125`) — **hardcoded 512** despite `Volume.sectorSize` existing in the contract (`contract.ts:116`); rendered as "Reading sector: X/Y" (`:258-262`, `en.ts:157`). "No sector progress" = `tel.bytes_scanned` not advancing in the engine telemetry (bridge merely forwards, `events.rs:360`).
   - ETA "Remaining time" (`:246-252`), pause/resume/stop (`:203-241`), used-time on completion (`:264-268`).
7. **Terminal correction**: when `run()` returns, `commands/scan.rs:428-443` latches exact `files_found`/`data_found_bytes` into `Terminal` + `live`, and the final emit (`:446-453`) plus `compose_scan_progress`'s terminal branch (`events.rs:166-169`) snap `overall=1.0`, `stage=done`, exact counters — the numbers only become truthful at scan end. `getScanState` during the live scan returns the same 0-byte progress (`commands/scan.rs:610-618`).

**Mock behavior for contrast** (`connector/mock/mock-connector.ts`): stage weights 0.04/0.14/0.42/0.24/0.16 (`:143-149`), overall computed per tick (`:589-596`), found-file curve `floor(stats.files · (1−e^(−3.4·overall)))` (`:615-625`), dataFound follows the same curve — the mock **never** produces "21 (0 B)" nor a 75 % plateau.

---

## The elevation prompt surface

**Component**: `ui/src/features/consent/elevation-banner.tsx:16-47`, mounted once in the shell above every page's `<Outlet/>` (`app/shell.tsx:350`).

**Visibility logic** (`elevation-banner.tsx:21-22`): render only when the posture query has data AND `posture.data.administrator === false`. `isLoading || isError || !data → null`. Because `useSecurityPosture` sets `retry: false` (`queries/system.ts:49`), **any invoke failure silently suppresses the banner** — no error surface exists for a failed posture probe.

**Unelevated decision chain**: `connector.system.getSecurityPosture()` (`queries/system.ts:44-45`) → `invoke('system_security_posture')` (`tauri-connector.ts:425-427`) → Rust `commands/system.rs:70-79` → Win32 TokenElevation probe `is_elevated()` (`system.rs:37-60`, OpenProcessToken/GetTokenInformation/TokenElevation). Registered at `lib.rs:84`. Non-Windows → always false (`system.rs:62-68`).

**Relaunch action**: `useRelaunchElevated` (`queries/system.ts:53-57`) → `invoke('system_relaunch_elevated')` (`tauri-connector.ts:428`) → `commands/system.rs:84-124`: `ShellExecuteW(hwnd=null, "runas", <own exe>, null, null, SW_SHOWNORMAL)` (`:88-103`); rc ≤ 32 → bridge error `DEVICE_PERMISSION` "the elevation request was refused…" (`:105-111`); on success `app.exit(0)` (`:113`). The only OS-visible subprocess from this flow is the standard UAC consent prompt (by design).

**"No elevation request at launch" context**: the app exe has **no requireAdministrator manifest** anywhere in the repo (grep over `installer/`, `src-tauri` finds only NSIS comments; the NSIS *installer* is `RequestExecutionLevel admin`, `installer/recoverywizard.nsi:151`, but the installed Tauri app exe itself is asInvoker). The design is user-mode + banner + manual relaunch (`system.rs:1-9` doc); the banner is the only launch-time elevation signal, and it depends entirely on the fragile query above. (The portable lane is explicitly "no install — run as administrator", `build.ps1:216`.)

**Styling — the "ugly/off-theme" root cause** (`elevation-banner.tsx:25-45`):
- Banner div: `border-b border-amber-300 bg-amber-50 px-4 py-2 text-[13px] text-amber-900` + dark `amber-500/40`, `amber-500/10`, `amber-200` (`:27`).
- Button: `variant="secondary"` + overrides `border-amber-400 bg-transparent text-amber-900 hover:bg-amber-100 dark:text-amber-200 dark:hover:bg-amber-500/20` (`:31-34`).
- These are **raw Tailwind amber palette classes** — they bypass the app's entire semantic token system. The token layer defines the sanctioned warning surfaces: `--warning/--warning-vivid/--warning-muted` (`styles/tokens.css:71-73`, dark `:157`, HC `:212-214,263-265`) and Tailwind maps them to `bg-warning-muted`/`text-warning` utilities (`styles/index.css:41-43`). Every other banner in the app uses them (Home permission banner `home-page.tsx:230`; disconnected `:236`; partial-results `results-route.tsx:75`). The amber literals also clash with both identity palettes in play (see Theming) and with `#EBF2FF` ice-blue window frame (`shell.tsx:130`). Same disease in `consent-modal.tsx:85` (`text-amber-500`).

---

## OS/subprocess touches from the UI

Frontend (webview) side — only two Tauri API import sites beyond the connector: `getCurrentWindow` in `shell.tsx:4` (title-bar drag/double-click maximize `:123-127`) and `window-controls.tsx:2` (min/max/close on the custom chrome; hidden in browser runtime via `lib/runtime.ts:18`). No `@tauri-apps/plugin-*` imports in `ui/src` — plugins are Rust-side only (`lib.rs:30-31` dialog + opener).

Rust shell/bridge (spawn/execute inventory):
| Site | What | Visible window? |
|---|---|---|
| `commands/system.rs:88-103` | `ShellExecuteW "runas"` own exe (elevation relaunch) | Standard UAC prompt only (intended) |
| `commands/system.rs:368-388` | `system_pick_directory` → `tauri_plugin_dialog` blocking folder picker | Native dialog (intended) |
| `commands/system.rs:390-412` | `system_open_path` → `tauri_plugin_opener` (`open_path` dirs / `reveal_item_in_dir` files) | Explorer window — **currently unreachable from the UI** (dead context item, `results-explorer.tsx:476-484`) |
| `bridge/events.rs:250,418`, `commands/scan.rs:362`, `commands/recovery.rs:318`, `bridge/logging.rs:233` | plain `std::thread::spawn` emitter/run/log threads | none |
| engine `cr_fsck.cpp:50-51` `live_spawn` (chkdsk/fsck) | `CreateProcessW` with **`CREATE_NO_WINDOW`** | no |
| engine `cr_bitlocker.cpp:63-64` `run_command` (manage-bde / powershell Unlock/Disable-BitLocker, `:358,376`) | `CreateProcessW` with **`CREATE_NO_WINDOW`** | no |
| engine `cr_health.cpp:306` `query_trim_state` | **`_popen("fsutil behavior query disabledeletenotify 2>nul","r")`** | **YES — `_popen` from a GUI-subsystem parent (main.rs:2 hides the app's own console only in release) spawns a visible cmd.exe console per health read. This is the "console popups when opening disk health" symptom.** |

Also note `commands/system.rs:321-334`: destination validation writes+deletes a real probe file `.rw-probe-<pid>` in the target dir (not a subprocess, but a filesystem touch from a UI-initiated command).

---

## Theming system + bypasses

**Architecture** (ADR-007, 3 layers):
1. Adopted raw palettes: `ui/src/theme/adopted/tokens.css` (immutable) — `:root` light = Apple-system neutrals (`#ffffff/#f5f5f7`, text `#1d1d1f/#6e6e73`) with **coral ink `#ff6b4a`** brand + `--cta-gradient` blue override; `[data-scheme=dark]` graphite; `[data-theme=ember|tide|blossom]` custom families (`adopted/themes.ts:33,46-52`).
2. Semantic mapping: `ui/src/styles/tokens.css:26-139` — surfaces/text/status/focus/radius/motion + a11y-darkened text grades (`--accent:#b83410`, `--warning:#a84c07`, `--danger:#d70015`), CTA gradient `#1f7dff→#0066ff` (`:60-62`), dark-family lifts (`:148-169`), derived `.hc` high-contrast layer (`:185-278`), density (`:281-286`).
3. Tailwind v4 `@theme inline` mapping: `ui/src/styles/index.css:18-86` (`--color-primary: var(--accent)` etc.), dark variant keyed off `[data-scheme='dark']` (`:15`).
ThemeProvider: `app/theme.tsx:100-155` — modes system/light/dark/ember/tide/blossom/high-contrast, persisted `rw.ui.theme` (note: adopted module's own `diskgenie.theme` key `themes.ts:78` is unused by the app).

**The product's *actual* visual identity is a second, hardcoded palette** — the EaseUS-style chrome baked inline: window frame `bg-[#EBF2FF]` (`shell.tsx:130`), primary blue `#1677FF` (+hover `#0958D9`, pressed `#003EB3`), borders `#DCE8FF/#C8DAF5/#E2E8F0`, text `#1E293B/#334155/#64748B/#7A8BA8`, orange buy pill `#FF7A2F→#FF4D00` (`shell.tsx:153`), plus alternate blues `#0066FF/#0052CC` in scan dialogs (`scan-progress-page.tsx:377,437`). `ProgressGauge` hardcodes `#1677FF/#D0E2FF` (`components/ui/progress.tsx:72,80`).

**Bypass inventory (theme-token violations):**
- `elevation-banner.tsx:27,34` — raw `amber-*` (the user's complaint; worst offender, see above).
- `consent-modal.tsx:85` — `text-amber-500` icon (raw amber).
- Hundreds of inline hex utilities across pages (`shell.tsx`, `home-page.tsx`, `health-page.tsx`, `backup-page.tsx`, `sd-card-page.tsx`, `scan-progress-page.tsx:150,160,183,276,360,377,427,437` …) — they bypass `--accent`/`--warning`/etc., so switching to ember/tide/blossom changes only the mapped utilities, not the hardcoded chrome: the "premium, theme-consistent" bar is structurally unmet for any non-default palette.
- SVG illustration literals (`scan-progress-page.tsx:453-527`, `sd-card-page.tsx:205-232`) — off-token decoration colors.

---

## Localization

- Two locales: `en`, `zh-CN` (`i18n/index.ts:5,17-20`); detection = stored `rw.ui.locale` → `navigator.language` zh-prefix → en (`:11-22`); language switcher in the title-bar dropdown (`shell.tsx:183-190`); `document.documentElement.lang` maintained (`:38,42`).
- Practically complete translation coverage in the two dictionaries (en.ts 534+ keys, zh-CN.ts mirrored).
- **User-visible untranslated English:**
  - All bridge/engine error **messages** rendered raw: health error EmptyState (`health-page.tsx:219`), scan start toasts (`home-page.tsx:98` renders `t(reasonKey)` when present but `err.message` otherwise), consent/toast errors across pages. The Rust messages are English literals (`commands/system.rs:108-110,402`; `devices.rs:27,499`; …).
  - `recovery-page.tsx:382` placeholder `"D:\\Recovered Files"`.
  - `shell.tsx:61` `ariaLabel: 'Hardware Disk (Internal Drives)'` hardcoded English.
  - About/settings "Data source" badges mislabel on the real backend (see Settings section).
- Formatting is locale-aware via Intl (`lib/format.ts`) with tabular numerals enforced (`styles/index.css:110-117`).

---

## Mock-vs-real connector selection

- Factory: `ui/src/connector/index.ts:23-37` — `VITE_CONNECTOR === 'mock'` (exact string) → `MockConnector`; **anything else, including undefined, → the real Tauri connector**. Default is tauri (`index.ts:4-11` doc; flipped at P4).
- Mock entry points: `pnpm dev` script pins `VITE_CONNECTOR=mock` (`ui/package.json:12`); vitest pins it via `vite.config.ts:67-69` (`test.env`); `ui-e2e.yml:48-50` builds with `env: VITE_CONNECTOR: mock`.
- **Shipped builds do NOT set it**: `build.ps1` ui lane → `pnpm tauri build` (`build.ps1:165`) → `tauri.conf.json` `beforeBuildCommand: "pnpm build"` (`ui/src-tauri/tauri.conf.json`) → plain `vite build` (`package.json:13`) with no env → tauri. `release.yml` runs `./build.ps1 -Target all` with only `CR_UI_LIB_DIR` manipulation (release.yml:62-66) — no `VITE_CONNECTOR` anywhere in the release lane. `ui-ci.yml:91` `pnpm build` — also unpinned → tauri. No `.env` files exist under `ui/`.
- Mock fixtures contain **no "21"** as a files-found value and no 75 % plateau: the only near-miss is `21_000` fileCount for the USB device (`connector/mock/fixtures.ts:563`) and a "~21 s" quick-pass comment (`mock-connector.ts:141`); the mock's live `dataFound` is a nonzero saturating curve (`mock-connector.ts:622-625`) and its overall spans 0→1 across 5 weighted stages (`:143-149,589-596`). **The user's observed "21 (0 B)" at 75 % cannot come from mock leakage — it is real-bridge data** (engine counters + the `dataFoundBytes` live-gap + the engine's 40/35/25 weighted overall, see the chain).
- Mock-only side effects guarded: `window.__rwInjectConsent` e2e hook exists only when the mock backend is active (`index.ts:50-65`); dev-panel scenario controls return null on the real connector (`index.ts:71-80`).

---

### Cross-cutting defects surfaced by this map (candidate work items)
1. Live `dataFoundBytes` is always 0 (bridge gap, no ABI event field) → "(0 B)" during the entire scan + corrupted recovery space-estimate (`recovery-page.tsx:75-82`).
2. Engine weighted overall mathematically parks at exactly 75 % between deep-walk completion and carve progress (cr_scan.cpp:131-146) → "stuck at 75 %, Advanced Scanning".
3. Sector counter hardcodes 512-byte sectors (`scan-progress-page.tsx:124-125`).
4. "Searching: <path>" ticker can never render on the real backend (`events.rs:193`).
5. `_popen` fsutil in health path flashes consoles (engine `cr_health.cpp:306`).
6. Home page's `?media` removable view is unreachable; internal-only filter hides removable disks.
7. Elevation banner + consent icon use raw amber Tailwind classes, bypassing tokens; broad hardcoded-blue chrome undermines the multi-palette token system.
8. `connectorMode` missing from real `system_app_info` → About shows "mock" warning badge, Settings shows "tauri" success badge (both wrong), context-menu "Open location" gate stuck at 'loading' and the item has no onClick (dead `openPath` op).
9. Posture query `retry:false` + no focus refetch → a transient invoke failure permanently hides the elevation banner for the session.
10. Settings user-signs import passes `file.name` instead of file contents (`settings-page.tsx:253` vs `commands/system.rs:198-210`).
