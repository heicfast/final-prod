# MAJOR M-111 — the og's in-app partition file-browse capability (ExplorerPartition → CWindowsFS live-mounted-FS adapter) has no core counterpart and no scope registration

**Severity:** MAJOR (a decoded og engine-side capability with core-side absence and an unmade scope decision — the M-082 precedent class)
**Module:** mom-capabilities (FileSystem.mo / PartitionRecovery.dll) / query-browse surface
**Parameters affected:** A42-adjacent (public surface shape — a browse-existing-files seam), A54/A56 (module boundaries / interface shapes — the IFSInterface adapter family), P46 (contract documentation — the scope posture is unregistered)
**Corpus references:** the full consumer trace (first-hand, this lane + the T11a read-agent; every load-bearing cite lead-verified): `ghidra-drwui/DRWUI_decompiled.c:438885-438895` (`GetProcAddress("CreateExplorerPartitionObject")` — one of PartitionRecovery.dll's five loaded object factories), `:1421095-1421125` (FUN_140904fc0, the partition item's browse button: mounted-with-path → `ShellExecuteA("open",…)` :1421113; **else → the in-app browse FUN_14091c620 :1421116-1421118**), `:1433564-1433690` (the ExplorerPartition wrapper: create + `vtable+8` open, then `vtable+0x40(explorer, path, listA, listB)` directory enumeration with 0x468-byte entries, `QString::fromUtf8` name reads, QTableView async fill `:1414842-1414929`); `ghidra-partition/PartitionRecovery_decompiled.c:8440-8503` (ExplorerPartition::Open — `FUN_1800017f0("FileSystem")` :8481, the FS-type check {5,6,0x18}→raw-flag :8485-8490, **the IFileSystemModule vtable+0x38 factory call** :8491, the IFSInterface stored at this+0x10 :8492; the enumeration impl `FUN_18000d970` :8699-8790 — slot0 FindFirstFile :8746, dir-test attr&0x10 :8772, FindNext loop :8779, FindClose :8783); `ghidra-mo/FileSystem_decompiled.c:10650-10800` (the factory's mounted-vs-raw dispatch: CWindowsFS when OS-accessible — the drive-letter early branch :10695-10701, the probe FUN_18000d590 :10896), `:14025-14268` (CWindowsFS: the ctor's drive-letter capture :14061, the Win32 FindFirstFileW/FindNextFileW/FindClose service :14132-14268, the `"X:\<dir>\*"` enumeration :14198-14218; the IFSInterface vtable: slot0 FindFirstFile, +0x08 FindNext, +0x10 FindClose, +0x20 **GetUsedSpace**, +0x28 Release); the internal consumers: `ghidra-mo/DeviceManager_decompiled.c:48960-49109` (CUpdatePartitionUsedSpaceName — the FileSystem module's +0x20 GetUsedSpace path :49031-49093) + `ghidra-mo/PartitionRecoveryD_decompiled.c:6717-6731` (the lost-partition root-openability probe through the same factory). Full trace: `evidence/decompiled-quotes/s10f-cwindowsfs-consumer-trace.md`.
**Core references:** audited `deea0241`. No counterpart: the core's only `FindFirstFile*` uses are the `.crsf` session listings (cr_ui.cpp / rrecov.cpp — the OQ's verified census, still true); volume content is always served through the raw engines; no live-mounted browsing mode exists anywhere in `src/` (the CWindowsFS greps: zero). No PARITY-DELTAS row registers the absence (the no-UI posture covers the Qt table, not the engine-side adapter seam).
**Status:** FIXED (the scope decision registered)
**Prior audit check:** OQ-S8-1 (tracked-open since session-8) asked exactly this consumer question; resolved this session by the trace above (the OQ file closes with the resolution and converts to this finding per the OQ-SA-2 precedent).

## Summary

The og's Partition-Recovery feature offers a per-partition **in-app file browser** for existing (not deleted) files: mounted partitions open Windows Explorer, everything else opens an in-app Qt table view whose backend is the `ExplorerPartition` object chain — `PartitionRecovery.dll`'s exported factory → the FileSystem.mo module's `IFileSystemModule::CreateFileSystem` vtable slot → **CWindowsFS**, the live-mounted-filesystem adapter that serves directory enumeration through the Win32 find chain, or the raw parsers for unmounted/lost partitions behind the same interface. The same adapter also feeds partition used-space computation and the lost-partition root-openability probe. The core has no counterpart surface — no live-FS adapter, no browse seam — and the absence is unregistered (the no-UI posture does not cover the engine-side seam an ABI host would need to offer the equivalent feature).

## What the reference does

The consumer chain (all cites in the Corpus references block; the trace file quotes each): DRWUI loads PartitionRecovery.dll's object factories including `CreateExplorerPartitionObject` (:438891). The partition item's browse button (:1421095-1421125) either ShellExecutes the mounted path or opens the in-app browser (FUN_14091c620 → the ExplorerPartition wrapper → `vtable+0x40(explorer, path, listA, listB)` enumeration into 0x468-byte entries rendered in a QTableView). Engine side, `ExplorerPartition::Open` requests the FileSystem module by name (:8481), computes the raw flag from the FS type ({5,6,0x18} raw; else 0), and calls the module's vtable+0x38 factory (:8491); the factory's mounted-vs-raw dispatch creates CWindowsFS for OS-accessible volumes (drive-letter early branch + the probe) or the raw parsers otherwise. CWindowsFS serves the IFSInterface vtable: FindFirstFile/FindNext/FindClose/GetUsedSpace/Release, enumerating `"X:\<dir>\*"` via Win32. Two internal consumers ride the same factory: DeviceManager's used-space computation and PartitionRecoveryD's root-openability probe.

## What our core does

No live-mounted-FS adapter; no IFSInterface-equivalent seam; no browse-existing-files surface at any layer (CLI, ABI, or engine). The only Win32 find uses are the session listings. Volume content is served exclusively through the raw engines. No register row records the scope decision.

## Gap

Falsifiable claims: (a) the og exposes a per-partition browse of a mounted volume's existing files through engine-side infrastructure (the cites above); (b) no core surface can enumerate a mounted volume's existing files through the recovery engine's object model (grep-verified); (c) no PARITY-DELTAS row registers the absence as deliberate.

## Impact

- If the merged product (an ABI host with UI) wants feature parity for "browse this partition's files", the core offers no seam — the capability decision is forced late and ad hoc.
- The used-space and root-openability halves are internal capabilities with the same seam requirement.
- The no-UI registered posture covers the Qt table only; the engine-side adapter family (IFileSystemModule::CreateFileSystem / IFSInterface) is an architectural surface the core does not model.

## Reproduction / verification

Decompiled evidence (strength 2): the consumer trace with every load-bearing cite lead-verified (the GetProcAddress site, the raw-flag factory chain, the browse trigger, the enumeration impl) — `evidence/decompiled-quotes/s10f-cwindowsfs-consumer-trace.md` (the T11a read-agent's full report preserved there, with the lead's verification log). Core-side: the FindFirstFile/IFSInterface/CWindowsFS grep census (zero).

## Fix plan (for the engineering team)

1. **The scope decision first** (one docs commit): either register the deliberate absence (PARITY-DELTAS row: "no in-app browse of mounted volumes — the merged product's hosts use the OS shell; the engine serves recovery content only") or open the capability. This finding closes either way once the decision is registered.
2. If YES: model the seam — a `browse` query surface over the existing storage layer (a LiveFsAdapter interface with the og's operation set: first/next/close + used-space), CWindowsFS-equivalent implementation via the Win32 find chain in src/win/, raw-engine fallback behind the same interface for unmounted volumes (the core's engines already provide this half), and an ABI entry point.
3. Tests: `Browse.MountedVolumeEnumeration` (fixture mounted dir → entries), `Browse.RawFallback` (image partition → the raw engines), `Browse.UsedSpace`.
4. Acceptance: the scope decision row exists; if implemented, a host can enumerate a mounted volume's existing files through the ABI.

## Tests to add

- (Only if the capability is opened) `Browse.MountedVolumeEnumeration`, `Browse.RawFallback`, `Browse.UsedSpace`.

## Related findings

- OQ-S8-1 (RESOLVED-CLOSED this session — the consumer trace this finding is built on).
- M-082 (the same class: a decoded MOM live-module capability with core absence and a scope decision).
- M-100 (closed-as-covered — the archive browse-inside surface, a different layer).
- I-016 (the engine factory-contract register — the IFileSystemModule slot family now has a consumer trace).

## Open questions

None. (The scope decision is the engineering team's, not an audit unknown.)

---

## CORROBORATION ADDENDUM (session-11 S11 lane, 2026-10-29)

This finding's consumer trace was independently re-derived by the S11 lane's
OQTRACE agent (concurrent execution, reconciled at push per the
no-duplication convention): the same chain (DRWUI `:438891` factory resolve →
PartitionRecovery `:8481-8491` `"FileSystem"` request + module-vtable+0x38 →
CWindowsFS for mounted volumes) with three additional surface details — the
browse table lives under the `"PartitionLostSep"` QFrame (`DRWUI:1412268`),
the 4-column QTableView widths `0x183/0x96/0x50/0x6e` (`:1412346-1412352`),
and the root listing seeds from `QString "\\"` (`:1412353-1412358`). No
discrepancy with the filed trace; the corroboration record is at
`evidence/decompiled-quotes/s11-oq-traces.md` §1 (two independent traces,
same cites — the evidence class strengthens from single-lane to
double-verified).

## Resolution

**Status:** FIXED (the option-1 registration posture — the finding's own "closes either way once the decision is registered")
**Fixed in commit:** (this push; the S82 fix lane, 2026-10-29)
**Verified by:** the D-158 PARITY-DELTAS row exists verbatim with the consumer-trace cites (the browse button's ShellExecute primary, the ExplorerPartition/factory/CWindowsFS chain, the two internal consumers); the no-UI posture note in PARITY-OG-CORE-INVENTORY cross-referenced; the core-side absence re-verified at the current tree (the FindFirstFile/IFSInterface/CWindowsFS grep census — the session-list listings only).
**Resolution notes:** The scope decision is REGISTERED as a deliberate divergence (D-158): for mounted volumes the og's own primary path is the OS shell (ShellExecuteA — a host-one-liner), and for unmounted/lost partitions the og's in-app table is backed by the raw parsers behind the same interface — the role the core's engines already fill through the lost-partition result tree. The engine-side adapter seam (IFileSystemModule::CreateFileSystem / IFSInterface / CWindowsFS) is registered as an og-internal boundary whose only un-duplicated half (GetUsedSpace) is a host-UX concern. No code change: the capability is declined with the rationale on file, the R-20-adjacent surface-inventory rows already carry the derivation source (PARAMETERS Group K), and future integrators find the decision instead of discovering the hole. If the merged product later wants the seam, the trace in this finding's references is the implementation map.
