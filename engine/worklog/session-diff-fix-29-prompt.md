# Session 29 prompt (token redacted) — diff-to-fix fix session, DFX29 (re-stamped from 25: the ID was squatted by the parallel INFO-closure lane)

Mandate (verbatim summary; full text as delivered, token redacted):

- Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
  For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
  independently verify, fix, prove with tests/benchmarks, mark fixed with evidence.
  Do not trust the audit blindly; every finding re-verified against the research
  corpus and the running code before any change. A false finding is corrected in
  the audit trail rather than "fixed" by breaking good code.
- Two repositories only: heicfast/core_recovery (modified) and
  heicfast/easeus-drw-research (ground truth, read-only). GitHub token via env
  only, never committed: [REDACTED — env-only credential].
- SOP: Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
  FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review → Test →
  Benchmark → Document → Push → Loop.
- Re-implement from understanding; never paste decompiled code; cite corpus
  artifacts in commits. Fix causes, not symptoms. Every fix paired with a test
  that fails before and passes after. Safety hardenings preserved. No stubs, no
  mocks, no TODOs, no fallbacks that mask failure. One finding, one verified
  fix, one push. Pull frequently, never force-push, leave main green.
- Performance fixes measured, not guessed. Windows direct I/O multiples of
  4 KB / 64 KB. Record numbers in Resolution + SUMMARY.
- Audit-trail updates after every fix: Resolution section in the finding file,
  move to findings/FIXED/, RESOLVED-INDEX row, CROSS-REF-MATRIX cell → MATCH,
  modules/<module>.md verdict, SUMMARY counts, worklog.md entry.
- Session-specific directive from the user: fix findings one by one with full
  context, FOCUS ON MINOR INITIALLY, put fixed findings in the fixed directory,
  update all docs and counts properly, target at least 20+ fixes before
  session end.

## Session-29 lane decision (recorded at bootstrap)

At fetch time origin/main = `02b950f`: DFX23 (bootstrap `354c555`) claimed the
MINOR tier top-down starting at N-009; DFX24 (bootstrap `02b950f`) claimed the
MAJOR tier. Both lanes bootstrap-only at fetch. Per the user's MINOR-first
mandate this session works the SAME MINOR tier but BOTTOM-UP (reverse of
DFX23's queue: N-064 → N-071 → N-032 → N-072 → N-033 → N-050 → N-019 →
N-075 → N-066 → N-065 → N-059 → N-049 → N-036 → N-031 → N-027 → N-026 →
N-025 → N-007/I-004 → N-009) so the two lanes stay disjoint until they meet
in the middle; first-landing-wins per protocol, pull before every push, any
already-landed finding is classified FIXED-ALREADY and skipped. The
unclaimed INFORMATIONAL tier (acknowledgment sweep) and tracked-open
push-forward are claimed for the MINOR-drain continuation.
