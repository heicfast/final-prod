# INFORMATIONAL I-016 — Engine factory-contract and host-binding register (9/9 exports verified): the og's per-DLL searcher factories, vtable interface, engine-enable gates, and dead FS codes — the core's flat C ABI is the deliberate equivalent

**Severity:** INFORMATIONAL (deliberate architectural difference + verified reference register; two items inside are actionable notes for parity tables and one gate decision)
**Module:** abi (cr_ui) / orchestration
**Parameters affected:** P42 (public surface shape — different shape, equivalent semantics), P45-adjacent (ABI policy), P54/P56/P57 (module boundaries/wiring), P1-adjacent (detection scope via engine gates), P21-adjacent (FS-code spaces)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/*_decompiled.c` (9 factory tables — see `evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` §5), `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — host invoker FUN_14056fe40 (:906144-906162), lazy LoadLibrary of the 10 engine DLLs from the exe dir (:903488, :227506-227538), FS-code dispatch incl. Btrfs=0xB (:913062-913065) and the FS-string map (:955212-955227: "btrfs"→0xB, "ext4"→9, "zfs"→0xD, else 6), engine-enable gates scanext4/scanzfs (:913038-913046, :913104-913112, :917169), Config.dat bAddSpecifyDirFilter (:917053-917060), settings+0x60 RAW gate (:917266-917270), deep-scan complete set NTFS/FAT/exFAT/ZFS/HFS/ReFS/RAW (:917068-917262), IAuxiliaryBitmap global 10 MB+512 B singleton (:897296-897335, consumed at vt+0x30 :906092/:912925), RAW per-file naming hook (vt+0x48, :917292-917296), cancel-poll vt+10 per record batch, 64-bit {facility<<32|code} Search status (HFS+ engine :460-505); `re-workspace/artifacts/SearchDLL/engine-matrix--factory-contract-verification.md` (seed — 3 corrections filed to CORPUS-ERRATA)
**Core references:** `include/cr/cr_ui.h` + `src/ui/cr_ui.cpp` (flat C ABI, 61 exports, version 1u), `src/core/orchestrator/cr_scan.cpp` (engine dispatch by FsFamily — single in-process switch, no DLL loading, no per-FS config gates), audited commit `63a9167`
**Status:** READ-ONLY REGISTER — deliberate difference; one embedded decision item (engine-enable gates) noted for the engineering team

## Summary

The og binds its engines as per-DLL COM-ish factories: each SearchDLL exports `EUCreate<FS>QuickSearcher` + `EUCreate<FS>Searcher` (one out-slot argument; two verified exceptions: ReFS quick takes 3 args, Btrfs complete is an 8-byte NULL stub), DRWUI lazy-LoadLibrary's them and consumes a vtable interface (attach-partition, sub-partition list, scan mode, IAuxiliaryBitmap, path string, RAW naming hook, Search → 64-bit status, complete-run, pre/teardown). The core's flat C ABI with an in-process FsFamily switch is the deliberate clean-room equivalent — statically linked, cross-platform, one surface. This register preserves the verified factory contract (so parity tables never guess) and records the two real notes inside it: (a) the og's per-FS engine-enable gates (scanext4/scanzfs config, RAW settings gate) have no core counterpart — fs_hint forces a family but cannot disable ext4/zfs individually in an all-family scan; (b) the og's FS-code space (3=NTFS 9=EXT4 0xB=Btrfs 0xD=ZFS) must never be silently equated with the core's (1=NTFS 4=ext4 7=Btrfs 8=ZFS).

## What the reference does

Full table + vtable decode in
`evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` §5. Key verified
facts: 9/9 export pairs with exact addresses; failure = null out-slot (no HRESULT —
the corpus artifact's HRESULT claim is a doc error, CORPUS-ERRata row filed);
`EUCreateReFSQuickSearcher(p1,p2,p3)` 3-arg arity deviation; ReFS DLL refuses to load
pre-Win8 (DllMain gate); `EUCreateBtrfsComplateSearcher` = NULL stub (I-003); RAW's
6th export literally named `GetRawUserInfoMap` (the only registry export DRWUI
consumes); dead codes 4/5 → `EUCreateEXT2QuickSearcher` with an unshipped
Ext2SearchDLL.dll (a dead legacy path in the og itself — parity tables must not
"fix" it); the og's own engine-matrix artifact's EXT4 "grep miss" note refuted.

## What our core does

One flat C ABI (61 exports, opaque handles, version 1u) over an in-process
orchestrator that dispatches engines by `FsFamily` in a single switch
(`cr_scan.cpp:778-875`). No DLL loading, no per-engine factories, no per-FS config
gates (scan config: run_quick/run_deep/run_carve/whole_disk/fs_hint/threads). The
IAuxiliaryBitmap concept (10 MB global scratch the host supplies to engines) has no
core counterpart — the core's engines allocate their own working state.

## Gap

As a defect: none (deliberate). Two recorded notes:
1. **Engine-enable gates**: the og can disable ext4/zfs scans via config and RAW via
   settings; the core cannot disable individual engines (fs_hint selects one family;
   carve toggles do not exist — M-029 §Gap (e)). A host wanting og-equivalent
   scan-scope control needs per-family enable flags in ScanConfig.
2. **FS-code spaces**: og {3=NTFS, 9=EXT4, 0xB=Btrfs, 0xD=ZFS, 6=default} vs core
   {1=NTFS, 4=ext4, 7=Btrfs, 8=ZFS} — any cross-reference table (parity docs, bench
   scripts) must translate, never equate.

## Impact

- Parity-table correctness (note 2) — silent equation would misroute future probes.
- Scan-scope control (note 1) — hosts lose a reference capability; low user impact
  (defaults-on is a superset posture) but the config surface is absent.

## Reproduction / verification

Decompiled evidence (method 2): the full export table + host call sites quoted with
file:line this session (two-agent convergence; three corrections to the corpus's own
engine-matrix artifact filed to CORPUS-ERRATA).

## Fix plan (for the engineering team)

None required for the register itself. Optional for note 1: add
`fs_exclude_mask` (or per-family enable flags) to ScanConfig + cr_ui scan config —
one flag field, one dispatch guard, two tests (`Scan.ExcludeExt4`, `Scan.ExcludeZfs`).
Note 2 needs no code — it needs a warning line in any parity/cross-ref doc that maps
FS codes (add to CAPABILITY-MATRIX when next touched).

## Tests to add

If note 1 lands: `Scan.ExcludeExt4`, `Scan.ExcludeZfs`.

## Related findings

- **I-013 (parallel S3/S4 leg — ReFS host-OS runtime gate superset)** — the ReFS
  pre-Win8 load-gate item in this register's "What the reference does" is that
  finding's subject; recorded here as a register entry, decided there.
- **I-003** — Btrfs complete-stub superset (the deep-set difference this register's
  host-binding decode corroborates).
- **M-029** — carve toggles (the RAW-gate config surface family).
- **M-030** — the driver-side wire contract register (companion surface).
- **S-023** — the grade=1 engine default that this register's engine-side decode
  established.

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** the flat C ABI (61 exports, version 1u) confirmed in cr_ui.h/cr_ui.cpp; the og's per-DLL factory/vtable contract and the FS-code spaces (btrfs 0xB, ext4 9, zfs 0xD) registered in the finding + engine-matrix artifact.
**Resolution notes:** Deliberate architectural difference, preserved: in-process registry vs 10 lazily-loaded engine DLLs. The embedded decision item (engine-enable gates scanext4/scanzfs vs the core's always-on engines) is noted for the engineering team as a policy question, not a defect; the dead FS codes are registered. No core change.

**Status:** ACKNOWLEDGED (deliberate architectural difference + two standing notes)
**Verified by:** this session: core remains a flat C ABI with an
in-process FsFamily switch (no per-DLL factories, no per-FS config
gates); the og FS-code space {3=NTFS, 9=EXT4, 0xB=Btrfs, 0xD=ZFS, 6=def}
vs core {1=NTFS, 4=ext4, 7=Btrfs, 8=ZFS} difference is live in the
parity tables (translation required, never equation).
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59i**. Note 1 (per-family enable flags absent from ScanConfig)
remains a host-capability note, not a parity debt — if a host needs
og-equivalent scan-scope control, add per-family flags then; note 2
(FS-code translation) is the load-bearing rule for every future parity
table.
