# MAJOR M-115 — the thumbnail grade-promotion trigger is mis-modeled: the og promotes on clean recovery-READ success (decode-independent, tier-gated, preview-pane-excluded); core promotes only on decode success — core under-promotes readable-but-undecodable files (OQ-S11-1 resolved by decode)

**Severity:** MAJOR (a grading-model divergence on a user-visible confidence signal: damaged-but-readable files grade lower than the reference grades them; the og's text-snapshot and media-frame promotion triggers have no core counterpart at all)
**Module:** preview/grading (the thumbnail → grade promotion seam)
**Parameters affected:** D15 (confidence grading — the promotion trigger's inputs), B2/D14-adjacent (the grading model's decision boundaries), A42 (the observable `cr_ui_query_row.grade`)
**Corpus references:** `ghidra-drwui/DRWUI_decompiled.c:422507-422514` — the guard, verbatim: `iVar2 = FUN_140089670(node); if ((iVar2 != 2) && ((local_167 != '\0' || (((local_168 == '\0' && (local_160 == 0)) && ((*(char *)(param_1 + 0x5c) != '\0' || (*(int *)(param_1 + 0x58) != 3)))))))) { FUN_140089630(node, 2); }`; binary-verified at `og-codebase/DRWUI.exe` VA `0x140263b2c-0x140263b9a` (grade==2 skip `cmp eax,0x2; je` @0x140263b3d; local_167 `[rsp+0x21]` @0x140263b42; local_168 `[rsp+0x20]` @0x140263b4b; local_160 `[rsp+0x28]` @0x140263b54; +0x5c @0x140263b5b; +0x58==3 @0x140263b6b; the whole guard inside the payload!=0 block @0x1402639c1); the decode chain: `:422433` local_168 = `FUN_1403148a0(file)` (TxtViewFiles membership, `:535890-535964`; the FilterFolder.ini `[Filter] TxtViewFiles` anchor per `artifacts/config-files/config-data-reference.md:39`); `:422442` local_160 = `FUN_1403eba50(r2m, ctx, &cancel)` — the recovery-read verdict (0 = clean success with data handle, −1 cancelled, −2 no data, −3 data-but-flagged-failed, `:664341-664359`); `:422447-422455` the text-snapshot render (`FUN_1403d7ee0` 200×200 GDI, `FUN_1403d88e0` first-min(100,size)-bytes Arial-13 → BMP-in-HGLOBAL, `:652885-652959`) setting local_167=1; `:422469-422471` the media-frame extractor path; `:422319-422340` the +0x5c flag = (Graphics-type ∨ fourcc∈{jpg,jpeg,tif,tiff} ∨ TxtView) ∧ category∈{1,2,4}; `:421503-421505` category literal 3 = "PreViewFile"; `:652280-652281` the cat-3 no-decode early return; `:421400-421402` the dispatcher's ≤0x14-size skip
**Core references:** audited `461825d8`-lineage — `src/ui/cr_ui.cpp:3737-3742` (the promotion: `if (node->grade() != tree::Grade::High) node->set_grade(tree::Grade::High)` on the `make_thumbnail` Ok path only), `:3694-3695` (`DecodeFailed → ERR_TRUNCATED` returns before the promotion), `:3754` (the comment cites ":422498-518" — the guard actually lives at :422507-422514), `:3753` (the promotion precedes both the probe return and the too-small-buffer return — fires on any successful decode); `include/cr/preview/cr_thumbnail.h:39-42` (text formats refused — the text-snapshot trigger impossible), `:45-46` (video key-frames deferred)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (S11-lead push-time reconciliation):** filed as M-112 by this lane; renumbered **M-115** — the parallel S11B lane's M-112 (the CSL 0x44c size-field finding, itself renumbered from their M-111 draft over the explorer-browse M-111) pushed first and keeps the ID per the SA-9 rule.

**Prior audit check:** OQ-S11-1 (posed in `evidence/s11-board-truth.md` §1 residual 2) is RESOLVED by this finding's decode — and the resolution inverts the hypothesis: the board-truth lane guessed the og's flags gate promotion to a node SUBSET (core would over-promote); the decode shows the gating is by TRIGGER (read-verdict + request-tier + payload), and the dominant core error is UNDER-promotion. M-083 (retired PARITY-VERIFIED at `e56baac7`) verified only the `!=2` + setter legs — its parity record is correct as far as it went; this finding owns the four undecoded gating conditions. Distinct from M-108 (preview-panel text eligibility — a different surface), M-109 (the 200 MB eligibility anchor), N-137 (the ABI header truth defect), N-142 (the S11B lane's dead CrPreview wiring — the rebase-leftover struct fields this finding's decode supersedes in provenance), and N-144 (the TxtView text-snapshot thumbnail surface — the render capability, filed separately from this grading trigger).

## Summary

The og's thumbnail worker promotes a file node's grade to 2 when its bytes were successfully READ for thumbnailing — the image decode happens later and its failure never retracts the promotion. Four gating conditions apply: a payload must exist, the read verdict must be clean (local_160==0) unless a text snapshot was rendered (local_167), the file must not be a TxtView-family file that failed the text render, and the request must be a list-tier thumbnail (not the category-3 preview-pane raw-data request). Core's implementation — landed as M-083 and parity-verified on the `!=2` leg — promotes only when the image DECODES successfully. Net: for damaged-but-readable images the og grades 2 (recoverable) and core grades lower; for TxtView text files and media files the og promotes via snapshot/frame triggers core does not have at all; and for sub-20-byte decodable images core promotes where the og never even creates a work item.

## What the reference does

`FUN_1402636e0` (the `ThumbnailFactory::ThumbnailWorkItem` body, ctor at
`:422303-422341`) runs: cancel/session gates → build recovery context → for
non-media files: size gate (200 MiB, `:422428` — M-109's anchor), lock,
`local_168 = isTxtView(file)`, then **`local_160 = FUN_1403eba50(...)` — the
recovery read** (`:422442`). On clean read (0) with a payload handed off, the flow
proceeds to `MemThumbnail::AttachData` where the DECODE happens — and the promotion
guard at `:422507-422514` has already fired by the read/payload evidence: `grade
!= 2 && (textSnapshotRendered || (!isTxtView && readVerdict==0 && (tierFlag ||
category != 3)))` → `FUN_140089630(node, 2)`. The TxtView branch renders the first
≤100 bytes to a 200×200 Arial-13 BMP (`:422447-422455`) and promotes if the render
is non-empty; the media branch extracts a frame (`:422469-422471`) and promotes on
payload. The preview-pane request (category 3, the "PreViewFile" task) NEVER
promotes through this path (the `!= 3` clause) and its `AttachData` returns before
any decode (`:652280-652281`). The dispatcher only creates work items for
image-family/TxtView/media files with size > 0x14 (`:421391-421403`). Every claim
above is decompile-cited and the guard is binary-verified
(`0x140263b2c-0x140263b9a`).

## What our core does

`cr_ui.cpp:3737-3742` promotes inside `cr_ui_thumbnail_generate` only after
`make_thumbnail` returns Ok: a `DecodeFailed` maps to `ERR_TRUNCATED`
(`:3694-3695`) and returns before the promotion line. Text formats are refused by
the engine matrix (`cr_thumbnail.h:39-42`), so no text-snapshot trigger exists;
video key-frames are a named deferral (`:45-46`). The promotion fires on every
successful decode including size probes and too-small-buffer calls (`:3753`
precedes both returns), for any node the caller names (no family gate, no ≤20-byte
skip). The in-code comment cites the og as `":422498-518: grade != 2 ->
FUN_140089630(node, 2)"` — it implemented exactly that leg and no more.

## Gap

Falsifiable by case table (the decode's, verified against both sides):
- Damaged image with clean extents (reads OK, decode fails): og promotes
  (read==0, payload, !TxtView, tier) — core does not (`DecodeFailed` first). This
  is the dominant real-world case: partially-overwritten JPEG/TIFF files.
- TxtView text file (.txt et al.): og renders the text snapshot and promotes on
  non-empty — core refuses the format outright.
- Audio/video file: og promotes on frame-extraction payload — core defers the
  format.
- Preview-pane-shaped request: neither promotes (core's ABI has no category
  channel; the og's cat-3 exclusion matches core's preview surface by doctrine).
- Decodable image ≤ 20 bytes: og skips work-item creation — core promotes
  (over-promotion edge).

## Impact

The grade is the product's recoverability/confidence signal — it drives the
results-tree display, the sort order (`cr_ui_query_row.grade`), and the
deep-scan regrade narrative. For damaged-but-readable images (the exact population
a recovery product serves), the og tells the user "grade 2 — recovered/readable"
and core says nothing (grade stays Average or lower): the merged product
under-reports recovered quality vs the reference, and files that the og surfaces as
high-confidence are sorted/displayed lower. The TxtView/media triggers are
second-order (their formats are refused/deferred in core by registered decisions —
N-144/OQ-S10-1 own the surfaces) but the grading consequence for text files is
user-visible today. No data loss; no crash.

## Reproduction / verification

Decompiled + binary-verified (Lane C agent's decode, every load-bearing cite
lead-re-verified at filing — the guard text at `:422507-514` re-read first-hand
and matches; the exempt-style masks and branch structure confirmed). Execution
comparison not feasible (the og's UI worker needs the full product); the core side
is code-read: `cr_ui.cpp:3694-3695` vs `:3737-3742` ordering proves the
decode-gating. Full decode record: `evidence/s11-lead-deep-lanes.md` §Lane-C
(the full control-flow walk, the object layout, the case table, and the
disassembly confirmation).

## Fix plan (for the engineering team)

1. Re-base the promotion trigger on read evidence, not decode success: the ABI's
   generation path already runs the extent reads inside `make_thumbnail` — the
   engine must disclose the read verdict separately from the decode verdict
   (e.g. `ThumbnailOutcome { read_ok, decoded, truncated, … }`), and
   `cr_ui_thumbnail_generate` promotes when `read_ok && payload_obtained`
   (decode-independent), matching the og's `local_160==0` semantics.
2. Implement the request-tier exclusion: add the `preview-pane` distinction to the
   ABI (a flag on `cr_ui_thumbnail_generate`, or the doctrine note that the
   preview surface never promotes — which already matches); document that the
   list-tier path promotes on read.
3. The TxtView text-snapshot trigger rides N-144 (the render surface): when that
   lands, its success path feeds the same `read_ok` promotion; until then record
   the gap in the module row (text files promote in the og, not in core).
4. The media-frame trigger rides OQ-S10-1's deferred formats: same seam.
5. Fix the ≤20-byte edge: refuse thumbnail generation for nodes with size ≤ 0x14
   (the dispatcher's skip) — one line in the ABI gate.
6. Correct the comment cite at `cr_ui.cpp:3754` (":422498-518" → ":422507-514").
7. Tests: a damaged-image fixture (intact extents, corrupt entropy stream) whose
   grade must become 2 (RED today); a ≤20-byte image fixture that must NOT promote
   (RED today — core promotes); the preview-pane non-promotion pin (GREEN today,
   pins the doctrine).
8. Acceptance: the case table in this finding passes both directions; the
   `cr_ui_query_row.grade` observable matches the og's behavior on the three
   trigger classes (read-success, text-snapshot, frame-extraction) or carries the
   registered deferral note for the latter two.

## Tests to add

- `CrUi.ThumbnailPromotesOnReadNotDecode` (damaged-image fixture: grade 2 without
  a decodable stream).
- `CrUi.ThumbnailTinyFileDoesNotPromote` (the ≤0x14 skip).
- `CrUi.PreviewPaneNeverPromotes` (the doctrine pin).

## Related findings

M-083 (retired — this finding is the residual of its verified-but-partial parity);
M-109 (the 200 MiB eligibility anchor at the same function — its fix plan should
absorb the read-before-decode ordering note); M-108 (the TxtView eligibility
model — the preview-panel side of the same config surface); N-144 (the
text-snapshot render capability this finding's text trigger depends on);
OQ-S10-1 (the media formats the frame trigger depends on); N-137 (the header-truth
defect at the same ABI surface).

## Open questions

None — the tier-value names (which of {0,1,2,4} maps to which view mode) remain
unresolved (virtual callers, no xrefs), but the tier-vs-preview split and all
promotion semantics are pinned; the unresolved names do not affect the fix.

## Resolution

**Status:** FIXED
**Fixed in commit:** `1d7a761b` (the S47 MAJOR-first fix lane)
**Verified by:** CrUi.ThumbnailPromotesOnReadNotDecode (RED pre-fix — the damaged-but-readable PNG: intact extents, corrupt IDAT entropy; the og's dominant real-world class; post-fix the decode still refuses with ERR_TRUNCATED but the grade becomes 2 on the clean read) · CrUi.ThumbnailTinyFileDoesNotPromote (RED pre-fix — the 8-byte 1×1 PBM decodes via the stb PNM path but sits under the og's 0x14 work-item floor; post-fix the generation is refused with ERR_UNSUPPORTED and no promotion fires) · CrUi.ThumbnailUnreadableSpanNeverPromotes (the source-truth pin: the promotion is gated on the disclosed read verdict, never on the return code) — the RED states proven by the stashed-engine rebuild; the existing ThumbnailSuccessPromotesGradeTo2 / ThumbnailTruncationDisclosed / ThumbnailUnsupportedFormat / ThumbnailTwoCallPattern / ThumbnailCapsAllowEmbeddedValidated / ThumbnailHeaderMatchesContract all stay green; the suites 71/71 (cr_ui_tests) + 1228/1228 (cr_tests).
**Resolution notes:** The fix plan's steps 1, 2, 5, and 6 landed: (1) the engine discloses the read verdict separately from the decode verdict — `Thumbnail::read_ok`, set by `make_thumbnail_status` on the Ok AND DecodeFailed paths exactly when the generation gained NO unreadable span (the og's verdict-0 clean-read equivalent; the og's -3 data-but-flagged-failed class maps to the gained-failure case and does NOT promote); the never-fetched-payload refusals (Unsupported sniff, Oversized bomb guard, BudgetExceeded, BadParameter) leave it false — the named-refusal families keep their existing refusal semantics; (2) `cr_ui_thumbnail_generate` promotes on `thumb.read_ok` BEFORE the status check, so a DecodeFailed-after-clean-reads promotes exactly as the og's guard dictates (grade != 2 && readVerdict==0 && payload/tier — the ABI is the list-tier surface by doctrine, and the preview-pane category-3 exclusion is structural: the preview surface carries no promotion at all); (5) the og's dispatcher floor landed at the ABI gate — `logical_size <= 0x14` refuses the generation (DRWUI :421391-421403), closing the over-promotion edge in BOTH directions (tiny files neither generate nor promote); (6) the stale ":422498-518" cite is gone (the new comment block cites :422507-514 + the binary-verified VA range). Steps 3 and 4 ride their owners unchanged per the fix plan's own routing: the TxtView text-snapshot trigger rides N-144 (MINOR, the render surface — the module row records that text files promote in the og, not in core, until that lands) and the media-frame trigger rides OQ-S10-1's deferred formats; the Unsupported-family (webp/heic/wmf/emf/ico) non-promotion is the registered phase-2 deferral family (I-028) — the og's GDI+ decodes those and promotes, the core's named refusal carries no payload read and therefore no promotion, the module row records the gap. The corpus guard was re-read first-hand this session (DRWUI_decompiled.c:422500-422514 — the exact four-condition guard) before the change. No safety hardening touched (the bomb guard and the honest-refusal statuses are unchanged); no benchmark (grading-observable, not perf-tagged).

---

### Convergence addendum (S47 INFO-first lane, 2026-10-29 — pushed second per SA-9; both lanes' Resolutions preserved)

**Status:** CONVERGENT (this lane built the identical fix independently — the same `Thumbnail::read_ok` disclosure, the same promote-on-read placement, the same 0x14 dispatcher floor — and verified the same corpus guard first-hand at DRWUI :422507-514 before either push; the hot-window merge kept the earlier landing's code per SA-9 and folded this lane's complementary parts in).
**This lane's unique supplements to the landing:**
1. `CrUi.ThumbnailPromotesOnReadNotDecodeDimsRefusal` — the OTHER DecodeFailed exit pinned end-to-end: a JPEG-shaped no-SOF fixture (clean extents, `jpeg_dims` marker-walk refusal at the native_dims seam — the landing's damaged-PNG fixture exercises the stb entropy-seam refusal; together the two refusal classes cover both DecodeFailed exits the promotion consumes). RED-proven by this lane's stashed-source rebuild before the window merged the remote landing.
2. `CrUi.PreviewPaneNeverPromotes` — the doctrine pin the landing's test set lacked: the preview surface (the core's pane equivalent — the ABI has no category channel) leaves grades untouched after open+read, mirroring the og's category-3 exclusion (:422507-514's `!= 3` clause + :652280-281's early return).
3. The ABI contract block in `include/cr/cr_ui.h` (cr_ui_thumbnail_generate's doc): the 0x14 floor's ERR_UNSUPPORTED refusal + the decode-refusal-promotes-on-clean-read semantics now documented at the ABI surface, not just in code comments.
4. The merged engine's `read_ok` coverage spans every DecodeFailed exit: the dims seam, the stb decode seam, both TIFF seams, and the landing's own helper + RAW-path sites — 7 assignment sites, all idempotent on the same clean-read predicate (`!reader_gained_failures`).
**Verified by:** the merged-tree family 11/11 green (the landing's 3 tests + this lane's 2 + the 6 existing family members), full suite 1385/1385 after the same-window repairs (the N-171 double-increment dedup, the S-058 polarity test flip, the e1/e2 unused reds, the M-119 win_syntax_check part_slice retype — convergent with the S53 lane's byte-equivalent repair — and the format-drift sweeps).
