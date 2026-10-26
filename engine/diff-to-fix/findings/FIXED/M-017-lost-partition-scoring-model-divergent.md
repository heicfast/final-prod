# MAJOR M-017 — Lost-partition scoring model divergent: og uses content-driven ln(count)/ln(max) with FS weights, bonus, and +500 table-match; core uses absolute structural evidence points

**Severity:** MAJOR (candidate ranking parity; result-set composition)
**Module:** lost-partition
**Parameters affected:** A30 (candidate scoring), B3-adjacent (result ordering), D15-adjacent
**Corpus references:** `DRWUI_decompiled.c:842164-842278` — the scorer (`FUN_14050fda0`): count = `files + 50·folders` (NTFS family; FS==8 → f+10·d; FS 1/2 (FAT) → (f+d)·50), `+= *(longlong*)(info+0x68)` (per-FS bonus, search-DLL supplied), `+= 500` on partition-table match (`"Found once partition:%I64d\r\n"` `:842260-66`); denominator = max over all scan candidates, `denom = (maxCount > 1) ? log(maxCount) : 1.0` (`:842226-239`, natural log); numerator `if (1 < count) num = log(num)` else 1.0 (`:842269-72`); `score = num/denom` (`:842274`). Post-hoc `AdjustPartitionInfoOrder` `FUN_140510410 :842756-842851`: 0.1/0.3 score-gap keep rules, ~16 MiB over-large duplicate → flag + score-copy `:842851` + splice-to-front, FAT multi-child keep `:842812-821`. Verified to 6 decimals against all 187 rows of `re-workspace/raw-evidence/ui-interaction-recovery-flow/applog_log_CalcPartition.log` (e.g. 429f → ln(429)/ln(168849) = 0.503579; (0,25)+bonus20 → 0.585518; count==1 → 1/ln(max) = 0.081928)
**Core references:** `src/core/lost/cr_candidates.cpp:61-158` (absolute evidence points: NTFS OEM+DBR +15 / BPB +25 / $MFT alive +30; FAT +20/+10; exFAT 15+20; GPT 10+20), `:264` (`c.score = pts`), `:289-304` (overlap penalties), `:305-306` (sort desc) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied. Resolves tracked-open OQ-T9 (see `resolved/tracked-open/`).

## Summary

The og ranks lost-partition candidates by a **relative, content-driven** score: a per-filesystem weighted count of the files/folders the candidate actually contains (plus a per-FS bonus supplied by the search DLL and a flat +500 "files" when the candidate matches an existing partition-table entry), normalized as ln(count)/ln(best-count-across-this-scan). The core ranks by **absolute structural evidence** (boot-sector/bpb/$MFT liveness points with overlap penalties). The two models order candidates differently on almost any multi-candidate fixture: a file-rich partition outranks a file-poor one in the og even when the file-poor one has cleaner boot sectors — the og's score measures *recovery value*, the core's measures *signature plausibility*.

## What the reference does

Quotes above. Notable og quirks the fix must reproduce for parity: the **count==1 numerator stays 1.0** (so a 1-file partition scores 1/ln(max) ≈ 0.08 — ranking it above every 2..6-file candidate in the observed log); the bonus field travels over IPC from the SearchDLLs (observed NTFS 60/30, FAT32 20); the over-large (~16 MiB tolerance, `:842776-793`) duplicate gets the first candidate's score copied and is spliced to the front of the result list.

## What our core does

Structural points (all healthy NTFS candidates tie at 70 before penalties), overlap penalties, same-FS nested dedup. None of the og's content weighting, per-scan normalization, table-match bonus, or post-hoc ordering rules.

## Gap

Falsifiable claim: on a fixture with a 300-file NTFS candidate (weak boot sector) and a 20-file NTFS candidate (clean boot sector), the og ranks the 300-file candidate first (ln(300) > ln(20+bonus) under any observed bonus); the core ranks the clean-boot candidate first (15+25+30=70 vs e.g. 55). Generalizes to every multi-candidate scan.

## Impact

Result ordering is user-visible (which lost volume sits at the top of the list) and drives auto-selection semantics; comparisons of "which candidate won" in bench harnesses diverge mechanically. The og's score is also the input to AdjustPartitionInfoOrder's keep/prune rules — absent a compatible score, porting those rules faithfully is impossible.

## Reproduction / verification

- The formula was decoded from `DRWUI_decompiled.c:842164-842278` and **verified numerically against all 187 log rows to 6 decimals** (the strongest available executed-corpus evidence — the og's own output).
- Core side verified first-hand (`cr_candidates.cpp` reads above).

## Fix plan (for the engineering team)

1. Add a per-FS **content probe pass** to candidate evaluation: count files/folders found in the candidate (the scan already opens candidate volumes for probes — reuse the walk with an early-exit budget).
2. Compute `count` with the og's weights: FS 8 → f + 10·d; FS 1/2 → (f+d)·50; otherwise (NTFS 3, exFAT 7, …) → f + 50·d; add the per-FS bonus (pin the setters in the SearchDLLs — see OQ-S2M-2; until pinned, use observed values NTFS 60/30, FAT32 20 as defaults) and **+500** when the candidate's FS/DataAreaStart matches a partition-table entry.
3. Score = ln(max(count,1))/ln(max(maxCount,1)) with the og's exact edge cases (count==1 numerator 1.0; maxCount ≤ 1 → denom 1.0). Keep the structural points as a tie-break/secondary sort if desired, but the primary key must be the content score.
4. Port `AdjustPartitionInfoOrder`: 0.1/0.3 gap keeps, over-large (~16 MiB) duplicate → copy first candidate's score + splice-to-front, FAT multi-child keep.
5. Bench: re-run lost-partition scenarios; candidate ordering must flip to og order on the log-reproduced fixtures.

## Tests to add

- `LostPart.ScoringMatchesLogRows` — table-driven test reproducing rows from `applog_log_CalcPartition.log` (429f→0.503579, 0f+25d FAT32→0.585518, 1f→0.081928-class, capped 1.0).
- `LostPart.FileRichCandidateOutranksCleanBoot` — the 300-vs-20 fixture (fails before).
- `LostPart.TableMatchBonus500` and `LostPart.OverLargeDuplicateRescoredToFront`.

## Related findings

- S-006 (probe families — the scoring fix rides on the same candidate-content pass) · N-025 (LDM geometry minors) · OQ-S2M-2 (bonus setter provenance)

## Open questions

The bonus (+0x68) setter inside the SearchDLLs is not yet pinned (observed values and the IPC consumer are decoded) — tracked in OQ-S2M-2.

---

## Session-3 parallel-leg addendum (2026-10-26, S3-LOST agent — OQ-T9's premise corrected; merge-order decode completed)

The parallel S3-LOST leg resolved OQ-T9 from the other end and renumbered its
two findings into this addendum (full decode: `evidence/decompiled-quotes/s3-calcpartition-scoring.md`
+ executed vtable dumps in `evidence/executions/s3-calcpartition-vtables/`):

1. **The `*CalcPartition` classes are NOT scorers — OQ-T9's premise was half-wrong.**
   An executed RTTI COL walk on the og binaries resolved
   `Fat32CalcPartition::vftable @ 0x180053680` and `ExFatCalcPartition::vftable @
   0x18004b9f0`: vtable slot 4 (the "score" call site the session-2 leg located at
   FATSearchDLL_decompiled.c:5744-5747) returns `*(u32*)(this+0x18)` — the volume's
   **sectors-per-cluster** — proved by the read outside the table,
   `(cluster−2)*SPC + start` at :39076-39083. These classes are per-lost-volume
   **geometry calculators** that let the FAT/exFAT engines read files outside the
   allocation table; only those two engines instantiate them (zero refs in the
   other seven SearchDLLs). OQ-T9's "scoring via CalcPartition" framing is retired;
   this finding's DRWUI decode is the real scoring model.
2. **Scoring model confirmed independently + extended.** The parallel leg decoded
   the same `FUN_14050fda0` constants (files + 50·folders NTFS family; FS==8 →
   f + 10·d — **FS code 8 is HFS+**; FAT 1/2 → (f+d)·50) and the same +0x68
   per-FS bonus and +500 table-match bonus, and arithmetically validated against
   the same CalcPartition.log (3 rounds, ≤2e-5 residual) — independent
   confirmation of this finding's numbers.
3. **`AdjustPartitionInfoOrder` decoded in full** (beyond the post-hoc paragraph
   above): `FUN_140510410 :842495-842859` runs (a) containment-in-known-partition
   drop, (b) FS-grouped dedup, (c) top-2 tie rules (score gap ≤ 0.1 keep both;
   ≤ 0.3 keep when the runner-up is FAT — **NTFS-over-FAT priority**), (d) FAT
   multi-vector drop, (e) **±16 MiB size-match promotion to result #2**
   (log-proven in CalcPartition.log round 3), (f) surfaced-result **cap
   2/4/5/6**, and sorts by a **stable descending merge sort** (order stability
   for equal scores — the core's `std::sort` is not stable; `std::stable_sort`
   or an explicit tie key is needed for parity).
4. **Related new findings from the same decode:** N-034 (FAT lost-boot gate
   constants: numFATs ∈ {1,2}, FAT12 spf ≤ 200 probe, root_entries×32 % bps —
   complements the N-010b engine-side gates), N-035 (candidate extents are
   **data-area-anchored** `EndSec = DataAreaStart + totalSector` vs the core's
   whole-volume extent — the containment-drop and 16-MiB rules consume different
   extents than the core's), S-019 (BitLocker "-FVE-FS-" lost-partition probe
   family missing — S-006 under-listed the og).

---

## Session-3 parallel-leg convergence note (2026-10-26)

The *storage/census parallel leg* decoded the same scorer independently against
the round-1 CalcPartition log (25+ rows fit to <= 3e-5, including the raw<=1
identity non-monotonicity: 1 file -> 0.083079 ranks above 2 files -> 0.057586)
and reached the same model — independent corroboration of this finding's
arithmetic. Its decode additionally pinned: the evidence-accumulator producers
on `CSearchedNTFSPartitionInfo+0x68` (NTFSSearchDLL :3046/:3078 = +80 system
files, :7230 = +50 boot-SPC validated, :7220 = -10 read failure, :28973 = +30)
and the display cap (top 2; 5 when >= 2 adjusted candidates, DRWUI :845676-701).
Both legs rate the table-match polarity inversion (og +500 vs core -10/-20 at
`cr_candidates.cpp:289-303`) the sharpest actionable slice of this finding.

---

## Addendum (session 6 — the CalcPartition scorer classes decoded in the function-map artifact, 2026-10-26)

The per-engine-function-map artifact (first-cited this session) evidences the scorer-side classes this finding's model diverges from: **`Fat32CalcPartition` ×39 references** (fat-engine--deep-dive.md:20-22 — "the FAT32-specific geometry/capacity calculator used during validation"; the function map's RTTI census at per-engine-function-map.md:13) and **`ExFatCalcPartition` ×54** (per-engine-function-map.md:14) — plus `ICalcPartition` in the FAT engine's class inventory (fat-exfat-engine-internals.md:22, "geometry calculation"). The SPC-re-derivation interpretation (the scorers re-derive expected sectors-per-cluster geometry and rank candidates by the overlap) now has named-class evidence; the parallel session-3 legs' dSuggestted decode (recorded in this finding's body) remains the quantitative model. No change to the finding's substance or fix plan — this addendum adds the class-level citations the fix leg will need when porting the scorer family. Related new context: PartitionRecoveryD's two-generator-set architecture (I-024/OQ-S6-1) is the module that DRIVES these scorers on the og side — the decode order for the fix leg is OQ-S6-1 first (which generator set hosts the CalcPartition calls), then this finding's port.

## Resolution

**Status:** FIXED
**Fixed in commit:** <this push — DFX20 session, M-017 landing>
**Verified by:** 7 tests: LostScoring.ScoringMatchesLogRows (the QA-log rows reproduced to print tolerance — 429f→0.5036, 51f→0.32665, the count==1 edge 1/ln(max), the max=1.0 identity), LostScoring.FileRichCandidateOutranksCleanBoot, LostScoring.TableMatchBonus500, LostScoring.SizeMatchPromotionToResultTwo, LostScoring.ContainmentDropAndFATTieRule (incl. the FAT-specific 0.3 arm and its NTFS counterpart), LostScoring.CountOneEdgeRanksAboveSmallCounts, and LostScoringIntegration.FileRichCandidateOutranksThinCandidateInScan — the finding's falsifiable fixture EXECUTED at the orchestrator level (fail-before proven: with the rank_and_adjust call temporarily disabled the test goes RED on the discovery-order list; restored -> green). Suite 706/706 linux-gcc (RelWithDebInfo + ASan).
**Resolution notes:** CONFIRMED-DIVERGENCE, fixed per the finding's 5-step plan plus the session-3 addendum's full AdjustPartitionInfoOrder decode. New pure-core module cr/lost/cr_scoring.{h,cpp}: (1) the og's raw counts — FS 8/HFS+ = files + folders*10; FS 1/2/FAT16-32 = (files+folders)*50 (the unsigned FS-1<2 edge preserved: FAT12 falls to the else branch); else = files + folders*50; + the per-candidate bonus; +500 on a known-partition-table match (start + family). (2) DECODE CORRECTION the fix pinned during bring-up: the denominator is computed in the og's PASS 1 from the PRE-+500 maximum (:842209-842236); the +500 is a pass-2 per-candidate numerator add (:842260-842267) — the QA-log rows only fit this way (429f: ln(429)/ln(168818), the top candidate's pre-bonus raw). (3) ln/ln normalization with the raw<=1 numerator edge (the count==1 non-monotonicity). (4) Bonus defaults reproduce the observed rows (FAT16/32 = 20, everything else 0; the producer decode stays OQ-S2M-2 — not invented). (5) The full AdjustPartitionInfoOrder port: known-extent containment drop (corpus arbitration: the round-1 log proves the +500 exact-match candidate SURVIVED, so with one live table feeding both rules the drop is proper-subset-only — the og's ActivePartition list and scorer array are disjoint inputs), top-2 tie rules (gap <= 0.1 drops #2; <= 0.3 drops #2 when #2 is FAT — NTFS-over-FAT priority), the size-match promotion re-decoded FIRST-HAND this session (LAB_140510a0e :842843-842858: the first size-matched candidate at sorted position >= 1 — |totalSector - reference|*512 < 16 MiB+1 — is marked promoted and, when it sits at position >= 2, spliced to RESULT #2 with the FOLLOWING candidate's score copied into it, matching the log's round-3 landing exactly), the FAT multi-child drop left as a REGISTERED RESIDUAL (no core-side input — the og reads a CFATSearchedPartitionInfo sub-object vector; inventing one would be hallucination), STABLE descending sort (std::stable_sort, the og's merge sort), and the result cap NOT ported (a UI curation layer the og feeds into a secondary list — the core surfaces every candidate; dropping results in the core would be data loss). (6) Orchestrator wiring: the candidate fan-out now counts recovered content per candidate root (files + folders; both live and deleted — recovery value), ranks, and rewrites stats.lost_candidates in the og's order; `score` becomes the og-shaped permille (the og's dSuggested is a double printed %f — the permille preserves the int score field's persisted shape); the structural probe points remain as the evidence list. (7) Persistence: REC_PARTITION_CONTEXT v3 (57 bytes, length-detected like v2) carries content_files/content_folders so a reloaded session keeps the ranked order + scores. The reference-sector producer (the og's *(this+0x40)) is unpinned: the core passes the scan view's total sectors and the API takes it explicitly (0 = the rule honestly does not fire) — registered, not papered over. OQ-T9 stays resolved (the addendum's geometry-calculator retirement); OQ-S2M-2 (bonus setters) and the FAT multi-child residual remain tracked.
