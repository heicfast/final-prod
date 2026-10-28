# s42 evidence — the scan-mode state-machine decode (I-044 / N-181 / N-182's worksheet)

**Corpus:** `easeus-drw-research` @ `be0cfe9`; decompile = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (cites re-read first-hand this session); `og-codebase/config/version.ini` read directly.
**Core:** `core_recovery` @ `b26017a7`.

## §1 The advanced-phase start order — RAW first (:916443-916540)

```c
:916484  plVar11 = list_head;
:916486  do {
:916491      plVar7 = __RTDynamicCast(entry, 0, &ICompleteFileSystemSearchController::RTTI,
                                        &CRAWSearchController::RTTI);
:916494      if (plVar7 != 0) (**(code **)(*plVar7 + 0x80))(plVar7);   /* start RAW first */
:916498      entry = next;
:916499  } while (entry != list_head);
…
:916511  if (*(int *)(FUN_1400b1f30() + 0x6c) == 0) {              /* the hardwired deep gate */
             …start the non-RAW controllers (the second loop)…
```

## §2 The target resolver — quick mode 0 / deep mode 1 (:567902-567925)

```c
:567904  local_430 = FUN_14035a6d0(param_1, 0, device, …);   /* quick */
:567906      fallback = device;
:567911      FUN_140162e00("Quick scan StartSector=%I64d, TotalSector=%I64d\r\n", …);
             …
:567918-925  the deep twin: FUN_14035a6d0(param_1, 1, device, …) + "Deep scan StartSector=…"
```

`FUN_14035a6d0` (:575024-575200): the deep-only path (param_2 != 0), gated on device type 6 (vtable 0xa8), derives a sub-range target via `FUN_1400d03e0`, flags small volumes `< 0x12C00000` (300 MB) at :575053-57, and extends over neighbor partition entries of FS code 0xE/6. Common case = the same device (the fallback) — the D-36 "same StartSector" wording is the common case, not an invariant.

## §3 The auto-advance state machine (:567875-567892 + the ScanThread)

`CDoScanPartition::DoScan` pushes the phase queue: the quick state at scan start and **state 1 (deep) unconditionally** when +0x3d/+0x3f are clear (resume/import suppression flags):

```c
:567875  if (*(char *)(param_1 + 0x3d) == '\0' && *(char *)(param_1 + 0x3f) == '\0') {
:567877      local_370 = param_1 + 0x1b;  local_3f0 = 1;
:567879      FUN_140351160(local_370, &local_3f0);            /* push state 1 */
```

The scan thread (`FUN_14034ddc0`, :568079-568175) pops states in order; state 0/4 → progress mode 0 + events 0/1; state 1 → mode 1 + events 2/3; abort before the next phase only on stop or settings+0x6c != 0. Controllers per state (`FUN_14056bb20`, :903252-903380): **0 = CDeletedRecoveryController (quick), 1/5 = CGenericRecoveryController (advanced), 4 = CRecycleBinRecoveryController, 6/7 = CCloudRecoveryController**. The recoverability grade (`FUN_140a10e90`: 0% → 0; 1-29% → 1; ≥30% → 2, :1564478-1564510) only selects the coaching text (:1287098-1287130) — **the auto-advance is unconditional and result-independent**; `CQuickScanCompleteWnd`/`CAutoRecoverEntryWnd` (30 s into advanced, auto-close 60 s, :1286935-945, :1077015) is the auto-recover offer, not a deep-scan decision.

## §4 The hardwired gates (:947721-947722)

`DAT_141bc4d60` (settings+0x60) init **1** and `DAT_141bc4d6c` (settings+0x6c) init **0** — no runtime writer anywhere in DRWUI (grep across the decompile): the RAW gate is always-on and the deep-suppression never fires in the shipped product. (`showRaw` in config.ini is a UI display state; RawConfig.ini is per-format toggles = M-024.)

## §5 The host-side sweep (:919487-919505 + the chunk loop)

```c
:919487  bool FUN_140585150(controller, _, chunk_bits) {
:919499      iVar1 = memcmp(bitmap_region, *(param_1 + 0x1e8), chunk_bits >> 3);
:919500      return iVar1 == 0;                                 /* all-0xff → skip chunk */
```

Chunk size = `0x400000 / sector_size` sectors floored at 0x2000 (4 MiB); per non-skipped chunk: the BitLocker detector, then **every** controller's `vtable[0x98](start, chunkSectors, bitmapObject)` fan-out (:916880-885); the bitmap object installs into the `CStorageDeviceWithCache` reader (:919095-919125).

## §6 The creation-site citation correction + ZFS shipped-off

- Searcher-set **creation** is `FUN_140581500` (:917015-917040: the `bAddSpecifyDirFilter`/+0x1a1 precondition, the NTFS→FAT→exFAT→ZFS(gated :917163)→HFS→ReFS order, RAW at :917166, the naming callback at :917255-270); `FUN_140580720` (:916443) is the **run** phase. `architecture/04-recovery-workflow.md` §3.3 and PARITY-DELTAS D-36 cite the run function for creation — erratum (both files).
- `og-codebase/config/version.ini` (BUILDPE copy identical): `scanext4=1`, **no `scanzfs` key** — the ZFS quick factory (:913104-110), the advanced-set ZFS searcher (:917163), and the `CZfsVolumeAnalyze` append (:794584+) are all compiled out of the shipped build (the engine vtable+0x70 alternative path is engine-internal state — [INFERRED] WinPE/licensing).
- The FS-code remap at the lost-partition→engine assignment: {2→3, 3→2, else identity} (:916587-916610) — volume-type→engine-code translation (core's FsFamily probe is direct; no action).

## §7 Core-side pins (at b26017a7)

Stage order quick→deep→carve (`cr_scan.cpp:1170-1275`); default-on (`cr_scan.h:186-189`); stage masks + weights 40/35/25 (`cr_scan.h:72-74`, `:519-521`); `--quick-only`/`--no-carve` CLI opt-outs (`rrecov.cpp:1373-1380`); the occupancy/free-map gates the concurrent option of N-181 would share (`cr_scan.cpp:2118-2177`).
