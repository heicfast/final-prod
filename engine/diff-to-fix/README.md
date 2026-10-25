# diff-to-fix/ — parity audit roadmap (core_recovery vs easeus-drw-research)

**This directory is the deliverable of a read-only parity audit.** No source
code was modified to produce it. It is the roadmap future sessions execute
against.

| | |
|---|---|
| **Audited core commit** | `67193b6` (`main`, includes concurrent fixes through `9fc82b0`) |
| **Corpus (ground truth) commit** | `03ad46c` (`easeus-drw-research` main) |
| **Audit date** | 2026-10-25 |
| **Method** | Parallel exhaustive reads (5 agents) → context sheets → module-by-module parameter audit → findings with evidence |

## How to navigate

1. **`SUMMARY.md`** — start here. Counts per severity, per module, top risks,
   and the recommended fix order (first 20 fixes).
2. **`SEVERITY-SCALE.md`** — verdict definitions and discipline.
3. **`PARAMETERS.md`** — the 70-parameter taxonomy (11 groups) every module is
   scored against.
4. **`CROSS-REF-MATRIX.md`** — module × parameter matrix; every cell is a
   finding ID, `MATCH`, `NOT-APPLICABLE`, or `INFO-<id>`.
5. **`findings/<SEVERITY>/<ID>-<slug>.md`** — one file per finding, in the
   mandated template (summary, reference behavior with citations, our behavior
   with file:line, gap, impact, reproduction, fix plan, tests, related, open
   questions).
   - `ACUTE/` — wrong recovery results / data loss / crashes / security.
   - `SEVERE/` — user-observable behavioral gaps.
   - `MAJOR/` — performance, edge coverage, architecture.
   - `MINOR/` — cosmetics, doc drift.
   - `INFORMATIONAL/` — deliberate, reviewed differences.
6. **`modules/<module>.md`** — per-module overview: every parameter verdict
   with links to findings.
7. **`evidence/`** — `decompiled-quotes/` (exact RE snippets, file+address),
   `executions/` (run logs where empirical comparison was possible),
   `<scenario>/` fixtures.
8. **`tracked-open/`** — UNVERIFIED items that could not be resolved in this
   session, each with a precise question and the corpus artifact to examine.

## Finding ID scheme

- `A-###` ACUTE, `S-###` SEVERE, `M-###` MAJOR, `N-###` MINOR (naming), `I-###` INFORMATIONAL.
- IDs are stable once issued; superseded findings are marked, never renumbered.

## Conventions

- Every finding's **Core references** cite the audited commit `67193b6`.
  Files moved/changed after that hash are noted in the finding when known.
- Corpus citations use paths relative to `easeus-drw-research/` root.
- The corpus carries its own errata (`re-workspace/artifacts/CORRECTIONS.md`,
  8 rows at audit time); this audit folds those corrections in and flags new
  errata candidates it found (see findings and `tracked-open/`).

## What was audited

- **Core**: all 52 `src/**.cpp` (24,793 LOC), all 53 `include/cr/**.h`,
  37 unit-test files (248 TEST() cases), build system, CI workflows, fuzz
  harnesses, docs, CLI (`rrecov`), UI ABI (`cr_ui`).
- **Corpus**: 9 architecture docs, 274 artifact docs (SearchDLL engine
  deep-dives, PartitionRecovery/RawFixer, SaveScan .rsf format, config-files,
  UIUX, runtime-families, per-binary), raw decompiled trees
  (ghidra-searchdll, ghidra-partition, ghidra-mo, ghidra-drivers), and the
  og-codebase binaries themselves (38 PE surfaces dumped, IOCTL census,
  raw_signs.json + FileFilter.xml decoded).

## Execution constraints of this audit environment

Empirical og-execution (running DRW itself) and new Ghidra decompiles were not
possible here (no Windows/wine, no Ghidra). Where the corpus was the only
evidence, findings cite the artifact + decompiled quote. Items requiring
og-execution are in `tracked-open/` with precise questions. The core itself
was compiled and executed on Linux (table parser, unit tests) where feasible —
logs in `evidence/executions/`.
