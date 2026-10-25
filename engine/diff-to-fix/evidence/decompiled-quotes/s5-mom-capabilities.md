# Evidence — Session-5 MOM/product-capability decode (decompiled quotes)

> Session 5 (2026-10-26). Audited core `7cf6754`; corpus `03ad46c`.
> Agents A–D context sheets consolidated; every load-bearing quote re-verified
> first-hand by the session lead (spot-checks noted inline). Abbreviations:
> `Fixup:` = `re-workspace/reversed/ghidra-mo/Fixup_decompiled.c`;
> `EFIBoot:` = `.../ghidra-mo/EFIBoot_decompiled.c`;
> `CheckVersion:` = `.../ghidra-mo/CheckVersion_decompiled.c`;
> `PRD:` = `.../ghidra-mo/PartitionRecoveryD_decompiled.c`;
> `MM:`/`PR:`/`UIL:` = `.../ghidra-partition/{MainModule,PartitionRecovery,UILogic}_decompiled.c`;
> `NFA:`/`FFA:` = `.../ghidra-mo/{NTFS,FAT}FileSystemAnalyser_decompiled.c`;
> `FSC:` = `.../ghidra-mo/FileSystemCheck_decompiled.c`;
> `RAW:` = `.../ghidra-searchdll/RAWSearchDLL_decompiled.c`;
> `DRWUI:` = `.../ghidra-drwui/DRWUI_decompiled.c`.

## 1. Fixup.mo — BCD hive binary patching (the boot-repair engine)

**Binding model** (PE export/import dump, this session): Fixup.mo is a link-time
shared library (36 exports: IObject family + 13 `Bcd_*` + 6 `BootIni_*` +
3 `Log_*`), NOT a MOM GetModule factory — imported by MainModule.dll,
RecoveryModule.dll, ToolsModule.dll, UILogic.dll, CloneModule.dll,
PartitionRecovery.dll. (Corrects `artifacts/MOM-modules/Fixup.md:5` and
`mo-module-map.md:39` implication.)

- CBcd ctor `FUN_180001000` (Fixup:2-17): default store `"Boot\BCD"`
  (0x4443425c746f6f42 at Fixup:13); CEFIFirstBcd ctor `FUN_1800010b0`
  (Fixup:22-61) overrides to `"EFI\MICROSOFT\BOOT\BCD"` (char-by-char,
  Fixup:40-58). Dual-singleton dispatch: every `Bcd_*` export runs CBcd
  `DAT_18000e1e0`, then CEFIFirstBcd `DAT_18000e630` if `CBcd::IsActive`
  (`FUN_180001fd0`, Fixup:743-756) says not-this-store; results OR-ed
  (pattern Fixup:4273-4286, 4308-4324, 4347-4363).
- `Bcd_Init` → `FUN_1800017a0` (Fixup:415-494): opens `%WINDIR%\BcdLog.txt`
  (:437-:444), queries system-partition GUID via device-interface vtable+0x18
  (:450), on failure logs `"Query system partition GUID failed!\n"` (:452),
  clears read-only attr (:485-488).
- `Bcd_BcdProcPartition(disk, partLBA)` → `FUN_180001980` (Fixup:499-599):
  3-retry BCD open; on sharing violation force-unmounts the live hive
  (`RtlInitUnicodeString(L"\Registry\Machine\BCD00000000")` :533; SeRestore
  0x12/SeBackup 0x11 enable `FUN_180001390` :243-255; `ZwUnloadKey` :542);
  reads MBR signature via `"\\.\PHYSICALDRIVE%d"` sector-0 (`FUN_180001570`,
  Fixup:326-331); scans hive cells — allocated-cell marker 0xffffffa0 (-0x60)
  (:559), partition byte-offset at +0x1c → LBA via >>9 (:565-568), MBR
  signature at +0x10 (:570-575); match records the BCD-boot partition
  (this+0x10c/+0x110, :577-578).
- **Offset rewrite primitives** (LEAD-VERIFIED first-hand): `FUN_180002520`
  (Fixup:957-992): `SetFilePointer(h, -0x1c, &hi, FILE_CURRENT)` then
  `WriteFile(h, &(newOffset << 9), 8, ...)` — the BCD device element's
  partition byte-offset patched in place. `FUN_180002eb0` (Fixup:1317-1361):
  scans for records whose low/high dword == disk signature (:1339), seeks
  back -0x20/-0x1c (:1341-1345), verifies stored == old<<9, writes new<<9
  (:1351). Two formats: pre-Vista device cells (size -0x60, Fixup:1048) vs
  Vista+ `"Element"`-named records (0x746e656d656c45, Fixup:1061) with -0x108
  cells (:1065); OS gate GetVersionExA >= 6.1 (Fixup:4582-4591, 4657-4666).
- `Bcd_UnlockBcdFile` (Fixup:4640-4765): after a partition move/resize,
  reparses the hive and rewrites ALL device elements referencing the partition
  (`FUN_180002920` :1120-1217 / `FUN_180002c00` :1222-1312).
  `Bcd_UnlockBcdNewPos(newLBA)` (Fixup:4770-4804). `Bcd_RestoreBcdFile`
  (Fixup:4562-4625): UNDO — repatches with old/new swapped (`FUN_180003290`
  passes (+0x118, +0x110) reversed, Fixup:1455). `Bcd_IsVistaBootManager`
  (Fixup:843-921): reads the partition PBR, checks 0x55AA at 510/511 (:887),
  scans first ~4 KiB for `"BOOTMGR "` (:889), checks bootmgr file exists
  (:902-903).
- Post-patch flush `FUN_180001440` (Fixup:263-306): DeviceIoControl
  **0x90083 = FSCTL_ALLOW_EXTENDED_DASD_IO** + FlushFileBuffers (:297-302).
- boot.ini legacy side: ARC-path parser `FUN_180004a40` (Fixup:2605+:
  `multi` :2637 / `scsi` :2652 / `signature` :2665 / `partition` :2757);
  `\ArcName` object-directory resolution `FUN_180003f70` (Fixup:2067-2193,
  ZwOpenDirectoryObject `L"\ArcName"` :2112-2119, ZwQuerySymbolicLinkObject
  :2142, prefix-match `\Device\Harddisk%d\Partition0` :2148); boot.ini rewrite
  `FUN_1800050d0` (Fixup:2999-3227, ARC rebuild `%s(%d)%s(%d)...` :3120-3162,
  signature form `%s(%08x)...` :3145, entries rewritten `"%s=%s"` :3196);
  `BootIni_UpdateFile` (Fixup:4879-4888).

## 2. Wired-into-recovery proof (the apply/undo chains)

- **MainModule apply chain** (MM = MainModule_decompiled.c): op dispatcher
  `FUN_180072620` (MM:81203) → on success layout write `FUN_180076bb0`
  (MM:82932, device vtable+8 write at MM:82904 region) → `FUN_180076780`
  (MM:82857) → **`FUN_180075580` = post-write boot fixup** (MM:82322):
  `Bcd_Init` (:82391); if `Bcd_IsVistaBoot()==0` → `BootIni_ForceUpdated`
  (:82408) + `BootIni_UpdateFile` (:82409); else → `Bcd_IsBcdPartition`
  (:82416) + `Bcd_LockBcdFile` (:82417) + `Bcd_UnlockBcdFile` (:82419); if the
  partition moved: `Bcd_BcdProcPartition` (:82423) → **`Bcd_UnlockBcdNewPos`**
  (:82424).
- **MainModule undo chain**: `FUN_180072620` failure → `FUN_180077630`
  (MM:83117): `BootIni_Init` (:83003), **`"ChkDsk.exe %C: /F /X"`** sprintf
  (MM:83206) executed by `FUN_180075e00` (CreateProcessA + Wow64 redirection
  disabled + WaitForSingleObject, MM:82546-82609), **`Bcd_RestoreBcdFile()`**
  (MM:83212).
- **UI device-refresh re-sync**: `CEPMUILogic::RefreshDeviceManager`
  (UIL:7649 string) → `BootIni_Init` + `BootIni_SetArcAccess` +
  `BootIni_IsUpdated` + `BootIni_UpdateFile` (UIL:7697-7700).
- **Boot Repair TOOL**: DRWUI navigation key `NavigationDataRecoverBootRepair`,
  gate `bShowBootRepair` (DRWUI:1453643, 1453900-1453910); launcher
  `FUN_1406a28a0` opens `<appdir>/../BootRepair/BootRepair.exe`
  (DRWUI:1094360-1094373); the tool is downloaded on demand
  (`TOOLS\BootRepair\BootRepair.exe` check DRWUI:494869; download
  `https://d1.easeus.com/drw/PE/tools.zip` DRWUI:494832-494835).

## 3. EFIBoot.mo — UEFI NVRAM Boot####/BootOrder repair (NOT ESP files)

- MOM factory module (exports GetModule; module name `"EFIBoot"` EFIBoot:909,
  class `"CEFIBoot"` :1209). Corrects `mo-module-map.md:38` ("EFI System
  Partition handling") — CEFIBoot manipulates **firmware variables**, not ESP
  files.
- Core APIs: dynamic `LoadLibraryW(Kernel32)` + `GetProcAddress(
  "SetFirmwareEnvironmentVariableW"/"GetFirmwareEnvironmentVariableW")`
  (EFIBoot:3281-3290); XP path enables SeSystemEnvironmentPrivilege
  (LookupPrivilegeValueW :3248, AdjustTokenPrivileges :3270); EFI global GUID
  **{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}** at 17 sites (EFIBoot:3385-4208).
- Device context `FUN_180001230` (EFIBoot:205-233): 0x54-byte partition
  descriptor (start/size, MBR-or-GPT marker, MBR signature / GPT GUID).
- State machine (dispatcher gates at +0xc): state 2 enumerate (:251-376,
  `Boot%04X` serialization :4040-4130); state 3 apply (:381-454 →
  `FUN_1800055b0`); state 4 update-in-place (:459-520 → `FUN_180004840`,
  patches partition start inside the load option `*(int*)(node+2) = ctx[0x11]`
  :~3641, Set back :3654-3660) and count (:525-586 → `FUN_180004b40`
  :3860-3960); state 1 delete (:591-652 → `FUN_180004e70`: remove `Boot####`
  vars, Set NULL/0 :3820-3822, rewrite BootOrder :3825-3832); support probe
  (:657-693 → `FUN_180005940` :4255-4276).
- Entry matching `FUN_1800046f0` (:3405-3480): reads Boot#### into a 1 MiB
  buffer, walks the EFI device path past the UTF-16 description, matches END
  (0x7FFF :3436) + MEDIA nodes (0x04/0x01 :3440): MBR form signature node+0xc
  + start LBA node+4/+8 (:3455-3460); GPT form GUID node+0xc + start LBA
  node+0x10 (:3462-3467).
- Create/repair `FUN_1800055b0` (:4130-4250): rewrites partition start/size/
  GUID inside an EFI_LOAD_OPTION device path (:4198-4206), finds the first
  free Boot#### slot (bitmap over BootOrder :4222-4240), Sets the variable
  (:4248-4251), rewrites BootOrder (:4260-4273).

## 4. CheckVersion.mo — offline OS-install detection (server-vs-client)

- Module main `FUN_180003050` (CheckVersion:1731-1860): candidate dirs
  **`"\winnt"`** (:1768) + **`"\windows"`** (:1802); `DiskContainer`
  (`FUN_1800033a0` :1907-1908) scans with an `IsWindowsServerInstalled`
  functor (:1833-1837).
- `DiskContainer::scan` `FUN_180003580` (:2027-2095): enumerates
  **rdisk 0..31 × partition 1..32** as ARC paths
  `"multi(0)disk(0)rdisk(%d)partition(1)"` (:2052) /
  `"...partition(%d)"` (:2066), resolves ArcPath→NT device, runs the
  predicate per partition (:2076).
- `IsWindowsServerInstalled::operator()` `FUN_1800029e0` (:1401-1662): per
  candidate root formats **`"%s\System32\config\system"`** (:1461) — the
  offline SYSTEM hive; reads **`\$$$PROTO.HIV\SELECT\CURRENT`** (:1473-1479)
  for the active ControlSet; then
  **`\$$$PROTO.HIV\CONTROLSET00%d\CONTROL\PRODUCTOPTIONS\PRODUCTTYPE`**
  (:1497-1504) and compares the REG_SZ against **`L"ServerNT"`** (:1563;
  LEAD-VERIFIED first-hand) and **`L"LanmanNT"`** (:1579; LEAD-VERIFIED) and
  `L"WinNT"` (DAT_180015920, :1547).
- FS sniffing backends to read \windows on any FS: FAT32/FAT16 BPB parsers
  (:5280-5430, backend tables tagged `L"fat32"` :5342 / `L"fastfat"` :5421);
  NTFS OEM `RtlCompareMemory(buf+3,"NTFS    ",8)` (:9665); CDFS
  `"CD001"`/`"CDROM"` (:1152-1153); `"\\.\PhysicalDrive"` per-disk cache
  (:2176).
- Consumer: UNRESOLVED in-session — no host binary's strings reference
  "CheckVersion" (checked: all PartitionRecovery DLLs, DRWUI.exe, .mo set);
  possibly dead-shipped legacy or loaded via an untraced dynamic path.

## 5. MainModule/PartitionRecovery.dll — the partition-table WRITE path

- **Layout write via IOCTLs on a raw disk handle** (LEAD-VERIFIED first-hand
  in PR): open device via `IObject::GetInterfaceEx` (PR:46844-46848); legacy
  path **`DeviceIoControl(0x7C100 = IOCTL_DISK_DELETE_DRIVE_LAYOUT)`**
  (PR:46953; MM:109164) → raw sector-0 read → zero all 4 entries at
  0x1BE..0x1FD + write 0x55/0xAA at 0x1FE/0x1FF (PR:46957-46962;
  MM:109184-109196) → write sector 0 back → **`DeviceIoControl(0x7C010 =
  IOCTL_DISK_SET_DRIVE_LAYOUT, DRIVE_LAYOUT_INFORMATION{4 entries}, 0xA8)`**
  (PR:46999; LEAD-VERIFIED: `(**(code **)(*(longlong *)local_1140 + 0x28))
  (local_1140,0x7c010,local_1120,local_1130)` at PR:46995-47002); modern path
  (OS >= 5.1): **`DeviceIoControl(0x7C058, 0x18-byte SET_PARTITION_INFO_EX-
  style, 0x18)`** (PR:47015; MM:109227-109229).
- Per-partition entry writer `FUN_180038b10` (PR:32548-32672); MBR-entry wipe
  `FUN_180039030` (PR:32700-32790); **GPT variant `FUN_1800392f0`**
  (PR:32797-32860): **`GPT_GenerateSignature`** (new disk GUID, PR:32816) then
  wipe + entry-write + set signature via interface vtable+0x80 (PR:32825);
  protective entry `FUN_180039470` (PR:32863-32920) via 0x7C058.
- `IOCTL_DISK_UPDATE_PROPERTIES` 0x70140 (PR:46873-46880, 3× Sleep(1000)
  retry; MM:107015); `FSCTL_DISMOUNT_VOLUME` 0x90020 (MM:81983);
  `FSCTL_EXTEND_VOLUME` 0x900F0 ("[==dbg==]ExtendNTFSVolume" MM:78950).
- RecoveryModule object factories (PR export table):
  `CreateScanLostPartitionObject`, `CreateCheckPartitionObject`,
  `CreateEnumPartitionObject`, `CreateExplorerPartitionObject`,
  **`CreateRecoveryPartitionObject`**; `ScanLostPartition`/`IScanLostPartition`
  classes PR:14083-14162. Apply + boot-config repair run as one composite
  operation (commonmbrgptconvert.cpp flow, PR:47290+, `BootIni_Init`
  PR:47302).

## 6. Win32Bcd.dll + WinChkdsk.exe

- Win32Bcd.dll (96 exports): a **bcdedit.exe CLI wrapper** — commands
  `/store ` (Win32Bcd:1123), `/set ` (:606, :819), `/enum ` (:5206),
  `/enum ACTIVE /v` (:6790), `/create %s /application osloader` (:6411),
  `/create %s /device` (:6423) via CreateProcessW (:213), output parsed back.
  Sole importer: CloneModule.dll (WinPE-media BCD creation).
- WinChkdsk.exe: loads `%WINDIR%\System32\<dll>` (:3361) and
  **`GetProcAddress("Chkdsk")`** (:3490-3492 — ufat.dll/untfs.dll's Chkdsk
  export); hooks ulib.dll MESSAGE family to redirect output (:3405-3470);
  VSS coordination; assert paths cite
  `i:\_drw_main\partitionrecovery\legacy_mod\mod.winchkdsk\main.cpp` (:4107+).
  MainModule runs **`"ChkDsk.exe %C: /F /X"`** synchronously after operations
  (MM:83206, executor MM:82546-82609).

## 7. Core-side absence (LEAD-VERIFIED by grep over src/ + include/)

- `include/cr/lost/cr_lostpart.h:91-92` — "Materialize a candidate as a
  bounded virtual volume (R3.2: read-only, the user's table is never
  repaired)" — the lost-partition contract is read-only by design.
- Zero hits for: bcd/BCD00000000/boot.ini/bootmgr (0), BootOrder/
  SetFirmwareEnvironmentVariable/8BE4DF61 (0), chkdsk (0),
  IsWindowsServer/ServerNT/LanmanNT/ProductType (0),
  SET_DRIVE_LAYOUT/DELETE_DRIVE_LAYOUT (0), pending-op engine (0).
- ESP only as a partition kind: `FsKind::EfiSystem`
  (include/cr/table/cr_table_parse.h:37, set at
  src/core/table/cr_table_parse.cpp:276-277).
