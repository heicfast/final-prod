# MAJOR M-113 — the recovered-OS boot-environment write-side surgery is absent and unregistered: the og's genuefisystemfile.cpp chain (MountedDevices rewrite, BCD bootmgr device-element re-pointing, SOFTWARE-hive surgery, SystemSetup/bootstat) — the trail's R-69 "PRESENT" verdict covers only BCD-cell and NVRAM re-pointing

**Severity:** MAJOR (a first-class capability gap in the boot-repair family: a recovered Windows whose boot device identity changed will not boot when recovered by core, where the og fixes it; the trail's own gap-board marks the family PRESENT, so the gap is also a doc-truth defect)
**Module:** boot-repair / apply (the recovery-side pending-op chain)
**Parameters affected:** P1-adjacent (recovery correctness — the recovered OS boots or not), A54-58 (the apply/bootfix module boundary), D46 (documentation of the contract — R-69's over-coarse verdict)
**Corpus references:** `ghidra-partition/RecoveryModule_decompiled.c:73731-73786` — the MountedDevices rewrite: builds `"%c:\Windows\System32\Config\System"` (`:73759`), `RegLoadKey("TempSystem", path)` (`:73762-73763`), opens `"TempSystem\MountedDevices"` (`:73731`), writes (`:73777`), unloads (`:73784-73786`); the trace-file string `"i:\_drw_main\partitionrecovery\mod.native\genuefisystemfile.cpp"` with line tags 0x1d9-0x1e5 (`:73739-73776`); `:74837-74887` — the BCD surgery: opens `"%s\EFI\Microsoft\Boot\BCD"` (`:74837`), loads it as hive `"TmpBCD"` (`:74851-74852`), reads the Windows-Boot-Manager object's `Objects\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\Elements\23000003\Element` (`:74855`), rewrites the referenced object's device element `TmpBCD\Objects\%s\Elements\12000004\Element` as REG_BINARY (`:74875-74878`), unloads (`:74887`); `:73610-73621` and `:76496-76504` — the SOFTWARE-hive surgery (`"TmpSoftWare"`, `"TmpSoftWare\Microsoft\Windows NT\CurrentVersion"`, `"ProductName"`); `:42851` — `"System\Setup"`/`"SystemPartition"` and `"\Windows\bootstat.dat"`; the same code static-linked in MainModule (strings ×6 "TmpBCD"/×6 "TempSystem"/×5 "TmpSoftWare"), PartitionRecovery.dll, ToolsModule, CloneModule, UILogic
**Core references:** audited `461825d8`-lineage — the whole-repo grep `TmpBCD|TempSystem|TmpSoftWare|bootstat|9dea862c|23000003` → **zero hits** (src/, include/, docs/, diff-to-fix/ — verified by the Lane D agent and re-verified by the lead); `src/win/cr_bootfix.cpp:174-303` (the current surface: `bcd_file_plan/apply/execute/undo` — BCD **cell** LBA re-pointing + NVRAM only); `include/cr/reghive/cr_offlineos.h` (the READ-side profile: MountedDevices read, no writes); `docs/REFERENCE-API-GAP.md:125` (R-69 names the og family: `CGenUEFISystemFile/CGenBIOSSystemFile`, `CBcdAccess/CBcdBootProc/CBcdEditProc`, `CRebuildMBR`, `CExtendBootPart`) and `:293` (the "PRESENT (bcdfix/nvramfix)" verdict this finding refines)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11-lead Lane D tree sweep (the EPM-heritage shared layer's never-compared regions); NOT covered by M-040 (cr_bcdfix — BCD cell re-pointing only, a different mechanism at a different level), N-121 (ESP auto-mount — the mount, not the surgery), D-107 (the offlineos profile READ + its registered follow-up "cr_bootfix taking the BCD path from the profile's MountedDevices location" — the read-side seed of this write-side family), D-90 (boot.ini + chkdsk), S-029/cr_apply (the apply transaction, not the boot surgery), or S-030 (image mount).

## Summary

The og's recovery-side pending-op chain performs offline registry-hive surgery on the RECOVERED Windows installation so that it actually boots on new hardware/disk layout: it loads the recovered SYSTEM hive as "TempSystem" and rewrites MountedDevices; it loads the recovered EFI\Microsoft\Boot\BCD as hive "TmpBCD", walks the Windows-Boot-Manager object's device reference, and rewrites the target boot application's device element (12000004) to the new device; it loads the SOFTWARE hive ("TmpSoftWare") and touches CurrentVersion, SystemSetup/SystemPartition, and bootstat.dat. None of this exists in core — the repo-wide grep is zero — and the trail's own gap-board marks the family "PRESENT (bcdfix/nvramfix)", a verdict that covers only BCD-cell LBA re-pointing and NVRAM entries, a different and much smaller mechanism.

## What the reference does

Four documented surgery operations (all decompile-cited above, from the
static-linked genuefisystemfile.cpp unit present in six EPM DLLs):
1. **MountedDevices rewrite** — RegLoadKey the recovered SYSTEM hive under
   "TempSystem", write the disk-signature mappings under
   TempSystem\MountedDevices so the recovered OS resolves its boot disk, unload.
2. **BCD device-element re-pointing** — load the BCD file as hive "TmpBCD", read
   the Windows-Boot-Manager object's {9dea862c…} 23000003 element (the
   boot-application reference), and rewrite that application's 12000004 device
   element (REG_BINARY) to the new boot device, unload. (Core's cr_bcdfix instead
   re-points LBA-valued cells inside the BCD binary — useful when the partition
   moved, useless when the disk identity/GUID changed, which is the
   device-element case.)
3. **SOFTWARE-hive surgery** — "TmpSoftWare" + CurrentVersion/ProductName.
4. **SystemSetup / SystemPartition / bootstat.dat** touches (first-boot
   re-setup semantics).

## What our core does

`cr_bootfix.cpp` implements `bcd_file_plan/apply/execute/undo` (BCD cell
re-pointing) + the NVRAM fix; `cr_offlineos.h` reads the offline profile
(MountedDevices read-side); nothing anywhere writes a loaded hive, touches a BCD
device element, or handles SystemSetup/bootstat. The whole-repo grep for the
og's hive-aliases and object GUIDs is zero.

## Gap

Falsifiable: recover a Windows system partition to a new disk whose signature/GUID
differs from the source (the standard bare-metal-restore scenario). The og's apply
chain re-points the BCD device element and rewrites MountedDevices — the recovered
OS boots. Core's apply+bootfix re-points BCD LBA cells and NVRAM — the recovered OS
hangs at 0xc000000e (boot device inaccessible) because the device element still
names the old disk. The R-69 row at REFERENCE-API-GAP.md:293 says "PRESENT" for a
family whose write-side half is absent.

## Impact

Bare-metal restore (disk replaced, or system partition recovered to different
hardware) — the flagship scenario for a partition-recovery product's "recover the
whole system" flow — produces an unbootable result in core where the og produces a
booting one. This is user-observable failure of a first-class capability, plus a
gap-board truth defect (R-69's verdict) that would mislead future parity sessions
into not scheduling the work.

## Reproduction / verification

Decompiled evidence (Lane D agent's sweep; the load-bearing cites re-read by the
lead at filing — the TempSystem chain at :73731-73786 and the TmpBCD chain at
:74837-74887 match the agent's quotes). The core-side absence is grep-proven
(zero hits repo-wide). Execution comparison not feasible (needs a full Windows
restore drill); the mechanism divergence is structural and code-cited on both
sides. Full record: `evidence/s11-lead-deep-lanes.md` §Lane-D item D-1.

## Fix plan (for the engineering team)

1. Architectural: extend `cr_bootfix` with a third mechanism beside
   cell-repointing and NVRAM: **hive-load surgery** — a `reghive_writer` module
   (RegLoadKey/RegSaveKey/RegUnLoadKey wrappers, Win32-only, stubbed on Linux as
   the honest ERR_UNSUPPORTED per the platform doctrine) with three operations:
   `rewrite_mounted_devices(system_hive, mappings)`, `repoint_bcd_device_element(
   bcd_path, new_device_bytes)` (the {9dea862c…}→23000003→12000004 walk), and
   `touch_system_setup(software_hive, …)`.
2. Files: new `src/win/cr_reghive_write.cpp` + `include/cr/reghive/cr_reghive_write.h`
   (C++-only like cr_offlineos per D-107's ABI-deferred precedent); the apply
   chain gains a pending-op class mirroring the og's CGenUEFISystemFile dispatch.
3. The device bytes: derive from the apply transaction's target-disk identity
   (the signature/GUID the core already knows from the device layer).
4. Tests: a fixture SYSTEM hive + BCD pair (synthetic, committed under
   tests/data/) — plan/apply/undo round-trips asserting the MountedDevices values,
   the 12000004 element bytes, and the undo restores byte-exact; a Win32-lane
   integration test with reg.exe cross-verification where available.
5. Contract/docs: flip REFERENCE-API-GAP R-69's §b verdict from "PRESENT
   (bcdfix/nvramfix)" to "PARTIAL (cell+nvram landed; the genuefisystemfile
   write-side family tracked by M-113)"; module row in modules/boot-repair.md.
6. Acceptance: the synthetic-hive round-trip suite passes; the R-69 row is
   truthful; a manual drill on a VM (recover system partition to a
   different-signature disk) boots.

## Tests to add

- `Bootfix.MountedDevicesRewriteRoundTrip` / `Bootfix.BcdDeviceElementRepoint` /
  `Bootfix.SystemSetupTouch` (+ undo variants, byte-exact restore).
- `Bootfix.ReghiveWriterRefusedOffWindows` (the honest-refusal pin).

## Related findings

M-040 (the landed cell-repointing — a different mechanism this finding extends);
N-121 (the ESP mount that precedes the BCD surgery); D-107 (the read-side profile
whose registered follow-up anticipated this); S-029/cr_apply (the transaction
framework the new pending-op rides); M-114 (the other MainModule write-side
behavior the same sweep found).

## Open questions

The BIOS twin (`CGenBIOSSystemFile` — R-69 names it) was not decoded this session:
the boot.ini-level coverage (D-90) may or may not absorb it. The fix lane should
decode it before scoping the module (one read of the same RecoveryModule tree,
the sibling function family near :73610-:76600).
