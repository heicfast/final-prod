# SEVERE S-045 — Thumbnail format coverage: the og ships video-keyframe, camera-RAW and HEIC pipelines; the core refuses (WebP/HEIC/RAW/PDF/video)

**Severity:** SEVERE
**Module:** preview (thumbnail engine)
**Parameters affected:** B1 (detection/decode coverage), A28-adjacent (format validators), P47/P48 (throughput/latency of the surface on those classes)
**Corpus references:** `DRWUI_decompiled.c` — the media thumbnail path: category check {8=Video, 9=Audio} → `FUN_1403da720` socket request with the extent list (28-byte entries) in a 602-byte header (:422456-59, :653868-653975) to the EaseUS_VideoViewer agent (ports 20171/20172, :653821-31), answer `[602B header][≤1 MiB BMP]` (:654551-630); RAW: launches `EURawImg.exe` under `DRW_EURawImg_Mutex` (:760253, :760312+), LibRaw-based (`per-binary/EURawImg.md:28-48`), converter divides both dims by 8 and PNG-encodes (:759990-760040); HEIC/HEIF: preview-router special cases `.heic` (0x7c) / `.heif` (0x7d) / AVIF-class (0x6c) (:664817-24); `EUImgRepairFile` imports `heif_image_handle_get_thumbnail` (EUImgRepairFile_imports.txt:6, use :331251); PDF: PDFViewer imports `FPDF_LoadPage/FPDF_RenderPage` (pdfium raster); the format-group registry string at :828027 (jpg_jpeg; tiff+24 RAW extensions; heic_heif; ai_pdf; zip/office; …) — tabulated in `evidence/decompiled-quotes/s8-thumbnail-pipeline.md` §4 and the agent G-C report
**Core references:** audited `ff879f9` — `src/core/preview/cr_thumbnail.cpp:201-228, :1039-1044` (WebP/HEIC sniffed-and-named refusals, `thumbnail_format_supported("webp"/"heic"/"raw"/"pdf"/"mp4") == false`), the phase-2 deps note in `modules/preview.md:141-147` (libwebp / libheif+libde265 named as deferred dependencies)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og produces thumbnails for video key-frames (out-of-process ffmpeg
agent), camera RAW (LibRaw via EURawImg, ⅛-downscale + PNG handoff),
HEIC/HEIF (libheif embedded-thumbnail-first), PDF (pdfium raster), and
cloud content. The core's thumbnail engine covers still images only
and refuses the rest by name. The deferral was registered with named
dependencies before the og-side pipelines were decoded; this finding
formalizes the verdict now that the corpus evidence is on the table.

## What the reference does

Per-class pipelines, all corpus-cited above: video/audio categories
route to a socket agent that decodes via ffmpeg `avio_alloc_context`
range reads (MediaGraph :3207-3222) and returns a BMP; RAW launches
EURawImg (LibRaw static, 467 exports) with a ≈200 MB shared-memory
bound, converts to GDI+, downscales both dims by 8 and PNG-encodes the
handoff; HEIC/HEIF are flag-routed in the preview data source and
decoded by the heif.dll family (the repair-side EUImgRepairFile uses
the embedded-thumbnail-first call the core's own header cites); PDF
rasterizes through pdfium.

## What our core does

`cr_thumbnail.cpp` decodes seven still-image families (JPEG/PNG/BMP/
GIF/PSD/PNM/HDR + native baseline TIFF) and refuses WebP, HEIC, RAW,
PDF, and all video with named `Unsupported` statuses; the deps note in
preview.md registers libwebp/libheif+libde265 as the phase-2 path.

## Gap

Falsifiable: a .heic phone photo, a .cr2/.nef/.arw capture, an .mp4
clip, a .pdf — each yields a thumbnail in the og and a named refusal
in the core.

## Impact

Mainstream-content capability gap: HEIC is the default capture format
on current phones; RAW is the archive format of every camera workflow;
video key-frames and PDF covers are the dominant thumbnail use cases
in a recovery product's result grid. Users observe empty preview
slots for the majority of modern media.

## Reproduction / verification

Corpus: the evidence file §4 + the format-group registry string and
import-table citations (all decompile/import cites listed above).
Core: the refusal sites read directly this session. Execution
comparison not applicable pre-fix (the pipelines are absent).

## Fix plan (for the engineering team)

1. Phase 2a (vendored, still in-process, following the stb precedent):
   libheif+libde265 for HEIC/HEIF — implement the
   embedded-thumbnail-first strategy (mirror
   `heif_image_handle_get_thumbnail`, the call EUImgRepairFile uses)
   with full decode as fallback; libwebp for WebP.
2. Phase 2b: camera RAW — vendor libraw (or a minimal DNG/CR2/NEF/ARW
   TIFF-based subset), following the og's ⅛-downscale handoff shape
   (decode → downscale → feed the existing box filter).
3. Phase 2c: PDF page-1 raster (pdfium or a minimal parser) and video
   key-frame extraction — the og uses an out-of-process agent; the
   core's engine-layer equivalent is a bounded ffmpeg/libav integration
   or an explicit host-side callback contract (record the layer
   decision in preview.md — the og's agent split is UI-layer, so a
   host-callback seam is the layer-faithful port).
4. Update `thumbnail_format_supported` + the refusal names; extend
   fixtures (`scripts/gen_thumbnail_fixtures.py`) per family.
5. Acceptance: per family — a real fixture thumbnails; hostile/corrupt
   variants refuse cleanly under ASan; budgets hold (M-080's raised
   caps); suite + fuzz green.

## Tests to add

ThumbnailHeicEmbeddedFirst; ThumbnailHeicFullDecodeFallback;
ThumbnailWebpDecodes; ThumbnailRaw EighthScaleHandoff (CR2/NEF/ARW);
ThumbnailPdfFirstPage; ThumbnailVideoKeyFrame (or the host-callback
contract test); per-family hostile-noise refusals.

## Related findings

S-043 (orientation), M-080 (budget), M-085 (TIFF LZW — the in-family
sibling of this coverage gap), I-028 (superset register).

## Open questions

Whether the og's AVIF-class preview flag (0x6c at :664824, the
"avif" string at :474353) routes a real decode path or a stub —
needs a targeted decode of `DAT_1414d42f4`'s consumer before claiming
AVIF coverage parity.
