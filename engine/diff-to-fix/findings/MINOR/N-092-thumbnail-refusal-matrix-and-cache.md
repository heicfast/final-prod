# MINOR N-092 — Thumbnail engine gaps: WMF/EMF/ICO absent from the honest-refusal matrix, and no cache layer

**Severity:** MINOR
**Module:** preview (thumbnails)
**Parameters affected:** Q68 (the refusal matrix's honesty), X47/X49 (throughput/memory — the cache), A54 (layering — the cache is a host/ABI concern)
**Corpus references:** og thumbnail matrix: GDI+ rasterizes BMP/GIF/JPEG/PNG/TIFF **+ WMF/EMF/ICO** (`DRWUI_imports.txt:3356` region — 26 Gdip imports incl. `GdipGetImageThumbnail`; `re-workspace/artifacts/UIUX/18-preview-architecture.md`); og cache: `ThumbnailFactory::ClearCache` (`re-workspace/artifacts/UIUX/30-interaction-driver-ground-truth.md:62` — UI-side cache exists)
**Core references:** audited `35adcb9`. `include/cr/preview/cr_thumbnail.h:35-38` — the deferred-families list names webp/heic/raw/video/pdf/jpeg-in-TIFF/tiles; **WMF/EMF/ICO not named** — `sniff()` returns "unknown" for them (src/core/preview/cr_thumbnail.cpp:220-226 region). No cache anywhere in `cr_thumbnail.*` (every `make_thumbnail` call decodes; `preview.md:103-154` registers no cache scope).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two thumbnail-engine residuals from the 05e6786 landing verification: (a) the og's GDI+ path rasterizes metafiles (WMF/EMF) and icons (ICO) — the core's format matrix neither implements nor *names* them in its honest-refusal list, so a `.wmf` result row gets `Unsupported("unknown")` where the og shows a thumbnail and the refusal's self-documentation is incomplete; (b) the og caches thumbnails (`ThumbnailFactory`) — the core decodes on every call, with no cache layer or registered scope note for hosts.

## What the reference does

GDI+ (the og's thumbnail engine) handles metafile and icon formats natively; `ThumbnailFactory` caches results (UIUX/30:62) so repeated row renders don't re-decode.

## What our core does

The v1 decode matrix covers JPEG/PNG/BMP/GIF/TIFF(native) + PSD/PNM/HDR(stb), with named-and-refused WebP/HEIC/RAW/video/PDF/JPEG-in-TIFF/tiles. WMF/EMF/ICO fall through to "unknown" (unnamed refusal). No cache: N consecutive `make_thumbnail` calls decode N times.

## Gap

(a) A `.wmf`/`.emf`/`.ico` node yields `Unsupported("unknown")` — the format is neither implemented nor in the documented refusal set; (b) no perf reuse across calls.

## Impact

(a) Refusal-matrix honesty gap (the header's "named + honestly refused" contract has three holes); (b) a throughput concern for hosts doing repeated renders — the ABI surface (pending handoff) will inherit it.

## Reproduction / verification

Corpus-derived (the GDI+ import census; UIUX/30:62); core first-hand (the sniff() fall-through read; the header's refusal list read; the no-cache grep).

## Fix plan (for the engineering team)

1. Add WMF/EMF/ICO to the named-refusal list in `cr_thumbnail.h:35-38` + `sniff()` (one-line trio; honest `Unsupported("wmf")` etc.) — the cheap half now; the rasterization itself (GDI+ on Windows / a metafile parser) can stay deferred with the other named families.
2. Cache: decide the layer (host-side is the architecture-correct home per the in-process doctrine; if the ABI lands a thumbnail export, a session-scoped LRU keyed by node id is the candidate) — record the decision in preview.md; do not add silently.

## Tests to add

- `Thumbnail.RefusalNamesWmfEmfIco` (each returns `Unsupported` with the format name — fails before, passes after step 1).
- (If cache lands) `Thumbnail.CacheHitReducesDecodes` (a decode counter seam).

## Related findings

- M-080 (this session — the budget gap on the same engine).
- I-015/M-066 (the preview register).

## Open questions

None.
