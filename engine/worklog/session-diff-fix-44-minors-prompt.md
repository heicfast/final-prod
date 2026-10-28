# Session prompt — the diff-to-fix execution mandate (Session-44 minors lane)

Saved at session open (T1), token redacted. Read twice at open; re-read periodically.

## Mandate
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE,
MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the finding, fix it in
the codebase, prove the fix with tests, and mark it fixed in the audit trail with evidence.
The audit is a hypothesis; this session is the verifier. A false finding is corrected in
the audit trail rather than "fixed" by breaking good code. User directive this lane:
**priority to MINOR first**, one by one, fixed findings land in `findings/FIXED/`, all
docs/counts updated, aim for 20+ fixes.

## Repositories (only two)
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git credential store only, never committed, never logged)

## Key rules
1. Read before touching: audit docs, corpus citations in full, core repo line-by-line where touched.
2. Independent verification loop per finding: reproduce evidence → re-verify against corpus →
   classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING).
3. Re-implement from understanding; never paste decompiled code; never copy proprietary strings;
   cite the corpus artifact in the commit message.
4. Fix the cause, not the symptom. Match architecture and wiring.
5. Every fix paired with a test that fails before and passes after.
6. Safety hardenings preserved (destination overlap refusal, BitLocker legitimate-unlock-only,
   RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
7. No shortcuts, no stubs, no mocks, no TODOs. Unfixable-this-session → tracked-open with next step.
8. One finding, one verified fix, one push. Pull frequently; never force-push; leave main green.
9. Win7-inclusive import gate intact; signing stays disabled.
10. Update audit trail per fix: finding Resolution + findings/FIXED/ move + CROSS-REF-MATRIX +
    SUMMARY counts + modules/*.md + worklog.md + docs where behavior changed.

## SOP
Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark → Document → Push → Loop.
