# Session prompt — diff-to-fix closing session (token redacted)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding against the research corpus (easeus-drw-research @ 03ad46c,
read-only, the only source of truth), fix it in the codebase, prove the fix with
tests and benchmarks, and mark it fixed in the audit trail with evidence. The
audit is a hypothesis; this session is the verifier. False findings are corrected
in the audit trail rather than "fixed" by breaking good code.

Repos: core https://github.com/heicfast/core_recovery (modified),
corpus https://github.com/heicfast/easeus-drw-research (read-only).
Token: REDACTED (env only, never committed).

SOP: Verify -> Reproduce -> Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) -> Fix -> Re-review -> Test ->
Benchmark -> Document -> Push -> Loop. One finding, one verified fix, one push.
Fixed findings move to findings/FIXED/ with a Resolution section. Audit trail
files (CROSS-REF-MATRIX, SUMMARY, modules/*.md, finding files) updated per fix.
Safety hardenings preserved. Win7-inclusive gate intact. No stubs, no mocks,
no fallbacks that mask failure. Every fix paired with fail-before/pass-after tests.

Fix order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL (ACKNOWLEDGED) ->
TRACKED-OPEN. Performance fixes measured, not guessed.
