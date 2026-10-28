# modules/mom-capabilities.md — the never-audited MOM/product-capability surfaces (session 5)

Reference: the og's PartitionRecovery `.mo` module family + product DLLs
(`og-codebase/PartitionRecovery/` — 30 `.mo` modules + host DLLs) per
`re-workspace/artifacts/PartitionRecovery/mo-module-map.md` (rows corrected
by I-020), `mom-modules--ui-bridge-and-dependency-matrix.md`,
`pending-operation-engine.md`, and the session-5 decodes
(`evidence/decompiled-quotes/s5-mom-capabilities.md` +
`s5-image-fscheck-persistence.md`).

Core (audited `7cf6754`): the core is a recovery engine; the module rows
below cover the og surfaces that prior sessions never dispositioned.
Audited 2026-10-26 (session 5) — first-time coverage for every row here.

## Rows and verdicts

### apply-write (lost-partition table write) — og: MainModule/PartitionRecovery.dll write path
- **B1/P42 apply operation** — `S-029` (RecoveryModule
  `CreateRecoveryPartitionObject` → IOCTL 0x7C100/0x7C010/0x7C058 chain +
  GPT GUID regen + 0x70140 refresh; core contractually read-only,
  cr_lostpart.h:91-92).
- **P62 consent/write safety** — folded into S-029's fix plan (consent-gated
  apply, S-010 quiesce reuse).
- Everything else `NA` pending the S-029 scope decision.

### boot-repair (Fixup.mo + EFIBoot.mo + Win32Bcd + WinChkdsk + BootRepair tool)
- **P25 BCD cell patching** — `M-040` (offset rewrite `newLBA<<9`,
  signature+offset matching, dual-store Boot\BCD + EFI\Microsoft\Boot\BCD).
- **P19/P20 UEFI NVRAM variables** — `M-040` (Boot####/BootOrder via
  Get/SetFirmwareEnvironmentVariableW, GUID 8BE4DF61; device-path match on
  MBR signature / GPT GUID + start LBA).
- **P35 undo** — `M-040` (Bcd_RestoreBcdFile + "ChkDsk.exe %C: /F /X").
- **P23 error codes** — NA (no core counterpart to diverge).
- boot.ini legacy — `M-040` (ARC-path rewrite on device refresh; likely
  skippable per Win7 posture — fix-plan decision).

### imaging (disk-image scan targets) — og: DRWUI CImageFileDevice + Device.mo/libvmdk
- **P1/P42 image inputs** — `S-030` (vmdk/vhd/vhdx/edisk/img/dmg as
  first-class scan devices; core --disk/--volume only).
- **P27 fragment handling (VMDK extents/parent chains)** — `S-030` (libvmdk
  read translation, 512-B sectors).
- **P8 multi-volume** — `S-030` (image-bounded devices classify MBR/GPT at
  LBA0).
- **P47 throughput** — NA until implemented (OQ-S4-3 lane).

### check-phase (FileSystemCheck.mo) — chkdsk-grade audit
- **P1/P3/P25 FAT audit** — `M-041` (copy-compare 0x1005, entry-hole +
  duplicate-ref 0x1006, dir-tree audit 0x1002/0x1004/0x1007, orphan sweep
  0x1003, DBR ceilings).
- **P1/P3 NTFS audit** — `M-041` ($MFT/$MFTMirr proofs 0x1008/0x1009,
  version gate {1.2,3.0,3.1} 0x100a, $Bitmap structural 0x100b,
  MFT×$Bitmap cross-check 0x1002/0x1003).
- **P23 error taxonomy** — `M-041` (0x1001-0x100e; the fix adopts numeric
  parity).
- **P35 escalation ladder** — og: check → RawFixer → WinChkdsk → RAW;
  core's stance MATCH (cr_scan.cpp:546-566 never aborts) but the check
  phase itself is the finding.
- **B6 damaged-volume (rawfixer overlap)** — MATCH-with-note: the repair-
  oriented checks inside cr_rawfixer (CheckMFT/CheckFatTable/CheckRootDir)
  are og-decoded (S-008 FIXED); M-041 is the AUDIT layer beyond them.

### recognition (volume analysers — content-probing contract)
- **P1 NTFS recognition** — `M-043` (MFT $Volume proof + backup-boot
  verify; core boot-bytes-only, no scanner backup read).
- **P1 FAT recognition** — `M-045` (gate family) + `N-050` (root-dir proof
  absent).
- **P1 UDF recognition** — **M-044 FIXED (s21)**: `FsFamily::Udf` (11) + the og-exact probe in `probe_family` (boot-bytes fast-reject; AVDP sector at +0x100; Main-VDS sector at u32@AVDP+0x14; "UDF"@[0x19..0x1b] — FUN_180002580 :1413-1435). No engine on either side (no UDF SearchDLL in the og 9-engine set); carve-only disposition; fs_hint 11 in the ABI. Findings/FIXED/M-044.
- **P1 ext recognition** — MATCH (S-006's CExtX accept-gate decode is the
  probe_ext implementation).
- **P15 damage grading** — MATCH-by-omission (analysers output type +
  geometry + label only; no health score — grading is S-023's DRWUI-side
  lane).

### session-persistence (lostpart.dat/partinfo.dat semantics)
- **P12/P36 resume re-validation** — `M-042` (og re-reads + drops stale;
  core restores blind).
- **P34 checkpoint** — `M-042` (og unconditional per-hit append vs core
  config-gated) + `N-049` (record richness: bps/SPC/hidden/size-source not
  persisted).
- **P13 layout snapshot** — `M-042` (partinfo.dat whole-disk layout-change
  detection; core has single-volume identity card only).

### logging (Log.mo / PubLog / Logger.dll)
- **P35/P54 logging seam** — `N-048` (three og layers incl. 10 MB
  rollover + 2 MB ring; core app-only vfprintf, engines silent).

### mom-tool-ops (EPM partition-management surfaces)
- **P54 scope boundary** — `I-019` (EditLdm2* LDM write-side, Ntfs2FATProc
  conversion, partition merge/resize deals, FatFormat.mo, Burn.dll —
  registered exclusion; no recovery-flow consumers).
- Read-side LDM (Dmrp*) — MATCH (covered by modules/ldm.md; this row only
  registers the WRITE side).

### mom-runtime (Mom.mo / Common / Config / Thread / Error / Log / RegLib / CallbackOperator)
- **P54/P56 module framework** — MATCH-with-note (informational): the MOM
  runtime is the og's plugin architecture; the core's static linking is a
  deliberate architectural difference registered in PARITY-ARCH. RegLib's
  offline-hive reader has a core counterpart added by S-014
  (cr_reghive.cpp, BitLocker-scoped — the og's use is broader: FVE keys +
  CheckVersion's ProductType reads; the ProductType lane is M-042-adjacent
  via CheckVersion, below).
- **CheckVersion.mo (OS-aware recovery)** — registered here: offline
  OS-install detection (\winnt/\windows sweep, rdisk 0..31 × partition
  1..32, offline SYSTEM hive ProductType ServerNT/LanmanNT/WinNT) —
  core absent; **MATCH-BY-OMISSION (dead-shipped: OQ-S5-1 RESOLVED s25)**
  — the consumer sweep is exhausted: zero name-requests in every
  decompiled host (the MOM load contract is on-demand by name,
  mom-module-system §1.1; the module's "CheckVersion" string appears
  only in its own GetName), zero references in every PE binary of the
  corpus (BUILDPE payload + the full product tree; the single
  `xmlCheckVersion` hit in EUFileRepair/CORE_RL_xml_.dll is libxml2's
  coincidental API), zero config-driven loads, and no generic
  directory-enumeration loader exists (the FindFirstFileW sites are the
  recovered-filesystem walker with ACL checks). The capability never
  runs in the shipped product; the core's absence is parity.
  **M-082 RE-SCOPE (DFX-38, 2026-10-29):** the dead-shipped verdict
  covers ONLY the CheckVersion copy. The **DeviceManager.mo copy of the
  same machinery is LIVE** (loaded by name — the s25 evidence's own
  FUN_1800017f0("DeviceManager") trace) and is the full offline-OS /
  boot-volume identification stack (the 0x418-byte CPartitionBootInfo
  profile: CurrentVersion identity + SystemRoot + MountedDevices
  system-partition location + Select-aware ControlSet + ProductOptions —
  :62034-62330). The core now carries the capability:
  `reghive::offlineos::profile_os()` (cr_offlineos.h, C++ API; the
  boot-fix auto-locate consumer + the ABI export are the recorded
  follow-ups — PARITY-DELTAS D-107). The two copies are separately
  dispositioned; this row's MATCH-BY-OMISSION no longer covers the
  surface by omission.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-029 | SEVERE | Lost-partition APPLY (table write) missing — the recovery endgame |
| S-030 | SEVERE | Disk-image scan targets missing (vmdk/vhd/vhdx/edisk/img/dmg) |
| M-040 | MAJOR | Boot-chain repair family absent (BCD patch + NVRAM + boot.ini + post-op chkdsk) |
| M-041 | MAJOR | Damage-audit/check phase missing (chkdsk-grade FAT/NTFS audit + WinChkdsk ladder) |
| M-042 | MAJOR | Session-resume re-validation + layout snapshot missing (lostpart.dat semantics) |
| M-043 | MAJOR | NTFS content-proof recognition + scanner backup-boot read missing |
| M-044 | MAJOR | UDF recognition family missing (analyser result 0xc) — **FIXED (s21)** |
| M-045 | MAJOR | FAT boot-gate family diverges (bps/SPC/label/numFATs/cap/band-retry) — resolves S-006's FATX remainder |
| N-048 | MINOR | Engine-level logging seam absent (three og layers) |
| N-049 | MINOR | Persisted lost records thinner than og (geometry/evidence fields) |
| N-050 | MINOR | Recognition contract boot-only (no content proof before engine dispatch) |
| I-019 | INFORMATIONAL | EPM tool ops registered as scope exclusion |
| I-020 | INFORMATIONAL | Corpus-doc corrections (CTbFile, virtual views, EFIBoot, Fixup, FATX, ext-table) |

## Resolutions this session

- **OQ-S4-1 RESOLVED** → S-023's fix-plan step 3 input complete (169-entry
  extension table dumped; `.data` not `.rdata`; unsorted).
- **OQ-S4-2 RESOLVED** → M-029 §Gap (b) corrected (single-site
  "filter_generalraw" category display-name mapping; no per-file filter).
- **S-006 scoped remainder RESOLVED** → no Xbox FATX parser exists;
  CFatxBlockParser = FAT12/16/32 boot parser; gates moved to M-045.

## Open (tracked)

- **OQ-S5-1 RESOLVED (s25)**: dead-shipped — the sweep is exhausted
  (no host requests the module by name, no PE binary references it, no
  config-driven load, no enumeration loader; PE edition drops the MOM
  system entirely). MATCH-by-omission registered above.


## Session-8 section (2026-10-28, audited `35adcb9`)

**M-082** (MAJOR) re-scopes the offline-OS verdict: the CheckVersion
dead-shipped MATCH-by-omission covered only that module's copy; the
**DeviceManager** copy is live (by-name load evidenced) and is the full
offline-OS/boot-volume identification stack (CurrentVersion profile,
`\Select\Current` ControlSet resolution, `\MountedDevices` system-partition
mapping, ProductOptions server/client, per-dir installer sweep —
DeviceManager_decompiled.c:51303/:62034-62410/:62854-62920). The core's
reghive is FVE-only; cr_bootfix takes the BCD path as input. Fix plan = the
capability decision + (if yes) a reghive extension. **OQ-S8-1**: CWindowsFS
decoded (the live-mounted-FS IFSInterface adapter, FileSystem_decompiled.c
:10650-10797 factory) — its og consumer is the open question.
**I-027**: the og-codebase data-asset ground-truth register (quota contract,
json_raw shield, Aliyun caps, schtasks autostart, filter_generalraw value).

## Session-10 S10-F rows (function-level completeness pass, 2026-10-29; audited `44f043a5`)

| Parameter | Verdict |
|---|---|
| register completeness — the product-frame binaries | **DIVERGENCE-MINOR — N-134** (the 103-binary ghidra-tree census: CloneModule.dll [the clone/PreOS host, CEUCloneMgr ×398, the sole Win32Bcd importer], legacy.dll [LibTomCrypt substrate, role undecoded], EuDownloader.exe [the curl downloader], Boot.dll/BootDriver.dll/peformat.dll [the media-builder family — distinct from M-040's repair side] + the six telemetry binaries name-absent — none registered in I-010/I-019/I-027; the engine-bearing trees are CLEAN at binary level; evidence s10f-product-frame-register.md) |
| MOM module decompile-tree verification | **MATCH** — the 30 .mo decompiles re-checked against the roll-up + trail at binary level: 27 name-cited, 3 infra-only (DeviceAdapter / FileSystemAnalyser-base / ExFatFileSystemAnalyser-open-tail) — re-confirms the inventory's rollup-covered claim |
