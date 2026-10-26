# Session 31 prompt (DFX31 — saved token-redacted; re-stamped 25→30→31
# through the same-window ID churn; DFX30 owns the tracked-open TAIL,
# this lane takes the disjoint tracked-open HEAD)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/` — for every
finding (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN)
independently verify against the research corpus and the running code
before any change; fix confirmed divergences with fail-before/pass-after
tests; correct false findings in the audit trail rather than breaking
good code. Repos in scope (only these two):

- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only):
  https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged.

Session emphasis (user instruction): fix all audit findings one by one,
with full context and knowledge; move fixed findings into
`findings/FIXED/`; update all docs and counts properly; target at least
20+ fixes before session end.

Lane note at bootstrap: parallel lanes claimed the open finding tiers — DFX23 the MINOR queue (354c555), DFX24 the
MAJOR queue + tracked-open-if-tier-drains (02b950f). To keep lanes
disjoint (the repo's standing convention, first-landing-wins), this lane
(DFX31) claims the TRACKED-OPEN HEAD set (disjoint from the parallel
DFX30 tail claim at 29a8932): OQ-RS (RS-1..RS-8), OQ-S2M-2, OQ-S3-JPEG,
OQ-S4-3, OQ-S20-1. First-landing-wins per OQ; pull before each
resolution. The user mandate is served either way — the audit closes.

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
7. No shortcuts, no fallbacks that mask failure, no stubs. Production
   grade only.
8. One finding, one verified fix, one push. Pull frequently, never
   force-push, leave main green.
9. Track every resolution in the audit trail (finding file Resolution
   sections, CROSS-REF-MATRIX, SUMMARY, modules/*.md, worklog.md).
