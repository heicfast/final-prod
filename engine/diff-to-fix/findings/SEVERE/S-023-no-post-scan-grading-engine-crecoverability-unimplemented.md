# SEVERE S-023 — No post-scan grading engine: the og's CRecoverability "Recovery Chances" model has no core counterpart, and the core's confidence model is a different decision system

**Severity:** SEVERE
**Module:** orchestration / tree / abi (new module rows: query, preview — see modules/orchestration.md §grading)
**Parameters affected:** P15 (confidence grading model), P13 (report schema — per-category/scan statistics), P5 (deleted-file classification — adjacent), P11 (node fields — grade storage), P38 (progress — grading pass telemetry)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — FUN_14038ac90 (:605686), FUN_14038aad0 (:605634-605679), FUN_14038b0e0 (:605879-605920), FUN_140089630/140089670/1400895b0 (:134188-134213, :134149-134166), FUN_1401d7110 (:340852-340873), FUN_1401c6f10 RecoveryAssistant (:330758-330960), FUN_14039aa40 category mapper (:614863-614904), FS-string map (:955212-955227); `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:39242/39779` (engine default grade); `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` (seed, 2 glosses corrected this session); `og-codebase/log/DRWUI.log` (runtime telemetry)
**Core references:** `src/core/orchestrator/cr_scan.cpp` (no post-scan grading pass — only `tree_.refresh_stats()` + aggregate `ScanStats` counters; grep "grade|recoverab|assistant" → 0 hits), `include/cr/tree/cr_node.h:37-45` (Confidence 5-tier, $Bitmap model "R2.4"), `src/core/query/cr_query.cpp:112` (`confidence_min` predicate over the 5-tier ordinal), `src/core/recovery/cr_report.cpp` (no per-category/grade statistics), audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og computes every "Recovery Chances" verdict in a DRWUI-side background pass (CRecoverability) that runs at scan completion and re-runs per selection: a 3-grade model {0 Low, 1 Average, 2 High} stored monotonically in bits 5-7 of a node flag byte, with a precise decision tree (size floor, category fast path, extension-table-gated deep validation, NTFS device-index rule, small-file rule, twin/overlap rule, existing-file fast grade, small-result heuristic). The core has **no post-scan grading stage at all**: its 5-tier Confidence is set per-engine during the walk (a different decision system with different inputs and boundaries), no monotonic grade storage exists, no per-selection re-grading API exists, and the RecoveryAssistant statistics engine (8 counters + 6 per-category counts, own log channel) has no counterpart. This also resolves tracked-open OQ-S2M-1 (the og's grading locus).

## What the reference does

(Every quote below is verbatim from the decompile; consolidated in
`evidence/decompiled-quotes/s4-crecoverability-grading-engine.md`.)

1. **Engines emit grade=1 only.** The only engine-side grade writes are the node-ctor
   defaults (`NTFSSearchDLL_decompiled.c:39242/39779`: `*(u8*)(node+0x73) = 0x20`). Zero
   grade/quality/confidence strings in any of the 9 engine DLLs. **All 0/2 verdicts are
   DRWUI-side** — this resolves OQ-S2M-1 definitively.
2. **Grade storage**: bits 5-7 of node+0x73 (low 5 bits = flags). Setter FUN_140089630 is
   **monotonic-max** — a grade can never be downgraded, and grade 2 is sticky (the
   grade-0 branch refuses to clear a 0x40 node). Companion FUN_1400895b0 sets bit 2
   (overlap flag) and forces grade ≥ 2.
3. **Decision tree** (FUN_14038ac90, plus the two background jobs queued by
   FUN_14038aad0 after logging "Recoverability analyze begin."):
   - size < 1 → grade 0 (byte floor, not 0);
   - category byte == 0xC → grade 2 (guarded by grade != 2);
   - FS ∉ {3,10,0xB} (NTFS/ReFS/Btrfs) → generic path: extension extraction + overlap
     tree insert (deep validation only for extensions in a **169-entry static table**
     starting with key "Raw");
   - NTFS device-walk job (gated on FS==3): device index ∈ [0,9) → grade 2;
     vt+0xb8 != 0 or vt+0x100 == 0xa8 → grade 0; size < 0x400 → grade 2 else grade 0;
   - twins: two nodes resolving to the same extent-end key → both flagged, new node
     grade 2;
   - existing-file bit (bit 6 of +0x72) → grade 2 at insert if size ≥ 1;
   - **per-selection re-grade**: FUN_1401d7110 re-runs the full tree for grade-1 nodes
     when the user selects/previews them;
   - **small-result heuristic**: total < 100 → matched forced 0 (the gauge never shows
     for tiny results); matched/total composition feeds the
     "Quick scan matched=%d, total=%d" / "Recovery ability matched=%d, total=%d"
     telemetry.
   - the "−2" in the seed artifact is an **extent-address offset** for device-kind
   ∈ {1,4} (FAT/exFAT), not a grade adjustment (two verbatim sites :605754, :605924).
4. **RecoveryAssistant** (FUN_1401c6f10): separate `RecoveryAssistant.log` channel;
   "analyze begin/finish" + GetTickCount64 elapsed → +0x38; 8 counters (TotalFileCount,
   rawCount, lostPartitionExist/DeleteFileCount, QuickScanExist/DeleteFileCount,
   DeepScanFileCount, QuickScanDeleteExtend); per-category counts at IDs
   5/6/7/8/9/0xB (Graphics/Document/Archive/Video/Audio/Other).

## What our core does

- `src/core/orchestrator/cr_scan.cpp` — there is **no post-scan grading pass**: the scan
  completes with `tree_.refresh_stats()` and aggregate `ScanStats`
  (files/deleted/carved/lost/bytes). No "Recoverability analyze" equivalent, no
  background grading jobs, no per-selection re-grade hook anywhere in the ABI.
- `include/cr/tree/cr_node.h:37-45` — `Confidence` is a 5-tier enum
  (Unknown/Excellent/PartialRisk/Overwritten/MetadataOnly) computed **per-engine during
  the walk** from $Bitmap-style extent allocation checks ("R2.4"). It is a different
  decision system: different inputs (allocation vs category/device/overlap/extension),
  different boundaries (5 ordinals vs 3 grades), different locus (in-walk vs post-scan),
  different lifecycle (fixed at emit vs monotonic + re-graded on demand).
- `src/core/query/cr_query.cpp:112` — `confidence_min` filters over the 5-tier ordinal;
  no grade model exists for consumers to consume.
- `src/core/recovery/cr_report.cpp` — the report carries queue/totals statistics only;
  no per-category counts, no scan-phase statistics (quick/deep/lost/raw counts), no
  grade distribution.

## Gap

Falsifiable claims:
(a) The og exposes a 3-grade "Recovery Chances" verdict per file, computed post-scan and
re-computed on selection, with monotonic sticky storage — the core exposes none of this
(any API consumer asking "how recoverable is this file?" gets the in-walk 5-tier
confidence, which answers a different question with different semantics).
(b) The og computes and surfaces 8 scan-statistics counters + 6 per-category counts at
scan completion (own log channel + the celebration window) — the core computes neither
per-category statistics nor phase-decomposed counters.
(c) The og's small-result heuristic (total < 100 → matched = 0) has no counterpart.

## Impact

- **API consumers** (the cr_ui surface is the product's front door): no way to produce
  og-equivalent "Recovery Chances" UI; a host building the results screen on the core
  cannot match the reference product's headline feature.
- **Report/telemetry parity**: the recovery report and any statistics surface are missing
  the reference's post-scan analysis layer entirely.
- **Filter model interop**: M-023/M-029's category model depends on per-category counts
  in the og's results header; the core's query counts categories per-row at query time
  (O(n) per query) instead of having the assistant's precomputed map.
- The in-walk 5-tier Confidence is *not wasted work* — it is a superset on the
  allocation axis — but it is not a substitute: the og's model adds category, device,
  twin/overlap, extension-table and small-result inputs the core never consults.

## Reproduction / verification

- **Decompiled evidence** (method 2): every quote above extracted this session by the
  independent-verification leg from `ghidra-drwui/DRWUI_decompiled.c` /
  `NTFSSearchDLL_decompiled.c`; consolidated with two-agent convergence (the corpus-
  artifact leg independently corrected the same two seed-artifact glosses). Stored at
  `evidence/decompiled-quotes/s4-crecoverability-grading-engine.md`.
- **Corpus documentation evidence** (method 3):
  `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` §1-§2 (seed —
  read before the decompile pass; its §1 "ZFS special-case" and "grade adjustment −2"
  glosses were corrected by the decompile, see CORPUS-ERRATA).
- **Core-side verification** (grep, executed): `rg -i "grade|recoverab|assistant"
  src/core/ include/cr/` → no post-scan grading surface; `cr_scan.cpp` completes into
  `refresh_stats()` + `ScanStats` only.
- Not verified by og-execution (Windows UI binary; the corpus log `og-codebase/log/
  DRWUI.log` corroborates the runtime telemetry strings).

## Fix plan (for the engineering team)

1. **Architecture**: add a post-scan grading pass to the orchestrator (a
   `grade_tree()`-style stage between scan completion and session save), plus a
   re-grade entry point on the ABI for per-selection refinement. Keep the existing
   in-walk Confidence untouched (it is a superset on the allocation axis); add the og's
   3-grade model as a separate, monotonic field.
2. **Files to modify**: `include/cr/tree/cr_node.h` (new grade field: 2 bits, monotonic
   setter semantics — never downgrade, grade-2 sticky), `src/core/tree/cr_node.cpp`
   (setter), `src/core/orchestrator/cr_scan.cpp` (invoke the grading pass at completion,
   after `refresh_stats()`), new `src/core/tree/cr_grading.cpp` (the decision tree),
   `include/cr/cr_ui.h` + `src/ui/cr_ui.cpp` (expose `cr_ui_node_grade()` +
   `cr_ui_scan_regrade(node_ids)`), `src/core/session/cr_records.cpp` (persist the grade
   bits), `src/core/recovery/cr_report.cpp` (assistant statistics block).
3. **Algorithm** (in priority order, from the decompile): size<1→0; category-0xC→2
   (guarded by !=2); generic path for FS ∉ {NTFS,ReFS,Btrfs}: extension-table gate
   (169-entry table — port the table from the .rdata at 0x141bb0500) + overlap-tree
   twin detection (extent-end key `base+len+idx*unit`, device-kind ∈ {1,4} applies −2
   to the INDEX, not the grade); NTFS device-walk: device idx ∈ [0,9)→2, vt-flags → 0,
   size<0x400→2 else 0; existing-file bit→2; small-result heuristic total<100→matched=0;
   per-selection re-grade for grade-1 nodes only.
4. **Statistics**: port RecoveryAssistant — 8 counters + 6 category counts
   (IDs 5/6/7/8/9/0xB), written to the report schema and (optionally) a log channel.
5. **Tests to add** (unit): grade monotonicity (2 never downgraded by 0); sticky-2;
   size<1→0; category-0xC fast path; extension-table gating (in-table vs not);
   small-file <0x400→2/else 0; twin-address → both flagged; re-grade API on grade-1
   only; assistant counters on a mixed fixture (carved+lost+quick+deep); session
   round-trip preserves grade bits; report contains the statistics block.
   (Integration): a FAT16/NTFS fixture where the in-walk Confidence and the og-model
   grade differ, asserting both are present and independent.
6. **Contract/doc updates**: ABI-CONTRACT.md gains the grade API + report statistics
   section; CAPABILITY-MATRIX row for "Recovery Chances grading: core/og parity".
7. **Acceptance criteria**: (a) an API consumer can reproduce the og's 3-grade verdicts
   for the fixture set (grade sets compared against the decision tree, not og
   execution — mark UNVERIFIED-BY-EXECUTION until a Windows bench leg runs the og);
   (b) report contains the 8 counters + 6 category counts; (c) 306/306 suite stays
   green + the new unit tests; (d) no change to existing Confidence semantics (pin
   tests stay green).

## Tests to add

`Grading.MonotonicMaxNeverDowngrades`, `Grading.Grade2Sticky`, `Grading.SizeFloorIs1`,
`Grading.Category0xFastPath`, `Grading.ExtensionTableGatesDeepValidation`,
`Grading.SmallFileUnder1024High`, `Grading.TwinAddressFlagsBoth`, `Grading.RegradeOnlyGrade1`,
`Grading.AssistantCountersMixedFixture`, `Grading.SessionRoundTripPreservesGrade`,
`Report.ContainsAssistantStatistics`, `Abi.NodeGradeAndRegradeLifecycle`.

## Related findings

- **OQ-S2M-1** (tracked-open) — RESOLVED by this finding's evidence (engines emit
  grade=1 only; verdicts are DRWUI-side).
- **M-023 / M-029** — the category model the assistant counts is the same 8-category
  space those findings cover; the grading fix should share the category source.
- **N-030** — raw-sign "confidence" is dead data in the og; do not conflate it with
  this grade model (different field, different consumer).

## Open questions

1. The 169-entry extension table's exact contents (the .rdata blob at 0x141bb0500 was
   located but not dumped this session) — the fix leg must dump it before porting;
   until then the extension-gate rule cannot be byte-matched. Tracked as
   `tracked-open/OQ-S4-1-grading-extension-table.md`.
2. The vt+0xb8 / vt+0x100 == 0xa8 conditions on the NTFS device walk (which device
   properties they read) — decode needed for exact reimplementation; the structural
   rule (nonzero → grade 0) is pinned.
3. Og-execution verification of grade outputs (needs the Windows bench leg; the corpus
   DRWUI.log telemetry strings corroborate the model but not per-file values).

---

## Session-5 addendum (2026-10-26) — fix-plan step 3 input complete: the 169-entry extension table is dumped

OQ-S4-1 is RESOLVED this session (tracked-open updated). The deep-validation
extension gate's exact entry set — the 9-byte-stride static table at
`.data` 0x141bb0500 in DRWUI.exe — is dumped in full at
`evidence/decompiled-quotes/s5-oq-s4-1-extension-table-dump.md` (entries
0..168, first "Raw", last "COMMON"; corrections: the table is in `.data`
not `.rdata`, and it is unsorted build order — the runtime map sorts).
Step 3 of this finding's fix plan can now be executed verbatim against the
dumped list; the extension-gate test should pin membership for
representative entries per family (documents DOCX/PDF, photo raws NEF/CR3,
video MP4/MKV, archives ZIP/7Z, and the "Raw"/"COMMON" sentinels).
