# WIRING-AUDIT — every seam between the React UI and the cr_ui engine (Agent D, full refresh)

Scope: `ui/src` (React 19) → `ui/src/connector/contract.ts` → `ui/src/connector/tauri/tauri-connector.ts` (`invoke`/`listen`) → `ui/src-tauri/src/commands/*.rs` → `ui/src-tauri/src/bridge/*.rs` (cr_ui.h ABI v4, 93 exports) → engine; events back via `bridge/events.rs` → Tauri emit → frontend `listen()`. Boundary truth: `contracts/` (command-surface.json, error-codes.json, event-mapping.md, type-maps.md, validate.mjs). This is a RESEARCH-ONLY audit; no code was changed.

All paths relative to `/home/z/my-project/work/final-prod`.

---

## 0. HEADLINE FINDING (read this first)

**The Rust bridge's ABI struct layouts are STALE relative to the engine header — engine commit `eb4bb5f3` (D-ENUM-02/D-NVME-01) appended fields to `cr_ui_disk_info` and `cr_ui_smart_report` WITHOUT re-pinning `ui/src-tauri/src/bridge/abi.rs` (last touched by the earlier D4 commit `ce71143e`).** The append-only "zero-init safe for pre-existing hosts" ABI policy is only true for *field semantics*; it is FALSE for *by-value row arrays and engine-filled out-params* when the host struct size no longer matches. Both drifts are live memory-corruption bugs on every machine, and they mechanically reproduce the user's three reported defects (see §4). The ABI version gate (`cr_ui_abi_version() == 4`, lib.rs:98-107 / scan.rs:124-130) cannot catch this (append-only within a version), and `contracts/validate.mjs` checks only names, never layouts.

| Struct | Engine (engine/include/cr/cr_ui.h) | Bridge (ui/src-tauri/src/bridge/abi.rs) | Δ |
|---|---|---|---|
| `cr_ui_disk_info` | ends `… trim_known, trim_on,` **`bus_type`** (:435, commit eb4bb5f3) → sizeof **304** | `CrUiDiskInfo` (:174-189) ends `trim_on` (:188); comment :184-186 still asserts "v4 carries NO bus_type" → sizeof **296** | **+8 stride** |
| `cr_ui_smart_report` | ends `… data_bytes_read, data_bytes_written,` **`power_cycles_nvme`** (:1569) → sizeof **240** | `CrUiSmartReport` (:648-682) ends `data_bytes_written` (:681) → sizeof **224** | **+16 size** |
| `cr_ui_smart_attr` | `… name[40],` **`threshold`** (:1537) → sizeof 72 | `CrUiSmartAttr` (:637-644) ends `name[40]` → sizeof 72 (tail pad absorbs) | 0 (safe; field unread) |

Consequences:

1. **`devices_list` / `devices_refresh` / `resolve_target` (scan_start, session re-attach):** `bridge/util.rs:156-169` allocates `needed × 296` bytes and passes `cap = needed`; the engine (`cr_ui.cpp:1852-1886`) fills `needed` rows at a **304-byte stride** and returns OK → (a) **8 × needed bytes written past the Vec end (heap overflow on every enumeration, any disk count)**; (b) every row after the first is decoded at the wrong offset — row 1's `number` reads row 0's `trim_on` (0 for the plain two-call), row 2's `number` reads the previous row's serial tail (0) → **all disks decode as `disk-0` duplicates with garbage sizes/models/kinds**.
2. **`health_read`:** `commands/health.rs:32` stack-allocates the 224-byte `report`; the engine (`cr_ui.cpp:4892` `memset(rep,0,sizeof(*rep))` + field fills to :4974) writes **240 bytes → a 16-byte stack smash on every health call**, able to clobber adjacent locals (`n`, `needed` at health.rs:21-33) → wrong disk re-queried or spurious failure.

Everything else at the *name* level is wired: see the matrix (§1). The break is *layout*-level, which is exactly the seam every prior session's name-level gates (validate.mjs, ABI version gate, runner DOM probes on single-bus Azure disks where row 0 stays intact) could not see.

---

## 1. Three-way matrix — UI connector op ↔ Rust command ↔ cr_ui ABI export

Surface counts (verified by direct enumeration):
- Connector contract: **54 ops** across 9 domains (44 request ops + 10 event subscriptions) — contract.ts:130-855.
- Manifest: **43 commands** (command-surface.json), **10 events**.
- Rust bridge registers **43 commands** (lib.rs:33-90); the TS connector invokes **42 distinct commands** (getDeviceInfo reuses `devices_list` client-side, tauri-connector.ts:198-206; respondToApplyConsent aliases `consent_respond`, tauri-connector.ts:499-500).
- ABI: **93 `CR_UI_API` exports** in cr_ui.h; bridge binds **92 cr_ui fns + the mangled `cr_log_set_sink`** (abi.rs:686-975) — function-level parity complete (`cr_ui_health_read_ex2` bound but never called).
- **Ops with no backend: 0. Backends with no UI caller: 1 (`scan_close`, internal registry hygiene). Name-level mismatches: 0. Layout/semantic mismatches: see §2.**

### 1a. Commands (op → command → ABI)

| Domain | Contract op (contract.ts) | Command (commands/) | cr_ui ABI calls | Notes / flags |
|---|---|---|---|---|
| devices | listDevices (:131) | devices_list (devices.rs:427) | enumerate_disks, enumerate_volumes, open_disk, open_volume, device_close | ▸ **L1 layout drift** (bus_type), ▸ **M4 kind axis** (no 'sd'), Win32 usage via GetDiskFreeSpaceExW (devices.rs:52-80) |
| devices | getDeviceInfo (:137) | *(client-side over devices_list)* | — | documented reuse; volume-id→owner resolution in TS (tauri-connector.ts:198-206) |
| devices | refreshDevices (:138) | devices_refresh (devices.rs:437) | enumerate_disks/volumes | same composer — same ▸L1 |
| devices | unlockVolume (N-064) (:146) | volume_unlock_dictionary (devices.rs:459) | unlock_volume_with_dictionary | letter-only ids (`vol-L`); ERR_FVE_LOCKED→device-permission |
| scan | startScan (:273) | scan_start (scan.rs:92) | abi_version, open_disk/open_volume(_with_key), device_geometry, scan_create, scan_attach_device, scan_set_event_fn, scan_run (dedicated thread), scan_repair_save_undo (run tail) | ABI gate; NG-21 pre_scan_repair only for volume targets; W1c autosave_dir; consent armed. ▸ resolve_target re-enumerates → **L1 feeds scan targets** |
| scan | pauseScan/resumeScan/cancelScan (:274-276) | scan_pause/resume/cancel (scan.rs:501/519/556) | scan_pause/resume/cancel | transients w/ 2.5 s expiry; cancel force-denies consent gates first (scan.rs:573) |
| scan | getScanState (:277) | scan_state (scan.rs:588) | scan_busy, scan_stopping, scan_telemetry | ▸ **T-poll**: composes from the cached live cell only (scan.rs:610-618); the emitter is the sole telemetry refresher (events.rs:364-415) |
| scan | respondToConsent (:305) | consent_respond (repair.rs:32) | — (registry only) | shared echo-token space with the apply family |
| sessions | cancelSave (N-060) (:547) | scan_save_cancel (scan.rs:696) | scan_save_cancel | save-scoped |
| results | getResultTree (:437) | results_tree (results.rs:888) | scan_snapshot/session_snapshot, snapshot_* | bridge-side tree over NodeMeta cache (G-7) |
| results | queryResults (:442) | results_query (results.rs:483) | (same snapshot path) | bridge-side predicate — `cr_ui_query_run` never used (documented G-7) |
| results | countResults (:449) | results_count (results.rs:510) | — | count_only variant |
| results | getNodeDetail (:448) | node_detail (results.rs:905) | snapshot_node | physical path only for structured+lettered |
| preview | requestPreview (:516) | preview_open (preview.rs:465) | preview_open_snapshot/session, preview_size/metadata/read/unreadable, archive_entry_count/at, preview_close | damage truth: unreadable>0 → preview-damaged |
| preview | getThumbnail (M-102) (:524) | preview_thumbnail (preview.rs:477) | thumbnail_generate (sessions only), preview reads | BMP data-URL encoder bridge-side |
| sessions | saveSession (:544) | session_save (sessions.rs:85) | scan_save_session, scan_get_stats | auto-name in the store; refuses while running |
| sessions | listSessions (W1a) (:548) | sessions_list (sessions.rs:147) | session_list | light rows; unreadable files keep degraded rows |
| sessions | loadSession (W1b) (:549) | session_load (sessions.rs:357) | session_load_with_events, session_get_info, session_stats | load progress from the calling thread; best-effort device re-attach (model+serial) |
| sessions | getSessionMeta (:556) | session_meta (sessions.rs:373) | session_list | row lookup, no load |
| sessions | resumeLastSession (M-154) (:560) | session_resume_last (sessions.rs:412) | session_last_autosave_path + load family | ERR_RANGE_OUT_OF_BOUNDS → session-not-found |
| sessions | resumeSession (:567) | session_resume (sessions.rs:454) | session_resume, scan_attach_device, scan_verify_identity, scan_set_event_fn, scan_run | borrowed device pinned via jobs list; identity verify → session-corrupt |
| recovery | startRecovery (:645) | recovery_start (recovery.rs:52) | recovery_create/set_destination/enqueue_node/subtree/snapshot/subtree_snapshot, queue_count; process on dedicated thread | options map: collision/verifyOnWrite/rebuildFolders only (BF-14: keepEmptyFolders/fileRepair dropped); 'all' pages the bridge query at 500 |
| recovery | cancelRecovery (:651) | recovery_cancel (recovery.rs:368) | recovery_cancel | terminal = no-op |
| recovery | getRecoveryState (:652) | recovery_state (recovery.rs:388) | — (registry) | |
| recovery | getRecoveryItems (:653) | recovery_items (recovery.rs:438) | recovery_report_json | outcome map done/salvaged→done etc. |
| recovery | getRecoveryReport (:661) | recovery_report (recovery.rs:484) | recovery_report_json | per-list cap 500 + truncatedItemLists |
| health | getHealth (:694) | health_read (health.rs:19) | health_read (base only) | ▸ **L2 layout drift** (power_cycles_nvme); NG-6/NG-9 NVMe truth; threshold column nulled (BF-03; ex2 exists unbound) |
| repair | dryRunRepair/applyRepair/getUndoLog/undoRepairs (:733-739) | repair_dry_run/apply/undo_log/undo (repair.rs:41-65) | — | typed not-implemented (G-1 gap cluster) |
| lostPartition | applyLostPartition (S-029) (:777) | lost_partition_apply (lost_partition.rs:53) | open_disk, apply_lost_partition, device_close | single-flight per device; consent armed; geometry from scan-state rows |
| lostPartition | undoLastApply (:783) | lost_partition_undo (:192) | apply_undo_last | |
| lostPartition | saveApplyUndo (:785) | lost_partition_save_undo (:213) | apply_save_undo | |
| lostPartition | respondToApplyConsent (:789) | consent_respond (shared) | — | |
| lostPartition | pendingApplyConsent (:791) | apply_consent_pending (:234) | — (registry) | re-hydration |
| system | getAppInfo (:825) | system_app_info (system.rs:41) | abi_version, user_sign_count | returns connectorMode "tauri" (U-01) — ▸M5 doc drift in type-maps §10 |
| system | loadUserSigns (M-048) (:836) | system_load_user_signs (system.rs:119) | register_user_signs, user_sign_count | exe-adjacent defaults auto-loaded at startup (lib.rs:110) |
| system | validateDestination (:837) | system_validate_destination (system.rs:210) | enumerate_volumes | same-letter overlap (F12); real write probe |
| system | pickDirectory (:849) | system_pick_directory (system.rs:285) | — (dialog plugin) | hint is accepted but ignored (`_hint`, system.rs:287) |
| system | listMountPoints (:853) | system_list_mounts (system.rs:257) | enumerate_volumes | letters + GetDiskFreeSpaceExW |
| system | openPath (:854) | system_open_path (system.rs:307) | — (opener plugin) | folders open, files revealed |
| *(internal)* | — | scan_close (scan.rs:659) | scan_close | refuses while busy/jobs live; the only backend with no UI caller |

### 1b. Events (UI listener ↔ bridge emitter ↔ engine event)

| Event name | UI subscription (tauri-connector.ts) | Bridge emitter | Engine source | Payload parity |
|---|---|---|---|---|
| `scan-progress:<scanId>` | onScanProgress (:236-246) | emitter thread (events.rs:414) + run tail (scan.rs:446-453) + scan_state | CR_EVT_SCAN_PROGRESS/STAGE/FILES_FOUND/ERROR_RANGE + telemetry snapshot | ✅ all 17 contract fields (events.rs:175-194); currentPath always null (G-2) |
| `scan-finished:<scanId>` | onScanFinished (:250-260) | run thread (scan.rs:455-458) | run() rc latch | ✅ {status, failure?, autosaved} |
| `save-progress:<scanId>` | onSaveProgress (:262-272) | emitter (events.rs:341-347) | CR_EVT_SAVE_PROGRESS (9) | ✅ {fraction} |
| `lost-candidate:<scanId>` | onLostCandidate (:274-284) | emitter (events.rs:298-325) | CR_EVT_LOST_CANDIDATE (6) + N-061 log-seam join (logging.rs:71-126) | ✅ {lba, sectorCount?, fs?, score?, count} |
| `scan-autosaved:<scanId>` | onScanAutosaved (:286-296) | emitter (events.rs:353-361) | CR_EVT_AUTOSAVED (12) | ✅ {rc} |
| `consent-request:<scanId>` | onConsentRequest (:302-312) | consent sink on the run thread (consent.rs:158-160) | cr_ui_repair_consent_fn | ✅ ConsentRequestEvent (consent.rs:136-147) |
| `apply-consent-request:<deviceId>` | onApplyConsent (:487-497) | apply consent sink (consent.rs:214-217) | cr_ui_apply_consent_fn | ✅ ApplyConsentEvent (consent.rs:189-203) |
| `session-load-progress:<path>` | onSessionLoadProgress (:371-384) | calling-thread sink (events.rs:77-102) | CR_EVT_SESSION_LOAD_PROGRESS (11) | ✅ {path, permille, nodesDone, nodesTotal} |
| `recovery-progress:<jobId>` | onRecoveryProgress (:416-426) | job emitter (events.rs:528-529) + terminal (events.rs:601) | recovery progress sink + stats/report drain | ✅ all 13 fields (events.rs:566-583); currentFileProgress always 0 |
| `recovery-finished:<jobId>` | onRecoveryFinished (:428-438) | process thread (events.rs:603-606) | process() rc latch | ✅ {status} |

All 10 W-01 dead-letter events are now listened for and emitted. Name-level event parity is complete.

---

## 2. Mismatch register (both sides cited)

**Layout / memory-safety (CRITICAL — new findings):**

- **L1 — `cr_ui_disk_info` row-stride drift + heap overflow.** Engine: cr_ui.h:435 (`uint32_t bus_type;`, commit eb4bb5f3) and cr_ui.cpp:1865 (`o.bus_type = d.bus_type;` — written by the PLAIN `cr_ui_enumerate_disks` too). Bridge: abi.rs:174-189 (no bus_type; stale comment :184-186 "v4 carries NO bus_type"), buffer sized `needed × 296` at util.rs:163-167. Engine writes `needed × 304`. Result: 8×needed-byte heap overflow + every row ≥ 1 decoded shifted (ids collapse to `disk-0`, sizes/models/kinds garbage). Affects devices_list/refresh/getDeviceInfo/resolve_target (scan_start, sessions.rs attach_source_device:667-738).
- **L2 — `cr_ui_smart_report` out-param overflow.** Engine: cr_ui.h:1569 (`power_cycles_nvme`, eb4bb5f3), cr_ui.cpp:4892 memset 240 B. Bridge: abi.rs:648-682 (224 B), local at health.rs:32 → 16-byte stack smash per health_read.
- **L3 — `cr_ui_smart_attr.threshold` append (SAFE).** cr_ui.h:1537 vs abi.rs:637-644: both sizeof 72 (tail padding) — no corruption; bridge nulls the column (health.rs:130) per BF-03. Re-pin + adopt `cr_ui_health_read_ex2` (bound at abi.rs:940-946, never called) with base-export fallback to light the Threshold column.

**Semantic / axis gaps:**

- **M4 — bus_type → `Device.kind` never mapped.** Engine now provides the SD/MMC axis (bus 12/13); bridge devices.rs:215 still emits only `usb`|`internal`. Contract DeviceKind includes 'sd' (contract.ts:28); sd-card-page.tsx:30-34 filters `kind === 'sd' || 'usb'` and its `find(kind === 'sd')` NEVER matches — SD cards can never be first-class on the SD page even after L1 is fixed. (This was the stated PURPOSE of the engine-side D-ENUM-02 append — the host half never landed.)
- **M5 — type-maps.md §10 says `connectorMode` is REMOVED**; system.rs:54 returns it and the UI depends on it (queries/dev.ts:12, About badge). Code is right, doc is stale.
- **M6 — type-maps.md §5 says extensions cross "lowercased WITH dot"**; the bridge strips dots (results.rs:119) and compares dot-less (state.rs:784-789) — internally consistent, doc drift only (the engine-side `cr_ui_query` path is unused anyway).
- **M7 — `ResultsPage.totalBytes`** is the FULL filtered sum (results.rs:453-459, 472-479); type-maps §5 says "page sum". Richer than documented; harmless.
- **M8/M9 — additive payload fields** the contract doesn't declare: health `transport`, `tableEmpty` (health.rs:151-152); session rows `valid` (sessions.rs:219). Additive, harmless.
- **M10 — error-translation gaps.** (a) `engine_failure_code` (scan.rs:465-473) maps only 5 engine codes; every other run() failure lands as `unknown` in `scan-finished.failure`/`ScanState.failure.code` (e.g. a blind-scan ERR_READ_FAILED 0x50020002 → 'unknown', though the TS numeric table has a row for it — tauri-connector.ts:101). (b) error-codes.json:14-20 documents an ERR_OPEN_FAILED→device-gone refinement "when enumeration no longer lists the device" that the bridge never implements (devices.rs probe_status:84-90 always 'permission-denied'). (c) `preview-timeout` and `preview-encrypted` are UI vocabulary members with reasonKeys (tauri-connector.ts:85-89) that NO bridge or engine path ever produces — dead vocabulary (the UI renders a generic message if they ever appear).
- **M11 — `DeviceKind` 'optical'/'virtual'** are never produced by the bridge (and the engine's volume enumeration skips DRIVE_CDROM/UNKNOWN entirely — cr_enumerate.cpp:295-296); cosmetic.
- **M12 — contract `ScanStage` includes `'tree-build'`** (contract.ts:166) but the bridge stage map only knows quick-index/deep-walk/carve + terminal 'done' (events.rs:256-264); 'tree-build' is never emitted (organizing-phase label collapses to 'done'-adjacent behavior). Minor.
- **M13 — designed gaps re-confirmed (documented, not defects):** `currentPath` always null (G-2; the "Searching:" ticker at scan-progress-page.tsx:155-164 can never render); `quickScanAvailable`/`canScanWholeDisk` hard-coded true (devices.rs:180, 228); `Volume.lostPartition` absent pre-scan (G-10); recovery `keepEmptyFolders`/`fileRepair` silently dropped (recovery.rs:100-113, BF-14); RepairDomain = typed not-implemented (repair.rs:41-65).

---

## 3. Threading / lifecycle seams

- **T1 — command threading is clean.** All 43 commands are `async`; every engine-touching one uses `tauri::async_runtime::spawn_blocking` (e.g. devices.rs:431, scan.rs:102, health.rs:20); `consent_respond`, `system_app_info`, `system_open_path`, the repair gap trio run sync-class (registry/plugin only). No command blocks the Tauri main thread.
- **T2 — scan/recovery run on dedicated `std::thread`s** (scan.rs:362, recovery.rs:318); run/process are synchronous on their threads per the ABI contract. Engine event sinks are non-blocking `try_send` into bounded channels (512 scan / 64 recovery — events.rs:46-66, scan.rs:257, recovery.rs:117); single emitter thread per handle drains and coalesces; terminal events are latched by the run/process threads, never the emitter (events.rs:13-15).
- **T3 — consent gates block the engine run thread indefinitely by design** (consent.rs:90-96, no timeout); `scan_cancel` force-denies first (scan.rs:569-577), the run tail closes survivors (scan.rs:374-377), and the apply path closes device gates in its tail (lost_partition.rs:113-116). Deadlock discipline (§4.5: no registry lock held while blocking) is honored (consent.rs:148-163).
- **T4 — close/lifetime discipline.** `scan_close` refuses while busy or while jobs/resumed scans reference the entry (scan.rs:664-687); `ScanEntry::Drop` enforces unsub→sink-box-free→snapshot→scan→device order (state.rs:291-327); resumed scans borrow the loaded entry's device pinned via its jobs list (sessions.rs:584-591). `SendPtr` send/sync is the documented bridge discipline (util.rs:12-31).
- **T5 — poll-side counter refresh gap (real, secondary).** `scan_state` composes progress ONLY from the cached `ScanLive` cell (scan.rs:610-618); the sole refresher of `bytes_scanned`/`files_found`/`data_found_bytes`/ETA is the emitter's event-driven telemetry snapshot (events.rs:364-415). If the engine's event cadence stalls (a D4e-class freeze in an unswept code path), the 500 ms state poll cannot recover the counters — the UI freezes with the engine. (The engine now has byte-axis triggers, cr_scan.cpp:642-673, so this is resilience, not the current root cause.)
- **T6 — emitter permanent exit on a poisoned lock** (`let Ok(mut live) = live.lock() else { break }`, events.rs:253): one panic anywhere while holding `live` kills progress forever with no restart. Defense gap.
- **T7 — events after listener gone** are Tauri emit no-ops (safe dead letters in the harmless direction). Store teardown unsubscribes when the last listener leaves (queries/scan.ts:69-84).
- **T8 — window close / navigation:** no on-exit hook; run/process threads keep the engine busy until process teardown; the exit guard consults `readScanState` (app/exit-guard.tsx, queries/scan.ts:305-307). Navigation unmounts the poller; re-hydration paths exist (consent via `pendingConsent`, sessions via `pendingApplyConsent`).
- **T9 — `session_load`** runs `cr_ui_session_load_with_events` synchronously on the blocking pool (sessions.rs:363-368) — a large session ties one pool thread; acceptable, bounded.

---

## 4. DEFECT-TRACE — the three user-reported defects, ranked break points

### D1 — "disks missing from the disks page (2 internal + 1 SD)"

1. **[CRITICAL — most likely] L1 struct drift (§0).** `enumerate_disks` (util.rs:156-169) vs the engine's 304-byte rows (cr_ui.cpp:1852-1886): rows ≥ 1 decode shifted → `number` reads 0 (row 1) / serial-tail zeros (rows 2+) → **every disk row gets id `disk-0`** → home-page.tsx:260-292 renders `key={device.id}` duplicates → React collapses/dedups rows ("2 internal missing"), sizes/models are garbage, and the 8×N heap overflow destabilizes the page. Also `refreshDevices` (same composer) reproduces it on every refresh. **Why CI never saw it:** the v0.9.0 forensic run's Home census (RELEASE-RUN.md) pre-dates eb4bb5f3, and Azure runner disks are single-bus virtual disks where row 0 (the only intact row) is the pattern — multi-disk runners would have caught it.
2. **[HIGH, compounding] M4 kind axis:** even with layouts fixed, the bridge never maps bus_type 12/13 (SD/MMC) → `kind: 'sd'`; the SD page's first-class card (`sd-card-page.tsx:31-35`) never matches, and pre-eb4bb5f3 the engine mislabeled SATA as removable (the original D-ENUM-02 complaint — the engine half is fixed, the host half is not).
3. **[LOW] engine volume filter** drops DRIVE_UNKNOWN/CDROM (cr_enumerate.cpp:295-296) — optical/odd volumes invisible by design.

**Fix seam:** add `bus_type: u32` to `CrUiDiskInfo` (abi.rs:174-189) and map kind = bus 7→usb, 12/13→sd, else internal (devices.rs:215 + compose_volume_row path), then re-validate on a multi-disk host.

### D2 — "SMART only for the first disk; switching shows nothing"

1. **[CRITICAL — most likely] the D1 row scramble:** the health page's device list comes from `devices_list` (health-page.tsx:43-52 via use-physical-devices.ts:20-24); with all ids `disk-0`, `list.find(d => d.id === deviceId)` always resolves to the first row → every selection re-queries disk 0 (or a garbage id that fails) — exactly "SMART only for the first disk, switching shows nothing".
2. **[CRITICAL, independent] L2 stack smash:** every `health_read` writes 16 bytes past the bridge's `report` local (health.rs:32 vs cr_ui.cpp:4892+): can clobber `n`/`needed` (health.rs:21-33) → the second engine call re-reads disk 0 or sizes the attrs buffer wrong; with `retry: false` (queries/health.ts:11) the query dies and the pane renders nothing.
3. **[MEDIUM] D-NVME-01 residue:** the engine's NVMe protocol-query fix (eb4bb5f3) is in — NVMe-only drives now report; if a specific controller still refuses, health.rs:54-59 turns it into the health-unavailable error path (empty pane).
4. **[LOW] `cr_ui_health_read` opens GENERIC_READ|GENERIC_WRITE** (cr_ui.cpp:4896) — a disk exclusively held by another process fails the open → error pane.

**Fix seam:** add `power_cycles_nvme: u64` to `CrUiSmartReport` (abi.rs:648-682); optionally bind threshold via `cr_ui_health_read_ex2` with base fallback.

### D3 — "scan stuck at 75% 'advanced scanning', files found 21 (0 B), no sector progress"

Context: overall = (40·quick + 35·deep + 25·carve)/100 (cr_scan.cpp:131-146; zero-init weights ⇒ 40/35/25) — **75% is exactly quick+deep complete with carve at 0 permille**; the "advanced" label is `scanPhaseOf(stage ∈ {deep-walk, carve})` (contract.ts:171-175; scan-progress-page.tsx:90, 186). "Files Found: N (size)" renders `progress.dataFoundBytes` (:190-194); "Reading sector: X/Y" renders `bytesScanned/512` (:131-132, 264-269).

1. **[CRITICAL — most likely on the CURRENT tree] the L1 heap overflow fires in the scan path too:** `scan_start`→`resolve_target` re-enumerates (scan.rs:132 → devices.rs:246-321) → 8×N bytes of heap corruption immediately before `scan_create/attach/run`, plus a possibly-wrong `bytes_total` (geometry of a scrambled target). The resulting UB can freeze any of the run/emitter/registry machinery — a single root cause unifying all three defects on a multi-disk machine. (Note the user's machine has ≥3 disks — the overflow is guaranteed there; it is also present on single-disk machines at 8 bytes.)
2. **[HIGH if the install is stale] build/deploy seam:** D4a (live carve bytes, cr_scan.cpp:508-518), D4b (live found-size, :589-592 + event stamping :609), D4e (remnant-sweep ticks, RELEASE-RUN.md) are all in the tree and were runner-validated at v0.9.0 — the frozen-75%/0 B signature is the PRE-D4 engine's exact behavior (worklog/UI-MAP.md:112-132, DIAGNOSIS.md:85-110). If the user's installed build predates the current engine lane, that alone reproduces the report verbatim. Verify via the app log's ABI-gate line and the install version before anything else.
3. **[MEDIUM — real wiring weakness either way] T5/T6:** the 500 ms `scan_state` poll never re-reads `cr_ui_scan_telemetry` itself (scan.rs:610-618 — counters come only from the emitter's event-driven refresh, events.rs:364-415), and the emitter dies permanently on a poisoned lock (events.rs:253). Any engine-side cadence stall becomes an unrecoverable UI freeze. Adding a telemetry refresh inside `scan_state` (and/or a heartbeat-driven refresh) would make the poll the true fallback it claims to be.
4. **[LOW] overall is event-fed only** (events.rs:266-271): a dropped CR_EVT_SCAN_PROGRESS window parks `overall` until the next event even if telemetry moved.

**Fix seam (in order):** re-pin abi.rs structs (L1+L2+L3), rebuild, reinstall; then verify on the real machine; then harden scan_state with a direct telemetry read.

---

## 5. Missing / dead API surface

**Engine ABI exports bound but never called by the bridge (23 — declared in abi.rs under the documented "complete header binding" allow, abi.rs:29-38):** `cr_ui_open_image`, `cr_ui_last_transport_error`, `cr_ui_device_transport`, `cr_ui_enumerate_disks_ex`, `cr_ui_session_load` (plain — the _with_events twin is used), `cr_ui_scan_assistant`, `cr_ui_session_regrade`, `cr_ui_scan_checkpoint_count`, `cr_ui_query_run`, `cr_ui_query_rows_free`, `cr_ui_query_apply_buckets`, `cr_ui_carve_format_count`, `cr_ui_carve_format_info`, `cr_ui_scan_carve_tally_count`, `cr_ui_scan_carve_tally`, `cr_ui_user_sign_info`, `cr_ui_disk_map`, `cr_ui_disk_map_free`, `cr_ui_thumbnail_format_supported`, `cr_ui_health_read_ex2`, `cr_ui_image_create`, `cr_ui_fsck_volume`, `cr_ui_turn_off_bitlocker`. (The capability map's DISABLE rows; the new `bus_type` axis is the one that SHOULD leave this list.)

**UI/design-implied surface with no backing:** `DeviceKind 'sd'` (engine axis exists, host mapping missing — M4); 'optical'/'virtual' kinds; SMART Threshold column (ex2 exists unbound); RepairDomain (4 ops, typed not-implemented); `currentPath` ticker (G-2); `preview-encrypted`/`preview-timeout` vocabulary; `SessionMeta.dataFoundBytes` on the session-save return uses the latched terminal value only.

**UI ops with no dedicated command (by design):** `getDeviceInfo` (client-side over devices_list), `respondToApplyConsent` (aliases consent_respond).

**Backend with no UI caller:** `scan_close` (internal).

**Fake/mock-only surface:** the MockConnector remains the test double behind `VITE_CONNECTOR=mock` (connector/index.ts:23-37); its event cadence/weights differ from the real engine (mock 5-stage weights vs engine 40/35/25), which is why mock-driven tests never reproduced the 75% plateau — the shipped build never carries it.

**Gate gaps that let L1/L2 through:** the ABI gate compares the version number only (lib.rs:98-107, scan.rs:124-130); `contracts/validate.mjs` cross-checks command/event/error NAMES (validate.mjs:39-131) but never struct layouts or sizes. Recommended: a layout-pin lane (checked-in offset/size manifest generated from cr_ui.h, asserted against `mem::size_of`/`offset_of` in a Rust unit test; plus a multi-disk forensic-repro gate).

---

## 6. Bottom line

Name-level wiring is complete and healthy: 42/42 invoked commands implemented and manifest'd; 10/10 events emitted with payload parity; error model consistent at the vocabulary level. The user's three defects trace to a **single un-pinned ABI re-pin miss** (engine eb4bb5f3 appended `cr_ui_disk_info.bus_type` and `cr_ui_smart_report.power_cycles_nvme` without touching `ui/src-tauri/src/bridge/abi.rs`) — producing a guaranteed heap overflow + row scramble in every enumeration (disks missing, health keyed to `disk-0`) and a per-call stack smash in health, with the scan freeze explained either by the same UB or by a stale install (D4 fixes are present in-tree). Secondary hardening: map bus_type→kind 'sd', refresh telemetry inside scan_state, restart-proof the emitter, and add a struct-layout CI gate so append-only ABI drift can never ship silently again.
