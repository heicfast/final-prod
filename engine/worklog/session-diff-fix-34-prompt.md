# Session 34 prompt (saved at bootstrap — token redacted; lane re-stamped
27→32→34 across three push races: each ID was squatted by parallel lanes
before this lane's claim landed — first-landing-wins honored throughout)

Mandate (user, verbatim excerpts):

- Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
  For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
  independently verify the finding, fix it, prove the fix with tests and benchmarks,
  and mark it fixed in the audit trail with evidence. Do not trust the audit blindly;
  every finding re-verified against the research corpus and the running code first.
- Repos in scope (only two): core (modified) = heicfast/core_recovery;
  research corpus (ground truth, read-only) = heicfast/easeus-drw-research.
  GitHub token: REDACTED (env/git config only, never committed, never logged).
- Read before touching anything: diff-to-fix README/SUMMARY/SEVERITY-SCALE/
  PARAMETERS/CROSS-REF-MATRIX, every finding, every module doc, evidence dir;
  corpus line by line where a finding requires it; core repo line by line at
  current main. Record the audited commit everywhere.
- Independent verification loop per finding: reproduce evidence → re-verify
  against corpus → classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE
  / OUTDATED-EVIDENCE / NEW-FINDING) → only then fix.
- Resolution section template per finding (Status / Fixed in commit / Verified by /
  Resolution notes). Never delete finding files. Move fixed ones to findings/FIXED/.
- Fixing rules: re-implement from understanding (never paste decompiled code,
  never copy proprietary strings, cite corpus artifact in commit message); fix the
  cause not the symptom; match architecture and wiring; every fix paired with a
  test that fails before and passes after; safety hardenings preserved (destination
  overlap refusal, BitLocker legitimate-unlock-only, RawFixer explicit consent,
  4Kn handling, cancellable IRP, SDDL gating); no shortcuts/stubs/mocks/TODOs; no
  hallucinated parity; ReFS and LDM remain first-class; pull frequently, never
  force-push, leave main green; one finding, one verified fix, one push.
- Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFO → TRACKED-OPEN; within tier,
  largest user impact first. This session's user directive: **focus on MINOR
  initially**, target **20+ fixes**, fixed findings moved to fixed directory, all
  docs/counts updated properly.
- Performance findings: baseline first, fix, re-benchmark, accept or revert; record
  numbers. Windows direct I/O reads: multiples of 4KB/64KB where possible.
- Tests: unit for logic, integration on seeded fixtures, structure-aware fuzz for
  parsers, TSan for threading, ASan/UBSan for memory, CLI contract tests for CLI
  changes, ABI consumer test for C ABI changes, benchmarks for perf findings.
- Audit trail updates after every fix: finding Resolution, CROSS-REF-MATRIX cell
  → MATCH, modules/*.md verdict, SUMMARY.md counts, README if needed, worklog.md.
- CI gates: MSVC /W4 /WX, DLL smoke, CLI contract, seeded integration, C consumer,
  fuzz smoke, ASan/UBSan/TSan, Win7-inclusive import allowlist, signing stays
  disabled. No continue-on-error.
- Docs: CAPABILITY-MATRIX, architecture.md, ground-truth-contracts.md,
  error-fix-registry.md, PARITY-DELTAS.md updated in the same commit as behavior
  changes; resolved TODO/FIXME/HACK notes removed.
- Deliverables: every finding resolved to FIXED / FIXED-ALREADY / INVALID /
  SUPERSEDED; matrix fully MATCH-populated; SUMMARY counts; module verdicts;
  worklog handoff; green CI; perf numbers; fail-before/pass-after tests; pushed
  commits with evidence-citing messages; no finding file deleted; diff-to-fix/
  as the final evidence set.

Session lane (DFX34): MINOR tier BACK-TO-FRONT from the tail —
N-064 → N-071 → N-072 → N-075 → N-066 → N-065 → N-059 → N-050 → N-049 →
N-036 → N-033 → N-031 → N-027 → N-026 → N-007/I-004 → N-009-remainder →
N-019 → N-032 — 20+ fix target (groups count per sub-fix), every parallel
landing absorbed with a convergence record per first-landing-wins.
