# MINOR N-144 — TxtView text-snapshot thumbnails absent: the og renders the first ≤100 bytes of text files as a 200×200 Arial-13 BMP thumbnail (and grades on its success); core refuses text formats outright at the thumbnail surface

**Severity:** MINOR (a missing thumbnail surface for the text family — user-visible in the og's results grid (text files show a content-preview image), absent in core; compounds M-112's text-trigger grading gap)
**Module:** preview/thumbnail (the text-family render path)
**Parameters affected:** B1 (the thumbnail input-eligibility family), P47-adjacent (the render cost), D15-adjacent (the grade promotion the og wires to this render — owned by M-112)
**Corpus references:** `ghidra-drwui/DRWUI_decompiled.c:422447-422455` — the text-snapshot branch: when the file's extension is in FilterFolder.ini `[Filter] TxtViewFiles`, `FUN_1403d7ee0(renderer, data, 200, 200)` (`:652606-652633`, the GDI CompatibleDC+bitmap render) then `FUN_1403d88e0` (`:652885-652959`): GlobalLock the recovered data, take the first `min(100, size)` bytes (`:652919-652921`), sanitize into a 0x6e buffer (`:652914-652922`), non-empty check (`:652940`), font "Arial" size 13 (`:652947-652948`), draw, pack a BMP-in-HGLOBAL with the 'BM' header (`FUN_1403d8090` at `:652710`), fetch via `FUN_1403d83b0`; the payload becomes the work item's thumbnail and sets `local_167 = 1` (the M-112 promotion trigger); the config anchor: `[Filter] TxtViewFiles` default `".txt;"` (`FUN_1403148a0` at `:535890-535964`, the default string `:535934-535937`; `artifacts/config-files/config-data-reference.md:39`); the 119-extension TxtViewFiles list (the M-108 eligibility whitelist — the same config key's full shipped value)
**Core references:** audited `461825d8`-lineage — `include/cr/preview/cr_thumbnail.h:39-42` (the format matrix: text formats not in the supported set), `src/ui/cr_ui.cpp:3777+` (`cr_ui_thumbnail_format_supported` returns false for text), `:3689-3693` (the named-refusal mapping for unsupported formats); no text render path exists
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11-lead Lane C decode (the promotion-guard work surfaced the render branch); NOT covered by M-108 (the text-PREVIEW eligibility model — the preview panel's read/view surface; this is the THUMBNAIL grid surface), N-092 (WMF/EMF/ICO named refusals), OQ-S10-1 (codec-bound image formats), M-112 (the grading trigger that rides this render — filed separately: this finding owns the render capability, M-112 owns the grade semantics), or I-028 (the thumbnail superset register — text snapshots are an og capability core lacks, not a core superset).

## Summary

For files whose extension is in the TxtViewFiles config list (.txt by default; the shipped list has 119 extensions), the og's thumbnail worker renders a content-preview image: the first up-to-100 bytes of the file, sanitized, drawn in Arial 13 onto a 200×200 bitmap, packed as a BMP, and used as the row's thumbnail (with a grade promotion on success — M-112's territory). Core's thumbnail surface refuses text formats by name — text rows render no thumbnail at all.

## What the reference does

The render chain is fully decoded (cites above): read the file, take ≤100 bytes,
sanitize, draw text at Arial 13 on a 200×200 GDI surface, pack a BMP, hand it to
the thumbnail payload slot. The eligibility is the TxtViewFiles config list — the
same key M-108 decoded for the preview panel's eligibility model.

## What our core does

`thumbnail_format_supported` returns false for text extensions; the matrix at
`cr_thumbnail.h:39-42` carries no text family; there is no text-render path.

## Gap

Falsifiable: a .txt fixture — the og's results grid shows a thumbnail image of
its first bytes; `cr_ui_thumbnail_format_supported("txt")` returns false and
`cr_ui_thumbnail_generate` returns the named Unsupported.

## Impact

User-visible results-grid difference for the entire text family (119 shipped
extensions): the og shows content-preview thumbnails, core shows the generic
file-type icon. No recovery-data impact; the grading consequence (the og promotes
text files whose snapshot renders) is M-112's.

## Reproduction / verification

Decompiled evidence (the Lane C decode; the render chain cites re-read by the
lead at filing — the :422447-422455 branch and the Arial-13/min-100 constants at
:652919-:652948 match). Core-side code-read. Full record:
`evidence/s11-lead-deep-lanes.md` §Lane-C (Section A.2 and candidate finding 2).

## Fix plan (for the engineering team)

1. Add a text-snapshot renderer to the thumbnail engine: a new
   `render_text_snapshot(data, len)` that takes the first ≤100 bytes, sanitizes
   (the og's 0x6e-buffer rule — non-printables to a placeholder), and rasterizes
   to the 256-px-or-configured thumbnail size (core's max_edge defaults; the og's
   fixed 200×200 is the config-era shape, not a contract). The font (Arial 13 /
   the nearest metric-compatible default on Linux) rides the engine's existing
   text-render utilities if present, else a minimal stb-truetype-based path.
2. Eligibility: reuse the TxtViewFiles list — the engine already needs it for
   M-108's fix (the same config key); one shared table, two consumers (preview
   eligibility + thumbnail eligibility).
3. ABI: no new call needed — `cr_ui_thumbnail_format_supported` gains the text
   extensions and `cr_ui_thumbnail_generate` routes text nodes to the snapshot
   renderer; the truncation-is-data doctrine applies unchanged.
4. Tests: a .txt fixture renders non-empty; a binary-with-text-extension fixture
   (non-printable head) renders the sanitized placeholder; the 100-byte bound
   pinned; `ThumbnailFormatSupported` gains the text rows.
5. Acceptance: text fixtures produce thumbnails; the M-112 promotion trigger's
   text leg becomes reachable (coordinate the landing with M-112's fix).

## Tests to add

- `Thumbnail.TextSnapshotRenders` / `Thumbnail.TextSnapshotSanitizes` /
  `Thumbnail.TextSnapshotHundredByteBound`.

## Related findings

M-112 (the grade trigger that consumes this render's success); M-108 (the same
config key's preview-panel consumer — share the table); N-137 (the ABI header
truth defect at the same surface); I-028 (the thumbnail superset register — this
is the counter-direction: an og capability core lacks).

## Open questions

The exact shipped TxtViewFiles list's 119 extensions (M-108's evidence has the
count and the decode; the fix should cite its own evidence file rather than
re-decode).

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S76 MINOR-first fix lane, 2026-10-29)
**Verified by:** `Thumbnail.TextSnapshotRenders` (the printable head renders: white 256px canvas, exact deterministic ink = the sum of the rendered glyphs' bitmap bits, read_ok rides the M-115 rule) + `Thumbnail.TextSnapshotHundredByteBound` (a 200-byte fixture of 100 'A' + 100 'B' renders exactly 100×ink('A') — the B's beyond the og's 100-byte cap never render) + `Thumbnail.TextSnapshotControlsAreBlankAndEmptyRefuses` (controls advance blanks with no ink; the all-control head refuses Unsupported per the og's empty-string branch) + `Thumbnail.TextSnapshotUtf16BomDecodes` (LE and BE BOM heads decode, the BOM itself never renders) + `CrUi.TextSnapshotThumbnailForTxtNode` (the full-stack dispatch: a .txt node answers the 256×256 snapshot with source_format "txt" and both canvas classes present). RED pre-fix: compile-errors on the stash-reverted tree (make_text_snapshot/thumbnail_font_* absent). Full suite **1567/1567** including the re-pinned `CrUi.ThumbnailUnsupportedFormat` / `CrUi.ThumbnailSuccessPromotesGradeTo2` / the C-consumer `cr_ui_abi_check` (DOC.TXT's contract changed by design: the text node now succeeds via the snapshot — the promotion test's text leg now proves the og's own +0x28-class promotion).
**Resolution notes:** The og's render chain re-decoded first-hand at the fix, correcting the finding's own characterization: FUN_1403d7ce0 is a PLAIN capped copy into the zeroed 110-byte buffer (no "sanitize" substitution exists — the finding's "non-printables to a placeholder" is corrected; controls render no ink in GDI's control-glyph class, which the core models as a blank advance); the UTF-16 gate is FUN_1403d8840's BOM check (len>4 && FF FE | FE FF) with FUN_1403d7e20's wide conversion; the empty check is FUN_1400a6200 on the draw string. Landed: `make_text_snapshot` (the og's exact decision sequence — 100-byte cap, BOM-aware decode with surrogate pairs, renderable-empty refusal, then the render) with the embedded 6x12 bitmap font (`cr_thumbnail_font.cpp`, generated by `scripts/gen_thumb_font.py` from DejaVu Sans Mono under the Bitstream Vera license family — the Arial-13 rasterization is a Windows font artifact the core cannot embed; the readable-preview capability is the parity surface, and the font's provenance + license note live in the generated header). The render: white max_edge canvas (the og's fixed 200×200 is the config-era shape, not a contract — the finding's own note), LF/CR line breaks, TAB to the 8-column stop, C0/DEL blank advances, non-ASCII '?' (the ANSI/CP_ACP lossy class). Dispatch: the ABI's thumbnail_generate routes TxtViewFiles-eligible extensions (the M-108 table — one shared table, two consumers, exactly the finding's plan) to the snapshot before the image sniff, mirroring the og's :422447-455 branch; `thumbnail_format_supported` gains "txt". Safety hardenings preserved: the M-115 read-gated promotion now fires for successful text snapshots (the og's own +0x28 trigger class — the promotion test re-pinned to prove it), the 20-byte work-item floor and the preview-panel (category-3) exclusion untouched.
