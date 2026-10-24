//! System-domain shell commands.

use serde::Serialize;

#[derive(Serialize)]
pub struct AppEnvironment {
    pub os: String,
    pub arch: String,
    pub family: String,
    pub webview_version: String,
}

/// Opens the given path in the system file explorer (folder, or parent of a file).
#[tauri::command]
pub fn reveal_in_explorer(path: String) -> Result<(), String> {
    let p = std::path::Path::new(&path);
    if !p.exists() {
        return Err(format!("path does not exist: {}", p.display()));
    }
    #[cfg(target_os = "windows")]
    {
        if p.is_dir() {
            std::process::Command::new("explorer")
                .arg(&path)
                .spawn()
                .map_err(|e| e.to_string())?;
        } else {
            std::process::Command::new("explorer")
                .arg(format!("/select,{}", path))
                .spawn()
                .map_err(|e| e.to_string())?;
        }
        Ok(())
    }
    #[cfg(not(target_os = "windows"))]
    {
        let _ = p;
        Err("reveal_in_explorer is only available on Windows".into())
    }
}

/// Checks existence without following the full connector validation pipeline.
/// Real destination-overlap logic stays engine-side (connector contract).
#[tauri::command]
pub fn path_exists(path: String) -> bool {
    std::path::Path::new(&path).exists()
}

/// Environment info surfaced by the connector's System domain.
#[tauri::command]
pub fn app_environment() -> AppEnvironment {
    AppEnvironment {
        os: std::env::consts::os.to_string(),
        arch: std::env::consts::arch.to_string(),
        family: std::env::consts::family.to_string(),
        webview_version: String::from("webview2"),
    }
}
