# Session-47 diff-to-fix prompt (token redacted)

## IM Chat Context (gateway metadata)
- session_id: web-07f0814c-6a17-45cd-97f8-39a458cf3454
- chat_id: 74640b56-b1af-48ff-924d-5ca57025b36e
- channel: zai-web
- trace_id: 1a1849df16488d11

## Mandate (abridged, token redacted)

Senior systems engineer mandate (25+ years Windows storage internals, C++20 production
engineering, parity remediation against reverse-engineered references): execute the
roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR, MINOR,
INFORMATIONAL, and TRACKED-OPEN — independently verify the finding, fix it in the
codebase, prove the fix with tests and benchmarks, and mark it fixed in the audit trail
with evidence. The audit is a hypothesis; every finding re-verified against the research
corpus (github.com/heicfast/easeus-drw-research, read-only, the only source of truth for
reference behavior) and the running code before any change. A false finding is corrected
in the audit trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

Workflow: Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review → Test →
Benchmark → Document → Push → Loop. One finding, one verified fix, one push.
Pull frequently, never force-push, leave main green. Re-implement from understanding;
never paste decompiled code; cite the corpus artifact in commit messages. Every fix
paired with a test that fails before and passes after (unit / integration / structure-
aware fuzz / TSan / ASan-UBSan as applicable). Safety hardenings preserved (destination
overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent, 4Kn, cancellable
IRP, SDDL gating). No stubs/mocks/fallbacks that mask failure. The audit trail is the
deliverable: finding files get Resolution sections (never deleted), moved to
findings/FIXED when closed; CROSS-REF-MATRIX / modules/*.md / SUMMARY.md / FIXED-LEDGER
updated per fix. Windows CI MSVC /W4 /WX + Win7-inclusive import allowlist gate stays
green; Linux CI green; ASan/UBSan/TSan clean on touched code.

Session-specific directive (the user's closing line): **fix all the audit findings
priority-to-MINOR-first, one by one, with full context and knowledge; move fixed
findings into the fixed directory; update all docs and counts properly; aim for at
least 20+ fixes this session.** This lane continues the S46 territory claim verbatim
(the unclaimed MINOR set; the driver-repo pair N-107/N-127 stays tracked-OQ per the
S42 liveness verdict).

Audited pin: core `fa654673` (the S46 claim commit; no fixes landed between the claim
and this lane), corpus `be0cfe9`.
