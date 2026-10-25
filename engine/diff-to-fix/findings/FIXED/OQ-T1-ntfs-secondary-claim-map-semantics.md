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

---

## Resolution (diff-to-fix remediation session — resolved without new Ghidra)

**Status:** RESOLVED (decompile-first read; the full corpus decompile already
contained every function the suggested probe targeted)
**Resolved by:** direct reads of `NTFSSearchDLL_decompiled.c` at the exact
sites the OQ lists — no RE tooling was needed; evidence lines below.
**Resolution notes — all three hypotheses settled:**

1. **Direction (H2 refuted):** the secondary test composes with the primary
   bit test via `||` into the SAME `iVar20` (intact/re-allocated) counter
   (`:21224-21228`: `bVar5 = FUN_18002c2e0(plVar17, lVar18); if (hit ||
   (plVar13 = plVar17[0xc], plVar13 != 0 && (cVar6 = (*plVar13->vt[4])(
   plVar13, lVar18), cVar6 != 0))) iVar20++;`). A secondary hit swings the
   accounting toward **re-allocated** — exactly like a primary hit. The
   "still-free overlay" hypothesis (H2) is wrong.

2. **Object identity (H1/H3 both wrong):** the "bitmap" handed to the
   viability loop is a **0x70-byte scan-built claimed-sector bit-set** —
   NOT the volume $Bitmap read from disk. Constructor `FUN_18002c330`
   (found by tracing the on-demand allocation at `:18529-18535`:
   `operator new(0x70)` when controller+0x250, context+0x40 and +0x48 are
   all null): buffer@+0x00, byte-size@+0x08, bit-limit@+0x10,
   sectors-per-cluster@+0x18, backing-file handle/mapping@+0x20/+0x28,
   wstring path@+0x30 (ctor-inits from a global constant), base-offset@+0x50,
   mapped-flag@+0x58, **secondary-ptr@+0x60**, memory budget 0x5f400000@+0x68.
   Bits are SET during the scan by marking already-processed files' run
   sectors (range-setter `FUN_18002c1e0`, called from `FUN_180017190` at
   `:18409`, mark call at `:18554`), i.e. "claimed by a file the engine has
   already emitted". The og's destroy test therefore answers "are this
   deleted file's sectors now claimed by live content the walk saw" —
   equivalent to $Bitmap re-allocation on healthy volumes, smaller on
   damaged MFTs (fewer live files seen ⇒ fewer claims ⇒ og keeps more).

3. **The secondary map is DORMANT in the og build:** `FUN_18002c330`
   initializes the +0x60 pointer to NULL (`param_1[0xc] = 0`, `:35675`) and
   **no code path anywhere in the decompile assigns it a non-null value**
   (the only writes to the slot are the ctor zero and other classes' `+0x60`s;
   exhaustively grepped). The vtable+0x20 call is interface polymorphism the
   shipped engine never instantiates — the `||` short-circuits at the null
   check `:21225`. The INFERRED "claim/special-set" reading in the artifact
   and this OQ was right about the direction but wrong about it mattering.

**Consequence for the core:** our single-$Bitmap `reused_ratio` is not
missing any og input (the og's secondary contributes nothing in this build).
The map-SOURCE difference (scan-claimed-set vs $Bitmap) is a damaged-media
edge documented in S-001's Resolution — not a grading divergence on healthy
volumes. OQ-T1 is closed; no code change was required by its answer.
