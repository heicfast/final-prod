# S3-LOST evidence — OQ-T9 decode: the CalcPartition family, the real candidate scoring, and AdjustPartitionInfoOrder

Session 3, agent S3-LOST. Corpus HEAD `03ad46c`, core HEAD `34b3441`.
Everything below is decompile-quoted with `file:line`, log-quoted, or produced by
the RE probe in `evidence/executions/s3-calcpartition-vtables/` (vtable slot
resolution via RTTI COL walk — see that README).

**Headline:** the `*CalcPartition` classes are **not** candidate scorers. They are
per-lost-volume **geometry calculators** (cluster↔sector address mapping) that let
the FAT/exFAT engines read files out of a volume that is not in the partition
table. The vtable "slot 4" value of session-2's note is the volume's
**sectors-per-cluster**. The actual lost-partition candidate *scoring* is a
**recovered-file/folder count** computed per candidate, log-normalized in
DRWUI.exe (`dSuggested`), ranked by a stable merge sort, then curated by
`AdjustPartitionInfoOrder` (DRWUI.exe), which merges near-ties with an
FS-priority rule, drops candidates contained in known partitions, promotes a
size-matched candidate (±16 MiB) to the front, and caps the surfaced result set
to 2/4/5/6 entries. All of this is cross-validated against the vendor's own
`og-codebase/log/CalcPartition.log` (three real search rounds, 79/101/6
candidates).

---

## 1. Fat32CalcPartition — RTTI, vtable, and the slot-4 method

RTTI names (FATSearchDLL.dll `rabin2_strings_data.txt:545-546`):
`.?AVFat32CalcPartition@@` @ VA `0x18006c540` (TypeDescriptor `0x18006c530`),
`.?AVICalcPartition@@` @ VA `0x18006c570` (TD `0x18006c560`).

Vtable extraction (executed, see `evidence/executions/s3-calcpartition-vtables/fat_vtable_dump.txt`):

| slot | VA | Ghidra symbol | body (decompile cite) |
|---|---|---|---|
| 0 | 0x180030520 | `FUN_180030520` | `*(u64*)(this+0x10) = v;` — SetStart (FATSearchDLL_decompiled.c:38716-38721) |
| 1 | 0x180030530 | `FUN_180030530` | `*(u32*)(this+0x18) = v;` — SetClusterSize (:38726-38731) |
| 2 | 0x180030b90 | `FUN_180030b90` | flatten getter: out{+0x00=+0x10, +0x0c=+0x08, +0x08=+0x18, +0x10=+0x1c, +0x20=+0x20, +0x18=+0x28} (:38991-39001) |
| 3 | 0x180030540 | `FUN_180030540` | `return *(u64*)(this+0x10);` — GetStart (:38736-38740) |
| **4** | **0x180030550** | **`FUN_180030550`** | **`return *(u32*)(this+0x18);`** — GetClusterSize (:38745-38749) |
| 6 | 0x180030a20 | `FUN_180030a20` | deleting dtor (vftable re-store at :38936/:38981) |
| 11 | 0x180030bc0 | `FUN_180030bc0` | BuildDataRunList for a file in the lost volume (:39012-39071) |

Object layout (from the find-or-create `FUN_180032c80`, :41036-41072, and the
ctor-copy in `FUN_180033cd0`, :41928-41957): `+0x00 vftable · +0x08 sector size
(u32, init 0x200) · +0x10 start LBA (u64) · +0x18 sectors-per-cluster (u32) ·
+0x1c, +0x20 (u32) · +0x28 volume size in sectors (u64) · +0x38/+0x58/+0x68 three
sub-objects · +0x48 device ptr · total 0x78 bytes`.

**Proof that slot 4 = sectors-per-cluster, not a score** —
`FUN_180030d90` (FATSearchDLL_decompiled.c:39076-39083):

```c
longlong FUN_180030d90(longlong param_1,uint param_2)   // this, cluster#
{
  if (1 < param_2) {
    return (ulonglong)((param_2 - 2) * *(int *)(param_1 + 0x18))   // (cluster-2) * SPC
         + *(longlong *)(param_1 + 0x10);                          // + start LBA
  }
  return 0;
}
```

Same arithmetic appears in the verify path `FUN_180033960` (:41814-41815,
`lVar17 = (uVar18 - 2) * uVar2 + lVar10` — first-cluster LBA of a root-dir
entry) and in the per-file path `FUN_180008d50` (:6133,
`(uVar2 - 2) * iVar5 + puVar7[2]`). The `+0x18` value is therefore the FAT
geometry's sectors-per-cluster; (start, SPC) is the **volume identity key** used
to attribute found files to lost volumes: `FUN_1800084b0` searches the
candidate map for `slot4(candidate) == *(int*)(fileRecord + 0xc)` and
`slot3(candidate) == *(u64*)(fileRecord + 0x10)` (:5653-5654, :5744-5753) —
the same match made through the direct-field fast path
(`iVar3 = (int)plVar7[3]`, :5745) or the slot-4 virtual call (:5748).

Candidate intake: `FUN_18000cd30` (:8852-8921) receives a
**vector of CSearchedPartitionInfo***, `__RTDynamicCast`s each to
`CFATSearchedPartitionInfo` (:8879-8882), and builds `map<start,+0xc>` — the
FAT subclass carries start at `+0x10` and SPC at `+0xc` (local_30/local_28,
:8885-8886). Find-or-create with geometry dedup: `FUN_180032c80` (:40894-41073)
searches for an existing CalcPartition with matching (start, SPC) —
`iVar3 == iVar14` over the equal-start range (:41005-41007) — else constructs
one and calls slot 0 then slot 1 with (start, SPC) (:41065-41066). A separate
4-field identity dedup (sector size, start, SPC, +0x1c) drops duplicates via
slot 2 + slot 6 (`FUN_180033cd0`, :42048-42059). Size backfill: when a later
candidate's start is >0x2800 sectors (5 MiB) past a previous candidate's start,
the previous candidate's size field is set to the gap (:42085-42125).

## 2. ExFatCalcPartition — identical shape

RTTI: exFATSearchDLL.dll `rabin2_strings_data.txt:559/562`
(`ExFatCalcPartition` TD `0x18005efc8`). Vtable @ `0x18004b9f0` (executed,
`exfat_vtable_dump.txt`): slots 0-4 are byte-identical accessors —
`FUN_180022640/650/660/670` at exFATSearchDLL_decompiled.c:28584-28617
(slot 4 = `return *(u32*)(this+0x18);` :28613-28617). Flatten getter
`FUN_180022d10` :28929-28939 is identical to the FAT one. Slot 11
(`FUN_180022d40`, :28944-29036) builds a `CExFatDataRunList` per cluster of the
FAT chain for a file in the lost volume (uses `+0x18` SPC and `+0x08` sector
size, :29006-29007). Slot 5 (`FUN_180023000`, :29066-29075) appends to the
internal list at `this+0x78` — the extra slot the FAT class does not override.
The `buildFileSet` log string (`:41437`,
`"buildFileSet:%lld,sec:%lld,ICalcPartition:null,partition:%lld,%u\r\n"`) fires
when a found file's volume pointer (`file+0x98`) is null; otherwise the code
reads the volume's slot-4 cluster size — direct field for the exFAT class
(`uVar15 = *(uint *)(pppppppppppplVar19 + 3)`, :41453-41455) — and does
`(cluster - 2) * *(uint *)(partInfo + 0xc)` cluster math (:41491-41493).

**No other engine has a CalcPartition class.** `rg -c CalcPartition` over all
`ghidra-searchdll/*_decompiled.c`: FAT 39 refs, exFAT 54 refs, NTFS/EXT4/
Btrfs/HFSPlus/Zfs/ReFS/RAW = 0. The other engines carry the *SearchedPartitionInfo*
half instead: `CSearchedNTFSPartitionInfo` (NTFSSearchDLL strings),
`CSearchedBtrfsPartitionInfo` (BtrfsSearchDLL strings), base
`CSearchedPartitionInfo` (HFSPlus/Zfs/ReFS DLL strings).

## 3. The real scoring — dSuggested in DRWUI.exe

`FUN_14050fda0` (DRWUI_decompiled.c:842164-842279) computes `dSuggested`
(`CSearchedPartitionInfo+0x70`, double):

```c
// pass 1: denominator = log(max raw) over all candidates (:842209-842236)
raw(info) =  files(+0x20) + folders(+0x28)*10            if FS(+0x08) == 8   (HFS+)   :842214-842215
          |  (files + folders) * 0x32                    if FS-1 < 2 (FAT16/32)        :842217-842218
          |  folders*0x32 + files                        otherwise                     :842220-842221
          ;  raw += *(u64*)(info + 0x68)                                                 :842223
denom = raw_max > 1 ? log(raw_max) : 1.0                                                 :842231-842239
// pass 2: per candidate (:842241-842277)
if candidate matches an entry of the known-partition array param_3
   (stride 0x18: {start(+8), FS(+0x10)}) on FS AND StartSector(+0x18)+param_4 == start:
       log "Found once partition:%I64d";  raw += 500                                     :842260-842267
score(+0x70) = (raw > 1 ? log(raw) : raw) / denom                                        :842269-842274
```

`+0x68` is a serialized per-candidate integer bonus (ctor default 1,
`FUN_1405aa6a0` :942638; in the field-list serializer :942701). Its producer
semantics are not decoded (see residuals) — empirically 0 and ~60 in the QA log.

**FS enum** (`FUN_140510080`, :842341-842391): 0 FAT12, 1 FAT16, 2 FAT32,
3 NTFS, 4 EXT2, 5 EXT3, 6 RAW, 7 EXFAT, 8 HFSPLUS, 9 EXT4, 10 REFS, 0xE Unknow.

**Ranking:** `FUN_1405178a0` = merge sort, comparator
`*(double*)(right+0x70) < *(double*)(left+0x70) || ==` → take left
(:846927-846928) — descending dSuggested, stable. Called at :845669 right
before `FUN_140510410` (AdjustPartitionInfoOrder) at :845670.

## 4. AdjustPartitionInfoOrder (FUN_140510410, DRWUI_decompiled.c:842495-842859)

Input: the score-sorted candidate list (`param_2`), an output list
(`param_3`, cleared :842539-842551), and a reference sector count
(`param_4 = *(this+0x40)`, :845670). Decoded rules, in order:

1. **Known-partition containment drop** — `FUN_14050ffb0` (:842284-842323):
   remove every candidate with `extent.start <= cand.StartSector &&
   cand.StartSector + cand.totalSector(+0x60) <= extent.start + extent.size`
   (:842305-842306) — i.e. fully inside an existing extent (the
   "ActivePartition" list, logged :845645).
2. **FS-grouped merge loop** (:842556-842745): buckets candidates into
   `map<FS, list>` (tree walk keyed on `*(int*)(info+8)`, :842574-842585),
   dedups pointer-identical infos via a seen-list (:842708-842713, splice
   :842728-842737).
3. **Top-2 tie rules** (:842752-842769): if `score(#1) - score(#2) <= 0.1` →
   drop #2 (mark `+0x80=1`, :842758-842759); if `<= 0.3 && (FS(#2) == 2 ||
   FS(#2) == 1)` → drop #2 (:842762-842768) — **NTFS outranks FAT at equal
   evidence**.
4. **Size-match promotion** (:842770-842796): first candidate at index ≥ 1 with
   `|totalSector - param_4| * 0x200 < 0x1000001` (±16 MiB+1 byte) is marked
   `+0x78=1, +0x80=1`, **logged as
   `"AdjustPartitionInfoOrder %p FileSystem %d StartSector:%I64d DataAreaStart:%I64d totalSector:%I64d fileCount:%I64d folderCount:%I64d dSuggestted%f"`**
   (:842782-842787) and spliced to the front of the results (LAB_140510a0e,
   :842842-842858).
5. **FAT multi-entry drop** (:842811-842821): FS ∈ {1,2} whose
   `CFATSearchedPartitionInfo` vector `(+0xb0 - +0xa8) >> 3 > 1` (the dynamic
   cast :842814-842816) is dropped.
6. **Final filter** (:842797-842831): everything marked `+0x80` is freed; the
   rest move to the output list.

**Result curation cap** (caller, :845665-845731): the primary result vector
accepts at most `2` candidates normally, `4` when flag `this+0x4c` is set,
`5` when a `+0x78`-marked (size-matched/promoted) candidate exists
(:845676-845685), `6` when flag `this+0x50` is set (:845691-845693); the
overflow goes to a secondary list (:845717-845731).

## 5. Ground truth: og-codebase/log/CalcPartition.log (vendor QA night)

Three rounds. Round 1 (I: scan, region 0-270977023): 79 candidates → 5 results.
Round 2: 101 candidates → 5 results. Round 3 (434 M-sector disk): 6 candidates
→ 5 results, with an explicit `AdjustPartitionInfoOrder ... StartSector:5216
... dSuggestted0.000000` promotion line; the 0-score candidate 5216 becomes
result #2, ahead of four 0.632748-scored ghosts.

**Formula validation against round 1** (NTFS: raw = folders*50 + files):
top candidate `files:3618 folders:3304` → raw = 168818; it is the scanned
partition itself (StartSec 0 + scan base 705794048 == I:'s start ⇒ +500, per
DRWUI.log "Quick scan StartSector=705794048") → raw_max = 169318,
ln = 12.0384. Then: cand(51 files) → ln(51)/12.0384 = 0.32666 (logged
0.326652 ✓); cand(2 files, 1 folder) raw 52 → 0.32829 (logged 0.328265 ✓);
cand(1 file, 1 folder) raw 51 → identical score to cand(51 files), logged
identical 0.326652 ✓; cand(0/0) with `+0x68 ≈ 60` → ln(60)/12.0384 = 0.34016
(logged 0.340154 ✓). All rows reproduce within print rounding (≤2e-5).

Extent semantics visible in the log: `EndSec = DataAreaStart + totalSec`
(e.g. round-1 cand39 FAT32 `StartSec:28836189 DataAreaStart:29308200 EndSec:
271449027 totalSec:242140827`) — **totalSector is the data-area length and the
candidate extent is anchored at DataAreaStart**, not at the partition start
(472011 sectors = the reserved+FAT region excluded from the extent).

Cross-check: `re-workspace/artifacts/config-files/qa-logs--session-forensics.md`
§3 ("candidate generation → scoring by recoverable files → order adjustment →
final set"; "15+ overlapping NTFS candidates"; "AdjustPartitionInfoOrder
resolution picking the final set") — matches the decode.

## 6. Per-FS boot/candidate gates (candidate generation side)

### FAT family — PartitionRecoveryD.mo (deep-scan gates)
`FUN_180004610` (PartitionRecoveryD_decompiled.c:2516-2606) — the flavor
classifier:

```c
spf = u16@+0x16 ? : u32@+0x24;  tot = u32@+0x20 ? : u16@+0x13;        // :2527-2538
if (u16@+0xB == 0 || byte@+0xD == 0) type = 0xFF;                     // :2539-2541
else if ((u16@+0xB & 0x1FF) == 0) {
  clusterCount = (tot - u16@+0xE - spf*byte@+0x10
                  - u16@+0x11*0x20 / u16@+0xB) / byte@+0xD;           // :2543-2546
  if      (clusterCount < 0xFF5)  { FAT12gate→1; FAT16gate→2; FAT32gate→3; }   // :2547-2562
  else if (clusterCount < 0xFFF5) { FAT16gate→2; FAT32gate→3; FAT12gate→1; }   // :2564-2579
  else if (clusterCount < 0xFFFFFF0) { FAT32gate→3; FAT16gate→2; FAT12gate→1; }// :2581-2596
  else type = 0xFF;
}
// pre-check FUN_180004ce0 (:2731-2790) matches the boot OEM against the
// literal string "-FVE-FS-" (bytes 2d 46 56 45 2d 46 53 2d at :2778-2785):
// BitLocker volumes classify as type 0xB and skip FAT classification (:2604-2606).
```

Classic FAT cluster-count brackets (`<0xFF5` FAT12, `<0xFFF5` FAT16, else
FAT32) with the expected flavor's gate tried first per bracket. The gates:

- **FAT12 gate** `FUN_1800048f0` (:2613-2644): numFATs(+0x10)≠0; some
  total≠0; SPC(+0xD) ∈ {1,2,4,8,0x10,0x20,0x40,0x80} (:2625-2629);
  `u16@+0x16 ∈ [1,200]` (small-FAT, :2630-2632); sig 0x55AA (:2634).
- **FAT16 gate** `FUN_180004a40` (:2650-2680): same minus the ≤200 bound
  (spf@+0x16 ≠ 0 only, :2666-2668).
- **FAT32 gate** `FUN_180004b70` (:2685-2726): `memcmp(boot+0x52,"FAT32   ",8)`
  (:2692); numFATs≠0; total32(+0x20)≠0; SPC ∈ {1,2,4,8,0x10,0x20,0x40,0x80}
  (:2700-2704); spf32(+0x24)≠0 (:2705); sig 0x55AA.

### FAT family — FATSearchDLL engine-open validator
`FUN_18002f970` (FATSearchDLL_decompiled.c:38037-38090):
- FAT16: `_strnicmp(boot+0x36,"FAT16   ",8)` (:38048); numFATs(+0x10) ∈ {1,2}
  (:38049); SPC(+0xD)-1 ∈ bit positions of mask `0x800000008000808b` OR spc==0x80
  → **SPC ∈ {1,2,4,8,16,32,64,128}** (:38050-38052); bps(+0xB) == sector size
  (512, :38053); root entries(+0x11) > 0xDF (≥224, :38054);
  `root_entries << 5 % bps == 0` (:38055); bytes/FAT(+0x16) ≠ 0 (:38056);
  sig (:38056-38057) → type 1, returns root-dir sectors (:38058-38059).
- FAT32: `_strnicmp(boot+0x52,"FAT32   ",8)` (:38062); numFATs ∈ {1,2} (:38066);
  SPC set as above (:38067-38072); bps == 512 (:38073-38075);
  spf32(+0x24) ≠ 0 (:38075); sig (:38076) → type 2 (:38084).
- else type = 0xE (:38088).

### NTFS — NTFSSearchDLL engine validator
`FUN_18003f480` (NTFSSearchDLL_decompiled.c:50996-51033):
`_strnicmp(boot+3,"NTFS    ",8)` (:50996); bps(+0xB) nonzero 512-multiple
(:50999-51000); SPC via `FUN_18003f5d0` (:51038-51050) handling the 0xF0|n
negative encoding (`1 << (-(char)spc & 0x1F)`), must be one of
{1,2,4,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000} (:51004-51008);
clusters-per-MFT-record (+0xE) zero-derived-or-encoded (:51001-51019);
MFT-record size (+0x40): `0 → 0x400` default, negative → 2^(-n), else
`val*bps*SPC` (`FUN_18003f630`, :51055-51073). PartitionRecoveryD's own NTFS
site is a plain `"NTFS    "` memcmp (:10346/:10365).

### exFAT / ext / FATX / Linux / BitLocker
exFAT: `CExFatBlockParser` (PartitionRecoveryD_decompiled.c:732). ext:
`CExtXBlockParser` (:1223; magic 0xEF53 in FileSystem.mo:4274 and
ExtFileSystemAnalyser.mo:3412). FATX: `CFatxBlockParser` (:3528). Linux:
`CBlockParserLinux` (:606). Win32/FAT: `CBlockParseWin32` (:661) +
`CFAT12Table`/`CFAT16Table`/`CFAT32Table` (:2990/:3087/:3271). NTFS:
`CNTFSBlockParser` (:9811). BitLocker: `BitlockerParser` (:721) + the
"-FVE-FS-" boot classification (:2778-2785, type 0xB). Scan-pass location
generators: `CGeneratorSetFast1MSearch` (:8664), `CGeneratorSetFastSearch`
(:8720), `CGeneratorSetCompleteSearch` (:6463), `CGeneratorSetRegSearch`
(:6558). **No Btrfs/HFS+/ZFS/ReFS block parser exists in PartitionRecoveryD**
(grep over the decompile: zero hits) — those FS types surface only in the
DRWUI enum and the per-engine `CSearched*PartitionInfo` subclasses
(CSearchedBtrfsPartitionInfo in BtrfsSearchDLL strings).

## 7. Core comparison (HEAD 34b3441)

- `src/core/lost/cr_candidates.cpp:61-158`: probes return **static boot-field
  evidence points** — NTFS OEM +15 (:69), valid BPB +25 (:73), $MFT liveness
  +30 (:84); FAT valid BPB +20 (:116) + FAT-area probe +10 (:121); exFAT OEM
  +15 (:135) + BPB +20 (:136-137); GPT +10+20 (:152-154). No recovered-content
  evidence anywhere; every candidate that survives dedup is materialized and
  scanned (`src/core/orchestrator/cr_scan.cpp:995-1020`) with **no cap and no
  promotion rules**; ordering is `std::sort` by score (`cr_candidates.cpp:305`,
  not stable).
- Dedup/overlap: same-FS nested containment, first-outer-wins
  (`cr_candidates.cpp:270-288`, probe-sized outers only post-GAP-S6-6
  :280-281), overlap penalties −20 inside / −10 overlapping (:289-304) —
  candidates inside known partitions are KEPT (penalized), never dropped.
- FAT gate: `fat::validate_boot` (`src/core/fat/cr_chain.cpp:261-319`): bps
  512..4096 pow-2 (:266-268), SPC any nonzero pow-2 (:270), **no numFATs
  check** (:274), no "FAT16  "/"FAT32  " name-string check, no root-entry
  floor, no spf≠0 gate (flavor pick at :286-293), `first_data_sector < total`
  (:305-306). Flavor by cluster count `< 4085 / < 65525`
  (`flavor_from_clusters`, :12-19) — **MATCH** with the og's 0xFF5/0xFFF5
  brackets (PartitionRecoveryD :2547/2564/2581).
- Probe dispatch (`cr_candidates.cpp:229-249`): NTFS / EXFAT / EFI PART /
  else-FAT. **No "-FVE-FS-" (BitLocker) branch** — a BitLocker boot sector
  fails the FAT catch-all and is dropped; no BitLocker deep parser.

## 8. Per-FS scoring/gate table (og vs core)

| FS | og candidate gate (constants) | og score weight (dSuggested raw) | core probe (points) | verdict |
|---|---|---|---|---|
| FAT12 | numFATs≠0, total≠0, SPC∈{1..128}, spf@+0x16 ∈ [1,200], 55AA (PRD:2613) | (files+folders)×50 (DRWUI:842217) | FAT catch-all: BPB 20 + FAT-probe 10 (cr_candidates.cpp:116-124) | DIVERGENT (gates+weights) |
| FAT16 | numFATs≠0, total≠0, SPC∈{1..128}, spf16≠0, 55AA (PRD:2650); engine-open: name "FAT16   ", numFATs∈{1,2}, bps==512, root≥224, root×32%512==0 (FATSDLL:38048-38057) | (files+folders)×50 | same | DIVERGENT |
| FAT32 | name "FAT32   "@+0x52, numFATs≠0, total32≠0, SPC∈{1..128}, spf32≠0, 55AA (PRD:2692-2711); engine-open adds bps==512, numFATs∈{1,2} (FATSDLL:38062-38076) | (files+folders)×50 | same | DIVERGENT |
| NTFS | OEM "NTFS    ", bps 512-mult, SPC powers {1..0x1000} incl. 0xF0-encoded, MFT-rec size enc (NTFSSDLL:50996-51073) | folders×50 + files (DRWUI:842221) | OEM 15 + BPB 25 + $MFT 30 (cr_candidates.cpp:61-107) | DIVERGENT (boot-points vs content-count) |
| exFAT | CExFatBlockParser (PRD:732); exFAT engine slot-4 geometry (exFATSDLL:28613) | folders×50 + files | OEM 15 + BPB 20 (cr_candidates.cpp:128-142) | DIVERGENT |
| HFS+ | no PRD block parser; enum 8 | files + folders×10 (DRWUI:842214) | MISSING (S-006) | MISSING |
| ext2/3/4 | CExtXBlockParser, magic 0xEF53 (PRD:1223; FileSystem.mo:4274) | folders×50 + files | MISSING (S-006) | MISSING |
| Btrfs | no PRD block parser; BtrfsSearchDLL carries CSearchedBtrfsPartitionInfo | folders×50 + files | MISSING (S-006) | MISSING |
| ZFS | no PRD block parser | folders×50 + files | MISSING (S-006) | MISSING |
| ReFS | no PRD block parser; enum 10 | folders×50 + files | MISSING (S-006) | MISSING |
| FATX | CFatxBlockParser (PRD:3528) | (folders×50+files via 'else') | MISSING (S-006 family) | MISSING |
| Linux (non-ext) | CBlockParserLinux (PRD:606) | else-branch | MISSING (S-006 family) | MISSING |
| BitLocker | "-FVE-FS-" OEM → type 0xB (PRD:2778) + BitlockerParser (PRD:721) | else-branch | MISSING — falls into FAT catch-all and is dropped (cr_candidates.cpp:243-248) | **MISSING (new — S-014)** |
| GPT hdr | (table layer, not CalcPartition) | — | "EFI PART" +10/+20 (cr_candidates.cpp:144-158) | og-side n/a in this pipeline |

## 9. What this resolves / corrects

- **OQ-T9 premise corrected**: ICalcPartition slot 4 is `GetSectorsPerCluster`
  of a lost-volume geometry calculator (FAT/exFAT only), not a confidence
  score. The session-2 machinery note stands mechanically (vtable slot 4,
  direct-field fast path) but its interpretation as "scoring" was wrong.
- **The og's actual scoring model** (A30) and **merge/curate strategy** (A29)
  are now fully decoded and log-validated — see findings S-014/M-013/M-014/
  N-027/N-028 filed this session.

---

## 10. Addendum — executed RE probe for the BitLocker boot signature (S-014)

    $ rg -a --byte-offset -o "\-FVE\-FS\- PartitionRecoveryD.mo
    172520:-FVE-FS-
    172536:-FVE-FS-
    $ rg -a --byte-offset -o "FVE-FS" Partition.mo
    218217:FVE-FS
    $ rg -a -c "FVE" BitLockerLib.dll
    2

The on-disk BitLocker OEM signature string is present in PartitionRecoveryD.mo
(twice — comparison literal + format template), Partition.mo, and
BitLockerLib.dll — corroborating the decompiled classifier
(`FUN_180004ce0`, PartitionRecoveryD_decompiled.c:2778-2785).
