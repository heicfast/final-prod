# Session 38 — diff-to-fix closing session (minor-first directive)

Saved at session start per SOP (§ Final reminder). Token redacted.

## Mandate excerpt

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with tests
and benchmarks, and mark it fixed in the audit trail with evidence. The audit is a
hypothesis; the verifier is this session. A false finding is corrected in the audit
trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — env/git config only, never committed]

## Session-specific directives (from the user's closing line)

- "Fix all the audit findings (priority to minor first) one by one, with full
  context and knowledge and update and put fixed findings in fixed directory."
  → Work order: MINOR tier first (35 open), then INFORMATIONAL acknowledgements,
  then SEVERE/MAJOR, then TRACKED-OPEN. No ACUTE open (all A-* live in findings/FIXED/).
  → "fixed directory" = the existing `diff-to-fix/findings/FIXED/` convention:
  resolved findings move there (nothing deleted; RESOLVED-INDEX.md + gate docs updated).
- "Update all docs, counts and all properly."

## Session start state (T1 record)

- core_recovery HEAD: 84a18323bf1e680804c4c1d10130bd1d9a003d9f (main, up to date with origin)
- corpus (easeus-drw-research) commit: recorded in worklog entry for T1 (read-only clone)
- Board at session open (per SUMMARY/README session-9 close): 76 active
  (7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFORMATIONAL) + 2 tracked-open
  (OQ-S4-3, OQ-S8-1) + 250 FIXED.
- Last audited product-code commit per README: 35adcb9; code has since moved
  (9e5dedc M-081 fix, b92ebef school-1 formats, dd225da, 095a329, 9cb098c, and
  trail commits through 84a1832). Every finding is re-verified at the current
  tree before classification — no finding is fixed against stale line numbers.

## Full SOP

The full SOP text (sections 1-13 + final reminder) is the one used by all prior
fixing sessions (see worklog/session-diff-fix-prompt.md lineage). Core rules:
verify → reproduce → classify → fix → re-review → test → benchmark → document →
push → loop; one finding, one verified fix, one push (MINORs may batch per module);
pull frequently, never force-push, leave main green; token never committed or logged;
no shortcuts, no fallbacks that mask failure, no stubs; every fix paired with a
failing-then-passing test; audit trail files are the deliverable.
