# SEVERE S-004 — ext4 journal salvage parses only descriptor-referenced blocks; the og parses every block

**Severity:** SEVERE
**Module:** ext4
**Parameters affected:** B2 (recovery correctness — salvage yield), B6 (damaged-volume behavior), A25 (algorithm steps), B3 (both directions)
**Corpus references:** `re-workspace/artifacts/CORRECTIONS.md` row 6; decompiled `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` :12195–12310 (agent-B verified: jsb gate `bswap(u32@0)==0xC03B3998`, blocktype ∈ {3,4} at :12212, then **every block of the journal range feeds the directory-block parser**, 1 MiB windows, cancel per block, "GotJournalDir/TraverseJournalDir" traces)
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:639–830` (journal salvage; pass 1 collects descriptor tags → `Txn::data_blocks`, pass 2 parses **only** those blocks), `:666–681` (the D-07 re-pin comment that claims og-parity while describing the narrower surface) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's journal salvage is deliberately dumb: after gating on the journal superblock
magic + blocktype {3,4}, it feeds **every block** of the declared journal range to the
directory-block parser — including blocks no descriptor references (partial old
transactions in the circular buffer, mid-write tails). Our core parses only
**descriptor-referenced** data blocks. Journal remnants that carry directory entries but no
surviving descriptor surface in the og's results and not in ours. In the opposite
direction, our core does not gate salvage on the jsb at block 0, so it salvages journals
the og's coarse gate refuses — the two salvage sets differ in **both** directions.

## What the reference does

Decompiled journal loop (agent-B verified, B-ext4.md §3.3):

```
read jsb (first journal block, BE-decoded):
  bswap(u32@0) == 0xC03B3998                              [:12207]
  bswap(u32@4) == 3 or == 4     ← JBD2 SUPERBLOCK_V1/V2   [:12212-12215]
  blocksize = bswap(u32@jsb+0xC);  start = bswap(u32@jsb+0x14)
  maxlen = (end + 0xFFFFF) >> 0x14 (÷1 MiB)               [:12219-12224]
  reads 0x100000 (1 MiB) windows; per window: iterations = 0x100000 / blocksize
  every block in range feeds the directory-block parser
  cancel check vt[0x10] per block
```

CORRECTIONS.md row 6: "no commit/revoke transaction semantics exist engine-side" — the
every-block feed is the documented, corrected reading (supersedes the deep-dive's
transaction-flavored implication).

## What our core does

`cr_ext4_engine.cpp:748–782` (pass 1): walks blocks, recognizes descriptor headers, decodes
tags into `Txn::data_blocks` — **only descriptor-referenced blocks** are collected.
`:787–790` (pass 2 comment): "salvage directory entries from EVERY descriptor's data blocks
(committed or not - og parity, E-2)". Non-referenced blocks (no descriptor header, or
descriptor overwritten in the ring) are never parsed. Additionally `:726–745` derives tag
width from the jsb but does **not** gate salvage on jsb presence/type {3,4} — a journal
whose first block is not a jsb is still salvaged by pass 1 (og refuses).

## Gap

Two falsifiable claims:
1. A journal block containing valid directory entries but referenced by no surviving
   descriptor (overwritten ring) yields entries in the og's result set and not in ours.
2. A journal whose first block is a descriptor (not a jsb) is salvaged by our core and
   refused by the og (gate at :12212 fails).

## Impact

- Damaged ext4 volumes (the salvage path's trigger is `root_lost`) recover fewer names in
  our core than in the og whenever the circular journal holds unreferenced dir-content
  remnants — common on journals that wrapped.
- Claim 2 makes us a superset in a rarer case; the net effect is asymmetric salvage sets,
  not a strict subset. Bench inventories on damaged-ext4 fixtures diverge mechanically.

## Reproduction / verification

- Corpus-derived: decompile quote with lines; CORRECTIONS row 6.
- Core-derived: pass-1/pass-2 code quotes (descriptor-only collection; no jsb gate).
- Executed: not run against og (no Windows); the delta is fully determined by the quoted
  code paths. Unit-level repro possible: journal image with a dir-content block not
  referenced by any descriptor → assert 0 salvaged entries (current) vs > 0 (og-expect).

## Fix plan

1. Replace pass 1+2 with an og-parity "parse every block" loop over the journal extent
   data: for each block (in 1 MiB windows for cache friendliness), attempt
   `parse_dir_block`; collect entries with the existing dedup
   (`seen` set + `visited_inodes` filter). Keep the escape-restoration logic
   (JBD2_FLAG_ESCAPE) — the og's dir-parser implicitly handles escaped blocks by failing
   (first 4 bytes zeroed); restoring first is strictly better and does not change the
   accept set for dir blocks (a dir block never starts with the journal magic).
2. Keep the tag machinery only if needed for telemetry (descriptor counts); it is no longer
   the salvage driver.
3. Decide the jsb gate: for strict parity, require jsb magic + blocktype {3,4} at the first
   block before salvaging (og behavior); recommend keeping our no-gate behavior as a
   registered superset OR matching og exactly — register the choice in PARITY-DELTAS.
4. Update the D-07 comment (currently self-contradictory: claims "we match that salvage
   surface" while implementing descriptor-referenced-only).
5. Files: `src/core/ext4/cr_ext4_engine.cpp` only.
6. Tests (below).

## Tests to add

- `test_ext4_volume.cpp::Journal_Unreferenced_Dir_Block_Salvaged`: journal image where
  block 5 contains a valid dir block but no descriptor references it → entry emitted
  (fails before, passes after).
- `test_ext4_volume.cpp::Journal_First_Block_Not_Jsb_No_Salvage` (if strict parity chosen):
  journal starting with a descriptor block → 0 salvaged entries.
- `test_ext4_volume.cpp::Journal_Escaped_Block_Restored_And_Parsed`: escaped dir block
  (first 4 bytes zeroed) → entry emitted.
- Bench: damaged-ext4 fixture (root_lost) re-run — expect our salvage inventory to grow
  toward the og's.

## Related findings

- N-004 (CatchBuffer bad-sector layering — same module, different mechanism).

## Open questions

None.
