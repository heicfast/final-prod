//! bridge/state.rs — handle registries + the snapshot metadata cache.
//!
//! Layout (worklog/INTEGRATION-PLAN.md §4.2): `BridgeState` is Tauri-managed
//! state holding the scan/job registries. Entries are `Arc`ed so long passes
//! (the O(n) metadata build) hold only the ENTRY's snapshot mutex — never the
//! registry lock — while generation swaps stay serialized on that same mutex
//! (a stale handle can never be closed mid-pass).
//!
//! Close order is enforced on Drop per the engine's lifetime rules: preview
//! handles are transient (opened + closed inside a single command), snapshot
//! caches close before the owning session/scan, scan before device, device
//! last. One controlling thread per run-handle: run/process execute on
//! dedicated spawned threads; token and telemetry queries are any-thread safe
//! by the engine's own contract.
//!
//! v4 deltas vs the v2 bridge: the consent registry carries BOTH families
//! (pre-scan repair, keyed by scan; lost-partition apply, keyed by device —
//! plan §4.1's "two consent families, separate registries keyed by handle",
//! unified under one echo-token respond path); ScanLive carries the
//! lost-candidate ROWS (the N-061 log-line geometry joined to the
//! CR_EVT_LOST_CANDIDATE LBA — see bridge::logging); the session listing is
//! the W1a light-records call (cr_ui_session_list), replacing the v2
//! per-file load+close cache.

use std::collections::HashMap;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::mpsc::SyncSender;
use std::sync::{Arc, Mutex};

use super::abi;
use super::error::{code, BridgeError, BridgeResult};
use super::filefilter::category_for_extension;
use super::util::{iso_from_filetime, SendPtr};

pub const BAD_SECTOR_MAP_CAP: usize = 4096; // bounded map; the count stays exact
/// Bound for the per-scan lost-candidate row list (the count stays exact).
pub const LOST_CANDIDATE_CAP: usize = 4096;

// ---------------------------------------------------------------------------
// Scan registry
// ---------------------------------------------------------------------------

/// What a registry entry is backed by. A live scan owns a `cr_ui_scan` (+
/// the borrowed `cr_ui_device`); a loaded session owns a `cr_ui_session`
/// (browsable tree, results, recovery — the post-scan surface).
pub enum ScanKind {
    Live {
        scan: SendPtr<abi::CrUiScan>,
        device: SendPtr<abi::CrUiDevice>,
        /// True when THIS entry owns the device (scan_start's own open).
        /// False for RESUMED scans: the device is the loaded session
        /// entry's own — the resumed entry borrows it (the loaded entry is
        /// pinned by the resume pushing the resumed scan id into its jobs
        /// list, so scan_close refuses while the borrower lives) and must
        /// not close it. The session handle outlives the resumed scan the
        /// same way (cr_ui.h: resume loads into a fresh tree, but the
        /// borrowed device definitely outlives every scan using it).
        owns_device: bool,
    },
    Session {
        session: SendPtr<abi::CrUiSession>,
        /// The re-attached source device (best-effort: resolved by model +
        /// serial against fresh enumeration at load time; None when the
        /// source is absent or ambiguous — preview/recovery then fail
        /// truthfully per node, browsing keeps working).
        device: Option<SendPtr<abi::CrUiDevice>>,
    },
}

/// Terminal latch written by the run thread exactly once.
#[derive(Clone)]
pub struct Terminal {
    pub status: &'static str, // 'completed' | 'cancelled' | 'failed'
    pub engine_code: u32,     // run() return code (0 / 0x50020005 / other)
    pub completed_at: String,
    /// Exact terminal counters (cr_ui_scan_get_stats after run returns).
    pub files_found: u64,
    pub data_found_bytes: u64,
    /// W1c: the completion auto-save outcome (stats.autosaved; the per-attempt
    /// rc rode CR_EVT_AUTOSAVED).
    pub autosaved: bool,
}

/// One lost-partition candidate row: the event's LBA joined with the N-061
/// log-line geometry (sectors/fs/score). `sectors == None` when the geometry
/// line was not seen (the log seam is always installed, so this is defensive
/// only) — such rows are countable but not applicable (the apply surface
/// needs full geometry).
#[derive(Clone, Copy)]
pub struct LostCandidateRow {
    pub lba: u64,
    pub sectors: Option<u64>,
    pub fs: Option<u32>,
    pub score: Option<u32>,
}

/// The single live-progress cell. The emitter thread is the ONLY writer
/// (event-mapping.md marshaling rule); `scan_state` reads under the lock.
pub struct ScanLive {
    pub stage: &'static str,
    pub stage_progress: f64,
    pub overall: f64, // clamped monotonic (engine guarantee + bridge defense)
    pub bytes_scanned: u64,
    pub bytes_total: u64,
    pub files_found: u64,
    /// G-3: no live found-bytes counter exists in telemetry — 0 live,
    /// exact value latched at completion.
    pub data_found_bytes: u64,
    pub errors: u32,
    pub bad_sectors: Vec<(u64, u64)>, // (startByte, sizeBytes), bounded
    pub lost_candidates: Vec<LostCandidateRow>, // bounded, count-exact
    pub fs_check_errors: u32,
    pub repair_summary: Option<String>, // CR_EVT_REPAIR outcome line (log surface)
    pub last_heartbeat_ms: Option<u64>, // engine watchdog liveness (G-8 synthesis)
    pub last_progress_ms: u64,
    pub save_permille: u32,
    /// W1c: the last CR_EVT_AUTOSAVED rc (0 = saved); None before the event.
    pub autosave_rc: Option<u64>,
    /// Token-call transient ('pausing' | 'resuming') with an expiry —
    /// cleared by the emitter when telemetry reflects the new state.
    pub transient: Option<(&'static str, u64 /*expiry_unix_ms*/)>,
    pub last_throughput: f64,
    pub last_eta_seconds: Option<u64>,
}

impl ScanLive {
    pub fn new(bytes_total: u64) -> Self {
        ScanLive {
            stage: "volume-analysis",
            stage_progress: 0.0,
            overall: 0.0,
            bytes_scanned: 0,
            bytes_total,
            files_found: 0,
            data_found_bytes: 0,
            errors: 0,
            bad_sectors: Vec::new(),
            lost_candidates: Vec::new(),
            fs_check_errors: 0,
            repair_summary: None,
            last_heartbeat_ms: None,
            last_progress_ms: 0,
            save_permille: 0,
            autosave_rc: None,
            transient: None,
            last_throughput: 0.0,
            last_eta_seconds: None,
        }
    }
}

/// One row of the per-generation metadata cache (index-addressed, matching
/// `cr_ui_snapshot_node` indices). Built in ONE DFS pass (parents precede
/// children in pre-order, so full paths assemble incrementally — O(total
/// path bytes), never O(n x depth)).
#[derive(Clone)]
pub struct NodeMeta {
    pub node_id: u64,
    pub parent: u32, // CR_UI_SNAP_NONE preserved for roots
    pub name: String,
    pub path: String, // '/'-joined from the snapshot root; root row: ""
    pub is_dir: bool,
    pub size: u64,
    pub modified_ft: u64,
    pub created_ft: u64,
    pub accessed_ft: u64,
    pub origin: u32,     // 0 structured / 1 carved / 2 lost / 3 virtual
    pub deleted: u32,    // 0 active / 1 deleted / 2 partial / 3 damaged
    pub confidence: u32, // ordinal 0..4
    pub grade: u32,      // 0 low / 1 average / 2 high (S-023)
    pub extent_count: u32,
    pub hash_valid: u32,
    /// 1 when the engine's twin/overlap rule flagged this node.
    pub overlap: u32,
    pub category: u8, // 0 dirs / bits 1..8 per cr_ui.h category_mask
    /// Subtree aggregates (files only; dirs accumulate in the reverse pass).
    pub file_count: u64,
    pub tree_size: u64,
    pub child_count: u32,
}

impl NodeMeta {
    /// contracts/type-maps.md §4 origin map.
    pub fn origin_label(&self) -> &'static str {
        match (self.origin, self.deleted) {
            (0, 0) => "existing",
            (0, _) => "deleted",
            (1, _) => "raw",
            (2, _) => "lost-path",
            _ => "unsaved",
        }
    }

    pub fn confidence_label(&self) -> &'static str {
        match self.confidence {
            4 => "excellent",
            3 => "good",
            2 => "fair",
            1 => "poor",
            _ => "unknown",
        }
    }

    pub fn modified_iso(&self) -> Option<String> {
        iso_from_filetime(self.modified_ft)
    }
}

/// The cached snapshot + its metadata, one generation at a time per entry.
pub struct SnapshotCache {
    pub gen: u64,
    pub handle: SendPtr<abi::CrUiSnapshot>,
    pub partial: bool,
    pub node_count: u32,
    pub metas: Option<Vec<NodeMeta>>,
    pub last_refresh_ms: i64,
    /// Terminal scans never re-snapshot (the tree is stable).
    pub frozen: bool,
}

impl Drop for SnapshotCache {
    fn drop(&mut self) {
        let p = self.handle.get();
        if !p.is_null() {
            unsafe { abi::cr_ui_snapshot_close(p) };
        }
    }
}

pub struct ScanEntry {
    pub scan_id: String,
    pub kind: ScanKind,
    /// The resolved target (contract-shaped Device or Volume JSON) the UI
    /// showed when the scan started — returned verbatim by getScanState.
    pub target_json: serde_json::Value,
    /// Owning device id + volume id (isSourceOfActiveScan + destination
    /// overlap checks). volume_id is None for whole-disk targets.
    pub device_id: String,
    pub volume_id: Option<String>,
    /// params echoed back by getScanState (ScanParams shape).
    pub params_json: serde_json::Value,
    /// G-11 view scoping: normalized lowercase '/'-joined prefix (no drive
    /// letter); None = whole volume.
    pub path_scope: Option<String>,
    pub started_at: String,
    pub live: Arc<Mutex<ScanLive>>,
    pub terminal: Arc<Mutex<Option<Terminal>>>,
    /// True once at least one snapshot exists (recoverableDuringScan).
    pub snapshot_exists: Arc<AtomicBool>,
    /// The boxed sink sender handed to the engine as the event-fn user
    /// pointer (freed by Drop after unsubscribing; it also keeps the
    /// channel open for the emitter).
    pub sink_user: SendPtr<SyncSender<abi::CrUiEvent>>,
    pub snap: Mutex<SnapshotCache>,
    /// Job ids created against this entry (close guard: the borrowed device
    /// must outlive every recovery; scan_close refuses while jobs are live).
    pub jobs: Mutex<Vec<String>>,
}

impl ScanEntry {
    pub fn is_terminal(&self) -> bool {
        self.terminal.lock().map(|t| t.is_some()).unwrap_or(false)
    }

    pub fn device(&self) -> *mut abi::CrUiDevice {
        match &self.kind {
            ScanKind::Live { device, .. } => device.get(),
            ScanKind::Session { device, .. } => device
                .as_ref()
                .map(|d| d.get())
                .unwrap_or(std::ptr::null_mut()),
        }
    }

    pub fn scan_handle(&self) -> *mut abi::CrUiScan {
        match &self.kind {
            ScanKind::Live { scan, .. } => scan.get(),
            ScanKind::Session { .. } => std::ptr::null_mut(),
        }
    }

    pub fn session_handle(&self) -> *mut abi::CrUiSession {
        match &self.kind {
            ScanKind::Live { .. } => std::ptr::null_mut(),
            ScanKind::Session { session, .. } => session.get(),
        }
    }
}

impl Drop for ScanEntry {
    fn drop(&mut self) {
        // Unsubscribe FIRST so no engine thread can touch the sink box after
        // this point (scan_close also refuses while busy — double lock).
        if let ScanKind::Live { scan, .. } = &self.kind {
            unsafe {
                abi::cr_ui_scan_set_event_fn(scan.get(), None, std::ptr::null_mut());
            }
        }
        // Free the leaked sink Sender (Box::from_raw under the SendPtr).
        let p = self.sink_user.get();
        if !p.is_null() {
            unsafe { drop(Box::from_raw(p)) };
            self.sink_user = SendPtr::new(std::ptr::null_mut());
        }
        // Order per the engine's lifetime rules: snapshot cache closes first
        // (its own Drop), then scan/session, then the borrowed device.
        match &self.kind {
            ScanKind::Live { scan, device, owns_device } => {
                unsafe { abi::cr_ui_scan_close(scan.get()) };
                if *owns_device {
                    unsafe { abi::cr_ui_device_close(device.get()) };
                }
            }
            ScanKind::Session { session, device } => {
                unsafe { abi::cr_ui_session_close(session.get()) };
                if let Some(d) = device {
                    unsafe { abi::cr_ui_device_close(d.get()) };
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Job registry
// ---------------------------------------------------------------------------

/// The single live-progress cell for a recovery job (emitter is the writer).
pub struct JobLive {
    pub status: &'static str, // validating | running | completed | completed-partial | cancelled | failed
    pub overall: f64,
    pub bytes_done: u64,
    pub bytes_total: u64,
    pub done_count: u64,
    pub failed_count: u64,
    pub skipped_count: u64,
    pub total_count: u64,
    pub current_file: Option<String>,
    pub verifying: bool,
    pub last_throughput: f64,
    pub eta_seconds: Option<u64>,
}

impl JobLive {
    pub fn new(total_count: u64) -> Self {
        JobLive {
            status: "validating",
            overall: 0.0,
            bytes_done: 0,
            bytes_total: 0,
            done_count: 0,
            failed_count: 0,
            skipped_count: 0,
            total_count,
            current_file: None,
            verifying: false,
            last_throughput: 0.0,
            eta_seconds: None,
        }
    }
}

pub struct JobEntry {
    pub job_id: String,
    pub scan_id: String,
    pub recovery: SendPtr<abi::CrUiRecovery>,
    pub destination: String,
    pub options_json: serde_json::Value,
    /// Explicit selection node ids; empty for recover-all.
    pub node_ids: Vec<String>,
    pub started_at: String,
    pub finished: Mutex<Option<(&'static str, String)>>,
    pub live: Arc<Mutex<JobLive>>,
    /// The boxed progress-fn sender handed to the engine (keeps the sample
    /// channel open for the emitter; freed on Drop).
    pub sink_user: SendPtr<SyncSender<(u64, u64)>>,
}

impl Drop for JobEntry {
    fn drop(&mut self) {
        unsafe { abi::cr_ui_recovery_close(self.recovery.get()) };
        // Free the leaked progress-fn Sender box.
        let p = self.sink_user.get();
        if !p.is_null() {
            unsafe { drop(Box::from_raw(p)) };
        }
        // The device is NOT closed here — it belongs to the scan entry.
    }
}

// ---------------------------------------------------------------------------
// BridgeState
// ---------------------------------------------------------------------------

pub struct BridgeState {
    pub scans: Mutex<HashMap<String, Arc<ScanEntry>>>,
    pub jobs: Mutex<HashMap<String, Arc<JobEntry>>>,
    /// Pending consent gates, keyed by the echo-token requestId — BOTH
    /// families (plan §4.1: repair consents belong to a scan, apply consents
    /// to a device; the respond path is the shared echo token). Entries are
    /// removed by the sink once the gate resolves — a resolved-then-
    /// unregistered request refuses consent-not-found on any late respond
    /// (the truthful stale answer).
    pub consents: Mutex<HashMap<String, PendingConsent>>,
    /// Devices with an apply in flight (single-flight: a second apply while
    /// a consent pends is the typed repair-consent-required refusal).
    pub apply_in_flight: Mutex<std::collections::HashSet<String>>,
    pub next_scan: AtomicU64,
    pub next_job: AtomicU64,
    pub next_consent: AtomicU64,
    pub next_apply_consent: AtomicU64,
    pub sessions_dir: PathBuf,
}

/// Which consent family a pending gate belongs to.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum ConsentFamily {
    /// Pre-scan repair consent (S-008) — keyed by scan id.
    Repair,
    /// Lost-partition apply consent (S-029) — keyed by device id.
    Apply,
}

/// One pending consent request in the registry (bridge::consent owns the
/// flow; this is the state-side record so respond/cancel/state can reach
/// the gate without touching the FFI pipe).
pub struct PendingConsent {
    pub family: ConsentFamily,
    /// Repair: the scan id. Apply: the device id.
    pub owner_id: String,
    /// The contract-shaped `consent-request` / `apply-consent-request`
    /// payload — re-hydration truth for the pending surfaces.
    pub payload: serde_json::Value,
    pub gate: Arc<crate::bridge::consent::ConsentGate>,
}

impl BridgeState {
    pub fn new() -> Self {
        BridgeState {
            scans: Mutex::new(HashMap::new()),
            jobs: Mutex::new(HashMap::new()),
            consents: Mutex::new(HashMap::new()),
            apply_in_flight: Mutex::new(std::collections::HashSet::new()),
            next_scan: AtomicU64::new(1),
            next_job: AtomicU64::new(1),
            next_consent: AtomicU64::new(1),
            next_apply_consent: AtomicU64::new(1),
            sessions_dir: sessions_dir(),
        }
    }

    pub fn mint_scan_id(&self) -> String {
        format!("scan-{}", self.next_scan.fetch_add(1, Ordering::Relaxed))
    }

    pub fn mint_job_id(&self) -> String {
        format!("job-{}", self.next_job.fetch_add(1, Ordering::Relaxed))
    }

    /// The consent echo tokens — globally unique, never derived from
    /// anything the UI could guess into authority (plan §4.5). The two
    /// families carry distinct prefixes so a token is self-describing.
    pub fn mint_consent_id(&self) -> String {
        format!("consent-{}", self.next_consent.fetch_add(1, Ordering::Relaxed))
    }

    pub fn mint_apply_consent_id(&self) -> String {
        format!("apply-{}", self.next_apply_consent.fetch_add(1, Ordering::Relaxed))
    }

    /// Look up a scan entry (cloned Arc — safe to use after releasing the
    /// registry lock).
    pub fn scan_entry_arc(&self, scan_id: &str) -> BridgeResult<Arc<ScanEntry>> {
        let guard = self.scans.lock().map_err(|_| poisoned())?;
        guard
            .get(scan_id)
            .cloned()
            .ok_or_else(|| {
                BridgeError::bridge(
                    code::SCAN_NOT_FOUND,
                    format!("scan {scan_id} is not registered (unknown or closed id)"),
                )
            })
    }

    pub fn job_entry_arc(&self, job_id: &str) -> BridgeResult<Arc<JobEntry>> {
        let guard = self.jobs.lock().map_err(|_| poisoned())?;
        guard
            .get(job_id)
            .cloned()
            .ok_or_else(|| {
                BridgeError::bridge(
                    code::UNKNOWN,
                    format!("job {job_id} is not registered"),
                )
            })
    }

    /// Register a pending consent (the consent sink; lock released before
    /// the owning thread blocks on the gate — §4.5 deadlock rule).
    pub fn register_consent(&self, request_id: &str, pc: PendingConsent) {
        if let Ok(mut c) = self.consents.lock() {
            c.insert(request_id.to_string(), pc);
        }
    }

    /// Remove a resolved request (the sink, after the gate answers — a
    /// late double-respond then hits consent-not-found via the miss path).
    pub fn unregister_consent(&self, request_id: &str) {
        if let Ok(mut c) = self.consents.lock() {
            c.remove(request_id);
        }
    }

    /// Force-deny every pending REPAIR consent belonging to a scan (cancel
    /// path + run-tail hygiene). Idempotent per gate; entries stay until the
    /// sink unregisters them (its wait has to wake first).
    pub fn close_scan_consents(&self, scan_id: &str) {
        if let Ok(c) = self.consents.lock() {
            for pc in c.values() {
                if pc.family == ConsentFamily::Repair && pc.owner_id == scan_id {
                    pc.gate.close();
                }
            }
        }
    }

    /// Force-deny every pending APPLY consent belonging to a device
    /// (teardown hygiene; the modal's own Deny is the normal path).
    pub fn close_device_consents(&self, device_id: &str) {
        if let Ok(c) = self.consents.lock() {
            for pc in c.values() {
                if pc.family == ConsentFamily::Apply && pc.owner_id == device_id {
                    pc.gate.close();
                }
            }
        }
    }

    /// The pending consent payload for a scan (scan_state re-hydration),
    /// or None when nothing pends.
    pub fn pending_consent(&self, scan_id: &str) -> Option<serde_json::Value> {
        self.consents
            .lock()
            .ok()
            .and_then(|c| {
                c.values()
                    .find(|pc| pc.family == ConsentFamily::Repair && pc.owner_id == scan_id)
                    .map(|pc| pc.payload.clone())
            })
    }

    /// The pending APPLY consent payload for a device (re-hydration for the
    /// lost-partition screen), or None when nothing pends.
    pub fn pending_apply_consent(&self, device_id: &str) -> Option<serde_json::Value> {
        self.consents
            .lock()
            .ok()
            .and_then(|c| {
                c.values()
                    .find(|pc| pc.family == ConsentFamily::Apply && pc.owner_id == device_id)
                    .map(|pc| pc.payload.clone())
            })
    }
}

fn poisoned() -> BridgeError {
    BridgeError::bridge(code::UNKNOWN, "bridge registry lock poisoned")
}

/// Session store (contracts/README.md): ProgramData in installed mode;
/// `.\sessions\` beside the exe in portable mode — decided by the portable
/// marker probe (util::portable_mode, NG-18; the pre-fix ProgramData env-var
/// check never detected portable builds — the variable is always set on
/// Windows, so portable sessions silently landed in ProgramData).
fn sessions_dir() -> PathBuf {
    if super::util::portable_mode() {
        let local = std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|d| d.join("sessions")))
            .unwrap_or_else(|| PathBuf::from("sessions"));
        let _ = std::fs::create_dir_all(&local);
        return local;
    }
    if let Ok(pd) = std::env::var("ProgramData") {
        if !pd.is_empty() {
            let dir = PathBuf::from(pd)
                .join("HeicFast")
                .join("Recovery Wizard")
                .join("sessions");
            let _ = std::fs::create_dir_all(&dir);
            return dir;
        }
    }
    let local = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.join("sessions")))
        .unwrap_or_else(|| PathBuf::from("sessions"));
    let _ = std::fs::create_dir_all(&local);
    local
}

// ---------------------------------------------------------------------------
// Snapshot refresh + metadata pass
// ---------------------------------------------------------------------------

pub const SNAPSHOT_REFRESH_MIN_MS: i64 = 1500;

/// Refresh policy: live scans re-snapshot at a bounded cadence; terminal
/// scans freeze; session entries take ONE stable snapshot. Returns the
/// current generation. The registry lock is held only to fetch the entry.
pub fn refresh_snapshot(state: &BridgeState, scan_id: &str) -> BridgeResult<u64> {
    let entry = state.scan_entry_arc(scan_id)?;
    let terminal = entry.is_terminal();
    let now = super::util::now_unix_ms();
    let mut snap = entry.snap.lock().map_err(|_| poisoned())?;

    let fresh_enough = now.saturating_sub(snap.last_refresh_ms) < SNAPSHOT_REFRESH_MIN_MS;
    if snap.frozen || (fresh_enough && !snap.handle.is_null()) {
        return Ok(snap.gen);
    }
    if let ScanKind::Session { session, .. } = &entry.kind {
        if !snap.handle.is_null() {
            snap.frozen = true;
            return Ok(snap.gen);
        }
        let mut out: *mut abi::CrUiSnapshot = std::ptr::null_mut();
        super::error::check(
            unsafe { abi::cr_ui_session_snapshot(session.get(), &mut out) },
            "cr_ui_session_snapshot",
        )?;
        install_snapshot(&mut snap, out, true, now);
        entry.snapshot_exists.store(true, Ordering::Release);
        return Ok(snap.gen);
    }
    let scan_ptr = entry.scan_handle();
    if scan_ptr.is_null() {
        return Ok(snap.gen);
    }
    let mut out: *mut abi::CrUiSnapshot = std::ptr::null_mut();
    super::error::check(
        unsafe { abi::cr_ui_scan_snapshot(scan_ptr, &mut out) },
        "cr_ui_scan_snapshot",
    )?;
    install_snapshot(&mut snap, out, terminal, now);
    entry.snapshot_exists.store(true, Ordering::Release);
    Ok(snap.gen)
}

fn install_snapshot(
    snap: &mut SnapshotCache,
    fresh: *mut abi::CrUiSnapshot,
    terminal: bool,
    now: i64,
) {
    let mut count: u32 = 0;
    let mut partial: u32 = 1;
    unsafe {
        abi::cr_ui_snapshot_node_count(fresh, &mut count);
        abi::cr_ui_snapshot_partial(fresh, &mut partial);
    }
    // Replacing the generation closes the old handle in its Drop — safe:
    // every meta pass and enqueue against it finished under this mutex.
    *snap = SnapshotCache {
        gen: snap.gen + 1,
        handle: SendPtr::new(fresh),
        partial: partial != 0,
        node_count: count,
        metas: None,
        last_refresh_ms: now,
        frozen: terminal,
    };
}

/// Build (or reuse) the metadata cache for the entry's CURRENT generation.
/// Runs under the entry's snapshot mutex only — never the registry lock.
pub fn ensure_metas(entry: &ScanEntry) -> BridgeResult<()> {
    let mut snap = entry.snap.lock().map_err(|_| poisoned())?;
    if snap.metas.is_some() || snap.handle.is_null() {
        return Ok(());
    }
    let handle = snap.handle.get();
    let count = snap.node_count as usize;

    // DFS pre-order: parents precede children — paths assemble in O(n).
    let order = super::util::snapshot_dfs(handle)?;
    let mut metas: Vec<NodeMeta> = Vec::with_capacity(count);
    for _ in 0..count {
        metas.push(placeholder_meta());
    }
    for &idx in &order {
        if idx as usize >= count {
            continue; // defensive: engine index contract says < count
        }
        let (info, name) = super::util::snapshot_node(handle, idx)?;
        let parent = info.parent;
        let path = if parent == abi::CR_UI_SNAP_NONE || parent as usize >= count {
            String::new() // volume root / virtual root row
        } else {
            let parent_path = metas[parent as usize].path.clone();
            if parent_path.is_empty() {
                name.clone()
            } else {
                format!("{parent_path}/{name}")
            }
        };
        let is_dir = info.attributes & 0x10 != 0;
        let category = if is_dir {
            0
        } else {
            category_for_extension(&extension_of(&name))
        };
        metas[idx as usize] = NodeMeta {
            node_id: info.node_id,
            parent,
            name,
            path,
            is_dir,
            size: info.logical_size,
            modified_ft: info.modified,
            created_ft: info.created,
            accessed_ft: info.accessed,
            origin: info.origin,
            deleted: info.deleted,
            confidence: info.confidence,
            grade: info.grade,
            extent_count: info.extent_count,
            hash_valid: info.hash_valid,
            overlap: info.overlap,
            category,
            file_count: if is_dir { 0 } else { 1 },
            tree_size: if is_dir { 0 } else { info.logical_size },
            child_count: 0,
        };
    }
    // Direct-child counts, then reverse DFS folds subtree aggregates.
    for i in 0..count {
        let parent = metas[i].parent;
        if parent != abi::CR_UI_SNAP_NONE && (parent as usize) < count {
            metas[parent as usize].child_count += 1;
        }
    }
    for &idx in order.iter().rev() {
        if idx as usize >= count {
            continue;
        }
        let (fc, ts) = (metas[idx as usize].file_count, metas[idx as usize].tree_size);
        let parent = metas[idx as usize].parent;
        if parent != abi::CR_UI_SNAP_NONE && (parent as usize) < count {
            metas[parent as usize].file_count += fc;
            metas[parent as usize].tree_size += ts;
        }
    }
    snap.metas = Some(metas);
    Ok(())
}

fn placeholder_meta() -> NodeMeta {
    NodeMeta {
        node_id: 0,
        parent: abi::CR_UI_SNAP_NONE,
        name: String::new(),
        path: String::new(),
        is_dir: false,
        size: 0,
        modified_ft: 0,
        created_ft: 0,
        accessed_ft: 0,
        origin: 0,
        deleted: 0,
        confidence: 0,
        grade: 0,
        extent_count: 0,
        hash_valid: 0,
        overlap: 0,
        category: 0,
        file_count: 0,
        tree_size: 0,
        child_count: 0,
    }
}

/// Lowercased extension WITHOUT the dot ("jpg"); "" when the name carries
/// none (the engine's classification treats unknown -> Other).
pub fn extension_of(name: &str) -> String {
    match name.rfind('.') {
        Some(at) if at + 1 < name.len() => name[at + 1..].to_lowercase(),
        _ => String::new(),
    }
}

/// Clone the metadata cache (payload assembly needs owned rows sometimes).
pub fn metas_snapshot(state: &BridgeState, scan_id: &str) -> BridgeResult<(Arc<ScanEntry>, Vec<NodeMeta>, bool)> {
    refresh_snapshot(state, scan_id)?;
    let entry = state.scan_entry_arc(scan_id)?;
    ensure_metas(&entry)?;
    // Extract under the lock, then release before moving `entry` out.
    let (metas, partial) = {
        let snap = entry.snap.lock().map_err(|_| poisoned())?;
        let metas = snap
            .metas
            .clone()
            .ok_or_else(|| BridgeError::bridge(code::UNKNOWN, "metadata cache missing after build"))?;
        (metas, snap.partial)
    };
    Ok((entry, metas, partial))
}

/// Session-info label composition used by sessions list/save meta rows
/// (contracts/type-maps.md §9): label_full (N-090) + model when present.
pub fn session_target_label(info: &abi::CrUiSessionInfo) -> String {
    let label = {
        let full = super::util::fixed_str(&info.label_full);
        if full.is_empty() {
            super::util::fixed_str(&info.label)
        } else {
            full
        }
    };
    let model = super::util::fixed_str(&info.model);
    if model.is_empty() {
        label
    } else if label.is_empty() {
        model
    } else {
        format!("{label} ({model})")
    }
}

/// How long without heartbeats/progress before `deviceConnected` reads
/// false (2x the engine's 5 s watchdog window — event-mapping.md §G-8).
pub const DISCONNECT_AFTER_MS: u64 = 10_000;
