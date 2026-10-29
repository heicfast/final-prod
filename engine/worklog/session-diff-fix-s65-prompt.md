# Session S65 — diff-to-fix execution lane (the user's fix mandate, "priority to informational first", 20+ fixes)

Saved token-redacted per the SOP. (Lane renamed S64→S65 at the push-rebase:
the racing window's worklog open took the S64 seat first; both lanes ride the
same user mandate.)

## Scope

Two repos, only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token lives in env/git config only; never committed, never logged.

## Mandate

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently
verify the finding, fix it in the codebase, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. The audit is
a hypothesis; the corpus is the only source of truth. A false finding is
corrected in the trail rather than "fixed" by breaking good code.

## Classification per finding

CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE (INVALID) /
OUTDATED-EVIDENCE (SUPERSEDED) / NEW-FINDING. Resolution template per §2 of
the mandate. Fixed findings move to `diff-to-fix/findings/FIXED/`.

## Fixing SOPs (non-negotiable)

- Re-implement from understanding; never paste decompiled code; never copy
  proprietary strings; cite the corpus artifact in the commit message.
- Fix the cause, not the symptom. Match architecture and wiring.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- No stubs, no mocks, no TODOs, no fallbacks that mask failure.
- One finding, one verified fix, one push, one finding-file update.
- Pull frequently; never force-push; leave main green.
- Windows direct I/O: lengths multiple of 4 KiB, prefer multiples of 64 KiB.
- Win7-inclusive import allowlist gate intact; signing stays disabled.

## Deliverables

Every finding resolved (FIXED / FIXED-ALREADY / INVALID / SUPERSEDED /
ACKNOWLEDGED), the matrix fully MATCH, SUMMARY counts current, modules/*.md
verdicts current, worklog.md handoff, CI green, no finding file deleted,
fixed findings archived in findings/FIXED/.

## User's session directive

"Fix all the audit findings (priority to informational first) one by one,
with full context and knowledge and update and put fixed findings in fixed
directory. Update all docs, counts and all properly. Aim for at least 20+
fixes in this session."

## Lane opens at

Core `24e480c4` (main HEAD at open, 2026-10-29), corpus `be0cfe9`.
Board at open (re-counted first-hand): 0 ACUTE · 0 SEVERE · 20 MAJOR ·
13 MINOR · 0 INFO active / 151 FIXED / 4 tracked-open.

The informational-first pass: all 21 I- findings already ride
findings/FIXED/ — this lane re-verifies each ACKNOWLEDGED rationale as it
passes, then works the MINOR and MAJOR ladders (largest user impact first
within each tier), then the tracked-open items.
