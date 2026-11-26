# SB-A-01 fix evidence — the per-query metadata deep clone (P1)

## Defect (before)

`metas_snapshot` (bridge/state.rs) cloned the ENTIRE `Vec<NodeMeta>`
metadata cache — two heap `String`s per row (`name`, `path`) — on EVERY
results query: each 200-row page fetch (`results_query` → `query_page`),
every `results_count`, every tree-page expansion, every node-detail, and
every re-fetch triggered by the scan-progress invalidations (tree+results+
count are invalidated at every 10% progress bucket and every
lost-candidate flush, throttled to ~1/s). For a 1M-file tree that is
millions of string copies per page fetch — O(total tree rows) of work to
serve a 200-row page, exactly while the user is scrolling results or
watching a live scan. The same O(n) clone repeated for the recovery
enqueue paging loop.

## Fix (after)

The cache is now shared as `Arc<Vec<NodeMeta>>`:
- `SnapshotState.metas: Option<Arc<Vec<NodeMeta>>>` (state.rs:242)
- `ensure_metas` stores `Some(Arc::new(metas))` — one wrap at build time
- `metas_snapshot` returns `(Arc<ScanEntry>, Arc<Vec<NodeMeta>>, bool)` —
  `snap.metas.clone()` under the snapshot lock is now ONE refcount bump
  instead of the O(n) row/string copy.
Soundness: the cache is generation-stable under the snapshot mutex — a new
snapshot generation replaces the `Arc` wholesale under the lock; readers
either see the old generation (their Arc keeps it alive) or the new one.
All consumers are read-only (`.iter()`, `.get()`, `.len()`, `&[NodeMeta]`
helper params via deref coercion — verified by grep: zero mutation or
Vec-ownership patterns on `metas` across the bridge).

Arithmetic (before → after per `metas_snapshot` call, 1M-row tree):
- before: 1M `NodeMeta` row copies + 2M heap string allocations/copies
  (name+path), held for the duration of payload assembly;
- after: one 16-byte refcount bump; the payload assembly then reads through
  the Arc (the rows are copied lazily only where JSON values are built —
  the pre-existing behavior for the served page).

## Proof status (honest)

The Rust toolchain is not installable in this Linux sandbox, so the
compile is proven by the pushed `bridge-integration` CI lane (Windows:
builds the real engine, links the real bridge, `cargo test`) — the change
is a type-level swap with all consumers verified read-only by grep; the
ui-ci lane additionally runs `cargo check` on the bridge crate. The
allocation arithmetic above is the documented bound (a before/after heap
profile on Windows remains the runtime OPEN-ITEMS capture).
