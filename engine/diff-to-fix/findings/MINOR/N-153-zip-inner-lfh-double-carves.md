# MINOR N-153 — the ZIP family double-carves multi-entry archives: every inner local-file-header position emits a second "zip" (LFH → the shared EOCD), the outer extent's containment never applied

**Severity:** MINOR (duplicate results inflating the carved tree — one extra file per inner entry whose tail-to-EOCD span clears the 128-byte floor; no data loss, every extent is a subset of the real archive)
**Module / Surface:** carve / the ZIP family rows (modules/carve.md)
**Parameters affected:** A2-adjacent (candidate dedup), B3-adjacent (the false-positive population), F9-adjacent (the overlap rule's scope — the M-117 no-jump design left same-format inner candidates undeduped)
**Corpus references:** the og's extent-consumption model: the block-scan controller consumes the walked file's sectors (searchdll-family--architecture.md's sector bitmap; the same basis as the S-007 repeating-sync rule and N-151's nested-declaration repair) — an inner LFH inside an emitted archive's extent is never re-probed; the og's S-032 inventory presents the inner ENTRIES as child nodes instead (rawsearchdll--zip-analyser-carving.md :29-38)
**Core references:** discovered during N-115's verification at `7320a2f6`-lineage (the multi-entry test fixture): `src/core/carve/cr_registry.cpp` — FMT_ZIP/FMT_DOCX/FMT_ZIP_CD carry no `suppress_overlap`; the M-117 design note in `cr_carve.cpp` deliberately removed the post-emission scan jump (overlapping candidates of OTHER formats stay carvable — correct), but the same-format inner-LFH candidate has no suppression path: the exact-start F9 dedup misses it (different start), and `find_by_extent` is an exact (offset, class) lookup, not containment. Reproduction (executed, the plain-zip path untouched by N-115): a 2-entry stored zip (200-byte payloads) at 4096 emits TWO nodes — fmt=FMT_ZIP at 4096 (size 594, the real archive) + fmt=FMT_ZIP at 4331 (size 235, the second entry's LFH through the shared EOCD). Small-payload archives escape only because the inner span falls under the 128-byte floor.
**Status:** READ-ONLY FINDING — filed by the session-40 lane B during N-115's verification (the discovered-divergence protocol); fixed in the same window

## Summary

A multi-entry ZIP emits its real archive node AND one phantom "zip" per inner local-file-header whose remaining span to the EOCD clears the min-size floor: the inner LFH is a valid PK\x03\x04 magic position, the gates pass, the EOCD scan finds the shared EOCD, and nothing suppresses a same-format candidate strictly inside an emitted extent. The og never produces the phantom: its controller consumes the walked extent and its inventory presents the inner entries as child nodes.

## What the reference does

Sector-bitmap extent consumption: one archive, one file, its entries as children.

## What our core does (pre-fix)

N+1 files for an N-entry archive (subject to the 128-byte floor per inner span).

## Gap

Falsifiable: the reproduction above — 2 nodes where the og yields 1.

## Impact

Duplicate carved results on every multi-entry archive (the dominant archive shape); inflated counts; no integrity effect.

## Fix plan

`suppress_overlap = true` on the three ZIP-family rows (FMT_ZIP, FMT_DOCX, FMT_ZIP_CD — the CD-anchored headless-salvage row included: a surviving CD inside an emitted archive's extent is already carved as part of it; a headless zip's CD is never inside an emitted extent, so the salvage path is unaffected). Cross-format nesting stays carvable (only the family's own emissions register covered ranges).

## Tests to add

- `Carve.MultiEntryZipCarvesOnce` — the reproduction (RED pre-fix: 2 nodes; GREEN: 1).

## Related findings

N-151 (the same nested-candidate class for the declaration families); M-117 (the no-jump design whose dedup gap this closes); S-032 (the inner-entry inventory the og presents instead).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-115+N-153 landing — recorded post-push in the ledger row)
**Verified by:** `Carve.MultiEntryZipCarvesOnce` (RED pre-fix: the outer node + the inner-LFH phantom; GREEN post-fix: one node at the archive start with the full extent), the whole ZIP suite staying green, and N-115's multi-entry fixtures (which exposed the defect) now asserting single-file trees. Suite green (full counts in the N-115 resolution).
**Resolution notes:** `suppress_overlap = true` on FMT_ZIP, FMT_DOCX, FMT_ZIP_CD — an inner PK\x03\x04 / PK\x01\x02 position strictly inside an emitted archive's extent is the same archive (the og's sector-bitmap consumption never re-probes it; its inventory presents the entries as children — S-032). The headless-zip CD salvage is unaffected (a CD that survives a destroyed head is never inside an emitted extent); cross-format nesting stays carvable (only the family's own emissions register covered ranges — the M-117 no-jump design's intent preserved).
