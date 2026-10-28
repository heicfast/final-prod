# Session-47 diff-to-fix prompt (token redacted)

## IM Chat Context (gateway metadata)
- session_id: web-1c17abfb-0a46-4ece-adf0-81106bc2c247
- chat_id: 5a0ffd69-fdce-4dd6-9632-0cb60bf9a183
- channel: zai-web
- trace_id: 1a1849ab13a4aa25

## Mandate (abridged, token redacted)

Senior systems engineer mandate (25+ years, Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered references):
execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE,
SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the
finding, fix it in the codebase, prove the fix with tests and benchmarks, and mark
it fixed in the audit trail with evidence. The audit is a hypothesis; every finding
re-verified against the research corpus and the running code before any change.
A false finding is corrected in the audit trail rather than "fixed" by breaking
good code.

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
Safety hardenings preserved (destination-overlap refusal, BitLocker
legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable IRP, SDDL
gating). ReFS and LDM stay first-class. No stubs/mocks/fallbacks that mask
failure. The audit trail is the deliverable: finding files get Resolution
sections, CROSS-REF-MATRIX cells go to MATCH, SUMMARY/module docs/ledger/worklog
updated. Resolved findings move to findings/FIXED/ per house convention.

Fix order: THE USER'S SESSION DIRECTIVE — INFORMATIONAL FIRST ("Fix all the audit
findings(priority to informational first) one by one... Aim for atleast 20+ fixes
in this session"), then the remaining tiers in the SOP order with the live
territory division respected (the parallel S45 lane holds ACUTE/SEVERE per its
saved mandate; the S46 lane holds the MINOR set; this lane takes the unclaimed
INFORMATIONAL board, the tracked-open set, and the MAJOR tier under
earlier-push-wins). Performance fixes measured, not guessed. Windows CI green
(MSVC /W4 /WX, Win7-inclusive import gate), Linux CI green (ASan/UBSan/TSan,
fuzz smoke). No shortcuts, no samples, no assumptions, no hallucination,
production grade only.
