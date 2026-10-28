# MAJOR M-082 — The og's DeviceManager offline-OS/boot-volume identification is live-module machinery the core lacks — and the trail's MATCH-by-omission verdict scoped only the dead-shipped CheckVersion

**Severity:** MAJOR
**Module:** mom-capabilities / reghive / boot-repair
**Parameters affected:** A54-58 (module boundaries/architecture on the offline-OS surface), S35-adjacent (error recovery around offline installs), B8-adjacent (multi-volume boundaries — system-partition location across disks), Q68 (the mom-capabilities disposition must be re-scoped)
**Corpus references:** `re-workspace/reversed/ghidra-mo/DeviceManager_decompiled.c:51303` (the debug trace `"IsBootVolume-0 nDiskNum = %d uDiskSignture = %d nPartitionNum = %d u64PartStartSector = %lld"` — the boot-volume identification machinery, RTTI `CPartitionBootInfo`); `:62034-62119` (the offline SOFTWARE hive profile: `\Microsoft\Windows NT\CurrentVersion` → BuildLab/BuildLabEx/CSDVersion/CurrentBuildNumber/CurrentVersion/ProductName/ProductId/SystemRoot, 8×0x80 into a 0x418-byte record; SystemRoot lowercased + drive-letter-validated); `:62184-62195` (SYSTEM hive `\MountedDevices\DosDevices\<letter>` → 12-byte value → disk signature + partition offset — the system-partition location); `:62213-62245` (`\Select\Current` → `\ControlSet<NNN>` with ControlSet001 fallback); `:62275+` (`ProductOptions` lstrcmpiA "Terminal Server" — server-vs-client); `:62854-62920` (drive-letter root → FindFirstFileA top-level sweep probing Windows dirs); hive-mount classes tagged `"SOFTWARE_EASEUS_HMG"`/`"SYSTEM_EASEUS_HMG"` (:62021/:62025); module load status: by-name `FUN_1800017f0("DeviceManager")` evidenced live (`diff-to-fix/evidence/decompiled-quotes/s25-oq-s5-1-checkversion-dead-shipped.md:13`)
**Core references:** audited `35adcb9`. `src/core/reghive/` is BitLocker-FVE-only (`cr_reghive.h:5-12`; harvest walks `\ControlSet001\Control\FVEAutoUnlock` only; the header itself notes the og "does NOT consult the Select key" for FVE — cr_reghive.h:102-104 — whereas DeviceManager **does**: `:62213-62245`). `src/win/cr_bootfix.cpp:126` takes the BCD path as input (`open_hive(path,…)`) — no install discovery. No trail finding greps IsBootVolume/PartitionBootInfo/EASEUS_HMG (verified zero hits trail-wide).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The trail's only verdict on offline-OS awareness is mom-capabilities.md:102-115 — "MATCH-BY-OMISSION (dead-shipped: OQ-S5-1 RESOLVED s25)" — but that disposition's evidence scoped the *CheckVersion.mo* copy of the machinery, which is dead-shipped. The **DeviceManager** copy is in a module the same s25 evidence proves is loaded by name, and it is the full offline-OS/boot-volume identification stack: offline SOFTWARE+SYSTEM hive profiling of candidate Windows installs (version, ProductName, SystemRoot), `\MountedDevices` system-partition mapping to disk-signature+offset, ControlSet resolution via `\Select\Current` (with the 001 fallback), server-vs-client detection, and a per-directory installer sweep. The core has no counterpart — its offline-hive work is FVE-keys only, and its boot-fix takes a caller-supplied BCD path. The disposition must be re-scoped and the capability decision made explicitly.

## What the reference does

DeviceManager's `CPartitionBootInfo` chain (decompile cites above): given a candidate partition, it composes `<win>\System32\config\{SOFTWARE,SYSTEM}`, mounts them via the in-house HMG classes, reads the OS identity fields (8 strings from CurrentVersion), resolves the real ControlSet via `\Select\Current` (fallback 001), reads the system partition's location from SYSTEM's `\MountedDevices\DosDevices\<sysroot-letter>` (12-byte value = disk signature + partition offset), classifies server-vs-client via `ProductOptions`/"Terminal Server", and reports `IsBootVolume`-class results with disk/partition/start-sector coordinates. Callers sweep per-partition and per-drive-letter-directory. This identifies *which partition is the (offline) system partition* and *what Windows it hosts* — the input to boot repair, offline-OS-targeted recovery, and PE-context flows.

## What our core does

`reghive` harvests FVE auto-unlock keys from offline hives (hardcoded ControlSet001, by design for the FVE case). `cr_bootfix` repairs BCD stores the caller points it at. Nothing identifies an offline OS install, resolves its system partition, or profiles it. The mom-capabilities verdict covering this surface cited CheckVersion's dead-shipped status and scoped the core's absence of "OS-install classification" to that module.

## Gap

The core cannot identify an offline Windows installation or its system partition: no code path profiles offline SOFTWARE/SYSTEM hives for OS identity, resolves `\Select\Current`, or maps `\MountedDevices` to a partition. Falsifiable: given a disk image with an offline Windows install and no caller hint, the og's DeviceManager locates the system partition (disk signature + offset) and OS identity; the core has no equivalent operation.

## Impact

Architectural/roadmap-level: boot-repair workflows (BCD fix currently needs the caller to know the path), any future offline-OS-targeted recovery flow, and PE-context operation all consume this identification in the og. No current user-facing regression (the core's flows don't ask the question yet) — hence MAJOR, not SEVERE — but the trail's current "MATCH by omission" record is factually wrong about the scope it covers, and the capability decision is now unowned.

## Reproduction / verification

- Corpus-derived, first-hand this session: the IsBootVolume trace string at :51303 (grep-verified in the decompile); the hive-profile block read (:62034-62245 region); the EASEUS_HMG tags (2 hits); the by-name load evidence (s25 file :13).
- Core first-hand: reghive scope read (FVE-only, ControlSet001 hardcode); cr_bootfix input contract read; trail grep zero hits.

## Fix plan (for the engineering team)

1. Decision first (owner call): does the merged product need offline-OS identification? The boot-repair surface (cr_bootfix) is the natural consumer — auto-locating the BCD/system partition instead of requiring the caller's path. Record the decision in PARITY-DELTAS either way (this finding closes the "unowned" state).
2. If yes — new `src/core/reghive/` extension (or a sibling `cr_offlineos`): mount offline SOFTWARE+SYSTEM (the existing hive reader), read the 8 CurrentVersion identity fields, resolve the ControlSet via `\Select\Current` with the 001 fallback (mirroring the og's resolution, not our FVE hardcode), map `\MountedDevices\DosDevices\<letter>` to disk-signature+offset, classify ProductOptions. Expose as a small C++ API first; ABI only when a host-facing flow consumes it.
3. Tests: a fixture offline Windows pair (SOFTWARE+SYSTEM with the real key shapes — the corpus QA tree's structure); Select-vs-fallback resolution; MountedDevices value decode; the server-vs-client classification.
4. Re-scope `modules/mom-capabilities.md:102-115` to separate the dead CheckVersion copy from the live DeviceManager copy (this finding's citation set).

## Tests to add

- `OfflineOs.ProfileReadsCurrentVersion` (8 fields, 0x418 record shape).
- `OfflineOs.ControlSetResolvesViaSelect` (Select/Current → ControlSet002 honored; 001 fallback when absent).
- `OfflineOs.SystemPartitionFromMountedDevices` (12-byte value → signature+offset).
- `OfflineOs.ServerVsClient` (ProductOptions string).

## Related findings

- OQ-S5-1 (RESOLVED s25 — the CheckVersion half; this finding extends its lineage to the live module).
- S-014 (FIXED — the FVE offline-hive work this would share the hive reader with).
- OQ-S8-1 (this session — CWindowsFS, the other never-decoded MOM candidate).

## Open questions

The og-side consumer of the identification output inside DeviceManager's own flows (vs. its MOM callers) is not fully traced — same class of question OQ-S5-1 was. The capability decision (fix-plan step 1) does not depend on it; the roadmap wiring does.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's M-082 landing (DFX-38; the module, the tests, and the doc re-scope in one commit — sha in the worklog entry)
**Verified by:** the 5-test `OfflineOs.*` family (all RED-by-absence pre-landing — the capability did not exist): `ProfileReadsCurrentVersion` (the 8 identity fields + the \Device\ SystemRoot rejection gate), `ControlSetResolvesViaSelectWithFallback` (Current=2 → "\ControlSet002"; absent → the 001 fallback), `SystemPartitionFromMountedDevices` (the 12-byte value decode: signature 0x1A2B3C4D + offset 0x100000, the decoy D: letter ignored, SystemRoot lowercased to "c:\windows"), `ServerVsClientClassification` (WinNT/winnt-ci/ServerNT/LanmanNT/unknown → 1/1/3/2/0), `ToleratesMissingKeysAndOddValueShapes` (no CurrentVersion → empty profile not a failure; an 8-byte MountedDevices value → no location claim; null SOFTWARE → refused). Full suite **1045/1045** linux-release. The grammar was re-verified against the corpus BEFORE implementation, including one first-hand binary decode the finding's citation set did not carry: **DAT_18007d37c (the ControlSet rendering format) is `%03d`** — read from the shipped `og-codebase/PartitionRecovery/DeviceManager.mo` .rdata at file offset 0x7bf7c this session (the plain-%d reading would have produced "\ControlSet1" and failed against real-Windows ControlSet001 naming; the test caught the first-draft error and the binary settled it).
**Resolution notes:** The fix plan's step-1 decision is recorded as YES in PARITY-DELTAS **D-107** (the capability lands as a C++ API; the boot-fix auto-locate consumer and the ABI export are the recorded follow-ups — "ABI only when a host-facing flow consumes it", the plan's own staging). What landed: `include/cr/reghive/cr_offlineos.h` + `src/core/reghive/cr_offlineos.cpp` — the og's 0x418-byte CPartitionBootInfo walk over the S-014 regf parser: the 8 `\Microsoft\Windows NT\CurrentVersion` fields (0x80-capped, present-if-found), the SystemRoot lowercase + drive-letter gate ((byte)(c+0x9F)<0x1A), the `\MountedDevices` "\DosDevices\<x>:" VALUE name (leading-backslash value names — the og passes the full string as the value name, :62137-62195) with the exactly-12-byte gate (u32 signature + u64 offset), the `\Select\Current` resolution with the 001 fallback and the %03d rendering, and the ProductOptions classification (WinNT→1, LanmanNT→2, ServerNT→3, lstrcmpiA-ci). The mom-capabilities.md row is re-scoped per fix-plan step 4: the dead-shipped CheckVersion verdict now covers ONLY the CheckVersion copy; the live DeviceManager copy's capability is implemented (the row's MATCH-BY-OMISSION no longer covers the surface by omission). Not landed (deliberate, recorded): the drive-letter sweep (:62854-62920 — the og's per-root FindFirstFileA Windows-dir probe; the core's caller enumerates partitions and reads the hives directly), the IsBootVolume trace plumbing, and the bootfix auto-locate wiring (the follow-up consumer).
