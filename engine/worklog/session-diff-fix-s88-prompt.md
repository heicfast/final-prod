# Session-88 (S88) — diff-to-fix session prompt (re-homed per SA-9: the racing windows took S68-S87) (token redacted)

Saved at session open per the mandate. Core at open: `ddab68b4` (main, clean
clone); corpus `be0cfe9` throughout.

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

## Session-specific directive

Fix all the audit findings (priority to MAJOR first) one by one, with full
context and knowledge; update and put fixed findings in the FIXED directory.
Update all docs and counts properly. Aim for at least 20+ fixes in this
session.

## Operating rules (the SOPs, verbatim order)

1. Read before touching anything: diff-to-fix/ in full (README, SUMMARY,
   SEVERITY-SCALE, PARAMETERS, CROSS-REF-MATRIX, every finding, every module
   doc, every evidence artifact); the corpus line-by-line where a finding
   requires it; the core repository in full; record the audited commit.
2. Independently verify each finding before fixing: reproduce the evidence,
   re-verify against the corpus, classify
   CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE /
   NEW-FINDING; only then fix.
3. Fixing rules: re-implement from understanding (never paste decompiled
   code); fix the cause not the symptom; match architecture and wiring; every
   fix paired with a test that fails before and passes after; safety
   hardenings are not slop — preserve them; no shortcuts, no fallbacks that
   mask failure, no stubs/mocks/TODOs; no hallucinated parity; ReFS and LDM
   remain first-class; do not break contracts without updating consumers in
   the same change; pull frequently, never force-push; token hygiene.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN;
   within each tier, correctness before performance, data integrity before
   latency, silent-failure paths before observable inefficiencies.
5. Performance fixes: measure, do not guess — baseline, fix, re-benchmark,
   accept or revert; record numbers. Windows direct I/O: multiple of 4 KB,
   multiple of 64 KB where possible.
6. Adding tests: unit for logic, integration on seeded fixtures, fuzz for
   parsers touched, TSan for threading, ASan/UBSan for memory, CLI contract
   for grammar changes, ABI consumer for C ABI surfaces, benchmarks for
   performance-tagged findings. Test names describe behavior, not finding IDs.
7. Update the audit trail after every fix: Resolution section in the finding
   file, CROSS-REF-MATRIX → MATCH, modules/<module>.md, SUMMARY.md counts,
   README if needed, worklog.md.
8. Discovered divergences: file new findings with full evidence, add matrix
   rows, update SUMMARY, announce in commit messages. No silent fixes.
9. CI and quality gates: Windows CI green (MSVC /W4 /WX, DLL smoke, CLI
   contract, integration, C consumer, fuzz smoke, Win7-inclusive import
   allowlist); Linux CI green (pure-core, CLI, sanitizers on touched tests,
   fuzz smoke); no continue-on-error; main stays green; Win7-inclusive
   loadability gate intact; signing stays disabled.
10. Documentation: every fix documented in Resolution + worklog.md;
    CAPABILITY-MATRIX / architecture / ground-truth-contracts /
    error-fix-registry updated in the same commit; resolved TODOs removed;
    PARITY-DELTAS for deliberate divergences.
11. Deliverables: every finding in a terminal state; the audit trail is the
    deliverable; one finding, one verified fix, one push.
12. First 20 TODOs as listed in the mandate; spawn 20 more on completion.
13. SOPs: verify → reproduce → classify → fix → re-review → test → benchmark
    → document → push → loop. The audit trail is the deliverable.

## The session's board at open (from SUMMARY.md + findings/ enumeration)

0 ACUTE · 0 SEVERE · 6 MAJOR (M-086, M-087, M-089 driver-repo-resident per
OQ-S10-3; M-101, M-105, M-107 in-scope) · 3 MINOR (N-107, N-127
driver-repo-resident; N-223 FIXED in-file but not yet in FIXED/) · 4
tracked-open (OQ-S10-1, OQ-S10-3, OQ-S4-3, OQ-S79-1) · 202 FIXED.
M-107 members landed so far: X3F (1), CRW (2), DWG gen-1/gen-2 (3), PSP (4),
WPG (5) — remaining: WPD, SIT, APE, EPS, DXF, SWF-RECT + the DWG gen-3
R2007 remainder.

## Defect found at open (the sixth N-223-class recurrence)

`ddab68b4` itself committed live conflict markers into worklog.md:2-103 (the
S87 N-181 concession entry vs the S84 DWG-member-3 entry) — filed as N-224
this session and repaired both-sides-kept before any other work.
