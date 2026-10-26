# Session DFX35 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus)

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
# Session 35 (DFX35, re-stamped from 23 — squatted by a parallel lane) — diff-to-fix MAJOR-first closing session

Saved verbatim from the session prompt, token redacted. Read twice before working.

## Mandate

Senior systems engineer mandate (25+ years, Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered
references). Execute the roadmap in `core_recovery/diff-to-fix/`. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with
tests and benchmarks, and mark it fixed in the audit trail with evidence.
The audit is not trusted blindly; every finding is re-verified against the
research corpus and the running code before any change. A false finding is
corrected in the audit trail rather than "fixed" by breaking good code.

## Repositories in scope (only two)

- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged
  (REDACTED from this file).

No other repository is cloned, referenced, or examined. Reference behavior is
never guessed — verified against the corpus.

## Operating loop (per finding, priority ACUTE → SEVERE → MAJOR → MINOR → INFO → TRACKED-OPEN)

1. Reproduce the audit's evidence on current main.
2. Re-verify the cited corpus artifact in full (not just the excerpt).
3. Classify: CONFIRMED-DIVERGENCE / ALREADY-FIXED (FIXED-ALREADY) /
   FALSE-POSITIVE (INVALID) / OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING.
4. Fix the cause, not the symptom; match architecture and wiring.
5. Pair every fix with tests that fail before and pass after
   (unit / integration / fuzz / TSan / ASan-UBSan as applicable).
6. Benchmark any performance-tagged finding (baseline → fix → re-measure;
   accept only if no parameter regresses; Windows direct I/O in multiples of
   4 KiB, and 64 KiB where possible).
7. Update the audit trail: Resolution section in the finding file, move to
   findings/FIXED/, CROSS-REF-MATRIX.md cell → MATCH, modules/<module>.md
   verdict, SUMMARY.md counts, worklog.md. No finding file deleted.
8. One finding, one verified fix, one push. Pull frequently, never
   force-push, keep main green.

## Session-specific instruction

Fix all the audit findings (focus on MAJOR initially) one by one, with full
context and knowledge; update and put fixed findings in the FIXED directory.
Update all docs, counts and all properly. Target for at least 20+ fixes
before session end.

## Standing rules

- Safety hardenings preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer explicit consent, 4Kn handling,
  cancellable IRP, SDDL gating).
- ReFS and LDM remain first-class, full implementations.
- Win7-inclusive import allowlist gate intact; signing stays disabled.
- No stubs, no mocks, no TODO, no placeholder, no fallback that masks
  failure. Production grade only.
- Corpus wins over the audit when evidence conflicts.

## Baseline (recorded at session start)

- Core HEAD: 716d121 (worklog DFX16-M013 rebase reconciliation)
- Corpus: 03ad46c (unmoved)
- Open inventory at start: MAJOR 14 · MINOR 19 · INFO 24 · tracked-open 11
- SEVERE/ACUTE tiers: empty
# Session DFX35 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus)

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
