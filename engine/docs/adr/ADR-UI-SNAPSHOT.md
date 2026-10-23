# ADR-UI-SNAPSHOT - mid-scan result access via immutable value snapshots

Status: ACCEPTED (G-02/G-03, implemented `include/cr/tree/cr_snapshot.h`)

## Context

The UI must enumerate results and recover selected files while a scan is
running. `ResultTree` serializes all mutations on one mutex, but unsynchronized
traversal is C++-UB (add_file writes `next_sibling_` of an already-visible tail
node), and mid-scan recovery reads node extents through the live tree.

## Options considered

1. **Reader-writer lock on the tree.** Long-lived UI traversals hold the read
   side; engine emitters stall behind them (throughput death, priority inversion
   under TSan-visible contention).
2. **Link-stability / epoch schemes.** Nodes are arena-stable, but the sibling
   write of a visible tail node keeps unsynchronized traversal a data race
   regardless of link stability - C++ UB TSan can (correctly) flag.
3. **Copy-on-write VALUE snapshot: one O(n) copy under the existing mutex.**
   Race-free by construction (all reads under the lock; the snapshot owns every
   byte it exposes). Cost equals one `serialize_tree` pass (~150-300 ms at 1M
   nodes), paid at stage boundaries or on explicit demand.

## Decision

Option 3. `cr::tree::Snapshot` is a flat array of `SnapNode` rows (topology by
index + fully copied content), captured under `ResultTree::mutex_`.

- **SNAP_PARTIAL**: the capture API stamps `partial` - mid-stage captures never
  pretend to be final state (the orchestrator clears it at stage boundaries).
- **Byte parity**: `Snapshot::dfs` reproduces the live recursive
  first_child/next_sibling walk exactly - a snapshot-based serialization is
  byte-identical to the live one (session-parity contract, G-13).
- **G-03 recovery from snapshot**: `SafeWriter::enqueue(const Snapshot&, index,
  dir)` materializes writer-owned `FileProperties` (value store; no QueueItem
  copy hazards) and recovers reading only snapshot-owned bytes + the source
  device (concurrent-read safe by the IStorageDevice contract). The recovery
  determinism test proves byte-identical destination content vs the live path.

## Consequences

- The capture stalls emitters for its duration (documented, bounded by O(n)
  copy cost). Stage-boundary refresh amortizes this in production.
- Snapshot memory is transient (reader drops the previous on swap); the double
  tree+snapshot memory during capture is accepted.
- The UI integration (G-12 query over snapshots, cr_ui shim) consumes the same
  object - one materialization serves live listing AND loaded sessions.
