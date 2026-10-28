# MAJOR M-102 — the thumbnail engine has no ABI surface: the 05e6786 engine (make_thumbnail, 20 tests, the registered I-015 superset) is unreachable from cr_ui — no export exists, so a C host cannot generate thumbnails at all

**Severity:** MAJOR
**Module / Surface:** ABI / preview (modules/abi.md, modules/preview.md)
**Parameters affected:** A42 (public surface shape — the engine exists but is C++-only), 65-adjacent (the engine's own tests are green; the ABI half is the gap), 46-adjacent (the contract is undocumented at the ABI level)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/10-preview-subsystem.md` (the og's thumbnail surface: CGetListThumbnailThread, the async per-row fetch, the 256-px GDI+ convention — DRWUI_decompiled.c:652395-652410); the PARITY-OG-CORE-INVENTORY §D handoff (the thumbnail ABI is the recorded Agent-1 handoff).
**Core references:** audited commit `ff879f9` — `include/cr/preview/cr_thumbnail.h` + `src/core/preview/cr_thumbnail.cpp` (the 05e6786 engine: make_thumbnail(pr, caps, out), 1,045 lines, 20 tests) — **grep for a cr_ui thumbnail export: zero hits** (cr_ui.h carries no thumbnail symbol; the PARITY-OG-CORE-INVENTORY §C.2 row records the engine DONE and the ABI as the handoff).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** the thumbnail engine landed as a deliberate superset (I-015 lineage, registered); the ABI half was recorded as the Agent-1 handoff and never formalized as a finding — this closes the loop so the roadmap carries it with an ID.

## Summary

Core's thumbnail engine is landed and tested (v1 still images: JPEG/PNG/BMP/GIF/TIFF + PSD/PNM/HDR superset) but exists only as the C++ API — no cr_ui export exposes it. A C host (the ABI's primary consumer, and the CLI) cannot render result thumbnails at all, while the og ships thumbnails as a core results-page element. The export needs the two-call size pattern, the caps pass-through, and the format-support probe.

## What the reference does

The og's results page fetches thumbnails per row through a dedicated worker (UIUX/10 §2; the 256-px convention) — host-side, but backed by engine-readable data at 256 px.

## What our core does

make_thumbnail exists (C++); cr_ui.h has no thumbnail symbol; rrecov has no thumbnail verb — the engine is unreachable from every shipped consumer surface.

## Gap

Falsifiable: grep cr_ui.h for thumbnail — zero; the og's host renders thumbnails for every image row; core's C hosts cannot.

## Impact

The whole results-page thumbnail UX is blocked on the ABI half; the engine's 20 tests validate a surface nobody can call.

## Reproduction / verification

Grep-verified this session (cr_ui.h + the PARITY-OG-CORE-INVENTORY §C.2/§D rows).

## Fix plan (for the engineering team)

1. cr_ui_thumbnail_generate(h or file-path + range, caps, out-buf two-call pattern, format-probe export) — the established house conventions (two-call size, caller-owned buffers, append-only export numbering + the 81-export count-site lock). 2. Truncated/unreadable disclosure must cross the ABI verbatim (the engine's Ok-but-truncated contract). 3. ABI-CONTRACT section + the count sites (81→82) + the API-DELTA row flip. 4. Optional CLI verb (thumbnail <ref> → PNG file) for smoke testing.

## Tests to add

- `CrUi.ThumbnailTwoCallPattern` — probe + fill, the ABI-CONTRACT smoke.
- `CrUi.ThumbnailTruncationDisclosed` — the truncated case crosses the ABI honestly.
- `CrUi.ThumbnailUnsupportedFormat` — the format-probe returns false for deferred formats.
- `ui_abi_check.c` — the C-consumer gate block.

## Related findings

M-077 (the preview caps — the same subsystem); the 05e6786 engine's I-015 registration; PARITY-OG-CORE-INVENTORY §D (the handoff note this formalizes).

## Open questions

Async batch fetching (the og's per-row worker pattern) is host-side by the in-process doctrine (I-015) — the ABI stays synchronous per the house pattern; recorded so the fix does not over-build.

## Resolution

**Status:** FIXED
**Fixed in commit:** <this landing>
**Verified by:** `CrUi.ThumbnailTwoCallPattern` (probe + too-small-refusal + fill with exact quadrant pixels + the caps pass-through at max_edge 8), `CrUi.ThumbnailTruncationDisclosed` (the truncated JPEG crosses the ABI honestly — Ok+truncated=1 or the named refusal, never silent), `CrUi.ThumbnailUnsupportedFormat` (the probe's true/false matrix + the text node's named Unsupported), the plain-C consumer block in `tests/ui_abi_check.c` (the two-call + the bad-argument gates), and the engine pin `Thumbnail.PngDecodesAndDownscalesExactly.truncated==0` (the poisoned-disclosure defect this landing's fail-before caught — see notes). Full ctest **1075/1075** linux-release (gcc Werror); the 4 ABI tests are the fail-before proof themselves (they do not compile against a tree without the export — a C host could not generate thumbnails at all, the finding's own impact line).
**Resolution notes:** Two exports landed in the house shapes: `cr_ui_thumbnail_generate(dev, sess, node_id, caps, rgba, rgba_cap, info)` — the two-call pattern (rgba NULL = the size probe: the decode runs because output dims depend on it, pixels discarded; a too-small buffer is the named 0x50030009 ERR_NO_MEMORY with `bytes_needed` preserved and nothing partially written) — and `cr_ui_thumbnail_format_supported(utf8)`. The caps block mirrors the engine's `ThumbnailCaps` with per-field zero-defaults (NULL caps = all defaults; nonzero values ride the engine's own validation — no silent blending of half-sets). The reader follows the preview surface's F-1 basis rule (the parent disk when the scan device is a volume slice). M-083's grade promotion is wired INTO the success path (see its own resolution). DISCOVERED-DURING-FIX (the ABI test's fail-before): the engine reported `truncated=1` for COMPLETE PNGs — stb's post-parse EOF probe (reading past IEND to detect stream end, the API shape) poisoned the disclosure signal, meaning "truncated" would have crossed the ABI as false data for every strict-format image. Root-cause fix in the engine: JPEG is the ONE lenient family (the zero-fill leniency the finding's own contract text names); the strict decoders (png/bmp/gif/psd/pnm/hdr) refuse short streams outright, so their Ok proves completeness — `out.truncated` now gates on `is_jpeg && hit_eof`, pinned by the engine test. Docs: ABI-CONTRACT §7i (the full contract: two-call, caps, truncation-is-data, status mapping, the grade side-effect) + the count sites (83 exports) + the API-DELTA 17b row. No version bump (§9: appended exports + new structs only). The async worker stays host-side per the in-process doctrine (the finding's open question — not over-built).
