//! commands/results.rs — results_tree / results_query / results_count /
//! node_detail (contracts/command-surface.json; type-maps §4-§5).
//!
//! Node id scheme (bridge-minted, opaque to the UI, intentionally matching
//! the retired simulator's visible format so zero components change):
//!   files `f<snapshot index>`, dirs `d<snapshot index>`, plus the
//!   tree-navigation ids 'root' / 'lost-paths' / 'raw-all' / 'lost-<idx>' /
//!   'c<idx>' / 'ext:<cat>:<ext>'.
//!
//! ONE filter implementation serves BOTH live scans and loaded sessions
//! (G-7): the engine's cr_ui_query_run cannot express the contract's
//! 'existing' atom (active-only structured files — no deleted_never knob
//! exists in cr_ui_query), so the bridge filters its own metadata cache for
//! both. Semantics are therefore identical mid-scan and post-scan, and the
//! REAL partial tree replaces the simulator's visibleFraction heuristic.
//! docs/INTEGRATION-VALIDATION.md records the measured cost; G-7 keeps the
//! future engine-side query migration as the follow-up.

use std::sync::Arc;

use serde_json::{json, Value};

use crate::bridge::abi::CR_UI_SNAP_NONE;
use crate::bridge::error::{code, BridgeError, BridgeResult};
use crate::bridge::filefilter::{category_bit, category_label};
use crate::bridge::state::{extension_of, metas_snapshot, BridgeState, NodeMeta, ScanEntry};
use crate::bridge::util::iso_from_filetime;

/// The UI's category-chip order (mock CATEGORY_IDS parity) → engine bits.
const CATEGORY_ORDER: [&str; 8] = [
    "graphics",
    "video",
    "audio",
    "document",
    "email",
    "archive",
    "bookmarks",
    "other",
];

const MS_1601_TO_1970: u64 = 11_644_473_600_000;

/// Parse the fixed ISO shape this bridge (and `new Date().toISOString()`)
/// produce: YYYY-MM-DDTHH:MM:SS[.sss]Z -> unix ms. None on mismatch.
fn parse_iso_unix_ms(s: &str) -> Option<i64> {
    let b = s.as_bytes();
    if b.len() < 20 || b[4] != b'-' || b[7] != b'-' || (b[10] != b'T' && b[10] != b' ') || b[13] != b':' || b[16] != b':' {
        return None;
    }
    let num = |from: usize, len: usize| -> Option<i64> {
        s.get(from..from + len)?.parse::<i64>().ok()
    };
    let (y, mo, d) = (num(0, 4)?, num(5, 2)?, num(8, 2)?);
    let (h, mi, sec) = (num(11, 2)?, num(14, 2)?, num(17, 2)?);
    // days from civil (Hinnant) — same algorithm family as util's inverse.
    let yy = if mo <= 2 { y - 1 } else { y };
    let era = yy.div_euclid(400);
    let yoe = yy - era * 400; // [0, 399]
    let mp = if mo > 2 { mo - 3 } else { mo + 9 };
    let doy = (153 * mp + 2) / 5 + d - 1;
    let doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    let days = era * 146_097 + doe - 719_468;
    Some(days * 86_400_000 + h * 3_600_000 + mi * 60_000 + sec * 1000)
}

fn iso_to_filetime(s: &str) -> u64 {
    match parse_iso_unix_ms(s) {
        Some(ms) => ((ms + MS_1601_TO_1970 as i64).max(0) as u64) * 10_000,
        None => 0,
    }
}

/// The incoming ResultsFilter (contract shape, camelCase from TS).
#[derive(Default)]
struct FilterIn {
    node_id: Option<String>,
    tree_mode: String,
    search: Option<String>,
    categories: Vec<String>,
    date_from: Option<String>,
    date_to: Option<String>,
    size_min: u64,
    size_max: u64,
    extensions: Vec<String>,
    origins: Vec<String>,
    hide_empty_files: bool,
}

fn parse_filter(v: &Value) -> FilterIn {
    let s = |k: &str| v.get(k).and_then(|x| x.as_str()).map(|x| x.to_string());
    let mut f = FilterIn {
        node_id: s("nodeId"),
        tree_mode: s("treeMode").unwrap_or_else(|| "path".into()),
        search: s("search"),
        categories: v
            .get("categories")
            .and_then(|x| x.as_array())
            .map(|a| a.iter().filter_map(|c| c.as_str().map(String::from)).collect())
            .unwrap_or_default(),
        date_from: s("dateFrom"),
        date_to: s("dateTo"),
        size_min: v.get("sizeMinBytes").and_then(|x| x.as_u64()).unwrap_or(0),
        size_max: v
            .get("sizeMaxBytes")
            .and_then(|x| x.as_u64())
            .unwrap_or(0),
        extensions: v
            .get("extensions")
            .and_then(|x| x.as_array())
            .map(|a| {
                a.iter()
                    .filter_map(|e| e.as_str())
                    .map(|e| e.trim_start_matches('.').to_lowercase())
                    .collect()
            })
            .unwrap_or_default(),
        origins: v
            .get("origins")
            .and_then(|x| x.as_array())
            .map(|a| a.iter().filter_map(|o| o.as_str().map(String::from)).collect())
            .unwrap_or_default(),
        hide_empty_files: v.get("hideEmptyFiles").and_then(|x| x.as_bool()).unwrap_or(false),
    };
    if f.size_max == 0 {
        f.size_max = u64::MAX; // contract: null/0 = unbounded
    }
    f
}

/// The resolved predicate (nodeId semantics + entry scope + filter atoms).
struct Pred {
    origin_mask: u8,             // allowed origin VALUES (bit 1<<origin); 0xF = all
    require_active_only: bool,   // 'existing' atom: structured && deleted == 0
    require_deleted: bool,       // 'deleted' atom: structured && deleted != 0
    path_prefix: Option<String>, // lowercase '/'-joined; scope or subtree
    category_bits: u8,           // 0 = all
    ext_exact: Option<String>,   // ext:<cat>:<ext> leaves
    search: Option<String>,
    size_min: u64,
    size_max: u64,
    date_from_ft: u64,
    date_to_ft: u64,
    extensions: Vec<String>,
    hide_empty_files: bool,
}

fn starts_with_ci(hay: &str, prefix: &str) -> bool {
    hay.len() >= prefix.len()
        && hay[..prefix.len()].eq_ignore_ascii_case(prefix)
}

fn resolve_pred(
    entry: &Arc<ScanEntry>,
    f: &FilterIn,
    metas: &[NodeMeta],
) -> BridgeResult<Pred> {
    let mut p = Pred {
        origin_mask: 0xF,
        require_active_only: false,
        require_deleted: false,
        path_prefix: entry.path_scope.clone(),
        category_bits: 0,
        ext_exact: None,
        search: f.search.as_deref().map(|s| s.to_lowercase()),
        size_min: f.size_min,
        size_max: f.size_max,
        date_from_ft: f.date_from.as_deref().map(iso_to_filetime).unwrap_or(0),
        date_to_ft: f.date_to.as_deref().map(iso_to_filetime).unwrap_or(0),
        extensions: f.extensions.clone(),
        hide_empty_files: f.hide_empty_files,
    };
    if p.hide_empty_files && p.size_min < 1 {
        p.size_min = 1;
    }

    // nodeId semantics (mock's visible tree contract + ratified subtree rule)
    let node_id = f.node_id.as_deref();
    match (f.tree_mode.as_str(), node_id) {
        ("path", Some("root")) => p.origin_mask = 1,
        ("path", Some("lost-paths")) => p.origin_mask = 4,
        ("path", Some("raw-all")) => p.origin_mask = 2,
        ("path", Some(id)) if id.starts_with("lost-") => {
            let idx: usize = id["lost-".len()..]
                .parse()
                .map_err(|_| bad_node(id))?;
            let m = metas.get(idx).ok_or_else(|| bad_node(id))?;
            p.origin_mask = 4;
            p.path_prefix = join_prefix(entry, &m.path);
        }
        ("path", Some(id)) if id.starts_with('d') => {
            let idx: usize = id[1..].parse().map_err(|_| bad_node(id))?;
            let m = metas.get(idx).ok_or_else(|| bad_node(id))?;
            p.origin_mask = 1;
            p.path_prefix = join_prefix(entry, &m.path);
        }
        ("type", Some(id)) if id.starts_with('c') => {
            let cat_idx: usize = id[1..].parse().unwrap_or(usize::MAX);
            if let Some(label) = CATEGORY_ORDER.get(cat_idx) {
                p.category_bits = category_bit(label);
            }
        }
        ("type", Some(id)) if id.starts_with("ext:") => {
            let mut parts = id["ext:".len()..].splitn(2, ':');
            let cat = parts.next().unwrap_or("");
            let ext = parts.next().unwrap_or("").to_lowercase();
            p.category_bits = category_bit(cat);
            if !ext.is_empty() {
                p.ext_exact = Some(ext);
            }
        }
        ("raw", Some(_)) => p.origin_mask = 2,
        _ => {}
    }

    // Filter-bar atoms (the UI's only origin shape is ['existing']).
    for o in &f.origins {
        match o.as_str() {
            "existing" => {
                p.origin_mask &= 1;
                p.require_active_only = true;
            }
            "deleted" => {
                p.origin_mask &= 1;
                p.require_deleted = true;
            }
            "raw" => p.origin_mask &= 2,
            "lost-path" => p.origin_mask &= 4,
            "unsaved" => p.origin_mask &= 8,
            _ => {}
        }
    }
    if !f.categories.is_empty() {
        let bits: u8 = f.categories.iter().filter_map(|c| {
            let b = category_bit(c);
            if b == 0 { None } else { Some(b) }
        }).fold(0u8, |acc, b| acc | b);
        p.category_bits = if p.category_bits == 0 { bits } else { p.category_bits & bits };
    }
    if p.origin_mask == 0 {
        p.origin_mask = 0xF; // contradictory selection degenerates to all
    }
    Ok(p)
}

fn bad_node(id: &str) -> BridgeError {
    BridgeError::bridge(
        code::UNKNOWN,
        format!("node id '{id}' does not resolve in this tree"),
    )
}

/// Combine the entry's G-11 scope with a subtree prefix: the subtree wins
/// when it sits inside the scope; outside the scope it matches nothing
/// (the unreachable marker can never prefix a real path).
fn join_prefix(entry: &Arc<ScanEntry>, subtree: &str) -> Option<String> {
    let trimmed = subtree.trim_end_matches('/').to_lowercase();
    if trimmed.is_empty() {
        return entry.path_scope.clone();
    }
    match &entry.path_scope {
        None => Some(trimmed),
        Some(scope) if starts_with_ci(&trimmed, scope) => Some(trimmed),
        Some(_) => Some("\u{0}unreachable".to_string()),
    }
}

fn matches_pred(m: &NodeMeta, p: &Pred) -> bool {
    if m.is_dir {
        return false; // the list shows files only
    }
    if p.origin_mask & (1u8 << (m.origin.min(7))) == 0 {
        return false;
    }
    if p.require_active_only && !(m.origin == 0 && m.deleted == 0) {
        return false;
    }
    if p.require_deleted && !(m.origin == 0 && m.deleted != 0) {
        return false;
    }
    if let Some(prefix) = &p.path_prefix {
        if !starts_with_ci(&m.path, prefix) {
            return false;
        }
    }
    if p.category_bits != 0 && p.category_bits & m.category == 0 {
        return false;
    }
    if let Some(ext) = &p.ext_exact {
        if extension_of(&m.name) != *ext {
            return false;
        }
    }
    if !p.extensions.is_empty() {
        let ext = extension_of(&m.name);
        if !p.extensions.contains(&ext) {
            return false;
        }
    }
    if m.size < p.size_min || m.size > p.size_max {
        return false;
    }
    if p.date_from_ft != 0 && m.modified_ft < p.date_from_ft {
        return false;
    }
    if p.date_to_ft != 0 && m.modified_ft > p.date_to_ft {
        return false;
    }
    if let Some(search) = &p.search {
        if !search.is_empty() && !m.name.to_lowercase().contains(search.as_str()) {
            return false;
        }
    }
    true
}

/// The ResultNode row (contract shape; type-maps §4 field maps).
fn result_node(m: &NodeMeta, idx: usize, scan_id: &str) -> Value {
    let extension = extension_of(&m.name);
    let extension_json = if extension.is_empty() {
        Value::Null
    } else {
        json!(format!(".{extension}"))
    };
    json!({
        "id": format!("f{idx}"),
        "scanId": scan_id,
        "kind": "file",
        "name": m.name,
        "path": (if m.origin == 1 { Value::Null } else { json!(m.path) }),
        "sizeBytes": m.size,
        "modifiedAt": m.modified_iso(),
        "extension": extension_json,
        "category": category_label(m.category),
        "confidence": m.confidence_label(),
        "origin": m.origin_label(),
        "childCount": Value::Null,
        // G-4: no renderer exists for video thumbnails — graphics only.
        "thumbnailAvailable": m.category == 1,
        "parentId": (if m.parent == CR_UI_SNAP_NONE {
            Value::Null
        } else {
            json!(format!("d{}", m.parent))
        }),
    })
}

// ---------------------------------------------------------------------------
// results_query / results_count
// ---------------------------------------------------------------------------

fn sort_matches(metas: &[NodeMeta], hits: &mut [usize], field: &str, dir_desc: bool) {
    match field {
        "name" => hits.sort_by(|a, b| {
            let cmp = metas[*a]
                .name
                .to_lowercase()
                .cmp(&metas[*b].name.to_lowercase())
                .then_with(|| metas[*a].name.cmp(&metas[*b].name));
            if dir_desc { cmp.reverse() } else { cmp }
        }),
        "modified" => hits.sort_by(|a, b| {
            let cmp = metas[*a].modified_ft.cmp(&metas[*b].modified_ft);
            if dir_desc { cmp.reverse() } else { cmp }
        }),
        "size" => hits.sort_by(|a, b| {
            let cmp = metas[*a].size.cmp(&metas[*b].size);
            if dir_desc { cmp.reverse() } else { cmp }
        }),
        "confidence" => hits.sort_by(|a, b| {
            let cmp = metas[*a]
                .confidence
                .cmp(&metas[*b].confidence)
                .then_with(|| metas[*a].name.cmp(&metas[*b].name));
            if dir_desc { cmp.reverse() } else { cmp }
        }),
        // "type": the engine's sort 7 — category, then extension, then name.
        _ => hits.sort_by(|a, b| {
            let ext = |m: &NodeMeta| extension_of(&m.name);
            let cmp = metas[*a]
                .category
                .cmp(&metas[*b].category)
                .then_with(|| ext(&metas[*a]).cmp(&ext(&metas[*b])))
                .then_with(|| metas[*a].name.cmp(&metas[*b].name));
            if dir_desc { cmp.reverse() } else { cmp }
        }),
    }
}

/// Paging + count request shape (keeps query_page's argument list canonical).
pub struct PageReq {
    pub offset: u64,
    pub limit: u32,
    pub count_only: bool,
}

pub fn query_page(
    state: &Arc<BridgeState>,
    scan_id: &str,
    filter: &Value,
    sort_field: &str,
    sort_desc: bool,
    page: PageReq,
) -> BridgeResult<Value> {
    let PageReq { offset, limit, count_only } = page;
    let t0 = std::time::Instant::now();
    let (entry, metas, _partial) = metas_snapshot(state, scan_id)?;
    let f = parse_filter(filter);
    let pred = resolve_pred(&entry, &f, &metas)?;

    let mut hits: Vec<usize> = Vec::new();
    let mut total_bytes = 0u64;
    for (i, m) in metas.iter().enumerate() {
        if matches_pred(m, &pred) {
            hits.push(i);
            total_bytes += m.size;
        }
    }
    let total = hits.len() as u64;
    if count_only {
        return Ok(json!(total));
    }
    sort_matches(&metas, &mut hits, sort_field, sort_desc);

    let page: Vec<Value> = hits
        .iter()
        .skip(offset as usize)
        .take(limit as usize)
        .map(|&i| result_node(&metas[i], i, scan_id))
        .collect();
    Ok(json!({
        "nodes": page,
        "total": total,
        "totalBytes": total_bytes,
        "offset": offset,
        "pageSize": limit,
        "elapsedMs": t0.elapsed().as_millis() as u64,
    }))
}

#[tauri::command]
pub async fn results_query(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    filter: Value,
    sort: Value,
    page: Value,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    let sort_field = sort.get("field").and_then(|f| f.as_str()).unwrap_or("name").to_string();
    let sort_desc = sort.get("direction").and_then(|d| d.as_str()) == Some("desc");
    let req = PageReq {
        offset: page.get("offset").and_then(|o| o.as_u64()).unwrap_or(0),
        limit: page.get("limit").and_then(|l| l.as_u64()).unwrap_or(200) as u32,
        count_only: false,
    };
    tauri::async_runtime::spawn_blocking(move || {
        query_page(&state, &scan_id, &filter, &sort_field, sort_desc, req)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

#[tauri::command]
pub async fn results_count(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    filter: Value,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    let req = PageReq { offset: 0, limit: 0, count_only: true };
    tauri::async_runtime::spawn_blocking(move || {
        query_page(&state, &scan_id, &filter, "name", false, req)
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// results_tree
// ---------------------------------------------------------------------------

/// One tree row's composition (keeps tree_node's argument list canonical).
struct TreeSpec<'a> {
    id: &'a str,
    parent_id: Option<&'a str>,
    label: &'a str,
    icon: &'a str,
    child_count: u64,
    file_count: u64,
    size_bytes: u64,
    origin: &'a str,
    special: Option<&'a str>,
}

fn tree_node(spec: TreeSpec<'_>) -> Value {
    let mut v = json!({
        "id": spec.id,
        "parentId": spec.parent_id,
        "label": spec.label,
        "icon": spec.icon,
        "childCount": spec.child_count,
        "fileCount": spec.file_count,
        "sizeBytes": spec.size_bytes,
        "origin": spec.origin,
    });
    if let Some(s) = spec.special {
        v["special"] = json!(s);
    }
    v
}

/// Lost-file aggregates under a dir's path prefix.
fn lost_under(metas: &[NodeMeta], prefix: &str) -> (u64, u64) {
    let mut count = 0u64;
    let mut size = 0u64;
    for m in metas {
        if !m.is_dir && m.origin == 2 && starts_with_ci(&m.path, prefix) {
            count += 1;
            size += m.size;
        }
    }
    (count, size)
}

pub fn tree_page(
    state: &Arc<BridgeState>,
    scan_id: &str,
    parent_id: Option<String>,
    mode: &str,
) -> BridgeResult<Value> {
    let (entry, metas, _partial) = metas_snapshot(state, scan_id)?;
    let scope = entry.path_scope.clone();
    let mut nodes: Vec<Value> = Vec::new();

    match mode {
        "type" => {
            if let Some(pid) = &parent_id {
                if let Some(cat) = pid.strip_prefix('c') {
                    let cat_idx: usize = cat.parse().unwrap_or(usize::MAX);
                    if let Some(label) = CATEGORY_ORDER.get(cat_idx) {
                        let bit = category_bit(label);
                        // Extension leaves: aggregate per extension.
                        let mut exts: std::collections::BTreeMap<String, (u64, u64)> =
                            std::collections::BTreeMap::new();
                        for m in &metas {
                            if m.is_dir || m.category != bit || m.origin == 3 {
                                continue;
                            }
                            if let Some(s) = &scope {
                                if !starts_with_ci(&m.path, s) {
                                    continue;
                                }
                            }
                            let e = extension_of(&m.name);
                            let slot = exts.entry(e).or_insert((0, 0));
                            slot.0 += 1;
                            slot.1 += m.size;
                        }
                        for (ext, (count, size)) in exts {
                            nodes.push(tree_node(TreeSpec {
            id: &format!("ext:{label}:{ext}"),
            parent_id: Some(pid),
            label: &ext.to_uppercase(),
            icon: "directory",
            child_count: 0,
            file_count: count,
            size_bytes: size,
            origin: "existing",
            special: None,
        }));
                        }
                    }
                }
            } else {
                for (idx, label) in CATEGORY_ORDER.iter().enumerate() {
                    let bit = category_bit(label);
                    let mut count = 0u64;
                    let mut size = 0u64;
                    for m in &metas {
                        if m.is_dir || m.category != bit || m.origin == 3 {
                            continue;
                        }
                        if let Some(s) = &scope {
                            if !starts_with_ci(&m.path, s) {
                                continue;
                            }
                        }
                        count += 1;
                        size += m.size;
                    }
                    nodes.push(tree_node(TreeSpec {
            id: &format!("c{idx}"),
            parent_id: None,
            label,
            icon: "category",
            child_count: if count > 0 { 1 } else { 0 },
            file_count: count,
            size_bytes: size,
            origin: "existing",
            special: None,
        }));
                }
            }
        }
        "raw" => {
            let (count, size) = origin_sums(&metas, 1, scope.as_deref());
            nodes.push(tree_node(TreeSpec {
            id: "raw-all",
            parent_id: None,
            label: "raw",
            icon: "group",
            child_count: 0,
            file_count: count,
            size_bytes: size,
            origin: "raw",
            special: Some("raw-found"),
        }));
        }
        _ => {
            // path mode
            match parent_id.as_deref() {
                None => {
                    if let Some(scope_str) = &scope {
                        // Scoped scan: the scanned folder itself is the root.
                        let root_idx = metas
                            .iter()
                            .position(|m| m.is_dir && m.path.eq_ignore_ascii_case(scope_str));
                        if let Some(idx) = root_idx {
                            let m = &metas[idx];
                            let top_dirs = metas
                                .iter()
                                .filter(|x| x.is_dir && x.parent == idx as u32)
                                .count() as u64;
                            nodes.push(tree_node(TreeSpec {
            id: &format!("d{idx}"),
            parent_id: None,
            label: &m.name,
            icon: "directory",
            child_count: top_dirs,
            file_count: m.file_count,
            size_bytes: m.tree_size,
            origin: "existing",
            special: None,
        }));
                        }
                    } else {
                        // Structured totals (existing + deleted + lost files
                        // that live in the directory tree; carved excluded —
                        // they hang under the synthetic raw group).
                        let (mut fc, mut fs) = (0u64, 0u64);
                        let mut top_dirs = 0u64;
                        for m in &metas {
                            if m.is_dir {
                                if m.path.is_empty() || m.path.contains('/') || m.origin == 3 {
                                    continue;
                                }
                                top_dirs += 1;
                            } else if m.origin == 0 || m.origin == 2 {
                                fc += 1;
                                fs += m.size;
                            }
                        }
                        nodes.push(tree_node(TreeSpec {
            id: "root",
            parent_id: None,
            label: "root",
            icon: "device",
            child_count: top_dirs,
            file_count: fc,
            size_bytes: fs,
            origin: "existing",
            special: None,
        }));
                        let (lost_c, lost_s) = origin_sums(&metas, 2, None);
                        let lost_top_dirs = lost_dirs(&metas, 0, true).len() as u64;
                        nodes.push(tree_node(TreeSpec {
            id: "lost-paths",
            parent_id: None,
            label: "lost-paths",
            icon: "group",
            child_count: if lost_top_dirs > 0 { 1 } else { 0 },
            file_count: lost_c,
            size_bytes: lost_s,
            origin: "lost-path",
            special: Some("lost-path"),
        }));
                        let (raw_c, raw_s) = origin_sums(&metas, 1, None);
                        nodes.push(tree_node(TreeSpec {
            id: "raw-all",
            parent_id: None,
            label: "raw",
            icon: "group",
            child_count: 0,
            file_count: raw_c,
            size_bytes: raw_s,
            origin: "raw",
            special: Some("raw-found"),
        }));
                    }
                }
                Some("root") => {
                    // Depth-1 dirs (their paths carry no '/').
                    for (idx, m) in metas.iter().enumerate() {
                        if !m.is_dir || m.origin == 3 {
                            continue;
                        }
                        if m.path.is_empty() || m.path.contains('/') {
                            continue;
                        }
                        if !in_scope(&m.path, scope.as_deref()) {
                            continue;
                        }
                        nodes.push(dir_tree_node(idx, m, Some("root")));
                    }
                }
                Some("lost-paths") => {
                    for idx in lost_dirs(&metas, 0, true) {
                        let m = &metas[idx];
                        let (c, s) = lost_under(&metas, &m.path);
                        let deeper = lost_dirs(&metas, idx, false).len() as u64;
                        nodes.push(tree_node(TreeSpec {
            id: &format!("lost-{idx}"),
            parent_id: Some("lost-paths"),
            label: &m.name,
            icon: "directory",
            child_count: deeper,
            file_count: c,
            size_bytes: s,
            origin: "lost-path",
            special: None,
        }));
                    }
                }
                Some(pid) if pid.starts_with("lost-") => {
                    let idx: usize = pid["lost-".len()..].parse().map_err(|_| bad_node(pid))?;
                    for child in lost_dirs(&metas, idx, false) {
                        let m = &metas[child];
                        let (c, s) = lost_under(&metas, &m.path);
                        let deeper = lost_dirs(&metas, child, false).len() as u64;
                        nodes.push(tree_node(TreeSpec {
            id: &format!("lost-{child}"),
            parent_id: Some(pid),
            label: &m.name,
            icon: "directory",
            child_count: deeper,
            file_count: c,
            size_bytes: s,
            origin: "lost-path",
            special: None,
        }));
                    }
                }
                Some(pid) if pid.starts_with('d') => {
                    let idx: usize = pid[1..].parse().map_err(|_| bad_node(pid))?;
                    for (ci, m) in metas.iter().enumerate() {
                        if m.is_dir && m.parent == idx as u32 {
                            if let Some(s) = &scope {
                                if !starts_with_ci(&m.path, s) {
                                    continue;
                                }
                            }
                            nodes.push(dir_tree_node(ci, m, Some(pid)));
                        }
                    }
                }
                Some("raw-all") => { /* carved files carry no path structure */ }
                Some(other) => return Err(bad_node(other)),
            }
        }
    }
    Ok(json!({ "nodes": nodes, "parentHasMore": false }))
}

fn dir_tree_node(idx: usize, m: &NodeMeta, parent: Option<&str>) -> Value {
    tree_node(TreeSpec {
        id: &format!("d{idx}"),
        parent_id: parent,
        label: &m.name,
        icon: "directory",
        child_count: m.child_count as u64,
        file_count: m.file_count,
        size_bytes: m.tree_size,
        origin: "existing",
        special: None,
    })
}

fn in_scope(path: &str, scope: Option<&str>) -> bool {
    match scope {
        None => true,
        Some(s) => starts_with_ci(path, s),
    }
}

fn origin_sums(metas: &[NodeMeta], origin: u32, scope: Option<&str>) -> (u64, u64) {
    let mut c = 0u64;
    let mut s = 0u64;
    for m in metas {
        if !m.is_dir && m.origin == origin && in_scope(&m.path, scope) {
            c += 1;
            s += m.size;
        }
    }
    (c, s)
}

/// Dir indices containing lost files at the requested relation:
/// `top=true` → shallowest lost dirs (no lost-containing ancestor);
/// `top=false` → direct children of `under` that contain lost files.
fn lost_dirs(metas: &[NodeMeta], under: usize, top: bool) -> Vec<usize> {
    let mut out = Vec::new();
    for (idx, m) in metas.iter().enumerate() {
        if !m.is_dir || idx == under {
            continue;
        }
        if top {
            // candidate must have no lost-containing ancestor dir
            let mut has_ancestor = false;
            let mut p = m.parent;
            while p != CR_UI_SNAP_NONE && (p as usize) < metas.len() {
                if lost_under(metas, &metas[p as usize].path).0 > 0 {
                    has_ancestor = true;
                    break;
                }
                p = metas[p as usize].parent;
            }
            if !has_ancestor && lost_under(metas, &m.path).0 > 0 {
                out.push(idx);
            }
        } else if m.parent == under as u32 && lost_under(metas, &m.path).0 > 0 {
            out.push(idx);
        }
    }
    out
}

#[tauri::command]
pub async fn results_tree(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    parent_id: Option<String>,
    mode: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || tree_page(&state, &scan_id, parent_id, &mode))
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

// ---------------------------------------------------------------------------
// node_detail
// ---------------------------------------------------------------------------

#[tauri::command]
pub async fn node_detail(
    state: tauri::State<'_, Arc<BridgeState>>,
    scan_id: String,
    node_id: String,
) -> Result<Value, BridgeError> {
    let state = state.inner().clone();
    tauri::async_runtime::spawn_blocking(move || {
        let (entry, metas, _partial) = metas_snapshot(&state, &scan_id)?;
        let idx: usize = node_id
            .strip_prefix('f')
            .and_then(|r| r.parse().ok())
            .ok_or_else(|| bad_node(&node_id))?;
        let m = metas.get(idx).ok_or_else(|| bad_node(&node_id))?;
        let node = result_node(m, idx, &scan_id);

        // Physical path: real on-disk location exists only for structured
        // rows on a lettered volume (raw/lost rows have no live location).
        let letter = entry
            .target_json
            .get("letter")
            .and_then(|l| l.as_str())
            .map(|l| l.trim_end_matches(':').to_string());
        let full_physical_path = if m.origin == 0 {
            letter.map(|l| {
                format!(
                    "{l}:\\{}\\{}",
                    m.path.replace('/', "\\"),
                    m.name
                )
            })
        } else {
            None
        };

        // Confidence reason composed from REAL evidence fields (type-maps §4).
        let reason = compose_confidence_reason(m);

        Ok(json!({
            "node": node,
            "fullPhysicalPath": full_physical_path,
            "createdAt": iso_from_filetime(m.created_ft),
            "accessedAt": iso_from_filetime(m.accessed_ft),
            "confidenceReason": reason,
            "badSectorsOverlap": m.confidence <= 1,
            "clusterCount": m.extent_count,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}

fn compose_confidence_reason(m: &NodeMeta) -> Option<String> {
    let grade = match m.grade {
        2 => "recoverability high",
        1 => "recoverability average",
        _ => "recoverability low",
    };
    if m.origin == 1 {
        let mut s = format!("Carved from the raw stream ({grade})");
        if m.hash_valid != 0 {
            s.push_str("; content hash verified");
        }
        if m.overlap != 0 {
            s.push_str("; overlap rule flagged a duplicate");
        }
        return Some(s);
    }
    let mut s = match m.confidence {
        4 => "Headers and extents fully readable".to_string(),
        3 => "Slight fragmentation, all clusters readable".to_string(),
        2 => "Some clusters marked pending reallocation".to_string(),
        1 => "Overlaps an unreadable area".to_string(),
        _ => "Not yet verified".to_string(),
    };
    s.push_str(&format!(" ({grade})"));
    Some(s)
}

// ---------------------------------------------------------------------------
// T14 measurement tests — #[ignore]d, NEVER part of the normal ui-ci lane
// (which runs cargo check + clippy only). The validation-evidence workflow
// runs them explicitly with `cargo test --release -- --ignored --nocapture`
// and records the JSON lines into docs/INTEGRATION-VALIDATION/t14-bridge.md
// (docs/INTEGRATION-VALIDATION.md "Pending evidence" — the G-7 filter cost
// over the metadata cache at 10^5/10^6 scales + the NodeMeta memory bound).
// ---------------------------------------------------------------------------
#[cfg(test)]
mod t14_validation {
    use super::{matches_pred, Pred};
    use crate::bridge::state::NodeMeta;
    use std::alloc::{GlobalAlloc, Layout, System};
    use std::sync::atomic::{AtomicU64, Ordering};
    use std::time::Instant;

    /// Deterministic synthetic metas: a realistic name/path/category mix
    /// (25% dirs — excluded by the list filter, like real trees — plus a
    /// spread of origins/deleted states/categories/extensions).
    fn synth_metas(n: usize) -> Vec<NodeMeta> {
        let exts = ["jpg", "png", "pdf", "docx", "mp4", "zip", "txt", "xlsx"];
        (0..n)
            .map(|i| {
                let ext = exts[i % exts.len()];
                let is_dir = i % 4 == 0;
                let name = if is_dir {
                    format!("d{i}")
                } else {
                    format!("f{i}.{ext}")
                };
                NodeMeta {
                    node_id: i as u64,
                    parent: if i == 0 { u32::MAX } else { (i / 10) as u32 },
                    name: name.clone(),
                    path: format!("users/public/docs/{name}"),
                    is_dir,
                    size: if is_dir { 0 } else { (i as u64 % 900_000) + 100 },
                    modified_ft: 132_000_000_000_000_000 + (i as u64) * 3_600_000_000,
                    created_ft: 131_000_000_000_000_000,
                    accessed_ft: 133_000_000_000_000_000,
                    origin: (i % 4) as u32,
                    deleted: if i % 3 == 0 { 0 } else { 1 },
                    confidence: (i % 5) as u32,
                    grade: (i % 3) as u32,
                    extent_count: (i % 7) as u32,
                    hash_valid: (i % 2) as u32,
                    overlap: 0,
                    category: if is_dir { 0 } else { 1 + (i % 8) as u8 },
                    file_count: 0,
                    tree_size: 0,
                    child_count: 0,
                }
            })
            .collect()
    }

    fn timed_pass(metas: &[NodeMeta], pred: &Pred, label: &str) {
        let t0 = Instant::now();
        let hits: usize = metas.iter().filter(|m| matches_pred(m, pred)).count();
        let dt = t0.elapsed();
        let per_node_ns = dt.as_nanos() as f64 / metas.len() as f64;
        println!(
            "T14JSON: {{\"leg\": \"g7-filter-{label}\", \"nodes\": {}, \"hits\": {}, \"totalMs\": {:.3}, \"nsPerNode\": {:.1}}}",
            metas.len(),
            hits,
            dt.as_secs_f64() * 1000.0,
            per_node_ns
        );
    }

    #[test]
    #[ignore = "T14 measurement — the validation-evidence workflow runs it (cargo test --release -- --ignored --nocapture)"]
    fn g7_filter_cost_over_metadata_cache() {
        for &scale in &[100_000usize, 1_000_000] {
            let metas = synth_metas(scale);
            // The two pred shapes the UI sends: the wide 'all files' pass and
            // the scoped + category-filtered pass (the worst case that still
            // walks every row: a scope prefix + a category bit + a search).
            let all_files = Pred {
                origin_mask: 0xF,
                require_active_only: false,
                require_deleted: false,
                path_prefix: None,
                category_bits: 0,
                ext_exact: None,
                search: None,
                size_min: 0,
                size_max: u64::MAX,
                date_from_ft: 0,
                date_to_ft: 0,
                extensions: Vec::new(),
                hide_empty_files: false,
            };
            timed_pass(&metas, &all_files, "all");
            // The scoped pred (the worst case that still walks every row:
            // a scope prefix + a category bit + a search) consumes all_files
            // via the struct-update move — measured after its own pass.
            let scoped = Pred {
                path_prefix: Some("users/public/docs".to_string()),
                category_bits: 0b0000_0101, // graphics + documents
                search: Some("f1".to_string()),
                ..all_files
            };
            timed_pass(&metas, &scoped, "scoped");
        }
    }

    /// Counting allocator — measures the NodeMeta cache's real heap cost.
    /// Module-scope static (a #[global_allocator] must live at module scope);
    /// it only exists in the TEST binary and wraps System 1:1. The counter is
    /// growth-only (alloc/realloc bytes); the tests snapshot deltas, and the
    /// workflow runs with --test-threads=1 so no other test allocates inside
    /// a measurement window.
    struct Counting;
    static ALLOCATED: AtomicU64 = AtomicU64::new(0);

    #[global_allocator]
    static A: Counting = Counting;

    unsafe impl GlobalAlloc for Counting {
        unsafe fn alloc(&self, l: Layout) -> *mut u8 {
            let p = System.alloc(l);
            if !p.is_null() {
                ALLOCATED.fetch_add(l.size() as u64, Ordering::Relaxed);
            }
            p
        }
        unsafe fn dealloc(&self, p: *mut u8, l: Layout) {
            System.dealloc(p, l)
        }
        unsafe fn realloc(&self, p: *mut u8, l: Layout, n: usize) -> *mut u8 {
            let q = System.realloc(p, l, n);
            if !q.is_null() {
                ALLOCATED.fetch_add(n as u64, Ordering::Relaxed);
            }
            q
        }
    }

    #[test]
    #[ignore = "T14 measurement — the validation-evidence workflow runs it (cargo test --release -- --ignored --nocapture --test-threads=1)"]
    fn node_meta_cache_memory_bound() {
        for &scale in &[100_000usize, 1_000_000] {
            ALLOCATED.store(0, Ordering::Relaxed);
            let before = ALLOCATED.load(Ordering::Relaxed);
            let metas = synth_metas(scale);
            let after = ALLOCATED.load(Ordering::Relaxed);
            let bytes = after.saturating_sub(before);
            let per_node = bytes / scale as u64;
            println!(
                "T14JSON: {{\"leg\": \"nodemeta-memory\", \"nodes\": {}, \"heapBytes\": {}, \"bytesPerNode\": {}, \"structSizeBytes\": {}}}",
                scale,
                bytes,
                per_node,
                std::mem::size_of::<NodeMeta>()
            );
            // The documented bound: the cache is O(n) over the tree with
            // small per-node strings (INTEGRATION-VALIDATION "memory bounds"
            // row). 1 KiB/node would already be pathological.
            assert!(
                per_node < 1024,
                "per-node cache cost {per_node} exceeds the documented bound"
            );
            drop(metas);
        }

        // The static bounded structures, pinned so the doc row stays true:
        // bad-sector map cap 4,096 rows; the scan event channel is 512 deep
        // and the recovery progress channel 64 (state.rs constants).
        println!(
            "T14JSON: {{\"leg\": \"static-bounds\", \"badSectorMapCap\": {}, \"scanEventChannel\": 512, \"recoveryProgressChannel\": 64}}",
            crate::bridge::state::BAD_SECTOR_MAP_CAP
        );
    }
}
