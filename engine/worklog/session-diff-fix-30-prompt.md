<<<<<<< HEAD
# Session DFX25 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus)

Saved at session start, token redacted. Date: 2026-10-26.

## Mandate (abridged, token redacted)

Senior systems engineer role. Execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it, prove the fix with tests, and mark it
in the audit trail with evidence.

Two repos in scope only:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

User directive for this session: focus on INFORMATIONAL and tracked-open
initially, then continue one by one. Target at least 20+ fixes before session
end. Put fixed findings in the fixed directory. Update all docs, counts.

## Core SOPs

- Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark → Document → Push → Loop.
- One finding, one verified fix, one push.
- No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs.
- Every fix paired with a test that fails before, passes after.
- Safety hardenings preserved (dest overlap refusal, BitLocker legit-unlock-only,
  RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- Pull frequently, never force-push, keep main green.
- INFORMATIONAL: verify deliberate-difference rationale, mark ACKNOWLEDGED, no
  code change unless rationale wrong.
- Track open items: resolve or push forward with precise next step + evidence.

## Session start state

- Core HEAD: 716d121 (worklog(DFX16-M013): rebase reconciliation)
- Corpus HEAD: 03ad46c (unmoved throughout history)
- Open: 0 ACUTE, 0 SEVERE, 14 MAJOR, 19 MINOR, 22 INFORMATIONAL, 11 tracked-open
- FIXED: 149
=======
# Session 30 prompt (saved token-redacted at bootstrap; re-stamped 25→30 past same-window squats)

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

Lane note at bootstrap: two parallel lanes are claimed in the same
window — DFX-front claimed the full open MINOR tier (354c555, front-first
order: N-009, N-007/I-004, N-025, N-026, N-027, N-031, N-036, N-049,
N-059, N-065, N-066, N-075, N-019, N-050, N-033, N-072, N-032, N-071,
N-064) and DFX-major claimed the open MAJOR tier (02b950f). This lane
(DFX30) claims the DISJOINT TAIL of the MINOR queue, worked back-to-front
to maximize distance from DFX-front's front-first cursor: N-064, N-071,
N-032, N-072, N-033, N-019, N-050. First-landing-wins on any overlap;
pull before every fix; union-merge if a parallel landing arrives mid-fix.
After the tail slice drains, pull and absorb whatever MINORs remain
unclaimed (DFX-front front cursor may have advanced or stalled).

Key SOPs (unchanged from all prior sessions):

1. Read before touching: audit output, corpus citations (full artifact,
   not the excerpt), current main (pull — other agents may have pushed).
2. Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review →
   Test → Benchmark → Document → Push → Loop.
3. One finding, one verified fix, one push, one finding-file update.
4. Every fix paired with a test that fails before and passes after.
5. Safety hardenings preserved (destination overlap refusal, BitLocker
   legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable
   IRP, SDDL gating). ReFS and LDM stay first-class.
6. No shortcuts, no fallbacks that mask failure, no stubs, no mocks in
   production paths, no TODO left behind by a fix. Production grade only.
7. Win7-inclusive import gate intact; signing stays disabled; CI green
   on every push; ASan/UBSan/TSan on touched code; fuzz for parsers.
8. Audit trail is the deliverable: finding files keep their Resolutions,
   CROSS-REF-MATRIX cells go to MATCH, SUMMARY counts updated,
   modules/*.md verdicts updated, worklog.md chronological.
>>>>>>> 540811b (worklog(DFX30): session-25 bootstrap — disjoint MINOR-tail lane claim (back-to-front cursor), baseline 737/737 green)
