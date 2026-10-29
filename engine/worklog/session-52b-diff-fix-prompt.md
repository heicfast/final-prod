# Session prompt — the diff-to-fix execution mandate (S52-B lane; renamed at the rebase per SA-9 — the remote S45 lane pushed first)

**Saved:** 2026-10-29 (S45 lane opens)
**Pin:** core `869da779` audited / current main `659f0f6c` / corpus `be0cfe9`

## Mandate summary

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding, fix it in the codebase, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. The audit is a
hypothesis; this lane is the verifier. A false finding is corrected in the
audit trail rather than "fixed" by breaking good code.

Two repositories in scope, only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged. (redacted)

## Operating loop (per finding)

1. Reproduce the audit's evidence on current main.
2. Re-verify the cited corpus artifact in full.
3. Classify: CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
   OUTDATED-EVIDENCE / NEW-FINDING.
4. Fix the cause, not the symptom. Re-implement from understanding; never
   paste decompiled code; cite the corpus artifact in the commit.
5. Pair every fix with a test that fails before and passes after (unit /
   integration / fuzz / TSan / ASan / UBSan as applicable).
6. Performance fixes: baseline → fix → re-benchmark → accept or revert;
   numbers recorded in the Resolution and SUMMARY.md.
7. Add the Resolution section to the finding file; move it to findings/FIXED/;
   update CROSS-REF-MATRIX.md, SUMMARY.md, modules/<module>.md, worklog.md.
8. One finding, one verified fix, one push. Pull frequently, never force-push,
   keep main green.

## Fix order

ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN. Within a tier:
correctness before performance, data integrity before latency, silent-failure
paths before observable inefficiencies. User directive: aim for 20+ fixes this
session; fixed findings go to the fixed directory; all docs and counts updated.

## Standing rules

- Safety hardenings preserved (destination-overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer explicit consent, 4Kn handling,
  cancellable IRP, SDDL gating) — documented improvements, never regressed.
- ReFS and LDM stay first-class; never downgraded to report-only.
- Win7-inclusive import gate intact; signing stays disabled.
- No stub, no mock, no TODO, no placeholder, no fallback masking failure.
- No hallucinated parity: no finding marked FIXED without a failing-then-
  passing test.
- Commit messages carry finding ID, divergence, fix, test, corpus citation.

## Deliverables checklist

1. Every finding resolved: FIXED / FIXED-ALREADY / INVALID / SUPERSEDED.
2. INFORMATIONAL findings ACKNOWLEDGED.
3. TRACKED-OPEN items resolved or precisely pushed forward.
4. CROSS-REF-MATRIX fully MATCH where fixed.
5. SUMMARY counts current; performance numbers recorded.
6. modules/*.md verdicts current.
7. worklog.md chronological with SHAs and test evidence.
8. Green CI posture: Linux build+tests clean locally; sanitizers clean on
   touched code.
9. Before/after numbers for performance findings.
10. Tests named for behavior, not finding IDs.
11. All changes pushed with evidence-citing messages.
12. No finding file deleted — audit trail preserved (moved to FIXED/).
