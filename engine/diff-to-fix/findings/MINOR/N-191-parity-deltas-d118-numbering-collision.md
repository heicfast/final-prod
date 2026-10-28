# MINOR N-191 — PARITY-DELTAS carries two rows numbered D-118 (N-143's id-8-demote row and M-125's linear-permille row)

**Severity:** MINOR
**Module:** trail registry (docs/PARITY-DELTAS.md — the deliberate-divergence register's numbering)
**Parameters affected:** Q46/Q68-class (documentation accuracy of the registry), the D-87 renumber convention's own applicability
**Corpus references:** n/a (core-side registry bookkeeping)
**Core references:** audited `34d6cf10` — `docs/PARITY-DELTAS.md:3602` ("### D-118 — no generic id-8 demote on special-reject/plain-accept (N-143 registration...)", landed cb93adf9) and `docs/PARITY-DELTAS.md:3643` ("**D-118 — the engine's linear permille vs the og's display remap (progress semantics, M-125):**", landed f368f81c's trail wave) — both read first-hand this session; the M-125 row's divergent `**D-118` formatting (vs `### D-118`) is why exact-heading greps see only one
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The N-143 registry landing and the M-125 registry landing both numbered
their new PARITY-DELTAS row "D-118" — the N-143 row pushed first
(cb93adf9), the M-125 row rode the later f368f81c wave without noticing
the seat was taken. The register now has two D-118s, which breaks
D-number-based cross-references (the trail's findings cite D-numbers as
stable identifiers — e.g. D-117/D-119/D-120 cites land adjacent to both).

## What the reference does

n/a — core-side registry bookkeeping; the registry's own convention
(the D-87 renumber precedent, and the SA-9 earlier-push-wins rule applied
to D-seats) says the earlier row keeps the number and the later row
renumbers to the next free D-seat.

## What our core does

Both rows stand at HEAD: `:3602` (N-143, the id-8 demote decline) and
`:3643` (M-125, the linear-permille display divergence). The next free
D-number at HEAD is D-124 (D-121/D-122 landed with the I-043/I-042
retirements, D-123 with the I-040 register note per cb2abd42, D-135/D-136
with N-177/N-165 per the hot window — see the ledger for the D-map; the
renumber should take the next free seat at fix time, verified then).

## Gap

Falsifiable: `rg "^### D-118|^\*\*D-118" docs/PARITY-DELTAS.md` returns
two rows.

## Impact

Registry cross-reference integrity only — any future finding citing
"D-118" is ambiguous between the two rows. No code, no behavior.

## Reproduction / verification

Executed class: the two-row grep quoted in the Core references, run
first-hand at `34d6cf10`; also surfaced by the landing sweep
(`evidence/executions/s50-landing-parity-sweep.md` row 13's registry note).

## Fix plan (for the engineering team)

1. Architectural: none.
2. Files: `docs/PARITY-DELTAS.md` + the findings/docs that cite the
   renumbered row (the M-125 trail citations).
3. Algorithm: renumber the M-125 row (the later landing) to the next free
   D-seat at fix time (expected D-137+, verify against the ledger first);
   keep N-143's row at D-118 (earlier-push-wins); normalize its heading to
   the `### D-<n> —` form; update the two or three trail citations of the
   M-125 row (grep "D-118" across diff-to-fix/ and docs/).
4. Tests: a one-line docs gate — `tools/doc_truth_gate.py` (or the verify
   script that suits) gains a duplicate-D-number invariant: every
   `D-\d+` heading/row unique.
5. Contract/doc: the renumber note in the registry's header.
6. Acceptance: the grep returns one D-118 (N-143's); the new row number is
   unique; the duplicate-number invariant green.

## Tests to add

The duplicate-D-number registry invariant (script-level).

## Related findings

N-143, M-125 (both FIXED — whose registry rows collide); N-129 (the
SA-9/renumber-convention lineage).

## Open questions

None.
