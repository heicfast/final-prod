//! Build script — engine ABI link wiring + the Tauri context build.
//!
//! The bridge statically imports cr_ui.dll (docs/INTEGRATION-PLAN.md §4.8: a
//! libloading dynamic path is deliberately NOT used — static import matches
//! the release bundle's import-lib contract and avoids dual definitions).
//! The import library `cr_ui.lib` is therefore required AT LINK TIME on
//! Windows; `cargo check`/`clippy` do not link and need nothing.
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
            println!(
                "cargo:rustc-link-search=native={manifest}/target/{profile}"
            );
        }
        println!("cargo:rustc-link-lib=dylib=cr_ui");
        println!(
            "cargo:warning=linking cr_ui.lib (static import of cr_ui.dll); build.ps1 -Target engine stages the kit when CR_UI_LIB_DIR is unset"
        );
    }
    tauri_build::build()
}
