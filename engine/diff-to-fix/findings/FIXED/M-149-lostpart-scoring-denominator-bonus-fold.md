# MAJOR M-149 — lost-partition scoring math: the +500 table-match bonus is folded into the DENOMINATOR (the og's max loop has no bonus walk) and the count==0 numerator is 1.0 (the og's is 0.0 — log-proven); the 9c36efba convergence merge dropped the corrected implementation and the "next pass" never ran

**Severity:** MAJOR
**Module:** lost-partition (candidate scoring)
**Parameters affected:** A30 (candidate scoring — features and thresholds), B1-adjacent (the surfaced candidate order in degenerate scans), X47/X48-adjacent (score values surfaced downstream)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` FUN_14050fda0 :842186-842240 (the max do-loop: per-FS weighted count + the +0x68 add, **no +500 walk**), :842242-842245 (the count==0 / null-payload numerator `uVar12 = 0; uVar14 = 0x3ff00000;` → the den fallback AND `dVar13 = 0.0` for the null case in the per-entry loop), :842246-842274 (the per-entry loop with the +500 table-match at :842258-842266 and the `count > 1 ? ln : (double)count` numerator); the og's own runtime output `og-codebase/log/CalcPartition.log` (the last round's row 6: `files:0 folders:0 dSuggestted:0.000000`; round-1 row 2 `files:429 folders:0 dSuggestted:0.503579` — the numeric fit in `evidence/executions/s79-calcpartition-score-fit/`)
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` (the N-200 probe-evidence landing shifted the file; the cites refreshed) — `src/core/lost/cr_candidates.cpp:1091-1101` (was :1054-1065 at the audited pin) (the single-pass loop: `if (c.table_match) count += 500;` BEFORE `if (count > max_count) max_count = count;` — **the bonus is inside the denominator**), `:688-694` (`score_ratio` (was :655-665 at the audited pin): `num = count > 1 ? log(count) : 1.0` — **count==0 → 1.0**); the dropped correct implementation preserved in git history at `48a6c099:src/core/lost/cr_scoring.cpp` (`normalize()`: `num = raw > 1 ? ln(raw) : (double)raw`; pass-1 max without the +500); the convergence merge `9c36efba` (the wrong-side-wins record, worklog DFX20-M017-CONV :11740-11775)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's lost-partition ghost-score computes `ln(count)/ln(max)` with two errors relative to the og (and relative to this repo's own M-017 correction record): the +500 known-table bonus is added to the count BEFORE the max is taken — so the denominator includes the bonus and a table-matched leader scores exactly 1000 permille instead of the og's >1.0, while every non-matched candidate's score is deflated by the inflated denominator; and a candidate with count 0 gets the numerator 1.0 where the og's runtime log prints 0.000000 for exactly that shape. The M-017 convergence history explains both: two lanes landed implementations, the merge dropped the corrected one per FIRST-LANDING-WINS, recording "the two decode divergences for the next pass" — and no later pass re-verified the canonical form.

## What the reference does

The og's FUN_14050fda0, re-read first-hand:

**The max loop (the denominator source) — no bonus, no table-match walk:**

```c
/* :842210-842240 (the second loop; the dead three-maxima first loop is a
 * decompiler artifact of the same walk) */
do {
    lVar8 = plVar5[2];
    lVar6 = lVar11;                                  /* 0 */
    if (lVar8 != 0) {
        if (*(int *)(lVar8 + 8) == 8) { lVar6 = *(lVar8+0x20) + *(lVar8+0x28) * 10; }      /* HFS+ */
        else if (*(int *)(lVar8 + 8) - 1U < 2) { lVar6 = (*(lVar8+0x28) + *(lVar8+0x20)) * 0x32; } /* FAT */
        else { lVar6 = *(lVar8+0x28) * 0x32 + *(lVar8+0x20); }                            /* NTFS */
        lVar6 = *(longlong *)(lVar8 + 0x68) + lVar6;  /* + the dSuggested base — NO +500 */
    }
    ...
    if (lVar6 <= lVar9) lVar6 = lVar9;               /* running max */
} while (...);
if (1 < lVar6) { dVar13 = log((double)lVar6); ... }  /* den = ln(max) */
else { uVar12 = 0; uVar14 = 0x3ff00000; }            /* den = 1.0 */
```

**The per-entry loop — the +500 lands HERE, and the zero edge is 0.0:**

```c
/* :842246-842274 */
lVar9 = plVar7[2];
if (lVar9 == 0) {
    dVar13 = 0.0;                                    /* null payload -> num = 0.0 */
} else {
    ... /* the same weighted + 0x68 count */
    if (lVar8 != 0) {
        for (lVar6 = *param_3; lVar6 != param_3[1]; lVar6 = lVar6 + 0x18) {
            if ((iVar1 == *(int *)(lVar6 + 0x10)) &&
               (*(longlong *)(lVar9 + 0x18) + param_4 == *(longlong *)(lVar6 + 8))) {
                FUN_14050fa20("Found once partition:%I64d\r\n", ...);
                lVar8 = lVar8 + 500;                 /* the table-match bonus, numerator-only */
                break;
            }
        }
    }
    dVar13 = (double)lVar8;
    if (1 < lVar8) { dVar13 = log(dVar8); }          /* num = count>1 ? ln : (double)count */
}
*(double *)(lVar9 + 0x70) = dVar13 / den;            /* the score, a raw double */
```

**The og's own runtime output confirms both edges** (`og-codebase/log/CalcPartition.log`, the 00:43:57 round): row 1 `files:16 folders:2 dSuggestted:1.000000` (the leader, no table-match — a matched leader would print >1.0); rows 2-5 `files:0 folders:0 dSuggestted:0.632748` (empty w/ a nonzero dSuggested base); **row 6 `files:0 folders:0 dSuggestted:0.000000`** — the count==0 shape scores ZERO. The round-1 numeric fit (`ln(429)/ln(168878) = 0.503577` vs the logged `0.503579`, residual 2e-6) is reproduced by the fit script in the executions evidence.

## What our core does

`src/core/lost/cr_candidates.cpp:1091-1107` — a SINGLE pass, bonus before max:

```cpp
u64 max_count = 1;
for (auto& c : adjusted) {
    u64 files = 0, folders = 0;
    content_counts(effective, c, cancel, files, folders);
    u64 count = weighted_count(og_fs_code(c.fs), files, folders);
    count += fs_bonus(c);
    c.table_match = table_match(c, known);
    if (c.table_match)
        count += 500;                 /* <- inside the max accumulation */
    c.content_count = count;
    if (count > max_count)
        max_count = count;            /* the denominator now includes the +500 */
}
for (auto& c : adjusted)
    c.score = int(score_ratio(c.content_count, max_count) * 1000.0);
```

and `score_ratio` at `:688-694` (the re-verified pin):

```cpp
const double num = count > 1 ? std::log(double(count)) : 1.0;   /* count==0 -> 1.0 */
const double den = max_count > 1 ? std::log(double(max_count)) : 1.0;
```

Worked example (leader count 260, table-matched → 760; runner-up 200): og — den = ln(260) = 5.56, leader = ln(760)/5.56 = 1.18 (>1.0), runner-up = ln(200)/5.56 = 0.953; core — den = ln(760) = 6.63, leader = 1000 permille, runner-up = ln(200)/6.63 = 0.799. Every non-matched candidate is deflated by the ratio ln(max)/ln(max+500) whenever the leader is matched.

The count==0 edge: the core's `fs_bonus` returns 0 for every non-NTFS/non-FAT family (`:781-794`), and `content_counts` returns early with zero counts when materialize fails or the family has no walk (`:694-696`, GptTable/BitLocker/Unknown) — so empty ext4/HFS+/Btrfs/ZFS/ReFS/BitLocker/GPT candidates reach `score_ratio` with count 0 and score `1.0/ln(max)` (up to 1000 permille in all-empty scans; ~200 permille in mixed scans) where the og prints 0.000000. Empty candidates TIE with genuine 1-file candidates (both num 1.0) and the tie breaks on structural points — an ordering inversion the og never produces.

## Gap

Two falsifiable claims: (1) a table-matched leader's score is exactly 1000 permille in the core and >1.0 (the raw double) in the og, with every runner-up deflated by the denominator inflation — reproducible by replaying any CalcPartition-style row set through both formulas; (2) a candidate with weighted count 0 scores 1.0/ln(max) in the core and 0.0 in the og — the og's own log row (files:0 folders:0 → 0.000000) is the ground truth.

## Impact

The ghost-score drives the candidate ORDER (the stable sort at :1112-1116) and the 16-MiB size-match promotion (which copies the leader's score). The order among content-bearing candidates is preserved (both formulas are monotone in count), but: (a) every surfaced score VALUE diverges systematically when the leader is table-matched — the scores are persisted in the session records and surfaced in the CLI's candidate listing; (b) empty candidates (failed materialize, encrypted/table families) are inflated from the og's floor to mid-band scores and can tie-or-beat genuine 1-file candidates via the structural tie-break; (c) in degenerate all-empty scans every candidate scores 1000 permille in the core and 0 in the og. This is the exact class M-017 itself was filed for, resurfacing through a convergence merge.

## Reproduction / verification

Three legs, all first-hand this session: (1) decompile — the two og loops quoted above (the max loop without any +500 walk; the per-entry +500 and the `dVar13 = 0.0` zero edge); (2) executed-evidence — the og's own `CalcPartition.log` rows fit numerically (`evidence/executions/s79-calcpartition-score-fit/fit_script.py` + `fit_output.txt`: the round-1 rows fit the no-bonus denominator at residual ≤2e-6; the last round's zero row proves the 0.0 edge); (3) git archaeology — the corrected two-pass implementation at `48a6c099:src/core/lost/cr_scoring.cpp` (`normalize()` with `(double)raw` and the pre-+500 max) was dropped by the merge `9c36efba` whose own message records "the two decode divergences for the next pass", and `git grep rank_and_adjust` at HEAD returns zero hits (the corrected form is fully gone from the tree).

## Fix plan (for the engineering team)

1. Split the single pass back into the og's two passes: pass 1 computes `count = weighted + fs_bonus` (NO +500) for every candidate and takes `max_count`; pass 2 recomputes with `+ (table_match ? 500 : 0)` and scores `num/den` with `den = max_count > 1 ? ln(max_count) : 1.0`.
2. Restore the og's numerator edge: `num = count > 1 ? ln(count) : (double)count` — count==0 → 0.0, count==1 → 1.0 (the identity quirk the original landing's comment already documents).
3. Decide the >1.0 representation: the og stores the raw double at +0x70 (the log's `%f`); the core's permille int can exceed 1000 legitimately — either widen to a double score field or clamp with an explicit registration (the dropped implementation clamped at 4000 permille; if that is kept, register it in PARITY-DELTAS).
4. Files: `src/core/lost/cr_candidates.cpp` (the two loops + `score_ratio`), `include/cr/lost/cr_lostpart.h` (the score field if widened).
5. Tests: (a) a CalcPartition log-row replay test — feed the 186 fitted rows and assert the core's scores match the logged six-decimal values (the M-017 landing's own test class, re-pinned to the two-pass form); (b) the zero-row discriminator: an empty candidate scores 0, not 1000/`ln(max)`; (c) the matched-leader discriminator: the leader scores >1000 permille (or the registered clamp) and the runner-up's score is NOT deflated by the bonus.
6. Acceptance criteria: the log-row replay matches ≤1e-5 residual; no empty candidate ever outscores a 1-file candidate; the 16-MiB promotion and the stable-sort order are unchanged on the intact population (the existing LostDiscovery suite stays green).

## Tests to add

- `Lost.ScoringTableMatchBonusIsNumeratorOnly` (RED today: the leader's score is exactly 1000).
- `Lost.ScoringZeroCountScoresZero` (RED today: 1.0/ln(max)).
- `Lost.ScoringCalcPartitionLogReplay` (the 186-row fit, both rounds).
- `Lost.ScoringEmptyCandidateNeverOutranksOneFile` (the tie-break ordering pin).

## Related findings

- M-017 (FIXED — the original scoring-model landing whose convergence record this finding closes out).
- The lost-partition.md standing note "0.1/0.3 gap keeps … FAT multi-child keep — no core counterpart" (the AdjustPartitionInfoOrder SURVIVAL rules, M-017's known remainder — separate gap, not re-filed here; the 0.1/0.3 thresholds consume the score values this finding repairs).

## Open questions

The >1.0 representation choice (raw double vs the 4000-permille clamp the dropped implementation used) is the only engineering decision; the og-side math itself is fully decoded and log-verified.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** the M-149 S80-lane commit (this session; SHA in the commit footer)
**Verified by:** LostDiscovery.ScoringTableMatchBonusIsNumeratorOnly + LostScoring.MatchesLogRows — RED first-hand via the stash dance (2/2 FAILED at the pre-fix tree: the matched leader exactly 1000, the runner-up deflated, the zero edge 1.0), GREEN post-fix; LostDiscovery.ScoringZeroCountScoresZero (the og's 0.0 floor + the ordering discriminator); the full LostDiscovery/LostScoring suite 27/27; full suite 1624/1624.
**Resolution notes:** The two-pass og-exact form restored: pass 1 computes count = weighted + fs_bonus for every candidate and takes max_count with NO +500 walk (FUN_14050fda0 :842210-842240); pass 2 stores content_count = count + (table_match ? 500 : 0) — the bonus lands in the NUMERATOR only (:842258-842266). score_ratio's numerator edge restored to `count > 1 ? ln(count) : (double)count` — count==0 scores 0.0 (the og's own CalcPartition.log 'files:0 folders:0 dSuggestted:0.000000' row), count==1 keeps the identity quirk. The >1.0 representation: the int-permille field carries values above 1000 legitimately (the og's raw double >1.0 at +0x70); registered in PARITY-DELTAS D-151 (the dropped 48a6c099 clamp NOT adopted — no realistic maximum approaches it and the unclamped field is the honest representation). Seven stale pins re-truthed to the og's zero floor (the empty-candidate inflation class the finding itself describes: exFAT 244→0, HFS+/Btrfs/ZFS/ReFS/BitLocker ≥N→0 with the structural evidence re-pinned on structural_points — the fields the evidence actually lives in). Files: src/core/lost/cr_candidates.cpp (score_ratio + the two-pass loop), tests/unit/test_lostpart.cpp.
