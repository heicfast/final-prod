//! commands/system.rs — the SystemDomain commands (type-maps §10).
//!
//! Driver status probes the SCM service `RDA` (OQ-5 pinned: the engine's
//! cr_transport.h default service name, matching the deployed driver), then
//! opens `\\.\RDA` and reads IOCTL_RDA_GET_VERSION (0x60002448 — u64
//! (Major<<32)|Minor). The note carries the truthful test-signing posture
//! (docs/DEV-SIGNING.md; PARITY-DELTAS): the driver is a REQUIRED component
//! - the engine refuses every raw-disk open without it (ERR_DRIVER_REQUIRED).
//!
//! validateDestination mirrors the connector contract: same-letter source →
//! 'overlap' (the engine backstops with 0x50050001 at enqueue); non-mount →
//! 'invalid'; free < needed → 'out-of-space'; a real write probe fails →
//! 'unwritable'.

use serde_json::{json, Value};

use crate::bridge::error::{code, BridgeError, BridgeResult};
use crate::bridge::util::enumerate_volumes;

const RDA_SERVICE: &str = "RDA";
const RDA_DEVICE: &str = "\\\\.\\RDA";
const IOCTL_RDA_GET_VERSION: u32 = 0x6000_2448;
const DRIVER_REQUIRED_NOTE: &str =
    "required component - the engine refuses raw-disk access without it; \
     this build is development test-signed (docs/DEV-SIGNING.md): stock \
     Secure-Boot machines must enable test signing (bcdedit /set \
     testsigning on) or disable Secure Boot to load it";

// ---------------------------------------------------------------------------
// Driver probe (Windows; truthful answer elsewhere)
// ---------------------------------------------------------------------------

#[cfg(windows)]
fn driver_probe() -> (bool, bool, Option<String>, String) {
    use windows_sys::Win32::Foundation::{CloseHandle, GENERIC_READ, INVALID_HANDLE_VALUE};
    use windows_sys::Win32::Storage::FileSystem::{
        CreateFileW, FILE_SHARE_READ, FILE_SHARE_WRITE, OPEN_EXISTING,
    };
    use windows_sys::Win32::System::IO::DeviceIoControl;
    use windows_sys::Win32::System::Services::{
        CloseServiceHandle, OpenSCManagerA, OpenServiceA, QueryServiceStatus, SERVICE_QUERY_STATUS,
        SERVICE_RUNNING, SERVICE_STATUS,
    };

    const SC_MANAGER_CONNECT: u32 = 0x0001;

    // installed = the SCM service exists.
    let scm = unsafe {
        OpenSCManagerA(std::ptr::null(), std::ptr::null(), SC_MANAGER_CONNECT)
    };
    if scm.is_null() {
        return (false, false, None, "SCM is not accessible".into());
    }
    let name = b"RDA\0";
    let svc = unsafe { OpenServiceA(scm, name.as_ptr(), SERVICE_QUERY_STATUS) };
    unsafe { CloseServiceHandle(scm) };
    if svc.is_null() {
        return (false, false, None, format!("the {RDA_SERVICE} service is not installed"));
    }
    let mut status = SERVICE_STATUS {
        dwServiceType: 0,
        dwCurrentState: 0,
        dwControlsAccepted: 0,
        dwWin32ExitCode: 0,
        dwServiceSpecificExitCode: 0,
        dwCheckPoint: 0,
        dwWaitHint: 0,
    };
    let queried = unsafe { QueryServiceStatus(svc, &mut status) } != 0;
    unsafe { CloseServiceHandle(svc) };
    let service_running = queried && status.dwCurrentState == SERVICE_RUNNING;

    // running = service running AND the device opens.
    let wide: Vec<u16> = RDA_DEVICE.encode_utf16().chain([0]).collect();
    let dev = unsafe {
        CreateFileW(
            wide.as_ptr(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            std::ptr::null(),
            OPEN_EXISTING,
            0,
            std::ptr::null_mut(),
        )
    };
    if dev == INVALID_HANDLE_VALUE {
        let note = if service_running {
            format!("service is running but {RDA_DEVICE} does not open (driver not loaded — it is required for every scan; {DRIVER_REQUIRED_NOTE})")
        } else {
            format!("service is stopped; the driver is required for every scan; {DRIVER_REQUIRED_NOTE}")
        };
        return (true, false, None, note);
    }
    let mut version: u64 = 0;
    let mut returned: u32 = 0;
    let _ = unsafe {
        DeviceIoControl(
            dev,
            IOCTL_RDA_GET_VERSION,
            std::ptr::null(),
            0,
            &mut version as *mut u64 as *mut core::ffi::c_void,
            8,
            &mut returned,
            std::ptr::null_mut(),
        )
    };
    unsafe { CloseHandle(dev) };
    let version = if returned >= 8 {
        Some(format!("{}.{}", (version >> 32) & 0xFFFF_FFFF, version & 0xFFFF_FFFF))
    } else {
        None
    };
    (true, true, version, format!("running; development test-signed build; {DRIVER_REQUIRED_NOTE}"))
}

#[cfg(not(windows))]
fn driver_probe() -> (bool, bool, Option<String>, String) {
    (false, false, None, "the raw-disk driver ships Windows-only".into())
}

#[tauri::command]
pub async fn system_driver_status() -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(|| {
        let (installed, running, version, note) = driver_probe();
        Ok(json!({
            "installed": installed,
            "version": version,
            "running": running,
            "note": note,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

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
        // G-6: no cr_ui export carries an engine version string (the ABI
        // number 2 is a contract number, not a product version) — null.
        "engineVersion": Value::Null,
        "platform": std::env::consts::OS,
    }))
}

// ---------------------------------------------------------------------------
// Destination validation + mounts
// ---------------------------------------------------------------------------

/// Volume letter -> (free, total) via GetDiskFreeSpaceExW.
#[cfg(windows)]
fn letter_free_total(letter: u8) -> Option<(u64, u64)> {
    use windows_sys::Win32::Storage::FileSystem::GetDiskFreeSpaceExW;
    let root: Vec<u16> = format!("{}:\\", letter as char).encode_utf16().chain([0]).collect();
    let mut free: u64 = 0;
    let mut total: u64 = 0;
    let mut _avail: u64 = 0;
    let ok = unsafe { GetDiskFreeSpaceExW(root.as_ptr(), &mut free, &mut total, &mut _avail) };
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
            return Ok(destination_result(Some("out-of-space"), Some(free), needed_bytes));
        }
        // Real write probe: create + delete a temp file in the directory.
        let dir = std::path::PathBuf::from(path.trim());
        let probe = dir.join(format!(".rw-probe-{}", std::process::id()));
        match std::fs::File::create(&probe) {
            Ok(_) => {
                let _ = std::fs::remove_file(&probe);
                Ok(destination_result(None, Some(free), needed_bytes))
            }
            Err(_) => Ok(destination_result(Some("unwritable"), Some(free), needed_bytes)),
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
pub async fn system_open_path(
    app: tauri::AppHandle,
    path: String,
) -> Result<(), BridgeError> {
    // reveal_in_explorer absorbed here (plan §4.7): files are REVEALED with
    // selection in the system explorer; folders open directly.
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
