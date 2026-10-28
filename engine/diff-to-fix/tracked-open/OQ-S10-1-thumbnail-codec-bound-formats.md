# OQ-S10-1 — The codec-bound thumbnail halves (HEIC/WebP/PDF/video key-frames): vendoring plan and the Win7 import-gate audit

**Status:** OPEN (session-10, this lane) — spun out of S-045's Resolution (the TIFF-RAW half landed at 3aba383)
**Module:** preview (thumbnail engine)
**Parameters affected:** B1 (decode coverage), P47/P48

## The remaining scope (from S-045's fix plan)

1. **HEIC/HEIF** (libheif + libde265, embedded-thumbnail-first - mirror heif_image_handle_get_thumbnail, the EUImgRepairFile call) + the AVIF-class flag question (0x6c at DRWUI :664824, the "avif" string at :474353 - decode path or stub).
2. **WebP** (libwebp).
3. **PDF page-1 raster** (pdfium) — the finding's Open question on scope.
4. **Video key-frames** — the og uses an out-of-process ffmpeg agent (ports 20171/2, the 602-byte header + <=1 MiB BMP answer protocol, :422456-59/:653868-653975/:654551-630); the layer-faithful core port is the bounded ffmpeg/libav integration OR the explicit host-side callback contract (preview.md's recorded layer decision - the og's agent split is UI-layer).

## Why not this session

Each is a vendored codec stack (10^5+ lines) whose integration re-opens the Win7-inclusive import allowlist gate (docs/PREBUILT-ARTIFACTS + the loadability gate) - a per-library audit project, not a single-session fix. The engine keeps them honestly refused BY NAME (thumbnail_format_supported false).

## The next concrete step

1. Pick ONE library first (libheif - HEIC is the default phone capture format, the highest-value half).
2. Vendor under third_party/ with the import-surface inventory (which Win32 APIs each touches) filed as an audit artifact.
3. Extend scripts/gen_thumbnail_fixtures.py per family (real .heic/.webp/.pdf/.mp4 fixtures) + the hostile-noise refusals.
4. Record the layer decision for video (in-engine vs the host-callback seam) in modules/preview.md BEFORE implementing (the og's split is UI-layer).
## Session-40 disposition note (2026-10-29)

Still OPEN, scope unchanged and still honest: each half is a vendored codec stack
with its own Win7-inclusive import-gate audit (libheif first, per the recorded step).
Verified current this session: the refusals remain BY NAME (thumbnail_format_supported
false for HEIC/WebP/PDF/video - no silent degrade), and the preview module's layer
decision for video (the og's out-of-process agent split is UI-layer) stays the
recorded pre-implementation requirement. Nothing new to resolve; the next concrete
step stands verbatim.
