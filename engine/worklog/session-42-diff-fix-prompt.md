# session-42-diff-fix-prompt.md — the mandate for this fix session (token redacted)

Repositories (only two, in scope):
- Core (modified): heicfast/core_recovery
- Research corpus (ground truth, read-only): heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git-config only, never committed, never logged)

Mandate: execute the roadmap in core_recovery/diff-to-fix/. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify
the finding against the corpus and the running code, fix it in the codebase,
prove the fix with tests and benchmarks, and mark it fixed in the audit trail
with evidence. A false finding is corrected in the audit trail rather than
"fixed" by breaking good code.

Per-finding loop: Reproduce -> Re-verify vs corpus -> Classify
(CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE /
NEW-FINDING) -> Fix -> Test (fail-before/pass-after) -> Benchmark when
performance-tagged -> Document -> Push.

SOPs: re-implement from understanding (never paste decompiled code, never copy
proprietary strings); fix the cause not the symptom; match architecture and
wiring; every fix paired with a test that fails before and passes after;
preserve safety hardenings (destination overlap refusal, BitLocker
legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating);
no stubs/mocks/TODOs; no hallucinated parity; ReFS and LDM stay first-class;
coordinate ABI/CLI contract changes in the same commit; pull frequently, never
force-push, keep main green; one finding, one verified fix, one push.

Fix order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL (ACKNOWLEDGED)
-> TRACKED-OPEN. Performance fixes measured (baseline -> fix -> re-benchmark ->
accept/revert). Windows direct I/O reads: multiples of 4KB, 64KB where
possible. Test types: unit, integration on seeded fixtures, structure-aware
fuzz for parsers, TSan for threading, ASan/UBSan for memory, CLI contract for
grammar/exit codes, ABI consumer test under MSVC /W4 /WX.

Audit trail updates per fix: finding Resolution section; CROSS-REF-MATRIX cell
-> MATCH; modules/<m>.md verdict row; SUMMARY.md counts; README audited
commit; worklog.md. Discovered divergences get new finding files (never silent
fixes). CI gates: MSVC /W4 /WX, DLL smoke, CLI contract, seeded integration,
C consumer, fuzz smoke, Win7-inclusive import allowlist, ASan/UBSan/TSan on
Linux. Signing stays disabled. Docs updated in the same commit (CAPABILITY-
MATRIX, architecture, ground-truth-contracts, error-fix-registry, PARITY-
DELTAS); resolved TODOs/FIXMEs removed.

Session goal (user directive): all SEVERE findings first, then descend; at
least 20+ fixes this session; fixed findings moved to findings/FIXED/; all
docs/counts updated; release tagged if clean; worklog handoff with exact
commit and next steps.

The corpus is the only source of truth. The audit is a hypothesis; this
session is the verifier. No shortcuts, no samples, no assumptions, no
hallucination, production grade only.
