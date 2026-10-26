# Session 4 — diff-audit continuation prompt (token redacted)

Mandate (user, verbatim structure):
- Senior systems auditor, READ-ONLY. Only writes: `diff-to-fix/` + `worklog.md` (+ `worklog/` prompt files).
- Audit `heicfast/core_recovery` against `heicfast/easeus-drw-research` across every module,
  engine, function, feature, API, and 50+ parameters.
- This session: find what prior sessions missed, continue working, remove the FIXED things,
  go deeper, more og-codebase vs core comparison, mark ALL remaining features/implementations/
  functions as findings.
- Repos in scope (only two): github.com/heicfast/core_recovery (audited), github.com/heicfast/
  easeus-drw-research (ground truth). Token: ghp_[REDACTED] (env/git config only, never committed).
- Verdict set: MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR / -INFORMATIONAL / UNVERIFIED.
  Only MATCH and INFORMATIONAL are acceptable end states; everything else is a finding.
- Evidence bar: executed comparison > decompiled evidence > corpus doc > independent RE probe.
  No hallucination; false positives cost more than honest UNVERIFIEDs.
- Finding template + output tree as per diff-to-fix/ structure already established (README.md,
  SUMMARY.md, SEVERITY-SCALE.md, PARAMETERS.md, CROSS-REF-MATRIX.md, findings/{ACUTE,SEVERE,
  MAJOR,MINOR,INFORMATIONAL,FIXED}, modules/, evidence/{decompiled-quotes,executions},
  tracked-open/).
- Session lanes selected after state triage (see worklog bootstrap entry): (1) never-audited
  core modules (preview, query, win-layer I/O), (2) the API-session lane that was bootstrapped
  but never executed (cr_ui C ABI surface, driver utilization, measured performance),
  (3) remaining feature-level sweep vs og-codebase, (4) tracked-open OQ resolution,
  (5) FIXED/ verification hygiene.
- Commit and push after every coherent wave. Record audited commit hash everywhere.

Read twice before starting. T1 begins with clone/pull + hash recording.
