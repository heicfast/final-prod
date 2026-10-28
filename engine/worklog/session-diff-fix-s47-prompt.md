# Session-47 diff-to-fix prompt (token redacted)

## Mandate (abridged, token redacted)

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with tests
and benchmarks, and mark it fixed in the audit trail with evidence. The audit is a
hypothesis; every finding re-verified against the research corpus and the running
code before any change. A false finding is corrected in the audit trail rather than
"fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

Workflow: Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review → Test →
Benchmark → Document → Push → Loop. One finding, one verified fix, one push.
Pull frequently, never force-push, leave main green. Re-implement from
understanding; never paste decompiled code; cite the corpus artifact in commit
messages. Every fix paired with a test that fails before and passes after.
Safety hardenings preserved. No stubs/mocks/fallbacks that mask failure.
The audit trail is the deliverable: finding files get Resolution sections,
CROSS-REF-MATRIX cells go to MATCH, SUMMARY/module docs/ledger/worklog updated.

User's session directive: **fix all the audit findings (priority to SEVERE first)
one by one, with full context and knowledge; update and put fixed findings in the
fixed directory; update all docs, counts and all properly. Aim for at least 20+
fixes this session.**

Fix order: A-014 (ACUTE) → S-055..S-061 → S-040/S-050 → MAJOR → MINOR → INFO →
tracked-open. Performance fixes measured, not guessed. Windows CI green
(MSVC /W4 /WX, Win7-inclusive import gate), Linux CI green (ASan/UBSan/TSan,
fuzz smoke). No shortcuts, no samples, no assumptions, no hallucination,
production grade only.
