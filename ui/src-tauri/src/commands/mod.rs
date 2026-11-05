//! The Tauri command set — the manifest in contracts/command-surface.json
//! (validate.mjs phase 2 cross-checks it both ways against the TS
//! connector's invokes).
//!
//! Domain modules: devices (+ unlock), scan (+ save-cancel), results,
//! preview (+ archive/thumbnails), sessions (+ resume family), recovery,
//! health, repair (consent respond + the gap trio), lost_partition (the
//! S-029 apply family), system (posture/relaunch/user-signs/mounts/picker).

pub mod devices;
pub mod health;
pub mod lost_partition;
pub mod preview;
pub mod recovery;
pub mod repair;
pub mod results;
pub mod scan;
pub mod sessions;
pub mod system;
