//! The Tauri command set — the manifest in contracts/command-surface.json
//! (validate.mjs phase 2 cross-checks it both ways against the TS
//! connector's invokes).
//!
//! Domain modules: devices (+ unlock), scan (+ save-cancel), results,
//! preview (+ archive/thumbnails), sessions (+ resume family), recovery,
//! health, repair (consent respond + the gap trio), lost_partition (the
//! S-029 apply family), system (posture/relaunch/user-signs/mounts/picker),
//! imaging (the Create Image flow — lane C), diagnostics (telemetry /
//! assistant / carve catalog + tally / disk map / transport / regrade —
//! lane C), maintenance (fsck + bitlocker-off — lane C).

pub mod devices;
pub mod diagnostics;
pub mod health;
pub mod imaging;
pub mod lost_partition;
pub mod maintenance;
pub mod preview;
pub mod recovery;
pub mod repair;
pub mod results;
pub mod scan;
pub mod sessions;
pub mod system;
