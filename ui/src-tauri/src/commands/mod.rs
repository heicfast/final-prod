//! commands — the Tauri command set implementing contracts/command-surface.json.
//! contracts/validate.mjs phase 2 cross-checks this registration both ways
//! (every manifest command has an impl here; every impl is in the manifest).

pub mod devices;
pub mod health;
pub mod recovery;
pub mod repair;
pub mod results;
pub mod scan;
pub mod sessions;
pub mod system;
