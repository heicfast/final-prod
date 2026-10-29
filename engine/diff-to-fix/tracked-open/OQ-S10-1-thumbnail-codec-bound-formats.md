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

## Session-48 disposition note (2026-10-29 — pushed forward with NEW DECODE EVIDENCE: the AVIF question answered, the agent seam mapped, the HEIC hint decoded)

Still OPEN on the vendoring decision (each half is a per-library Win7 import-gate audit —
an owner-level call, not a single-session fix). Verified current at the S48 tree: the
by-name refusals stand (thumbnail_format_supported at cr_thumbnail.cpp:85 admits
jpeg/png/bmp/gif/tiff/raw/psd/pnm/hdr only — HEIC/WebP/PDF/video honestly refused, no
silent degrade). NEW EVIDENCE this session (all read first-hand):

1. **The AVIF question ANSWERED — a REAL decode path, not a stub.** The extension
   dispatch at DRWUI:664818-664830 maps `.heic`→0x7c / `.heif`→0x7d / `.avif`→0x6c
   (DAT_1414d42f4, the thirdFUN_1400c9810 arm); the format id flows into the agent
   factory FUN_14048b330 → FUN_14048bf10, whose answer loop carries a DEDICATED
   AVIF post-processor: `if (*param_5 == 0x6c) FUN_14048b7f0(param_1)` on the
   raw-bytes answer path — the 0x6c branch is real decode machinery (the agent
   returns the still-compressed frame; the UI side rasterizes). AVIF is also in the
   og's own advertised photo set (the Supported-Formats string, DRWUI:1261392) and
   the 16-entry extension registry beside webp/braw/pages/numbers (:474356).
2. **The agent seam mapped — ALL the codec-bound halves ride the SAME out-of-process
   protocol** (the OQ's video item named ports 20171/2; the photo codecs ride the
   sibling `ThreadImg` shared-memory channel): `Mutex_ThreadImg` +
   `ShareMemory_ThreadImg` + events `DRWRAWReadOver_ThreadImg`/`DRWUIWriteOver_ThreadImg`,
   a 20-byte request header + raw file bytes, a ~200 MiB size cap (0xc800015), a
   10 s answer wait (FUN_14048bf10 :664344+). This STRENGTHENS the preview module's
   recorded layer decision: the og's agent split is UI-layer for the PHOTO codecs
   too, not just video — a core-side vendored stack remains the parity-faithful
   shape, and the host-callback seam question now covers all four families
   uniformly.
3. **The HEIC/HEIF embedded-thumbnail-first hint decoded**: the agent request
   header's flag byte is set to 1 exactly when the format id is 0x7c/0x7d
   (`local_68 = (*param_5==0x7c||*param_5==0x7d)` at :664368) — the wire form of
   the heif_image_handle_get_thumbnail-first policy the OQ's item 1 named. The
   libheif vendoring lane now has the exact request semantics to mirror.

**Next concrete step (sharpened):** unchanged owner decision on vendoring (libheif
first — now with the hint-byte semantics decoded); the video layer decision gains
the uniform-agent-seam evidence above. No fixture-generator change until a library
is chosen (the recorded step 3's ordering stands).

## Session-86 disposition note (2026-10-30, the S86 fix lane — pushed forward; scope unchanged and honest)

Still OPEN, the vendoring project exactly as scoped (each half is a
10^5+-line codec stack with its own Win7-inclusive import-gate audit).
This session's first-hand verification:

1. **The BY-NAME refusals stand** — `thumbnail_format_supported`
   (src/core/preview/cr_thumbnail.cpp:85-93) supports exactly
   {jpeg, png, bmp, gif, tiff, raw, psd, pnm, hdr, txt}; HEIC/WebP/PDF/
   video remain honestly refused by name (no silent degrade, no stub).
2. **The layer decisions stand** — the preview module's video decision
   (the og's out-of-process agent split is UI-layer) and the S37
   addendum's uniform-agent-seam evidence (the photo codecs ride the
   sibling `ThreadImg` shared-memory channel: `Mutex_ThreadImg` +
   `ShareMemory_ThreadImg` + the 20-byte request header, the ~200 MiB
   cap, the 10 s answer wait) remain the recorded parity-faithful shape.
3. **The next concrete step unchanged**: libheif first (the
   embedded-thumbnail-first hint semantics already decoded — the request
   header's flag byte set to 1 exactly for format ids 0x7c/0x7d), vendored
   under third_party/ with the import-surface inventory filed as an
   audit artifact, THEN the fixture extension. No fixture-generator
   change until a library is chosen (the recorded ordering stands).

## Session-86 disposition note (2026-10-30 — pushed forward, current-tree verified)

Still OPEN on the owner-level vendoring decision (each half is a per-library
Win7-inclusive import-gate audit - libheif first, per the recorded step).
Verified current at the S86 tree: the by-name refusals stand honest -
`thumbnail_format_supported` (src/core/preview/cr_thumbnail.cpp:85) admits
exactly jpeg/png/bmp/gif/tiff/raw/psd/pnm/hdr/txt; HEIC/WebP/PDF/video are
refused BY NAME (no silent degrade - the OQ's own honesty requirement).
The S48 decode evidence (the ThreadImg shared-memory agent seam, the
embedded-thumbnail-first hint byte for 0x7c/0x7d, the AVIF post-processor)
remains the vendoring lane's wire-form ground. Nothing new to resolve
without the owner decision; the next concrete step stands verbatim
(libheif first; the layer decision for video before implementing).
