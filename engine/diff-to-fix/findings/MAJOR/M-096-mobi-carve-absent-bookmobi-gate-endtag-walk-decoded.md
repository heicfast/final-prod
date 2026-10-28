# MAJOR M-096 — the MOBI (Kindle/PalmDB) carve format is absent: the og probes the 8-byte "BOOKMOBI" pair at +0x3C (nothing before it is validated — full-salvage posture) and sizes by a 512-step end-tag scan capped at 500 MB — core has nothing

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1, A2 (Kindle e-books never carved), A28-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:3600-3615` (MOBIAnalyser id 0x85 — list-layer, gated by the RawConfig "MOBI" token); the probe `:29369-29431` (BE u32 built from +0x3C..+0x3F == "BOOK" and +0x40..+0x43 == "MOBI" — the canonical PalmDB type/creator pair; bytes 0-0x3B are NOT validated; extension "MOBI" registered :29412-29424); the size walk `:29436-29485` (MOBIRawFileInfo ctor building a CalculateSize_EndTag walker: 4-byte end-tag value 0x0E98E0D0A, scan step 0x200, **cap 0x1F400000 = 524,288,000 (500 MB)**).
**Core references:** audited commit `ff879f9` — `src/core/carve/` — no row, validator, or raw-sign entry for this format (kTable cr_registry.cpp read in full this session; targeted greps: zero hits) — the rawsigns "pdb" entry is the MSVC program-database format (mirrors og raw_signs.json:209-214), not PalmDB
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred MOBI ("key sites not decoded first-hand"); this session decoded the probe and the end-tag walk.

## Summary

Kindle MOBI e-books are absent from core. The og accepts any stream whose PalmDB header has the BOOKMOBI type/creator pair at +0x3C — a deliberately weak, full-salvage gate (fragmented or header-damaged books still carve) — and sizes the result by a 512-byte-step end-tag scan (constant 0x0E98E0D0A) capped at 500 MB. The weak grammar must be ported as-is for parity (the og's own FP posture) — the finding's fix plan says so explicitly.

## What the reference does

MOBIAnalyser (id 0x85) is list-registered behind the RawConfig MOBI token (:3600-3615). The probe (:29369-29431) checks only the 8-byte BOOKMOBI pair at +0x3C/+0x40 — the PalmDB type/creator fields; 0x3C bytes of preceding header are unexamined (fragment salvage). Size: the EndTag walker with the 0x0E98E0D0A needle stepping 0x200, cap 500 MB.

## What our core does

No MOBI analyser; the only 'pdb'-adjacent carve entry is the MSVC program-database raw sign (a different format entirely).

## Gap

Falsifiable: a stream with BOOKMOBI at +0x3C carves in the og (id 0x85, extension mobi, end-tag extent); core carves nothing.

## Impact

Recovery of Kindle e-book libraries (.mobi/.azw share the BOOKMOBI creator); the weak front gate also recovers books whose PalmDB header start is overwritten.

## Reproduction / verification

Decompile-direct: probe + walk read this session and quoted above.

## Fix plan (for the engineering team)

1. Add FMT_MOBI ("mobi", display "Kindle e-book") as a list-layer probe. 2. Implement the 8-byte BOOKMOBI gate at +0x3C EXACTLY (do not strengthen — the og's salvage posture is the parity target; a stronger gate would diverge). 3. Size = the 0x200-step end-tag scan for 0x0E98E0D0A, cap 500 MB; fallback bounded extent with the heuristic note. 4. Wire the RawConfig-equivalent CarveFlags toggle.

## Tests to add

- `Carve.MobiBookMobiPairAccepted` — BOOKMOBI at +0x3C carves.
- `Carve.MobiFragmentedHeaderStillCarves` — 0x3C garbage bytes before the pair still carve (salvage parity).
- `Carve.MobiOtherPalmCreatorsRejected` — TEXt/REAd (eReader) etc. reject (the og gates BOOKMOBI only).
- `Carve.MobiEndTagExtent` — the 0x0E98E0D0A needle bounds the size.

## Related findings

S-007 (batch-2 parent); S-041 (school 1, landed `b92ebef`) (SWF — the other list-layer book/media format).

## Open questions

The exact byte pattern of the 0x0E98E0D0A end tag (the EndTag comparator's byte order) needs one read of the walker body (:29436-29485 partially decoded) — the constant is decoded, its serialization is not; not blocking the gate landing.
