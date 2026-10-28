# MINOR N-173 — FAT 2106-02-07+/2107 dates collapse to the zero FileTime (u32 clamp) while the og converts them fully; the exFAT sibling decoder has no clamp (core-internal inconsistency)

**Severity:** MINOR
**Module:** fat (datetime edge)
**Parameters affected:** time and timestamp handling (9), detection correctness edge
**Corpus references:** `DosDateTimeToFileTime` (the og's converter, FAT:32135/:32144/:32151) handles the full FAT year field (≤2107) — no 2106 cutoff
**Core references:** `src/core/fat/cr_lzn.cpp:173-174` (`return secs > 0xFFFFFFFFULL ? 0 : u32(secs);` — the clamp), the u64 FileTimes contract `include/cr/cr_types.h:45-50`, the unclamped exFAT sibling `cr_exfat_quick.cpp:190-200`; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The FAT path clamps unix seconds to u32: dates past 2106-02-07 06:28:15 (legal FAT
dates up to 2107-12-31, year field 127) return 0 → the caller emits the invalid
(1601-01-01) FileTime. The og converts them fully. The clamp's "u32 props contract"
comment is stale (FileTimes is u64), and the exFAT twin has no clamp — the two FAT-family
decoders disagree with each other.

## What the reference does

Full conversion (the Win32 call has no such cutoff).

## What our core does

The clamp (cite above) + the inconsistent sibling.

## Gap

date word 0xFFBF (2107-12-31): og → real FILETIME; core → 0.

## Impact

Edge-year class only (timestamps on files dated 2106-2107 — rare, but FAT-legal and
occasionally set by buggy writers); the core-internal inconsistency is the larger
defect (two decoders, two truths for the same family).

## Reproduction / verification

Core-derived (the clamp + sibling read directly; the stale comment noted).
Corpus-derived (the converter cites). Arithmetic: 2107-12-31 unix secs ≈ 0xFFFFFFFF+… —
exceeds u32.

## Fix plan

1. Drop the clamp: `fat_date_to_unix`/the lzn caller returns u64 seconds (the
   downstream FileTimes is u64); or raise the ceiling consistently.
2. Files: `src/core/fat/cr_lzn.cpp` (+ the comment).
3. Tests: the 2107 boundary (below) + a 2106-02-07 exact-boundary case.
4. Acceptance: FAT and exFAT produce identical results for the same packed date/time;
   the 2107 fixture yields the real timestamp.

## Tests to add

- `Fat.Date2107ConvertsFully` (fails today)
- `Fat.DateBoundary2106_02_07` (the exact u32 edge)
- `Fat.ExfatFtSameDateSameResult` (family consistency)

## Related findings

- A-014 (the same helper's time-word validation — fix together)
- I-006/I-007 lineage (timestamp superset registrations)

## Open questions

None.
