//! Recovery Wizard — Tauri shell + the cr_ui engine bridge.
//!
//! The shell is deliberately thin: window lifecycle, the dialog/opener
//! plugins, and the bridge command set (crate::commands — the manifest in
//! contracts/command-surface.json). All recovery domain logic lives in the
//! engine behind the bridge; the frontend talks only to the connector
//! contract (ui/src/connector/contract.ts).

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
        .invoke_handler(tauri::generate_handler![
            // devices
            commands::devices::devices_list,
            commands::devices::devices_refresh,
            // scan
            commands::scan::scan_start,
            commands::scan::scan_pause,
            commands::scan::scan_resume,
            commands::scan::scan_cancel,
            commands::scan::scan_state,
            commands::scan::scan_close,
            // results
            commands::results::results_tree,
            commands::results::results_query,
            commands::results::results_count,
            commands::results::node_detail,
            // preview
            commands::preview::preview_open,
            commands::preview::preview_thumbnail,
            // sessions
            commands::sessions::session_save,
            commands::sessions::sessions_list,
            commands::sessions::session_load,
            // recovery
            commands::recovery::recovery_start,
            commands::recovery::recovery_cancel,
            commands::recovery::recovery_state,
            commands::recovery::recovery_items,
            commands::recovery::recovery_report,
            // health
            commands::health::health_read,
            // repair (G-1 gap cluster: typed truthful refusals) + the
            // consent answer (re-added per G-1 (a): the scan-page prompt)
            commands::repair::consent_respond,
            commands::repair::repair_dry_run,
            commands::repair::repair_apply,
            commands::repair::repair_undo_log,
            commands::repair::repair_undo,
            // system
            commands::system::system_driver_status,
            commands::system::system_app_info,
            commands::system::system_validate_destination,
            commands::system::system_pick_directory,
            commands::system::system_list_mounts,
            commands::system::system_open_path,
        ])
        .setup(|app| {
            // Reference the main window so the builder cannot strip it; the
            // frontend owns presentation, we only size the shell.
            let _main = app.get_webview_window("main");
            // Startup ABI gate (plan §4.8): refuse to bind on mismatch. The
            // frontend's getAppInfo also surfaces the state truthfully.
            let abi_v = unsafe { bridge::abi::cr_ui_abi_version() };
            if abi_v != bridge::abi::CR_UI_ABI_VERSION {
                bridge::logging::error(&format!(
                    "cr_ui ABI version {abi_v} != expected {}; the shipped engine kit is \
                     incompatible with this build (reinstall or rebuild the engine lane)",
                    bridge::abi::CR_UI_ABI_VERSION
                ));
                eprintln!(
                    "cr_ui ABI version {abi_v} != expected {}; the shipped engine kit is \
                     incompatible with this build (reinstall or rebuild the engine lane)",
                    bridge::abi::CR_UI_ABI_VERSION
                );
            } else {
                bridge::logging::info("ABI gate: engine kit version matches");
            }
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
    bridge::logging::info("shell exited");
}
