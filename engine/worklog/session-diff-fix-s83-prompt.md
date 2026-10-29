# Session-83 fix-lane prompt (token redacted)

Role: senior systems engineer (25+ years), Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered
references. Mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and
TRACKED-OPEN — independently verify the finding, fix it in the codebase,
prove the fix with tests and benchmarks, and mark it fixed in the audit
trail with evidence. The audit is a hypothesis; you are the verifier. A
false finding is corrected in the audit trail rather than "fixed" by
breaking good code.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged (REDACTED).

## Classification loop per finding

Reproduce the evidence → re-verify against the corpus (read the cited
artifact IN FULL; when the cite is a Ghidra decompile line number, note
that the decompile is CI-generated and not committed — disassemble the
corpus binary itself; the DLL bytes are the arbitrator) → classify:
CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE (INVALID) /
OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING → only then fix. Every fix
paired with a test that fails before and passes after. No stub, no mock,
no TODO, no placeholder, no fallback that masks failure. One finding, one
verified fix, one push. Pull frequently; never force-push; leave main
green. When a racing lane lands the same surface mid-session, rebase and
re-verify rather than seesaw; when the racing lane's rationale is
byte-exact refuted, land the correction with the evidence.

## Fix order

ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (mark ACKNOWLEDGED) →
TRACKED-OPEN (resolve or push forward precisely). Largest user impact
first: correctness before performance, data integrity before latency,
silent-failure paths (real files that never carve!) before observable
inefficiencies.

## Audit-trail updates per fix

Finding file gains a Resolution section; CROSS-REF-MATRIX cell → MATCH;
modules/<module>.md verdict row; SUMMARY counts; worklog.md. Findings are
never deleted. Discovered divergences get new finding files (never silent
fixes). Discovered REFUTATIONS of prior landings' rationale get recorded
in the trail with the byte-exact evidence. CI green both platforms;
sanitizers on touched code; the Win7-inclusive import gate intact;
signing stays disabled.

## Session goal (user's directive)

Fix ALL the audit findings (priority to major first) one by one, with full
context and knowledge; put fixed findings in the fixed directory; update
all docs and counts properly; aim for at least 20+ fixes in this session;
do not stop before that. Good luck — the user believes in you.
