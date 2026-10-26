//! Recovery Wizard — Tauri shell.
//!
//! The shell is deliberately thin: window lifecycle, native dialogs, and small
//! filesystem conveniences for the future real connector adapter. All recovery
//! domain logic lives behind the connector contract (see src/connector and
//! docs/CONNECTOR-CONTRACT.md). No UI feature logic belongs here.

mod commands;

use tauri::Manager;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            commands::system::reveal_in_explorer,
            commands::system::path_exists,
            commands::system::app_environment,
        ])
        .setup(|app| {
            // Reference the main window so the builder cannot strip it; the
            // frontend owns presentation, we only size the shell.
            let _main = app.get_webview_window("main");
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
