# S69-C context sheet — sector/LBA/calculation arithmetic fresh eyes

Agent C of lane S69 (Task ID: S69-C), read-only. Corpus pin `be0cfe9`, core pin
`3799dee2`. Paths: EXT4SearchDLL/FATSearchDLL/NTFSSearchDLL =
`re-workspace/reversed/ghidra-searchdll/*_decompiled.c`; PRD =
`re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c`; ExtAnalyser =
`ghidra-mo/ExtFileSystemAnalyser_decompiled.c`; FileSystem =
`ghidra-mo/FileSystem_decompiled.c`.

## 1. EXT4SearchDLL — ext2/3 vs ext4 classification

**Premise correction: the SB feature flags live at compat=+0x5C, incompat=+0x60,
ro_compat=+0x64, s_rev_level=+0x4C (not 0x230/0x240 — those offsets do not
exist in this file).**

**a) Superblock validator `FUN_180004710` (EXT4SearchDLL:3214-3247):**
```c
3222:  uVar2 = 0xef53;
3223:  if (((short)param_1[0xe] == -0x10ad) && ((*(byte *)(param_1 + 0x18) & 1) == 0)) {   // SB+0x38 magic==0xEF53; SB+0x60 incompat bit0 (COMPRESSION) clear
3224:    uVar1 = param_1[1];                                                                  // SB+0x04 s_blocks_count(lo)
3225:    if ((uVar1 != 0) && (((*param_1 != 0 && (param_1[8] != 0)) && (param_1[10] != 0)))) { // inodes_count, SB+0x20 bpg, SB+0x28 ipg != 0
3226:      uVar3 = param_1[5];                                                                // SB+0x14 s_first_data_block ∈ {0,1}
3232:      uVar3 = 0x400 << ((byte)param_1[6] & 0x1f);                                        // blocksize = 1024 << s_log_block_size(SB+0x18)
3234:      lVar4 = CONCAT44(param_1[0x54],uVar1);                                             // 64-bit: s_blocks_count_hi(SB+0x150):s_blocks_count
3238:           (uVar2 = lVar4 / (longlong)(ulonglong)(uint)param_1[8], ...                   // group_count = blocks / blocks_per_group
3240:          return CONCAT71((int7)(uVar2 >> 8),1);                                        // (group_count<<8)|1
```

**b) The only feature-flag consumers (SB copied to ctx at +0x98; ctx+0xf4 =
SB+0x5C compat, ctx+0xf8 = SB+0x60 incompat):**
```c
3365:        if ((*(byte *)(param_1 + 0xf4) & 4) != 0) {                 // compat & HAS_JOURNAL(0x4)
3366:          *(undefined4 *)(param_1 + 0x6c) = *(undefined4 *)(param_1 + 0x178);  // s_journal_inum (SB+0xE0)
3368:        if ((*(byte *)(param_1 + 0xf8) & 0x10) != 0) {              // incompat & META_BG(0x10)
3369:          uVar17 = (uint)*(ushort *)(param_1 + 0x196);              // s_desc_size (SB+0xFE) ... clamp 32..64 (:3397-3405)
3407:          _Var19 = ... ceil(groups*desc_size/blocksize)*blocksize   // GDT buffer
```

**c) ext4-ness decided per-inode, not per-SB** — `i_flags & 0x80000`
(EXT4_EXTENTS_FL) selects extent-tree vs indirect-block parsing (10 sites:
3944, 3955, 4419, 4447, 5026, 5045, 12190, 12221, 12288, 12317):
```c
3955:      if ((*(uint *)(param_2 + 0x20) & 0x80000) == 0) {
3956:        uVar3 = FUN_180005190(param_1,param_2,lVar4);      // classic indirect blocks
3959:      else { lVar4 = FUN_180005340(param_1,(short *)(param_2 + 0x28),lVar4,'\0'); }  // extent tree
```

**d) THE ext2-vs-ext3/4 RULE — PRD `FUN_180002b90` (PRD:1273-1293):**
**type 5 (ext2) iff `!(compat&HAS_JOURNAL) && journal_inum==0`; type 6
(ext3/4) iff `compat&HAS_JOURNAL && journal_inum!=0`; MIXED → REJECTED**
(`param_2+0x45c` = SB+0x5C compat, `param_2+0x4e0` = SB+0xE0 journal_inum).
Related validators: `ExtFileSystemAnalyser_decompiled.c:3404-3431`,
`FileSystem_decompiled.c:4266-4293`, both reading `ro_compat(SB+0x64) & 0x200`
(BIGALLOC) → cluster size = `blocksize << s_log_cluster_size(SB+0x1C)`.

## 2. PRD — partition-size calculation (CalcPartition family)

No MBR-entry walk in PRD (that lives in `Partition_decompiled.c:988-1041`,
`Device_decompiled.c:48464+`). PRD derives partition size from FS metadata
into a 5-field info struct (setters PRD:502-561: +0x00 start, +0x08
size-sectors, +0x10 aux size, +0x18 FS-type id, +0x1c sectors-per-unit,
+0x20 sector size).

**Ext size** — `FUN_180002ff0` (PRD:1386-1402) + `FUN_180003090`
(PRD:1407-1414), called from the ext analyzer at PRD:1276-1293:
```c
1393:  uVar1 = 0x400 << ((byte)*(undefined4 *)(param_2 + 0x18) & 0x1f);  // blocksize = 1024 << s_log_block_size
1399:    return (ulonglong)uVar1 / (ulonglong)uVar3;                     // / device sector size → sectors per block
1401:  return (ulonglong)uVar1 / 0x200;                                  // default 512B sectors
1412:  uVar1 = FUN_180002ff0(param_1,param_2);
1413:  return *(int *)(param_2 + 4) * (int)uVar1;                        // TOTAL SECTORS = s_blocks_count(SB+0x04) × sectors_per_block
```
**NOTE: PRD's ext size uses 32-bit `s_blocks_count` only (`*(int *)(param_2+4)`)
— NOT the 64-bit CONCAT44 form EXT4SearchDLL:3234 uses.** Sibling analyzers:
NTFS (type 4) `FUN_180011010` PRD:9856-9865 → size = `*(u64*)(boot+0x28)`
TotalSectors (`FUN_180011bb0` PRD:10315-10319); exFAT (type 8)
`FUN_180001f50` PRD:801-808 → VolumeLength (boot+0x48); FAT `FUN_180006250`
PRD:3575-3585 → `FUN_1800044e0` (PRD:2446-2456): totsec32(boot+0x20) else
totsec16(boot+0x13), data-area consistency `FUN_1800041c0` (PRD:2287-2289:
`reserved + nfats×fatsz + (root_entries×32 + bps − 1)/bps < total`).

**CHS→LBA generator** `FUN_18000ed90` (PRD:8561-8580), fed synthetic
geometries {255,63}/{240,63} at PRD:20514-20515 and a 1 MiB-step variant
{1, 0x100000/sector_size} at PRD:20639:
```c
8567:  local_18 = *(longlong *)(param_1 + 0x20) * *(longlong *)(param_1 + 0x28) +   // LBA = cylinder × sectors_per_cylinder
8568:             *(longlong *)(param_1 + 0x30) * (ulonglong)*(uint *)(param_1 + 0x1c); // + head × spt (track-start stepping)
8576:  if ((local_18 < *(ulonglong *)(param_1 + 8)) || (*(ulonglong *)(param_1 + 0x10) < local_18)) {
8577:    local_18 = 0xffffffffffffffff; }                                             // out of range → -1
```

## 3. FATSearchDLL — DOS timestamp→epoch

**No hand-rolled 1980 arithmetic — delegated wholesale to Win32
`DosDateTimeToFileTime`, zero-on-failure.** Main site FAT:32133-32154 (dir
entry: CrtDate@+0x10/CrtTime@+0x0E → +0x25; WrtDate@+0x18/WrtTime@+0x16 →
+0x2d; LstAccDate@+0x12 with time=0 → +0x35). Second site `FUN_180032b80`
(FAT:40828-40872, LFN skip loop then :40858-40863). On API failure the
FILETIME is zeroed (FAT:32137-32139, :32146-32148, :32152-32154,
:40861-40864). Invalid DOS dates (month 0, day 0, year>127) surface as
epoch-zero timestamps — no clamps anywhere.

## 4. NTFSSearchDLL — MFT record-size / record-count arithmetic

**a) MFT record size from BPB+0x40 — `FUN_18003f630` (NTFS:51055-51073):**
```c
51060:  if (((int)*(char *)(param_1 + 0x40) & 0x80U) == 0) {
51061:    if (*(char *)(param_1 + 0x40) == '\0') { iVar1 = 0x400; }                        // default 1024
51065:    else { iVar1 = (int)*(char *)(param_1 + 0x40) * (uint)*(ushort *)(param_1 + 0xb) // clusters_per_record × bps(0x0B)
51066:            * (uint)*(byte *)(param_1 + 0xd); }                                      //   × spc(0x0D)
51069:  else { iVar1 = 1 << (~*(byte *)(param_1 + 0x40) + 1 & 0x1f); }                     // negative → 1<<(−value)
```
Used at NTFS:51117-51118 (record size in sectors) and NTFS:31618-31621
(validated even && ≤0x10, min 1 — the N-196 walk gates), MFT walk starts at
`$MFT_LCN(SB+0x30) × spc + part_start` (NTFS:31628).

**b) Damaged-$MFT record-count estimate — `FUN_18000ecb0` (NTFS:11274-11277):**
```c
11274:    lVar9 = (**(code **)(**(longlong **)(param_1 + 0xb8) + 0x18))();     // device size
11275:    lVar15 = (lVar9 - *(longlong *)(lVar15 + 0x10)) / (longlong)uVar11;  // record_count = (device_end − $MFT_start) / record_size
11276:    puVar10[0x15] = lVar15;                                              // store record count
11277:    puVar10[0xc] = uVar11 * lVar15;                                      // $MFT byte size = record_size × record_count
```
Second extension site NTFS:10727 (`file_size / record_size` added to the
MFT-end estimate; default record size 0x200 if 0). Cluster-count sibling at
NTFS:31568: `*(u64*)(param_1 + 0xd0) = *(u64*)(param_1 + 0x88) / spc`.

## Follow-up candidates for lead verification
1. The ext2-vs-ext3/4 journal rule (PRD:1273-1293, mixed → rejected) vs core's
   ext classifier — OQ-S52-1 closure + possible probe-acceptance finding.
2. PRD's ext size uses 32-bit s_blocks_count only (PRD:1413) vs EXT4SearchDLL's
   64-bit (EXT4SearchDLL:3234) — check which core uses in its lost-partition
   ext size derivation.
3. FAT timestamp zero-on-failure vs core's hand-rolled conversion clamps.
4. Damaged-$MFT record-count estimate (device_end−$MFT_start)/record_size vs
   core's MFTMirr fallback sizing.
