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

## Resolution (stage b + the win layer landed — the finding CLOSES on the implemented family)

**Status:** FIXED — stages (a)+(b) implemented and proven; (c)/(d) registered dispositions (D-90); the host wiring boundary documented
**Fixed in commit:** stage (a) `94c9dda` (DFX34 lane); stage (b) + the win layer `da46b14` (this lane)
**Verified by:** stage (a): BcdFix.* 5 tests (the stage-a Resolution above). Stage (b): NvramFix.* 11 tests — `NvramFix.MatchesMbrBySignatureStartSize`, `NvramFix.MatchesGptByGuidAlone` (corrected from the mis-decoded AndStart pin), **`NvramFix.PatchUpdatesLoadOption`** (the finding's fifth named test), `NvramFix.UnmatchedEntriesUntouched`, `NvramFix.ConsentDefaultDeny`, `NvramFix.UndoRestoresOriginalVariables`, `NvramFix.SetFailureStopsAndReports`, `NvramFix.MalformedDevicePathBounded`, `NvramFix.NullSeamRejected`, `NvramFix.AbsentBootOrderIsSilentNoop` (corrected - FUN_180004b40 :3629 wraps the walk in if(uVar10 != 0): absent BootOrder is the og's silent no-op), `NvramFix.EmptyPlanIsNoOp` — all fail-before by absence (the module did not exist). Suites 881/881 linux-release + 881/881 linux-gcc ASan+LSan + 35/35 cr_ui_tests at the landing tree. The win layer compiles on the Windows CI lane (MSVC /W4 /WX; the pure-module tests run there too).
**Resolution notes:** Stage (b) — the UEFI NVRAM Boot####/BootOrder updater — landed as the pure module `cr/nvramfix` (include/cr/nvramfix + src/core/nvramfix) re-implemented from a fresh first-hand decode of EFIBoot_decompiled.c this session: the matcher FUN_1800046f0 (:3366-3445) walks each BootOrder entry's EFI_LOAD_OPTION (description skip: ONE step past the NUL — the corpus's `while(*p)p++;p++`), matching MEDIA_HARDDRIVE nodes (PartitionNumber u32@4 / Start u64@8 / Size u64@16 / Signature[16]@24 / MBRType u8@40 / SignatureType u8@41) on MBR-mode signature+start+size (the flags collected across the WHOLE walk - they may ride different nodes) and GPT-mode GUID ALONE (CORRECTED post-landing against the corpus - the GPT branch :3417-3424 compares the two GUID u64s and never the start; see the convergence addendum below); the rewrite (FUN_180004b40's patch loop :3660-3700) overwrites PartitionNumber, PartitionStart, PartitionSize and Signature with the new partition's values in every matched entry and writes the variable back through the injected seam. No create/delete (the finding's v1 scope). The win layer (`src/win/cr_bootfix.cpp` + `include/cr/win/cr_bootfix.h`) completes BOTH stages' Windows sides: the firmware seam resolved exactly like the og (GetModuleHandleW+GetProcAddress, EFIBoot :3281-3290 — zero new static imports, the Win7 allowlist gate untouched), SeSystemEnvironmentPrivilege via ntdll!RtlAdjustPrivilege (the og's XP path :3248-3270), the stage-a file side (OVERLAPPED positional writes + FlushFileBuffers durability tail), and the \Registry\Machine\BCD00000000 force-unload retry on sharing violation (ZwUnloadKey under SeRestore/SeBackup, Fixup :533/:542/:243-255). The S-029 post-apply WIRING is the host/UI-layer boundary — the og's own boot repair runs in the product host (MainModule's apply chain), not the engine library; the win layer exposes the API that wiring consumes, and the apply plan's `boot_repair_available` field remains the truthful consent-surface marker until a host consumes it. Stages (c) boot.ini and (d) WinChkdsk stay at their registered dispositions (PARITY-DELTAS D-90: boot.ini skippable — Win7 boots BCD; ChkDsk.exe delegation is UI-layer — the core's contract ends at the undo record). With (a)+(b) implemented and (c)/(d) registered, the finding's family is fully dispositioned: CLOSED.

---

## Convergence addendum (the correction wave — four corpus-proven fixes on the canonical landing + the two-lane record, 2026-10-26)

Two lanes implemented stage (b) independently in the same window; the
canonical landing above (da46b14, `cr/nvramfix` + the win seam) pushed
first and stands. A convergent lane's module (cr/efifix, the same
injected-seam design, 9 proof tests including the finding's named
NvramPatchUpdatesLoadOption) was dropped per first-landing-wins; its
decode evidence and this session's reconciliation are recorded in the
worklog (DFX36-T4/T5).

The decode-correction wave (ported onto the canonical module) proved
FOUR divergences in the original landing, each fixed against the
corpus:

1. **GPT match: GUID ALONE.** FUN_1800046f0 :3417-3424 compares the
   two GUID u64s (node+0x18 vs ctx+0x34, node+0x20 vs ctx+0x3c) and
   breaks with a match immediately; no start/size compare exists in
   the GPT branch (the start is REWRITE PAYLOAD, not a match key).
   The original "GUID AND start" reading was a mis-decode.
   `NvramFix.MatchesGptByGuidAlone`.
2. **MBR match: WALK-LEVEL flags.** bVar3/bVar4 are collected across
   the whole device path (:3431-3434) and may ride different nodes;
   the original per-node AND was stricter than the og.
   `NvramFix.MbrFlagsMayRideDifferentNodes`.
3. **Absent BootOrder: SILENT NO-OP**, not a read failure
   (FUN_180004b40 :3629 - `if (uVar10 != 0)` wraps the whole walk).
   `NvramFix.AbsentBootOrderIsSilentNoop`.
4. **A missing Boot#### variable STOPS the walk** (FUN_1800046f0
   rc=2 on a zero-byte get -> the caller breaks the whole loop
   :3648; earlier planned entries stand). The `stopped_early` out-
   param is the observability superset - the og signals nothing.
   `NvramFix.MissingBootVariableStopsTheWalk`.

Plus `kMaxBootEntries = 0xffff` (the og's ushort entry-count
truncation :3637) and the `rrecov apply --boot-fix` post-apply wiring
(plan -> consent -> execute; failures advisory - the table write
already succeeded, the og's posture). The full-position rewrite
(FUN_180004b40 :3660-3700: PartitionNumber, PartitionStart,
PartitionSize, Signature in every matched option's HARDDRIVE nodes)
is the canonical update pass; the renumber-only sibling
(:3520-3572, PartitionNumber alone) stays unimplemented - it serves
the og's renumber flow, not the moved-partition repair.

The current proof roster (13 NvramFix tests): MatchesMbrBySignature-
StartSize, MatchesGptByGuidAlone, MbrFlagsMayRideDifferentNodes,
PatchUpdatesLoadOption (the finding's named fifth), UnmatchedEntries-
Untouched, ConsentDefaultDeny, UndoRestoresOriginalVariables,
SetFailureStopsAndReports, MalformedDevicePathBounded, NullSeam-
Rejected, AbsentBootOrderIsSilentNoop, MissingBootVariableStopsTheWalk,
EmptyPlanIsNoOp.
