# Session DFX-23 — diff-to-fix roadmap execution (MINOR-first mandate)

Saved at session start (2026-10-26). Token redacted. Full mandate as issued:
execute the roadmap in core_recovery/diff-to-fix/ — for every finding
(ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN): independently verify
against corpus + running code, classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING), fix with paired fail-before/
pass-after tests, prove, and mark in the audit trail with evidence. False
findings are corrected in the audit trail, not "fixed" by breaking good code.

Session-specific mandate from the operator:
- Focus on MINOR findings initially; one finding, one verified fix, one push.
- Target at least 20+ fixes before session end.
- Fixed findings move to diff-to-fix/findings/FIXED/ (existing convention).
- Update all docs, counts, CROSS-REF-MATRIX, SUMMARY, modules/*.md properly.
- Two repos only: heicfast/core_recovery (fix) + heicfast/easeus-drw-research
  (ground truth, read-only, corpus 03ad46c). No other repos.
- Re-implement from understanding; never paste decompiled code; cite corpus
  artifacts in commits. Safety hardenings preserved. No stubs/mocks/TODOs.
- Pull frequently; never force-push; keep main green; never commit the token.

Baseline at session start: core 716d121 (DFX16-M013 rebase reconciliation
worklog commit; N-062 was the last fix landing). Corpus 03ad46c unmoved.
