# Session 40 — the diff-fix closing mandate (token redacted)

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged. [REDACTED
  from the saved copy per the mandate's own token-hygiene rule — the live
  credential lives only in the session env and the git remote config, which
  .gitignore-style hygiene keeps out of every commit; rotate after session.]

Mandate summary (full text in the live session): execute the roadmap in
`core_recovery/diff-to-fix/`. For every finding — SEVERE, MAJOR, MINOR,
INFORMATIONAL, and TRACKED-OPEN — independently verify the finding against the
corpus and the running code, fix it, prove the fix with tests, and mark it fixed
in the audit trail with evidence. False findings are corrected in the audit
trail rather than "fixed" by breaking good code.

User's session-40 line: "Fix all the audit findings (priority to tracked open
first) one by one ... update and put fixed findings in fixed directory. Update
all docs, counts and all properly. Aim for at least 20+ fixes in this session."

SOP: Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark ->
Document -> Push -> Loop. One finding, one verified fix, one push. Pull often,
never force-push, leave main green. No shortcuts, no fallbacks that mask
failure, no stubs, no mocks. Production grade only.

Baselines recorded at session open (T1):
- core_recovery: 461825d8 (main)
- corpus: be0cfe9 (unchanged since session 8 — matches every active finding's cite base)
- Suite: 1052 tests / 1010 passed / 42 Win-only skipped / 0 failed (linux-release, g++ 14.2)
- clang-format 18.1.8 (the CI pin)
- Board at open: 49 active (6 SEVERE · 20 MAJOR · 18 MINOR · 5 INFO) + 3 tracked-open
  (OQ-S4-3, OQ-S10-1, OQ-S10-3) per the filesystem recount below.
