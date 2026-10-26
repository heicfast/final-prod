# MAJOR M-056 — TIFF-family carved sizes are inflated by the candidate offset: tiff_common's out-of-line-entry end is absolute but consumed as relative (every real multi-strip TIFF carves at the 256 MiB cap on real disks)

**Severity:** MAJOR (NEW-FINDING, discovered during the N-070 verification — the new OverlapScore fixture's size assertion exposed it) · **Module:** carve (TIFF family validator) · **Parameters affected:** A28 (validator logic — end-of-file computation), B1-adjacent (recovered extent correctness), A29 (merge/dedup — the inflated extents shift len_class buckets)
**Corpus references:** `re-workspace/artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md:25-40` (the og's analysers COMPUTE end-of-file by walking structure — the size the og reports is the structure's own end, not offset-shifted); `rawsearchdll--analyser-registry-census.md:53` (the TIFF/camera-RAW family the defect lands on)
**Core references:** `src/core/carve/cr_analysers.cpp` `tiff_common()` — the `count > 4` branch computed `data_at = hit + u64(val)` (ABSOLUTE device coordinates) while `end_data`'s consumer does `r.end = hit + (end_data > 512 ? end_data : 512)` (end_data expected RELATIVE) — the candidate offset was added to the file's own size. The `count <= 4` inline branch was already relative (`max_off = val + count`). Audited at `a333511` (session-22 tree); the defect predates the session (present since the TIFF family's introduction)
**Status:** NEW-FINDING — filed and FIXED in the same session (fail-before: both new N-070 tests RED on the size assertions with the fix stashed)

## Summary

`tiff_common` walks the IFD chain and tracks the furthest referenced data
offset to size the carved file. For out-of-line entries (count > 4 — the
normal shape of every real multi-strip TIFF: StripOffsets/StripByteCounts
arrays) it computed the data position in absolute device coordinates
(`hit + val`), but the final `r.end = hit + end_data` re-based `end_data`
as if it were relative. The carved size became `hit + real_size` — the
candidate's own disk offset added to its own size.

## What the reference does

The og's analysers compute end-of-file by walking the structure — the
reported size is the structure's own extent (the census/granular artifacts'
model throughout: "format-specific parsers that COMPUTE end-of-file by
walking structure", searchdll-family--architecture.md:73-adjacent §4.3).
No offset-shifting exists in the reference model.

## What our core did (pre-fix)

`file_len = hit + real_end` for every TIFF-family candidate (TIFF, TIFF_BE,
CR2, DNG, ORF variants, RW2 — all route through tiff_common) whose IFD has
out-of-line entries. On real disks (candidate offsets in the GiB range) the
computed size exceeds the registry's 256 MiB cap and the clamp lands it at
exactly the cap: **every multi-strip TIFF carved at 256 MiB**, absorbing
neighboring files' data into the output. On small fixtures the size was
merely wrong (hit + real), and no prior test pinned TIFF-family sizes — the
BeTiffAndOrfVariantsCarve fixture exercises the same count>4 path but only
asserts formats, which is how the defect survived.

## Gap

Falsifiable and falsified in-session: with the fix stashed,
`CarveEngine.OverlapScorePrefersHigherConfidence` reports
`logical_size 1049784` (hit 1048576 + 1208) where the structure's end is
1208, and `CarveEngine.TiffVsRawTiebreak` reports hit + 1210 where the
structure's end is 1210. Both green post-fix.

## Impact

Wrong recovered-file sizes for the whole TIFF family (one of the most
common carved formats) on every real volume: multi-strip TIFFs absorb
up to (cap − real_size) bytes of neighboring data; the extent also feeds
the F9 dedup len_class, shifting dedup buckets. No crash; wrong output
bytes (bloat) rather than truncation. MAJOR.

## Reproduction / verification

Executed (fail-before on the stash-isolated pre-fix tree, pass-after):

```
$ git stash push src/core/carve/cr_analysers.cpp
$ ctest -R "OverlapScore|TiffVsRaw"
  CarveEngine.OverlapScorePrefersHigherConfidence ... Failed
    logical_size: 1049784 vs 1208      (hit + 1208)
  CarveEngine.TiffVsRawTiebreak .................... Failed
    logical_size: 1049786 vs 1210      (hit + 1210)
$ git stash pop && ctest -R "Carve"
  100% tests passed out of 58
```

## Fix plan (executed this session)

1. `data_at = u64(val)` — the out-of-line data offset is relative to the
   TIFF base (as the inline branch already assumed); drop the `hit +`.
2. Pin the sizes: the two new tests assert exact logical_size values
   (1208 / 1210) so the basis can never silently re-fuse.

## Tests to add

- `CarveEngine.OverlapScorePrefersHigherConfidence` (size assertion = the
  M-056 fail-before)
- `CarveEngine.TiffVsRawTiebreak` (size assertion pins the out-of-line
  end at 1210 relative)

## Related findings

- N-070 (the overlap-scoring fix whose test exposed this)
- N-051..N-054 quartet (the same TIFF-family registry surface)

## Open questions

None — the relative-basis reading is the only self-consistent one with the
inline branch and the `r.end = hit + end_data` consumer.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-070/M-056 carve batch (one-line basis fix in tiff_common + the two size pins)
**Verified by:** fail-before executed on the stash-isolated pre-fix tree (both tests RED with the hit-inflated sizes quoted above); pass-after full carve suite 58/58 and full linux-release suite green (see the N-070 commit for the suite count).
**Resolution notes:** The `count > 4` branch's `data_at` now uses the offset relative to the TIFF base, matching the inline branch's basis and the `r.end = hit + end_data` consumer. Real-volume effect: multi-strip TIFFs (StripOffsets/StripByteCounts arrays are the normal shape) now carve at their structural end instead of the 256 MiB cap; single-strip files (inline values) were already correct. Discovered during the N-070 fixture design — the OverlapScore test's size assertion is what exposed the basis mix; both new tests pin exact sizes so the class cannot silently return. No behavior change for any accepting candidate other than the size basis.
