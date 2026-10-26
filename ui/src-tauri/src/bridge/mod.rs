//! bridge — the cr_ui engine bridge (docs/INTEGRATION-PLAN.md §4).
//!
//! Module map:
//! - `abi`     the FFI surface, pinned 1:1 from engine/include/cr/cr_ui.h
//! - `error`   BridgeError + the UI vocabulary bridge-side codes
//! - `util`    two-call helpers, time/base64 conversion, owned strings
//! - `state`   registries, snapshot metadata cache, lifecycle discipline
//! - `events`  worker-thread sinks → channel → single emitter → Tauri emit
//! - `filefilter` GENERATED extension→category mirror of the engine table
//!
//! The command modules under `crate::commands` implement the manifest in
//! contracts/command-surface.json; contracts/validate.mjs phase 2 cross-checks
//! every command both ways.

pub mod abi;
pub mod error;
pub mod events;
pub mod filefilter;
pub mod state;
pub mod util;

pub use error::{BridgeError, BridgeResult};
pub use state::BridgeState;
