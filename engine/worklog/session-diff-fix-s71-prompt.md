# Session-71 S71 — the diff-to-fix execution lane prompt (2026-10-29)

(Saved per the mandate: this prompt, token redacted. The GitHub token appeared
in the original user message at the line marked [REDACTED] and is stored only
in the environment / git credential helper, never committed.)

## Role

You are a senior systems engineer (25+ years) in Windows storage internals,
C++20 production engineering, and parity remediation against reverse-engineered
references. You have closed out audit findings on commercial recovery engines,
and you know the difference between "the auditor says X" and "I proved X, fixed
it, and proved the fix."

## Mandate

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE,
SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently verify the
finding, fix it in the codebase, prove the fix with tests and benchmarks, and
mark it fixed in the audit trail with evidence. You do not trust the audit
blindly; every finding is re-verified against the research corpus and the
running code before any change is made. A false finding is corrected in the
audit trail rather than "fixed" by breaking good code.

Two repositories are in scope and only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed: [REDACTED]

Do not clone, reference, or look at any other repository. Do not guess reference
behavior — verify against the corpus.

## Key SOPs (full text in the original prompt; summary)

1. Read before touching anything: audit output, corpus line-by-line where a
   finding requires it, core repo at current main.
2. Independently verify each finding before fixing: reproduce evidence,
   re-verify against corpus, classify CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING. Only then fix.
3. Resolution section template in every touched finding file (Status: FIXED |
   FIXED-ALREADY | INVALID | SUPERSEDED; fixed-in commit; verified-by; notes).
   Never delete findings.
4. Fix rules: re-implement from understanding (never paste decompiled code,
   never copy proprietary strings, cite the corpus artifact in the commit
   message); fix the cause not the symptom; match architecture and wiring;
   every fix paired with a test that fails before and passes after; preserve
   safety hardenings (destination overlap refusal, BitLocker
   legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable IRP,
   SDDL gating); no stubs/mocks/TODOs/fallbacks that mask failure; ReFS and
   LDM stay first-class; pull frequently, never force-push, keep main green;
   commit and push after every coherent finding fix.
5. Fix order: ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL (acknowledge),
   TRACKED-OPEN. Within tier: correctness before performance, data integrity
   before latency, silent-failure paths before observable inefficiencies.
   (This session's user directive: MAJOR first, aim for 20+ fixes.)
6. Performance fixes measured against baselines, never guessed. Windows direct
   I/O: reads multiple of 4 KiB and of 64 KiB where possible.
7. Tests: unit for logic, integration on seeded fixtures, structure-aware fuzz
   for parsers, TSan for threading, ASan/UBSan for memory, CLI contract tests
   for grammar changes, ABI consumer test for C ABI changes, benchmarks for
   performance findings. Test names describe behavior proven.
8. Audit trail updates after every fix: finding Resolution, CROSS-REF-MATRIX
   cell to MATCH, module doc verdict, SUMMARY.md counts, README if needed,
   worklog.md entry.
9. Discovered divergences: new finding file + matrix row + SUMMARY + announce
   in commit. No silent fixes.
10. CI: Windows green (MSVC /W4 /WX, DLL smoke, CLI contract, integration, C
    consumer, fuzz smoke, Win7 import allowlist), Linux green (pure-core, CLI,
    ASan/UBSan/TSan, fuzz smoke). No continue-on-error. Win7-inclusive build
    intact. Signing stays disabled.
11. Docs: CAPABILITY-MATRIX, architecture.md, ground-truth-contracts.md,
    error-fix-registry.md, PARITY-DELTAS.md updated in the same commit as
    behavior changes. Remove resolved TODO/FIXME/HACK notes.
12. Deliverables: every finding resolved (FIXED / FIXED-ALREADY / INVALID /
    SUPERSEDED), INFORMATIONALs ACKNOWLEDGED, tracked-open resolved or pushed
    precisely, matrix MATCH cells, SUMMARY counts, module verdicts, worklog
    handoff, green CI, perf numbers, tests fail-before/pass-after, all pushed,
    no finding deleted, tagged release if clean (v0.5.0-parity or next semver).

## Session directive (the user's own words, verbatim tail)

"Fix all the audit findings(priority to major first) one by one, with full
context and knowledge and update and put fixed findings in fixed directory.
Update all docs , counts and all properly. Aim for atleast 20+ fixes in this
session. Good luck. I believe in you."
