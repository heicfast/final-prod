# Session-60 diff-to-fix execution prompt (MINOR-first closing lane)

Saved at lane open (token redacted per the standing rule). Core HEAD at open:
`98a5b139` (the last MSVC /W4 repair; suite 1460/1460). Corpus: `be0cfe9`.

## The mandate (verbatim summary, token removed)

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with tests
and benchmarks, and mark it fixed in the audit trail with evidence. Do not trust
the audit blindly; every finding is re-verified against the research corpus and the
running code before any change is made. A false finding is corrected in the audit
trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research

Do not clone, reference, or look at any other repository. Do not guess reference
behavior — verify against the corpus.

## Key SOPs (the operative rules)

1. Read before touching anything: the audit output first (README, SUMMARY,
   SEVERITY-SCALE, PARAMETERS, CROSS-REF-MATRIX, every finding, every module doc,
   every evidence artifact), then the corpus line by line where a finding requires
   it, then the core repository. Record the audited commit in the worklog and in
   every finding touched.
2. Independently verify each finding before fixing: reproduce the evidence,
   re-verify against the corpus, classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING), only then proceed.
3. For each finding touched, add a Resolution section at the bottom of the finding
   file (Status / Fixed in commit / Verified by / Resolution notes). Do not delete
   the finding — move it to findings/FIXED/ once resolved.
4. Fixing rules: re-implement from understanding (never paste decompiled code);
   fix the cause not the symptom; match architecture and wiring; every fix paired
   with a test that fails before and passes after (unit for logic, integration on
   seeded fixtures, structure-aware fuzz for parsers, TSan for threading,
   ASan/UBSan for memory); safety hardenings are preserved and noted; no stubs,
   no mocks, no TODOs, no fallbacks that mask failure; ReFS and LDM stay
   first-class; pull frequently, never force-push, keep main green; token never
   committed or logged.
5. Fix order: the session directive is MINOR-FIRST (the user's closing line:
   "Fix all the audit findings (priority to minor first) one by one ... Aim for
   at least 20+ fixes in this session"), then INFORMATIONAL acknowledgements,
   then TRACKED-OPEN push-forward, ascending to the higher tiers as capacity
   allows.
6. Performance-tagged findings: baseline, fix, re-benchmark, accept or revert;
   Windows direct I/O reads must be 4 KB-multiple and 64 KB-multiple where
   possible.
7. Test names describe the behavior proven, not the finding ID.
8. Audit-trail updates after every fix: the finding's Resolution section, the
   CROSS-REF-MATRIX cell to MATCH, modules/*.md verdict rows, SUMMARY.md counts,
   README audited-commit line if needed, worklog.md entry, FIXED-LEDGER row.
9. Discovered divergences during fixing: new finding files per §8, fixed this
   session or precisely tracked.
10. CI gates: Linux green (Werror build, ctest, format, lints, doc-truth gate);
    Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, integration, fuzz
    smoke, ABI consumer, Win7-inclusive import allowlist gate); no
    continue-on-error; signing stays disabled.
11. Deliverables: every finding resolved to FIXED / FIXED-ALREADY / INVALID /
    SUPERSEDED / ACKNOWLEDGED; the matrix fully populated; SUMMARY counts
    current; worklog handoff; the diff-to-fix directory is the final evidence
    set. Fixed findings live in findings/FIXED/.

## Board at lane open (filesystem truth at `98a5b139`)

48 active findings: 0 ACUTE · 1 SEVERE (S-040) · 23 MAJOR · 22 MINOR
(N-107, N-127, N-131, N-140, N-144, N-145, N-178, N-181, N-182, N-192, N-194,
N-195, N-196, N-197, N-198, N-199, N-200, N-201, N-202, N-203, N-204, N-205) ·
2 INFO (I-045, I-046) + 4 tracked-open (OQ-S10-1, OQ-S10-3, OQ-S4-3, OQ-S52-1)
+ 136 FIXED files.

This lane's plan: the 20 core-repo-resident MINORs fixed one-by-one (N-107 and
N-127's fix surfaces live in the out-of-grant rda-driver repo — they stay
properly tracked under OQ-S10-3), both INFOs acknowledged with their register
work, and the four OQs pushed forward with fresh verification at HEAD.
