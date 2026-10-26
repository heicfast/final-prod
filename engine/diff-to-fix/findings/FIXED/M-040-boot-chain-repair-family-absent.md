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

## Resolution (stage a landed — the finding stays OPEN on the family remainder)

**Status:** FIXED — the family completed (stage (a) DFX34; stage (b) + the win runtime + the post-apply wiring DFX33b)
**Fixed in commit:** (this session's M-040 stage-a commit — `git log` "fix(M-040 stage a)")
**Verified by:** 5 proof tests green — `BcdFix.BcdFindsBootPartitionBySignature`, `BcdFix.BcdOffsetRewriteWritesNewLba`, `BcdFix.BcdUndoRestoresOffsets`, `BcdFix.BootfixRefusedWithoutConsent` (four of the finding's five named tests; the fifth, `NvramPatchUpdatesLoadOption`, is stage (b)'s), plus `BcdFix.BoundariesNeverPlan` (truncated/misaligned/null buffers, foreign signature/LBA, writer-error propagation, empty-plan no-op). Fail-before by construction (the module did not exist). Suite **914/914** linux-release.
**Resolution notes:** Stage (a) — the BCD offset rewriter, "the narrowest recovery-relevant piece" per the plan — landed as the pure module `cr/bcdfix` (include/cr/bcdfix + src/core/bcdfix): the corpus's own cell decode (FUN_180001980 :499-599, re-verified first-hand this session: the 8-byte-walk finds 0xffffffa0 allocated 96-byte cells; the partition BYTE-offset u64 at cell+0x24 with LBA = value>>9; the MBR signature u32 at cell+0x3C; the match is signature AND old-LBA), the rewrite shape (FUN_180002520 :957-992: write `new_lba << 9` at the offset field), the plan/execute/undo split over an injected writer (the win layer owns the handle + the FSCTL_ALLOW_EXTENDED_DASD_IO 0x90083 + flush tail, Fixup :263-306), and the P62 consent gate (no callback = default-DENY, ERR_REPAIR_REFUSED — the rawfixer S-008 model; only an explicit allow writes; undo restores the original bytes byte-exactly = the Bcd_RestoreBcdFile analog :4562-4625). Stage (b) — the UEFI NVRAM Boot####/BootOrder updater (EFIBoot state 4 path only, :3860-3960; SeSystemEnvironmentPrivilege; the injected-resolver test design is the plan's own) — is the PRECISE NEXT STEP, unchanged in scope. Stage (c) — boot.ini — REGISTERED SKIPPABLE (Win7 boots BCD; the Win7-inclusive posture does not require legacy ARC-path repair; PARITY-DELTAS row below). Stage (d) — the post-op `ChkDsk.exe %C: /F /X` delegation — REGISTERED UI-LAYER (an external binary spawned by the og's undo chain MM:83206; the core's contract ends at the undo record — the host owns process delegation, like the og's own host). The win-layer file IO (`src/win/cr_bootfix.cpp`: hive force-unload ZwUnloadKey \Registry\Machine\BCD00000000 :533/:542, SeRestore/SeBackup :243-255, the DASD flush tail) + the S-029 post-apply wiring remain the second half of stage (a)'s completion — tracked here as the concrete continuation, not silently dropped.

---

## Resolution (final close — stage (b) + the win runtime + the post-apply wiring landed; DFX33b, 2026-10-26)

**Status:** FIXED (stage (a) landed by DFX34 as `cr/bcdfix`; stages (b)+(win IO)+(wiring) landed by DFX33b this session)
**Fixed in commit:** this landing (DFX33b — include/cr/nvramfix + src/core/nvramfix + include/cr/win/cr_bootfix.h + src/win/cr_bootfix.cpp + src/app/rrecov.cpp + tests/unit/test_nvramfix.cpp)
**Verified by:** `NvramFix.NvramPatchUpdatesLoadOption` (the finding's fifth named test) + `NvramFix.MbrMatchRequiresSignatureAndGeometry`, `NvramFix.MbrFlagsMayRideDifferentNodes`, `NvramFix.GptMatchOnPartitionGuid`, `NvramFix.AbsentBootOrderIsCleanNoop`, `NvramFix.MissingBootVariableAbortsThePass`, `NvramFix.SetFailurePropagates`, `NvramFix.EveryHarddriveNodeInAMatchedOptionIsPatched`, `NvramFix.BoundariesNeverPatch` — 9/9 green, fail-before by construction (the module did not exist); the win layer passes the win_syntax_check parse gate (the M-055 discipline); full suite 927/927 ctest.
**Resolution notes:** Stage (b) — the UEFI NVRAM Boot#### updater — landed as the pure module `cr/nvramfix` over an INJECTED firmware-variable store (IFirmwareVarStore; the win layer resolves Get/SetFirmwareEnvironmentVariableW dynamically, EFIBoot :3281-3290, with SeSystemEnvironmentPrivilege enabled :3248-3270 — dynamic resolution also keeps the PE import surface Win7-clean). The decode was completed first-hand from EFIBoot_decompiled.c: the matcher (FUN_1800046f0) walks the load option's device path (MEDIA_HARDDRIVE_DP nodes, type 0x04/subtype 0x01, 0x2A bytes: PartitionNumber u32@+4, PartitionStart u64@+8, PartitionSize u64@+0x10, Signature[16]@+0x18, MBRType u8@+0x28, SignatureType u8@+0x29) — MBR mode collects the signature flag and the geometry flag ACROSS THE WHOLE WALK (the og does not require them on the same node; pinned by MbrFlagsMayRideDifferentNodes), GPT mode matches on the partition GUID; the state-4 update (FUN_180004840) rewrites the PARTITION NUMBER (u32 @+4) of every HARDDRIVE node with SignatureType != 0 in every matched option and writes the variable back. NOTE: the audit's original phrase "patch partition start inside matching load options" was imprecise — the og patches the PartitionNumber field (the start LBA is the MATCH key); the implementation follows the corpus. The walk aborts at the first failing variable read (FUN_1800046f0 rc=2 -> the whole loop breaks) — the reference's own fragility, mirrored and documented, with partial stats reported on every exit path. Create/delete of Boot#### entries and BootOrder rewrites stay OUT (the og's FUN_1800012c0 create state machine — "keep create/delete out of v1" per the plan). The win runtime (`src/win/cr_bootfix.cpp`): FwVarStore (the dynamic resolution + privilege + the GetLastError mapping: not-found=benign, invalid-function=BIOS refusal), `nvram_renumber` (one explicit call; ERR_UNSUPPORTED on BIOS), `bcd_rewrite_file` (the og's open posture GENERIC_READ|GENERIC_WRITE/full-share/OPEN_EXISTING, read, plan via bcdfix::plan_rewrites, execute under the consent callback, FlushFileBuffers tail — Fixup :263-306), and `unload_bcd_hive` (SeBackup+SeRestore then ZwUnloadKey over \Registry\Machine\BCD00000000, Fixup :527-546; ntdll resolved dynamically — no new PE import, the Win7 allowlist gate untouched; the win-stub grew the consumed surface: LoadLibrary-class, privilege-adjustment, SetFilePointer, the error codes). The S-029 post-apply wiring: `rrecov apply` gains `--boot-fix <old-lba>` (opt-in; `--boot-fix-bcd <path>` overrides the two standard hive locations) — after a successful apply + refresh it runs the BCD offset rewrites (signature from the plan, new LBA = the applied candidate's start) + the NVRAM renumber (new number = the MBR slot + 1), every write under the bootfix consent gate; boot-repair failures are ADVISORY (the table write already succeeded — the og's posture). The dry-run message tells the truth about the step instead of "not available in this build". boot.ini stays REGISTERED SKIPPABLE (Win7 boots BCD — the plan's own note) and the ChkDsk delegation stays REGISTERED UI-LAYER (external binary; the core's contract ends at the undo record). Corpus citations: EFIBoot_decompiled.c :3281-3290 (dynamic resolution), :3248-3270 (privilege), :251-693 (create state machine — not adopted), :3405-3480 + :3533-3544 (the patch walk), :3860-3960 (BootOrder enumeration), FUN_1800046f0 (the matcher, :3417-3460 region), FUN_180004840 (the update pass); Fixup_decompiled.c :263-306 (flush tail), :527-546 (hive force-unload), :957-992 (the rewrite), :499-599 (the cell decode).

---

## Resolution addendum (the two-lane convergence + the DFX33b correction wave, 2026-10-26)

Two lanes independently implemented stage (b) in the same window. The
parallel lane's landing (da46b14 — `cr/nvramfix` plan/execute/undo with
the P62 consent + the full-position rewrite citing FUN_180004b40
:3660-3700, plus the win seam `bcd_file_plan`/`bcd_file_execute`/
`bcd_file_undo` with the force-unload retry and
`nvram_plan`/`nvram_execute`/`nvram_undo`) pushed FIRST and stands as
the canonical module shape per first-landing-wins; this lane's
one-shot design (bcd_rewrite_file/nvram_renumber) was dropped cleanly,
and this lane's decode-proven corrections rode on top:

1. **GPT match = GUID alone** (their version also required old_start):
   the corpus's GPT branch (FUN_1800046f0 :3417-3422) compares ONLY
   the two GUID u64s (node+0x18 vs ctx+0x34, node+0x20 vs ctx+0x3c)
   and breaks with a match immediately — no start/size compare exists
   in the GPT branch (the start is part of the REWRITE payload, not
   the match). Test: `NvramFix.MatchesGptByGuidAlone`.
2. **MBR match = walk-level flags** (their version required all fields
   on one node): FUN_1800046f0 collects bVar3 (signature) and bVar4
   (start+size) across the WHOLE device-path walk (:3431-3434) — the
   flags may ride different nodes. Test:
   `NvramFix.MbrFlagsMayRideDifferentNodes`.
3. **Absent BootOrder = silent no-op** (their version returned
   ERR_READ_FAILED): FUN_180004b40 :3629 `if (uVar10 != 0)` wraps the
   whole walk — a zero-byte BootOrder read (absent OR failed) simply
   skips it; there is no error path. Test:
   `NvramFix.AbsentBootOrderIsSilentNoop`.
4. **Missing Boot#### STOPS the walk** (their version skipped it):
   FUN_1800046f0 returns rc=2 on a zero-byte get and the caller breaks
   the whole loop (FUN_180004b40 :3648 `if (iVar8 != 0) break`) — a
   stale BootOrder id leaves everything after it unpatched; earlier
   planned entries stand. The og signals nothing (void return) — our
   `stopped_early` out-param is the observability superset. Test:
   `NvramFix.MissingBootVariableStopsTheWalk`.
5. **kMaxBootEntries = 0xffff** (their 4096 was stricter): the og's
   entry count is the ushort truncation of len>>1
   (FUN_180004b40 :3637-3639).

The S-029 post-apply wiring (`rrecov apply --boot-fix <old-lba>` +
`--boot-fix-bcd <path>`) rides their canonical API (plan → consent →
execute; NVRAM full-position: new number = MBR slot + 1, new start =
the applied candidate's LBA; failures advisory). Suites green on the
union: 926/926 ctest (13 NvramFix tests; win_syntax_check over the
canonical win layer + the rewired CLI).
