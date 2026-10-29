# modules/tree.md — result-tree core audit verdicts

Reference: the og's result-tree family (the CResultNode/grading surfaces in
DRWUI + PartitionRecovery — the recovery-chances gauges and the tree the
scanners populate; the corpus artifacts at `re-workspace/artifacts/SearchDLL/`
+ `architecture/04-recovery-workflow.md` §3.2's FS code space).

Core (audited `0d7b1d9`-lineage, re-verified 2026-10-29): `src/core/tree/
cr_node.cpp` (483 LOC — the node schema), `cr_grading.cpp` (506 LOC — the
twin/overlap + confidence gauges), `cr_diskmap.cpp` (225 LOC — the disk-map
overlay), `cr_snapshot.cpp` (136 LOC — the flat enumeration index) +
`include/cr/tree/{cr_node,cr_grading,cr_diskmap,cr_snapshot}.h` (927 LOC
combined). Tests: `tests/unit/test_grading.cpp` (32 tests), `test_snapshot.cpp`
(4), `test_diskmap.cpp` (7), `test_node_tree.cpp` (4).

This row closes I-030's remaining module-doc gap (N-154 item 3): the tree
module's verdicts were previously scattered across orchestration.md (the
grading battery), visualization.md (the snapshot surfaces) and the findings;
they are transcribed here, verdict-for-verdict, from the matrix and the
retired findings' own Resolutions.

## Verdicts

- **#15 Confidence grading model** — `M` with open work: the twin-key
  device-family set **M-147** (renumbered from the M-133/M-138 chain — the
  og adjusts kinds {FAT16, EXT2-family} vs the core's whole-FAT-family shift;
  the corpus cites DRWUI:605752-605756 + :605924-605927), the deep-read
  validation **M-134** (open), the grading extension-table gate polarity
  **S-058** (open), the ReFS/Btrfs insert-time High grade **S-061** (open).
  Landed: M-125's progress-display math family, M-138's twin-key narrowing
  itself, N-183's gauge-population/QSDE batch (open).
- **#29 Merge and dedup strategy (twin/overlap detection)** — the M-147
  family above; the twin map's address-key contract pinned by the M-138
  Resolution's tests (TwinKeyFat32DoesNotShift et al.).
- **Snapshot/visualization surfaces** — `M`: the flat index-addressed
  enumeration (cr_snapshot), the disk-map overlay's free-cell truth (S-009
  FIXED: `kCellAllocFree` real), the S-052 LDM layout persistence (the
  tree's LDM roots carry the matching basis ids).
- **Node schema (cr_node.h)** — `M`: the D-1 fs-family truth, the N-126
  unsaved-category extension, the OQ-SA-1 origin dwords (fat_origin_cluster
  / ntfs_origin_lcn), the carve-format/confidence fields.
- **F59/F60 failure policy** — `NA` (advisory surfaces; no recovery-failure
  semantics in this module).

## Findings index

| ID | Severity | One-line | State |
|---|---|---|---|
| M-125 | MAJOR | the progress-display math (piecewise remap + 99.99 cap) | FIXED |
| M-138 | MAJOR | the twin-key device-family shift set narrowed to {Fat16} | FIXED |
| S-009 | SEVERE | the free overlay: engine bitmap truth produced + persisted | FIXED |
| S-052 | SEVERE | LDM span recovery basis (layout-relative extents) | FIXED |
| N-132 | MINOR | the false-landed API-DELTA triplet | FIXED |
| N-148 | MINOR | comment-truth batch (the og_prop_flags passive datum) | FIXED |
| M-134 | MAJOR | grading deep-read validation absent | OPEN |
| M-147 | MAJOR | the twin-key family set (the renumbered active finding) | OPEN |
| S-058 | SEVERE | grading ext-table gate polarity inverted | OPEN |
| S-061 | SEVERE | ReFS/Btrfs insert-time High grade absent | OPEN |
| N-183 | MINOR | gauge population/score formula QSDE batch | OPEN |

## Test debt

The grading battery is the deepest-tested area (32 tests + the M-125/M-138
RED-proven pins); the snapshot/diskmap legs carry 11 combined. The node-tree
schema itself is pinned indirectly through every consumer suite (the session
round-trips, the ABI row contracts) rather than by a dedicated schema test —
acceptable because the schema is append-only with compile-time consumers.

## S55 addendum (2026-10-29 — the sixth continuation lane: the grading deep dive)

New on this module: **N-205 — FIXED in the S60 lane** (the recoverability
rawCount population split: `AssistantStats::raw_tail_zero` now ticks the og's
exact population — the forced observe-only tail-zero probe over grade-1 NTFS
nodes with 0 < key < device-size, the 0x77fffff default arm, pass 0 of
grade_tree; raw_count keeps the Origin::Carved census as the REGISTERED
deliberate divergence D-146; the M-134 resolution's phantom `raw_tail_zero`
sentence repaired by addendum; Grading.RawTailZero* + RawCountSemanticsRegistered
RED-side proof — findings/FIXED/N-205-rawcount-population-semantics-og-validated-vs-core-carved-plus-phantom-raw-tail-zero.md).
**OQ-S42-3 closed this session** (retired to findings/FIXED/): the last
undecoded sub-gates are now pinned — the vt+0xb8 bool gate → grade 0, the
vt+0x100 FS-code field (0x23 ctor default in every SearchDLL; 0xa8 → grade 0 +
auto-High block; the 0x23 arm selects the ≤0x77fffff size bound), the
bit-7-of-+0x72 setter (the quick-scan extend-map registration,
FUN_140638400), and the bit-2-of-+0x74 setters (the drive-letter-path
recovered bit, two sites) — with the 0xa8-producer no-writer-found
observation registered as the likely-dead-branch residual. Evidence:
`../evidence/s55-lane-sheets/S55-D3-grading.md` (the full rule matrix).
