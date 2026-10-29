# MINOR N-205 — the recoverability rawCount counts different populations: the og's forced deep-read probe ∧ size-rule ticks vs the core's Origin::Carved census — and the M-134 resolution's claimed `raw_tail_zero` input does not exist at HEAD

**Severity:** MINOR
**Module:** grading (recovery-assistant statistics)
**Parameters affected:** #15 Confidence grading model (the assistant counter family), #13 Report schema (the raw_count report field), #65 Test coverage (the resolution's phantom verification claim)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:331183-331311` (FUN_1401c7d80 — the forced quick-scan probe walk), `:331278` / `:331289` (the rawCount increment `*(int*)(param_1+0x1a0)+1`, gated on the forced probe's return ∧ the size rule), `:330974-330999` (FUN_1401c76e0 — the size rule: FS-code 0x23 → size ≤ 0x77fffff; any other code → size ≥ one cluster), `:330915-330916` (the rawCount log line reading byte +0x1A0), `:134418` (the CFileSystemNode ctor's 0x23 default — the field every SearchDLL also defaults: NTFSSearchDLL:39787, FAT:33549, exFAT:19915, ReFS:6393, Btrfs:11273, HFSPlus:17216, EXT4:13014, Zfs:39414)
**Core references:** `include/cr/tree/cr_grading.h:110-128` (AssistantStats — the header comment's `raw_count = Origin::Carved` mapping at `:117`), `src/core/tree/cr_grading.cpp:365` (`++out.raw_count` — the Origin::Carved census), `src/core/recovery/cr_report.cpp:170` (the report's `raw_count` field), `src/ui/cr_ui.cpp:2487/:4331` + `src/app/rrecov.cpp:2484` (the ABI surfaces); the M-134 Resolution's claim "`raw_tail_zero` in AssistantStats" — **grep `raw_tail_zero` over `src/ include/` returns ZERO hits at HEAD `9633d94f`** (phantom)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two coupled defects on the assistant's `raw_count` statistic. (a) Semantic: the og's rawCount is a **validated-recoverable** count — the forced deep-read probe (`FUN_14038c470(node, addr, 1, 0)`) ANDed with the FS-code-dependent size rule (`0x23 → ≤ 0x77fffff`) — ticked only for grade-1 NTFS nodes at quick-scan populate; the core's `raw_count` is a plain **Origin::Carved census** of every carved file. The two counts diverge on every scan that carves anything (the og's gate validates content; the core's gate is provenance). (b) Bookkeeping: the landed M-134 resolution states "The N-183 rawCount consumer now has its input (`raw_tail_zero` in AssistantStats)" — no such field exists anywhere at HEAD; the resolution's verification claim is phantom on this leg.

## What the reference does

At quick-scan populate, `FUN_1401c7d80` (D:331183-311) walks the grade-1 NTFS nodes with `0 < addr < device-size` and calls the deep read in FORCED mode (`FUN_14038c470(param_2, param_4, 1, 0)` — param_3=1 is the observe-only forced probe). The increment arm (D:331278, D:331289): if the forced probe returns nonzero (the previous-sector tail-zero signature — the intact-file padding evidence) AND the size rule `FUN_1401c76e0` passes → `*(int*)(assistant+0x1a0) += 1` — the rawCount counter. The size rule (D:330974-330999): reads the node's FS-code field (`vt+0x100`); **code 0x23 — the ctor default in DRWUI and every SearchDLL — selects the `size ≤ 0x77fffff` (~120 MiB) arm**; any other code selects `size ≥ one cluster` (the divisor at global+0x70). The counter is surfaced in the recoverability log (`rawCount` at D:330915-330916, byte +0x1A0) and feeds the assistant statistics block. Population semantics: rawCount = "carved-family files whose content signature was deep-read-confirmed and whose size fits the FS arm" — a quality-validated subset, NOT the raw carve census.

## What our core does

`cr_grading.cpp:365` ticks `++out.raw_count` for every node with `Origin::Carved` — the header comment (`cr_grading.h:117`) registers this as the documented mapping. The report (`cr_report.cpp:170`) and both ABI surfaces carry the field. The M-134 landing added the deep-read machinery (`deep_read_grade`, `GradeReader`, the graded one-shot bit) but NO counter wired to its return value: the forced-probe population, the size-rule arms, and the +0x1a0-style increment have no core counterpart, and the resolution's stated `raw_tail_zero` AssistantStats member does not exist (grep-verified zero hits repo-wide at HEAD).

## Gap

Stated falsifiably: a quick scan of 500 NTFS grade-1 nodes where 60 pass the forced tail-zero probe with sizes ≤ 0x77fffff produces og rawCount = 60; the same scan in the core produces raw_count = (count of Origin::Carved nodes) — 0 if nothing was carved, or thousands if the carve stage ran — with no deep-read gating and no size arm. Separately: `grep -r raw_tail_zero src/ include/` = 0 hits, contradicting the M-134 Resolution's "now has its input" sentence.

## Impact

The report/ABI `raw_count` numbers diverge from the og's semantics on every scan that reports them (the og: a validated-recoverable count over the quick-scan NTFS population; the core: a provenance census over the carve population). The phantom resolution claim weakens the audit trail: a future session reading M-134's resolution would believe the N-183 consumer has its input and skip the work. MINOR (a statistics-field semantic + a bookkeeping-truth defect; the grade values themselves are owned by the landed rules).

## Reproduction / verification

Og-side: all five citation regions read first-hand this session (the forced-probe walk, both increment sites, the size rule's two arms with the 0x23 constant, the +0x1A0 log read, the ctor defaults across all eight SearchDLLs). Core-side: the Origin::Carved tick at `:365`, the header mapping at `:117`, the report/ABI consumers, and the `raw_tail_zero` zero-hit grep executed at `9633d94f`. Worksheets: `evidence/s55-lane-sheets/S55-D3-grading.md` §a.5 (the forced-probe decode) + §c (the rule matrix row) + the DC-1 candidate.

## Fix plan (for the engineering team)

1. Add `u64 raw_tail_zero = 0;` to `AssistantStats` (the field the M-134 resolution already claimed) and tick it exactly where the og does: at the scan's completion pass, for grade-1 NTFS-family nodes with `0 < key < device-size`, when `deep_read_grade` returns true AND the size rule passes (FS-code default arm: size ≤ 0x77fffff; the core's FS-code model: the CandidateFs-NTFS arm — document the mapping).
2. Decide and REGISTER the `raw_count` semantics: either (a) re-point `raw_count` at the og's population (breaking the report field's meaning — needs the ABI doc updated), or (b) keep Origin::Carved and update the header comment + report docs to state the deliberate divergence with the og's rawCount explicitly (the D-row). Option (b) is the smaller change; option (a) is the parity-true one.
3. Repair the M-134 Resolution text (or add a correction addendum in the FIXED file): the `raw_tail_zero` sentence was aspirational, not landed — cite this finding.
4. Tests: `Grading.RawTailZeroTicksForcedProbePopulation` (a grade-1 NTFS node with a zeroed-tail final sector and size ≤ 0x77fffff ticks; a same-size node with nonzero tail does not; a >0x77fffff node does not); `Grading.RawCountSemanticsRegistered` (pins whichever option step 2 chose).

## Tests to add

- `Grading.RawTailZeroTicksForcedProbePopulation` (fails today)
- `Grading.RawTailZeroSizeArmBoundary` (0x77fffff in/out, fails today)
- `Grading.RawCountSemanticsRegistered` (fails today — the mapping is comment-only)

## Related findings

N-183 (active — the gauge/QSDE accounting family this counter feeds), M-134 (FIXED — the deep-read landing whose resolution carries the phantom claim), M-146 (active — the probe-family gate mismatch in the same walk), S-058 (active — the ext-table polarity in the same pass).

## Open questions

None — the og's `vt+0xb8()` gate and the FS-code `0xa8` value interact with this walk (see OQ-S42-3's S55 addendum), but the rawCount increment arm itself is fully decoded and does not depend on either gate.

## Resolution

**Status:** FIXED
**Fixed in commit:** (the S60 lane's N-205 landing — raw_tail_zero + D-146 + the M-134 repair; SHA in the worklog entry and the ledger row)
**Verified by:** Grading.RawTailZeroTicksForcedProbePopulation + Grading.RawTailZeroSizeArmBoundary + Grading.RawCountSemanticsRegistered (the registration leg RED until the D-row landed [D-146 post-renumber] — the executed fail output named the missing row; the counter legs could not compile against the pre-fix tree, the field's absence being the finding's own phantom claim, re-verified by grep at the lane's open); the full Grading family 40/40.
**Resolution notes:** Corpus re-verified first-hand before the change: FUN_1401c7d80's forced-probe walk (DRWUI:331183-331311), both increment arms (:331278/:331289), the size rule's two arms with the 0x23 ctor default (FUN_1401c76e0, DRWUI:330974-330999), the +0x1a0 counter read (:330915-916), and the ctor-default census across the SearchDLLs — all as filed. The landing, per the fix plan's steps: (1) `u64 raw_tail_zero = 0;` in AssistantStats, ticked exactly where the og ticks — grade_tree's new pass 0 (the quick-scan-populate analog, before the solo rules mutate grades — the og's walk sees the NTFS population still grade 1, no insert-time hook grades NTFS): NTFS-family nodes, grade Average, `0 < extent-end < device-bytes`, `forced_tail_zero_probe` true (the observe-only port of FUN_14038c470's param_3=1 form — the same final-sector tail-zero read as deep_read_grade but with no grade mutation and no twin-key index gate, the forced call passing the address directly), AND `size <= 0x77fffff` (the 0x23 default arm; the core's NTFS nodes map to it — CandidateFs has no per-node FS-code twin, documented at the tick site); (2) option (b) adopted: raw_count keeps the Origin::Carved census — the registered deliberate divergence is docs/PARITY-DELTAS.md **D-143** (the header's counter-mapping block now states the split where it silently asserted a mapping), NOT option (a)'s mid-stream re-pointing of a shipped report field; (3) the M-134 Resolution's phantom "raw_tail_zero in AssistantStats" sentence repaired by a correction addendum in findings/FIXED/M-134-grading-deep-read-validation-absent.md (the claim was aspirational at its landing; this landing makes it true); (4) the ABI surface deliberately untouched — the C struct keeps its shipped field set (a memset-with-new-size on an old-consumer allocation is the classic append hazard; the D-row records the future-bump path). The disjointness of the two populations is pinned by the third test (a carved node ticks raw_count only; the tail-zero NTFS node ticks raw_tail_zero only).
