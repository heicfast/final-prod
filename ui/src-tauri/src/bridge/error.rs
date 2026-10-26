//! bridge/error.rs — the error the bridge returns to the TS connector.
//!
//! Shape contract (contracts/error-codes.json + docs/INTEGRATION-PLAN.md §4.6):
//! every command returns `Result<T, BridgeError>` serialized as
//! `{ code: number, bridgeCode: string | null, message: string }`.
//! - `code` carries the ENGINE numeric space (cr_error.h) when the engine
//!   produced the failure; 0 when the failure is bridge-generated.
//! - `bridgeCode` carries the UI vocabulary (ConnectorErrorCode) when the
//!   bridge itself classified the failure (scan-not-found, scan-scope-invalid,
//!   preview-*, destination-*, the gap methods' not-implemented ...).
//! - The TS connector maps `bridgeCode` directly and `code` through the
//!   error-codes table; the UI renders reasonKey i18n only, never messages.
//!
//! ERR_CANCELLED (0x50020005) is NEVER an error here: the commands that can
//! observe it translate it to scan/recovery status 'cancelled' per
//! contracts/error-codes.json ("the UI's cancel paths resolve, not reject").

use serde::Serialize;

use super::abi;

/// Bridge-classified failure codes (ConnectorErrorCode members the bridge
/// mints itself; the engine code stays 0 for these).
pub mod code {
    pub const SCAN_NOT_FOUND: &str = "scan-not-found";
    pub const SESSION_NOT_FOUND: &str = "session-not-found";
    pub const SCAN_NOT_RUNNING: &str = "scan-not-running";
    pub const SCAN_NOT_PAUSED: &str = "scan-not-paused";
    pub const SCAN_SCOPE_INVALID: &str = "scan-scope-invalid";
    pub const DESTINATION_INVALID: &str = "destination-invalid";
    pub const DEVICE_GONE: &str = "device-gone";
    pub const DEVICE_PERMISSION: &str = "device-permission";
    pub const PREVIEW_TOO_LARGE: &str = "preview-too-large";
    pub const PREVIEW_MISSING_DATA: &str = "preview-missing-data";
    pub const PREVIEW_UNSUPPORTED: &str = "preview-unsupported";
    pub const PREVIEW_DAMAGED: &str = "preview-damaged";
    pub const NOT_IMPLEMENTED: &str = "not-implemented";
    pub const HEALTH_UNAVAILABLE: &str = "health-unavailable";
    pub const SESSION_CORRUPT: &str = "session-corrupt";
    pub const UNKNOWN: &str = "unknown";
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct BridgeError {
    /// Engine numeric code (cr_error.h space); 0 when bridge-generated.
    pub code: u32,
    /// UI ConnectorErrorCode when the bridge classified the failure itself.
    pub bridge_code: Option<&'static str>,
    /// Human-readable diagnostic (logged; the UI renders reasonKey, not this).
    pub message: String,
}

impl std::fmt::Display for BridgeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.message)?;
        if let Some(bc) = self.bridge_code {
            write!(f, " [bridge: {bc}]")?;
        }
        Ok(())
    }
}

impl std::error::Error for BridgeError {}

impl BridgeError {
    /// A failure the engine produced: capture its numeric code + the
    /// thread-local diagnostic text.
    pub fn engine(code: u32, context: &str) -> Self {
        let last = unsafe {
            let p = abi::cr_ui_last_error();
            if p.is_null() {
                String::new()
            } else {
                crate::bridge::util::cstr_to_string_lossy(p)
            }
        };
        BridgeError {
            code,
            bridge_code: None,
            message: format!("{context}: engine error {code:#010x}: {last}"),
        }
    }

    /// A failure the bridge classified itself (engine code 0).
    pub fn bridge(bridge_code: &'static str, message: impl Into<String>) -> Self {
        BridgeError {
            code: 0,
            bridge_code: Some(bridge_code),
            message: message.into(),
        }
    }

    /// The typed not-implemented refusal used by EXACTLY the three gap
    /// methods (repair dry-run / undo-log / undo) — contracts/error-codes.json.
    pub fn gap(surface: &str) -> Self {
        BridgeError::bridge(
            code::NOT_IMPLEMENTED,
            format!(
                "{surface}: the current engine ABI exposes no standalone repair surface \
                 (docs/INTEGRATION-GAPS.md G-1). Pre-scan repair analysis runs read-only \
                 during scans (default-deny: nothing is ever written without a consent \
                 surface); standalone repair is CLI-only today (rrecov fix)."
            ),
        )
    }

    /// Map an engine code with no UI-visible meaning onto the truthful
    /// fallback (unknown) while keeping the numeric code in the payload.
    pub fn engine_unknown(code: u32, context: &str) -> Self {
        let mut e = BridgeError::engine(code, context);
        e.bridge_code = Some(code::UNKNOWN);
        e
    }
}

pub type BridgeResult<T> = Result<T, BridgeError>;

/// Check an engine return code; `context` names the ABI call for the log.
pub fn check(rc: u32, context: &str) -> BridgeResult<()> {
    if rc == abi::CR_UI_OK {
        Ok(())
    } else {
        Err(BridgeError::engine(rc, context))
    }
}
