# Session-42 diff-fix mandate (2026-10-29) — token redacted

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with tests
and benchmarks, and mark it fixed in the audit trail with evidence. Do not trust the
audit blindly; every finding is re-verified against the research corpus and the
running code before any change is made. A false finding is corrected in the audit
trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed. [REDACTED]

SOP: Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark ->
Document -> Push -> Loop. One finding, one verified fix, one push. Pull frequently.
Never force-push. Keep main green. No shortcuts, no stubs, no hallucinated parity.

Key rules:
- Re-implement from understanding; never paste decompiled code; cite the corpus
  artifact in the commit message.
- Every fix paired with a test that fails before and passes after.
- Resolution section template appended to every finding file (FIXED / FIXED-ALREADY /
  INVALID / SUPERSEDED + commit + verified-by + notes).
- Update CROSS-REF-MATRIX.md, modules/*.md, SUMMARY.md, worklog.md after each fix.
- Move fixed findings into findings/FIXED/ (the remote convention).
- Safety hardenings preserved: destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating.
- Win7-inclusive import gate intact; signing stays disabled.
