# Session 33 (DFX33) — diff-to-fix closing prompt (token redacted)

Saved at session start. Core HEAD at start: `2d1626b` (merge: worklog union).
Corpus: `03ad46c` (unmoved since session 1).

## Mandate (condensed from the user prompt; full text mirrored)

- Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding
  (ACUTE → SEVERE → MAJOR → MINOR → INFORMATIONAL → TRACKED-OPEN):
  independently verify against the corpus, classify
  (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
  OUTDATED-EVIDENCE / NEW-FINDING), fix with tests that fail before and
  pass after, and record a Resolution section in the finding file.
- Repos in scope, only: heicfast/core_recovery (read-write) and
  heicfast/easeus-drw-research (read-only ground truth).
- Re-implement from understanding; never paste decompiled code; cite the
  corpus artifact in commit messages. Fix causes, not symptoms. Match
  architecture and wiring. No stubs/mocks/TODOs/fallbacks that mask
  failure. Safety hardenings preserved. Pull frequently, never
  force-push, leave main green.
- One finding, one verified fix, one push, one finding-file update.
- Performance fixes measured, not guessed (baseline → fix → re-benchmark
  → accept or revert). Windows direct I/O: 64 KiB-multiple reads.
- CI must stay green: Linux (gcc -Wall -Wextra -Wpedantic -Werror,
  ASan/UBSan/TSan lanes, fuzz smoke) + Windows (MSVC /W4 /WX, DLL smoke,
  CLI contract, seeded integration, C consumer, fuzz smoke, Win7
  import-allowlist gate). Signing stays disabled.
- Audit trail updates after every fix: finding Resolution, CROSS-REF-
  MATRIX (MATCH cells), modules/*.md verdicts, SUMMARY counts,
  README if needed, worklog.md. Fixed findings move to findings/FIXED/
  (preserved, never deleted).
- New divergences discovered during verification are filed as
  NEW-FINDINGs with evidence, announced in commit messages, fixed the
  same session if feasible.
- Target for this session: 20+ fixes before session end; tagged release
  if the audit closes cleanly.

## Token hygiene

The GitHub token was provided out-of-band (env-only at clone time). It is
never committed, never logged, never written to any repo file. This file
redacts it. Remote URL reset to tokenless https form after clone.
