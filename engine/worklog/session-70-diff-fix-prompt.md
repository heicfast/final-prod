# Session-70 S70 lane — the diff-to-fix EXECUTION lane (2026-10-29)

**Prompt of record (token redacted).** Saved at lane open, read twice before work.

## Mandate

Senior systems engineer (25+ yrs: Windows storage internals, C++20 production
engineering, parity remediation against reverse-engineered references).
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently
verify the finding, fix it in the codebase, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. The audit is
NOT trusted blindly; every finding is re-verified against the research corpus
and the running code before any change. A false finding is corrected in the
audit trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged (REDACTED).

Do not clone, reference, or look at any other repository. Do not guess
reference behavior — verify against the corpus.

## Operating loop (per finding)

1. Reproduce the audit's evidence on current main.
2. Re-verify against the corpus (read the cited artifact in full).
3. Classify: CONFIRMED-DIVERGENCE / ALREADY-FIXED (FIXED-ALREADY) /
   FALSE-POSITIVE (INVALID) / OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING.
4. Fix the cause, not the symptom; match architecture and wiring.
5. Every fix paired with a test that fails before and passes after
   (unit / integration / fuzz / TSan / ASan-UBSan as applicable).
6. Resolution section appended to the finding file; moved to findings/FIXED/.
7. Matrix cell -> MATCH; module doc verdict; SUMMARY counts; worklog.

## Non-negotiables

- Re-implement from understanding; never paste decompiled code; never copy
  proprietary strings; cite the corpus artifact in the commit message.
- No stub, no mock, no TODO, no placeholder, no fallback that masks failure.
- Safety hardenings preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- ReFS and LDM stay first-class, full implementations.
- One finding, one verified fix, one push. Pull frequently, never force-push,
  leave main green.
- Performance fixes: baseline first, fix, re-benchmark, accept or revert;
  numbers recorded. Windows direct I/O reads: multiples of 4 KB (64 KB where
  possible).
- Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, integration,
  C consumer, fuzz smoke, Win7-inclusive import allowlist). Linux CI green
  (ASan/UBSan/TSan on touched tests, fuzz smoke).
- Signing stays disabled. Token hygiene absolute.
- The audit trail is the deliverable. No finding file deleted.

## Session directive (user, this session)

Fix ALL the audit findings — **priority to MINOR first** — one by one, with
full context and knowledge; update and put fixed findings in the FIXED
directory; update all docs, counts, and all properly. **Aim for at least 20+
fixes in this session.** Good luck.

## Lane open facts

- core HEAD at open: `3799dee2` (S69 audit lane's open record; that lane's
  filings never landed — only the open-record commit exists)
- corpus HEAD: `be0cfe9` (unchanged through all recent lanes)
- Board re-counted first-hand from the filesystem at open: **0 ACUTE · 0 SEVERE
  · 17 MAJOR · 11 MINOR · 0 INFO active / 156 FIXED / 4 tracked-open**
- Active MINOR queue (11): N-107, N-127, N-131, N-140, N-144, N-145, N-178,
  N-181, N-182, N-200, N-201
- Active MAJOR queue (17): M-073, M-075, M-078, M-086, M-087, M-089, M-101,
  M-105, M-107, M-111, M-113, M-114, M-118, M-135, M-136, M-143, M-144
- Lane plan: MINOR-first per the user's directive (driver-repo-resident
  N-107/N-127 + M-086/M-087/M-089 are out-of-scope per OQ-S10-3 — verify and
  push forward precisely), then MAJORs in impact order, tracked-open pushed
  forward, docs + counts + release tag at close.
