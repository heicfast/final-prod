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

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-092 commit — the session-38 minor-first lane)
**Verified by:** `Thumbnail.MetafileAndIconFamiliesAreNamedRefusals` (new) + `Thumbnail.FormatSupportIsTheDocumentedV1Matrix` extended — the new test EXECUTED RED on the pre-fix sniff (all five forms failed with "the metafile/icon family must be refused BY NAME, not 'unknown'"), GREEN after; full suite re-run green; clang-format 18.1.8 clean.
**Resolution notes:** Re-verified at current main (`84a1832`-lineage + the S-042 landing): sniff() at cr_thumbnail.cpp:201-228 carried no metafile/icon detection (fall-through to "unknown"), and the header's deferred-families list (cr_thumbnail.h:34-41) did not name WMF/EMF/ICO — both halves of the finding's (a) confirmed; the corpus side re-verified first-hand (DRWUI_imports.txt:3356-region GDI+ import block incl. GdipGetImageThumbnail read directly; GDI+ rasterizes metafiles/icons natively — the og's own thumbnail path). The fix, per the finding's fix-plan step 1 (the cheap half now): the family is named-and-refused — placeable WMF (0xD7CDC69A key), standard WMF (u16 type 1/2 + u16 header-size 9), EMF (the " EMF" signature dword at +40 — the make_thumbnail sniff buffer grew 16→48 bytes for it), ICO/CUR (reserved-0 + type word); all five forms return `Unsupported` with `source_format` set to the family name, joining webp/heic in the documented phase-2 refusal group; the header contract + v1-matrix comment extended. Step 2 (the cache decision) is DECIDED AND REGISTERED, not silently dropped: no engine-side cache — the og caches UI-side (ThumbnailFactory::ClearCache, UIUX/30:62), the core's in-process doctrine keeps make_thumbnail stateless; the decision is recorded in modules/preview.md's new cache-scope block + cr_thumbnail.h's contract comment, with the session-scoped-LRU-if-ABI-lands candidate noted for the M-102 handoff. The rasterization itself stays deferred with the other named families (GDI+ dependency — a Windows-only path the core's cross-platform engine deliberately does not take without a decision). `thumbnail_format_supported("wmf"/"emf"/"ico")` returns false by construction (the supported-set is unchanged) and is now pinned false explicitly.
