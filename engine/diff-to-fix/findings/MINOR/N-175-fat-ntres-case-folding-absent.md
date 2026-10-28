# MINOR N-175 — FAT 8.3 NTRes case-folding absent: the og applies the NT reserved byte's case flags (strupr/strlwr per 0x00/0x08/0x10/0x18); the core never reads `nt_reserved` — short names render always-uppercase

**Severity:** MINOR
**Module:** fat (8.3 name presentation)
**Parameters affected:** filename handling (10)
**Corpus references:** `FATSearchDLL_decompiled.c:44191-44216` — the name builder applies NTRes (+0x0C) before joining: 0x00 → base+ext upper; 0x08 → base lower/ext upper; 0x10 → base upper/ext lower; 0x18 → both lower
**Core references:** `src/core/fat/cr_fat_quick.cpp` / `cr_lzn.cpp` / `include/cr/fat/cr_fat_wire.h:89` — the field is declared (`nt_reserved`) but never read (grep-verified); `short_name_to_u16` emits the raw on-disk bytes (usually uppercase); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Windows stores the display case of 8.3 names in the NT reserved byte; the og's name
builder applies it (a dentry stored "FILE001 DAT" + NTRes 0x18 renders "file001.dat").
The core never reads the field — such files render "FILE001.DAT". Case-only divergence,
systematic for NT-created short names.

## What the reference does

The four-way fold (cites above, lane E first-hand decode).

## What our core does

The field is dead in the core (declared, never consumed).

## Gap

"FILE001 DAT" + NTRes 0x18: og `file001.dat`; core `FILE001.DAT`.

## Impact

Presentation-only (case) on the no-LFN 8.3 population; name-equality benches that
compare case-sensitively will diverge. Recovered content unaffected.

## Reproduction / verification

Corpus-derived (the fold table decoded with line cites). Core-derived (the dead-field
grep + the raw-bytes emission read).

## Fix plan

1. Read `nt_reserved` in the 8.3 name builder and apply the four-way fold (mirror of
   the og's builder; the LFN path is unaffected — LFN carries true case).
2. Files: the short-name assembly in `cr_lzn.cpp` (or wherever `short_name_to_u16`
   lives).
3. Tests: the four NTRes values on one dentry.
4. Acceptance: the four cases render as the og's table; LFN names unchanged.

## Tests to add

- `Fat.NtResCaseFold00/08/10/18` (a 4-case parametrized pin; fails today)

## Related findings

- N-172 / N-176 (the same name-builder family)

## Open questions

None.
