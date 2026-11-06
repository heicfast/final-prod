//! commands/sessions.rs — session_save / sessions_list / session_load /
//! session_resume_last / session_resume / session_meta.
//!
//! Session store: %ProgramData%\HeicFast\Recovery Wizard\sessions\ (portable
//! builds: .\sessions\ beside the exe — the marker probe, NG-18). Format
//! .crsf (CRSFMT01); auto-name `auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf`
//! (N-061 parity with the reference's auto_<volume>_<date> convention).
//!
//! v4 surfaces:
//! - W1a light listing: `cr_ui_session_list` decodes every *.crsf's LIGHT
//!   records (SESSION_STATE + VOLUME_INFO — the heavy payloads are skipped
//!   by type before any payload I/O), newest-first by the embedded save
//!   time. This replaces the v2 per-file load+close probe pass.
//! - W1b load progress: `cr_ui_session_load_with_events` with the
//!   calling-thread sink emitting `session-load-progress:<path>`.
//! - M-154 pointed resume: `cr_ui_session_resume_last` loads the auto
//!   session the last_scan.info sidecar names (no directory scan).
//! - The scan RESUME (the CLI's `scan --resume`): load + session_resume +
//!   best-effort device re-attach + scan_verify_identity (the boot-hash
//!   warning — ERR_BAD_CHECKSUM is the typed session-corrupt identity
//!   mismatch) + run.

use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::sync::Mutex;

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::consent::ConsentPipe;
use crate::bridge::error::{check, code, BridgeError, BridgeResult};
use crate::bridge::events::{scan_flags, SessionLoadPipe};
use crate::bridge::state::{
    session_target_label, BridgeState, ScanEntry, ScanKind, ScanLive, SnapshotCache, Terminal,
};
use crate::bridge::util::{
    enumerate_disks, enumerate_volumes, fixed_str, iso_from_unix_ms, now_iso, now_unix_ms, SendPtr,
};
use crate::commands::scan::{launch_run_thread, ScanPrep};

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
fn auto_name(label: &str, dir: &Path) -> PathBuf {
    let secs = now_unix_ms().div_euclid(1000);
    let days = secs.div_euclid(86_400);
    let tod = secs.rem_euclid(86_400);
    let (_y, mo, d) = crate::bridge::util::civil_parts(days);
    let (h, mi, s) = (tod / 3600, (tod % 3600) / 60, tod % 60);
    dir.join(format!(
        "auto_{label}_{mo:02}_{d:02}_{h:02}_{mi:02}_{s:02}.crsf"
    ))
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
        let (busy, _s, _p) = scan_flags(scan_ptr);
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

/// The W1a light listing: one row per *.crsf, newest-first (the embedded
/// save time; pre-v5 rows break ties by path bytes — the engine's own
/// deterministic order). Unreadable files still get a row (valid = 0) — a
/// listing never hides damage.
#[tauri::command]
pub async fn sessions_list(
    state: tauri::State<'_, Arc<BridgeState>>,
) -> Result<Vec<Value>, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || -> BridgeResult<Vec<Value>> {
        let dir = crate::bridge::util::cstr(&state.sessions_dir.to_string_lossy());
        let mut needed: u32 = 0;
        check(
            unsafe { abi::cr_ui_session_list(dir.as_ptr(), std::ptr::null_mut(), &mut needed) },
            "cr_ui_session_list(size)",
        )?;
        if needed == 0 {
            return Ok(Vec::new());
        }
        let mut rows = vec![unsafe { std::mem::zeroed::<abi::CrUiSessionRow>() }; needed as usize];
        let mut cap = needed;
        check(
            unsafe { abi::cr_ui_session_list(dir.as_ptr(), rows.as_mut_ptr(), &mut cap) },
            "cr_ui_session_list",
        )?;
        rows.truncate(cap as usize);
        Ok(rows.iter().map(render_row).collect())
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

fn render_row(r: &abi::CrUiSessionRow) -> Value {
    let path = fixed_str(&r.path);
    let file_name = path.rsplit(['\\', '/']).next().unwrap_or(&path).to_string();
    let valid = r.valid != 0 && r.error == abi::CR_UI_OK;
    let label = {
        let full = fixed_str(&r.label_full);
        if full.is_empty() {
            fixed_str(&r.label)
        } else {
            full
        }
    };
    // save_unix == 0 = a pre-v5 session: the row's own fallback is the file
    // mtime, which the light record does not carry — the UI renders the
    // "legacy session" marker instead of a fabricated time.
    let saved_at = if r.save_unix != 0 {
        json!(iso_from_unix_ms((r.save_unix * 1000) as i64))
    } else {
        Value::Null
    };
    let completed =
        r.configured_mask != 0 && r.configured_mask & r.stage_done_mask == r.configured_mask;
    json!({
        "id": format!("sess-{}", r.save_unix.max(1)),
        "fileName": file_name,
        "path": path,
        "savedAt": saved_at,
        // v0.8.0 forensic (run 42170466748): a VALID whole-disk session
        // with an empty volume label rendered as '(unreadable session)' —
        // a lie the Backup screen told about a perfectly loadable
        // checkpoint (the light row read fine: date + counts present;
        // only the label field was empty — whole-disk sessions carry no
        // volume label). Split the cases: unreadable means unreadable;
        // a valid row without a label says so. (BF-26: recording the
        // disk identity in the session header is upstream engine scope.)
        "targetLabel": (if !valid {
            json!("(unreadable session)")
        } else if label.is_empty() {
            json!("(no volume label)")
        } else {
            json!(label)
        }),
        "filesFound": (if valid { json!(r.files_found + r.deleted_found + r.carved_found) } else { json!(0) }),
        "dataFoundBytes": (if valid { json!(r.bytes_scanned) } else { json!(0) }),
        "completed": (if valid { json!(completed) } else { json!(false) }),
        "valid": valid,
    })
}

/// Load a session through `cr_ui_session_load_with_events` (W1b: the load
/// emits `session-load-progress:<path>` from the calling thread; the
/// caller-owned cancel flag is left NULL — a torn load never half-claims
/// state, so an abort is a fresh command, not a flag). Registers a
/// Session-kind entry and returns its scan id.
fn load_session_entry(
    state: &Arc<BridgeState>,
    app: &tauri::AppHandle,
    path: &str,
) -> BridgeResult<Arc<ScanEntry>> {
    let target = PathBuf::from(path);
    if !target.is_file() {
        return Err(BridgeError::bridge(
            code::SESSION_NOT_FOUND,
            format!("session file not found: {path}"),
        ));
    }
    let pipe = Box::new(SessionLoadPipe {
        app: app.clone(),
        path: path.to_string(),
        last_permille: 0,
    });
    let pipe_ptr = Box::into_raw(pipe);
    let mut sess: *mut abi::CrUiSession = std::ptr::null_mut();
    let path_c = crate::bridge::util::cstr(path);
    let rc = unsafe {
        abi::cr_ui_session_load_with_events(
            path_c.as_ptr(),
            Some(crate::bridge::events::session_load_sink),
            pipe_ptr as *mut std::ffi::c_void,
            std::ptr::null_mut(),
            &mut sess,
        )
    };
    unsafe { drop(Box::from_raw(pipe_ptr)) };
    if rc != abi::CR_UI_OK {
        // 0x50030001/0x50030002/0x50040001-3 -> session-corrupt (the TS
        // error-codes table maps the numeric; the bridge message carries
        // the engine's own diagnostic).
        return Err(BridgeError::engine(rc, "cr_ui_session_load_with_events"));
    }
    let session = SendPtr::new(sess);
    let mut info: abi::CrUiSessionInfo = unsafe { std::mem::zeroed() };
    check(
        unsafe { abi::cr_ui_session_get_info(sess, &mut info) },
        "cr_ui_session_get_info",
    )?;
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
        kind: ScanKind::Session { session, device },
        target_json,
        device_id: String::new(),
        volume_id: None,
        params_json: json!({ "targetId": path }),
        path_scope: None,
        started_at: now_iso(),
        live: Arc::new(Mutex::new(ScanLive::new(info.slice_bytes))),
        terminal: Arc::new(Mutex::new(Some(Terminal {
            status: if completed { "completed" } else { "cancelled" },
            engine_code: 0,
            completed_at: now_iso(),
            files_found,
            data_found_bytes: 0,
            autosaved: false,
        }))),
        snapshot_exists: Arc::new(std::sync::atomic::AtomicBool::new(false)),
        sink_user: SendPtr::new(std::ptr::null_mut()),
        snap: Mutex::new(SnapshotCache {
            gen: 0,
            handle: SendPtr::new(std::ptr::null_mut()),
            partial: false,
            node_count: 0,
            metas: None,
            last_refresh_ms: 0,
            frozen: true,
        }),
        jobs: Mutex::new(Vec::new()),
    });
    state
        .scans
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "registry lock"))?
        .insert(scan_id.clone(), entry.clone());
    Ok(entry)
}

#[tauri::command]
pub async fn session_load(
    app: tauri::AppHandle,
    state: tauri::State<'_, Arc<BridgeState>>,
    path: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = load_session_entry(&state, &app, &path)?;
        Ok(json!({ "scanId": entry.scan_id }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// One row's meta from the light listing (the cheap pass — no load).
#[tauri::command]
pub async fn session_meta(
    state: tauri::State<'_, Arc<BridgeState>>,
    path: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || -> BridgeResult<Value> {
        let dir = crate::bridge::util::cstr(&state.sessions_dir.to_string_lossy());
        let mut needed: u32 = 0;
        check(
            unsafe { abi::cr_ui_session_list(dir.as_ptr(), std::ptr::null_mut(), &mut needed) },
            "cr_ui_session_list(size)",
        )?;
        let mut rows = vec![unsafe { std::mem::zeroed::<abi::CrUiSessionRow>() }; needed as usize];
        let mut cap = needed;
        check(
            unsafe { abi::cr_ui_session_list(dir.as_ptr(), rows.as_mut_ptr(), &mut cap) },
            "cr_ui_session_list",
        )?;
        rows.truncate(cap as usize);
        let row = rows
            .iter()
            .find(|r| fixed_str(&r.path).eq_ignore_ascii_case(&path))
            .ok_or_else(|| {
                BridgeError::bridge(
                    code::SESSION_NOT_FOUND,
                    format!("session {path} not found in the store"),
                )
            })?;
        Ok(render_row(row))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// M-154: the pointed resume — load the AUTO session the last_scan.info
/// sidecar names (no directory scan, no sort). ERR_RANGE_OUT_OF_BOUNDS (no
/// live pointer) crosses as the typed session-not-found; the caller falls
/// back to sessions_list.
#[tauri::command]
pub async fn session_resume_last(
    app: tauri::AppHandle,
    state: tauri::State<'_, Arc<BridgeState>>,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || -> BridgeResult<Value> {
        let dir = crate::bridge::util::cstr(&state.sessions_dir.to_string_lossy());
        let mut path_buf = [0i8; abi::CR_UI_SESSION_ROW_PATH];
        let rc = unsafe {
            abi::cr_ui_session_last_autosave_path(
                dir.as_ptr(),
                path_buf.as_mut_ptr(),
                path_buf.len() as u32,
            )
        };
        if rc == 0x5002_0004 {
            // ERR_RANGE_OUT_OF_BOUNDS: no live pointer names a present file.
            return Err(BridgeError::bridge(
                code::SESSION_NOT_FOUND,
                "no last-scan pointer names a present auto session",
            ));
        }
        check(rc, "cr_ui_session_last_autosave_path")?;
        let path = unsafe { crate::bridge::util::cstr_to_string_lossy(path_buf.as_ptr()) };
        if path.is_empty() {
            return Err(BridgeError::bridge(
                code::SESSION_NOT_FOUND,
                "the last-scan pointer is empty",
            ));
        }
        let entry = load_session_entry(&state, &app, &path)?;
        Ok(json!({ "scanId": entry.scan_id, "path": path }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// The scan RESUME (the CLI's `scan --resume`): load the session, resume it
/// into a FRESH scan handle, re-attach the original target (best-effort by
/// model + serial), verify the volume identity (the boot hash —
/// ERR_BAD_CHECKSUM is the typed session-corrupt mismatch), and RUN.
#[tauri::command]
pub async fn session_resume(
    app: tauri::AppHandle,
    state: tauri::State<'_, Arc<BridgeState>>,
    path: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    let state_for_launch = state.clone();
    let app_for_launch = app.clone();
    let prep =
        tauri::async_runtime::spawn_blocking(move || session_resume_blocking(&state, &app, &path))
            .await
            .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))??;
    let scan_id = prep.entry.scan_id.clone();
    launch_run_thread(app_for_launch, state_for_launch, prep);
    Ok(json!({ "scanId": scan_id }))
}

fn session_resume_blocking(
    state: &Arc<BridgeState>,
    app: &tauri::AppHandle,
    path: &str,
) -> BridgeResult<ScanPrep> {
    // Load (W1b progress rides the load events), then resume the session
    // into a fresh scan handle.
    let entry = load_session_entry(state, app, path)?;
    let ScanKind::Session { session, device } = &entry.kind else {
        return Err(BridgeError::bridge(
            code::UNKNOWN,
            "internal: entry is not a session",
        ));
    };
    let Some(dev_ptr) = device.as_ref().map(|d| d.get()) else {
        return Err(BridgeError::bridge(
            code::DEVICE_GONE,
            "the session's source device is not attached — resume needs the original volume/disk",
        ));
    };

    // The resumed config: whole_disk reflects the re-attached view (a
    // volume-slice match resumes as a volume scan; the whole-disk fallback
    // resumes as a whole-disk scan — the attach result IS the original
    // target shape). The NG-21 guard applies as on scan_start; W1c autosave
    // stays on. The consent gate is armed (arm_repair_hooks keeps it).
    let is_volume_view = attach_is_volume(&entry);
    let mut cfg: abi::CrUiScanConfig = unsafe { std::mem::zeroed() };
    cfg.whole_disk = u32::from(!is_volume_view);
    cfg.pre_scan_repair = u32::from(is_volume_view);
    cfg.pre_scan_check = 1;
    let autosave_dir = crate::bridge::util::cstr(&state.sessions_dir.to_string_lossy());
    cfg.autosave_dir_utf8 = autosave_dir.as_ptr();
    let new_scan_id = state.mint_scan_id();
    let consent_user = SendPtr::new(Box::into_raw(Box::new(ConsentPipe {
        scan_id: new_scan_id.clone(),
        app: app.clone(),
        state: state.clone(),
    })));
    cfg.repair_consent = Some(crate::bridge::consent::consent_sink);
    cfg.repair_consent_user = consent_user.get() as *mut std::ffi::c_void;

    let mut scan: *mut abi::CrUiScan = std::ptr::null_mut();
    check(
        unsafe { abi::cr_ui_session_resume(session.get(), &cfg, &mut scan) },
        "cr_ui_session_resume",
    )?;
    let scan_ptr = SendPtr::new(scan);
    check(
        unsafe { abi::cr_ui_scan_attach_device(scan, dev_ptr) },
        "cr_ui_scan_attach_device",
    )?;

    // Volume-identity check (the CLI's boot-hash warning): a mismatch means
    // the session came from a DIFFERENT volume — the typed refusal, never a
    // silent cross-volume resume.
    let vr = unsafe { abi::cr_ui_scan_verify_identity(scan) };
    if vr == abi::CR_UI_ERR_BAD_CHECKSUM {
        unsafe { abi::cr_ui_scan_close(scan) };
        return Err(BridgeError::engine(
            vr,
            "cr_ui_scan_verify_identity (identity mismatch)",
        ));
    }
    // 0 = match or unverifiable (legacy session with no hash) — both run on.

    // Channel + sink BEFORE run (the sink is read at scan start).
    let (tx, rx) = std::sync::mpsc::sync_channel::<abi::CrUiEvent>(512);
    let sink_user = Box::into_raw(Box::new(tx));
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

    let mut geom: abi::CrUiGeometry = unsafe { std::mem::zeroed() };
    let _ = unsafe { abi::cr_ui_device_geometry(dev_ptr, &mut geom) };

    let live = Arc::new(Mutex::new(ScanLive::new(geom.total_bytes)));
    let terminal: Arc<Mutex<Option<Terminal>>> = Arc::new(Mutex::new(None));
    let snapshot_exists = Arc::new(std::sync::atomic::AtomicBool::new(false));

    // The resumed entry BORROWS the loaded session's device (owns_device =
    // false — the loaded entry's Drop closes it). Pin the loaded entry for
    // the borrower's lifetime: the resumed scan id rides its jobs list, so
    // scan_close on the loaded entry refuses while the resumed scan lives.
    let device_borrowed = SendPtr::new(dev_ptr);
    if let Ok(mut jobs) = entry.jobs.lock() {
        jobs.push(new_scan_id.clone());
    }
    let new_entry = Arc::new(ScanEntry {
        scan_id: new_scan_id.clone(),
        kind: ScanKind::Live {
            scan: scan_ptr,
            device: device_borrowed,
            owns_device: false,
        },
        target_json: entry.target_json.clone(),
        device_id: entry.device_id.clone(),
        volume_id: entry.volume_id.clone(),
        params_json: json!({ "targetId": path, "resumed": true }),
        path_scope: None,
        started_at: now_iso(),
        live,
        terminal,
        snapshot_exists,
        sink_user: SendPtr::new(sink_user),
        snap: Mutex::new(SnapshotCache {
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
        .insert(new_scan_id.clone(), new_entry.clone());

    Ok(ScanPrep {
        entry: new_entry,
        events: rx,
        consent: consent_user,
    })
}

/// Did the source re-attach resolve to a VOLUME view (vs a whole disk)?
/// Mirrors attach_source_device's preference order: the geometry of the
/// attached view equals the session's slice bytes iff the volume-slice
/// match landed (a whole-disk fallback carries the parent's full extent).
fn attach_is_volume(entry: &Arc<ScanEntry>) -> bool {
    volume_slice_attached(entry)
}

fn volume_slice_attached(entry: &Arc<ScanEntry>) -> bool {
    let ScanKind::Session {
        device: Some(d), ..
    } = &entry.kind
    else {
        return false;
    };
    // Geometry comparison: a volume view's total equals the session's slice
    // bytes (the slice-on-parent-disk the engine opened); a whole-disk view
    // is the parent's full extent.
    let mut geom: abi::CrUiGeometry = unsafe { std::mem::zeroed() };
    if unsafe { abi::cr_ui_device_geometry(d.get(), &mut geom) } != abi::CR_UI_OK {
        return false;
    }
    let slice = entry
        .target_json
        .get("capacity")
        .and_then(|c| c.get("totalBytes"))
        .and_then(|b| b.as_u64())
        .unwrap_or(0);
    slice > 0 && geom.total_bytes == slice
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
