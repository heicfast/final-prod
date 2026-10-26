//! commands/repair.rs — the G-1 cluster: the consent surface + the gap trio.
//!
//! The REAL repair path is the scan's pre-scan analysis: `scan_start` arms
//! `pre_scan_repair = 1` WITH the consent callback (bridge::consent — the
//! G-1 follow-up (a) surface that re-added `consent_respond`). The callback
//! fires at most ONCE per scan (cr_ui.h S-008), the scan page renders the
//! typed prompt from `consent-request:<scanId>` / scan-state re-hydration,
//! and this command is the ONLY way a decision crosses back.
//!
//! The standalone workbench trio (dryRunRepair / getUndoLog / undoRepairs)
//! remains the typed not-implemented: the 78-export ABI exposes no
//! standalone repair surface (RawFixer analyze/execute/undo cross the ABI
//! only as pre-scan repair inside a scan). The UI renders the truthful
//! message, never a dead button; docs/INTEGRATION-GAPS.md G-1 records the
//! proposed append-only ABI additions and the CLI-only undo reality
//! (`rrecov fix --undo-apply <file>` — the bridge saves the undo log after
//! an approved repair lands).

use std::sync::Arc;

use serde_json::Value;

use crate::bridge::consent::consent_respond_impl;
use crate::bridge::error::{code, BridgeError};
use crate::bridge::state::BridgeState;

/// The consent answer (plan §4.3/§4.5; re-added per G-1 (a)). `requestId`
/// is the echo token from the consent-request payload — never guessable
/// into authority. Unknown/expired/double-responded ids throw the typed
/// consent-not-found.
#[tauri::command]
pub async fn consent_respond(
    state: tauri::State<'_, Arc<BridgeState>>,
    request_id: String,
    allow: bool,
) -> Result<(), BridgeError> {
    consent_respond_impl(state.inner(), &request_id, allow)
}

#[tauri::command]
pub async fn repair_dry_run(_device_id: String) -> Result<Value, BridgeError> {
    Err(BridgeError::gap("repair.dryRunRepair (standalone repair workbench)"))
}

#[tauri::command]
pub async fn repair_apply(
    _device_id: String,
    _options: Value,
) -> Result<Value, BridgeError> {
    Err(BridgeError::gap(
        "repair.applyRepair (standalone apply; the real repair path is the scan's \
         pre-scan analysis — the consent prompt inside the scan)",
    ))
}

#[tauri::command]
pub async fn repair_undo_log(_device_id: String) -> Result<Value, BridgeError> {
    Err(BridgeError::gap("repair.getUndoLog (undo-log reading)"))
}

#[tauri::command]
pub async fn repair_undo(_device_id: String) -> Result<Value, BridgeError> {
    Err(BridgeError::gap(
        "repair.undoRepairs (undo is CLI-only today: rrecov fix --undo-apply <file>)",
    ))
}

// Silence the unused-code lint on the module (code is referenced by name in
// the manifest checks; keep the import explicit for readability).
#[allow(unused)]
fn _vocab() -> &'static str {
    code::NOT_IMPLEMENTED
}
