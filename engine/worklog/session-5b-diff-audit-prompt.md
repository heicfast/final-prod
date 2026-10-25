# Session-5 diff-audit prompt (token redacted)

Saved: 2026-10-26. Continuation leg. Original prompt preserved verbatim except
the GitHub token line, redacted below.

> GitHub token (env/git config only, never committed): [REDACTED]

Mission for this session (user's continuation instruction):
- This is a continuous session of this work; multiple agents are already working
  and completed. Now look for other things which earlier sessions missed and
  continue working and update the fixed things and go more deep, do more
  comparison on og-codebase and our core and mark all remaining features,
  implementations, functions, as findings too. Focus on every minor detailing
  too.

Standing SOPs from the original prompt (unchanged, re-read twice):
- READ-ONLY agent: only diff-to-fix/ and worklog.md are written. No source
  changes, no fixes applied.
- Evidence or nothing; no hallucination; no shortcuts, no samples, no
  assumptions; production grade only.
- Verdict set: MATCH / DIVERGENCE-ACUTE / -SEVERE / -MAJOR / -MINOR /
  -INFORMATIONAL / UNVERIFIED. Only MATCH and INFORMATIONAL are acceptable end
  states; every DIVERGENCE-* and UNVERIFIED is a finding.
- 70-parameter taxonomy (diff-to-fix/PARAMETERS.md); every module scored
  against every applicable parameter.
- Finding template as in diff-to-fix/findings/ (Severity/Module/Parameters/
  Corpus refs/Core refs/Summary/What the reference does/What our core does/
  Gap/Impact/Reproduction/Fix plan/Tests to add/Related findings/Open
  questions).
- Verification ladder: executed comparison > decompiled evidence > corpus doc
  > independent RE probe; else UNVERIFIED → tracked-open with a precise
  question.
- ReFS and LDM are first-class.
- Commit and push after every coherent wave; each finding commit includes its
  ID and one-line title; record the audited commit hash in README.md and every
  finding's Core references.
