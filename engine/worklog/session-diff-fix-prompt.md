# Session prompt — diff-to-fix closing lane (token REDACTED)

Saved at session start per mandate. The GitHub token referenced in the original
prompt is intentionally redacted and is never committed; it lives only in git
remote config / env. Two repos in scope only:
- Core (modified): https://github.com/heicfast/core_recovery  — HEAD at session start: 98a5b139
- Corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research — HEAD: be0cfe9

Mandate summary (full text in the user prompt; this is the operative digest):
- Execute the roadmap in core_recovery/diff-to-fix/. For every finding
  (ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN): independently verify,
  fix in codebase, prove with tests, mark fixed in audit trail with evidence.
- User override this session: fix findings with priority to INFORMATIONAL first,
  then the severity ladder. Fixed findings move to the fixed/ directory
  (preserved, not deleted). Update all docs and counts. Target 20+ fixes.
- Verify -> Reproduce -> Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
  FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) -> Fix -> Test -> Document -> Push.
- One finding, one verified fix, one push. Pull frequently, never force-push.
- Corpus is the only source of truth for reference behavior.
- No shortcuts, no stubs, no mocks, no TODOs, no hallucinated parity.
