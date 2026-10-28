/*
 * cr_thumbnail.h - engine-side thumbnail generation over the bounded
 * PreviewReader (session-owner directive; the og decodes thumbnails entirely
 * UI-side - GDI+ GdipGetImageThumbnail at 256 px, the MediaGraph/ffmpeg
 * media agent's <=1 MiB BMP payloads, EURawImg/LibRaw for RAW, libheif's
 * embedded thumbnails - so the core capability is a deliberate, documented
 * superset in the I-015 in-process-reader lineage; corpus anchors and the
 * phased format matrix live in docs/PARITY-OG-CORE-INVENTORY.md Section C.2).
 *
 * Contract (mirrors the preview module's bounded-read doctrine):
 *   - all bytes flow through the caller's PreviewReader; the engine
 *     NEVER opens a file and never reads more than caps.max_read logical
 *     bytes for one thumbnail (work is additionally bounded by caps.max_work
 *     fetch units - deterministic counters, no wall-clock bound);
 *   - dims-first decompression-bomb guard: the header-declared w*h is
 *     checked against caps.max_pixels BEFORE any full decode allocates;
 *     oversized inputs are refused honestly (Oversized), with the EXIF
 *     embedded-thumbnail fast path (caps.allow_embedded, JPEG only) as the
 *     og-parity escape - GDI+/LibRaw/libheif all prefer embedded thumbs;
 *   - unreadable device spans are never silently thumbnailed: if any span
 *     the reader could not fetch was touched during generation, the call
 *     fails with UnreadableSpan (a thumbnail decoded over zero-fill would
 *     misrepresent recovered content);
 *   - output is straight-alpha RGBA8, row-major, width*4 stride, downscaled
 *     aspect-preserving with max_edge applied to BOTH axes (the og's
 *     width-derived GDI+ scale quirk is deliberately corrected), never
 *     upscaled, premultiplied box-filter averaging;
 *   - S-043: JPEG output is EXIF-oriented - the og reads GDI+ property
 *     0x112 and applies the matching RotateFlip BEFORE the 256-px pass
 *     (DRWUI :652307-309); the engine applies the same 8-case transform
 *     to the full-decode path BEFORE the downscale (the embedded-fast
 *     path serves camera-pre-rotated thumbs as-is);
 *   - honesty: unsupported formats are named (webp/heic are documented
 *     phase-2; the GDI+ metafile/icon family wmf/emf/ico is likewise
 *     named-and-refused - the og's GDI+ path rasterizes them natively,
 *     the core's matrix defers them), truncation is flagged, decode
 *     failures carry a status name for logging - never a synthesized image.
 *
 * v1 format matrix: JPEG (baseline+progressive), PNG, BMP/DIB, GIF (first
 * frame), TIFF baseline (uncompressed / PackBits / Deflate strips, 8/16-bit,
 * gray / palette / RGB / RGBA, both endians - native), plus PSD / PNM / HDR
 * via the vendored public-domain decoder (third_party/stb). Deferred with
 * named dependencies: WebP (libwebp), HEIC (libheif+libde265, embedded
 * first), RAW (LibRaw thumbnail API), video key-frames (ffmpeg), PDF page
 * pixmaps (pdfium), JPEG-in-TIFF / tiled TIFF, WMF/EMF metafiles + ICO/CUR
 * icons (GDI+ on Windows - the og's own native path; refused by name).
 *
 * Cache scope (deliberate, registered): no engine-side thumbnail cache -
 * repeated renders are the host's layer (the og caches UI-side in
 * ThumbnailFactory; the core's in-process doctrine keeps the engine
 * stateless across calls). Recorded in modules/preview.md.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_THUMBNAIL_H_
#define CR_THUMBNAIL_H_

#include "cr/preview/cr_preview.h"
#include <string>
#include <string_view>
#include <vector>

namespace cr {
namespace preview {

/* Bounding policy for one thumbnail generation. Defaults follow the corpus
 * anchors: max_edge 256 (the og UI-side GDI+ convention), max_pixels 64 MP
 * (keeps worst-case decode memory at 256 MiB RGBA), max_read 4 MiB (the
 * PreviewReader kMaxRead documented superset, M-028 cost doctrine),
 * max_work 1<<22 fetch units (the extract_metadata work-guard scale). */
struct ThumbnailCaps {
    u32 max_edge = 256;
    u64 max_pixels = 64ull * 1024 * 1024;
    u32 max_read = 4u << 20;
    u32 max_work = 1u << 22;
    bool allow_embedded = true;
};

/* The generated thumbnail. `rgba` holds width*height*4 bytes (RGBA8,
 * straight alpha, row-major). `source_format` names the decoded family
 * (jpeg|png|bmp|gif|tiff|psd|pnm|hdr). `from_embedded` marks the EXIF
 * embedded-thumbnail fast path (JPEG only). `truncated` is set when the
 * input stream was short - on failures it accompanies the status, and on
 * Ok it discloses a decoder-tolerated short stream (stb's JPEG zero-fill
 * leniency: the image decoded, the short input is never silent); it is
 * never a silent loss. */
struct Thumbnail {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> rgba;
    std::string source_format;
    bool from_embedded = false;
    bool truncated = false;
};

enum class ThumbStatus : int {
    Ok = 0,         /* Thumbnail is filled */
    Unsupported,    /* format sniffed and named, no v1 decoder (webp/heic/...) */
    DecodeFailed,   /* damaged stream the decoder refused (truncated set) */
    Oversized,      /* dims-first bomb guard refused (from header, before decode) */
    BudgetExceeded, /* caps.max_read / max_work exhausted */
    UnreadableSpan, /* the reader zero-filled a span we decoded over (refused) */
    BadParameter,   /* caps out of range / empty input */
};

/* Stable status names for logging and tests. */
const char* thumbnail_status_name(ThumbStatus s);

/* v1 decoder surface (honest capability reporting; the deferred phases are
 * documented in the module header). Accepts the canonical family names
 * above ("jpeg", "png", ...). */
bool thumbnail_format_supported(std::string_view format);

/* Generate one thumbnail from the node's logical content (the reader is
 * non-const on purpose: read_at accumulates the reader's diagnostics, same
 * contract as extract_metadata). On Ok the Thumbnail is fully populated;
 * on failure no pixels are ever synthesized - `source_format`/
 * `truncated` may carry diagnostics, nothing else is written. */
ThumbStatus make_thumbnail(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out);

} // namespace preview
} // namespace cr

#endif // CR_THUMBNAIL_H_
