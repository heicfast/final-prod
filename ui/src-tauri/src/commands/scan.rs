//! commands/scan.rs — scan_start / pause / resume / cancel / state / close.
//!
//! The documented binding lifecycle (docs/CONNECTION-GUIDE.md; plan §3.2 #4):
//! abi gate -> resolve target -> open -> scan_create (zero-init config:
//! stage_mask 7, weights 40/35/25, threads auto) -> attach -> subscribe
//! BEFORE run -> dedicated run thread (run() is synchronous on its thread).
//!
//! G-11 pathScope: the engine has no folder-scoped scan — the bridge runs
//! the whole-volume/whole-disk scan and applies path-prefix scoping to every
//! result view. Scan duration reflects the WHOLE volume; only results are
//! scoped (documented semantics delta, INTEGRATION-VALIDATION).
//!
//! Pre-scan repair + damage audit run READ-ONLY with the engine's own
//! default-deny (repair_consent = NULL: "the plan is surfaced via events and
//! stats and NEVER executed" — cr_ui.h S-008). No consent surface exists in
//! the UI to approve an apply; the analysis evidence rides stats/session
//! (INTEGRATION-GAPS G-1).

use std::sync::atomic::Ordering;
use std::sync::mpsc::sync_channel;
use std::sync::{Arc, Mutex};

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, check, BridgeError, BridgeResult};
use crate::bridge::events::{
    emit_scan_progress, scan_flags, scan_status, spawn_scan_emitter,
};
use crate::bridge::state::{BridgeState, ScanEntry, ScanKind, ScanLive, Terminal};
use crate::bridge::util::{now_iso, now_unix_ms, SendPtr};
use crate::commands::devices::{disk_id, resolve_target, volume_id};

/// Normalize an absolute Windows path into the '/'-joined lowercase prefix
/// the meta cache compares against (no drive letter, no trailing slash).
/// `None` = whole volume. Errors are the bridge's scan-scope-invalid.
pub fn normalize_scope(raw: &str, volume_letter: Option<u8>) -> BridgeResult<Option<String>> {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return Ok(None);
    }
    let bytes = trimmed.as_bytes();
    if bytes.len() < 3 || !bytes[0].is_ascii_alphabetic() || bytes[1] != b':' || (bytes[2] != b'\\' && bytes[2] != b'/') {
        return Err(BridgeError::bridge(
            code::SCAN_SCOPE_INVALID,
            format!("folder scope '{trimmed}' is not an absolute path (expected X:\\...)"),
        ));
    }
    let letter = bytes[0].to_ascii_uppercase();
    if let Some(want) = volume_letter {
        if letter != want {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                format!("folder scope '{trimmed}' is not on the scanned volume ({}:)", want as char),
            ));
        }
    }
    let rest = trimmed[3..].replace('\\', "/");
    let rest = rest.trim_end_matches('/');
    if rest.is_empty() {
        return Ok(None); // the volume root itself — whole-volume scan
    }
    Ok(Some(rest.to_lowercase()))
}

#[tauri::command]
pub async fn scan_start(
    app: tauri::AppHandle,
    state: tauri::State<'_, Arc<BridgeState>>,
    target_id: String,
    path_scope: Option<String>,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    let state_for_launch = state.clone();
    let (entry, scan_id, rx) = tauri::async_runtime::spawn_blocking(move || {
        scan_start_blocking(&state, target_id, path_scope)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))??;
    launch_run_thread(app, state_for_launch, entry, rx);
    Ok(json!({ "scanId": scan_id }))
}

/// The blocking preparation. Returns the registered entry + the event
/// receiver for the emitter; the command wrapper launches the threads.
pub fn scan_start_blocking(
    state: &Arc<BridgeState>,
    target_id: String,
    path_scope: Option<String>,
) -> BridgeResult<(Arc<ScanEntry>, String, std::sync::mpsc::Receiver<abi::CrUiEvent>)> {
    // ABI gate: refuse to bind on mismatch (plan §4.8).
    let abi_v = unsafe { abi::cr_ui_abi_version() };
    if abi_v != abi::CR_UI_ABI_VERSION {
        return Err(BridgeError::bridge(
            code::UNKNOWN,
            format!("cr_ui ABI version {abi_v} != expected {CR_ABI}; the shipped engine kit is incompatible", CR_ABI = abi::CR_UI_ABI_VERSION),
        ));
    }

    let target = resolve_target(&target_id)?;
    let volume_letter = target.volume.map(|v| v.letter).filter(|l| *l != 0);

    // G-11 scope: UI-supplied (absolute path on the target volume) or the
    // virtual quick-target's implicit scope. Whole-disk targets accept a
    // scope only when its letter belongs to a volume on that disk.
    let scope_raw = path_scope.or_else(|| target.implicit_scope.clone());
    let scope = match &scope_raw {
        Some(s) => {
            if target.volume.is_none() {
                // disk target: the scope's letter must sit on this disk
                let letter = s.as_bytes().first().copied().unwrap_or(b'?');
                let on_disk = crate::bridge::util::enumerate_volumes()
                    .map(|vs| {
                        vs.iter()
                            .any(|v| v.disk == target.disk.number && v.letter == letter.to_ascii_uppercase())
                    })
                    .unwrap_or(false);
                if !on_disk {
                    return Err(BridgeError::bridge(
                        code::SCAN_SCOPE_INVALID,
                        format!("folder scope '{s}' is not on disk {}", target.disk.number),
                    ));
                }
                normalize_scope(s, Some(letter.to_ascii_uppercase()))?
            } else {
                normalize_scope(s, volume_letter)?
            }
        }
        None => None,
    };

    // Open the device view. A locked volume is refused (0x50060001) — the UI
    // shows the truthful locked status; unlock surfaces are a documented gap.
    let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
    let open_rc = if let Some(v) = &target.volume {
        if v.letter != 0 {
            unsafe { abi::cr_ui_open_volume(v.letter as i8, &mut dev) }
        } else {
            unsafe { abi::cr_ui_open_disk(v.disk, &mut dev) }
        }
    } else {
        unsafe { abi::cr_ui_open_disk(target.disk.number, &mut dev) }
    };
    if open_rc == abi::CR_UI_ERR_FVE_LOCKED {
        return Err(BridgeError::bridge(
            code::DEVICE_PERMISSION,
            "the volume is BitLocker-locked: unlock surfaces are not available in this build (docs/INTEGRATION-GAPS.md)",
        ));
    }
    if open_rc != abi::CR_UI_OK {
        return Err(BridgeError::engine(open_rc, "cr_ui_open_disk/volume"));
    }
    let device = SendPtr::new(dev);

    // Bytes total: the opened view's geometry (slice bytes for volumes).
    let mut geom: abi::CrUiGeometry = unsafe { std::mem::zeroed() };
    check(unsafe { abi::cr_ui_device_geometry(dev, &mut geom) }, "cr_ui_device_geometry")?;

    // Zero-init config = documented defaults. Pre-scan repair + damage audit
    // run READ-ONLY (default-deny: NULL consent — cr_ui.h S-008 / M-041).
    let mut cfg: abi::CrUiScanConfig = unsafe { std::mem::zeroed() };
    cfg.whole_disk = if target.volume.is_some() { 0 } else { 1 };
    cfg.pre_scan_repair = 1;
    cfg.pre_scan_check = 1;
    // repair_consent stays NULL: analysis outcomes ride events + stats; the
    // engine NEVER executes without a consent callback (default-deny).

    let mut scan: *mut abi::CrUiScan = std::ptr::null_mut();
    check(unsafe { abi::cr_ui_scan_create(&cfg, &mut scan) }, "cr_ui_scan_create")?;
    let scan_ptr = SendPtr::new(scan);
    check(unsafe { abi::cr_ui_scan_attach_device(scan, dev) }, "cr_ui_scan_attach_device")?;

    // Channel + sink box BEFORE run (the sink is read at scan start).
    // Bounded (512): the sink try_sends — full channel means the UI is
    // slow and the emitter coalesces; the engine worker thread never blocks.
    let (tx, rx) = sync_channel::<abi::CrUiEvent>(512);
    let sink_user = Box::into_raw(Box::new(tx.clone()));
    check(
        unsafe {
            abi::cr_ui_scan_set_event_fn(
                scan,
                Some(crate::bridge::events::scan_event_sink),
                sink_user as *mut std::ffi::c_void,
            )
        },
        "cr_ui_scan_set_event_fn",
    )?;

    let scan_id = state.mint_scan_id();
    let (target_json, device_id, volume_id_str) = if target.volume.is_some() {
        (
            target.volume_json.clone().unwrap_or(Value::Null),
            disk_id(target.disk.number),
            target.volume.as_ref().map(volume_id),
        )
    } else {
        (target.device_json.clone(), disk_id(target.disk.number), None)
    };
    let params_json = json!({
        "targetId": target_id,
        "pathScope": scope_raw,
    });
    let live = Arc::new(Mutex::new(ScanLive::new(geom.total_bytes)));
    let terminal: Arc<Mutex<Option<Terminal>>> = Arc::new(Mutex::new(None));
    let snapshot_exists = Arc::new(std::sync::atomic::AtomicBool::new(false));

    let entry = Arc::new(ScanEntry {
        scan_id: scan_id.clone(),
        kind: ScanKind::Live {
            scan: scan_ptr,
            device,
        },
        target_json,
        device_id,
        volume_id: volume_id_str,
        params_json,
        path_scope: scope,
        started_at: now_iso(),
        live: live.clone(),
        terminal: terminal.clone(),
        snapshot_exists: snapshot_exists.clone(),
        sink_user: SendPtr::new(sink_user),
        snap: Mutex::new(crate::bridge::state::SnapshotCache {
            gen: 0,
            handle: SendPtr::new(std::ptr::null_mut()),
            partial: false,
            node_count: 0,
            metas: None,
            last_refresh_ms: 0,
            frozen: false,
        }),
        jobs: Mutex::new(Vec::new()),
    });

    state
        .scans
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "registry lock"))?
        .insert(scan_id.clone(), entry.clone());

    Ok((entry, scan_id, rx))
}

/// Spawn the emitter + run threads (called by the command wrapper once the
/// blocking prep succeeded). The run thread latches the terminal state and
/// emits the final events; it NEVER holds the registry lock.
pub fn launch_run_thread(
    app: tauri::AppHandle,
    state: Arc<BridgeState>,
    entry: Arc<ScanEntry>,
    rx: std::sync::mpsc::Receiver<abi::CrUiEvent>,
) {
    let scan_id = entry.scan_id.clone();
    let live = entry.live.clone();
    let snapshot_exists = entry.snapshot_exists.clone();
    let scan_ptr = crate::bridge::util::SendPtr::new(entry.scan_handle());
    spawn_scan_emitter(rx, app.clone(), scan_id.clone(), live.clone(), snapshot_exists.clone(), scan_ptr);

    let state_for_run = state.clone();
    let entry_for_run = entry.clone();
    let app_for_run = app.clone();
    std::thread::spawn(move || {
        let rc = unsafe { abi::cr_ui_scan_run(entry_for_run.scan_handle()) };

        // Exact terminal counters from stats (never fabricated).
        let mut stats: abi::CrUiScanStats = unsafe { std::mem::zeroed() };
        let _ = unsafe { abi::cr_ui_scan_get_stats(entry_for_run.scan_handle(), &mut stats) };
        let files_found = stats.files_found + stats.deleted_found + stats.carved_found;

        let (status, failure) = if rc == abi::CR_UI_OK {
            ("completed", None)
        } else if rc == abi::CR_UI_ERR_CANCELLED {
            // Cancel is a STATUS, never an error (error-codes.json).
            ("cancelled", None)
        } else {
            let ui_code = match rc {
                0x5008_0001 | 0x5008_0003 | 0x5003_0005 => "repair-failed",
                0x5006_0001 => "device-permission",
                _ => "unknown",
            };
            ("failed", Some(ui_code))
        };

        // Freeze one final snapshot (browsable tree — valid on cancel too),
        // then compute the exact found-bytes sum (G-3: latched once).
        let _ = crate::bridge::state::refresh_snapshot(&state_for_run, &entry_for_run.scan_id);
        let mut data_found = 0u64;
        if crate::bridge::state::ensure_metas(&entry_for_run).is_ok() {
            if let Ok(snap) = entry_for_run.snap.lock() {
                if let Some(metas) = &snap.metas {
                    data_found = metas
                        .iter()
                        .filter(|m| !m.is_dir)
                        .map(|m| m.size)
                        .sum();
                }
            }
        }

        let completed_at = now_iso();
        let term = Terminal {
            status,
            engine_code: rc,
            completed_at,
            files_found,
            data_found_bytes: data_found,
        };
        if let Ok(mut t) = entry_for_run.terminal.lock() {
            *t = Some(term);
        }
        if let Ok(mut l) = entry_for_run.live.lock() {
            l.data_found_bytes = data_found;
            l.files_found = files_found;
        }

        // Terminal events: final progress sample + the latch notification.
        emit_scan_progress(
            &app_for_run,
            &entry_for_run.scan_id,
            &entry_for_run.live,
            &entry_for_run.terminal,
            &entry_for_run.snapshot_exists,
            entry_for_run.scan_handle(),
        );
        use tauri::Emitter;
        let _ = app_for_run.emit(
            &format!("scan-finished:{}", entry_for_run.scan_id),
            json!({ "status": status, "failure": failure }),
        );
    });
}

// ---------------------------------------------------------------------------
// Token commands (any-thread safe engine calls)
// ---------------------------------------------------------------------------

fn set_transient(state: &Arc<BridgeState>, scan_id: &str, name: &'static str) -> BridgeResult<()> {
    let entry = state.scan_entry_arc(scan_id)?;
    if entry.is_terminal() {
        return Err(BridgeError::bridge(
            code::SCAN_NOT_RUNNING,
            format!("scan {scan_id} has already finished"),
        ));
    }
    let (busy, _stopping, _paused) = scan_flags(entry.scan_handle());
    if !busy {
        return Err(BridgeError::bridge(
            code::SCAN_NOT_RUNNING,
            format!("scan {scan_id} is not running"),
        ));
    }
    if let Ok(mut l) = entry.live.lock() {
        l.transient = Some((name, (now_unix_ms() as u64) + 2500));
    }
    Ok(())
}

#[tauri::command]
pub async fn scan_pause(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<(), BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        set_transient(&state, &scan_id, "pausing")?;
        let entry = state.scan_entry_arc(&scan_id)?;
        check(unsafe { abi::cr_ui_scan_pause(entry.scan_handle()) }, "cr_ui_scan_pause")
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn scan_resume(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<(), BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        if entry.is_terminal() {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                format!("scan {scan_id} has already finished"),
            ));
        }
        let (busy, _s, paused) = scan_flags(entry.scan_handle());
        if !busy {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                "cannot resume a scan that is not running",
            ));
        }
        if !paused {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_PAUSED,
                "scan is not paused",
            ));
        }
        set_transient(&state, &scan_id, "resuming")?;
        check(unsafe { abi::cr_ui_scan_resume(entry.scan_handle()) }, "cr_ui_scan_resume")
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn scan_cancel(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<(), BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        if entry.is_terminal() {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                format!("scan {scan_id} has already finished"),
            ));
        }
        check(unsafe { abi::cr_ui_scan_cancel(entry.scan_handle()) }, "cr_ui_scan_cancel")
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// State query + close
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn scan_state(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let (busy, stopping, paused) = scan_flags(entry.scan_handle());
        let term = entry.terminal.lock().ok().and_then(|t| t.clone());
        let live = entry
            .live
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "live lock"))?;
        let now = now_unix_ms();
        let status = scan_status(&live, term.as_ref(), busy, stopping, paused, now);
        let failure = term.as_ref().and_then(|t| {
            if t.status == "failed" {
                let ui_code = match t.engine_code {
                    0x5008_0001 | 0x5008_0003 | 0x5003_0005 => "repair-failed",
                    0x5006_0001 => "device-permission",
                    _ => "unknown",
                };
                Some(json!({ "code": ui_code }))
            } else {
                None
            }
        });
        let progress = crate::bridge::events::compose_scan_progress(
            &scan_id,
            &live,
            term.as_ref(),
            entry.snapshot_exists.load(Ordering::Relaxed),
            busy,
            stopping,
            paused,
        );
        Ok(json!({
            "scanId": entry.scan_id,
            "params": entry.params_json,
            "target": entry.target_json,
            "status": status,
            "startedAt": entry.started_at,
            "completedAt": term.as_ref().map(|t| t.completed_at.clone()),
            "progress": progress,
            "failure": failure,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn scan_close(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<(), BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        // Refuse while the run is in flight (the sink box + borrowed device
        // must outlive the engine's threads — cr_ui.h lifetime rules).
        let (busy, _s, _p) = scan_flags(entry.scan_handle());
        if busy {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                "cannot close a scan while it is running (cancel it first)",
            ));
        }
        let active_jobs = entry
            .jobs
            .lock()
            .map(|j| j.len())
            .unwrap_or(0);
        if active_jobs > 0 {
            return Err(BridgeError::bridge(
                code::UNKNOWN,
                "cannot close a scan with recovery jobs still referencing it",
            ));
        }
        state
            .scans
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "registry lock"))?
            .remove(&scan_id);
        Ok(())
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
