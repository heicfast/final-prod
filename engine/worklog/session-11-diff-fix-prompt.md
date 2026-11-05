# Session-11 diff-fix prompt (redacted)

**Saved:** 2026-10-29 (session-11 fix lane). **Token redacted per SOP.**

## Mandate
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding, fix it in the codebase, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. The audit is
a hypothesis; this session is the verifier. False positives are corrected in
the audit trail rather than "fixed" by breaking good code.

## Repositories
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed. [REDACTED]
- No other repository may be cloned, referenced, or consulted.

## Key SOPs
- Verify -> Reproduce -> Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
  FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) -> Fix -> Re-review ->
  Test -> Benchmark -> Document -> Push -> Loop.
- Re-implement from understanding; never paste decompiled code; never copy
  proprietary strings; cite the corpus artifact in the commit message.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- One finding, one verified fix, one push. Pull frequently; never force-push.
- Performance fixes measured, not guessed; baseline -> fix -> re-benchmark.
- Windows direct I/O reads: multiple of 4KB and of 64KB where possible.
- The corpus is the only source of truth; when audit evidence conflicts with
  the corpus, the corpus wins.
- No shortcuts, no fallbacks that mask failure, no stubs/mocks/TODOs.
  Production grade only.
