# MINOR N-198 — exFAT boot-gate strictness beyond the og: core-only `volume_length >= 2048` + device-fit, `cluster_count <= 0xFFFFFFF5`, `fat_offset >= 24` bounds — register the deliberate hardening stance

**Severity:** MINOR
**Module:** exfat (boot gates)
**Parameters affected:** B3 (false-positive suppression — core stricter), Q68 (registration completeness)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c` FUN_180021e80 (:28180-28210 — the complete og gate set, re-read first-hand this session: 0x55AA + "EXFAT" + MustBeZero sweep + `fat_length(+0x54) != 0` + `heap_offset(+0x58) != 0` + `fat_offset(+0x50) != 0` + `volume_length(+0x48) != 0` + `cluster_count(+0x5c) != 0` + numFATs ∈ {1,2} + shift band + root-cluster > 1 + ordering/fit inequalities — the `!= 0` forms, no numeric floors)
**Core references:** audited `34d6cf10` — `src/core/exfat/cr_exfat_dentry.cpp:150-152` (`volume_length < 2048 || > u64max/bps || × bps > device size`), `:158` (`cluster_count > 0xFFFFFFF5`), `:164-165` (`fat_offset < BOOT_REGION_SECTORS → reject`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's exFAT boot gates require the four size fields to be merely nonzero (plus ordering/fit); the core adds three numeric bounds the og does not have: volume_length ≥ 2048 sectors (+ device fit), cluster_count ≤ 0xFFFFFFF5, fat_offset ≥ 24 (the boot-region size). A garbage boot sector with fat_offset ∈ [1,23] or volume_length ∈ (0, 2048) enters the og's engine (scanning nonsense) and is refused by the core. Core-stricter-in-the-FP-direction — a deliberate-hardening candidate that was never registered.

## What the reference does

The complete FUN_180021e80 gate conjunction (quoted in the finding header; the `!= 0` forms confirmed line-by-line this session — the full gate set is the same function S-062's backup re-validation calls).

## What our core does

The three numeric bounds quoted above (`cr_exfat_dentry.cpp:150-165`).

## Gap

Falsifiable per bound: a boot sector with `fat_offset = 10` (nonzero, ordering-consistent): og accepts into the engine; core rejects with ERR_BAD_PARAMETER. Nonsense-BPB noise only — the og's engine then fails on the FAT read; the core refuses earlier and cheaper.

## Impact

FP-suppression divergence only; no recovery population is lost (no real formatter emits fat_offset < 24 — the boot region is 24 sectors by spec). The board's completeness standard requires the stance registered rather than silent.

## Reproduction / verification

Decompiled evidence (the complete gate set re-read first-hand); core side quoted at `34d6cf10`.

## Fix plan (for the engineering team)

1. Register the three bounds in PARITY-DELTAS as deliberate hardening (the D-70 precedent: core-stricter bounds with the og sites acknowledged and the "no real-formatter population" rationale).
2. No behavior change recommended.
3. Files: `docs/PARITY-DELTAS.md`.

## Tests to add

- `Exfat.HardeningBoundsPinned` (the three refusal edges asserted — currently unasserted as deliberate).

## Related findings

- S-066 (the same function's missing re-base — the opposite direction on the same surface; fix both in one pass over the exFAT open).
- M-142/N-191's family (acceptance-band registration completeness).

## Open questions

None.
