//! commands/images.rs — M04 / BF-12: image-file scan targets (the
//! reference's "Import a disk image" home-page flow).
//!
//! Corpus lineage: the device-selection "Import a disk image" affordance
//! with the file filter `*.vmdk *.vhd *.vhdx *.edisk *.dmg *.img`
//! (rev/strings_ascii.txt:11803-11816, REFERENCE-API §5). The `.edisk`
//! family is EXCLUDED (M03/D-IMG-2: the og's container internals are
//! corpus-UNKNOWN and IP-barred — a registered divergence). The engine
//! surface is already shipped: `cr_ui_open_image` (cr_ui.h:322, the
//! 512-byte-sector read-only view over [offset, offset+bytes)) + the
//! CLI's `scan --image file [--whole-disk]`; this module is the bridge
//! half that makes an imported image a first-class scan TARGET.
//!
//! Truth rules this module enforces:
//! - The registry is IN-MEMORY (an imported image survives refreshes in
//!   this app run; it does not survive an app restart — re-importing is
//!   one click, and the honest scope is documented). Sessions saved from
//!   an image scan resume by PATH (session-96 identity) — the registry
//!   plays no part in resume.
//! - Validation is typed and truthful: a missing file is device-gone
//!   (the target is not present), an unsupported extension or an empty
//!   file is scan-scope-invalid (the og's filter, minus .edisk).
//! - The Device row's capacity IS the file size (byte-for-byte view);
//!   `canScanWholeDisk: true` (the image is the disk); volumes: [] (the
//!   volumes live INSIDE the image — the scan finds them).
//! - Kind is the dedicated `"image"` (NOT 'virtual' — the D-ENUM-05
//!   lesson: every scanning surface filters kind !== 'virtual'; an
//!   imported image is a scannable target and must never be filtered).

use std::collections::HashMap;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

use serde_json::{json, Value};

use crate::bridge::error::{code, BridgeError, BridgeResult};

/// The og's import filter, minus `.edisk` (M03): the families the engine
/// actually opens. Case-insensitive on this side; the ENGINE owns the
/// content probe (a renamed file that is not an image fails the scan
/// open with its own typed refusal).
pub const IMAGE_EXTENSIONS: [&str; 5] = ["vmdk", "vhd", "vhdx", "dmg", "img"];

/// One imported image target.
pub struct ImageTarget {
    pub id: String,
    pub path: String,
    pub file_name: String,
    pub size_bytes: u64,
    /// The lowercase extension family (display + the filter's truth).
    pub family: String,
}

impl ImageTarget {
    /// The Device-shaped row the connector consumes (devices_list merges
    /// these under the same key the enumeration rows use).
    pub fn device_row(&self) -> Value {
        json!({
            "id": self.id,
            "kind": "image",
            "busType": "file",
            "model": format!("{} image", self.family.to_uppercase()),
            "label": self.file_name,
            "serial": String::new(),
            "capacity": {
                "totalBytes": self.size_bytes,
                "usedBytes": 0,
                "freeBytes": 0,
            },
            "status": "ok",
            "isSourceOfActiveScan": false,
            "volumes": [],
            // The image IS the disk: the whole-disk entry point.
            "canScanWholeDisk": true,
        })
    }
}

/// The in-memory registry (its own managed state — the ImagingState seam
/// discipline: image targets are not scan-tree or disk-enumeration state).
pub struct ImageTargetsState {
    targets: Mutex<HashMap<String, Arc<ImageTarget>>>,
    next: AtomicU64,
}

impl ImageTargetsState {
    pub fn new() -> Self {
        ImageTargetsState {
            targets: Mutex::new(HashMap::new()),
            next: AtomicU64::new(1),
        }
    }

    fn mint_id(&self) -> String {
        format!("imgfile-{}", self.next.fetch_add(1, Ordering::Relaxed))
    }

    /// The rows for devices_list (refresh-stable within the app run).
    pub fn rows(&self) -> Vec<Value> {
        self.targets
            .lock()
            .map(|m| m.values().map(|t| t.device_row()).collect())
            .unwrap_or_default()
    }

    /// The registry lookup behind scan_start's imgfile- arm.
    pub fn get(&self, id: &str) -> Option<Arc<ImageTarget>> {
        self.targets.lock().ok().and_then(|m| m.get(id).cloned())
    }
}

/// The extension of a path, lowercase, None when absent.
fn ext_of(path: &str) -> Option<String> {
    std::path::Path::new(path)
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| e.to_ascii_lowercase())
}

/// Import (register) an image file as a scannable target. Idempotent per
/// absolute path: re-importing the same file returns the existing row
/// (the og's dialog cannot double-list one file).
#[tauri::command]
pub async fn image_target_import(
    images: tauri::State<'_, Arc<ImageTargetsState>>,
    path: String,
) -> Result<Value, BridgeError> {
    let images = images.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let trimmed = path.trim().to_string();
        if trimmed.is_empty() {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                "image path is empty",
            ));
        }
        let p = std::path::Path::new(&trimmed);
        if !p.is_file() {
            return Err(BridgeError::bridge(
                code::DEVICE_GONE,
                format!("image file '{trimmed}' is not present"),
            ));
        }
        let Some(family) = ext_of(&trimmed) else {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                format!(
                    "'{trimmed}' has no extension — the import filter is {}",
                    IMAGE_EXTENSIONS
                        .iter()
                        .map(|e| format!("*.{e}"))
                        .collect::<Vec<_>>()
                        .join(" ")
                ),
            ));
        };
        if !IMAGE_EXTENSIONS.contains(&family.as_str()) {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                format!(
                    "'.{family}' is not a supported image family — the import filter is {}",
                    IMAGE_EXTENSIONS
                        .iter()
                        .map(|e| format!("*.{e}"))
                        .collect::<Vec<_>>()
                        .join(" ")
                ),
            ));
        }
        let meta = std::fs::metadata(p).map_err(|e| {
            BridgeError::bridge(
                code::DEVICE_GONE,
                format!("cannot read image file '{trimmed}': {e}"),
            )
        })?;
        let size = meta.len();
        if size == 0 {
            return Err(BridgeError::bridge(
                code::SCAN_SCOPE_INVALID,
                format!("image file '{trimmed}' is empty (0 bytes)"),
            ));
        }
        // Canonicalize for the idempotence key (best-effort; the trimmed
        // path stands when the FS refuses).
        let key = std::fs::canonicalize(&p)
            .map(|c| c.to_string_lossy().to_string())
            .unwrap_or_else(|_| trimmed.clone());
        let file_name = p
            .file_name()
            .and_then(|n| n.to_str())
            .unwrap_or("disk image")
            .to_string();
        let mut guard = images
            .targets
            .lock()
            .map_err(|_| BridgeError::bridge(code::UNKNOWN, "image targets lock"))?;
        if let Some(existing) = guard.values().find(|t| t.path == key) {
            return Ok(existing.device_row());
        }
        let id = images.mint_id();
        let target = Arc::new(ImageTarget {
            id: id.clone(),
            path: key,
            file_name,
            size_bytes: size,
            family,
        });
        let row = target.device_row();
        guard.insert(id, target);
        Ok(row)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

/// The imported rows (the devices_list merge consumes the same state;
/// this command exists for the connector's explicit re-query).
#[tauri::command]
pub async fn image_target_list(
    images: tauri::State<'_, Arc<ImageTargetsState>>,
) -> Result<Value, BridgeError> {
    let rows = images.inner().rows();
    Ok(json!(rows))
}

/// The scan-side resolver: an imgfile- id must resolve against the
/// registry (device-gone when absent — the same typed refusal a pulled
/// USB disk gets).
pub fn resolve_image_target(
    images: &Arc<ImageTargetsState>,
    target_id: &str,
) -> BridgeResult<Arc<ImageTarget>> {
    images.get(target_id).ok_or_else(|| {
        BridgeError::bridge(
            code::DEVICE_GONE,
            format!("image target {target_id} is not imported (app restart clears imports)"),
        )
    })
}

/// The registry prefix scan_start routes on.
pub fn is_image_target(target_id: &str) -> bool {
    target_id.starts_with("imgfile-")
}
