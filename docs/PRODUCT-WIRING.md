# PRODUCT-WIRING — runtime wiring map of the merged product (v0.6.0-rc1, af5c838a)

Task T4-A. Read-only forensic map. Every claim carries `path:line` (paths relative to repo
root `final-prod/`). This document explains how the shipped product actually runs end to end
and enumerates every seam where the pipeline can silently fail, produce empty output, or
produce canned/fake output — the four reported user symptoms (installer PowerShell popup;
one drive + percentage-only SMART; scan stuck near 40% with no tree results; dead/fake
surfaces) are each traceable to seams documented here.

Stack: React 19 UI (`ui/src`) → Tauri 2 Rust bridge (`ui/src-tauri/src`) → C ABI `cr_ui`
(engine v2 ABI, `engine/include/cr/cr_ui.h` + `engine/src/ui/cr_ui.cpp`) → C++20 engine core
(`engine/src/core/**`) → Windows platform layer (`engine/src/win/*`) → **`rda.sys` kernel
driver, the ONLY raw-disk transport** (`engine/src/win/cr_transport.cpp:1-12`,
`docs/PARITY-DELTAS.md` D-FALL). Driver contract: `driver/src/shared/inc/rda_ioctl.h`.

---

## 0. The one-paragraph mental model

The UI never talks to the engine directly; it talks to `connector` (`ui/src/connector/index.ts:16`)
whose only implementation is `createTauriConnector` (`ui/src/connector/tauri/tauri-connector.ts:176`).
Every method is a `invoke('<command>')` cross-checked against `contracts/command-surface.json`.
The bridge holds registries (`ui/src-tauri/src/bridge/state.rs:411-424`) of live scans / jobs,
marshals engine events from worker threads through a bounded channel to one emitter thread per
handle (`ui/src-tauri/src/bridge/events.rs:196-322`), and serves results by re-snapshotting the
engine's in-memory result tree (`cr_ui_scan_snapshot`) into a bridge-side metadata cache that the
bridge itself filters (`ui/src-tauri/src/commands/results.rs:1-20`, gap G-7). Scans always run
quick→deep→carve with weights 40/35/25 (`engine/include/cr/orchestrator/cr_scan.h:71-75`).

---

## A. End-to-end path diagrams

### A.1 Device enumeration → Home / Health drive list

1. **Home page mount** — `ui/src/features/device-selection/home-page.tsx:69` `useDevices()` →
   react-query `queryKey ['devices']` (`ui/src/queries/devices.ts:5-10`, defaults at
   `ui/src/queries/client.ts:4-16`).
2. **Connector** — `connector.devices.listDevices()` → `invoke('devices_list')`
   (`ui/src/connector/tauri/tauri-connector.ts:179`).
3. **Tauri command** — `devices_list` (`ui/src-tauri/src/commands/devices.rs:419-426`) →
   `spawn_blocking(list_devices)` (`devices.rs:100`).
4. **Two-call FFI enumeration** — `enumerate_disks()` / `enumerate_volumes()`
   (`ui/src-tauri/src/bridge/util.rs:156-184`) → `cr_ui_enumerate_disks/volumes`
   (`ui/src-tauri/src/bridge/abi.rs:480-481`).
5. **Engine disk enumeration** — `cr_ui_enumerate_disks` → `enumerate_disks_impl`
   (`engine/src/ui/cr_ui.cpp:1461-1501`) → `win::enumerate_disks(200, list)`
   (`cr_ui.cpp:1465`, loop `engine/src/win/cr_enumerate.cpp:142` n=0..199 opening
   `\\.\PhysicalDriveN` via `open_disk_for_query` `cr_enumerate.cpp:101-119`, incl. the
   0-access metadata retry for share-blocked disks) **plus** the cfgmgr32 interface pass
   (`cr_enumerate.cpp:181-223`) merged by `merge_disk_enumeration`
   (`engine/include/cr/win/cr_enumerate.h:199-246`). Each disk's partition table is parsed
   **through the RDA driver** (`cr_enumerate.cpp:165-170` `open_rda_partition(L"RDA", n, 0)`)
   — driver absent ⇒ `table_ok=false`, disk still listed.
6. **Engine volume enumeration** — `enumerate_volumes_impl` (`cr_ui.cpp:1503-1533`) →
   `win::enumerate_volumes` (`cr_enumerate.cpp:227-264`): `GetLogicalDriveStringsW`
   (`cr_enumerate.cpp:224`) filtered to `DRIVE_FIXED | DRIVE_REMOVABLE` (`cr_enumerate.cpp:230`)
   — **lettered volumes only**; extents via `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`
   (`cr_enumerate.cpp:240-247`).
7. **Bridge row composition** — one `Device` per disk: id `disk-<n>` (`devices.rs:20-22`),
   kind = `removable ? 'usb' : 'internal'` (`devices.rs:207`, G-9: no bus type), volumes with
   id `vol-<L>` / `vol-<disk>-<offset>` (`devices.rs:26-32`), usage from Win32
   `GetDiskFreeSpaceExW` per letter (`devices.rs:52-73`), status from a fresh
   `cr_ui_open_disk` probe (`devices.rs:122-127`; any failure ⇒ `'permission-denied'`
   `devices.rs:77-83`, including `ERR_DRIVER_REQUIRED` when the driver is missing).
8. **UI filter** — Home: `physical = kind!=='virtual'`, `internal = kind==='internal'`,
   `visible = showRemovable ? removable : internal` (`home-page.tsx:78-84`) — **USB/SD disks
   are invisible on Home unless `?media`**. Health page (`ui/src/features/health/health-page.tsx:43-49`)
   and `usePhysicalDevices` (`ui/src/hooks/use-physical-devices.ts:21`) list all non-virtual
   disks (USB included).
9. **Card render** — `DeviceCard` (`home-page.tsx:511+`); volume chips only when
   `volumes.length > 1 || some volume not ok` (`home-page.tsx:530-531`).

### A.2 Health / SMART read (Health page)

1. Selected disk id (`?device=` or first device — `use-physical-devices.ts:22-24`) →
   `useHealth(deviceId)` (`ui/src/queries/health.ts:5-13`, `retry:false`).
2. `connector.health.getHealth` → `invoke('health_read', { deviceId })`
   (`tauri-connector.ts:342-344`).
3. `health_read` parses `disk-<n>` (`ui/src-tauri/src/commands/health.rs:15-23`) — any other id
   (e.g. a volume id `vol-C`) is refused `device-gone`.
4. **Two-call pattern, first call** `cr_ui_health_read(n, &report, null, 0, &needed)`
   (`health.rs:27-29`) → `health_read_impl` (`engine/src/ui/cr_ui.cpp:3623-3702`):
   - opens `\\.\PhysicalDrive<n>` **GENERIC_READ|GENERIC_WRITE** (`cr_ui.cpp:3630-3639`) —
     requires elevation; non-elevated ⇒ `ERR_OPEN_FAILED` "administrator rights required".
   - ATA SMART: `win::health::read_smart` (`engine/src/win/cr_health.cpp:215-262`) over the
     transport chain `IOCTL_ATA_PASS_THROUGH` → `SMART_RCV_DRIVE_DATA (0x7C088)` → CSMI
     CC-0x19 miniport (`cr_health.cpp:43-197`); pure flow `read_smart_ata`
     (`engine/src/core/health/cr_smart_ata.cpp:94-190`): first channel answering IDENTIFY
     (0xEC) serves SMART READ DATA (0xB0/D0) + THRESHOLDS (0xB0/D1, non-fatal), 30×12 table
     parse (`cr_smart_ata.cpp:134-184`).
   - NVMe health log page 2 via `IOCTL_STORAGE_QUERY_PROPERTY` (`cr_health.cpp:264-305`).
   - SSD heuristic `looks_like_ssd` (seek penalty, `cr_health.cpp:327-345`) and TRIM via
     **`_popen("fsutil behavior query disabledeletenotify")`** (`cr_health.cpp:307-325`) —
     spawns a console child process **twice per health_read** (once per call, and the bridge
     calls `cr_ui_health_read` twice, below).
   - Returns OK if *either* `rep->available` (ATA) *or* `rep->nvme_available`
     (`cr_ui.cpp:3697-3701`); `ERR_HEALTH_UNAVAILABLE` (0x50070001) only when both fail.
5. **Bridge gate** — `if report.available == 0 → typed health-unavailable`
   (`health.rs:43-48`) — **this fires even when `nvme_available == 1`**: a pure-NVMe drive
   (ATA chain silent, NVMe log OK) is converted to an ERROR by the bridge although the engine
   succeeded.
6. **Second call** for attributes: `cr_ui_health_read(n, &report, attrs, needed, &mut cap)`
   (`health.rs:50-54`) re-runs the whole probe; `attrs.truncate(cap)`.
7. **Grading** — status good/caution/bad from `failing_now`/`critical_warning`/failing
   attrs/wear/low spare (`health.rs:58-78`); threshold always `null` (`health.rs:104`,
   engine carries no threshold — N-055 history).
8. **UI render** — hero ring shows `healthScore` computed **in the UI**
   (`ui/src/features/health/health-page.tsx:100-106`): `good → max(88, 100-(wearPercent ?? 4))`
   (default **96%**), `caution → 72`, `bad → 38`. The SMART table renders
   `health.data.attributes` (`health-page.tsx:485`) with a count badge (`:454`). Error ⇒
   EmptyState "S.M.A.R.T. not available" (`health-page.tsx:215-222`).

### A.3 Scan start → progress → partial results → completion → results tree

1. **Start** — Home card / volume chip / Health CTA / quick-targets (`home-page.tsx:88-103`,
   `health-page.tsx:54-67`) → `useStartScan` → `connector.scan.startScan({targetId, pathScope})`
   → `invoke('scan_start')` (`tauri-connector.ts:195-201`; `ui/src/queries/scan.ts:188-198`
   stores `{scanId,targetId}` in `useScanSessionStore` `ui/src/stores/scan-session-store.ts:24-29`).
2. **scan_start_blocking** (`ui/src-tauri/src/commands/scan.rs:102-273`):
   - ABI gate (`scan.rs:109-115`).
   - `resolve_target` (`ui/src-tauri/src/commands/devices.rs:238-313`): `disk-N` /
     `vol-<L>` / `vol-<disk>-<offset>` / virtual `vol-recycle-bin`→system volume +
     implicit scope `X:\$Recycle.Bin` (`devices.rs:244-250`), `vol-desktop`→`%USERPROFILE%\Desktop`
     (`devices.rs:330-334`).
   - **G-11 pathScope is view-scoping only**: the engine always scans the WHOLE volume/disk;
     the bridge filters result views by prefix (`scan.rs:8-11`, `scan.rs:40-67`).
   - open: lettered volume → `cr_ui_open_volume` (bitlocker gate → whole-disk RDA open +
     `PartitionSlice` on the extent, `engine/src/ui/cr_ui.cpp:1186-1234`); disk →
     `cr_ui_open_disk` (RDA grammar only; driver missing ⇒ `ERR_DRIVER_REQUIRED`,
     `cr_ui.cpp:1152-1183`).
   - **config is zero-initialized** except `whole_disk`, `pre_scan_repair=1`,
     `pre_scan_check=1`, consent callback (`scan.rs:184-194`): `stage_mask=0` ⇒ engine
     default **quick|deep|carve** (`engine/src/ui/cr_ui.cpp:1036-1040`), weights 0 ⇒
     **40/35/25** (`engine/include/cr/orchestrator/cr_scan.h:71-75`), threads 0 ⇒ auto.
   - event sink subscribed BEFORE run (`scan.rs:201-215`, bounded channel 512).
3. **Run thread** — `cr_ui_scan_run` executes synchronously on a dedicated thread
   (`scan.rs:300`); engine `scan_run_impl` (`cr_ui.cpp:1729-1815`) starts the 5 s heartbeat
   watchdog (`cr_ui.cpp:875-916`, fires only standing telemetry, never synthetic progress).
4. **Engine pipeline** — `ScanController::run` (`engine/src/core/orchestrator/cr_scan.cpp:827-1184`):
   1. disk-truth pass (`cr_scan.cpp:841-897`);
   2. **pre-scan repair** (`cr_scan.cpp:916-981`) — `repair::analyze` read-only; if a repairable
      plan exists, `cfg_.repair.consent(rplan, user)` is invoked ON THE RUN THREAD
      (`cr_scan.cpp:944`), which is the bridge's `consent_sink`
      (`ui/src-tauri/src/bridge/consent.rs:121-159`): mints requestId, registers the gate,
      emits `consent-request:<scanId>`, and **BLOCKS the run thread** on
      `wait_for_decision()` with **no timeout** (`consent.rs:86-92,156`); answered via
      `consent_respond` (`ui/src-tauri/src/commands/repair.rs:31-38`). Cancel force-denies
      first (`scan.rs:490-495`).
   3. **pre-scan damage audit** (`cr_scan.cpp:983-1002`) — read-only fscheck, never aborts.
   4. read pipe: `build_read_pipe(slice, 8 MiB cache, 256 KiB blocks)` over
      retry+bisection (`cr_scan.cpp:1049-1058`).
   5. fs probe when no hint (`cr_scan.cpp:1033-1037` `probe_family(pipe)`).
   6. `run_quick` (engine pass with deep semantics when deep follows — single MFT walk,
      `cr_scan.cpp:1186-1206`, `run_engine_dispatch` `cr_scan.cpp:1232-1350`);
   7. `run_deep` (`cr_scan.cpp:1446-1642`): NTFS ⇒ **`scan_mft_remnants`** — a sequential
      4 MiB-block sweep of the ENTIRE volume (`engine/src/core/ntfs/cr_mftwalk.cpp:2117-2164`,
      block const `cr_mftwalk.cpp:227`); whole-disk ⇒ `lost::discover` + candidate fan-out;
   8. `run_carve_stage` (`cr_scan.cpp:1708+`).
   9. post-pass: junk filterfolder + recoverability grading (`cr_scan.cpp:1166-1181`).
5. **Progress events** — engine emits typed `cr_ui_event`s via the trampoline
   (`cr_ui.cpp:809-846`): `CR_EVT_SCAN_STAGE` at `tel_stage_begin_` (`cr_scan.cpp:307-322`),
   `CR_EVT_SCAN_PROGRESS` coalesced to ≤1000 per scan (`emit_progress_event_`
   `cr_scan.cpp:433-451`), `CR_EVT_FILES_FOUND`/`ERROR_RANGE` only at stage boundaries
   (`emit_boundary_events_` `cr_scan.cpp:453-485`), `CR_EVT_HEARTBEAT` every 5 s while
   stalled (`cr_ui.cpp:875-916`).
6. **Bridge marshaling** — sink `try_send`s POD copies (`events.rs:36-43`); emitter maps
   stage 0→`quick-index`, 1→`deep-walk`, 2→`carve` (`events.rs:211-215`), clamps overall
   monotonic (`events.rs:220-227`), coalesces telemetry (`events.rs:278-307`), and emits
   `scan-progress:<scanId>` (`events.rs:318`). `currentPath` is **always null** (G-2,
   `events.rs:145-147`). Terminal status latched by the run thread (`scan.rs:339-398`)
   incl. final snapshot freeze + exact found-bytes sum (`scan.rs:355-367`).
7. **UI progress** — `useScanProgress` external store (`ui/src/queries/scan.ts:26-88,216-229`)
   plus 500 ms `scan_state` polling while running (`scan.ts:200-210`). The store invalidates
   tree/results/count queries **only when the 10 % bucket advances**
   (`scan.ts:41,55-60`) or on terminal status (`scan.ts:44-54`).
8. **Partial results visibility** — the live `ResultsExplorer` (`scan-progress-page.tsx:142-148`)
   fetches `results_tree`/`results_query` on mount and per invalidation →
   `refresh_snapshot` (`state.rs:520-570`, min 1.5 s cadence) → `cr_ui_scan_snapshot`
   (`cr_ui.cpp:2282-2312`, mid-scan capture is supported, `engine/src/core/tree/cr_snapshot.cpp:16-76`)
   → `ensure_metas` builds the metadata cache (`state.rs:582-668`); first successful snapshot
   flips `recoverableDuringScan=true` (`state.rs:558,567,572`).
9. **Completion** — terminal progress + `scan-finished:<scanId>` (`scan.rs:385-398`); UI shows
   the celebration modal and navigates to `/results/:scanId` (`scan-progress-page.tsx:478-536`);
   `ResultsRoute` re-checks `scan_state` (`ui/src/features/results-explorer/results-route.tsx:22-60`).

### A.4 Results tree / query (post-scan and live)

`results_tree` / `results_query` / `results_count` / `node_detail`
(`ui/src-tauri/src/commands/results.rs:439-534,819-833,913-925,899-911`) all funnel through
`metas_snapshot` (`state.rs:690-708`) and ONE bridge-side filter (`matches_pred`
`results.rs:308-355`; node-id scheme `f<idx>`/`d<idx>`/`root`/`lost-paths`/`raw-all`/`c<n>`/`ext:`
`results.rs:8-18`). Path-mode tree: root row + `lost-paths` + `raw-all` (`results.rs:646-688`);
children of `root` = **only metas that are dirs whose path has no '/'** (`results.rs:701-715`).

### A.5 Preview

`usePreview` (`ui/src/queries/preview.ts:7-15`) → `preview_open`
(`ui/src-tauri/src/commands/preview.rs:139+`): resolve `f<idx>` → node_id
(`preview.rs:70-96`), open `cr_ui_preview_open_snapshot/_session` (needs the source device
still attached — `preview.rs:98-124`), kind decided from engine metadata, image cap 16 MiB /
hex 64 KiB (`preview.rs:12-15`), unreadable spans ⇒ typed `preview-damaged` (`preview.rs:8-14`).
Thumbnails: image-kind only, null otherwise (G-4).

### A.6 Recovery

`useStartRecovery` (`ui/src/queries/recovery.ts:66-84`) → `recovery_start`
(`ui/src-tauri/src/commands/recovery.rs:56-97`): `cr_ui_recovery_create` on the entry's device
→ `set_destination` (engine arms F12 overlap) → enqueue per selection: live scans enqueue by
snapshot index, sessions by node_id (`recovery.rs:117-198`); recover-all pages the bridge filter
500 rows at a time (`recovery.rs:174-197`) → process thread `cr_ui_recovery_process`
(`recovery.rs:247+`), byte-level progress (`events.rs:46-56,330-427`) + report-tail polling for
`currentFile` (`events.rs:431-454`), terminal map from stats (`recovery.rs:249-270`).

### A.7 Sessions save/load

Save: `session_save` (`ui/src-tauri/src/commands/sessions.rs:62-131`) refuses while running,
auto-names `auto_<label>_<M>_<D>...crsf` under `%ProgramData%\HeicFast\Recovery Wizard\sessions`
(`state.rs:523-546`). List: stat-first + lazy load+close per file (`sessions.rs:134-236`).
Load: `session_load` (`sessions.rs:240-364`) → `cr_ui_session_load` → registry entry with
terminal state; **source device re-attach is best-effort by model+serial+slice**
(`sessions.rs:373-419`) — absent/ambiguous ⇒ preview/recovery fail truthfully per node.

### A.8 Driver status / system

`system_driver_status` (`ui/src-tauri/src/commands/system.rs:39-134`): SCM service `RDA` +
`\\.\RDA` open + `IOCTL_RDA_GET_VERSION (0x60002448)` (`system.rs:15-17`). The engine↔driver
contract: PID-token grammar `\\.\RDA\<token>\<disk>\<part>` (`cr_transport.cpp:318-343`,
token `cr_transport.cpp:312-322`), **ReadFile completion `Information = Length*512`**
(driver `rda_ioctl.h:124-139`, unscale `cr_transport.cpp:53-78`), 64 KiB read envelope
(`cr_transport.cpp:184-215`), single-handle per-thread discipline, handle self-heal on
`ERROR_INVALID_HANDLE` (`cr_transport.cpp:217-268`). Missing driver ⇒ `ERR_DRIVER_REQUIRED`
at every open (`cr_transport.cpp:376-394`, `cr_ui.cpp:1158-1183`).

---

## B. The stage/progress model as the UI presents it

- **Stage vocabulary**: engine stages 0/1/2 (`cr_ui.h:236`) → bridge `quick-index`/`deep-walk`/
  `carve` (`events.rs:211-215`); bridge initial stage `volume-analysis` (`state.rs:92`);
  terminal `done` (`events.rs:132`). UI phase mapping `scanPhaseOf`
  (`ui/src/connector/contract.ts:156-166`): `volume-analysis|quick-index → quick`,
  `deep-walk|carve → advanced`, else `organizing`.
- **Labels** (`ui/src/i18n/en.ts:154-158`): "Quick Scanning {{percent}}" / "Advanced Scanning
  {{percent}}" rendered at `scan-progress-page.tsx:176-184`; "Files Found: N (size)"
  (`scan-progress-page.tsx:185-190`); "Reading sector: X/Y" where X/Y = `bytesScanned/512` and
  `bytesTotal/512` — **hard-coded 512 division regardless of real sector size**
  (`scan-progress-page.tsx:128-129`); "Searching: <path>" ticker is **never shown** because
  `currentPath` is always null (`events.rs:145-147`, G-2).
- **The ~40% point is the quick→deep boundary by construction**: overall =
  `weighted_progress(40/35/25)` over stage permilles (`engine/include/cr/orchestrator/cr_scan.h:71-75`,
  `cr_scan.cpp:355-390`). Quick completing ⇒ overall = 40.0%. The label flips from "Quick" to
  "Advanced" at that moment (stage event deep-walk). There is no user-facing "quick scan then
  advanced" button — the scan ALWAYS runs all three stages (bridge zero-init `scan.rs:184`,
  engine default mask 7 `cr_ui.cpp:1036`).
- **Where sector counts come from**: `bytesScanned` = engine telemetry `bytes_scanned`
  (carve-true bytes; filled at every progress emit `events.rs:278-283`), `bytesTotal` = the
  opened view's geometry total (`scan.rs:172-174,230`).
- **Deep-stage progress granularity is coarse by design**: during the NTFS remnant sweep the
  progress callback fires **once per parallel chunk** (chunk = volume/threads,
  `cr_mftwalk.cpp:2135-2152`) and **only once at the very end in single-threaded mode**
  (`cr_mftwalk.cpp:2155-2162` — `remnant_range_scan` itself never ticks, `cr_mftwalk.cpp:2087-2113`).
  Between ticks the 5 s heartbeat repeats the standing permille (`cr_ui.cpp:875-916`). So a
  multi-hundred-GB volume legitimately sits at 40.x% for a very long time (4 MiB sequential
  reads through the 64 KiB-envelope driver path).
- **filesFound freezes mid-deep**: FILES_FOUND events fire only at stage boundaries
  (`cr_scan.cpp:453-485`); the bridge refreshes `live.files_found` from telemetry on each
  progress emit (`events.rs:282`) — but telemetry counts only advance when stages publish.
- **Consent gate blocks everything**: while `consent_sink` waits (no timeout), NO stage has
  begun — no stage/progress/files events at all; UI shows 0%/starting + the prompt
  (`consent.rs:86-92`, `scan-progress-page.tsx:334-413`).

---

## C. Every silent-failure / empty / fake seam (numbered)

### C-1 … why the UI might show only ONE drive (symptom 2a)

1. **Home hides non-internal disks by default** — `home-page.tsx:79-84`: `visible = internal`
   unless `?media`. A USB/SD disk (`kind:'usb'`, `devices.rs:207`) never appears on Home.
   User-visible: "all other drives invisible" on the landing page.
2. **Health/Home list physical disks, not volumes** — `use-physical-devices.ts:21` +
   `home-page.tsx:78`: one card per *disk*. A machine with C:+D:+E: on one SSD shows ONE card;
   volume chips render only when `volumes.length>1 || status!=ok` (`home-page.tsx:530-531`).
   User-visible: "only one drive reported".
3. **Unlettered volumes are structurally unenumerable** — `cr_enumerate.cpp:224-230` uses
   `GetLogicalDriveStringsW` + `DRIVE_FIXED|DRIVE_REMOVABLE` only. The bridge's
   `vol-<disk>-<offset>` id (`devices.rs:26-32`) can never be produced from real enumeration;
   network drives (`DRIVE_REMOTE`) and optical are dropped entirely.
4. **Status probe conflates every failure with permission** — `devices.rs:122-127,77-83`:
   `cr_ui_open_disk` probe fails (driver missing ⇒ `ERR_DRIVER_REQUIRED`, elevation, busy) ⇒
   card shows locked/permission badge but still lists — unless the *0-access metadata open*
   also fails (`cr_enumerate.cpp:101-119`), in which case the disk vanishes silently from
   the list ("the honest limit").
5. **Enumeration table read requires the driver** — `cr_enumerate.cpp:163-170`: driver absent
   ⇒ `table_ok=false` (silent — only layout info degrades; the disk still lists).
6. **health_read only accepts `disk-N` ids** — `health.rs:15-23`: a volume id yields
   `device-gone`; the Health page passes the selected physical disk so this is consistent,
   but any future volume-scoped call silently errors.

### C-2 … why SMART shows a percentage but ZERO attributes (symptom 2b)

7. **The percentage is UI-fabricated** — `health-page.tsx:100-106`: `healthScore` = 96 (good,
   default wear 4), 72 (caution), 38 (bad). It renders whenever `health.data` exists — even
   with `attributes: []`. The engine NEVER produces a percentage for ATA disks; the bridge
   produces only status/telemetry fields (`health.rs:111-122`).
8. **`available==0` gate throws away healthy NVMe data** — `health.rs:43-48` refuses when
   `report.available==0` even though the engine returned OK with `nvme_available=1`
   (`cr_ui.cpp:3697-3701`). Every NVMe-only drive (most modern laptops) shows the
   "not available" error state — no table, no wear, nothing.
9. **Zero-attribute success is representable** — if the ATA chain answers IDENTIFY and SMART
   READ DATA but the table parses to zero non-zero ids (`cr_smart_ata.cpp:134-137` skips
   `p[0]==0` slots), the engine returns `available=1, attribute_count=0`; the bridge's second
   call then returns `attrs=[]` with status `good` (`health.rs:50-78`) ⇒ the UI renders the
   fake 96% ring over an empty table (`health-page.tsx:485`, badge "0" `:454`). Some USB
   bridges/virtual controllers return zeroed SMART tables exactly like this.
10. **Second-call drift** — the bridge calls `cr_ui_health_read` twice (`health.rs:27-54`); if
    the second call's ATA leg fails but NVMe/OK path returns OK, `attr_count=0` and
    `report.available=0` are NOT re-checked (`health.rs:52-54` only checks rc) ⇒ status good,
    zero rows, wear maybe from NVMe ⇒ same fake-percentage rendering.
11. **Health open requires GENERIC_READ|GENERIC_WRITE on PhysicalDrive** — `cr_ui.cpp:3630-3639`:
    non-elevated ⇒ ERR_OPEN_FAILED → "administrator rights required" message (the one honest
    message), surfaced as `device-permission`.
12. **`_popen(fsutil …)` per health read** — `cr_health.cpp:307-325`: spawns a console child
    (twice, because of the two-call bridge pattern). On a GUI-subsystem process this can flash
    console windows — a second "popup" source besides the installer one (symptom 1 is the
    installer's `ExecWait powershell` — `installer/recoverywizard.nsi:349,608`).
13. **Thresholds are always null** — `health.rs:104`: per-attribute threshold column renders
    "—" for every row even when the engine read 0xD1 thresholds (`cr_smart_ata.cpp:126-148`)
    — the ABI struct has no threshold field (`abi.rs:413-420`).

### C-3 … why a scan sits near 40% forever with no results (symptom 3)

14. **40% is the quick/deep weight boundary** — `cr_scan.h:71-75`; quick done ⇒ exactly 40%.
    The crawl after it is the deep stage (NTFS remnant sweep over the whole volume,
    `cr_mftwalk.cpp:2117-2164`, or lost-discovery on whole-disk scans, `cr_scan.cpp:1496-1620`).
    On large volumes this is *hours* of 4 MiB sequential reads — progress ticks only per
    chunk (`cr_mftwalk.cpp:2148-2151`) or once at the end (`cr_mftwalk.cpp:2161-2162`).
15. **Single-threaded remnant sweep emits NO progress at all** — `cr_mftwalk.cpp:2155-2159`:
    with `threads==1` (auto = hw-1 clamp 1..8, `cr_scan.h:192`), the ONLY progress callback is
    the terminal one; the UI would freeze at exactly 40.0% until deep ends (heartbeats repeat
    the standing permille, `cr_ui.cpp:875-916`).
16. **UI refetch gating: 10 % buckets** — `scan.ts:41,55-60`: tree/results/count refetch only
    when `floor(overall*10)` advances. Between 40% and 50% (the entire deep stage) the results
    pane never refetches. User-visible: counters frozen, tree frozen — "no results ever appear"
    even when quick found hundreds of thousands of files.
17. **The path-mode tree is structurally empty against the real engine** (CRITICAL): the engine
    result tree hangs everything under virtual roots `Found` / `Deleted` / `Lost Files` /
    carved root (`cr_scan.cpp:199-205`, `ResultTree::add_root` `engine/src/core/tree/cr_node.cpp:109-127`
    sets props but leaves `attributes=0` and `origin=Structured`). The bridge classifies nodes
    as directories via `attributes & 0x10` (`state.rs:636`) — so the roots are *files*, and
    every real directory's path starts with the root's name and contains '/'
    (`state.rs:625-634`). The tree's depth-1 listing (`results.rs:701-715`) requires
    `is_dir && !path.contains('/')` ⇒ **zero rows under `root`**; root/lost-paths/raw-all all
    show childCount 0. The e2e double's fixture has top-level dirs (`ui/tests/fakes/test-connector.ts:164-175`,
    `DIRS` with `parentId: null`) — a tree shape the real engine never produces, so tests
    stay green. (The *file list* still shows files; the virtual roots themselves appear as
    0-byte "files" named Found/Deleted.)
18. **fs probe failure yields an empty-but-successful quick pass** — `run_engine_dispatch`
    default case returns OK with zero files for unknown FS (`cr_scan.cpp:1345-1348`);
    `probe_family` failing (e.g. boot sector unreadable through the driver) is silent. Scan
    then "progresses" 40→75→carve with nothing in the tree.
19. **Consent gate: prompt lost ⇒ scan frozen at 0%** — if the `consent-request` event is
    missed AND `scan_state.pendingConsent` re-hydration is not consulted (it is —
    `scan.rs:545`, `scan.ts:121-129` — but only while the page polls), the run thread stays
    blocked forever with no timeout (`consent.rs:86-92`). The prompt is non-dismissable
    (`scan-progress-page.tsx:334-341`); any UI bug that unmounts it without answering deadlocks
    the scan at "starting" with 0%.
20. **Driver read failures are absorbed by retry/bisection** — the read pipe isolates bad
    ranges and zero-fills (`cr_scan.cpp:1049-1058`, M-058); mid-scan driver failures surface
    only as `errors`/badSectorMap growth (`events.rs:232-238`), never as a scan error, so a
    driver that fails every read yields an "empty successful" scan, not a failure.
21. **Pause/resume transients** — `set_transient` 2.5 s window (`scan.rs:406-425`) and
    `deviceConnected` synthesized from heartbeat freshness with a 10 s window
    (`events.rs:96-106`, `state.rs:753-754`): a stalled-but-alive engine reports
    `running` + connected forever.
22. **Sector counts are cosmetic** — hard-coded `/512` (`scan-progress-page.tsx:128-129`)
    inflates counts 8× on 4 Kn drives; `bytesTotal` for a volume target is the slice size
    (`scan.rs:172-174`), for whole-disk the disk size.

### C-4 … installer popup (symptom 1)

23. **NSIS runs PowerShell synchronously with a visible window** — `installer/recoverywizard.nsi:349`
    (`ExecWait 'powershell.exe -NoProfile … install.ps1'`, uninstall at `:608`): the console
    host flashes during install. (Root cause already confirmed by T1; kept here because it is
    part of the shipped path's wiring.)
24. **`fsutil` `_popen` console flashes** (see C-12) — any health_read (Health page visit,
    two calls per drive) can flash a console on some configurations.

### C-5 … dead / fake / canned surfaces (symptom 4)

25. **Lost Partition page is 100% canned** — `ui/src/features/lost-partition/lost-partition-page.tsx:48-150`
    renders hard-coded `RESCUE_DISK_SPECS` (fake "Disk 2 (Basic, MBR, 58.31 GB, Generic- SD/MMC…)",
    fake found-partition rows with fake LBAs and "High" recoverability). "Search for Lost
    Partition" is `setStep('scan-completed')` (`lost-partition-page.tsx:155-172`) — no scan
    runs. Real lost-partition output exists only as scan-time events the UI never renders
    (`events.rs:240-244` keeps a truthful counter with "no badge surface").
26. **Repair page is canned + its commands are typed not-implemented** —
    `ui/src/features/repair/repair-page.tsx` renders a fake queue ("WinMemoryCleaner (1).exe" —
    asserted in `ui/tests/reference-surfaces.test.tsx:88-97`); every repair command throws
    `not-implemented` (`ui/src-tauri/src/commands/repair.rs:41-66`, gap G-1).
27. **Home "Select Folder" dialog shows fake subfolders** — `home-page.tsx:385-437`:
    hard-coded lists `['Users\Documents','Users\Downloads',…]` / `['Projects','Media',…]`
    regardless of the real filesystem; the real picker is `system_pick_directory`
    (`system.rs:289-307`, native dialog). The fake tree is only dismissed by Continue.
28. **Health page hard-codes "512B" sector size and a "2%" wear fallback** —
    `health-page.tsx:403,434`.
29. **`quickScanAvailable` is always true, `lostPartition` hints never exist** —
    `devices.rs:173` (G-10): no per-volume gating, so the UI can never degrade honestly.
30. **Sd-card page filters to `kind==='sd'` which never occurs** — `devices.rs:207` maps
    removable→'usb' (G-9); `ui/src/features/device-selection/sd-card-page.tsx` filters
    `kind==='sd'` ⇒ the page is permanently empty (dead surface by type-map mismatch).
31. **Backup ("ChronoSnap") page is marketing copy over session files** —
    `ui/src/features/backup/backup-page.tsx:14-60`: static feature bullets + the real
    sessions list; nothing "backup" is wired.
32. **Activation/About dialogs** — `ui/src/features/settings-about/*` render static info;
    `engineVersion` is honestly null (G-6, `system.rs:154-164`).
33. **`scan_close` is registered but never invoked by the UI** — the connector has no method
    calling it (`command-surface.json:13` notes "internal (bridge lifecycle)"); entries
    accumulate until window close.
34. **lib.rs ABI-gate only eprintln's** — `ui/src-tauri/src/lib.rs:76-83`: an ABI mismatch is
    logged to stderr (invisible in a windowed app; no file logging anywhere — T1 finding),
    the app still starts and every scan_start then fails with the typed message
    (`scan.rs:109-115`).

---

## D. Error-flow map — what reaches the UI vs what is swallowed

**Reaches the UI** (reject → `toConnectorError` `tauri-connector.ts:124-143` → reasonKey):
- All bridge `BridgeError`s with `bridgeCode` (scan-not-found, scan-scope-invalid, consent-not-found,
  device-gone, health-unavailable, preview-*, destination-*, not-implemented — `error.rs:24-40`).
- Engine numeric codes through `ENGINE_CODE_MAP` (`tauri-connector.ts:87-122`), e.g.
  `ERR_DRIVER_REQUIRED 0x50020007 → driver-required` (`tauri-connector.ts:93`).
- Run-thread failures: rc → `failed` + `failure.code` mapping (`scan.rs:339-351,521-532`);
  ERR_CANCELLED is translated to status `cancelled`, never an error (`error.rs:14-16`).

**Swallowed / degraded at the bridge**:
- `scan_event_sink` `try_send` drops events when the 512-channel is full ("UI slow",
  `events.rs:36-43`) — progress samples lost silently (coalescing is by design).
- Unknown event types ignored (`events.rs:272`).
- `cr_ui_scan_get_stats` / telemetry failures ignored (`scan.rs:315`, `events.rs:280`) —
  terminal counters can read 0 without any error.
- Second `health_read` attributes failure only surfaces if rc != OK (`health.rs:52`);
  `available==0`/empty attrs is NOT re-checked (C-10).
- Enumeration errors: `devices_list` propagates (`devices.rs:419-426`), but per-disk probe
  failures degrade to `permission-denied` status (`devices.rs:77-83`) — the underlying code
  (driver-required vs busy vs locked) is dropped.
- `refresh_snapshot`/`ensure_metas` failures propagate as `unknown` (`state.rs:690-708`) —
  a mid-scan snapshot failure makes the results pane error out while the scan continues.
- Lost candidates and fs-check errors are counted but never rendered (`events.rs:240-244,267-271`).
- `emit` failures (`let _ = app.emit`) are ignored everywhere (`events.rs:188,318,424`).

**Swallowed / degraded at the engine**:
- Candidate-volume scan errors are swallowed by design (`cr_scan.cpp:1218-1224` — "an
  unviable candidate volume is not a scan error").
- Remnant-sweep unreadable blocks are skipped forward (`cr_mftwalk.cpp:2103-2107`).
- Unknown-FS quick/deep passes return OK with nothing (`cr_scan.cpp:1345-1348`).
- Driver read failures become bad-sector ranges, never scan errors (C-20).
- Pre-scan repair failure/denial never aborts (`cr_scan.cpp:976-981`).

---

## E. What is NOT wired (affordances vs capabilities)

**UI affordances whose commands throw not-implemented**: `repair.dryRunRepair`,
`repair.applyRepair`, `repair.getUndoLog`, `repair.undoRepairs` (`repair.rs:41-66`) —
the Repair workbench UI renders a fake queue over these.

**Engine capabilities the UI never calls** (declared in `abi.rs:460-548`, unused by
`commands/`): image opens (`cr_ui_open_image`), BitLocker unlock surfaces
(`cr_ui_open_volume_with_key`, `cr_ui_unlock_volume_with_dictionary`), the query family
(`cr_ui_query_run` — replaced by the bridge-side filter, G-7), session resume
(`cr_ui_session_resume` — no UI resume-from-checkpoint), assistant stats
(`cr_ui_scan_assistant`), apply/lost-partition family, checkpoint count, scan_close
(registered but unreachable from the connector), `cr_ui_last_transport_error` (never read
by the bridge — the driver-vs-open distinction is lost, N-041's diagnostic surface).

**Events with no UI surface**: `CR_EVT_LOST_CANDIDATE` (counter only), `CR_EVT_FS_CHECK`
(counter only), `CR_EVT_CHECKPOINT` (none), `CR_EVT_REPAIR` (log line only), save-progress
(spinner event exists; session save UI is a plain mutation without the spinner subscription
in `ui/src/queries/sessions.ts`).

---

## OPEN QUESTIONS

- **OQ-1**: Does the shipped `rda.sys` actually complete the grammar open for disks ≥ 1 and
  volumes on the user's machine (symptom 2a)? Evidence that would resolve: a
  `cr_ui_last_transport_error()` trace or Process Monitor log of `\\.\RDA\<token>\…` creates
  during `devices_list`. The bridge never reads `cr_ui_last_transport_error` (E).
- **OQ-2**: Which deep-stage path is the user's 40.x% crawl — NTFS remnant sweep (volume
  target) or lost-discovery (whole-disk target)? `scan start:` N-048 log lines
  (`cr_scan.cpp:1055-1058`) would resolve; the shipped app has no file logging (C-34), so
  add logging or reproduce with `rrecov`.
- **OQ-3**: Is the empty-path-tree seam (C-17) total (zero rows under `root` for every real
  engine scan) or do some engines emit depth-1 dirs? Static reading says total: every
  structured node's path begins with a root role name (`state.rs:625-634`). A one-line
  bridge test against `engine/tests/data/images/qemu-disk1.vhdx` would pin it.
- **OQ-4**: Did the user's one visible SMART drive return a genuinely empty ATA table
  (C-9) or a second-call drift (C-10)? A bridge-level log of both `cr_ui_health_read`
  results (available, attr_count, rc) would resolve.
- **OQ-5**: Whether `fsutil` `_popen` flashes a visible console in the installed (windowed)
  build — needs a run on a clean Windows host (C-12/C-24).
