# Session diff-fix-13 prompt (token redacted)

(ID disambiguated from the parallel DFX10 leg during the bootstrap merge;
provenance: this session's mandate, archived verbatim-condensed, token
redacted. See worklog.md DFX13-T1.)

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding against the research corpus and the running code, fix it in
the codebase, prove the fix with tests and benchmarks, and mark it fixed in the
audit trail with evidence. A false finding is corrected in the audit trail
rather than "fixed" by breaking good code.

Two repositories in scope, only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): [REDACTED — never in any
  file, git credential store only]

Rules (condensed from full SOP):
1. Read before touching: diff-to-fix/ in full; corpus artifacts cited by
   findings, in full; core repo at current main; every commit since the
   audited commit.
2. Verify -> Reproduce -> Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED /
   FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) -> Fix -> Test -> Benchmark
   -> Document -> Push. Resolution section appended to every finding touched;
   finding files never deleted; fixed findings moved to findings/FIXED/ with
   RESOLVED-INDEX rows.
3. Fix order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL (verify the
   deliberate-difference rationale; ACKNOWLEDGED) -> TRACKED-OPEN. Target at
   least 20+ fixes this session. Within tier: correctness before performance,
   data integrity before latency.
4. One finding, one verified fix, one push. Pull frequently; never force-push;
   keep main green. Token never committed or logged.
5. Corpus wins over audit text. Re-implement from understanding — never paste
   decompiled code or proprietary strings. Fix the cause, not the symptom.
   Match architecture and wiring, not only outputs.
6. Every fix paired with a test that fails before and passes after (unit /
   integration on seeded fixtures / structure-aware fuzz for parsers / TSan
   for threading / ASan-UBSan for memory). No test, no FIXED.
7. Safety hardenings are not slop (destination overlap refusal, BitLocker
   legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable IRP,
   SDDL gating) — preserve them and note the preservation in Resolutions.
8. No stub, no mock, no TODO, no placeholder, no fallback that masks failure.
   Unresolvable-this-session items stay TRACKED-OPEN with a precise next step.
9. Performance fixes: baseline -> fix -> re-benchmark -> accept or revert;
   numbers recorded in the Resolution and SUMMARY. Direct I/O stays 4 KiB-
   and 64 KiB-aligned.
10. Windows CI green (/W4 /WX, DLL smoke, CLI contract, seeded integration,
    C consumer, fuzz smoke, Win7-inclusive import allowlist; signing stays
    disabled). Linux CI green (pure core, CLI, ASan/UBSan/TSan, fuzz smoke).
11. Audit trail updated with every fix: finding Resolution + move to FIXED/,
    CROSS-REF-MATRIX cell -> MATCH, modules/*.md verdicts, SUMMARY counts,
    worklog.md (this file's parent). Discovered divergences get new finding
    files — no silent fixes. Resolved TODOs/FIXMEs/HACKs removed from code
    and docs. PARITY-DELTAS gains any new deliberate divergence.
12. Deliverables: no finding left in DIVERGENCE-*/UNVERIFIED state; matrix
    fully MATCH; SUMMARY current counts + top remaining risks; worklog
    handoff with exact commit and next steps; tagged release if clean;
    diff-to-fix/ is the final evidence set.
