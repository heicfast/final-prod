# MINOR N-057 — Dead code + test debt bundle: TableLayout::Ebr never assigned; diskmap bad-sector overlay and budget-growth paths untested with a write-only accumulator

**Severity:** MINOR
**Module:** table + visualization (diskmap)
**Parameters affected:** Q65 (test coverage — diskmap overlay/budget paths), A54/A56 (module boundaries — dead enum value is interface drift), Q69 (stub/dead-code scan)
**Corpus references:** `re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md` + `09-results-phase-deep-dive.md` (the og's disk map renders found/deleted/bad-sector overlay states — the bad-sector overlay is a user-visible surface the core's diskmap models)
**Core references:** `include/cr/table/cr_table_parse.h:42` (`enum class TableLayout : u32 { None = 0, Mbr, Ebr, Gpt, GptDamaged };` — `Ebr` has no assignment site in src/include/tests: grep verified empty), `src/core/tree/cr_diskmap.cpp:129-157` (bad-sector overlay + budget growth paths — no test reaches them), `:39` (`u64 total = 0;` accumulator in the overlay budget block — write-only in the current flow) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two small hygiene items surfaced by the never-cited-file sweep: (1) `TableLayout::Ebr` is
defined but never assigned by any parser — either dead or a missing classification (EBR
chains are detected via the MBR path's extended handling without setting the layout); (2)
the diskmap's bad-sector overlay and budget-growth paths (the S-009 family's visualization
side) have no test coverage, and the overlay budget block keeps a `total` accumulator whose
value is never read.

## What the reference does

The og's scan/results phases render the disk map with overlay states including bad-sector
marking (UIUX 08/09); EBR/extended layout is a first-class classification in the og's
partition model (MOM Partition.md).

## What our core does

`TableLayout::Ebr` defined at `cr_table_parse.h:42`, never assigned (the extended-chain
walk lives in the MBR parse without tagging the layout). `cr_diskmap.cpp`'s overlay block
(`:129-157`) computes and clamps against `kOverlayBudgetBytes` (64 MiB, `:20`) and
maintains `total` (`:39`) without a reader; tests/unit/test_snapshot-style coverage exists
for the base diskmap but not the overlay/budget paths.

## Gap

Falsifiable: no code path produces `TableLayout::Ebr`; no test drives a bad-sector overlay
or a >64 MiB overlay budget through the diskmap.

## Impact

Hygiene/debt only — no behavior divergence proven. The dead enum risks silent misuse (a
future consumer switching on it gets Nothing); the untested overlay paths guard the
user-visible bad-sector rendering. MINOR.

## Reproduction / verification

Greps verified first-hand this session (assignment-site search across src/include/tests;
diskmap test-file enumeration).

## Fix plan

1. Decide the enum: either set `TableLayout::Ebr` in the MBR extended-chain parse (matching
   the og's classification) or delete the value. Recommend SETTING it — the layout is real
   information the lost-partition/report layers could consume.
2. Add diskmap tests: a synthetic slice with bad-sector overlays; a fixture crossing the
   64 MiB budget (cell-size growth asserted); remove or read the `total` accumulator.
3. Fold into the S-009 fuzz ask (diskmap fuzz harness) when that lands.

## Tests to add

- `DiskMap.BadSectorOverlayRendered`
- `DiskMap.OverlayBudgetGrowsCellSize`
- `Table.EbrLayoutAssigned` (after step 1)

## Related findings

- S-009-partial (bad-sector policy family)
- N-041 (dead-code family precedent)

## Open questions

Whether the og's disk-map bad-sector overlay granularity (per-cell vs per-sector) matches
ours is UNVERIFIED — the UIUX artifacts describe the visual, not the cell math; a UI-layer
decode would pin it. Not blocking the test-debt fix.
