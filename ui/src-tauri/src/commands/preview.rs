//! commands/preview.rs — preview_open / preview_thumbnail (the payload
//! assembly; preview reads + closes are internal helpers, not commands —
//! the contract has no paging surface).
//!
//! Kind dispatch is bridge-side from ENGINE metadata (ADR-008): the UI never
//! classifies extensions. Payload bounds (ADR-003): image assembly capped at
//! 16 MiB (beyond → preview-too-large — a truncated base64 prefix renders as
//! a corrupt image, which would be a lie); hex reads capped at 64 KiB; the
//! engine itself caps reads at 4 MiB/call.
//!
//! G-4 (no renderer): video/audio → 'av' metadata payload with a null
//! thumbnail; document/archive → 'office' metadata with no extracted text.
//! The preview pane renders the truthful "preview not rendered" hint itself.
//!
//! Damage truth (cr_ui_preview_unreadable reports zero-filled spans): any
//! unreadable span → preview-damaged. The payload shape carries no damage
//! annotation field, so returning zero-filled content as if intact would be
//! the lie; the typed refusal with preview.reason.damaged is the truth.

use std::sync::Arc;

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, check, BridgeError, BridgeResult};
use crate::bridge::state::{metas_snapshot, BridgeState, ScanEntry};
use crate::bridge::util::{base64_encode, fixed_str};

const IMAGE_CAP: u64 = 16 * 1024 * 1024;
const THUMBNAIL_CAP: u64 = 8 * 1024 * 1024;
const HEX_READ: u32 = 64 * 1024;

fn mime_for(format: &str) -> &'static str {
    match format.to_ascii_lowercase().as_str() {
        "jpg" | "jpeg" => "image/jpeg",
        "png" => "image/png",
        "gif" => "image/gif",
        "bmp" => "image/bmp",
        "webp" => "image/webp",
        "tif" | "tiff" => "image/tiff",
        "svg" => "image/svg+xml",
        "ico" => "image/x-icon",
        "heic" | "heif" => "image/heic",
        _ => "application/octet-stream",
    }
}

/// Resolve an `f<idx>` id to (meta index, node_id) under one snapshot lock.
fn resolve_node(
    entry: &Arc<ScanEntry>,
    node_id: &str,
) -> BridgeResult<(usize, u64)> {
    let idx: usize = node_id
        .strip_prefix('f')
        .and_then(|r| r.parse().ok())
        .ok_or_else(|| {
            BridgeError::bridge(
                code::UNKNOWN,
                format!("node id '{node_id}' is not a previewable file row"),
            )
        })?;
    let snap = entry
        .snap
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "snapshot lock"))?;
    let metas = snap
        .metas
        .as_ref()
        .ok_or_else(|| BridgeError::bridge(code::UNKNOWN, "metadata cache missing"))?;
    let m = metas
        .get(idx)
        .ok_or_else(|| BridgeError::bridge(code::UNKNOWN, "node index out of range"))?;
    Ok((idx, m.node_id))
}

/// Open a preview handle for the node (snapshot row for live scans; session
/// node_id for loaded sessions). The caller closes it (transient lifetime).
fn open_preview(
    entry: &Arc<ScanEntry>,
    idx: usize,
    node_id: u64,
) -> BridgeResult<*mut abi::CrUiPreview> {
    let dev = entry.device();
    if dev.is_null() {
        return Err(BridgeError::bridge(
            code::DEVICE_GONE,
            "the source device is not attached (removed or ambiguous after session load) - \
             browsing keeps working; previews and recovery need the source",
        ));
    }
    let mut pv: *mut abi::CrUiPreview = std::ptr::null_mut();
    let snap = entry
        .snap
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "snapshot lock"))?;
    let rc = match &entry.kind {
        crate::bridge::state::ScanKind::Session { session, .. } => unsafe {
            abi::cr_ui_preview_open_session(dev, session.get(), node_id, &mut pv)
        },
        _ => unsafe {
            abi::cr_ui_preview_open_snapshot(dev, snap.handle.get(), idx as u32, &mut pv)
        },
    };
    check(rc, "cr_ui_preview_open")?;
    Ok(pv)
}

/// Read up to `cap` logical bytes from the preview (engine caps 4 MiB/call;
/// bridge-side chunking honors the bound for larger reads).
fn read_bytes(pv: *mut abi::CrUiPreview, cap: u64) -> BridgeResult<Vec<u8>> {
    let mut out = Vec::new();
    let mut offset = 0u64;
    while offset < cap {
        let want = ((cap - offset).min(4 * 1024 * 1024)) as u32;
        let mut buf = vec![0u8; want as usize];
        let mut placed: u32 = 0;
        check(
            unsafe { abi::cr_ui_preview_read(pv, offset, buf.as_mut_ptr().cast(), want, &mut placed) },
            "cr_ui_preview_read",
        )?;
        out.extend_from_slice(&buf[..placed as usize]);
        if placed < want {
            break; // logical EOF
        }
        offset += placed as u64;
    }
    Ok(out)
}

fn unreadable_total(pv: *mut abi::CrUiPreview) -> u64 {
    let mut total: u64 = 0;
    let mut ranges: u32 = 0;
    let _ = unsafe { abi::cr_ui_preview_unreadable(pv, &mut total, &mut ranges) };
    total
}

pub fn preview_payload(
    state: &Arc<BridgeState>,
    scan_id: &str,
    node_id: &str,
    thumbnail: bool,
) -> BridgeResult<Value> {
    let (entry, _metas, _partial) = metas_snapshot(state, scan_id)?;
    let (idx, node_id_u64) = resolve_node(&entry, node_id)?;
    let pv = open_preview(&entry, idx, node_id_u64)?;

    let mut size: u64 = 0;
    let mut meta: abi::CrUiMetadata = unsafe { std::mem::zeroed() };
    let rc_size = unsafe { abi::cr_ui_preview_size(pv, &mut size) };
    let rc_meta = unsafe { abi::cr_ui_preview_metadata(pv, &mut meta) };
    let unreadable = unreadable_total(pv);
    let kind = fixed_str(&meta.kind);
    let format = fixed_str(&meta.format);

    // Assemble everything while the handle is open; close right after.
    let payload = (|| -> BridgeResult<Value> {
        if rc_size != abi::CR_UI_OK {
            return Err(BridgeError::engine(rc_size, "cr_ui_preview_size"));
        }
        if rc_meta != abi::CR_UI_OK {
            return Err(BridgeError::engine(rc_meta, "cr_ui_preview_metadata"));
        }
        // Zero-filled unreadable spans: the truthful refusal (see header).
        if unreadable > 0 {
            return Err(BridgeError::bridge(
                code::PREVIEW_DAMAGED,
                format!(
                    "preview reports {unreadable} unreadable byte(s) (zero-filled); \
                     the content is not intact"
                ),
            ));
        }
        let cap = if thumbnail { THUMBNAIL_CAP } else { IMAGE_CAP };

        match kind.as_str() {
            "image" => {
                if thumbnail {
                    if size > cap {
                        return Ok(Value::Null); // OQ-3: oversized -> null
                    }
                    let bytes = read_bytes(pv, size.min(cap))?;
                    let data_url = format!("data:{};base64,{}", mime_for(&format), base64_encode(&bytes));
                    return Ok(json!({ "dataUrl": data_url }));
                }
                if size > IMAGE_CAP {
                    return Err(BridgeError::bridge(
                        code::PREVIEW_TOO_LARGE,
                        format!("image is {size} bytes; the preview bound is {IMAGE_CAP}"),
                    ));
                }
                if size == 0 {
                    return Err(BridgeError::bridge(
                        code::PREVIEW_MISSING_DATA,
                        "the node has no readable content",
                    ));
                }
                let bytes = read_bytes(pv, size)?;
                Ok(json!({
                    "kind": "image",
                    "dataUrl": format!("data:{};base64,{}", mime_for(&format), base64_encode(&bytes)),
                    "width": meta.width,
                    "height": meta.height,
                    "bounded": false,
                }))
            }
            "text" => {
                if size == 0 {
                    return Err(BridgeError::bridge(
                        code::PREVIEW_MISSING_DATA,
                        "the node has no readable content",
                    ));
                }
                let snippet = fixed_str(&meta.text_snippet);
                Ok(json!({
                    "kind": "text",
                    "text": snippet,
                    "encoding": "UTF-8",
                    "totalSize": size,
                    "truncated": meta.truncated != 0 || (snippet.len() as u64) < size,
                }))
            }
            "video" | "audio" => {
                let mut metadata = serde_json::Map::new();
                if !format.is_empty() {
                    metadata.insert("container".into(), json!(format));
                }
                if !fixed_str(&meta.codec).is_empty() {
                    metadata.insert("codec".into(), json!(fixed_str(&meta.codec)));
                }
                if meta.width != 0 || meta.height != 0 {
                    metadata.insert("width".into(), json!(meta.width));
                    metadata.insert("height".into(), json!(meta.height));
                }
                let duration_s = if meta.duration_ms > 0 {
                    Some(meta.duration_ms as f64 / 1000.0)
                } else {
                    None
                };
                Ok(json!({
                    "kind": "av",
                    "metadata": metadata,
                    // G-4: no renderer exists — the pane shows the truthful
                    // "preview not rendered" hint for the null thumbnail.
                    "thumbnailDataUrl": Value::Null,
                    "durationSeconds": duration_s,
                }))
            }
            "document" | "archive" => {
                let mut metadata = serde_json::Map::new();
                if !format.is_empty() {
                    metadata.insert("format".into(), json!(format));
                }
                if meta.width != 0 && meta.height != 0 {
                    metadata.insert("pages".into(), json!("1"));
                }
                Ok(json!({
                    "kind": "office",
                    "metadata": metadata,
                    "extractedText": Value::Null, // G-4: no text extractor exists
                    "truncated": false,
                }))
            }
            "unsupported" => {
                if size == 0 {
                    return Err(BridgeError::bridge(
                        code::PREVIEW_MISSING_DATA,
                        "the node has no readable content",
                    ));
                }
                Err(BridgeError::bridge(
                    code::PREVIEW_UNSUPPORTED,
                    "the engine truthfully declines this preview kind (metadata still shown)",
                ))
            }
            _ => {
                // Hex fallback (ADR-003 bounded payload): the default view.
                if size == 0 {
                    return Err(BridgeError::bridge(
                        code::PREVIEW_MISSING_DATA,
                        "the node has no readable content",
                    ));
                }
                let bytes = read_bytes(pv, HEX_READ as u64)?;
                Ok(json!({
                    "kind": "hex",
                    "bytes": bytes,
                    "totalSize": size,
                    "baseOffset": 0,
                }))
            }
        }
    })();
    unsafe { abi::cr_ui_preview_close(pv) };
    payload
}

#[tauri::command]
pub async fn preview_open(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    node_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || preview_payload(&state, &scan_id, &node_id, false))
        .await
        .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn preview_thumbnail(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    node_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || preview_payload(&state, &scan_id, &node_id, true))
        .await
        .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
