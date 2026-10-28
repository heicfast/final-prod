# boot-repair — the post-recovery boot-chain family (M-040; the finding's new module row)

The og's five-component family (Fixup.mo BCD surgery + EFIBoot.mo NVRAM +
Win32Bcd media authoring + WinChkdsk delegation + the DRWUI Boot Repair
tool) against the core's staged port.

| Parameter | Verdict |
|---|---|
| P25 BCD cell patching | **MATCH (stages a+b landed)** — `cr/bcdfix` (stage a, s34 DFX34): the corpus's own cell decode (0xffffffa0 cells; byte-offset u64 @+0x24, LBA = value>>9; MBR signature u32 @+0x3C; Fixup :499-599 re-verified first-hand) + the `new_lba << 9` rewrite (:957-992) + plan/execute/undo over an injected writer + the P62 consent gate (default-DENY, ERR_REPAIR_REFUSED — the rawfixer S-008 model). The win-layer file side landed with stage b (`da46b14`): OVERLAPPED positional writes + FlushFileBuffers durability tail + the \Registry\Machine\BCD00000000 force-unload retry (ZwUnloadKey under SeRestore/SeBackup, Fixup :533/:542/:243-255 — dynamic ntdll resolution, zero new static imports) |
| P42 public surface | **MATCH (stage b landed, da46b14 + the correction wave)** — `cr/nvramfix`: the UEFI NVRAM Boot####/BootOrder updater (the FUN_1800046f0 matcher + the FUN_180004b40 full-position rewrite, first-hand decoded) over the injected Get/Set seam; the win layer resolves Get/SetFirmwareEnvironmentVariableW dynamically (the og's :3281-3290 pattern) + SeSystemEnvironmentPrivilege via ntdll!RtlAdjustPrivilege (:3248-3270). No create/delete (v1 scope). 13 NvramFix tests incl. the finding's named NvramPatchUpdatesLoadOption + the four correction-wave pins (GPT GUID-alone, MBR walk-level flags, absent-BootOrder no-op, missing-variable walk stop); the GPT matcher reads GUID ALONE (:3417-3424) and the full-position rewrite is FUN_180004b40 :3660-3700 |
| boot.ini ARC repair | **REGISTERED SKIPPABLE (D-90)** — Win7 boots BCD; the Win7-inclusive envelope has no legacy consumer |
| post-op chkdsk | **REGISTERED UI-LAYER (D-90)** — external-binary delegation is the host's; the core's contract ends at the byte-exact undo record |
| P54/P55 module boundaries | the pure rewriter rides the reghive precedent (pure module consumed by the win layer); the family never links into the engines |
| P62 consent | **MATCH** — the rawfixer model verbatim: no consent callback = nothing is ever written; only an explicit allow executes a plan; undo is the host-invoked restore from the recorded original bytes |


## Session-8 lead-lane wave (audited `ff879f9`)
- **N-121**: the ESP auto-mount half (CUpdateGptSystemBootPartition) — undispositioned.
- **N-122**: the NVRAM create/delete exclusion — recorded only in M-040's resolution text.

### Session-11-lead rows (2026-10-29)

- **M-113** (MAJOR): the recovered-OS boot-environment write-side surgery absent + unregistered — the og's genuefisystemfile.cpp chain (TempSystem MountedDevices rewrite; TmpBCD {9dea862c...}->23000003->device-element 12000004 re-pointing; TmpSoftWare CurrentVersion; SystemSetup/SystemPartition/bootstat). R-69's "PRESENT (bcdfix/nvramfix)" verdict covers only BCD-cell + NVRAM — a different, smaller mechanism. The BIOS twin (CGenBIOSSystemFile) is the fix lane's open decode.

**N-121 (FIXED — registration):** the ESP auto-mount step (the og's CUpdateGptSystemBootPartition / MountGptPartition) is a REGISTERED HOST CONTRACT (PARITY-DELTAS D-90/N-121): the host discovers the ESP by type GUID, mounts it, passes the BCD path, unmounts after (exception-safe); the core's implementation needs the DeviceManager :49956 behavioral decode first — the registered next step.
