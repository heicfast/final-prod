# Session 40 — the SEVERE-first diff-to-fix execution prompt (token redacted)

**Lane identity (the collision convention, earlier remote push wins):** the parallel
session-40 lane (minor-first directive) pushed its prompt file first and keeps the
`session-diff-fix-40-prompt.md` path; THIS lane (the same user mandate, SEVERE-first
priority per the prompt's explicit line) rebrands to session-diff-fix-40-severe-prompt.md
(the 39/39B rebrand precedent).

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding
(ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN): independently verify the finding
against the research corpus (`easeus-drw-research`, ground truth, read-only), reproduce
the evidence, classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
OUTDATED-EVIDENCE / NEW-FINDING), fix in the codebase, prove with tests that fail before
and pass after, mark the finding FIXED in the audit trail with evidence, move the file to
findings/FIXED/, update CROSS-REF-MATRIX / SUMMARY / modules / FIXED-LEDGER / docs.

Priority order this session: SEVERE first (the user's explicit line), then MAJOR, MINOR,
INFORMATIONAL, TRACKED-OPEN. Target 20+ fixes.

SOPs (unchanged from all prior sessions): re-implement from understanding, no decompiled
pastes, fix the cause not the symptom, one finding one verified fix one push, pull
frequently, never force-push, keep main green, every fix paired with a test, safety
hardenings preserved, no stubs/mocks/TODOs, token never committed nor logged.

Audited-commit pins at this lane's open: core `461825d8` (the post-`4a6a5164` pull,
pre-`7320a2f6`), corpus `be0cfe9`. Baseline: linux-release build clean with g++ 14.2
-Wall -Wextra -Werror -Wshadow, ctest 1113/1113 green at the open tree.

(The original prompt embedded a GitHub token; it is redacted here per the token-hygiene
rule. The credential lives in the local git credential store only.)
