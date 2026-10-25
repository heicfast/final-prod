# Session prompt — diff-to-fix execution session 10 (2026-10-26)

Mandate: execute the roadmap in core_recovery/diff-to-fix/. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify
the finding against the research corpus and the running code, fix it, prove the
fix with tests, and mark it fixed in the audit trail with evidence. The corpus
(easeus-drw-research @ 03ad46c) is the only source of truth for reference
behavior. A false finding is corrected in the audit trail (INVALID), never
"fixed" by breaking good code.

Repos: heicfast/core_recovery (modified), heicfast/easeus-drw-research (read-only).
[Token redacted — env/git config only, never committed.]

SOP: Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark ->
Document -> Push -> Loop. One finding, one verified fix, one push. No shortcuts,
no stubs, no mocks, no fallbacks masking failure. Production grade only.
Fix order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL -> TRACKED-OPEN
(user note: prioritize major-impact findings; 20+ fixes this session).
