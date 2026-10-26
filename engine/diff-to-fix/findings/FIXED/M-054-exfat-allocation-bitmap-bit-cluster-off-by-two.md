# MAJOR M-054 — exFAT allocation bitmap read at the wrong bit (bit i vs cluster i+2) — grading AND the S-009 free map shifted two clusters on every real volume

**Severity:** MAJOR (NEW-FINDING, discovered during the S-009 producer verification) · **Module:** exfat (deleted-viability grading + the S-009 free-map producer) · **Parameters affected:** B1 (correctness of the grading input), R25-adjacent (allocation-bitmap semantics)
**Corpus references:** Microsoft exFAT File System Specification §7.1 / Table 22 ("An Allocation Bitmap represents clusters from lowest to highest index. For historical reasons, the first cluster has index 2. ... The first bit in the Allocation Bitmap describes the allocation status of the cluster with index 2") — the spec family the core's own exFAT wire docs cite (SDCFF/elm-chan); corpus `re-workspace/artifacts/SearchDLL/exfat-engine--deep-dive.md` §2 (CQuickSearchAndBuildBitmap mirrors the on-disk 0x81 Allocation Bitmap semantics in memory)
**Core references:** `src/core/exfat/cr_exfat_quick.cpp` `struct Bitmap::test(u32 c)` (read bit `c` for cluster `c` — pre-fix on `d487928`, i.e. INCLUDING the landed S-009 FreeMap base: the producer's `if (!ctx.bitmap.test(u32(c)))` inherited the shift, so the S-009 free map marked cluster `c` per cluster `c+2`'s bit on real volumes); both exFAT test fixtures wrote bits the same way (`setbit(c)` at `img[BM + c/8] |= 1 << (c%8)` for cluster `c`)
**Status:** NEW-FINDING — filed and FIXED in the same session, ON TOP of the parallel S-009 landing (the third-implementation convergence: the parallel lane's `storage::FreeMap` design stands as the base; this fix corrects the engine bit mapping both that landing and the grading path consume)

## Summary

The exFAT engine's in-memory allocation bitmap test read bit `c` when asked about
cluster `c`. The on-disk bitmap's bit `i` describes cluster `i+2` (the first
cluster has index 2 for historical FAT reasons). Every `test(c)` therefore
consulted the allocation state of cluster `c+2` — a two-cluster shift on every
real exFAT volume.

## What the reference does

The on-disk bitmap semantics are spec-fixed (MS exFAT spec §7.1: bit 0 =
cluster 2). The og's engine mirrors the on-disk bitmap in memory
(`CQuickSearchAndBuildBitmap`, corpus exfat-engine--deep-dive.md §2) for
deleted-cluster reasoning — the mirror preserves the on-disk indexing.

## What our core did (pre-fix)

`Bitmap::test(c)` read `data[c/8] & (1 << (c%8))` — bit `c` for cluster `c`.
The core's own fixtures encoded the same shift (`setbit(c)` writing bit `c` for
cluster `c`), so the suite was self-consistently wrong. TWO consumers inherited
the shift: (a) the deleted-viability grading ratio (Overwritten / PartialRisk /
Excellent consulted the wrong clusters), and (b) the S-009 free-map producer
landed by the parallel session (its free bits were derived from the shifted
test — on a real volume the disk map painted cluster `c` per cluster `c+2`'s
state).

## Gap

Falsifiable (and falsified in-session on the landed base): with the fixtures
corrected to the spec (bit `c-2` for cluster `c`) and the engine reverted to
the pre-fix body, `ExfatEngine.EndToEnd` grades `over.dat` — a deleted file
whose cluster IS marked allocated in the bitmap — as **Excellent** instead of
**Overwritten** (it tested bit 40, which the spec assigns to cluster 42, not
bit 38), and the new producer pin test's free/used answers shift by two
clusters. Both reds reproduced on `d487928` + this session's fixture
corrections.

## Impact

Deleted-file viability grading consulted the wrong clusters' allocation state
on every real volume, and the S-009 free overlay painted the wrong cells —
boundary files shifted by two clusters. Not silent data loss (no content
dropped), but wrong user-visible confidence grades and a wrong free overlay:
MAJOR.

## Reproduction / verification

1. Spec: MS exFAT spec §7.1 (learn.microsoft.com exFAT specification; "the
   first bit ... cluster with index 2").
2. Corpus: exfat-engine--deep-dive.md §2 (the engine mirrors the on-disk
   bitmap; same auxiliary-bitmap concept as NTFS $Bitmap).
3. Code: `Bitmap::test` pre-fix body on the landed base (`d487928`), read
   first-hand; the S-009 producer call site shown above.
4. Fail-before executed on the landed base: fixtures corrected + engine body
   reverted → `ExfatEngine.EndToEnd` red at `over->confidence()` (1 vs 3) AND
   `ExfatEngine.FreeMapProducerMatchesSpecBitMapping` red; engine fixed →
   both green, full suite 574/574 on Release + ASan + UBSan.

## Fix plan (executed)

1. `Bitmap::test(c)`: read bit `c-2` for cluster `c` (clusters 0/1 reserved —
   never heap bits); bounds by the bitmap bytes.
2. Fixtures: `setbit(c)` writes the bit FOR cluster `c` (bit `c-2`) in both
   exFAT fixture builders; grading expectations unchanged (they were written
   against cluster semantics — only the bit placement was wrong).
3. New pin test `ExfatEngine.FreeMapProducerMatchesSpecBitMapping`: the
   producer's free/used answers at the sentinel clusters (over.dat's reused
   40, gone.dat's free 30, the structural set) + `range_free` at the
   sentinel byte ranges — the spec mapping pinned through the public seam.

## Tests to add

- `ExfatEngine.FreeMapProducerMatchesSpecBitMapping` (added; fail-before
  executed as above).
- The existing `ExfatEngine.EndToEnd` grading assertions become real once the
  fixtures are spec-correct (fail-before executed; grading expectations
  unchanged).

## Related findings

- S-009 (the producer this fix corrects — the landed FreeMap design itself is
  sound; only the engine's bit indexing was shifted) · M-013 (exFAT grading
  family) · N-067 (upcase-table validation, separate surface)

## Open questions

None — the spec is explicit and the corpus describes the mirror semantics.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-054 commit on top of `d487928`; corpus `03ad46c` unmoved)
**Verified by:** fail-before EXECUTED on the landed base: with the fixtures spec-correct and ONLY the `Bitmap::test` body reverted, `ExfatEngine.EndToEnd` red at `over->confidence()` (Excellent(1) vs Overwritten(3)) and `ExfatEngine.FreeMapProducerMatchesSpecBitMapping` red (shifted free/used answers); pass-after: full suite **574/574 Release + 574/574 ASan + 574/574 UBSan** (three lanes, executed this session on this tree).
**Resolution notes:** `Bitmap::test` now reads bit `c-2` for cluster `c` per MS exFAT spec §7.1 (bit 0 = cluster 2), with clusters 0/1 rejected as reserved-never-heap; both fixture builders write the spec-correct bit for each cluster (`setbit(c)` → bit `c-2`). The grading expectations (gone free→Excellent, over set→Overwritten) are UNCHANGED — they were always the intended behavior; only the fixture's bit placement and the engine's read offset were shifted, mutually canceling in the old suite. The corrected mapping flows into the parallel session's landed `storage::FreeMap` producer unchanged (its `!test(c)` inversion now inverts the RIGHT bit), so the S-009 disk-map free overlay and the grading path consume the same spec-correct truth; the new producer pin test guards the seam. The parallel landing's design decisions (free-set bits, absolute base, virtual-cluster convention, 8 MiB loader cap with conservative tail) are untouched and stand. No safety hardening affected; no ABI change.

**Post-landing correction (DFX16, this session):** the M-054 landing shipped
one stale pin — `FreeMapProducerMatchesSpecBitMapping`'s
`EXPECT_EQ(fm.cluster_count, 100u)` was written against the PRE-hardening
S-009 semantics; the landed producer (per 0ba21d8 defect 3, the FreeMap
contract in cr_freemap.h — `base_byte` = byte of cluster 0, coverage
`[base, base + count*cb)` — and the FAT family's pin of 5002 = count+2)
covers the two virtual clusters plus all heap clusters, i.e. 102 for this
fixture. The pushed tree was red at exactly this assertion (102 vs 100);
verified first-hand at `3edaa83`. The engine code is correct and untouched;
the test pin was corrected to 102u with the semantic comment
(DFX16 repair commit). Suite 591/591 green after the correction. The
Resolution's "574/574 executed green" claim above was recorded against a
pre-merge-order tree and did not hold on the pushed main — recorded here
for audit honesty.
