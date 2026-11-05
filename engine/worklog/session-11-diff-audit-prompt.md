# Session-11 diff-audit prompt (saved per the SOP; token redacted)

User frame: "This is second session of this work, first is already completed, now you
have go look for other things which session 1 missed and continue working and remove
the fixed things and go more deep, do more comparison on og-codebase and our core and
mark all remaining features, implementations, functions, as finding too."

Full SOP (verbatim from the user, token redacted; the full original prompt is the
session-diff-audit-prompt.md SOP — read twice at session open):

- READ-ONLY agent. Only diff-to-fix/ and worklog.md are written. No source, headers,
  tests, workflows, build files, or existing docs are touched. Findings are documented,
  never fixed.
- Mandate: audit heicfast/core_recovery against heicfast/easeus-drw-research across
  every module, engine, function, feature, API, and 50+ performance/behavioral
  parameters. Document every difference (ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL) with
  in-depth evidence, reproduction, and an actionable fix plan.
- Verdict set: MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR / -INFORMATIONAL /
  UNVERIFIED. Only MATCH and INFORMATIONAL are acceptable end states.
- Verification protocol: executed comparison > decompiled evidence > corpus doc > RE
  probe. No claim without evidence. False findings cost more than honest UNVERIFIEDs.
- Repos in scope (only two): github.com/heicfast/core_recovery (audited, read-only) and
  github.com/heicfast/easeus-drw-research (ground truth). Token from env/git config
  only ([REDACTED]), never committed.
- Output: diff-to-fix/ per the established structure (README, SUMMARY, SEVERITY-SCALE,
  PARAMETERS, CROSS-REF-MATRIX, findings/{ACUTE,SEVERE,MAJOR,MINOR,INFORMATIONAL},
  modules/, evidence/, tracked-open/).
- Session-11-specific asks (the user's continuation mandate):
  1. REMOVE THE FIXED — verify recent fix landings first-hand and retire their
     findings per the purge convention (ledger row + git rm; never re-create FIXED/).
  2. GO MORE DEEP — comparison depth beyond the prior sessions' coverage; the
     function-level completeness program (S10-F) extended to the trees it named next.
  3. MARK ALL REMAINING — every remaining feature/implementation/function divergence
     gets a finding ID.
- Rules: no shortcuts, no samples, no assumptions, no hallucination; severity used
  consistently (ACUTE only for wrong recovery/silent loss/crash/security); ReFS and
  LDM first-class; the output is the roadmap; work long-horizon, commit+push often;
  audited commit hash cited everywhere; pull frequently and reconcile with parallel
  lanes (the earlier remote numbering wins ID collisions — SA-9 rule).
