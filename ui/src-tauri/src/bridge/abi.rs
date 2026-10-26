//! bridge/abi.rs — the cr_ui C-ABI binding.
//!
//! Pinned 1:1 from `engine/include/cr/cr_ui.h` (ABI version 2, the 78-export
//! surface). Every struct mirrors the header's DECLARATION ORDER under
//! `#[repr(C)]` — natural alignment reproduces the MSVC layout for both
//! sides, so no manual padding is ever written here. When the header gains
//! an appended field (the ABI policy is append-only within a version), this
//! file gains it in the same position and `contracts/validate.mjs` phase 2
//! keeps the command surface honest.
//!
//! Threading contract honored throughout the bridge (cr_ui.h ABI RULES):
//! scan/recovery run on the caller's thread; pause/cancel/telemetry and
//! snapshot capture are safe from any thread; progress callbacks fire on
//! engine worker threads, must not block, and carry NO cancellation
//! semantics. See docs/INTEGRATION-PLAN.md §4.4 and contracts/event-mapping.md.

// The ABI surface is PINNED 1:1 to engine/include/cr/cr_ui.h (the whole
// 78-export surface, not just the calls the bridge makes today). Unconsumed
// declarations are therefore EXPECTED: exports the product has no surface
// for yet (images, unlock aids, the standalone repair workbench - G-1's
// remaining follow-up (b), the query family the bridge-side filter replaced
// - G-7, resume/assistant) stay declared so the file remains the complete,
// diffable header binding. This allow is scoped to this module ONLY; dead
// LOGIC elsewhere is removed, not silenced.
#![allow(dead_code, clippy::missing_safety_doc)]

use std::ffi::c_void;

// ---- selected error codes (same numeric space as cr_error.h) ----------------

pub const CR_UI_OK: u32 = 0x0000_0000;
pub const CR_UI_ERR_INTERNAL: u32 = 0x5009_0001;
pub const CR_UI_ERR_BAD_PARAMETER: u32 = 0x5003_0003;
pub const CR_UI_ERR_CANCELLED: u32 = 0x5002_0005;
pub const CR_UI_ERR_TRUNCATED: u32 = 0x5003_0004;
pub const CR_UI_ERR_UNSUPPORTED: u32 = 0x5003_0008;
pub const CR_UI_ERR_NO_MEMORY: u32 = 0x5003_0009;
pub const CR_UI_ERR_BAD_CHECKSUM: u32 = 0x5003_0002;
pub const CR_UI_ERR_FVE_LOCKED: u32 = 0x5006_0001;

pub const CR_UI_ABI_VERSION: u32 = 2;

// ---- opaque handles ----------------------------------------------------------

#[repr(C)]
pub struct CrUiDevice {
    _private: [u8; 0],
}
#[repr(C)]
pub struct CrUiScan {
    _private: [u8; 0],
}
#[repr(C)]
pub struct CrUiSession {
    _private: [u8; 0],
}
#[repr(C)]
pub struct CrUiSnapshot {
    _private: [u8; 0],
}
#[repr(C)]
pub struct CrUiPreview {
    _private: [u8; 0],
}
#[repr(C)]
pub struct CrUiRecovery {
    _private: [u8; 0],
}

// ---- callbacks ----------------------------------------------------------------

pub type CrUiProgressFn =
    Option<unsafe extern "C" fn(user: *mut c_void, stage: u32, stage_permille: u32, overall_permille: u32, bytes_done: u64, bytes_total: u64, error_ranges: u32)>;

pub type CrUiRecoveryProgressFn =
    Option<unsafe extern "C" fn(user: *mut c_void, bytes_done: u64, bytes_total: u64)>;

// ---- typed event stream --------------------------------------------------------

pub const CR_EVT_SCAN_STAGE: u32 = 1;
pub const CR_EVT_SCAN_PROGRESS: u32 = 2;
pub const CR_EVT_FILES_FOUND: u32 = 3;
pub const CR_EVT_ERROR_RANGE: u32 = 4;
pub const CR_EVT_CHECKPOINT: u32 = 5;
pub const CR_EVT_LOST_CANDIDATE: u32 = 6;
pub const CR_EVT_REPAIR: u32 = 7;
pub const CR_EVT_HEARTBEAT: u32 = 8;
pub const CR_EVT_SAVE_PROGRESS: u32 = 9;
pub const CR_EVT_FS_CHECK: u32 = 10;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiEvent {
    pub r#type: u32,
    pub stage: u32,
    pub timestamp_ms: u64,
    pub bytes_done: u64,
    pub bytes_total: u64,
    pub stage_permille: u32,
    pub overall_permille: u32,
    pub files_found: u64,
    pub deleted_found: u64,
    pub carved_found: u64,
    pub err_start_bytes: u64,
    pub err_len_bytes: u64,
    pub error_ranges: u32,
    pub checkpoint_index: u64,
    pub candidate_lba: u64,
    pub repair_actions: u32,
    pub repair_outcome: u32,
    pub err_module: u32,
    pub err_operation: u32,
    pub err_attempts: u32,
    pub err_context_pad: u32,
    pub check_errors: u64,
    pub check_family: u32,
    pub check_pad: u32,
}

pub type CrUiEventFn = Option<unsafe extern "C" fn(user: *mut c_void, evt: *const CrUiEvent)>;

// ---- devices / enumeration -----------------------------------------------------

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiGeometry {
    pub total_bytes: u64,
    pub logical_sector: u32,
    pub physical_sector: u32,
    pub sector_count: u64,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiDiskInfo {
    pub number: u32,
    pub size_bytes: u64,
    pub logical_sector: u32,
    pub physical_sector: u32,
    pub removable: u32,
    pub layout: u32,
    pub table_ok: u32,
    pub model: [u8; 128],
    pub serial: [u8; 128],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiVolumeInfo {
    pub letter: u8,
    pub disk: u32,
    pub offset_on_disk: u64,
    pub bytes: u64,
    pub fs_name: [u8; 16],
    pub label: [u8; 64],
    pub bitlocker: u32,
}

// ---- scan lifecycle -------------------------------------------------------------

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiRepairPlan {
    pub detected: u32,
    pub fs_family: [u8; 8],
    pub action_count: u32,
    pub refused: u32,
    pub reason: [u8; 160],
    pub advisory_count: u32,
}

pub type CrUiRepairConsentFn =
    Option<unsafe extern "C" fn(user: *mut c_void, plan: *const CrUiRepairPlan) -> u32>;

#[repr(C)]
pub struct CrUiScanConfig {
    pub stage_mask: u32,
    pub whole_disk: u32,
    pub fs_hint: u32,
    pub quick_weight: u32,
    pub deep_weight: u32,
    pub carve_weight: u32,
    pub threads: u32,
    pub volume_label_utf8: *const i8,
    pub volume_model_utf8: *const i8,
    pub volume_serial_utf8: *const i8,
    pub volume_firmware_utf8: *const i8,
    pub volume_wwn_utf8: *const i8,
    pub volume_interface_utf8: *const i8,
    pub checkpoint_path_utf8: *const i8,
    pub progress: CrUiProgressFn,
    pub progress_user: *mut c_void,
    pub pre_scan_repair: u32,
    pub repair_consent: CrUiRepairConsentFn,
    pub repair_consent_user: *mut c_void,
    pub memory_warn_mb: u32,
    pub raw_category_name_utf8: *const i8,
    pub pre_scan_check: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiStageTel {
    pub permille: u32,
    pub units_done: u64,
    pub units_total: u64,
    pub bytes_done: u64,
    pub elapsed_ms: u64,
    pub completed: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiTelemetry {
    pub overall_permille: u32,
    pub quick: CrUiStageTel,
    pub deep: CrUiStageTel,
    pub carve: CrUiStageTel,
    pub files_found: u64,
    pub deleted_found: u64,
    pub carved_found: u64,
    pub lost_partitions: u64,
    pub bytes_scanned: u64,
    pub elapsed_ms: u64,
    pub error_ranges: u32,
    pub paused: u32,
    pub running: u32,
    pub cancelled: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiScanStats {
    pub files_found: u64,
    pub deleted_found: u64,
    pub carved_found: u64,
    pub lost_partitions: u64,
    pub bytes_scanned: u64,
    pub elapsed_ms: u64,
    pub cached_hits: u64,
    pub cached_misses: u64,
    pub error_ranges: u32,
    pub repair_ran: u32,
    pub repair_planned: u32,
    pub repair_applied: u32,
    pub repair_denied: u32,
    pub check_ran: u32,
    pub check_errors: u32,
    pub check_detected: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiAssistantStats {
    pub total_file_count: u64,
    pub raw_count: u64,
    pub lost_partition_exist_file_count: u64,
    pub lost_partition_delete_file_count: u64,
    pub quick_scan_exist_file_count: u64,
    pub quick_scan_delete_file_count: u64,
    pub deep_scan_file_count: u64,
    pub quick_scan_delete_extend: u64,
    pub category_counts: [u64; 8],
    pub matched: u64,
    pub total: u64,
    pub small_result: u32,
    pub graded_count: u64,
}

// ---- sessions -------------------------------------------------------------------

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiSessionInfo {
    pub fs_family: u32,
    pub stage_done_mask: u32,
    pub configured_mask: u32,
    pub slice_offset: u64,
    pub slice_bytes: u64,
    pub logical_sector: u32,
    pub physical_sector: u32,
    pub label: [u8; 32],
    pub scanned_bytes: u64,
    pub elapsed_ms: u64,
    pub model: [u8; 48],
    pub serial: [u8; 32],
    pub boot_hash: u32,
}

// ---- snapshot enumeration --------------------------------------------------------

pub const CR_UI_SNAP_NONE: u32 = 0xFFFF_FFFF;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiNodeInfo {
    pub parent: u32,
    pub first_child: u32,
    pub next_sibling: u32,
    pub prev_sibling: u32,
    pub node_id: u64,
    pub fs_kind: u32,
    pub record_ref: u64,
    pub origin: u32,
    pub deleted: u32,
    pub confidence: u32,
    pub grade: u32,
    pub overlap: u32,
    pub is_full: u32,
    pub logical_size: u64,
    pub allocated_size: u64,
    pub created: u64,
    pub modified: u64,
    pub changed: u64,
    pub accessed: u64,
    pub attributes: u32,
    pub flags: u32,
    pub extent_count: u32,
    pub resident_bytes: u32,
    pub carve_format: u32,
    pub carve_confidence: u32,
    pub hash_valid: u32,
    pub sha128: [u8; 16],
    pub name_len: u32,
}

// ---- query (session trees) --------------------------------------------------------

#[repr(C)]
pub struct CrUiQuery {
    pub category_mask: u32,
    pub size_min: u64,
    pub size_max: u64,
    pub modified_from: u64,
    pub modified_to: u64,
    pub confidence_min: u32,
    pub extensions: *const *const i8,
    pub extension_count: u32,
    pub name_substring: *const i8,
    pub path_prefix: *const i8,
    pub deleted_only: u32,
    pub origin_mask: u32,
    pub sort: u32,
    pub dir: u32,
    pub offset: u64,
    pub limit: u32,
    pub count_only: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiQueryRow {
    pub node_id: u64,
    pub name: *mut i8,
    pub path: *mut i8,
    pub size: u64,
    pub modified: u64,
    pub origin: u32,
    pub deleted: u32,
    pub confidence: u32,
    pub category: u32,
    pub grade: u32,
}

// ---- preview ------------------------------------------------------------------------

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiMetadata {
    pub kind: [u8; 24],
    pub format: [u8; 24],
    pub width: u32,
    pub height: u32,
    pub orientation: [u8; 8],
    pub datetime: [u8; 40],
    pub duration_ms: u64,
    pub codec: [u8; 32],
    pub text_snippet: [u8; 256],
    pub truncated: u32,
}

// ---- recovery -------------------------------------------------------------------------

#[repr(C)]
pub struct CrUiRecoveryConfig {
    pub read_chunk: u64,
    pub rename_attempts: u32,
    pub skip_hash_verify: u32,
    pub skip_dir_reconstruct: u32,
    pub conflict: u32,
    pub progress: CrUiRecoveryProgressFn,
    pub progress_user: *mut c_void,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiRecoveryStats {
    pub queued: u64,
    pub done: u64,
    pub refused: u64,
    pub errors: u64,
    pub hash_mismatch: u64,
    pub resumed: u64,
    pub salvaged: u64,
    pub dirs: u64,
    pub nothing_to_recover: u64,
}

// ---- health -----------------------------------------------------------------------------

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiSmartAttr {
    pub id: u32,
    pub value: u32,
    pub worst: u32,
    pub raw: u64,
    pub failing: u32,
    pub name: [u8; 40],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CrUiSmartReport {
    pub available: u32,
    pub transport: [u8; 24],
    pub ata_version: u32,
    pub smart_version: u32,
    pub attribute_count: u32,
    pub failing_now: u32,
    pub reallocated_sectors: u64,
    pub pending_sectors: u64,
    pub uncorrectable: u64,
    pub power_on_hours: u64,
    pub spin_retry: u64,
    pub ssd_wear_level: u64,
    pub temperature_c: u64,
    pub nvme_available: u32,
    pub temperature_c_x10: u32,
    pub available_spare: u32,
    pub available_spare_threshold: u32,
    pub percentage_used: u64,
    pub data_units_read: u64,
    pub data_units_written: u64,
    pub power_on_hours_nvme: u64,
    pub unsafe_shutdowns: u64,
    pub media_errors: u64,
    pub critical_warning: u32,
    pub ssd_like: u32,
    pub trim_known: u32,
    pub trim_enabled: u32,
    pub host_reads: u64,
    pub host_writes: u64,
    pub controller_busy_time: u64,
    pub num_error_log_entries: u64,
    pub data_bytes_read: u64,
    pub data_bytes_written: u64,
}

// ---- extern declarations (78-export surface, ABI v2) -----------------------------------

extern "C" {
    // version & diagnostics
    pub fn cr_ui_abi_version() -> u32;
    pub fn cr_ui_last_error() -> *const i8;
    pub fn cr_ui_string_free(s: *mut i8);

    // devices
    pub fn cr_ui_open_image(utf8_path: *const i8, offset: u64, bytes: u64, out: *mut *mut CrUiDevice) -> u32;
    pub fn cr_ui_open_disk(disk: u32, out: *mut *mut CrUiDevice) -> u32;
    pub fn cr_ui_open_volume(letter: i8, out: *mut *mut CrUiDevice) -> u32;
    pub fn cr_ui_last_transport_error() -> u32;
    pub fn cr_ui_open_volume_with_key(letter: i8, utf8_hex_fvek: *const i8, out: *mut *mut CrUiDevice) -> u32;
    pub fn cr_ui_unlock_volume_with_dictionary(letter: i8, utf8_user_passwords: *const *const i8, count: u32) -> u32;
    pub fn cr_ui_device_close(dev: *mut CrUiDevice);
    pub fn cr_ui_device_geometry(dev: *mut CrUiDevice, out: *mut CrUiGeometry) -> u32;
    pub fn cr_ui_device_transport(dev: *mut CrUiDevice, buf: *mut i8, cap: u32, needed: *mut u32) -> u32;

    // enumeration (Windows platform layer)
    pub fn cr_ui_enumerate_disks(out: *mut CrUiDiskInfo, count: *mut u32) -> u32;
    pub fn cr_ui_enumerate_volumes(out: *mut CrUiVolumeInfo, count: *mut u32) -> u32;

    // scan lifecycle
    pub fn cr_ui_scan_create(cfg: *const CrUiScanConfig, out: *mut *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_close(s: *mut CrUiScan);
    pub fn cr_ui_scan_attach_device(s: *mut CrUiScan, dev: *mut CrUiDevice) -> u32;
    pub fn cr_ui_scan_run(s: *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_pause(s: *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_resume(s: *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_cancel(s: *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_busy(s: *mut CrUiScan, out: *mut u32) -> u32;
    pub fn cr_ui_scan_stopping(s: *mut CrUiScan, out: *mut u32) -> u32;
    pub fn cr_ui_scan_telemetry(s: *mut CrUiScan, out: *mut CrUiTelemetry) -> u32;
    pub fn cr_ui_scan_get_stats(s: *mut CrUiScan, out: *mut CrUiScanStats) -> u32;
    pub fn cr_ui_scan_assistant(s: *mut CrUiScan, out: *mut CrUiAssistantStats) -> u32;
    pub fn cr_ui_scan_checkpoint_count(s: *mut CrUiScan, out: *mut u64) -> u32;
    pub fn cr_ui_scan_set_event_fn(s: *mut CrUiScan, f: CrUiEventFn, user: *mut c_void) -> u32;
    pub fn cr_ui_scan_save_session(s: *mut CrUiScan, utf8_path: *const i8) -> u32;
    pub fn cr_ui_scan_save_cancel(s: *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_snapshot(s: *mut CrUiScan, out: *mut *mut CrUiSnapshot) -> u32;
    pub fn cr_ui_scan_verify_identity(s: *mut CrUiScan) -> u32;
    pub fn cr_ui_scan_repair_save_undo(s: *mut CrUiScan, utf8_path: *const i8) -> u32;

    // sessions
    pub fn cr_ui_session_load(utf8_path: *const i8, out: *mut *mut CrUiSession) -> u32;
    pub fn cr_ui_session_close(sess: *mut CrUiSession);
    pub fn cr_ui_session_get_info(sess: *mut CrUiSession, out: *mut CrUiSessionInfo) -> u32;
    pub fn cr_ui_session_stats(sess: *mut CrUiSession, out: *mut CrUiScanStats) -> u32;
    pub fn cr_ui_session_snapshot(sess: *mut CrUiSession, out: *mut *mut CrUiSnapshot) -> u32;
    pub fn cr_ui_session_resume(sess: *mut CrUiSession, original_cfg: *const CrUiScanConfig, out: *mut *mut CrUiScan) -> u32;

    // snapshot enumeration
    pub fn cr_ui_snapshot_close(snap: *mut CrUiSnapshot);
    pub fn cr_ui_snapshot_node_count(snap: *mut CrUiSnapshot, out: *mut u32) -> u32;
    pub fn cr_ui_snapshot_partial(snap: *mut CrUiSnapshot, out: *mut u32) -> u32;
    pub fn cr_ui_snapshot_node(snap: *mut CrUiSnapshot, index: u32, out: *mut CrUiNodeInfo, name_buf: *mut i8, name_cap: u32) -> u32;
    pub fn cr_ui_snapshot_dfs(snap: *mut CrUiSnapshot, indices: *mut u32, count: *mut u32) -> u32;

    // query (session trees)
    pub fn cr_ui_query_run(sess: *mut CrUiSession, q: *const CrUiQuery, rows: *mut *mut CrUiQueryRow, row_count: *mut u32, total_matched: *mut u64) -> u32;
    pub fn cr_ui_query_rows_free(rows: *mut CrUiQueryRow, row_count: u32);

    // preview
    pub fn cr_ui_preview_open_snapshot(dev: *mut CrUiDevice, snap: *mut CrUiSnapshot, index: u32, out: *mut *mut CrUiPreview) -> u32;
    pub fn cr_ui_preview_open_session(dev: *mut CrUiDevice, sess: *mut CrUiSession, node_id: u64, out: *mut *mut CrUiPreview) -> u32;
    pub fn cr_ui_preview_close(pv: *mut CrUiPreview);
    pub fn cr_ui_preview_size(pv: *mut CrUiPreview, out: *mut u64) -> u32;
    pub fn cr_ui_preview_read(pv: *mut CrUiPreview, offset: u64, buf: *mut c_void, len: u32, placed: *mut u32) -> u32;
    pub fn cr_ui_preview_metadata(pv: *mut CrUiPreview, out: *mut CrUiMetadata) -> u32;
    pub fn cr_ui_preview_unreadable(pv: *mut CrUiPreview, total_bytes: *mut u64, range_count: *mut u32) -> u32;

    // recovery
    pub fn cr_ui_recovery_create(source: *mut CrUiDevice, cfg: *const CrUiRecoveryConfig, out: *mut *mut CrUiRecovery) -> u32;
    pub fn cr_ui_recovery_close(rec: *mut CrUiRecovery);
    pub fn cr_ui_recovery_set_destination(rec: *mut CrUiRecovery, utf8_path: *const i8) -> u32;
    pub fn cr_ui_recovery_enqueue_node(rec: *mut CrUiRecovery, sess: *mut CrUiSession, node_id: u64) -> u32;
    pub fn cr_ui_recovery_enqueue_subtree(rec: *mut CrUiRecovery, sess: *mut CrUiSession, node_id: u64) -> u32;
    pub fn cr_ui_recovery_enqueue_snapshot(rec: *mut CrUiRecovery, snap: *mut CrUiSnapshot, index: u32) -> u32;
    pub fn cr_ui_recovery_enqueue_subtree_snapshot(rec: *mut CrUiRecovery, snap: *mut CrUiSnapshot, index: u32) -> u32;
    pub fn cr_ui_recovery_queue_count(rec: *mut CrUiRecovery, out: *mut u32) -> u32;
    pub fn cr_ui_recovery_process(rec: *mut CrUiRecovery) -> u32;
    pub fn cr_ui_recovery_cancel(rec: *mut CrUiRecovery) -> u32;
    pub fn cr_ui_recovery_get_stats(rec: *mut CrUiRecovery, out: *mut CrUiRecoveryStats) -> u32;
    pub fn cr_ui_recovery_report_json(rec: *mut CrUiRecovery, source_utf8: *const i8, json_out: *mut *mut i8) -> u32;

    // health (Windows platform layer)
    pub fn cr_ui_health_read(disk: u32, rep: *mut CrUiSmartReport, attrs: *mut CrUiSmartAttr, attr_cap: u32, attr_count: *mut u32) -> u32;
}
