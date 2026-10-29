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
