# WIRING-AUDIT — Agent B (small-bug session, full refresh)

Repo: `/home/z/my-project/final-prod`. RESEARCH ONLY — no file modified except this one.
Method: every chain read link-by-link in source (no runtime available in this sandbox; every claim below cites the file:line actually read).

Stack audited: React 19 UI (`ui/src`) → TypeScript connector contract (`ui/src/connector/contract.ts`, **71 ops across 12 domains** — 59 request ops + 12 event subscriptions; the session brief's "36 ops / 9 domains" is stale) → `ui/src/connector/tauri/tauri-connector.ts` (`invoke`/`listen`) → Tauri commands (`ui/src-tauri/src/commands/*.rs`, registered in `ui/src-tauri/src/lib.rs:43-123`) → cr_ui C ABI (`ui/src-tauri/src/bridge/abi.rs`, 92 bound exports of the 92 in `engine/include/cr/cr_ui.h`) → engine impl (`engine/src/ui/cr_ui.cpp`, 5502 lines) → engine modules. Boundary truth: `contracts/command-surface.json` (59 commands), `contracts/event-mapping.md` (12 `CR_EVT_*`), `contracts/error-codes.json`.

---

## Method

- **Chains traced: 86.** 59 command chains (57 invoked by the TS connector + 2 bridge-internal: `scan_close`, `image_target_list`), 12 event chains (emit → listen → hook → component, incl. unmount cleanup), 4 window-control chains (minimize / toggle-maximize / close / guard-destroy) + title-bar drag + dblclick-maximize, 3 stop/pause/resume chains traced down to the engine's loop-head cancellation points, 8 route transitions + title-bar menu items.
- Cross-checks run: `node contracts/validate.mjs` → **green** (59 commands impl'd Rust-side, 57 invoked TS-side, 41 error codes, 12 events). `python3 tools/gen_abi_layouts.py` regeneration → **byte-identical** to the checked-in `abi_layouts.rs` and the header checksum gate (`engine/tests/abi_layout_dump.header.sha` matches) — the ABI struct layout gate (BF-ABI-01) is in sync; the previous audit's bus_type/power_cycles_nvme layout drifts are **fixed** (`abi.rs:207-212`, `abi.rs:755-758`).
- Registration: all 59 manifest commands are registered in the `invoke_handler` (lib.rs:43-123); every TS `invoke('<name>')` matches a registered command, and every camelCase JS arg maps to the Rust snake_case param (Tauri 2 convention).
- Every event emitted by the bridge has a TS listener and vice versa (see Clean chains).
- Mock-vs-real divergence was checked on every defect candidate (this repo's test lane runs the mock in a browser, so mock-green/real-red breaks escape CI — two confirmed instances below).

## Chain map (op → Tauri command → cr_ui ABI → engine fn)

Status: ✔ = full chain traced and real. `gap` = intentional typed not-implemented. `bridge` = bridge-side logic (no engine call, by design).

| # | Connector op (contract.ts) | Tauri command (commands/) | cr_ui ABI → engine impl | Status |
|---|---|---|---|---|
| devices.listDevices / getDeviceInfo (:142-149) | devices_list (devices.rs:492) | enumerate_disks / enumerate_volumes / open_disk / open_volume / device_geometry / device_close → cr_ui.cpp impls | ✔ |
| devices.refreshDevices (:149) | devices_refresh (devices.rs:510) | enumerate_disks / volumes | ✔ |
| devices.importImageFile (:160) | image_target_import (images.rs:124) | bridge registry + file probe (open_image at scan time) | ✔ bridge |
| devices.unlockVolume (:168) | volume_unlock_dictionary (devices.rs:538) | cr_ui_unlock_volume_with_dictionary → cr_bitlocker.cpp | ✔ (FVEK arm never called by UI — **SB-B-03**) |
| scan.startScan (:312) | scan_start (scan.rs:93) | abi_version, open_disk/open_volume(_with_key)/open_image, device_geometry, scan_create, scan_attach_device, scan_set_event_fn, run on dedicated thread, repair_save_undo in tail | ✔ |
| scan.pauseScan (:313) | scan_pause (scan.rs:664) | cr_ui_scan_pause → PausableToken::pause (cr_device.h:176) | ✔ |
| scan.resumeScan (:314) | scan_resume (scan.rs:682) | cr_ui_scan_resume → PausableToken::resume | ✔ |
| scan.cancelScan (:315) | scan_cancel (scan.rs:719) | cr_ui_scan_cancel → token.cancel() + consent force-deny first (state.close_scan_consents, scan.rs:736) | ✔ (see stop-path note) |
| scan.getScanState (:316) | scan_state (scan.rs:751) | cr_ui_scan_busy / scan_stopping / scan_telemetry + registry | ✔ |
| scan.respondToConsent (:344) | consent_respond (repair.rs:32) | registry gate (consent.rs:247) | ✔ bridge |
| scan.getScanTelemetry (:351) | scan_telemetry (diagnostics.rs:67) | cr_ui_scan_telemetry / get_stats / checkpoint_count | ✔ |
| scan.getScanAssistantStats (:357) | scan_assistant_stats (diagnostics.rs:164) | cr_ui_scan_assistant | ✔ |
| scan.listCarveFormats (:362) | carve_catalog (diagnostics.rs:221) | cr_ui_carve_format_count / carve_format_info | ✔ |
| scan.getCarveTally (:367) | scan_carve_tally (diagnostics.rs:253) | cr_ui_scan_carve_tally(_count) + carve_format_info | ✔ |
| scan.getDiskMap (:374) | disk_map (diagnostics.rs:359) | cr_ui_disk_map / disk_map_free / session_get_info | ✔ |
| results.getResultTree (:437) | results_tree (results.rs:940) | scan_snapshot / session_snapshot / snapshot_node_count / partial / dfs / node | ✔ |
| results.queryResults (:443) | results_query (results.rs:535) | snapshot + bridge-side metadata filter | ✔ |
| results.getNodeDetail (:451) | node_detail (results.rs:957) | cr_ui_snapshot_node | ✔ |
| results.countResults (:454) | results_count (results.rs:562) | snapshot + cache | ✔ |
| results.regradeNodes (:466) | session_regrade (diagnostics.rs:601) | cr_ui_session_regrade | ✔ |
| preview.requestPreview (:519) | preview_open (preview.rs) | preview_open_snapshot / open_session / size / metadata / read / unreadable / archive_entry_count / at / close | ✔ |
| preview.getThumbnail (:537) | preview_thumbnail (preview.rs) | thumbnail_generate (+ snapshot twin), preview_open_*, preview_read | ✔ |
| sessions.saveSession (:547) | session_save (sessions.rs:85) | cr_ui_scan_save_session + scan_get_stats | ✔ engine — **but bridge refuses while busy → SB-B-01** |
| sessions.cancelSave (:550) | scan_save_cancel (scan.rs:857) | cr_ui_scan_save_cancel (save-scoped flag) | ✔ |
| sessions.listSessions (:551) | sessions_list (sessions.rs:147) | cr_ui_session_list (light rows) | ✔ |
| sessions.loadSession (:555) | session_load (sessions.rs:357) | cr_ui_session_load_with_events + get_info + stats | ✔ (progress event name → **SB-B-04**) |
| sessions.onSessionLoadProgress (:558) | — (listen) | session-load-progress:<path> emitted from the calling thread (events.rs:97-122) | ⚠ SB-B-04 |
| sessions.getSessionMeta (:563) | session_meta (sessions.rs:373) | cr_ui_session_list row | ✔ |
| sessions.resumeLastSession (:569) | session_resume_last (sessions.rs:412) | session_last_autosave_path + load_with_events + info + stats | ✔ |
| sessions.resumeSession (:577) | session_resume (sessions.rs:454) | load_with_events, session_resume, scan_attach_device, scan_verify_identity, scan_set_event_fn, scan_run | ✔ |
| recovery.startRecovery (:820) | recovery_start (recovery.rs:52) | recovery_create / set_destination / enqueue_node / subtree / snapshot / subtree_snapshot / deleted_tree(_snapshot) / queue_count; process on its own thread | ✔ |
| recovery.previewDeletedTreeReconstruction (:795) | recovery_preview_deleted_tree (recovery.rs) | cr_ui_reconstruct_deleted_preview(_snapshot) | ✔ |
| recovery.cancelRecovery (:811) | recovery_cancel (recovery.rs) | cr_ui_recovery_cancel (engine writer loop honors it) | ✔ |
| recovery.getRecoveryState / getRecoveryItems / getRecoveryReport (:802-818) | recovery_state / recovery_items / recovery_report | registry + cr_ui_recovery_report_json / recovery_get_stats | ✔ |
| health.getHealth (:1164) | health_read (health.rs:20) | cr_ui_health_read_ex2 (base fallback) | ✔ |
| repair.dryRunRepair / applyRepair / getUndoLog / undoRepairs (:1252-1259) | repair_dry_run / apply / undo_log / undo (repair.rs:41-65) | none — typed `not-implemented` refusal (documented G-1 gap) | `gap` (documented) |
| lostPartition.applyLostPartition (:1220) | lost_partition_apply (lost_partition.rs:53) | open_disk, apply_lost_partition (consent sink), device_close | ✔ |
| lostPartition.undoLastApply (:1224) | lost_partition_undo (lost_partition.rs:192) | open_disk, apply_undo_last, device_close | ✔ |
| lostPartition.saveApplyUndo (:1227) | lost_partition_save_undo (lost_partition.rs:213) | open_disk, apply_save_undo, device_close | ✔ |
| lostPartition.pendingApplyConsent (:1232) | apply_consent_pending (lost_partition.rs:234) | registry | ✔ bridge |
| system.getAppInfo (:1290) | system_app_info (system.rs:41) | cr_ui_abi_version + user_sign_count | ✔ |
| system.loadUserSigns (:1301) | system_load_user_signs (system.rs:119) | cr_ui_register_user_signs + user_sign_count | ✔ |
| system.validateDestination (:1308) | system_validate_destination (system.rs:210) | enumerate_volumes + Win32 GetDiskFreeSpaceExW + write probe | ✔ |
| system.pickDirectory (:1321) | system_pick_directory (system.rs:285) | dialog plugin (blocking_pick_folder on the pool) | ✔ |
| system.pickImageFile (:1344) | system_pick_image_file (system.rs:312) | dialog plugin + the 5-family filter | ✔ |
| system.listMountPoints (:1350) | system_list_mounts (system.rs:257) | enumerate_volumes + Win32 | ✔ |
| system.openPath (:1352) | system_open_path (system.rs:330) | opener plugin (open / reveal) | ✔ |
| system.getDeviceTransport (:1356) | device_transport (diagnostics.rs:534) | open_disk/volume, device_transport, last_transport_error, device_close | ✔ |
| imaging.startImageCreation (contract.ts ~:1125) | image_create_start (imaging.rs:242) | cr_ui_image_create_ex (cancel flag + progress fn) + enumerate_disks/volumes pre-gates | ✔ |
| imaging.getImageState | image_create_state (imaging.rs:550) | registry + filesystem `.partial` probe | ✔ |
| imaging.cancelImageCreation | image_create_cancel (imaging.rs:606) | caller-owned volatile flag → engine producer loop head | ✔ |
| maintenance.runFsck (:1383) | volume_fsck (maintenance.rs:83) | cr_ui_fsck_volume → cr_fsck.cpp (CREATE_NO_WINDOW spawn) | ✔ |
| maintenance.turnOffBitlocker (:1390) | volume_bitlocker_off (maintenance.rs:154) | cr_ui_turn_off_bitlocker | ✔ |
| (no TS caller — by design) | scan_close (scan.rs:820) | cr_ui_scan_close + registry Drop | ✔ bridge-internal |
| (no TS caller) | image_target_list (images.rs:219) | registry rows | ✔ bridge — **SB-B-05** |

**Events (12, all emitted ↔ subscribed, cleanup verified):**

| Event (emit site) | TS listener (tauri-connector.ts) | UI hook → component | Status |
|---|---|---|---|
| `scan-progress:<id>` (events.rs:337,574) | onScanProgress (:289) | queries/scan.ts:32 external store → useScanProgress | ✔ |
| `scan-finished:<id>` (scan.rs:618) | onScanFinished (:303) | useScanFinishedBridge → scan page | ✔ |
| `save-progress:<id>` (events.rs:463) | onSaveProgress (:315) | useSaveProgressFraction | ✔ |
| `lost-candidate:<id>` (events.rs:423) | onLostCandidate (:327) | useLostCandidateBridge (1 s throttle) | ✔ |
| `scan-autosaved:<id>` (events.rs:477) | onScanAutosaved (:339) | useScanAutosaveStamp | ✔ |
| `consent-request:<id>` (consent.rs:167) | onConsentRequest (:355) | useConsentBridge + state re-hydration | ✔ |
| `apply-consent-request:<deviceId>` (consent.rs:228) | onApplyConsent (:570) | queries/lost-partition.ts:58 | ✔ |
| `session-load-progress:<path>` (events.rs:113) | onSessionLoadProgress (:451) | useSessionLoadFraction → backup page | ⚠ **SB-B-04** |
| `recovery-progress:<jobId>` (events.rs:689,761) | onRecoveryProgress (:499) | queries/recovery.ts:34 store | ✔ |
| `recovery-finished:<jobId>` (events.rs:763) | onRecoveryFinished (:511) | queries/recovery.ts:150 | ✔ |
| `image-progress:<imageId>` (imaging.rs:459,522) | onImageProgress (:650) | queries/imaging.ts:38 store | ✔ |
| `image-finished:<imageId>` (imaging.rs:543) | onImageFinished (:662) | useImageFinishedBridge | ✔ |
| `disk-health:select` (UI emitTo, lib/disk-health-window.ts:57) | listen in features/health/disk-health-window.tsx:170 | window selector | ✔ |

---

## Findings (defects)

### SB-B-01 — P1 — CONFIRMED (code-level): ExitGuard "Save and exit" can never succeed on the real backend; mid-scan save-on-exit is unreachable

Chain:
1. `ui/src/components/ui/window-controls.tsx:169` — custom X → `close()` → close-requested (guard always listening).
2. `ui/src/app/exit-guard.tsx:62-76` — guard fires for statuses {starting, running, paused, pausing, resuming, stopping}; dialog offers Save-and-exit.
3. `ui/src/app/exit-guard.tsx:93-107` — `doSaveAndExit` → `saveSession.mutate(null)`.
4. `ui/src/queries/sessions.ts:15-18` → `ui/src/connector/tauri/tauri-connector.ts:440-441` — `invoke('session_save', { scanId, path: null })`.
5. **BREAK: `ui/src-tauri/src/commands/sessions.rs:100-106`** — `if busy { Err(SCAN_NOT_RUNNING, "save the session after the scan finishes (or cancel it first)") }`. Every status the guard gates on implies busy (running/paused/stopping — `scan_status` events.rs:145-156) or pre-run ('starting': busy=false → engine refuses `ERR_BAD_PARAMETER` "scan has not run", cr_ui.cpp:2989-2990).
6. The ENGINE supports mid-run saves: `engine/src/ui/cr_ui.cpp:2987-3009` (`scan_save_session_impl` only requires `has_ctrl`; the N-060 adapter comment at :2912-2924 explicitly designs for "a scan cancelled mid-run can still persist its partial session").
7. Mock divergence: `ui/src/connector/mock/mock-connector.ts:1709-1745` saves in ANY state; `ui/tests/exit-guard.test.tsx:98-113` passes "Save and exit runs the REAL session save" **only against the mock**. Browser-mode tests can never see the real refusal.

User impact: during a live scan, closing the window → the guard's primary recommended action always fails with a toast ("save refused"), the window stays open; the only exits are "Keep scanning" or "Exit without saving" (losing all unsaved scan work since the last milestone auto-save). The W-02 product behavior advertised in the guard's own header comment is dead on the shipped backend.
Severity P1, confidence high (contradiction is fully visible in code; no runtime needed).

### SB-B-02 — P0 (if confirmed) — NEEDS RUNTIME PROOF: `core:window:allow-destroy` missing — the main window may be unclosable via the custom X

Chain:
1. `ui/src-tauri/tauri.conf.json` — `decorations: false` (frameless; the OS title bar's close button does not exist; the ONLY close affordance is the custom X).
2. `ui/src/app/shell.tsx:356-360` mounts `ExitGuard` unconditionally in the shell → `ui/src/app/exit-guard.tsx:57-89` **always registers `onCloseRequested`** in the Tauri runtime.
3. `ui/src/components/ui/window-controls.tsx:169` — X → `getCurrentWindow().close()`. With a JS close-requested listener registered, Tauri's Rust `close` emits the event and does NOT close server-side; closing is then entirely up to the JS side, which calls `destroy()`:
   - `ui/src/app/exit-guard.tsx:75` (terminal/unknown-status path), `:98` (save-and-exit success), `:110` ("Exit without saving"), plus the `@tauri-apps/api` 2.12.4 `onCloseRequested` wrapper's own auto-`destroy()` when the handler does not preventDefault (the "no active scan" early-return at exit-guard.tsx:63).
4. **BREAK: `ui/src-tauri/capabilities/default.json:7-17`** grants `core:window:allow-close/minimize/toggle-maximize/is-maximized/start-dragging/set-focus` but **NOT `core:window:allow-destroy`** — the `plugin:window|destroy` command is denied by the capability system.
5. `void win.destroy()` discards the rejection → unhandled rejection in console, window stays open.

User impact (if confirmed): the main window cannot be closed by its X at all — in any state, with or without a running scan; the process must be killed from the taskbar/task manager (which also kills a running scan ungracefully). The disk-health window is unaffected (no close-requested listener → `close()` closes server-side under allow-close).
Severity P0, confidence: medium-high — the permission requirement is a hard fact of Tauri 2's per-command capability model (the file itself proves the authors knew minimize/toggle-maximize need explicit grants); the one unverified link is the exact auto-destroy behavior of the 2.12.4 JS wrapper (not installable in this sandbox). **30-second runtime proof: build once on Windows, click the title-bar X with and without a scan running.**

### SB-B-03 — P1 — CONFIRMED: the unlock dialog's FVEK field is a dead control; the BitLocker key-unlock surface (M-033) is unreachable from the UI

Chain:
1. `ui/src/features/device-selection/unlock-volume-dialog.tsx:76-88` — FVEK input rendered with its own label/hint; button enabled when only the FVEK is filled (`:98` — `!password && !fvek` gate).
2. **BREAK: `ui/src/features/device-selection/unlock-volume-dialog.tsx:37-52`** — `onUnlock` sends ONLY `{ volumeId, passwords: [password] }`; the `fvek` state is never read after input.
3. `ui/src/connector/tauri/tauri-connector.ts:263-264` → `volume_unlock_dictionary` (passwords only — the ABI takes no key).
4. The key-open path exists end-to-end but has **zero UI callers**: `contract.ts:211` (`ScanParams.bitlockerFvek`) → `tauri-connector.ts:268-275` (passes it) → `commands/scan.rs:183-198` (`cr_ui_open_volume_with_key`) → engine M-033. Repo-wide grep: no UI code ever sets `bitlockerFvek` (only the mock echoes it, mock-connector.ts:1700).
5. Mock: `mock-connector.ts:653-669` ignores both fields and unlocks unconditionally — the e2e/vitest lane cannot see the dead wire.

User impact: a user holding only a BitLocker recovery key (FVEK) types it into a field that looks purpose-built for it, clicks Unlock, and gets a guaranteed failure toast ("the dictionary did not unlock the volume") — the shipped dictionary attempt without their key. Locked volumes are only unlockable via password.
Severity P1, confidence high (dead wire is visible in code; mock-green class).

### SB-B-04 — P2 — STRONG CODE EVIDENCE, NEEDS RUNTIME PROOF: `session-load-progress:<path>` embeds a raw Windows path in a Tauri event name — the W1b load spinner is dead on Windows

Chain:
1. `ui/src-tauri/src/bridge/events.rs:112-113` — `app.emit(&format!("session-load-progress:{}", pipe.path))` where `pipe.path` is the session file path, e.g. `C:\ProgramData\HeicFast\Recovery Wizard\sessions\auto_FOO_11_26_09_30_00.crsf` (sessions.rs:228-239 builds the entry; the store dir is `state.sessions_dir`).
2. **BREAK point: the event NAME contains `\`, spaces and `.`** — Tauri 2 documents/requires event names to be limited to alphanumeric plus `-`, `/`, `:`, `_` (the `listen` command validates; a Windows path violates it on every real install).
3. TS side: `ui/src/connector/tauri/tauri-connector.ts:455-460` — `listen('session-load-progress:' + path)` → the listen rejects (invalid name) → `pending.then(unlisten)` never runs → `void`-swallowed rejection, subscription never attaches.
4. Bridge side: `events.rs:112` — `let _ = app.emit(...)` swallows the emit error; `sessions.rs` load still completes normally.
5. The other 11 event names embed only bridge-minted ids (`scan-1`, `job-1`, `img-1`, `disk-0`) — this is the only path-bearing event name. Mock: `mock-connector.ts:1793-1796` routes through its own registry (no Tauri event system) → tests pass.

User impact: the W1b session-load progress spinner (`useSessionLoadFraction`, backup-page.tsx:32) never renders on any real Windows path; console carries an unhandled rejection. The load itself works (results render after); only the progress surface is dead — i.e., the "W-01 dead-letter fix" for this event is stillborn on the shipped platform.
Severity P2, confidence medium-high (charset rule is a documented Tauri 2 fact; exact emit-vs-listen validation split unverified in sandbox).

### SB-B-05 — P2 — CONFIRMED: `image_target_list` is registered with zero callers; the manifest's claimed consumers do not invoke it

Chain: `lib.rs:48` registers → `commands/images.rs:219` implements (registry rows) → command-surface.json documents `serves: []` "exists for the dev panel + diagnostics" → repo-wide grep for the command name outside `command-surface.json`/`lib.rs`/`images.rs`: **zero** TS/JS invocations (`queries/dev.ts` has none; the dev panel invokes nothing raw; `rules/no-raw-invoke.yml` forbids it). Dead registered command; the documented rationale is unfulfilled. Harmless (registered commands cost nothing), but the manifest text misstates reality. Severity P2-informational, confidence high.

### SB-B-06 — P2 — CONFIRMED (documented deviation, listed for completeness): the whole `repair` domain is a typed refusal, not a wired feature

Chain: `contract.ts:1252-1259` RepairDomain → `tauri-connector.ts:535-547` → `repair.rs:41-65` — all four ops return `BridgeError::gap(...)` (`not-implemented` + truthful scope) → the repair page renders the refusal honestly (`features/repair/repair-page.tsx:22-27` documents the posture). The real repair path is the in-scan pre-scan consent flow (scan.rs + consent.rs, fully wired ✔). No ABI exists for standalone repair (the manifest itself lists empty abiRefs). This is a recorded product gap, not a hidden break — listed so the count of "wired ops" is honest: **4 of 71 contract ops are typed refusals by design.**

### SB-B-07 — P3 — CONFIRMED: dead route `tools`

`ui/src/app/router.tsx:33` — `{ path: 'tools', element: <HealthPage /> }`; no link, button, or redirect targets `/tools` anywhere (the sidebar's Disk Health entry is `/health`, shell.tsx:71-75). Harmless dead route (catch-all fallback makes stray navigation safe). Severity P3.

### Mock-divergence class (meta-finding)

Three of the four confirmed breaks above (SB-B-01, SB-B-03, SB-B-04) share one root pattern: **the mock connector implements a more permissive semantics than the real bridge** (mock saveSession any-state; mock unlockVolume ignoring inputs; mock event registry with no Tauri name rules), and the entire automated lane (vitest + Playwright e2e) runs browser-mode against the mock (`connector/index.ts:22-33`; `isTauriRuntime()` false → guards/chrome inactive). Every "green" wire under the real backend is therefore only as trustworthy as the mock's fidelity — this class deserves a dedicated contract-conformance test double or a `tauri dev` smoke lane.

## Needs-runtime-proof list

1. **SB-B-02** (P0 candidate) — build once on Windows: (a) click X with no scan; (b) with a running scan choose each of the three guard buttons. Watch for `window.destroy not allowed` / no-op. Fix expected: add `core:window:allow-destroy` to `capabilities/default.json` (and audit whether the JS wrapper's auto-destroy also needs it — it does).
2. **SB-B-04** — load a large session from a path containing `\` and a space; check whether `session-load-progress:<path>` listeners fire (devtools console for the rejected listen). Fix expected: percent-encode/hash the path into the event name, or key by a bridge-minted load id.
3. **Stop-path latency** (documented honest limit, not a defect): `cr_ui_scan_cancel` is honored at engine loop heads (44 `checkpoint()` sites across ntfs/fat/exfat/ext4/btrfs/refs/zfs/hfsplus/carve/lost/orchestrator — verified); transports do NOT poll the token (`engine/include/cr/storage/cr_device.h:139-148`, F-B-16), so an in-flight raw read on a hung/dying disk finishes or fails on its own before the cancel is observed. Measure the drain window on a dying disk (the state poll already covers 'stopping' — queries/scan.ts:105-121).
4. **`emitTo`/`getByLabel` permission set for the disk-health window** — `core:webview:allow-create-webview-window` and `core:event:allow-emit-to` are granted; `WebviewWindow.getByLabel`'s `get_all_webview_windows` and the re-open `setFocus` should be smoke-tested on the built app (believed covered by core:webview/default + allow-set-focus respectively).
5. **PausableToken pause while stopping** — `scan_pause` during 'stopping' is a no-op at the engine (`PausableToken::pause` returns early when cancelled, cr_device.h:181-186); verify the UI's pause button disabled-state during stopping matches (scan-progress-page renders pause only for `status === 'running'` ✔ — already consistent, runtime-confirm only).

## Clean chains verified (spot list — full matrix above)

- **Stop/cancel/pause chain — COMPLETE and REAL:** scan page stop button (`scan-progress-page.tsx:278-293`) → confirm dialog (`:395-452`) → `useCancelScan` (queries/scan.ts:164-175) → `connector.scan.cancelScan` → `invoke('scan_cancel')` → `commands/scan.rs:719-744` — force-denies any pending consent gate FIRST (`state.close_scan_consents`, the §4.5 deadlock rule) → `cr_ui_scan_cancel` → `PausableToken::cancel()` (cr_device.h:196-207, wakes paused waiters) → every engine loop head polls `checkpoint()` (44 sites) → `run()` returns `ERR_CANCELLED` → the run thread latches `'cancelled'`, freezes a browsable final snapshot, persists the undo log if a repair applied, emits the final progress sample + `scan-finished` (scan.rs:525-622). Pause/resume block/wake cooperatively in `checkpoint()`. Image-stop: `image_create_cancel` → entry flag → `cr_ui_image_create_ex` producer loop head refuses within one 64 KiB chunk → `.partial` kept + terminal `image-finished` = the ack (imaging.rs:596-629 + engine contract). Recovery-cancel: `recovery_cancel` → `cr_ui_recovery_cancel` → the safe-writer loop's own checkpoint (cr_safewriter.cpp:682).
- **Window controls:** minimize (`allow-minimize` ✔), maximize/restore + live glyph swap (`window-controls.tsx:119-172`, `allow-toggle-maximize` + `allow-is-maximized` ✔, debounced resize follow), title-bar drag region (`data-tauri-drag-region` + `allow-start-dragging` ✔), dblclick-maximize (`shell.tsx:123-127`, button-adjacency guard ✔); close — wired but see **SB-B-02**. Browser mode correctly renders none of the cluster (`isTauriRuntime()` gates).
- **Close-during-scan semantics:** the guard reads the ENGINE's state fresh (`readScanState` — one uncached invoke), shows the three-way dialog; teardown paths force-deny pending consents (`close_scan_consents` on the run tail scan.rs:540 and on cancel scan.rs:736). The engine is in-process, so "Exit without saving" (process death) cannot orphan a scan *thread* — the OS reclaims it; this matches the reference product's behavior. No scan-cancel is issued before destroy (acceptable, but combined with SB-B-01 it means "save on exit" is impossible and "exit" may be blocked by SB-B-02).
- **All 12 event chains** emit ↔ subscribe with disciplined cleanup (external-store pattern with last-listener unsubscribe: queries/scan.ts:69-84, queries/recovery.ts:25-60, queries/imaging.ts:18-50; effect-return unlisten everywhere else) — except the session-load-progress path (SB-B-04).
- **Registration parity:** 59/59 commands registered; `validate.mjs` green both directions; every command argument camelCase↔snake_case matches; error JSON `{code, bridgeCode, message}` (error.rs:64-73) ↔ `toConnectorError` (tauri-connector.ts:167-192) with the reasonKey table — all 34 reasonKey leaves exist in both i18n dictionaries (`i18n/en.ts`, `zh-CN.ts`).
- **Keyboard/menu/router:** no global shortcut surface exists in the product (only Escape clears the results selection — results-explorer.tsx:130-137 — and Enter commits search :358); every menu item (settings nav, theme toggle, about dialog, language switch, buy/activation) has a real handler; all sidebar routes resolve; all scan-start sites (`home`, `sd-card`, `health`, `backup`, `results-route`) set the active-scan store and navigate to the live route with `?scanId=`.
- **ABI layout gate:** regenerated `abi_layouts.rs` is byte-identical to checked-in; header checksum gate passes — the earlier session's `bus_type`/`power_cycles_nvme` struct drift is fixed (BF-ABI-01 closed; spot-verified `CrUiDiskInfo` abi.rs:193-213 vs cr_ui.h:434-441 and `CrUiSmartReport` abi.rs:721-764 vs header tail).
- **Imaging flow** (create dialog → start/state/cancel/events/reveal), **recovery flow** (start/preview-deleted-tree/items/report/cancel/events), **health** (read + separate disk-health window with capability `disk-health.json`), **fsck/bitlocker** one-shots, **device enumeration incl. virtual quick-targets** (`vol-recycle-bin`/`vol-desktop` are REAL bridge targets — devices.rs:315-322, mapped to the system volume with implicit pathScope — not mock-only ids as the fixture naming suggests), **dictionary unlock (password arm)**, **imported-image targets**, **session save/list/load/meta/resume/resume-last**, **consent (both families + rehydration)** — all fully wired and real.

---

### Summary counts

- **86 chains traced**; 71 contract ops: 59 wired to real engine/bridge implementations (55 fully real + 4 typed-refusal repair ops + 2 bridge-registry ops by design), 12 event surfaces (11 clean, 1 suspect — SB-B-04).
- **7 defects recorded:** SB-B-01 (P1, confirmed), SB-B-02 (P0-candidate, runtime proof), SB-B-03 (P1, confirmed), SB-B-04 (P2, runtime proof), SB-B-05 (P2, confirmed), SB-B-06 (P2, documented gap), SB-B-07 (P3, confirmed).
- The three confirmed mock-green/real-red breaks (01, 03, 04) all evade the current test lanes; a tauri-runtime smoke lane is the systemic fix.
