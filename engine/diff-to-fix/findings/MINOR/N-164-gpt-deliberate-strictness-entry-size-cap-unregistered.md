# MINOR N-164 — the core's deliberate GPT strictness (entry_size==128 exact + 1 MiB entries cap) is documented in code but absent from the PARITY-DELTAS registry

**Severity:** MINOR
**Module:** table (GPT) / registry discipline
**Parameters affected:** registry truthfulness; on-disk struct validation (20)
**Corpus references:** og allocates `entry_count × 0x80` with 64-bit saturation (`ghidra-mo/Partition_decompiled.c:4530-4541`), reads `entry_size × entry_count` bytes as a 32-bit product (:4541), and its consumers stride a hardcoded 0x80 (:4703) — a non-128 entry_size mis-walks the array in the og; the og has no entry-count cap
**Core references:** `src/core/table/cr_table_parse.cpp:38` (`entry_size != sizeof(GptEntry) → ERR_BAD_PARAMETER`), `:15/:40` (the 1 MiB entries-read cap ≈ 8192 entries), the in-code comment :76-81 documenting "Stricter than the og…"; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core refuses GPT headers whose entry_size ≠ 128 and caps the entry-array read at
1 MiB; the og hardcodes the 0x80 stride (mis-walking non-128 arrays) and has no cap.
This is deliberate core strictness, documented in the code comment — but the
PARITY-DELTAS registry (whose own rule is "a delta that is not in this registry is a
bug, not a delta") has no row for it. Also unregistered in the same block: the core's
salvage-only entries_start_lba bounds check (:100) which the og lacks.

## What the reference does

The og's alloc/read/stride behavior (cites above) — a corrupt non-128 entry_size
produces an og mis-walk rather than a refusal.

## What our core does

Explicit refusal + cap (cites above), with the honest in-code comment.

## Gap

Registry-only: the deliberate strictness is not registered.

## Impact

Drift risk only: a future session may read the og's acceptance of non-128 arrays as a
parity gap and weaken the core's guard. No behavioral issue.

## Reproduction / verification

Core-derived (lead auditor read :38/:40/:76-81). Corpus-derived (lane A: the alloc/read/
stride cites). Registry grep: no GPT-strictness row.

## Fix plan

1. Register one PARITY-DELTAS row: "GPT entry_size==128 refusal + 1 MiB entries cap +
   salvage entries_lba bounds check — deliberate strictness over the og's hardcoded
   0x80 stride / unbounded alloc" with both sides' cites. Doc edit for the fix lane.
2. No code change.
3. Acceptance: the row exists; the code comment cites it.

## Tests to add

- None (the existing GPT tests already pin the strictness; optionally
  `Gpt.Non128EntrySizeRefusedIsRegistered` as a doc-truth gate check).

## Related findings

- M-123 (the core's MISSING strictness — the mirror image; both rows belong in the same
  registry block)
- M-132 / N-161 (the same file)

## Open questions

None.
