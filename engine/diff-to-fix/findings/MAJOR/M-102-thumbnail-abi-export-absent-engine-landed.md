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
