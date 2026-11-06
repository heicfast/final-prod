# WIRING-AUDIT — every UI↔core seam, and every mismatch

Product: `/home/z/my-project/final-prod` (Tauri+React UI → Rust bridge → cr_ui ABI v4 → C++ engine).
Scope of this sheet: commands/*.rs, bridge/*.rs, ui/src/connector/{contract.ts,tauri/tauri-connector.ts},
ui/src/queries/*.ts, ui/src/features/**, contracts/*, engine/include/cr/cr_ui.h, engine/src (telemetry paths).
Every claim carries file:line. Line numbers verified 2025 via direct read; engine side cited from
`engine/src/core/orchestrator/cr_scan.cpp`, `engine/include/cr/cr_ui.h`, `engine/include/cr/orchestrator/cr_scan.h`.

---

## 1. The seam map

Command registration: `ui/src-tauri/src/lib.rs:33-92` (45 commands). TS side: the ONE backend is
`connector/tauri/tauri-connector.ts` (`connector/index.ts:23-37`; mock is dev/test-only).

| Tauri command (Rust) | Engine ABI calls | JSON to frontend | Frontend consumer |
|---|---|---|---|
| `devices_list` / `devices_refresh` (commands/devices.rs:427-444) | `cr_ui_enumerate_disks/_volumes` (util.rs:156-184, 2-call), open-probe `cr_ui_open_disk` per disk (devices.rs:130), `cr_ui_open_volume` FVE probe (devices.rs:151) | `Device[]` incl. `volumes[]` (devices.rs:208-229) | `useDevices`/`useRefreshDevices` (queries/devices.ts:5-20) via `devices.listDevices/refreshDevices` (tauri-connector.ts:189-201) |
| `volume_unlock_dictionary` (devices.rs:459-505) | `cr_ui_unlock_volume_with_dictionary` | `void` | `useUnlockVolume` (devices.ts:23-30) → unlock-volume-dialog |
| `scan_start` (commands/scan.rs:92-110, prep :116-332) | `cr_ui_abi_version`, `cr_ui_open_volume[_with_key]`/`cr_ui_open_disk`, `cr_ui_device_geometry`, `cr_ui_scan_create`, `cr_ui_scan_attach_device`, `cr_ui_scan_set_event_fn`; run thread: `cr_ui_scan_run`, `cr_ui_scan_get_stats`, `cr_ui_scan_repair_save_undo`, `cr_ui_scan_snapshot` | `{scanId}` | `useStartScan` (queries/scan.ts:88-98) |
| `scan_pause` / `scan_resume` / `scan_cancel` (scan.rs:500-581) | `cr_ui_scan_pause` / `_resume` / `_cancel` (+ consent force-deny first, :573) | `void` | `usePauseScan`/`useResumeScan`/`useCancelScan` (scan.ts:148-175) |
| `scan_state` (scan.rs:587-656) | `cr_ui_scan_busy/_stopping/_telemetry` (events.rs:198-211), registry | `ScanState` (scan.rs:637-652) | `useScanState` (scan.ts:100-123), 500 ms poll while running/starting/stopping |
| `scan_close` (scan.rs:658-691) | registry remove → `ScanEntry::Drop` (state.rs:291-327) | `void` | **NO frontend caller** (manifest `serves:"internal"`; no connector method — see §3/§4) |
| `scan_save_cancel` (scan.rs:696-719) | `cr_ui_scan_save_cancel` | `void` | `useCancelSave` (queries/sessions.ts:31-35) |
| `results_tree` (commands/results.rs:887-898) | `cr_ui_scan_snapshot` / `cr_ui_session_snapshot` + `cr_ui_snapshot_dfs/_node/_node_count/_partial` (state.rs:605-755) | `TreePage` | `useResultTree` (queries/results.ts:21-32) |
| `results_query` (results.rs:482-507) | **none live** — bridge-side filter/sort/page over the NodeMeta cache (G-7, results.rs:1-17) | `ResultsPage` (results.rs:472-479) | `useResultsPages` (results.ts:34-55), page 200 |
| `results_count` (results.rs:509-526) | same | `number` | `useCountResults` (results.ts:58-66) |
| `node_detail` (results.rs:904-948) | metas only | `NodeDetail` (results.rs:936-944) | `useNodeDetail` (results.ts:68-74) |
| `preview_open` (commands/preview.rs:464-474) | `cr_ui_preview_open_snapshot/_session`, `_size`, `_metadata`, `_unreadable`, `_read` (chunked ≤4 MiB), `cr_ui_archive_entry_count/_at` | `PreviewPayload` union (preview.rs:329-458) | `usePreview` (queries/preview.ts:7-15) |
| `preview_thumbnail` (preview.rs:476-486) | same + `cr_ui_thumbnail_generate` (session-kind only, preview.rs:223-278) | `{dataUrl}\|null` | `useThumbnail` (results.ts:76-83) |
| `session_save` (commands/sessions.rs:85-140) | `cr_ui_scan_save_session`, `cr_ui_scan_get_stats` | `SessionMeta` (sessions.rs:126-136) | `useSaveSession` (sessions.ts:12-18) |
| `sessions_list` (sessions.rs:147-172) | `cr_ui_session_list` (2-call, W1a) | `SessionMeta[]` (render_row :174-221) | `useSessions` (sessions.ts:5-10) |
| `session_load` (sessions.rs:356-369) | `cr_ui_session_load_with_events` (W1b sink), `cr_ui_session_get_info`, `cr_ui_session_stats`, source re-attach `cr_ui_open_volume/_disk` (:667-738) | `{scanId}` | `useLoadSession` (sessions.ts:20-29) |
| `session_meta` (sessions.rs:372-405) | `cr_ui_session_list` | `SessionMeta` | connector method only (tauri-connector.ts:314-315) — **no query hook calls it** (§3) |
| `session_resume_last` (sessions.rs:411-447) | `cr_ui_session_last_autosave_path` + load | `{scanId, path}` | `useResumeLastSession` (sessions.ts:49-53) → Home continue-last |
| `session_resume` (sessions.rs:453-486) | load + `cr_ui_session_resume`, `cr_ui_scan_attach_device`, `cr_ui_scan_verify_identity`, `cr_ui_scan_set_event_fn`; launches run thread | `{scanId, targetId}` | `useResumeSession` (sessions.ts:39-45) |
| `recovery_start` (commands/recovery.rs:51-79, blocking :81-289) | `cr_ui_recovery_create`, `_set_destination`, `_enqueue[_subtree][_snapshot]`, `_queue_count`; process thread `cr_ui_recovery_process`, `_get_stats`; emitter `_get_stats`, `_report_json` | `{jobId}` | `useStartRecovery` (queries/recovery.ts:66-84) |
| `recovery_cancel` / `recovery_state` / `recovery_items` / `recovery_report` (recovery.rs:367-569) | `cr_ui_recovery_cancel`; registry; `cr_ui_recovery_report_json` (×2) | `void` / `RecoveryJob` / `RecoveryItem[]` / `RecoveryReport` | `useCancelRecovery`/`useRecoveryJob`/`useRecoveryItems`/`useRecoveryReport` (recovery.ts:95-128) |
| `health_read` (commands/health.rs:18-157) | `cr_ui_health_read` (2-call) | `DiskHealth` (health.rs:138-153) | `useHealth` (queries/health.ts:5-13) |
| `consent_respond` (commands/repair.rs:31-38) | registry gate only (bridge/consent.rs:229-250) | `void` | `useRespondToConsent` (scan.ts:202-207) + `respondToApplyConsent` (tauri-connector.ts:415-416) |
| `repair_dry_run` / `repair_apply` / `repair_undo_log` / `repair_undo` (repair.rs:40-65) | **none** — typed `not-implemented` (G-1 gap trio) | throws | connector `repair.*` (tauri-connector.ts:368-381) → repair-page message |
| `lost_partition_apply` (commands/lost_partition.rs:52-121) | `cr_ui_open_disk`, `cr_ui_apply_lost_partition` (+ apply consent sink, consent.rs:179-222) | `ApplyResult` (lost_partition.rs:43-50) | `useApplyLostPartition` (queries/lost-partition.ts:13-26) |
| `lost_partition_undo` / `lost_partition_save_undo` (lost_partition.rs:191-228) | `cr_ui_apply_undo_last` / `cr_ui_apply_save_undo` | `ApplyResult` / `void` | `useUndoLastApply` / `useSaveApplyUndo` (lost-partition.ts:28-39) |
| `apply_consent_pending` (lost_partition.rs:233-246) | registry | `ApplyConsentEvent\|null` | `useApplyConsentPending` (lost-partition.ts:41-50, 4 s poll) |
| `system_app_info` (commands/system.rs:130-147) | `cr_ui_abi_version`, `cr_ui_user_sign_count` | `AppInfo` — **omits `connectorMode`** (§5 M-8) | `useAppInfo` (queries/system.ts:5-11) |
| `system_security_posture` / `system_relaunch_elevated` (system.rs:70-124) | Win32 `OpenProcessToken`/`GetTokenInformation`/`ShellExecuteW("runas")` | `{administrator}` / `void` | `useSecurityPosture`/`useRelaunchElevated` (system.ts:42-57) → ElevationBanner (features/consent/elevation-banner.tsx:16-47) |
| `system_load_user_signs` (system.rs:202-210) | `cr_ui_register_user_signs`, `cr_ui_user_sign_count` | `{count}` | `useLoadUserSigns` (system.ts:59-63) — **bug, §5 M-7** |
| `system_validate_destination` (system.rs:293-338) | Win32 probes + enumeration | `DestinationValidation` | `useValidateDestination` (system.ts:24-29) |
| `system_pick_directory` / `system_list_mounts` / `system_open_path` (system.rs:340-412) | dialog/opener plugins; enumeration + `GetDiskFreeSpaceExW` | `string\|null` / `MountPoint[]` / `void` | `usePickDirectory` / `useMountPoints` (system.ts:13-36) / **openPath: no UI caller** (§5 M-9) |

---

## 2. Event wiring

The bridge emits 10 event families (contracts/command-surface.json `events`). The frontend listens
to **4** (`listen()` appears only at tauri-connector.ts:233, 249, 348, 407). Verified by grep over
`ui/src` — no other `listen` call sites exist.

| Event | Producer (bridge) | Frontend consumer | Payload vs contract verdict |
|---|---|---|---|
| `scan-progress:<id>` | emitter thread (bridge/events.rs:389-398) + terminal emit from run thread (events.rs:214-236; commands/scan.rs:446-453) | `onScanProgress` (tauri-connector.ts:229-239) → external store (queries/scan.ts:22-84) | Field names match `ScanProgressEvent` (contract.ts:192-213). **Dropped/constant fields below.** |
| `scan-finished:<id>` | run thread, `{status, failure?: string, autosaved}` (scan.rs:455-458) | **NONE** — terminal detected via `scan-progress.status` change (scan.ts:38-50) | `failure` is a bare string; contract `ScanState.failure` expects `{code, reasonKey?}` (contract.ts:229). Latent (nothing reads either). |
| `save-progress:<id>` | CR_EVT_SAVE_PROGRESS → `{fraction}` (events.rs:330-336) | **NONE** | Save spinner has no progress feed; `cancelSave` exists (sessions.ts:31-35) but the fraction is dead. |
| `lost-candidate:<id>` | CR_EVT_LOST_CANDIDATE + N-061 log join `{lba, sectorCount?, fs?, score?, count}` (events.rs:287-313) | **NONE** — lost-partition page polls `scan_state.lostCandidates` instead (features/lost-partition/lost-partition-page.tsx:77-78; scan.rs:623-651) | Works via polling; event is redundant dead wiring (≤500 ms staleness either way). |
| `consent-request:<id>` | consent sink on the engine run thread (bridge/consent.rs:158-160) | `onConsentRequest` (tauri-connector.ts:245-255) → `useConsentBridge` (scan.ts:183-199) + state re-hydration | Matches `ConsentRequestEvent` (contract.ts:245-257). ✔ |
| `apply-consent-request:<id>` | apply consent sink (consent.rs:214-217) | `onApplyConsent` (tauri-connector.ts:403-413) → `useApplyConsentBridge` (lost-partition.ts:54-62) | Matches `ApplyConsentEvent` (contract.ts:676-691). ✔ |
| `recovery-progress:<id>` | job emitter 250 ms loop (events.rs:410-516) + terminal from process thread (events.rs:569-591) | `onRecoveryProgress` (tauri-connector.ts:344-354) → job store (recovery.ts:22-61) | Matches `RecoveryProgressEvent` (contract.ts:537-550); `currentFileProgress` constant 0, `currentFile` ≤2 s stale (below). |
| `recovery-finished:<id>` | process thread `{status}` (events.rs:587-590) | **NONE** — status change on `recovery-progress` covers it | Redundant dead event. |
| `session-load-progress:<path>` | calling-thread sink `{path, permille, nodesDone, nodesTotal}` (events.rs:77-102) | **NONE** — backup page shows only `isPending` (features/backup/backup-page.tsx:162,171) | Load progress never rendered; large sessions load with a bare spinner. |
| `scan-autosaved:<id>` | CR_EVT_AUTOSAVED `{rc}` (events.rs:342-350) | **NONE** — sessions list invalidated indirectly by the progress store terminal handler (scan.ts:46-49) | Redundant dead event. |

**Fields the engine provides that the bridge drops (scan-progress):**
- `CR_EVT_SCAN_PROGRESS.bytes_done` / `bytes_total` — live carve-true bytes, stamped per sample at
  engine/src/core/orchestrator/cr_scan.cpp:596-597 (`e.bytes_done = stage == 2 ? st.bytes_done : 0`).
  The emitter reads only `stage_permille`/`overall_permille` (events.rs:266-273) and refreshes
  `bytesScanned` from `cr_ui_scan_telemetry` instead (events.rs:357-360), whose `bytes_scanned`
  is stamped **only at stage boundaries + completion** (cr_scan.cpp:535 inside `tel_counts_`, called
  at boundaries per the N-167 comment cr_scan.cpp:521-528). Net effect: frozen byte counter during
  carve (§5 M-1).
- `CR_EVT_SESSION_LOAD_PROGRESS.bytes_done/bytes_total` (nodes) are forwarded (events.rs:97-98) but
  nobody listens.
- `CR_EVT_ERROR_RANGE.err_module/err_operation/err_attempts` — intentionally log-only per
  contracts/event-mapping.md:35 (documented, not a defect).

**Fields the frontend expects that nothing provides:**
- `ScanProgressEvent.dataFoundBytes` mid-scan — `ScanLive.data_found_bytes` is 0 live by design
  (bridge/state.rs:107-109, G-3); **no `bytes_found` exists anywhere in the ABI**
  (`cr_ui_telemetry` cr_ui.h:663-674; `cr_ui_event` cr_ui.h:233-247; `cr_ui_scan_stats`
  cr_ui.h:938-950) although the core computes `ScanStats.bytes_found`
  (engine/include/cr/orchestrator/cr_scan.h:586; stamped cr_scan.cpp:529, 2699). The UI headline
  renders "Files Found: N (0 B)" (features/scan-progress/scan-progress-page.tsx:183-188) until the
  terminal latch (§5 M-2). The mock fed it live (connector/mock/mock-connector.ts:643,661,1037).
- `ScanProgressEvent.currentPath` — hard `null` (events.rs:193, G-2 "never fabricated"); the
  "Searching: <path>" ticker is gated on it and never renders (scan-progress-page.tsx:148-157).
  The mock fed it (mock-connector.ts:630,671).
- `Volume.lostPartition` (LostPartitionHint) — never emitted (devices.rs:179, G-10); Home's
  lost-partition badge can never render (features/device-selection/home-page.tsx:523, 595).
- `AppInfo.connectorMode` — bridge never returns it (system.rs:133-146) (§5 M-8).

**Fields that are constants / synthesized:**
- `currentFileProgress: 0` always (events.rs:558) — documented indeterminate (event-mapping.md:62-64).
- `currentFile` — report tail polled at 2 s cadence, only names *committed* items (events.rs:484-489;
  event-mapping.md:59-61).
- `recoverableDuringScan` — `snapshot_exists` flag, set after first successful `cr_ui_scan_snapshot`
  (state.rs:629, 642).
- `deviceConnected` — synthesized from heartbeat/progress freshness within 10 s (events.rs:142-152;
  DISCONNECT_AFTER_MS state.rs:833).
- `quickScanAvailable: true` constant (devices.rs:180; sessions.rs:314).
- `SmartAttribute.threshold: null` constant (health.rs:130, BF-03 — base ABI carries no threshold).

---

## 3. Contract conformance

**`node contracts/validate.mjs` verdict: GREEN, exit 0.**
Output: `40/40 engine error codes mapped`; `26/26 ConnectorErrorCode vocabulary covered`;
`12/12 CR_EVT_* mapped in event-mapping.md`; `45 commands, 10 events, well-formed`;
`phase 2 rust side: 45 command implementations cross-checked`; `phase 2 ts side: 44 invoked
commands cross-checked`; `contracts validation green`.

The validator is structural (names, vocab, presence) — it does **not** check payload field shapes,
which is where every real defect below lives.

**Manifest → Rust → frontend, both directions:**
- All 45 `command-surface.json` rows have `#[tauri::command]` implementations (validator check 5).
- All 44 connector-invoked commands are in the manifest (check 6). The 45th, `scan_close`
  (manifest `serves: "internal (registry Drop; the fresh contract dropped the surface)"`),
  is exempted by the validator's `internal`-prefix rule (validate.mjs:125) — **an intentional
  orphan with no frontend caller**, which is the root of leak L2 (§4).
- Connector methods with **no UI consumer above the connector** (dead one level up):
  `sessions.getSessionMeta` (contract.ts:488; tauri-connector.ts:314-315; no hook/feature calls it)
  and `system.openPath` (contract.ts:785; tauri-connector.ts:449-451; nothing calls it — see M-9).

**Error codes:**
- Every `error-codes.json` engine row has a numeric entry in the TS `ENGINE_CODE_MAP`
  (tauri-connector.ts:92-132; 40 codes, complete vs cr_error.h) and `bridgeCode` values resolve
  through `REASON_KEYS` (:62-89, all 26 vocabulary members).
- Bridge-side `code` module (bridge/error.rs:27-49) defines 17 bridge-minted codes; consistent
  with `bridgeOnlyCodes` (error-codes.json:339-401).
- **Orphan vocabulary (declared, mapped, never produced by the real bridge):** `preview-encrypted`
  and `preview-timeout` — listed in error-codes.json:373-386 and REASON_KEYS:80-81, but
  `bridge/error.rs` has no such constants and no command path throws them (preview.rs classifies
  only unsupported/too-large/missing-data/damaged). Mock-only vocabulary.
- `engine_failure_code` (scan.rs:465-473) re-implements a 5-row subset of the TS table for the
  `scan-finished`/`scan_state` `failure` field (repair-failed / device-permission / device-gone /
  elevation-required / unknown) — in lockstep today, duplicate-source risk.

---

## 4. Threading / lifecycle audit (scan_start run thread + emitter)

**Lifecycle recap:** `scan_start` → blocking prep (open, create, attach, subscribe-before-run,
register entry, sink box `Box::into_raw` scan.rs:258) → `launch_run_thread` spawns
(a) the emitter thread draining the bounded(512) channel (scan.rs:349-356 → events.rs:242-402) and
(b) the run thread executing `cr_ui_scan_run` + terminal latch + final events (scan.rs:362-459).
Sink contract (never block on an engine worker thread; try_send; coalesce) is honored
(events.rs:46-66; event-mapping.md:10-27). Terminal latching is single-writer: only the run thread
writes `Terminal` (scan.rs:437-439); the emitter always passes `None` (events.rs:392). ✔

**L1 — jobs registry never shrinks (leak).** `BridgeState.jobs` insert at recovery.rs:282; there is
no removal path anywhere (grep: only state.rs:513 `unregister_consent` and scan.rs:686 `scans.remove`
exist). `JobEntry::Drop` (state.rs:384-394) — the only place `cr_ui_recovery_close` and the boxed
progress-sender free run — therefore never executes before process exit. Every recovery job leaks an
engine recovery handle + one boxed `SyncSender` for the app's lifetime.

**L2 — scans registry is append-only in practice (leak).** `ScanEntry` removal exists only in
`scan_close` (scan.rs:686), which **no frontend code can call** (§3). Each live scan or session load
accumulates: the engine scan/session handle + device handle (state.rs:308-325), the snapshot handle +
O(n) `NodeMeta` cache (state.rs:211-229; ~96 B/node struct + strings, bounded <1 KiB/node by test
results.rs:1140-1143), the sink box, **and the emitter thread itself** — the emitter's `for evt in rx`
only ends when the sink `Sender` drops (state.rs:300-305), which only happens in `ScanEntry::Drop`.
In a long-lived desktop session doing several scans, threads + engine handles + metadata grow without
bound. The UI-side store cleanup (scan.ts:75-79) only drops listeners.

**L3 — `ScanEntry.jobs` never shrinks.** Job ids are pushed (recovery.rs:283-287; sessions.rs:589-591)
and never removed, so after the first recovery job `scan_close` refuses forever
("cannot close a scan with recovery jobs still referencing it", scan.rs:675-681) — which, combined
with L2, makes scan entries structurally unleakable once any recovery ran.

**R1 — `scan_close` during the post-run drain (BF-24/BF-25 class; latent today).** When
`cr_ui_scan_run` returns, `busy` flips false immediately but the terminal latch happens only after
the final snapshot freeze + full metadata pass (scan.rs:418-443) — seconds to minutes at 10⁶ nodes.
`scan_close`'s guard is busy-only (scan.rs:668-674), so it succeeds in that window: the registry row
is removed while the run thread still holds its `Arc` (entry survives — correct), final events still
emit, but every later `scan_state`/tree/query on that scanId throws `scan-not-found`
(state.rs:484-492) — the scan's results are orphaned. Harmless today (nothing calls `scan_close`);
fatal the day close is wired into the UI. A terminal-latch check (or `is_terminal()`-based refusal)
is missing.

**R2 — emitter use-after-free window on teardown.** The emitter thread does **not** hold the
`ScanEntry` Arc — only `live`, `snapshot_exists` Arcs and a raw `SendPtr<CrUiScan>`
(scan.rs:346-356; events.rs:242-249). `ScanEntry::Drop` unsubscribes the event fn, frees the sink
box, then calls `cr_ui_scan_close` (state.rs:291-327). Unsubscribing stops *new* events, but std mpsc
still delivers already-queued events after all senders drop, so the emitter can be inside
`cr_ui_scan_telemetry(scan_ptr)` (events.rs:359) concurrently with or after `cr_ui_scan_close` —
a dangling-handle call. Only reachable via `scan_close` (R1's window or post-terminal), i.e. latent,
but the emitter should hold the entry Arc (or a close barrier) to serialize Drop vs drain.

**R3 — status regression during the drain.** In the same post-run pre-latch window,
`scan_status` falls through busy/stopping/paused to `"starting"` (events.rs:134-136): after a
cancel, the UI polls `stopping` → run returns → **`starting`** → `cancelled`. The 500 ms poll keeps
running (scan.ts:118 includes `starting`), so the status line visibly regresses before settling.

**Sound parts (verified, no action):** consent §4.5 deadlock discipline — the run thread blocks on
the gate with the registry lock released (consent.rs:148-164); `scan_cancel` force-denies pending
gates *before* the token call (scan.rs:569-576); the run tail closes survivors (scan.rs:377);
apply teardown closes device gates + clears in-flight (lost_partition.rs:113-116). Snapshot
generation swaps are serialized on the entry's snap mutex and the metadata pass holds only that
mutex, never the registry lock (state.rs:605-669, 673-755). The job emitter exits cleanly via
`recv_timeout(250ms)` + `finished` flag (events.rs:424-443). `snapshot_exists` uses
`Ordering::Release/Acquire` correctly (state.rs:629/642; events.rs:229). Session-load sink emits on
the calling thread per the header and frees its pipe on every path (sessions.rs:260-283).

---

## 5. Mismatch table (centerpiece — ranked by user impact)

Severity: **[BUG]** user-visible wrong behavior · **[UX]** degraded/missing UX · **[COS]** cosmetic/doc · **[LAT]** latent (dead wiring today, breaks when wired).

| # | Mismatch | UI expectation (file:line) | Engine/bridge reality (file:line) | Severity |
|---|---|---|---|---|
| M-1 | **bytes_scanned frozen during carve** — "Reading sector: X/Y" stalls for the whole carve stage (the longest stage). | `bytesScanned` drives the sector counter (scan-progress-page.tsx:124-125, 257-262); mock streamed it live | Event carries live carve bytes `e.bytes_done/e.bytes_total` (cr_scan.cpp:596-597); emitter drops them, reads permilles only (events.rs:266-273); telemetry `bytes_scanned` stamped only at stage boundaries (cr_scan.cpp:535 w/ :521-528; cr_ui.h:665) | **[BUG]** |
| M-2 | **Found size is 0 B the whole scan** — headline reads "Files Found: N (0 B)" until terminal. | `dataFoundBytes` in the live headline + celebration modal (scan-progress-page.tsx:183-188, 418-425); contract.ts:203; mock fed it (mock-connector.ts:643) | No `bytes_found` in `cr_ui_telemetry`/`cr_ui_event`/`cr_ui_scan_stats` (cr_ui.h:663-674, 938-950); core has it (cr_scan.h:586, cr_scan.cpp:529) but it never crosses; bridge latches 0 live (state.rs:107-109), computes the sum from snapshot metas only at completion (scan.rs:418-426) | **[BUG]** |
| M-3 | **loadUserSigns import always fails** — Settings import passes the file *name*, not the document text. | Contract: "the webview's file input supplies the document text" (contract.ts:764-767); call site settings-page.tsx:253 `loadUserSigns.mutate(file.name, …)` | Bridge parses the string as a JSON document (system.rs:157-164, 202-210) → engine register gets `"userdata-defaults.json"` as content → parse refusal every time | **[BUG]** |
| M-4 | **"Open Location" context item is enabled but does nothing.** | Results-explorer context menu item has no `onClick` at all (results-explorer.tsx:476-484); `openPath` connector method exists (contract.ts:785; tauri-connector.ts:449-451) | Bridge `system_open_path` is fully implemented (system.rs:390-412) — dead on the vine | **[BUG]** |
| M-5 | **Elevation deferred (no manifest)** — first run is a wall of 0x500C0001 refusals until the user notices the banner and relaunches. | ElevationBanner + relaunch flow (elevation-banner.tsx:16-47; queries/system.ts:42-57) | No `requestedExecutionLevel` manifest anywhere (src-tauri has no *.manifest; Cargo.toml:1-46 / tauri.conf.json carry no elevation config; NSIS bundle default). Every raw-disk verb refuses unelevated (error-codes.json:328-337) | **[UX]** |
| M-6 | **SD cards read as 'usb'** — the SD-card page can never prefer an actual SD row. | `kind: 'sd'` expected (contract.ts:28; sd-card-page.tsx:30,33 filters/prefers `kind==='sd'`; default id `'dev-sd'` :27 never matches `disk-N`) | `kind = removable ? "usb" : "internal"` — no bus_type in `cr_ui_disk_info` (devices.rs:215; type-maps.md §1 G-9) | **[UX]** |
| M-7 | **About dialog labels the shipped product "Mock"** with a warning badge. | `AppInfo.connectorMode` required (contract.ts:730); about-dialog.tsx:35-38 renders warning+"Mock" when ≠ 'tauri' | `system_app_info` never returns `connectorMode` (system.rs:133-146); also `engineVersion:"cr_ui ABI v4"` vs type-maps §10 "null (G-6)" | **[COS]** (user-visible mistrust) |
| M-8 | **6 of 10 bridge events have no listener** — save/load progress, lost-candidate stream, autosave/finished notifications all dead. | Save spinner (contract SessionsDomain + cancelSave sessions.ts:31-35), load spinner (backup-page.tsx:162,171), candidate stream | Emitters exist and fire: save-progress (events.rs:330-336), session-load-progress (:92-100), lost-candidate (:287-313), scan-autosaved (:342-350), scan-finished (scan.rs:455-458), recovery-finished (events.rs:587-590); only 4 listens exist (tauri-connector.ts:233,249,348,407) | **[UX]** (functionality survives via polling, but the designed feedback loops are unwired) |
| M-9 | **quickScanAvailable always true** — the "quick unavailable → offer deep only" contract branch is unreachable. | contract.ts:114-115 ("when false the UI must offer deep only"); mock could flip it (mock-connector.ts:309) | Constant `true` (devices.rs:180, G-9/G-10; sessions.rs:314). No UI consumer reads it today → latent | **[LAT]** |
| M-10 | **currentPath always null** — "Searching: <path>" ticker never renders. | Ticker gated on `progress.currentPath` (scan-progress-page.tsx:148-157); contract.ts:211-212 | Hard `null`, never fabricated (events.rs:193, G-2); no ABI event carries the item | **[UX]** |
| M-11 | **Home lost-partition badge can never render.** | `hasLost = volumes.some(v => v.lostPartition)` (home-page.tsx:523,595); contract.ts:112-113 | Hint never provided — lost candidates are scan-time events (devices.rs:179, G-10) | **[UX]** |
| M-12 | **Status regresses to "starting" after Stop** during the terminal drain. | Poll set includes `starting` (scan.ts:118); status line shows phase labels (scan-progress-page.tsx:174-181) | Post-run pre-latch window: busy=0, terminal=None → `"starting"` (events.rs:134-136); latch happens after snapshot+metas pass (scan.rs:418-443) | **[UX]** |
| M-13 | **Session list shows bytes *scanned* as the found size.** | `SessionMeta.dataFoundBytes` = found-data size (contract.ts:477; type-maps §9 "snapshot sum") | `render_row` maps `r.bytes_scanned` into `dataFoundBytes` (sessions.rs:217) — carve-true-bytes, a different quantity than the terminal snapshot sum (scan.rs:418-426) | **[COS]** (semantic lie in a list column) |
| M-14 | **scan-finished failure shape mismatch.** | `ScanState.failure?: {code, reasonKey?}` (contract.ts:229) | Event payload `failure` is a bare string (scan.rs:412-414, 457); `scan_state` emits `{code}` only (scan.rs:603-609) | **[LAT]** (no consumer of either) |
| M-15 | **Recovery currentFile is 2 s-stale and only names committed items; currentFileProgress constant 0.** | Live file row (recovery-page.tsx:657-659); per-file bar implied by contract.ts:542 | Report-tail poll at 2 s (events.rs:484-489), committed-only semantics (event-mapping.md:59-61); `currentFileProgress: 0` constant (events.rs:558) | **[UX]** (documented degradation) |
| M-16 | **Preview of damaged files refuses instead of returning annotated content.** | type-maps §8: "preview-damaged annotation rides the payload (content still returned)" | Bridge throws `preview-damaged` and returns nothing (preview.rs:317-326) | **[COS]** (doc/code mismatch; truthful either way) |
| M-17 | **Leaked registries / threads (L1-L3).** | UI assumes scans/sessions/jobs are managed resources | jobs never removed (recovery.rs:282; no remove site); scans removed only by the UI-unreachable `scan_close` (scan.rs:658-691); `entry.jobs` never shrinks → close refuses forever (scan.rs:675-681); emitter thread lifetime tied to the never-run Drop (state.rs:291-327) | **[BUG]** (resource leak, long sessions) |
| M-18 | **scan_close mid-drain orphans results (R1) + emitter UAF window (R2).** | Future close callers expect the results to remain browsable | busy-only guard (scan.rs:668-674) misses the run-tail window; emitter lacks the entry Arc (events.rs:242-249) | **[LAT]** |
| M-19 | **Dead vocabulary:** `preview-encrypted` / `preview-timeout` never produced. | REASON_KEYS + contract.ts:74-75 expect them | No bridge path throws them (bridge/error.rs:27-49; preview.rs) | **[COS]** |
| M-20 | **Polling cadences:** scan_state 500 ms while running/starting/stopping (scan.ts:105-121) + apply-consent re-hydration 4 s (lost-partition.ts:48) + job report-tail 2 s (events.rs:408,484) are the *only* live feedback; results tree/count invalidate on 10 % overall buckets (scan.ts:37-56) — coarse but bounded. Engine coalesces progress to ≤1000 events/scan (cr_scan.cpp:632-636), so the 512-deep channel (scan.rs:257) can drop superseded samples under burst — by design (events.rs:52). | — | — | **[COS]** (as-designed; recorded for fix-order context) |

Also verified clean (no mismatch): device id schemes, bitlocker tri-state, FVEK/dictionary unlock
flow, consent echo-token flow both families, destination validation ladder, recovery report
truncation contract, thumbnail BMP fallback, session resume identity check, W1a/W1b/W1c session
surfaces, error vocabulary 40/40 + 26/26.

---

## Fix-order recommendation (drives the fix sessions)

1. **M-1** — honor `evt.bytes_done/bytes_total` on CR_EVT_SCAN_PROGRESS when `stage==2`
   (events.rs:266-273): one-line-class fix, un-freezes the carve counter.
2. **M-2** — add `bytes_found` to `cr_ui_telemetry` (tail-append ABI pattern, cr_ui.h already uses
   it for label_full/repair fields) and latch it in ScanLive; alternatively derive from live
   snapshot sums at the 1.5 s refresh cadence.
3. **M-3** — read the file in the webview (`file.text()`) before `loadUserSigns.mutate`.
4. **M-17/L1-L3** — add job cleanup (terminal + report fetched → remove) and a UI-reachable
   `closeScan` connector method; shrink `entry.jobs` on job completion.
5. **M-5** — ship a `requireAdministrator` manifest or NSIS elevation config.
6. **M-8** — wire the four missing listeners (save/load progress at minimum).
7. **M-6/M-9/M-10/M-11** — engine-side bus_type + quick-gate + path/lost-hint ABI appends, or
   truthful UI removal of the dead affordances.
8. **M-18/R1-R3** — terminal-latch guard in scan_close + emitter holds the entry Arc + a
   `finishing` status word for the drain window.
