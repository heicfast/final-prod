# SEVERE S-015 — raw_signs min/max are KiB in the og, bytes in the core: raw-sign files above 50 KiB are never carved

**Severity:** SEVERE
**Module:** carve (raw-sign layer)
**Parameters affected:** A28 (carve validator logic), B1/B2 (detection/recovery correctness), B3 (false-positive rate), X47/X48 (throughput/latency of the raw-sign pass), D15 (confidence/size fields of raw-sign nodes)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :23765-24190 (loader FUN_18001ea40: `"min"`/`"max"` keys at DAT_18006b048/`0x18006b04c`, pair `{min@+0, max@+8}` via FUN_180022590), :25020-25260 (probe FUN_1800205b0: `lVar19 = *(longlong*)(fmt + 8)`; `if (lVar19 == 0) lVar17 = 0x7d000`; byte-span `< lVar17 * 0x400`; `lVar19 = lVar19 << 10`; `*(param_1+0x10) = lVar19`), :24533-24612 (lazy accessor FUN_18001fae0 + init thunk at VA 0x180022b90). Full quotes: `diff-to-fix/evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §2.
**Core references:** audited commit `34b3441` — `scripts/gen_rawsigns.py` :8 (`"min": <min bytes>, "max": <max bytes>`), :41 (`int(e.get("min", 0)), int(e.get("max", 0))` raw); `src/core/carve/cr_rawsigns_data.cpp` `kFormats[]` (`{"a", 1ull, 51200ull}` — bytes); `src/core/carve/cr_carve.cpp` :207 (`u64 len = fmt.max_size;`), :211-228 (tail window `[start+min_size, start+max_size)` bytes), :232-233 (`if (len < fmt.min_size) continue;`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's RAWSearchDLL interprets the `min`/`max` fields of `raw_signs.json` as **KiB counts**, multiplying by 1024 at every consumption site (emitted size = `max << 10`; span gate = `max * 0x400`; default when `max==0` = 0x7d000 KiB = 500 MiB). The core's generated table and carving pass treat the same numbers as **plain bytes**. Every raw-sign format's size window is therefore 1024× too small in the core: the common `max=51200` entries carve at most **50 KiB** where the og carves **50 MiB**, and the `max=204800` media entries (vob, ifo, cpi, cue, lrv, moi, moff, m3u, wpl, prproj) cap at 200 KiB where the og allows 200 MiB.

## What the reference does

The loader stores the JSON `min`/`max` per extension into a map (values accepted as int or truncated double). The `CommonRawAnalyser::CommonRawFileInfo` probe — the class that owns the raw_signs layer, reached when the u32/u16/linear analyser dispatch all miss — reads the matched format's `max` and:

```c
lVar19 = *(longlong *)(*(longlong *)(param_1 + 0x118) + 8);   /* fmt->max */
lVar17 = lVar19;
if (lVar19 == 0) {
    lVar17 = 0x7d000;              /* default max = 512000 KiB = 500 MiB */
}
if ((byte_span_of_scanned_sectors) < (ulonglong)(lVar17 * 0x400)) {
    return 2;                        /* need more data before accepting */
}
lVar19 = lVar19 << 10;              /* KiB -> bytes */
*(longlong *)(param_1 + 0x10) = lVar19;   /* candidate size */
```

The comparison multiplies the JSON value by 0x400 (1024) to get bytes, and the emitted candidate size shifts it left by 10. Both device classes (sector-shift < 8 and >= 8) take the same scaled comparison. With the corpus table's values this means: `max=51200` → 52,428,800 bytes; `max=204800` → 209,715,200 bytes; `min=1` → a 1 KiB floor; `min=10240` → 10 MiB. The KiB reading is also the only one that makes the table self-consistent: a VOB (DVD video object) with a 200 **KiB** maximum would be useless; 200 **MiB** is the sensible media bound.

## What our core does

`scripts/gen_rawsigns.py` documents its assumption in the header comment — `"min": <min bytes>, "max": <max bytes>` — and passes the JSON integers through unscaled into `kFormats[]` (`cr_rawsigns_data.cpp`, e.g. `{"a", 1ull, 51200ull}`). The carving pass then:

- sets `len = fmt.max_size` for header-only hits — **50 KiB**, clamped only by the device end (`cr_carve.cpp:207,230-231`);
- searches tail signs only inside `[start+min_size, start+max_size)` **bytes** with a hard 2 MiB window bound (`:211-215`);
- rejects candidates shorter than `fmt.min_size` **bytes** (`:232-233`) — a 1-byte floor where the og requires 1 KiB.

## Gap

For every one of the 139 raw-sign formats, the core's acceptable carved-size window is 1024× narrower than the og's, and its minimum-size floor is 1024× lower. Falsifiable claim: for any fixture where a raw-sign-format file (no grammar validator — e.g. a 5 MiB `.vob`, a 300 KiB `.flac`, a 2 MiB `.sqlite`) lies in a region with no filesystem metadata, the og recovers it and the core's raw-sign layer does not.

## Impact

Recovery correctness (recall) on the raw-sign safety net — the layer whose entire purpose is formats without structured metadata. Every raw-sign file between 50 KiB and 50 MiB (and up to 200 MiB for the media entries) is silently absent from our results on metadata-free regions. The 1-byte minimum floor also admits sub-KiB junk the og would reject (false-positive surface, parameter B3). The tail-sign pairing window is likewise 1024× too narrow, so even tail-confirmed candidates only pair inside the first 50 KiB. Users recovering media-heavy volumes (the product's core scenario for deep scan) see the largest gap.

## Reproduction / verification

Corpus-derived (decompiled evidence, first-hand this session):
- Loader min/max key strings dumped from the binary (`"max"`@0x18006b048, `"min"`@0x18006b04c); pair insertion at :24100-24116.
- Consumer scaling quoted at :25200-25240 (`0x7d000` default, `* 0x400` gate, `<< 10` size) — `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §2.
- Core side read at commit `34b3441` (cites above).
- Executed comparison: NOT RUN (no Windows/wine here; the og's raw-sign emission needs a running product). The bench harness's `full_format`/`quick_format` fixtures with seeded `.vob`/`.flac`/`.sqlite` payloads in unallocated space are the empirical vehicle — registered as the acceptance test below.

## Fix plan (for the engineering team)

1. **Units**: change `scripts/gen_rawsigns.py` to emit `min_kib * 1024` / `max_kib * 1024` (keep the KiB comment accurate), or scale at load — but keep ONE conversion point. Regenerate `cr_rawsigns_data.cpp`. 139 formats; `kMaxSignBytes` is unaffected (sign bodies don't change).
2. **Default max**: when a format's `max` is 0/absent, the og uses 512000 KiB (500 MiB). If our table has no zero-max entries this is dormant — assert it in the generator (fail if `max == 0` appears) rather than shipping a silent divergence.
3. **Tail window**: revisit the 2 MiB tail-search bound (`cr_carve.cpp:215`) — with KiB-scaled windows the og's tail search can span up to ~150 MiB for `min=10240,max=204800` formats. A bounded forward-read tail scan at, say, 8 MiB with a documented divergence note is acceptable if full-range scanning is too costly; but make the bound a named constant with the og's true range documented next to it.
4. **Minimum floor**: `len < fmt.min_size` now compares against KiB-scaled floors (1 KiB typical) — verify the emitted `logical_size` for header-only hits equals the og's `max << 10` clamped to device end (our current clamp logic already does this shape).
5. **Tests to add** (fail-before/pass-after):
   - `RawSigns.UnitsAreKib` — table assert: `kFormats[Vob].max_size == 204800 * 1024`.
   - `RawSigns.CarvesFiveMiBVob` — fixture: 5 MiB `DVDV`-led `.vob` payload at an unaligned unallocated offset; expect exactly one carved node with the right extent (fails today: no hit, since 5 MiB > 50 KiB window).
   - `RawSigns.MinFloorOneKib` — 400-byte `.flac`-signed blob: expect NO candidate (og rejects < 1 KiB); today the core can emit it.
   - `RawSigns.TailPairWindowScaled` — `.flac` (min 1024 KiB) with a valid tail sign at +2 MiB: expect tail-paired candidate with the blended confidence path taken.
6. **Docs**: update `cr_rawsigns.h`'s "documented divergences" block — the min/max line moves from assumed-bytes to decoded-KiB (cite this finding).

## Tests to add

See fix plan §5 (four named tests). Bench-level: one `full_format` scenario seeded with raw-sign-format payloads sized 100 KiB / 5 MiB / 150 MiB, compared against og recall on the same image.

## Related findings

S-007 (carve grammar formats missing — orthogonal layer), M-005 (carve bitmap gating), N-030 (confidence field dead in og — same decode session), N-031 (dispatch/stride deltas), OQ-T8 (RESOLVED by the same decode).

## Open questions

The exact og behavior for `min` at the tail-walk (whether `min` gates the tail search start or only final acceptance) was not fully decoded; the KiB unit is certain for `max` (three independent sites) and near-certain for `min` (same stored pair). If a divergence matters after the unit fix, probe `FUN_1800205b0`'s tail walk at :25240+.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit; see `git log` for the S-015 fix on main)
**Verified by:** fail-before/pass-after — pre-fix tree (worktree stashed to pre-S-015 source): `RawSigns.UnitsAreKib`, `RawSigns.CarvesFiveMiBVob`, `RawSigns.TailPairWindowScaled`, `RawSigns.MinFloorOneKib`, `CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic`, `CarveEngine.RawSignTailPairWithinFormat` ALL FAILED (5/6 directly: byte-unit table, 200 KiB stub extents, no far-tail pairing, no floor, cross-format pairing artifact); post-fix all PASS. Re-based `CarveEngine.GrammarValidationAndSpecificity` (jfif noise plant: 256 KiB fixture for the true 100 KiB floor, extent now max-clamped not byte-stubbed) + `CarveEngine.SecondPassFullyDeduped` (sign-layer-live invariant holds on the enlarged fixture). Full suite 310/310 Linux ASan.
**Resolution notes:** Units fixed at the single conversion point: `scripts/gen_rawsigns.py` now documents the corpus min/max as KiB (per FUN_1800205b0's `*0x400` gate / `<<10` size / `min<<10` acceptance / `0x7d000` KiB default) and emits `min*1024`/`max*1024` bytes; the generator REFUSES zero-max entries (og's 500 MiB default rule would be a silent divergence — asserted instead); regenerated `cr_rawsigns_data.cpp` (139 formats; all windows now KiB-scaled: vob 10 MiB..200 MiB, flac 1 MiB..50 MiB, sample 1 KiB..50 MiB). Tail walk rebuilt: the old single-read `hi-lo <= 2 MiB` gate (which under true units would have disabled ALL pairing — max−min reaches ~190 MiB) is replaced by a chunked forward scan (1 MiB reads + `kMaxSignBytes` overlap so straddling signs stay contiguous; unreadable chunk aborts to header-only, never fabricating a tail), bounded by the NAMED constant `kRawSignTailScanLimit` (8 MiB) with the og's true full-window range documented beside it in `cr_rawsigns.h` (documented divergence per the finding's own fix plan §3). Min floor and emitted size now match the og shape by construction (header-only extent = max<<10 device-clamped; acceptance gate `len >= min<<10`). **Discovered divergence fixed in the same commit (same layer, exposed by the 1024x larger windows):** `scan_tail` matched ALL 62 tail signs across formats while the og pairs tails within the CANDIDATE FORMAT'S OWN record (FUN_1800205b0's tail loop walks the format record's list) — cross-format pairing fabricated close positions (proven: a .sample candidate was closed by flp's tail, size 3150336; the session-1 fixture even relied on the artifact - a .fst node "tail-paired" via flp's tail). New `scan_tail_for_format` filters to the candidate's format; the unfiltered `scan_tail` remains for diagnostics; `CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic` pins the honest semantics (fst carries no tail signs -> header-only even with flp's tail in range) and `RawSignTailPairWithinFormat` + `RawSigns.TailPairWindowScaled` pin in-format pairing at ~2 MiB distance. `cr_rawsigns.h`'s divergence block rewritten (units decoded; tail-walk bound; per-format scoping); `docs/parity/PARITY-CORE-REC.md` F-A3-14 marked RESOLVED. Bench-level og-recall comparison (100 KiB/5 MiB/150 MiB seeded payloads) remains the acceptance vehicle as the finding records — registered in the finding's own Tests-to-add section for the bench harness (no Windows host in this session; same limitation class as M-009's Resolution). Win7-inclusive gate unaffected (table data + logic, no new imports).
