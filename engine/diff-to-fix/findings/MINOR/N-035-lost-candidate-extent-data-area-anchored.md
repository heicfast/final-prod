# MINOR N-035 — Lost-candidate extent semantics: og anchors extents at the data area (DataAreaStart), core anchors at the partition start

**Severity:** MINOR
**Module:** lost-partition
**Parameters affected:** A29 (merge/dedup extent math), B6 (extent reporting), A30
**Corpus references:** `og-codebase/log/CalcPartition.log` (round 1 cand39: `StartSec:28836189 DataAreaStart:29308200 EndSec:271449027 totalSec:242140827` — `EndSec = DataAreaStart + totalSec`; round 2 cands 36-40 show DataAreaStart − StartSec gaps of 690029/2724233/422345/336043/834699 sectors); `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:842476-842479` (`EndSec = DataAreaStart + totalSec` in the dump: `local_98 = local_a0 + local_90`)
**Core references:** `src/core/lost/cr_candidates.cpp:117` (`c.sector_count = p.total_bytes / p.bytes_per_sector` — whole-volume), `:262-263` (extent clamp to device), `:274-275` (containment math on `[start_lba, start_lba + sector_count)`), `src/core/lost/cr_lostpart.h` (Candidate has no data-area fields)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's `CSearchedPartitionInfo` carries BOTH `StartSector` and
`DataAreaStart`, and its `totalSector`/`EndSec` extent math is anchored at the
**data area** (`EndSec = DataAreaStart + totalSector`), excluding the
reserved+FAT metadata region (up to hundreds of MiB on large FAT32). Our
Candidate's extent is `[start_lba, start_lba + sector_count)` with
`sector_count` = the whole volume. The og's containment-drop rule (M-017) and
its 16-MiB size-match both operate on the data-area extent; our dedup and
overlap penalties operate on the whole-volume extent — different geometry into
the same-shaped comparisons.

## What the reference does

Log ground truth (three rounds): every candidate line pairs `StartSec` with
`DataAreaStart` and computes `EndSec = DataAreaStart + totalSec` — the dump
code at DRWUI_decompiled.c:842476-478 makes the arithmetic explicit
(`local_98 = local_a0 + local_90` where `local_a0 = *(i64*)(info+0x10)`
(DataAreaStart) and `local_90 = *(i64*)(info+0x60)` (totalSector)). FAT32
candidates show metadata gaps of 472011 (round 1) and 690029..2724233 sectors
(round 2) between StartSec and DataAreaStart — the reserved+two-FATs region.
NTFS candidates show DataAreaStart == StartSec (no gap).

## What our core does

`probe_fat` sets `c.sector_count = p.total_bytes / p.bytes_per_sector`
(cr_candidates.cpp:117) — the full volume including the metadata region — and
all downstream extent math (dedup :274-275, overlap :290, materialize :181-189)
uses `[start_lba, start_lba + sector_count)`. There is no data-area field on
Candidate.

## Gap

Falsifiable claim: for a lost FAT32 volume whose FAT region overlaps a peer
candidate's data area, the og's containment/size-match rules (which use
data-area extents) produce a different keep/drop/promote decision than our
whole-volume extents — e.g. our outer candidate "contains" a peer because its
FAT region spans it, where the og's data-area extent would not.

## Impact

- Secondary interaction with M-017's rules (their inputs differ by the
  metadata gap).
- User-visible extent reporting: our lost-partition sizes include the metadata
  region; the og reports data-area sizes (its `totalSector` is what the QA log
  prints).

## Reproduction / verification

- Log quotes (arithmetic shown above) + decompile quote of the dump line.
- Core quotes (single-anchored extent).

## Fix plan (for the engineering team)

1. Add `data_start_lba` (and keep `start_lba`) to `Candidate`; set it in the
   FAT probe from `p.first_data_sector` (already computed at
   cr_chain.cpp:304), and equal to `start_lba` for NTFS/exFAT.
2. Use the data-area extent for containment/overlap classification (and for
   M-017's rules when implemented); keep the whole-volume extent for
   materialize/slicing (reads need the metadata region).
3. Report both sizes in stats (`sector_count` whole-volume,
   `data_sector_count`) for honesty.

## Tests to add

- `Lost_Fat32_Extent_Uses_DataArea_For_Containment` (peer inside the FAT
  region is not "contained").
- `Lost_Candidate_Reports_DataArea_Offset`.

## Related findings

- M-017 (the rules that consume these extents), M-017 (score unaffected).

## Open questions

None.
