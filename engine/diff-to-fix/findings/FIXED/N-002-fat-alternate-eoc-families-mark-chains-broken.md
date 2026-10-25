# MINOR N-002 — Alternate EOC families (dirty/reserved FAT variants) mark chains broken instead of ending them cleanly

**Severity:** MINOR
**Module:** fat
**Parameters affected:** A27 (fragment handling), B4 (grading via chain-end semantics), D15
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` :8030–8048 (agent-B verified); `re-workspace/artifacts/SearchDLL/fat-engine--deep-dive.md` §3
**Core references:** `src/core/fat/cr_chain.cpp:95–127` (`FatTable::next`), `:166–170` (`follow_chain` bogus-pointer branch) — audited commit `67193b6`
**Status:** CLOSED — FIXED in `fb44073` (verified independently by the session-2 audit: FATSearchDLL :8030-8048 mask-24 family re-read first-hand — `(DAT & 0xfffffff) == 0xffffff8` plus the 0x7fffff8 sub-family exactly as the fix models; core cr_chain.cpp :73-80 applies the og tolerance on the RAW entry before canonical tests; test Fat32EocMask24AndAlternateFamiliesEndClean green in the 313/313 ASan suite; see Resolution)

## Summary

The og's FAT32 chain termination recognizes a family of tolerant EOC forms written by some
BIOS/embedded formatters: a mask-24 form (`(entry & 0x07FFFFF8) == 0x07FFFFF8`) and an
alternate form family (`(entry & 0x0FFFFFFF) == 0x0FFFFFF0 && (next & 0xFFFFFFF0) ==
0xFFFFFFF0`). Our core recognizes only the canonical `>= 0x0FFFFFF8` (after `&0x0FFFFFFF`) and
treats the remaining reserved range as a pointer; out-of-bounds pointers set `broken=true`.
Chains ending in the alternate forms therefore end **cleanly** in the og and **broken** in
ours — same bytes recovered, different chain grade.

## What the reference does

Decompiled termination checks (:8030–8048, agent-B verified, B-fat.md §3.2):

```
entry == 0xFFFFFFF8                                        → EOC
(entry & 0x0FFFFFFF) == 0xFFFFFF8 && (next & 0xFFFFFFF8) == 0xFFFFFFF8 → EOC
(next & 0x07FFFFF8) == 0x07FFFFF8                          → EOC (mask-24 secondary)
(entry & 0x0FFFFFFF) == 0xFFFFFF0 && (next & 0xFFFFFF0) == 0xFFFFFF0 → EOC (alternate family)
```

Documented in `fat-engine--deep-dive.md` §3 as tolerant termination for corrupt volumes
(reserved 0x0FFFFFF6-7 also treated as end).

## What our core does

`src/core/fat/cr_chain.cpp:107–116`:

```cpp
if (v >= CHAIN_EOC_MIN && v < 0x0FFFFFF0)
    return CHAIN_EOC_MIN; /* canonical marker */
if (v >= 0x0FFFFFF0) {
    /* FAT12 0xFF0-0xFF6 / FAT16 0xFFF0-0xFFF6 / FAT32 reserved: treat as
     * reserved-value -> end of chain for walking purposes */
    return CHAIN_EOC_MIN;
}
```

Everything else (including values like 0x17000000 or 0x27FFFFF8, i.e. the mask-24 family with
high bits set) is returned as a cluster number; `follow_chain:166–170` then sees
`n > t.max_cluster()` → `r.broken = true`.

## Gap

Falsifiable claim: a FAT32 chain whose last FAT entry is e.g. 0x27FFFFF8 (mask-24 family with
dirty bit 0x20000000) ends with `broken=false` (clean EOC) in the og and `broken=true` in the
core, with identical `clusters` content up to that point.

## Impact

- Grading only (chain-end semantics feed confidence downgrade paths); recovered bytes
  identical.
- Affected volumes: those written by BIOS/embedded formatters that use non-canonical EOC
  values — rare, but a real legacy-floppy/industrial-storage population.

## Reproduction / verification

- Corpus-derived: decompile quote (file+lines).
- Core-derived: code quotes; `CHAIN_EOC_MIN = 0x0FFFFFF8` (`include/cr/fat/cr_fat_engine.h`).
- Executed: unit-level — construct `FatTable` with entry `0x27FFFFF8` following a valid
  cluster; assert `follow_chain().broken == false` after fix (fails before). Not run against
  og (both code paths quoted).

## Fix plan

1. In `FatTable::next` (FAT32 branch), before returning the raw value, apply the og's family
   tests in order: mask-24 (`(v & 0x07FFFFF8) == 0x07FFFFF8` with high-dirty bits masked) →
   `CHAIN_EOC_MIN`; alternate family (`(v & 0x0FFFFFFF) >= 0x0FFFFFF0` is already covered by
   the existing `>= 0x0FFFFFF0` branch — verify it subsumes the og's `(e&0xFFFFFFF)==0xFFFFFF0`
   two-entry form; the og's form also inspects `next`, which our single-entry walk cannot —
   document the single-entry approximation).
2. Files: `src/core/fat/cr_chain.cpp` only.
3. Tests (below).

## Tests to add

- `test_fat_volume.cpp::FAT32_EOC_Mask24_Family_Ends_Clean`: chain {5, 6, 0x27FFFFF8} →
  `broken == false`, clusters {5,6}.
- `test_fat_volume.cpp::FAT32_EOC_Alternate_Family_Ends_Clean`: chain {5, 0x0FFFFFF5} →
  `broken == false` (reserved-range already handled; pin it).

## Related findings

- I-002 (BAD-cluster family — same termination-semantics area).

## Open questions

The og's alternate-family test consults both `entry` and `next` (two FAT slots); our walker
tests one entry at a time. Whether any real formatter writes the two-slot form is unknown —
the single-entry approximation is documented rather than resolved.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: N-002`)
**Verified by:** fail-before/pass-after — with `src/core/fat/cr_chain.cpp`
reverted, `FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean` fails at the
first `broken` expectation (the mask-24 chain {5, 6, 0x27FFFFF8} ends
broken); restored, it passes: chains {5,6}, {20}, {10} all end cleanly.
Suite 295/295 (Linux ASan).
**Resolution notes:** the FAT32 branch of `FatTable::entry_at` now applies
the og's mask-24 tolerance (`(raw & 0x07FFFFF8) == 0x07FFFFF8 -> EOC`,
FATSearchDLL Next :8030-8048) on the RAW 32-bit entry before the canonical
28-bit tests; the mask-24 test also subsumes the og's raw `0xFFFFFFF8`
form (checked: 0xFFFFFFF8 & 0x07FFFFF8 == 0x07FFFFF8). The alternate
family was already covered by the existing reserved-range branch
(0x0FFFFFF0..F7 -> EOC after masking) and is pinned by the test's third
chain. The single-entry approximation for the og's two-slot forms
((entry,next) pairs) is documented in the code comment as the finding's
fix plan prescribed — our walker tests one entry at a time and the masked
tests collapse the two-slot semantics.
