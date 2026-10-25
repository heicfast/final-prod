# MINOR N-055 — Device enumeration breadth: og spans WMI+SetupAPI+VDS+StorageSpaces+FunctionDiscovery; core is a PhysicalDrive CreateFileW loop

**Severity:** MINOR
**Module:** storage (win enumeration layer)
**Parameters affected:** B8 (multi-volume/cross-partition boundaries — device discovery), P42 (public surface), A54 (module boundaries — enumeration stack)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:57-59` (enumeration spans WMI, setupapi, VDS, StorageSpaces, FunctionDiscovery), `re-workspace/artifacts/config-files/vendor-logs--qa-session-forensics.md:13-29` (device discovery census in QA logs), `re-workspace/artifacts/UIUX/24-dynamic-ground-truth-ingestion.md:110-112` (the device-list ingestion path)
**Core references:** `src/win/cr_enumerate.cpp:15-60` (enumeration = `\\.\PhysicalDrive%u` CreateFileW loop 0..N + IOCTL geometry/descriptor; grep SetupDi/WMI/VDS/StorageSpaces: 0 hits)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og discovers disks/volumes through five enumeration stacks (WMI, SetupAPI, VDS, Storage Spaces, Function Discovery); the core enumerates `\\.\PhysicalDrive0..N` directly. Devices exposed only through the richer stacks (notably Storage Spaces virtual disks behind odd filter stacks, and interface-level events) may be missed or under-described by the plain loop.

## What the reference does

The MOM module-system architecture (first-cited this session) documents the og's device-discovery fan-in across WMI (MSFT_* namespaces), SetupAPI device interfaces, VDS (dynamic disks), Storage Spaces, and Function Discovery, merged into the device list the UI presents. The QA-session forensics artifact shows the census arriving through those paths in the field.

## What our core does

`cr_enumerate.cpp` walks PhysicalDrive indexes 0..N with CreateFileW; anything that does not surface as a readable `\\.\PhysicalDriveN` (or via the volume-letter companion path) is invisible to it.

## Gap

Falsifiable claim: a device visible to the og's enumeration merge but absent from the PhysicalDrive namespace (or readable only through the VDS/Storage Spaces stacks) appears in the og's device list and not in the core's.

## Impact

Discovery-coverage edge cases on exotic stacks (Storage Spaces pools with hidden physical members, VDS-pack disks without drive letters). The core's image-first workflows and the bench harness bypass enumeration, so day-to-day parity is unaffected; the gap concentrates on the Windows disk-selection surface.

## Reproduction / verification

Corpus-derived (three artifacts first-cited this session; the five-stack list quoted). Core side read in-source at the audited commit. UNVERIFIED-BY-EXECUTION for the actual miss cases (would need a Storage Spaces host — tracked in the open question).

## Fix plan (for the engineering team)

1. Decide the posture explicitly: (a) keep PhysicalDrive-first as the documented engine scope and register the delta (recommended first step — honest and cheap), or (b) extend `cr_enumerate.cpp` with a SetupAPI disk-interface pass (CM_Get_Dev_Interface_List / SetupDiEnumDeviceInterfaces GUID_DEVINTERFACE_DISK) to catch devices the numbered loop misses, plus a Storage Spaces note (Storage Spaces members DO surface as PhysicalDrive in practice — the artifact's claim needs a live check before code).
2. If (b): add `enumerate_via_setupapi()` merging (dedup by device instance path) into the same `PhysicalDisk` vector; keep the Win7 import allowlist constraint in mind (setupapi is already allowed for the class-coinstallers; verify tools/ref_imports_um.json).
3. Tests: `Enumerate.SetupApiMerge` (mock interface set → merged list), `Enumerate.DedupByInstancePath`.
4. Acceptance: the delta is either registered in PARITY-DELTAS with the scope rationale, or the merged enumeration ships with tests.

## Tests to add

- `Enumerate.SetupApiMerge` (only if option b)
- `Enumerate.DedupByInstancePath`

## Related findings

M-034 (no NAS remote device layer — the same discovery-breadth family); N-032 (health transports CSMI/USB bridges — same device-stack breadth); M-030 (driver utilization — the same win-layer posture).

## Open questions

Whether Storage Spaces pool members genuinely fail to surface as `\\.\PhysicalDriveN` on current Windows builds is UNVERIFIED-BY-EXECUTION (no Storage Spaces host in the bench fleet). The corpus artifacts assert the five-stack design; the miss-case needs a live probe before code follows.
