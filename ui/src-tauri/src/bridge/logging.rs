//! bridge/logging.rs — NG-15: the product's operational log.
//!
//! `%ProgramData%\HeicFast\Recovery Wizard\logs\app.log` (installed mode;
//! `.\logs\` beside the exe when ProgramData is absent — portable mode,
//! the same resolution as the session store). A dedicated writer thread +
//! channel honors the engine's contract (cr_log.h: sinks fire from ANY
//! engine thread and must be cheap or internally deferred — a blocking
//! sink blocks the emitting worker). Rotation at 1 MiB to `app.log.old`
//! (one generation kept).
//!
//! Two producers feed the channel: the ENGINE's diagnostic lines (scan
//! brackets, stage boundaries, read failures, memory warnings — through
//! the registered `cr_ui_set_log_sink`) and the BRIDGE's lifecycle facts
//! (startup, the ABI gate, scan lifecycle, failures). Before `init()` the
//! module is a silent no-op, so early-startup code can call `info`/`error`
//! unconditionally.

use std::ffi::c_void;
use std::ffi::CStr;
use std::fs::OpenOptions;
use std::io::Write;
use std::path::PathBuf;
use std::sync::mpsc::{Sender, channel};
use std::sync::OnceLock;
use std::time::{SystemTime, UNIX_EPOCH};

const ROTATE_BYTES: u64 = 1024 * 1024;

static SENDER: OnceLock<Sender<String>> = OnceLock::new();

/// The logs directory: ProgramData in installed mode, beside the exe in
/// portable mode (mirrors `state::sessions_dir`'s contract — the shared
/// marker probe, util::portable_mode, NG-18).
fn logs_dir() -> PathBuf {
    if crate::bridge::util::portable_mode() {
        let local = std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|d| d.join("logs")))
            .unwrap_or_else(|| PathBuf::from("logs"));
        let _ = std::fs::create_dir_all(&local);
        return local;
    }
    if let Ok(pd) = std::env::var("ProgramData") {
        if !pd.is_empty() {
            let dir = PathBuf::from(pd)
                .join("HeicFast")
                .join("Recovery Wizard")
                .join("logs");
            let _ = std::fs::create_dir_all(&dir);
            return dir;
        }
    }
    let local = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.join("logs")))
        .unwrap_or_else(|| PathBuf::from("logs"));
    let _ = std::fs::create_dir_all(&local);
    local
}

/// Minimal UTC timestamp (no chrono dependency): `YYYY-MM-DD HH:MM:SSZ`.
fn timestamp() -> String {
    let secs = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    let days = secs / 86_400;
    let rem = secs % 86_400;
    // days-from-civil inverse (Howard Hinnant's algorithm)
    let z = i64::try_from(days).unwrap_or(0) + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z.rem_euclid(146_097);
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = if m <= 2 { y + 1 } else { y };
    format!(
        "{y:04}-{m:02}-{d:02} {:02}:{:02}:{:02}Z",
        rem / 3600,
        (rem % 3600) / 60,
        rem % 60
    )
}

fn send(line: String) {
    if let Some(tx) = SENDER.get() {
        let _ = tx.send(line);
    }
}

/// Info-level bridge lifecycle fact.
pub fn info(msg: &str) {
    send(format!("{} INFO  {msg}", timestamp()));
}

/// Error-level bridge lifecycle fact.
pub fn error(msg: &str) {
    send(format!("{} ERROR {msg}", timestamp()));
}

/// The engine sink (cr_log.h levels: 0 Error, 1 Warn, 2 Info).
unsafe extern "C" fn engine_sink(level: u32, line: *const i8, _user: *mut c_void) {
    if line.is_null() {
        return;
    }
    let text = unsafe { CStr::from_ptr(line.cast()) }.to_string_lossy();
    let lvl = match level {
        0 => "ERROR",
        1 => "WARN ",
        _ => "INFO ",
    };
    send(format!("{} [engine] {lvl} {text}", timestamp()));
}

/// Install the log: writer thread + the engine sink. Idempotent (the
/// second call is a no-op). Called once from the shell's `run()`.
pub fn init() {
    if SENDER.get().is_some() {
        return;
    }
    let (tx, rx) = channel::<String>();
    if SENDER.set(tx).is_err() {
        return; // lost a race with another init: the winner's channel serves
    }
    let dir = logs_dir();
    let path = dir.join("app.log");
    let old = dir.join("app.log.old");
    std::thread::Builder::new()
        .name("rw-applog".into())
        .spawn(move || {
            while let Ok(line) = rx.recv() {
                // rotate on size (best-effort; a failed rotation never drops
                // the line — the write below still happens)
                if let Ok(meta) = std::fs::metadata(&path) {
                    if meta.len() >= ROTATE_BYTES {
                        let _ = std::fs::rename(&path, &old);
                    }
                }
                if let Ok(mut f) = OpenOptions::new().create(true).append(true).open(&path) {
                    let _ = writeln!(f, "{line}");
                }
            }
        })
        .ok();
    unsafe {
        crate::bridge::abi::cr_ui_set_log_sink(Some(engine_sink), std::ptr::null_mut());
    }
    info("app log opened (bridge + engine sinks active)");
}
