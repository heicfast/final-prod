# MINOR N-203 — the resume round-trip drops the candidate's data-area extent: resumed lost-partition candidates degenerate to zero-area records the fresh-run rules can never re-derive

**Severity:** MINOR
**Module:** lost-partition / session
**Parameters affected:** #12 Session persistence format (field equivalence), #5 Orphan reconstruction behavior (candidate assembly fidelity), #29 Merge and dedup strategy (the dedup/containment consumers)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:20154-20238` (FUN_180020a60 — the found-candidate record builder), `:976-994` (FUN_1800025a0 — the payload flattener filling the 0x448 image), `:20748-20842` (the load-time replay: dedup key {start @+0x28, end @+0x30, disk @+0x08} at `:20825-20831`), `:21289-21332` (FUN_180023280 — the lostpart.dat reader); `og-codebase/PartitionRecovery/lostpart.dat` (the shipped 0x448 record: start 0x800, end 0x0749d7bf, bps 512, SPC 8)
**Core references:** `src/core/orchestrator/cr_scan.cpp:2652-2675` (save: PartitionContext fill — no data-area fields), `:2844-2880` (load: candidate restore — no data-area fields), `src/core/lost/cr_candidates.cpp:534-599` (revalidate_candidate: the probe's scratch — which re-derives the data area — is discarded; `c` is const), `:187-188` (probe_fat sets data_start_lba/data_sector_count), `:964-965` (dedup consumes data_start_lba window), `:1008-1009` (containment vs known partitions consumes the data extent), `:1065-1066` (leader scoring consumes data_sector_count), `include/cr/session/cr_records.h:170-190` (PartitionContext: no data-area members), `include/cr/lost/cr_lostpart.h:70-71` (Candidate: data_start_lba/data_sector_count, default 0) — at audited core `9633d94f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's lostpart.dat record is a full `CSearchedPartitionInfo` image whose replay key is {start, end, disk} — with `EndSec` anchored at the data-area boundary (the N-035 decode: EndSec = DataAreaStart + totalSector), so the data-area extent survives the og's cross-process round-trip by construction. The core's session round-trip persists start/size/fs/score/geometry/label (PartitionContext v3) but NOT `data_start_lba`/`data_sector_count`, and the resume path never re-derives them (revalidate_candidate probes into a discarded scratch). A resumed session's candidates therefore carry `data_sector_count = 0`, while every fresh-run consumer of the data-area window (dedup, known-partition containment, leader scoring) was built against the N-035-landed anchored extents.

## What the reference does

The found-candidate record builder `FUN_180020a60` (PRD:20154-20238) appends a 0x448 record per hit: a 0x18 header (magic, phase counter, disk number, scan-position, kind flag) plus the 0x430-byte `CSearchedPartitionInfo` image flattened by `FUN_1800025a0` (PRD:976-994) — start LBA (+0x28), end LBA (+0x30, via `FUN_180002760` = start+size−1 with the extended-partition quirk), bps (+0x38), CHS-align bool (+0x441), SPC (+0x444), with the wider object fields (score double at +0x70; DataAreaStart per the N-035/`modules/lost-partition.md:159-163` decode) riding the same image. At load, the replay path `FUN_180021080`'s restore arm (PRD:20748-20842) re-applies every record through the dedup key `{start @+0x28, end @+0x30, disk @+0x08}` (PRD:20825-20831) — candidates re-enter the assembly pipeline with their full extent identity, data-area anchoring included (the shipped `lostpart.dat` sample's end-LBA `0x0749d7bf` validates exactly against `partinfo.dat`'s live entry: 2048 + 122,277,824 − 1).

## What our core does

The save path fills `PartitionContext` with start_lba, sector_count, fs_kind, score, kind, the M-042 v2 geometry tail (bps/SPC/hidden/flags) and the M-137 v3 presentation tail (disk_layout/disk_flags/volume_label) — `cr_scan.cpp:2652-2675`; the struct (`cr_records.h:170-190`) has no data-area members. The load path (`cr_scan.cpp:2844-2880`) restores exactly those fields; `data_start_lba`/`data_sector_count` stay at their `0` defaults (`cr_lostpart.h:70-71`). The re-validation pass (`cr_candidates.cpp:534-599`) re-probes the start LBA into a **scratch** candidate — the probe DOES re-derive the data area (`:187-188`) — but only the boolean verdict is returned; the scratch is discarded and the persisted candidate is never enriched. The fresh-run assembly consumes the anchored extents at `:964-965` (dedup window), `:1008-1009` (containment), `:1065-1066` (leader size tie-break) — consumers a resumed candidate reaches with a `[0, 0)` window.

## Gap

Stated falsifiably: a lost FAT32 candidate with `start_lba=1000, sector_count=2,000,000, data_start_lba=3000, data_sector_count=1,500,000` (N-035's anchored shape) round-trips through save→load with `data_start_lba=0, data_sector_count=0`; the og's record replays with the full extent identity. A re-saved session re-drops it (the fields are never re-derived), and any rule that re-runs over restored candidates sees a degenerate window where the fresh run saw the anchored extent.

## Impact

Resume-path fidelity: dedup suppression between an overlapping fresh hit and a restored candidate never fires (window `[0,0)` matches nothing), the ±16 MiB size-match and leader-selection behaviors operate on different inputs than a fresh run, and the known-partition containment drop is skipped for restored candidates. Not user-catastrophic (the restored candidates still list and recover at their start/size), but the resume path silently behaves differently from the fresh path on the exact rules N-035's landing calibrated — and the divergence compounds across re-saves. MINOR.

## Reproduction / verification

Core-side executed reads (this session, lead-verified): the save fill, the load restore, the revalidate const-candidate + discarded scratch, and all three fresh-run consumers read at the cited lines at `9633d94f`; grep `data_start_lba|data_sector_count` over `src/ include/` returns hits ONLY in `cr_candidates.cpp` and `cr_lostpart.h` (zero hits in `cr_scan.cpp` / `cr_records.*` / any session path). Corpus-side: the record builders, flattener, replay key, and the shipped-sample arithmetic cross-check re-read first-hand (`:20154-20238`, `:976-994`, `:20748-20842`, the two .dat files hex-verified). Worksheet: `evidence/s55-lane-sheets/S55-D1-localdb.md` (a)/(b).

## Fix plan (for the engineering team)

1. Append a v4 tail to `PartitionContext`: `u64 data_start_lba; u64 data_sector_count;` (append-only wire, v1–v3 sessions deserialize with zeros — the existing skip-unknown rule already covers this).
2. Save path (`cr_scan.cpp`, the pc fill): copy both fields from the candidate.
3. Load path (`cr_scan.cpp:2854+`): restore both fields into the candidate.
4. `revalidate_candidate`: when the probe succeeds AND the persisted record carried no data area (v≤3 session), copy the scratch's re-derived data-area fields into the candidate (drop the const, or return an enriched copy) — this repairs old sessions on their first resume.
5. Tests: `Session.RoundTripKeepsDataAreaExtent` (save→load, fields equal); `Session.V3SessionResumeReDerivesDataArea` (a hand-built v3 record + live probe restores the anchored window); `LostDedup.RestoredCandidateSuppressesOverlappingFreshHit` (the falsifiable dedup case above).

## Tests to add

- `Session.RoundTripKeepsDataAreaExtent` (fails today)
- `Session.V3SessionResumeReDerivesDataArea` (fails today)
- `LostDedup.RestoredCandidateSuppressesOverlappingFreshHit` (fails today)

## Related findings

N-035 (FIXED — the fresh-run anchored extents this finding's resume path drops), M-042 (FIXED — the session-checkpoint persistence architecture), M-137 (FIXED — the v3 presentation tail the v4 proposal extends), N-131 (active — the fs-native parent-anchor persistence gap, same record-family seam).

## Open questions

None.
