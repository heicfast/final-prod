# MINOR N-161 — GPT entry span underflow: `sector_count = last_lba − first_lba + 1` with no first≤last gate — a corrupt entry yields a wrapped count that passes the `!= 0` check

**Severity:** MINOR
**Module:** table (GPT entry emission)
**Parameters affected:** on-disk struct validation (20), detection correctness (1) edge
**Corpus references:** og span consumption undecoded (OQ-1 in the lane sheet: the CPartitionEntryGPT consumer chain, `FUN_1800085b0` P:5273-5286, `FUN_180008ff0` P:5791+) — the og side is OPEN, so this is filed as a core-side robustness gap, not a parity claim
**Core references:** `src/core/table/cr_table_parse.cpp:273-274` (the span math), `:297` (the `sector_count != 0` gate it slips past), downstream `cr_scan.cpp:1680-1683` (slice create / table_volumes_skipped); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`emit_gpt_entries` computes `sector_count = last_lba − first_lba + 1` with no
`first_lba <= last_lba` check; a corrupt entry (first=5, last=0) underflows to
0xFFFFFFFFFFFFFFFC, which passes the `sector_count != 0` gate. Downstream, the wrapped
byte length either zeroes (rejected in slice create) or produces a slice whose reads all
fail → the volume is skipped with no diagnostic. No crash, no wrong bytes — but the
corrupt entry masquerades as a giant volume instead of being named invalid.

## What the reference does

Not decoded this session (OQ-1): if the og computes the same expression without a gate,
this is a shared defect (MATCH-class); if it validates, divergent. Filed for the
core-side guard regardless — the core's own honesty doctrine wants the gate.

## What our core does

cr_table_parse.cpp:273-274 — the unchecked subtraction; :297 — the `!= 0` gate only.

## Gap

No `first_lba <= last_lba` validation anywhere between parse and slice-create.

## Impact

Corrupt-entry diagnostics and skip accounting (a wrapped count lands in
table_volumes_skipped instead of an explicit invalid-entry counter); no user-visible
wrong data. Theoretical only for hostile images.

## Reproduction / verification

Core-derived (lead auditor read the span math + the gate). Arithmetic: 0 − 5 + 1 =
0xFFFFFFFFFFFFFFFC (u64). Lane A sheet row 1.6.

## Fix plan

1. Add `if (e.last_lba < e.first_lba) { skipped_invalid_entries++; continue; }` before
   emission; surface the counter in ScanStats (one field).
2. Files: `src/core/table/cr_table_parse.cpp`, `include/cr/orchestrator/cr_scan.h`.
3. Tests: `Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted` (fails today: no counter,
   wrapped count flows).
4. Acceptance: the corrupt-entry fixture increments the counter; no slice attempted.

## Tests to add

- `Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted`
- `Gpt.EntrySpanOverflowBoundary` (first=0, last=UINT64_MAX)

## Related findings

- M-123 (the same file's header gates)
- M-132 (the ladder the skipped entry flows through)

## Open questions

The og's own span consumption (OQ-1) — the artifacts to decode are listed in the lane
sheet; resolving it upgrades this to MATCH or a parity divergence.
