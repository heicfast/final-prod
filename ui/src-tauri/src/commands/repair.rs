//! commands/repair.rs — the G-1 gap cluster. The 78-export ABI exposes NO
//! standalone repair surface (RawFixer analyze/execute/undo live behind the
//! C++ core API and cross the ABI only as pre-scan repair inside a scan).
//! All four methods throw the typed not-implemented with the truthful scope
//! and follow-up pointer; the UI surfaces render the message, never a dead
//! button.
//!
//! docs/INTEGRATION-GAPS.md G-1 records the proposed append-only ABI
//! additions and the CLI-only undo reality (rrecov fix --undo-apply).

use serde_json::Value;

use crate::bridge::error::{code, BridgeError};

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
         read-only pre-scan analysis, default-deny without a consent surface)",
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
