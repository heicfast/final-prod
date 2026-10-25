# Session: diff-to-fix remediation — MINOR-first close-out wave (session 11)

Saved verbatim from the session prompt, token redacted. Read twice at start.

## Mandate

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with
tests and benchmarks, and mark it fixed in the audit trail with evidence.
The audit is a hypothesis; this session is the verifier. No hallucinated
parity: a finding is FIXED only with a test that proves it.

## Repos

- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- Token: `<redacted — credential store only>` (env/git config only, never committed)

## Session-specific instruction

Fix all the audit findings **(pick minor ones first)** one by one, with full
context and knowledge; update and put fixed findings in the FIXED directory.
Update all docs, counts and all properly. Target at least 20+ fixes before
session end.

## Operating loop (per finding)

1. Reproduce the audit's evidence on current main.
2. Re-verify the cited corpus artifact in full (not just the excerpt).
3. Classify: CONFIRMED-DIVERGENCE / ALREADY-FIXED (FIXED-ALREADY) /
   FALSE-POSITIVE (INVALID) / OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING.
4. Fix the cause, not the symptom; match architecture and wiring.
5. Test that fails before and passes after (unit + integration; fuzz for
   parsers; TSan/ASan/UBSan where threading/memory touched).
6. Resolution section in the finding file; move to findings/FIXED/;
   RESOLVED-INDEX row; CROSS-REF-MATRIX → MATCH; modules/*.md verdict;
   SUMMARY counts; worklog.md entry.
7. One finding, one verified fix, one push. Pull frequently, never
   force-push, leave main green.

## Hard rules

- No decompiled-code pasting; re-implement from understanding; cite corpus
  artifacts in commit messages.
- No stub/mock/TODO/fallback-masking. Production grade only.
- Safety hardenings preserved (dest-overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL
  gating, Win7-inclusive import gate, signing stays disabled).
- ReFS and LDM remain first-class.
- Audit trail is the deliverable; no finding files deleted.
