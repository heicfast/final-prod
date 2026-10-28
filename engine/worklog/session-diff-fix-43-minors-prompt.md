# Session 43 — the minor-first diff-to-fix execution prompt (token redacted)

**Lane identity (the collision convention, earlier remote push wins):** the parallel
fix lane took the `session-diff-fix-42-prompt.md` path (its b26017a7 push) and the
continuation audit lane took `session-42-continuation-audit-prompt.md`; THIS lane
(the same user mandate, the operative line "fix all the audit findings (priority to
minor first) one by one … put fixed findings in fixed directory. Update all docs,
counts and all properly. Aim for at least 20+ fixes") rebrands to
session-diff-fix-43-minors-prompt.md (the 39/40-severe rebrand precedent).

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding
(ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN): independently verify the finding
against the research corpus (`easeus-drw-research`, ground truth, read-only), reproduce
the evidence, classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
OUTDATED-EVIDENCE / NEW-FINDING), fix in the codebase, prove with tests that fail before
and pass after, mark the finding FIXED in the audit trail with evidence, move the file to
findings/FIXED/, update CROSS-REF-MATRIX / SUMMARY / modules / FIXED-LEDGER / docs.

Priority order this session: MINOR first (the user's explicit operative line), then the
tracked-open dispositions, then the MAJOR tier as far as the session reaches. The
remaining SEVERE set (S-048/S-050/S-040) is the parallel S42 fix lane's declared
territory; the INFORMATIONAL tier was acknowledged and retired by the e41cbb7e window.

SOPs (unchanged from all prior sessions): re-implement from understanding, no decompiled
pastes, fix the cause not the symptom, one finding one verified fix one push, pull
frequently, never force-push, keep main green, every fix paired with a test, safety
hardenings preserved, no stubs/mocks/TODOs, token never committed nor logged.

Audited-commit pins at this lane's open: core `632518b7` (origin/main after the
S12/S42/S-054/N-125/INFO-tier parallel windows), corpus `be0cfe9`. Baseline: the lane's
own from-scratch linux-release build (g++ 14.2 -Wall -Wextra -Werror -Wshadow) with the
full suite executed green (1203/1203 in this environment).

(The original prompt embedded a GitHub token; it is redacted here per the token-hygiene
rule. The credential lives in the local git credential store only.)
