# Evidence — S10-F/T4: the never-dispositioned product-frame binaries (the register-completion batch)

> Session-10 S10-F lane (ghidra-tree disposition pass). Corpus `be0cfe9`;
> core product-code tree `44f043a5`. The T4 read-agent enumerated all 103
> decompiled binaries across the nine ghidra trees and mapped each to its
> trail disposition; the lead spot-verified every strong candidate's RTTI
> claims and zero-disposition claim first-hand (the greps below are the
> lead's own re-runs).

## 1. The full-tree disposition census (T4 agent, lead-spot-verified)

| tree | binaries | dispositioned | OOS-registered | never-dispositioned |
|---|---|---|---|---|
| ghidra-searchdll (9 engines) | 9 | 9 | 0 | 0 |
| ghidra-mo (30 .mo) | 30 | 27 | 0 | 3 weak (DeviceAdapter / FileSystemAnalyser-base / ExFatFileSystemAnalyser — all infra-only or registered-open-tail; see §4) |
| ghidra-partition (19) | 19 | 13 | 1 (GetDriverInfo, N-106) | 5 strong (CloneModule, Boot.dll, BootDriver.dll, peformat.dll + weak Logger-family) |
| ghidra-core (5) | 5 | 4 | 1 (DRW.exe OOS) | 0 |
| ghidra-misc (20) | 20 | 5 | 9 | 6 (legacy.dll, EuDownloader.exe strong; telemetry set family-adjacent) |
| ghidra-wave2 (6) | 6 | 5 | 1 | 0 (VideoFileRepairer now N-130) |
| ghidra-repair (14) | 14 | 2 | 12 | 0 |
| ghidra-drwui (1) | 1 | 1 | 0 | 0 (UI-pure remainder OOS-registered) |
| ghidra-drivers (7) | 7 | 6 | 1 (EuMrx NAS) | 0 |

**Total: 103 binaries — 71 dispositioned, 23 OOS-registered (14 name-absent
at family level), 9 never-dispositioned (6 substantive + 3 weak).** The
engine-bearing trees are CLEAN at the binary level — the residue is
exclusively product-frame (tool-op / media-builder / telemetry / downloader
/ crypto-substrate surfaces).

## 2. The six substantive never-dispositioned binaries (lead-verified)

### 2.1 CloneModule.dll — `ghidra-partition/CloneModule_decompiled.c`

RTTI census (lead grep): `CEUCloneMgr` ×398, `CCopyDiskPendingOperation`
×23, `CEUPreOsMgr` ×73, `CEUClonePostItem` ×10. The disk-clone + PreOS host:
copies disks/partitions, builds WinPE pre-OS environments
(`preospeandbcdproc.cpp`, `peandbcdproc.cpp`, `pecommon.cpp`), extracts
ISO9660 assets, authors media BCD (`%s\Boot\BCD`,
`%s\EFI\Microsoft\Boot\BCD`, `BOOTMGR`, `WGL4_BOOT.TTF`) — the sole
importer of Win32Bcd.dll per s5-mom-capabilities:194. Trail state: 6 hits,
ALL importer-enumeration context (PROTOCOL.md:119, gpt-guid-probe.md:40-42,
s5-mom-capabilities:21/:194, CORPUS-ERRATA E40, M-040's sole-importer note)
— zero behavioral disposition; no register row names the clone/preos
families (I-019's EPM tool-op exclusion lists EditLdm2*/Ntfs2FAT/
merge-resize/FatFormat/Burn — not clone/preos).

### 2.2 legacy.dll — `ghidra-misc/legacy_decompiled.c`

LibTomCrypt fingerprint (lead grep): `kdf_pbkdf1_derive` ×2,
`des_get_ctx_params` ×2, `cipher_generic_init` ×3. 275 fns / 118 named.
The legacy crypto substrate; per deepening-batch1 §4 "most plausibly the
old .rsf / scan-session decryption (pre-CryptoAPI era) or legacy-license
blobs". Trail: ZERO hits for `legacy.dll` / `libtomcrypt`. The registered
`.rsf-import` exclusion (CAPABILITY-MATRIX §3) never names the binary; the
role is undecoded.

### 2.3 EuDownloader.exe — `ghidra-misc/EuDownloader_decompiled.c`

curl-linked downloader (lead grep: 287 `curl_` references; RTTI
`CEudDownload`, `EudData2ParentProcess`). The on-demand tool/asset fetch
engine behind DRWUI's tools.zip / BootRepair downloads. Trail: ZERO hits;
no Cloud/installer register names it (I-010:7 names only Cloud/,
cloud_session.dll, EuICloudRecovery.dll).

### 2.4 Boot.dll — `ghidra-partition/Boot_decompiled.c`

`legacy_mod\mod.boot` — the boot-media boot module (lead grep: `IBoot` ×8,
`IBCDEdit` ×4, `CEditBootItem` ×9, `BOOTMGR` ×13). Edits boot.ini and
`\BOOT\BCD`, dumps system-partition boot sectors, extracts ISO9660 files,
resolves `\\.\HDBOOTDOSDEVICE`, `CBoot::GetUSBActivePartFsType` — the
boot-file half of the "Create Recovery USB/CD" media builder. Trail: ZERO
hits (all "Boot" grep hits are EFIBoot/MainModule/BootRepair). The
WinPE-media-builder family is registered OOS (I-010:10, §5 WinPE row) but
Boot.dll is never named; its BCD/boot.ini editing targets MEDIA, not
post-recovery repair (M-040's family) — the distinction is unrecorded.

### 2.5 BootDriver.dll — `ghidra-partition/BootDriver_decompiled.c`

`CBootDriver` — the boot-media builder proper ("Create Recovery USB/CD":
`BootDriverExcute`, IMAPI2 `CBurnBootCD` chain, urls.ini/UIConfig.ini
config). Trail: ZERO hits; deepening-batch3 §3 documents it corpus-side but
that artifact is trail-cited only at §4 (AutoUpdate, N-101).

### 2.6 peformat.dll — `ghidra-partition/peformat_decompiled.c`

The WinPE-media partition/format helper (`CLogInBootICE`, `CLogWriter`,
TB-family exports) — WinPE prep kin of FatFormat.mo. Trail: ZERO hits.

## 3. The telemetry name-absent set (family-adjacent, register-level)

`AliyunWrap.dll` / `AliyunWrapExe.exe` / `send_dmp_with_aliyun.exe` /
`EuUserRate.dll` / `firebasefetch.exe` / `firebaseonlineconfig.dll`
(ghidra-misc) — the Aliyun telemetry backbone + crash-dump uploader +
rating UI + Firebase remote-config fetch chain. I-027 registers only the
AliyunConfig.ini caps asset ("REPORTING-OOS row 51"); I-010 names only
cloud_session.dll/EuICloudRecovery.dll in the Cloud family; the
crash-supervisor row covers DRW.exe/DRWUI/DRWReport but not the upload
executables by name. deepening-batch3 §1/§5/§6 document them corpus-side,
cited in the trail only at §4. No recovery semantics — register-level gap.

## 4. The three weak MOM candidates (verified NOT gaps)

- **DeviceAdapter.mo** (99 named fns) — roll-up row only ("adapter layer,
  IBlockDevice/IDiskGeometry bridging"); infrastructure-only, zero behavior
  parameters. Consistent with the SURFACE-INVENTORY §4/§9 claim.
- **FileSystemAnalyser.mo base** (150) — the analyser base + module factory;
  the recognition-family contract is covered by mom-capabilities.md:62-72.
- **ExFatFileSystemAnalyser.mo** (163) — the recognition body's unpinned
  state is a REGISTERED open tail (N-050's resolution notes: "the
  conservative boot-region proof is the registered stand-in until that
  body is decoded").

These confirm (at decompile-tree level) the inventory's rollup-covered
claim rather than contradicting it.

## 5. Engine-class completeness (the T3a/T3b lanes, for the record)

The 25 never-cited-by-name engine classes across NTFS/FAT/exFAT/Btrfs/
EXT4/HFS+ were each verified COVERED semantically (the T3a/T3b reports —
vtable slots re-derived from the og binaries via the trail's own RTTI
walker, core equivalents cited file:line, trail dispositions cited). Two
behavioral gaps surfaced and were filed as findings (M-110 @eaDir; N-133
engine-side volume labels). Everything else: naming noise, not coverage
noise — the engines' class surface is CLEAN.
