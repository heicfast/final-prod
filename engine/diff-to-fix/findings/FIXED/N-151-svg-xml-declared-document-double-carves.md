# MINOR N-151 — the XML/SVG family double-carves XML-declared documents: the declaration-start candidate (the `<?xm` row, override→SVG) AND the interior direct-`<svg` candidate (the `<svg` row) both emit — one document, two files

**Severity:** MINOR (duplicate results inflating the carved tree; no data loss — both extents are subsets of the same document)
**Module / Surface:** carve / the M-093 XML-SVG chain + the N-113 text-school rows' nested-candidate shapes (modules/carve.md)
**Parameters affected:** A2-adjacent (candidate dedup), F9-adjacent (the overlap rule's scope)
**Corpus references:** the og's sector-aligned probe model (RAWSearchDLL's list-layer probes fire on sector-aligned buffers; the walked file's extent is consumed — an interior re-match of the same document never re-fires: searchdll-family--architecture.md's block-scan model, the same basis as the S-007 repeating-sync suppress_overlap rule the carve engine already carries)
**Core references:** discovered during this lane's N-113 verification at pristine main `538d529b`: `src/core/carve/cr_registry.cpp` — the four M-093 rows (FMT_XML/FMT_XML_BOM/FMT_SVG/FMT_SVG_CD01) carried no `suppress_overlap`, and the parallel N-113 landing's VCF/GPX rows likewise; `src/core/carve/cr_carve.cpp`'s exact-start F9 dedup does not cover a nested same-format candidate at a different start. Reproductions (all executed RED on the parallel lane's landed tree `c0f69250`): (1) `<?xml version="1.0"?>\n<svg xmlns="x"></svg>` at 4096 → TWO nodes (fmt=SVG at 4096 size 43 + fmt=SVG at 4118 size 21); (2) a two-card VCF (adjacent BEGIN:VCARD…END:VCARD\r\n pairs) → the outer extent covers both cards (the parallel landing's own test) AND an inner second file at the second card; (3) a BOM'd GPX → the BOM-row file AND an interior declaration-row file.
**Status:** FIXED (this session, the session-40 lane B — discovered during N-113's verification per the discovered-divergence protocol, fixed over the parallel N-113 landing)

## Summary

The M-093 landing and the parallel N-113 landing registered their rows without the repeating-candidate suppression the nested-declaration family needs: an XML-declared SVG matches BOTH the `<?xm` key (at the declaration — the validator's window finds the `<svg` needle and overrides to SVG) AND the `<svg` key (at the interior element start); a multi-card VCF re-matches BEGIN:VCARD inside the walked extent; a BOM'd GPX re-matches the declaration inside the BOM stream. All three shapes emit two files for one document — the exact-start F9 dedup cannot see a nested candidate at a different start.

## What the reference does

The og's probes fire on sector-aligned buffers and the walked extent is consumed by the scan controller — an interior re-match of the same document never produces a second file (the same basis as the S-007 repeating-sync rule the engine already implements for the MPEG-audio/TS families).

## What our core does (pre-fix)

Both candidates emit in all three shapes (the reproductions above; the parallel N-113 landing's tests passed because they asserted the outer extent, not the tree count).

## Gap

Falsifiable: plant any of the three reproductions and count the carved nodes — pre-fix two, the og one.

## Impact

Duplicate results (user-visible duplication in the carved tree, inflated counts); no data-integrity effect (both extents are subsets of the same document).

## Reproduction / verification

Executed on the parallel lane's landed tree `c0f69250` (all three RED pre-fix; GREEN post-fix — see the tests).

## Fix plan

`suppress_overlap = true` on the four M-093 rows + the parallel landing's FMT_VCF/FMT_GPX/FMT_GPX_BOM rows (the interior `<?xm`/`<svg`/BEGIN:VCARD re-match is the same document — the repeating-sync family rule applied to the declaration family).

## Tests to add

- `Carve.SvgDeclaredDocumentCarvesOnce` — the SVG reproduction (RED on pristine main).
- `Carve.TextSchoolVcfInnerCardIsSameFile` — the multi-card count pin (RED on the parallel N-113 landing).
- `Carve.GpxBomFormCarvesOnce` — the BOM-form count pin (RED on the parallel N-113 landing).

## Related findings

M-093 (the landing whose rows lacked the flag); N-113 (the text school — the parallel landing's rows needed the same rule; this lane's verification supplied it); S-007 (the suppress_overlap mechanism's origin).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this lane's N-151 rider over the parallel N-113 landing — recorded post-push in the ledger row)
**Verified by:** the three tests above, all RED on the parallel lane's landed tree `c0f69250` and GREEN after the flags (plus the parallel landing's own nine TextSchool tests and the M-093 SVG suite staying green). Suite 1082/1082 (1040 pass / 42 env skips / 0 fail, linux-release).
**Resolution notes:** `suppress_overlap = true` on the four M-093 rows (FMT_XML, FMT_XML_BOM, FMT_SVG_CD01, FMT_SVG) and the three N-113 rows (FMT_VCF, FMT_GPX, FMT_GPX_BOM) — the interior re-match of an emitted declaration stream is the same document, the repeating-sync family rule (S-007's mechanism) applied to the nested-declaration shapes. Discovered during this lane's N-113 verification: this lane's own N-113 implementation hit the push window against the parallel lane's equivalent landing (b6e0a9ef — their landing stands per the earlier-push-wins rule); the verification of THEIR landing exposed the three double-carve shapes their tests missed (their multi-card test asserted the outer extent, not the count). No behavior change for non-nested documents (side-by-side files are unaffected — the suppression only fires strictly inside an emitted extent).
