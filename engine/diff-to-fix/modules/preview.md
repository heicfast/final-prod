# modules/preview.md — preview module audit verdicts (NEW in session 4)

Reference: og's preview is a **UI-layer subsystem** — DRWUI's VAPreviewListener speaks a
local-socket/thumbnail protocol to a media-agent child process (`tcp://localhost:`
20171/20172 + ZeroMQ REP control + `EaseUS_VideoViewer.exe`): 602-byte (0x25A) MediaInfo
header, payload = size−0x25A bounded (0, 0x100000] = **1 MiB BMP cap**, 10 s bounded
wait (100×100 ms), GlobalAlloc(0x42, (size+0xDA5)&~0xFFF) HGLOBAL handoff, id
correlation at +0xC, error codes 0xFFFFFFFF/0xE/0xFD; UI-level eligibility: files >
100 MB not previewable, free-tier videos 15 s. Evidence:
`evidence/decompiled-quotes/s4-crecoverability-grading-engine.md` (context) + the
session-4 context sheets; register finding **I-015**.
**IInWindowStream is NOT a preview contract** — it is DRWUI's embedded 7-Zip/LZMA SDK
input interface (vftable :800729-800735, adjacent CLZInWindow/IMatchFinder; zero
occurrences in any other reversed tree).

Core (audited `63a9167`): `src/core/preview/cr_preview.cpp` (772 LOC) +
`include/cr/preview/cr_preview.h` — in-process bounded PreviewReader (resident prefix
→ extents → sparse zero-fill; zero-fill + `unreadable_spans` failure policy; device-read
diagnostics; `kMaxRead` 4 MiB, `kMaxDecode` 256 MiB) + `extract_metadata` (10 format
families: jpeg/png/gif/bmp/tiff/webp/wav/mp4/pdf/zip→docx + text fallback; IFD0-only
EXIF tags 274/306; moov-at-end 64 KiB tail; raw-deflate docx text snippet) + the
`cr_ui` preview ABI entries. Tests: 14 unit tests + fuzz target. Prior sessions never
audited this module (zero matrix row, zero findings) — session 4 is the first pass.

## Verdicts

- **P1/P2 (detection/recovery correctness)** — `NA`: preview reads content the engines
  already emitted; no new detection surface.
- **P15-adjacent (grade re-run on preview)** — finding **S-023**: the og re-grades
  grade-1 nodes at preview/selection time; the core has no re-grade hook.
- **P28-adjacent (parser grammar)** — `M`-with-defect: parsers are bounded and honest
  (strict fetch, short-window guards) — the docx-path exception is CLOSED: **M-028
  FIXED (s21)** (all four docx reads through the cap-gating `checked_read`).
- **P46 (contract documentation)** — **M-028 FIXED (s21)**: the header now carries the
  true anchors (og preview payload cap = 1 MiB at the UI socket layer, I-015;
  IInWindowStream = the 7-Zip/LZMA input interface, vftable :800729-800735; the
  core's 4 MiB kMaxRead = deliberate documented superset) and the compressed-extent
  whole-block cost disclosure.
- **P47/P49 (throughput/memory on untrusted input)** — **M-028 FIXED (s21)**: the
  executed bypass (65,568 device bytes against a 4,096-byte cap, `truncated=false`)
  is closed by cost-gated budgets — `PreviewReader::read_cost` prices every fetch
  BEFORE the device is touched (compressed windows cost the whole psize block);
  a 4 KiB budget now yields `device_bytes ≤ 4096, truncated=true, archive/zip`.
- **P65 (test coverage)** — **M-028 FIXED (s21)**: the caps-bypass class is pinned
  (DocxCapsMaxReadHonored / DocxTailBeyondBudgetDegradesToZip /
  CompressedExtentReportsTruncatedUnderBudget /
  CompressedExtentWholeBlockReadDisclosed / MetadataTruncatedFlagTruthfulAllPaths,
  all fail-before against the executed scenario) + the fuzz harness gained the
  input-derived tight-budget trap; the prior 14 tests unchanged and green.
- **P54/P56 (module boundaries)** — `M`-equivalent (deliberate): in-process reader vs
  the og's socket protocol — register **I-015**.
- **P69 (dead code)** — `M`: no stubs; all parsers live.

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-028 | MAJOR | Caps.max_read bypass on the zip/docx path (executed proof) + 4 MiB/IInWindowStream provenance misattributions + compressed-extent whole-block reads — **FIXED (s21)** |
| I-015 | INFORMATIONAL | Preview architecture register: og socket protocol (1 MiB BMP cap, 602-byte header, 10 s wait, 100 MB eligibility) vs core in-process reader — deliberate |

## Not-audit-note

The "mov" format is documented in `cr_preview.h:88`'s comment but never produced by
any parser path (only "mp4" is; MP4 brand detection covers qt brands) — cosmetic doc
drift, folded into M-028's doc-fix step.

---

## Dev-sign session wave 4 (2026-10-28, audited `dd82db9` — UIUX/10-preview-subsystem.md's first read)

19 parameters from the never-cited preview artifact: 6 MATCH (mid-scan
preview via live snapshot, streaming range-read semantics, failure-surface
truthfulness, plus 3), 6 NOT-IMPLEMENTED as deliberate UI/host scope, 6
already covered (I-015 socket protocol + 100 MB/15 s caps, S-023
Recovery-Chances grading, M-021/I-010 repair bridge, M-028 IInWindowStream).
NEW:

- **M-066 (MAJOR)** — archive-content enumeration at preview time: the og's
  ArchivePreviewModel family browses damaged zips in preview; the core's
  zip branch hunts only `word/document.xml` (`cr_preview.cpp:685-715`), and
  S-032's children are carve-path/tree-level only — FS-origin archives
  preview opaque.
- **N-088 (MINOR)** — M-028's IInWindowStream correction didn't reach
  `cr_ui.h:1035` / `docs/API-SURFACE-AUDIT.md:94`; `cr_preview.h:116` lists
  a never-produced `mov` format.

## Fix-session delta (M-066 + N-088 closed — 2026-10-28)

- **M-066 → FIXED**: the archive-content inventory (Metadata.entries:
  name/size/compressed/crc_present; kMaxPreviewArchiveEntries = 512 +
  truncated) collected on the existing CD walk inside docx_sniff; the
  docx upgrade coexists; FS-origin archives now enumerate (the
  ArchivePreviewModel contract). Belt fix: extract_metadata's final
  truncated ASSIGNMENT clobbered the inventory flag -> OR. 4 tests.
- **N-088 → FIXED**: cr_ui.h's provenance claim, API-SURFACE-AUDIT:94's
  row name, REFERENCE-API-GAP R-33 corrected (IInWindowStream = the
  7-Zip/LZMA SDK input interface, M-028); the mov ghost removed from
  the format comment; the PreviewDocs grep guard holds the three
  contract sites.

---

## Thumbnail engine (Agent 3, 2026-10-28 — session-owner directive, engine-side superset)

The og generates thumbnails entirely on the UI side (I-015 register): GDI+
`GdipGetImageThumbnail` at 256 px (width-derived scale), the MediaGraph/ffmpeg
media agent returning BMP payloads bounded (0, 1 MiB], EURawImg/LibRaw for RAW,
libheif embedded thumbnails for HEIC, pdfium for PDF page pixmaps — the recovery
engines never decode. The session owner mandated an ENGINE-SIDE thumbnail
capability; per the I-015 in-process doctrine this lands as a deliberate,
documented superset: `cr_thumbnail` (`include/cr/preview/cr_thumbnail.h` +
`src/core/preview/cr_thumbnail.cpp`).

**v1 surface (all corpus anchors in docs/PARITY-OG-CORE-INVENTORY.md C.2):**
- Formats: JPEG (baseline+progressive), PNG, BMP/DIB, GIF (first frame), PSD,
  PNM, HDR via the vendored public-domain decoder (`third_party/stb`,
  STBI_NO_STDIO, fed through a budget-gated callback adapter — the engine never
  opens files); TIFF baseline NATIVE (uncompressed / PackBits / Deflate strips,
  8/16-bit, gray / palette / RGB / RGBA, both endians; predictor-2, tiled TIFF
  and JPEG-in-TIFF honestly refused). The PackBits count is the TIFF6 signed
  form `1-n` (the 257-n unsigned-byte reading is a classic off-by-256 — pinned
  by the packbits fixture test).
- Dims-first bomb guard: NATIVE header parses for every format (never
  `stbi_info` — stb's own header scan refuses x*y*n ~ 2^30 shapes as "corrupt",
  which would misreport bombs as decode failures); oversized inputs are
  refused before any decode allocation, proven by device-byte accounting.
- EXIF embedded-thumbnail fast path (native APP1 walk, IFD0→IFD1, tags
  0x201/0x202) for oversized JPEGs — the og-parity strategy (GDI+/LibRaw/
  libheif all prefer embedded thumbs).
- Bounded doctrine: every logical byte is accounted against
  `ThumbnailCaps.max_read` BEFORE it moves (M-028 cost lineage) + work-unit
  guard; no wall-clock bound.
- Truth gate: a thumbnail is NEVER decoded over zero-filled unreadable spans
  (UnreadableSpan status); truncated streams are disclosed (truncated flag on
  failures AND on Ok — stb's JPEG zero-fill leniency is surfaced, never
  silent); unsupported formats are named (webp/heic = documented phase-2).
- Output: straight-alpha RGBA8, aspect-preserving premultiplied box downscale,
  max_edge on BOTH axes (the og's width-only GDI+ quirk corrected), never
  upscaled.

**Deferred phases (named deps, honest `thumbnail_format_supported` = false):**
WebP (libwebp), HEIC (libheif+libde265, embedded-first), RAW (LibRaw thumbnail
API), video key-frames (ffmpeg — the biggest dependency decision), PDF page
pixmaps (pdfium), JPEG-in-TIFF / tiled TIFF. Out of scope: SVG rasterization
(og ships qsvg for UI assets), post-repair enhancement (EUImageEnhance domain),
thumbcache recovery (separate feature row — carve signatures 01 00 09 00 /
F7 89 34 2C already known).

**Tests:** `tests/unit/test_thumbnail.cpp` (20 tests incl. the 6-variant TIFF
suite; fixtures via `scripts/gen_thumbnail_fixtures.py` into
`tests/data/thumbnails/` with SHA256SUMS) + `fuzz/fuzz_thumbnail.cpp` (every
parser over hostile bytes, resident + extent-backed + tight-budget lanes).
ABI exposure of the thumbnail surface is the registered Agent 1 handoff.


## Session-8 section (2026-10-28, audited `35adcb9`)

The 05e6786 thumbnail landing verified (both-axis 256 px, dims-first bomb
guard, truncation disclosure, the I-015-lineage placement — the ABI row 27
N/A-by-layer disposition stands; registration gap → N-091). New findings:
**M-080** (MAJOR — the default 4 MiB read budget starves ordinary photos: a
6 MiB 12-MP JPEG is neither EXIF-fast-pathed (12 MP < 64 MP) nor fully
readable (6 MiB > 4 MiB) → BudgetExceeded; fix = fast-path on byte-oversize or
raise the byte default), **M-077** (the og's 100 MB eligibility / 15 s time
caps — R-34's shipped-defaults half), **N-092** (WMF/EMF/ICO unnamed in the
refusal matrix + no cache scope).


## Session-8 wave-2 — the thumbnail engine's parameter verdicts (audited `ff879f9` lineage, corpus `be0cfe9`; this lane's half — see the deep-audit lane's M-080/N-092 rows for the budget/refusal-matrix halves)

Scored against the og's thumbnail pipeline (decode evidence:
`../evidence/decompiled-quotes/s8-thumbnail-pipeline.md`):

- **S-043 (SEVERE)** — EXIF orientation not applied (og rotates per tag
  0x112 before the 256-px pass, FUN_1402fcc30 :521694-725 + call order
  :652307-309; the core has zero orientation code).
- **S-045 (SEVERE)** — format coverage: og ships video-keyframe (ffmpeg
  agent, ports 20171/2), RAW (LibRaw/EURawImg, 1/8-downscale+PNG), HEIC
  (heif.dll embedded-first), PDF (pdfium) pipelines; the core refuses all
  by name (the registered phase-2 deps). Complements N-092's unnamed
  WMF/EMF/ICO trio + cache gaps.
- **M-083 (MAJOR)** — thumbnail success promotes the node to grade 2 in
  the og (:422505-13); the core's engine is grade-agnostic (S-023 family).
- **M-085 (MAJOR)** — TIFF LZW (compression 5) refused; a mainstream
  scanner/Photoshop variant the og's GDI+ decodes.
- **M-080 addendum** — this lane decoded the og's still-image eligibility
  gate (200 MB, :422428) correcting that finding's "no input cap" claim;
  severity discussion (SEVERE recommended) recorded in the addendum.
- **I-028 (INFORMATIONAL)** — the superset register: both-axis scaling
  (og is width-derived — portrait thumbs exceed 256 on height),
  straight-alpha RGBA8 (og white-flattens), PSD/PNM/HDR, dims-first bomb
  guard, UnreadableSpan truth gate. Deliberate; do not "fix" away.
- **N-103 (MINOR)** — R-28/PLAN-API-EXPANSION still read "no dedicated
  thumbnail op"; the cr_ui ABI exposure remains the open Agent-1 handoff.
- MATCH rows: still-image family coverage (JPEG/PNG/BMP/GIF + TIFF
  baseline), bounded range-read doctrine, never-upscale-below-threshold,
  honest named refusals, the EXIF embedded fast-path strategy.
- UNVERIFIED (corpus silent, tracked in I-028): og bomb policy,
  unreadable-span behavior, GdipGetImageThumbnail internals, og
  WebP/SVG/AVIF thumbnail paths, the 200×200 GDI surface, the
  :828027 format-group registry's consumer.
