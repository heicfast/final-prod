# # # Session-44 fix-lane prompt (2026-10-29, saved at T1 — token redacted per the mandate)

## Role and mandate

You are a senior systems engineer (25+ years) in Windows storage internals, C++20
production engineering, and parity remediation against reverse-engineered references.
Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE,
MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the finding, fix
it in the codebase, prove the fix with tests and benchmarks, and mark it fixed in the
audit trail with evidence. The audit is not trusted blindly; every finding is
re-verified against the research corpus and the running code before any change.

Two repositories in scope, only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed, never logged (`ghp_<REDACTED>`).

## Operating rules (per the mandate, condensed)

1. Read the audit output first: README/SUMMARY/SEVERITY-SCALE/PARAMETERS/CROSS-REF-MATRIX,
   every finding file, every module doc, every evidence artifact. Read the corpus line by
   line where a finding requires it; never assert reference behavior from finding text alone.
2. For every finding: reproduce the evidence, re-verify against the corpus, classify
   (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING),
   then fix. A Resolution section is appended to every finding file; findings are never deleted.
3. Fix the cause, not the symptom. Match architecture and wiring. Every fix paired with a
   test that fails before and passes after. No shortcuts, no fallbacks masking failure,
   no stubs/mocks/TODOs. Safety hardenings preserved (dest overlap refusal, BitLocker
   legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (ACKNOWLEDGED) → TRACKED-OPEN.
   Largest user impact first within each tier. Performance fixes measured, not guessed
   (baseline → fix → re-benchmark → accept or revert; Windows direct I/O 64 KB-aligned).
5. One finding, one verified fix, one push, one finding-file update. Pull frequently,
   never force-push, leave main green. Token hygiene absolute.
6. Audit trail updated after every fix: finding Resolution, CROSS-REF-MATRIX → MATCH,
   modules/*.md verdicts, SUMMARY counts, README nav, worklog.md; FIXED-LEDGER row;
   resolved findings land in findings/FIXED/ per the session-11 amendment.
7. CI gates: MSVC /W4 /WX + the full Windows matrix green; Linux green incl.
   ASan/UBSan/TSan + fuzz smoke; Win7-inclusive import gate intact; signing stays
   disabled. No continue-on-error.
8. Docs updated in the same commit as behavior changes (CAPABILITY-MATRIX, architecture,
   ground-truth-contracts, error-fix-registry, PARITY-DELTAS); resolved TODOs removed.
9. Discovered divergences get new finding files (full evidence, same template) — never
   silent fixes.
10. Deliverables end-state: no finding in DIVERGENCE-*/UNVERIFIED state; matrix MATCH cells;
    SUMMARY current; tagged release if clean; worklog handoff with next steps.

## Session opening state (recorded at T1, re-truthed at the post-merge baseline)

- Core HEAD at open: `d4b487c8` (baseline 1191/1191 green); RE-MERGED over the hottest
  parallel window to `0ff6cc92` (35 commits absorbed: the S40-severe lane's S-048/S-054,
  M-117, M-120, N-125, N-126, N-115, N-153, the S12 lane's filings, the S42-main INFO
  tier + bookkeeping); merged-tree verification 1217/1217 green first-hand.
- Corpus HEAD: `be0cfe95ecf22ac67e0f3a823dd003163b196d38` (unmoved).
- Active board after the merge (filesystem truth at 0ff6cc92): 2 SEVERE (S-040, S-050 -
  claimed by the active S40-severe lane per its 75dd2c96 territory claim) · 22 MAJOR ·
  16 MINOR (claimed by the 43-minors lane) · 0 INFO · 3 tracked-open. THIS LANE
  (session-44) claims the MAJOR tier - the unclaimed set.
- Driver-repo-resident findings (M-086/M-087/M-089, N-107/N-127, OQ-S10-3) are outside
  the two-repo scope of this session; they stay tracked, untouched.
- Target: 20+ verified fixes this session, severe-first.
