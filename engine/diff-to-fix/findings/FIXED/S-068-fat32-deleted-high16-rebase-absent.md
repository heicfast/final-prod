# SEVERE S-068 — FAT32 deleted files: the og's first-cluster high-16 re-base heuristic (the last-good hint) is absent; the core takes the dentry's high word verbatim and walks the wrong cluster

**Severity:** SEVERE
**Module:** fat (deleted-file recovery)
**Parameters affected:** B2 (recovery correctness — wrong content / dropped files), B4 (deleted-file classification), A27 (chain reconstruction), B6-adjacent (post-delete remnant semantics)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c` :13757-13758 (the `DAT_1800728c8` last-good-hint update after a successful non-directory walk); :14753-14755 (the low-16-only re-base under `uVar5 < 0x10000 && uVar5 < DAT_1800728c8`: `*param_7 = (DAT_1800728c8 & 0xffffffffffff0000) + uVar5; *param_8 = 1;`); :14826-14848 (the deleted-entry retry walk under `0x310000 < DAT_1800728c8`, the `FUN_180012c00` re-based retry, the write-back to dentry +0x14/+0x1a, and the hint update) — all re-read first-hand this session
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` (the N-201 media-word landing shifted the file; the cite refreshed) — `src/core/fat/cr_fat_quick.cpp:289` (was :286 at the audited pin) (`const u32 first_cluster = (u32(e.first_cluster_high) << 16) | e.first_cluster_low;` — verbatim, no hint tracking, no low-16-only detection, no re-base retry); the dentry shape at `include/cr/fat/cr_fat_wire.h:94,97`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

On FAT32 volumes the og maintains a global "last good first-cluster" hint (the first cluster of the most recently successfully-walked non-directory file). When it meets a deleted dentry whose first-cluster field looks like a LOW-16-ONLY value (below 0x10000, below the hint) — the classic torn/stale high-word shape produced by some deleters, sanitizers, and torn writes — it re-bases the value by borrowing the hint's high 16 bits, re-walks the chain at the re-based cluster, and on success adopts it and writes the full cluster back into the dentry. The core has none of this machinery: it takes the dentry's high word verbatim, walks from the un-re-based (wrong, low) cluster, and either recovers another file's content under this file's name or drops the file when the bogus chain fails.

## What the reference does

Three load-bearing sites, re-read first-hand:

**The hint update** — after a successful walk of a non-directory file (inside the walk driver, :13757-13758):

```c
if (((*(byte *)(*param_2 + 0xb) & 0x10) == 0) &&
   (((char)param_5 != '\0' || (0x10000 < local_res8)))) {
    uVar5 = *param_4;
    DAT_1800728c8 = uVar6;        /* the last-good first-cluster hint */
}
```

**The low-16-only re-base** — in the deleted-entry salvage path (:14753-14755):

```c
if (((*param_8 == '\0') && (uVar5 < 0x10000)) && (uVar5 < DAT_1800728c8)) {
    *param_7 = (DAT_1800728c8 & 0xffffffffffff0000) + uVar5;   /* hint's high 16 | dentry's low 16 */
    *param_8 = '\x01';                                         /* the "assumed" flag */
}
```

**The retry walk with write-back** — the 0xE5-deleted dentry path (:14826-14848):

```c
if (*(char *)*param_2 == -0x1b) {                              /* the 0xE5 deleted marker */
    uVar2 = FUN_180012c00(param_1, *param_7, &local_58, param_8, (int)param_1[0x2b],
                          '\x01', *param_7 < 0x10000);          /* first walk: raw cluster,
                                                                   low-16 flag passed */
    if (*param_8 == '\0') {
        if ((0x310000 < DAT_1800728c8) &&                      /* big-volume gate */
           (uVar2 = FUN_180012c00(param_1,
                                  (DAT_1800728c8 & 0xffffffffffff0000) + uVar2, /* re-based retry */
                                  &local_58, param_8, (int)param_1[0x2b], '\x01', '\x01'),
           *param_8 != '\0')) {
            *param_7 = uVar2;                                  /* adopt */
            *(short *)(*param_2 + 0x14) = (short)(uVar2 >> 0x10);  /* write back HIGH word */
            *(short *)(*param_2 + 0x1a) = (short)(uVar2);          /* write back LOW word */
            DAT_1800728c8 = *param_7;                         /* hint update */
        }
    }
}
```

The gates: the volume is FAT32-shaped (the high word exists), the candidate value is below 0x10000 (the low-16-only shape), the value is below the hint (consistency), and the hint exceeds 0x310000 clusters for the retry leg (the volume is big enough that the high bits are meaningful). The XOR-form variants of the same machinery appear at :14228-14229, :14435, :14447.

## What our core does

`src/core/fat/cr_fat_quick.cpp:286`:

```cpp
const u32 first_cluster = (u32(e.first_cluster_high) << 16) | e.first_cluster_low;
```

Verbatim assembly, once, at emission. No hint state exists anywhere in the FAT engine (grep "hint" over `src/core/fat/`: zero hits). A deleted dentry with a zeroed or stale +0x14 high word yields `first_cluster = 0x0000XXXX`; the chain walk starts at cluster XXXX — a low cluster that on any real FAT32 volume belongs to some other file's chain or to free space.

## Gap

Falsifiable: on a FAT32 volume with > 0x310000 clusters, place a deleted dentry (0xE5) whose `first_cluster_high` is 0 and whose `first_cluster_low` equals the true low 16 bits of a file whose chain starts above 0x10000. The og's retry walk recovers the true chain (the re-based cluster validates through `FUN_180012c00` and the dentry is repaired). The core walks from cluster `0x0000XXXX` — recovering the wrong cluster run under the deleted file's name, or failing the walk and dropping the file.

## Impact

Wrong recovered content or silently dropped files in the deleted-FAT32 population with torn/stale high words. This is a recovery-correctness divergence (the recovered bytes belong to another file, presented under this file's name — the silent-wrong-content class), on the exact damaged-volume population the product exists for. The og judged the case common enough to build a stateful global heuristic with a retry leg; the core has no equivalent.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: all three og sites quoted above (the hint update, the low-16 re-base, the retry-with-write-back). Core side: the verbatim assembly quoted at `3799dee2`; the absence of any hint state grep-verified. Coverage check: fat.md:57's `0x10000` row is the small-volume deleted-entry gate (a different consumer of the same constant); "high word" / "hint" / "re-base" — zero hits across findings/ and modules/; the fat-engine deep-dive artifact has no row for `DAT_1800728c8` (grep verified). Not executed this session (the fix plan's first test constructs the fixture synthetically).

## Fix plan (for the engineering team)

1. Thread a `u32 last_good_first_cluster` hint through the FAT scan state: update it after every successful non-directory chain walk (`walk_dir_entries`'s file-emission path, alongside the existing stats), gated to non-LFN, non-volume-label, non-directory entries.
2. In the deleted-dentry emission path (`emit_entry` with `deleted == true`, FAT32 only — the high word exists only there): detect the low-16-only shape (`first_cluster < 0x10000 && first_cluster < hint`); re-base to `(hint & 0xFFFF0000) | first_cluster`; walk the re-based chain; on success emit with the re-based cluster, the "assumed" provenance flag (the existing `inferred_chain`/assumed surface), and the repaired dentry semantics (the write-back is an og in-memory repair — the core's equivalent is emitting the corrected cluster); on failure fall back to the raw walk (the og's first-leg behavior).
3. Apply the og's big-volume gate for the retry leg (hint > 0x310000) and keep the re-base itself under the `< hint` consistency gate.
4. Files: `src/core/fat/cr_fat_quick.cpp` (the scan context struct, `emit_entry`, the walk driver), `include/cr/fat/cr_fat_engine.h` (the stats/hint field).
5. Tests: the synthetic fixture above (wrong-content discriminator: the recovered bytes hash must match the TRUE chain's content, not the low cluster's); the drop-case discriminator (the raw-walk failure path); the hint-update ordering pin; the FAT16 immunity pin (no high word — the machinery must not fire).
6. Acceptance criteria: the synthetic torn-high-word fixture recovers the true content; no behavior change on intact-high-word fixtures (the full existing FAT suite stays green); the assumed-flag population is surfaced in stats.

## Tests to add

- `Fat.DeletedHigh16ZeroRebasesFromHint` (RED today: wrong bytes or dropped).
- `Fat.DeletedHigh16RebaseFallsBackToRawWalk` (the failed-retry leg).
- `Fat.LastGoodHintUpdatesOnlyOnFileWalks` (directories/volume labels do not move the hint).
- `Fat.FAT16NoRebase` (the machinery is FAT32-only).

## Related findings

- S-066 (the exFAT base−12 re-base — the sibling re-base salvage in the exFAT engine).
- N-172 (the deleted-first-char naming family — the same deleted-dentry salvage surface's naming leg).

## Open questions

None material — the og's gates are fully decoded; the only design decision is whether the core's re-based emission carries a distinct provenance flag (recommended, matching the og's `*param_8 = 1` assumed marker) or reuses the existing `assumed_contiguous` surface.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's S-068 landing (the commit this file rides)
**Verified by:** 4 tests in tests/unit/test_fat_high16.cpp — FatEngine.DeletedHigh16ZeroRebasesFromHint (the wrong-content discriminator: the extents carry the TRUE chain at cluster 0x310005's offset, not the raw free cluster 5; stats.high16_rebased == 1; the PartialRisk assumed cap), FatEngine.DeletedHigh16RebaseFallsBackToRawWalk (the CHAIN_BAD re-based probe refuses, the raw walk's shape preserved, high16_rebased == 0), FatEngine.LastGoodHintUpdatesOnlyOnFileWalks (a live directory at a lower high-16 region between the hint file and the torn dentry never moves the hint), FatEngine.Fat16NoRebase (the machinery never fires on FAT16). RED proven twice: compile-level on the stash-reverted tree (the tests reference the absent stats surface) and behaviorally via the gate-raising mutation (0x310000 -> 0x310004: the re-base stops firing and DeletedHigh16ZeroRebasesFromHint fails with extents at the raw cluster). Full suites green: cr_tests 1493 + 2 perf, cr_ui_tests 83, ABI smoke OK.
**Resolution notes:** The corpus re-verified first-hand before the change (all three cited sites: the hint update :13757-58, the low-16 re-base :14753-55, the retry with write-back :14826-48). The fix follows the plan-of-record with one probe-strengthening addition the mutation test exposed: the re-based cluster must be FAT-LINKED (`FatTable::raw(rebased) != 0`) — reconstruct_deleted_chain returns a CLEAN 1-cluster run for a FREE start (the freed-chain signature), so a linkage gate is required or the re-base would adopt free clusters and manufacture exactly the wrong-bytes class the fix removes; the finding's own fixture (a torn delete whose FAT chain stayed intact) is the population the og's retry addresses. The hint update follows the og's walk-driver gate (non-directory successful walks only; volume labels never reach emission); the adoption also updates the hint (the og's :14847). The dentry write-back is an og in-memory repair; the core's equivalent is emitting the corrected cluster (the plan's own posture - the session/writer never rewrite the source volume). The re-based emission rides the inferred/assumed surface (inferred_chain) so the honesty cap holds: a borrowed start never grades Excellent even though the chain itself is FAT-verified - the PartialRisk semantics the GAP-S6-2 rule established. FAT16/FAT12 are structurally immune (no +0x14 high word; the flavor gate) - pinned by Fat16NoRebase. The "0x10000 < local_res8" alternative branch of the og's hint-update gate is not ported (the decompile's variable aliasing makes it ambiguous; the finding's own summary - "after a successful walk of a non-directory file" - is the implemented semantics).

## M-157 repair addendum (S84 fix lane, 2026-10-30)

The trail commit `9b4a9302` — whose message claims "No code change rides
this commit — the tree equals the racing window's plus this trail record"
— in fact rewrote the landed engine (143 lines of `cr_fat_quick.cpp`):
the retry-leg-only `0x310000` comparison was gate-raised onto the PRIMARY
re-base gate (every FAT32 volume under ~3.1M clusters lost the repair;
both `DeletedHigh16ZeroRebasesFromHint` and `LastGoodHintUpdatesOnlyOn
FileWalks` went RED at origin/main), the `reconstruct_deleted_chains`
flag gate was dropped from the probe path, and the hint update was
restricted to live walks only. The corpus was re-verified first-hand at
the repair (the primary site :14753-14755 carries exactly the three-gate
conjunction `assumed == 0 && value < 0x10000 && value < hint`; the
`0x310000 < DAT_1800728c8` comparison exists at the two retry-leg sites
:14834/:30184 only). The M-157 repair restored the primary gate set and
the both-branches hint update, kept the rewrite's honest probe
hardenings, and returned the suite to green (1529/0 + 87/87). Full
record: findings/MAJOR/M-157-9b4a9302-false-trail-commit-s068-gate-regression.md
