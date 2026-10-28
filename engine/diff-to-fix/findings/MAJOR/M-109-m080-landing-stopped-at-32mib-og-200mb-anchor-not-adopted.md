# MAJOR M-109 — the M-080 landing stopped at 32 MiB: the og's 200 MB thumbnail-eligibility anchor was not adopted — the 32–200 MB band without an embedded EXIF thumb is still refused (BudgetExceeded) where the og thumbnails everything, the test still cites the refuted "no input cap" doctrine, and no PARITY-DELTAS row registers the bound

**Severity:** MAJOR
**Module:** preview (thumbnail engine)
**Parameters affected:** P48-adjacent (capability band), A69-adjacent (test-doctrine truth), D-parameter (unregistered divergence)
**Corpus references:** the M-080 addendum's og-side anchor (evidence/s8-thumbnail-pipeline.md §4, re-verified first-hand by the session-10 landing-verification lane): `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:422428`-area — `if (0xc800000 < lVar4) { … skip; }` inside the thumbnail work item FUN_1402636e0 (the 200 MB still-image eligibility gate; 0xC800000 = 209,715,200), plus the media < 64 KiB skip at :422464; UIUX/10:23 is the separate 100 MB preview-panel cap (correctly not conflated)
**Core references:** audited `2beb555` — `include/cr/preview/cr_thumbnail.h:82` (`u32 max_read = 32u << 20;` — raised 4→32 MiB by the M-080 fix `5ccc0da`), `src/core/preview/cr_thumbnail.cpp:1184-1205` (the byte-oversize leg → embedded fast path → `BudgetExceeded`), `tests/unit/test_thumbnail.cpp:187` (`EXPECT_EQ(32u << 20, c.max_read); /* M-080: the og GDI+ path has no input cap */` — the doctrine the finding's own addendum refuted), no PARITY-DELTAS row for the 32 MiB bound (grep-verified)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** expanded-on-M-080 (renumbered M-105→M-108→M-109 across two push-time reconciliations (the SA-10 lane's DRWService M-105 pushed first and keeps M-105; the lane-B preview-text M-108 pushed first and keeps M-108 — the S10-F lane resolved the resulting filename collision per the same rule, 2026-10-29)) — the fix lane landed both mechanics of the fix plan (the byte-oversize embedded-fast-path leg; a raised default using the plan's own "e.g., 32 MiB" example value) but did not execute the addendum's designated re-pin (the og-side 200 MB eligibility anchor); the Resolution records never mention the 200 MB gate. Verified by the session-10 landing-parity lane (three-agent pass; the lead re-verified the test-doctrine line and the 32 MiB constant first-hand).

## Summary

The M-080 fix raised the thumbnail input budget from 4 MiB to 32 MiB and added the byte-oversize embedded-EXIF fast path — resolving the finding's headline class (the ordinary 2–8 MiB photo). But the og-side anchor the finding's addendum designated (the 200 MB still-image eligibility gate at DRWUI FUN_1402636e0) was not adopted: files between 32 MiB and 200 MB without an embedded EXIF thumbnail (PNG, TIFF, PSD, edited/exported JPEGs) still return BudgetExceeded where the og thumbnails everything up to 200 MB; the pinning test still cites the refuted "the og GDI+ path has no input cap" doctrine; and no PARITY-DELTAS row registers the 32 MiB bound as a deliberate divergence.

## What the reference does

The og's thumbnail work item gates still-image input at 200 MB (0xC800000, DRWUI_decompiled.c:422428-area) — below that, GDI+ decodes and thumbnails; above, the item is skipped. There is no 32 MiB-class intermediate bound on the thumbnail path (the 100 MB UIUX/10:23 cap is the preview panel, a different surface; the ≤1 MiB socket payload is the transport output).

## What our core does

`ThumbnailCaps::max_read` defaults to 32 MiB (cr_thumbnail.h:82). The byte-oversize leg (cr_thumbnail.cpp:1184-1205) tries the embedded-EXIF fast path, then refuses with BudgetExceeded. So: 6 MiB 12-MP JPEG → decodes (fixed ✓); a 64 MiB PSD or a 150 MiB TIFF without an embedded thumb → BudgetExceeded (the og thumbnails it). The pinning test (`CapsDefaultsPinTheCorpusAnchors`, test_thumbnail.cpp:187) asserts 32 MiB with the comment "the og GDI+ path has no input cap" — the exact doctrine M-080's own addendum corrected (the 200 MB gate). No D-row registers the bound.

## Gap

Falsifiable: a 64 MiB PNG (no EXIF) requests a thumbnail. Our engine: BudgetExceeded. The og (≤200 MB gate): a thumbnail is produced. Secondary gap: the corpus-anchor test cites a refuted doctrine instead of the 200 MB gate it should pin.

## Impact

User-observable refusal band (32–200 MB still images without embedded thumbs) on a parity surface the og serves; the test-doctrine mis-pin will propagate the wrong anchor to every future caps change (the same defect class that caused the original 4 MiB mis-pin); the unregistered bound leaves PARITY-DELTAS silent on a real behavioral divergence.

## Reproduction / verification

Executed-class inputs: the landing-verification pass (session-10, three parallel agents + first-hand lead spot-checks of test_thumbnail.cpp:187 and cr_thumbnail.h:82); the og-side gate re-read first-hand at the DRWUI decompile cite; M-080's addendum (evidence/s8-thumbnail-pipeline.md §4) carries the original decode. A direct execution proof is a one-fixture extension: a synthetic 40 MiB PNG through make_thumbnail (BudgetExceeded today; the og-side contract says thumbnails).

## Fix plan (for the engineering team)

1. Adopt the og anchor: `max_read` default → 200 MB (0xC800000), OR keep 32 MiB and register the bound in PARITY-DELTAS with the unserved band named — the og-parity path is the former.
2. Files: `include/cr/preview/cr_thumbnail.h` (the default), `tests/unit/test_thumbnail.cpp` (re-pin the anchor comment to the DRWUI :422428 gate; assert 0xC800000), `docs/PARITY-DELTAS.md` (a D-row if any divergence is kept).
3. The memory-guard note: with max_read at 200 MB, cap the decode work via the existing max_pixels (64 MP) so the raised input bound cannot OOM the decode path (the guard already exists — max_pixels bounds the output; the comment should say so).
4. Tests: `Thumbnail.SixtyFourMiBPngDecodes` (the band proof — fails before, passes after); the re-pinned `CapsDefaultsPinTheCorpusAnchors`.
5. Acceptance: the 32–200 MB band thumbnails (or is registered as a deliberate divergence); the test's cited doctrine names the 200 MB gate; PARITY-DELTAS carries the row if divergent.

## Tests to add

- `Thumbnail.LargeStillImageWithinOgBand` (a >32 MiB, <200 MB fixture decodes Ok).
- `Thumbnail.OversizePastOgGateRefused` (>200 MB → the honest refusal, matching the og's skip).

## Related findings

M-080 (the parent finding, FIXED — its addendum's re-pin is this finding's subject); M-085 (the TIFF decoder that widens the band's affected formats); S-045 (the format-coverage refusals — different axis, same refusal surface).

## Open questions

None — the og-side gate is decompile-pinned and the band arithmetic is direct.
