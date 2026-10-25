# Session DFX17 (diff-to-fix session 17) — MINOR-first close-out wave

## Mandate (user prompt, token redacted)

You are a senior systems engineer (25+ years) in Windows storage internals,
C++20 production engineering, and parity remediation against
reverse-engineered references. Your mandate this session: execute the
roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE,
MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the
finding, fix it in the codebase, prove the fix with tests and benchmarks,
and mark it fixed in the audit trail with evidence.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — env/git config only, never committed, never logged]

Key SOPs:
- Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark -> Document -> Push -> Loop
- One finding, one verified fix, one push, one finding-file update.
- No shortcuts. No fallbacks that mask failure. No mock, no stub, no TODO.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- Pull frequently, never force-push, leave main green.
- Fixed findings moved to findings/FIXED/ with RESOLVED-INDEX rows;
  counts and all docs updated.
- User priority for this session: FIX ALL MINOR FINDINGS FIRST,
  target 20+ fixes before session end.

(Reference: full SOP text in the session prompt; this is the working digest.)
