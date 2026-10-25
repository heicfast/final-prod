# MAJOR M-005 — Carve is not allocation-bitmap gated (perf + false-positive surface)

**Severity:** MAJOR · **Module:** carve (engine gating) · **Parameters:** X47 (throughput), B3 (false positives), X51-adjacent
**Status:** READ-ONLY FINDING — no fix applied. (History: filed by session 1 inside the grouped
`MINOR/N-008-MAJOR-M-005-*.md` file; this standalone file, created during session 2, is now the
canonical home — the ZIP half of the old group lives in N-008/N-014d.)
**Audited commit:** `e93decf` · corpus `03ad46c`

## Summary

The og gates carve probing with the volume allocation bitmap when one is available: clusters
marked used are skipped wholesale (the whole cluster, plus dispatch suppression for mid-cluster
positions in used clusters), concentrating the carve sweep on free space where deleted files
actually live — both a large throughput win and a false-positive reducer (live-file regions are
not offered as carve candidates). The core carves the whole slice unconditionally.

## What the reference does (session-2 decompile proof — the evidence session 1 lacked)

`RAWSearchDLL_decompiled.c` block-scan `FUN_18000c820 :9446-9460`: with a bitmap object present
(`*param_4 != 0`, `param_4+0x1d == 0`), per candidate position the cluster is tested via
`FUN_180058df0 :67842-67856` — a plain bit test `*(byte*)(base + bit>>3) & 1 << (bit&7)` with
bit-count bound at `[2]` and cluster bias at `[10]`; used → skip the whole cluster
(`local_1e0 += (uVar9 - 1)`) and suppress dispatch; no bitmap present → dispatch everywhere
(raw disks keep whole-device carving).

## What our core does

`src/core/orchestrator/cr_scan.cpp:1040-1075` drives the carve over the whole region; no bitmap
input exists anywhere in the carve path (grep clean). The NTFS `VolumeBitmap` is loaded for
viability grading (`cr_scan.cpp:706-707`) but never offered to the carve driver.

## Gap

Falsifiable claim: on a 50%-used NTFS volume, the og's carve sweep touches ~half the clusters
(and never proposes candidates inside live files); the core probes 100% and can surface
"recoverable" candidates that are fragments of live files.

## Impact

Throughput (~2× on typical used volumes), false positives (live fragments), and I/O budget.
Also the same plumbing S-009's remainder needs for the disk-map's free/used overlay — one
wiring serves both (see the fix plan).

## Reproduction / verification

Decompile-verified this session (agent S2-D2; quote sheet in
`evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`); core absence grep-verified.

## Fix plan

1. Thread a `BitmapProvider` (nullable) into the carve driver: NTFS `VolumeBitmap` and the exFAT
   allocation bitmap are the two producers available today; expose via the scan controller
   (this is S-009-remainder item 1's plumbing — implement once, consume twice).
2. Region iterator: used cluster → skip the cluster (og's `+= spc - 1` semantics); absent
   provider → whole-region carve (raw disks unchanged).
3. Add `carve_bitmap_skipped_bytes` telemetry; bench the used-volume scenario for the
   throughput claim.

## Tests to add

- `Carve.BitmapSkipsUsedClusters` — used NTFS volume: no candidate offsets inside live-file
  extents; raw-disk mode unchanged (`Carve.NoBitmapCarvesEverything`).
- Bench: used-volume carve wall-clock before/after (expect ≥ 1.5× on 50% used).

## Related findings

- S-009 remainder (shares the bitmap wiring) · S-007 (formats — the sweep content) ·
  I-009 (probing-stride superset stays)

## Open questions

None — the og-side semantics are fully quoted.
