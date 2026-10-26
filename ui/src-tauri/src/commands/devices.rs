//! commands/devices.rs — devices_list / devices_refresh (serves listDevices,
//! getDeviceInfo, refreshDevices; contracts/command-surface.json).
//!
//! Field maps: contracts/type-maps.md §1. Usage bytes come from Win32
//! GetDiskFreeSpaceExW per lettered volume (the engine reports totals only);
//! unlettered volumes contribute zero usage. Status: fresh open probe —
//! OK → 'ok'; 0x50020001 → 'permission-denied'; 0x50060001 on a volume →
//! 'locked'. quickScanAvailable is always true (G-9/G-10: no per-volume gate
//! exists in the engine; the mock's flip was simulation-only). Lost-partition
//! hints are absent pre-scan (G-10: scan-time events).

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, BridgeError, BridgeResult};
use crate::bridge::state::BridgeState;
use crate::bridge::util::{enumerate_disks, enumerate_volumes, fixed_str};

/// The UI-visible device id for a disk number.
pub fn disk_id(number: u32) -> String {
    format!("disk-{number}")
}

/// The UI-visible volume id: `vol-<L>` for lettered volumes,
/// `vol-<disk>-<offset>` for unlettered ones (type-maps §1).
pub fn volume_id(v: &abi::CrUiVolumeInfo) -> String {
    if v.letter != 0 {
        format!("vol-{}", v.letter as char)
    } else {
        format!("vol-{}-{}", v.disk, v.offset_on_disk)
    }
}

/// Volume FS-name table (type-maps §1): engine strings are Windows FS names;
/// the map is a table, not display-text matching. 'raw' (carve-only) never
/// appears from enumeration — volumes enumerate with their probed FS.
fn map_fs(name: &str) -> &'static str {
    match name.to_ascii_lowercase().as_str() {
        "ntfs" => "ntfs",
        "fat32" => "fat32",
        "exfat" => "exfat",
        "ext4" => "ext4",
        "btrfs" => "btrfs",
        "refs" => "refs",
        "hfs+" => "hfsplus",
        "zfs" => "zfs",
        _ => "unknown",
    }
}

/// Win32 per-letter usage (cfg-gated; the engine reports totals only).
#[cfg(windows)]
fn letter_usage(letter: u8) -> Option<(u64, u64)> {
    use windows_sys::Win32::Storage::FileSystem::GetDiskFreeSpaceExW;
    let root: Vec<u16> = format!("{}:\\", letter as char).encode_utf16().chain([0]).collect();
    let mut free: u64 = 0;
    let mut total: u64 = 0;
    let mut _avail: u64 = 0;
    let ok = unsafe {
        GetDiskFreeSpaceExW(root.as_ptr(), &mut free as *mut u64, &mut total as *mut u64,
            &mut _avail as *mut u64)
    };
    if ok == 0 {
        return None;
    }
    let used = total.saturating_sub(free);
    Some((used, free))
}

#[cfg(not(windows))]
fn letter_usage(_letter: u8) -> Option<(u64, u64)> {
    None
}

/// Probe a disk's openability (status row): open OK → 'ok'; open failure →
/// 'permission-denied' (refined by the caller when enumeration dropped it).
fn probe_status(rc_open: u32) -> &'static str {
    if rc_open == abi::CR_UI_OK {
        "ok"
    } else {
        "permission-denied"
    }
}

/// BitLocker tri-state (type-maps §1): locked when the probe refused with
/// 0x50060001; unlocked when the probe opened; false when not FVE-detected.
fn bitlocker_state(detected: u32, probe_rc: u32) -> Value {
    if detected == 0 {
        return json!(false);
    }
    if probe_rc == abi::CR_UI_ERR_FVE_LOCKED {
        json!("locked")
    } else {
        json!("unlocked")
    }
}

/// Compose the contract-shaped Device rows. `active_sources` feeds
/// `isSourceOfActiveScan` from the bridge's scan registry.
pub fn list_devices(state: &BridgeState) -> BridgeResult<Vec<Value>> {
    let disks = enumerate_disks()?;
    let volumes = enumerate_volumes()?;
    let active: Vec<(String, Option<String>)> = state
        .scans
        .lock()
        .map(|g| {
            g.values()
                .filter(|e| !e.is_terminal())
                .map(|e| (e.device_id.clone(), e.volume_id.clone()))
                .collect()
        })
        .unwrap_or_default();

    let mut out = Vec::with_capacity(disks.len());
    for d in &disks {
        let model = fixed_str(&d.model);
        let serial = fixed_str(&d.serial);
        let disk_vols: Vec<&abi::CrUiVolumeInfo> =
            volumes.iter().filter(|v| v.disk == d.number).collect();

        // Fresh open probe — closed immediately; failure refines status.
        let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
        let rc = unsafe { abi::cr_ui_open_disk(d.number, &mut dev) };
        if !dev.is_null() {
            unsafe { abi::cr_ui_device_close(dev) };
        }
        let status = probe_status(rc);

        // Usage: per-lettered volume sums (type-maps §1).
        let mut used = 0u64;
        let mut free = 0u64;
        let mut vol_rows = Vec::with_capacity(disk_vols.len());
        for v in &disk_vols {
            let (v_used, v_free) = if v.letter != 0 {
                letter_usage(v.letter).unwrap_or((0, 0))
            } else {
                (0, 0)
            };
            used += v_used;
            free += v_free;
            // Locked-volume probe only when FVE was detected (cheap path).
            let bl_probe = if v.bitlocker != 0 && v.letter != 0 {
                let mut vd: *mut abi::CrUiDevice = std::ptr::null_mut();
                let prc = unsafe { abi::cr_ui_open_volume(v.letter as i8, &mut vd) };
                if !vd.is_null() {
                    unsafe { abi::cr_ui_device_close(vd) };
                }
                prc
            } else {
                abi::CR_UI_OK
            };
            let v_status = if bl_probe == abi::CR_UI_ERR_FVE_LOCKED {
                "locked"
            } else if v.bitlocker != 0 && bl_probe != abi::CR_UI_OK {
                "permission-denied"
            } else {
                "ok"
            };
            let label = fixed_str(&v.label);
            vol_rows.push(json!({
                "id": volume_id(v),
                "deviceId": disk_id(d.number),
                "label": label,
                "letter": (if v.letter != 0 { json!(format!("{}:", v.letter as char)) } else { Value::Null }),
                "fs": map_fs(&fixed_str(&v.fs_name)),
                "capacity": {
                    "totalBytes": v.bytes,
                    "usedBytes": v_used,
                    "freeBytes": v_free,
                },
                "status": v_status,
                // G-10: lost candidates are scan-time events — no pre-scan hint.
                "quickScanAvailable": true,
                "sectorSize": d.logical_sector,
                "bitlocker": bitlocker_state(v.bitlocker, bl_probe),
            }));
        }

        let primary = disk_vols
            .iter()
            .find(|v| v.letter != 0)
            .or_else(|| disk_vols.first());
        let label = match primary {
            Some(v) if v.letter != 0 => {
                let l = fixed_str(&v.label);
                if l.is_empty() {
                    format!("Local Disk ({}:)", v.letter as char)
                } else {
                    format!("{} ({}:)", l, v.letter as char)
                }
            }
            _ if !model.is_empty() => model.clone(),
            _ => format!("Hard Disk #{}", d.number),
        };
        let is_source = active
            .iter()
            .any(|(dev_id, vol_id)| {
                dev_id == &disk_id(d.number)
                    || vol_id
                        .as_deref()
                        .is_some_and(|v| disk_vols.iter().any(|dv| volume_id(dv) == v))
            });
        out.push(json!({
            "id": disk_id(d.number),
            // G-9: the engine struct has no bus type — removable is the only
            // signal; 'sd'/'optical' are indistinguishable (INTEGRATION-GAPS).
            "kind": (if d.removable != 0 { "usb" } else { "internal" }),
            "model": (if model.is_empty() { format!("Hard Disk #{}", d.number) } else { model }),
            "label": label,
            "serial": serial,
            "capacity": {
                "totalBytes": d.size_bytes,
                "usedBytes": used,
                "freeBytes": free,
            },
            "status": status,
            "isSourceOfActiveScan": is_source,
            "volumes": vol_rows,
            // Whole-disk entry point exists whenever a disk view opens.
            "canScanWholeDisk": true,
        }));
    }
    Ok(out)
}

/// Resolved scan target: everything scan_start needs from enumeration.
pub struct ResolvedTarget {
    pub disk: abi::CrUiDiskInfo,
    pub volume: Option<abi::CrUiVolumeInfo>,
    pub device_json: Value,
    pub volume_json: Option<Value>,
    /// Implicit pathScope for the virtual Home quick-targets.
    pub implicit_scope: Option<String>,
}

/// Resolve a UI targetId to its enumeration rows (the same shapes
/// listDevices produced — getScanState echoes them back).
pub fn resolve_target(target_id: &str) -> BridgeResult<ResolvedTarget> {
    let disks = enumerate_disks()?;
    let volumes = enumerate_volumes()?;

    // Virtual Home quick-targets (UI §3.4): mapped to the system volume with
    // an implicit pathScope — plan §3.1.
    let (want_volume_letter, implicit_scope): (Option<u8>, Option<String>) = match target_id {
        "vol-recycle-bin" => (
            system_drive_letter(),
            Some(format!("{}:\\$Recycle.Bin", system_drive_char())),
        ),
        "vol-desktop" => (system_drive_letter(), Some(desktop_scope())),
        _ => (None, None),
    };

    let (disk_ref, vol_ref): (&abi::CrUiDiskInfo, Option<&abi::CrUiVolumeInfo>) =
        if let Some(letter) = want_volume_letter {
            let v = volumes.iter().find(|v| v.letter == letter).ok_or_else(|| {
                BridgeError::bridge(
                    code::DEVICE_GONE,
                    format!("the system volume ({letter}:) is not present"),
                )
            })?;
            let d = disks
                .iter()
                .find(|d| d.number == v.disk)
                .ok_or_else(|| BridgeError::bridge(code::DEVICE_GONE, "owning disk not present"))?;
            (d, Some(v))
        } else if let Some(rest) = target_id.strip_prefix("disk-") {
            let n: u32 = rest.parse().map_err(|_| {
                BridgeError::bridge(code::DEVICE_GONE, format!("bad device id {target_id}"))
            })?;
            let d = disks.iter().find(|d| d.number == n).ok_or_else(|| {
                BridgeError::bridge(
                    code::DEVICE_GONE,
                    format!("device {target_id} is not present (removed or renumbered)"),
                )
            })?;
            (d, None)
        } else if let Some(rest) = target_id.strip_prefix("vol-") {
            let v = if rest.len() == 1 {
                let letter = rest.as_bytes()[0];
                volumes.iter().find(|v| v.letter == letter)
            } else {
                // vol-<disk>-<offset> (unlettered) — exact id match
                volumes.iter().find(|v| volume_id(v) == target_id)
            };
            let v = v.ok_or_else(|| {
                BridgeError::bridge(
                    code::DEVICE_GONE,
                    format!("volume {target_id} is not present (removed or renumbered)"),
                )
            })?;
            let d = disks
                .iter()
                .find(|d| d.number == v.disk)
                .ok_or_else(|| BridgeError::bridge(code::DEVICE_GONE, "owning disk not present"))?;
            (d, Some(v))
        } else {
            return Err(BridgeError::bridge(
                code::DEVICE_GONE,
                format!("target {target_id} is not a device or volume id"),
            ));
        };

    // Reuse the row composer for the exact Device/Volume JSON the UI saw.
    let device_json = compose_single_disk(disk_ref, &volumes)?;
    let volume_json = vol_ref.map(|v| compose_volume_row(v, disk_ref));
    Ok(ResolvedTarget {
        disk: *disk_ref,
        volume: vol_ref.copied(),
        device_json,
        volume_json,
        implicit_scope,
    })
}

fn system_drive_char() -> char {
    std::env::var("SystemDrive")
        .ok()
        .and_then(|s| s.chars().next())
        .unwrap_or('C')
}

fn system_drive_letter() -> Option<u8> {
    std::env::var("SystemDrive")
        .ok()
        .and_then(|s| s.chars().next())
        .map(|c| c.to_ascii_uppercase() as u8)
        .or(Some(b'C'))
}

fn desktop_scope() -> String {
    std::env::var("USERPROFILE")
        .map(|p| format!("{p}\\Desktop"))
        .unwrap_or_else(|_| "C:\\Users\\Public\\Desktop".to_string())
}

/// Compose ONE Device row (resolve_target reuses listDevices' shapes without
/// probing every other disk — the target row only).
fn compose_single_disk(
    d: &abi::CrUiDiskInfo,
    volumes: &[abi::CrUiVolumeInfo],
) -> BridgeResult<Value> {
    let model = fixed_str(&d.model);
    let serial = fixed_str(&d.serial);
    let disk_vols: Vec<&abi::CrUiVolumeInfo> =
        volumes.iter().filter(|v| v.disk == d.number).collect();
    let mut used = 0u64;
    let mut free = 0u64;
    let mut vol_rows = Vec::with_capacity(disk_vols.len());
    for v in &disk_vols {
        let (v_used, v_free) = if v.letter != 0 {
            letter_usage(v.letter).unwrap_or((0, 0))
        } else {
            (0, 0)
        };
        used += v_used;
        free += v_free;
        vol_rows.push(compose_volume_row(v, d));
    }
    let primary = disk_vols.iter().find(|v| v.letter != 0);
    let label = match primary {
        Some(v) => {
            let l = fixed_str(&v.label);
            if l.is_empty() {
                format!("Local Disk ({}:)", v.letter as char)
            } else {
                format!("{} ({}:)", l, v.letter as char)
            }
        }
        None if !model.is_empty() => model.clone(),
        None => format!("Hard Disk #{}", d.number),
    };
    Ok(json!({
        "id": disk_id(d.number),
        "kind": (if d.removable != 0 { "usb" } else { "internal" }),
        "model": (if model.is_empty() { format!("Hard Disk #{}", d.number) } else { model }),
        "label": label,
        "serial": serial,
        "capacity": {
            "totalBytes": d.size_bytes,
            "usedBytes": used,
            "freeBytes": free,
        },
        "status": "ok",
        "isSourceOfActiveScan": true, // it is about to be
        "volumes": vol_rows,
        "canScanWholeDisk": true,
    }))
}

fn compose_volume_row(v: &abi::CrUiVolumeInfo, d: &abi::CrUiDiskInfo) -> Value {
    let (v_used, v_free) = if v.letter != 0 {
        letter_usage(v.letter).unwrap_or((0, 0))
    } else {
        (0, 0)
    };
    json!({
        "id": volume_id(v),
        "deviceId": disk_id(d.number),
        "label": fixed_str(&v.label),
        "letter": (if v.letter != 0 { json!(format!("{}:", v.letter as char)) } else { Value::Null }),
        "fs": map_fs(&fixed_str(&v.fs_name)),
        "capacity": {
            "totalBytes": v.bytes,
            "usedBytes": v_used,
            "freeBytes": v_free,
        },
        "status": "ok",
        "quickScanAvailable": true,
        "sectorSize": d.logical_sector,
        "bitlocker": (if v.bitlocker != 0 { json!("unlocked") } else { json!(false) }),
    })
}

// ---------------------------------------------------------------------------
// Tauri commands
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn devices_list(
    state: tauri::State<'_, std::sync::Arc<BridgeState>>,
) -> Result<Vec<Value>, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || list_devices(&state))
        .await
        .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn devices_refresh(
    state: tauri::State<'_, std::sync::Arc<BridgeState>>,
) -> Result<Vec<Value>, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || list_devices(&state))
        .await
        .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
