# WIRING-AUDIT — every seam between the React UI and the cr_ui core (Agent D, Phase 0)

Scope: ui/src (React) → contract.ts → tauri-connector.ts (invoke) → commands/*.rs → bridge/abi.rs (cr_ui.h ABI v4) → engine; events back via bridge/events.rs → Tauri emit → frontend `listen()`. All citations file:line in /home/z/my-project/final-prod.

Surface-size truth first: the connector contract exposes **50 methods** (contract.ts:131-786) and command-surface.json declares **45 commands** — the "36 operations" figure quoted in the task and in error-codes.json:324/326 ("the 36 methods") is a stale count that no gate checks.

## Operation inventory (command → TS method → ABI → events → JSON shape → mismatch flags)

Key: `▸` = mismatch flag (detail below the table). ABI export lists abbreviated; full refs in contracts/command-surface.json:5-737.

| Rust command | Serves (contract.ts) | ABI exports | Events (emit) | Returns / flags |
|---|---|---|---|---|
| devices_list (devices.rs:427) | listDevices, getDeviceInfo | enumerate_disks/volumes, open_disk/volume, device_geometry, device_close | — | Device[] ▸1 ▸2 ▸3 ▸4 |
| devices_refresh (devices.rs:437) | refreshDevices | enumerate_disks/volumes | — | Device[] (same composer) |
| volume_unlock_dictionary (devices.rs:459) | unlockVolume | unlock_volume_with_dictionary | — | void; ERR_FVE_LOCKED→device-permission |
| scan_start (scan.rs:92) | startScan | abi_version, open_disk/volume(+_with_key), device_geometry, scan_create, scan_attach_device, scan_set_event_fn, scan_run, scan_repair_save_undo | scan-progress / consent-request / lost-candidate / scan-autosaved per scanId ▸5 | {scanId} ▸6 |
| scan_pause (scan.rs:501) | pauseScan | scan_pause | — | transient 'pausing' 2.5 s expiry (scan.rs:495) |
| scan_resume (scan.rs:519) | resumeScan | scan_resume | — | scan-not-paused refusal |
| scan_cancel (scan.rs:556) | cancelScan | scan_cancel | — | force-denies consent gates first (scan.rs:573) ▸7 |
| scan_state (scan.rs:588) | getScanState | scan_busy/stopping/telemetry | — | ScanState ▸8 ▸9 |
| scan_close (scan.rs:659) | internal (registry Drop) | scan_close | — | refuses while busy/jobs (scan.rs:668-681) |
| scan_save_cancel (scan.rs:696) | sessions.cancelSave | scan_save_cancel | — | save-scoped N-060 |
| results_tree / results_query / results_count / node_detail (results.rs) | getResultTree / queryResults / countResults / getNodeDetail | scan_snapshot, session_snapshot, snapshot_node/_count/_partial/_dfs | — | TreePage / ResultsPage / number / NodeDetail (G-7 live filter in Rust, type-maps.md:115-118) |
| preview_open / preview_thumbnail (preview.rs) | requestPreview / getThumbnail | preview_open_snapshot/_session, preview_size/metadata/read/unreadable, archive_entry_count/_at, preview_close, thumbnail_generate | — | PreviewPayload; hex bytes arrive as JSON array → Uint8Array normalize (tauri-connector.ts:174-180) |
| session_save (sessions.rs) | saveSession | scan_save_session, scan_get_stats | save-progress:<id> ▸5 | SessionMeta; refuses while running |
| sessions_list / session_meta (sessions.rs) | listSessions / getSessionMeta | session_list | — | W1a light rows |
| session_load (sessions.rs) | loadSession | session_load_with_events, session_get_info, session_stats | session-load-progress:<path> ▸5 | {scanId} |
| session_resume_last / session_resume (sessions.rs) | resumeLastSession / resumeSession | last_autosave_path, session_load_with_events, session_resume, scan_attach_device, scan_verify_identity, scan_set_event_fn, scan_run | consent-request ▸5 | {scanId[,path,targetId]} |
| recovery_start (recovery.rs) | startRecovery | recovery_create/set_destination/enqueue_* | recovery-progress:<jobId> | {jobId}; BF-14 options dropped |
| recovery_cancel / _state / _items / _report (recovery.rs) | cancelRecovery / getRecoveryState / getRecoveryItems / getRecoveryReport | recovery_cancel, recovery_report_json, recovery_get_stats | recovery-finished:<jobId> ▸5 | RecoveryJob / Item[] / Report |
| health_read (health.rs:19) | getHealth | health_read (base ABI only) | — | DiskHealth ▸10 ▸11 ▸12 ▸13 |
| consent_respond (repair.rs) | respondToConsent + respondToApplyConsent | none (registry-only) | — | echo token, both families |
| repair_dry_run / apply / undo_log / undo (repair.rs) | dryRunRepair / applyRepair / getUndoLog / undoRepairs | none | — | typed not-implemented (error.rs:105-115) |
| lost_partition_apply / _undo / _save_undo (lost_partition.rs) | applyLostPartition / undoLastApply / saveApplyUndo | open_disk, apply_lost_partition / apply_undo_last / apply_save_undo, device_close | apply-consent-request:<deviceId> | ApplyResult verbatim |
| apply_consent_pending (lost_partition.rs) | pendingApplyConsent | none | — | ApplyConsentEvent \| null |
| system_app_info (system.rs:131) | getAppInfo | abi_version, user_sign_count | — | AppInfo ▸14 |
| system_security_posture (system.rs:71) | getSecurityPosture | none (Win32 TokenElevation) | — | {administrator} |
| system_relaunch_elevated (system.rs:85) | relaunchElevated | none (ShellExecuteW runas + app.exit(0)) | — | refused→device-permission |
| system_load_user_signs (system.rs) | loadUserSigns | register_user_signs, user_sign_count | — | {count} |
| system_validate_destination / system_pick_directory / system_list_mounts / system_open_path (system.rs) | validateDestination / pickDirectory / listMountPoints / openPath | enumerate_volumes (validate, mounts) | — | DestinationValidation / string\|null / MountPoint[] / void |

Frontend event subscriptions (the only `listen()` calls in ui/src): scan-progress (tauri-connector.ts:233), consent-request (:249), recovery-progress (:348), apply-consent-request (:407). **5 of the 10 event families the bridge emits have no listener and no contract method**: scan-finished (scan.rs:455), save-progress (events.rs:332), lost-candidate (events.rs:304), session-load-progress (events.rs:92), scan-autosaved (events.rs:346), recovery-finished (events.rs:587). Completion/candidates/autosave reach the UI only indirectly (final progress sample + 500 ms scan_state poll + sessions query invalidation, queries/scan.ts:37-56).

### Mismatch flags

- ▸1 **kind degradation (BF-02)**: bridge emits `kind: removable!=0 ? "usb":"internal"` (devices.rs:215, 382) — v4 `cr_ui_disk_info` has no bus_type (abi.rs:182). DeviceKind 'sd'/'optical'/'virtual' (contract.ts:28) are never produced; 'sd' is unreachable though sd-card-page exists.
- ▸2 **status vocabulary**: DeviceStatus 'disconnected' (contract.ts:41) is never emitted by devices_list; the UI's disconnected branch (home-page.tsx:199-201, 522) is dead. Open failure → 'permission-denied' even when the cause is elevation (devices.rs:84-90).
- ▸3 **usage zeros**: unlettered volumes contribute 0 used/free (devices.rs:141-145) — capacity usedBytes can read 0 on fully-occupied unlettered disks.
- ▸4 getDeviceInfo is resolved client-side by re-invoking full devices_list (tauri-connector.ts:191-199) — N open-probes per call, no server-side resolution.
- ▸5 **dead-letter events** (list above).
- ▸6 **dataFoundBytes pinned 0 live (G-3)**: state.rs:107-109 ("no live found-bytes counter exists in telemetry — 0 live, exact value latched at completion"); type-maps.md:44. The mock DOES emit live bytes (mock-connector.ts:624, 661) — the UI's "Files Found: N (size)" was built against a live value. `currentPath` likewise always null (G-2, events.rs:193) — the "Searching: …" ticker (scan-progress-page.tsx:148-157) never renders on the real backend; mock supplies it (mock-connector.ts:630-633).
- ▸7 cancel is fire-and-forget at the seam: `scan_cancel` returns when the token is set (scan.rs:574-577); the UI navigates immediately (scan-progress-page.tsx:361-370) while the engine may drain for minutes (BF-25).
- ▸8 **ScanStage 'tree-build' never emitted**: event-mapping.md:32 promises pre-run→'volume-analysis', post-run→'tree-build' then 'done'; the bridge emits only 'volume-analysis' default (state.rs:130), quick-index/deep-walk/carve (events.rs:257-262), and terminal 'done' (events.rs:178). Doc-vs-impl drift; contract.ts:166 still carries 'tree-build'.
- ▸9 **failure.code double-mapped**: scan_state/scan-finished failure codes come from the bridge's own subset table `engine_failure_code` (scan.rs:465-473: only 0x50080001/3, 0x50060001, 0x50020007, 0x500C0001; everything else 'unknown'), NOT the frontend ENGINE_CODE_MAP (tauri-connector.ts:92-132). A run() failing with ERR_OPEN_FAILED (0x50020001) reports failure.code 'unknown' in ScanState but 'device-permission' had it crossed as a command rejection.
- ▸10 **HealthStatus enum drift**: contract.ts:597 `HealthStatus = 'good'|'caution'|'bad'` — the bridge can return `status:"unknown"` (health.rs:101, empty-but-readable ATA table per NG-9). health-page.tsx:100-104 maps unknown into the else/"bad" visual branch → honest "no data" renders as Bad.
- ▸11 **extra fields**: health JSON adds `transport` and `tableEmpty` (health.rs:151-152) not in contract.ts DiskHealth (additive, but undocumented drift).
- ▸12 threshold column always null (BF-03) — documented (health.rs:127-130).
- ▸13 **health unelevated mislabel**: engine open failure returns ERR_OPEN_FAILED with an "administrator rights required" message (cr_ui.cpp:4891-4894); it maps to 'device-permission' (tauri-connector.ts:93), not 'elevation-required' (0x500C0001 never returned by health_read) — no banner, generic toast.
- ▸14 **AppInfo.connectorMode required in TS** (contract.ts:730) but never sent (system.rs:133-146; type-maps.md:210 says "REMOVED") — runtime undefined on a required field.

## The scan chain (hop-by-hop) and the 75 % symptom

Hops (each verified; every hop is a real file:line):

1. **UI scan-start** — home-page.tsx:89-102 `beginScan` → `useStartScan` (queries/scan.ts:88-98)
   - `connector.scan.startScan(params)` → `invoke('scan_start', {targetId, pathScope: params.pathScope ?? null, bitlockerFvek: …})` (tauri-connector.ts:208-215)
   - success → `setActive({scanId, targetId})` (queries/scan.ts:94) → navigate `/scan/<targetId>/live?scanId=` (home-page.tsx:94).
2. **Rust preparation** (scan_start → spawn_blocking → scan_start_blocking, scan.rs:91-110, 116-332):
   - ABI gate: `cr_ui_abi_version() != 4` → refusal (scan.rs:124-130; startup log-only twin at lib.rs:100-109).
   - `resolve_target` (devices.rs:246-321): `disk-N` / `vol-L` / `vol-<disk>-<offset>` / virtual `vol-recycle-bin`+`vol-desktop` (implicit scopes :252-259).
   - scope normalize (scan.rs:46-80; disk-target letter check :141-157).
   - open: `cr_ui_open_volume` / `cr_ui_open_volume_with_key` (FVEK, :174-190); FVE-locked → device-permission :191-197; `CR_UI_ERR_ELEVATION_REQUIRED` → engine code crosses :198-205.
   - geometry → `bytesTotal` (scan.rs:212-216, 288).
   - zeroed `CrUiScanConfig` :229-241 — stage_mask 7 comes from `ScanConfig` defaults `run_quick/run_deep/run_carve = true` (cr_scan.h:197-200); **weights default 40/35/25** (cr_scan.h:72-75); `whole_disk` set for disk targets :230; `pre_scan_repair` volume-only (NG-21) :231; `pre_scan_check=1` :232; `autosave_dir` = sessions store (W1c) :233-234; consent sink armed :235-241.
   - `cr_ui_scan_create` :244-247 → `cr_ui_scan_attach_device` :249-252.
   - **bounded(512) channel + `scan_event_sink` subscribed BEFORE run** :257-268 (the ordering event-mapping.md:6-8 mandates) → registry insert :321-325 → `{scanId}` returned :109.
3. **Thread launch** (scan.rs:339-460): `spawn_scan_emitter` (events.rs:242-402) + a dedicated run thread (scan.rs:362) calling `cr_ui_scan_run` :364.
4. **Engine run** — `ScanController::run` (cr_scan.cpp:1101):
   - `tel_stage_begin_` fires CR_EVT_SCAN_STAGE per stage (cr_scan.cpp:450-466).
   - `tel_publish_` (cr_scan.cpp:485-520) → `overall_permille = weighted_progress(weights 40/35/25, …)` (cr_scan.cpp:131-146, monotonic clamp :518-519).
   - `emit_progress_event_` coalesces: fires only when overall permille advances (cr_scan.cpp:623-642) — **≤1000 progress events per scan plus stage flips**.
   - `emit_boundary_events_` (FilesFound + ErrorRange deltas, cr_scan.cpp:644+); live counters refreshed per event (N-166, cr_scan.cpp:567-575).
   - heartbeat watchdog thread: CR_EVT_HEARTBEAT every 5 000 ms **only when no CR_EVT_SCAN_PROGRESS arrived in the window**, payload = standing truth, never a permille advance (cr_ui.cpp:1137-1189; contract at cr_ui.h:163-173).
5. **Shim → bridge**: `scan_event_trampoline_` (cr_ui.cpp:1066-1104) copies the engine POD field-for-field into `cr_ui_event` → the bridge sink `scan_event_sink` does one `try_send` of the POD copy (events.rs:46-53; channel-full = coalescing, never blocks the worker).
   - Rust `CrUiEvent` (abi.rs:125-153) is field-order/-layout-identical to C `cr_ui_event` (cr_ui.h:225-272) — verified member-by-member, incl. the 4-byte pad before `checkpoint_index` implied by repr(C).
6. **Emitter thread** (events.rs:250-401) — the ONLY writer of `ScanLive` (state.rs:98-99):
   - CR_EVT_SCAN_STAGE :256-264 → stage map 0→'quick-index', 1→'deep-walk', 2→'carve' (0xFFFFFFFF = standing stage kept).
   - CR_EVT_SCAN_PROGRESS :266-273 → `stageProgress = stage_permille/1000`; `overall = overall_permille/1000` clamped monotonic.
   - CR_EVT_FILES_FOUND :274-277 → `files_found = files+deleted+carved` (cumulative).
   - CR_EVT_ERROR_RANGE :278-285 → bad_sectors push (cap 4096, state.rs:36) + errors = cumulative count.
   - CR_EVT_LOST_CANDIDATE :287-314 → N-061 geometry join via `logging::take_candidate_geometry(lba)` + emits `lost-candidate:<scanId>`.
   - CR_EVT_HEARTBEAT :326-329 → **liveness only** (`last_heartbeat_ms`; no scan-progress emit).
   - CR_EVT_SAVE_PROGRESS :330-336 / CR_EVT_FS_CHECK :337-341 / CR_EVT_AUTOSAVED :342-350 / CR_EVT_REPAIR :315-325.
   - on any emit-worthy event: telemetry poll (events.rs:357-387) refreshes `bytes_scanned`, the files sum, throughput/ETA, and clears the pause/resume transient → `compose_scan_progress` (events.rs:155-195) → `app.emit("scan-progress:<scanId>")` :398.
7. **Terminal** (run thread tail, scan.rs:379-459): stats fetch :380-382 (`files_found = files+deleted+carved`), undo-log persist when repair applied :389-404, status mapping (OK→completed, 0x50020005→cancelled, else failed+`engine_failure_code`) :406-414, final snapshot freeze + exact `data_found_bytes` from snapshot metas :418-426, Terminal latch :429-443, **final progress sample** :446-453 + `scan-finished:<scanId>` :455-458 (unheard, ▸5).
8. **Frontend receive**: `listen('scan-progress:'+scanId)` (tauri-connector.ts:229-239) → per-scan external store (queries/scan.ts:22-84):
   - store sets `current` on every event; invalidates scanState on status change (:38-40) and tree/results/count/sessions on terminal (:42-50) or each 10 % bucket (:51-56).
   - primes once from `getScanState` when no event has arrived (:60-68) — the subscribe-race backstop.
   - parallel 500 ms `useScanState` poll while status is running/starting/stopping (queries/scan.ts:100-123) — reads the live cell through scan_state (scan.rs:588-656), so the chain survives long no-event windows **by polling, not by events**.
9. **Render** (scan-progress-page.tsx):
   - `progress = useScanProgress(scanId)` :66; `status` :79; `overall = progress.overallProgress` :80; `phase = scanPhaseOf(progress.stage ?? 'volume-analysis')` :83.
   - gauge `value={overall}` :161-166.
   - line 1 label: `t(phase === 'quick' ? 'scan.quickScanning' : 'scan.advancedScanning', {percent: formatPercent(overall)})` :174-181 (en.ts:154-155 "Quick/Advanced Scanning {{percent}}") + `t('scan.filesFoundWithSize', {count: filesFound, size: formatBytes(dataFoundBytes)})` :183-188 (en.ts:153 "Files Found: {{count}} ({{size}}").
   - line 2: "Remaining time" (etaSeconds>0, :246-252) + `t('scan.readingSector', {current: sectorsScanned, total: sectorsTotal})` :257-262 where `sectorsScanned = Math.floor(bytesScanned/512)` :124-125.
   - "Searching: <path>" ticker :148-157 — never renders (currentPath always null, ▸6).

**Where each observed number comes from:**

- **75 %** = `overallProgress` = `evt.overall_permille/1000` (events.rs:267-271) = `weighted_progress(40,35,25)` — when quick AND deep complete (permille 1000 each, tel_stage_end_ cr_scan.cpp:468-483) the overall is exactly `(40·1000+35·1000+25·0)/100 = 750‰ = 75.0 %`. 75 % is precisely the quick+deep boundary entering carve. The forensic corpus recorded the same boundary: "75%→76.1%, 197 items" (tools/forensic/drive-ui.mjs:648-649).
- **"advanced scanning"** = stage 1/2 → `scanPhaseOf`→'advanced' (contract.ts:171-175) → the i18n label above.
- **"files found 21"** = `filesFound` ← telemetry `files_found+deleted_found+carved_found` (events.rs:361) ← engine live tree counters (cr_scan.cpp:567-575, N-166). It is a **real early-scan count**, not a mock fixture — the mock never runs in shipped builds (connector/index.ts:23-26, VITE_CONNECTOR default 'tauri').
- **"(0 B)"** = `dataFoundBytes` — pinned **0 during the whole live scan by design** (state.rs:107-109 G-3; type-maps.md:44); the mock updates it live (mock-connector.ts:624,661) so the screen was designed expecting live bytes.
- **"no sector progress"** = `bytesScanned` ← telemetry `bytes_scanned`, which is **carve-true-bytes only** (cr_scan.h:614; emit_event_ zeroes bytes_done unless stage==2, cr_scan.cpp:596-597). During quick+deep the Reading-sector line shows 0/N; during a carve stall it freezes.
- **"stuck"** = BF-22 (worklog/BEHAVIOR-FINDINGS.md:36): the carve matcher grinds free space in memory with reads idle. Seam behavior during the stall: heartbeats continue every 5 s → `deviceConnected` stays true (events.rs:142-152, DISCONNECT_AFTER_MS=10 s, state.rs:833); the 500 ms scan_state poll keeps returning busy=1/'running' from the live cell (queries/scan.ts:105-121; scan.rs:595-618) — the poll survives the no-event window, so the UI does not hang; it faithfully renders a stalled engine with **no stall/watchdog indicator anywhere** (the og bumped coarse progress on its watchdog; we deliberately show standing truth, cr_ui.cpp:1106-1115).

**Candidate break points for symptom (a), ranked:**

1. **Engine BF-22 carve stall** (primary, recorded): identical on ABI v2 and v4 engines; environmental/known-perf. Not a wiring regression; the wiring's job is to *surface* it, which it currently doesn't (no "stalled" state).
2. **G-3 live found-bytes = 0** (bridge, designed-in): "Files Found: 21 (0 B)" reads like a broken counter to users. Fix path exists: `CrUiScanStats.bytes_found` is stamped live from `tree_.total_logical()` at every count refresh (cr_scan.cpp:522-529, N-167) — the bridge already polls stats at emit time (events.rs:357-360) but `CrUiTelemetry` (abi.rs:262-277) carries no bytes_found; polling `cr_ui_scan_get_stats` instead would give live bytes.
3. **bytesScanned carve-only** (engine telemetry design): "Reading sector: 0/N" through quick+deep and frozen in carve — indistinguishable from "no sector progress" even on a healthy scan until carve engages.
4. **No stall surfacing** (UI): heartbeats are distinguishable from progress at the bridge (events.rs:326-329) but the payload doesn't expose "alive but stalled"; ETA freezes (recomputed only on events, events.rs:362-377); device-lost modal requires status==='paused' (scan-progress-page.tsx:82) so a running stall shows nothing.
5. **Subscribe race (cosmetic contributor)**: `listen()` registers asynchronously after `startScan` resolves; events before registration are dropped and only recovered by the async getScanState prime (queries/scan.ts:60-68) — a scan whose quick+deep finish inside that window appears to "jump straight to 75 %".

## The device chain — "only 1 of 3 disks"

Hops:

1. useDevices (queries/devices.ts:5-10, no refetchInterval — enumeration is pull-only) → `invoke('devices_list')` (tauri-connector.ts:189).
2. devices.rs:107-232 `list_devices` (spawn_blocking, devices.rs:427-434) → `enumerate_disks()` + `enumerate_volumes()` (bridge/util.rs:156-184, two-call protocol with exact-size retry semantics).
3. Engine `cr_ui_enumerate_disks` (cr_ui.cpp:1929) → `enumerate_disks_impl` (cr_ui.cpp:1824-1879) → `win::enumerate_disks(200, list)`:
   - Stack 1: numbered `\\.\.PhysicalDrive0..199` loop (cr_enumerate.cpp:139-153) — `open_disk_for_query` :98-113 retries a 0-access metadata open on ACCESS_DENIED/SHARING_VIOLATION (N-071) so busy disks stay listed (:110).
   - Stack 2: cfgmgr32 GUID_DEVINTERFACE_DISK pass :156-158, 168-212 (device number via IOCTL_STORAGE_GET_DEVICE_NUMBER).
   - Pure merge :158 → merge_disk_enumeration (cr_enumerate.h:228-255).
4. Volumes: `cr_ui_enumerate_volumes` → `win::enumerate_volumes` (cr_enumerate.cpp:214-254): GetLogicalDriveStringsW (cap 64) + GetDriveTypeW — **only DRIVE_FIXED and DRIVE_REMOVABLE letters** (:225); extents → first extent's disk/offset/bytes :232-240; bitlocker::is_fve_volume :248.
5. Row composition (devices.rs:121-230): per-disk open probe :129-134 → status; per-letter GetDiskFreeSpaceExW usage :53-75; FVE volume probe :149-158; kind from removable :215; label fallback chain :186-201; canScanWholeDisk=true :228.

Filters/dedups on the path (all verified): engine merge dedups **by disk number and instance path only** (cr_enumerate.h:228-255) — identical-model/serial disks do NOT collapse (proven by test_enumerate_merge.cpp:58-116); the bridge adds no dedup/filter; the only UI-side filter is `kind !== 'virtual'` (use-physical-devices.ts:21) plus the Home view split below.

**Candidate causes ranked:**

1. **Home default view shows internal only** — home-page.tsx:80-83: `visible = showRemovable ? removable : internal` where `showRemovable = params.has('media')` (:77). With ▸1 (BF-02) every disk whose STORAGE_DEVICE_DESCRIPTOR BusType is 7/11/12 (USB attached, SD, MMC — cr_enumerate.h:89) is `kind:'usb'` and is **excluded from the default list**. Two of three disks behind USB enclosures/card readers → exactly 1 row visible. The disks exist in the payload; the view hides them (they appear only via the media/SD-card routes).
2. **Open-failure absence** — cr_enumerate.cpp:98-113: a disk that fails both the GENERIC_READ open and the 0-access metadata open "stays absent" (:96-97). Exclusive locks (AV/imaging) survive via the busy flag; policy-blocked raw devices vanish entirely.
3. **Permission mislabel (not absence)** — devices.rs:129-134 probes `cr_ui_open_disk` per disk; unelevated, non-system raw opens fail → `status:'permission-denied'` rows render dimmed (▸2/▸13) — could be perceived as "missing/disks don't work" but rows are present.
4. **Volume-less disks** — cr_enumerate.cpp:224-225 drops DRIVE_UNKNOWN/NO_ROOT_DIR/CDROM/RAMDISK letters; a disk with only unlettered/Raw/BitLocker-locked volumes still lists as a disk row (label falls back to model, devices.rs:190-201) with zero scannable volume rows.
5. Not a cause: identical-model collapse (ruled out above); 200-disk cap (cr_ui.cpp:1828); letters-buffer cap 64 (cr_enumerate.cpp:216-218).

## The health chain — visible console

The hop chain:

1. health-page.tsx:51 `useHealth` (queries/health.ts:5-13, staleTime 60 s, retry:false) → `invoke('health_read', {deviceId})` (tauri-connector.ts:361-363).
2. health.rs:19-157 (`spawn_blocking`, two-call attrs :34-67; `disk-N` id parse :21-29).
3. Engine `cr_ui_health_read` → `health_read_impl` (cr_ui.cpp:4879-4958):
   - `CreateFileW("\\.\PhysicalDrive%u", GENERIC_READ|GENERIC_WRITE)` :4887 — **requires an elevated token**; unelevated → ERR_OPEN_FAILED with "administrator rights required" text :4891-4894 (▸13).
   - `win::health::read_smart` :4897 (ATA table, cr_health.cpp:190-262).
   - `read_nvme_health` :4926 — IOCTL_STORAGE_QUERY_PROPERTY, NVMe log page 2 (cr_health.cpp:264-299).
   - `looks_like_ssd` :4946 (seek-penalty probe, cr_health.cpp:325-342).
   - **`win::health::query_trim_state(trim)` at cr_ui.cpp:4948 → `_popen("fsutil behavior query disabledeletenotify 2>nul", "r")` at cr_health.cpp:306**, first `=`-match parsing :312-320.

- **Console visibility confirmed engine-side**: `_popen` spawns `cmd.exe`; from a GUI-subsystem (windows) exe every child console process flashes a window. It fires **once per health_read** (per disk), plus once per `cr_ui_enumerate_disks_ex` call with CR_ENUM_FLAG_TRIM (cr_ui.cpp:1843-1846) — the bridge's enumeration calls the plain `cr_ui_enumerate_disks` (util.rs:158) with flags=0, so **only the health path pops the console**. No UI seam defect beyond invoking it.
- **No timeout/lifecycle guard at the seam**: health.rs runs on `spawn_blocking` with no deadline; a hung fsutil blocks that worker forever (frontend sees a pending query). First-match parsing is og-parity (cr_health.cpp:319 "first match wins").
- Note: the same `_popen` is the engine's only trim source; `trimEnabled` (health.rs:147) is derived from it — disabling the console (CREATE_NO_WINDOW) is an engine-side fix, not a bridge one.

## Lifecycle & threading

- **Registry ownership** (state.rs:40-327):
  - `BridgeState.scans: HashMap<scanId, Arc<ScanEntry>>`; ids minted `scan-N` (state.rs:457); `sessions_dir` at state.rs:572.
  - `ScanKind::Live{scan, device, owns_device}` (state.rs:47-60) — resumed scans borrow the loaded session's device (pinned via the jobs list; scan_close refuses while borrowers live, scan.rs:675-681).
  - `ScanLive` has exactly ONE writer: the emitter thread (state.rs:98-99); `scan_state` only reads it under the lock.
  - The run thread latches `Terminal` exactly once (state.rs:71-83; scan.rs:429-443) and is also the only emitter of `scan-finished` (scan.rs:446-458).
  - Drop order enforced per the engine lifetime rules: unsubscribe event fn → free the leaked sink Sender box → snapshot cache (own Drop → `cr_ui_snapshot_close`) → `cr_ui_scan_close` → `cr_ui_device_close` (state.rs:291-327).
- **Window close / app exit mid-scan — SEAM GAP**: lib.rs:29-117 registers no `RunEvent::ExitRequested`/`Exit` handler and no window-close hook; Tauri's default tears the process down with the window, killing the detached run thread, the emitter, and the engine's worker pool mid-scan:
  - No cancel is propagated on close; nothing is saved — W1c auto-save fires only when `run()` completes (cr_ui.h:210-223), so close-mid-scan persists NOTHING.
  - G-13 checkpoint files exist engine-side (cr_ui.cpp:870-873 ck_path) but nothing at this seam reloads them; `session_resume` is user-driven only.
  - The consent gate's documented unblock set is "answered / cancelled / process exits" (consent.rs:25-27) — process death is the third path, i.e. teardown relies on dying.
- **Event emission threading**:
  - Sinks run on engine worker threads and only `try_send` (events.rs:46-53, 56-66); bounded channel 512 (scan.rs:257).
  - One emitter thread per scan (events.rs:242-402); one per recovery job with a 250 ms recv timeout keep-alive loop (events.rs:410-516).
  - Terminal events are emitter-NEVER: the run/process threads latch state first, then emit the final sample + finished event themselves (events.rs:13-15; scan.rs:446-458; recovery events.rs:570-591).
  - Session-load progress fires on the CALLING thread and emits directly, no channel hop (events.rs:68-102; cr_ui.h:195-209).
- **Cancellation propagation** (UI → bridge → engine):
  - `scan_cancel` (scan.rs:556-581) force-denies the scan's pending consent gates FIRST (:573 — §4.5: a pending consent blocks the run thread inside the callback; the engine observes the cancel token only after it returns), then `cr_ui_scan_cancel`.
  - Engine observes the token only at stage checkpoints — **unbounded inside the carve matcher (BF-25, BEHAVIOR-FINDINGS.md:39)**: the UI navigates to results on invoke success (scan-progress-page.tsx:361-370) while run() drains; 'cancelled' (and the partial-results banner gated on status==='cancelled') latches only when run() exits (scan.rs:406-411); saves are refused while busy (command-surface.json:268).
  - Recovery cancel mirrors this: committed items stay committed; terminal cancel is a truthful no-op (command-surface.json:376-388).
- **Pause/resume**: token calls + bridge transient state ('pausing'/'resuming', 2 500 ms expiry, scan.rs:494-497) cleared when telemetry catches up (events.rs:378-385). The scan_state poll keeps 'stopping' alive deliberately (queries/scan.ts:105-121 — the v0.8.0 forensic fix for the frozen 'stopping' query).
- **Consent lifecycle**: the engine asks at most once per scan, ON the run thread, before any engine read (cr_ui.h S-008; consent.rs:1-41); the gate blocks with NO user-facing timeout (consent.rs:87-96); re-hydration via `scan_state.pendingConsent` (scan.rs:619-622) covers subscribe-late and navigate-away; the run tail closes survivors and frees the pipe (scan.rs:374-377); an approved repair's undo log is persisted by the run tail (scan.rs:389-404).
- **Session save/resume wiring**: `session_save` refuses while running; `scan_save_cancel` is save-scoped (N-060, scan.rs:696-719); `session_resume` re-attaches by model+serial (best-effort) and verifies identity (ERR_BAD_CHECKSUM → session-corrupt, command-surface.json:331-350); auto-saves land because `cfg.autosave_dir` is set at scan_start (scan.rs:233-234) — but `scan-autosaved` is unheard (▸5) and the sessions list refreshes only via the terminal-status query invalidation (queries/scan.ts:49).

## Error mapping

- Wire shape: `BridgeError {code: u32, bridgeCode: string|null, message}` (error.rs:51-60, camelCase serde) → `toConnectorError` (tauri-connector.ts:134-153): bridgeCode direct, else ENGINE_CODE_MAP (:92-132). UI renders reasonKey only (REASON_KEYS :62-89 — all 26 codes covered).
- **Codes the frontend cannot distinguish / mislabels:**
  - `preview-timeout` and `preview-missing-data` share reasonKey 'preview.reason.damaged' (tauri-connector.ts:81-82) — indistinguishable in UI copy.
  - ERR_TRUNCATED (0x50030004), ERR_RANGE_OUT_OF_BOUNDS (0x50020004), ERR_CYCLE_LIMIT/DEPTH_LIMIT, ERR_NO_MEMORY, ERR_IMAGE_CORRUPT, ERR_INTERNAL all → 'unknown' — six engine codes collapse into one bucket.
  - **ERR_BAD_PARAMETER (0x50030003) hard-maps to 'scan-scope-invalid' regardless of surface** (tauri-connector.ts:102) — a bad-parameter from health/preview/enumeration surfaces as a scan-scope toast. The engine's context refinement cannot cross a numeric-only channel.
  - ERR_OPEN_FAILED → 'device-permission' vs bridge-refined device-gone (devices.rs:262-308): two truths for one code; the elevation sub-case (▸13) never gets the elevation banner.
  - ▸9: ScanState.failure.code / scan-finished failure use the bridge's 4-entry subset table (scan.rs:465-473) — disagrees with the 38-entry TS table for every code outside the subset (e.g. ERR_OPEN_FAILED → 'unknown' here, 'device-permission' there).
  - ERR_CANCELLED never crosses as an error (translated to status 'cancelled', scan.rs:406-411) — correct per invariant (error-codes.json:405).
- validate.mjs keeps the vocabulary closed (checks 1-2) and it is green; every mismatch above is *inside* green rows.

## What validate.mjs cannot catch

contracts/validate.mjs is a **name/vocabulary linter, not a contract checker**. Concretely it validates: every cr::Err has an error-codes row (check 1, :39-55), every uiCode exists in ConnectorErrorCode and vice versa (check 2, :57-77), every CR_EVT_* define appears in event-mapping.md's table (check 3, :79-88 — a regex over markdown!), manifest well-formedness/duplicate names (check 4, :90-97), every manifest command has a Rust fn and vice versa (check 5, :108-116), every `invoke('name'` literal is in the manifest (check 6, :118-130).

It cannot catch any of:

1. **Runtime payload shape drift** — field names/types/optionality of the JSON vs contract.ts (▸10 'unknown' outside HealthStatus; ▸14 missing AppInfo.connectorMode; ▸11 extra health fields; a renamed field would pass).
2. **Value semantics** — dataFoundBytes pinned 0 live vs mock-live (G-3), bytesScanned carve-only, etaSeconds 0-vs-null, units (permille vs fraction) — all numeric contracts are unchecked.
3. **Event listeners** — nothing verifies the frontend subscribes to what the bridge emits; 5-6 event families are dead letters (▸5) and the gate is green.
4. **Doc-vs-impl drift inside contracts/** — event-mapping.md:32 promises 'tree-build' (▸8); the md table only needs to *contain* the event name.
5. **Duplicate translation tables** — engine_failure_code (scan.rs:465) vs ENGINE_CODE_MAP drift silently (▸9).
6. **Dead enum branches** — DeviceStatus 'disconnected', DeviceKind 'sd'/'optical'/'virtual', ScanStage 'tree-build', SmartAttribute status 'unknown' — none ever produced.
7. **Count claims** — "36 operations" vs the real 45 commands/50 methods.
8. **Lifecycle/threading regressions** — window-close mid-scan, cancel drain windows, consent blocking, threading classes — out of scope of every check.
9. **Mock/real behavioral divergence** — the test double (mock-connector.ts:624,630-633,661) satisfies the same TS types while differing observably (live bytes, live currentPath) — exactly the class that produced the "(0 B)" and missing-ticker symptoms.

## Bottom line (seam verdict per live defect)

- (a) scan: engine-truth stall (BF-22) faithfully rendered; the 75 % is the 40+35 weight boundary; "(0 B)" and the dead ticker are **real bridge-vs-contract divergences (G-3/G-2) the mock masked**; no stall indicator exists anywhere on the seam.
- (b) devices: rows are complete at the bridge; the **Home view's internal-only filter + BF-02 kind degradation** hides removable-bus disks — the single most likely cause of "1 of 3".
- (c) console: engine `_popen` (cr_health.cpp:306) fired per health_read; seam adds only the missing timeout.
- (d) elevation: the designed user-mode posture (installer admin at recoverywizard.nsi:151, app asInvoker, banner + relaunchElevated system.rs:85-124) — not a wiring bug; but health/enumeration failures mislabel as device-permission instead of elevation-required (▸13).
