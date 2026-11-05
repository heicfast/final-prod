# MINOR N-183 — the gauge population + score formula + QSDE counter mapping: the og's recoverability aggregate is computed over a 20-format candidate set with a (m/t+0.05)·10 score; the core uses all files and maps QuickScanDeleteExtend to the wrong rule

**Severity:** MINOR
**Module:** grading / query (recovery-chances focus battery)
**Parameters affected:** #15 Confidence grading model; #13 Report schema (the aggregate verdict numbers)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:605502-513` (the member-set lookup feeding the gauge-total counters 5820/5830); `.rdata` 0x1413ca6bc-0x1413ca824 (the 20 lowercase keys: doc pdf mov ppt xls cr2 arw dng cr3 mp4 orf raf docx pptx xlsx heic nef jpeg jpg mp3); `:340824-340847` (`FUN_1401d6ff0`: matched = quick-scan engine counts + globals 5838/5828/5818; total = quickTotal + 5820/5830/5840; total<100 → matched=0); `:374451-481` (`FUN_14021b030`: score = int((matched/total + 0.05) * 10)`); `:605899` (the NTFS fixer-index rule ticks `DAT_141be5818`, the gauge-matched global — not QSDE); `:331164-331170` (the real QuickScanDeleteExtend: the assistant field at +0x1b8, incremented per node when bit 7 of +0x72 is clear)
**Core references:** `src/core/tree/cr_grading.cpp:207-216` (the NTFS rule ticks `quick_scan_delete_extend` — the wrong counter), `:318-334` (the gauge: matched = grade==High count over ALL files; total = total_file_count; small_result <100), `include/cr/tree/cr_grading.h:89-122` (AssistantStats) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Three sub-divergences in the recoverability aggregate's accounting: (a) the og's gauge counts a **20-format candidate population** (the member set) plus auto-High classes, while the core counts every file; (b) the og renders a 0-10 score via `int((matched/total + 0.05) * 10)` for the verdict copy — the core has no score formula; (c) the core's `quick_scan_delete_extend` stat is ticked by the NTFS device-index rule, but in the og that rule ticks the gauge-matched global `DAT_141be5818` while QSDE is a separate assistant field incremented on a per-node flag bit.

## What the reference does

(a) The insert-time hook counts gauge totals only for nodes whose (lowercased) extension is in the 20-format member set (`:605502-513`, the found-branch ticking 5820/5830 by the gauge-split bit) plus the email/Btrfs/ReFS auto-High ticks (5830/5828 — see S-061); (b) `FUN_1401d6ff0` composes matched from the quick-scan engine counts plus globals 5838/5828/5818 and applies the total<100 small-result heuristic; `FUN_14021b030` then computes `score = int((matched/total + 0.05) * 10)` behind the "Your files are highly recoverable…"/"Good news…"/"Great news…" verdict copy; (c) `DAT_141be5818` (gauge-matched) is ticked by the NTFS fixer-index rule (`:605899`) and by both overlap merges (`:606093`, `:606191`), while QuickScanDeleteExtend is the assistant's +0x1b8 field incremented per node when bit 7 of +0x72 is clear (`:331164-331170`, the unlogged ninth counter +0x1bc taking the else case).

## What our core does

`cr_grading.cpp:318-334`: `out.total = out.total_file_count` (all files), `out.matched = count(grade==High)` (all files), `small_result = total < 100 → matched = 0` (this part matches). No score formula exists anywhere. The NTFS solo rule ticks `++stats->quick_scan_delete_extend` with the comment `/* DAT_141be5818 */` (`:213`) — the counter it labels is the gauge-matched global, so the core's QSDE stat counts the NTFS fixer-index population instead of the og's flag-bit population.

## Gap

Stated falsifiably: a scan of 1000 files of which 300 are `.jpg`/`.doc`-family (the member set) with 200 High produces og gauge numbers over the member-set population and a score like 7; the core produces matched/total over all 1000 and no score; the core's QSDE field equals the NTFS-index count while the og's equals its own per-node flag count.

## Impact

The aggregate verdict numbers surfaced in the report/ABI (`scan_analysis`) diverge from the og's — a marketing-copy-adjacent surface in the og, a stats block in the core. The QSDE mis-mapping makes the one named stat semantically wrong. MINOR (accounting surface; the per-file grades are owned by S-058/S-061/M-138/M-134 — M-133 renumbered to M-138 at the N-160 doc-truth repair).

## Reproduction / verification

All sites read first-hand this session (the member-set extraction is byte-exact from `.rdata` via the s5 dump's ctor map + this session's re-read; the score formula and gauge composition at the cited lines; the QSDE field at `:331164-170`). Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-grading-gate-polarity.md` §6.

## Fix plan (for the engineering team)

1. Add the 20-format member set as the gauge's candidate population (`kGaugeCandidateExtensions` — lowercase, matching the og's member set exactly); tick gauge totals for it and for the auto-High classes; keep matched = grade-High within that accounting.
2. Port the score formula `int((matched/total + 0.05) * 10)` into the report's scan_analysis block (clamped when total==0).
3. Fix the QSDE mapping: `quick_scan_delete_extend` increments per node with the og's flag condition (the core's `DeletedState` mapping of bit 7 of +0x72 — the deleted/non-existing axis), and the NTFS fixer-index tick moves to the gauge-matched counter it actually is.
4. Tests: pin the member-set membership (20 keys), the score formula boundaries (0.05 rounding), and the QSDE-vs-NTFS-count distinction.

## Tests to add

- `Grading.GaugePopulationIsMemberSet` (fails today)
- `Grading.ScoreFormulaRoundsLikeOg` (fails today: no formula)
- `Grading.QSDENotTheNtfsIndexCount` (fails today)

## Related findings

S-058/S-061 (the per-file rules feeding matched), M-134 (the deep read's rawCount return feeding the same accounting).

## Open questions

None (the ninth unlogged counter +0x1bc's consumer is UI-side only — registered in the worksheet).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-183 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Grading.GaugePopulationIsMemberSet` (a 7-file tree: gauge_total counts exactly the 4 member-set files while `total` keeps all 7), `Grading.ScoreFormulaRoundsLikeOg` (200 member files, 100 High → int((0.5+0.05)×10) = 5; the ≥100 floor cleared; the small-result zeroing pinned by the first draft's own failure — a 20-file tree floors to 0, corrected in the pin), `Grading.QSDENotTheNtfsIndexCount` (the deleted-NTFS fixer-index tick ≠ the 3-node non-existing QSDE count — the fused counters now provably distinct). RED pre-fix: the tests do not compile without the new fields, and the behavioral RED was pinned during development (the first working build showed gauge_total/QSDE both 0). Suite 1361 ran / 1319 pass / 42 env-skips / 0 fail.
**Resolution notes:** All three sub-gaps landed per the fix plan: (a) `kGaugeCandidateExtensions` — the 20 lowercase keys byte-exact from the .rdata ctor map (re-read at DRWUI:605502-513 via the finding's evidence), counted with the og's lowercased set lookup (the first draft's case-sensitive compare missed every uppercase extension — caught by the pin) plus the auto-High classes (email category + non-existing ReFS/Btrfs per S-061); (b) the score formula `int((matched/total + 0.05) * 10)` (FUN_14021b030 :374451-481) computed over the gauge's own population, 0 on empty, the small-result zeroing applied before it; (c) the QSDE re-mapping — `quick_scan_delete_extend` now counts the per-node non-existing population (the +0x1b8 field, DRWUI :331164-170, DeletedState != Active), and the NTFS fixer-index rule ticks its own `ntfs_device_index_high` (the DAT_141be5818 gauge-matched global it actually is). The ScanAnalysis report block carries all four new fields append-only (schema v1's append-only contract; the cr_ui copy updated); the all-files total/matched view stays beside the member-set view as data. One rule-order truth documented in the QSDE pin: the existing-rule shadow on live NTFS nodes means the fixer-index rule fires on the non-existing population — matching the og's insert-time-High-then-device-walk composition.
