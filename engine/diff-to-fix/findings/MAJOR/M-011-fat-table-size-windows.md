# MAJOR M-011 — FAT table-size windows: core rejects whole volume geometries the og scans (FAT32 FAT > 16 MiB, FAT16 spf < 12)

**Severity:** MAJOR (scan-failure on valid geometries — SEVERE-class on those volumes)
**Module:** fat
**Parameters affected:** B1 (detection), S35 (error recovery), X49 (memory), C20-adjacent
**Corpus references:** `FATSearchDLL_decompiled.c` — child-table validation `FUN_180036e90 :44715-44793` (FAT32 spf ∈ [256, 0x96000] sectors @512 = 128 KiB–300 MiB: `if (((uint)(0x100 / (uVar7 >> 9)) <= uVar3) && (uVar3 <= (uint)(0x96000 / (uVar7 >> 9))))` at `:44775`; FAT16 gate `:44743` `0xc/(bps>>9) <= spf` — og REJECTS spf < 12)
**Core references:** `src/core/fat/cr_chain.cpp:80` — `if (table_bytes == 0 || table_bytes > (16u << 20)) return ERR_BAD_PARAMETER;` — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og accepts FAT32 tables from 128 KiB to **300 MiB** (whole-loaded) and requires FAT16 tables of at least 12 sectors; the core hard-rejects any FAT table above **16 MiB** — `scan_fat` aborts the whole volume with `ERR_BAD_PARAMETER` — and accepts FAT16 tables the og would reject. A FAT32 volume with a 17 MiB FAT (≈ 4.25 M clusters — plausible on multi-terabyte volumes with larger clusters) scans in the og and fails outright in the core.

## What the reference does

`FUN_180036e90` validates each FAT flavor's sectors-per-FAT before construction: FAT12 spf ∈ [1,12]; FAT16 spf ∈ [12, 256/(bps>>9)]; FAT32 spf ∈ [256, 0x96000]/(bps>>9) — i.e. up to 300 MiB of table at 512 B sectors, loaded whole (the factory constructs `CFAT32Table` over the entire region, `:3809-3814`). On validation failure the factory retries alternate FAT regions (`:3817-3950`) before giving up on the volume.

## What our core does

`cr_chain.cpp:80` applies a single 16 MiB cap for every flavor and returns `ERR_BAD_PARAMETER`, which `scan_fat` propagates as a volume-scan failure (no fallback, no mirror-region retry at this gate). There is no per-flavor lower bound corresponding to the og's FAT16 ≥ 12-sector rule.

## Gap

Falsifiable claims: (a) FAT32 with a 17 MiB FAT → og scans (window above), core returns ERR_BAD_PARAMETER and the volume is not scanned at all; (b) FAT16 with spf = 8 sectors → og declines the table (flavor gate), core accepts it as valid FAT16.

## Impact

Volumes at the large end of FAT32's range silently disappear from recovery. Combined with M-012 (no mirror-only mode) this makes mid-corruption FAT volumes a hard failure class where the og degrades gracefully.

## Reproduction / verification

Decompile-verified (agent S2-B quotes; `evidence/decompiled-quotes/fat-exfat-session2.md`); core line verified first-hand.

## Fix plan

1. `cr_chain.cpp:80` — replace the flat 16 MiB cap with per-flavor windows matching the og: FAT12 spf ∈ [1,12], FAT16 spf ∈ [12, 256] (@512), FAT32 spf ∈ [256, 614400] (@512) — bounded by device size; for FAT32 above the whole-load budget, prefer a windowed accessor (same shape as the S-013 exFAT fix) over refusing.
2. Add the FAT16 ≥ 12-sector lower bound to `cr_fat_quick.cpp`'s flavor gate.
3. On load failure, retry alternate FAT regions (og factory `:3817-3950` behavior) before failing the volume.

## Tests to add

- `Fat.LargeFat32TableAccepted` — 17 MiB FAT fixture: volume scans, chains resolve (fails before).
- `Fat.SmallFat16Rejected` — spf=8 fixture: og-parity decline (core currently accepts).

## Related findings

- S-013 (the exFAT sibling of this class, where the core is lossier rather than stricter) · M-012 (mirror/fallback behavior on load failure) · N-023 (flavor boundary 65524)

## Open questions

None.
