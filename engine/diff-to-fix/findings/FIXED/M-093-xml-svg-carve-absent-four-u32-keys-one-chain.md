# MAJOR M-093 — the generic-XML (id 0x13) and SVG (id 0x5b) carve formats are absent: the og binds ONE chain on four u32 keys (<?xm / UTF-8-BOM+'<' / <svg / the numeric key 0x1cd) with a 512-byte tag window and </svg> end-tag walk — core has neither format

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1, A2 (two formats absent: generic XML documents and SVG vector graphics), A28-adjacent, B19-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:1806-1837` (the four u32-key registrations all constructing the **SVGAnalyser chain** — `<?xm` 0x6d783f3c, UTF-8 BOM+'<' 0x3cbfbbef, numeric key **0x1cd** (undecoded semantics — see Open questions), `<svg` 0x6776733c); the analyser `:49622-49703` (accept if `memcmp(buf,"<?xml version=\"",15)==0` or at +3 (BOM); then copy the first 512 bytes and strstr for the `<svg` variants → **SVG id 0x5b**; else the generic-XML marker (DAT_180093968 — a .data string not byte-decoded, see Open questions) → **XML id 0x13**); SVGRawFileInfo bounds {0, 0x100000} (:49721-49726); the `</svg>` end-tag string at .data 0x180093950 with the family walker (:49749+).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — no XML id 0x13, no SVG id 0x5b, no <?xm key rows
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred the text school; this session decoded the shared chain, the four keys, and the probe.

## Summary

Generic XML and SVG — two og formats sharing one analyser chain bound to four u32 keys — are absent from core. The og accepts `<?xml version="` declarations (or the UTF-8 BOM form), then looks inside a 512-byte window for `<svg` (→ SVG id 0x5b, sized by a </svg> end-tag walk, 1 MiB cap) or falls to a generic-XML marker (→ XML id 0x13). Configuration files, Office custom XML, RSS feeds, and vector graphics are unrecoverable by content in core.

## What the reference does

The four u32 keys (incl. the numeric 0x1cd whose semantics remain undecoded) all construct the Brance::SVGAnalyser → FileSVGAnalyser chain (:1806-1837). The probe (:49622-49703) requires the XML declaration (offset 0 or +3 for the BOM), then a 512-byte window strstr: an `<svg` substring stamps id 0x5b; otherwise the generic-XML .data marker stamps id 0x13. Bounds {0, 1 MiB}; the size walk is the family end-tag scan for `</svg>`.

## What our core does

No XML/SVG analyser; the DOCX first-entry `[Content_Types].xml` gate is the only XML-adjacent carve byte.

## Gap

Falsifiable: `<?xml version="1.0"?>` + `<svg ...>` + ... + `</svg>` carves in the og as id 0x5b with the end-tag extent; core carves nothing.

## Impact

Recovery correctness for XML configuration/data files and SVG vector assets; both are common in modern disks and web/app payloads.

## Reproduction / verification

Decompile-direct: registration sites and probe read this session and quoted above; the `<svg`/`</svg>` strings cross-check in the binary string tables.

## Fix plan (for the engineering team)

1. Add FMT_XML and FMT_SVG with the four u32-key rows bound to one validator (the shared chain). 2. Implement the declaration gate + the 512-byte `<svg` window → SVG; the generic-XML fallback marker → XML (decode DAT_180093968 from .data first — see Open questions). 3. Size: the </svg> end-tag walk for SVG (1 MiB cap); the generic-XML size rule needs the same .data decode. 4. Wire kTrialOrder slots; register the 0x1cd key with an honest 'undecoded semantics' note if not resolved.

## Tests to add

- `Carve.SvgDeclaredSvgAccepted` — the declaration + <svg…</svg> form carves as SVG with end-tag extent.
- `Carve.SvgBomFormAccepted` — the BOM+'<' key path.
- `Carve.GenericXmlFallbackId` — declaration without svg → XML id (after the DAT decode).
- `Carve.XmlNoSvgTagRejected` — no window hit rejects.

## Related findings

S-007 (batch-2 parent); M-092 (HTML — the same text-school family); N-113 (the smaller text formats).

## Open questions

Two .data strings need a binary hexdump read before landing: DAT_180093968 (the generic-XML acceptance marker, ≤7 printable bytes, rabin2 min-length hides it) and the numeric key 0x1cd's semantics (a 4CC the census never identified). The corpus gap is recorded; the keyed rows can land with the three decoded keys first.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (recorded post-push; `git log --grep=M-093`)
**Verified by:** `Carve.SvgDeclaredSvgAccepted` (the declaration + "<svg " form carves as FMT_SVG with the exact end-tag extent and the .svg extension), `Carve.SvgBomFormAccepted` (the BOM key path — the declaration at +3), `Carve.SvgDirectKeysAccepted` (BOTH direct paths: "<svg " and the decoded numeric key CD 01 00 00), `Carve.GenericXmlFallbackId` (the "csl" marker stamps FMT_XML with the .xml extension — the id 0x13 semantics), `Carve.XmlNoMarkerNoSvgRejected` (an ordinary declaration without svg/csl rejects — the probe's exact acceptance set). Full suite 1081/1081 green (Linux release).
**Resolution notes:** Both open questions were discharged with first-hand binary reads before the fix: **DAT_180093968 = "csl"** (the Corel symbol-library marker — the generic-XML id 0x13 acceptance string; decoded from a PE .data hexdump at VA 0x180093968) and **the numeric key 0x1cd = a raw dword magic CD 01 00 00** (DAT_180093940 — files starting with those four bytes stamp SVG directly via the probe's else-if path). The full needle set was likewise hexdump-decoded from .data: the six svg needles "<svg " / "<svg\n " / "<svg\r " / "<svg> " / "<svg>\n" / "<svg>\r" (0x180093920-0x180093970), the "</svg>" walker string (0x180093950), and the 15-byte declaration "<?xml version=\"" (0x180093930). The probe (FUN_180041670 :49622-49703) is case-SENSITIVE (memcmp/strstr — unlike HTML's _memicmp) and its 512-byte window includes the BOM bytes for the BOM-keyed candidate. The id split (0x5b vs 0x13) is content-decided, not key-decided — carried by a new `CarveResult::format_override` that the emit site resolves against the registry (the effective row supplies carve_format/ext/name/base-confidence; the dispatch row's bounds gate the candidate, matching the og where all four keys share the FileInfo). A second decode discovery landed with this fix: the og's shared end-marker scan (FUN_180054470 :64131-64160) is TWO-VARIANT case-aware (_mbslwr_s pass then _mbsupr_s — it finds all-lower and all-upper forms, never mixed) — the M-092 HTML scanner was refined to these og-exact semantics in the same change (its full case-insensitive scan was a superset). No sector round-up for this family (FUN_180041950's tail sets the size verbatim; the HTML twin FUN_180042e60 is the round-up exception). Recorded residual (the M-106 stand-in discipline): the walker's 0x44c branch — the CSL size formula `dword@0x44c + 0x480` for files spanning offset 0x44c — is decoded only partially and stays unported; markerless files take the bounded 1 MiB cap fallback per the family convention.
