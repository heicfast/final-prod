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
pixmaps (pdfium), JPEG-in-TIFF / tiled TIFF, **WMF/EMF metafiles + ICO/CUR icons
(GDI+ on Windows — the og's own native rasterization path; named-and-refused
since the N-092 fix: placeable-WMF key 0xD7CDC69A, standard-WMF type/header-size
header, EMF `" EMF"`@+40, ICO/CUR type word — never "unknown")**. Out of scope:
SVG rasterization (og ships qsvg for UI assets), post-repair enhancement
(EUImageEnhance domain), thumbcache recovery (separate feature row — carve
signatures 01 00 09 00 / F7 89 34 2C already known).

**Cache scope (decided + registered, the N-092 fix's second half):** no
engine-side thumbnail cache. The og caches UI-side (`ThumbnailFactory::ClearCache`,
UIUX/30:62); the core's in-process doctrine keeps `make_thumbnail` stateless
across calls — repeated renders are the host's layer (a session-scoped LRU keyed
by node id becomes the candidate for a future host-side cache — the ABI export landed
as `cr_ui_thumbnail_generate` in `e56baac7`, M-102/M-083 retired (PARITY-VERIFIED by
session-11-lead, `evidence/s11-board-truth.md`; the residual header-contradiction filed
as N-137); the engine stays cacheless per this doctrine — the cache question re-opens
with the landed export). Recorded here + in `include/cr/preview/cr_thumbnail.h`'s contract
comment; the decision is deliberate, not an omission.

**Tests:** `tests/unit/test_thumbnail.cpp` (21 tests incl. the 6-variant TIFF
suite and the metafile/icon named-refusal pin; fixtures via
`scripts/gen_thumbnail_fixtures.py` into
`tests/data/thumbnails/` with SHA256SUMS) + `fuzz/fuzz_thumbnail.cpp` (every
parser over hostile bytes, resident + extent-backed + tight-budget lanes).
ABI exposure of the thumbnail surface LANDED (`cr_ui_thumbnail_generate` +
`cr_ui_thumbnail_format_supported`, `e56baac7`; M-102/M-083 retired after the
session-11 independent verification — `evidence/s11-board-truth.md`; the residual
header-contradiction filed as N-136).


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

- **S-043 (FIXED)** — EXIF orientation not applied (og rotates per tag
  0x112 before the 256-px pass, FUN_1402fcc30 :521694-725 + call order
  :652307-309; the core has zero orientation code). Fixed by the
  closing-fix session: the 8-case transform (JPEG APP1/IFD0 0x112 + TIFF
  tag 274) applied before the downscale; the embedded fast path serves
  as-stored per the CIPA DC-008 convention with the IFD1-declared 0x112
  as the not-already-oriented marker (registered PARITY-DELTAS D-105);
  8 tests, fail-before executed — Resolution in findings/FIXED/S-043.
- **S-045 (SEVERE)** — format coverage: og ships video-keyframe (ffmpeg
  agent, ports 20171/2), RAW (LibRaw/EURawImg, 1/8-downscale+PNG), HEIC
  (heif.dll embedded-first), PDF (pdfium) pipelines; the core refuses all
  by name (the registered phase-2 deps). Complements N-092's unnamed
  WMF/EMF/ICO trio + cache gaps.
- **M-083 — FIXED (`e56baac7`; session-11B conformed)** — thumbnail success
  promotes the node to grade 2 in the og (:422505-13); landed with the
  cr_ui_thumbnail_generate success path.
- **M-085 (MAJOR)** — TIFF LZW (compression 5) refused; a mainstream
  scanner/Photoshop variant the og's GDI+ decodes.
- **M-080 addendum** — this lane decoded the og's still-image eligibility
  gate (200 MB, :422428) correcting that finding's "no input cap" claim;
  severity discussion (SEVERE recommended) recorded in the addendum.
- **I-028 (INFORMATIONAL)** — the superset register: both-axis scaling
  (og is width-derived — portrait thumbs exceed 256 on height),
  straight-alpha RGBA8 (og white-flattens), PSD/PNM/HDR, dims-first bomb
  guard, UnreadableSpan truth gate. Deliberate; do not "fix" away.
- **N-103 — FIXED (session 38, `e4f3184`)** — R-28/PLAN-API-EXPANSION rows;
  the cr_ui ABI exposure landed as M-102's `cr_ui_thumbnail_generate`.
- MATCH rows: still-image family coverage (JPEG/PNG/BMP/GIF + TIFF
  baseline), bounded range-read doctrine, never-upscale-below-threshold,
  honest named refusals, the EXIF embedded fast-path strategy.
- UNVERIFIED (corpus silent, tracked in I-028): og bomb policy,
  unreadable-span behavior, GdipGetImageThumbnail internals, og
  WebP/SVG/AVIF thumbnail paths, the 200×200 GDI surface, the
  :828027 format-group registry's consumer.


## Session-8 lead-lane wave (audited `ff879f9`)
- **M-100**: archive browse-inside preview (ArchivePreviewModel — the in-place damaged-zip
  entry browse) — absent; reuses the S-032 parser; formalized from R-37.
- **M-102 — FIXED (`e56baac7`; session-11B conformed)**: the thumbnail ABI
  export — `cr_ui_thumbnail_generate` (two-call) + the format probe landed;
  the file retired to history per the purge convention (the retirement the
  e56baac7 landing itself missed — session-11B/T5 executed it).

---

## Session-39B resolution (2026-10-29, M-108 FIXED)

**M-108 → MATCH.** The og's TxtViewFiles extension-whitelist model landed:
`src/core/preview/cr_preview_text_exts_data.cpp` (generated by
`scripts/gen_preview_text_exts.py` from the corpus FilterFolder.ini — the
TxtViewFiles value byte-exact, 97 tokens / 92 unique case-folded; the
finding-time "119" was the artifact's ellipsis-note miscount, corrected in
the Resolution) + `cr_preview_text_exts.cpp` (the case-insensitive
membership — FUN_140310aa0's toupper fold, decoded first-hand this session)
+ the `extract_metadata` fall-through now gates on extension membership:
a whitelisted extension renders as text whatever its bytes (the BOM-aware
snippet: UTF-8 BOM skipped, UTF-16LE/BE decoded including surrogate pairs
with the N-016b U+FFFD rule, Latin-1/high bytes passed raw for the host to
render); a non-whitelisted extension is `unsupported` (the printable-head
false-positive class closed); an extensionless name is `unsupported` (the
og's empty-needle early return, decoded this session). The
previewer-resolution precedence for a whitelisted extension whose CONTENT
matches a rich magic (a .txt file starting with "PK") is corpus-silent —
the core keeps the rich previews first; registered in PARITY-DELTAS. Tests:
PreviewMeta.Utf8BomTextIsText / Utf16BomTextIsText /
Utf16SurrogatePairDecodes / Latin1TextIsText / PrintableHeadBinaryNotText /
ExtensionlessNameIsNotText / WhitelistMembershipCaseInsensitive +
PreviewTextWhitelist.ByteExactAgainstCorpus — the fail-before PROVEN by
mutation (the pre-fix printable-head heuristic restored: 7 RED, among them
every M-108 behavior test). Suite 1102/1102 (ctest, Linux release).

## Session-10 lane-B addendum (2026-10-29, audited `84a1832`)

**M-108 (MAJOR, NEW — renumbered from the lane's M-103 at push):** the text-preview
eligibility model diverges — the og gates by extension membership in the 119-entry
TxtViewFiles whitelist (decompile-verified at DRWUI:535858/:53937); the core
content-sniffs the first 16 bytes and refuses any byte ≥ 0x7F. EXECUTED proof:
UTF-8-BOM, UTF-16LE, and Latin-1 text fixtures all return `kind=unsupported` (no
snippet) while a printable-head binary returns `text` with a garbage snippet
(`evidence/executions/s10-preview-text-eligibility.md`). The TxtViewFiles whitelist
is absent from the core (API-DELTA row 44's "landed" claim is false — N-132).
(Post-rebase note: the parallel lanes landed S-043/M-080/M-085/S-045-partial — the
thumbnail family moved; M-108 completes the PREVIEW text-eligibility surface, which
no prior finding owned.)

## Session-10 wave-3 addendum (2026-10-29, landing-parity verification)

- **M-109 → MATCH (session 39B, FIXED)**: max_read = 0xC800000 (the og's
  still-image eligibility gate, DRWUI :422425-429 re-verified first-hand);
  the refuted "no input cap" doctrine replaced at all four sites; the band
  proof a real 34 MiB in-test-built PNG (build_large_png); the past-gate
  refusal pinned at 200 MB+1. DISCOVERED during the band test:
  **M-112** (the StbioSource single-shot read defect — every PNG with a
  > 4 MiB IDAT failed outofdata; the read_cb loop landed with the band
  test as the shared regression proof; filed per the §8 duty).
- **M-108 (MAJOR)** — the M-080 landing's residual: the og's 200 MB
  thumbnail-eligibility anchor (DRWUI FUN_1402636e0, :422428-area) not
  adopted; the 32–200 MB band without an embedded EXIF thumb still
  BudgetExceeded; the pinning test cites the refuted "no input cap"
  doctrine. Filed from the landing-parity pass (evidence/
  s10-landing-parity-verification.md).
- Verified PARITY-CLEAN this wave: M-085's LZW (every semantic, incl. the
  early-change growth point — independently reproduced against a
  from-scratch reference decode), M-080's two mechanics (the residual is
  M-108's), S-043's landing was verified in the earlier sweep batches.

### Session-11-lead rows (2026-10-29)

- **M-115** (MAJOR): the promotion trigger is mis-modeled — the og promotes on clean recovery-READ success (decode-independent, tier-gated, preview-pane-excluded); core promotes only on decode Ok. Core UNDER-promotes damaged-but-readable images (the dominant case); the text/media triggers absent (their surfaces below); the <=20-byte over-promotion edge. OQ-S11-1 RESOLVED (the decode, binary-verified at 0x140263b2c-b9a).
- **N-144** (MINOR): the TxtView text-snapshot thumbnails (<=100 bytes, Arial 13, 200x200 BMP; TxtViewFiles-gated) absent — core refuses text outright.
- **N-148 item 3** (MINOR): cr_ui.cpp:3754's og cite is 9 lines loose (the guard is :422507-514).
- Verified MATCH this session: the 19-behavior results-phase interaction map (filter/sort/search/preview/recover/session/regrade families; the rest host-side by doctrine) — §Lane-C Section B.

### Session-42-C rows (2026-10-29 — the S42-C continuation lane, post-third-reconciliation) (2026-10-29 — the S42-C continuation lane, post-reconciliation)

- **M-121** (MAJOR, retroactively filed + retired at FIXED/): the StbioSource
  single-shot read vs the reader's 4 MiB clamp — every photo-scale PNG (>4 MiB
  IDAT) failed `outofdata` once M-109 raised the band to the og's 200 MB; the
  landing's own band test caught it and the same commit (`76e2d351`) fixed it
  with the read loop (cr_thumbnail.cpp:163-181) — but the finding was never
  filed under any ID (the S12 lane's twice-interrupted Wave-1 duty; the S42-C
  retroactive filing; RENUMBERED TWICE at the hot-window reconciliations: the
  collision-draft M-112 [the ACTIVE CSL finding owns it] → the S12-planned M-118
  [consumed by the remote S12 lane's MountPoints2] → M-122 [the DBX chain owns M-121]). The phantom-SHA
  ledger rows repaired with it. The one live code-side residual: the loop's
  comment at cr_thumbnail.cpp:163 still cites "N-112" — one-line repair for the
  next code-touching lane (carried in M-121's fix plan; verified standing at
  the reconciled tree).
- The `Thumbnail.*` pin family re-verified green at the pin by the landing-parity
  pass (evidence/s42c-landing-parity.md §8).
## S42 continuation lane (2026-10-29 — the preview focus battery; core `b26017a7`)

- **Q46 (media metadata families) — DIVERGENCE-MINOR, N-184**: extract_metadata
  implements MP4 + WAV only (cr_preview.cpp:985-1079); AVI/MKV/MOV/FLAC/MP3/M4A
  return unsupported where the og's agent-side pipeline derives metadata for every
  family it opens. The battery's covered ground (M-108 landed, N-144/M-115 active,
  I-028/OQ-S10-1 registered) re-verified current; the doc-truth staleness cluster
  (R-28/R-34/ABI-CONTRACT/API-DELTA rows) filed as N-180.
