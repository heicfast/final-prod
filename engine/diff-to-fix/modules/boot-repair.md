# boot-repair — the post-recovery boot-chain family (M-040; the finding's new module row)

The og's five-component family (Fixup.mo BCD surgery + EFIBoot.mo NVRAM +
Win32Bcd media authoring + WinChkdsk delegation + the DRWUI Boot Repair
tool) against the core's staged port.

| Parameter | Verdict |
|---|---|
| P25 BCD cell patching | **MATCH (family complete)** — `cr/bcdfix` (s34 DFX34): the corpus's own cell decode (0xffffffa0 cells; byte-offset u64 @+0x24, LBA = value>>9; MBR signature u32 @+0x3C; Fixup :499-599 re-verified first-hand) + the `new_lba << 9` rewrite (:957-992) + plan/execute/undo over an injected writer + the P62 consent gate; `src/win/cr_bootfix.cpp` (s33b DFX33b): bcd_rewrite_file (the og's open posture, consent-gated execute, FlushFileBuffers tail :263-306) + unload_bcd_hive (SeBackup/SeRestore + ZwUnloadKey \Registry\Machine\BCD00000000 :527-546, ntdll dynamic — no new PE import) |
| P42 public surface | **MATCH (stage b landed, s33b DFX33b)** — `cr/nvramfix` over the injected IFirmwareVarStore (FwVarStore in the win layer: dynamic Get/SetFirmwareEnvironmentVariableW :3281-3290 + SeSystemEnvironmentPrivilege :3248-3270): the matcher (FUN_1800046f0: MBR sig+start+size flags across the whole walk / GPT GUID) + the state-4 update (FUN_180004840: PartitionNumber patched in every HARDDRIVE node of matched options; create/delete stays out per the plan); `rrecov apply --boot-fix <old-lba>` is the post-apply wiring |
| boot.ini ARC repair | **REGISTERED SKIPPABLE (D-90)** — Win7 boots BCD; the Win7-inclusive envelope has no legacy consumer |
| post-op chkdsk | **REGISTERED UI-LAYER (D-90)** — external-binary delegation is the host's; the core's contract ends at the byte-exact undo record |
| P54/P55 module boundaries | the pure rewriter rides the reghive precedent (pure module consumed by the win layer); the family never links into the engines |
| P62 consent | **MATCH** — the rawfixer model verbatim: no consent callback = nothing is ever written; only an explicit allow executes a plan; undo is the host-invoked restore from the recorded original bytes |
