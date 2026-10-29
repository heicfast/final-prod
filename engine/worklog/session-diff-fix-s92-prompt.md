# Session-92 (S92) — the diff-to-fix fix mandate prompt (token redacted)

[Seat note: born-collided twice — S89 then S91; the racing windows took each
seat name first (their prompt files hold the seats); this lane renumbers to
S92 per the convention.]

[Seat note: born-collided on S89 — the racing windows took S89 (their
prompt file + their worklog entries hold the seat) and S90; this lane
renumbers to S91 per the convention.]

Saved at the session open, 2026-10-30. The verbatim user mandate with the
GitHub token redacted (the token never enters any repo file; it lives in
the session env / the local credential store only).

---

You are a senior systems engineer (25+ years) in Windows storage internals, C++20 production engineering, and parity remediation against reverse-engineered references. You have closed out audit findings on commercial recovery engines, and you know the difference between "the auditor says X" and "I proved X, fixed it, and proved the fix."

Your mandate this session: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the finding, fix it in the codebase, prove the fix with tests and benchmarks, and mark it fixed in the audit trail with evidence. You do not trust the audit blindly; every finding is re-verified against the research corpus and the running code before any change is made. A false finding is corrected in the audit trail rather than "fixed" by breaking good code.

Two repositories are in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED — ghp_***]

Do not clone, reference, or look at any other repository. Do not guess reference behavior — verify against the corpus.

[The full SOP sections 1–13 follow verbatim: read-before-touching; the
verify→reproduce→classify→fix loop (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING); the Resolution template;
fixing rules (re-implement from understanding, never paste decompiled code,
never copy proprietary strings, fix the cause, every fix paired with a
failing-then-passing test, safety hardenings preserved, no stubs or
fallbacks); fix order ACUTE→SEVERE→MAJOR→MINOR→INFORMATIONAL→TRACKED-OPEN
with the user's closing-line override "priority to minor first";
performance fixes measured not guessed (the 4 KiB/64 KiB direct-I/O
Windows rule); the test-adding rules; the audit-trail update rules
(CROSS-REF-MATRIX → MATCH, modules/*.md, SUMMARY counts, worklog);
discovered-divergence filing; CI gates (MSVC /W4 /WX, Win7-inclusive
import allowlist, signing stays disabled); documentation duties; the 20
TODOs; and the SOPs — verify → reproduce → classify → fix → re-review →
test → benchmark → document → push → loop; one finding one verified fix
one push; pull frequently, never force-push, leave main green.]

Final user line: "Fix all the audit findings (priority to minor first) one
by one, with full context and knowledge and update and put fixed findings
in fixed directory. Update all docs, counts and all properly. Aim for
atleast 20+ fixes in this session."

---

**S92 seat note (renumbered from the born-collided S89/S91):** opened at core `390069ab` (the S87/S88 racing windows'
landing head; pulled through their N-224/N-225/M-159/DWG-gen-2 absorptions
before this lane's first push), corpus `be0cfe9` (read-only guarantee
held). Baseline verified first-hand at the open: linux-release build
clean, full ctest **1683/1683 green**, marker sweep clean (the S87 window
swept the worklog block this lane also found at its open — converged
before any push; no concession needed). The racing lanes took seats
S68–S88; this lane takes S89.
