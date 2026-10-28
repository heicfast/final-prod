# Session prompt — the parity close (diff-to-fix execution, all tiers)

Saved at session start (T1). Token redacted per mandate.

## Mandate
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE,
MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the finding, fix it in
the codebase, prove the fix with tests and benchmarks, and mark it fixed in the audit
trail with evidence. The audit is a hypothesis; the corpus is ground truth.

## Scope (two repos only)
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git-config only, never committed. [REDACTED]

## Session baseline (T1)
- Audited commit per README (session 9): product code `35adcb9`, driver `323e75a`, corpus `be0cfe9`
- Current main at session start: `84a1832` (trail-only commits after the audited code commit)
- Corpus at `be0cfe9` (unmoved)
- Board at session start: 75 active (7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) + 250 FIXED + tracked-open

## Method (per finding)
Verify → Reproduce → Classify (CONFIRMED / FIXED-ALREADY / INVALID / SUPERSEDED) → Fix →
Re-review → Test → Benchmark (perf-tagged) → Document → Push → Loop.

One finding, one verified fix, one push. No shortcuts, no stubs, no hallucinated parity.
Safety hardenings preserved. Findings moved to findings/FIXED/ on resolution.

## SOPs
- Re-implement from understanding; never paste decompiled code; never copy proprietary
  strings; cite the corpus artifact in commits.
- Fix the cause not the symptom; match architecture and wiring.
- Every fix paired with a failing-then-passing test.
- Pull frequently; never force-push; keep main green.
- Token hygiene; commit+push after every coherent fix.
