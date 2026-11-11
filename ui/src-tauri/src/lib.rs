//! Recovery Wizard — Tauri shell + the cr_ui engine bridge (ABI v4).
//!
//! The shell is deliberately thin: window lifecycle, the dialog/opener
//! plugins, and the bridge command set (crate::commands — the manifest in
//! contracts/command-surface.json). All recovery domain logic lives in the
//! engine behind the bridge; the frontend talks only to the connector
//! contract (ui/src/connector/contract.ts). No UI feature logic belongs
//! here.

mod bridge;
mod commands;

use std::sync::Arc;

use tauri::Manager;

use bridge::state::BridgeState;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    // NG-15: the operational log FIRST — every later fact (the ABI gate,
    // scan lifecycle, engine diagnostics) lands in
    // %ProgramData%\HeicFast\Recovery Wizard\logs\app.log. The shipped
    // app previously wrote NOTHING (eprintln to a lost stderr) — an
    // installed-app failure left no evidence trail.
    bridge::logging::init();
    bridge::logging::info("shell starting");

    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .manage(Arc::new(BridgeState::new()))
        // Lane C: the imaging registry — its own managed state (image jobs
        // address a physical disk, not a scan tree; the seam note is in
        // COORDINATION.md).
        .manage(Arc::new(commands::imaging::ImagingState::new()))
        .invoke_handler(tauri::generate_handler![
            // devices (+ the N-064 dictionary unlock)
            commands::devices::devices_list,
            commands::devices::devices_refresh,
            commands::devices::volume_unlock_dictionary,
            // scan (+ the N-060 save-scoped cancel)
            commands::scan::scan_start,
            commands::scan::scan_pause,
            commands::scan::scan_resume,
            commands::scan::scan_cancel,
            commands::scan::scan_state,
            commands::scan::scan_close,
            commands::scan::scan_save_cancel,
            // results
            commands::results::results_tree,
            commands::results::results_query,
            commands::results::results_count,
            commands::results::node_detail,
            // preview (M-116 archive browse-inside rides preview_open; the
            // M-102 session thumbnails ride preview_thumbnail)
            commands::preview::preview_open,
            commands::preview::preview_thumbnail,
            // sessions (W1a light list, W1b load events, M-154 pointed
            // resume, the scan resume)
            commands::sessions::session_save,
            commands::sessions::sessions_list,
            commands::sessions::session_load,
            commands::sessions::session_meta,
            commands::sessions::session_resume_last,
            commands::sessions::session_resume,
            // recovery
            commands::recovery::recovery_start,
            commands::recovery::recovery_cancel,
            commands::recovery::recovery_state,
            commands::recovery::recovery_items,
            commands::recovery::recovery_report,
            // health (base ABI — BF-03)
            commands::health::health_read,
            // repair: the consent answer (both families) + the G-1 gap trio
            commands::repair::consent_respond,
            commands::repair::repair_dry_run,
            commands::repair::repair_apply,
            commands::repair::repair_undo_log,
            commands::repair::repair_undo,
            // lost-partition apply (S-029)
            commands::lost_partition::lost_partition_apply,
            commands::lost_partition::lost_partition_undo,
            commands::lost_partition::lost_partition_save_undo,
            commands::lost_partition::apply_consent_pending,
            // system: user signs, mounts, picker, opener (D1: elevation is
            // the launch-time manifest — no posture/relaunch commands)
            commands::system::system_app_info,
            commands::system::system_load_user_signs,
            commands::system::system_validate_destination,
            commands::system::system_pick_directory,
            commands::system::system_list_mounts,
            commands::system::system_open_path,
            // imaging (lane C — the Create Image flow): start + state; NO
            // cancel at the ABI (cr_ui_image_create runs cancel=nullptr —
            // the documented divergence, PARITY-DELTAS-registered)
            commands::imaging::image_create_start,
            commands::imaging::image_create_state,
            // diagnostics (lane C — the observability surface)
            commands::diagnostics::scan_telemetry,
            commands::diagnostics::scan_assistant_stats,
            commands::diagnostics::carve_catalog,
            commands::diagnostics::scan_carve_tally,
            commands::diagnostics::disk_map,
            commands::diagnostics::device_transport,
            commands::diagnostics::session_regrade,
            // maintenance (lane C — the privileged one-shots)
            commands::maintenance::volume_fsck,
            commands::maintenance::volume_bitlocker_off,
        ])
        .setup(|app| {
            // Reference the main window so the builder cannot strip it; the
            // frontend owns presentation, we only size the shell.
            let _main = app.get_webview_window("main");
            // Startup ABI gate (plan §4.1): log the mismatch, refuse to bind
            // per-scan (scan_start's own gate). The frontend's getAppInfo
            // also surfaces the state truthfully.
            let abi_v = unsafe { bridge::abi::cr_ui_abi_version() };
            if abi_v != bridge::abi::CR_UI_ABI_VERSION {
                bridge::logging::error(&format!(
                    "cr_ui ABI version {abi_v} != expected {}; the shipped engine kit is \
                     incompatible with this build (reinstall or rebuild the engine lane)",
                    bridge::abi::CR_UI_ABI_VERSION
                ));
            } else {
                bridge::logging::info("ABI gate: engine kit version matches");
            }
            // M-048: the exe-adjacent user-sign defaults (the CLI's startup
            // posture; the DLL does not auto-load).
            commands::system::load_default_user_signs();
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
    bridge::logging::info("shell exited");
}
