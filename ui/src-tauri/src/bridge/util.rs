//! bridge/util.rs — two-call helpers, string/time conversion, base64.
//!
//! Memory rules (cr_ui.h): heap results are freed by the owning side —
//! `cr_ui_string_free` for char*, `cr_ui_query_rows_free` for query rows,
//! `_close` per handle family. Strings cross to TS as UTF-8 (serde);
//! timestamps convert 100ns-1601 → ISO-8601 in THIS file only (the single
//! conversion point, docs/INTEGRATION-PLAN.md §4.4).

use super::abi;
use super::error::{check, BridgeError, BridgeResult};

/// A raw FFI handle carried inside the registry. The registry's Mutex
/// serializes every use of the pointer; the engine's own contract makes
/// token/telemetry/snapshot calls any-thread safe. `Send + Sync` here is the
/// explicit bridge discipline (docs/INTEGRATION-PLAN.md §4.2), not an accident.
#[derive(Clone, Copy)]
pub struct SendPtr<T>(*mut T);

impl<T> SendPtr<T> {
    pub fn new(p: *mut T) -> Self {
        SendPtr(p)
    }
    pub fn get(&self) -> *mut T {
        self.0
    }
    pub fn is_null(&self) -> bool {
        self.0.is_null()
    }
}
unsafe impl<T> Send for SendPtr<T> {}
unsafe impl<T> Sync for SendPtr<T> {}

// ---- strings -----------------------------------------------------------------

/// Read a NUL-terminated UTF-8 C string (lossy: the engine guarantees UTF-8;
/// lossy is defense in depth so a malformed byte can never poison a payload).
pub unsafe fn cstr_to_string_lossy(p: *const i8) -> String {
    if p.is_null() {
        return String::new();
    }
    let mut len = 0usize;
    while unsafe { *p.add(len) } != 0 {
        len += 1;
        if len > 1 << 20 {
            break; // defensive: never walk an unbounded string
        }
    }
    let bytes = unsafe { std::slice::from_raw_parts(p as *const u8, len) };
    String::from_utf8_lossy(bytes).into_owned()
}

/// Read a fixed-capacity NUL-terminated char array ([u8; N]) into a String.
pub fn fixed_str<const N: usize>(buf: &[u8; N]) -> String {
    let end = buf.iter().position(|&b| b == 0).unwrap_or(N);
    String::from_utf8_lossy(&buf[..end]).into_owned()
}

/// Borrow a Rust string as a C string for the duration of a call. The engine
/// never retains `const char*` arguments past the call (cr_ui.h ownership
/// rules), so a transient `CString` is correct.
pub fn cstr(s: &str) -> std::ffi::CString {
    std::ffi::CString::new(s).unwrap_or_default()
}

// ---- time: 100ns-since-1601 → ISO-8601 (the single conversion point) ----------

const MS_1601_TO_1970: u64 = 11_644_473_600_000; // unix-ms at 1601-01-01

/// days_from_civil (Howard Hinnant's algorithm) — the inverse maps a unix
/// day number back to Y/M/D without any platform calendar dependency.
fn civil_from_unix_days(z: i64) -> (i64, u32, u32) {
    let z = z + 719_468;
    let era = if z >= 0 { z } else { z - 146_096 } / 146_097;
    let doe = (z - era * 146_097) as u64; // [0, 146096]
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146_096) / 365; // [0, 399]
    let y = yoe as i64 + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
    let mp = (5 * doy + 2) / 153; // [0, 11]
    let d = (doy - (153 * mp + 2) / 5 + 1) as u32; // [1, 31]
    let m = if mp < 10 { mp + 3 } else { mp - 9 } as u32; // [1, 12]
    (if m <= 2 { y + 1 } else { y }, m, d)
}

/// Civil date parts for a unix day count (the auto-name composer uses M/D).
pub fn civil_parts(days: i64) -> (i64, u32, u32) {
    civil_from_unix_days(days)
}

/// Format unix milliseconds as `YYYY-MM-DDTHH:MM:SS.sssZ` (UTC, the shape
/// `new Date().toISOString()` produced — the contract's Iso8601 examples).
pub fn iso_from_unix_ms(unix_ms: i64) -> String {
    let secs = unix_ms.div_euclid(1000);
    let ms = unix_ms.rem_euclid(1000) as u32;
    let days = secs.div_euclid(86_400);
    let tod = secs.rem_euclid(86_400);
    let (y, mo, d) = civil_from_unix_days(days);
    let h = tod / 3600;
    let mi = (tod % 3600) / 60;
    let s = tod % 60;
    format!("{y:04}-{mo:02}-{d:02}T{h:02}:{mi:02}:{s:02}.{ms:03}Z")
}

/// Convert an engine FILETIME-shaped value (100ns units since 1601-01-01,
/// e.g. cr_ui_node_info.modified) to ISO-8601; 0 → None (unbounded/absent).
pub fn iso_from_filetime(ft: u64) -> Option<String> {
    if ft == 0 {
        return None;
    }
    let unix_ms = (ft / 10_000) as i64 - MS_1601_TO_1970 as i64;
    Some(iso_from_unix_ms(unix_ms))
}

/// Current wall time as ISO-8601 (bridge-tracked startedAt/completedAt).
pub fn now_iso() -> String {
    iso_from_unix_ms(now_unix_ms())
}

pub fn now_unix_ms() -> i64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_millis() as i64)
        .unwrap_or(0)
}

// ---- base64 (standard alphabet, padded) -----------------------------------------

const B64: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

pub fn base64_encode(data: &[u8]) -> String {
    let mut out = String::with_capacity(data.len().div_ceil(3) * 4);
    for chunk in data.chunks(3) {
        let b0 = chunk[0] as u32;
        let b1 = chunk.get(1).map(|&b| b as u32).unwrap_or(0);
        let b2 = chunk.get(2).map(|&b| b as u32).unwrap_or(0);
        let n = (b0 << 16) | (b1 << 8) | b2;
        out.push(B64[(n >> 18) as usize & 63] as char);
        out.push(B64[(n >> 12) as usize & 63] as char);
        out.push(if chunk.len() > 1 {
            B64[(n >> 6) as usize & 63] as char
        } else {
            '='
        });
        out.push(if chunk.len() > 2 {
            B64[n as usize & 63] as char
        } else {
            '='
        });
    }
    out
}

// ---- two-call protocol helpers ----------------------------------------------------

/// Two-call enumeration of disks (cr_ui.h: *count is IN capacity when out
/// != NULL, OUT needed when out == NULL).
pub fn enumerate_disks() -> BridgeResult<Vec<abi::CrUiDiskInfo>> {
    let mut needed: u32 = 0;
    let rc = unsafe { abi::cr_ui_enumerate_disks(std::ptr::null_mut(), &mut needed) };
    check(rc, "cr_ui_enumerate_disks(size)")?;
    if needed == 0 {
        return Ok(Vec::new());
    }
    let mut buf = vec![unsafe { std::mem::zeroed::<abi::CrUiDiskInfo>() }; needed as usize];
    let mut cap = needed;
    let rc = unsafe { abi::cr_ui_enumerate_disks(buf.as_mut_ptr(), &mut cap) };
    check(rc, "cr_ui_enumerate_disks")?;
    buf.truncate(cap as usize);
    Ok(buf)
}

pub fn enumerate_volumes() -> BridgeResult<Vec<abi::CrUiVolumeInfo>> {
    let mut needed: u32 = 0;
    let rc = unsafe { abi::cr_ui_enumerate_volumes(std::ptr::null_mut(), &mut needed) };
    check(rc, "cr_ui_enumerate_volumes(size)")?;
    if needed == 0 {
        return Ok(Vec::new());
    }
    let mut buf = vec![unsafe { std::mem::zeroed::<abi::CrUiVolumeInfo>() }; needed as usize];
    let mut cap = needed;
    let rc = unsafe { abi::cr_ui_enumerate_volumes(buf.as_mut_ptr(), &mut cap) };
    check(rc, "cr_ui_enumerate_volumes")?;
    buf.truncate(cap as usize);
    Ok(buf)
}

/// Two-call DFS order of a snapshot (parents precede children — pre-order).
pub fn snapshot_dfs(snap: *mut abi::CrUiSnapshot) -> BridgeResult<Vec<u32>> {
    let mut needed: u32 = 0;
    let rc = unsafe { abi::cr_ui_snapshot_dfs(snap, std::ptr::null_mut(), &mut needed) };
    check(rc, "cr_ui_snapshot_dfs(size)")?;
    if needed == 0 {
        return Ok(Vec::new());
    }
    let mut idx = vec![0u32; needed as usize];
    let mut cap = needed;
    let rc = unsafe { abi::cr_ui_snapshot_dfs(snap, idx.as_mut_ptr(), &mut cap) };
    check(rc, "cr_ui_snapshot_dfs")?;
    idx.truncate(cap as usize);
    Ok(idx)
}

/// One row of the snapshot: node info + full (never silently truncated) name.
/// Single call with a generous buffer; re-fetch with exact size on TRUNCATED.
pub fn snapshot_node(
    snap: *mut abi::CrUiSnapshot,
    index: u32,
) -> BridgeResult<(abi::CrUiNodeInfo, String)> {
    let mut info: abi::CrUiNodeInfo = unsafe { std::mem::zeroed() };
    let mut buf = vec![0i8; 1024];
    let rc = unsafe {
        abi::cr_ui_snapshot_node(snap, index, &mut info, buf.as_mut_ptr(), buf.len() as u32)
    };
    if rc == abi::CR_UI_ERR_TRUNCATED {
        // exact-size retry (name_len carries the needed size)
        let need = info.name_len as usize + 1;
        buf = vec![0i8; need];
        let rc = unsafe {
            abi::cr_ui_snapshot_node(snap, index, &mut info, buf.as_mut_ptr(), buf.len() as u32)
        };
        check(rc, "cr_ui_snapshot_node(retry)")?;
    } else {
        check(rc, "cr_ui_snapshot_node")?;
    }
    Ok((info, unsafe { cstr_to_string_lossy(buf.as_ptr()) }))
}

/// The engine's heap-string report JSON, freed on drop.
pub struct OwnedCrString(*mut i8);

impl OwnedCrString {
    /// Take ownership of a `char**` out-param; `None` when the engine handed
    /// back NULL (which it never does on success — defensive only).
    pub unsafe fn from_raw(p: *mut i8) -> Option<Self> {
        if p.is_null() {
            None
        } else {
            Some(OwnedCrString(p))
        }
    }
    pub fn to_json(&self) -> BridgeResult<serde_json::Value> {
        let s = unsafe { cstr_to_string_lossy(self.0) };
        serde_json::from_str(&s).map_err(|e| {
            BridgeError::bridge(
                super::error::code::UNKNOWN,
                format!("engine report JSON did not parse: {e}"),
            )
        })
    }
}

impl Drop for OwnedCrString {
    fn drop(&mut self) {
        if !self.0.is_null() {
            unsafe { abi::cr_ui_string_free(self.0) };
            self.0 = std::ptr::null_mut();
        }
    }
}
