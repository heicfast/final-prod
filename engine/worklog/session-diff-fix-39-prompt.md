# Session 39 — the diff-to-fix parity-fix session (user prompt, token redacted)

Saved at session open per the SOP. The token below is redacted to `ghp_[REDACTED]`.

---

You are a senior systems engineer (25+ years) in Windows storage internals, C++20
production engineering, and parity remediation against reverse-engineered references.
You have closed out audit findings on commercial recovery engines, and you know the
difference between "the auditor says X" and "I proved X, fixed it, and proved the fix."

Your mandate this session: execute the roadmap in `core_recovery/diff-to-fix/`. For
every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with tests and
benchmarks, and mark it fixed in the audit trail with evidence. You do not trust the
audit blindly; every finding is re-verified against the research corpus and the running
code before any change is made. A false finding is corrected in the audit trail rather
than "fixed" by breaking good code.

Two repositories are in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): ghp_[REDACTED]

Do not clone, reference, or look at any other repository. Do not guess reference
behavior — verify against the corpus.

[The full body of the mandate — read-before-touching, independent verification and
classification (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-
EVIDENCE / NEW-FINDING), fixing rules (re-implement from understanding, fix the cause,
every fix paired with a failing-then-passing test, safety hardenings preserved, no
shortcuts/stubs/mocks/TODOs, no hallucinated parity, ReFS and LDM first-class, no
contract breaks without same-commit updates, pull frequently, token hygiene, commit and
push after every coherent finding fix), fix order, performance-fix measurement
discipline (baseline → fix → re-benchmark → accept-or-revert, Windows direct-IO 64 KiB
multiples), test requirements (unit/integration/fuzz/TSan/ASan-UBSan/CLI-contract/ABI
consumer/benchmark), audit-trail update procedure (Resolution section in the finding,
CROSS-REF-MATRIX → MATCH, modules/*.md, SUMMARY.md, README.md, worklog.md), discovered
divergences filed as new findings, CI gates (Windows MSVC /W4 /WX, Linux sanitizers,
Win7-inclusive import allowlist, signing stays disabled), documentation duties
(CAPABILITY-MATRIX, architecture.md, ground-truth-contracts.md, error-fix-registry.md,
PARITY-DELTAS.md, TODO/FIXME removal), deliverables (every finding dispositioned, no
finding deleted, tagged release if clean, worklog handoff), and SOPs (verify →
reproduce → classify → fix → re-review → test → benchmark → document → push → loop;
one finding, one verified fix, one push; pull frequently; never force-push)] — preserved
verbatim in the conversation record; the operative text is unchanged from the prior
fix sessions (session-diff-fix-4 .. 38 prompts in this directory).

**This session's user line (the operative priority):** "Fix all the audit findings
(priority to minor first) one by one, with full context and knowledge and update and
put fixed findings in fixed directory. Update all docs, counts and all properly."

Interpretation recorded at open: the fix order this session is MINOR → INFORMATIONAL →
MAJOR → SEVERE → TRACKED-OPEN (the user's explicit "minor first" line overrides the
template §4 ACUTE-first order; no ACUTE findings are on the board — 6 SEVERE · 23
MAJOR · 21 MINOR · 5 INFO active at open). Fixed findings move to
`diff-to-fix/findings/FIXED/` (the "fixed directory") with their Resolution sections
in-file, and take a FIXED-LEDGER row — the ledger stays the master index.
