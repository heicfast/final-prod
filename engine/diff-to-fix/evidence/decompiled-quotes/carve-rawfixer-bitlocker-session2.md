# Evidence — carve dispatch, signatures, RawFixer repair set, BitLocker (session 2)

Source: `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`,
`ghidra-partition/RawFixer_decompiled.c` (86,576 lines), `ghidra-core/DRWService_decompiled.c`,
`ghidra-mo/bitlockerhelp_decompiled.c`, `raw_signs.json` (og-codebase). Readers: agents S2-D2,
S2-F (scratch `S2-D2-context.md`, `S2-F-context.md`). Core: `e93decf`.

## Carve dispatch (OQ-T8) + bitmap gate (M-005)

```
FUN_18000c430 :9269-96  sectors/block = 0x400000/sector_size (4 MiB blocks, 8 MiB double
                        buffer + 0x1000 slack); per-block progress/cancel (:9623-27)
probing stride  :9496   candidate = buffer + half + ss*sector_index   // SECTOR-ALIGNED ONLY
bitmap gate     :9446-60  *param_4 != 0 → FUN_180058df0(bitmap, cluster) bit test
                        (:67842-56) → used: skip cluster (local_1e0 += spc-1), no dispatch
dispatch FUN_18000cf60 :9766-9842  config gate: config/config.ini "only_common"
                        :9843-885  u32 RB-tree exact find → analyser vtable+0x40 or +0x10
                        :9894-920  registry list sweep (accept requires computed size)
                        :9932-10063 u16 multimap equal_range walk
                        first-accept-wins per layer; no cross-layer resolution; per-key
                        modern→legacy chains (e.g. MP3 :37067, GZ :2783-93, MDB :2805-15)
```
Signature table: all session-1 S-007 formats + ~26 additional confirmed (bytes + lines in the
S-007 session-2 update); ISO9660 absent from og (core-only superset). raw_signs.json (139
extensions) parsed — none of the missing formats appear in it either (it is a data mirror, not
the engine registry).

## RawFixer 33-method repair set (S-008 update)

```
CRawFixer::Start / CheckPartition (:85643/:85701)    family dispatch/orchestration
CheckDBR (exFAT :9695) · CheckSector (:9853)
FixFromDBR  NTFS :65974  primary←backup; fallback = LAST sector of volume
                        (FUN_180063410 total-1 :66010-14)   [core volume-end MATCH]
            exFAT :9951  primary←backup-12
FixDBR (exFAT :10774)    in-place field fixes
UpdateDBR (exFAT :11322-11492)  11-sector template DAT_18009b040 + sector-0 overlay
                        (:11413-25); fresh checksum fill-repeated into sector 11
                        (:11429-34); read-back verify 24 sectors (:11440);
                        WriteSector 12 @LBA0 AND 12 @LBA12 (main+backup, :11450-66)
FixByScanSector (:9572)  deep DBR reconstruction from candidate boots (disk)
CheckMFT (NTFS :67805-67964)  bps gate; $MFT LCN 1-sector read; record parse+datarun;
                        spec-exact record sizing cpr<0→1<<(-cpr&0x1f) else cpr*bps
                        (:67878-87); MIRROR-BACKED MFT REPAIR (disk write, sentinel 0xFFFFFFFF)
IsValidRootDir (NTFS :68068) · CheckBackupDBR (FAT :60505, sector-6)
CheckFatTable (FAT :60891-61010)  FAT1 sanity; FAT2 mirror read at FAT1+spf
                        (u32@DBR+0x24); repair = WriteSector FAT2→FAT1 (:60968-81)
CheckRootDir (FAT :61228) · CheckCluster · ChecksumDBR (exFAT :10461-10554 — predicate ROL31+add skipping table DAT_180079a40 = {106,107,112}: MS-spec skip, core MATCH — corrected in fat-exfat-session2.md; the earlier no-skip reading and its N-017 candidate are RETRACTED)
TryFixFromSectorsPerCluster (:11201/:11223)  · CheckMustBeZero (:10912)
CheckRoot (exFAT :10168) · ReadCluster (:10297, post-repair verify)
CopySectors (:10637) + Undo on every path      [consent = pre-scan DRWUI phase, F63]
```

## BitLocker auto-unlock (S-014)

```
DRWService_decompiled.c:104   RegOpenKeyExA(HKLM,
                    "SYSTEM\\CurrentControlSet\\Control\\FVEAutoUnlock", ...)
DRWService.exe/strings_ascii.txt:32   (same path)
bitlockerhelp_decompiled.c:3128-55   snprintf("%s\\Windows\\System32\\config\\SYSTEM")
                    → hive check → "\ControlSet001\Control\FVEAutoUnlock\" → per-subkey
                    "%s%s\\Data" REG_BINARY        [offline hive — dead-system path]
RegLib_decompiled.c:203/275         OpenHiveByVolumeA/W
artifacts DRWService-exe--bitlocker-service.md §2-3   per-volume GUID subkeys; ≤100 records;
                    blob ≤ 256 B
Core: src/win/cr_bitlocker.cpp:219  RegOpenKeyExW(HKLM,
      L"SYSTEM\\CurrentControlSet\\Services\\FVEAutoUnlock")  [WRONG HIVE PATH]
      :176-236 enum_binary_blobs: recursive, every REG_BINARY ≥32 B, no GUID association,
      no caps; no offline-hive reader anywhere (grep clean)
```

## Scope/wiring confirmations (I-010 / S2-F sweep)

PID token `b0|(b1+b0)<<8|(b2+b1)<<16|(b3+b2)<<24` ↔ `compute_pid_token` `cr_transport.cpp:285-89`
(byte-identical) · 200-disk cap ↔ `cr_rda.h:66` · IOCTLs 0x60002400/04/08 ↔ `cr_rda.h:41-43` ·
driver-absent duality `Device_decompiled.c:7793-7822` ↔ `cr_transport.h:23-37` · FilterFolder
junk list (`filtering-priority--config-layer.md` §3; DRWUI log `PushFolderFilter:84`) → M-018.
