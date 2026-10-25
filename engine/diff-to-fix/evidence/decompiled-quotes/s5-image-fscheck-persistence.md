# Evidence — Session-5 disk-image scan + FS-analyser/check-phase + lostpart persistence + FATX decode

> Session 5 (2026-10-26). Audited core `7cf6754`; corpus `03ad46c`.
> Abbreviations: `DM:` = `re-workspace/reversed/ghidra-mo/DeviceManager_decompiled.c`;
> `Dev:` = `.../ghidra-mo/Device_decompiled.c`;
> `NTFSUtil:` = `.../ghidra-mo/NTFSUtil_decompiled.c`;
> `PRD:` = `.../ghidra-mo/PartitionRecoveryD_decompiled.c`;
> `FSC:` = `.../ghidra-mo/FileSystemCheck_decompiled.c`;
> `NFA:`/`FFA:` = `.../ghidra-mo/{NTFS,FAT}FileSystemAnalyser_decompiled.c`;
> `Log:` = `.../ghidra-mo/Log_decompiled.c`.

## A. Disk-image scan (the real capability — DRWUI + Device.mo)

- **CTbFile is NOT an image abstraction** (corrects mo-module-map.md:21 and
  module-internals-wave2--ldm-reglib-ctbfile.md:89-96): ctor `FUN_180047550`
  (DM:48133-48143) = {vftable; HANDLE; path; refcount}; open `FUN_180047610`
  (DM:48160-48245) = plain CreateFileW, **no extension/format check**; the
  tree's ONLY construction site `FUN_18004d410` (DM:51016-51086) reads
  **boot.ini** (`L"\\boot.ini"` DM:50993/51003, >0xa00001 reject :51051,
  INI parser `"boot loader"`/`"default"` :51120-51125). "Tb" = Todo-Backup
  lineage (Burn.dll's `CTbCreator<ITbFile,CBurnFile>` Burn:752/775).
- **The real image layer lives in DRWUI.exe**: extension-dispatch factory
  `FUN_1405b3410` (DRWUI_decompiled.c:948237-948409): `"edisk"`→FUN_1400dafc0
  (:948270-948278), `"vhdx"`→CVHDXDisk (:948293), `"vmdk"`→VmdkDevice
  (:948311), `"vhd"`→CVHDDisk (:948347); backend interface `IImageFile`
  (:948440); **unrecognized extension (.img/.dmg/raw) → flat-file fallback**
  CreateFileW + GetFileSizeEx (:948389-948393). Construction
  `FUN_140178250` (DRWUI:275744-275889): **reads LBA0 (0x1000 bytes) and
  classifies MBR/GPT** (:275877-275880). UI: "Import a disk image"
  (DRWUI:1212897/1213510); filter **`"*.vmdk *.vhd *.vhdx *.edisk *.dmg *.img"`**
  (DRWUI:1213924); telemetry `Select_diskImage` (:1213030). Compressed-image
  variant `CImageFileDeviceWithDecompress` (:993808); embedded VMDK parser
  `disk_vmdk::Vmdk` (:978187+, "Unsupported or corrupt VMDK (magic mismatch)"
  :978668; sparse-extent regexes `.*[sS](\d{3,})\.vmdk` :970533).
- **Engine-side VMDK block device**: Device.mo `FUN_1800545c0`
  (Dev:49106-49124): `libvmdk_handle_read_buffer_at_offset(h, buf,
  sector*0x200, count*0x200)` — 512-byte sectors; parent-chain support
  `FUN_180054120` (Dev:48965-49100: open_extent_data_files :49017-49023,
  recursive parent open :49062, set_parent_handle :49072). **Sole libvmdk
  importer: Device.mo** (Device_imports.txt: initialize/open_wide/
  open_extent_data_files/close/read_buffer_at_offset/get_media_size/
  get_utf16_parent_filename/set_parent_handle/error_backtrace_sprint).
- **Core side (LEAD-VERIFIED)**: `src/app/rrecov.cpp:495-506` — only
  `--disk N` (→open_disk_view :241) and `--volume X` (→open_volume_view
  :270); unknown options rejected :562-565. Transport opens only device
  namespaces (`\\.\PhysicalDrive%u` cr_transport.cpp:782-786,
  `\\.\HarddiskVolume...` :788-792). `IStorageDevice`
  (include/cr/storage/cr_device.h:89-98) has **no image/file-backed
  subclass**; zero vmdk/vhd/vhdx/edisk hits in src/include/tests.

## B. FileSystemCheck.mo — the chkdsk-grade audit layer (read fully)

**CCheckFAT::Check = `FUN_1800015b0`** (FSC:462) — 8-event host callback
(events: 3=SetRange :1648, 4=SetPos :1629, 5=OnError(code,extra) :1730,
6=continue/stop :1097, 7=status text :1669, 2=path :1751):
1. DBR per-type parsers (LEAD-VERIFIED first-hand, FSC:549-570 dispatch +
   error 0x100d): FAT12 `FUN_180002a60` (:1315): `"FAT1"+'2'` (:1325), SPC
   pow2 (:1327-1335), numFATs 1-4 (:1336), **cluster-count ceiling =
   sectorsPerFAT × 341.3333** (:1358-1360); FAT16 `FUN_180002c20` (:1372):
   ceiling = spf × 0x100 (:1415); FAT32 `FUN_180002dd0` (:1429):
   `"FAT3"+'2'` at +0x52 (:1440), spf16==0||==spf32 (:1454), rootCluster in
   (1, totSec/spc) (:1455), ceiling = (bps×spf32)>>2 (:1471).
2. FAT load + validation (:580-626): entry-hole check `FUN_180005fd0`
   (:4386-:4411: entry < clusterCount or >= reserved 0xff6/0xfff6/0xffffff6);
   **duplicate-reference scan** `FUN_180005e90` (:4337-4381: two FAT entries
   → same target cluster ⇒ fail). Fail → 0x1006.
3. Comparing FAT (:627-784): single-FAT → media byte in FAT[0]
   (0xf8/0xfff8/0xffffff8, :663-684); multi-FAT → **entry-by-entry compare of
   adjacent copies** (:689-781), mismatch → **0x1005** (:749).
4. Cluster-chain/dir audit (:789-1041): global visited array (:797-801);
   FAT32 root walker `FUN_1800027a0` (:1114: TBitmap **loop detection**
   :1174-1175; **cross-link** vs global array → callback(5,{0x1002,cluster})
   :1157-1167); recursive dir walker `FUN_180003360` (:1772: LFN 0x0F/
   deleted 0xE5 skip :1929, invalid first-cluster → 0x1007 :1941-1948,
   `.`/`..` validation :1893-1907, **file-size vs chain-length mismatch →
   0x1004** :2050-2076, **directory cycle → 0x1007** :2079-2097); post-walk
   **orphan scan**: unvisited clusters with FAT entry != 0 and != BAD
   {0xff7,0xfff7,0xffffff7} (:874-876) → host decision → **0x1003** (:949).

**CCheckNTFSVolume::Check = `FUN_1800045a0`** (FSC:2757): volume-size gate
0x1001 (:2832-2835); **$MFT** record 0 parse (:2876-2888), $FILE_NAME(0x30)
must be resident (:2900) + name == "$MFT" (:2909-2916) else 0x1008;
**$MFTMirr** same "$MFT" proof (:2921-2971) else 0x1009; **NTFS version gate**
(LEAD-VERIFIED first-hand, FSC:2994-3003): accepted set u32 {0x102, 3, 0x301}
(= 1.2 / 3.0 / 3.1) else **0x100a**; **$Bitmap** structural checks (:3788:
rejects $ATTRIBUTE_LIST :3820/:3942 + resident $DATA :3833, size sanity
0x100b :3875-3883); then `CollectTheLocationOfData` `FUN_180004d80`
(:3061-3249): walks **every allocated MFT record**, for every non-resident
datarun checks each LCN against (a) on-disk $Bitmap — free-but-allocated →
**0x1003 "Bitmap mark it as free!!!"** (:3196-3209); (b) a fresh bitmap —
already claimed → **0x1002 "Please use 'Chkdsk' first!!!"** (:3211-3224).
C++ exception → "The volume have some errors,please use Chkdsk to check,
and use Chkdsk /F to fix!" → 0x100e (:9295-9301).
The module launches `WinChkdsk.exe`/`x64\WinChkdsk.exe` as a child process
(launcher `FUN_180009090` :6956, CreateProcessA :7217).
Error taxonomy: 0x1001 size, 0x1002 cross-link, 0x1003 orphan/bitmap-free,
0x1004 size≠chain, 0x1005 FAT copies differ, 0x1006 chain/FAT invalid,
0x1007 dentry invalid, 0x1008 $MFT, 0x1009 $MFTMirr, 0x100a version,
0x100b $Bitmap size, 0x100d DBR, 0x100e exception.

**Core side (LEAD-VERIFIED)**: `src/core/repair/cr_rawfixer.cpp` —
`ntfs_check_mft` (:177; $MFT record-0 name + mirror-slot-0 restore;
IsValidRootDir report-only :203-223) and `fat_check_tables_and_root` (:273;
FAT mirror compare with entry-signature arbitration + sector-granular
torn-copy restore :302-359; CheckRootDir = 512-byte readability :363-380).
No FAT entry-hole/duplicate scans, no pairwise-copy reporting, no dir-tree
audit, no orphan scan, no NTFS version gate, no MFT×$Bitmap cross-check
(cr_bitmap.cpp used for grading only).

## C. FileSystemAnalysers — content-probing recognition

- Type-code enum (decoded): 1=FAT12, 2=FAT16, 3=FAT32, 4=NTFS,
  **0xb=BitLocker-FAT, 0xc=UDF**, 0xff=unknown.
- **CNTFSVolumeAnalyser::Analyze** `FUN_180005470` (NFA:4259): boot gates
  (bps multiple of 512 ≤0x1000 :4292-4299, SPC pow2 :4300-4307) → result 4
  (:4308); **backup-boot verification** (LEAD-VERIFIED first-hand,
  NFA:4309-4327): computes the volume-end backup sector index
  (`FUN_1800048f0`), reads sector idx+1, `memcmp` against the primary
  (:4321-4323), on match stores idx+1 as the verified location at +0x510
  (:4324-4327); **content probe** — reads the first 10 MFT records
  (`FUN_180005180` :4083-4134), validates each (FILE magic + USA count <0x11
  + USA offset in [0x2a, 0x2a+0xd7) :4038-4058), requires the **$Volume
  signature**: $STANDARD_INFORMATION flags & 6 == 6 (HIDDEN|SYSTEM,
  FUN_180001120 :105-:120) and $FILE_NAME == L"$Volume" (:121-123), extracts
  the volume label from $VOLUME_NAME (0x60) (:4363-4370).
- **CFatVolumeAnalyser::Analyze** `FUN_180002580` (FFA:1385): **UDF probe**
  (LEAD-VERIFIED first-hand, FFA:1413-1435): reads sector +0x100 and the
  sector at u32@+0x14, checks **"UDF" at [0x19..0x1b]** → **result 0xc**
  (:1434); FAT classifier `FUN_1800012e0` (:331): **BitLocker OEM
  "-FVE-FS-"** (u64 0x2d53462d4556462d, LEAD-VERIFIED :358) → **0xb**
  (:420); thresholds <0xff5 → FAT12-first, <0xfff5 → FAT16-first (:379/:391)
  with per-flavor validators (FAT12 :429 spc bitmask 0x800000008000808b :444
  + spf in [1,200] :448; FAT32 :487 requires "FAT32   " 0x2020203233544146
  :497 + spf32 != 0 :504 + 0x55AA); then **root-directory verification**
  (type 1/2 fixed-root scan; type 3 root-cluster chain walk + 8.3/LFN label
  extraction 0xE5→'#', `FUN_180002180` :1124) — the analyser does NOT
  require the "FAT12"/"FAT16" strings (lenient for damaged FATs, unlike
  FileSystemCheck :1325/:1382).
- **CExtVolumeAnalyser** superblock validator `FUN_180004670`
  (EXT:3404-3425): magic 0xef53, inodes/blocks/bpg/ipg nonzero,
  group-count arithmetic, blockSize==1024 ⇔ first_data_block==1.
- No damage grading in any analyser: output = type byte + geometry + label.

**Core side (LEAD-VERIFIED)**: `ScanController::probe_family`
(src/core/orchestrator/cr_scan.cpp:473-530) dispatches on boot bytes only
(OEM strings, 55AA+bps, magics); no MFT/root-dir content proof, no UDF
family; NTFS backup boot read only by the consent-gated rawfixer
(cr_rawfixer.cpp:589-600), never by the scanner (cr_bootstrap.cpp:329-376);
the FAT engine's backup-boot fallback is sector-6/FAT32-only
(cr_fat_quick.cpp:351-368).

## D. lostpart.dat / partinfo.dat — the og's lost-partition persistence

- Path builder `FUN_180025dc0` (PRD:23151-23166): module's own directory
  (GetModuleFileNameW + truncate at last `\`).
- Readers: `FUN_180023120` (PRD:21241-21284) reads 0x18-byte records
  (partinfo), magic gate u32=="Part"; `FUN_180023280` (PRD:21289-21332)
  reads **0x448-byte records** (lostpart), same magic gate.
- Writers: `FUN_180025f90` (PRD:23220-23271) appends one 0x18 record;
  `FUN_1800260f0` (PRD:23276-23327) appends one 0x448 record; **reset
  `FUN_180025e30`** (PRD:23171-23215, LEAD-VERIFIED first-hand): on first
  search use, `DeleteFileW` BOTH files (:23196-23198) then rebuilds partinfo
  from the live device (`FUN_1800264e0` :23453-23552) and appends each live
  partition as a 0x18 record.
- **Incremental scan append**: `FUN_180020a60` (PRD:20154-20238) builds the
  full 0x448 record per FOUND lost partition and appends it
  (LEAD-VERIFIED call site: the scan loop `FUN_180021080` at PRD:20409 —
  `if ((iVar1 == 0) && (iVar1 = FUN_180020a60(param_1,local_88), iVar1 != 0))
  break;`), then fires the host callback (:20231). Clone-interrupt checkpoint
  `FUN_180025c60` (PRD:23088-23121) appends a header-only record (only when
  cancelled or state==4, call site PRD:20079-20082).
- **Resume re-validation**: at search start `FUN_180021e90` (PRD:20742-20744)
  loads both files; `FUN_1800229c0` (PRD:20998-21039) compares persisted
  partinfo records against the freshly enumerated layout → "layout changed"
  flag; then each persisted lostpart record is re-applied (PRD:20748-20842):
  flag +0x14 != 0 → **re-reads the disk at the record's start LBA +0x28**
  (vtable read, PRD:20778) and re-validates via `FUN_180021080`, **dropping
  records whose {start +0x28, end +0x30, type +8} no longer match**
  (LEAD-VERIFIED first-hand, PRD:20820-20835).
- On-disk layouts (validated against the shipped files):
  partinfo.dat (24 B/record): +0x00 "Part" · +0x04 u64 start LBA (2048) ·
  +0x0C u64 size in sectors (0x0749cfc0) · +0x14 u32 MBR type (7=NTFS).
  lostpart.dat (0x448 B/record): +0x00 "Part" · +0x04 u32 part-number/max-slot
  · +0x08 u32 disk number · +0x14 u32 applied-flag (1=re-apply on load) ·
  payload CSearchedPartitionInfo at +0x18: +0x28 u64 start LBA (0x800=2048) ·
  +0x30 u64 end LBA (start+size-1; +1 if type==4 extended quirk,
  FUN_180002760:1035-1045; on-disk 0x0749d7bf = 2048+0x0749cfc0-1) · +0x38
  u32 bytes/sector (512) · +0x441 CHS-align bool · +0x444 u32 SPC (=8 → NTFS).
  CSearchedPartitionInfo setters PRD:502-559; type map 1=FAT12, 2=FAT16,
  3=FAT32, 4=NTFS (:9856), 8=exFAT (:803), 0xB=BitLocker (:66).
- **Core side (LEAD-VERIFIED)**: `cr_scan.cpp:1227-1238` saves one
  REC_PARTITION_CONTEXT per lost candidate ({start_lba, sector_count,
  fs_kind, score, kind=1}, codec cr_records.cpp:368-386), at end-of-scan AND
  mid-scan checkpoints (`emit_checkpoint_` :428-452, config-gated);
  `cr_scan.cpp:1293-1310` restores candidates **blind** — no boot re-read,
  no drop of stale records, no whole-disk layout snapshot equivalent
  (closest: single-volume identity card cr_records.cpp:307-310); candidate
  `evidence[]`/`size_from_device_end` not persisted (cr_lostpart.h:56-66).

## E. Logging seam (three og layers; core has none at engine level)

- **Log.mo**: 64 KiB shared staging mapping `EPM_{1E956C11-…}` (Log:159-176),
  named mutex `EPMLOG_{1BFCF65F-…}` (:193), file = `<exe-dir>\EPMLOG.log`
  (:604-621/723-743), **10 MB truncate-rollover** (nFileSizeLow > 0xa00000 →
  TRUNCATE, :71-73), line format `[%04d-%02d-%02d %02d:%02d:%02d:%03d]
  (T:%d/P:%d) msg\r\n` (:699); per-disk dump channel `<exe-dir>\HD%03d.dpt`
  (:1428-1479); optional fsclog.dll forwarder (:1758/:2063).
- **PubLog.dll**: InitLog(file, LOGLEVEL, LOGTYPE) — LOGTYPE 0 =
  CDirectLog `./log/DefaultLog.txt` (PubLog:371-381), LOGTYPE 1 = **2 MB
  ring** mapping `DRW_{1E956C11-…}` (:119-135); API Log/LogInfo/LogWarning/
  LogError/DebugTrace/ReleaseTrace.
- **Logger.dll**: singleton async queue + worker thread (Logger:1630-1696/
  1776/1815), UTF-16 append under LockFileEx (:1943-1950), **10 MB cap →
  keep-second-half compaction** (:1962-2015), process-name tag (:2020-2058).
- Who logs: PartitionRecoveryD.mo 3 events (scan START/END with disk/method/
  range/time + clone read error — PRD:21962, 22121, 20075); SearchDLLs: only
  FATSearchDLL (ReleaseTrace), NTFSSearchDLL + RAWSearchDLL (LogInfo) import
  PubLog; Btrfs/EXT4/HFSPlus/ReFS/Zfs/exFAT import none
  (ghidra-searchdll/*_imports.txt).
- **Core**: app-only `--log` FILE* → vfprintf (`src/app/rrecov.cpp:122-143`);
  src/core/** logs nothing.

## F. CFatxBlockParser = FAT12/16/32 boot parser — NOT Xbox FATX (S-006 remainder resolved)

- "Fatx" = FAT-x. FatLib.mo's `FATXDBR` is the classic FAT BPB (bps +0xb,
  SPC +0xd, reserved +0xe, numFats +0x10, root entries +0x11, total16 +0x13,
  SPF16 +0x16, total32 +0x20, SPF32 +0x24 — FatLib_decompiled.c:594-690).
  **The Xbox magic 0x58544658/"XTFA" appears NOWHERE in the corpus**
  (definitive negative grep over reversed/ + og-codebase/).
- Registration: CBlockParseWin32 ctor (PRD:646-684) chains
  CFatxBlockParser (:667) → CNTFSBlockParser (:675) → CExFatBlockParser
  (:681) → BitlockerParser (:685).
- **Accept criteria `CFatxBlockParser::parse` = `FUN_180006250`**
  (PRD:3553-3595) via classification `FUN_180004610` (PRD:2516-2608):
  1. BitLocker pre-gate: memcmp(boot+3, "-FVE-FS-", 8) → type 0xB
     (:2604-2606).
  2. spf = u16@+0x16 ? : u32@+0x24; totSec = u32@+0x20 ? : u16@+0x13
     (:2527-2538).
  3. REJECT if u16@+0xB (bps) == 0 or u8@+0xD (SPC) == 0 (:2539-2541).
  4. REJECT if (bps & 0x1FF) != 0 — **bps must be a 512-multiple (ANY
     512-multiple: 1536/2560/8192 accepted)** (:2542-2543, 2600-2602).
  5. clusterCount = (totSec − reserved − spf×numFats −
     ceil(rootEntries×32/bps)) / SPC (:2543-2546).
  6. Band → validator try-order: <0xFF5 → [12,16,32]; <0xFFF5 → [16,32,12];
     <0xFFFFFF0 → [32,16,12]; **≥0xFFFFFF0 → REJECT** (:2547-2598).
  7. Per-type validators (first pass wins): FAT12 `FUN_1800048f0` (:2613-
     2645): numFats≠0; NOT(total16==0 && total32==0); SPC ∈
     {1,2,4,8,16,32,64,128}; **SPF16 ≠ 0 AND ≤ 200**; 0x55AA. FAT16
     `FUN_180004a40` (:2650-2680): same minus the ≤200 cap. FAT32
     `FUN_180004b70` (:2685-2726): **memcmp(boot+0x52, "FAT32   ", 8)==0**;
     numFats≠0; u32@+0x20≠0; SPC ∈ pow2 ≤128; SPF32≠0; 0x55AA.
  8. Size sanity `FUN_1800041c0` (:2260-2310): firstDataSector < totSec
     else reject 0xfffffffa (:3569-3573).
- **Core gates for contrast (LEAD-VERIFIED first-hand,
  src/core/fat/cr_chain.cpp:261-319)**: bps must be 512-4096 AND power-of-2
  (:266-268 — rejects 1536/2560/8192); SPC pow2 but **unbounded** (:269-271
  — accepts 256+); no "FAT32   "@+0x52 label check; no numFats≠0 check
  (fat_area can be 0); no ≥0xFFFFFF0 cluster-count cap; flavor is
  deterministic `flavor_from_clusters` (<4085/<65525) with no band-ordered
  multi-try; 55AA + firstData<totSec MATCH (:262, :305).

## G. NTFSUtil.mo — virtual views are EPM conversion, not recovery (corpus correction)

- `CFatVirtualView` family lives in namespace `Ntfs2FATProc` (log string
  `"Ntfs2FATProc::CFatVirtualView::VirtualFormat"` NTFSUtil:34084; census
  :26464-:34130) — the EPM NTFS→FAT **conversion** engine:
  `_initFat32DbrProc` fabricates a FAT32 DBR from template DAT_1800d2000
  (jump 0xEB58 0x90 :34176-34177, OEM "MSWIN4.1" :34180, media 0xF8
  :34186); `CFatTableProc::FatProc` **writes** FAT chains (:32924);
  `CFatVirtualFileItem::TransferFromNtfsItem` (:29731-29842) with
  decompress/decrypt (:28199/:28288) and `_resolveConflict` (:31205).
  Damaged-FAT *reading* is absent — it is a write path.
- `CFirstVolumeMergeDeal` (ctor FUN_18002bb90 → base CVolumeMergeDeal
  FUN_18007cf10, NTFSUtil:97149-97226; merge body FUN_18002be00 :34519)
  merges **partition B into partition A** for the EPM CMergePartition
  pending-op (siblings CSecondVolumeMergeDeal :89583, CNewVolumeMergeDeal
  :37994, COldVolumeResizeMoveDeal/CResizeMoveAlgorithm) — not split-volume
  recovery. Corrects mo-module-map.md:35/:54-55.
- LdmManager.mo `EditLdm2*` (write-side: MoveResizeVol ~:51370-56495,
  UpdateLdmData :59164, ApplyConfig :84393-84633, DeleteDisk :84818,
  DeleteVolume :84994; commit-sequence/VBLK logging :52807-52832) —
  partition-management mutators; **no binary outside LdmManager.mo
  references EditLdm2** (tree-wide grep). Read-side Dmrp* (incl.
  DmrpVolume::RepairRaid5 :46357) is the recovery-relevant surface.
