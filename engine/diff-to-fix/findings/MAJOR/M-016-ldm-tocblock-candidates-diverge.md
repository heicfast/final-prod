# MAJOR M-016 — LDM TOCBLOCK candidate set/order diverges: core misses config+0, prefers +1 where the og prefers +2

**Severity:** MAJOR (dynamic-disk volumes missed on nonstandard-but-valid layouts)
**Module:** ldm (table)
**Parameters affected:** B1 (detection), C20 (offsets), B8-adjacent
**Corpus references:** `ghidra-mo/LdmManager_decompiled.c` — TOCBLOCK probe `FUN_180072ad0 :59573-59606` (candidates: `config+2` `:59573`, then `config+0` `:59584`, then `config+1` `:59593`; first-valid wins; failure trace `"disk%d Cannot find TOCBLOCK sign \r\n"` `:59606`); config start from PRIVHEAD@0x12B `:59748`; name checks `"config"`@+0x24 `:59612`, second bitmap name@+0x46 `:59615`; bitmap range gate `"bitmaps are out of range. Giving up."` `:59625`; VMDB anchored at TOCBLOCK-declared bitmap1 `:59630-59632`
**Core references:** `src/core/table/cr_ldm.cpp:1112-1131` — `const u64 toc_offs[4] = {1, 2, 2045, 2046};` first-valid wins — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og probes TOCBLOCK at {config+2, config+0, config+1} (first valid wins); the core probes {config+1, config+2, config+2045, config+2046}. Three concrete divergences: (a) a disk whose only valid TOCBLOCK sits at **config+0** parses in the og and returns `ERR_BAD_CHECKSUM` in the core — LDM volumes missed; (b) the kernel's tail copies (config+2045/2046) are a core superset (og cannot find them); (c) on two valid-but-conflicting copies at +1 and +2, the og prefers the **+2** copy and the core prefers **+1** — different VMDB positions → different database state on torn-write disks.

## What the reference does

Quoted above: the og's ordered candidate walk with per-copy magic + name validation, first-valid-wins semantics, and VMDB anchoring from the winner's bitmap1 field.

## What our core does

`cr_ldm.cpp:1115` — `{1, 2, 2045, 2046}`; first parseable copy wins; later copies only counted for the `tocblocks_matched` stat.

## Gap

Falsifiable claims per (a)(b)(c) above; each is constructible with a single-sector edit of a dynamic-disk fixture.

## Impact

Detection completeness for LDM (a first-class capability per the audit mandate) and database-state choice on torn disks.

## Reproduction / verification

Decompile-verified (agents S2-D1/S2-G3, quote at `:59573-59608`); core list verified first-hand at `e93decf`. Empirical single-sector fixtures are one-line extensions of the existing `test_ldm.cpp` corpus.

## Fix plan

1. Append `0` to `toc_offs` after `2046` (preserving the core's tail-copy superset): `{1, 2, 2045, 2046, 0}` — or, for strict order parity, `{2, 0, 1, 2045, 2046}`. Recommended: `{2, 0, 1, 2045, 2046}` reproduces og preference exactly (a +2 copy wins over +1) while keeping the superset tail copies behind og-parity positions.
2. Keep the mismatched-copy counter semantics.

## Tests to add

- `Ldm.TocblockAtConfigPlus0` — only valid copy at config+0: volumes recover (fails before).
- `Ldm.TocblockPrefersPlus2OnConflict` — valid-but-different copies at +1 and +2: VMDB follows the +2 copy (pins og preference).
- `Ldm.TocblockTailCopies` — core superset stays (2045/2046 accepted).

## Related findings

- OQ-T7 (resolved MATCH — the RAID-5 math itself is og-correct; this finding is about *finding* the database) · N-025 (config-size and interleave geometry minors)

## Open questions

None.
