//! commands/system.rs — the SystemDomain commands (type-maps §10).
//!
//! v4 truth: the product is USER-MODE. There is no driver, no service, no
//! certificate store. D1 (the privilege model): the product requires the
//! administrator token AT PROCESS CREATION via the embedded app manifest
//! (`ui/src-tauri/app.manifest` — requireAdministrator, wired through
//! tauri-build's WindowsAttributes::app_manifest in build.rs). The OS
//! elevation consent happens at launch; there is NO in-app posture probe,
//! NO banner, and NO ShellExecuteW "runas" relaunch — an unelevated
//! process running this product cannot exist (the loader refuses the
//! launch). The engine's own ERR_ELEVATION_REQUIRED (0x500C0001)
//! vocabulary remains the honest backstop for any host that loads the
//! engine without the product shell.
//!
//! validateDestination mirrors the connector contract: same-letter source →
//! 'overlap' (the engine backstops with 0x50050001 at enqueue); non-mount →
//! 'invalid'; free < needed → 'out-of-space'; a real write probe fails →
//! 'unwritable'.
//!
//! User-taught signatures (M-048): the bridge auto-loads the exe-adjacent
//! `userdata-defaults.json` at startup (lib.rs setup — the CLI's
//! rrecov.cpp:1346-1383 posture; the DLL does NOT auto-load), and
//! `system_load_user_signs` re-registers from a user-chosen file (atomic
//! replace). `system_app_info` carries the active count for the About row.

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::check;
use crate::bridge::error::{code, BridgeError, BridgeResult};
use crate::bridge::util::enumerate_volumes;

/// The exe-adjacent defaults file the bridge auto-loads at startup.
pub const USER_SIGN_DEFAULTS_FILE: &str = "userdata-defaults.json";

// ---------------------------------------------------------------------------
// App info
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn system_app_info(app: tauri::AppHandle) -> Result<Value, BridgeError> {
    let pkg = app.package_info();
    Ok(json!({
        "name": pkg.name,
        // VERSION single source of truth: tauri.conf.json's version is the
        // lockstep consumer build.ps1's Assert-VersionConsistency gate
        // enforces against the root VERSION file.
        "version": pkg.version.to_string(),
        // U-01 (the false mock badge): this command only exists on the REAL
        // bridge — the backend identity is 'tauri' by construction. The
        // pre-fix response omitted the field, the About dialog's
        // `connectorMode === 'tauri'` check fell through to the MOCK warning
        // badge on the shipped product (a false status on a truth surface).
        "connectorMode": "tauri",
        // G-6: the only engine identity the ABI carries is the CONTRACT
        // number (cr_ui_abi_version) — reported as such, never dressed up
        // as a product version.
        "engineVersion": format!("cr_ui ABI v{}", unsafe { abi::cr_ui_abi_version() }),
        "platform": std::env::consts::OS,
        // M-048: the active user-taught signature count (About row).
        "userSigns": unsafe { abi::cr_ui_user_sign_count() },
    }))
}

// ---------------------------------------------------------------------------
// User-taught signatures (M-048)
// ---------------------------------------------------------------------------

/// Register a user-signature JSON document (the og's EUUserData shape:
/// `{ "<ext>": { "signs": ["<offset>,<HEX>"], "mime": "<type>" }, ... }`).
/// Atomic replace — never a silent partial load. Used by the Settings
/// import action; the exe-adjacent defaults file is loaded at startup.
pub fn register_user_signs_json(json_bytes: &str) -> BridgeResult<u32> {
    let c = crate::bridge::util::cstr(json_bytes);
    check(
        unsafe { abi::cr_ui_register_user_signs(c.as_ptr(), json_bytes.len() as u32) },
        "cr_ui_register_user_signs",
    )?;
    Ok(unsafe { abi::cr_ui_user_sign_count() })
}

/// Startup auto-load of the exe-adjacent defaults file (the CLI's
/// rrecov.cpp:1346-1383 posture; the DLL does NOT auto-load). Absent file
/// or a parse failure is a LOGGED no-op — never a startup failure.
pub fn load_default_user_signs() {
    let Some(dir) = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.to_path_buf()))
    else {
        return;
    };
    let path = dir.join(USER_SIGN_DEFAULTS_FILE);
    match std::fs::read_to_string(&path) {
        Ok(text) if !text.trim().is_empty() => match register_user_signs_json(&text) {
            Ok(count) => {
                crate::bridge::logging::info(&format!(
                    "user signs: {count} loaded from {}",
                    path.display()
                ));
            }
            Err(e) => {
                crate::bridge::logging::error(&format!(
                    "user signs: {} failed to parse ({}) — starting with none",
                    path.display(),
                    e
                ));
            }
        },
        Ok(_) => {}  // empty file: nothing to load
        Err(_) => {} // absent: the normal case
    }
}

/// The Settings import action: the webview's file input hands over the
/// DOCUMENT text (a path is not observable from the sandboxed input), so
/// the registration is content-based — the atomic engine replace either
/// way. The exe-adjacent defaults stay path-based at startup (internal).
#[tauri::command]
pub async fn system_load_user_signs(content: String) -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(move || {
        let count = register_user_signs_json(&content)?;
        Ok(json!({ "count": count }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// Destination validation + mounts
// ---------------------------------------------------------------------------

/// Volume letter -> (free, total) via GetDiskFreeSpaceExW.
#[cfg(windows)]
fn letter_free_total(letter: u8) -> Option<(u64, u64)> {
    use windows_sys::Win32::Storage::FileSystem::GetDiskFreeSpaceExW;
    let root: Vec<u16> = format!("{}:\\", letter as char)
        .encode_utf16()
        .chain([0])
        .collect();
    let mut free: u64 = 0;
    let mut total: u64 = 0;
    let mut _avail: u64 = 0;
    let ok = unsafe {
        GetDiskFreeSpaceExW(
            root.as_ptr(),
            &mut free as *mut u64,
            &mut total as *mut u64,
            &mut _avail as *mut u64,
        )
    };
    if ok == 0 {
        return None;
    }
    Some((free, total))
}

#[cfg(not(windows))]
fn letter_free_total(_letter: u8) -> Option<(u64, u64)> {
    None
}

fn letter_of_path(path: &str) -> Option<u8> {
    let b = path.trim().as_bytes();
    if b.len() >= 2 && b[0].is_ascii_alphabetic() && b[1] == b':' {
        Some(b[0].to_ascii_uppercase())
    } else {
        None
    }
}

fn destination_result(reason: Option<&str>, free: Option<u64>, needed: u64) -> Value {
    let reason_key = reason.map(|r| match r {
        "overlap" => "recovery.overlapReason",
        "out-of-space" => "recovery.insufficientSpaceBody",
        "unwritable" => "errors.destinationUnwritable",
        _ => "errors.destinationInvalid",
    });
    json!({
        "allowed": reason.is_none(),
        "reason": reason,
        "reasonKey": reason_key,
        "freeBytes": free,
        "neededBytes": needed,
    })
}

/// The source id's volume letters (a device id contributes all its volumes;
/// a volume id contributes its own) — the F12 same-letter overlap check.
fn source_letter_set(source_device_id: &str) -> Vec<u8> {
    let volumes = enumerate_volumes().unwrap_or_default();
    if let Some(rest) = source_device_id.strip_prefix("disk-") {
        let n: u32 = rest.parse().unwrap_or(u32::MAX);
        volumes
            .iter()
            .filter(|v| v.disk == n && v.letter != 0)
            .map(|v| v.letter)
            .collect()
    } else if let Some(rest) = source_device_id.strip_prefix("vol-") {
        if rest.len() == 1 {
            vec![rest.as_bytes()[0].to_ascii_uppercase()]
        } else {
            Vec::new()
        }
    } else {
        Vec::new()
    }
}

#[tauri::command]
pub async fn system_validate_destination(
    path: String,
    source_device_id: String,
    needed_bytes: u64,
) -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(move || {
        let letter = match letter_of_path(&path) {
            Some(l) => l,
            None => return Ok(destination_result(Some("invalid"), None, needed_bytes)),
        };
        if source_letter_set(&source_device_id).contains(&letter) {
            return Ok(destination_result(Some("overlap"), None, needed_bytes));
        }
        let volumes = enumerate_volumes().unwrap_or_default();
        if !volumes.iter().any(|v| v.letter == letter) {
            return Ok(destination_result(Some("invalid"), None, needed_bytes));
        }
        let Some((free, _total)) = letter_free_total(letter) else {
            return Ok(destination_result(Some("invalid"), None, needed_bytes));
        };
        if needed_bytes > 0 && free < needed_bytes {
            return Ok(destination_result(
                Some("out-of-space"),
                Some(free),
                needed_bytes,
            ));
        }
        // Real write probe: create + delete a temp file in the directory.
        let dir = std::path::PathBuf::from(path.trim());
        let probe = dir.join(format!(".rw-probe-{}", std::process::id()));
        match std::fs::File::create(&probe) {
            Ok(_) => {
                let _ = std::fs::remove_file(&probe);
                Ok(destination_result(None, Some(free), needed_bytes))
            }
            Err(_) => Ok(destination_result(
                Some("unwritable"),
                Some(free),
                needed_bytes,
            )),
        }
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn system_list_mounts() -> Result<Vec<Value>, BridgeError> {
    tauri::async_runtime::spawn_blocking(|| -> BridgeResult<Vec<Value>> {
        let volumes = enumerate_volumes()?;
        let rows: Vec<Value> = volumes
            .iter()
            .filter(|v| v.letter != 0)
            .filter_map(|v| {
                let (free, total) = letter_free_total(v.letter)?;
                Some(json!({
                    "id": format!("vol-{}", v.letter as char),
                    "label": crate::bridge::util::fixed_str(&v.label),
                    "path": format!("{}:\\", v.letter as char),
                    "freeBytes": free,
                    "totalBytes": total,
                }))
            })
            .collect();
        Ok(rows)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// Directory picker + open-path (the dialog/opener plugins)
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn system_pick_directory(
    app: tauri::AppHandle,
    _hint: Option<Value>,
) -> Result<Value, BridgeError> {
    use tauri_plugin_dialog::DialogExt;
    // The native picker cannot filter by device; the connector validates the
    // result against the hint (excludeDeviceId → validateDestination) — the
    // same posture the retired simulator's suggestion logic had. The
    // blocking pick runs on the blocking pool (never the main thread, which
    // must keep pumping the dialog's message loop).
    let picked = tauri::async_runtime::spawn_blocking(move || {
        app.dialog()
            .file()
            .blocking_pick_folder()
            .map(|p| p.to_string())
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?;
    Ok(json!(picked))
}

#[tauri::command]
pub async fn system_open_path(app: tauri::AppHandle, path: String) -> Result<(), BridgeError> {
    // Files are REVEALED with selection in the system explorer; folders
    // open directly.
    use tauri_plugin_opener::OpenerExt;
    let p = std::path::PathBuf::from(&path);
    if !p.exists() {
        return Err(BridgeError::bridge(
            code::UNKNOWN,
            format!("path does not exist: {path}"),
        ));
    }
    if p.is_dir() {
        app.opener()
            .open_path(path, None::<&str>)
            .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("open_path: {e}")))?;
    } else {
        app.opener()
            .reveal_item_in_dir(path)
            .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("reveal_item_in_dir: {e}")))?;
    }
    Ok(())
}
