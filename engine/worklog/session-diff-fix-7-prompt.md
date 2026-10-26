# Session diff-fix-7 prompt (token redacted)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding against the research corpus and the running code, fix it in
the codebase, prove the fix with tests and benchmarks, and mark it fixed in the
audit trail with evidence. A false finding is corrected in the audit trail
rather than "fixed" by breaking good code.

Two repositories in scope, only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED — never in any file, env only]

Rules (condensed from full SOP; full text in this file's history):
1. Read before touching: diff-to-fix/ in full; corpus artifacts cited by
   findings, in full; core repo at current main.
2. Verify -> Reproduce -> Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) -> Fix -> Test -> Document
   -> Push. Resolution section appended to every finding touched; finding files
   never deleted; fixed findings moved to findings/FIXED/ with RESOLVED-INDEX.
3. One finding, one verified fix, one push. Pull frequently; never force-push;
   keep main green. Token never committed or logged.
4. Corpus wins over audit text. Re-implement from understanding — never paste
   decompiled code or proprietary strings. Safety hardenings preserved.
5. Every fix paired with tests that fail before and pass after. No stubs, no
   mocks, no TODOs, no silent leave-behinds.
6. CI green on Windows and Linux; Win7-inclusive import gate intact; ASan/
   UBSan/TSan clean on touched code; signing stays disabled.

This session's user-specified ordering: **low severity first** — INFORMATIONAL,
then MINOR, then MAJOR, then SEVERE, then TRACKED-OPEN — while keeping the
"no finding left unresolved" mandate of the closing session.

Deliverables: all findings resolved (FIXED / FIXED-ALREADY / INVALID /
SUPERSEDED / ACKNOWLEDGED); CROSS-REF-MATRIX fully MATCH; SUMMARY counts
current; modules/*.md verdict tables current; worklog.md handoff; tagged
release if clean; diff-to-fix/ is the final evidence set.
