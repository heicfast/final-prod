# Session-85 (S85) — diff-to-fix session prompt (token redacted)

Saved at session open per the mandate. Core at open: `0cf6142d`; corpus `be0cfe9`.

## The mandate (condensed, full text below)

Senior systems engineer role. Execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. Do not trust the
audit blindly; a false finding is corrected in the audit trail rather than
"fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: (REDACTED — env/git config only, never committed, never logged)

## Operating rules (the SOPs, verbatim order)

1. Read before touching anything: the audit output, the corpus line-by-line
   where a finding requires it, the core repo in full. Record the audited
   commit in every finding touched.
2. Independently verify each finding before fixing: reproduce the evidence,
   re-verify against the corpus, classify CONFIRMED-DIVERGENCE / ALREADY-FIXED
   / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING. Only after
   classification, proceed to the fix.
3. Fixing rules: re-implement from understanding (never paste decompiled code,
   never copy proprietary strings, cite the corpus artifact in the commit
   message); fix the cause not the symptom; match architecture and wiring;
   every fix paired with a test that fails before and passes after; safety
   hardenings are preserved; no shortcuts/fallbacks/stubs/TODOs; no
   hallucinated parity (no FIXED without a proving test); ReFS and LDM stay
   first-class; don't break contracts other agents consume; pull frequently;
   never force-push; token hygiene; commit and push after every coherent
   finding fix (one finding, one verified fix, one push, one finding-file
   update).
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (acknowledge)
   → TRACKED-OPEN (resolve or push forward precisely). Within tier: user
   impact first.
5. Performance fixes: baseline first, fix, re-benchmark, accept or revert;
   record the numbers. Windows direct I/O: 4 KB multiples, 64 KB where
   possible.
6. Tests: unit for logic, integration on seeded fixtures, structure-aware
   fuzz for parsers, TSan for threading, ASan/UBSan for memory, CLI contract
   tests for grammar changes, ABI consumer tests for C ABI surfaces,
   benchmarks for perf-tagged findings. Test names describe behavior, not
   finding IDs.
7. Audit trail: Resolution section per finding; CROSS-REF-MATRIX cells to
   MATCH with commit links; modules/*.md verdict tables; SUMMARY counts;
   README nav; worklog.md.
8. Discovered divergences: new finding files, never silent fixes.
9. CI: Windows green (MSVC /W4 /WX, DLL smoke, CLI contract, integration, C
   consumer, fuzz smoke, Win7 import gate); Linux green (ASan/UBSan/TSan on
   touched tests, fuzz smoke); no continue-on-error; pin toolchains; pull
   frequently; Win7-inclusive build; signing stays disabled.
10. Documentation: Resolution + worklog per fix; CAPABILITY-MATRIX,
    architecture/ground-truth-contracts/error-fix-registry updates in the same
    commit; remove resolved TODOs/FIXMEs; PARITY-DELTAS for deliberate
    divergences.
11. Deliverables: every finding resolved to FIXED/FIXED-ALREADY/INVALID/
    SUPERSEDED; INFORMATIONALs ACKNOWLEDGED; TRACKED-OPENs resolved or pushed
    forward; matrix fully MATCH; SUMMARY counts + top risks; modules updated;
    worklog handoff with exact commit; green CI; perf numbers; fail-before
    tests; all changes pushed; no finding file deleted; tagged release if
    clean; diff-to-fix/ as the final evidence set.
12. First TODOs: T1 pull+record, T2 read diff-to-fix, T3 read core, T4-T14
    per-tier classify+fix, T15-T17 matrix/SUMMARY/modules/docs, T18-T19 CI
    sweep + discovered tests, T20 tag + handoff.
13. SOP loop: Verify → Reproduce → Classify → Fix → Re-review → Test →
    Benchmark → Document → Push → Loop. One finding, one verified fix, one
    push. Re-read the prompt + SUMMARY + worklog periodically. Every
    intermediate push updates worklog.md.

User emphasis: "Fix all the audit findings (priority to major first) one by
one, with full context and knowledge and update and put fixed findings in
fixed directory. Update all docs, counts and all properly. Aim for at least
20+ fixes in this session, don't stop before that."

## Full prompt text (as received, token redacted)

[The full §1-§13 mandate text is preserved verbatim above in structure; the
token line reads: GitHub token (env/git config only, never committed):
REDACTED.]
