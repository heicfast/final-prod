# Session-89 S93 fix-lane prompt (2026-10-30) — token redacted

Saved at lane open per the house SOP. Token redacted: `ghp_[REDACTED — never
committed, never logged; env/git-config only]`. Read twice before work; the
operative SOP is summarized below verbatim from the user's mandate.

## The mandate (abridged to the operative rules; the full text is the user's S93 message)

- Senior systems engineer posture: 25+ years Windows storage internals,
  C++20 production engineering, parity remediation against reverse-engineered
  references. Execute the roadmap in `core_recovery/diff-to-fix/`.
- For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL,
  TRACKED-OPEN — independently verify, fix, prove with tests/benchmarks, mark
  fixed in the audit trail with evidence. Never trust the audit blindly;
  re-verify every finding against the research corpus and the running code.
  A false finding is corrected in the audit trail, not "fixed" by breaking
  good code.
- Two repositories in scope, only two: core (modified)
  `github.com/heicfast/core_recovery`; corpus (ground truth, read-only)
  `github.com/heicfast/easeus-drw-research`. Token env/git-config only,
  never committed. Do not clone/reference/look at any other repository.
  Do not guess reference behavior — verify against the corpus.
- Read before touching: diff-to-fix README/SUMMARY/SEVERITY-SCALE/
  PARAMETERS/CROSS-REF-MATRIX, every finding, every module overview, the
  evidence directory; the cited corpus artifact in full; every core header,
  source, test, workflow, build file, doc; every commit since the audited
  commit. Record the audited commit in the worklog and in every finding
  touched.
- Per finding: reproduce the evidence → re-verify against the corpus →
  classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
  OUTDATED-EVIDENCE / NEW-FINDING) → only then fix.
- Resolution template at the bottom of every touched finding file
  (Status / Fixed-in commit / Verified-by / notes). Never delete findings.
- Fixing rules: re-implement from understanding, never paste decompiled
  code, never copy proprietary strings, cite the corpus in the commit
  message. Fix the cause not the symptom. Match architecture and wiring.
  Every fix paired with a test that fails before and passes after. Safety
  hardenings are not slop — preserve them. No shortcuts, no fallbacks that
  mask failure, no stub/mock/TODO/placeholder. If a finding cannot be
  fixed this session, leave it TRACKED-OPEN with precise remaining scope
  and the next concrete step. No hallucinated parity.
- Order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN
  (the user's S93 closing line: "priority to major first", "aim for at
  least 20+ fixes", "fix all remaining ones").
- Performance fixes: baseline first, fix, re-benchmark, accept or revert;
  record numbers. Windows direct I/O: 4 KB-multiple lengths, 64 KB-multiple
  where possible.
- One finding, one verified fix, one push. Pull frequently, never
  force-push, keep main green. Commit messages state the finding ID, the
  divergence, the fix, the test, and the corpus citation.
- CI gates: Windows MSVC /W4 /WX green (DLL smoke, CLI contract, seeded
  integration, C consumer, fuzz smoke, Win7-inclusive import allowlist);
  Linux green (pure-core, CLI, ASan/UBSan/TSan on touched tests, fuzz
  smoke). No continue-on-error. Win7-inclusive build. Signing stays
  disabled.
- Docs: every fix in the finding's Resolution + worklog.md; CAPABILITY-
  MATRIX / architecture / ground-truth-contracts / error-fix-registry
  updated in the same commit; remove resolved TODO/FIXME/HACK notes;
  PARITY-DELTAS carries every deliberate divergence.
- Deliverables: every finding FIXED / FIXED-ALREADY / INVALID / SUPERSEDED;
  INFORMATIONALs ACKNOWLEDGED; TRACKED-OPENs resolved or precisely pushed
  forward; CROSS-REF-MATRIX fully MATCH; SUMMARY counts current; modules
  verdicts current; worklog handoff; green CI both platforms; per- fix
  before/after tests; tagged release if clean (v0.5.0-parity or next
  semver); the diff-to-fix/ directory is the final evidence set.
- §12 first-20-TODO list (T1 pull repos/record commits/save prompt → T2
  read diff-to-fix in full → T3 read core in full → T4/T5 ACUTE → T6/T7
  SEVERE → T8/T9 MAJOR → T10 perf → T11/T12 MINOR → T13 INFORMATIONAL →
  T14 TRACKED-OPEN → T15 matrix → T16 SUMMARY → T17 modules+docs → T18 CI
  sweep → T19 discovered tests → T20 tag+handoff+final sweep).
- §13 SOP loop: Verify → Reproduce → Classify → Fix → Re-review → Test →
  Benchmark → Document → Push → Loop. Re-read this prompt + SUMMARY +
  worklog periodically. Every intermediate push updates worklog.md.
  The audit trail is the deliverable.

## Lane identity

- Session: S93 (the next free seat at the open — S85's entry 5 landed the
  racing window's M-107 members 6/7/8 at `fc4ffdf4`; this lane opened
  after pulling it).
- Core open pin: `fc4ffdf4` (pulled from `7953c5e8`).
- Corpus: `be0cfe9` (read-only guarantee held throughout).
- Queue at open: M-107's remaining members (SIT, APE, SWF-RECT + the DWG
  gen-3 remainder), M-101 (Versions + Tags Extracted branches), M-105
  (DRWService, option (a)), N-225 (fixed by the S87 merge; needs the
  move to findings/FIXED/ + live verification), the five driver-repo
  findings (M-086/M-087/M-089/N-107/N-127 — out of the two-repo grant,
  OQ-S10-3), and the four tracked-open items.
