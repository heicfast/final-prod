# MAJOR M-085 — TIFF thumbnails: LZW-compressed TIFF refused (the og's GDI+ handles it); a mainstream scanner/Photoshop variant

**Severity:** MAJOR
**Module:** preview (thumbnail engine)
**Parameters affected:** B1 (decode coverage), A28-adjacent (the native TIFF validator's compression gate)
**Corpus references:** the og's still-image path decodes TIFF through GDI+ (`DRWUI_decompiled.c` :652299 `CreateStreamOnHGlobal` → the GDI+ load at :652307-309; GDI+ TIFF support includes LZW — the corpus shows the pipeline, the codec matrix is GDI+-internal [UNVERIFIED at codec granularity, see Open questions]); the og's carve side detects TIFF via the u32 keys (census: `II\x2a\x00`/`MM\x00\x2a`) independent of compression
**Core references:** audited `ff879f9` — `src/core/preview/cr_thumbnail.cpp:506, :660-698` (native TIFF decoder accepts compression 1/8/32946/32773 only; LZW(5), JPEG-in-TIFF, tiled, predictor-2, planar honestly refused), the phase-2 note in the file header
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's native TIFF thumbnail decoder refuses LZW-compressed TIFF
(compression tag 5) — the compression variant written by most scanners
and Photoshop's default "LZW" TIFF export. The og's GDI+ pipeline
decodes LZW TIFF. The refusal is honest and documented, but the
format row's coverage verdict must record the divergence.

## What the reference does

The og funnels TIFF through GDI+ (:652299-309); GDI+'s built-in TIFF
codec decodes LZW (and JPEG-in-TIFF, and tiled layouts). The og's
thumbnail path has no compression-type gate of its own.

## What our core does

The native TIFF validator/decoder gates on the compression tag:
accepted {1 (none), 8 (Deflate), 32946 (Deflate-old), 32773
(PackBits)}; LZW(5), JPEG(7), tiled, predictor-2, planar → named
refusal (:660-698).

## Gap

Falsifiable: an LZW-compressed baseline TIFF thumbnails in the og;
the core returns a named refusal.

## Impact

Mainstream scanner output and Photoshop LZW exports get no
thumbnails. Within the already-shipped TIFF family — a coverage hole
on one of the two most common TIFF compressions.

## Reproduction / verification

Core: the compression gate read directly this session. Corpus: the
GDI+ pipeline cite above; the codec-level claim (GDI+ LZW support) is
external-to-corpus platform knowledge — the corpus pins the pipeline,
not the codec matrix; flagged accordingly in Open questions.

## Fix plan (for the engineering team)

1. Implement LZW decode in the native TIFF path (the classic
   TIFF LZW: 8-bit clear/EOI codes 256/257, MSB-first packing,
   per-strip code tables) — ~120 lines, well-specified.
2. Reuse the existing dims-first/budget/truth gates; extend the TIFF
   test matrix (the 6-variant parameterized suite) with LZW fixtures
   from `scripts/gen_thumbnail_fixtures.py`.
3. Acceptance: LZW baseline/gray/palette fixtures thumbnail
   byte-identically to reference decodes; hostile LZW (bad code
   tables, EOI-less strips) refuses under ASan; fuzz clean.

## Tests to add

ThumbnailTiffLzwBaseline; ThumbnailTiffLzwGray; ThumbnailTiffLzwNoise
(bad table); ThumbnailTiffLzwTruncated.

## Related findings

S-045 (the family coverage finding), M-080 (budget), I-028
(superset register).

## Open questions

GDI+'s exact TIFF codec matrix (LZW/JPEG-in-TIFF/tiles) is
platform-internal — if codec-granular parity is needed, a runtime
probe on Windows (GdipLoadImageFromFile on crafted fixtures) or an
MS-doc citation should pin it; the corpus alone shows the pipeline.
