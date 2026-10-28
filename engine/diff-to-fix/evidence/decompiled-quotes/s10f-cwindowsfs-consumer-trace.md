# Evidence — S10-F/T11a: the CWindowsFS consumer trace (OQ-S8-1 RESOLVED)

> Session-10 S10-F lane. Corpus `be0cfe9`; core `deea0241`. The T11a read-agent's
> full trace, with the lead's first-hand verification log appended (every
> load-bearing cite re-read before M-111 was filed).

## The MOM dispatch shape (decoded)

- FileSystem.mo exports exactly one MOM entry: `GetModule` @ 0x18000dae0
  (PartitionRecovery/FileSystem.mo/rabin2_exports.txt:35; decompile
  FileSystem_decompiled.c:11214-11221). Loader contract: name→`<Name>.mo`→
  LoadLibraryA→GetProcAddress("GetModule") (FileSystem_decompiled.c:2439-2447;
  mom-module-system--architecture.md:24-49).
- The factory FUN_18000cf10 (FileSystem_decompiled.c:10650-10800) has ZERO
  direct callers in any decompile — it is the vtable method
  `IFileSystemModule::CreateFileSystem(IObject* partitionEntry, bool rawOnlyFlag)`
  at vtable slot +0x38, signature-matched at every consumer site.
- The IFSInterface vtable (from CWindowsFS methods): slot0 FindFirstFile
  (dir, 0x468-entry) FUN_1800119f0 :14132-14268; +0x08 FindNext(handle, entry)
  FUN_180011ea0; +0x10 FindClose FUN_1800119e0 :14120-14127; +0x18 Init
  (factory :10792); +0x20 GetUsedSpace; +0x28 Release(1) (dtor FUN_180011980
  :14105).
- CWindowsFS construction: the "early unconditional branch" (:10695-10701)
  fires when the partition's IVolumeEntryInfo+0x30 returns a drive-letter
  byte ≠ 0 (stored at +0x8, :14061-14062; FindFirstFile then enumerates
  "X:\<dir>\*", :14198-14218) AND raw-flag == 0. The type dispatch
  (4/NTFS, 8/HFS+, masked others) uses probe FUN_18000d590 :10896 →
  CWindowsFS when OS-accessible. Raw-flag = 1 is passed by callers exactly
  for FS types 5/6/0x18 (the ext family).

## Consumer A — USER-VISIBLE: the og's in-app partition file browser

1. DRWUI loads PartitionRecovery.dll, resolves 5 object factories —
   DRWUI_decompiled.c:438859-438894, GetProcAddress ×5 incl.
   "CreateExplorerPartitionObject" at :438891 (stored at holder+0x28).
2. Wrapper FUN_14027dc90:438721-438733 calls fn-ptr +0x28.
3. Open browser on selected partition — FUN_14091c620 (DRWUI:1433564-
   1433587): creates ExplorerPartition + 0x80-byte adapter, explorer->
   vtable+8(explorer, adapter) :143582-143583.
4. Enumerate directory with a UI-supplied path — FUN_14091c800
   (DRWUI:1433617-1433690): QString::toUtf8(param_4) → explorer->
   vtable+0x40(explorer, path, listA, listB) :1433661-1433669; entry by
   index = 0x468 bytes :1433675, name via QString::fromUtf8 :1433680.
   Count: FUN_14091ca70 :1433701-1433733.
5. UI trigger — a button-click slot in the partition-scan frame:
   FUN_140904fc0 (DRWUI:1421095-1421127, registered as clicked handler at
   :1420641-1420659): if mounted-with-path → ShellExecuteA("open",…)
   :1421113; else → in-app ExplorerPartition browse :1421116-1421118;
   owning class CPartitionScanFrame::vftable :1421168.
6. Results rendered in a QTableView (async fill) — DRWUI:1414842-1414852
   (count), :1414888-1414899 (name-by-index), :1414923-1414929 (columns).
7. Engine side: ExplorerPartition class — PartitionRecovery_decompiled.c
   :8240/:8263 (IExplorerPartition/ExplorerPartition::vftable), export
   CreateExplorerPartitionObject :890 + rabin2_exports.txt:55, RTTI
   .?AVExplorerPartition@@ (strings_ascii.txt:3487-3488).
8. THE FACTORY CALL — ExplorerPartition::Open = FUN_18000d220
   (PartitionRecovery_decompiled.c:8440-8503): FUN_1800017f0("FileSystem")
   :8481, fstype via +0x30 :8484, raw-flag = 1 iff fstype ∈ {5,6,0x18}
   :8485-8490, module->vtable+0x38(module, partition, flag) :8491,
   IFSInterface stored at this+0x10 :8492.
9. Enumeration impl — FUN_18000d970 (:8699-8790): slot0(FS, dir, entry)
   :8746, attr&0x10 dir test :8772, dirs→listA / files→listB :8774-8777,
   slot+0x08 FindNext loop :8779-8781, slot+0x10 FindClose :8783; close =
   FUN_18000d440 :8508-8518 (Release(1)).

## Consumer B — internal: used-space computation

DeviceManager_decompiled.c:48960-49109 (CUpdatePartitionUsedSpaceName,
self-identified by CLogWriter::Log strings): FUN_180005150("FileSystem")
:49031/:49056/:49088 → vtable+0x38 :49035/:49060/:49092 → +0x20
GetUsedSpace :49036/:49061/:49093 → +0x28 Release — for FS types 5/6,
0x18 and 8 (NTFS/FAT go via CNTFSLib::GetSpaceInformation :48998 /
GetFatPartitionUsedSpace :49013). PartitionRecovery_decompiled.c:15805-
15824 — same-family switch, case 8 (HFS+).

## Consumer C — internal: lost-partition-scan validation

PartitionRecoveryD_decompiled.c:6755-6805 (FUN_18000b880): builds the
engine's module set incl. FUN_18000bfd0("FileSystem") :6795. :6634-6750
(FUN_18000b4a0): constructs a partition-entry from a scan range, factory
call vtable+0x38(module, partition, raw-flag{5,6,0x18}) :6717, then a
ROOT-OPENABILITY PROBE: memset 0x468 entry :6723-6727, slot0(FS,
DAT_18002c3ec) = FindFirstFile(root) :6728, FindClose :6730, found=1
:6731. Reached via FUN_1800201d0 :19936-19954 (a CRecoverySearchEngine
method).

## Negative results (explicitly checked)

- File-repair / preview chains are NOT consumers: no IFSInterface/
  CWindowsFS/ExplorerPartition refs in ghidra-repair/* or ghidra-wave2/*;
  DRWUI picks user files via standard QFileDialog (getOpenFileName/
  getExistingDirectory :1213941, :1225959, :1203144, :1382015).
- DRWUI's 18 L"FileSystem" hits are TELEMETRY (FUN_14006da00, the
  analytics string-param helper — same call shape as L"Click_Help").
- Address-collision disclaimer: FUN_18000cf10 hits in Burn/ZfsSearchDLL/
  RecoveryModule/PartitionRecovery are different functions in those
  binaries' own address spaces.
- No corpus artifact predicted the ExplorerPartition chain (grep over
  artifacts/: only unrelated browser_* profile strings).

## The lead's first-hand verification log (before M-111 was filed)

- DRWUI:438885-438895 read: the GetProcAddress sequence incl.
  CreateExplorerPartitionObject at :438891 — CONFIRMED verbatim.
- PartitionRecovery:8478-8495 read: FUN_1800017f0("FileSystem") :8481,
  the {5,6,0x18} raw-flag computation :8485-8490, the vtable+0x38 factory
  call :8491, the +0x10 store :8492 — CONFIRMED verbatim.
- DRWUI:1421095-1421125 read (FUN_140904fc0): the mounted→ShellExecuteA
  :1421113 vs the in-app FUN_14091c620 :1421116-1421118 branch —
  CONFIRMED verbatim.
- Core census re-run at deea0241: FindFirstFile* = the .crsf session
  listings only; CWindowsFS/IFSInterface = zero — CONFIRMED.
