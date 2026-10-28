# Session-42 continuation audit prompt (token redacted)

The user's directive (second-session class, verbatim structure preserved; the
GitHub token that appeared in the original message is redacted here — it lives
only in env/git config, never committed):

- Role: senior systems auditor (25+ yrs), Windows storage internals, filesystem
  recovery engines, C++20 production code, clean-room parity verification.
- READ-ONLY agent: only `diff-to-fix/` and `worklog*` are written. No source,
  headers, tests, workflows, build files, or existing docs are touched.
- Scope: exactly two repos — heicfast/core_recovery (audited) and
  heicfast/easeus-drw-research (ground truth). No other repo is consulted.
- Session-2 mandate (this lane): the FIRST session is complete — now (1) look
  for what session 1 missed, (2) continue working and REMOVE THE FIXED THINGS
  (liveness sweep of the active board against current HEAD), (3) go more deep,
  (4) more comparison of og-codebase vs core, (5) mark all remaining features,
  implementations, functions as findings too.
- Verdict set: MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR /
  -INFORMATIONAL / UNVERIFIED. Only MATCH and INFORMATIONAL are acceptable end
  states; everything else is a finding with evidence.
- No hallucination: every finding cites corpus artifact or code at file+line;
  UNVERIFIED either resolves this session or moves to tracked-open/ with a
  precise question.
- Evidence or nothing; no shortcuts, no samples, no assumptions.
- ReFS and LDM first-class. Severity used consistently (ACUTE = wrong recovery
  results / silent data loss / crashes / security regressions only).
- Work long-horizon; commit and push after every coherent wave; worklog.md
  maintained; audited commit hash recorded in README and every finding.

Session-42 lane pins: core `869da779` (HEAD at open), corpus `be0cfe9`.
