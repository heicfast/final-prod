# Session prompt — diff-to-fix closing session (token redacted)

Saved at session start per SOP. The GitHub token referenced in the original
prompt is redacted here (credential hygiene: env/git-credential-store only,
never committed, never logged).

## Mandate (verbatim summary)

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding against the corpus (`easeus-drw-research` @ `03ad46c`,
read-only ground truth) and the running code, fix it in the codebase, prove
the fix with tests and benchmarks, and mark it fixed in the audit trail with
evidence. The audit is a hypothesis; the corpus wins on any conflict. A false
finding is corrected in the audit trail (INVALID), not "fixed" by breaking
good code.

Repos in scope (only two):
- Core (modified): github.com/heicfast/core_recovery
- Corpus (read-only): github.com/heicfast/easeus-drw-research

## Operating loop (per finding)

Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix the cause → Test
(fail-before, pass-after) → Benchmark (perf-tagged only) → Document
(finding Resolution + matrix + module + SUMMARY + worklog) → Push → Loop.

## Non-negotiables

- Re-implement from understanding; never paste decompiled code; cite the
  corpus artifact in commit messages.
- Every fix paired with a failing-then-passing test. No hallucinated parity.
- Safety hardenings preserved (dest-overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable IRP,
  SDDL gating) — noted in Resolutions when touched.
- No stubs, no mocks, no TODOs, no silent leave-behinds. Unresolvable items
  stay TRACKED-OPEN with precise next steps.
- One finding, one verified fix, one push. Pull frequently; never force-push;
  leave main green.
- Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, seeded
  integration, C consumer, fuzz smoke, Win7 import allowlist) and Linux CI
  green (core, CLI, ASan/UBSan/TSan, fuzz smoke).
- Token never committed or logged.
- Audit trail preserved: finding files never deleted; resolutions appended.

## Fix order

ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (acknowledge) → TRACKED-OPEN.
Within tier: correctness before performance; data integrity before latency.

## Session-start state (recorded at T1)

- Core HEAD: `7c44bdc7fa94696c7e066e5d1c5e3f9e25f55e59` (main)
- Corpus HEAD: `03ad46ccd6086000e53d7d3871ef43cd7f32e462` (main, unmoved)
- Audit-baseline commit (README, session 5b leg): `7cf6754`-lineage rebased
  onto `c75f646`; sessions 6-8 landed many fixes since.
- Open inventory at session start: SEVERE 3 (S-009, S-029, S-030) · MAJOR 39
  (incl. M-053 filed+fixed in-session last time — active count net) · MINOR
  55 · INFO 22 (all ACKNOWLEDGED) · tracked-open 16 files (several RESOLVED
  in-place). FIXED/ = 72 files.
- Target: 20+ verified fixes this session, each with fail-before/pass-after
  tests, audit-trail updates, and pushes.
