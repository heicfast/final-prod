# MINOR N-112 — the LZH (LHARC/LHA) archive carve format is absent: the og registers it LIST-ONLY (the u32 E4 52 5C 7B key the census attributed to LZH actually binds OneNote — erratum E53), gates the -lh?- method family at +2, and walks local header chains (200 MiB cap)

**Severity:** MINOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1-adjacent, A2-adjacent, A28-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:3097-3133` (Brance::LZHAnalyser id 0x24 + FileLZHAnalyser id 0x24 — **list-only**; no u32 key: the 0x7b5c52e4 key at :2175 binds OneAnalyser, read verbatim this session — erratum E53); the probe `:46937-46973` (FUN_18003db20: `*(int*)(buf+2)` ∈ {"-lh5" 0x35686c2d, "-lh0" 0x30686c2d, "-lhd" 0x64686c2d, "-lh7" 0x37686c2d} AND `buf[6]=='-'` — the level-1 LHARC method field at bytes 2-6); the size walk `FUN_18003dd70` entry-chain walk (invoked from :47010-47013 with the FileInfo bounds {0, 0xc800000 = 200 MiB} checked at :46927).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — no lzh probe (the rawsigns "crwl" entry is unrelated)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred LZH; the census's u32-key attribution was wrong (E53); this session decoded the list registration + grammar + walk.

## Summary

LZH archives are absent from core. The og's registration is list-layer ONLY (the census's u32-key row was a mis-attribution now corrected as E53): the probe gates the -lh0/-lh5/-lhd/-lh7 method field at +2 with the trailing '-' at byte 6, and the size is a local-header entry-chain walk bounded at 200 MiB. Japanese-legacy archive niche; MINOR.

## What the reference does

See corpus field: list registration, method-family memcmp, entry-chain walk, 200 MiB cap.

## What our core does

No LZH analyser.

## Gap

Falsifiable: a level-1 header with "-lh5-" at bytes 2-6 carves in the og (id 0x24, entry-walk extent); core carves nothing.

## Impact

Legacy archive niche (Japanese software distribution); the structural walk means torn archives still salvage their leading entries — parity value for the archive-recovery story.

## Reproduction / verification

Decompile-direct: probe + walk read this session.

## Fix plan (for the engineering team)

1. Add FMT_LZH as a list-layer probe. 2. Implement the method-family gate + the entry-chain walk (header-size-driven stride, bounds {0, 200 MiB}). 3. Fail-closed on a broken entry chain (the og walks until the chain breaks).

## Tests to add

- `Carve.LzhMethodFamilyAccepted` — -lh0/-lh5/-lh7 forms carve.
- `Carve.LzhEntryChainExtent` — a two-entry archive sizes to the chain end.
- `Carve.LzhBrokenChainSalvagesLeadingEntries` — a torn second header ends the walk cleanly.

## Related findings

S-007 (batch-2 parent); S-041 (school 1, landed `b92ebef`) (ONE — the chain that actually owns the E4 52 5C 7B key); erratum E53.

## Open questions

None material.
