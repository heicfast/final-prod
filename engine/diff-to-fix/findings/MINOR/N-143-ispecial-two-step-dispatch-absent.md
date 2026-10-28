# MINOR N-143 — the og's two-step analyser validation (ISpecialRawFileAnalyser full-validate first, then the plain probe, with the id-8 generic-stamp + chain-continue on plain-accept-after-special-reject) has no core counterpart: core runs one validator per candidate and rejects outright

**Severity:** MINOR (a dispatch-tier semantic difference in the carve engine: candidates the og demotes to generic (id 8) and keeps scanning, core rejects entirely — affects the mixed-content edge where a special analyser's strict tier refuses what the plain tier admits)
**Module:** carve (the analyser dispatch tiers)
**Parameters affected:** A25 (engine algorithm steps — the probe→validate sequence), A28-adjacent (the carve validator's accept/reject criteria), D15-adjacent (the generic-format fallback class)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:9877-9883` — the dispatch's two-step: `vtable+0x40` returns the ISpecialRawFileAnalyser sub-object, and if present its full validate (with sector size + context) runs FIRST; `:10020-10039` — the special-reject/plain-accept continuation: the candidate is stamped **format id 8 (generic)** and the chain CONTINUES (`:10026-10039`); `:2235-2238` — FileZIPAnalyser carries both vtables (the multiple-inheritance shape that makes the two-step reachable)
**Core references:** audited `461825d8`-lineage — `src/core/carve/cr_carve.cpp:349-427` (the trial loop: one `validate_*` call per matching row, first-accept-wins with the N-070 accumulate-highest-score emission), `:367` (the single validator invocation); no special/plain tier split, no generic-stamp continuation (the id-8 semantics itself is N-031-addendum-known — the "generic" format exists in the og's list layer)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11-lead Lane A infrastructure walk (verdict-table row 5); NOT covered by D-75 (the registered per-key chain/fallback divergence — this is the sibling facet: the special-tier two-step at dispatch, not the registry chains), N-031 (the only_common/id-8 notes — which knew the id-8 stamp but not the two-step's control flow), N-070 (the emission scoring — orthogonal), or I-031 (the parallel infra register).

## Summary

The og's candidate dispatch runs a two-tier validation: a candidate analyser that implements the ISpecial interface gets its full context-aware validate first, and if that rejects but the plain probe accepts, the candidate is not lost — it is stamped as the generic format (id 8) and the per-position chain keeps scanning for a better analyser. Core's dispatch is single-tier: one validator, reject = the candidate is dropped for that position. On mixed-content edges (e.g. a ZIP whose special tier finds the archive corrupt but whose plain signature matches), the og emits a generic-format candidate and core emits nothing.

## What the reference does

The dispatch loop (cites above) per candidate analyser: fetch the special
sub-object; if non-null, run its full validate with sector size and scan context;
if the special rejects, fall to the plain probe (`vtable+0x10`); a plain-accept
after a special-reject stamps id 8 (generic) and continues the chain so a later
analyser can still claim the position with a real id.

## What our core does

One validator call per candidate row; rejection drops the position's candidacy
for that row (other rows' validators still run — the trial loop — but there is no
demote-to-generic continuation and no special/plain tier distinction inside a
row's validation).

## Gap

Falsifiable: a fixture whose signature matches a special-bearing analyser (e.g.
ZIP) but whose content fails that analyser's strict tier and passes a plain
admission: the og emits a format-8 generic candidate for the position; core emits
none (unless another row's validator independently accepts).

## Impact

A carve-completeness edge: the og's generic-stamp path salvages signature-true
but grammar-failing hits as generic candidates; core loses them. The class is
narrow (needs a special-bearing format whose strict tier fails while the plain
tier passes — corrupt ZIP/RIFF-family archives are the practical case), and the
id-8 generic format is itself an og concept core deliberately lacks (N-031's
registered posture). Bounded user-visible effect: fewer low-quality candidates.

## Reproduction / verification

Decompiled evidence (Lane A walk, verdict-table row 5 + Section 5 item 4; the
two-step cites re-read by the lead at filing). Core-side code-read. No execution
(a generic-format emission comparison needs the id-8 surface core lacks). Full
record: `evidence/s11-lead-deep-lanes.md` §Lane-A rows 5 + Section 5.

## Fix plan (for the engineering team)

1. Fold into the D-75 register first (the registered per-key chain/fallback
   divergence) as its dispatch-tier sibling — the decision whether the generic
   (id 8) fallback class should exist in core at all is a posture call, not a
   mechanical fix: the og uses it to salvage corrupt archives; core's
   honest-refusal doctrine may legitimately decline.
2. If matched: add a `demote_generic` continuation to the trial loop — when a
   row's validator rejects but the row's signature matched strongly (the plain
   tier), emit a generic-annotated candidate only if no other row accepts the
   position (the og's chain-continue order).
3. If declined: register the D-row with the doctrine rationale and the count of
   affected fixture classes (corrupt ZIP/RIFF).
4. Tests either way: the corrupt-ZIP fixture class pinned (emitted-as-generic or
   refused-by-name, per the decision).
5. Acceptance: the posture is registered; the fixture class behaves as decided.

## Tests to add

- `Carve.SpecialRejectPlainAcceptGenericContinuation` (if matched) or the
  named-refusal pin (if declined).

## Related findings

D-75 (the registered chain/fallback divergence this extends); N-031 (the id-8
generic format's registered knowledge); N-070 (the emission model); N-142 (the
same walk's filter-exempt find — the same rebuild/dispatch neighborhood).

## Open questions

Whether any core format row pairs a strict-tier validator with a looser
signature admission today (the two-step's precondition) — the trial loop's rows
each have one validator, so the special/plain pairing would have to be
constructed inside a row's validator when ported; the og's dual-vtable ZIP is
the only decompile-confirmed pairing.
