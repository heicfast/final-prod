//! commands/maintenance.rs — the privileged one-shot verbs
//! (contracts/command-surface.json; type-maps §13).
//!
//! Both commands are §4.7 DISABLE-register rows the corpus contradicts:
//!
//! - `volume_fsck` (row 20) — the check-phase escalation endpoint (M-075,
//!   the diff-to-fix lineage): launch Windows chkdsk on the volume at
//!   `letter`. The og's FileSystemCheck ladder (FileSystemCheck.mo
//!   strings:126) is the corpus lineage; the in-scan pre_scan_check audit
//!   (already surfaced through scan stats) is the read-only rung, this is
//!   the check/fix rung. The engine's spawn vehicle is the D-CONS-01
//!   hidden-subprocess discipline (CREATE_NO_WINDOW + piped stdio,
//!   engine/src/win/cr_fsck.cpp:51) — no console ever reaches the user's
//!   desktop. Bounded: the engine's internal synchronous bound is 15
//!   minutes; on timeout the exit code is STILL_ACTIVE (259) and the
//!   process is left running — both cross to the UI verbatim.
//! - `volume_bitlocker_off` (row 21) — the CBitlockerTurnOffOperation
//!   endpoint (M-078): the DESTRUCTIVE decrypt twin of the unlock
//!   surface. Explicit opt-in only — nothing in the scan/open path ever
//!   calls it (D-16's legitimate-unlock-only register is untouched: this
//!   is decrypt, not bypass). OK means ACCEPTED, not plaintext: the OS
//!   decrypt continues in the background after the request is accepted
//!   (manage-bde's documented shape) — the progress callback fires at
//!   request start and at the command's settle only (the honest
//!   unobservable-percentage contract, cr_ui.h:1634-1650). The UI gates
//!   this behind an explicit confirmation dialog; the bridge surfaces
//!   the engine's refusal vocabulary truthfully (ERR_FVE_LOCKED when
//!   both command forms failed).
//!
//! Threading: both verbs are synchronous-on-caller-thread engine calls
//! with minutes-scale bounds — spawn-join on the blocking pool, exactly
//! like the recovery start path. No registry, no events: the invoke
//! promise IS the lifecycle (fsck's 15-minute bound is documented; the
//! UI renders a bounded-progress posture from the documented bound).

use serde_json::json;

use crate::bridge::abi;
use crate::bridge::error::{check, code, BridgeError};

/// Parse the UI's volume argument ("E" / "e:" / "E:\\") to the engine's
/// narrow-ASCII letter. The typed refusal for anything else matches the
/// engine's own ERR_BAD_PARAMETER posture — but with the bridge's
/// clearer message.
fn volume_letter(letter: &str) -> BridgeResult<i8> {
    let b = letter.trim().as_bytes();
    if b.len() >= 1 && b[0].is_ascii_alphabetic() {
        Ok(b[0].to_ascii_uppercase() as i8)
    } else {
        Err(BridgeError::bridge(
            code::SCAN_SCOPE_INVALID,
            format!("volume '{letter}' is not a drive letter (A..Z)"),
        ))
    }
}

// ---------------------------------------------------------------------------
// volume_fsck — the chkdsk check/fix escalation
// ---------------------------------------------------------------------------

/// The engine's raw chkdsk exit codes -> the verdict vocabulary. The
/// verdictKey is the FINAL i18n key (the connector passes it through; the
/// UI renders it via t() — the single translation point is the bridge).
fn fsck_verdict(exit_code: i32) -> (&'static str, &'static str) {
    match exit_code {
        0 => ("clean", "maintenance.fsckClean"),
        1 => ("fixed", "maintenance.fsckFixed"),
        2 => ("scheduled", "maintenance.fsckScheduled"),
        3 => ("errors-remain", "maintenance.fsckErrorsRemain"),
        259 => ("timeout-still-running", "maintenance.fsckTimeout"),
        _ => ("spawn-failed", "maintenance.fsckSpawnFailed"),
    }
}

#[tauri::command]
pub async fn volume_fsck(letter: String, fix: Option<bool>) -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(move || {
        let l = volume_letter(&letter)?;
        // flags: 0 = check posture (chkdsk X:), 1 = fix posture (/f) — the
        // guidance string's two halves (cr_ui.h:1614-1632).
        let flags: u32 = if fix.unwrap_or(false) { 1 } else { 0 };
        let mut exit_code: i32 = -1;
        check(
            unsafe { abi::cr_ui_fsck_volume(l, flags, &mut exit_code) },
            "cr_ui_fsck_volume",
        )?;
        let (verdict, verdict_key) = fsck_verdict(exit_code);
        let level: fn(&str) = if exit_code == 0 || exit_code == 1 || exit_code == 2 {
            crate::bridge::logging::info
        } else {
            crate::bridge::logging::error
        };
        level(&format!(
            "fsck {}:{} exit {exit_code} ({verdict})",
            l as char,
            if flags == 1 { "/f" } else { "" }
        ));
        Ok(json!({
            "letter": format!("{}:", l as char),
            "exitCode": exit_code,
            "verdict": verdict,
            "verdictKey": verdict_key,
            "fix": flags == 1,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// volume_bitlocker_off — the destructive decrypt twin
// ---------------------------------------------------------------------------

/// The settle/start progress tick logger (cr_ui.h: the callback fires at
/// request start and at the command's settle — the OS decrypt percentage
/// is not observable through this surface, so the ticks are operational
/// evidence in the app log, never a fabricated percentage).
struct BitlockerPipe {
    letter: i8,
}

unsafe extern "C" fn bitlocker_progress_sink(
    user: *mut std::ffi::c_void,
    _stage: u32,
    stage_permille: u32,
    _overall_permille: u32,
    _bytes_done: u64,
    _bytes_total: u64,
    _error_ranges: u32,
) {
    if user.is_null() {
        return;
    }
    let pipe = unsafe { &*(user as *const BitlockerPipe) };
    let tick = if stage_permille >= 1000 {
        "settled"
    } else {
        "request started"
    };
    crate::bridge::logging::info(&format!("bitlocker-off {}:{} {tick}", pipe.letter as char,));
}

#[tauri::command]
pub async fn volume_bitlocker_off(letter: String) -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(move || {
        let l = volume_letter(&letter)?;
        let pipe = Box::into_raw(Box::new(BitlockerPipe { letter: l }));
        let rc = unsafe {
            abi::cr_ui_turn_off_bitlocker(
                l,
                Some(bitlocker_progress_sink),
                pipe as *mut std::ffi::c_void,
            )
        };
        unsafe { drop(Box::from_raw(pipe)) }; // the call is synchronous: the
                                              // sink can never fire past this point
        check(rc, "cr_ui_turn_off_bitlocker")?;
        Ok(json!({
            "letter": format!("{}:", l as char),
            // OK means ACCEPTED, not plaintext — the OS decrypt continues
            // in the background (the honest contract; the UI's completion
            // copy says exactly this).
            "accepted": true,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
