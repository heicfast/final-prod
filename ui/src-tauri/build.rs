//! Build script — engine ABI link wiring + the Tauri context build.
//!
//! The bridge statically imports cr_ui.dll (plan §4.1: a libloading dynamic
//! path is deliberately NOT used — static import matches the release
//! bundle's import-lib contract and avoids dual definitions). The import
//! library `cr_ui.lib` is therefore required AT LINK TIME on Windows;
//! `cargo check`/`clippy` do not link and need nothing.
//!
//! Link search order:
//!   1. `CR_UI_LIB_DIR` (set by build.ps1's ui lane / a developer who built
//!      the engine lane; the documented dev-loop escape hatch), then
//!   2. `<manifest>/target/release` and `<manifest>/target/debug` — the
//!      directories build.ps1 stages the engine kit into (cr_ui.lib beside
//!      where cargo will emit the exe, so the DLLs also load from there by
//!      default loader order).
//!
//! Off Windows the directives are skipped (the product ships Windows-only;
//! non-Windows `cargo check` still type-checks the bridge through the
//! #[cfg(windows)] gates).

fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    if target_os == "windows" {
        if let Ok(dir) = std::env::var("CR_UI_LIB_DIR") {
            if !dir.is_empty() {
                println!("cargo:rustc-link-search=native={dir}");
            }
        }
        let manifest = std::env::var("CARGO_MANIFEST_DIR").unwrap_or_else(|_| ".".into());
        for profile in ["release", "debug"] {
            println!("cargo:rustc-link-search=native={manifest}/target/{profile}");
        }
        println!("cargo:rustc-link-lib=dylib=cr_ui");
        println!(
            "cargo:warning=linking cr_ui.lib (static import of cr_ui.dll, ABI v4); build.ps1 -Target engine stages the kit when CR_UI_LIB_DIR is unset"
        );
    }
    // D-PRIV-01 (the missing privilege model): the product requires the
    // administrator token for every raw-disk surface (README: scanning,
    // recovery, health, repair, lost-partition apply), and the reference
    // ships `requireAdministrator` on both DRW.exe and DRWUI.exe (corpus
    // rev/DRWUI.exe/strings_ascii.txt:41023) — UAC fires ONCE at launch
    // through the standard OS flow; no in-app restart flow exists in the
    // reference and none is needed here. tauri-build's stock embed carries
    // no trustInfo section at all (asInvoker by loader default); this wires
    // our manifest (app.manifest — trustInfo + the stock Common-Controls
    // dependency) through WindowsAttributes::app_manifest.
    let windows_attributes = tauri_build::WindowsAttributes::new()
        .app_manifest(include_str!("app.manifest"));
    tauri_build::try_build(
        tauri_build::Attributes::new().windows_attributes(windows_attributes),
    )
    .expect("tauri_build::try_build failed");
}
