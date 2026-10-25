# diff-to-fix/ — parity audit roadmap (core_recovery vs easeus-drw-research)

**This directory is the deliverable of a read-only parity audit.** No source
code was modified to produce it. It is the roadmap future sessions execute
against.

| | |
|---|---|
| **Audited core commit** | `5d6da5f` (session 2 baseline; session 1 baseline was `67193b6`; concurrent fixes through `5d6da5f` include A-001/M-001/S-001/S-005 + GAP-S6-11 disk-map). Parallel remediation leg landed S-002/S-003-evidence/S-004/S-008/M-006/N-002/N-003/N-010-theirs/I-001..I-005-ack + clang-format gate repair (HEAD now `8574039`). |
| **Corpus (ground truth) commit** | `03ad46c` (`easeus-drw-research` main, unmoved) |
| **Audit dates** | 2026-10-25 (session 1) + 2026-10-25 (session 2) |
| **Method** | Parallel exhaustive reads (5 agents in session 1, 4 agents in session 2) → context sheets → module-by-module parameter audit → findings with evidence |

## Session 2 + parallel-leg combined delta

Session 2's mandate (find what session 1 missed, continue the audit, remove
the fixed things) landed alongside a parallel remediation leg that closed 6
more findings + filed 1 new finding + acknowledged 5 informational
supersets. Both legs reconciled in the post-rebase state.

- **17 new findings authored in session 2:** 2 SEVERE (S-010, S-011),
  3 MAJOR (M-007, M-008, M-009), 11 MINOR (N-011..N-020, N-021 — was N-010
  renumbered to avoid conflict with parallel leg's N-010), 2 INFORMATIONAL
  (I-006, I-007). Plus 9 new OQs folded into the new findings' "Open
  questions" sections.
- **6 findings FIXED by parallel leg:** S-004, S-008, M-006-fuzz, N-002,
  N-003, N-010-theirs (parallel leg's new finding, also FIXED).
- **5 informational findings ACKNOWLEDGED by parallel leg:** I-001..I-005
  (deliberate supersets, pin tests added).
- **5 findings moved to `findings/FIXED/` by session 2 wave 1:** A-001,
  S-001, S-005, M-001, OQ-T1.
- **4 more findings moved to `findings/FIXED/` by session 2 rebase
  reconciliation:** S-004, S-008, N-002, N-003, N-010-theirs.
- **1 finding's M-006 component FIXED** (combined S-009/M-006 finding file
  remains in SEVERE/ pending split — tracked in N-020).

## How to navigate

1. **`SUMMARY.md`** — start here. Counts per severity (active vs FIXED),
   per module, top risks, and the recommended fix order (first 20 fixes
   — updated for session 2 + parallel leg).
2. **`SEVERITY-SCALE.md`** — verdict definitions and discipline.
3. **`PARAMETERS.md`** — the 70-parameter taxonomy (11 groups) every module
   is scored against.
4. **`CROSS-REF-MATRIX.md`** — module × parameter matrix; every cell is a
   finding ID, `MATCH`, `NOT-APPLICABLE`, or `INFO-<id>`. Session 2 changes
   marked **(S2)**; parallel-leg fixes marked `X-n FIXED`.
5. **`findings/<SEVERITY>/<ID>-<slug>.md`** — one file per finding, in the
   mandated template.
   - `ACUTE/` — wrong recovery results / data loss / crashes / security.
     (Empty as of session 2 — A-001 was the only ACUTE, now FIXED.)
   - `SEVERE/` — user-observable behavioral gaps.
   - `MAJOR/` — perf, edge coverage, architecture.
   - `MINOR/` — cosmetics, doc drift.
   - `INFORMATIONAL/` — deliberate, reviewed differences (with pin tests
     for I-001..I-005).
   - `FIXED/` — **resolved findings** (audit trail preserved; not counted
     in active totals). Catalogued in `FIXED/RESOLVED-INDEX.md`.
6. **`modules/<module>.md`** — per-module overview: every parameter
   verdict with links to findings.
7. **`evidence/`** — `decompiled-quotes/` (exact RE snippets, file+address),
   `executions/` (run logs where empirical comparison was possible),
   `<scenario>/` fixtures.
8. **`tracked-open/`** — UNVERIFIED items that could not be resolved in
   either session, each with a precise question and the corpus artifact
   to examine.
9. **`CORPUS-ERRATA.md`** — 18 corpus errata candidates surfaced by the
   audit (session 1 + session 2 combined).

## Finding ID scheme

- `A-###` ACUTE, `S-###` SEVERE, `M-###` MAJOR, `N-###` MINOR (naming),
  `I-###` INFORMATIONAL.
- IDs are stable once issued; superseded findings are marked, never
  renumbered. Resolved findings are moved to `findings/FIXED/` (preserved
  for audit trail) rather than deleted.
- Session 2 added IDs S-010, S-011, M-007, M-008, M-009, N-011..N-020,
  N-021 (was N-010, renumbered to avoid conflict with parallel-leg's N-010),
  I-006, I-007. Parallel leg added N-010 (FAT BAD sentinel — FIXED in
  same commit).

## Conventions

- Every finding's **Core references** cite the audited commit (`67193b6`
  for session 1 findings; `5d6da5f` for session 2 findings). Files
  moved/changed after those hashes are noted in the finding when known.
- Corpus citations use paths relative to `easeus-drw-research/` root.
- The corpus carries its own errata (`re-workspace/artifacts/CORRECTIONS.md`,
  8 rows at audit time); session 1 folded those corrections in. Session 2
  surfaced 18 NEW corpus errata candidates (6 from S2-A, 4 from S2-B,
  4 from S2-C, 4 from S2-D) — cataloged in `CORPUS-ERRATA.md`.

## What was audited

- **Core (session 1 + session 2):** all 52 `src/**.cpp` (24,793 LOC), all
  53 `include/cr/**.h`, 37 unit-test files (313+ TEST() cases after parallel
  leg's S-002/S-003/S-004/S-008/N-002/N-003/N-010 additions), build
  system, CI workflows, fuzz harnesses, docs, CLI (`rrecov`), UI ABI
  (`cr_ui`). Session 2 added line-by-line reads of:
  `cr_mftwalk.cpp` (NTFS hard-link/ADS/sparse/reparse/$AttrList),
  `cr_attributes.cpp`, `cr_runlist.cpp`, `cr_bitmap.cpp`, all 8 engine
  `.cpp` files (timestamp decode), `cr_query.cpp` (filter/sort/paging),
  `cr_safewriter.cpp` (writer behavior), `cr_crsf.cpp` + `cr_records.cpp`
  (session resume), `cr_bitlocker.cpp` (BitLocker), `cr_destio.cpp`
  (destination I/O), `cr_rawfixer.cpp` (RawFixer), `cr_scan.cpp`
  (orchestrator threading), `cr_transport.cpp` (Windows I/O patterns),
  `cr_ui.cpp` (ABI surface), `rrecov.cpp` (CLI completeness).
- **Corpus (session 1 + session 2):** 9 architecture docs, 274 artifact
  docs, raw decompiled trees. Session 1 used the `.mo` files
  (ghidra-searchdll, ghidra-partition). **Session 2 added first-hand
  reads of the `re-workspace/reversed/ghidra-partition/*.c` decompiled
  files** (RecoveryModule_decompiled.c, PartitionRecovery_decompiled.c,
  RawFixer_decompiled.c) — these contained the canonical repair-path
  lock sequence (S-010 evidence) and the ACL-preservation
  `SetFileSecurityW` calls (N-009d refinement). 38 PE surfaces dumped,
  IOCTL census, raw_signs.json + FileFilter.xml decoded.

## Execution constraints of this audit environment

Empirical og-execution (running DRW itself) and new Ghidra decompiles
were not possible here (no Windows/wine, no Ghidra). Where the corpus was
the only evidence, findings cite the artifact + decompiled quote (file:line
on the .c file). Items requiring og-execution are in `tracked-open/` with
precise questions, or in each finding's "Open questions" section. The
core itself was compiled and executed on Linux (table parser, unit
tests) where feasible — logs in `evidence/executions/`.

## Status of the active roadmap (post-rebase)

- **Active findings:** 42 (0 ACUTE, 6 SEVERE [incl. S-009 partial], 5 MAJOR,
  16 MINOR, 7 INFO [all ACKNOWLEDGED], 8 tracked-open — see SUMMARY.md
  for the per-module breakdown).
- **Resolved findings:** 10 (in `findings/FIXED/` — see
  `findings/FIXED/RESOLVED-INDEX.md`).
- **Top priority for next remediation wave:** S-010 (safety regression on
  the repair path) — see SUMMARY.md recommended fix order.

---

# Third leg — main-auditor (session 2, 2026-10-25): the decompile-exhaustive pass

**What this leg added** (on top of the two prior session-2 legs): the exhaustive reads of
the decompiled trees that session 1 only spot-checked (NTFSSearchDLL two-pass architecture,
FAT/exFAT mirror+windows+validators, EXT4/HFS+ journal+decmpfs payloads, Btrfs/ZFS/ReFS
labels/profiles/pages, RAWSearchDLL dispatch+signatures, RawFixer 33-method repair set,
LdmManager RAID-5+TOCBLOCK+SPEC, DRWUI scoring+restore naming, NTFSLib boot ladder,
BitLocker DRWService/bitlockerhelp paths), a 424-file cited-vs-uncited corpus coverage
sweep, independent first-hand re-verification of every ACUTE/SEVERE claim, and a 282/282
Linux suite run at the audited commit (`evidence/executions/build-suite-session2.md`).

**IDs are renumbered past both prior legs:** A-004 · S-012/S-013/S-014 · M-011..M-021 ·
N-023..N-029 · I-009/I-010/I-011 · tracked-open OQ-S2M-1/OQ-S2M-2. One candidate was
**retracted by this leg's own re-verification** (exFAT fixer checksum predicate — core
MATCH, see `modules/exfat.md` and the evidence correction) and one of this leg's earlier
notes was corrected (A-001's LDM half — the parallel leg's executed proof wins; recorded in
`findings/FIXED/A-001-*.md`).

**Verified tree facts for whoever executes next:**
- **A-002's constant restore has still not landed** — `cr_table_wire.h:87-89` carries the
  fabricated LDM GUIDs with comments claiming Microsoft identity (checked at `34b3441`).
- The `fb44073` N-003 fix's `validate_entry_sets = false` default rests on a refuted
  premise — the og DOES validate (dual-form, incl. deleted sets); M-013 carries the fix.
- S-001's destroy rule is mis-scoped (S-012): one flag flip restores og pass-1 membership
  parity today; the GUID-dump pass 2 is the real missing feature.

Navigation: `SUMMARY.md` (third-leg close-out section at the end) · `CROSS-REF-MATRIX.md`
(main-auditor leg cell table) · `findings/FIXED/RESOLVED-INDEX.md` (this leg's verification
addenda) · `evidence/decompiled-quotes/*session2*` + `ntfs-pass2-guid-dump.md` +
`ldm-raid5-and-scoring.md` (the quote sheets).
