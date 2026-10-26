# MAJOR M-039 — Cross-engine sector-occupancy bitmap feed missing (og fills one per-volume bitmap from every engine; core's diskmap is visualization-only)

**Severity:** MAJOR
**Module:** carve, orchestration (cross-engine)
**Parameters affected:** A28 (carve target policy — FS-allocated regions), B29 (dedup), O37 (stage model — cross-stage data flow), P47 (throughput — wasted carve)
**Corpus references:** identical bit-set/test code in EXT4SearchDLL (`FUN_180012040` :14892-93, device-sized init :2763, fill from every extent `fillbitmap` :6866-6869), BtrfsSearchDLL (:2547-2551 per-node marking; **sparse-file-backed bitmap** CreateFileW + FSCTL_SET_SPARSE 0x900c4 + MapViewOfFile with new[] fallback :16096-16159), ZfsSearchDLL (:40576/:40599); the og feeds one occupancy picture from the FS engines to downstream phases.
**Core references:** `src/core/visualization/cr_diskmap.cpp:44-60` (in-memory viz diskmap only — not an input to the carve stage; grep-verified no cross-stage occupancy consumer). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-024` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

All og FS engines fill a sector-occupancy bitmap (in-memory, or mmap'd from a sparse file for large volumes) as they walk; that bitmap is available to later phases (the raw/carve phase can skip FS-allocated regions). Our core's only analogue is the visualization diskmap — nothing feeds the carve stage, so we re-carve regions the FS engines already accounted for (duplicate/contaminated candidates filtered only by the extent-dedup tree after the fact).

## What the reference does

The bit-set/test primitives are identical across the three engines (quoted); Btrfs's sparse-file backing solves the large-volume memory problem; the bitmaps are initialized device-sized and filled from every extent/node read.

## What our core does

Per-stage dedup (F9 extent dedup vs structured + prior carved, cr_carve.cpp) — post-hoc, not a pre-filter.

## Gap

No cross-stage occupancy input. Falsifiable: on a volume where carve overlap-filtering matters, the og's raw phase skips FS-allocated regions; ours carves them and relies on dedup to suppress (different candidate sets when extents are fragmented or partial).

## Impact

Throughput (carving already-accounted space) and candidate-set differences on fragmented volumes. No correctness loss when dedup works; the gap is efficiency + edge-case parity.

## Reproduction / verification

Corpus-derived (three engines' primitives quoted). Core: code read. UNVERIFIED-BY-EXECUTION (benchmark on a fragmented fixture would quantify the wasted carve; the bench harness can do this).

## Fix plan

1. Promote the orchestrator's stage model: FS engines emit their consumed-extent set (the emit path already builds extents per file — aggregate into a volume bitmap or interval set in the orchestrator).
2. Feed it to the carve stage as a skip-filter for stride positions whose header hit lies inside an FS-allocated run (keep the dedup tree as the safety net).
3. Memory shape: interval set first (simple); the og's sparse-file mmap is a Windows-specific optimization to adopt only if profiling demands.

## Tests to add

- `Carve.SkipsFsAllocatedRegions` — fixture with a structured file at offset X and a fake signature inside X's extent; assert no carve candidate at X (fails today).
- `Carve.StructuredBoundaryPartialOverlap` — signature straddling a structured extent boundary; assert the og-compatible disposition (dedup still guards).

## Related findings

M-005 (carve target policy — adjacent), S-015 (parallel leg, FIXED) (raw-signs bounds — same stage), O38 (stage weighting).

## Open questions

The og's exact downstream consumer (RAWSearchDLL's use of the bitmap, if any — OQ-X1 from the census) — decode before claiming user-observable parity beyond throughput; the architectural gap stands regardless.


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-039 commit — DFX24)
**Verified by:** `CarveEngine.SkipsFsAllocatedRegions`, `CarveEngine.StructuredBoundaryPartialOverlap`, `CarveEngine.OccupancyGateJumpsWholeInterval` — fail-before EXECUTED as an engine mutation (the gate's entry condition neutralized: 2/3 RED — the in-extent signature carves, the jump test sees no suppression; the straddling-boundary pin stays green by design — the og's bitmap gate checks the candidate POSITION, a before-interval start is not suppressed), reverted-green. Suite 895/895 linux-release; the full orchestrator suite stays green (the gate only suppresses positions the dedup tree already suppressed on the existing fixtures).
**Resolution notes:** The fix plan's three steps landed verbatim. (1) The orchestrator aggregates every STRUCTURED node's extents from the result tree after the engine passes (the emit path already built them per file — "aggregate into a volume bitmap or interval set in the orchestrator") into a sorted, coalesced [start, end) interval set, rebased slice-relative; carved nodes never feed the gate (they are the carve's own output — feeding them would suppress re-carve on resume; the og's bitmap is FS-engine-fed only). (2) `CarveFlags::occupied_ranges` feeds both dispatch layers: the grammar layer suppresses positions inside an interval and jumps to the interval END (the M-005 cluster-jump analog at extent granularity — coalesced extents make the jump skip whole files), the raw-sign 512-stride layer suppresses its probes the same way; the extent-dedup tree stays as the safety net (the finding's own requirement). (3) Memory shape: the interval set (the finding's "interval set first" — the og's sparse-file mmap stays a Windows-specific optimization for profiling to demand). New telemetry: `CarveStats::occupancy_skipped` / `ScanStats::carve_occupancy_skipped`. The og's cited primitives (EXT4 FUN_180012040 :14892-93 + fill-from-every-extent :6866-6869, Btrfs per-node :2547-2551, Zfs :40576/:40599) are the bitmap-set/test shape; the core's interval set covers the same truth at the walk's extent granularity. The OQ-X1 downstream-consumer question (RAWSearchDLL's own bitmap use) stays tracked — the architectural gap this finding priced is closed regardless.
