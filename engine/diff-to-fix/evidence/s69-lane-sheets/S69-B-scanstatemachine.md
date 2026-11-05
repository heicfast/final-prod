# S69-B context sheet — scan state machine & the N-182 resolver decode

Lead-executed (Agent B's scope; the subagent infra timed out twice — the lane
lead performed the decode). Corpus pin `be0cfe9`, core pin `3799dee2`.
DRWUI = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`; PRD =
`re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c`.

## A. Prior-coverage verification

- N-182's corpus half (the resolver FUN_14035a6d0) was decoded via the s42
  worksheet at summary level; this lane re-read the FULL body first-hand
  (DRWUI:575024-575170) and the quick/deep call pair (DRWUI:567902-567925) —
  the summary is accurate with two refinements (below).
- N-181 (carve-after-deep), watchdog family (S54 Axis 4), progress math
  (M-125/M-126), S-057 auto-save-on-stop: re-verified via the liveness
  battery (see liveness-battery.md); no contradiction.

## B. Fresh decode — the scan-range resolver FUN_14035a6d0 (first-hand, full body)

Signature `FUN_14035a6d0(controller, mode, device, volume_entry, out_flag)`:

1. **Gates**: `(mode == 0 || mode == 1)` AND `*(char*)(volume_entry + 0x6f2) == 0`
   AND `FUN_14006db20(controller + 0xa8) != 0` AND
   `device->vtable[0xa8]() == 6` (device type 6). The derived-range path
   additionally requires `mode != 0` — **quick (mode 0) enters the gates but
   always returns the original device**; only deep (mode 1) derives.
2. **Small-volume rule** (DRWUI:575053-575057): `start = device->vtable[0x68]()`
   (GetStartSector — confirmed by the caller's printf mapping,
   DRWUI:567911-567918: vtable[0x68] → StartSector, vtable[0x18] → TotalSector);
   `if (start < 0x12C00000 / sector_size)` — **start sector below the
   300 MB-in-sectors line — then `*(controller + 0x118) = 1` (the small-volume
   flag) AND `start = 0`**: the deep scan is EXTENDED BACKWARD to LBA 0,
   covering the pre-partition gap. (Refinement 1: N-182's "<300 MB small-volume
   rule" is precisely this start-reset-to-zero — an extension, not just a flag.)
3. **Partition-entry match** (DRWUI:575061-575101): walks the partition-table
   object at `volume_entry + 0x99` via map iterators, finding the entry whose
   `+4`, `+0x89`, `+0x91` fields match the volume entry's (id/start/size
   identity).
4. **Size extension** (DRWUI:575103-575113): `end += matched_entry->(+0x91)`
   (the entry's size field), then — for the NEXT entry —
   `if (next->(+0x10) == 0xE || next->(+0x10) == 6) end += next->(+0x91)`:
   **the deep range extends over the current entry PLUS one following entry
   whose FS code is 0x0E or 0x06 (both FAT16-family codes).**
   (Refinement 2: the neighbor rule extends by exactly ONE following
   FAT16-family entry, not an unbounded walk.)
5. **Device-size clamp** (DRWUI:575115-575119): `if (device_total_sectors <
   start + end) end = device_total_sectors − start`.
6. **Sub-range construction** (DRWUI:575121-575137): `malloc(0x40)` +
   `FUN_1400d03e0(obj, table, start, end)` builds the derived range device;
   success → `*out_flag = 1`, return the derived device; ANY failure
   (no table, no match, construction fail) → return the ORIGINAL device
   (the fallback = same range as quick).

**The call pair verified first-hand** (DRWUI:567902-567925):
`FUN_14035a6d0(this, 0, volume_entry->device, volume_entry)` → log
`"Quick scan StartSector=%I64d, TotalSector=%I64d"`; then
`FUN_14035a6d0(this, 1, ...)` → `"Deep scan StartSector=..."`. Both log
lines print vtable[0x68] (start) and vtable[0x18] (total) of the RETURNED
device — the trace pair is the observable seam for the derived range.

## C. Core counterpart

`src/core/orchestrator/cr_scan.cpp:1181-1188` (unchanged this session, re-read):
`run_deep` operates on the same `slice` the quick pass used; no resolver, no
small-volume reset, no neighbor extension. The scan-start log band
(`cr_scan.cpp:1323-1330`) logs one extent for both stages.

## D. Divergence candidates

1. No new finding beyond N-182 — the decode CONFIRMS and REFINES the active
   finding (two refinements above). Recommend an addendum to N-182 carrying:
   the start-reset-to-zero semantics, the one-neighbor-FAT16 limit, the
   fallback structure, and the printf seam.

## E. Device-type-6 chase (N-182's open question — narrowed)

- The type getter is vtable slot +0xa8 on the scan device object (the
  `volume_entry + 4` device of the CDoScanPartition's volume entry,
  DRWUI:567904/:567918).
- The type VALUE lives in `CDiskDeviceType` (Partition.mo): ctor
  `FUN_180001560` (Partition_decompiled.c:279-291) sets the type field at
  **+0x20 with default 4**; setter `FUN_180001540`
  (Partition_decompiled.c:269-272). The value is data-driven — stamped by the
  device-enumeration layer (DeviceManager.mo's CWindowsStorageDevice family /
  DeviceAdapter.mo's CDiskDevice/CPartitionDevice), not a constant getter.
- No artifact documents the enum. Cheap constant-getter probes over
  Device/DeviceAdapter/DeviceManager/Partition decompiles found no `return 6`
  type getter — the 6 is set through the CDiskDeviceType field at enumeration
  time (likely from IOCTL_STORAGE_QUERY_PROPERTY device characteristics).
- **Next probe for a future lane**: map vtable slot +0xa8 of
  CVolumeDevice/CWindowsStorageDevice (Device.mo vftable listings at
  Device_decompiled.c:546-611) to the concrete getter, then find its
  CDiskDeviceType construction call in DeviceManager.mo's enumeration; the
  type-6 identity will be the enumerator's classification constant.

## F. Not reached this lane (honest register)

- Engine-side cancel/pause poll points (NTFSSearchDLL/RAWSearchDLL vtable
  callback poll decode vs core's CancelToken loop-head) — the subagent
  timeouts ate the lane budget; prior coverage (D-18's cancellable-first
  design, the S54 Axis 4 walk) dispositions the policy level; the per-engine
  poll-site census remains open for a future lane.
- Stage-transition bookkeeping (record-1 mask semantics) — the rsf-format
  artifact + S-057/N-049 dispositions cover the observable behavior; no
  contradiction found in the liveness battery.
- Quick→advanced transition trigger — not chased.
