# Session 24 prompt (re-stamped from 23 — DFX23 was claimed by a parallel
# lane in the same window; first-landing-wins; saved token-redacted)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/` — for every
finding (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN)
independently verify against the research corpus and the running code
before any change; fix confirmed divergences with fail-before/pass-after
tests; correct false findings in the audit trail rather than breaking
good code. Repos in scope (only these two):

- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged.

Session emphasis (user instruction): fix all audit findings — minor
first, one by one, with full context and knowledge; move fixed findings
into `findings/FIXED/`; update all docs and counts properly; target at
least 20+ fixes before session end.

Lane note at bootstrap: the parallel DFX23 lane claimed the full open
MINOR tier at 354c555. To keep lanes disjoint (the repo's standing
convention), this lane (DFX24) claims the open MAJOR tier + tracked-open
push-forward. The user mandate is served either way — the audit closes.

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

Session-24 lane facts recorded at bootstrap:

- Session ID: DFX24 (DFX23 claimed by the parallel MINOR lane).
- Claim tree: 354c555 (= 716d121 content); baseline linux-release
  **737/737** green, first-hand.
- Open inventory at bootstrap: MINOR 19 (DFX23's queue) · MAJOR 14
  (this lane) · INFO 24 · tracked-open 11 · FIXED/ 149.
- Corpus commit: 03ad46c (verify unmoved at first use).
