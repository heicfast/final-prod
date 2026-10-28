# INFORMATIONAL I-028 — Thumbnail engine superset register: deliberate core improvements over the og pipeline (both-axis scaling, alpha preservation, PSD/PNM/HDR, bomb guards, truth gates) + the UNVERIFIED residuals

**Severity:** INFORMATIONAL
**Module:** preview (thumbnail engine)
**Parameters affected:** A25 (algorithm — deliberate differences), P49/P50 (memory/CPU posture), B2-adjacent (output semantics)
**Corpus references:** `DRWUI_decompiled.c` — the og's width-derived scale (:652388-410, evidence §2), white-flatten HBITMAP output (:652388 via GdipCreateHBITMAPFromBitmap with 0xff,0xff,0xff), no dims-first bomb guard found in the corpus, no unreadable-span refusal path found; the og's PSD/PNM/HDR thumbnail support: no corpus evidence found (repair-UI string tables only)
**Core references:** audited `ff879f9` — `src/core/preview/cr_thumbnail.h:24-27` (both-axis ≤256 box, never upscale — the header claims to correct the og's width-derived quirk, now corpus-evidenced), `:67-74` (straight-alpha RGBA8 vs the og's white-flattened HBITMAP), PSD/PNM/HDR via vendored stb (:201-228), the dims-first native guard (:778-910), the UnreadableSpan truth gate (:916-26, :982-85), truncated-on-Ok disclosure (:994-98); the engine landed `05e6786`
**Status:** READ-ONLY FINDING — no fix applied (deliberate differences, recorded)

## Summary

The core's thumbnail engine deliberately improves on the og pipeline
in five registered ways, all now corpus-evidenced as differences:
(1) both-axis ≤256 scaling (the og's scale is width-derived — portrait
thumbnails can reach 256×1280); (2) straight-alpha RGBA8 output (the
og flattens to white HBITMAPs); (3) PSD/PNM/HDR decode support beyond
the og's GDI+ set; (4) a dims-first decompression-bomb guard the
corpus shows no og equivalent for; (5) the UnreadableSpan truth gate
(never a thumbnail over zero-filled spans) with truncation disclosure.
None of these is a defect; all are recorded so future parity work
does not "fix" them away.

## What the reference does / What our core does

Tabulated above with citations; the full parameter matrix is in the
session's agent G-C report and `evidence/decompiled-quotes/
s8-thumbnail-pipeline.md`.

## Gap

None (deliberate, documented superset posture).

## Impact

Positive divergences. The register exists so the divergence ledger
stays truthful — the og's width-derived quirk means og portrait
thumbnails exceed 256px on height, which the core never does (a
visible difference an eagle-eyed user could notice in a side-by-side).

## Reproduction / verification

Corpus-evidenced (the scale/flatten/output cites above); core code
read directly this session.

## Fix plan

None — keep the register. When the ABI lands (N-103), document the
both-axis + RGBA8 contract in ABI-CONTRACT so hosts do not assume
og-shaped output.

## Tests to add

None (the existing 21 test groups pin the behavior).

## Related findings

S-043/M-080/S-045/M-083/M-085 (the defect side of the same audit),
N-103 (the ABI handoff).

## Open questions

UNVERIFIED residuals (corpus silent, no og claim either way): the
og's decompression-bomb policy (GDI+ internal allocation); the og's
behavior on unreadable device spans mid-thumbnail; whether
GdipGetImageThumbnail returns embedded thumbnails (GDI+ internals);
og WebP/SVG/AVIF thumbnail decode (repair-UI strings only); the
purpose of the 200×200 GDI surface in the work item (:422448); the
consumer of the :828027 format-group registry. All tracked here as
context, none blocking.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

benign cite drift: the RGBA8/both-axes contract now at cr_thumbnail.h:24-27 (exact); PSD/PNM/HDR :42-43; test count grew 21→30; the anticipated ABI documentation landed (ABI-CONTRACT §7i). Register content fully accurate.

## Resolution

**Status:** ACKNOWLEDGED (the deliberate superset posture confirmed and preserved; register content verified accurate at session-42 HEAD)
**Fixed in commit:** this session's I-028 landing (session-42; cite-by-title: "fix(trail): I-028 - the thumbnail superset register acknowledged at session-42 HEAD")
**Verified by:** the register's cites re-verified at current HEAD: `include/cr/preview/cr_thumbnail.h` (the both-axis max-edge + straight-alpha RGBA8 contract text; PSD/PNM/HDR via vendored stb; the UnreadableSpan truth gate); `docs/ABI-CONTRACT.md` carries the RGBA8/orientation output contract (the anticipated §7i documentation landed, as the S11B addendum recorded); the thumbnail test group count now 32 (grew 21→30→32 — the pins live and extending). No divergence from the register's claims found.
**Resolution notes:** The five registered deliberate improvements over the og's thumbnail pipeline — (1) both-axis ≤256 scaling (vs the og's width-derived scale that lets portrait thumbnails reach 256×1280), (2) straight-alpha RGBA8 output (vs the og's white-flattened HBITMAPs), (3) PSD/PNM/HDR decode beyond the og's GDI+ set, (4) the dims-first decompression-bomb guard, (5) the UnreadableSpan truth gate with truncation disclosure — are CONFIRMED intentional and are preserved. Future parity work must NOT "fix" these toward the og's shape; hosts consume the documented ABI-CONTRACT output contract. The UNVERIFIED residuals in the Open questions (the og's GDI+-internal bomb policy, unreadable-span behavior, embedded-thumbnail return, WebP/SVG/AVIF decode) remain corpus-silent and non-blocking; the codec-bound half rides OQ-S10-1.
---
---

## Resolution (session-42, 2026-10-29 — RETIRED to FIXED/)

**Verified FIXED at core `869da779`.** The finding's own fix plan asked
exactly one thing — "when the ABI lands, document the both-axis + RGBA8
contract in ABI-CONTRACT" — and it landed: `docs/ABI-CONTRACT.md` §7i
(:599-625, rode the M-102 `cr_ui_thumbnail_generate` landing) documents the
two-call size-probe pattern, the display-oriented RGBA8 output with the
EXIF-before-downscale transform, the max_edge-256/max_pixels-64MP/max_read-
32MiB/max_work-1<<22 caps contract, the truncation-is-data mapping, and the
M-083 grade-promotion side-effect. The engine contract itself
(`include/cr/preview/cr_thumbnail.h:24-27`, both-axis never-upscale,
straight-alpha RGBA8) is intact and pinned by 30 test groups. The
register's UNVERIFIED-residual list is context-only (corpus-silent items,
none blocking). Liveness sweep S42 verified first-hand: `cr_ui.h:1290`
(`cr_ui_thumbnail_generate`), `cr_ui.cpp` thumbnail dispatch, ABI-CONTRACT
§7i present at HEAD. No further work owed; the register remains in force
as the deliberate-divergence record.

**Board action:** moved to `findings/FIXED/`; FIXED-LEDGER row appended.
