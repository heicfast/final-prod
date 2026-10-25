# Session 16 — diff-to-fix remediation prompt (token redacted)

Saved at session bootstrap. The GitHub token was provided out-of-band
(env/git credential store only); it is redacted here and must never
appear in any committed file or log.

## Mandate

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix
with tests and benchmarks, and mark it fixed in the audit trail with
evidence. The audit is a hypothesis; the corpus is ground truth. A false
finding is corrected in the audit trail rather than "fixed" by breaking
good code.

Two repositories in scope, only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — env/git config only, never committed]

## Operating loop (per finding, priority ACUTE → SEVERE → MAJOR → MINOR → INFO → TRACKED-OPEN)

1. Reproduce the audit's evidence on current main; if not reproducible → candidate for correction.
2. Re-verify against the corpus artifact directly (read in full, not just the excerpt).
3. Classify: CONFIRMED-DIVERGENCE | ALREADY-FIXED (FIXED-ALREADY) | FALSE-POSITIVE (INVALID) | OUTDATED-EVIDENCE (SUPERSEDED) | NEW-FINDING.
4. Fix the cause, not the symptom; match architecture and wiring; re-implement from understanding (never paste decompiled code, never copy proprietary strings; cite the corpus artifact in the commit message).
5. Every fix paired with a test that fails before and passes after. Unit for logic, integration for seeded fixtures, structure-aware fuzz for parsers, TSan for threading, ASan/UBSan for memory.
6. Record a Resolution section in the finding file (status, commit, verified-by, notes); move to findings/FIXED/; update CROSS-REF-MATRIX, module doc, SUMMARY counts, worklog.
7. One finding, one verified fix, one push. Pull frequently; never force-push; leave main green.

## Non-negotiables

- No stub, no mock, no TODO, no placeholder, no fallback that masks failure.
- Safety hardenings preserved (destination overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- ReFS and LDM stay first-class.
- Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, seeded integration, C consumer, fuzz smoke, Win7-inclusive import allowlist). Linux CI green (core, CLI, ASan/UBSan/TSan, fuzz smoke).
- Win7-inclusive build envelope intact. Signing stays disabled.
- Performance fixes measured before/after, never guessed. Windows direct I/O reads multiple-of-4KB and multiple-of-64KB where possible.
- Token hygiene: never commit, never log.
- Audit trail is the deliverable: no finding file deleted, resolved findings carry Resolution sections.

## Session-16 entry state

- Bootstrap commit: `53629a3` (core main), corpus `03ad46c` (unmoved).
- Baseline suite: 584/584 green (linux-gcc RelWithDebInfo + ASan).
- Open inventory at bootstrap: SEVERE 1 (S-029) · MAJOR 32 · MINOR 43 · INFO 22 · FIXED/ 94 files · tracked-open 16.
- User directive: fix all MAJOR findings first, target 20+ fixes this session, fixed findings go to the fixed directory, all docs/counts updated.
