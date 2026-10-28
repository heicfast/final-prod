# Session prompt — the diff-to-fix execution mandate (S42 lane)

Saved at session open (T1), token redacted. Read twice at open; re-read periodically.

## Mandate
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE,
MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the finding, fix it in
the codebase, prove the fix with tests, and mark it fixed in the audit trail with evidence.
The audit is a hypothesis; this session is the verifier. A false finding is corrected in
the audit trail rather than "fixed" by breaking good code.

## Repositories (only two)
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git credential store only, never committed, never logged)

## Key rules
1. Read before touching: audit docs, corpus citations in full, core repo line-by-line where touched.
2. Independent verification loop per finding: reproduce evidence → re-verify against corpus →
   classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING)
   → only then fix.
3. Resolution section added to every finding touched (FIXED / FIXED-ALREADY / INVALID / SUPERSEDED
   with commit SHA, test names, notes). Findings never deleted — moved to findings/FIXED/ per the
   session-39 user amendment ("put fixed findings in fixed directory").
4. Re-implement from understanding; never paste decompiled code; cite corpus artifact in commit message.
5. Every fix paired with a test that fails before and passes after.
6. Safety hardenings preserved (destination overlap refusal, BitLocker legitimate-unlock-only,
   RawFixer consent, 4Kn, cancellable IRP, SDDL gating) — noted in Resolutions.
7. No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs. Production grade only.
8. One finding, one verified fix, one push, one finding-file update (batch only per-module minors).
9. Performance fixes: baseline first, fix, re-benchmark, accept-or-revert; record numbers.
10. Windows direct I/O: multiple-of-4KB, multiple-of-64KB where possible.
11. CI green both platforms; Win7-inclusive import allowlist gate intact; signing stays disabled.
12. Update CROSS-REF-MATRIX (MATCH end state), SUMMARY counts, modules/*.md, docs/*, FIXED-LEDGER,
    worklog.md — the audit trail is the deliverable.
13. Fix order per user: MAJOR FIRST this session (the user's explicit final directive), then
    SEVERE, MINOR, INFO, TRACKED-OPEN. Aim 20+ fixes.
14. Discovered divergences: new finding files, never silent fixes.
15. Pull frequently, never force-push, leave main green.
