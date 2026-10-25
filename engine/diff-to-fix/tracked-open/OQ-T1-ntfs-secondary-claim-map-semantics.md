# OQ-T1 (tracked-open) — Secondary claim map in the og NTFS viability probe: semantics undecoded

**Status:** UNVERIFIED — could not be resolved in this session (needs targeted RE decode)
**Module:** ntfs
**Parameters affected:** B4 (deleted-file classification), D15 (confidence grading model)
**Corpus artifact to examine next:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` — `FUN_18001acc0` viability block (~:21209–:21231), the bitmap-object layout (controller +0x40/+0x48), and the secondary-map vtable slot `bitmap[0xC] → vt+0x20`; cross-reference `FUN_18002c2e0` (primary bit test, :35643–35657).

## Precise question

In the og's deleted-file viability loop, each run's cluster is tested with

```
hit = FUN_18002c2e0(bitmap, sector)
      || secondary: bitmap[0xC] → vt+0x20(sector)
```

(agent-B verified, NTFSSearchDLL_decompiled.c :21216–21231). What is the object behind
`bitmap[0xC]`, what does its vtable+0x20 method test, and in which direction does a secondary
hit swing the intact/total accounting (i.e., does a secondary hit count the cluster as
*re-allocated* or as *still-intact*)?

Candidate hypotheses to disambiguate (from corpus context, all unproven):

- H1: a second bitmap view maintained by a different engine pass (e.g., FAT-family
  `$Bitmap`-like structure built during the same scan) used to corroborate allocation.
- H2: a "recently freed" overlay (inverse semantics — hit means still-free → counts intact).
- H3: the volume-bitmap object of the *complete* scan's own writer (self-referential update
  path).

## Why it matters

- The destroy branch (`intact == total → Release()`, S-001) consumes the hit count. Whether the
  secondary map adds or removes hits changes which files the og destroys — and therefore what
  strict parity for S-001's fix actually requires.
- Our core uses a single $Bitmap `reused_ratio` (`src/core/ntfs/cr_mftwalk.cpp:678`,
  `src/core/ntfs/cr_bitmap.cpp:86–102`). If H2 is correct, our ratio is biased toward
  "reused" relative to the og.

## Evidence already in hand

- Primary test decompile (B-ntfs.md §3.3, verified :35643–35657): pure bit test
  `buffer[pos >> 3] & (1 << (pos & 7))` with bounds + null guards.
- The `||` composition is verified at :21216–21231 (agent-B).
- `ntfs-engine-algorithms-v2--deleted-viability.md` §3 labels the secondary map INFERRED —
  the corpus itself flags it unresolved (its own OQ).

## Suggested probe (next session with Ghidra)

1. Open NTFSSearchDLL in Ghidra; navigate to FUN_18001acc0; find the bitmap-object fetch
   (controller+0x40 / fallback +0x48).
2. Identify the type at slot [0xC] via RTTI (`CBuildVolumeBitmap*`?) and decompile its
   vtable+0x20 entry.
3. Compare its bit-source (which buffer, which base) against the primary
   (`bitmap[0]` buffer, base `+0x50`, limit `+0x10`).
4. Record the direction (allocated vs free) and update S-001's fix plan accordingly.

## Core-side reference (audited commit 67193b6)

- `src/core/ntfs/cr_bitmap.cpp:14–84` (load), `:86–102` (reused_ratio — single source).
