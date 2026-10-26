//! Thin shell commands for the future real connector adapter.
//!
//! Contract mirror: `RecoveryConnector` (src/connector/contract.ts) — System
//! domain. These commands are the Rust side that `connector/tauri/` will invoke
//! once the engine integration session wires the real bridge.

pub mod system;
