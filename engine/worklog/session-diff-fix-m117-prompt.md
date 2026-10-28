# Session: the M-first diff-to-fix execution lane (2026-10-29)

The full mandate: execute the roadmap in `core_recovery/diff-to-fix/` — for
every finding (SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN), independently
verify against the corpus (easeus-drw-research @ be0cfe9, read-only), fix in
the codebase, prove with tests (fail-before/pass-after), record the
Resolution in the finding file, update the audit trail (matrix / SUMMARY /
modules / FIXED-LEDGER), one finding per commit, push. User directive this
session: **priority to MAJOR first**, aim for 20+ fixes, fixed findings go to
findings/FIXED/.

Two repos only: heicfast/core_recovery (modified), heicfast/easeus-drw-research
(ground truth, read-only). Token in env/git config only, never committed.
No shortcuts / no stubs / no mocks / production grade only. Corpus wins over
finding text. Pull frequently, never force-push, keep main green. Baseline at
session open: core 4e74ee2f, suite 1119 tests / 1077 passed / 0 failed
(linux-release), build green.

(Generated from the session prompt; the GitHub token is intentionally
redacted from this file.)
