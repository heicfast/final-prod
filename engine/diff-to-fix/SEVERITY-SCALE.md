# SEVERITY-SCALE.md — verdict definitions used by this audit

Every module × parameter cell receives exactly one verdict. Only `MATCH` and
`DIVERGENCE-INFORMATIONAL` are acceptable end states; every other verdict
produces a finding file under `findings/` (or `tracked-open/` for `UNVERIFIED`
that cannot be resolved in-session).

## MATCH

Our implementation matches the reference on this parameter, with evidence
(corpus citation, decompiled quote, or executed comparison).

## DIVERGENCE-ACUTE

Wrong behavior that produces **incorrect recovery results, silent data loss,
crashes, or security regressions**. Must be fixed before release.

Examples (from this codebase's real risk surface): a wrong GPT type-GUID
constant that classifies live system partitions as `Other`; a deleted-chain
heuristic that emits plausible-size wrong-byte files; a session-writer defect
that corrupts resume state; an unvalidated carve accept that floods results
with false positives.

## DIVERGENCE-SEVERE

Significant behavioral difference: missing feature the reference has, wrong
error semantics, or a state-machine mismatch **users will observe**.

Examples: an engine the reference ships (deleted-file classification on a
filesystem where the reference classifies) is absent; resume loses a stage;
progress jumps non-monotonically where the reference is monotonic; a filter
category the reference exposes is missing.

## DIVERGENCE-MAJOR

Performance gap, missing edge-case coverage, incomplete state handling, or
architectural mismatch affecting maintainability or correctness **at scale**.

Examples: an O(n) lock convoy the reference does not have; a 4Kn-sector
geometry assumption; a constant used where the reference derives from disk
layout; per-engine divergence in timestamp precision with no documented
rationale.

## DIVERGENCE-MINOR

Cosmetic, naming, doc drift, low-impact inefficiency, or minor inconsistency.

Examples: a name that drifted from the reference vocabulary; a doc that
describes an older behavior; a log line missing; an informational field
formatted differently.

## DIVERGENCE-INFORMATIONAL

A **deliberate** difference (safety hardening, documented improvement, or scope
exclusion) recorded but not a defect. These still get finding files so future
sessions know they were reviewed, not missed.

Examples: refusing BE ZFS pools the reference would attempt; a stricter
bounds check that rejects corrupt structures the reference tolerates; Win7
support the reference dropped in later builds.

## UNVERIFIED

Cannot be verified from the corpus or by execution in this environment. Must
either be resolved in-session (RE probe or executed comparison) or moved to
`tracked-open/` with a precise question and the corpus artifact to examine
next. Nothing may ship as UNVERIFIED inside `findings/`.

## Severity discipline (from the session SOP)

- Acute is reserved for wrong recovery results, silent data loss, crashes, or
  security regressions — never for "ugly but correct".
- Minor is reserved for cosmetics and doc drift — never for behavior.
- Do not inflate or deflate. A false ACUTE costs the engineering team a
  panicked week; a deflated SEVERE ships a wrong-result bug.
- Informational is for *reviewed and accepted* differences only — using it to
  bury a real gap is a audit failure.

## Verification strength (required per finding)

1. Executed comparison (strongest) — logs in `evidence/executions/`.
2. Decompiled evidence — exact quote, file + address, in `evidence/decompiled-quotes/`.
3. Corpus documentation evidence — file + line range + quoted excerpt.
4. Independent RE probe — tool + command + output cited.

A finding with none of these is not written.
