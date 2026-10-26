# boot-repair — the post-recovery boot-chain family (M-040; the finding's new module row)

The og's five-component family (Fixup.mo BCD surgery + EFIBoot.mo NVRAM +
Win32Bcd media authoring + WinChkdsk delegation + the DRWUI Boot Repair
tool) against the core's staged port.

| Parameter | Verdict |
|---|---|
| P25 BCD cell patching | **MATCH (stage a landed, s34 DFX34)** — `cr/bcdfix`: the corpus's own cell decode (0xffffffa0 cells; byte-offset u64 @+0x24, LBA = value>>9; MBR signature u32 @+0x3C; Fixup :499-599 re-verified first-hand) + the `new_lba << 9` rewrite (:957-992) + plan/execute/undo over an injected writer + the P62 consent gate (default-DENY, ERR_REPAIR_REFUSED — the rawfixer S-008 model). Pure core only so far: the win-layer file IO (hive force-unload :533/:542, SeRestore/SeBackup, the DASD 0x90083 + flush tail :263-306) is the tracked continuation on the finding |
| P42 public surface | **OPEN (stage b)** — the UEFI NVRAM Boot####/BootOrder updater (EFIBoot state-4 path :3860-3960, SeSystemEnvironmentPrivilege, injected-resolver test design) is the precise next step |
| boot.ini ARC repair | **REGISTERED SKIPPABLE (D-90)** — Win7 boots BCD; the Win7-inclusive envelope has no legacy consumer |
| post-op chkdsk | **REGISTERED UI-LAYER (D-90)** — external-binary delegation is the host's; the core's contract ends at the byte-exact undo record |
| P54/P55 module boundaries | the pure rewriter rides the reghive precedent (pure module consumed by the win layer); the family never links into the engines |
| P62 consent | **MATCH** — the rawfixer model verbatim: no consent callback = nothing is ever written; only an explicit allow executes a plan; undo is the host-invoked restore from the recorded original bytes |
