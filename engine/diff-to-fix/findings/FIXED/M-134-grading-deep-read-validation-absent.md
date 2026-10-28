# MAJOR M-134 — the og's deep-read grade validation (sector-content grading) and full-tree re-grade are absent; the core re-grades solo rules only

**Severity:** MAJOR
**Module:** grading / query (recovery-chances focus battery)
**Parameters affected:** #15 Confidence grading model; #32 Cancellation (bounded reads); #9 detection correctness (grade column)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:606291-606419` (`FUN_14038c470`, the deep read — decoded this session); `:605743-605803` (the completion-pass call site + the "graded" bit); `:340852-340873` (`FUN_1401d7110`, the per-selection re-grade calling the FULL per-node tree); `:324935` region (the address-map lookups the tree uses)
**Core references:** `src/core/tree/cr_grading.cpp:6-9` (the registered non-adoption: "deep validation read: NOT ADOPTED"), `:337-346` (`regrade_node` re-runs `solo_rules` only), `:228-335` (`grade_tree` has no reader in `GradeContext`); `include/cr/tree/cr_grading.h:24-27` (the UNDECODED registration — now decoded by this finding) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's grading engine contains a rule the core deliberately does not have (registered "UNDECODED, not adopted"): a **content read** that flips a file's grade on actual disk bytes — a zeroed or 0xFF-filled first sector forces grade 0, and a previous-sector tail of 16 zero bytes forces grade 2 — plus a per-selection re-grade that re-runs the **entire** per-node decision tree (twin map + table gate + deep read), not just the solo rules. This session decoded `FUN_14038c470` structurally, making the rule portable for the first time; the core's `regrade_node` can never produce an overlap flag or a High-from-read, and its `grade_tree` cannot read the disk at all.

## What the reference does

`FUN_14038c470(node, addr, param_3, param_4)` (`:606291-606419`, read first-hand this session):

1. Skips if the node is null or the overlap flag is already set (`:606318`).
2. Resolves a reader through the global manager chain (`FUN_1401b54f0/4fd0/50f0`, `:606319-606331`); `sectorSize = vt+0x28()` (`:606353`); default grade 1 (`:606354`).
3. `param_3 == 0` gate: a three-part pre-check (`FUN_1401bc5a0(mgr,node,addr)` ∧ the bit-7-of-+0x72 flag ∧ NOT the bit-2-of-+0x74 deep-read-skip bit) → grade 0 without reading (`:605355-361` region per the call-site decode; sub-gate meanings not fully decoded — see OQ-S42-3).
4. **addr == 0**: read 1 sector at offset 0 — **first 32 bytes all-zero → grade 0**; **first 4 bytes 0xFF → grade 0** (`:606364-606392`).
5. **addr > 0**: read 1 sector at `addr−1` — **last 16 bytes (sectorSize−0x10) all-zero → grade 2** (param_3==0) / set the "raw" flag (param_3==1) (`:606393-606411`).
6. `set_grade(final)` + mark-graded (`:606416-606417`); returns the previous-sector-tail-zero bool — consumed by the assistant's rawCount rule (`:331283`).

The completion pass invokes it at `:605798` after the twin check fails; the **per-selection re-grade** (`FUN_1401d7110`, `:340852-340873`) — gated on grade==1 ∧ "graded" bit clear — re-enters the **full** per-node tree `FUN_14038ac90` (`:605686-605815`), which includes the extension gate, the twin-map lookup (which can set another node's grade), and this deep read. The og's re-grade can therefore change a node's grade **based on disk content** when the user selects/previewes it.

## What our core does

- `cr_grading.cpp:6-9` (header of the file): the deep validation read is registered as decoded-but-not-adopted; `GradeContext` carries only `devices` (base/ordinal/kind) — no reader, no sector size, no read path.
- `regrade_node` (`:337-346`) re-runs `solo_rules` only — no table gate, no twin map, no deep read — and has no "graded"-bit tracking (re-grade eligibility = grade==1 only; the og additionally requires the graded bit clear, making its re-grade a true one-shot).
- `grade_tree` (`:228-335`) is a pure tree pass; it never touches the device.

## Gap

Stated falsifiably: for a deleted file whose first extent's on-disk bytes are zeroed (overwritten content), the og's grading flips the node to Low when the deep read runs; the core leaves it Average (or whatever the solo rules gave) regardless of content. For a file whose preceding sector ends in 16 zero bytes, the og grades High; the core never can. On re-selection, the og re-runs twins + deep read; the core re-runs only the solo rules.

## Impact

The grade column can disagree exactly in the cases users care most about — visibly-overwritten files that the og demotes to Low and the core leaves Average (users may burn recovery time on zeroed files), and tail-zeroed files the og promotes to High. The recovery-chances focus battery names this engine; the divergence is MAJOR (architectural: the grading pass needs a reader + the two threshold constants), filed below severe because the affected population is the not-in-table extension set after S-058's fix and the grade flip is one of three verdict values.

## Reproduction / verification

- **Decompiled evidence (first-hand):** the full `FUN_14038c470` walk at `:606291-606419`; the call site at `:605798`; the re-grade gate at `:340852-873`; the mark at `:605800`. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-grading-gate-polarity.md` (§5).
- **Core evidence (first-hand):** the non-adoption registration at `cr_grading.cpp:6-9` + `cr_grading.h:24-27`; the solo-only re-grade at `:337-346`.

## Fix plan (for the engineering team)

1. Extend `GradeContext` with an optional bounded reader: `struct GradeReader { IStorageDevice* dev; u32 sector_size; }` (the scan already owns the device + slice basis; thread it at `cr_scan.cpp:1220-1234`'s `grade_tree` call).
2. Implement `deep_read_grade(node, addr, reader)`: addr==0 → 1-sector read, first-32-zero → Low, first-4-0xFF → Low; addr>0 → 1-sector read at addr−1, last-16-zero → High; default no change; respect the skip bits (overlap set → no read). Bounded: one sector per node, only for the S-058 population (not-in-table), only when the twin check failed.
3. Add the "graded" bit to the node (or reuse an nflags bit) as the re-grade one-shot gate; `regrade_node` grows to re-run the table gate + twin lookup + deep read (the full `FUN_14038ac90` shape) instead of `solo_rules` only.
4. Resolve OQ-S42-3's sub-gates at fix time (the `FUN_1401bc5a0` pre-check trio and `vt+0x28`'s sector-size source) — the structure allows shipping with the reads unconditional first and the gates in a follow-up, but the finding's acceptance requires the two threshold rules themselves.
5. Tests (below). Acceptance: a fixture with a zeroed first extent shows Low after grading in the core (og-parity); a tail-zeroed predecessor shows High; the re-grade path changes a grade on a second call only when the graded bit is clear.

## Tests to add

- `Grading.DeepReadZeroedFirstSectorGradesLow`
- `Grading.DeepRead0xFFHeadGradesLow`
- `Grading.DeepReadPrevSectorTailZeroGradesHigh`
- `Grading.RegradeRunsFullTreeOnce` (the graded-bit one-shot; fails today — re-grade re-runs forever, though idempotent)
- `Grading.DeepReadSkipsOverlapFlagged`

## Related findings

- **S-058** — the gate polarity (defines the population that reaches this rule).
- **M-138** — the twin-key family set (the twin check that precedes the deep read; renumbered from M-133 at the N-160 doc-truth repair).
- **N-183** — the rawCount consumer of this rule's return value.

## Open questions

The three sub-gates inside the deep read (`FUN_1401bc5a0`'s predicate, the bit-7-of-+0x72 and bit-2-of-+0x74 flags, and the `vt+0x28` sector-size source) — tracked as **OQ-S42-3** with the exact next-probe sites.

## Resolution

**Status:** FIXED
**Fixed in commit:** `9455f50a` (the S47 MAJOR-first fix lane)
**Verified by:** Grading.DeepReadZeroedFirstSectorGradesLow · Grading.DeepRead0xFFHeadGradesLow · Grading.DeepReadPrevSectorTailZeroGradesHigh · Grading.DeepReadSkipsOverlapFlagged (the twins keep their High, the unflagged solo takes the content rule) — all four RED on the neutered-rule rebuild (the pre-fix semantics: no content rule could ever fire; the API-extending tests proven by stubbing the rule body to the pre-fix no-op, the same evidence class as the stashed-engine proof) · Grading.RegradeRunsFullTreeOnce (the graded-bit one-shot: first call true + bit set, second call false) · Grading.DeepReadReaderAbsentChangesNothing (the structural back-compat pin: no reader -> no content rule, the ABI session path) — the full suites 1244/1244 + 71/71 (the one interaction found — the M-115 tiny-PBM test asserting an absolute grade — was re-isolated to a before/after comparison: the M-134 prev-sector rule legitimately promotes that fixture at completion, and the M-115 claim is about the thumbnail surface only).
**Resolution notes:** The fix plan executed with its own stated allowances. (1) `GradeReader { IStorageDevice* dev; u32 sector_size; }` added to `GradeContext`, threaded at the scan's completion pass (`cr_scan.cpp` — the run() device the extents are rebased to; the og resolves its reader through the global manager chain, FUN_14038c470 :606319-331). (2) `deep_read_grade` implements the two decoded threshold rules exactly (addr==0: 1-sector read at 0, first-32-all-zero -> Low, first-4-0xFF -> Low [:606364-392]; addr>0: 1-sector read at addr-1, last-16-all-zero -> High [:606393-411]; the overlap-flag skip [:606318]; read-failure -> no change), invoked at grade_tree's pass 2 for the exact og population — the not-in-table nodes whose twin check failed (the :605798 call site), one bounded sector per node. (3) The graded one-shot bit on FileNode (runtime-only by design — its purpose is per-selection one-shot semantics, and a reloaded session re-running the idempotent rules once more matches the og's observable behavior on unchanged content); `regrade_node` grew the full per-node tree shape — solo rules, the S-058 table gate, the twin lookup (with the tree parameter; the og's FUN_14038ac90 can set ANOTHER node's grade through the twin map — the core's regrade now does too), and the deep read — gated on grade==1 && graded-bit-clear. (4) OQ-S42-3's sub-gates (the FUN_1401bc5a0 pre-check trio, the bit-7/bit-2 flags, the vt+0x28 sector-size source) remain unadopted per the fix plan's explicit allowance ("the structure allows shipping with the reads unconditional first... the finding's acceptance requires the two threshold rules themselves") — OQ-S42-3 stays the tracked owner of the trio decode. The registered seam: the session-regrade ABI (device-decoupled by design) runs the full rule set without the content half — the deep read's reader exists only where a device handle does (the completion pass); threading a device through the session ABI is the recorded follow-up shape. The og's rawCount return-value consumer is N-183's surface, untouched. No safety hardening affected; no benchmark (grading-observable, not perf-tagged).
**Fixed in commit:** (this session's M-134 landing, session-52 lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Grading.DeepReadPrevSectorTailZeroGradesHigh` / `DeepReadZeroedFirstSectorGradesLow` / `DeepRead0xFFHeadGradesLow` / `RegradeRunsFullTreeOnce` (all four RED pre-fix — the core never read the disk and the re-grade was solo-rules-only; GREEN post-fix), `Grading.DeepReadNonZeroTailStaysAverage` (the control), `Grading.DeepReadSkipsOverlapFlagged` (the og's first-skip pin), `Grading.DeepReadFat16SkippedLikeOgFailedRead` (the failed-read class pin — the shifted kinds' keys land outside any reader in the og, the read fails, the grade never changes; the core skips the same population). Suite 1379/1379 (linux-release).
**Resolution notes:** FUN_14038c470 re-read first-hand in full (the function, the completion-pass call site :605798, the re-grade gate :340852-873) plus the twin-key arithmetic at the call site — the deep read's `addr` parameter IS the node's twin key (extent end + shifted index · unit, computed at :605774 for the map lookup and reused for the read), which settles the address semantics the finding left ambiguous: the read at (key − 1) is the extent's LAST sector; the key==0 path (no extent) reads the reader's sector 0. The port: `GradeReader` (a plain-C sector callback — test-wireable, no device-type dependency) on GradeContext, wired at the orchestrator's grade_tree call with the scan's own slice and geometry-derived sector size; `deep_read_grade` in cr_grading.cpp implementing the two threshold rules (first-32-zero / first-4-0xFF → Low; last-16-zero of the final sector → High) with the overlap skip, the null-reader bail, and the nonzero-index failed-read class all matched to the og's control flow; pass 3 in grade_tree runs it for unflagged S-058 candidates; the "graded" bit (a new in-memory FileNode bit, `deep_read_done`) marks exactly where the og marks — not for flagged nodes, not on the null-reader bail, yes on failed reads; `regrade_node` now re-enters the full per-node path (solo rules + the S-058 table gate + the deep read) behind the og's grade==1 ∧ graded-bit-clear one-shot gate. The twin-map re-lookup on re-grade is documented-skipped: the og's persistent address map can only re-discover collisions the completion pass already flagged on a stable tree (the flag survives); rebuilding the map per selection would be cost without behavior. The pre-check trio (FUN_1401bc5a0 ∧ bit-7-of-+0x72 ∧ ¬bit-2-of-+0x74 → presumptive grade 0 without reading) stays unadopted per OQ-S42-3 — the read-based rules are the evidence path; the OQ is updated with this session's control-flow decode (the trio's goto target and the mark placement). The N-183 rawCount consumer now has its input (`raw_tail_zero` in AssistantStats).
