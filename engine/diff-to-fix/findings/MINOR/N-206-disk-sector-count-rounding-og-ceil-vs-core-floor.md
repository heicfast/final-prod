# MINOR N-206 — disk sector-count rounding: the og CEILS the last partial sector (GET_DRIVE_GEOMETRY_EX path), the core FLOORS it — the tail of a non-sector-multiple device is unaddressable in the core

**Severity:** MINOR
**Module:** storage / transport (device geometry derivation)
**Parameters affected:** #47 Throughput (addressable scan range), #2 Recovery correctness (the final partial sector's content), #20 On-disk struct offsets and sizes (the device-length→sector-count mapping)
**Corpus references:** `re-workspace/reversed/ghidra-mo/Device_decompiled.c:56523-56545` (the geometry cascade: open-access 0x70000 → IOCTL 0x74004 [GET_LENGTH_INFO-class] → IOCTL 0x7405C [drive-geometry-EX-class]), `:56536-56538` (`local_60 = (local_50 − 1 + local_34) / local_34` — the CEIL form on the geometry-EX length, with `local_34 = 0x200` fallback when geometry reports zero), `:56538-56545` (the 4 MiB device chunk sizing that follows: `0x400000 / sector_size` at obj+0x10)
**Core references:** `src/win/cr_transport.cpp:134` (`g.sector_count = g.total_bytes / (g.logical_sector ? g.logical_sector : 512)` — FLOOR, unconditionally), `:139` (the volume-handle fallback path, also floor: `exact / 512`), `src/core/storage/cr_image.cpp:427` (the raw-image path: `total_sectors * SECTOR` — sector-multiple images only, floor-equivalent) — at audited core `9633d94f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When a device's (or image's) byte length is not an exact sector multiple, the og and the core disagree on how many sectors it has: the og's authoritative geometry-EX path computes **ceil(length/sector)** — `(length − 1 + ss) / ss` — so the final partial sector is addressable (reads that touch it succeed, zero-padded by the driver); the core computes **floor(length/sector)** everywhere — the tail partial sector is invisible: reads spanning it are clamped or refused, and any format signature living in that tail is unreachable. The two agree on every sector-multiple length (the overwhelmingly common case); they diverge exactly on truncated images, odd-sized virtual disks, and mis-declared RAW files.

## What the reference does

The device-open cascade (`Device_decompiled.c:56523-56545`): after the 0x70000 access call, the 0x74004 length IOCTL yields `{length → local_28}` and a first division `local_60 = local_28 / local_34` (floor — the fallback). The 0x7405C geometry-EX IOCTL then overwrites the sector count with `local_60 = (local_50 − 1 + local_34) / local_34` — the CEIL form (for length = q·ss + r, 0 < r < ss: (q·ss + r − 1 + ss)/ss = q + 1) — with `local_34` defaulting to 0x200 when geometry reports a zero sector size. The geometry-EX result is the authoritative one (it runs after and overwrites). The subsequent 4 MiB chunk sizing (`0x400000 / local_34` at `:56538-56545`) and the whole scan-range derivation ride this sector count — the og scans the tail partial sector as a full (driver-padded) sector.

## What our core does

`cr_transport.cpp:134`: `g.sector_count = g.total_bytes / logical_sector` — floor, for the geometry-EX-present path. `:139`: the volume-handle fallback, `exact / 512` — floor. The image path (`cr_image.cpp:427`) sizes by `total_sectors * SECTOR` — a sector-multiple image declares its own multiple; a truncated image floors at open. Every downstream consumer (PartitionSlice bounds, scan ranges, the lost-partition device-tail bound at `cr_candidates.cpp:930-933` (`disk->size() / ls`), the carve window fill) inherits the floored count.

## Gap

Stated falsifiably: a 1,025-byte device image at 512-byte sectors yields og sector_count = 3 (ceil), core sector_count = 2 (floor); a read of sectors [2,3) succeeds zero-padded in the og and is refused/clamped in the core; a file signature whose header lies at image offset 1024 is reachable by the og and unreachable by the core.

## Impact

Edge-case correctness on non-sector-multiple sources — truncated forensic images, oddly-sized RAW/VHD variants, and files mis-mapped as devices. The last ≤511 bytes of such a source are structurally invisible to the core (every layer from slice bounds to carve windows floors), while the og addresses them as a padded final sector. Not a mainstream-path defect (real disks are sector-multiple by construction), but the user's "sector numbers / calculation" directive names exactly this class, and the divergence is a clean, provable arithmetic difference. MINOR.

## Reproduction / verification

Og-side: the cascade and both division forms read first-hand at `Device_decompiled.c:56523-56545` this session (the ceil overwrite at `:56536-56538` quoted in the finding; the 0x200 fallback and the 4 MiB chunk derivation in the same region). Core-side: both transport division sites and the image-path sizing read at the cited lines at `9633d94f`. Arithmetic verified by evaluation (1025/512: og 3, core 2; 1024/512: both 2). Worksheet: `evidence/s55-lane-sheets/S55-D2-scanmath.md` §3, DC-3.

## Fix plan (for the engineering team)

1. `src/win/cr_transport.cpp`: replace both sector-count divisions with the ceil form `g.sector_count = (g.total_bytes + ss − 1) / ss;` (and the `exact` fallback likewise), with a comment citing the og's `(L−1+ss)/ss` at Device:56536-56538.
2. Audit the read path for the final partial sector: `read_at` on the last sector must succeed with a zero-padded tail (the driver/og semantics) — the core's file-backed devices already zero-pad short reads; verify the volume/direct paths do too, and add the tail-read test.
3. The device-tail bound consumers (`disk->size() / ls` sites, e.g. `cr_candidates.cpp:930-933`) should switch to the geometry's sector_count accessor so the ceil propagates — grep `size() / ` for the division sites.
4. Tests: `Transport.TruncatedImageSectorCountCeils` (a 1025-byte raw image: sector_count == 3); `Transport.TailPartialSectorReadableZeroPadded` (read of the last sector succeeds, tail bytes zero); `LostCandidates.TailBoundUsesCeiledCount` (a candidate at the final partial sector is not dropped as past-the-disk).

## Tests to add

- `Transport.TruncatedImageSectorCountCeils` (fails today)
- `Transport.TailPartialSectorReadableZeroPadded` (fails today)
- `LostCandidates.TailBoundUsesCeiledCount` (fails today)

## Related findings

M-119 (FIXED — the partition-boundary read semantics: the og clamps+zero-fills+ succeeds; this finding is the device-length sibling of that landed volume-level rule), M-144 (active — the ext4 volume caps' boundary family), OQ-S4-3 (the chunking bench — the ceil changes the block count on odd lengths).

## Open questions

None (the og's own 0x74004 fallback floor path is dead in practice — the geometry-EX path always runs after and overwrites; the shipped behavior is ceil).
