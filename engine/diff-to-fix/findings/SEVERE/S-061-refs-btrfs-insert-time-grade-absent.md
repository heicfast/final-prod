# SEVERE S-061 — ReFS and Btrfs files never receive the og's insert-time High grade: the whole-filesystem "Recovery Chances" column diverges

**Severity:** SEVERE
**Module:** grading / query (recovery-chances focus battery)
**Parameters affected:** #15 Confidence grading model; #9 detection correctness (grade column); #13 report schema (grade byte)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:605481-605497` (the insert-time hook's device-vtable branch: kind 0xB → grade 2, kind 10 → grade 2, with the 5830/5828 counter ticks); `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md:33-52` (the insert-hook architecture); `:605736-605737` (the completion pass's exclusion of FS 3/10/0xB from the generic path — consistent: insert already graded them)
**Core references:** `src/core/tree/cr_grading.cpp:217-219` (`fs==Refs || fs==Btrfs → return true` — no grade set); `:207-216` (the NTFS rule that IS implemented); `include/cr/tree/cr_grading.h:20-24` (the registered non-adoption note); `tests/unit/test_grading.cpp` (no ReFS/Btrfs grade test exists) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

At tree-insert time the og's CRecoverability hook grades every non-existing ReFS and Btrfs file **High (grade 2)** unconditionally (with gauge-counter ticks) — these two filesystems are excluded from the completion pass's generic path precisely because insert already graded them. The core's `solo_rules` returns early for ReFS/Btrfs **without setting any grade**, leaving every ReFS/Btrfs file at the engine-default Average. The entire ReFS and Btrfs population therefore shows a different "Recovery Chances" verdict in the core than in the og.

## What the reference does

The insert-time hook `FUN_14038a470` (read first-hand at `:605434-605541`; tree-verified this session) gates on non-existing nodes (bit 6 of +0x72 clear). In its param_3==0 path, it fetches the node's device/engine object (`FUN_140088570`) and its FS kind via `vtable+0xa0`:

```c
605484    iVar2 = (**(code **)(*local_90 + 0xa0))(local_90);
605485    if (iVar2 == 0xb) {                       /* Btrfs */
605486        FUN_140089630(local_res10,2);         /* grade 2 (High) */
605487        DAT_141be5830 = DAT_141be5830 + 1;    /* gauge total   */
605488        DAT_141be5828 = DAT_141be5828 + 1;    /* gauge matched */
605489        goto LAB_14038a801;
605490    }
605491    iVar2 = (**(code **)(*local_90 + 0xa0))(local_90);
605492    if (iVar2 == 10) {                       /* ReFS */
605493        FUN_140089630(local_res10,2);         /* grade 2 (High) */
605494        DAT_141be5830 = DAT_141be5830 + 1;
605495        DAT_141be5828 = DAT_141be5828 + 1;
605496        goto LAB_14038a801;
605497    }
```

The email fast-path immediately above (`:605476-479`) shows the identical shape — the og treats "email", "Btrfs", "ReFS" as unconditional-High classes at insert. The completion pass (`FUN_14038ac90`) then excludes FS {3 NTFS, 10 ReFS, 0xB Btrfs} from the generic grading path (`:605736-605737`) — for NTFS because the device-walk thread owns it, for ReFS/Btrfs because insert already did it. The gauge counters (5830 total / 5828 matched) put these files in the recoverability gauge's numerator unconditionally.

## What our core does

`cr_grading.cpp:217-219`:

```cpp
if (fs == u32(FsKindCode::Refs) || fs == u32(FsKindCode::Btrfs))
    return true; /* excluded from the generic path (the og's device
                  * walk covers these; no further decoded rule) */
```

The comment's premise is now disproven: the og's **insert hook** covers these (not the device walk), with an unconditional grade-2 rule this session decoded. The core returns without touching the grade; ReFS/Btrfs files keep `Grade::Average`. No test in `test_grading.cpp` covers a ReFS or Btrfs node's grade (the file tests setter semantics, the table, twins, counters — never these two kinds), so the divergence is unpinned on both sides.

## Gap

Stated falsifiably: a deleted `notes.txt` on a ReFS volume shows grade **High** in the og's results (insert-time rule `:605492-496`) and **Average** in the core (early return, no grade set). Same for every Btrfs file.

## Impact

The recovery-chances column — the user's primary prioritization signal, surfaced in query rows, the ABI, the report, and the session grade byte — is wrong for the entire ReFS and Btrfs population. ReFS and LDM are first-class scope per the standing rules; Btrfs likewise. The gauge's matched/total inherits the missing numerator (the og ticks 5828/5830 for each such file). Filed severe per the session directive (recovery-chances status is a named focus area; verified diff → severe).

## Reproduction / verification

- **Decompiled evidence (first-hand):** `:605481-497` quoted above; the exclusion at `:605736-737`; the email twin-shape at `:605476-479` proving the pattern. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-grading-gate-polarity.md` (shared with S-058; §4 covers this rule).
- **Core evidence (first-hand):** `cr_grading.cpp:217-219` + the header's own "no further decoded rule" note at `cr_grading.h:20-27`; `test_grading.cpp` (absence of coverage verified by read).

## Fix plan (for the engineering team)

1. In `solo_rules` (`cr_grading.cpp`), replace the bare `return true` for ReFS/Btrfs with: `n->set_grade(Grade::High);` (+ the gauge/population accounting the email fast-path already uses — mirror `:605486-488`). Note the og gates this on **non-existing** (deleted-state) nodes at insert; existing files already grade High via the Active rule — order the ReFS/Btrfs rule after the Active check or make the tick conditional on `deleted_state() != Active` (both produce identical observable outcomes because Active→High precedes it in `solo_rules`'s current order — keep it simple: unconditional High after the Active rule, exactly like the email rule's placement).
2. Update `include/cr/tree/cr_grading.h:20-27`'s non-adoption note to the decoded rule with the `:605481-497` cites.
3. Tests (below); re-pin none (no existing test pins the wrong behavior — the gap was untested, not mis-tested).
4. Acceptance: a ReFS-fixture scan's tree shows High on deleted ReFS rows in the query rows + the session grade byte; same for a Btrfs fixture; the report's matched count includes them.

## Tests to add

- `Grading.RefsDeletedFilesGradeHigh` (fails today: Average).
- `Grading.BtrfsDeletedFilesGradeHigh` (fails today: Average).
- `Grading.RefsExistingStillHigh` (passes; guards the Active-rule precedence).

## Related findings

- **S-058** — the generic-path polarity (the same engine's other population divergence).
- **M-134** — the deep-read validation (the third leg of the same branch, for the FS families that take it).
- **N-183** — the gauge composition batch (the 5828/5830 ticks this rule feeds).

## Open questions

None.
