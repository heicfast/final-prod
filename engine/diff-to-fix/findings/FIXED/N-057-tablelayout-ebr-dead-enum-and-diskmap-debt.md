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

---

## Addendum (session 8 — S-009 pays the bad-overlay coverage debt)

The S-009 free-overlay fix (this session) landed
`DiskMap.FreeNeverOverridesUsedOrBadAndDownsampleIsConservative`, which
drives the bad-sector overlay path end-to-end (a ReadErrorMap span through
`build_disk_map`: FsBad cell marking + `bad_needed` accounting + the
bad_ranges fill) — the finding's "no test drives a bad-sector overlay"
item is closed. The remaining open scope narrows to: (1) the
`TableLayout::Ebr` dead-enum decision (assign or remove), (2) the
budget-growth path (>64 MiB overlay → cell doubling) and the write-only
`total` accumulator at `cr_diskmap.cpp:39`.

## Resolution

**Status:** FIXED (all three sub-items)
**Fixed in commit:** this session's table-visualization batch (DFX12)
**Verified by:** `Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr` + the re-pinned `Mbr.PlainWithExtended` (fail-before executed on the stash-isolated pre-fix parse: both RED — the enum value had zero assignment sites; green post-fix) + `DiskMap.OverlayBudgetGrowsCellSize` (the at-budget boundary keeps the 512-byte cell; one sector past it doubles to 1024 and the overlay vectors cap at the budget — the previously-untested `choose_cell_bytes` growth path). The write-only `total` accumulator is deleted (build + suite green — no reader existed). Suite 600/600 linux-release.
**Resolution notes:** fix-plan step 1 executed as the recommended SET (not delete): the MBR parse assigns `TableLayout::Ebr` when an extended chain is walked (after `parse_ebr_chain` — the chain's presence is the classification; `ebr_status` keeps carrying the integrity separately, so torn chains still classify Ebr with honest status). Plain 4-primary MBR disks keep `TableLayout::Mbr` — pinned. The ABI surfaces the classification as the APPENDED code 3 (cr_ui's layout mapping 0/1/2 frozen; cr_ui.h comment updated in the same change; the og models EBR/extended as a first-class partition classification — MOM Partition.md), and the CLI prints "EBR". The serialized-value stability question that ruled out deletion is documented in the finding text: the enum's `u32` values are ABI-adjacent (cr_ui maps them); deletion would have renumbered Gpt/GptDamaged. Step 2 executed: the budget-growth test covers the 64 MiB `kOverlayBudgetBytes` boundary exactly (the loop's `>` semantics pinned — at-budget does NOT grow), and `Overlays::total` is removed (set once, never read — grep-verified). Step 3 (diskmap fuzz harness) folds into S-009's outstanding fuzz ask — unchanged. The addendum's open question (per-cell vs per-sector bad-sector overlay granularity in the og) stays corpus-undecoded and unblocking, as the finding itself records.
