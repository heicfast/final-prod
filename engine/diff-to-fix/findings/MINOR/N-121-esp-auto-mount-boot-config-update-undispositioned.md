# MINOR N-121 — the og's boot-repair chain includes ESP auto-mount for the boot-config update (CUpdateGptSystemBootPartition — MountGptPartition/UnmountGptPartition) — core's bcdfix requires a host-supplied BCD path and the ESP-finding step is undispositioned

**Severity:** MINOR
**Module / Surface:** boot repair (modules/boot-repair.md)
**Parameters affected:** A42-adjacent (the boot-fix flow's reachability), 62-adjacent (a recovered system whose ESP has no letter)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/MOM-modules/mo-module-map.md:56-57` (CUpdateGptSystemBootPartition named in the boot-repair chain — "after recovering a system partition, DRW can repair the boot chain"); the class + its InitLayout/MountGptPartition/UnmountGptPartition trace strings (DeviceManager.mo rabin2 strings :321-332; rtti_classes.json:5342; DeviceManager_decompiled.c:49956).
**Core references:** audited commit `ff879f9` — `src/win/cr_bootfix.cpp` + the apply path — the landed M-040 stages require a host-supplied BCD path (rrecov apply --boot-fix-bcd file, rrecov.cpp:22); grep for ESP-mount/GPT-system-partition discovery: **zero**. No disposition row exists anywhere (not in D-90, not in boot-repair.md's rows, not in PARITY-DELTAS).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** M-040 (FIXED) landed the BCD/NVRAM stages; the corpus evidence for the ESP-mount half sits in the module map + trace strings and was never adjudicated.

## Summary

("The og's boot-repair flow finds and mounts the EFI System Partition itself (CUpdateGptSystemBootPartition's MountGptPartition) so the boot-config update reaches \\EFI\\MICROSOFT\\BOOT\\BCD on any recovered system. Core's boot-fix is unreachable unless the host mounts the ESP and passes the path. A recovered system whose ESP has no drive letter — the common case after lost-partition recovery — cannot complete the boot repair through core as it can through the og. The gap is undispositioned (neither implemented nor registered).",)

## What the reference does

As cited — the trace-string evidence (InitLayout/Mount/UnmountGptPartition) and the module-map placement in the boot chain. The behavioral decode is census-level (no artifact decodes the mount semantics — see Open questions).

## What our core does

bcdfix consumes a host-provided path; nothing discovers or mounts the ESP; no registration covers the omission.

## Gap

Falsifiable: an ESP without a drive letter — the og's flow reaches the BCD (mounts it); core's apply --boot-fix cannot (no path to give).

## Impact

The boot-repair flow's reachability in exactly the scenario it exists for (recovered system partitions); MINOR because the host CAN mount the ESP manually — but nothing documents that requirement either.

## Reproduction / verification

Corpus documentation evidence (the module map + trace strings, cited); core absence grep-verified this session.

## Fix plan (for the engineering team)

1. Either implement the ESP discovery+mount step in the win layer (find the ESP among the recovered/applied partitions by type GUID C12A7328-F81F-11D2-BA4B-00A0C93EC93B; mount via the existing elevation seam; unmount after) or register the host-must-mount requirement as a documented contract + a PARITY-DELTAS row. 2. If implementing: the mount/unmount pair must be exception-safe (unmount even on failure — the og's UnmountGptPartition implies the pairing). 3. boot-repair.md gains the row.

## Tests to add

- `BootFix.EspAutoMountReachesBcd` — a GPT fixture with an unlettered ESP: the fix path finds and mounts it (RED pre-fix).
- `BootFix.EspUnmountedAfterFix` — the pairing holds on success AND failure.

## Related findings

M-040 (the landed stages this extends); D-90 (the boot-family registration this row joins).

## Open questions

CUpdateGptSystemBootPartition's behavioral decode is trace-string-level (the corpus carries the class name + method strings, not the mount semantics) — the corpus lane should decode DeviceManager_decompiled.c:49956 before the implementation lands; the capability's existence is the pinned fact here.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

corpus cite path drift: mo-module-map.md now at re-workspace/artifacts/PartitionRecovery/mo-module-map.md:56-57 (was MOM-modules/); core state unchanged (rrecov.cpp:23 host-supplied path; no discovery/mount code); D-90 still lacks the ESP row
