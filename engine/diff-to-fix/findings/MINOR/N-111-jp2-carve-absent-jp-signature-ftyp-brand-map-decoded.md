# MINOR N-111 — the JP2/JPX/JPM (JPEG 2000 family) carve formats are absent: the og probes the "jP " signature box + the 0x0D0A870A payload and maps the ftyp major brand to three ids (jp2 /jpx /jpm) — core has nothing

**Severity:** MINOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1-adjacent, A2-adjacent, B21-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:3544-3559` (JP2Analyser id 0x80 — list-layer, RawConfig "JP2" token); the probe `:21812-21936`: BE u32@+4 == 0x6a502020 ("jP "), BE u32@+8 == **0x0d0a870a** (the signature-box payload), BE u32@+0x10 == 0x66747970 ("ftyp"); the ftyp major brand at +0x14 maps: "jp2 " → 0x80, "jpx " → 0x81, "jpm " → 0x82 (:21886-21909); the signature-box length u32@+0 (0x0000000C) is NOT checked (corrupt-length salvage-friendly); the size machinery is the separate CalculateSize ctor (:21941+, not decoded — recorded follow-up).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — HEIC/HEIF ftyp rows exist; the jP -signature family does not
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred JP2; this session decoded the signature gates and the brand map.

## Summary

JPEG 2000 images (jp2/jpx/jpm) are absent from core while the related HEIC/HEIF ftyp family landed earlier. The og probes the "jP " signature box + its payload constant, requires the ftyp box, and maps the major brand to three distinct format ids. The size walk is undecoded (recorded follow-up), so the fix lands the gates with a bounded-extent stand-in per the batch-1 convention.

## What the reference does

See corpus field: jP /0x0d0a870a/ftyp gates + the brand→id map; salvage-friendly unchecked length.

## What our core does

No JP2-family rows; the ftyp-based HEIC rows do not cover the jP signature family.

## Gap

Falsifiable: a jP -signature box + ftyp with brand "jp2 " carves in the og as id 0x80; core carves nothing.

## Impact

Archival/photo-lab niche; the brand-map decode also documents the og's three-way id split for the carve-format naming.

## Reproduction / verification

Decompile-direct: gates + brand map read this session and quoted above.

## Fix plan (for the engineering team)

1. Add FMT_JP2/FMT_JPX/FMT_JPM rows on a list-layer probe (or an extended ftyp family probe). 2. Implement the three gates + the brand map. 3. Bounded-extent stand-in with the heuristic note (the og's CalculateSize walk is the recorded follow-up).

## Tests to add

- `Carve.Jp2BrandMap` — jp2 /jpx /jpm brand forms stamp their respective ids.
- `Carve.Jp2SignaturePayloadGate` — a wrong +8 constant rejects.
- `Carve.Jp2UncheckedBoxLength` — a corrupt-length signature box still carves (the og's salvage posture).

## Related findings

S-007 (batch-2 parent); S-041 (school 1, landed `b92ebef`) (the other list-layer image family).

## Open questions

The JP2 CalculateSize walk (:21941+) is undecoded — recorded as the follow-up per the batch-1 convention.
