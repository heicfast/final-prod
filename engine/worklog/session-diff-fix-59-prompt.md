# Session-59 diff-to-fix prompt (this lane rebranded from the 45 draft at the hot-window merge - the S45..S57 seats were taken by the parallel lanes) (token redacted)

## IM Chat Context (gateway metadata)
- session_id: web-e60092bc-782b-4b89-88f0-5c5db6b0ed07
- chat_id: d0f392f5-35ec-4f58-b3f8-00f04cd84924
- channel: zai-web

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

Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (ACKNOWLEDGED) →
TRACKED-OPEN. Performance fixes measured, not guessed. Windows CI green
(MSVC /W4 /WX, Win7-inclusive import gate), Linux CI green (ASan/UBSan/TSan,
fuzz smoke). Tag a release if the audit closes cleanly. Aim: 20+ fixes.
