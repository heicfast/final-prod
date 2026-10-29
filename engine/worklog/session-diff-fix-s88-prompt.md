# Session-88 fix-lane prompt (token redacted) — the lane re-homed S84 → S88 per SA-9 (the racing windows took S68-S87)

Role: senior systems engineer (25+ years), Windows storage internals, C++20
production engineering, parity remediation against reverse-engineered
references. Mandate this session: execute the roadmap in
`core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR,
MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the finding,
fix it in the codebase, prove the fix with tests and benchmarks, and mark it
fixed in the audit trail with evidence. The audit is not trusted blindly;
every finding is re-verified against the research corpus and the running
code before any change is made. A false finding is corrected in the audit
trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only):
  https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED]

SOPs (strict, same as all prior sessions):
- Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test -> Benchmark ->
  Document -> Push -> Loop.
- Re-implement from understanding; never paste decompiled code; cite the
  corpus artifact in the commit message.
- Every fix is paired with a test that fails before and passes after
  (unit / integration / fuzz / TSan / ASan-UBSan as applicable; CLI contract
  tests for grammar changes; C consumer test for ABI changes; benchmark for
  performance-tagged findings).
- Safety hardenings are preserved (destination overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer explicit consent, 4Kn handling,
  cancellable IRP, SDDL gating).
- No shortcuts, no fallbacks that mask failure, no stubs, no mocks, no TODOs
  left behind. If a finding cannot be fixed this session, leave it
  TRACKED-OPEN with a precise remaining scope and the next concrete step.
- One finding, one verified fix, one push. Pull frequently; never
  force-push; leave main green. Commit messages state finding ID,
  divergence, fix, test, and corpus citation.
- Windows CI: MSVC /W4 /WX, DLL smoke, CLI contract, seeded integration,
  C consumer, fuzz smoke, Win7-inclusive import allowlist gate, signing
  stays disabled. Linux CI: pure-core logic, CLI, ASan/UBSan/TSan, fuzz.
- Performance fixes: baseline first, fix, re-benchmark, accept or revert;
  record numbers. Windows direct I/O reads multiple of 4 KB and 64 KB
  where possible.
- Audit trail updates after every fix: Resolution section in the finding
  file, CROSS-REF-MATRIX cells to MATCH, modules/*.md verdicts, SUMMARY.md
  counts, README if audited commit moved, worklog.md entry. Resolved
  findings move to findings/FIXED/ (preserved, never deleted). New
  findings discovered during verification are filed + fixed + announced.
- INFORMATIONAL findings: verify the deliberate-difference rationale, mark
  ACKNOWLEDGED; safety hardenings stay unchanged with a one-line
  confirmation.
- TRACKED-OPEN items: resolve to a fix this session or push forward with
  precise new evidence and the next concrete step.
- Deliverables end state: every finding in a resolved state
  (FIXED/FIXED-ALREADY/INVALID/SUPERSEDED/ACKNOWLEDGED), matrix populated
  with MATCH cells, SUMMARY counts current, modules verdicts current,
  worklog handoff with exact commit and remaining items, full green CI,
  no finding file deleted.

User's session order this time: "Fix all the audit findings (priority to
minor first) one by one, with full context and knowledge and update and put
fixed findings in fixed directory. Update all docs, counts and all properly.
Aim for at least 20+ fixes in this session, don't stop before that."

Opened at core `842a587e` (RED-again main at open), pulled through four
racing windows (16 + 38 commits scale) to the landing head `db468e29`,
corpus `be0cfe9` throughout (read-only guarantee held). This lane's
concessions per SA-9: the 27bda40e marker repair (racing N-219), the
S-068 substitution-clobber repair (racing N-220 / cd3479e7), and the
trail-marker/D-seat sweep (racing N-223 — their seat assignments stand:
N-145 keeps D-152, ext4-guards → D-163, the M-154 pointer → D-164).
