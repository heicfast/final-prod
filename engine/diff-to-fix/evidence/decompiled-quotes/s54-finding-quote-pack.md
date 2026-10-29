# S54 decompiled-quote pack — the lead-verified evidence for the session-54 filings

Every quote below was re-read first-hand by the lane lead against corpus `be0cfe9`
(decompile paths relative to `re-workspace/reversed/`) and core `34d6cf10`, in
addition to the agents' context sheets (`s54-deepdive-*.md`). This file is the
citation pack for S-063..S-066, M-138, M-141..M-146, N-187, N-188, N-193..N-199, I-045 (the S-062/M-139/M-140/N-189..N-192 seats renumbered at the push-rebase per SA-9), and the three
addenda (M-126 / N-178 / N-186).

## S-066 (renumbered from S-062 at the push-rebase) — exFAT lost-anchor re-base (FUN_180001630)

`ghidra-searchdll/exFATSearchDLL_decompiled.c:341-420` — the recognition ladder,
re-read in full:

- :354 the main gates (`FUN_180021e80(param_3, iVar1)`)
- :357-359 the backup read at `*(param_5 + 2) + 0xc` and the FULL gate re-run
  (`FUN_180021e80(0x180061280, iVar1)`)
- :366 the media-word test `if ((DAT_180061280 != -8) || (DAT_180061284 != -1))`
  (FAT[0] != 0xFFFFFFF8 || FAT[1] != 0xFFFFFFFF)
- :371-374 the `base >= 0xc` guard and the shifted probe
  `(*(ulonglong *)(param_5 + 2) - 0xc) + (ulonglong)*(uint *)(param_3 + 0x50)`
- :377-378 the re-base store `*(longlong *)(param_5 + 2) = … + -0xc;`

Core absence: `src/core/exfat/cr_exfat_dentry.cpp:104-197` (validate_boot_region —
no probe, no correction; `:174-177` the informational `boot_backup_ok`).

## S-063 — NTFS $MFTMirr bootstrap fallback (FUN_1800272b0 + getters)

`ghidra-searchdll/NTFSSearchDLL_decompiled.c`:

- :51179-51183 `FUN_18003f990` → `return *(undefined8 *)(param_1 + 0x30);` — $MFT LCN
- :51170-51174 `FUN_18003f970` → `return *(undefined8 *)(param_1 + 0x38);` — $MFTMirr LCN
  (adjacent qwords; the kernel-layout boot struct — total_sectors@+0x28, $MFT@+0x30,
  $MFTMirr@+0x38, cpr@+0x40, cib@+0x44)
- :31616-31627 the primary bootstrap: `lVar11 = FUN_18003f990(…)`, read at
  `lVar11 * uVar3 + lVar2`; `if (iVar5 == 0) goto LAB_180027467;`
- :31595-31609 the fallback (LAB_180027467): `lVar11 = FUN_18003f970(…)`, the SAME
  reader `FUN_18001a3c0((ulonglong)uVar3 * lVar11 + lVar2, …)`,
  `if (iVar5 != 0) goto LAB_1800274f5;` else `uVar8 = 0x610016;`
- corroborating artifact: `artifacts/SearchDLL/ntfs-engine-internals.md:30-33`
  ("MFT LCN, MFT mirror LCN … `$MFTMirr` — mirror fallback")

Core refusal: `src/core/ntfs/cr_bootstrap.cpp:410-421` (the "nothing to mirror FROM"
comment — the comment's premise is refuted by the mirror's record-0 being the
bootstrap record itself).

## S-064 — FAT string-driven flavor (FUN_18002f970)

`ghidra-searchdll/FATSearchDLL_decompiled.c:38044-38062`:

```c
iVar4 = _strnicmp((char *)(*param_1 + 0x36),"FAT16   ",8);
if ((iVar4 == 0) && (…numFATs/spc/bps/root gates…)) { *(undefined4 *)(param_1 + 1) = 1; … }
iVar4 = _strnicmp((char *)(*param_1 + 0x52),"FAT32   ",8);
```

No cluster-count term anywhere in the function (the bracket FUN_180004610 is the
lost-scan side). The consumer FUN_180021000 bails only on flavor 0xE (:26318).

Core: `src/core/fat/cr_chain.cpp:609` (`flavor_from_clusters`) + `:646-666`
(the per-bracket switch; the Fat12 case empty by design) + `:184-216`
(fat_table_validate — Fat12 media check family-tolerant).

## S-065 — ext4 indirect ladder unbounded (core) vs size-bounded (og)

Core: `src/core/ext4/cr_ext4_engine.cpp:445-475` (read_ladder — no size/budget
term; the `budget = 65536` at :426 gates only `walk_extent_node`).

Og: `ghidra-searchdll/EXT4SearchDLL_decompiled.c:3949-3954`:

```c
while( true ) {
  …
  if ((iVar2 == 0) || (param_4 <= lVar9)) break;   /* size bound */
  …
  lVar9 = lVar9 + iVar2;                            /* += block_size */
```

and FUN_1800068b0 :5021-5024 (`lVar16 < size` content bound).

## M-138 — the 0x400/0x1000 zero-defaults + the D-52 premise

`ghidra-mo/NTFSFileSystemAnalyser_decompiled.c:3427-3456`:

```c
/* FUN_180004830 (record size) */   bVar1 = *(byte *)(param_1 + 0x40);
  if ((char)bVar1 < '\0') return 1 << (~bVar1 + 1 & 0x1f);
  if (bVar1 == 0) return 0x400;
  return spc * bps * bVar1;
/* FUN_180004870 (index block) */   bVar1 = *(byte *)(param_1 + 0x44);
  … if (bVar1 == 0) return 0x1000; …
```

Plus NTFSSearchDLL FUN_18003f630 (0x400) and PartitionRecoveryD FUN_180011a10
(:10231-10247, 0x400) / FUN_180011a90 (:10254-10268, 0x1000). The NTFSLib
`:19315-19348` store-guard structure (both paths → `goto LAB_18001683f` at :19340,
the shared `return extraout_EAX` exit at :19348-19353; the only error-string paths
are OEM/bps-mask/mft-product at :19290-19310 + :19344) — D-52's cited rejection is
a misread of these guards.

## M-145/M-146/M-141 (the first two renumbered from M-139/M-140) — the analyser gate family, LoadNtfsVolume's mft gate

`ghidra-mo/NTFSFileSystemAnalyser_decompiled.c:3568-3577` (the full conjunction —
OEM `0x202020205346544e`, bps `& 0x1ff`, the zero sweeps +0x0E..0x17/+0x20..0x23,
`spc != 0` raw — NO pow2, NO 55AA); FUN_180005470 :4292-4307 (record 512-multiple
≤ 0x1000; index 4096-multiple — the +0x40/+0x44 getters, NOT bps/spc: the
cr_scan.cpp:770-779 comment misattributes them).

`ghidra-mo/NTFSLib_decompiled.c:19296-19301`: `if ((local_104b & 0xff) * local_1028
< local_1030) {` — raw spc × $MFT LCN < total_sectors (the only mft bound; no
mirror bound in any og layer).

## M-142 — HFS+ VH validator (FUN_18000c6e0)

`ghidra-searchdll/HFSPlusSearchDLL_decompiled.c:9900-10020` — the copy loop (4 ×
0x10 qwords from param_1 to &local_218 = 512 bytes), the frame-offset map
(uStack_20c=+0x0C journalInfoBlock, uStack_1f0=+0x28 blockSize, uStack_1ec=+0x2C
totalBlocks, local_1e8=+0x30 freeBlocks), the selective bswap32 band, and the gate
(:10005-10006):

```c
if ((((local_218 == 0x482b) || (local_218 == 0x4858)) && ((uStack_1f0 & 0x1ff) == 0)) &&
   (((uStack_216 < 0x11 && (local_1e8 <= uStack_1ec)) && (uStack_20c <= uStack_1ec)))) {
```

Core: `src/core/hfsplus/cr_hfsplus_engine.cpp:574-578` (the fork clamps).

## M-143 — ext4 journal inode-image scan (FUN_180005e50)

`ghidra-searchdll/EXT4SearchDLL_decompiled.c:4414-4424` (quoted in the finding);
the feed FUN_18000e5b0 :12230-12250; the finalize consumer FUN_1800041e0
:2982-2993. Core: `cr_ext4_engine.cpp:867-925` (parse_dir_block only).

## M-144 — ext4 GDT caps

Og: `EXT4SearchDLL_decompiled.c:3404-3408` (round_up allocation, no cap).
Core: `cr_ext4_engine.cpp:160-162` (MAX_GROUPS 1<<20) + `:718-722` (8 MiB).

## N-187/N-197 (N-197 renumbered from N-190) — PartitionRecoveryD candidate validator + the spc decode forms

`ghidra-mo/PartitionRecoveryD_decompiled.c:9880-9914` (FUN_180011150: boot-pair
FUN_180011260 → FUN_180011560 → the 0xfffffff1 reject) and :10023-10075
(FUN_180011560: `local_70 << 2` = 4 records; the FUN_180011b10 primary /
FUN_180011b30 mirror retry).

`ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_18003f5d0 (the 0xF0-nibble
shift form, quoted verbatim in N-190) vs NTFSLib :19307 (`< 0x81`) and the
analyser (`0x80 <`).

## N-198/N-199 (renumbered from N-191/N-192) — exFAT og gates + ReFS FSRS

`exFATSearchDLL_decompiled.c:28180-28210` (FUN_180021e80's full `!= 0` gate set —
quoted in the finding). `ghidra-mo/Partition_decompiled.c:1745-1759`
(`0x53466552` @+3 AND `0x53525346` @+0x10, length @+0x18).

## I-045 — ext4 ee_len raw multiply

`EXT4SearchDLL_decompiled.c:4044-4048` (the `* (longlong)*(int *)(param_1 + 0x58)`
raw multiply; no 0x8000/32768 test anywhere in the og's extent decoder).

## The addenda quotes

- M-126's seed: `DRWUI_decompiled.c:315150` — `total_bytes / 60 MiB + 60 s`
  (the S54 orchestration sheet, Axis 3, carries the derivation).
- N-178's pause type-gate: `DRWUI_decompiled.c:314405/:314476` — the `+0xD8 != 1`
  skip (the +0xD8 writer is the recorded next probe).
- N-186's v4 tail: `include/cr/session/cr_records.h:110-122` + the writer in
  `src/core/session/cr_records.cpp` (~:855) + the v3/v4 pins at
  `tests/unit/test_image.cpp:806-812`.
