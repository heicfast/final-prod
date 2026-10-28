# Session-40 diff-fix prompt (token redacted, saved per mandate)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the
finding, fix it, prove the fix with tests and benchmarks, and mark it fixed in the
audit trail with evidence. The audit is a hypothesis; this lane is the verifier.
False findings are corrected in the audit trail, not "fixed" by breaking good code.

Repos (only two): core (modified) github.com/heicfast/core_recovery; research corpus
(ground truth, read-only) github.com/heicfast/easeus-drw-research. GitHub token via
env/git-credential store only — never committed, never logged (redacted here:
`ghp_[REDACTED]`).

SOPs (non-negotiable, same as all prior sessions):
- Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark -> Document -> Push -> Loop.
- Re-implement from understanding; never paste decompiled code; cite the corpus artifact.
- Fix the cause, not the symptom. Match architecture and wiring.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings preserved (dest-overlap refusal, BitLocker legit-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- No shortcuts, no fallbacks masking failure, no stub/mock/TODO. Production grade only.
- One finding, one verified fix, one push. Pull frequently; never force-push; keep main green.
- Token hygiene; commit messages cite finding ID + divergence + fix + test + corpus cite.
- Windows CI green (MSVC /W4 /WX, DLL smoke, CLI contract, integration, C consumer, fuzz smoke, Win7 allowlist). Linux CI green (ASan/UBSan/TSan, fuzz smoke).
- Resolved findings: Resolution section in file, FIXED-LEDGER row, `git mv` to findings/FIXED/ (session-39 convention per the user's "put fixed findings in fixed directory").
- Update CROSS-REF-MATRIX / modules/*.md / SUMMARY counts / worklog.md on every landing.

User's session directive: **fix all the audit findings, priority to MINOR first, one
by one; put fixed findings in the fixed directory; update all docs and counts; aim
for 20+ fixes this session.**

Lane identity: session-40 (the minors continuation lane over the session-39 N-111
landing; opens at core `461825d8`, corpus `be0cfe9`).
