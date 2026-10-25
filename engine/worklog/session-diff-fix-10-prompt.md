# Session 10 — diff-to-fix execution prompt (token redacted)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every
finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix
with tests and benchmarks, and mark it fixed in the audit trail with
evidence. The audit is a hypothesis; this session is the verifier.

Repositories in scope (only these two):
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git-credential only, never committed, never logged.
  [REDACTED FROM THIS COPY]

Core SOPs (verbatim from the session prompt, condensed):
- Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark
  -> Document -> Push -> Loop.
- Re-implement from understanding; never paste decompiled code; cite the
  corpus artifact in the commit message.
- Fix the cause, not the symptom. Match architecture and wiring.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable IRP,
  SDDL gating) are preserved.
- No shortcuts, no fallbacks that mask failure, no stub/mock/TODO.
- One finding, one verified fix, one push, one finding-file update.
- Pull frequently; never force-push; leave main green.
- Windows direct I/O: reads multiple of 4 KB, prefer multiples of 64 KB.
- The corpus is the only source of truth; when a finding's evidence
  conflicts with the corpus, the corpus wins.
- No finding file deleted — the audit trail is the deliverable.
- Fixed findings move to findings/FIXED/ with a Resolution section.

Fix order this session (user directive): highest-impact first — the 3
remaining SEVERE (S-009, S-029, S-030), then the MAJOR tier in impact
order, then MINOR. Target: 20+ verified fixes before session end.
Session 10 baseline: core 8e79960, corpus 03ad46c, suite 533/533 green.
