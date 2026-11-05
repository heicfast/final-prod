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

## Resolution

**Status:** FIXED
**Fixed in commit:** (the S57 lane's N-191 landing; the ledger row carries the SHA)
**Verified by:** tools/doc_truth_gate.py check 5 (the new duplicate-D invariant): the registry now carries 135 UNIQUE D-rows, zero duplicates; the acceptance grep (`rg "^### D-118|^\*\*D-118" docs/PARITY-DELTAS.md`) returns exactly one row (N-143's, :3651). The gate runs in CI (checks.yml's local-mirror lane) and locally (ci-local.sh step 2's doc-truth family).
**Resolution notes:** Executed per the fix plan with one discovered extension (filed as N-207): M-125's later row renumbered to **D-127** (its `**D-118` heading normalized to the `### D-127 —` form), N-143's row keeps D-118 per earlier-push-wins; the live citations updated (M-125's finding file, modules/orchestration.md, the FIXED-LEDGER M-125 row) with renumber notes at every site. During verification the new gate invariant surfaced NINE more born-collided seats the audit had not mapped: D-105 x2 (the S-044 rider keeps; the embedded-fast-path row -> D-133), D-106 x2 (the N-105 deferral keeps; the missing-member row -> D-138), D-107 x3 (the M-082 rider keeps; the S-047 TS null-packet row -> D-132; the overlap-mapping row -> D-139), D-135 x3 (N-163 keeps; N-177 -> D-128; N-090 -> D-129), D-136 x2 (S-060 keeps; N-165 -> D-130), D-137 x2 (S-050 keeps; N-169 -> D-131) — all renumbered with their live citations (modules/{ext4,session,fat,carve,ldm}.md, ABI-CONTRACT.md, the five finding files, the FIXED-LEDGER rows). Two registry-truth repairs beyond the renumber: storage.md's N-045 cache-budget cite carried a stale D-75 (the row has been D-72 since the D-72-collision errata - re-truthed), and the gate's first form false-positived the DELIBERATE addendum continuation headings (the D-74 supersession form) - the invariant excludes ADDENDUM headings as row continuations, not seat claims. Historical worklog/SUMMARY prose left as history (the live registry + live docs are the cross-reference surface).
