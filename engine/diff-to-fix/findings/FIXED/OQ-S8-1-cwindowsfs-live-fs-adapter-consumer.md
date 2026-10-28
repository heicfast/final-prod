# OQ-S8-1 — CWindowsFS decoded: the og's live-mounted-filesystem adapter — who consumes it, and does the merged product need one?

**Status:** CLOSED — RESOLVED (session-10, S10-F lane, 2026-10-29): the consumer is decoded — **the og's user-visible in-app partition file browser** (the ExplorerPartition chain), plus two internal capability consumers (used-space; the lost-partition root-openability probe). The capability finding the OQ's resolution path prescribed is filed as **M-111** (MAJOR). Full trace: `evidence/decompiled-quotes/s10f-cwindowsfs-consumer-trace.md`.

## The decode (first-hand, this session)

`CWindowsFS` is an `IFSInterface` implementation in the MOM **FileSystem** module (`re-workspace/reversed/ghidra-mo/FileSystem_decompiled.c`):

- ctor `FUN_1800117d0` at `:14025` (`*param_1 = IFSInterface::vftable; *param_1 = CWindowsFS::vftable;`); it wraps a MOM partition/volume object, resolves a root-path string through the interface chain (`+0x38→+0x40→+0x68→+0x98`, `:14064-14076`), and serves directory enumeration via Win32 (`FindFirstFileW/FindNextFileW/FindClose` — `FileSystem_imports.txt:11-13`; `FindClose` at `:14125`; no registry, no CreateFile imports).
- Created by the module's IFSInterface factory `FUN_18000cf10` (`:10650-10797`) in a **mounted-vs-raw dispatch**: for FS types 4 (NTFS), 8 (HFS+) and the "other" family, a live-mounted probe (`FUN_18000d590`) selects CWindowsFS when the volume is OS-accessible (`new(0x30)` → ctor, at `:10698/:10729/:10768/:10789`), and the raw parsers (FAT `FUN_18000f210`, ext2 `FUN_1800051f0`, `FUN_1800012b0`, `FUN_180009d00`, `FUN_180007640`) when not. One unconditional early branch also creates it (`:10695-10701`).
- Module identity: `FUN_18000d330` `:10817` returns `"FileSystem"`; `FUN_18000d340` `:10826` returns `"CFileSystemModule"`.
- The class was previously only a registered decode candidate (`diff-to-fix/SURFACE-INVENTORY.md:66`); the census artifacts never name it in prose (`MOM-modules/FileSystem.md:15` lists only the vftable census line; `mo-module-map.md:26` omits it).

## Semantics

CWindowsFS is the og's **live-Win32 filesystem adapter**: existing files on a mounted, OS-accessible volume are served through the Win32 find/read chain instead of the raw parsers — i.e., a path by which the og's file-serving surface can present a mounted volume's live files (not deleted-file recovery) through the same MOM file-object machinery.

## Why it is a question, not a finding

The **og-side consumer** of this adapter is untraced (the same class of question OQ-S5-1 was before its s25 resolution): the factory dispatch proves it is constructible for mounted NTFS/HFS+/other volumes, but which flows request it (a browse-existing-files UI? the PE-context device manager? an installer-scan path?) is not decoded. The core has no counterpart — its only `FindFirstFile*` uses are the `*.crsf` session listings (`cr_ui.cpp:396`, `rrecov.cpp:927`); volume content is always served through the raw engines; there is no live-mounted browsing mode. Whether that absence is a parity gap depends on the consumer answer.

## Core state (verified first-hand)

- No CWindowsFS-equivalent: grep FindFirstFileW over `src/` → the two session-list sites only; the raw engines are the only volume-content path; `modules/mom-capabilities.md` has no row for it (only the SURFACE-INVENTORY decode-candidate line).

## The precise question to resolve next

**Which og flow(s) construct CWindowsFS, and do they serve user-visible existing-file browsing (the "browse this PC's files" surface) or an internal capability (e.g., a PE-context mounted-volume path)?** Resolution path: trace `FUN_18000cf10`'s callers in the MOM decompiles (DeviceManager/PartitionRecovery are the likely requestors — the `+0x40/+0x68/+0x98` interface chain matches the partition-object shape M-082's decode walked), then map the consumer's product function against the merged product's scope. If user-visible existing-file browsing is in the merged product's scope, file the capability finding; if internal, register the absence as deliberate (no-UI/no-browse posture) in PARITY-DELTAS.

## Evidence for the resolver

- Factory + ctor + dispatch cites (above, FileSystem_decompiled.c lines).
- `MOM-modules/FileSystem.md:15` (the ×3 vftable census line).
- The interface-chain shape cross-reference: M-082's DeviceManager decode (`:51759-51767`) walks the same `+0x40/+0x68/+0x98` chain on partition objects — start there.

## Related findings

- M-082 (this session — the other MOM decode candidate, resolved to a finding).
- OQ-S5-1 (the precedent resolution pattern: s25's CheckVersion dead-shipped evidence).
- SURFACE-INVENTORY.md:66 (the candidate registration this closes or converts).


---

## RESOLUTION (session-10 S10-F, 2026-10-29)

**The answer to the precise question:** the og flows that construct CWindowsFS:

1. **USER-VISIBLE** — the Partition-Recovery feature's in-app per-partition file
   browser: DRWUI's partition-item browse button (FUN_140904fc0, ShellExecuteA for
   mounted-with-path :1421113 vs the in-app FUN_14091c620 :1421116-1421118) → the
   `CreateExplorerPartitionObject` export (:438891) → `ExplorerPartition::Open`
   (PartitionRecovery_decompiled.c:8481-8492 — `FUN_1800017f0("FileSystem")`, the
   {5,6,0x18} raw-flag, the IFileSystemModule vtable+0x38 factory) → CWindowsFS for
   OS-accessible volumes (the raw parsers otherwise, same interface). The browse
   serves EXISTING files of mounted volumes through the Win32 find chain — exactly
   the "browse this partition's files" surface.
2. **Internal** — DeviceManager's partition used-space computation
   (DeviceManager_decompiled.c:49031-49093, the +0x20 GetUsedSpace slot).
3. **Internal** — PartitionRecoveryD's lost-partition root-openability probe
   (PartitionRecoveryD_decompiled.c:6717-6731).

**The parity consequence (per the OQ's own resolution path):** user-visible
existing-file browsing IS an og product surface, and the core has no counterpart
seam (no live-FS adapter, no browse surface; the only FindFirstFile* uses are the
.crsf listings) — filed as **M-111** (MAJOR): the capability finding with the
scope decision (register the deliberate absence vs open the seam). The
SURFACE-INVENTORY §4 decode-candidate row converts accordingly. Every
load-bearing cite in the trace was lead-verified first-hand before filing (the
verification log is the evidence file's final section).
