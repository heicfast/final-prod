# S3-LOST scratch report — OQ-T9 CalcPartition scoring decode + lost-partition deep pass

Session 3, agent S3-LOST (read-only parity auditor). Core HEAD `34b3441`,
corpus HEAD `03ad46c`. Protocol-compliant: only findings/evidence/module-
appends/this report written; corpus untouched; no commits.

## (a) What I read (files + line ranges — corpus side)

- `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`:
  38660-38790 (ICalcPartition ctor/dtor + slots 0-4 accessors), 38900-39071
  (Fat32 deleting dtor, flatten getter, slot-11 BuildDataRunList,
  cluster→sector math), 39076-39221 (FUN_180030d90/d90 proof + reverse
  mapping), 40770-41100 (root-dir/read helpers, find-or-create
  FUN_180032c80), 41270-42300 (verify/merge family FUN_180033270/33cd0 +
  size backfill 0x2800), 5560-5790 (file→volume attribution
  FUN_1800084b0 incl. :5744-5753 slot-4 walk), 6060-6189 (per-file volume
  registration), 7780-7999 (engine-open + boot validator call sites),
  38025-38200 (0x55AA check + FAT16/32 engine-open validator
  FUN_18002f970). Full-file greps: CalcPartition refs, 0x55AA, slot-call
  patterns, FUN_18002f970/950 callers.
- `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c`:
  28584-29075 (slots 0-4/5/11 + flatten), 41310-41494 (buildFileSet context,
  slot-4 read, cluster math).
- `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`:
  50960-51084 (NTFS boot validator FUN_18003f480 + SPC/MFT-record
  encodings).
- `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c`:
  2470-2800 (FAT flavor ladder + FAT12/16/32 gates + "-FVE-FS-" BitLocker
  classifier), 10300-10410 (NTFS memcmp sites), full vftable census (parser
  + generator classes), 13787-13880 (CRecoverySearchEngine MOM wrapper).
- `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`: 842164-842495
  (dSuggested formula + FS enum + containment filter + dump), 842495-842860
  (AdjustPartitionInfoOrder FUN_140510410 full), 845600-845840 (caller:
  sort→adjust→cap 2/4/5/6, result logging), 846885-846960 (merge sort),
  942560-942820 (CSearchedPartitionInfo + CExFatPartition ctors,
  serialization field list), 1045020-1045140 (CSearchedNTFSPartitionInfo copy
  ctor).
- Binaries (RE probes, read-only): FATSearchDLL.dll + exFATSearchDLL.dll RTTI
  string dumps + PE walks (executed — see (b)); PartitionRecoveryD.mo /
  Partition.mo / BitLockerLib.dll FVE-string greps (executed).
- Logs: `og-codebase/log/CalcPartition.log` (all 205 lines — 3 rounds),
  `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` (all),
  `re-workspace/artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md` §3,
  `re-workspace/artifacts/PartitionRecovery/mo-module-map.md`.
- Census greps across all 8 SearchDLLs (CalcPartition/RTTI), ui-resources/
  rtti_class_census.json (ICalcPartition/ExFatCalcPartition rows).

## (a) What I read — core side

- `src/core/lost/cr_candidates.cpp` (all 317 lines).
- `src/core/fat/cr_chain.cpp:10-49, 261-319` (flavor thresholds +
  validate_boot).
- `src/core/orchestrator/cr_scan.cpp:984-1048` (candidate consumption).
- Prior findings: S-006, M-002 (full), modules/lost-partition.md,
  modules/ldm.md, SEVERITY-SCALE.md, PARAMETERS.md (A29/A30/B1/B3/B6/B8),
  RESOLVED-INDEX.md, tracked-open/OQ-T8-T9 (full).

## (b) Findings filed (ID, title, 1-line gap, evidence type)

| ID | Title | Gap | Evidence |
|---|---|---|---|
| S-014 (SEVERE) | Lost BitLocker ("-FVE-FS-") partitions: probe family missing | og deep scan classifies FVE boot sectors (type 0xB) + BitlockerParser; core's dispatch drops them in the FAT catch-all | Decompiled (PartitionRecoveryD :2778/:2604/:721) + executed binary-string probe |
| M-013 (MAJOR) | Scoring: og log-normalized recovered-file counts vs core static boot points | og raw = files+folders×50 (×50/×50 FAT, ×10 HFS+), +0x68 bonus, +500 known-partition, ln(raw)/ln(max); core = 15/25/30 boot points, no content evidence, unstable sort | Decompiled (DRWUI :842164-842279, :846885) + vendor CalcPartition.log validation (≤2e-5) |
| M-014 (MAJOR) | Overlap merge/curate: containment-drop + FS-priority + ±16 MiB promotion + cap vs penalize-and-keep | og AdjustPartitionInfoOrder's 5 rules (drop-in-known, ≤0.1/≤0.3- FAT rules, FAT-vector drop, size-match promotion, cap 2/4/5/6) all absent in core | Decompiled (DRWUI :842284-842859, :845665) + log round-3 promotion proof |
| N-027 (MINOR) | FAT boot-gate strictness constants | og SPC ≤ 128, numFATs {1,2}, "FAT16/32   " name string, root ≥ 224, FAT12 spf ≤ 200; core looser on all | Decompiled (PRD :2613-2726; FATSearchDLL :38037-38090) vs core cr_chain.cpp |
| N-028 (MINOR) | Candidate extents data-area-anchored (og) vs whole-volume (core) | og EndSec = DataAreaStart + totalSec (metadata region excluded); core [start, start+count) | Vendor log arithmetic + DRWUI :842476-479 vs cr_candidates.cpp:117 |
| I-010 (INFO) | Post-S-006 probe surface is a deliberate superset of og's 7-family deep scan | og has NO Btrfs/HFS+/ZFS/ReFS block parsers (types only in its enum/engine subclasses); core's planned 5 probes exceed og — record + pin | Vftable census + zero-hit greps + DRWUI enum |

Evidence files: `evidence/decompiled-quotes/s3-calcpartition-scoring.md`
(§1-§10: per-FS table, formulas, all quotes);
`evidence/executions/s3-calcpartition-vtables/` (rtti_vtable_dump.py +
fat/exfat dumps + README — executed RTTI COL walk resolving both vtables).

## (c) Parameters newly verified MATCH

- **A30 (partial — FAT flavor brackets)**: og cluster-count brackets
  `<0xFF5 / <0xFFF5` (PartitionRecoveryD_decompiled.c:2547/:2564) ≡ core
  `<4085 / <65525` (cr_chain.cpp:14-17). MATCH on thresholds; the flavor
  *gate ordering* (bracket-tried-first) is an og detail without core analog
  (core derives flavor from one count — equivalent outcome).
- **A30 (machinery correction, not a verdict change)**: session-2's slot-4
  mechanics re-confirmed by executed vtable resolution (slot 4 =
  `*(u32*)(this+0x18)`; fast path `plVar7[3]` ≡ +0x18) — the *interpretation*
  changed (SPC, not score).
- **A29 (one MATCH element)**: the og's sort is descending-by-score stable —
  direction matches the core's `std::sort` descending intent (core's is
  unstable — filed under M-013).

## (d) UNVERIFIED residuals (precise question + next artifact)

1. **`+0x68` bonus producer**: what fills `CSearchedPartitionInfo+0x68`
   (serialized, ctor-default 1; log-observed values 0 and ~60, additive into
   the raw score)? Next artifact: DRWUI interop writers of `+0x68` in the
   PartitionRecovery result-import path (search DRWUI_decompiled.c for
   longlong stores to `+0x68` adjacent to `+0x60` totalSector writes; then
   PartitionRecovery.dll's export that builds the candidate blob).
2. **Reference size `this+0x40`** for the ±16 MiB size-match promotion
   (DRWUI manager field): plausibly the size of the partition being
   recovered. Next artifact: constructor/writers of the manager object at
   DRWUI_decompiled.c:845560-845670 (`this+0x40`).
3. **Per-candidate file/folder count pass mechanics** (NTFS in particular):
   the counts exist per candidate in the log and the FAT/exFAT engine-side
   attribution is decoded, but the bounded counting walk for NTFS candidates
   (full $MFT walk vs sampled) is not. Next artifact: NTFSSearchDLL
   CSearchedNTFSPartitionInfo fill sites (RTTI string in NTFSSearchDLL.dll
   strings_ascii.txt) + `CNTFSSearchController` partition logic.
4. **CFATSearchedPartitionInfo `+0xa8..+0xb0` vector** whose >1-element state
   drops FAT candidates in AdjustPartitionInfoOrder (:842817-842818): likely
   the matched-boot-sector/geometry-variant list. Next artifact:
   CFATSearchedPartitionInfo ctor/vector-fill sites in DRWUI (RTTI descriptor
   at :842814-842815).
5. **BitlockerParser geometry source** (size extent of FVE candidates) —
   next artifact: `ghidra-partition/BitLockerLib_decompiled.c` +
   BitlockerParser parse method (PartitionRecoveryD_decompiled.c after :721).
6. **OQ-T8 (open, not mine)**: RAWSearch dispatch order — untouched; only
   noting the FAT-engine-side facts I collected are orthogonal to it.

## (e) Verdicts appended

- `diff-to-fix/modules/lost-partition.md`: Session 3 section with **(S3)**
  rows for A30 (DIVERGENCE-MAJOR → M-013), A29 (DIVERGENCE-MAJOR → M-014),
  B1 (DIVERGENCE-SEVERE → S-014), B3 (DIVERGENCE-MINOR → N-027), probe
  superset (INFO → I-010), plus the machinery correction and a session-3
  findings index.
- `modules/ldm.md`: NOT appended — no new EBR/table-scoring facts landed
  (M-002 stands; the og's candidate pipeline being table-independent was
  already cited there).
- `tracked-open/OQ-T8-T9-...md`: appended a **RESOLVED (session 3)** section
  for OQ-T9 (append-only edit, as permitted); OQ-T8 explicitly left open.

## Notes for the consolidator

- S-006 is accurate as filed but under-lists the og by the BitLocker family —
  handled via the NEW S-014 (append-only discipline; S-006's own text
  untouched).
- The `zfs-vdev--mftmirr--calcpartition.md` §3 artifact's "detailed scoring
  walk remains an open item" is now closed by this session's evidence file;
  its "per-FS geometry calculators" framing was right, "scoring engines"
  framing superseded.
- All six findings cite exact decoded constants; M-013's formula is
  arithmetically validated against the vendor's own QA log (three rounds).
