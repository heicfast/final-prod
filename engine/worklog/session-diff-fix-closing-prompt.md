# diff-to-fix closing session — session prompt (token redacted)

Saved: 2026-10-26. Session start commit (core): `b65f1e6` (pulled fresh; includes
the s6 bootstrap leg `022c81d` + the s6 merge `b65f1e6`). Corpus: `03ad46c`
(unmoved). Audited commit recorded in `diff-to-fix/README.md`: session 4
`204137b`; fix legs have landed since (S-011/S-012/S-019/S-022, N-022/N-023/
N-034, minors batch 1) — every finding is re-verified against the CURRENT tree
before any change.

## Mandate (verbatim summary; token redacted)

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with
tests and benchmarks, and mark it fixed in the audit trail with evidence. Do not
trust the audit blindly; every finding is re-verified against the research
corpus and the running code before any change is made. A false finding is
corrected in the audit trail rather than "fixed" by breaking good code.

Two repositories in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED]

Do not clone, reference, or look at any other repository. Do not guess reference
behavior — verify against the corpus.

## Key SOPs

1. Read before touching anything: diff-to-fix/** in full; corpus line-by-line
   where a finding requires it; core repo in full; record the audited commit.
2. Independently verify each finding before fixing: reproduce evidence,
   re-verify against corpus, classify CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING. Only then fix.
3. Fixing rules: re-implement from understanding (never paste decompiled code);
   fix the cause not the symptom; match architecture and wiring; every fix
   paired with a test that fails before and passes after; safety hardenings are
   preserved (destination overlap refusal, BitLocker legitimate-unlock-only,
   RawFixer explicit consent, 4Kn handling, cancellable IRP, SDDL gating);
   no stubs/mocks/TODOs/fallbacks; ReFS and LDM remain first-class; pull
   frequently, never force-push, leave main green; token hygiene.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN;
   within tier: correctness before performance, data integrity before latency.
5. Performance fixes: baseline → fix → re-benchmark → accept or revert; record
   numbers. Windows direct I/O: reads multiple of 4 KB and of 64 KB where
   possible.
6. Tests: unit (fail-before/pass-after), integration on seeded fixtures,
   structure-aware fuzz for parsers, TSan for threading, ASan/UBSan for memory,
   CLI contract tests, ABI consumer test, benchmark comparison. Test names
   describe behavior proven.
7. Audit trail: Resolution section per finding; CROSS-REF-MATRIX cells to
   MATCH; modules/*.md verdict tables; SUMMARY.md counts; worklog.md.
   Never delete finding files.
8. Discovered divergences: new finding file + matrix row + SUMMARY update +
   fix or precise track; announce in commit message.
9. CI: Windows green (MSVC /W4 /WX, DLL smoke, CLI contract, seeded
   integration, C consumer, fuzz smoke, Win7-inclusive import allowlist gate);
   Linux green (pure-core, CLI, sanitizers on touched tests, fuzz smoke).
   No continue-on-error. Signing stays disabled.
10. Docs: every fix documented; capability matrix, architecture,
    ground-truth-contracts, error-fix-registry updated in the same commit;
    remove resolved TODO/FIXME/HACK notes; PARITY-DELTAS for deliberate
    divergences.

## Deliverables

All findings resolved (FIXED / FIXED-ALREADY / INVALID / SUPERSEDED), INFO
ACKNOWLEDGED, tracked-open resolved or pushed forward precisely, matrix fully
MATCH, SUMMARY counts, modules verdicts, worklog with SHAs, green CI, perf
numbers, fail-before/pass-after tests, all changes pushed, no finding deleted,
tagged release if clean, handoff with exact commit and next steps.

Loop: Verify → Reproduce → Classify → Fix → Re-review → Test → Benchmark →
Document → Push → Loop. One finding, one verified fix, one push.
