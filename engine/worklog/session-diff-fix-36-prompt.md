# Session 36 prompt — diff-to-fix closing wave (S-034, M-030, M-040, N-072, OQ residuals)

*Saved at session start, token redacted. Read twice before working.*

## Mandate

Senior systems engineer role (Windows storage internals, C++20 production
engineering, parity remediation against reverse-engineered references).
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently
verify the finding, fix it, prove the fix with tests and benchmarks, mark
it fixed in the audit trail with evidence. The audit is a hypothesis; this
session is the verifier. Corpus is the only source of truth; when a
finding's evidence conflicts with the corpus, the corpus wins.

Two repos only:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git-config only, never committed, never logged.

## Session start state (recorded at T1)

- core_recovery at `7ec5446` (merged from origin/main after fetch; the
  DFX24 M-039 landing absorbed).
- Corpus at `03ad46c` (unmoved, as every session).
- Open findings at start: S-034 (SEVERE, zfs ring txg history),
  M-030 (MAJOR, transport residuals), M-040 (MAJOR, boot-chain repair
  family), N-072 (MINOR, btrfs rescan cache + zfs label polling).
  Tracked-open: OQ-S4-3 (chunking relative perf — Windows-lane),
  OQ-Z-E-FLT (OQ-FLT-2 AdvanceFilterCondition decode residual).
- FIXED/ at start: 216 files.

## SOP (verbatim from the session prompt)

1. Read before touching anything: audit output, corpus (line by line
   where a finding requires it), core repo (every file, every commit
   since the audited commit).
2. Independently verify each finding before fixing: reproduce evidence,
   re-verify against corpus, classify CONFIRMED-DIVERGENCE /
   ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING.
3. Fixing rules: re-implement from understanding (never paste decompiled
   code, never copy proprietary strings, cite the corpus artifact in the
   commit message); fix the cause not the symptom; match architecture
   and wiring; every fix paired with a test that fails before and passes
   after; safety hardenings preserved; no shortcuts/fallbacks/stubs/TODOs;
   no hallucinated parity; ReFS and LDM first-class; no broken contracts;
   pull frequently; token hygiene; commit+push after every coherent
   finding fix.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL →
   TRACKED-OPEN. Within tier: correctness before performance, data
   integrity before latency, silent-failure before observable
   inefficiency.
5. Performance fixes measured, not guessed. Baseline → fix → re-bench →
   accept or revert. Windows direct I/O reads: multiples of 4 KB and of
   64 KB where possible.
6. Test proof set: unit + integration (seeded fixtures from
   diff-to-fix/evidence/ where possible) + structure-aware fuzz for
   parsers + TSan for threading + ASan/UBSan for memory + CLI contract
   tests for grammar changes + ABI consumer tests for C ABI surfaces +
   benchmark comparison for perf-tagged findings. Test names describe
   behavior, not finding IDs.
7. Audit trail updates after every fix: Resolution section in the finding
   file (FIXED | FIXED-ALREADY | INVALID | SUPERSEDED + commit + verified-by
   + notes), CROSS-REF-MATRIX.md cells to MATCH, modules/*.md verdict
   rows, SUMMARY.md counts, README.md navigation, worklog.md entry.
8. Discovered divergences: new finding file in the correct severity
   folder with full evidence + matrix row + SUMMARY update + fix or
   precise tracked-open. No silent fixes.
9. CI gates: Windows green (MSVC /W4 /WX, DLL smoke, CLI contract,
   seeded integration, C consumer, fuzz smoke, Win7-inclusive import
   allowlist), Linux green (pure-core, CLI, sanitizers, fuzz smoke). No
   continue-on-error. Win7-inclusive build intact. Signing stays
   disabled.
10. Documentation: finding Resolution + worklog.md; CAPABILITY-MATRIX
    updates; architecture/ground-truth-contracts/error-fix-registry in
    the same commit as behavior changes; remove resolved TODOs/FIXMEs;
    PARITY-DELTAS.md for every deliberate divergence.

## Deliverables

All findings resolved to FIXED / FIXED-ALREADY / INVALID / SUPERSEDED;
INFORMATIONALs ACKNOWLEDGED; tracked-open resolved or precisely pushed;
matrix fully MATCH; SUMMARY counts current; modules verdicts current;
worklog chronological; green CI; perf numbers recorded; tests
fail-before/pass-after; all changes pushed with evidence-citing
messages; no finding file deleted; tagged release if clean;
diff-to-fix/ is the final evidence set. User directive this session:
fix all remaining findings one by one, move fixed findings to the fixed
directory, update all docs and counts; target 20+ fix actions before
session end (the honest path: every sub-item of the multi-part findings
landed as its own verified commit + any NEW findings discovered during
verification, filed and fixed — no hallucinated parity to inflate
counts).

## First 20 TODOs

T1 pull both repos + record commits + save this prompt (token redacted);
T2 read diff-to-fix/ in full; T3 read core repo in full; T4/T5 verify +
fix ACUTE; T6/T7 verify + fix SEVERE; T8/T9 verify + fix MAJOR; T10
performance findings baseline/fix/re-bench; T11/T12 MINORs batched per
module; T13 INFORMATIONALs acknowledged; T14 tracked-open resolved or
pushed; T15 matrix; T16 SUMMARY; T17 modules + core docs + TODO/FIXME
sweep; T18 CI sweep both platforms; T19 discovered tests; T20 tag
release + handoff + final sweep.
