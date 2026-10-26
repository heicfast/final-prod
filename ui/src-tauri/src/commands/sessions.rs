//! commands/sessions.rs — session_save / sessions_list / session_load
//! (contracts/command-surface.json; type-maps §9).
//!
//! Session store: %ProgramData%\HeicFast\Recovery Wizard\sessions\ (portable
//! builds: .\sessions\ beside the exe). Format .crsf (CRSFMT01); auto-name
//! `auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf` (N-061 parity with the reference's
//! auto_<volume>_<date> convention).
//!
//! listSessions (OQ-4 resolved): stat-first listing against the directory
//! mtime, labels/counts filled lazily via one load+get_info+stats+close pass
//! per file, cached in the bridge — no sidecar index file.

use std::io::ErrorKind;
use std::path::PathBuf;
use std::sync::Arc;

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, check, BridgeError, BridgeResult};
use crate::bridge::state::{
    session_target_label, BridgeState, ScanEntry, ScanKind, ScanLive, SessionListRow,
    SnapshotCache, Terminal,
};
use crate::bridge::util::{enumerate_disks, enumerate_volumes, fixed_str, iso_from_unix_ms, now_iso, now_unix_ms, SendPtr};

/// Sanitize a volume label into the auto-name token (N-061: filesystem-safe).
fn sanitize_label(label: &str) -> String {
    let cleaned: String = label
        .chars()
        .map(|c| {
            if c.is_ascii_alphanumeric() || c == '-' || c == '_' {
                c
            } else {
                '_'
            }
        })
        .collect();
    let trimmed = cleaned.trim_matches('_').to_string();
    if trimmed.is_empty() {
        "volume".to_string()
    } else {
        trimmed
    }
}

fn entry_label(entry: &Arc<ScanEntry>) -> String {
    let l = entry
        .target_json
        .get("label")
        .and_then(|x| x.as_str())
        .unwrap_or("volume");
    sanitize_label(l)
}

/// Compose the auto-name from wall time: auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf
/// (UTC — documented deviation from the reference's local-date naming: std
/// gives no TZ offset and the bridge stays dependency-free/deterministic).
fn auto_name(label: &str, dir: &PathBuf) -> PathBuf {
    let secs = now_unix_ms().div_euclid(1000);
    let days = secs.div_euclid(86_400);
    let tod = secs.rem_euclid(86_400);
    let (_y, mo, d) = crate::bridge::util::civil_parts(days);
    let (h, mi, s) = (tod / 3600, (tod % 3600) / 60, tod % 60);
    dir.join(format!("auto_{label}_{mo:02}_{d:02}_{h:02}_{mi:02}_{s:02}.crsf"))
}

#[tauri::command]
pub async fn session_save(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    path: Option<String>,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let scan_ptr = entry.scan_handle();
        if scan_ptr.is_null() {
            return Err(BridgeError::bridge(
                code::UNKNOWN,
                "only a live scan can be saved (this entry is a loaded session)",
            ));
        }
        let (busy, _s, _p) = crate::bridge::events::scan_flags(scan_ptr);
        if busy {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                "save the session after the scan finishes (or cancel it first)",
            ));
        }

        let label = entry_label(&entry);
        let target_path = match path {
            Some(p) if !p.trim().is_empty() => PathBuf::from(p),
            _ => auto_name(&label, &state.sessions_dir),
        };
        let path_str = target_path.to_string_lossy().to_string();
        check(
            unsafe { abi::cr_ui_scan_save_session(scan_ptr, crate::bridge::util::cstr(&path_str).as_ptr()) },
            "cr_ui_scan_save_session",
        )?;

        // Exact meta from stats + the latched terminal state.
        let mut stats: abi::CrUiScanStats = unsafe { std::mem::zeroed() };
        let _ = unsafe { abi::cr_ui_scan_get_stats(scan_ptr, &mut stats) };
        let term = entry.terminal.lock().ok().and_then(|t| t.clone());
        let files_found = stats.files_found + stats.deleted_found + stats.carved_found;
        let completed = term.as_ref().map(|t| t.status == "completed").unwrap_or(false);

        // Invalidate the list cache (mtime-based; the new file changes it).
        *state
            .session_list_cache
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "cache lock"))? = None;

        Ok(json!({
            "id": format!("sess-{}", now_unix_ms().max(0) as u64),
            "fileName": target_path.file_name()
                .map(|f| f.to_string_lossy().to_string())
                .unwrap_or_else(|| path_str.clone()),
            "savedAt": now_iso(),
            "targetLabel": entry.target_json.get("label").and_then(|l| l.as_str()).unwrap_or("volume"),
            "filesFound": files_found,
            "dataFoundBytes": term.as_ref().map(|t| t.data_found_bytes).unwrap_or(0),
            "completed": completed,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// Read one session's meta (load + info + stats + close — the cheap pass OQ-4
/// sanctioned for listing).
fn probe_session_meta(path: &PathBuf) -> Option<(String, u64, u64, bool)> {
    let path_str = path.to_string_lossy().to_string();
    let mut sess: *mut abi::CrUiSession = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_session_load(crate::bridge::util::cstr(&path_str).as_ptr(), &mut sess) };
    if rc != abi::CR_UI_OK || sess.is_null() {
        return None;
    }
    let mut info: abi::CrUiSessionInfo = unsafe { std::mem::zeroed() };
    let mut stats: abi::CrUiScanStats = unsafe { std::mem::zeroed() };
    let ok_info = unsafe { abi::cr_ui_session_get_info(sess, &mut info) } == abi::CR_UI_OK;
    let ok_stats = unsafe { abi::cr_ui_session_stats(sess, &mut stats) } == abi::CR_UI_OK;
    unsafe { abi::cr_ui_session_close(sess) };
    if !ok_info {
        return None;
    }
    let label = session_target_label(&info);
    let files = if ok_stats {
        stats.files_found + stats.deleted_found + stats.carved_found
    } else {
        0
    };
    // dataFoundBytes: the session record carries no aggregate — the exact
    // sum needs a snapshot pass; for LIST rows the truthful cheap value is
    // the scanned-bytes context, so report the file-count-based zero pass
    // only when stats were readable (G-3 applies to list rows too).
    let completed = info.configured_mask != 0
        && info.configured_mask & info.stage_done_mask == info.configured_mask;
    Some((label, files, 0, completed))
}

#[tauri::command]
pub async fn sessions_list(
    state: tauri::State<'_, Arc<BridgeState>>,
) -> Result<Vec<Value>, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let dir_mtime = state
            .sessions_dir
            .metadata()
            .and_then(|m| m.modified())
            .ok()
            .and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
            .map(|d| d.as_millis() as i64)
            .unwrap_or(0);

        // Cache hit: same directory mtime -> reuse rows.
        {
            let cache = state
                .session_list_cache
                .lock()
                .map_err(|_| BridgeError::bridge(code::UNKNOWN, "cache lock"))?;
            if let Some(c) = cache.as_ref() {
                if c.dir_mtime_ms == dir_mtime {
                    return Ok(render_rows(&c.rows));
                }
            }
        }

        let mut rows: Vec<SessionListRow> = Vec::new();
        let read = std::fs::read_dir(&state.sessions_dir)
            .map_err(|e| {
                if e.kind() == ErrorKind::NotFound {
                    BridgeError::bridge(code::UNKNOWN, "sessions directory is missing")
                } else {
                    BridgeError::bridge(code::UNKNOWN, format!("sessions directory: {e}"))
                }
            })?;
        for entry in read.flatten() {
            let p = entry.path();
            if p.extension().and_then(|e| e.to_str()) != Some("crsf") {
                continue;
            }
            let saved_ms = entry
                .metadata()
                .and_then(|m| m.modified())
                .ok()
                .and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
                .map(|d| d.as_millis() as i64)
                .unwrap_or(0);
            rows.push(SessionListRow {
                file_name: p.file_name().map(|f| f.to_string_lossy().to_string()).unwrap_or_default(),
                path: p,
                saved_at_iso: iso_from_unix_ms(saved_ms),
                saved_at_ms: saved_ms,
                meta: None,
            });
        }
        // Fill meta lazily (one load+close per file), then cache.
        for row in rows.iter_mut() {
            if let Some((label, files, data_bytes, completed)) = probe_session_meta(&row.path) {
                row.meta = Some(crate::bridge::state::SessionListMeta {
                    target_label: label,
                    files_found: files,
                    data_found_bytes: data_bytes,
                    completed,
                });
            }
        }
        rows.sort_by(|a, b| b.saved_at_ms.cmp(&a.saved_at_ms));
        *state
            .session_list_cache
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "cache lock"))? =
            Some(crate::bridge::state::SessionListCache {
                dir_mtime_ms: dir_mtime,
                rows: rows.clone(),
            });
        Ok(render_rows(&rows))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

fn render_rows(rows: &[SessionListRow]) -> Vec<Value> {
    rows.iter()
        .map(|r| {
            let (label, files, data_bytes, completed) = match &r.meta {
                Some(m) => (
                    m.target_label.clone(),
                    m.files_found,
                    m.data_found_bytes,
                    m.completed,
                ),
                // Unreadable/foreign .crsf: truthful degraded row.
                None => ("(unreadable session)".to_string(), 0, 0, false),
            };
            json!({
                "id": format!("sess-{}", r.saved_at_ms.max(0) as u64),
                "fileName": r.file_name,
                "savedAt": r.saved_at_iso,
                "targetLabel": label,
                "filesFound": files,
                "dataFoundBytes": data_bytes,
                "completed": completed,
            })
        })
        .collect()
}

#[tauri::command]
pub async fn session_load(
    state: tauri::State<'_, Arc<BridgeState>>,
    path: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        // Bridge pre-check: a missing file is session-not-found (the engine
        // never sees it — error-codes.json ERR_SESSION_IO refinement).
        let target = PathBuf::from(&path);
        if !target.is_file() {
            return Err(BridgeError::bridge(
                code::SESSION_NOT_FOUND,
                format!("session file not found: {path}"),
            ));
        }
        let mut sess: *mut abi::CrUiSession = std::ptr::null_mut();
        let rc = unsafe { abi::cr_ui_session_load(crate::bridge::util::cstr(&path).as_ptr(), &mut sess) };
        if rc != abi::CR_UI_OK {
            // 0x50030001/0x50030002/0x50040001-3 -> session-corrupt (the TS
            // error-codes table maps the numeric; the bridge message carries
            // the engine's own diagnostic).
            return Err(BridgeError::engine(rc, "cr_ui_session_load"));
        }
        let session = SendPtr::new(sess);
        let mut info: abi::CrUiSessionInfo = unsafe { std::mem::zeroed() };
        check(unsafe { abi::cr_ui_session_get_info(sess, &mut info) }, "cr_ui_session_get_info")?;
        let mut stats: abi::CrUiScanStats = unsafe { std::mem::zeroed() };
        let _ = unsafe { abi::cr_ui_session_stats(sess, &mut stats) };
        let label = session_target_label(&info);
        let files_found = stats.files_found + stats.deleted_found + stats.carved_found;
        let completed = info.configured_mask != 0
            && info.configured_mask & info.stage_done_mask == info.configured_mask;

        // Best-effort source re-attach: model+serial match against fresh
        // enumeration (session records carry no disk number / letter).
        let device = attach_source_device(&info);

        let scan_id = state.mint_scan_id();
        let target_json = json!({
            "id": format!("vol-{scan_id}"),
            "deviceId": Value::Null,
            "label": label,
            "letter": Value::Null,
            "fs": "unknown",
            "capacity": {
                "totalBytes": info.slice_bytes,
                "usedBytes": 0,
                "freeBytes": 0,
            },
            "status": "ok",
            "quickScanAvailable": true,
            "sectorSize": info.logical_sector,
        });
        let entry = Arc::new(ScanEntry {
            scan_id: scan_id.clone(),
            kind: ScanKind::Session {
                session,
                device,
            },
            target_json,
            target_id: path.clone(),
            device_id: String::new(),
            volume_id: None,
            params_json: json!({ "targetId": path }),
            path_scope: None,
            started_at: now_iso(),
            live: Arc::new(std::sync::Mutex::new(ScanLive::new(info.slice_bytes))),
            terminal: Arc::new(std::sync::Mutex::new(Some(Terminal {
                status: if completed { "completed" } else { "cancelled" },
                engine_code: 0,
                completed_at: now_iso(),
                files_found,
                data_found_bytes: 0,
            }))),
            snapshot_exists: Arc::new(std::sync::atomic::AtomicBool::new(false)),
            events_tx: {
                // Session entries never subscribe a sink; a bounded placeholder
                // keeps the field's never-block shape without a live receiver.
                let (tx, _rx) = std::sync::mpsc::sync_channel(1);
                tx
            },
            sink_user: SendPtr::new(std::ptr::null_mut()),
            snap: std::sync::Mutex::new(SnapshotCache {
                gen: 0,
                handle: SendPtr::new(std::ptr::null_mut()),
                partial: false,
                node_count: 0,
                metas: None,
                last_refresh_ms: 0,
                frozen: true,
            }),
            jobs: std::sync::Mutex::new(Vec::new()),
        });
        state
            .scans
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "registry lock"))?
            .insert(scan_id.clone(), entry);
        Ok(json!({ "scanId": scan_id }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// Resolve the session's source device by model + serial over fresh
/// enumeration; the owning volume is pinned by slice offset+bytes. None when
/// the source is absent or ambiguous (multiple identical disks).
fn attach_source_device(info: &abi::CrUiSessionInfo) -> Option<SendPtr<abi::CrUiDevice>> {
    let model = fixed_str(&info.model);
    let serial = fixed_str(&info.serial);
    if model.is_empty() && serial.is_empty() {
        return None;
    }
    let disks = enumerate_disks().ok()?;
    let matches: Vec<_> = disks
        .iter()
        .filter(|d| {
            let dm = fixed_str(&d.model);
            let ds = fixed_str(&d.serial);
            (!model.is_empty() && dm.eq_ignore_ascii_case(&model))
                || (!serial.is_empty() && ds.eq_ignore_ascii_case(&serial))
        })
        .collect();
    if matches.is_empty() {
        return None;
    }
    // Prefer the volume whose slice matches exactly (offset + bytes).
    if let Ok(vols) = enumerate_volumes() {
        for v in &vols {
            if matches.iter().any(|d| d.number == v.disk)
                && v.offset_on_disk == info.slice_offset
                && v.bytes == info.slice_bytes
                && v.letter != 0
            {
                let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
                if unsafe { abi::cr_ui_open_volume(v.letter as i8, &mut dev) } == abi::CR_UI_OK
                    && !dev.is_null()
                {
                    return Some(SendPtr::new(dev));
                }
            }
        }
    }
    if matches.len() == 1 {
        let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
        if unsafe { abi::cr_ui_open_disk(matches[0].number, &mut dev) } == abi::CR_UI_OK
            && !dev.is_null()
        {
            return Some(SendPtr::new(dev));
        }
    }
    None // ambiguous or unopenable — preview/recovery fail truthfully per node
}
