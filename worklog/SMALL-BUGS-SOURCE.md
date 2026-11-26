# SMALL-BUGS-SOURCE — Audit Agent A (source audit)

Scope: individually-small, product-wrecking defects (swallowed errors, leaks, stuck states, wrong numbers, unbounded work) across the Rust bridge, the TS connector/queries/features, and the engine scan pipeline. RESEARCH ONLY — no files were modified except this one.

## Method

- **Files read: 64** — 26 Rust (all of `ui/src-tauri/src/bridge/*.rs`, all 15 `commands/*.rs`, `lib.rs`, `main.rs`), 32 TS/TSX (connector `tauri-connector.ts` + `connector/index.ts`, both workers, all 5 stores, 8 query modules, hooks, 3 lib modules, 11 feature/component files), 6 engine C++ (`cr_scan.cpp` ~40%, `cr_ui.cpp` targeted, `cr_snapshot.cpp`, `cr_runlist.cpp`, `cr_safewriter.cpp` targeted, `cr_parallel.cpp`).
- Approach: full reads for the bridge/commands/connector/stores/workers; feature files read end-to-end where defect-dense (scan-progress, results-explorer, virtual-file-list, preview-pane, exit-guard, consent-modal); engine read targeted at the scan pipeline (cancel checkpoints, progress math, hot loops, event emission order, snapshot contracts). Cross-checks performed: every `#[tauri::command]` vs the `lib.rs` `invoke_handler` list (all 60 registered — verified both directions); every TS `listen()` call site vs unmount cleanup; every mutation call site for `onError` presence; `cr_ui_snapshot_node`'s TRUNCATED contract vs the bridge's retry.
- Static defects below are distinguished from runtime-proof-needed suspicions. Where the code handles a case correctly, it is not reported (several heavily-commented prior fixes — e.g. the P-HB-01 heartbeat emission, the 'stopping' poll fix, U-03 user-signs import — were re-verified as correct and are listed under Clean areas).

## Findings

### SB-A-01 — full metadata-cache deep clone on every results query (P1, certain)
- **File:** `ui/src-tauri/src/bridge/state.rs:826-842` (`metas_snapshot`), consumed by `commands/results.rs:500` (`query_page`), `tree_page:633`, `node_detail:964`, and the recovery enqueue paging loop (`commands/recovery.rs:204`).
- **Category:** unbounded per-request allocation / performance spike.
```rust
let (metas, partial) = {
    let snap = entry.snap.lock().map_err(|_| poisoned())?;
    let metas = snap.metas.clone().ok_or_else(|| { ... })?;
    (metas, snap.partial)
};
```
- **Why a defect:** `metas` is `Vec<NodeMeta>` where every row carries two heap `String`s (`name`, `path`). This clones the ENTIRE cache — O(total tree rows) memory copy + one allocation per string — on **every** `results_query` page fetch (each 200-row page), every `results_count`, every tree-page expansion, every node-detail, and every re-fetch triggered by the scan-progress invalidations (tree+results+count are invalidated at every 10% progress bucket and at every lost-candidate flush, throttled to 1/s). For a 1M-file tree the cache is hundreds of MB (the T14 test itself budgets < 1 KiB/node); scrolling the results list deep-clones it per page. The T14 measurement pass measures the filter predicate, not this clone.
- **Impact:** UI-wide latency and memory spikes that grow with tree size exactly when the user is paging/scrolling results or watching progress; on a big restore this is the "spike/stall" class.
- **Fix direction:** return `Arc<Vec<NodeMeta>>` (or borrow under the lock) instead of cloning; the cache is already generation-stable under `entry.snap`'s mutex.

### SB-A-02 — `scan_close` is never called by the UI — every scan/session leaks its entry for the app run (P1, certain)
- **File:** `ui/src-tauri/src/commands/scan.rs:819-852` (the command); `ui/src-tauri/src/lib.rs:57` (registered); `ui/src/connector/tauri/tauri-connector.ts:267-390` (the scan surface — no `closeScan` method exists); grep of `ui/src` for `scan_close|closeScan` returns **zero matches**.
- **Category:** broken lifecycle / dead handler (UI side) / unbounded accumulation.
```rust
// scan.rs:843-847 — removal only happens here
state.scans.lock()...
    .remove(&scan_id);
```
- **Why a defect:** `contracts/command-surface.json` documents `scan_close` as the "UI surface" for teardown, and `ScanEntry::Drop` implements the full ordered teardown (unsubscribe event sink → free sink box → close snapshot cache → close scan → close device). But the connector exposes no method that invokes it, and no feature calls it. Every `scan_start`, `session_load`, `session_resume`, and `session_resume_last` inserts a permanent registry entry holding: the engine `CrUiScan`/`CrUiSession` handle, the **open device handle**, the frozen snapshot + the built metadata cache (the same O(n) cache from SB-A-01, retained forever), the 512-deep event channel + boxed sender, and one emitter thread per live scan. Recovery `JobEntry`s and imaging `ImageEntry`s are likewise never removed (jobs registry has no removal path at all).
- **Impact:** memory grows with each scan/browse of a session in one app run (a 1M-row tree keeps its full metadata cache resident); open device handles accumulate per scan (Windows handle pressure across a long session); one parked thread + channel per scan.
- **Fix direction:** add `closeScan` to the connector contract and call it when the results route leaves a scan (or on new-scan-start for the previous one), plus a job-reaper for terminal recovery jobs.

### SB-A-03 — recover-all while a scan is still running pages a shifting window (P1, needs-runtime-proof)
- **File:** `ui/src-tauri/src/commands/recovery.rs:145-151` (snap_ptr captured once) + `202-252` (the paged "all" loop) + `bridge/state.rs:640-676` (`refresh_snapshot` installs a new generation when >1500 ms since last refresh).
```rust
let snap_ptr = { let snap = entry.snap.lock()...; snap.handle.get() };  // once, before the loop
...
loop {
    let page = query_page(state, &scan_id, &filter, "name", false, PageReq { offset, limit: 500, ..})?;
    ...
    offset += fetched; if fetched < 500 { break; }
}
```
- **Why a (probable) defect:** `query_page` → `metas_snapshot` → `refresh_snapshot` may install a NEW snapshot generation between pages (live scans re-snapshot at ≥1500 ms cadence; the loop over a big tree takes far longer). The matches are recomputed against the newer, larger tree each page, but the `offset`-based window indexes into a **sorted, shifting** list: files discovered mid-scan sort before the current window and are silently skipped (or duplicated) — "recover all" mid-scan recovers a set that is not "all". The enqueue also uses `snap_ptr` captured before the loop while indices were minted against the refreshed generation. Mitigating finding (why it is *not* certain): the engine snapshot's node order is the arena append order (`engine/src/core/tree/cr_snapshot.cpp:28-63`) and mid-scan discovery appends, so pre-existing arena indices stay stable across generations — the wrong-file-by-index scenario requires mid-loop pruning/reordering, which the bridge's whole-volume scans don't trigger. The skip/duplicate window drift does not need reordering, only growth + sort instability, both real.
- **Impact:** under-recovery (missing files) for the marquee "recover all during scan" flow, silently.
- **Fix direction:** loop until `total` stops changing (re-compute `total` each iteration and restart from 0 when it grows), or pin the generation (freeze the snapshot at loop start) for both query and enqueue.

### SB-A-04 — lost-candidate cap: count saturates and the event pairs the new LBA with the previous row's geometry (P2, certain)
- **File:** `ui/src-tauri/src/bridge/events.rs:412-431`.
- **Category:** wrong number (bounded structure semantics).
```rust
let geom = crate::bridge::logging::take_candidate_geometry(evt.candidate_lba);
if live.lost_candidates.len() < LOST_CANDIDATE_CAP {   // 4096
    live.lost_candidates.push(LostCandidateRow { ... });
}
let count = live.lost_candidates.len();
let row = live.lost_candidates.last().copied();
let _ = app.emit("lost-candidate:{scan_id}", json!({
    "lba": evt.candidate_lba,
    "sectorCount": row.and_then(|r| r.sectors), ...
```
- **Why a defect:** once the 4096-row cap is reached, the new candidate is NOT pushed but the event still emits the NEW `lba` paired with the OLD last row's `sectors`/`fs`/`score` — a fabricated pairing (a lost-partition candidate shown with another candidate's size/FS/score). The `count` field (and `scan_state`'s `lostCandidates.count`, `scan.rs:808-811`, which also uses `len()`) saturates at 4096, while `state.rs:37-38` documents the list as "bounded, count-exact" — the bad-sector map keeps its count exact via a separate `errors` counter; the lost-candidate list has no such counter.
- **Impact:** on a heavily-damaged deep-scanned disk with >4096 scored candidates (plausible on carved disks), the lost-partition surface shows mismatched geometry per row and a frozen count. Degraded UX / wrong numbers, not a crash.
- **Fix direction:** keep an exact `lost_candidate_total` counter; emit `sectorCount/fs/score: null` (or the matched row) only when the row was actually pushed for this LBA.

### SB-A-05 — `scan_start` error paths leak the opened device, the leaked consent pipe, and the scan handle (P2, certain)
- **File:** `ui/src-tauri/src/commands/scan.rs:183-346` (and the image twin `357-495`).
- **Category:** resource leak on fallible path.
```rust
let consent_user = SendPtr::new(Box::into_raw(Box::new(ConsentPipe { ... })));   // :249
...
check(unsafe { abi::cr_ui_scan_create(&cfg, &mut scan) }, "cr_ui_scan_create")?;  // :258 — leaks dev + pipe on Err
check(unsafe { abi::cr_ui_scan_attach_device(scan, dev) }, ...)?;                  // :263 — leaks scan + dev + pipe
check(unsafe { abi::cr_ui_scan_set_event_fn(...) }, ...)?;                         // :273 — same
```
- **Why a defect:** after `cr_ui_open_disk/volume` succeeds and the `ConsentPipe` is `Box::into_raw`'d, every `?` return (scan_create failure, attach failure, set_event_fn failure, registry-lock failure) abandons the opened `CrUiDevice` handle (never `cr_ui_device_close`'d), the raw pipe box (freed only by the run tail, which never launches), and — after create — the scan handle. Scan-start failures are realistic (locked volume, gone device mid-open, engine refusal on attach).
- **Impact:** engine-side handle/heap leak per failed scan attempt; repeated failed attempts (locked-volume retry loop) compound it. No user-visible crash; pressure on engine resources.
- **Fix direction:** an RAII guard or explicit close-on-error before each `?` (the same pattern `lost_partition_apply_blocking` already gets right — device closed before `check(rc)`).

### SB-A-06 — `recovery_start` error paths leak the created recovery handle and the boxed sink sender (P2, certain)
- **File:** `ui/src-tauri/src/commands/recovery.rs:127-285`.
- **Category:** resource leak on fallible path.
```rust
let (tx, rx) = sync_channel::<(u64, u64)>(64);
let sink_user = Box::into_raw(Box::new(tx.clone()));
check(unsafe { abi::cr_ui_recovery_create(dev, &cfg, &mut rec) }, ...)?;      // :128
check(unsafe { abi::cr_ui_recovery_set_destination(rec, dest.as_ptr()) }, ...)?;  // :136 — leaks rec + sink box
```
- **Why a defect:** on `set_destination` refusal (a **realistic** user path — invalid/overlapping destination), or any enqueue refusal in the "nodes"/"all" loops, the created `CrUiRecovery` handle is never `cr_ui_recovery_close`'d and the raw `SyncSender` box is never freed. The `JobEntry::Drop` that owns them is only constructed on the success path.
- **Impact:** engine recovery-handle + channel-box leak per failed recovery start; users who retry after a destination validation error compound it.
- **Fix direction:** close `rec` + `Box::from_raw(sink_user)` on every early `Err` after create.

### SB-A-07 — `session_resume` error paths leak the fresh scan handle and the consent pipe (P2, certain)
- **File:** `ui/src-tauri/src/commands/sessions.rs:529-624`.
- **Category:** resource leak on fallible path.
```rust
let consent_user = SendPtr::new(Box::into_raw(Box::new(ConsentPipe { ... })));   // :529
check(unsafe { abi::cr_ui_session_resume(session.get(), &cfg, &mut scan) }, ...)?;  // :538 — leaks pipe
check(unsafe { abi::cr_ui_scan_attach_device(scan, dev_ptr) }, ...)?;                // :544 — leaks scan + pipe
check(unsafe { abi::cr_ui_scan_set_event_fn(scan, ...) }, ...)?;                     // :566 — same
```
- **Why a defect:** the consent pipe is freed only in the run tail (success path); the scan handle is closed only on the identity-mismatch branch (`:555`). Every other early error leaks both. `load_session_entry` itself is clean (pipe freed unconditionally at `:277`).
- **Impact:** same class as SB-A-05/06; a failing resume (attach refusal on a flaky USB source) leaks per attempt.
- **Fix direction:** mirror the identity-mismatch branch's `cr_ui_scan_close` + `Box::from_raw` on all early errors.

### SB-A-08 — stop/cancel (and pause/resume) mutations swallow failures — a refused stop is a silent no-op (P2, certain)
- **File:** `ui/src/features/scan-progress/scan-progress-page.tsx:431-438` (scan stop), `:260` and `:271` (pause/resume buttons); `ui/src/features/recovery-queue/recovery-page.tsx:629` (recovery cancel).
- **Category:** swallowed error / stuck state.
```tsx
// scan stop — onSuccess only; onError absent at both hook and call level
void cancel.mutate(undefined, {
  onSuccess: () => { markStopped(); ... void navigate(`/results/${scanId}`); },
});
...
// recovery cancel — no callbacks at all
cancel.mutate();
```
- **Why a defect:** `cr_ui_scan_cancel` / `cr_ui_recovery_cancel` are fallible engine calls (refused states, device-gone during drain). On rejection the confirmation dialog has already closed, no toast fires, no state changes — the scan keeps running with zero feedback (the user believes they stopped it). The sibling flows get this right (`resume.mutate` at `:381` has `onError`; `saveSession` at results-explorer:594 has one with an explicit honesty comment; home-page `beginScan`, image start/stop all carry `onError`).
- **Impact:** broken-button feel exactly on the destructive action; user waits on a scan they asked to stop.
- **Fix direction:** add `onError: (err) => toast.error(...)` to the three call sites (the codebase's own established pattern).

### SB-A-09 — icon (tile) view truncates at 120 rows with no loader or continuation (P2, certain)
- **File:** `ui/src/components/ui/virtual-file-list.tsx:428`.
- **Category:** missing data / broken view mode.
```tsx
function IconGridBody(...) {
  ...
  const visibleRows = rows.slice(0, 120);   // hard cap, no virtualization, no loadMore wiring
```
- **Why a defect:** the details view is virtualized with an infinite-scroll loader (`loadMore`/`hasNextPage`), but the icon grid renders only `rows.slice(0, 120)` of the loaded pages, is not virtualized, and never calls `loadMore` — every result past tile 120 is unreachable in icon view, with no "more" affordance or truncation notice. `rows` already holds up to the loaded pages (200+), and `total` may be thousands.
- **Impact:** silent data loss in a first-class view mode; user switching to icons on a large result set cannot reach most files.
- **Fix direction:** virtualize the grid (the virtualizer is already in the file) or wire `loadMore` + a truncation hint.

### SB-A-10 — imaging destination overlap gate silently disabled when volume enumeration fails (P2, likely)
- **File:** `ui/src-tauri/src/commands/imaging.rs:225`.
- **Category:** swallowed error disabling a safety gate.
```rust
let volumes = crate::bridge::util::enumerate_volumes().unwrap_or_default();
if volumes.iter().any(|v| v.letter == letter && v.disk == disk) {
    return DestGate::SourceOverlap;
}
```
- **Why a defect:** the same-disk overlap refusal (imaging disk N onto a volume of disk N — reads and writes interleaving on the same disk during the byte-for-byte copy) is skipped entirely when `enumerate_volumes` errors — the failure is swallowed and the gate silently opens. The adjacent free-space probe degrades honestly ("the engine's own write failure stays the truthful backstop"), but the overlap case's backstop is a *corrupt image*, not just a failed write. Note `system_validate_destination`'s overlap check (`system.rs:190`) swallows the same way, but there the engine's enqueue-time `0x50050001` refusal backstops it; the imaging one-shot has no such backstop.
- **Impact:** rare (enumeration failure), but when hit the user can image a disk onto itself and get a silently corrupt image.
- **Fix direction:** on enumeration failure, either type the error or keep the gate closed with the honest message.

### SB-A-11 — connector event disposers drop `listen()` rejections on the floor (P3, certain)
- **File:** `ui/src/connector/tauri/tauri-connector.ts:296-299` (pattern repeated at every `on*` method: `onScanProgress`, `onScanFinished`, `onSaveProgress`, `onLostCandidate`, `onScanAutosaved`, `onConsentRequest`, `onSessionLoadProgress`, `onRecoveryProgress`, `onRecoveryFinished`, `onApplyConsent`, `onImageProgress`, `onImageFinished`).
```ts
const pending = listen<ScanProgressEvent>(`scan-progress:${scanId}`, (e) => cb(e.payload));
return () => {
  void pending.then((unlisten) => unlisten());   // no .catch — rejection is unhandled
};
```
- **Why a defect:** if `listen()` rejects (webview IPC failure), the returned promise rejects unhandled (console noise), the consumer's callback never receives events, and nothing surfaces it. Low impact (listen failures are rare), but it is the unhandled-rejection shape the brief targets, and the subscription silently no-ops.
- **Fix direction:** `void pending.then(unlisten => unlisten()).catch(...)` plus an optional error surface.

### SB-A-12 — app-log writer thread spawn failure turns the whole product log into a silent no-op (P3, certain shape, rare trigger)
- **File:** `ui/src-tauri/src/bridge/logging.rs:236-252`.
```rust
std::thread::Builder::new().name("rw-applog".into()).spawn(...).ok();   // .ok() swallows the error
```
- **Why a defect:** if the spawn fails, the channel's receiver dies inside the dropped closure, `tx.send` fails into the void (`let _ = tx.send(line)`), and every subsequent `info`/`error`/engine diagnostic line is silently discarded — the operational log (the product's only evidence trail, per NG-15) silently stops existing. Rare trigger, but the entire purpose of this module is post-mortem evidence.
- **Fix direction:** fall back to a direct-file write on spawn failure, or at least log to stderr once.

### SB-A-13 — sessions-list rows fill `dataFoundBytes` with `bytes_scanned` (P3 latent, certain mismatch)
- **File:** `ui/src-tauri/src/commands/sessions.rs:217`.
```rust
"dataFoundBytes": (if valid { json!(r.bytes_scanned) } else { json!(0) }),
```
- **Why a defect:** the field name promises the found-data size; the light record carries no found-bytes axis, so the bridge substitutes scanned-bytes (often far larger than found size). Currently no shipped renderer consumes `dataFoundBytes` from session rows (verified by grep — the backup page renders `filesFound` only), so it is latent; any future consumer gets a wrong number. `session_save`'s response uses the correct `t.data_found_bytes`.
- **Fix direction:** null the field (the honest absent axis) instead of substituting.

### SB-A-14 — session list ids collide at seconds granularity (P3 latent, certain)
- **File:** `ui/src-tauri/src/commands/sessions.rs:197` — `"id": format!("sess-{}", r.save_unix.max(1))`. Two sessions saved in the same unix second get identical ids; used as list-row identity by consumers, React keys collide/mis-associate. Latent (same-second saves are uncommon; the auto-name path is unique per second but two explicit saves or auto+manual can land in one second).

### SB-A-15 — consent re-hydration re-fires `setRepair` with fresh object identity every 500 ms poll (P3, needs-runtime-proof)
- **File:** `ui/src/queries/scan.ts:194-198`.
```ts
reactUseEffect(() => {
  if (state?.pendingConsent) setRepair(state.pendingConsent);
}, [state?.pendingConsent, setRepair]);
```
- The bridge's `pending_consent` clones the payload per `scan_state` call, so while a consent pends, the 500 ms state poll delivers a **new object identity** each time → the consent store updates → the global modal re-renders every 500 ms for the whole pend duration. Correctness unaffected; render churn + potential input-flicker inside the modal. Needs runtime proof of perceptible cost.

## Needs-runtime-proof list

1. **SB-A-03** — recover-all mid-scan offset-paging drift (skip/duplicate files) + stale `snap_ptr` enqueue across snapshot generations. Static window exists; the wrong-file-by-index sub-case is mitigated by arena-order stability (`cr_snapshot.cpp`), so the skip/duplicate half is the one to prove with a live scan + recover-all + instrumented enqueue set.
2. **`getScanStore` side-effectful `getSnapshot`** (`ui/src/queries/scan.ts:137` + `22-84`): `useSyncExternalStore`'s render-phase `getSnapshot` call can create the store (registering the engine `listen` + priming async state) for a render that never commits (React concurrent interruption) → an orphaned store with 0 listeners whose engine listener lives for the app run. Same shape in `queries/recovery.ts:89-90` and `queries/imaging.ts:67-69`. Prove with a StrictMode/interruption harness; then move creation into `subscribe`.
3. **SB-A-15** — consent-modal re-render churn during a pend.
4. **SB-A-02's rate in practice** — Windows handle/memory growth across N scans in one app run (the registry-retention is static-certain; only the exhaustion timing needs measurement).

## Clean areas verified (brief)

- **Command registration:** all 60 `#[tauri::command]` fns are registered in `lib.rs`'s `invoke_handler` — no dead handlers bridge-side. (The defect is the inverse: `scan_close` is registered but unreachable from the UI — SB-A-02.)
- **Async discipline:** every blocking engine call (`scan_*`, `recovery_*`, `session_*`, `health_*`, `fsck`, `bitlocker`, `disk_map`, pickers) runs inside `spawn_blocking`; no `std::fs`/sleep/lock-across-await in async command bodies; consent's §4.5 rule (no registry lock held while the gate waits) is honored in both families.
- **Scan-cancel plumbing:** engine checkpoints at every stage boundary + engine-pass + lost-discovery + carve (`cr_scan.cpp:1527/1563/1583/2116`); `CancelPoolWatcher` aborts queued candidates; `run()` translates `ERR_CANCELLED` to status; the bridge's `scan_cancel` force-denies pending consent gates first (deadlock-free design verified).
- **Progress math:** `weighted_progress`/`phase_progress` use u64 intermediates, clamp, single-division; `tel_publish_` permilles are clamped-monotonic; the bridge's emitter coalesces and clamps monotonic `overall`/`frac`.
- **Event marshaling:** sinks are `try_send`-only (never block, never lock the registry); bounded channels (512 scan / 64 recovery+imaging); `lost-candidate` log-line-before-event ordering matches the bridge's stash join; `scan-finished`/`recovery-finished` single-writer terminal emits verified.
- **`cr_ui_snapshot_node` TRUNCATED contract:** engine fills `name_len` before the truncation return (`cr_ui.cpp:3817-3823`) — the bridge's exact-size retry (`util.rs:226-233`) is correct.
- **Engine hot-loop bounds:** runlist decode bounded (`RUNLIST_MAX_RUNS`, `max_clusters` accumulate-check with no reachable overflow); `SafeWriter::resolve_collision` bounded by `rename_attempts` with honest exhaustion; `ThreadPool::abort` drops the queue; snapshot capture is lock-serialized.
- **TS subscription cleanup:** every `useEffect` event subscription in queries/features returns its disposer (scan finished/autosaved/lost-candidate/consent bridges, recovery finished, imaging finished, exit-guard's close-requested with dispose-race guard); `useScanState`'s poll correctly keeps `'stopping'` alive.
- **Hex worker:** fresh worker per request, terminated on both success and error, transferable copy — no backpressure or lifecycle leak; fallback shares the exact formatter.
- **Stores:** zustand stores small and clean; `ui-store` localStorage guarded; `scan-session-store`'s `known` map grows one small entry per scan (bounded by scans/run — subsumed by SB-A-02's larger leak).
- **Error surfacing where it matters most:** start-scan, import-image, unlock, image start/stop, save-session, exit-guard save, user-signs import all render failures via `onError` toasts; the connector's `toConnectorError` mapping is exhaustive over the error-code tables.
