# UI-MAP — Phase 0 surface map, Agent B (full refresh, session T1)

Repo: `/home/z/my-project/work/final-prod` @ `8f3bedd1` (main; v0.9.2 train + the
unreleased `eb4bb5f3` D-ENUM-02/D-NVMe-01 engine fix). UI = React 19 + react-router
7 (hash) + TanStack Query + zustand, in `ui/src`; Tauri 2 shell + cr_ui bridge in
`ui/src-tauri`; C++ engine behind the `cr_ui` C ABI in `engine/`.
All citations `file:line` relative to repo root. Read-only audit — no file was
modified except this one.

**Framing fact for every defect below:** the user's installed product is the
v0.9.2 release; `eb4bb5f3` (the D-ENUM-02 + D-NVMe-01 engine fixes) landed
*after* the v0.9.2 tag (git order: `1ef7aa7c` v0.9.2 → `12d824a3` → `eb4bb5f3`
→ `0c80036f` → `8f3bedd1`). Anything fixed only in `eb4bb5f3` is NOT in the
user's build, and — critically — half of `eb4bb5f3` never reached the UI anyway
(see DEFECT TRACE #1/#2: the Rust bridge was never updated to consume the new
ABI fields).

---

## 1. UI tree inventory (every file under `ui/src`)

| Path | Role |
|---|---|
| `src/main.tsx` | Entry: StrictMode → Providers → App (`main.tsx:7-14`) |
| `src/app/router.tsx` | Hash router, route table `:22-45`; `App` `:47-49` |
| `src/app/shell.tsx` | WizardShell: custom title bar, Buy-Now pill `:150-157`, key/language/menu buttons `:159-227`, `WindowControls` `:227`, 3-group sidebar `:56-100` (Data Recovery / Advanced / File Backup), results-flow sidebar variant `:265-301`, global modals `ConsentModal`/`AboutDialog`/`ActivationDialog`/`ExitGuard` `:356-360`, DevPanel only in DEV `:355` |
| `src/app/providers.tsx` | ThemeProvider → QueryClientProvider → TooltipProvider → Toaster (`:21-28`); dev watchdog `:14-19` |
| `src/app/theme.tsx` | Theme context: mode `system/light/dark/ember/tide/blossom/high-contrast` (`:27`), localStorage `rw.ui.theme`/`rw.ui.density` (`:41-42`), `data-theme`+`data-scheme` attrs via `applyThemeAttributes` (`:89`), forced-colors tracking `:92-96` |
| `src/app/error-boundary.tsx` | Class ErrorBoundary `:14-34`; `ErrorScreen` (reason + Home/Retry, never a stack) `:37-75` |
| `src/app/exit-guard.tsx` | W-02 close-requested save guard: fresh `readScanState` on close `:62-76`, save-and-exit / keep / exit-without-saving dialog `:113-147` |
| `src/connector/contract.ts` | The 36-op / 9-domain connector contract + `scanPhaseOf` `:171-176`, `ScanProgressEvent` `:206-226`, Device/Volume/DiskHealth/RepairDryRun types |
| `src/connector/index.ts` | Connector factory: `VITE_CONNECTOR` unset/≠mock → **tauri (real)** (`:23-26`); mock only for `pnpm dev`, vitest (`vite.config.ts:67-69` pins `mock`), and the e2e lane (`.github/workflows/ui-e2e.yml` builds with `n: mock`) |
| `src/connector/tauri/tauri-connector.ts` | Real adapter: every op → `invoke('…')`; error map `REASON_KEYS` `:69-96`, engine numeric map `:99-139`, event names `scan-progress:{id}` `:240`, `scan-finished:{id}` `:254`, `save-progress:{id}` `:266`, `lost-candidate:{id}` `:278`, `scan-autosaved:{id}` `:290`, `consent-request:{id}` `:306`, `recovery-progress/finished:{id}` `:420/:432`, `session-load-progress:{path}` `:375`, `apply-consent-request:{id}` `:491` |
| `src/connector/mock/mock-connector.ts` | Test-double simulator (2,074 lines); stage weights 0.14/0.42/0.24 (`:149-153`) — **different from the real engine's 40/35/25**, so the mock never reproduces the "Advanced Scanning 75%" presentation |
| `src/connector/mock/fixtures.ts` | Mock devices (`dev-sys`, `dev-sdcard` kind `'sd'` `:393-394`, virtual targets `:331/:357`) |
| `src/connector/mock/node-space.ts`, `rng.ts` | Mock filesystem / seeded RNG |
| `src/features/device-selection/home-page.tsx` | Home / disks page (see §3) |
| `src/features/device-selection/sd-card-page.tsx` | SD-card page (see §3 — hard-coded card row) |
| `src/features/device-selection/unlock-volume-dialog.tsx` | N-064 BitLocker dictionary unlock dialog |
| `src/features/health/health-page.tsx` | Disk Health & S.M.A.R.T. surface (see §4) |
| `src/features/scan-progress/scan-progress-page.tsx` | Live scan surface (see §5) |
| `src/features/results-explorer/results-route.tsx` | `/results/:scanId` guard: fallback `:26/:41-43`, partial-results banner for cancelled scans `:45-59`, rescan CTA `:62-102` |
| `src/features/results-explorer/results-explorer.tsx` | 3-pane explorer (tree / virtualized list / preview), toolbar, 6-item context menu, smart-select `:158-183`, save-session with live fraction `:79-80`; used live during scan (`liveMode`) |
| `src/features/results-explorer/filter-bar.tsx` | Type/Date/Size filter popovers + Existing / Hide-0KB toggles |
| `src/features/results-explorer/page-fallback.tsx` | Not-found fallback → Home |
| `src/features/preview/preview-pane.tsx` | Preview pane: thumbnail, confidence badge, per-kind bounded render, hex/text in worker (`hex-worker-client`), expand modal `:49` |
| `src/features/recovery-queue/recovery-page.tsx` | Destination card (Local/Cloud tabs), source-drive overlap warning, space estimate from live avg (`:74-82`), options, live queue + report, recovery-finished bridge |
| `src/features/backup/backup-page.tsx` | Scan Snapshot page: sessions list, open (with load fraction), scan resume with targetId handoff `:51-69` |
| `src/features/repair/repair-page.tsx` | File Repair page: **mock dropzone/queue flow** (see §7) + the REAL `VolumeRepairWorkbench` (dry-run/apply/undo-log/undo, consent-gated) behind "Advanced Repair" |
| `src/features/lost-partition/lost-partition-page.tsx` | S-029 real-data surface: pick disk → whole-disk scan → live candidate rows from scan state `:77-83` → consent-gated apply/undo/undo-file |
| `src/features/settings-about/settings-page.tsx` | Theme/density/scale/locale, diagnostics log copy, user-signs import, About |
| `src/features/settings-about/about-dialog.tsx` | Version/engine/connector-mode rows |
| `src/features/settings-about/activation-dialog.tsx` | License modal (decorative — shows `activation.providerUnavailable`; paste fallback inserts fake key `DRW26-…` `:54`) |
| `src/features/consent/consent-modal.tsx` | Global non-dismissable consent modal (repair + lost-partition apply), mounted in shell |
| `src/features/dev-panel/dev-panel.tsx` | DEV-only scenario/seed/language/density panel + PerfOverlay; returns null when the real connector is active (`:50`) |
| `src/hooks/use-physical-devices.ts` | Picker hook: `?device=` search-param selection + first-device fallback (`:19-32`) |
| `src/queries/*.ts` | Query layer (the only legal connector consumer): `client.ts` (keys `:18-41`, staleTime 5 s default), `devices`, `scan` (progress external-store `:20-84`), `results`, `recovery`, `health`, `repair`, `lost-partition`, `preview`, `sessions`, `system`, `dev` |
| `src/stores/ui-store.ts` | zustand: viewMode/previewPaneWidth/sidebarCollapsed/devPanel, persisted `rw.ui.*` (`:8-10`) |
| `src/stores/scan-session-store.ts` | Active-scan registry + celebration-seen flag (memory only) |
| `src/stores/selection-store.ts` | Results selection (all/ids/exclusions + anchor) |
| `src/stores/filter-store.ts` | Results filter/sort/tree-mode state |
| `src/stores/consent-store.ts` | Pending repair/apply consent events for the global modal |
| `src/i18n/{index,en,zh-CN}.ts` | i18next, `rw.ui.locale` persistence, en + zh-CN |
| `src/lib/{format,hex-format,logger,runtime,utils}.ts` | Formatters, ring-buffer logger + watchdog, `isTauriRuntime` |
| `src/components/ui/*` | 24 primitives (button/dialog/table/results-tree/virtual-file-list/progress/window-controls/toaster…) |
| `src/theme/adopted/{themes,tokens}.css` | Two-layer token theme system (ADR-007) |
| `src/styles/{index,tokens}.css` | Token layer + base styles |
| `src/workers/hex.worker.ts` + `hex-worker-client.ts` | Off-main-thread hex/text preview |
| `src/assets/interim/chrome/*` | 4-state title-bar SVG glyphs (light/dark) |

Rust shell (`ui/src-tauri/src`): `lib.rs` (builder, command registry `:33-90`,
ABI gate log-only `:98-107`), `bridge/abi.rs` (the 92-export cr_ui binding),
`bridge/{state,events,error,util,consent,logging,filefilter}.rs`,
`commands/{devices,scan,results,preview,sessions,recovery,health,repair,lost_partition,system}.rs`.
`app.manifest` (`requireAdministrator` `:25`), wired through
`build.rs:45-47` (`WindowsAttributes::app_manifest`).

---

## 2. Screens, states, controls, data

| Route (router.tsx:31-42) | Screen | States | Key controls | Data requirements |
|---|---|---|---|---|
| `/` | Home / disks (`home-page.tsx:63`) | loading skeleton `:155-166`; devices error `:168-186`; no physical devices `:188-200`; permission/disconnected banners `:202-248`; success | refresh button `:225-234`, device cards (select/dbl-click scan) `:537-705`, volume chips `:676-702`, quick targets `:304-323`, folder dialog `:403-532`, "Search for Lost Data" `:391-399`, recent sessions `:325-355`, continue-last `:734-757` | `devices.listDevices`, `refreshDevices`, `startScan`, `pickDirectory`, `sessions.*` |
| `/sd-card` | SD card (`sd-card-page.tsx:20`) | (no loading/error states — the card row is static) | static card row `:78-111`, "show all external" `:114-144`, Scan Card `:191-198` | `devices.listDevices` (only for the hidden external list + scan target resolution) |
| `/tools`, `/health` | Disk health (`health-page.tsx:39`) | device loading `:69-84`; no devices `:86-98`; health loading `:210-214`; health error EmptyState `:215-222`; success (gauge + 4 KPI + SMART table) | disk radio cards `:138-208`, refresh `:123-134`, "Search for Lost Data" `:315-323` | `devices.listDevices`, `health.getHealth(deviceId)` |
| `/backup` | Scan snapshots (`backup-page.tsx:22`) | loading `:139-143`; empty `:144-156`; list | open (with live % `:190-193`), resume `:174-183`, start-search CTA `:125-131` | `sessions.list/load/resume`, load-progress event |
| `/scan/:deviceId/live` | Live scan (`scan-progress-page.tsx:53`) | no scanId → fallback `:127-129`; starting/running/paused/stopping (footer cluster `:166-313`); completed → celebration modal `:412-470`; deviceLost → reconnect modal `:317-347`; stop-confirm `:350-407` | pause/resume `:210-232`, stop `:233-248`, Recover-All CTA (gated on `recoverableDuringScan`) `:296-312` | `scan.getScanState` (500 ms poll `queries/scan.ts:105-121`), `scan-progress` event, `results.*` (liveMode), consent bridge |
| `/results/:scanId` | Results (`results-route.tsx:22` → `results-explorer.tsx:66`) | loading `:28-39`; missing → fallback `:41-43`; partial (cancelled) banner `:49-59`; success | tree/path-type-raw tabs, view toggle, filter bar, search, context menu, smart-select `:158-183`, save session `:79-80`, recover CTA | `results.getResultTree/queryResults/countResults/getNodeDetail`, `preview.*`, `sessions.save/cancelSave` |
| `/recover/:scanId` | Recovery (`recovery-page.tsx:60`) | destination card; validating; validation refused (overlap/unwritable/invalid); out-of-space modal; running queue; finished report | Local/Cloud tabs, drive radio rows, path input + browse, options (collision/folders/repair/verify), start/cancel | `system.listMountPoints/validateDestination/pickDirectory`, `recovery.*`, `scan.getScanState` |
| `/repair` | File repair (`repair-page.tsx:61`) | dropzone `:128-204`; queue `:206-…`; advanced workbench (real queries) | Add Files, Repair All (**local-only fake** `:121-124`), Advanced Repair toggle `:253-266` | `devices.listDevices`, `repair.dryRun/apply/undoLog/undo` (workbench only) |
| `/lost-partition` | Lost partition (`lost-partition-page.tsx:63`) | pick-disk; scanning (live candidates `:77-83`); has-candidates; apply consent | refresh, per-disk search, candidate select, Apply/Undo/Save-undo | `devices`, `scan.startScan/getScanState`, `lostPartition.*`, consent store |
| `/settings` | Settings (`settings-page.tsx:61`) | — | theme `:96-113`, scale `:115-…`, density, locale, log copy, user-signs import, About | `system.getAppInfo`, `system.loadUserSigns` |

Global surfaces: ErrorBoundary (any route), ConsentModal (shell `:356`),
AboutDialog/ActivationDialog (shell `:357-358`), ExitGuard (shell `:360`),
Toaster (top-right, token-themed `components/ui/toaster.tsx:13-34`),
ErrorScreen.

---

## 3. THE DISKS PAGE — full chain

**Mount → render chain**

1. `HomePage` mounts; `useDevices()` (`queries/devices.ts:5-10`) → query key
   `['devices']` → `connector.devices.listDevices()` (`tauri-connector.ts:196`)
   → `invoke('devices_list')`.
2. Rust `devices_list` (`commands/devices.rs:426-434`) → `spawn_blocking(list_devices)`
   (`:107-232`):
   - `enumerate_disks()` / `enumerate_volumes()` (`bridge/util.rs:156-184`) —
     two-call cr_ui protocol, `buf.truncate(cap)`.
   - Per disk: fresh open probe `cr_ui_open_disk` (`:129-134`) → status
     ok/permission-denied (`:84-90`); per-lettered-volume usage via
     `GetDiskFreeSpaceExW` (`:52-80`, unlettered contributes 0); BitLocker
     probe only when FVE detected (`:149-158`).
   - Row composition: `id = disk-{n}` (`:20-22`), `kind = removable ? "usb" :
     "internal"` (**`:215`** — the only kind axis), label from first lettered
     volume else model else `Hard Disk #N` (`:186-201`), capacity
     total/used/free (`:219-223`), volume rows (`:167-183`) with
     `quickScanAvailable: true` always (`:180`), `canScanWholeDisk: true`
     (`:228`).
3. Engine: `cr_ui_enumerate_disks` → `enumerate_disks_impl`
   (`engine/src/ui/cr_ui.cpp:1938`, copies `bus_type` at `:1865`) →
   `win::enumerate_disks` (`engine/src/win/cr_enumerate.cpp`) — numbered
   PhysicalDrive namespace + cfgmgr interface pass, merged; **never-drop fix
   (D3)**: a present-but-unopenable disk stays listed with `busy` + honest
   empties (`cr_enumerate.cpp:110-175`); `open_disk_for_query` 0-access retry
   (`:85-106`); bus/removable decode `cr_enumerate.h:107-119` — after
   `eb4bb5f3`: `removable = RemovableMedia(+10) || bus∈{7 USB,12 SD,13 MMC}`
     and `bus_type` captured at +28.
4. React: `devicesQuery.data` → `physical = kind !== 'virtual'` (`:83`),
   `internal = kind === 'internal'` (`:84`), `external = kind !== 'internal'`
   (`:90`), `visible = physical` (`:91`). Groups rendered: "Internal drives
   (N)" `:250-270` + "External drives (N)" when `external.length > 0`
   `:276-298`. Selection: local `selectedId`, effective default = first
   physical (`:93`).
5. Per-card rendered fields (`DeviceCard` `:537-705`): compact label `:579`,
   lost-partition badge `:630`, lock/disconnect icons `:631-650`, capacity
   bar (used/total, min 2%) `:653-672`, free/total GB text `:664-672`, volume
   chips (click = scan that volume) `:676-702`, unlock dialog for locked
   volumes `:596-604`.

**Filtering/dedup**: UI drops only `kind === 'virtual'` (never produced by the
real bridge). No dedup in UI or bridge. Engine merge dedups by disk number +
PnP path (`cr_enumerate.h:228-255` per the Phase-0 diagnosis). Refresh =
re-enumeration (engine has no cache — `cr_ui.h` refresh contract, "each call
RE-QUERIES the OS", `engine/include/cr/cr_ui.h:452-460`); the React query
cache is overwritten by `refreshDevices.onSuccess`
(`queries/devices.ts:16-19`). No race window: both `devices_list` and
`devices_refresh` run the identical `list_devices` (`devices.rs:426-444`).

**Where disks can get dropped / hidden (ranked)**

1. **(Historical, in the user's v0.9.2 build) The shifted bus table** — every
   SATA disk decoded `removable=1` → `kind:'usb'` → internal drives landed in
   the *External* group and "Internal drives (N)" under-counted; the page
   "looked incomplete". Fixed engine-side only in `eb4bb5f3`
   (`cr_enumerate.h:118-119`). **Not in the user's build.**
2. **(Current tree, unfixed) The bus_type ABI axis is dead at the bridge** —
   engine appends `bus_type` to `cr_ui_disk_info` (`cr_ui.h:422-436`) and
   stamps it (`cr_ui.cpp:1865`), but the Rust `CrUiDiskInfo`
   (`bridge/abi.rs:172-189`) has no such field and `devices.rs:215`/`:382`
   still derive kind solely from `removable`. Consequences: SD cards are
   `kind:'usb'` forever → the **/sd-card page's `kind === 'sd'` filter can
   never match** (`sd-card-page.tsx:30-33`) and the kind axis the fix was
   sold on never reaches the UI. (Struct sizes happen to stay equal —
   engine 304 B, Rust 300→304 B padded — so no corruption, just an unread
   field.)
3. Engine never-drop discrimination (fixed v0.9.0, `cr_enumerate.cpp:110-175`)
   — pre-0.9.0 builds dropped present-but-unopenable disks; not in the user's
   build.
4. E-01 volume-buffer wholesale failure at 16+ letters (fixed v0.9.1,
   `62fb7670`).
5. UI-side swallow: none — errors surface as the EmptyState `:168-186`; a
   partial engine list cannot be produced by the bridge (one `check()` fails
   the whole call).

---

## 4. THE SMART/HEALTH SURFACE — full chain

**Selection propagation**: `usePhysicalDevices` keeps the selected disk in the
URL search param `?device=` (`hooks/use-physical-devices.ts:19-32`;
`setParams(..., { replace: true })`), falling back to the first physical
device. `HealthPage` feeds `deviceId` into `useHealth(deviceId || null)`
(`health-page.tsx:51`). **The health query IS keyed on the selected disk**:
`queryKey: ['health', deviceId]` (`queries/health.ts:7`), `staleTime 60 s`,
`retry: false`. Switching disks = new key = fresh fetch; no shared cache, no
stale-selection path. The KPI header, however, uses
`selectedDevice = list.find(...) ?? list[0]` (`:52`) — cosmetic fallback only.

**Per-disk query chain**: `connector.health.getHealth(deviceId)`
(`tauri-connector.ts:445-447`) → `invoke('health_read', { deviceId })` → Rust
`health_read` (`commands/health.rs:19-157`): parse `disk-N` (`:21-29`) →
two-call `cr_ui_health_read` (`:32-67`) → availability gate "both sources
absent → HEALTH_UNAVAILABLE" (`:54-59`) → status grading (`:91-102`, no
invented percentage) → attribute rows with `threshold: null` on the base
export (`:119-135`) → JSON. Engine `health_read_impl`
(`engine/src/ui/cr_ui.cpp:4888-4995`): opens `\\.\PhysicalDriveN` RW
(`:4896-4900`), BF-28 elevation gate (`:4903-4916`), `read_smart` (ATA
passthrough → SMART_RCV_DRIVE_DATA → CSMI; `engine/src/win/cr_health.cpp:216+`),
`read_nvme_health` (`:4954`), per-device TRIM via
`query_trim_state_handle` (no subprocess; `:4980`), both-absent →
`ERR_HEALTH_UNAVAILABLE` (`:4987-4990`). Stateless per call — no handle cache,
no first-disk stickiness in engine or bridge.

**UI render**: synthesized circular gauge score
`good ? max(88, 100−(wearPercent ?? 4)) : caution 72 : 38` (`health-page.tsx:100-106`);
SMART table `:458-534` (threshold column renders `—` because the bridge nulls
it, `health.rs:127-131`); error state = EmptyState "Health data is not
available for this device" + the raw English bridge message
(`:215-222`, i18n `en.ts:402`).

**Where "first disk works, others show NOTHING" comes from (ranked)**

1. **D-NVMe-01 (in the user's v0.9.2 build, unfixed there)**:
   `read_nvme_health`'s query buffer was `#pragma pack(push,1)`-ed, placing
   `STORAGE_PROTOCOL_SPECIFIC_DATA` at offset 9 instead of the storage
   stack's expected offset 8 → every real NVMe controller refused →
   `nvme_available = 0`. An NVMe-only disk with no ATA table then trips the
   both-absent gate (`health.rs:54-59`) → error EmptyState = "nothing".
   Disk 0 (SATA, ATA passthrough) works. Fixed only in `eb4bb5f3`
   (`engine/src/win/cr_health.cpp`, offset-8 + static_asserted layout).
   Invisible on CI seeds (Azure VHDX has no NVMe passthrough). **This exactly
   matches "only the FIRST disk shows data".**
2. **(Current tree, NEW memory-safety hazard) ABI size drift on
   `cr_ui_smart_report`**: `eb4bb5f3` appended `uint64_t power_cycles_nvme`
   (`cr_ui.h:1569` → engine struct 240 B) but the Rust
   `CrUiSmartReport` (`abi.rs:646-681`) still ends at `data_bytes_written`
   (232 B). The engine's `health_read_impl` does
   `std::memset(rep, 0, sizeof(*rep))` (`cr_ui.cpp:4892`) → **8-byte stack
   overwrite past the Rust buffer on EVERY health_read** once the bridge is
   linked against the `eb4bb5f3` engine. Unpredictable local corruption
   (`needed`, `n`, vec state) → exactly the class of "works once then
   nothing/weird" symptom. Must be fixed before the next build.
3. **(Current tree, cosmetic gap) threshold column dead**: even post-ex2, the
   bridge calls base `cr_ui_health_read` only (`abi.rs:939-940` declares no
   `cr_ui_health_read_ex2`; `health.rs:34-35,127-131` nulls thresholds), so
   `eb4bb5f3`'s BF-03 threshold datum never reaches the SMART table.
4. UI-side stale-selection/staleTime: ruled out (per-device keys, §4 above).
5. Honest unavailability (USB bridges rejecting ATA passthrough) — real
   possibility on the SD card via a reader; renders the same EmptyState.

---

## 5. THE SCAN SURFACE — full chain

**Click → start**: Home "Search for Lost Data" (`home-page.tsx:391-399`) or a
card double-click/volume chip → `beginScan(targetId)` `:97-110` →
`useStartScan` (`queries/scan.ts:88-98`) → `connector.scan.startScan` →
`invoke('scan_start', …)` → Rust `scan_start` (`commands/scan.rs:91-110`) →
`scan_start_blocking` (`:116-332`): ABI gate `:123-130` → `resolve_target`
(`devices.rs:246-321`; virtual quick-targets mapped to system volume +
implicit scope `:252-259`) → scope normalization `:46-80` → open
(plain/FVEK; FVE-locked refusal `:191-197`; elevation refusal `:198-205`) →
zero-init config (stage_mask 7, **weights 40/35/25**, autosave dir =
sessions store) `:218-241` → `scan_create`/`attach` `:243-252` → event
channel (bounded 512) + sink `:254-268` → registry `:321-325` →
`launch_run_thread` `:339-460` (run thread + emitter thread). On success the
UI navigates to `/scan/{targetId}/live?scanId={id}` (`home-page.tsx:102`).

**Progress events → UI**: engine emits through the trampoline
(`engine/src/ui/cr_ui.cpp:1064-1107`, field-for-field incl. `bytes_found`
D4b `:1085`) → Rust sink `try_send` (`events.rs:45-56`) → emitter thread
(`events.rs:241-411`): `CR_EVT_SCAN_STAGE` → stage label `:256-267`;
`CR_EVT_SCAN_PROGRESS` → monotonic overall + live `data_found_bytes`
`:266-275`; `CR_EVT_FILES_FOUND` `:281-287`; error ranges `:288-295`; lost
candidates `:299-330`; heartbeat (liveness only, never advances permille)
`:337-…`; save progress `:341-348`; autosaved `:357-…`. On every emit a
telemetry poll refreshes `bytes_scanned` / `files_found` / `bytes_found` /
throughput / ETA (`:353-396`) and emits `scan-progress:{scanId}` with the
contract payload (`compose_scan_progress` `:146-197`). Terminal: run thread
latches `Terminal{files_found, data_found_bytes}` (exact snapshot re-sum,
`scan.rs:416-443`), emits final progress + `scan-finished:{scanId}`
`:446-458`.

**UI consumption**: `useScanProgress` external store
(`queries/scan.ts:20-84`) + `useScanState` 500 ms poll as fallback
(`:100-123`) + `useScanFinishedBridge` instant-completion invalidation
(`:219-231`). Display (`scan-progress-page.tsx`): circular gauge of
`overallProgress` `:168-173`; status line
`Quick/Advanced Scanning {percent} , Files Found: {n} ({size})` `:181-195`
(i18n `en.ts:161-163`; phase from `scanPhaseOf` — deep-walk AND carve both →
"Advanced", `contract.ts:171-176`); pause/resume/stop `:210-248`;
remaining-time + **"Reading sector: X/Y"** `:253-269` where
`sectorsScanned = bytesScanned/512` (**hard-coded 512**, `:131-132`);
autosave stamp `:272-285`; "Searching: <path>" ticker `:155-164` — **dead on
the real backend** (`currentPath` always null, `events.rs:193`, G-2);
Recover-All CTA disabled until completed or `recoverableDuringScan`
(`:296-312`); celebration modal `:412-470`; results tree live (liveMode +
lost-candidate bridge, `queries/scan.ts:266-287`).

**Engine truth axes (current tree)**: live carve bytes (D4a,
`engine/src/core/orchestrator/cr_scan.cpp:510-518` — **only stage 2** stamps
`bytes_scanned` live), live found-size (D4b, `refresh_live_counts_`
`:576-597` + `emit_event_` `:609`), 16 MiB bytes-axis event trigger
(`emit_progress_event_` `:637-668`), intra-window cancel checkpoints (D4c,
`engine/src/core/carve/cr_carve.cpp:622-627`), deep remnant tick (D4e,
v0.9.1), lock-free user-sign probe (D4f, v0.9.2).

**Where "75% Advanced Scanning, 21 files (0 B), no sector progress, stuck"
comes from (ranked)**

1. **The 75% number is exact math, by design**: engine weights 40/35/25;
   quick+deep complete ⇒ overall = 0.750 exactly, and the UI labels the
   carve "Advanced Scanning" (`contract.ts:175`) — a healthy scan *shows*
   75% at the carve boundary (documented divergence D-P6,
   `docs/PARITY-DELTAS.md:55`). The defect is the freeze after it.
2. **(User's v0.9.2 build) The 0 B size axis**: D4b (live `bytes_found`
   through the ABI + bridge fill `events.rs:276/284/375-376`) shipped in
   v0.9.0 — but the user's report predates/dates from v0.8.4 originally; if
   their installed build is v0.8.4, `dataFoundBytes` was pinned 0 until
   completion (the old `state.rs` G-3 latch). Verify the installed version
   before re-diagnosing.
3. **"No sector progress" persists even in the current tree**:
   `tel_publish_` stamps live `bytes_scanned` **only for stage 2 (carve)**
   (`cr_scan.cpp:510-518`); `stats.bytes_scanned` is only ever incremented
   by the carve (`:2704`) and boundary stamps copy `stats` (`:545`). So
   "Reading sector: 0/N" through the whole quick+deep phase (0→75%), then
   moves only during the carve — at the validation-bound carve rate this
   reads as "stuck in sector scanning" on multi-TB disks. The bytes-axis
   D4a fix deliberately covered the carve only.
4. **Stall classes fixed but possibly incomplete**: D4e (655 s deep remnant
   freeze, fixed v0.9.1), D4f (605 s zero-region user-sign freeze, fixed
   v0.9.2). If the user's machine runs v0.9.0, both are present; v0.9.1
   carries D4e only; v0.9.2 carries both. **Any residual stall on v0.9.2 is
   a NEW class** — the heartbeat proves liveness without progress
   (`events.rs:337-…`, engine `cr_ui.cpp:1142-1204`), so the forensic
   io-timeline is the discriminator.
5. **Files-found counter**: `filesFound` = quick+deep+carved cumulative from
   telemetry (`events.rs:361-363`); "21" was the real engine counter on the
   user's earlier run (DIAGNOSIS.md:109). The live tree (browse-while-scan)
   requires the lost-candidate/tree invalidations (`queries/scan.ts:51-56`)
   — present.

---

## 6. ELEVATION SURFACES + SUBPROCESS CENSUS

**Privilege model (current tree)**: launch-time only. `app.manifest:25`
`requireAdministrator`, embedded via `build.rs:45-47`; NO runtime posture
probe, NO relaunch command, NO banner (`commands/system.rs:4-13`;
`contract.ts:826-832`; `queries/system.ts:41-43`). The historical banner
(`ui/src/features/consent/elevation-banner.tsx`, amber hard-coded → re-themed
in `c6ff7c10` → **deleted in `68460a4c`**) no longer exists. The only
"restart as administrator" text reachable today is the error toast
`errors.elevationRequired` (`i18n/en.ts:543-544`) shown when `scan_start`
fails with engine `0x500C0001` (`tauri-connector.ts:138`;
`home-page.tsx:104-107`). Note `Cargo.toml:29-33` still advertises
"TokenElevation posture, the relaunch-as-admin verb" windows-sys features —
a stale comment; no code uses them (grep: zero `ShellExecute`/`TokenElevation`
in `src-tauri/src`).

**Subprocess spawn sites (complete census)**

| Site | Trigger | Window posture |
|---|---|---|
| `engine/src/win/cr_health.cpp:357-429` `query_trim_state` (fsutil) | **NOT called by health reads anymore** — health uses the per-device `query_trim_state_handle` (`:431-459`, no subprocess; called from `cr_ui.cpp:4981`). The fsutil path remains for the `_ex` enumeration TRIM flag (`cr_ui.cpp:1851`), which the bridge never sets (`util.rs:158/165` plain calls) | Direct `CreateProcessW`, `CREATE_NO_WINDOW` + `SW_HIDE` + pipes (`:384-394`) — the D2 fix |
| `engine/src/win/cr_bitlocker.cpp:62-65` | BitLocker unlock/apply (PowerShell primary `:358/:376`, manage-bde fallback `:360/:381`) — only from the unlock dialog / apply flows | `CREATE_NO_WINDOW` |
| `engine/src/win/cr_fsck.cpp` (chkdsk launcher, M-075) | Not reachable from the UI (no `cr_ui_fsck_volume` consumer) | `CREATE_NO_WINDOW` per header |
| Rust shell (`ui/src-tauri/src`) | **None** — zero `Command::new`/`powershell`/`fsutil` anywhere | — |
| Frontend | None | — |

**Defect reading**: on v0.9.0+ the "console flash when opening disk health"
cannot come from the in-tree code (health reads spawn nothing; the remaining
spawns are all `CREATE_NO_WINDOW`). See DEFECT TRACE #5.

---

## 7. Hard-coded / mocked / synthesized value census (real-connector build)

| Surface | Value | Citation |
|---|---|---|
| SD-card page main card | "SD/MMC/MS PRO", "Others", "58 GB", "Disk2" — pure i18n strings, no device data; default selection id `'dev-sd'` (a mock id; the mock fixture is `dev-sdcard`) | `sd-card-page.tsx:27,:96-110`; `i18n/en.ts:131-134`; `fixtures.ts:393` |
| SD-card "Scan Card" fallback | `beginScan(sdDevice?.id ?? 'dev-sd')` → `device-gone` refusal on the real bridge | `sd-card-page.tsx:37-38` |
| Home "Select Folder" dialog | Hard-coded subfolder lists (`Users\Documents`…/`Projects`…) and `'C'` letter fallback — a mock-shaped browser, though `pickDirectory` (native) is what actually feeds the scan | `home-page.tsx:424-433,:510-513` |
| Repair page dropzone/queue | `SAMPLE_REPAIR_FILES` with fake user paths (`C:\Users\umar0\…`), fake save path, **"Repair All" flips local state to 'repaired' + success toast — no backend call** | `repair-page.tsx:35-54,:78-80,:121-124` |
| Repair "browse" button | `toast.info(savePath)` — dead button | `repair-page.tsx:343-350` |
| Activation dialog | Fake license key on paste failure; activate → `providerUnavailable` (honest dead end) | `activation-dialog.tsx:54,:63` |
| Health gauge score | Synthesized 88-100 / 72 / 38 from status+wear (UI invents the % the bridge refuses to) | `health-page.tsx:100-106` |
| Health KPI extras | `· 512B` sector text; wear fallback `'2%'`; temperature bar fallback 35/36 °C | `health-page.tsx:403,:434,:348-353` |
| Scan sector count | Hard-coded 512 B divisor (volume `sectorSize` exists in device data) | `scan-progress-page.tsx:131-132` |
| "Searching:" ticker | Never renders on the real backend (`currentPath` always null) | `events.rs:193`; `scan-progress-page.tsx:155` |
| SMART threshold column | Always `—` (bridge nulls it; ex2 never wired) | `health.rs:127-131`; `abi.rs:939-940` |
| Quick-target ids | `vol-recycle-bin` / `vol-desktop` — resolved bridge-side to system volume + implicit scope (real) | `home-page.tsx:60-61`; `devices.rs:252-259` |

**Which connector is active in the shipped build**: the REAL one. The factory
defaults to `tauri` when `VITE_CONNECTOR` is unset (`connector/index.ts:23-26`);
`build.ps1:165` runs `pnpm tauri build` → `beforeBuildCommand: pnpm build`
(`tauri.conf.json`) with no env override; no `.env` file exists. Mock is pinned
only for `pnpm dev` (`package.json:12`), vitest (`vite.config.ts:67-69`), and
the e2e lane (`.github/workflows/ui-e2e.yml` builds with `n: mock`). The
`__rwInjectConsent` window hook and DevPanel are mock-only and absent in
shipped builds (`connector/index.ts:50-65`; `dev-panel.tsx:50`).

---

## 8. State management summary

- **zustand stores**: `ui-store` (view/preview-width/sidebar; persisted
  `rw.ui.*` in localStorage), `scan-session-store` (active scan + known ids +
  celebration flag; memory only), `selection-store` (results selection;
  memory), `filter-store` (results filter/sort/tree; memory),
  `consent-store` (pending consent events; memory).
- **TanStack Query** is the server-state layer; keys in `queries/client.ts:18-41`.
  Scan progress is an external store over the event stream (not query cache).
- **localStorage keys**: `rw.ui.theme`, `rw.ui.density`, `rw.ui.locale`,
  `rw.ui.sidebarCollapsed`, `rw.ui.viewMode`, `rw.ui.previewPaneWidth`,
  `rw.ui.scale`.
- **Source of truth per screen**: devices/health/scan/results/recovery/sessions
  = the engine through the bridge (poll + events); selection/filters/theme =
  local stores. The engine is authoritative for scan status (ExitGuard
  re-reads it on close, `exit-guard.tsx:62-76`).

---

## 9. DEFECT TRACE (per user-reported defect)

### D1 — disks page does not show all disks (2 internal + 1 SD)

Chain: `home-page.tsx:66` → `queries/devices.ts:5-10` → `tauri-connector.ts:196`
→ `devices.rs:426-434` → `list_devices :107-232` → `util.rs:156-169` →
`engine cr_ui.cpp:1937-1938` → `engine cr_enumerate.cpp` → rows → group render
`home-page.tsx:250-298`.

Candidates ranked:
1. **v0.9.2 build carries the shifted bus table** (fixed only in `eb4bb5f3`,
   unreleased): SATA disks → `removable=1` → `kind:'usb'` → wrong group,
   "Internal drives (N)" undercounts, page reads as incomplete
   (`engine/include/cr/win/cr_enumerate.h:113-119` pre-fix; commit eb4bb5f3).
2. **Current-tree integration gap**: `bus_type` appended to the engine ABI but
   never consumed by the bridge (`cr_ui.h:435` vs `abi.rs:172-189`;
   `devices.rs:215`) → SD remains `kind:'usb'`; `/sd-card` page's
   `kind==='sd'` filter is permanently empty; the visible "SD card" card is a
   hard-coded fake (`sd-card-page.tsx:30-33,96-110`).
3. Engine-side present-but-unopenable drop — fixed in v0.9.0
   (`cr_enumerate.cpp:110-175`); only relevant if the installed build is
   older.
4. UI filtering — ruled out for the current tree (both groups render).
5. Refresh race — ruled out (no engine cache; `devices.rs:426-444` identical
   paths).

### D2 — SMART/health only shows the FIRST disk; switching shows NOTHING

Chain: `health-page.tsx:51` → `queries/health.ts:5-13` (per-device key) →
`tauri-connector.ts:445-447` → `health.rs:19-157` → `cr_ui_health_read` →
`engine cr_ui.cpp:4888-4995`.

Candidates ranked:
1. **D-NVMe-01 in the user's v0.9.2 build**: the packed NVMe query buffer
   (offset-9 `STORAGE_PROTOCOL_SPECIFIC_DATA`) made every real NVMe
   controller refuse → NVMe-only disks hit the both-absent gate
   (`health.rs:54-59`) → error EmptyState = "nothing". First disk (SATA/ATA)
   works. Fixed in `eb4bb5f3` only.
2. **Current-tree stack overflow** (introduced by `eb4bb5f3` + stale bridge):
   engine `cr_ui_smart_report` = 240 B (appended `power_cycles_nvme`,
   `cr_ui.h:1569`) vs Rust `CrUiSmartReport` = 232 B (`abi.rs:646-681`);
   the engine's `memset(rep, 0, sizeof(*rep))` (`cr_ui.cpp:4892`) writes 8
   bytes past the Rust stack buffer on every health read. Any next build will
   ship UB on the health path. **Fix the bridge struct before anything else.**
3. Threshold/ex2 not wired (cosmetic): `abi.rs:939-940`, `health.rs:127-131`.
4. UI-side keying/staleness — ruled out (per-device query keys, §4).

### D3 — scan jumps to 75% "advanced scanning", "files found 21 (0 B)", no
sector progress, stuck

Chain: `home-page.tsx:97-110` → `queries/scan.ts:88-98` → `scan.rs:91-110 →
116-332 → 339-460` → `events.rs:241-411` → `scan-progress:{id}` →
`queries/scan.ts:20-84` → `scan-progress-page.tsx:166-313`.

Candidates ranked:
1. **75% is the designed weight boundary** (40/35/25; quick+deep = exactly
   750‰) combined with the single "Advanced" label for deep+carve
   (`contract.ts:171-176`) — the number itself is legitimate; the stall is
   the defect.
2. **The bytes axis is carve-only in the current tree**: `tel_publish_`
   stamps live `bytes_scanned` only for stage 2 (`cr_scan.cpp:510-518`);
   `stats.bytes_scanned` accrues only from the carve (`:2704`). "Reading
   sector: 0/N" for the entire quick+deep phase, then carve-crawl — the
   user-visible "no sector-scanning progress / stuck" persists even with all
   D4 fixes in.
3. **Version skew**: "21 (0 B)" is the v0.8.4-era symptom (dataFoundBytes
   pinned 0 until terminal). v0.9.0+ fills it live (`events.rs:276,284,375`).
   Confirm the installed version; if the user still sees 0 B on v0.9.2, the
   engine's `tree_.live_logical()` counter is the suspect (engine lane).
4. **Residual stall class on v0.9.2**: D4e/D4f fixed the two known freezes;
   any remaining multi-minute freeze is new — the heartbeat event
   (liveness-without-progress) + forensic io-timeline discriminate.
5. UI event loss — ruled out (external store + poll fallback + terminal
   bridge; coalescing channel only drops superseded samples).

### D4 — restart-as-admin prompt with broken theme/colors when not elevated

Chain (current tree): none — no elevation UI exists; the only reachable text
is the toast `errors.elevationRequired` on `scan_start` refusal
(`home-page.tsx:104-107`; `tauri-connector.ts:138`), and toasts are
token-themed (`toaster.tsx:13-34`).

Candidates ranked:
1. **The installed build predates v0.9.0** (v0.8.x): that tree carried
   `elevation-banner.tsx` with the raw-amber Tailwind classes (D-UI-01,
   `worklog/DIAGNOSIS.md:136-143`) — the *exact* "broken theme/colors"
   symptom. The banner was re-themed in `c6ff7c10` and deleted in `68460a4c`.
   A stale installed artifact explains the report with zero code changes
   needed.
2. **Manifest not embedded in the artifact the user runs** (portable zip /
   cached exe / different lane): then the app runs asInvoker, the engine's
   BF-28 gate returns `ERR_ELEVATION_REQUIRED` on first scan, and the user
   sees the toast — but a toast would be correctly themed, so this only
   explains the *prompt*, not the *colors*.
3. The launch-time UAC consent (OS chrome) being read as "the prompt" —
   possible but OS-styled, not "broken theme".

### D5 — visible console windows flash when opening disk health

Chain (current tree): health read spawns **no** subprocess
(`cr_ui.cpp:4981` per-device TRIM; `cr_health.cpp:431-459`). Census in §6.

Candidates ranked:
1. **The installed build predates the D2 fix** (`68460a4c`, v0.9.0): the
   v0.8.x engine's `cr_health.cpp:306` used `_popen("fsutil …")` — one
   visible cmd.exe flash per health read (`worklog/DIAGNOSIS.md:41-58`).
   Same stale-artifact hypothesis as D4.
2. **Stale `cr_ui.dll` beside the exe**: the ABI gate in `lib.rs:98-107` only
   LOGS a version mismatch (does not refuse to serve devices/health); the
   exe's static imports bind whatever same-named DLL is beside it. An old DLL
   (with `_popen`, old enumeration) can be loaded and serve `health_read` →
   flash returns. Verify the deployed DLL's ABI stamp.
3. BitLocker/repair flows (PowerShell `CREATE_NO_WINDOW`) — correctly hidden;
   not reachable from "opening disk health".

---

## 10. Other broken-looking surfaces noticed

- **Repair page is a fake product surface**: the dropzone/queue/Repair-All
  flow never touches the backend (`repair-page.tsx:121-124`); only the hidden
  "Advanced Repair" workbench is real. A user "repairing files" there is
  watching local state flip.
- **SD-card page is a fake product surface** (§7) — static card row, mock
  default id, dead "Get Help" toast.
- **`devices.rs` kind axis**: even post-D-ENUM-02, SD→'usb' merge persists at
  the bridge (BF-02/BF-31 only half-closed).
- **`abi.rs` header-pin discipline broken by `eb4bb5f3`**: two engine ABI
  appends (bus_type, smart-report tail) are missing from the Rust binding;
  one causes the §9-D2-2 stack overflow. The stated process
  ("this file gains it in the same position", `abi.rs:10-12`) was not
  followed; `contracts/validate.mjs` phase 2 checks commands, not struct
  layouts.
- **Mock/test blind spots**: mock stage weights (0.14/0.42/0.24,
  `mock-connector.ts:149-153`) never reproduce the real 40/35/25 "75%"
  presentation; e2e runs only against the mock connector
  (`.github/workflows/ui-e2e.yml`); the real chain is exercised only by the
  forensic-repro lane.
- **`lib.rs:98-107` ABI gate is log-only** at startup (scan_start refuses
  per-scan; devices/health do not) — a mismatched engine kit degrades
  silently on the exact surfaces the user reports.
- **Stale doc comment**: `Cargo.toml:29-33` claims a TokenElevation probe +
  relaunch verb that no code implements.
- **Hard-coded 512 B sector divisor** for the "Reading sector" display
  (`scan-progress-page.tsx:131-132`) — wrong on 4Kn drives.
- **Health KPI fabrications** (gauge score, '2%' wear fallback, '· 512B')
  contradict the bridge's no-fabrication discipline
  (`health-page.tsx:100-106,403,434`).

## 11. Recommended next actions (for the lead agent)

1. Re-pin `ui/src-tauri/src/bridge/abi.rs` to the current `cr_ui.h` (add
   `bus_type`, `threshold`, `power_cycles_nvme`, declare
   `cr_ui_health_read_ex2`) — this removes the 8-byte stack overflow AND
   unlocks the D-ENUM-02 kind split + BF-03 threshold column.
2. Update `commands/devices.rs:215/:382` to derive `kind` from `bus_type`
   (USB/SD/MMC classes; NVMe/SATA/… → internal) with `removable` as the
   documented fallback.
3. Verify the version of the user's installed build + the ABI stamp of the
   deployed `cr_ui.dll` (D4/D5 stale-artifact hypothesis decides half the
   register).
4. Engine lane: extend the live `bytes_scanned` stamp to stages 0/1 (D4a-2)
   so "Reading sector" moves during quick+deep.
5. Replace the fake SD-card and Repair surfaces with real flows or gate them
   behind the workbench until real.
