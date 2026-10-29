# Session-86 (S86) — diff-to-fix session prompt (token redacted)

Saved at session open per the mandate. Core at open: `b6801ade`; corpus `be0cfe9`.

## The mandate (condensed, full text below)

Senior systems engineer role. Execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. Do not trust
the audit blindly; a false finding is corrected in the audit trail rather than
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
3. Fixing rules: re-implement from understanding (never paste decompiled
   code, never copy proprietary strings, cite the corpus artifact in the
   commit message); fix the cause not the symptom; match architecture and
   wiring; every fix paired with a test that fails before and passes after;
   safety hardenings preserved; no stubs/mocks/TODOs; ReFS and LDM stay
   first-class; pull frequently, never force-push, leave main green; commit
   and push after every coherent finding fix.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN
   — **this session's user addendum: "priority to minor first"** (the two
   open MINORs are driver-repo-resident per OQ-S10-3 — the boundary trail
   items are handled first, then the in-repo MAJOR queue).
5. Performance fixes measured only against baseline → fix → re-benchmark;
   Windows direct I/O must use 4 KB-multiple and 64 KB-multiple-when-possible
   read lengths.
6. Test proof set: unit + integration on seeded fixtures + structure-aware
   fuzz for parsers + TSan/ASan/UBSan on touched code + CLI contract tests +
   ABI consumer test + benchmark comparison for perf-tagged findings.
7. Audit trail updates after every fix: Resolution section in the finding
   file, CROSS-REF-MATRIX row → MATCH, modules/*.md verdict, SUMMARY.md
   counts, README if needed, worklog.md.
8. Discovered divergences during fixing: new finding files in the correct
   severity folder, matrix row, SUMMARY update, fix this session or track
   precisely — never silently.
9. CI quality gates: Windows CI green (MSVC /W4 /WX, DLL smoke, CLI
   contract, seeded integration, C consumer, fuzz smoke, Win7-inclusive
   import allowlist); Linux CI green (pure core, CLI, ASan/UBSan/TSan on
   touched tests, fuzz smoke); no continue-on-error; signing stays
   disabled.
10. Documentation: every fix documented in the finding's Resolution +
    worklog.md; CAPABILITY-MATRIX / architecture.md /
    ground-truth-contracts.md / error-fix-registry.md updated in the same
    commit; resolved TODO/FIXME/HACK notes removed; PARITY-DELTAS.md holds
    every deliberate divergence.
11. Deliverables: every finding FIXED / FIXED-ALREADY / INVALID / SUPERSEDED
    with evidence; INFORMATIONALs ACKNOWLEDGED; TRACKED-OPENs resolved or
    precisely pushed; matrix populated; counts current; worklog handoff; the
    `diff-to-fix/` directory is the final evidence set; no finding file
    deleted; a tagged release if the audit closes cleanly.

## Session-specific user instructions (verbatim tail of the prompt)

"Fix all the audit findings (priority to minor first) one by one, with full
context and knowledge and update and put fixed findings in fixed directory.
Update all docs, counts and all properly. Aim for at least 20+ fixes in this
session, don't stop before that."

(The `findings/FIXED/` directory is the established relocation target for
resolved findings — fixed findings move there with their Resolution sections;
nothing is deleted.)

## First 20 TODOs (as given, adapted IDs preserved)

T1 pull+record → T2 read diff-to-fix → T3 read core → T4-T19 the per-severity
reproduce/classify/fix/test/benchmark/document loops → T20 tag release,
worklog handoff, final verification sweep.
