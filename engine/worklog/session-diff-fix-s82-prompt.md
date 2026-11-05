# Session 82 — the S82 fix-lane prompt (the user's mandate, token redacted)

Saved at the lane's open (core 04fb5e88 on the RED tree, re-based at
d37840c4, corpus be0cfe9). The GitHub token the user supplied is
deliberately NOT recorded here (env/git-config only, never committed,
never logged).

## The mandate (structure)

- Senior systems engineer (25+ years), Windows storage internals, C++20
  production engineering, parity remediation against reverse-engineered
  references.
- Execute the roadmap in core_recovery/diff-to-fix/: for every finding
  (ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN) — independently
  verify, fix, prove with tests and benchmarks, mark fixed with
  evidence. Do not trust the audit blindly; re-verify every finding
  against the research corpus and the running code before any change.
  A false finding is corrected in the trail, not "fixed" by breaking
  good code.
- Two repositories in scope and only two: core_recovery (modified) and
  easeus-drw-research (read-only corpus). Token: env/git-config only.
- Read before touching: the full diff-to-fix/ tree, the corpus
  line-by-line where required, the core repo in full.
- Verify -> Reproduce -> Classify (CONFIRMED-DIVERGENCE /
  ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) ->
  only then fix. Resolution sections in every finding touched.
- Re-implement from understanding; never paste decompiled code; cite
  the corpus. Fix the cause. Match architecture. Every fix paired with
  a test that fails before and passes after. Preserve safety
  hardenings. No stubs/mocks/TODOs/shortcuts. No hallucinated parity.
  ReFS/LDM first-class. Pull frequently, never force-push, keep main
  green. Token hygiene. One finding, one verified fix, one push.
- Order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL ->
  TRACKED-OPEN (the user's final line: "Fix all the audit findings
  [priority to major first] one by one ... at least 20+ fixes ... put
  fixed findings in fixed directory. Update all docs, counts and all
  properly.")
- Performance findings: baseline -> fix -> re-benchmark -> accept or
  revert; Windows direct I/O reads multiple of 4 KiB and 64 KiB where
  possible.
- Update CROSS-REF-MATRIX (MATCH cells), modules/*.md verdicts,
  SUMMARY counts, worklog with SHAs; no finding file deleted; the
  audit trail is the deliverable.
