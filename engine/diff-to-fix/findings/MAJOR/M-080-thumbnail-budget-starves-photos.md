# MAJOR M-080 — Thumbnail default read budget starves ordinary photos: a 6 MiB 12-MP JPEG gets no thumbnail under the shipped caps

**Severity:** MAJOR
**Module:** preview (thumbnails)
**Parameters affected:** X47/X48 (throughput/latency of the thumbnail path), B2-adjacent (no recovery effect — presentation quality), Q65 (test coverage of the budget boundary)
**Corpus references:** `re-workspace/artifacts/UIUX/10-preview-subsystem.md` + `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` §4 (:66-98 — the ≤1 MiB bound there is the *output payload* on the socket leg; the 100 MB value is preview eligibility, UIUX/10:23); `DRWUI_decompiled.c:652377-652419` (`FUN_1403d7840` — the og's GDI+ thumbnail path: width ≤ 0x100 → as-is; else scale = width/256.0; **no input-size cap on the GDI+ path**)
**Core references:** audited `35adcb9`. `include/cr/preview/cr_thumbnail.h:61-62`: `u64 max_pixels = 64ull*1024*1024; u32 max_read = 4u<<20;` (the shipped defaults); `src/core/preview/cr_thumbnail.cpp:951` — the embedded-EXIF fast path fires **only** when `u64(w)*u64(h) > caps.max_pixels` (:951-970); `:1009` the same guard on the decode path; `BudgetExceeded` at `cr_thumbnail.h:89` ("caps.max_read / max_work exhausted").
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The thumbnail engine's default `max_read` is 4 MiB, and its embedded-EXIF fast path triggers only for images whose pixel count exceeds `max_pixels` (64 MP). A byte-heavy but pixel-benign photo — the common case: a 6 MiB 12-MP JPEG from any modern phone — falls between the two guards: it is not fast-pathed (12 MP < 64 MP) and cannot be fully read (6 MiB > 4 MiB), so `make_thumbnail` returns `BudgetExceeded` and the user gets no thumbnail. The og's GDI+ path has no input-size cap on this class and thumbnails it.

## What the reference does

The og's UI-side `FUN_1403d7840` (DRWUI:652377-652419) scales any image GDI+ can load: width ≤ 256 → served as-is; else scale = width/256.0, thumbnail 256 wide. No input byte cap gates the GDI+/streaming thumbnail path (the corpus's ≤1 MiB bound is the socket *output* payload; 100 MB is the separate preview-eligibility policy). Ordinary multi-megabyte JPEGs/PNGs get 256-px thumbnails.

## What our core does

`Caps{max_read = 4 MiB, max_pixels = 64 MP}` (cr_thumbnail.h:61-62). The decode path checks pixels first (:951, :1009) and reads under the byte budget; the embedded-EXIF fast path (which avoids the full read) is gated on `w*h > max_pixels` — i.e., it exists to serve *huge-pixel* images, not *huge-byte* ones. A 6 MiB 12-MP JPEG: fails the fast-path guard (12 MP < 64 MP), then fails the read budget (6 MiB > 4 MiB) → `BudgetExceeded`. Verified by reading both guards; the test suite pins the bomb-guard and oversized-JPEG behaviors but not this middle case.

## Gap

Under shipped defaults, any image with `file_size > 4 MiB` and `w*h ≤ 64 MP` returns `BudgetExceeded` — no thumbnail. Falsifiable: a 6,291,456-byte 4000×3000 JPEG returns BudgetExceeded in core; the og returns a 256-px thumbnail. (Class distribution: most consumer photos since ~2012 are 2-8 MiB at 8-16 MP — the guard intersection is the mainstream case, not an edge.)

## Impact

Presentation-quality regression users see constantly once hosts bind the thumbnail surface (the ABI surface is the pending Agent-1 handoff per preview.md): the most common thumbnail request class fails. No recovery-correctness effect. The caps are caller-tunable, but the engine default is the current contract.

## Reproduction / verification

- Core first-hand: both guards read (cr_thumbnail.cpp:951-970 fast-path gate; the Budget plumbing :1026-1033); the defaults read (cr_thumbnail.h:61-62); `BudgetExceeded` semantics read (cr_thumbnail.h:89).
- Corpus-derived: FUN_1403d7840's no-input-cap GDI+ path (decompile read; the output-payload vs input-cap distinction pinned from UIUX/10 + DRWUI-subsystems §4).
- Logical verification: 4000×3000 = 12 MP ≤ 64 MP (no fast path); 6 MiB > 4 MiB (budget exceeded). The fixture class is constructible; not executed this session (no photo fixture in the tree) — the guard logic is decisive from source.

## Fix plan (for the engineering team)

1. Extend the embedded-EXIF fast path to byte-oversized inputs: fire it when `file_size > caps.max_read` AND the format is JPEG-with-EXIF-thumbnail (the fast path reads only the embedded thumb — a few KiB), not only when pixels exceed `max_pixels`. This is the surgical fix: the og's GDI+ thumbnail is *also* an embedded-first consumer (libheif's `heif_image_handle_get_thumbnail` is the corpus's embedded-first precedent on the HEIC leg).
2. Alternatively (or additionally), raise the default `max_read` for the thumbnail path specifically (e.g., 32 MiB) with the decode still pixel-capped at 64 MP — memory stays bounded by `max_pixels` (256 MiB RGBA worst case), the byte cap only gates I/O.
3. Decide before the ABI handoff (preview.md's Agent-1 note): the shipped default IS the ABI contract hosts will inherit.
4. Tests: the middle-class fixture (6 MiB 12-MP JPEG → thumbnail Ok, embedded path); a byte-oversized image without embedded thumb (→ the documented refusal, not a hang); the 64 MP guard unchanged.

## Tests to add

- `Thumbnail.OrdinaryPhotoDefaultBudget` (6 MiB 12-MP JPEG → Ok under zero-config caps — fails before, passes after).
- `Thumbnail.EmbeddedFastPathOnByteOversize` (byte-oversized + EXIF thumb present → served from embedded).
- `Thumbnail.RefusalMatrixNamesWmfEmfIco` (see N-092 — pin every refusal class by name).

## Related findings

- N-092 (this session — the thumbnail refusal-matrix gaps; same engine, different gap).
- M-077 (the preview caps — the sibling product-cap seam).
- I-015/D-59h (the in-process preview register the thumbnail engine extends).

## Open questions

None.

## Session-8 wave-2 addendum (the parallel continuation lane — the og's eligibility gate decoded; severity discussion)

First-hand decode of the og's thumbnail work item `FUN_1402636e0`
(:422375-560) corrects one claim in this finding's reference section and
sharpens its fix target:

1. **The og DOES bound still-image input — at 200 MB, not "no cap."** The
   work item skips still images with `size > 0xc800000` (200 MB) at
   :422428, and skips media (video/audio categories) below 64 KiB
   (0x10000) at :422464; the RAW sidecar path carries its own ≈200 MB
   bound (0xc800014, :760183). The GDI+ path itself is uncapped (the
   body's reading stands), but the pipeline's eligibility gate is the
   corpus anchor: **still images ≤ 200 MB thumbnail; > 200 MB skipped.**
   Evidence: `../../evidence/decompiled-quotes/s8-thumbnail-pipeline.md` §4.
2. **The wrong anchor is pinned by a test.** `CapsDefaultsPinTheCorpusAnchors`
   (test_thumbnail.cpp:131-138) pins `max_read = 4 MiB` citing "corpus
   anchors" — the value was borrowed from M-028's PreviewReader cost
   doctrine, a different surface. The re-pin belongs in this finding's
   fix (step 2): the og-side anchor for thumbnail input is the 200 MB
   eligibility gate.
3. **Severity discussion (recorded, not escalated):** this lane graded
   the same divergence SEVERE on first filing (the modal modern photo —
   12-48 MP, 5-15 MB — fails under shipped defaults; the thumbnail
   surface produces nothing for the majority of its intended input
   class). The first-filed MAJOR stands on the board per the extend-
   never-overwrite rule; the recommendation to re-grade SEVERE is
   recorded here for the fix-order owner: the two readings differ on
   whether "presentation quality with no recovery effect" (MAJOR) or
   "user-observable capability loss on the modal input" (SEVERE) is the
   right lens for a surface whose entire purpose is presentation.
4. The embedded-EXIF fast path's role is confirmed on the og side
   (EUImgRepairFile's `heif_image_handle_get_thumbnail` is the
   embedded-first precedent; GDI+ `GetThumbnailImage`'s internals stay
   UNVERIFIED — tracked in I-028).

**Status:** READ-ONLY — no fix applied.
