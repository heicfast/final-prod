//! bridge — the cr_ui engine bridge (docs/INTEGRATION-PLAN.md §4).
//!
//! Module map:
//! - `abi`     the FFI surface, pinned 1:1 from engine/include/cr/cr_ui.h
//! - `error`   BridgeError + the UI vocabulary bridge-side codes
//! - `util`    two-call helpers, time/base64 conversion, owned strings
//! - `state`   registries, snapshot metadata cache, lifecycle discipline
//! - `events`  worker-thread sinks → channel → single emitter → Tauri emit
//! - `consent` the pre-scan repair consent gate (S-008, plan §4.5, G-1 (a))
//! - `filefilter` GENERATED extension→category mirror of the engine table
//! - `logging`  the operational log: bridge lifecycle + the engine's
//!              diagnostic seam (NG-15; ProgramData ...\logs\app.log)
//!
//! The command modules under `crate::commands` implement the manifest in
//! contracts/command-surface.json; contracts/validate.mjs phase 2 cross-checks
//! every command both ways. (No blanket re-exports: each consumer imports the
//! exact types it uses - lib.rs manages Arc<BridgeState> directly.)

pub mod abi;
pub mod consent;
pub mod error;
pub mod events;
pub mod filefilter;
pub mod logging;
pub mod state;
pub mod util;
