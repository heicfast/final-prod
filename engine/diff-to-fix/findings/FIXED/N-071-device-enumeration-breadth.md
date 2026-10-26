# MINOR N-071 — Device enumeration breadth: og spans WMI+SetupAPI+VDS+StorageSpaces+FunctionDiscovery; core is a PhysicalDrive CreateFileW loop

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


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-071 commit — DFX31)
**Verified by:** 4 pure merge tests (tests/unit/test_enumerate_merge.cpp: SetupApiMerge, DedupByInstancePath, MergeWithEmptyInterfaceStackKeepsLoop, MergeWithEmptyLoopTakesAllInterfaces) — fail-before EXECUTED on the merge-disabled tree (3 RED: appended-miss/absent; restored -> 4/4 GREEN) — plus the win_syntax_check parse gate over the extended cr_enumerate.cpp + the cfgmgr32 stub pair (Linux lane, green). Suite 769/769 linux-release at the landing tree.
**Resolution notes:** Both fix-plan halves executed: option (b) shipped AND the residual registered. (1) The second enumeration stack: enumerate_disk_interfaces() — the cfgmgr32 GUID_DEVINTERFACE_DISK present-instance walk (the documented GUID value defined locally, no INITGUID games; CM_Get_Device_Interface_List_SizeW/CM_Get_Device_Interface_ListW, Win2000-era, added to the strict import allowlist — the Win7-inclusive gate holds) — every interface opened with the numbered loop's share mode, number from IOCTL_STORAGE_GET_DEVICE_NUMBER (kUnknownDiskNumber on query failure), geometry/alignment/descriptor through query_disk_handle (the loop's query factored verbatim — one shared query path for both stacks), table parse through the pure core when a number is known. (2) The merge: merge_disk_enumeration — PURE, header-inline (Linux-lane testable), dedup by disk number first (the loop's entry wins: it is richer), then by instance path (unknown-number entries dedup by path only); degrade path: any cfgmgr failure leaves stack 1's list untouched. (3) The residual (WMI metadata / VDS service load / Storage Spaces init / FunctionDiscovery / hot-plug eventing) REGISTERED as PARITY-DELTAS D-79 with the scope rationale: both ported stacks cover the readable-device space the engine consumes; the og's extra stacks feed UI metadata and eventing; the finding's own open question records the Storage-Spaces miss-case as UNVERIFIED (members surface as PhysicalDrive in practice). (4) Doc-truth: modules/storage.md's stale local ID for this finding (session-6's N-055 row — renumbered to N-071 at integration, the row text unchanged) corrected. The numbered loop's behavior is unchanged for every disk it already found (order preserved: loop disks first, interface-only disks appended).
