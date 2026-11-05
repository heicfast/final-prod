# Session-72 S72 fix-lane prompt (token redacted)

Saved at lane open. The user's mandate for this session (the twelfth
continuation of the diff-to-fix execution mandate; this lane takes the
MAJOR-first directive):

- Senior systems engineer posture (25+ years, Windows storage internals,
  C++20 production, parity remediation against reverse-engineered
  references). Execute the roadmap in `core_recovery/diff-to-fix/`.
- Two repositories in scope and only two: core (modified, heicfast/core_recovery)
  and the research corpus (ground truth, read-only, heicfast/easeus-drw-research).
  GitHub token from env/git config only — never committed, never logged
  (redacted here).
- Read before touching: every diff-to-fix doc, every finding, every module
  overview, every evidence artifact; the cited corpus artifact in full for
  each finding; the core repo in full at current main.
- For every finding (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL,
  TRACKED-OPEN), in priority order: reproduce the evidence, re-verify
  against the corpus, classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
  FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING), then fix.
- Fixing rules: re-implement from understanding (never paste decompiled
  code); fix the cause not the symptom; match architecture and wiring;
  every fix paired with a test that fails before and passes after; safety
  hardenings preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL);
  no shortcuts, no stubs, no mocks, no TODOs; ReFS and LDM stay
  first-class; do not break consumer contracts; pull frequently, never
  force-push, keep main green.
- One finding, one verified fix, one push, one finding-file update.
- Performance fixes: baseline, fix, re-benchmark, accept or revert; record
  the numbers. Windows direct I/O reads must be 4KB-multiple and 64KB-
  multiple where possible.
- Audit-trail updates after every fix: Resolution section in the finding
  file, CROSS-REF-MATRIX MATCH, modules/*.md verdict, SUMMARY counts,
  worklog entry. Findings move to findings/FIXED/ (never deleted).
- CI green on both lanes; Win7-inclusive import gate intact; signing stays
  disabled.
- The user's directive this lane: fix all the audit findings **priority to
  MAJOR first**, one by one, full context, aim for 20+ fixes; fixed
  findings go to the fixed directory; all docs and counts updated properly.

Lane opens at core `bbbe78b2` (main, post the S71 N-210 repair landing), the S71 seat conceded to the racing lane per SA-9, corpus `be0cfe9`. Baseline suite
1503/1503 green on Linux (linux-release preset, gcc 14.2, -Werror clean)
before the first change.
