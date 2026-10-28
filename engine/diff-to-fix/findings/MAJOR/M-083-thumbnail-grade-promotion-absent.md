# MAJOR M-083 — Thumbnail success promotes the node to grade 2 in the og; the core's engine is grade-agnostic

**Severity:** MAJOR
**Module:** preview (thumbnail) × tree (grading) — the S-023 family
**Parameters affected:** D15 (confidence grading model), B4 (deleted-file classification feed), A25 (the thumbnail pipeline's side-effects)
**Corpus references:** `DRWUI_decompiled.c` `FUN_1402636e0` :422498-422518 — `FUN_140089670(node)` (grade getter) != 2 → `FUN_140089630(node, 2)` (grade setter); the setter identified in `artifacts/DRWUI-subsystems--assistant-grading-preview.md:46` — verbatim quote in `evidence/decompiled-quotes/s8-thumbnail-pipeline.md` §3
**Core references:** audited `ff879f9` — `src/core/preview/cr_thumbnail.cpp` (no grading call anywhere; grade-agnostic by design), the S-023 grading fix (`grade_tree`) covers re-grade on preview/selection only; `modules/preview.md` thumbnail section (no grading note)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

In the og, a successful thumbnail decode promotes the result node's
grade to 2 ("high") — thumbnail decodability is grading signal, wired
directly into the thumbnail work item. The core's thumbnail engine
never touches grades; its grading model (S-023's `grade_tree`) keys on
preview success, not thumbnails.

## What the reference does

At the tail of the thumbnail work item (:422505-513): if the node's
current grade != 2 (and the category/flag conditions hold), call
`FUN_140089630(node, 2)`. The og treats "this file's bytes decode as
an image" as evidence of recoverability — thumbnail success is a
grade-raising event.

## What our core does

`make_thumbnail` returns a Thumbnail; nothing in the engine or its
call chain writes grades. S-023's `grade_tree` re-grades on preview
(the `extract_metadata` path), so a thumbnail-only consumer (the
anticipated host use) gets no grading side-effect.

## Gap

Falsifiable: a node graded below 2 whose file produces a thumbnail —
og: grade becomes 2; core: grade unchanged.

## Impact

Grading output ("Recovery Chances") diverges for thumbnail-decodable
files. Users comparing recovery quality between engines see different
grades for identical inputs whenever the thumbnail path (not the
preview path) is exercised. This is a data-model divergence in the
grading contract, not just a preview nicety.

## Reproduction / verification

Decompiled evidence (first-hand read this session):
`evidence/decompiled-quotes/s8-thumbnail-pipeline.md` §3, with the
setter's identity pinned by the DRWUI-subsystems artifact (:46). Core:
full engine read (zero grading references).

## Fix plan (for the engineering team)

1. Decide the seam: the engine stays pure (recommended — matches the
   core's layering) and the promotion is wired at the host/ABI layer:
   after a successful `make_thumbnail`, the host calls the existing
   grade surface; OR the core adds an optional grade hook parameter.
2. Whichever seam: the semantics are "thumbnail Ok ⇒ grade at least 2"
   (never downgrade; skip when already 2 — the og's != 2 guard).
3. Files: `src/ui/cr_ui.cpp` (when the thumbnail ABI lands — see
   N-103) or `include/cr/preview/cr_thumbnail.h` (hook param);
   `modules/preview.md` + the S-023 row addendum.
4. Acceptance: a below-2 node + successful thumbnail ⇒ grade 2; an
   already-2 node unchanged; failed thumbnail ⇒ unchanged.

## Tests to add

ThumbnailSuccessPromotesGradeTo2; ThumbnailFailureLeavesGrade;
ThumbnailAlreadyGrade2NoRewrite (idempotence).

## Related findings

S-023 (the grading model — FIXED; this is its thumbnail-path
extension), S-043/M-080/S-045 (the same engine's other verdicts).

## Open questions

None.
