# session-diff-fix-prompt.md — the mandate for this fix session (token redacted)

Repositories (only two, in scope):
- Core (modified): heicfast/core_recovery
- Research corpus (ground truth, read-only): heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git-config only, never committed, never logged)

Mandate: execute the roadmap in core_recovery/diff-to-fix/. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify
the finding against the corpus and the running code, fix it in the codebase,
prove the fix with tests and benchmarks, and mark it fixed in the audit trail
with evidence. A false finding is corrected in the audit trail rather than
"fixed" by breaking good code.

Per-finding loop: Reproduce -> Re-verify vs corpus -> Classify
(CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE /
NEW-FINDING) -> Fix -> Test (fail-before/pass-after) -> Benchmark when
performance-tagged -> Document -> Push.

SOPs: re-implement from understanding (never paste decompiled code, never copy
proprietary strings, cite corpus artifact in commit message); fix the cause not
the symptom; match architecture and wiring; one finding = one verified fix =
one push; pull frequently, never force-push, keep main green; safety hardenings
preserved; ReFS/LDM first-class; no stubs/mocks/fallbacks that mask failure; no
hallucinated parity — every FIXED has a failing-then-passing test; remove
resolved TODO/FIXME notes; update PARITY-DELTAS for deliberate divergences;
CI green Linux (ASan/UBSan/TSan on touched code) and Windows-equivalent gates;
Win7-inclusive import gate; signing disabled.

Resolution template per finding file (append, never delete):
## Resolution
**Status:** FIXED | FIXED-ALREADY | INVALID | SUPERSEDED
**Fixed in commit:** <sha>
**Verified by:** <tests + benchmarks / command evidence>
**Resolution notes:** <paragraph with corpus + code citations>

Target: close every open finding; fixed findings move state into findings/FIXED/
(updated docs/counts). Work long-horizon, commit and push often, worklog current.
