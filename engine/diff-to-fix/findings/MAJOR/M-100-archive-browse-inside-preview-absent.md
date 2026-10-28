# MAJOR M-100 — archive browse-inside preview is absent: the og enumerates damaged-zip contents for an in-place archive browse (ArchivePreviewModel, full MVC over the entries) — core's preview reads file bytes only, no archive-content enumeration

**Severity:** MAJOR
**Module / Surface:** preview (modules/preview.md)
**Parameters affected:** A42 (public surface shape), A2-adjacent (browse-inside changes what the user can verify before recovering)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/10-preview-subsystem.md:17` (the archive browse model); `UIUX/20-moc-surface-sink-architecture.md:170-174` (ArchivePreviewModel — a QAbstractItemModel with index/row/column/parent/child/rowCount/columnCount/data invokables over the damaged zip's contents).
**Core references:** audited commit `ff879f9` — `src/core/preview/` + `include/cr/preview/` — the preview reads ranges of the target file bytes (cr_ui_preview_read, +4 MiB/call); grep for archive-entry enumeration: **absent** (R-37: "none — preview reads file bytes only … would need an archive-listing preview mode", docs/REFERENCE-API-GAP.md gap row). S-032 landed the carve-time inner-entry INVENTORY — the preview-side browse is a different surface (listing an arbitrary archive on demand, damaged or not).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** recorded as R-37 GAP; never formalized as a finding. S-032 (FIXED) is the carve-time inventory — related but distinct.

## Summary

The og's preview subsystem can open an archive (including a damaged one) and browse its entry tree in place — the user verifies the recoverable contents BEFORE recovering (ArchivePreviewModel, a full item-model surface). Core's preview is byte-range only. With S-032's inner-entry inventory already landed for carves, the natural fix is exposing the same enumeration through the preview surface for arbitrary archive targets.

## What the reference does

ArchivePreviewModel (UIUX/20:170-174) exposes the zip's entry tree as an item model with the full invokable set — the browse-inside flow is part of the preview subsystem (UIUX/10:17).

## What our core does

Preview = byte ranges (cr_ui_preview_read). No archive-listing mode; the only entry inventory is the carve-time S-032 seam.

## Gap

Falsifiable: the og previews a zip's entry list in place; core's preview API has no operation returning entries for an archive path.

## Impact

User-facing verification flow (browse before recover — a conversion-relevant trust step in the og's funnel); leverages an already-landed parser (S-032), so the marginal cost is the surface, not the parsing.

## Reproduction / verification

Corpus documentation evidence (the two cites); core state grep-verified this session.

## Fix plan (for the engineering team)

1. Add an archive-listing preview mode: cr_ui_archive_list(path/handle → entries: name, size, compressed size, offset, CRC) over the S-032 entry-inventory parser (zip first; RAR/7z as the parser set allows). 2. Two-call size pattern per the house ABI convention; FROZEN-JSON for the CLI side. 3. Damaged-archive semantics: entries after the first broken header are still listed when the parser salvages them (the og's damaged-zip posture).

## Tests to add

- `Preview.ArchiveListZip` — a healthy zip lists all entries with correct fields.
- `Preview.ArchiveListDamagedSalvages` — a mid-archive corruption still lists the leading entries (the og's posture).
- `Preview.ArchiveListNonArchiveFailsClean` — a non-archive path returns the right error.

## Related findings

S-032 (the landed inventory parser this reuses); N-115 (the OOXML typing that feeds the same parser); API-DELTA §5 (preview family).

## Open questions

The og's ArchivePreviewModel is zip-scoped in the corpus decode — whether RAR/7z also browse is a UIUX decode question; the fix lands zip first (the decoded surface) and records the rest.
