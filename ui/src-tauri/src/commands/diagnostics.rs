//! commands/diagnostics.rs — the observability surface
//! (contracts/command-surface.json; type-maps §12).
//!
//! Every command here surfaces an engine export the §4.7 DISABLE register
//! held back, each now with the corpus citation that contradicts its
//! "no reference screen" rationale (worklog/lane-c/API-EXPANSION.md):
//!
//! - `scan_telemetry` + checkpoint count — the live stage axes behind the
//!   og's "Reading sector: X/Y" / "Advanced Scanning, N%" status family
//!   (UIUX/dynamic-ground-truth--scan-phase-captured.md:88-105).
//! - `scan_assistant_stats` — the RecoveryAssistant block + the
//!   CRecoverability verdict (UIUX/30:146-149, DRWUI-subsystems:9-52):
//!   per-category counts, the matched/total gauge, the small-result
//!   heuristic the og forces to zero (total < 100).
//! - `carve_catalog` — the og's EUGetAnalyserList surface
//!   (RAWSearchDLL exports; rawsearch-carving--deep-dive.md section 1):
//!   both layers (grammar validators + raw-sign prefilters), the
//!   immutable registry, any thread.
//! - `scan_carve_tally` — the og's GetRawUserInfoMap (RAWSearchDLL
//!   :72213-72221; the post-scan "found N .jpg / M .png" report,
//!   DRWUI :227538-227625), slots over the unified format-id space plus
//!   the user-sign aggregate.
//! - `disk_map` — the AnalysisGridWidget data producer (UIUX/08:11,
//!   UIUX/13:47): allocation/coverage/found/confidence overlays + bad
//!   ranges. The bridge bounds the cell budget (payload honesty: a 1 TiB
//!   volume at the engine's 64 KiB default would be 16 M cells/array —
//!   the map view stays bounded, caller-tunable).
//! - `device_transport` — the CLI's "[transport]" truth plus the M-030
//!   last-transport-error advisory (the presentation contract for hosts
//!   that surface transport diagnostics).
//! - `session_regrade` — the og's FUN_1401d7110 semantics (per-selection
//!   re-grade; grade-1-only eligibility, sticky 0/2 — S-023).
//!
//! Threading: all engine calls are synchronous-on-caller-thread — every
//! command runs on the blocking pool; the registry reads take the same
//! locks the existing commands take (the disk_map snapshot path holds
//! entry.snap ACROSS the engine call by state.rs's own design: generation
//! swaps serialize on that mutex, so the handle cannot be closed
//! mid-pass — the never-mid-pass rule the metadata build follows).

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{check, code, BridgeError, BridgeResult};
use crate::bridge::state::{BridgeState, ScanKind};
use crate::bridge::util::fixed_str;

// ---------------------------------------------------------------------------
// scan_telemetry — the live stage axes (telemetry was events-internal
// only; the UI gets the full structure now)
// ---------------------------------------------------------------------------

fn stage_tel_json(st: &abi::CrUiStageTel) -> Value {
    json!({
        "permille": st.permille,
        "unitsDone": st.units_done,
        "unitsTotal": st.units_total,
        "bytesDone": st.bytes_done,
        "elapsedMs": st.elapsed_ms,
        "completed": st.completed,
    })
}

#[tauri::command]
pub async fn scan_telemetry(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let scan_ptr = entry.scan_handle();
        if scan_ptr.is_null() {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                "telemetry is a live-scan surface — a loaded session's final counters ride \
                 session_meta (the scan handle is what carries live stage state)",
            ));
        }
        let mut tel: abi::CrUiTelemetry = unsafe { std::mem::zeroed() };
        check(
            unsafe { abi::cr_ui_scan_telemetry(scan_ptr, &mut tel) },
            "cr_ui_scan_telemetry",
        )?;
        // Best-effort companions (availability flags, never silent zeros):
        // scan_get_stats is the FINAL-counter surface (valid at/after run
        // completion); checkpoint_count is the internal axis the autosave
        // flow surfaces instead — each is included only when the engine
        // answered for THIS handle state.
        let mut stats: abi::CrUiScanStats = unsafe { std::mem::zeroed() };
        let stats_ok = unsafe { abi::cr_ui_scan_get_stats(scan_ptr, &mut stats) } == abi::CR_UI_OK;
        let mut checkpoints: u64 = 0;
        let checkpoints_ok =
            unsafe { abi::cr_ui_scan_checkpoint_count(scan_ptr, &mut checkpoints) }
                == abi::CR_UI_OK;
        let mut body = json!({
            "scanId": scan_id,
            "overallPermille": tel.overall_permille,
            "running": tel.running,
            "paused": tel.paused,
            "cancelled": tel.cancelled,
            "elapsedMs": tel.elapsed_ms,
            "filesFound": tel.files_found,
            "deletedFound": tel.deleted_found,
            "carvedFound": tel.carved_found,
            "lostPartitions": tel.lost_partitions,
            "bytesScanned": tel.bytes_scanned,
            "bytesFound": tel.bytes_found,
            "errorRanges": tel.error_ranges,
            "stages": {
                "quick": stage_tel_json(&tel.quick),
                "deep": stage_tel_json(&tel.deep),
                "carve": stage_tel_json(&tel.carve),
            },
            "statsAvailable": stats_ok,
            "checkpointsAvailable": checkpoints_ok,
        });
        if stats_ok {
            body["finalStats"] = json!({
                "filesFound": stats.files_found,
                "deletedFound": stats.deleted_found,
                "carvedFound": stats.carved_found,
                "lostPartitions": stats.lost_partitions,
                "bytesScanned": stats.bytes_scanned,
                "bytesUnreadable": stats.bytes_unreadable,
                "elapsedMs": stats.elapsed_ms,
                "errorRanges": stats.error_ranges,
                "checkRan": stats.check_ran,
                "checkErrors": stats.check_errors,
                "repairRan": stats.repair_ran,
                "repairApplied": stats.repair_applied,
                "autosaved": stats.autosaved,
            });
        }
        if checkpoints_ok {
            body["checkpoints"] = json!(checkpoints);
        }
        Ok(body)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// scan_assistant_stats — the S-023 recoverability counters
// ---------------------------------------------------------------------------

/// The category slot order (cr_ui.h: image video audio document archive
/// other email bookmarks — the og's RecoveryAssistant category block).
const CATEGORY_KEYS: [&str; 8] = [
    "image",
    "video",
    "audio",
    "document",
    "archive",
    "other",
    "email",
    "bookmarks",
];

#[tauri::command]
pub async fn scan_assistant_stats(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let scan_ptr = entry.scan_handle();
        if scan_ptr.is_null() {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                "the assistant statistics are the scan-completion grading pass — a loaded \
                 session has no scan handle (the ABI derives them at run completion)",
            ));
        }
        let mut st: abi::CrUiAssistantStats = unsafe { std::mem::zeroed() };
        check(
            unsafe { abi::cr_ui_scan_assistant(scan_ptr, &mut st) },
            "cr_ui_scan_assistant",
        )?;
        let categories: Value = serde_json::Map::from_iter(
            CATEGORY_KEYS
                .iter()
                .enumerate()
                .map(|(i, k)| (k.to_string(), json!(st.category_counts[i]))),
        )
        .into();
        Ok(json!({
            "scanId": scan_id,
            "totalFileCount": st.total_file_count,
            "rawCount": st.raw_count,
            "lostPartitionExistFileCount": st.lost_partition_exist_file_count,
            "lostPartitionDeleteFileCount": st.lost_partition_delete_file_count,
            "quickScanExistFileCount": st.quick_scan_exist_file_count,
            "quickScanDeleteFileCount": st.quick_scan_delete_file_count,
            "deepScanFileCount": st.deep_scan_file_count,
            "quickScanDeleteExtend": st.quick_scan_delete_extend,
            "categoryCounts": categories,
            // The og's gauge axes, verbatim: when total < 100 the og
            // forces matched to 0 (small-result heuristic — the gauge
            // never shows). The UI renders the verdict from these; the
            // bridge never invents one.
            "matched": st.matched,
            "total": st.total,
            "smallResult": st.small_result,
            "gradedCount": st.graded_count,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// carve_catalog — the static format registry (§4.7 row 16)
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn carve_catalog() -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(|| {
        let count = unsafe { abi::cr_ui_carve_format_count() };
        let mut formats: Vec<Value> = Vec::with_capacity(count as usize);
        for index in 0..count {
            let mut f: abi::CrUiCarveFormat = unsafe { std::mem::zeroed() };
            check(
                unsafe { abi::cr_ui_carve_format_info(index, &mut f) },
                "cr_ui_carve_format_info",
            )?;
            formats.push(json!({
                "index": index,
                "layer": f.layer, // 0 grammar validator, 1 raw-sign prefilter
                "ext": fixed_str(&f.ext),
                "minSizeBytes": f.min_size,
                "maxSizeBytes": f.max_size,
                "baseConfidence": f.base_confidence, // 0..1000
                "magicLen": f.magic_len,
                "magicOffset": f.magic_offset,
            }));
        }
        Ok(json!({ "formats": formats, "count": count }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// scan_carve_tally — the per-format found counts (§4.7 row 17)
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn scan_carve_tally(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let scan_ptr = entry.scan_handle();
        if scan_ptr.is_null() {
            return Err(BridgeError::bridge(
                code::SCAN_NOT_RUNNING,
                "the carve tally is a live-scan surface (cr_ui_scan_carve_tally takes the \
                 scan handle; a loaded session's counts ride its result rows)",
            ));
        }
        let slots = unsafe { abi::cr_ui_scan_carve_tally_count(scan_ptr) };
        let fmt_count = unsafe { abi::cr_ui_carve_format_count() };
        let mut rows: Vec<Value> = Vec::with_capacity(slots as usize);
        for index in 0..slots {
            let mut count: u64 = 0;
            check(
                unsafe { abi::cr_ui_scan_carve_tally(scan_ptr, index, &mut count) },
                "cr_ui_scan_carve_tally",
            )?;
            if index == fmt_count {
                // The aggregate user-sign slot (the og keys user signs by
                // dynamic extension strings — the fixed-table projection
                // is the honest aggregate).
                rows.push(json!({
                    "formatIndex": null,
                    "layer": null,
                    "ext": null,
                    "userSigns": true,
                    "count": count,
                }));
            } else {
                let mut f: abi::CrUiCarveFormat = unsafe { std::mem::zeroed() };
                check(
                    unsafe { abi::cr_ui_carve_format_info(index, &mut f) },
                    "cr_ui_carve_format_info",
                )?;
                rows.push(json!({
                    "formatIndex": index,
                    "layer": f.layer,
                    "ext": fixed_str(&f.ext),
                    "userSigns": false,
                    "count": count,
                }));
            }
        }
        Ok(json!({ "scanId": scan_id, "slots": rows, "slotCount": slots }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// disk_map — the AnalysisGridWidget data producer (§4.7 row 18)
// ---------------------------------------------------------------------------

/// The payload-honesty bound: cell arrays are JSON-number arrays; the
/// default view stays at UI-grid scale (the og's sector grid is a fixed
/// widget), the hard cap keeps a pathological caller from requesting a
/// multi-megabyte payload per overlay.
const DISK_MAP_DEFAULT_MAX_CELLS: u64 = 4096;
const DISK_MAP_HARD_MAX_CELLS: u64 = 65536;
/// The engine's own default granularity (cr_ui.h: cell_bytes = 0 picks
/// the engine cluster size or the 64 KiB default) — the bridge's auto
/// sizing never goes FINER than this floor, only coarser.
const DISK_MAP_CELL_FLOOR: u64 = 64 * 1024;

/// The bounded cell size for a view: the caller's request when it fits
/// the budget, else the auto size (total/max_cells rounded up to the
/// floor's granularity). Never below the engine's own floor.
fn bounded_cell(total: u64, requested: Option<u64>, max_cells: u64) -> u64 {
    let Some(req) = requested.filter(|c| *c > 0) else {
        return auto_cell(total, max_cells);
    };
    if total == 0 || total.div_ceil(req) <= max_cells {
        return req; // the caller's granularity fits the budget
    }
    auto_cell(total, max_cells) // past the budget: bump to satisfy the cap
}

fn auto_cell(total: u64, max_cells: u64) -> u64 {
    if total == 0 {
        return DISK_MAP_CELL_FLOOR;
    }
    total
        .div_ceil(max_cells)
        .max(DISK_MAP_CELL_FLOOR)
        .next_multiple_of(DISK_MAP_CELL_FLOOR)
}

fn geometry_json(g: &abi::CrUiDiskMapGeometry) -> Value {
    json!({
        "startByte": g.start_byte,
        "totalBytes": g.total_bytes,
        "cellBytes": g.cell_bytes,
        "cellCount": g.cell_count,
        "clusterBytes": g.cluster_bytes,
        "fsFamily": g.fs_family,
    })
}

#[tauri::command]
pub async fn disk_map(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    options: Value,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let want_start = options
            .get("startByte")
            .and_then(|v| v.as_u64())
            .unwrap_or(0);
        let want_total = options.get("totalBytes").and_then(|v| v.as_u64());
        let want_cell = options.get("cellBytes").and_then(|v| v.as_u64());
        let max_cells = options
            .get("maxCells")
            .and_then(|v| v.as_u64())
            .unwrap_or(DISK_MAP_DEFAULT_MAX_CELLS)
            .clamp(64, DISK_MAP_HARD_MAX_CELLS);

        match &entry.kind {
            ScanKind::Session { session, .. } => {
                let sess = session.get();
                // The session path: want is optional for sessions (the
                // engine defaults to the scanned slice); the caller's
                // window passes through with the same cell-budget bound.
                // Default total = the session's own slice extent.
                let mut info: abi::CrUiSessionInfo = unsafe { std::mem::zeroed() };
                check(
                    unsafe { abi::cr_ui_session_get_info(sess, &mut info) },
                    "cr_ui_session_get_info",
                )?;
                let total = want_total.unwrap_or(info.slice_bytes);
                let want = abi::CrUiDiskMapGeometry {
                    start_byte: want_start,
                    total_bytes: total,
                    cell_bytes: bounded_cell(total, want_cell, max_cells),
                    cell_count: 0,
                    cluster_bytes: 0,
                    fs_family: 0,
                };
                run_disk_map(sess, std::ptr::null_mut(), &want, &scan_id)
            }
            ScanKind::Live { .. } => {
                // The snapshot path: want is REQUIRED (snapshots are value
                // objects and carry no view). Default total = the scan's
                // own bytes_total. The entry.snap lock is held ACROSS the
                // engine call BY DESIGN — state.rs serializes generation
                // swaps on this mutex so the handle cannot be closed
                // mid-map (the never-mid-pass rule).
                let bytes_total = {
                    let live = entry
                        .live
                        .lock()
                        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "scan live lock"))?;
                    live.bytes_total
                };
                let total = want_total.unwrap_or(bytes_total);
                let want = abi::CrUiDiskMapGeometry {
                    start_byte: want_start,
                    total_bytes: total,
                    cell_bytes: bounded_cell(total, want_cell, max_cells),
                    cell_count: 0,
                    cluster_bytes: 0,
                    fs_family: 0,
                };
                let snap = entry
                    .snap
                    .lock()
                    .map_err(|_| BridgeError::bridge(code::UNKNOWN, "snapshot lock"))?;
                if snap.handle.get().is_null() {
                    return Err(BridgeError::bridge(
                        code::SCAN_NOT_RUNNING,
                        "no snapshot exists for this scan yet — the map view needs at least \
                         one tree generation (a live pre-start scan has none)",
                    ));
                }
                run_disk_map(std::ptr::null_mut(), snap.handle.get(), &want, &scan_id)
            }
        }
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// One disk-map call with the bad-ranges two-call pattern: the first leg
/// fills the overlays and reports the needed pair count; TRUNCATED with a
/// bigger need re-runs once with the exact buffer (each leg's overlays
/// are freed before the re-run — every leg owns what it filled).
fn run_disk_map(
    sess: *mut abi::CrUiSession,
    snap: *mut abi::CrUiSnapshot,
    want: *const abi::CrUiDiskMapGeometry,
    scan_id: &str,
) -> BridgeResult<Value> {
    let mut got: abi::CrUiDiskMapGeometry = unsafe { std::mem::zeroed() };
    let mut bad_pairs: Vec<u64> = vec![0; 64];
    let mut rc;
    loop {
        let mut alloc: *mut u8 = std::ptr::null_mut();
        let mut coverage: *mut u8 = std::ptr::null_mut();
        let mut found: *mut u8 = std::ptr::null_mut();
        let mut confidence: *mut u8 = std::ptr::null_mut();
        let mut bad_count: u32 = 0;
        rc = unsafe {
            abi::cr_ui_disk_map(
                sess,
                snap,
                want,
                &mut got,
                &mut alloc,
                &mut coverage,
                &mut found,
                &mut confidence,
                bad_pairs.as_mut_ptr(),
                (bad_pairs.len() / 2) as u32,
                &mut bad_count,
            )
        };
        if rc == abi::CR_UI_ERR_TRUNCATED && (bad_count as usize) * 2 > bad_pairs.len() {
            // The needed pair count exceeded the guess: free this leg's
            // overlays, size exactly, re-run once.
            unsafe {
                abi::cr_ui_disk_map_free(alloc);
                abi::cr_ui_disk_map_free(coverage);
                abi::cr_ui_disk_map_free(found);
                abi::cr_ui_disk_map_free(confidence);
            }
            bad_pairs = vec![0; bad_count as usize * 2];
            continue;
        }
        // Terminal leg (OK, a real failure, or a TRUNCATED that is not the
        // bad-pair sizing): own the arrays, copy out, free.
        let cell_count = got.cell_count as usize;
        let take = |p: *mut u8| -> Option<Vec<u8>> {
            if p.is_null() {
                None
            } else {
                Some(unsafe { std::slice::from_raw_parts(p, cell_count) }.to_vec())
            }
        };
        let allocation = take(alloc);
        let coverage_v = take(coverage);
        let found_v = take(found);
        let confidence_v = take(confidence);
        unsafe {
            abi::cr_ui_disk_map_free(alloc);
            abi::cr_ui_disk_map_free(coverage);
            abi::cr_ui_disk_map_free(found);
            abi::cr_ui_disk_map_free(confidence);
        }
        check(rc, "cr_ui_disk_map")?;
        let bad_ranges: Vec<Value> = bad_pairs[..(bad_count as usize) * 2]
            .chunks_exact(2)
            .map(|pair| json!({ "startByte": pair[0], "sizeBytes": pair[1] }))
            .collect();
        return Ok(json!({
            "scanId": scan_id,
            "geometry": geometry_json(&got),
            "allocation": allocation, // 0 unknown, 1 used, 2 free, 3 bad
            "coverage": coverage_v,   // 0 unscanned, 1 scanned
            "found": found_v,         // distinct files covering (capped 255)
            "confidence": confidence_v, // max Confidence ordinal 0..4
            "badRanges": bad_ranges,
        }));
    }
}

// ---------------------------------------------------------------------------
// device_transport — the transport truth + the M-030 advisory
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn device_transport(device_id: String) -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(move || {
        let (disk_n, letter): (Option<u32>, Option<i8>) =
            if let Some(rest) = device_id.strip_prefix("disk-") {
                (rest.parse().ok(), None)
            } else if let Some(rest) = device_id.strip_prefix("vol-") {
                let b = rest.as_bytes();
                if b.len() == 1 && b[0].is_ascii_alphabetic() {
                    (None, Some(b[0] as i8))
                } else {
                    (None, None)
                }
            } else {
                (None, None)
            };
        let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
        if let Some(n) = disk_n {
            check(
                unsafe { abi::cr_ui_open_disk(n, &mut dev) },
                "cr_ui_open_disk",
            )?;
        } else if let Some(l) = letter {
            check(
                unsafe { abi::cr_ui_open_volume(l, &mut dev) },
                "cr_ui_open_volume",
            )?;
        } else {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                format!("device id '{device_id}' is not a disk-N or vol-X target"),
            ));
        }
        // Two-call transport query (cr_ui.h: pass buf == NULL to learn the
        // needed size), then the advisory M-030 read — process-global,
        // never cleared, never gating.
        let mut needed: u32 = 0;
        check(
            unsafe { abi::cr_ui_device_transport(dev, std::ptr::null_mut(), 0, &mut needed) },
            "cr_ui_device_transport (size probe)",
        )?;
        let mut buf = vec![0i8; needed.max(1) as usize];
        check(
            unsafe { abi::cr_ui_device_transport(dev, buf.as_mut_ptr(), needed, &mut needed) },
            "cr_ui_device_transport",
        )?;
        let transport = unsafe { crate::bridge::util::cstr_to_string_lossy(buf.as_ptr()) };
        let last_err = unsafe { abi::cr_ui_last_transport_error() };
        unsafe { abi::cr_ui_device_close(dev) };
        Ok(json!({
            "deviceId": device_id,
            "transport": transport, // "direct" | "image" (session-96 truth)
            "lastTransportError": if last_err == 0 {
                Value::Null
            } else {
                json!(format!("{last_err:#010x}"))
            },
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// session_regrade — the per-selection re-grade (S-023; session trees)
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn session_regrade(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    node_ids: Vec<String>,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let entry = state.scan_entry_arc(&scan_id)?;
        let sess = entry.session_handle();
        if sess.is_null() {
            return Err(BridgeError::bridge(
                code::NOT_IMPLEMENTED,
                "the re-grade surface operates on session trees (cr_ui_session_regrade takes \
                 the session handle); a live scan's grades are the completion pass's own",
            ));
        }
        // Resolve the UI's snapshot-index id format (f<N>/d<N>) to engine
        // node_ids through the metadata cache — the same mapping the
        // recovery enqueue path uses.
        let mut ids: Vec<u64> = Vec::with_capacity(node_ids.len());
        {
            let snap = entry
                .snap
                .lock()
                .map_err(|_| BridgeError::bridge(code::UNKNOWN, "snapshot lock"))?;
            let metas = snap.metas.as_ref().ok_or_else(|| {
                BridgeError::bridge(code::UNKNOWN, "metadata cache missing for this session")
            })?;
            for id in &node_ids {
                let idx: usize = id.trim_start_matches(['f', 'd']).parse().map_err(|_| {
                    BridgeError::bridge(
                        code::SCAN_SCOPE_INVALID,
                        format!("node id '{id}' does not resolve in this tree"),
                    )
                })?;
                let m = metas.get(idx).ok_or_else(|| {
                    BridgeError::bridge(
                        code::SCAN_SCOPE_INVALID,
                        format!("node id '{id}' does not resolve in this tree"),
                    )
                })?;
                ids.push(m.node_id);
            }
        }
        let mut regraded: u32 = 0;
        check(
            unsafe {
                abi::cr_ui_session_regrade(sess, ids.as_ptr(), ids.len() as u32, &mut regraded)
            },
            "cr_ui_session_regrade",
        )?;
        Ok(json!({ "requested": ids.len(), "regraded": regraded }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
