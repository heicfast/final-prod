# Session prompt — the diff-to-fix execution mandate (token redacted)

**Session-60 S60 fix lane (2026-10-29).** Saved at session open (T1), token
redacted, read twice. Mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding against the corpus and the running code, fix it,
prove the fix with tests and benchmarks, and record the Resolution in the audit
trail with evidence.

Repos in scope (only these two):
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

SOPs: verify → reproduce → classify → fix → re-review → test → benchmark →
document → push → loop. One finding, one verified fix, one push. Pull frequently,
never force-push, leave main green. No shortcuts, no fallbacks masking failure,
no stubs, no mocks, no TODOs, no hallucinated parity. Every fix paired with a
test that fails before and passes after. Safety hardenings preserved
(destination-overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent,
4Kn handling, cancellable IRP, SDDL gating). ReFS/LDM first-class. Win7-inclusive
import gate intact. Signing stays disabled.

Priority this session per the user: tracked-open first, then the tier order.
Aim: 20+ fixes. Every finding ends FIXED / FIXED-ALREADY / INVALID / SUPERSEDED
(/ ACKNOWLEDGED for INFO). The audit trail is the deliverable.

Pins at open: core `98a5b139` (S55 audited-commit `9633d94f`, battery pin
`1fccb0a4`), corpus `be0cfe9`. Board at open: 49 active (1 SEVERE · 22 MAJOR ·
24 MINOR · 2 INFO) / 136 FIXED / 4 tracked-open.

---

# Session-60 diff-fix prompt (token redacted) — the MAJOR-first closing lane

Mandate: execute `diff-to-fix/` — for every finding, independently verify
(reproduce + re-read the corpus artifact), classify (CONFIRMED-DIVERGENCE /
FIXED-ALREADY / INVALID / SUPERSEDED / NEW-FINDING), fix, prove with tests
(fail-before/pass-after), and record the Resolution in the finding file.
Priority this session: the MAJOR tier first, then the remaining SEVERE (S-040),
then MINOR / INFORMATIONAL / TRACKED-OPEN. Aim 20+ fixes.

Two repositories in scope, only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
(GitHub token kept in git config only — never committed, never logged.)
The rda-driver repo is OUT OF SCOPE this session (OQ-S10-3 owns its
driver-resident findings: M-086/M-087/M-089/N-107/N-127 stay PUSHED-FORWARD).

SOPs (same as all prior sessions): verify → reproduce → classify → fix →
re-review → test → benchmark → document → push → loop. One finding, one
verified fix, one push. Re-implement from understanding — never paste
decompiled code. No stubs/mocks/fallbacks that mask failure. Safety
hardenings preserved (destination overlap refusal, BitLocker
legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
Performance-tagged fixes measured, not guessed. Windows direct I/O reads
stay 4 KB-multiple and 64 KB-multiple where possible. Win7-inclusive import
gate intact; signing stays disabled. Pull frequently; never force-push;
leave main green.

Deliverables: every finding resolved to FIXED / FIXED-ALREADY / INVALID /
SUPERSEDED with evidence; INFORMATIONALs ACKNOWLEDGED; tracked-open items
resolved or pushed with precise next steps; CROSS-REF-MATRIX / SUMMARY /
modules/*.md / FIXED-LEDGER updated; fixed findings moved to
findings/FIXED/; worklog handoff; full green CI.

Baselines recorded at session open:
- core HEAD: 98a5b139 (branch main)
- corpus HEAD: be0cfe9
- S55 filings pin: 9633d94f (per diff-to-fix/README.md)
- Linux suite baseline at 98a5b139: 1380 ran, 1338 PASSED, 0 FAILED,
  42 SKIPPED (Windows/tool-bound skips) — main green.
