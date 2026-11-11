//! commands/imaging.rs — the Create Image flow (image_create_start /
//! image_create_state; contracts/command-surface.json; type-maps §11).
//!
//! Corpus lineage (the reference's first-class home-page flow): "Create
//! Image" / "Select a drive or a partition to create a backup disk image"
//! (DRWUI strings_ascii:14534-14535, the DiskBackupDialog chain UIUX/03:
//! :125); byte-for-byte copy, elapsed/remaining progress, the space gates
//! ("Insuffcient disk space / Need additional %1", strings:11645-11731).
//! Engine surface: cr_ui_image_create (S-040/R-93) — flat raw image of
//! physical disk N, streamed to `<path>.crimg-tmp`, atomically renamed on
//! completion + a `<path>.sha256` sidecar; on failure the partial is KEPT
//! as `<path>.partial` (reported, never passed off as complete).
//!
//! Truth rules this module enforces:
//! - NO CANCEL: the ABI exports none (cr_ui_image_create runs its win-layer
//!   CancelToken as nullptr — engine/include/cr/cr_ui.h:1594-1612). The
//!   reference's stop-confirm dialog ("You need to re-create the disk
//!   image if you stop it") is a PARITY-DELTAS-registered divergence, not
//!   a fake stop button the bridge would pretend to honor.
//! - The destination pre-gates are BRIDGE-side honesty the engine cannot
//!   express (it writes the file, it does not inspect the volume):
//!   same-disk destination refusal (imaging disk N onto a volume of disk
//!   N is a guaranteed disk-full mid-read — refused up front with the
//!   typed reason) and the free-space gate (byte-for-byte needs the
//!   disk's total bytes + the sidecar + filesystem overhead headroom).
//! - `partialKept` in the terminal state is a FILESYSTEM truth probe
//!   (`<path>.partial` exists), never a guess from the engine's message
//!   text.
//!
//! Threading (the house async-job shape, recovery.rs's): the start command
//! pre-gates on the blocking pool, mints the registry entry, spawns the
//! emitter (drains the bounded progress channel, coalesces, emits
//! `image-progress:<id>`) and the worker (runs the synchronous engine call
//! on its own thread; latches the terminal state, then emits the terminal
//! progress + `image-finished:<id>` itself — the single-writer rule).

use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::mpsc::{sync_channel, Receiver, SyncSender};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, BridgeError, BridgeResult};
use crate::bridge::events::recovery_progress_sink;
use crate::bridge::util::{now_iso, now_unix_ms, SendPtr};

/// Headroom over the byte-for-byte image size the destination must carry:
/// the .sha256 sidecar (~66 B) plus filesystem metadata overhead for a
/// multi-GB single file (NTFS/FAT allocation granularity). 64 MiB is the
/// conservative production default.
const DEST_HEADROOM_BYTES: u64 = 64 * 1024 * 1024;

// ---------------------------------------------------------------------------
// The registry (lane C's own managed state — zero edits to bridge/state.rs)
// ---------------------------------------------------------------------------

/// The single live-progress cell for an image job (the emitter thread is
/// the ONLY writer; `image_create_state` reads under the lock).
pub struct ImageLive {
    pub status: &'static str, // running | completed | failed | cancelled
    pub bytes_done: u64,
    pub bytes_total: u64,
    pub overall: f64, // clamped monotonic
    pub last_throughput: f64,
    pub eta_seconds: Option<u64>,
}

impl ImageLive {
    fn new(bytes_total: u64) -> Self {
        ImageLive {
            status: "running",
            bytes_done: 0,
            bytes_total,
            overall: 0.0,
            last_throughput: 0.0,
            eta_seconds: None,
        }
    }
}

/// One image-creation job. The entry outlives the worker (the registry
/// holds the Arc) so `image_create_state` stays answerable after the
/// terminal event — the same posture as the recovery JobEntry.
pub struct ImageEntry {
    pub image_id: String,
    pub disk: u32,
    pub path: String,
    pub started_at: String,
    pub live: Arc<Mutex<ImageLive>>,
    /// Terminal latch: (status, finished-at ISO), written by the worker
    /// thread exactly once.
    pub finished: Mutex<Option<(&'static str, String)>>,
    /// The boxed progress-fn sender handed to the engine (keeps the sample
    /// channel open for the emitter; freed on Drop — JobEntry's shape).
    pub sink_user: SendPtr<SyncSender<(u64, u64)>>,
}

impl Drop for ImageEntry {
    fn drop(&mut self) {
        let p = self.sink_user.get();
        if !p.is_null() {
            unsafe { drop(Box::from_raw(p)) };
        }
        // No engine handle to close: cr_ui_image_create is a one-shot call
        // with no handle — the worker's return IS the lifecycle end.
    }
}

/// Lane C's imaging registry: image jobs are independent of scans/recovery
/// (they address a physical disk, not a scan tree), so they get their own
/// managed state instead of growing BridgeState (the seam discipline —
/// bridge/state.rs belongs to the defect lane's surface this session).
pub struct ImagingState {
    pub images: Mutex<HashMap<String, Arc<ImageEntry>>>,
    next_image: AtomicU64,
}

impl ImagingState {
    pub fn new() -> Self {
        ImagingState {
            images: Mutex::new(HashMap::new()),
            next_image: AtomicU64::new(1),
        }
    }

    fn mint_image_id(&self) -> String {
        format!("img-{}", self.next_image.fetch_add(1, Ordering::Relaxed))
    }

    fn entry_arc(&self, image_id: &str) -> BridgeResult<Arc<ImageEntry>> {
        self.images
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "images lock"))?
            .get(image_id)
            .cloned()
            .ok_or_else(|| {
                BridgeError::bridge(
                    code::IMAGE_NOT_FOUND,
                    format!("image job '{image_id}' is not registered"),
                )
            })
    }
}

// ---------------------------------------------------------------------------
// Destination pre-gates (bridge-side honesty the engine cannot express)
// ---------------------------------------------------------------------------

#[cfg(windows)]
fn letter_free_bytes(letter: u8) -> Option<u64> {
    use windows_sys::Win32::Storage::FileSystem::GetDiskFreeSpaceExW;
    let root: Vec<u16> = format!("{}:\\", letter as char)
        .encode_utf16()
        .chain([0])
        .collect();
    let mut free: u64 = 0;
    let mut _total: u64 = 0;
    let mut _avail: u64 = 0;
    let ok = unsafe {
        GetDiskFreeSpaceExW(
            root.as_ptr(),
            &mut free as *mut u64,
            &mut _total as *mut u64,
            &mut _avail as *mut u64,
        )
    };
    if ok == 0 {
        return None;
    }
    Some(free)
}

#[cfg(not(windows))]
fn letter_free_bytes(_letter: u8) -> Option<u64> {
    None // imaging is the Windows platform layer; the engine refuses elsewhere
}

fn letter_of_path(path: &str) -> Option<u8> {
    let b = path.trim().as_bytes();
    if b.len() >= 2 && b[0].is_ascii_alphabetic() && b[1] == b':' {
        Some(b[0].to_ascii_uppercase())
    } else {
        None
    }
}

/// The pre-gate verdict for a destination path against the imaged disk.
enum DestGate {
    Ok,
    /// Imaging disk N onto a volume of disk N (the typed refusal).
    SourceOverlap,
    /// Free space below the byte-for-byte need + headroom (free carried
    /// for the honest "need additional" axis).
    OutOfSpace {
        free: u64,
        needed: u64,
    },
}

fn destination_gate(path: &str, disk: u32, disk_bytes: u64) -> DestGate {
    let Some(letter) = letter_of_path(path) else {
        return DestGate::Ok; // no letter to reason about (UNC/relative):
                             // the engine's own write failure is the truth
    };
    // Same-disk: the destination volume lives on the disk being imaged.
    let volumes = crate::bridge::util::enumerate_volumes().unwrap_or_default();
    if volumes.iter().any(|v| v.letter == letter && v.disk == disk) {
        return DestGate::SourceOverlap;
    }
    let needed = disk_bytes.saturating_add(DEST_HEADROOM_BYTES);
    match letter_free_bytes(letter) {
        Some(free) if free < needed => DestGate::OutOfSpace { free, needed },
        _ => DestGate::Ok, // unknown free (query failed): the engine's own
                           // write failure stays the truthful backstop
    }
}

// ---------------------------------------------------------------------------
// image_create_start / image_create_state
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn image_create_start(
    app: tauri::AppHandle,
    imaging: tauri::State<'_, Arc<ImagingState>>,
    device_id: String,
    path: String,
) -> Result<Value, BridgeError> {
    let imaging = imaging.inner().clone();
    let (entry, image_id, rx) = tauri::async_runtime::spawn_blocking(move || {
        image_create_start_blocking(&imaging, &device_id, path)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))??;
    let finished = Arc::new(AtomicBool::new(false));
    spawn_image_emitter(
        rx,
        app.clone(),
        image_id.clone(),
        entry.live.clone(),
        finished.clone(),
    );
    spawn_image_worker(app, entry, finished);
    Ok(json!({ "imageId": image_id }))
}

fn image_create_start_blocking(
    imaging: &Arc<ImagingState>,
    device_id: &str,
    path: String,
) -> BridgeResult<(Arc<ImageEntry>, String, Receiver<(u64, u64)>)> {
    let trimmed = path.trim().to_string();
    if trimmed.is_empty() {
        return Err(BridgeError::bridge(
            code::SCAN_SCOPE_INVALID,
            "image destination path is empty",
        ));
    }
    // The connector speaks device ids; the imaging verb addresses the
    // whole physical disk (disk-N — the byte-for-byte source).
    let disk: u32 = device_id
        .strip_prefix("disk-")
        .and_then(|rest| rest.parse().ok())
        .ok_or_else(|| {
            BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                format!("imaging addresses whole disks: '{device_id}' is not a disk-N id"),
            )
        })?;
    // Target truth: the disk must be present, and its geometry is the
    // byte-for-byte size the destination must carry.
    let disks = crate::bridge::util::enumerate_disks()?;
    let d = disks.iter().find(|d| d.number == disk).ok_or_else(|| {
        BridgeError::bridge(
            code::DEVICE_GONE,
            format!("disk {disk} is not present (removed or renumbered)"),
        )
    })?;
    match destination_gate(&trimmed, disk, d.size_bytes) {
        DestGate::Ok => {}
        DestGate::SourceOverlap => {
            return Err(BridgeError::bridge(
                code::DESTINATION_OVERLAP,
                format!(
                    "the image destination is on disk {disk} itself — a byte-for-byte image of \
                     the source cannot land on the source (pick a volume on another disk)"
                ),
            ));
        }
        DestGate::OutOfSpace { free, needed } => {
            return Err(BridgeError::bridge(
                code::DESTINATION_OUT_OF_SPACE,
                format!(
                    "insufficient destination space: {free} bytes free, {needed} needed \
                     (byte-for-byte image + sidecar headroom)"
                ),
            ));
        }
    }

    // The progress pipe: the engine's own recovery-style cumulative-bytes
    // callback, drained by the emitter (recovery.rs's exact shape).
    let (tx, rx) = sync_channel::<(u64, u64)>(64);
    let sink_user = Box::into_raw(Box::new(tx));

    let image_id = imaging.mint_image_id();
    let entry = Arc::new(ImageEntry {
        image_id: image_id.clone(),
        disk,
        path: trimmed.clone(),
        started_at: now_iso(),
        live: Arc::new(Mutex::new(ImageLive::new(d.size_bytes))),
        finished: Mutex::new(None),
        sink_user: SendPtr::new(sink_user),
    });
    imaging
        .images
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "images lock"))?
        .insert(image_id.clone(), entry.clone());
    crate::bridge::logging::info(&format!(
        "image job {image_id}: disk {disk} -> {} ({:.1} GiB)",
        trimmed,
        d.size_bytes as f64 / (1024.0 * 1024.0 * 1024.0)
    ));
    Ok((entry, image_id, rx))
}

/// The emitter: drains the bounded sample channel (freshest wins), keeps
/// the live cell current, emits `image-progress:<id>` — the
/// spawn_job_emitter shape, minus the recovery report tail (image_create
/// has no mid-run stats query; the callback bytes are the only live axis).
fn spawn_image_emitter(
    rx: Receiver<(u64, u64)>,
    app: tauri::AppHandle,
    image_id: String,
    live: Arc<Mutex<ImageLive>>,
    finished: Arc<AtomicBool>,
) {
    std::thread::spawn(move || {
        let mut prev_bytes: u64 = 0;
        let mut prev_sample_ms: i64 = 0;
        loop {
            let sample = match rx.recv_timeout(Duration::from_millis(250)) {
                Ok(s) => {
                    let mut newest = Some(s);
                    while let Ok(s2) = rx.try_recv() {
                        newest = Some(s2);
                    }
                    newest
                }
                Err(_) => {
                    if finished.load(Ordering::Acquire) {
                        break; // the worker emitted the terminal payload
                    }
                    None
                }
            };
            let Ok(mut l) = live.lock() else { break };
            if finished.load(Ordering::Acquire) {
                continue;
            }
            if let Some((done, total)) = sample {
                let now = now_unix_ms();
                if prev_sample_ms > 0 {
                    let dt_s = (now - prev_sample_ms).max(1) as f64 / 1000.0;
                    let delta = done.saturating_sub(prev_bytes);
                    if delta > 0 {
                        let bps = delta as f64 / dt_s;
                        l.last_throughput = if l.last_throughput == 0.0 {
                            bps
                        } else {
                            l.last_throughput * 0.6 + bps * 0.4
                        };
                    }
                }
                prev_bytes = done;
                prev_sample_ms = now;
                l.bytes_done = done;
                l.bytes_total = total;
                let frac = if total == 0 {
                    0.0
                } else {
                    done as f64 / total as f64
                };
                if frac > l.overall {
                    l.overall = frac; // clamped monotonic
                }
                if l.status == "running" && l.bytes_total > l.bytes_done && l.last_throughput > 0.0
                {
                    l.eta_seconds =
                        Some(((l.bytes_total - l.bytes_done) as f64 / l.last_throughput) as u64);
                }
            }
            let payload = compose_image_progress(&image_id, &l);
            let _ = tauri::Emitter::emit(&app, &format!("image-progress:{image_id}"), payload);
        }
    });
}

fn compose_image_progress(image_id: &str, live: &ImageLive) -> Value {
    json!({
        "imageId": image_id,
        "status": live.status,
        "bytesDone": live.bytes_done,
        "bytesTotal": live.bytes_total,
        "overallProgress": live.overall,
        "throughputBytesPerSecond": live.last_throughput,
        "etaSeconds": live.eta_seconds,
    })
}

/// The worker: runs the synchronous engine call on its own thread, latches
/// the terminal state, then emits the terminal progress +
/// `image-finished:<id>` itself (the single-writer rule — the emitter
/// stops composing once `finished` is set).
fn spawn_image_worker(app: tauri::AppHandle, entry: Arc<ImageEntry>, finished: Arc<AtomicBool>) {
    std::thread::spawn(move || {
        let path = crate::bridge::util::cstr(&entry.path);
        let rc = unsafe {
            abi::cr_ui_image_create(
                entry.disk,
                path.as_ptr(),
                Some(recovery_progress_sink),
                entry.sink_user.get() as *mut std::ffi::c_void,
            )
        };
        let status: &'static str = if rc == abi::CR_UI_OK {
            "completed"
        } else if rc == abi::CR_UI_ERR_CANCELLED {
            "cancelled"
        } else {
            "failed"
        };
        let finished_at = now_iso();
        if let Ok(mut f) = entry.finished.lock() {
            *f = Some((status, finished_at.clone()));
        }
        if let Ok(mut l) = entry.live.lock() {
            l.status = status;
            if status == "completed" {
                // The atomic rename landed: the full byte count is truth.
                l.overall = 1.0;
                l.eta_seconds = Some(0);
                if l.bytes_total > 0 && l.bytes_done < l.bytes_total {
                    l.bytes_done = l.bytes_total;
                }
            }
            let payload = compose_image_progress(&entry.image_id, &l);
            let _ =
                tauri::Emitter::emit(&app, &format!("image-progress:{}", entry.image_id), payload);
        }
        // Filesystem truth: the engine keeps `<path>.partial` when any
        // bytes were committed before a failure/cancel — probed, never
        // parsed out of the engine's message text.
        let partial_kept = status != "completed"
            && std::path::PathBuf::from(&entry.path)
                .with_extension("partial")
                .exists();
        let level: fn(&str) = if status == "completed" {
            crate::bridge::logging::info
        } else {
            crate::bridge::logging::error
        };
        level(&format!(
            "image job {}: {} (rc {rc:#010x}, partial kept: {partial_kept})",
            entry.image_id, status
        ));
        finished.store(true, Ordering::Release);
        let _ = tauri::Emitter::emit(
            &app,
            &format!("image-finished:{}", entry.image_id),
            json!({ "imageId": entry.image_id, "status": status, "partialKept": partial_kept }),
        );
    });
}

#[tauri::command]
pub async fn image_create_state(
    imaging: tauri::State<'_, Arc<ImagingState>>,
    image_id: String,
) -> Result<Value, BridgeError> {
    let imaging = imaging.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = imaging.entry_arc(&image_id)?;
        let (status, finished_at) = entry
            .finished
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "image finished lock"))?
            .as_ref()
            .map(|(s, at)| (*s, Some(at.clone())))
            .unwrap_or(("running", None));
        let l = entry
            .live
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "image live lock"))?;
        let partial_kept = finished_at.is_some()
            && status != "completed"
            && std::path::PathBuf::from(&entry.path)
                .with_extension("partial")
                .exists();
        Ok(json!({
            "imageId": entry.image_id,
            "disk": entry.disk,
            "path": entry.path,
            "status": status,
            "bytesDone": l.bytes_done,
            "bytesTotal": l.bytes_total,
            "overallProgress": l.overall,
            "throughputBytesPerSecond": l.last_throughput,
            "etaSeconds": l.eta_seconds,
            "startedAt": entry.started_at,
            "finishedAt": finished_at,
            // Truthful: no cancel exists at the ABI (the design sheet's
            // recorded divergence) — the UI renders it as a documented
            // "runs to completion" contract, never a dead stop button.
            "cancelSupported": false,
            "partialKept": partial_kept,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
