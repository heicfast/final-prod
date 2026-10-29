# MAJOR M-146 — the deep-read grade's read address is the extent START; the og reads at the twin key (= the extent END + the shifted index term) — discovered during M-134 convergence verification

**Severity:** MAJOR (renumbered from the M-146 draft at the push window — the remote S54 lane's NTFS 55AA finding took the seat per earlier-push-wins)
**Module:** grading (the M-134 landing's address semantics)
**Parameters affected:** #15 Confidence grading model (the content-read grade flips on the wrong sector); #9 detection correctness (the grade column)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:322640-322671` (the decisive consumer: `*param_4 = local_18[0]` = the extent START, `*param_5 = local_20` = the LENGTH, `*param_3 = local_18[0] + local_20 + local_28 * *param_6` = START + LENGTH + shifted-index·unit — the twin key, with the same out-parameter triple the completion pass computes at :605757-774); `:605774` (`local_88 = local_80 + local_98 + local_a8 * local_b8[0]` — the twin key passed AS the deep read's addr at :605798); `:606364-411` (FUN_14038c470's reads at addr / addr−1)
**Core references:** `src/core/tree/cr_grading.cpp` `deep_read_grade` (the M-134 landing 9455f50a): `const u64 addr = pr->extents.front().offset / ss` — the START, no index term; at the current tree post the S-059 window
**Status:** READ-ONLY FINDING — filed by the session-52 lane after the convergence verification

## Summary

The M-134 landing's deep-read grade computes its read address from the extent's START sector.
The og passes the node's TWIN KEY as the read address — the extent END (start + length) plus the
shifted device-index term. The landed code therefore reads the sector BEFORE the file where the
og reads the file's LAST sector, applies the zeroed-head demote to files that start at sector 0
where the og applies it to the extentless class, and never reproduces the og's failed-read class
for the M-133 shifted kinds (a start-based address is always in range, so reads succeed where the
og's out-of-range keys fail and leave the grade unchanged).

## What the reference does (the decisive decode)

The key-computing helper at `:322640-671` (one of FUN_1401bb580's consumers, the same
out-parameter family the completion pass uses):

```c
322653      cVar1 = FUN_1401bb580(param_2,local_18,param_6,&local_20), ...
322666      *param_3 = local_18[0] + local_20 + local_28 * *param_6;   /* START + LENGTH + idx*unit */
322667      *param_5 = local_20;                                        /* the LENGTH */
322668      *param_4 = local_18[0];                                     /* the START */
```

FUN_1401bb580's `param_2` out-slot (`local_18[0]`) is the extent START and its `param_4`
out-slot (`local_20`) is the extent LENGTH — the twin key is their SUM plus the index term.
The completion pass (`:605757-774`) computes exactly this value into `local_88` and passes it to
`FUN_14038c470(param_2, local_88, 0, 0)` at `:605798`. Inside the deep read:

- `addr == 0` ⇔ the extent END is zero (the extentless/zero-length class) AND the index term is
  zero — read the reader's sector 0 (first-32-zero or first-4-0xFF → grade 0);
- `addr > 0` with a zero index term — read the sector at `addr − 1` = **the extent's LAST
  sector** (end-exclusive minus one); its final 16 bytes all-zero → grade 2;
- a NONZERO index term (the M-133 shifted kinds — FAT16 on any ordinal but 2; every non-zero
  ordinal) puts the key outside any reader: **the read FAILS and the grade never changes**.

## What our core does

`deep_read_grade` (cr_grading.cpp, the 9455f50a landing): `addr = extents.front().offset / ss`
(the START sector). Consequences, each falsifiable:

1. The tail-zero High rule reads the sector BEFORE the file, not the file's last sector — a
   file whose PREVIOUS sector ends in 16 zeros grades High; the og reads the file's own final
   sector (a file following a zero-padded region but ending in nonzero data grades High in the
   core, unchanged in the og).
2. The zeroed-head demote fires for files that START at sector 0 (the og: only the extentless
   class reads sector 0); a real file starting at sector 0 with a zeroed head demotes in the
   core, not in the og.
3. The extentless class (end == 0) returns early in the core (`extents.empty()` bail) — never
   reads sector 0; the og reads it.
4. The shifted kinds' failed-read class is absent: a start-based address is always in range,
   so FAT16 nodes get content-read grades the og never gives them (their keys carry −2·unit).

## Impact

Wrong grades on exactly the population the deep read governs (the not-in-table extensions after
S-058): spurious Highs from neighboring-sector padding, spurious Lows for sector-0 files, and
content grades for the shifted kinds the og structurally cannot produce. MAJOR — the same
surface M-134 itself governs; filed against the landing rather than as a reopen because the
machinery (reader, thresholds, one-shot) is correct and only the address derivation is wrong.

## Reproduction / verification

- Corpus-derived, first-hand this session: the `:322640-671` consumer's out-parameter decode
  (quoted above — the START/LENGTH/KEY triple is unambiguous), the `:605757-774` completion-pass
  arithmetic, and the `:605798` hand-off. Re-verified against the M-133 finding's own twin-key
  decode ("og: base + len + device_index * unit") — two independent reads agree.
- Core-derived: the landed `addr = offset / ss` (quoted); the four falsifiable consequences
  enumerated above.

## Fix plan

1. `deep_read_grade`: compute `end = offset + length` of the FIRST extent (0 when extentless);
   apply the M-133 index term (`index = ordinal − (Fat16 ? 2 : 0)`); skip the read when
   `index != 0` (the og's failed-read class); `end == 0` → read sector 0 (the extentless
   class); else read the sector CONTAINING byte `end − 1` (the core's extents are absolute
   bytes; the og's key is sector-indexed — the containing sector is the byte-exact equivalent).
2. Tests: the four consequences above (previous-sector-padding NOT High; the file's own
   zero-padded final sector High; sector-0-start files NOT demoted on their head; the extentless
   class reading sector 0; the shifted kind skipped).
3. Re-pin any M-134 test that encoded the start-based address.

## Tests to add

- `Grading.DeepReadTailZeroOfOwnLastSectorGradesHigh` (the og's true rule)
- `Grading.DeepReadPrevSectorPaddingNotHigh` (the core's current false positive)
- `Grading.DeepReadSector0StartFileNotDemotedByHead` (consequence 2)
- `Grading.DeepReadExtentlessReadsSector0` (consequence 3)
- `Grading.DeepReadShiftedKindSkipped` (consequence 4 — the failed-read class)

## Related findings

- **M-134** (the landing this corrects — its Resolution's address reading is superseded by this
  finding's :322640-671 decode)
- **M-133** (the twin-key/index arithmetic the address shares)

## Open questions

None — the out-parameter triple at :322666-668 is decisive.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-146 landing, session-52 lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Grading.DeepReadOwnLastSectorTailZeroGradesHigh` (the og's true rule: the file's own final sector's 16-zero tail), `Grading.DeepReadPrevSectorPaddingNotHigh` (RED pre-fix — the core's old false positive: the sector before the file ended in zeros and the file promoted), `Grading.DeepReadSector0StartFileNotDemotedByHead` (RED pre-fix — the second false class: a sector-0-starting file was demoted on its head), `Grading.DeepReadShiftedKindSkippedLikeOgFailedRead` (RED pre-fix — FAT16 got content grades the og structurally cannot give), the re-pinned `DeepReadZeroedFirstSectorGradesLow` / `DeepRead0xFFHeadGradesLow` (the extentless class reading sector 0; both RED pre-fix), and the re-pinned `DeepReadSkipsOverlapFlagged`. Suite 1419/1419 (linux-release).
**Resolution notes:** The address derivation replaced wholesale in `deep_read_grade` (cr_grading.cpp): the read address is the twin key — the FIRST extent's END (offset + length) with the M-133 index term (ordinal − 2 for Fat16); a nonzero index term skips the read (the og's out-of-range failed-read class), the extentless class (end == 0) reads the reader's sector 0, and the tail rule reads the containing sector of byte (end − 1) — the byte-exact equivalent of the og's sector-indexed key − 1. The :322640-671 out-parameter decode is recorded in the finding and in the code comment; the M-134 landing's Resolution text (which described the START reading) carries this finding's correction by reference. Four M-134-era tests re-pinned (their fixtures were FAT16 — a kind the og's deep read structurally never grades — and their address expectations encoded the START reading); two new discriminating tests added (PrevSectorPaddingNotHigh, Sector0StartFileNotDemotedByHead). The machinery (reader, thresholds, one-shot bit, re-grade shape) is the M-134 landing's and unchanged.
