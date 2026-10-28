# Session-56 — the diff-to-fix parity-fix mandate (ACUTE-first lane)

> Saved token-redacted at lane open. Repos in scope (only these two):
> Core (modified): https://github.com/heicfast/core_recovery
> Research corpus (ground truth, read-only): https://github.com/heicfast/easeus-drw-research
> GitHub token: env/git-config only, never committed, never logged. [REDACTED]

## Mandate summary

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding —
ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently
verify the finding (reproduce evidence, re-verify against the corpus),
classify it (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE /
OUTDATED-EVIDENCE / NEW-FINDING), fix it in the codebase, prove the fix
with tests (fail-before/pass-after), and mark it fixed in the audit trail
with evidence (Resolution section in the finding file, CROSS-REF-MATRIX
MATCH cell, modules/*.md row, SUMMARY.md counts, worklog.md entry).

SOPs: Verify -> Reproduce -> Classify -> Fix -> Re-review -> Test ->
Benchmark -> Document -> Push -> Loop. One finding, one verified fix, one
push. Pull frequently; never force-push; leave main green. Re-implement
from understanding; never paste decompiled code. Fix the cause, not the
symptom. Safety hardenings preserved. Performance fixes measured, not
guessed. Windows direct I/O: 4 KB-multiple lengths, prefer 64 KB
multiples. Win7-inclusive import gate intact. Signing stays disabled.
No shortcuts, no stubs, no mocks, no TODOs. Production grade only.

Fix order: ACUTE -> SEVERE -> MAJOR -> MINOR -> INFORMATIONAL ->
TRACKED-OPEN. Aim 20+ fixes this session. The audit trail
(`diff-to-fix/`) is the deliverable. Fixed findings move to
`findings/FIXED/` with Resolution sections; no finding file deleted.

## Lane coordination (session-56 open)

The S46 parallel lane (MINOR-first, territory claim at `659f0f6c`)
owns the unclaimed MINOR set. This lane (ACUTE-first per the user's
session directive) takes: **A-014, then the SEVERE board (S-055..S-061,
S-040, S-050), then the MAJOR tier (M-073..M-137), then the
INFORMATIONAL acknowledgments (I-035..I-044) as reached.** The
driver-repo pair (M-086/M-087/M-089/N-107/N-127 + OQ-S10-3) stays
tracked-OQ per the S42/S48 boundary disposition — no code disposition
possible from this repo's tree; those findings' core-side halves are
re-verified only.

Audited pin at lane open: core `fa654673` (opened; rebased through the
parallel windows to `f8f01513` after the red-main repair below),
corpus `be0cfe9`.

## Lane log

- **Red main repaired first (`f8f01513`):** the S-054 TS-pin tests
  (`Carve.TsM2TsCapIsTheOg200MB` + `Carve.TsStreamClampedAt200MB`) used
  terminator-style registry iteration over `kTable` — a table with no null
  terminator — walking off the end under ASan (the M-133/N-157 class).
  Both converted to count-based `registry_size()` iteration; suite
  1273/1273 green on the ASan lane. No finding retirement — the
  defect class is already owned by FIXED/M-133; this repair is its
  residue sweep.
