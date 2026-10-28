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
