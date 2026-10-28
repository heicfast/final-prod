# OQ-S8-1 — CWindowsFS decoded: the og's live-mounted-filesystem adapter — who consumes it, and does the merged product need one?

**Status:** OPEN QUESTION (tracked) — this is not a defect finding; it is a precisely-scoped question with the evidence needed to answer it.

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
