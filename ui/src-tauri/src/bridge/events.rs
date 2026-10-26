//! bridge/events.rs — engine-event marshaling (worker threads → Tauri emit).
//!
//! The non-negotiable marshaling rules (contracts/event-mapping.md):
//! 1. the sink runs on an ENGINE WORKER THREAD — it must not block, must not
//!    lock the registry, must not call back into the engine;
//! 2. the sink does exactly one thing: `try_send` a serialized copy of the
//!    event POD into the channel owned by the scan/job entry;
//! 3. a single emitter thread per handle drains the channel, coalesces
//!    superseded samples, and calls `app_handle.emit`;
//! 4. unknown event types are ignored (forward compatibility);
//! 5. HEARTBEAT never synthesizes progress.
//!
//! Terminal events are NOT emitter-produced: the run/process threads latch
//! the terminal state first, then emit the final progress sample +
//! `scan-finished:` / `recovery-finished:` themselves.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc::{Receiver, TryRecvError};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use tauri::{AppHandle, Emitter};

use super::abi;
use super::state::{JobLive, ScanLive, Terminal, BAD_SECTOR_MAP_CAP, DISCONNECT_AFTER_MS};

// ---------------------------------------------------------------------------
// Sinks (engine worker threads — never add anything blocking in here)
// ---------------------------------------------------------------------------

/// The scan event sink. `user` is a `*mut Sender<CrUiEvent>` boxed by the
/// scan-start path and freed by ScanEntry::Drop after unsubscribing.
pub unsafe extern "C" fn scan_event_sink(user: *mut std::ffi::c_void, evt: *const abi::CrUiEvent) {
    if user.is_null() || evt.is_null() {
        return;
    }
    let tx = unsafe { &*(user as *mut Sender<abi::CrUiEvent>) };
    let copy = unsafe { *evt };
    let _ = tx.try_send(copy); // channel-full = UI slow; the emitter coalesces
}

/// The recovery bytes-progress sink: only the freshest sample matters.
pub unsafe extern "C" fn recovery_progress_sink(
    user: *mut std::ffi::c_void,
    bytes_done: u64,
    bytes_total: u64,
) {
    if user.is_null() {
        return;
    }
    let tx = unsafe { &*(user as *mut Sender<(u64, u64)>) };
    let _ = tx.try_send((bytes_done, bytes_total));
}

/// Drain everything queued, keeping only the freshest sample.
pub fn coalesce_bytes(rx: &Receiver<(u64, u64)>) -> Option<(u64, u64)> {
    let mut last = None;
    loop {
        match rx.try_recv() {
            Ok(sample) => last = Some(sample),
            Err(TryRecvError::Disconnected) | Err(TryRecvError::Empty) => break,
        }
    }
    last
}

// ---------------------------------------------------------------------------
// Progress payload composition
// ---------------------------------------------------------------------------

/// Resolve the live status word for a scan (contracts/type-maps.md §3).
pub fn scan_status(
    live: &ScanLive,
    terminal: Option<&Terminal>,
    busy: bool,
    stopping: bool,
    paused: bool,
    now_ms: i64,
) -> &'static str {
    if let Some(t) = terminal {
        return t.status;
    }
    if let Some((name, expiry)) = live.transient {
        if now_ms < expiry as i64 {
            return name;
        }
    }
    if stopping {
        return "stopping";
    }
    if paused {
        return "paused";
    }
    if busy {
        return "running";
    }
    // Pre-run: the bridge-synthesized starting state (no telemetry evidence
    // of running yet); falls out naturally once run() engages.
    "starting"
}

/// deviceConnected synthesis (G-8): heartbeat-fresh within 2× the engine's
/// 5 s watchdog window, OR a progress sample inside the same window, OR
/// terminal / pre-start. Never fabricated beyond liveness evidence.
fn device_connected(live: &ScanLive, terminal: Option<&Terminal>, now_ms: i64) -> bool {
    if terminal.is_some() {
        return true;
    }
    let hb = live.last_heartbeat_ms.unwrap_or(0) as i64;
    let last_event_ms = hb.max(live.last_progress_ms as i64);
    if last_event_ms == 0 {
        return true; // starting: no evidence of loss yet
    }
    now_ms - last_event_ms < DISCONNECT_AFTER_MS as i64
}

/// Compose the contract-shaped `ScanProgressEvent` payload (camelCase).
pub fn compose_scan_progress(
    scan_id: &str,
    live: &ScanLive,
    terminal: Option<&Terminal>,
    snapshot_exists: bool,
    busy: bool,
    stopping: bool,
    paused: bool,
) -> serde_json::Value {
    let now = now_unix_ms();
    let status = scan_status(live, terminal, busy, stopping, paused, now);
    let (overall, files_found, data_found) = match terminal {
        Some(t) => (1.0_f64, t.files_found, t.data_found_bytes),
        None => (live.overall, live.files_found, live.data_found_bytes),
    };
    let bad_sectors: Vec<serde_json::Value> = live
        .bad_sectors
        .iter()
        .map(|(start, len)| serde_json::json!({ "startByte": start, "sizeBytes": len }))
        .collect();
    serde_json::json!({
        "scanId": scan_id,
        "status": status,
        "stage": (if terminal.is_some() { "done" } else { live.stage }),
        "stageProgress": (if terminal.is_some() { 1.0 } else { live.stage_progress }),
        "overallProgress": overall,
        "bytesScanned": live.bytes_scanned,
        "bytesTotal": live.bytes_total,
        "filesFound": files_found,
        "dataFoundBytes": data_found,
        "errors": live.errors,
        "badSectorMap": bad_sectors,
        "etaSeconds": (if terminal.is_some() { serde_json::json!(0) } else { serde_json::json!(live.last_eta_seconds) }),
        "throughputBytesPerSecond": live.last_throughput,
        "recoverableDuringScan": snapshot_exists,
        "deviceConnected": device_connected(live, terminal, now),
        // G-2: no ABI event carries the item being scanned — the UI hides
        // the ticker; the bridge never fabricates a path.
        "currentPath": serde_json::Value::Null,
    })
}

/// Fresh busy/stopping/paused probe (all any-thread safe, cr_ui.h).
pub fn scan_flags(scan_ptr: *mut abi::CrUiScan) -> (bool, bool, bool) {
    if scan_ptr.is_null() {
        return (false, false, false);
    }
    let mut b: u32 = 0;
    let mut s: u32 = 0;
    let mut tel: abi::CrUiTelemetry = unsafe { std::mem::zeroed() };
    unsafe {
        let _ = abi::cr_ui_scan_busy(scan_ptr, &mut b);
        let _ = abi::cr_ui_scan_stopping(scan_ptr, &mut s);
        let _ = abi::cr_ui_scan_telemetry(scan_ptr, &mut tel);
    }
    (b != 0, s != 0, tel.paused != 0)
}

/// Emit one scan-progress payload (with the telemetry poll inside).
pub fn emit_scan_progress(
    app: &AppHandle<impl tauri::Runtime>,
    scan_id: &str,
    live: &Mutex<ScanLive>,
    terminal: &Mutex<Option<Terminal>>,
    snapshot_exists: &AtomicBool,
    scan_ptr: *mut abi::CrUiScan,
) {
    let (busy, stopping, paused) = scan_flags(scan_ptr);
    let term = terminal.lock().ok().and_then(|t| t.clone());
    if let Ok(l) = live.lock() {
        let payload = compose_scan_progress(
            scan_id,
            &l,
            term.as_ref(),
            snapshot_exists.load(Ordering::Relaxed),
            busy,
            stopping,
            paused,
        );
        let _ = app.emit(&format!("scan-progress:{scan_id}"), payload);
    }
}

// ---------------------------------------------------------------------------
// Scan emitter thread
// ---------------------------------------------------------------------------

pub fn spawn_scan_emitter(
    rx: Receiver<abi::CrUiEvent>,
    app: AppHandle<impl tauri::Runtime>,
    scan_id: String,
    live: Arc<Mutex<ScanLive>>,
    snapshot_exists: Arc<AtomicBool>,
    scan_ptr: super::util::SendPtr<abi::CrUiScan>,
) {
    std::thread::spawn(move || {
        let scan_ptr = scan_ptr.get();
        for evt in rx {
            let Ok(live) = live.lock() else { break };
            let mut emit = false;
            match evt.r#type {
                abi::CR_EVT_SCAN_STAGE => {
                    live.stage = match evt.stage {
                        0 => "quick-index",
                        1 => "deep-walk",
                        2 => "carve",
                        _ => live.stage, // 0xFFFFFFFF none — standing stage
                    };
                    live.stage_progress = 0.0;
                    emit = true;
                }
                abi::CR_EVT_SCAN_PROGRESS => {
                    live.stage_progress = (evt.stage_permille as f64) / 1000.0;
                    let overall = (evt.overall_permille as f64) / 1000.0;
                    if overall > live.overall {
                        live.overall = overall; // clamped monotonic
                    }
                    emit = true;
                }
                abi::CR_EVT_FILES_FOUND => {
                    live.files_found = evt.files_found + evt.deleted_found + evt.carved_found;
                    emit = true;
                }
                abi::CR_EVT_ERROR_RANGE => {
                    if live.bad_sectors.len() < BAD_SECTOR_MAP_CAP {
                        live.bad_sectors.push((evt.err_start_bytes, evt.err_len_bytes));
                    }
                    live.errors = evt.error_ranges;
                    emit = true;
                }
                abi::CR_EVT_CHECKPOINT => { /* internal truth; no UI emission */ }
                abi::CR_EVT_LOST_CANDIDATE => {
                    // Badge surface is future page wiring (plan §3.2 #9): the
                    // bridge keeps the truthful count, fabricates nothing.
                    live.lost_candidates += 1;
                }
                abi::CR_EVT_REPAIR => {
                    // Default-deny posture (INTEGRATION-GAPS G-1): analysis
                    // outcomes ride the log + stats; no consent surface exists.
                    live.repair_summary = Some(format!(
                        "pre-scan repair: {} action(s), outcome {} (0 plan-ready, 1 applied, 2 denied, 3 failed)",
                        evt.repair_actions, evt.repair_outcome
                    ));
                }
                abi::CR_EVT_HEARTBEAT => {
                    // Liveness ONLY — never a synthetic permille advance.
                    live.last_heartbeat_ms = Some(now_unix_ms() as u64);
                }
                abi::CR_EVT_SAVE_PROGRESS => {
                    live.save_permille = evt.overall_permille.min(1000);
                    let _ = app.emit(
                        &format!("save-progress:{scan_id}"),
                        serde_json::json!({ "fraction": live.save_permille as f64 / 1000.0 }),
                    );
                }
                abi::CR_EVT_FS_CHECK => {
                    // Damage-audit count kept truthful; no scan-screen note
                    // exists in the contract (deferred with the badge above).
                    live.fs_check_errors = evt.check_errors as u32;
                }
                _ => { /* unknown type: ignored (cr_ui.h forward compat) */ }
            }
            if emit {
                live.last_progress_ms = now_unix_ms() as u64;
                // Coalesced telemetry snapshot at emit time (event-mapping §2):
                // bytes_scanned is carve-true-bytes the event cannot carry.
                if !scan_ptr.is_null() {
                    let mut tel: abi::CrUiTelemetry = unsafe { std::mem::zeroed() };
                    if unsafe { abi::cr_ui_scan_telemetry(scan_ptr, &mut tel) } == abi::CR_UI_OK {
                        live.bytes_scanned = tel.bytes_scanned;
                        live.files_found = tel.files_found + tel.deleted_found + tel.carved_found;
                        let elapsed_s = (tel.elapsed_ms as f64) / 1000.0;
                        if elapsed_s > 0.5 {
                            let bps = tel.bytes_scanned as f64 / elapsed_s;
                            live.last_throughput = if live.last_throughput == 0.0 {
                                bps
                            } else {
                                live.last_throughput * 0.6 + bps * 0.4
                            };
                            if live.last_throughput > 0.0 && live.bytes_total > live.bytes_scanned {
                                live.last_eta_seconds = Some(
                                    ((live.bytes_total - live.bytes_scanned) as f64
                                        / live.last_throughput) as u64,
                                );
                            }
                        }
                        // Transient expiry: telemetry caught up with the token call.
                        if let Some((name, expiry)) = live.transient {
                            let caught_up = (name == "pausing" && tel.paused != 0)
                                || (name == "resuming" && tel.paused == 0 && tel.running != 0);
                            if caught_up || now_unix_ms() as u64 >= expiry {
                                live.transient = None;
                            }
                        }
                    }
                }
                let (busy, stopping, paused) = scan_flags(scan_ptr);
                let payload = compose_scan_progress(
                    &scan_id,
                    &live,
                    None, // terminal latching is the run thread's business
                    snapshot_exists.load(Ordering::Relaxed),
                    busy,
                    stopping,
                    paused,
                );
                let _ = app.emit(&format!("scan-progress:{scan_id}"), payload);
            }
        }
    });
}

// ---------------------------------------------------------------------------
// Job emitter thread
// ---------------------------------------------------------------------------

const JOB_REPORT_POLL_MS: i64 = 2000;

pub fn spawn_job_emitter(
    rx: Receiver<(u64, u64)>,
    app: AppHandle<impl tauri::Runtime>,
    job_id: String,
    live: Arc<Mutex<JobLive>>,
    recovery_ptr: super::util::SendPtr<abi::CrUiRecovery>,
    finished: Arc<AtomicBool>,
) {
    std::thread::spawn(move || {
        let recovery_ptr = recovery_ptr.get();
        let mut prev_bytes: u64 = 0;
        let mut prev_sample_ms: i64 = 0;
        let mut last_report_ms: i64 = 0;
        let mut last_tail: Option<(String, String)> = None; // (path, hash_status)
        loop {
            let sample = match rx.recv_timeout(Duration::from_millis(250)) {
                Ok(s) => {
                    let mut newest = Some(s);
                    while let Ok(s2) = rx.try_recv() {
                        newest = Some(s2);
                    }
                    newest
                }
                Err(_) => {
                    if finished.load(Ordering::Acquire) {
                        break; // terminal payload already emitted by the process thread
                    }
                    None // timeout wakeup: keep counters fresh while running
                }
            };
            let Ok(mut live) = live.lock() else { break };
            if finished.load(Ordering::Acquire) {
                continue;
            }
            if let Some((done, total)) = sample {
                let now = now_unix_ms();
                if prev_sample_ms > 0 {
                    let dt_s = (now - prev_sample_ms).max(1) as f64 / 1000.0;
                    let delta = done.saturating_sub(prev_bytes);
                    if delta > 0 {
                        let bps = delta as f64 / dt_s;
                        live.last_throughput = if live.last_throughput == 0.0 {
                            bps
                        } else {
                            live.last_throughput * 0.6 + bps * 0.4
                        };
                    }
                }
                prev_bytes = done;
                prev_sample_ms = now;
                live.bytes_done = done;
                live.bytes_total = total;
                let frac = if total == 0 { 0.0 } else { done as f64 / total as f64 };
                if frac > live.overall {
                    live.overall = frac; // clamped monotonic
                }
            }
            if !recovery_ptr.is_null() {
                // Live counters from stats (event-mapping.md §Recovery).
                let mut stats: abi::CrUiRecoveryStats = unsafe { std::mem::zeroed() };
                if unsafe { abi::cr_ui_recovery_get_stats(recovery_ptr, &mut stats) } == abi::CR_UI_OK {
                    live.done_count = stats.done + stats.salvaged;
                    live.failed_count = stats.errors + stats.hash_mismatch;
                    live.skipped_count = stats.refused + stats.resumed + stats.nothing_to_recover;
                    live.total_count = stats.queued;
                }
                // Report tail at a bounded cadence: currentFile + verify signal.
                let now = now_unix_ms();
                if now - last_report_ms >= JOB_REPORT_POLL_MS {
                    last_report_ms = now;
                    if let Some(tail) = report_tail(recovery_ptr) {
                        last_tail = Some(tail);
                    }
                }
                if let Some((path, hash_status)) = &last_tail {
                    live.current_file = if live.status == "running" {
                        Some(path.clone())
                    } else {
                        None
                    };
                    // Verify-phase signal: bytes stalled + a committed item
                    // whose digest exists — never fabricated beyond the report.
                    live.verifying = live.status == "running"
                        && live.bytes_total > 0
                        && live.bytes_done == live.bytes_total
                        && matches!(hash_status.as_str(), "verified" | "computed");
                }
                if live.status == "running" && live.last_throughput > 0.0 && live.bytes_total > live.bytes_done {
                    live.eta_seconds = Some(
                        ((live.bytes_total - live.bytes_done) as f64 / live.last_throughput) as u64,
                    );
                }
            }
            let payload = compose_job_progress(&job_id, &live);
            let _ = app.emit(&format!("recovery-progress:{job_id}"), payload);
        }
    });
}

/// The report's most recent committed item: (path, hash_status). The report
/// is valid at any time (cr_ui.h); polled at a bounded cadence only.
fn report_tail(recovery_ptr: *mut abi::CrUiRecovery) -> Option<(String, String)> {
    let mut json: *mut i8 = std::ptr::null_mut();
    let source = super::util::cstr("bridge");
    let rc = unsafe { abi::cr_ui_recovery_report_json(recovery_ptr, source.as_ptr(), &mut json) };
    if rc != abi::CR_UI_OK || json.is_null() {
        return None;
    }
    let owned = unsafe { super::util::OwnedCrString::from_raw(json) }?;
    let v = owned.to_json().ok()?;
    let items = v.get("items")?.as_array()?;
    for item in items.iter().rev() {
        let outcome = item.get("outcome").and_then(|o| o.as_str()).unwrap_or("");
        if outcome != "pending" {
            let path = item.get("path").and_then(|p| p.as_str()).unwrap_or("").to_string();
            let hash = item
                .get("hash_status")
                .and_then(|h| h.as_str())
                .unwrap_or("not_computed")
                .to_string();
            return Some((path, hash));
        }
    }
    None
}

/// Compose the contract-shaped `RecoveryProgressEvent` (camelCase).
pub fn compose_job_progress(job_id: &str, live: &JobLive) -> serde_json::Value {
    serde_json::json!({
        "jobId": job_id,
        "status": live.status,
        "overallProgress": live.overall,
        "currentFile": live.current_file,
        // Bytes-level truth (cr_ui_recovery_progress_fn): no per-file
        // fraction exists — the UI renders the indeterminate state.
        "currentFileProgress": 0,
        "completedCount": live.done_count,
        "failedCount": live.failed_count,
        "skippedCount": live.skipped_count,
        "totalCount": live.total_count,
        "throughputBytesPerSecond": live.last_throughput,
        "verifying": live.verifying,
        "etaSeconds": live.eta_seconds,
    })
}

/// Terminal progress emit from the process thread (mirrors the scan path).
pub fn emit_job_terminal(
    app: &AppHandle<impl tauri::Runtime>,
    job_id: &str,
    live: &Mutex<JobLive>,
    status: &'static str,
) {
    if let Ok(mut l) = live.lock() {
        l.status = status;
        l.verifying = false;
        l.current_file = None;
        if status == "completed" || status == "completed-partial" {
            l.overall = 1.0;
            l.eta_seconds = Some(0);
        }
        let payload = compose_job_progress(job_id, &l);
        let _ = app.emit(&format!("recovery-progress:{job_id}"), payload);
    }
    let _ = app.emit(
        &format!("recovery-finished:{job_id}"),
        serde_json::json!({ "status": status }),
    );
}

fn now_unix_ms() -> i64 {
    super::util::now_unix_ms()
}
