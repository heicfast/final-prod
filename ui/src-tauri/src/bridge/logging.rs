//! bridge/logging.rs — NG-15: the product's operational log.
//!
//! `%ProgramData%\HeicFast\Recovery Wizard\logs\app.log` (installed mode;
//! `.\logs\` beside the exe in portable mode — the same resolution as the
//! session store, decided by the portable marker probe, NG-18). A dedicated
//! writer thread + channel honors the engine's contract (cr_log.h: sinks
//! fire from ANY engine thread and must be cheap or internally deferred — a
//! blocking sink blocks the emitting worker). Rotation at 1 MiB to
//! `app.log.old` (one generation kept).
//!
//! Two producers feed the channel: the ENGINE's diagnostic lines (scan
//! brackets, stage boundaries, read failures, memory warnings — through the
//! C++-mangled seam, BF-01) and the BRIDGE's lifecycle facts (startup, the
//! ABI gate, scan lifecycle, failures). Before `init()` the module is a
//! silent no-op, so early-startup code can call `info`/`error`
//! unconditionally.
//!
//! The lost-candidate geometry feed (N-061): the engine's discovery pass
//! writes EVERY candidate as a scored log line
//! (`lost candidate: lba=%llu sectors=%llu fs=%u score=%u` — the og's
//! CalcPartition.log convention; the format is pinned in
//! engine/src/core/orchestrator/cr_scan.cpp emit_lost_candidate_). The ABI
//! event (CR_EVT_LOST_CANDIDATE) carries the LBA only, so the sink ALSO
//! stashes the geometry keyed by LBA and the scan emitter joins the two
//! (the log line is written BEFORE the event fires, from the same call —
//! the join is deterministic). `take_candidate_geometry` consumes a stash
//! entry; unconsumed entries age out (bounded).

use std::ffi::c_void;
use std::ffi::CStr;
use std::fs::OpenOptions;
use std::io::Write;
use std::path::PathBuf;
use std::sync::mpsc::{Sender, channel};
use std::sync::{Mutex, OnceLock};
use std::time::{SystemTime, UNIX_EPOCH};

const ROTATE_BYTES: u64 = 1024 * 1024;

/// Bounded stash: at most this many unconsumed candidate rows.
const CANDIDATE_STASH_CAP: usize = 256;
/// Stash entries older than this are pruned (a join that never came).
const CANDIDATE_STASH_TTL_MS: u64 = 60_000;

static SENDER: OnceLock<Sender<String>> = OnceLock::new();

/// One stashed lost-candidate geometry row (the N-061 line's payload).
#[derive(Clone, Copy)]
struct CandidateGeom {
    lba: u64,
    sectors: u64,
    fs: u32,
    score: u32,
    seen_ms: u64,
}

static CANDIDATES: OnceLock<Mutex<Vec<CandidateGeom>>> = OnceLock::new();

fn candidates() -> &'static Mutex<Vec<CandidateGeom>> {
    CANDIDATES.get_or_init(|| Mutex::new(Vec::new()))
}

fn now_ms() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_millis() as u64)
        .unwrap_or(0)
}

/// Parse + stash a `lost candidate:` line (the pinned N-061 format).
fn feed_candidate_line(line: &str) {
    let Some(rest) = line.strip_prefix("lost candidate: lba=") else {
        return;
    };
    let mut it = rest.split(' ');
    let (Some(lba), Some(sectors), Some(fs), Some(score)) = (
        it.next().and_then(|v| v.parse::<u64>().ok()),
        it.next().and_then(|v| v.strip_prefix("sectors=").and_then(|v| v.parse::<u64>().ok()),
        it.next().and_then(|v| v.strip_prefix("fs=").and_then(|v| v.parse::<u32>().ok())),
        it.next().and_then(|v| v.strip_prefix("score=").and_then(|v| v.parse::<u32>().ok())),
    ) else {
        return; // not the pinned shape — never guess
    };
    if let Ok(mut c) = candidates().lock() {
        // replace a same-LBA row if present, else append; prune by cap + age
        if let Some(slot) = c.iter_mut().find(|g| g.lba == lba) {
            *slot = CandidateGeom { lba, sectors, fs, score, seen_ms: now_ms() };
        } else {
            c.push(CandidateGeom { lba, sectors, fs, score, seen_ms: now_ms() });
        }
        if c.len() > CANDIDATE_STASH_CAP {
            let excess = c.len() - CANDIDATE_STASH_CAP;
            c.drain(0..excess);
        }
        let now = now_ms();
        c.retain(|g| now.saturating_sub(g.seen_ms) < CANDIDATE_STASH_TTL_MS);
    }
}

/// Consume the stashed geometry for `lba` (the CR_EVT_LOST_CANDIDATE join).
pub fn take_candidate_geometry(lba: u64) -> Option<(u64, u32, u32)> {
    let mut c = candidates().lock().ok()?;
    if let Some(pos) = c.iter().position(|g| g.lba == lba) {
        let g = c.remove(pos);
        Some((g.sectors, g.fs, g.score))
    } else {
        None
    }
}

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

/// The engine sink (cr_log.h levels: 0 Error, 1 Warn, 2 Info). Fires on ANY
/// engine thread under the seam's mutex — one channel send, nothing else.
pub unsafe extern "system" fn engine_sink(level: u32, line: *const i8, _user: *mut c_void) {
    if line.is_null() {
        return;
    }
    let text = unsafe { CStr::from_ptr(line.cast()) }.to_string_lossy();
    // The N-061 candidate feed (deterministic join for the LBA-only event).
    feed_candidate_line(&text);
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
        // BF-01: the C++-mangled seam (cr_ui_exports.def:19) — no flat-C
        // export exists on ABI v4.
        crate::bridge::abi::cr_log_set_sink(Some(engine_sink), std::ptr::null_mut());
    }
    info("app log opened (bridge + engine sinks active)");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn candidate_line_parses_and_consumes() {
        feed_candidate_line("lost candidate: lba=4096 sectors=204800 fs=0 score=85");
        assert_eq!(
            take_candidate_geometry(4096),
            Some((204_800, 0, 85))
        );
        // consumed exactly once
        assert_eq!(take_candidate_geometry(4096), None);
        // a non-matching line never feeds
        feed_candidate_line("scan stage: quick");
        assert_eq!(take_candidate_geometry(1), None);
    }

    #[test]
    fn malformed_candidate_line_is_ignored() {
        feed_candidate_line("lost candidate: lba=abc sectors=x fs=y score=z");
        assert_eq!(take_candidate_geometry(0), None);
    }
}
