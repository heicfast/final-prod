# OQ-Z5 resolution evidence — CZfsSearchController vs CZfsQuickSearch: the IComplete/IQuick plugin-interface split

**Session:** DFX30 (tracked-open tail lane), 2026-10-26
**Method:** first-hand MSVC RTTI walk (TypeDescriptor → RTTICompleteObjectLocator → vftable) on `og-codebase/ZfsSearchDLL.dll`, slot-by-slot function reads in `re-workspace/reversed/ghidra-searchdll/ZfsSearchDLL_decompiled.c`.

## The vtables (measured)

| Class | vftable RVA | slots | Interface (named by the dtor's vftable restore) |
|---|---|---|---|
| `zfs_struct::CZfsSearchController` | 0x180030ea8 | **34** | `ICompleteFileSystemSearchController` (installed by FUN_180010cb0 :14806) |
| `zfs_struct::CZfsQuickSearch` | 0x180030c38 | **16** | `IQuickFileSystemSearchController` (installed by FUN_180001720 :404) |

Both factories are the DLL's only exports: `EUCreateZFSSearcher` (0x180011660,
allocates 0xE8 bytes) and `EUCreateZFSQuickSearcher` (0x180011750, allocates
0x120 bytes — the quick object is BIGGER: it carries its own result/state
buffer at +0x108/+0x110, initialized via FUN_18000afe0, freed in its dtor).

## What the slots actually do (read first-hand)

Shared base surface (slots 2-9 in both): the IFileProperties/device binding
setters (+8/+0x10/+0x18/+0x20/+0x28) and stubs (FUN_1800013a0/1450 return-0
family, FUN_1800014f0..1550 setters).

**Quick-only distinct slots:**
- [0] FUN_180006b30 — set `+0x98`.
- [1] FUN_180006b40 — **`vector<u64>::push_back(this->field_a0)` into the
  caller's vector**: the quick searcher's result is a SINGLE accumulated
  value (+0xa0), collected in bulk by the host after the one-shot run.
- [10] FUN_1800069f0 — dtor (restores the CZfsQuickSearch vftable, frees
  +0x108/+0x110, fires the +0xe0 std::function if set).
- [11] FUN_180006b80 — delegate to a member's vtable+0x78.
- [12] FUN_180001710 — set `+0x70`.
- [15] FUN_180006b90 — **the quick run loop** (~600 lines, ostream logging;
  drives the shared ZFS machinery chain FUN_1800023a0 / FUN_18000b460 /
  FUN_180007b60 / FUN_180009aa0 / FUN_18000dc50 / FUN_1800182c0 / …).

**Controller(complete)-only distinct slots:**
- [0] FUN_180010f80 — set `+0xd8`; [11] FUN_180011250 — set `+0xc0`;
  [12] FUN_180010f50 — set `+0xb8`; [27] FUN_180011510 — set `+0xd0`;
  [28]+[29] FUN_180010f70/FUN_180011500 — get/set `+0xc8`;
  [17] FUN_180010d20 — set byte `+0x70`; [26] FUN_180010f60 — returns 9.
- [10] FUN_180011200 — **free-and-rescan**: frees the +0xe0 buffer, re-enters
  the scan via FUN_180010cb0 with a 0x10 parameter (a restart entry the quick
  interface does not have).
- [19] FUN_180011260 — **per-hit streaming emission**: validates the hit via
  FUN_180010f90 (reads `count x sector_size`, scans 0x400 strides for the
  uberblock magic `0x00bab10c` 4-byte BE, then checks the **ZEC_MAGIC
  `0x0210da7ab10c7a11`** 8-byte BE at hit+0x1d8 — the zio_eck trailer,
  i.e. a 512-byte-slot geometry assumption), then computes
  `sector = offset / sector_size` via the device vtable and fires the
  std::function at +0xb0 with a property record (kind 0xd).
- [32] FUN_180010d30 — installs/replaces the +0xb0 std::function consumer
  (the callback-registration slot).
- [22]/[23] FUN_180001140 — return-0 stubs (reserved interface slots).

## The answer

The ZFS quick/complete split is the **product-wide engine-plugin interface
split** (`IQuickFileSystemSearchController` vs
`ICompleteFileSystemSearchController`; the engine-matrix artifact shows all 9
engines expose the factory pair), NOT a ZFS-specific behavioral fork. What
"complete" adds over "quick" in ZFS:

1. **Streaming per-hit emission** with property records through an installed
   std::function callback (live results/progress), vs quick's **one-shot
   bulk collect of a single result value** into the caller's vector.
2. A **restart entry** (free-buffer + re-scan).
3. **Five extra config channels** (+0xb8/+0xc0/+0xc8/+0xd0/+0xd8, one with
   get+set) vs quick's two.
4. The complete path's per-hit validation embeds the **uberblock magic +
   ZEC_MAGIC label probe** (0x400-stride scan; zec checked at +0x1d8 = a
   512-byte-slot assumption — the core's ring walk probes slot geometry
   shifts 10..13 kernel-correctly and is the registered superset, OQ-T6.3 /
   PARITY-DELTAS).

Both classes carry real ZFS scan machinery (quick's run loop is the ~600-line
FUN_180006b90; the Controller's scan path re-validates labels at emit sites).

## Core-parity consequence

The core models quick/deep at the orchestrator layer (run_quick/run_deep
stages, typed event stream, cancellation, config) over a single ZFS engine —
the union of both interfaces. No code change. The IQuick/IComplete interface
names + slot semantics are recorded here for the module doc; cross-ref
I-016 (engine factory contract register, FIXED/) which already registers the
factory-pair architecture.

## Reproduction

```sh
python3 oq_z5_vtables.py        # RTTI walk -> vtable extraction (34 vs 16 slots)
# slot reads: ZfsSearchDLL_decompiled.c lines cited per function above
grep -n "ICompleteFileSystemSearchController\|IQuickFileSystemSearchController" ZfsSearchDLL_decompiled.c
```
