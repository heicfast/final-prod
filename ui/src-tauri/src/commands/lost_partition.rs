//! commands/lost_partition.rs — the S-029 apply surface (consent-gated).
//!
//! `lost_partition_apply` transcribes a scan-state candidate (the N-061
//! geometry rows surfaced by scan_state.lostCandidates — apply never
//! invents geometry) into `cr_ui_apply_candidate`, opens a WHOLE-DISK view,
//! and runs `cr_ui_apply_lost_partition` with the consent callback armed
//! (bridge::consent::apply_consent_sink — the same gate design as the
//! pre-scan repair family, §4.5). The plan's refusal, the consent prompt
//! and the outcome all cross verbatim: `apply_result.outcome` 0 applied,
//! 1 consent denied, 2 refused, 3 table changed since plan, 4 applied but
//! the refresh tail failed (undo valid), 5 error.
//!
//! Single-flight per device: a second apply while a consent pends is the
//! typed repair-consent-required refusal (never a second concurrent write
//! planner on the same disk).
//!
//! `apply_consent_pending` is the re-hydration query (the scan consent's
//! scan_state.pendingConsent twin): the lost-partition screen re-renders an
//! interrupted prompt from it after navigating away and back.

use std::sync::Arc;

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::consent::ApplyConsentPipe;
use crate::bridge::error::{check, code, BridgeError, BridgeResult};
use crate::bridge::state::BridgeState;
use crate::bridge::util::{cstr, fixed_str};

fn disk_number(device_id: &str) -> BridgeResult<u32> {
    device_id
        .strip_prefix("disk-")
        .and_then(|r| r.parse().ok())
        .ok_or_else(|| {
            BridgeError::bridge(
                code::DEVICE_GONE,
                format!("'{device_id}' is not a whole-disk id (expected disk-N)"),
            )
        })
}

fn apply_result_json(r: &abi::CrUiApplyResult) -> Value {
    json!({
        "applied": r.applied != 0,
        "outcome": r.outcome,
        "undoEntries": r.undo_entries,
        "reason": fixed_str(&r.reason),
    })
}

#[tauri::command]
pub async fn lost_partition_apply(
    app: tauri::AppHandle,
    state: tauri::State<'_, Arc<BridgeState>>,
    device_id: String,
    candidate: Value,
    rebuild_table: Option<bool>,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        // The candidate row (scan-state geometry — lba/sectors/fs required;
        // apply transcribes, never invents).
        let lba = candidate
            .get("lba")
            .and_then(|v| v.as_u64())
            .ok_or_else(|| {
                BridgeError::bridge(code::SCAN_SCOPE_INVALID, "the candidate carries no lba")
            })?;
        let sectors = candidate
            .get("sectorCount")
            .and_then(|v| v.as_u64())
            .ok_or_else(|| {
                BridgeError::bridge(
                    code::SCAN_SCOPE_INVALID,
                    "the candidate carries no sector count (rescan the disk — \
                     apply needs the full N-061 geometry)",
                )
            })?;
        let fs = candidate.get("fs").and_then(|v| v.as_u64()).unwrap_or(5) as u32;
        if sectors == 0 {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                "the candidate's sector count is zero — not applicable",
            ));
        }

        // Single-flight per device.
        {
            let mut in_flight = state
                .apply_in_flight
                .lock()
                .map_err(|_| BridgeError::bridge(code::UNKNOWN, "apply registry lock"))?;
            if in_flight.contains(&device_id) {
                return Err(BridgeError::bridge(
                    code::REPAIR_CONSENT_REQUIRED,
                    "an apply is already pending on this device (answer its consent prompt first)",
                ));
            }
            in_flight.insert(device_id.clone());
        }
        // Teardown guarantee: whatever happens below, the in-flight marker
        // leaves and the device's pending gates close before returning.
        let outcome = lost_partition_apply_blocking(
            &state,
            &app,
            &device_id,
            lba,
            sectors,
            fs,
            rebuild_table.unwrap_or(false),
        );
        if let Ok(mut in_flight) = state.apply_in_flight.lock() {
            in_flight.remove(&device_id);
        }
        state.close_device_consents(&device_id);
        outcome
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

fn lost_partition_apply_blocking(
    state: &Arc<BridgeState>,
    app: &tauri::AppHandle,
    device_id: &str,
    lba: u64,
    sectors: u64,
    fs: u32,
    rebuild_table: bool,
) -> BridgeResult<Value> {
    let disk = disk_number(device_id)?;

    // M-137 presentation tail from fresh enumeration (the planner never
    // reads these; they are the listing's row context).
    let (disk_layout, disk_flags) = {
        let disks = crate::bridge::util::enumerate_disks()?;
        match disks.iter().find(|d| d.number == disk) {
            Some(d) => (d.layout, u32::from(d.table_ok != 0) << 1),
            None => (0, 0),
        }
    };
    let cand = abi::CrUiApplyCandidate {
        start_lba: lba,
        sector_count: sectors,
        fs,
        disk_layout,
        disk_flags,
    };

    // Whole-disk view (volume slices are refused by the engine — the table
    // lives at LBA 0).
    let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
    let open_rc = unsafe { abi::cr_ui_open_disk(disk, &mut dev) };
    if open_rc == abi::CR_UI_ERR_ELEVATION_REQUIRED {
        return Err(BridgeError::engine(
            open_rc,
            "cr_ui_open_disk (elevation required)",
        ));
    }
    check(open_rc, "cr_ui_open_disk")?;

    let pipe = Box::into_raw(Box::new(ApplyConsentPipe {
        device_id: device_id.to_string(),
        app: app.clone(),
        state: state.clone(),
    }));
    let mut result: abi::CrUiApplyResult = unsafe { std::mem::zeroed() };
    let rc = unsafe {
        abi::cr_ui_apply_lost_partition(
            dev,
            &cand,
            u32::from(rebuild_table),
            Some(crate::bridge::consent::apply_consent_sink),
            pipe as *mut std::ffi::c_void,
            &mut result,
        )
    };
    unsafe { drop(Box::from_raw(pipe)) };
    unsafe { abi::cr_ui_device_close(dev) };
    check(rc, "cr_ui_apply_lost_partition")?;
    crate::bridge::logging::info(&format!(
        "lost-partition apply on {device_id}: lba={lba} outcome={} undo_entries={}",
        result.outcome, result.undo_entries
    ));
    Ok(apply_result_json(&result))
}

/// Restore the last apply's undo log on this device (byte-exact sector
/// restore + properties refresh; idempotent on bytes).
#[tauri::command]
pub async fn lost_partition_undo(
    state: tauri::State<'_, Arc<BridgeState>>,
    device_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let disk = disk_number(&device_id)?;
        let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
        check(
            unsafe { abi::cr_ui_open_disk(disk, &mut dev) },
            "cr_ui_open_disk",
        )?;
        let mut result: abi::CrUiApplyResult = unsafe { std::mem::zeroed() };
        let rc = unsafe { abi::cr_ui_apply_undo_last(dev, &mut result) };
        unsafe { abi::cr_ui_device_close(dev) };
        check(rc, "cr_ui_apply_undo_last")?;
        Ok(apply_result_json(&result))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// Persist the last apply's undo log (the durable CRC-checked container,
/// loadable by `rrecov fix --undo-apply`).
#[tauri::command]
pub async fn lost_partition_save_undo(
    state: tauri::State<'_, Arc<BridgeState>>,
    device_id: String,
    path: String,
) -> Result<(), BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let disk = disk_number(&device_id)?;
        let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
        check(
            unsafe { abi::cr_ui_open_disk(disk, &mut dev) },
            "cr_ui_open_disk",
        )?;
        let path_c = cstr(&path);
        let rc = unsafe { abi::cr_ui_apply_save_undo(dev, path_c.as_ptr()) };
        unsafe { abi::cr_ui_device_close(dev) };
        check(rc, "cr_ui_apply_save_undo")
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// The pending APPLY consent payload for a device (null when none) — the
/// lost-partition screen's re-hydration query (the scan consent's
/// scan_state.pendingConsent twin). Registry-only, no engine call.
#[tauri::command]
pub async fn apply_consent_pending(
    state: tauri::State<'_, Arc<BridgeState>>,
    device_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        Ok(state
            .pending_apply_consent(&device_id)
            .unwrap_or(Value::Null))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
