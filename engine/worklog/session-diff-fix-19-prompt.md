# Session 19 prompt — diff-to-fix execution (MAJOR-first per user directive)

Session start: 2026-10-26. Baseline HEAD: `11afbfa` (pulled after
`5d63124`; the parallel lane's N-030 landing). Corpus: `easeus-drw-research`
@ `03ad46c` (unmoved, read-only).

Mandate (verbatim summary, token redacted):

- Senior systems engineer persona, 25+ years Windows storage internals,
  C++20 production engineering, parity remediation against
  reverse-engineered references.
- Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
  ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently
  verify the finding, fix it in the codebase, prove the fix with tests and
  benchmarks, mark it fixed in the audit trail with evidence.
- Do not trust the audit blindly; every finding is re-verified against the
  research corpus and the running code before any change. A false finding is
  corrected in the audit trail rather than "fixed" by breaking good code.
- Two repos only: core (modified) `heicfast/core_recovery`; corpus
  (ground truth, read-only) `heicfast/easeus-drw-research`. Token via env /
  git config only, never committed, never logged.
- Classification loop per finding: reproduce evidence → re-verify corpus →
  classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
  OUTDATED-EVIDENCE / NEW-FINDING) → fix → prove.
- Resolution template: Status / Fixed in commit / Verified by / Resolution
  notes. Finding files never deleted; moved to findings/FIXED/.
- Fixing rules: re-implement from understanding, never paste decompiled
  code; fix the cause not the symptom; match architecture and wiring; every
  fix paired with a test that fails before and passes after (unit /
  integration / fuzz / TSan / ASan-UBSan as applicable); safety hardenings
  preserved (dest overlap refusal, BitLocker legitimate-unlock-only,
  RawFixer consent, 4Kn, cancellable IRP, SDDL gating); no shortcuts, no
  fallbacks masking failure, no stubs/mocks/TODOs; no hallucinated parity;
  ReFS and LDM stay first-class; contracts updated in the same commit that
  changes them; pull frequently, never force-push, main stays green.
- Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN;
  user override this session: "fix all major ones first" (no active ACUTE;
  S-029 is the only active SEVERE — claimed by the DFX10 lane per the
  worklog claim map; re-check its status before touching).
- Performance fixes measured, not guessed: baseline → fix → re-benchmark →
  accept or revert. Windows direct I/O: multiples of 4 KB, prefer 64 KB.
- Audit-trail updates per fix: finding Resolution, CROSS-REF-MATRIX cell →
  MATCH, modules/*.md verdict, SUMMARY.md counts, README if needed,
  worklog.md entry. New divergences filed as new findings — no silent fixes.
- CI gates: Linux green (this environment); MSVC /W4 /WX, DLL smoke, CLI
  contract, seeded integration, C consumer, fuzz smoke, ASan/UBSan/TSan on
  touched tests; Win7-inclusive import allowlist gate intact; signing stays
  disabled.
- Deliverables: every finding resolved to FIXED / FIXED-ALREADY / INVALID /
  SUPERSEDED; INFORMATIONAL → ACKNOWLEDGED; TRACKED-OPEN resolved or pushed
  forward precisely; matrix fully MATCH; counts current; worklog handoff;
  release tag if clean. Target: **at least 20+ fixes this session.**
- Fixed findings go to `diff-to-fix/findings/FIXED/` (per user directive:
  "update and put fixed findings in fixed directory").
- SOPs: Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark →
  Document → Push → Loop. One finding, one verified fix, one push.
