# Session-56 diff-to-fix prompt (token redacted; seat renumbered S49→S56 across three hot-window rebases — the remote's S49/S50/S51 and S52..S55 seats pushed first)

## IM Chat Context (gateway metadata)
- session_id: web-6b58f680-59f5-4e0a-873d-280bf6147365
- chat_id: be84f4eb-db13-448e-ba85-e575cc9e8872
- channel: zai-web
- trace_id: 1a1849eeaa576111

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

Fix order: THE USER'S SESSION DIRECTIVE — MINOR FIRST ("Fix all the audit
findings(priority to minor first) one by one, with full context and knowledge and
update and put fixed findings in fixed directory. Update all docs, counts and all
properly. Aim for atleast 20+ fixes in this session"), then the remaining tiers
in the SOP order with the live territory division respected (the parallel S45
lane holds ACUTE/SEVERE per its saved mandate; the S46 lane holds the MINOR set
top-down; the S47/S48 lanes hold INFO + tracked-open + MAJOR — THIS LANE SWEEPS
THE MINOR BOARD BOTTOM-UP so the two MINOR lanes converge in the middle; every
push pulls first and skips anything the sibling lanes already landed, per
earlier-push-wins). After the MINOR board converges, this lane proceeds to any
still-open tier per SOP order under the same earlier-push-wins rule.
Performance fixes measured, not guessed. Windows CI green (MSVC /W4 /WX,
Win7-inclusive import gate), Linux verification green (the local
linux-release suite; ASan/UBSan/TSan, fuzz smoke where the change touches
memory/threading). No shortcuts, no samples, no assumptions, no hallucination,
production grade only.
