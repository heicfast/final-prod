# MINOR N-134 — product-frame register completion: six never-dispositioned og binaries (CloneModule / legacy.dll / EuDownloader / Boot.dll / BootDriver.dll / peformat.dll) + the telemetry name-absent set

**Severity:** MINOR (register-level gaps — the N-101/N-106/N-130 precedent class: corpus-documented surfaces with zero trail disposition; no engine parity risk except the CloneModule↔S-029 write-path overlap and legacy.dll's undecoded crypto role)
**Module:** mom-capabilities / product-frame registers (I-010/I-019/I-027/SURFACE-INVENTORY §5)
**Parameters affected:** I-010-lineage (scope/asset registers), Q68 (documentation — the registers' completeness), A54-adjacent (module boundaries — the clone/preos family's shared MBR/GPT convert write path)
**Corpus references:** `re-workspace/reversed/ghidra-partition/CloneModule_decompiled.c` (RTTI: CEUCloneMgr ×398, CEUPreOsMgr ×73, CCopyDiskPendingOperation ×23, CEUClonePostItem ×10 — the disk-clone + PreOS host; `preospeandbcdproc.cpp`/`peandbcdproc.cpp`/`pecommon.cpp`; the BCD authoring strings `%s\Boot\BCD`, `%s\EFI\Microsoft\Boot\BCD`, `BOOTMGR`, `WGL4_BOOT.TTF`); `ghidra-misc/legacy_decompiled.c` (LibTomCrypt: `kdf_pbkdf1_derive`, `des_get_ctx_params`, `cipher_generic_init` — the legacy crypto substrate, role undecoded); `ghidra-misc/EuDownloader_decompiled.c` (287 curl_ refs; RTTI CEudDownload/EudData2ParentProcess — the on-demand component downloader); `ghidra-partition/Boot_decompiled.c` (IBoot ×8, IBCDEdit ×4, CEditBootItem ×9, BOOTMGR ×13 — the media boot-file editor); `ghidra-partition/BootDriver_decompiled.c` (CBootDriver, `BootDriverExcute`, CBurnBootCD IMAPI2 chain — the media builder proper); `ghidra-partition/peformat_decompiled.c` (CLogInBootICE/CLogWriter, TB-family exports — the WinPE prep/format helper). The telemetry set: AliyunWrap.dll / AliyunWrapExe.exe / send_dmp_with_aliyun.exe / EuUserRate.dll / firebasefetch.exe / firebaseonlineconfig.dll (ghidra-misc). Full census + per-binary verification greps: `evidence/decompiled-quotes/s10f-product-frame-register.md`.
**Core references:** audited `44f043a5`. None of the six has a core counterpart or a register row: `CloneModule` → 6 trail hits, all importer-enumeration context (PROTOCOL.md:119, gpt-guid-probe.md:40-42, s5-mom-capabilities:21/:194, CORPUS-ERRATA E40, M-040's sole-importer note); `legacy.dll`/`EuDownloader`/`Boot.dll`/`BootDriver`/`peformat` → ZERO trail hits (grep-verified). I-019's EPM tool-op exclusion names EditLdm2*/Ntfs2FAT/merge-resize/FatFormat/Burn — not clone/preos; the WinPE-media-builder OOS family (I-010:10, SURFACE-INVENTORY §5) never names Boot.dll/BootDriver.dll/peformat.dll; I-027 registers only the AliyunConfig.ini caps asset, not the Aliyun binaries.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** the N-130 precedent (VideoFileRepairer.exe, filed by the parallel session-10 lane) established the pattern; the SURFACE-INVENTORY "corpus CLEAN" claim was verified at the artifact level — this finding is the decompile-tree-level completion (103 binaries censused; the engine-bearing trees are clean; the residue is exactly these product-frame surfaces).

## Summary

A decompile-tree-level disposition census of all 103 corpus binaries (nine ghidra trees) found six substantive never-dispositioned surfaces — all product-frame: the disk-clone/PreOS host (CloneModule.dll, the second-largest host DLL at 2,321 named functions), the LibTomCrypt legacy crypto substrate (legacy.dll, role undecoded — plausibly legacy .rsf/session or license decryption), the component downloader (EuDownloader.exe), and the boot-media family (Boot.dll, BootDriver.dll, peformat.dll) — plus six telemetry-chain binaries covered only at family level. The family registers exist (EPM tool ops, WinPE media builder, telemetry OOS) but none of these names was ever folded in. No engine parity risk except CloneModule's shared MBR/GPT-convert write path with S-029's apply lane and legacy.dll's undecoded role.

## What the reference does

Each binary is a shipped og product component (see the per-binary RTTI/string evidence in the census file). CloneModule.dll implements disk/partition cloning and WinPE PreOS environment construction, authoring media BCD via Win32Bcd.dll (its sole importer). legacy.dll embeds LibTomCrypt (PBKDF1, DES) — the pre-CryptoAPI crypto substrate whose consumer role the corpus has not decoded. EuDownloader.exe is the curl-based on-demand fetch engine behind DRWUI's tools.zip/BootRepair downloads. Boot.dll/BootDriver.dll/peformat.dll are the "Create Recovery USB/CD" media-builder chain (boot-file editing, IMAPI2 burning, WinPE prep/formatting) — a different target than M-040's post-recovery boot repair.

## What our core does

No counterpart for any of the six (correctly — they are outside the recovery-engine scope), but unlike every other out-of-scope family, none is REGISTERED: no PARITY-DELTAS row, no I-010/I-019/I-027 entry, no SURFACE-INVENTORY row names them. The trail's own completeness convention (every corpus-documented surface is either dispositioned, registered OOS, or a finding) is violated by these six names.

## Gap

Falsifiable claim: for each of the six binaries, no file under diff-to-fix/ dispositions it by name (legacy/EuDownloader/Boot/BootDriver/peformat: zero grep hits; CloneModule: importer-context only), and no register row names the clone/preos or downloader families.

## Impact

Register completeness only — future sessions re-derive these surfaces from scratch without the register (the exact cost the trail's register convention exists to avoid). The CloneModule overlap note prevents a future clone-capability session from re-walking S-029's already-audited write path. legacy.dll's undecoded role is the one open decode (tracked below).

## Reproduction / verification

Decompiled evidence (strength 2): the per-binary RTTI/string greps re-run by the lead (census file §2); the disposition greps (§1 table + the per-binary trail-hit counts). The 103-binary census method and per-tree verdicts: census file §1 (the T4 read-agent's enumeration, every strong candidate spot-verified by the lead).

## Fix plan (for the engineering team)

1. Register the families (one docs-only pass): PARITY-OG-CORE-INVENTORY / PARITY-DELTAS — add the clone/preos tool-op family row (I-019's exclusion, extended with CloneModule.dll + the shared `commonmbrgptconvert.cpp` note cross-referencing S-029's apply lane); add Boot.dll/BootDriver.dll/peformat.dll to the WinPE-media-builder exclusion (I-010:10) with the media-vs-repair distinction from M-040's family; add EuDownloader.exe + the six telemetry binaries to the product-infra/REPORTING OOS register (I-027's neighbors).
2. legacy.dll: register the LibTomCrypt fingerprint + the open decode question (consumer unknown: legacy session import? license blobs?) — either decode it in a corpus session or register it as permanently-undecoded with the fingerprint on file.
3. No core code change (nothing to implement — all six are out of the engine scope).

## Tests to add

- None (register-only).

## Related findings

- N-130 (the VideoFileRepairer precedent — same class, filed by the parallel lane).
- S-029 (the apply/write path whose MBR/GPT-convert overlap CloneModule shares).
- M-040 (the boot-repair family the media-builder family must be distinguished from).
- I-019/I-010/I-027 (the registers this finding completes).

## Open questions

- legacy.dll's consumer role (the decode: trace its 118 named functions' imports/callers in the og's load graph — the deepening-batch1 §4 hypothesis list is the starting point). Registered in the fix plan as the one open decode; not blocking the register rows.


---

## ADDENDUM (session-10 S10-F, 2026-10-29) — the sfc registration (OQ-S10-2's resolution)

The register gains the VideoViewer/sfc payload (211,614 B, " cfs-" magic +
30 "FAN." markers, opaque compressed/encrypted content): the OQ-S10-2
decisive probe (this session) found ZERO consumers across every reversed
binary and string table — **dead-shipped installer-chain payload** (the
PE_Tools/dlconfig class), registered here per the OQ's hypothesis 3. The
OQ file is closed with the full probe record
(findings/FIXED/OQ-S10-2-videoviewer-sfc-unknown-payload.md).
