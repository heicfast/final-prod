# Session 22 prompt (lane re-stamped from 19: IDs 19-21 were squatted by parallel legs in the same window) (saved at session start; token redacted)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/` — for every
finding (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN)
independently verify against the research corpus and the running code
before any change; fix confirmed divergences with fail-before/pass-after
tests; correct false findings in the audit trail rather than breaking
good code. Repos in scope (only these two):

- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged.

Session-22 emphasis (user instruction): **fix all minor findings first**,
one by one, with full context and knowledge; move fixed findings into
`findings/FIXED/`; update all docs and counts properly; target at least
20+ fixes before session end.

Key SOPs (unchanged from all prior sessions):

1. Read before touching: audit output, corpus citations (full artifact,
   not the excerpt), current main (pull — other agents may have pushed).
2. Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review →
   Test → Benchmark → Document → Push → Loop.
3. Re-implement from understanding; never paste decompiled code; cite the
   corpus artifact in the commit message.
4. Fix the cause, not the symptom; match architecture and wiring.
5. Every fix paired with a test that fails before and passes after.
6. Safety hardenings (destination overlap refusal, BitLocker
   legitimate-unlock-only, RawFixer explicit consent, 4Kn handling,
   cancellable IRP, SDDL gating) are preserved when touched.
7. No shortcuts, no fallbacks that mask failure, no stubs/mocks/TODOs in
   production paths. Production grade only.
8. One finding, one verified fix, one push, one finding-file update.
9. Pull frequently; never force-push; leave main green.
10. The audit trail (diff-to-fix/) is the deliverable; finding files are
    never deleted — Resolutions appended, files moved to findings/FIXED/.

Session-22 lane facts recorded at bootstrap:

- Session ID: DFX22 (IDs DFX17-21 squatted by parallel legs) (parallel lanes DFX17/DFX18 landed M-045, M-054
  NEW-FINDING, M-007, D-70/D-71 in the same window — pulled before any
  work; next free MAJOR id M-055, next free MINOR id N-077).
- Baseline after pull: linux-release suite green (see worklog.md entry).
- GitHub Actions runs observed stuck `in_progress` (runner capacity);
  Windows-lane proof channel for this session = Linux-side stub-header
  syntax gate + logic review, CI observed post-push where it executes.
