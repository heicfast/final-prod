//! commands/preview.rs — preview_open / preview_thumbnail (the payload
//! assembly; preview reads + closes are internal helpers, not commands —
//! the contract has no paging surface).
//!
//! Kind dispatch is bridge-side from ENGINE metadata (ADR-008): the UI never
//! classifies extensions. Payload bounds (ADR-003): image assembly capped at
//! 16 MiB (beyond -> preview-too-large — a truncated base64 prefix renders as
//! a corrupt image, which would be a lie); hex reads capped at 64 KiB; the
//! engine itself caps reads at 4 MiB/call.
//!
//! v4 surfaces:
//! - M-116 archive browse-inside: kind 'archive' assembles the engine's
//!   bounded inventory (archive_entry_count/_at, 512-entry hostile bound)
//!   into the 'archive' PreviewPayload — entries + truncated, never a
//!   fabricated listing.
//! - M-102 thumbnails: SESSION-kind nodes generate through
//!   thumbnail_generate (two-call probe -> alloc -> generate; EXIF-oriented,
//!   display-shaped RGBA, grade-promoting M-083) and cross as a BMP data URL
//!   (the bridge encodes the RGBA itself — a 40-line BI_RGB writer, no image
//!   dependency). LIVE scans keep the preview-read assembly (documented
//!   degradation, BF-13: snapshot-indexed thumbnails do not exist upstream).
//!
//! G-4 (no renderer): video/audio -> 'av' metadata payload with a null
//! thumbnail; document -> 'office' metadata with no extracted text. The
//! preview pane renders the truthful "preview not rendered" hint itself.
//!
//! Damage truth (cr_ui_preview_unreadable reports zero-filled spans): any
//! unreadable span -> preview-damaged. The payload shape carries no damage
//! annotation field, so returning zero-filled content as if intact would be
//! the lie; the typed refusal with preview.reason.damaged is the truth.

use std::sync::Arc;

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{check, code, BridgeError, BridgeResult};
use crate::bridge::state::{metas_snapshot, BridgeState, ScanEntry, ScanKind};
use crate::bridge::util::{base64_encode, fixed_str};

const IMAGE_CAP: u64 = 16 * 1024 * 1024;
const THUMBNAIL_CAP: u64 = 8 * 1024 * 1024;
const HEX_READ: u32 = 64 * 1024;
/// M-116's own hostile bound — the engine never lists more.
const ARCHIVE_ENTRY_CAP: usize = 512;
// (M-115's ≤20-byte small-node refusal is the ENGINE's own ERR_UNSUPPORTED
// — see session_thumbnail; the bridge does not re-check it, so no constant.)

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
fn resolve_node(entry: &Arc<ScanEntry>, node_id: &str) -> BridgeResult<(usize, u64)> {
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
        ScanKind::Session { session, .. } => unsafe {
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
            unsafe {
                abi::cr_ui_preview_read(pv, offset, buf.as_mut_ptr().cast(), want, &mut placed)
            },
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

/// The M-116 archive inventory (bounded at the engine's own 512-entry cap).
fn archive_entries(pv: *mut abi::CrUiPreview) -> BridgeResult<(Vec<Value>, bool)> {
    let mut count: u32 = 0;
    check(
        unsafe { abi::cr_ui_archive_entry_count(pv, &mut count) },
        "cr_ui_archive_entry_count",
    )?;
    let total = count as usize;
    let take = total.min(ARCHIVE_ENTRY_CAP);
    let mut rows = Vec::with_capacity(take);
    for i in 0..take {
        let mut e: abi::CrUiArchiveEntry = unsafe { std::mem::zeroed() };
        check(
            unsafe { abi::cr_ui_archive_entry_at(pv, i as u32, &mut e) },
            "cr_ui_archive_entry_at",
        )?;
        rows.push(json!({
            "name": fixed_str(&e.name),
            "size": e.size,
            "compressed": e.compressed,
            "crcPresent": e.crc_present != 0,
            "nameTruncated": e.name_truncated != 0,
        }));
    }
    Ok((rows, total > take))
}

/// Encode display-oriented RGBA8 (the engine's thumbnail output) as a BMP
/// data URL — a 32-bit BI_RGB writer, no image dependency. Rows are stored
/// bottom-up BGRX per the format; the browser renders `image/bmp` natively.
fn rgba_to_bmp_data_url(rgba: &[u8], width: u32, height: u32) -> String {
    let row = (width as usize) * 4;
    let mut bmp = Vec::with_capacity(54 + row * height as usize);
    // BITMAPFILEHEADER
    bmp.extend_from_slice(b"BM");
    let size = (54 + row * height as usize) as u32;
    bmp.extend_from_slice(&size.to_le_bytes());
    bmp.extend_from_slice(&0u32.to_le_bytes());
    bmp.extend_from_slice(&54u32.to_le_bytes());
    // BITMAPINFOHEADER
    bmp.extend_from_slice(&40u32.to_le_bytes());
    bmp.extend_from_slice(&(width as i32).to_le_bytes());
    bmp.extend_from_slice(&(height as i32).to_le_bytes());
    bmp.extend_from_slice(&1u16.to_le_bytes());
    bmp.extend_from_slice(&32u16.to_le_bytes());
    bmp.extend_from_slice(&0u32.to_le_bytes()); // BI_RGB
    bmp.extend_from_slice(&((row * height as usize) as u32).to_le_bytes());
    bmp.extend_from_slice(&2835u32.to_le_bytes());
    bmp.extend_from_slice(&2835u32.to_le_bytes());
    bmp.extend_from_slice(&0u32.to_le_bytes());
    bmp.extend_from_slice(&0u32.to_le_bytes());
    // pixels: bottom-up, BGRA byte order in the file (BMP is little-endian
    // BGRX; the engine's RGBA drops the alpha — BMP's reserved byte is 0).
    for y in (0..height as usize).rev() {
        let src = &rgba[y * row..y * row + row];
        for px in src.as_chunks::<4>().0 {
            bmp.push(px[2]); // B
            bmp.push(px[1]); // G
            bmp.push(px[0]); // R
            bmp.push(0); // reserved
        }
    }
    format!("data:image/bmp;base64,{}", base64_encode(&bmp))
}

/// The M-102 session-thumbnail path: two-call generate (probe -> alloc ->
/// generate). ERR_TRUNCATED is the engine's decode-REFUSAL marker (a damaged
/// stream the strict decoders reject) — surfaced as the typed
/// preview-damaged refusal, never as pixels. M-115's small-node refusal
/// (ERR_UNSUPPORTED at <= 20 bytes) degrades to the preview-read path only
/// for LIVE scans; sessions have no fallback (the node is addressed by id).
fn session_thumbnail(entry: &Arc<ScanEntry>, node_id: u64) -> BridgeResult<Option<Value>> {
    let dev = entry.device();
    if dev.is_null() {
        return Ok(None); // truthful: no source, no thumbnail
    }
    let ScanKind::Session { session, .. } = &entry.kind else {
        return Ok(None);
    };
    // Probe: generation runs, dims are decided, pixels are discarded.
    let mut info: abi::CrUiThumbnailInfo = unsafe { std::mem::zeroed() };
    let rc = unsafe {
        abi::cr_ui_thumbnail_generate(
            dev,
            session.get(),
            node_id,
            std::ptr::null(),
            std::ptr::null_mut(),
            0,
            &mut info,
        )
    };
    match rc {
        abi::CR_UI_OK => {}
        abi::CR_UI_ERR_TRUNCATED => {
            // decode refusal (ABI-CONTRACT 7i) — the typed damage truth
            return Err(BridgeError::bridge(
                code::PREVIEW_DAMAGED,
                "the engine's strict decoders refused this image (damaged or short stream)",
            ));
        }
        abi::CR_UI_ERR_UNSUPPORTED | abi::CR_UI_ERR_NO_MEMORY => {
            return Ok(None); // not a thumbnail-capable node / probe shape
        }
        _ => return Err(BridgeError::engine(rc, "cr_ui_thumbnail_generate(probe)")),
    }
    if info.width == 0 || info.height == 0 || info.bytes_needed == 0 {
        return Ok(None);
    }
    let cap = info.bytes_needed;
    let mut rgba = vec![0u8; cap as usize];
    let rc = unsafe {
        abi::cr_ui_thumbnail_generate(
            dev,
            session.get(),
            node_id,
            std::ptr::null(),
            rgba.as_mut_ptr(),
            cap,
            &mut info,
        )
    };
    check(rc, "cr_ui_thumbnail_generate")?;
    Ok(Some(
        json!({ "dataUrl": rgba_to_bmp_data_url(&rgba, info.width, info.height) }),
    ))
}

pub fn preview_payload(
    state: &Arc<BridgeState>,
    scan_id: &str,
    node_id: &str,
    thumbnail: bool,
) -> BridgeResult<Value> {
    let (entry, _metas, _partial) = metas_snapshot(state, scan_id)?;
    let (idx, node_id_u64) = resolve_node(&entry, node_id)?;

    // The M-102 session path (thumbnail requests only — full previews still
    // read the original bytes through the preview handle).
    if thumbnail && matches!(entry.kind, ScanKind::Session { .. }) {
        if let Some(v) = session_thumbnail(&entry, node_id_u64)? {
            return Ok(v);
        }
        // fall through to the preview-read assembly (the node may be a
        // non-image or an unsupported format — the payload below is truthful)
    }

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
                    let data_url = format!(
                        "data:{};base64,{}",
                        mime_for(&format),
                        base64_encode(&bytes)
                    );
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
            "archive" => {
                // M-116: the engine's bounded browse-inside inventory.
                let (entries, truncated) = archive_entries(pv)?;
                Ok(json!({
                    "kind": "archive",
                    "entries": entries,
                    "truncated": truncated,
                }))
            }
            "document" => {
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
