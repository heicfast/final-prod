# Session 40 — diff-to-fix execution prompt (saved per SOP; token redacted)

## Scope (two repos, only two)
- Core (modified): heicfast/core_recovery
- Research corpus (ground truth, read-only): heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed)

## Mandate
Execute the roadmap in core_recovery/diff-to-fix/. For every finding — ACUTE,
SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the
finding against the corpus and the running code, fix it, prove the fix with
tests and benchmarks, and mark it fixed in the audit trail with evidence.
Every finding is re-verified before any change. A false finding is corrected in
the audit trail rather than "fixed" by breaking good code.

## SOP per finding
Verify -> Reproduce -> Classify (CONFIRMED / ALREADY-FIXED / INVALID /
SUPERSEDED / NEW) -> Fix -> Test (fails before, passes after) -> Benchmark
(perf-tagged only) -> Document (Resolution in finding, matrix, module, SUMMARY,
worklog) -> Push. One finding, one verified fix, one push.

## Key session rules
- Re-implement from understanding; never paste decompiled code or proprietary strings.
- Fix the cause, not the symptom. Match architecture, not only outputs.
- No stubs/mocks/TODOs/fallbacks. No hallucinated parity — tests prove fixes.
- Safety hardenings preserved (overlap refusal, BitLocker legitimate-unlock-only,
  RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- ReFS/LDM first-class. Pull frequently; never force-push; main stays green.
- Windows direct I/O: reads multiple of 4KB/64KB where possible.
- CI gates: MSVC /W4 /WX, sanitizers, fuzz smoke, Win7-inclusive import allowlist.
- Token hygiene: never commit, never log.
- Deliverables per the master prompt §11 (Resolutions, matrix MATCH cells,
  SUMMARY counts, modules verdicts, worklog handoff, release tag if clean).

## Session goal
20+ fixes, SEVERE first: S-051, S-049, S-048, S-052, S-050, S-040, then the
MAJOR/MINOR board by user impact, INFO acknowledgments, tracked-open resolution
or truthful forward push (driver-repo findings are out of this session's
two-repo scope).
