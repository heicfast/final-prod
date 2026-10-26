//! commands/recovery.rs — recovery_start / cancel / state / items / report
//! (contracts/command-surface.json; type-maps §6).
//!
//! Flow: create (config from RecoveryOptions) -> set_destination (the F12
//! overlap refusal arms on lettered destinations) -> enqueue (session trees:
//! node_id handles; live scans: the snapshot index variants, G-02b race-free)
//! -> spawn the process thread (process() is synchronous on its thread) ->
//! progress events on recovery-progress:<jobId>.
//!
//! Item truth comes from the engine's own report (schema_version 1; outcome
//! done/salvaged -> done, refused/resumed/nothing_to_recover -> skipped,
//! error/hash_mismatch -> failed, pending -> pending). Per-list cap 500 with
//! truncatedItemLists — the same truncation contract the simulator shipped,
//! now backed by the engine's report.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc::channel;
use std::sync::{Arc, Mutex};

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, check, BridgeError, BridgeResult};
use crate::bridge::events::{emit_job_terminal, spawn_job_emitter};
use crate::bridge::state::{BridgeState, JobEntry, JobLive, ScanKind, ScanEntry};
use crate::bridge::util::{now_iso, SendPtr};
use crate::commands::results::query_page;

fn conflict_of(v: &Value) -> u32 {
    match v.get("collision").and_then(|c| c.as_str()) {
        Some("skip") => 1,
        Some("overwrite") => 2,
        _ => 0, // rename (default)
    }
}

/// Map the engine's report outcome to the UI's RecoveryItem state.
fn outcome_state(outcome: &str) -> &'static str {
    match outcome {
        "done" | "salvaged" => "done",
        "refused" | "resumed" | "nothing_to_recover" => "skipped",
        "error" | "hash_mismatch" => "failed",
        _ => "pending", // pending — never reported as done (truth rule 2)
    }
}

#[tauri::command]
pub async fn recovery_start(
    app: tauri::AppHandle,
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    selection: Value,
    destination: String,
    options: Value,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    let (job_entry, job_id, rx) = tauri::async_runtime::spawn_blocking(move || {
        recovery_start_blocking(&state, scan_id, selection, destination, options)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))??;
    let recovery_ptr = job_entry.recovery.get();
    let live = job_entry.live.clone();
    let finished = Arc::new(AtomicBool::new(false));
    spawn_job_emitter(rx, app.clone(), job_id.clone(), live.clone(), recovery_ptr, finished.clone());
    spawn_process_thread(app, job_entry, finished);
    Ok(json!({ "jobId": job_id }))
}

fn recovery_start_blocking(
    state: &Arc<BridgeState>,
    scan_id: String,
    selection: Value,
    destination: String,
    options: Value,
) -> BridgeResult<(Arc<JobEntry>, String, std::sync::mpsc::Receiver<(u64, u64)>)> {
    let entry = state.scan_entry_arc(&scan_id)?;
    let dev = entry.device();
    if dev.is_null() {
        return Err(BridgeError::bridge(
            code::DEVICE_GONE,
            "the source device is not attached — recovery needs the source volume/disk",
        ));
    }

    // Config map (type-maps §6): collision -> conflict; verifyOnWrite ->
    // skip_hash_verify; rebuildFolders -> skip_dir_reconstruct. The defaults
    // (read_chunk 0 = 1 MiB, rename_attempts 0 = 64) stay engine-side.
    let mut cfg: abi::CrUiRecoveryConfig = unsafe { std::mem::zeroed() };
    cfg.conflict = conflict_of(&options);
    cfg.skip_hash_verify = if options.get("verifyOnWrite").and_then(|v| v.as_bool()) == Some(false) { 1 } else { 0 };
    cfg.skip_dir_reconstruct = if options.get("rebuildFolders").and_then(|v| v.as_bool()) == Some(false) { 1 } else { 0 };

    let (tx, rx) = channel::<(u64, u64)>();
    let sink_user = Box::into_raw(Box::new(tx.clone()));
    cfg.progress = Some(crate::bridge::events::recovery_progress_sink);
    cfg.progress_user = sink_user as *mut std::ffi::c_void;

    let mut rec: *mut abi::CrUiRecovery = std::ptr::null_mut();
    check(unsafe { abi::cr_ui_recovery_create(dev, &cfg, &mut rec) }, "cr_ui_recovery_create")?;
    let recovery = SendPtr::new(rec);

    // Destination: the engine normalizes + arms the overlap refusal (F12).
    let dest = crate::bridge::util::cstr(&destination);
    check(
        unsafe { abi::cr_ui_recovery_set_destination(rec, dest.as_ptr()) },
        "cr_ui_recovery_set_destination",
    )?;

    // Enqueue: session trees use node_id handles; live scans enqueue against
    // the CURRENT snapshot generation (G-02b — the value path, race-free).
    let is_session = matches!(entry.kind, ScanKind::Session { .. });
    let sess = entry.session_handle();
    let snap_ptr = {
        let snap = entry
            .snap
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "snapshot lock"))?;
        snap.handle.get()
    };
    let scope_ok = entry.path_scope.is_none();
    let _ = scope_ok;

    let mut node_ids: Vec<String> = Vec::new();
    match selection.get("kind").and_then(|k| k.as_str()) {
        Some("nodes") => {
            let ids = selection
                .get("nodeIds")
                .and_then(|n| n.as_array())
                .cloned()
                .unwrap_or_default();
            for id in ids {
                let id_s = id.as_str().unwrap_or_default().to_string();
                let enqueue_rc = if is_session {
                    let node_id_u64 = resolve_session_node_id(&entry, &id_s)?;
                    if id_s.starts_with('d') {
                        unsafe { abi::cr_ui_recovery_enqueue_subtree(rec, sess, node_id_u64) }
                    } else {
                        unsafe { abi::cr_ui_recovery_enqueue_node(rec, sess, node_id_u64) }
                    }
                } else if id_s.starts_with('d') {
                    let idx: u32 = id_s[1..].parse().map_err(|_| bad_sel(&id_s))?;
                    check(
                        unsafe { abi::cr_ui_recovery_enqueue_subtree_snapshot(rec, snap_ptr, idx) },
                        "cr_ui_recovery_enqueue_subtree_snapshot",
                    )?;
                    node_ids.push(id_s);
                    continue;
                } else {
                    let idx: u32 = id_s.strip_prefix('f').and_then(|r| r.parse().ok()).ok_or_else(|| bad_sel(&id_s))?;
                    check(
                        unsafe { abi::cr_ui_recovery_enqueue_snapshot(rec, snap_ptr, idx) },
                        "cr_ui_recovery_enqueue_snapshot",
                    )?;
                    node_ids.push(id_s);
                    continue;
                };
                // Session path: surface enqueue refusals truthfully.
                check(enqueue_rc, "cr_ui_recovery_enqueue")?;
                node_ids.push(id_s);
            }
        }
        Some("all") => {
            // Bridge pages the filtered query and enqueues every matching
            // FILE (structure materializes through the writer's dir
            // reconstruction — G-08). G-11 scope rides the filter.
            let filter = selection.get("filter").cloned().unwrap_or_else(|| json!({}));
            let mut offset = 0u64;
            loop {
                let page = query_page(
                    state,
                    &scan_id,
                    &filter,
                    "name",
                    false,
                    offset,
                    500,
                    false,
                )?;
                let nodes = page.get("nodes").and_then(|n| n.as_array()).cloned().unwrap_or_default();
                if nodes.is_empty() {
                    break;
                }
                for node in &nodes {
                    let id_s = node.get("id").and_then(|i| i.as_str()).unwrap_or_default().to_string();
                    if is_session {
                        let node_id_u64 = resolve_session_node_id(&entry, &id_s)?;
                        check(
                            unsafe { abi::cr_ui_recovery_enqueue_node(rec, sess, node_id_u64) },
                            "cr_ui_recovery_enqueue_node",
                        )?;
                    } else {
                        let idx: u32 = id_s.strip_prefix('f').and_then(|r| r.parse().ok()).unwrap_or(u32::MAX);
                        check(
                            unsafe { abi::cr_ui_recovery_enqueue_snapshot(rec, snap_ptr, idx) },
                            "cr_ui_recovery_enqueue_snapshot",
                        )?;
                    }
                }
                let fetched = nodes.len() as u64;
                offset += fetched;
                if fetched < 500 {
                    break;
                }
            }
        }
        _ => {
            return Err(BridgeError::bridge(
                code::UNKNOWN,
                "selection must be { kind: 'nodes' | 'all' }",
            ));
        }
    }

    let mut queued: u32 = 0;
    check(unsafe { abi::cr_ui_recovery_queue_count(rec, &mut queued) }, "cr_ui_recovery_queue_count")?;

    let job_id = state.mint_job_id();
    let started_at = now_iso();
    let entry_live = entry.live.clone();
    let _ = entry_live;
    let job = Arc::new(JobEntry {
        job_id: job_id.clone(),
        scan_id: scan_id.clone(),
        recovery,
        device: SendPtr::new(dev),
        destination: destination.clone(),
        options_json: options.clone(),
        node_ids,
        started_at,
        finished: Mutex::new(None),
        live: Arc::new(Mutex::new(JobLive::new(queued as u64))),
        events_tx: tx,
        sink_user: SendPtr::new(sink_user),
    });

    state
        .jobs
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "jobs lock"))?
        .insert(job_id.clone(), job.clone());
    entry
        .jobs
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "jobs list lock"))?
        .push(job_id.clone());
    Ok((job, job_id, rx))
}

fn bad_sel(id: &str) -> BridgeError {
    BridgeError::bridge(code::UNKNOWN, format!("selection id '{id}' does not resolve in this tree"))
}

/// Session trees address nodes by engine node_id; live scans by snapshot
/// index. The registry stores the mapping in the metadata cache.
fn resolve_session_node_id(entry: &Arc<ScanEntry>, id: &str) -> BridgeResult<u64> {
    let snap = entry
        .snap
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "snapshot lock"))?;
    let metas = snap
        .metas
        .as_ref()
        .ok_or_else(|| BridgeError::bridge(code::UNKNOWN, "metadata cache missing"))?;
    let idx: usize = id
        .trim_start_matches(|c| c == 'f' || c == 'd')
        .parse()
        .map_err(|_| bad_sel(id))?;
    let m = metas.get(idx).ok_or_else(|| bad_sel(id))?;
    Ok(m.node_id)
}

fn spawn_process_thread(
    app: tauri::AppHandle,
    job: Arc<JobEntry>,
    finished: Arc<AtomicBool>,
) {
    std::thread::spawn(move || {
        let rc = unsafe { abi::cr_ui_recovery_process(job.recovery.get()) };
        let mut stats: abi::CrUiRecoveryStats = unsafe { std::mem::zeroed() };
        let _ = unsafe { abi::cr_ui_recovery_get_stats(job.recovery.get(), &mut stats) };
        let status: &'static str = if rc == abi::CR_UI_ERR_CANCELLED {
            "cancelled"
        } else if rc != abi::CR_UI_OK {
            "failed"
        } else if stats.done + stats.salvaged == 0 && stats.refused + stats.errors + stats.hash_mismatch + stats.nothing_to_recover > 0 {
            // nothing committed and something went wrong before the writer
            "failed"
        } else if stats.done + stats.salvaged > 0
            && stats.errors == 0
            && stats.refused == 0
            && stats.hash_mismatch == 0
        {
            "completed"
        } else if stats.queued == stats.nothing_to_recover {
            // metadata-only queue: truthful completed with empty lists
            "completed"
        } else {
            "completed-partial"
        };
        let finished_at = now_iso();
        if let Ok(mut f) = job.finished.lock() {
            *f = Some((status, finished_at.clone()));
        }
        if let Ok(mut l) = job.live.lock() {
            l.status = status;
            l.current_file = None;
            l.verifying = false;
            if status == "completed" || status == "completed-partial" {
                l.overall = 1.0;
                l.eta_seconds = Some(0);
            }
        }
        finished.store(true, Ordering::Release);
        // The emitter stops composing once `finished` is set; the process
        // thread emits the terminal payload itself (single-writer rule).
        emit_job_terminal(&app, &job.job_id, &job.live, status);
    });
}

// ---------------------------------------------------------------------------
// cancel / state / items / report
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn recovery_cancel(
    state: tauri::State<'_, Arc<BridgeState>>,
    job_id: String,
) -> Result<(), BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let job = state.job_entry_arc(&job_id)?;
        if job.finished.lock().map(|f| f.is_some()).unwrap_or(false) {
            return Ok(()); // terminal: the truthful no-op
        }
        check(unsafe { abi::cr_ui_recovery_cancel(job.recovery.get()) }, "cr_ui_recovery_cancel")
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn recovery_state(
    state: tauri::State<'_, Arc<BridgeState>>,
    job_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let job = state.job_entry_arc(&job_id)?;
        let status = job
            .finished
            .lock()
            .ok()
            .and_then(|f| f.as_ref().map(|(s, _)| *s))
            .or_else(|| job.live.lock().ok().map(|l| l.status))
            .unwrap_or("validating");
        Ok(json!({
            "jobId": job.job_id,
            "scanId": job.scan_id,
            "nodeIds": job.node_ids,
            "destination": job.destination,
            "options": job.options_json,
            "status": status,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// The engine's report JSON parsed (owned; freed on drop).
fn fetch_report(job: &Arc<JobEntry>) -> BridgeResult<serde_json::Value> {
    let mut json_ptr: *mut i8 = std::ptr::null_mut();
    let source = crate::bridge::util::cstr(&job.scan_id);
    let rc = unsafe {
        abi::cr_ui_recovery_report_json(job.recovery.get(), source.as_ptr(), &mut json_ptr)
    };
    check(rc, "cr_ui_recovery_report_json")?;
    let owned = unsafe {
        crate::bridge::util::OwnedCrString::from_raw(json_ptr)
            .ok_or_else(|| BridgeError::bridge(code::UNKNOWN, "engine returned a null report"))?
    };
    owned.to_json()
}

fn report_items(v: &serde_json::Value) -> Vec<&serde_json::Value> {
    v.get("items")
        .and_then(|i| i.as_array())
        .map(|a| a.iter().collect())
        .unwrap_or_default()
}

#[tauri::command]
pub async fn recovery_items(
    state: tauri::State<'_, Arc<BridgeState>>,
    job_id: String,
    page: Value,
) -> Result<Vec<Value>, BridgeError> {
    let state = state.inner().clone();
    let offset = page.get("offset").and_then(|o| o.as_u64()).unwrap_or(0) as usize;
    let limit = page.get("limit").and_then(|l| l.as_u64()).unwrap_or(500) as usize;
    tauri::async_runtime::spawn_blocking(move || {
        let job = state.job_entry_arc(&job_id)?;
        let report = fetch_report(&job)?;
        let items = report_items(&report);
        let rows: Vec<Value> = items
            .iter()
            .skip(offset)
            .take(limit)
            .map(|item| {
                let outcome = item.get("outcome").and_then(|o| o.as_str()).unwrap_or("pending");
                let state_word = outcome_state(outcome);
                let mut row = json!({
                    // Queue-stable id: the report's own item_id (the UI uses
                    // it as a React list key only).
                    "nodeId": format!("item-{}", item.get("item_id").and_then(|i| i.as_u64()).unwrap_or(0)),
                    "name": item.get("name").and_then(|n| n.as_str()).unwrap_or(""),
                    "sizeBytes": item.get("bytes").and_then(|b| b.as_u64()).unwrap_or(0),
                    "state": state_word,
                });
                if state_word == "failed" {
                    row["error"] = json!({
                        "code": "recovery-item-failed",
                        "reasonKey": "recovery.itemFailedReason",
                    });
                }
                if state_word == "done" {
                    row["savedAs"] = item.get("path").and_then(|p| p.as_str()).map(String::from);
                }
                row
            })
            .collect();
        Ok(rows)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn recovery_report(
    state: tauri::State<'_, Arc<BridgeState>>,
    job_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let job = state.job_entry_arc(&job_id)?;
        let report = fetch_report(&job)?;
        let items = report_items(&report);
        let status = job
            .finished
            .lock()
            .ok()
            .and_then(|f| f.as_ref().map(|(s, _)| *s))
            .or_else(|| job.live.lock().ok().map(|l| l.status))
            .unwrap_or("validating");

        let cap = 500usize;
        let mut succeeded: Vec<Value> = Vec::new();
        let mut failed: Vec<Value> = Vec::new();
        let mut skipped: Vec<Value> = Vec::new();
        let mut processed: u64 = 0;
        let mut verified_count = 0u64;
        for item in &items {
            let outcome = item.get("outcome").and_then(|o| o.as_str()).unwrap_or("pending");
            if item.get("hash_status").and_then(|h| h.as_str()) == Some("verified") {
                verified_count += 1;
            }
            let state_word = outcome_state(outcome);
            if state_word == "pending" {
                continue; // never-attempted rows are not processed
            }
            processed += 1;
            let row = json!({
                "nodeId": format!("item-{}", item.get("item_id").and_then(|i| i.as_u64()).unwrap_or(0)),
                "name": item.get("name").and_then(|n| n.as_str()).unwrap_or(""),
                "sizeBytes": item.get("bytes").and_then(|b| b.as_u64()).unwrap_or(0),
                "state": state_word,
                "error": if state_word == "failed" {
                    json!({ "code": "recovery-item-failed", "reasonKey": "recovery.itemFailedReason" })
                } else {
                    Value::Null
                },
                "savedAs": if state_word == "done" {
                    item.get("path").and_then(|p| p.as_str()).map(String::from)
                } else {
                    None
                },
            });
            match state_word {
                "done" => succeeded.push(row),
                "failed" => failed.push(row),
                _ => skipped.push(row),
            }
        }
        let truncated = succeeded.len() > cap || failed.len() > cap || skipped.len() > cap;
        succeeded.truncate(cap);
        failed.truncate(cap);
        skipped.truncate(cap);

        let totals = report.get("totals").cloned().unwrap_or_else(|| json!({}));
        let errors = totals.get("errors").and_then(|e| e.as_u64()).unwrap_or(0);
        let bytes_written = totals.get("bytes").and_then(|b| b.as_u64()).unwrap_or(0);
        let finished_at = job
            .finished
            .lock()
            .ok()
            .and_then(|f| f.as_ref().map(|(_, at)| at.clone()));

        Ok(json!({
            "jobId": job.job_id,
            "status": status,
            "startedAt": job.started_at,
            "finishedAt": finished_at,
            "succeeded": succeeded,
            "failed": failed,
            "skipped": skipped,
            "totalBytesWritten": bytes_written,
            "verified": verified_count > 0 && errors == 0,
            "processedTotal": processed,
            "truncatedItemLists": truncated,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
