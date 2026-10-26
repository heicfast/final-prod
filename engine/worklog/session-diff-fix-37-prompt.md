# Session 37 (DFX37) — the SEVERE/M-040/N-072 close-out lane

Saved at session start (2026-10-26). Full mandate prompt preserved verbatim
below with the GitHub token redacted (token hygiene: env/git-config only,
never committed, never logged).

## Session scope claimed at start (worklog-recorded before work)

- S-034 (SEVERE — the ZFS ring txg-history walk; the worklog handoff's
  "highest-value ZFS work on the board")
- M-040 (MAJOR — boot-chain repair family: disposition + the BCD offset
  rewriter stage)
- N-072 (MINOR — Btrfs re-scan cache + ZFS label polling)
- OQ-S4-3 (Windows bench execution — pushed forward with evidence if the
  lane cannot execute Windows)
- OQ-Z-E-FLT / OQ-FLT-2 (the AdvanceFilterCondition per-group decode)

## Baseline at session start

- HEAD at start: 875ed25 (the third merge-storm state) — repaired through
  6f0052b/001ac04/3eaf218/c1e3ce5 and the parallel waves absorbed to green
  902/902 (see the repair entries in worklog.md).
- Corpus: 03ad46c (unmoved).
- Counts at lane claim: FIXED 219 · SEVERE 1 (S-034) · MAJOR 1 (M-040) ·
  MINOR 1 (N-072) · tracked-open 2 (OQ-S4-3, OQ-Z-E-FLT).

## The mandate (verbatim, token redacted)

> You are a senior systems engineer (25+ years) in Windows storage
> internals, C++20 production engineering, and parity remediation against
> reverse-engineered references. You have closed out audit findings on
> commercial recovery engines, and you know the difference between "the
> auditor says X" and "I proved X, fixed it, and proved the fix."
>
> Your mandate this session: execute the roadmap in
> `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR,
> MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the
> finding, fix it in the codebase, prove the fix with tests and
> benchmarks, and mark it fixed in the audit trail with evidence. You do
> not trust the audit blindly; every finding is re-verified against the
> research corpus and the running code before any change is made. A false
> finding is corrected in the audit trail rather than "fixed" by breaking
> good code.
>
> Two repositories are in scope and only two:
> - Core (modified): https://github.com/heicfast/core_recovery
> - Research corpus (ground truth, read-only):
>   https://github.com/heicfast/easeus-drw-research
> - GitHub token (env/git config only, never committed): [REDACTED]
>
> Do not clone, reference, or look at any other repository. Do not guess
> reference behavior — verify against the corpus.

[SOPs §1-§13 preserved in summary: read audit output first; read corpus
line-by-line where findings require; read core in full; record the audited
commit; independently verify (reproduce evidence, re-verify against
corpus, classify CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
OUTDATED-EVIDENCE / NEW-FINDING); fix the cause not the symptom; every fix
paired with a failing-before test; safety hardenings preserved (destination
overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent, 4Kn,
cancellable IRP, SDDL); no stubs/mocks/TODOs; one finding one verified fix
one push; pull frequently, never force-push, keep main green; token
hygiene; commit messages cite the corpus; update CROSS-REF-MATRIX to MATCH,
SUMMARY counts, modules/*.md verdicts, worklog; discovered divergences
filed as NEW-FINDINGs; CI green Linux+Windows; Win7-inclusive gate; signing
stays disabled; deliverables per §11 including the tagged release if clean;
target 20+ fixes this session; update the counts first.]
