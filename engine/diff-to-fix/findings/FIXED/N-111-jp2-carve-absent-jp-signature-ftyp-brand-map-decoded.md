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

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-111 landing (the session-39 fix lane; SHA recorded in the ledger row and the worklog entry - cite-by-title per the N-135 rule: "fix(carve): N-111 - the JP2 family lands")
**Verified by:** `Carve.Jp2BrandMap` (the three brand forms stamp FMT_JP2/FMT_JPX/FMT_JPM with their extensions and the box-walk extent), `Carve.Jp2SignaturePayloadGate` (a corrupted payload dword rejects), `Carve.Jp2UncheckedBoxLength` (the bogus-length gate posture), `Carve.Jp2UnknownBrandRejected` (the exact-match map semantics - "j2k " rejects), `Carve.Jp2BoxWalkExtent` (the jp2c-consecutive rule), `Carve.Jp2EocScan` (the len==0 unbounded-box FF D9 scan), `Carve.Jp2Jp2cFinalAtSliceEdgeFallsBackBounded` (the slice-edge fallback). All RED pre-fix by construction (the formats did not exist - the finding's own gap proof). Full suite 1040/1040 (998 passed / 42 Win-only skipped / 0 failed, linux-release); clang-format clean; the F8 registry gate + the S-017 self-probe pass with the three new rows (pinned by the existing Carve registry tests).
**Resolution notes:** Re-verified against the corpus FIRST-HAND before the fix (the SOP): the probe gates (:21812-21936: "jP  " @+4 == 0x6a502020, payload @+8 == 0x0d0a870a, ftyp @+0x10 == 0x66747970, the brand map as an exact-match std::map walk - "jp2 " -> 0x80 / "jpx " -> 0x81 / "jpm " -> 0x82, unknown brands leave the walk unmatched and the probe returns 0) and the registration (:3544-3559, id 0x80, the RawConfig "JP2" token at DAT_180066bfc). **The finding's recorded follow-up ("the CalculateSize walk :21941+ not decoded") was executed this session**: FUN_18001c690/c760/c820/ca60 read line-by-line - the walk is the BE u32 box walk with (a) the len==1 u64 extended form at +8, (b) the len==0 (literal or extended) transition into the codestream byte scan from box_start+4 (the FF D9 EOC u16, a 16-consecutive-zero back-off size latching exactly once at run_start, the absolute guard 0x1F3FFFFF), (c) the high-bit-length size-0 defensive reject, and (d) the jp2c-consecutive rule (FUN_18001ca60's expected-end map - all three ids map to "jp2c"; the first non-jp2c-typed box after a codestream box ends the extent). **No stand-in was needed** - the decoded walk landed as the validator's extent machinery. Architecture: FMT_JP2 carries the og's own scan key (the "jP  " box type at +4, the box length at +0 never read - the salvage posture); FMT_JPX/FMT_JPM carry the brand-position keys (+0x14) so every family member resolves its own extension; the brand map rides CarveResult::format_override (the M-093 SVG precedent). One finding correction recorded: the "corrupt-length signature box still carves (with the correct extent)" inference is TRUE at the gate level but FALSE at the extent level - the og's own walk advances BY the bogus length and never terminates (verified in the c820 decode: the jump exits every window bound); the core's single-pass adaptation takes the family's bounded fallback there (documented in the validator comment + the test's comment). The registry rows are Ids-APPENDED (serialized carve_format values stay stable, the M-093/M-096 convention); max_size 0x1F400000 (the EOC guard 0x1F3FFFFF rounded - 500 MB, the MOBI-row family value).
