# Session-47 mandate — the MAJOR-first parity-fix lane (saved token-redacted per the standing convention)

**Saved:** 2026-10-29, lane opens at core `fa654673` / corpus `be0cfe9`.

## The mandate (verbatim summary, token redacted)

Senior systems engineer posture (25+ years, Windows storage internals, C++20 production
engineering, parity remediation against reverse-engineered references). Mandate: execute
the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE, SEVERE, MAJOR,
MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the finding, fix it in the
codebase, prove the fix with tests and benchmarks, and mark it fixed in the audit trail
with evidence. The audit is not trusted blindly; every finding is re-verified against the
research corpus and the running code before any change.

Two repositories in scope and only two:
- Core (modified): github.com/heicfast/core_recovery
- Research corpus (ground truth, read-only): github.com/heicfast/easeus-drw-research
- GitHub token: env/git config only, never committed (REDACTED)

No other repository may be cloned or referenced. Reference behavior is verified against
the corpus, never guessed.

### Operating loop (per finding)

Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
OUTDATED-EVIDENCE / NEW-FINDING) → Fix → Re-review → Test → Benchmark (perf-tagged only)
→ Document → Push → Loop.

### Fixing rules

- Re-implement from understanding; never paste decompiled code; never copy proprietary
  strings; cite the corpus artifact in the commit message.
- Fix the cause, not the symptom. Match architecture and wiring, not only outputs.
- Every fix paired with a test that fails before and passes after (unit / integration /
  structure-aware fuzz / TSan / ASan-UBSan as applicable).
- Safety hardenings preserved (destination-overlap refusal, BitLocker
  legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL gating).
- No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs/placeholders.
- One finding, one verified fix, one push, one finding-file update. Pull frequently;
  never force-push; leave main green.
- Performance fixes measured (baseline → fix → re-benchmark → accept or revert); Windows
  direct I/O reads multiple of 4 KB / 64 KB where possible.
- Resolution section added to every touched finding file (FIXED / FIXED-ALREADY /
  INVALID / SUPERSEDED + commit + verification + notes).
- Audit trail updates per fix: finding file Resolution, CROSS-REF-MATRIX cells → MATCH,
  modules/<module>.md verdict rows, SUMMARY.md counts, worklog.md.
- Fixed findings move to findings/FIXED/ with a FIXED-LEDGER row (the SA-10B convention).
- Discovered divergences get new finding files; no silent fixes.
- Win7-inclusive import gate intact; signing stays disabled; CI green both platforms.
- docs/CAPABILITY-MATRIX.md, architecture.md, ground-truth-contracts.md,
  error-fix-registry.md, PARITY-DELTAS.md updated in the same commit as behavior changes.

### This session's user directive

Fix all the audit findings (priority to MAJOR first) one by one, with full context and
knowledge; update and put fixed findings in the fixed directory; update all docs and
counts properly. Target: at least 20+ fixes this session.

## Lane division (coordination contract)

The parallel S45 lane claimed ACUTE/SEVERE (its saved mandate is ACUTE-first); the
parallel S46 lane claimed the MINOR board top-down. This lane (S47) claims the UNCLAIMED
MAJOR set at `fa654673`, per the user's explicit MAJOR-first directive: M-073, M-075,
M-078, M-101, M-105, M-107, M-111, M-113, M-114, M-115, M-118, M-119, M-123, M-124,
M-126, M-127, M-128, M-129, M-130, M-132, M-133, M-134, M-135, M-136, M-137 (25
in-tree; M-086/M-087/M-089 stay tracked-OQ per OQ-S10-3 — their fix surfaces live in
heicfast/rda-driver, out of this session's two-repo grant, re-verified before claiming).
Pulled-before-pushed discipline; one finding, one push; every fix RED-test-first.
