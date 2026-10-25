# ACUTE A-005 — HFS+ B-tree record-span convention inverted vs the on-disk format: on real volumes only record 0 of each node is emitted

**Severity:** ACUTE (silent multi-record loss on every real HFS+ volume — wrong recovery results at scale)
**Module:** hfsplus
**Parameters affected:** B1 (detection/result-set membership), B2 (recovery correctness), A25 (engine algorithms)
**Corpus references:** `HFSPlusSearchDLL_decompiled.c` `FUN_180010720` **:13049-13108** (the journal node-image validator — the only span-constrained decode in the corpus), `FUN_18000fbe0` **:12546-12560** (record N's key at slot `node_size - 2*(N+1)`), `FUN_18000fe90` **:12657+** (record data at the same slot's value); corpus artifact `re-workspace/artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md` :78-82 (the session-1 "records grow from the END backward" note — explicitly marked "[CODE; INFERRED]", i.e. inferred FROM THE CORE's code, not from the reference)
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp` `for_each_record` (record i = [slot(i+1), slot(i)) — END taken from the SHALLOWER slot = records-DOWN), `tests/unit/test_hfsplus_volume.cpp` `make_leaf_node` (fixtures packed DOWN from below the offset array) — audited commit lineage `4fe9bff`
**Status:** READ-ONLY FINDING — discovered during S-021's verification (the journal fixture's images failed the og-derived validator).

## Summary

The core's HFS+ B-tree record reader and every synthetic fixture use a
records-packed-DOWN convention: records sit at the TOP of the node's free
space just below the offset array, record 0 at the HIGHEST offset, and the
reader computes record i's span as [slot(i+1)'s value, slot(i)'s value)
(with node_size as record 0's end). The on-disk format (TN1150) and the
og's own executable validation pack records UP from offset 14: record 0
ALWAYS starts at 14, deeper offset-array slots hold HIGHER values, and the
deepest slot (node_size − 2·(numRecords+1)) is the free-space start. The
og's journal node-image validator (`FUN_180010720`) is the corpus's only
span-constrained decode and it hard-gates exactly the TN1150 shape:

```
:13066  if ((param_2 < 0x200) || ((param_2 & 0x1ff) != 0)) ...     // node size
:13069-79  kind gate (-1..2; header=>height 0, leaf=>height 1)
:13094-96  u16@(node_size-2) byteswapped == 0x0E                    // record 0's offset == 14
:13100-08  deeper slots (node_size-2k): values must ASCEND          // start_1 > start_0 ...
```

A node whose [end−2] holds a high offset (the records-DOWN shape) fails
the `== 14` sentinel and the og's journal harvest would never process it
— on a shipped product that works on real volumes, that is dispositive:
real nodes are records-UP.

## Impact

On any REAL HFS+ volume (whose leaf nodes typically hold dozens of
records each):

- The core's `for_each_record` computes record 1's span as
  [start_1, start_0) = [higher, 14) — the damage guard
  (`off_prev <= off_this`) silently SKIPS it, and likewise every record
  after 0. Only record 0 of each leaf node is ever emitted.
- The same reader backs the catalog walk, the S-002 attributes walk, the
  freed-node sweep, and the decmpfs resolution — the entire HFS+ surface.
- The synthetic test suite never caught it because `make_leaf_node`
  writes the same inverted shape the reader expects (self-consistent
  fixtures; the corpus artifact's "records grow from the END backward"
  note was inferred from the core's code, not the reference).

This is the largest silent-yield-loss class a recovery product can have:
real volumes return ~1/n of their files (n = records per node), all
silently.

## Fix plan

1. `for_each_record`: record i = [slot(i+1)'s value, slot(i+2)'s value)
   (the deeper slot holds the NEXT record's start; the last record ends at
   the free-space start in slot(numRecords+1)). Keep the damage-skip
   guard with the corrected direction.
2. `make_leaf_node` (and every fixture that writes nodes through it):
   records packed UP from offset 14; offset array [end−2] = 14,
   [end−4] = start_1, ..., deepest slot = free-space start.
3. The journal-harvest node-image validator already implements the
   og's (TN1150) shape — it is what exposed this.
4. Tests: the existing suite must stay green after the fixture flip
   (they share `make_leaf_node`, so the flip is one point); add
   `HfsPlus.LeafNodeLayoutMatchesOnDiskFormat` asserting [end−2] == 14
   and the ascending offset chain on a built fixture node (guards the
   convention itself against re-inversion).

## Tests to add

- `HfsPlus.LeafNodeLayoutMatchesOnDiskFormat` (fixture self-check).
- The S-021 journal-harvest tests (their images must pass the
  og-derived validator, which only accepts the TN1150 shape).

## Related findings

- S-021 (the journal harvest whose verification discovered this) · the
  session-1 artifact note that propagated the inverted inference
  (`ext4-hfs-engine-algorithms-v2.md` :78-82 — errata candidate for
  `CORPUS-ERRATA.md`).

## Open questions

- None on the direction (three independent sources agree: the og's
  executable validator, TN1150, and the impossibility argument on the
  shipped og). The corpus artifact's inference note should be corrected.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — "fix: A-005 + S-021 — HFS+ B-tree record-span convention corrected (TN1150/og-validated) + the journal deleted-record harvest")
**Verified by:** fail-before — with the corrected (on-disk-format) fixtures and the PRE-FIX reader reverted, `HfsPlus.EndToEndWalk`, `HfsPlus.DecmpfsInlineAndRsrcContentRecovered` and `HfsPlus.DecmpfsDisabledLeavesEmptyShell` all FAIL (the records-DOWN reader cannot parse records-UP nodes: the inversion made each side reject the other's shape — exactly what happens on real volumes where only record 0 survives); post-fix all 11 HfsPlus tests + the full suite 401/401 Linux ASan pass. `HfsPlus.LeafNodeLayoutMatchesOnDiskFormat` now guards the convention itself ([end-2] == 14, ascending offset chain, records at 14).
**Resolution notes:** `for_each_record` now computes record i as [slot(i+1)'s value, slot(i+2)'s value) — the next record's start is the DEEPER slot, the last record ends at the free-space start (slot(nrec+1)); `make_leaf_node` (the single node-builder every HFS+ fixture flows through) now packs records UP from offset 14 with [end-2] = record 0's offset (= 14), matching TN1150 and the og's executable gate (`FUN_180010720` :13094-13108: the `== 0x0E` sentinel + ascending-offset chain — decoded first-hand this session while implementing S-021's journal-image validator, whose rejection of the fixture nodes exposed the inversion). The discovery path is itself the proof chain: the og's journal validator only accepts records-UP nodes; the og ships and harvests on real volumes; therefore real nodes are records-UP and the core's reader (which skips records 1+ on that shape) was inverted. The session-1 corpus artifact's "records grow from the END backward" note was an inference FROM the core's code (its own "[CODE; INFERRED]" marker) — corrected in `CORPUS-ERRATA.md` (E29). The S-021 journal-harvest validator implements the same og gate and now passes the same fixtures end-to-end.
