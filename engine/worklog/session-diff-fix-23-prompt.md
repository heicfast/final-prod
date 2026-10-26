# Session DFX23 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus)

Saved at session start, token redacted. Date: 2026-10-26.

## Mandate (abridged, token redacted)

Senior systems engineer role. Execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it, prove the fix with tests, and mark it
in the audit trail with evidence.

Two repos in scope only:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

User directive for this session: focus on INFORMATIONAL and tracked-open
initially, then continue one by one. Target at least 20+ fixes before session
end. Put fixed findings in the fixed directory. Update all docs, counts.

## Core SOPs

- Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark → Document → Push → Loop.
- One finding, one verified fix, one push.
- No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs.
- Every fix paired with a test that fails before, passes after.
- Safety hardenings preserved (dest overlap refusal, BitLocker legit-unlock-only,
  RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- Pull frequently, never force-push, keep main green.
- INFORMATIONAL: verify deliberate-difference rationale, mark ACKNOWLEDGED, no
  code change unless rationale wrong.
- Track open items: resolve or push forward with precise next step + evidence.

## Session start state

- Core HEAD: 716d121 (worklog(DFX16-M013): rebase reconciliation)
- Corpus HEAD: 03ad46c (unmoved throughout history)
- Open: 0 ACUTE, 0 SEVERE, 14 MAJOR, 19 MINOR, 22 INFORMATIONAL, 11 tracked-open
- FIXED: 149
