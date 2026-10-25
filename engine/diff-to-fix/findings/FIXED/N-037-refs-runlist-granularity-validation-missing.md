# MINOR N-037 — ReFS runlist acceptance: og validates extent-granularity vs volume cluster size and discards inconsistent runlists; core accepts any parsed runs

**Severity:** MINOR
**Module:** refs
**Parameters affected:** B3 (false-positive guard), A27 (fragment/runlist reconstruction)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — FUN_18000e020 :10084–10286: granularity gate :10201–10204 (runs present, layout-gen ≠ 0, device geometry available), average-run-length computation :10213–10224, power-of-two band mapping :10225–10277, **discard on mismatch with the volume cluster size (device+0x14)** :10278–10281
**Core references:** `src/core/refs/cr_refs_engine.cpp` @ `34b3441` — emit_file run acceptance :746–757 (bounds-checked only: `lcn < volume_clusters`, `len <= volume_bytes`, no-offset-wrap) and parse_data_runs :293–310 (size sanity only: `clusters != 0 && clusters < 0x10000000`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

After building a file's non-resident runlist the og computes the extents' granularity (the
smallest power-of-two band covering the average run length in sectors) and **throws the whole
runlist away when it disagrees with the volume's cluster size** — a damaged-record
false-positive guard. Our core accepts any run that parses and is in-bounds, so a torn record
that decodes to plausible-looking but cluster-inconsistent runs yields garbage extents (the
file is offered as recoverable with wrong bytes).

## What the reference does

`FUN_18000e020:10201–10281`: once a runlist is built (and only when the volume's layout
generation was resolved and the device reports its geometry), the og computes
`uVar5 = ceil(size/sector) / total_run_clusters` — the average run length in sectors — maps it
to the enclosing power-of-two band (1..0x1000, :10225–10277), and compares the band with the
device's cluster size at device+0x14 (:10278). On mismatch the runlist is freed and the object
proceeds **without** extents (:10279–10280) rather than with garbage.

## What our core does

`parse_data_runs` sanity-checks run sizes only (nonzero, < 0x10000000 clusters, :306);
`emit_file` bounds-checks the resulting offset/length against the volume size (:753). No
granularity/consistency validation exists, so torn runs that pass the bounds check become
recoverable extents.

## Gap

Falsifiable: a synthetic record whose run entries decode to lengths inconsistent with the
volume cluster size (e.g., 3-sector runs on a 16 KiB-cluster volume) yields a metadata-only
result in the og and a full-extents result in ours.

## Impact

False-positive suppression only (B3): on damaged volumes the og refuses to present
cluster-inconsistent runs as recoverable; we may present files whose recovered bytes are
wrong. No effect on healthy volumes — MINOR.

## Reproduction / verification

Decompiled citation (og block quoted by line range) + core citation; deterministic from both.
Executable check: extend a `test_refs_volume.cpp` fixture with cluster-inconsistent runs →
extents present today (documents the divergence).

## Fix plan (for the engineering team)

1. In `emit_file` (after run collection): compute the average run length in clusters; verify
   every run's length and the file's extent boundaries are cluster-multiples consistent with
   `boot.cluster_bytes` (the og's band test is equivalent on real data: runs are
   cluster-granular); on failure, drop the extents and let the node fall to metadata-only
   grading (existing `Confidence::MetadataOnly`).
2. Count the discarded cases in `RefsScanStats` (new counter, e.g. `rejected_runlists`) for
   the damaged-volume bench.
3. Cross-link the implementation with S-020 fix step 4 (same code site — implement once).

## Tests to add

- `Refs_Inconsistent_Run_Granularity_Downgraded` (cluster-inconsistent runs → metadata-only,
  `rejected_runlists` incremented).
- `Refs_Consistent_Runs_Unaffected` (healthy fixture: extents unchanged — regression guard).

## Related findings

- S-020 (fix step 4 implements this at the same site — kept as its own finding for the
  standalone B3 row).
- N-036 (the recognizer feeding this validation).

## Open questions

None.

## Resolution

**Status:** FIXED (folded into the S-020 implementation, as the finding itself anticipated)
**Fixed in commit:** (this commit — the S-020 fix)
**Verified by:** Refs.RunlistGranularityMismatchDiscarded (fail-before FAILED: 4 garbage runs accepted; pass-after: discarded, metadata-only, granularity_discards 1).
**Resolution notes:** attr_runs_granularity_ok implements the FUN_18000e020 gate: pow2-floor(ceil(allocated_size/sector_bytes) / total_run_clusters) must equal sectors-per-cluster; mismatch clears the whole extent list. One finding premise corrected on the :10213 re-read: the og divides the props' FIRST FIELD (+0) — the allocated size, not the logical size (the alloc-vs-run-count invariant; with logical size the gate would discard every healthy small file). Suite 402/402.
