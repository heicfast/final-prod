# Session prompt — diff-to-fix fix session (2026-10-26) — token redacted

Mandate: execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN — independently
verify the finding, fix it in the codebase, prove the fix with tests and benchmarks,
and mark it fixed in the audit trail with evidence. The audit is a hypothesis; this
session is the verifier. False positives are corrected in the audit trail rather
than "fixed" by breaking good code.

Two repositories in scope, only two:
- Core (modified): https://github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed. REDACTED HERE.

Core SOP (from the session prompt, abridged to the operative rules):
1. Read diff-to-fix/ in full before touching anything; read cited corpus artifacts
   in full, not just excerpts; read the core repo line by line; record the audited
   commit in worklog and in every finding touched.
2. For every finding (priority ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL →
   TRACKED-OPEN): reproduce the evidence, re-verify against the corpus, classify
   (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE /
   NEW-FINDING), then fix. Add a Resolution section to every finding touched:
   Status (FIXED | FIXED-ALREADY | INVALID | SUPERSEDED), Fixed-in commit,
   Verified-by (tests/bench/evidence), Resolution notes.
3. Re-implement from understanding; never paste decompiled code; fix the cause
   not the symptom; match architecture and wiring; every fix paired with a test
   that fails before and passes after; preserve safety hardenings (destination
   overlap refusal, BitLocker legitimate-unlock-only, RawFixer explicit consent,
   4Kn handling, cancellable IRP, SDDL gating); no stubs/mocks/fallbacks; ReFS and
   LDM stay first-class; keep contracts updated in the same change; pull
   frequently, never force-push; commit+push after every coherent finding fix.
4. Fix order: ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL (ACKNOWLEDGED) →
   TRACKED-OPEN. Within tier: user impact first.
5. Performance fixes: baseline first, fix, re-benchmark, accept or revert; record
   numbers. Windows direct I/O: multiples of 4 KiB/64 KiB.
6. Tests: unit for logic, integration on seeded fixtures, structure-aware fuzz for
   parsers, TSan for threading, ASan/UBSan for memory, CLI contract tests for CLI
   changes, C-ABI consumer test for ABI changes, benchmarks for perf findings.
   Test names describe behavior proven.
7. Audit trail updates after every fix: finding Resolution, CROSS-REF-MATRIX cells
   → MATCH, modules/*.md verdicts, SUMMARY.md counts, README if needed, worklog.md.
8. Discovered divergences get NEW finding files + matrix rows + SUMMARY updates;
   announced in commit messages. No silent fixes.
9. CI green on Windows (MSVC /W4 /WX, DLL smoke, CLI contract, seeded integration,
   C consumer, fuzz smoke, Win7 import allowlist) and Linux (logic, CLI,
   ASan/UBSan/TSan, fuzz smoke). No continue-on-error. Win7-inclusive gate intact.
   Signing stays disabled.
10. Docs updated with behavior changes (CAPABILITY-MATRIX, architecture,
    ground-truth-contracts, error-fix-registry, PARITY-DELTAS); resolved TODOs/
    FIXMEs removed.
11. Deliverables: every finding resolved (FIXED/FIXED-ALREADY/INVALID/SUPERSEDED),
    INFO ACKNOWLEDGED, TRACKED-OPEN resolved or pushed forward precisely, matrix
    MATCH cells, SUMMARY counts, modules verdicts, worklog with commit SHAs, green
    CI, perf numbers, fail-before/pass-after tests, all pushed, no finding file
    deleted, tagged release if clean (v0.5.0-parity or next), worklog handoff.
12. T1..T20 task list; spawn more as needed.

Final reminders: corpus is the only source of truth; when finding evidence
conflicts with the corpus, the corpus wins; false positives cost more than honest
UNVERIFIEDs; no shortcuts, no samples, no assumptions, no hallucination,
production grade only; the audit trail is the deliverable; work long-horizon;
commit and push often; keep worklog.md current.
