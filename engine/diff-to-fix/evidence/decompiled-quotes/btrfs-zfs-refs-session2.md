# Evidence — Btrfs/ZFS/ReFS session-2 quotes (OQ-T6 resolution + profile/gate deltas)

Source: `re-workspace/reversed/ghidra-searchdll/{BtrfsSearchDLL,ZfsSearchDLL,ReFSSearchDLL}_decompiled.c`
(corpus `03ad46c`). Reader: agent S2-C2 (scratch `S2-C2-context.md`). Core: `e93decf`.

## ZFS (OQ-T6 resolved)

```
FUN_180018220(dev, &buf, 0x4000, 0x1c000)   :21052   label nvlist: 0x1C000 bytes at 0x4000
FUN_180018220(dev, &buf, 0x20000, 0x20000)  :21365   uberblock ring: 128 KiB at 0x20000
  → NO L1/L2/L3 constants anywhere; front-label region only
:21089-90   top-level key match: ONLY "vdev_tree" (type 9)
:21114-17   inside: ONLY "id" (type 2, BE-accumulated :21138-41)
  → NO version/name/pool_guid/ashift/asize label reads
uberblocks   slot stride 0x400; magic LE u64 0xbab10c; txg@slot+0x10; newest-txg-wins
             (:32321 param_1[2] < local_428); no ub_version gate; fixed 1 KiB geometry
multi-vdev   FUN_1800168b0 :19471 registry search by "id"; NO MATCH → returns FIRST entry
             (:19544-56) — never fails; mirror id 0 → local read OK; stripe → local-half
             fallback; raidz → checksum fail → drop
DVA math     :17996  asize = (dva_w1 + 0x2000) * 0x200   (offset + 4 MiB vdev skip)
checksum     :18093-811 fletcher4 4-accumulator; :18117 32-byte memcmp; gate type ∈ {7,0}
             :18129 fail → retry next DVA (index < 2); :18139-41 else drop
             NO sha256/fletcher2 constants; else-branch = BPE assembly FUN_180014bf0 (:18326,
             words minus #6/#10 — matches core bpe_payload cr_zfs_engine.cpp:127-31)
LZ4 only     FUN_180014200 :17838  comp==2 passthrough; comp ∈ {0xF, 0} LZ4 BE-u32 length
             (:17870-76); no LZJB/ZLE/GZIP
names        :24985/:25014 MultiByteToWideChar(0xFDE9 /*CP_UTF8*/, 0, …)  (S-005 ZFS anchor)
enumeration  "root_dataset" zap (:19934/:19983); $ORIGIN/SA registry keys; ZfsDecider
             from_txg/birth_txg grading (:2163-65, :2607-16); "zfs_debug.txt" side file (:8252)
```
Core refusal: `cr_zfs_engine.cpp:408-410 if (vdev != 0) return false;` — no fallback; no
checksum verify; no DVA retry (all reads `read_block(bp, 0, …)`).

## Btrfs (M-020 + re-confirmations)

```
SB scan FUN_18000d540 :10269-10326  device gate 0x10fff < size*sector; mirror table
             :10296/:10316; per-mirror bounds; magic u64@+0x40 == 0x4d5f53665248425f (:10298);
             sectorsize pow2 (:10299-300); nodesize band [0x86, 0x10000] (:10301 — N-006:
             upper bound inclusive, band otherwise as filed); newest-generation wins
             (:10303-04); mirror-0 magic miss ABORTS the loop (:10311-12 — I-003 confirmed)
chunk :2145-2169  type bits &1 DATA/&4 SYSTEM/&2 METADATA; RAID10 gate sub_stripes==0 ||
             num_stripes < sub_stripes → reject (:2162-68)
      :2498-2543  profiles: DUP &0x20, RAID0 &8, RAID1 &0x10, RAID10 &0x40; 2-copy rotating
             mirror index *(ushort*)(map+0x20), index = (index+1) % num_stripes (:2534-37);
             physical = stripes[index].offset + (logical - chunk) (:2540)
default subvol :3975-4053  name-hash 0x8dbfc2d2 "default", name_len 7, key type 0x84
             (:4019); fallback root 5 (:4060)
fs-tree :3760-3834  key-type dispatch (0x18 dir-item, 0x6c EXTENT_DATA, REG/PREALLOC
             type-1<2 :3790); no crc32c immediates in either implementation (module-file
             phrasing corrected); BtrfsCache.txt mmap cache (:1991)
```

## ReFS (N-026 + N-007/I-004 re-confirmations)

```
DBR gate FUN_180012390 :13511-66  "ReFS"@+3 (0x53466552), "FSRS"@0x10, major==3 &&
             minor > 3 (3.4+ ONLY); bps ∈ {0x100..0x1000} (:13544-45); cluster gate
             spc*bps ∈ {0x1000, 0x10000} ONLY (:13544-46)
open    FUN_180004980 :2410-96  reads 0x1000 at 0 (:2425); metadata base = spc*0x1e (:2460);
             minor-class dispatch 3/7/8/9 (:2466-76)
pages   FUN_180008620 :5802-90  512-byte stepping; id-page "ReFS+FSRS" (:5855-57) else
             SUPB 0x42505553 (:5865) / CHKP 0x504b4843 (:5870)   [core constants MATCH]
        FUN_18000fc80 :11659-737  size ≥ 0x20; types 1/2/3; FIRST-DWORD compare only
             (:11701 — N-007 confirmed verbatim); ref parse page+8, size-0x20 (:11715)
chkp    FUN_180012650 :13663-810  page-type-2; DYNAMIC body offset = *(uint*)(pagehdr+4)
             (:13743); version gate body+4==3 && 3<body+6 (:13744-45); body size 0x58 if
             minor ≥ 0xE else 0x44 (:13754-57); entry array after body (:13761-803)
             [core hardcodes body=0x50/0x56/0xA8 — cr_refs_engine.cpp:546/:567]
second copy  og: mid-volume formula ((pos/unit) >> 1) - 2 (FUN_180011e90 :13244, :2360)
             [core probes START + TAIL n-3/n-2 — cr_refs_engine.cpp:461-79]
checkpoint selection  newer-clock puVar6[2] < puVar7[2] (:1029-31) [core MATCH :523-24]
class census  no allocator/bitmap/deleted-grading classes in ReFSSearchDLL
```
