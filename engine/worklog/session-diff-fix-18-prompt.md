# Session 18 prompt (saved at session start; token redacted)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/` — for every
finding (ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN)
independently verify against the research corpus and the running code
before any change; fix confirmed divergences with fail-before/pass-after
tests; correct false findings in the audit trail rather than breaking
good code. Repos in scope (only these two):

- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged.

Session-18 emphasis (user instruction): **fix all minor findings first**,
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
   legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL)
   are preserved; note preservation when touching near them.
7. No shortcuts, no fallbacks that mask failure, no stubs/mocks/TODOs.
8. One finding, one verified fix, one push. Pull frequently, never
   force-push, leave main green.
9. Audit trail updates per fix: finding Resolution, RESOLVED-INDEX row,
   CROSS-REF-MATRIX cell → MATCH, modules/<module>.md verdict,
   SUMMARY.md counts, worklog.md entry.
10. Discovered divergences are filed as new findings, never silently fixed.
11. Windows direct I/O: multiples of 4 KB, 64 KB where possible.
12. Win7-inclusive import gate intact; signing stays disabled.

Session-18 start state (recorded at bootstrap):
- Core HEAD: 11afbfa (session-14/15 lineage; corpus 03ad46c unmoved)
- Inventory: SEVERE 1 (S-029) · MAJOR 32 · MINOR 41 files · INFO 22 ·
  FIXED 95 files · tracked-open 16
