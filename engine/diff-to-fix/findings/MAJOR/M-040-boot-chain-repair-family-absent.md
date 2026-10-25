# MAJOR M-040 — Post-recovery boot-chain repair family (BCD patch + UEFI NVRAM + boot.ini + post-op chkdsk) is absent and undispositioned

**Severity:** MAJOR
**Module:** boot-repair (Fixup.mo/EFIBoot.mo/Win32Bcd/WinChkdsk family) — new module row
**Parameters affected:** P25 (algorithm — BCD cell patching), P35 (error recovery — undo runs chkdsk + BCD restore), P42 (public surface — no repair ops), P54/P55 (module boundaries/dependency direction — link-time repair libs), P60 (bad-sector/volume handling — DASD_IO flush), P62 (consent — write-side, pairs with rawfixer's consent model)
**Corpus references:** `re-workspace/reversed/ghidra-mo/Fixup_decompiled.c` — offset rewrite `FUN_180002520` (:957-992: SetFilePointer −0x1c + WriteFile of `newOffset << 9`) and `FUN_180002eb0` (:1317-1361: disk-signature record scan + offset swap), hive force-unload (`\Registry\Machine\BCD00000000` :533, ZwUnloadKey :542, SeRestore/SeBackup :243-255), Vista+ `"Element"` cells (:1061-1065), `Bcd_RestoreBcdFile` undo (:4562-4625), `Bcd_IsVistaBootManager` (:843-921: 55AA + "BOOTMGR " scan), boot.ini ARC parser/rewriter (:2605+, :2999-3227), `\ArcName` resolution (:2067-2193), post-patch `FSCTL_ALLOW_EXTENDED_DASD_IO` 0x90083 + flush (:263-306); `re-workspace/reversed/ghidra-mo/EFIBoot_decompiled.c` — Get/SetFirmwareEnvironmentVariableW dynamic resolution (:3281-3290), EFI global GUID {8BE4DF61-…} (:3385-4208), Boot####/BootOrder create/patch/delete state machine (:251-693, :3405-3480, :3860-3960, :4130-4273), SeSystemEnvironmentPrivilege XP path (:3248-3270); `re-workspace/reversed/ghidra-partition/MainModule_decompiled.c` — apply chain `FUN_180075580` (:82322-82424: Bcd_Init → BootIni or Bcd unlock/newpos), undo chain `FUN_180077630` (:83117-83212: BootIni_Init, **"ChkDsk.exe %C: /F /X"** :83206, Bcd_RestoreBcdFile :83212), Bcd_Init in pending-op ctors (:31141, ExtendNTFSVolume :77949/:78950 0x900F0); `ghidra-partition/Win32Bcd_decompiled.c` — bcdedit CLI wrapper (/store :1123, /set :606, /enum :5206, /create :6411/:6423, CreateProcessW :213; sole importer CloneModule); `ghidra-partition/WinChkdsk_decompiled.c` — GetProcAddress("Chkdsk") on ufat/untfs (:3490-3492), ulib MESSAGE hooks (:3405-3470); `ghidra-drwui/DRWUI_decompiled.c` — Boot Repair tool gate bShowBootRepair (:1453643, :1453900-1453910), launcher (:1094360-1094373), on-demand download tools.zip (:494832-494869); `ghidra-partition/UILogic_decompiled.c` — RefreshDeviceManager boot re-sync (:7697-7700)
**Core references:** audited `7cf6754` — grep-verified zero hits for BCD/boot.ini/bootmgr/BootOrder/SetFirmwareEnvironmentVariable/8BE4DF61/chkdsk across src/ + include/; ESP exists only as a partition kind (`FsKind::EfiSystem`, include/cr/table/cr_table_parse.h:37); the consent-gated write pattern to reuse lives in rawfixer (cr_ui_repair_consent_fn, S-008 FIX)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og can repair the boot chain after recovery: Fixup.mo binary-patches partition offsets inside `\Boot\BCD` and `\EFI\MICROSOFT\BOOT\BCD` (and rewrites boot.ini ARC paths for legacy installs), EFIBoot.mo creates/patches/deletes UEFI `Boot####`/`BootOrder` firmware variables, Win32Bcd authors media BCD for WinPE, and the operation undo path runs `ChkDsk.exe %C: /F /X` + restores the BCD. This family is wired into the recovery hosts (RecoveryModule/PartitionRecovery.dll/MainModule) and re-synced on every device refresh. The core has none of it — no BCD code, no firmware-variable code, no chkdsk integration — and no prior session or parity doc ever dispositioned the family (FileRepair was excluded by PARITY-CORE-REC.md:164; boot repair was never even listed).

## What the reference does

Fixup.mo (a link-time library imported by six hosts, NOT a MOM module —
corrects the corpus artifact) exposes 13 `Bcd_*` + 6 `BootIni_*` C exports
over two singletons (CBcd for `\Boot\BCD`, CEFIFirstBcd for the ESP's
`\EFI\MICROSOFT\BOOT\BCD`). The core capability is **in-place BCD hive
surgery**: locate the BCD-boot partition by MBR signature + partition
byte-offset inside hive cells (allocated marker 0xffffffa0, offset at
+0x1c, signature at +0x10), then rewrite `newLBA << 9` at the cell's offset
field (FUN_180002520 :957-992). After a partition move/resize,
`Bcd_UnlockBcdFile` reparses and rewrites every device element referencing
the partition; `Bcd_UnlockBcdNewPos` writes the new position; the undo path
`Bcd_RestoreBcdFile` swaps old/new back. Locked hives are force-unloaded
(`ZwUnloadKey` on `\Registry\Machine\BCD00000000` with SeRestore/SeBackup
enabled), and post-patch the system partition is flushed with
FSCTL_ALLOW_EXTENDED_DASD_IO + FlushFileBuffers. EFIBoot.mo manipulates
UEFI NVRAM: enumerate/apply/update/count/delete `Boot####` entries whose
EFI device path matches the target partition (MBR signature or GPT GUID +
start LBA), patching load options in place and rewriting `BootOrder` — via
Get/SetFirmwareEnvironmentVariableW with the EFI global GUID. MainModule's
apply chain runs the family automatically after a layout write (MM:82322-
82424); its undo chain runs chkdsk `/F /X` synchronously + BCD restore
(MM:83206-83212). DRWUI additionally ships a separate downloadable
"Boot Repair" tool behind the `bShowBootRepair` gate.

## What our core does

Nothing in this family exists: zero grep hits for every symbol class. The
core's only write-side recovery capability is rawfixer (boot-sector-level,
consent-gated, S-008/S-010 FIXED). The core classifies an ESP partition
(`FsKind::EfiSystem`) but never mounts, reads, or repairs boot
configuration. No parity document records boot repair as in or out of
scope.

## Gap

The og has a five-component boot-repair family wired into the recovery
flow; the core has none of it and no disposition. Falsifiable: any core
call writing to a `bcd` path, a hive cell offset, or
SetFirmwareEnvironmentVariable — grep returns zero.

## Impact

After recovering or applying a partition (S-029), a Windows system whose
boot entries reference the old partition offsets will not boot — the og
fixes the BCD/NVRAM/boot.ini automatically, the core leaves the system
unbootable. Also affects clone/media workflows (Win32Bcd). The capability
is write-side and dangerous (hive patching, firmware variables), so any
implementation must ride the consent model — but "too dangerous to
implement silently" is a scope decision that must be DOCUMENTED, and it
never was.

## Reproduction / verification

Corpus-derived: every mechanism quote above is decompile-cited and the
load-bearing ones (offset rewrite WriteFile, apply/undo chains, chkdsk
string) were re-read first-hand by the lead this session. Windows-only
behavior — not executable in this session. Evidence:
`diff-to-fix/evidence/decompiled-quotes/s5-mom-capabilities.md` §1-§3, §6.

## Fix plan (for the engineering team)

1. **Disposition first** (blocks everything): decide implement vs document-
   exclusion in PARITY-DELTAS. If excluded: close this finding as
   INFORMATIONAL with the exclusion registered — the finding then exists
   to force that registration.
2. If implemented, stage it: (a) BCD **offset rewriter** only — the
   narrowest recovery-relevant piece: read `\EFI\MICROSOFT\BOOT\BCD` +
   `\Boot\BCD` from the ESP/legacy partition, locate device elements by
   MBR signature + partition offset (Fixup:499-599 decode), rewrite
   `lba << 9` (Fixup:957-992), flush (0x90083). Consent-gated, undo log =
   original 8-byte values + positions.
   (b) UEFI NVRAM entry updater (state 4 path only — patch partition start
   inside matching load options; EFIBoot:3860-3960) — requires
   SeSystemEnvironmentPrivilege; keep create/delete out of v1.
   (c) boot.ini: only if Win7-inclusive posture requires legacy support
   (Win7 boots BCD — likely skippable; document).
   (d) WinChkdsk delegation: out of core scope (external binary) —
   register as UI-layer responsibility.
3. Files: `src/win/cr_bootfix.cpp` + `include/cr/win/cr_bootfix.h`
   (Windows-only, like cr_bitlocker); plan/execute/undo shape mirroring
   cr_rawfixer; wired as an optional post-apply step of S-029's apply
   plan.
4. Tests: `BcdOffsetRewriteWritesNewLba` (fixture mini-hive),
   `BcdFindsBootPartitionBySignature`, `BcdUndoRestoresOffsets`,
   `BootfixRefusedWithoutConsent`, `NvramPatchUpdatesLoadOption` (mocked
   firmware API — inject the resolver).

## Tests to add

The five named tests (firmware-API test via injected resolver).

## Related findings

S-029 (apply path — the trigger for boot repair), M-041 (check phase —
the undo chain's chkdsk pairs with it), I-019 (tool-op boundary), S-010
FIX (the volume-lock discipline the write path must reuse).

## Open questions

Whether PartitionRecovery.dll's apply path calls the full Fixup chain or
only MainModule's op engine does (the composite commonmbrgptconvert flow
cites BootIni_Init at PR:47302 — the exact Bcd_* subset in that flow was
not enumerated this session).
