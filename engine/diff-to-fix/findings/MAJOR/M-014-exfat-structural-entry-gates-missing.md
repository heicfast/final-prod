# MAJOR M-014 — exFAT: og's structural entry gates are missing in the core (empty files suppressed, timestamp sanity, secondary-count cap, name validation)

**Severity:** MAJOR (false-positive surface; user-visible entry differences)
**Module:** exfat
**Parameters affected:** B3 (false positives), B4 (classification), B10 (names)
**Corpus references:** `exFATSearchDLL_decompiled.c` — `FUN_180002f90 :1787-1791` (secondary_count ∈ [2,18] via `(byte)(*param_1 - 2U) < 0x11`; create-timestamp sanity: month 1-12, day≠0, hour<24, min<60, sec<60), `FUN_180029280 :34562-34564` (stream accept requires `data_length != 0 && first_cluster != 0 && name_length != 0 && next-entry type&0x7F == 0x41` — **zero-length files are not emitted**), `FUN_180003030 :1814-1829` (name entry: flags&3==0, flags<4, first char ≠ 0, UTF-16 validity table), `FUN_180003090 :1840-1846` (volume-label/root triple placement `{0x83@0, 0x81@+0x20, 0x82@+0x40}`)
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:278-345` (type classes, stream reserved-zero, `fc_invalid` only) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og enforces a set of cheap structural gates on exFAT directory entries that the core lacks entirely: secondary-count bounds (2..18), per-timestamp field sanity (month/day/hour/minute/second ranges), stream fields all non-zero (so **empty files are dropped from results**), name-entry flag constraints and UTF-16 validity, and volume-label placement. The core's accept-set is wider, so garbage-adjacent entries the og rejects appear in core results.

## What the reference does

Quotes above: `FUN_180002f90` bounds the set and validates the create and modify timestamps field-by-field; `FUN_180029280` refuses streams with zero length/cluster/name; `FUN_180003030` validates the name entry; the root triple placement is checked in `FUN_180003090`.

## What our core does

`cr_exfat_quick.cpp:278-345` dispatches on entry-type classes, checks stream reserved-zero fields and `fc_invalid` (first cluster 0/1 or ≥ chain ceiling), but has none of the count/timestamp/length/name gates.

## Gap

Falsifiable claims: (a) an empty exFAT file (data_length=0) appears in core results, absent in og; (b) a dentry with month=0 garbage timestamps is skipped by og, emitted by core; (c) a set with secondary_count = 40 is rejected by og, walked by core (potential OOB-shaped walking on corrupt media); (d) a name entry with flags&3 != 0 is rejected by og, accepted by core.

## Impact

False-positive surface on corrupt/damaged media (the deep-scan scenario) and per-entry grading noise; (c) is also a robustness bound.

## Reproduction / verification

Decompile-verified (agent S2-B quotes; `evidence/decompiled-quotes/fat-exfat-session2.md`); core accept-set verified first-hand.

## Fix plan

1. In the exFAT entry-set walk, add the four gate families with the og's exact constants: secondary_count ∈ [2,18]; create+modify timestamp field sanity (treat all-zero as valid — the og skips only non-zero garbage); stream `data_length != 0 && first_cluster != 0 && name_length != 0`; name-entry `flags&3 == 0 && flags < 4 && first char != 0` + UTF-16 validity.
2. Decide the empty-file policy explicitly: og parity = suppress; register in `docs/parity/PARITY-DELTAS.md` if the core intentionally keeps them (then bench comparisons must filter).
3. Extend the structure-aware fuzz harness with mutants hitting each gate.

## Tests to add

- `ExFat.EmptyFileSuppressed` / `ExFat.GarbageTimestampRejected` / `ExFat.SecondaryCountCapped` / `ExFat.BadNameFlagsRejected` — all fail before, pass after.

## Related findings

- M-013 (checksum gates) · N-023 (tz and validator minors)

## Open questions

The `data_length != 0` gate also suppresses legitimate empty files (og behavior); confirm the product wants strict parity before inheriting the quirk — flagged in the fix plan.
