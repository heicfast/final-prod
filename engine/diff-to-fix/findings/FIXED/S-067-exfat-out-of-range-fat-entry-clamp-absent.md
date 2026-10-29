# SEVERE S-067 — exFAT chain walker: the og's out-of-range FAT-entry clamp (entry ≥ cluster_count → EOC) is absent; garbage entries are followed as raw cluster numbers, attaching wild out-of-volume extents and flagging the chain broken

**Severity:** SEVERE
**Module:** exfat (chain reconstruction)
**Parameters affected:** B2 (recovery correctness — wrong extents), B3 (false-positive/downgrade on damaged volumes), A27 (fragment/chain handling), X60-adjacent (bad-sector/corrupt-FAT handling)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c` :41315-41328 (the walk loop: `uVar30 < 0xfffffff6` bound, the `FUN_180021c60` call, and the clamp `if (*(uint *)(param_2 + 0x18) <= uVar15) uVar15 = 0xffffffff;`); FUN_180021c60 :28039-28166 (the entry lookup: the cached-window read, the FREE-continuation `iVar4 == 0 → *param_3 = 1; return param_2 + 1` at LAB_180021e5d) — both re-read first-hand this session
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` (the S71-S76 fix windows absorbed; the line cites refreshed) — `src/core/exfat/cr_exfat_quick.cpp:81` (`next()`'s raw return; was :71-82 at the audited pin) (`ExfatFat::next()`: FREE/BAD → 0, `>= EXFAT_EOC_MIN` → EOC, **no cluster-count clamp**); `:92-135` (`follow_exfat_chain`: the raw value pushed as a member at :110, the out-of-FAT-bounds `entry()` → FREE at :58/:67-68, the bounds probe at :123-126 setting `broken = true`); `include/cr/exfat/cr_exfat_wire.h:30` (cluster_count, the og's +0x18 analog)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's exFAT chain walker clamps every FAT entry to the volume's cluster count: a next-cluster value at or above `cluster_count` is mapped to EOC and the chain ends cleanly at the previous cluster. The core's walker has no such clamp — a garbage or stale entry in `[cluster_count, 0xFFFFFFF7]` is followed as a raw cluster number: the bogus cluster is pushed as a chain member, the FAT lookup for it reads out of bounds and returns FREE, and the walk marks the chain broken. On damaged volumes (the recovery tool's core population) the og recovers a clean partial file at normal confidence while the core attaches a wild out-of-volume extent and downgrades the result.

## What the reference does

The walk loop (the caller of the entry lookup), re-read first-hand at :41315-41328:

```c
while (puVar20 = local_4e0, uVar30 < 0xfffffff6) {
    ... /* compute the cluster's data offset; push the cluster to the run list */
    uVar15 = FUN_180021c60(param_2 + 0x12, uVar30, (undefined1 *)ppppplVar31);
    if (*(uint *)(param_2 + 0x18) <= uVar15) {
        uVar15 = 0xffffffff;                    /* entry >= cluster_count -> EOC */
    }
    if (((uVar30 == uVar15) || (local_568[0] != '\0')) ||
        (uVar30 = uVar15, uVar15 < 2)) break;   /* self-loop / flag / invalid */
}
```

`param_2 + 0x18` is the volume's cluster count (capped ≤ 0xFFFFFFF5 at boot validation, N-198's registered bound); `param_2 + 0x12` is the FAT context. `FUN_180021c60` (:28039-28166) returns the FAT entry for the cluster, applying the FREE-continuation itself (`iVar4 == 0 → *param_3 = 1; iVar4 = param_2 + 1` — the core's EXFAT-2 port cites exactly this and is accurate). The caller's clamp then handles every non-FREE value at or above the cluster count — which includes the whole reserved family (0xFFFFFFF6, BAD 0xFFFFFFF7, EOC ≥ 0xFFFFFFF8 — all ≥ any legal cluster_count ≤ 0xFFFFFFF5) AND every out-of-range garbage value. The loop bound `uVar30 < 0xfffffff6` is belt-and-braces on the current-cluster side. The chain ends cleanly: the clusters walked so far stand, no member is added for the clamped value, no failure flag is set.

## What our core does

`src/core/exfat/cr_exfat_quick.cpp:71-82`:

```cpp
u32 next(u32 c) const {
    u32 v = entry(c);
    if (v == EXFAT_FREE || v == EXFAT_BAD)
        return 0;
    /* EXFAT-3 (AUDIT-FAT): the spec EOC family is 0xFFFFFFF8..0xFFFFFFFF ... */
    if (v >= EXFAT_EOC_MIN)
        return EXFAT_EOC;
    return v;                                   /* <- raw: no cluster-count clamp */
}
```

`follow_exfat_chain` (:102-145 at the re-verified pin) then does, for the returned raw value:

```cpp
while (c >= 2) {
    ...
    r.clusters.push_back(c);                    /* the garbage cluster becomes a member */
    ...
    u32 n = fat.next(c);
    if (n == 0) {
        if (cluster_count != 0 && c + 1 >= cluster_count + 2) {
            r.broken = true;                    /* the downgrade */
            break;
        }
        ...
```

The out-of-volume cluster `c` fails the FAT-bounds read inside `entry()` (:58/:67-68 return EXFAT_FREE for `off + 4 > total_bytes`), `next()` maps that to 0, and the `c + 1 >= cluster_count + 2` probe flags the chain broken. The chain's extent list now contains a member whose cluster offset (`cluster_offset()` at :148-150) lies far beyond the volume — the extent assembly either fails or produces a wild extent — and the `broken` flag downgrades the node's confidence in the consumers.

## Gap

Falsifiable: on an exFAT volume whose FAT contains an entry with value in `[cluster_count, 0xFFFFFFF7]` at the chain position of a real file (a stale/garbage entry — post-format remnants, torn writes), the og's walk ends the chain cleanly at the previous cluster (partial file, full confidence, no wild member); the core's walk pushes the garbage cluster as a member and marks the chain broken. Synthetic fixture: FAT entry = cluster_count + 0x1000 at the mid-chain position of a 3-cluster file.

## Impact

Recovery correctness and grading on the damaged-volume population: recovered extents contain a wild member (wrong bytes or a failed extent) and the result is downgraded (`broken`) where the og's stays clean. This is the same population class the EXFAT-2 (FREE-continuation) and EXFAT-3 (EOC-family) fixes targeted — the third leg of the og's damaged-FAT chain semantics.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: both og sites quoted above (the walk loop with the clamp; the entry lookup with the FREE-continuation); the core sites quoted at `3799dee2`. Coverage check: the s12 detection sheet's EOC-family row and the in-tree EXFAT-3 comment cover `≥0xFFFFFFF8 / ==0xFFFFFFF7 / ==0` only; no trail row covers the `≥ cluster_count` clamp (grep "0xfffffff6" over findings/modules: zero hits). Not executed this session (the fix plan's first test is the synthetic fixture above).

## Fix plan (for the engineering team)

1. In `ExfatFat`, carry `cluster_count` (already available at the `follow_exfat_chain` call sites — it is passed as the `cluster_count` parameter today) into the struct (or extend `next()` to take it), and clamp: a returned entry `v >= cluster_count` → `EXFAT_EOC`. Keep the existing FREE/BAD and EOC-family mappings (they are subsets of the clamp once it is in place; the explicit family checks can stay for clarity).
2. Add the loop-side belt-and-braces: `follow_exfat_chain`'s `while (c >= 2)` gains `&& c < 0xFFFFFFF6` (the og's loop bound) — with the clamp in `next()` this is redundant, but it pins the og's shape.
3. Files: `src/core/exfat/cr_exfat_quick.cpp` (the `ExfatFat` struct + `next()`; the struct is constructed where the FAT window is set up).
4. Tests: the synthetic fixture above — a mid-chain entry at `cluster_count + 0x1000` (clean end, no broken flag, member count = the valid prefix), an entry exactly at `cluster_count` (boundary: clamped, clean end), an entry at `0xFFFFFFF6` (clean end — the og's loop-bound case), plus re-pins of the EXFAT-2/EXFAT-3 tests to prove no regression.
5. Acceptance criteria: no chain in the suite ever contains a member `≥ cluster_count + 2`; the EXFAT-2/EXFAT-3 families stay green; the broken-flag rate on the damaged-FAT fixtures drops to zero for this class.

## Tests to add

- `Exfat.ChainEndsCleanOnOutOfRangeEntry` (mid-chain `cluster_count + 0x1000`: members = valid prefix, `broken == false`, `assumed_contiguous == false`) — RED today.
- `Exfat.ChainClampsAtExactClusterCount` (boundary value `cluster_count`).
- `Exfat.ChainEndsOnReservedFFF6` (0xFFFFFFF6: clean end, no wild member) — RED today.
- `Exfat.Existing2And3FamiliesStayGreen` (the re-pin guard).

## Related findings

- The EXFAT-2/EXFAT-3 audit-file fixes (the FREE-continuation and EOC-family legs of the same damaged-FAT semantics — in-tree, cited at `cr_exfat_quick.cpp:75-78` and `:115-122`).
- S-066 (the exFAT base−12 lost-anchor re-base — the same engine's salvage family).

## Open questions

None — both og sites and both core sites were read directly; the only engineering decision is where to thread `cluster_count` into `next()`.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S80 parity fix lane, 2026-10-29)
**Verified by:** RED first-hand via the stash dance (the source fix stashed, rebuilt: `ExfatEngine.OutOfRangeFatEntryEndsChainCleanly` / `ExfatEngine.ExactClusterCountEntryClampsToEoc` / `ExfatEngine.ReservedFFF6EntryEndsChainCleanly` — 3/3 FAILED pre-fix; restored, rebuilt: 3/3 PASSED). Regression guards: the full exFAT suite 44/44 green (the one skip is the pre-existing mkfs.exfat tool pin), the EXFAT-2 family (`FreeEntryContiguousSalvage`) and the EXFAT-3 family (`EocFamilyTerminatesChain`) re-pinned green post-fix. The finding's own fixture note was corrected in flight: a 1-cluster plant never reaches the mid-chain entry (the walk's expect-bound caps members at ceil(data_len/cluster_bytes)), so the tests use the finding's specified 3-cluster shape with the garbage at the MID-CHAIN position (cluster 91's entry), where the wild member is reachable.
**Resolution notes:** `ExfatFat` now carries `cluster_count` (set from `p.cluster_count` in `scan_exfat`'s setup — the og's volume-context +0x18, itself a verbatim copy of the boot's +0x5C field, RAWSearchDLL:36750 `*(undefined4 *)(puVar14 + 0x18) = *(undefined4 *)(param_3 + 0x5c)`, re-verified first-hand this session); `next()` clamps every entry `v >= cluster_count` to `EXFAT_EOC` after the existing FREE/BAD and EOC-family mappings (the family checks stay for clarity — they are subsets of the clamp once the count is known; `cluster_count == 0` means unknown and disables the clamp — the boot gates guarantee nonzero on every validated volume). `follow_exfat_chain`'s loop gains the og's belt-and-braces bound `c < 0xFFFFFFF6` (exFATSearchDLL:41315 `while (uVar30 < 0xfffffff6)`), redundant with the clamp but pinning the og's shape. Corpus evidence re-verified first-hand before the fix: the clamp at :41348-41351 (`if (*(uint *)(param_2 + 0x18) <= uVar15) uVar15 = 0xffffffff;`), the loop bound at :41315, and FUN_180021c60's FREE-continuation at LAB_180021e5d (:28158-28163). The safety-hardening posture is unchanged: no hardening surface sits near this walk.
