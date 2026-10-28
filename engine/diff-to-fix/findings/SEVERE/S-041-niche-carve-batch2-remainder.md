# SEVERE S-041 — Niche carve batch-2 remainder: ~20 corpus-documented analyser formats still absent from the core registry

**Severity:** SEVERE
**Module / Surface:** Carve / raw-signature surfaces (RAWSearchDLL analyser roster vs the core's carve registry)
**Parameters affected:** A28 (carving validator logic), B1/B3 (detection correctness / false-positive posture for the absent family), C19 (magic bytes for the absent keys)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:59,:78,:97,:105,:107,:109,:111,:113,:115` (the family rosters naming every absent format); `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c:22633-22643` + `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:17705-17740` (FileCHMAnalyser — decoded this session); `retdec_decompiled.c:5654-5665` + `ghidra:1951-1962` (the key-284 registration = CFH11Analyser, decoded this session); `findings/FIXED/S-007-carve-missing-common-formats.md:250-262` (the batch-2 remainder with causes, pinned by the SCAN-2f session)
**Core references:** audited at core `35adcb9`: absence verified by exhaustive grep over `src/core/carve/` + `include/cr/carve/` (sweep log: `diff-to-fix/evidence/decompiled-quotes/s8-batch2-absence-sweep.md`); the FormatId enum `include/cr/carve/cr_signatures.h:23-173` (90 ids, none for the absent set); `src/core/carve/cr_registry.cpp` (the kTable)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** "expanded on prior finding S-007" — the batch-2 remainder was recorded only inside the FIXED S-007 finding's addendum (:250-262) and the SCAN-ENGINE-DELTA fix queue; per the trail's own N-068/N-069/N-070/S-032 precedent (sub-items split from S-007 addenda into their own files so fix legs can take them independently), the live recall gap now gets its own finding so the board reflects an open divergence instead of a footnote in a closed file.

## Summary

After S-007 niche batch 1 (`e018bf4`, 27 formats), the og's analyser roster still contains roughly twenty formats the core's carve registry does not key or validate at all: CHM, MNY, EMF, ONE, MPEG-TS (MTS/M2TS), 3DS, SWF, JP2, MOBI, LZH, HTML, SVG/XML, FCPEvt, OfficeTagFromXML, CRW, and the standalone-XML text school (VCF, GPX, HIK, OSP, POS, AEP, CHFP). The corpus documents every one of these in the family rosters and, for several, in the decompiled registration sites. This session resolved two of the recorded decode blockers (CHM's GUID compares and the MPEG-TS "numeric key 284" misattribution — see below), removing the stated causes for at least CHM; the remainder keeps its per-format causes. The divergence is a plain recall gap: files of these families that the reference product carves, the merged product does not.

## What the reference does

The og's RAWSearchDLL registers analysers for every format above (census family rosters, quoted with line cites in the evidence file):

- `@RawSpeed` namespace (:107): HTML, New3DS (3DS), One (OneNote), Dbx — plus the already-landed TAR/RAR/GZ/MID/PNG/PSP/RTF/SIT/WPG/MDB.
- `@RawFormat_xjq` namespace (:109): JP2, MOBI (the newest-generation author's set, with HEIC).
- `@Brance` namespace (:111): CRW (Canon RAW), LZH, SVG, OfficeTagFromXML — plus landed DMG/FLV/MXF/OGG/X3F.
- First-generation `File*Analyser` family (:113): FileMNY (MS Money), FileEMF, FileSWF, File3DS, FileONE, FileHTM, FileLZH, FileFCPEVENT, FileCRW — the fallback-generation validators.
- Standalone classes (:115): CVCFAnalyser (vCard), CGPXAnalyser (GPS Exchange), AEPAnalyser, CHFPAnalyser, CHIKAnalyser, COSPAnalyser, CPOSAnalyser.
- `@BruteForce` (:105): CMTS + CM2TS (the MPEG-TS pair — **list-only** frame-sync walkers, no magic-key registration), CFCPEvt (keyed `SQLi` → "SQLite format 3\0" memcmp at `0x180067eb0`, page-size/version gates).
- Keyed rows: CHM `ITSF` 0x46535449 (:97); LZH-family `E4 52 5C 7B` 0x7b5c52e4 (:78 — the census labels it "LZH arc"; the SCAN-3 census re-read attributes E4 52 5C 7B to OneNote and keeps LZH structural "-lh" — the collision is itself a decode question, see Fix plan); SVG/XML `<?xm`/BOM`<`/`<svg` (:59).

**Two decode blockers resolved this session (first-hand, both decompilers + raw-binary disassembly):**

1. **CHM — the "u64-widened GUID compares conflict with canonical ITSF layout" cause is REFUTED.** `FileCHMAnalyser::Analyse` (ghidra `RAWSearchDLL_decompiled.c:17705-17740`; retdec `:22633-22643`; asm ground truth at VA `0x180016b40-0x180016c14` of `og-codebase/RAWSearchDLL.dll`) compares two GUIDs at +0x18 and +0x28 as **genuine 64-bit compares** — and they are the **canonical ITSF v3 pair** {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC} @+0x18 and {7C01FD11-7BAA-11D0-9E0C-00A0C922E6EC} @+0x28 (the same bytes 7-Zip's Chm.cpp hardcodes). The "conflict" was a **retdec constant-printing artifact** — retdec renders the qword constants truncated to their sign-extended low dwords (`0x7c01fd10`, `-0x5ffff362` at :22635); the trail's claimed GUIDs (`799C39D5`, `D0ED6BCE`) appear **nowhere in the binary** (zero matches across the full .text). Full og grammar: `ITSF` dword@+0; dword@+8 == 0x60 (header length); GUID pair u64@+0x18/+0x20 and u64@+0x28/+0x30; dword@+0x60 == 0x1FE (the file-size chunk magic); dword@+0x70 == 0; dword@+0x74 == 0; size = **u64@+0x68 verbatim**; result type id 0x0E; **no min/max bounds** (raw u64 trust — bounded only by the engine's global 0xBEBC200 carve cap).
2. **"MPEG-TS numeric key 284" is a misattribution — key 284 (0x11C) is FreeHand 11.** The registration at `retdec_decompiled.c:5654-5665` / ghidra `:1951-1962` maps magic dword `0x0000011C` (bytes `1C 01 00 00`) → `BruteForce::CFH11Analyser` (format id 0x1D) — Macromedia FreeHand 11, grammar `1C 01 00 00 | 02 00 04 1C | 01 14 00 02 | 00 14 1C 01 | … | "11"@+0x22` (u16@+0x22 == 0x3131), type-only accept (no size written). MPEG-TS itself is **list-only**: CMTS (idx 0x76) and CM2TS (idx 0x74) are constructed and pushed to the brute-force list (`0x1800053ae-0x180005488`) with **no magic-key map entry** — the same list-layer posture as TAR. The census's ":99 (ICO sizes, MTS/ADTS sync)" gloss on the 0x11c row is wrong (corpus erratum E53, filed this session).

## What our implementation does

The core's carve registry (`src/core/carve/cr_registry.cpp`, 90 FormatIds at `include/cr/carve/cr_signatures.h:23-173`) has **no row, no key, and no validator** for any of: CHM/ITSF, MNY, EMF, ONE, MPEG-TS/MTS/M2TS, 3DS, SWF/FWS/CWS, JP2, MOBI/BOOKMOBI, LZH/-lh, HTML, SVG, FCPEvt (beyond the N-089 SQLite subtype relabel note), OfficeTagFromXML, CRW, VCF, GPX, and the rest of the text school. The absence was verified by exhaustive grep this session (patterns + results logged in `evidence/decompiled-quotes/s8-batch2-absence-sweep.md`): zero hits for every family across `src/core/carve/`, `include/cr/carve/`, and a full-tree sanity sweep (`ITSF|BOOKMOBI|jp2c|OfficeTag` → 0). The S-007 batch-1 addendum (:250-262) records the same set with per-format causes: CHM (GUID-compare conflict — **now refuted**), MNY (2-byte MS key, gates undecoded — FP-unsafe without), EMF (no keyed registration found; `01 00 00 00`'s first u16 is 0x0001), ONE (gates undecoded), MPEG-TS (key 284 — **now decoded as FH11; MTS/M2TS are list-only**), 3DS (MM u16 collides with TIFF-BE), SWF/JP2/MOBI (key sites not decoded first-hand), and the list-layer/text school (per-analyser decompile reads required).

## Gap

Falsifiably: for each format F in {CHM, MNY, EMF, ONE, MTS, M2TS, 3DS, SWF, JP2, MOBI, LZH, HTML, SVG/XML, FCPEvt-keyed, OfficeTagFromXML, CRW, VCF, GPX, AEP, CHFP, HIK, OSP, POS}, the og's RAWSearchDLL contains an analyser that accepts files of family F (census-cited; for CHM/FH11/FCPEvt additionally decompile-cited this session), and the core's carve registry contains no entry that accepts any file of family F (grep-verified absence). The reference recovers these families; the merged product does not.

## Impact

User-observable recall gap on real file families: CHM help files, MS Money databases, EMF metafiles, OneNote notebooks, MPEG-TS/AVCHD video (a large real-world family — camcorder .MTS/.M2TS), 3DS models, SWF/Flash, JPEG-2000, MOBI ebooks, LZH archives, HTML/SVG/XML documents, Final Cut Pro event databases, Canon CRW RAW photos, vCard/GPX and the XML-project school. This is the exact class the audit's SEVERE band exists for ("missing feature the reference has"). It also keeps the S-007 closure partial: batch-1's "31 rows / 27 formats" is honest, but the board previously read "0 active" while this remainder lived only in an addendum — this finding restores the truthful board state.

## Reproduction / verification

Absence: exhaustive grep sweep (logged in `evidence/decompiled-quotes/s8-batch2-absence-sweep.md`) — every pattern listed in "What our implementation does" returns zero hits in the core's carve sources; the FormatId enum is read end-to-end (90 ids). Presence in the og: census family rosters at the cited lines (quoted verbatim in the evidence file) + first-hand decompile reads this session for CHM (`ghidra:17705-17740`, `retdec:22633-22643`, asm `0x180016b40`), FH11/284 (`retdec:5654-5665`), and FCPEvt's `SQLi` keying (construct→insert chain re-verified 3-for-3 against semantic anchors EBML→MKV, ITSF→CHM, MRM→MRW). The CHM GUID constants were byte-verified against the raw binary's .text (8 occurrences, all inside FileCHMAnalyser).

## Fix plan (for the engineering team)

1. No architectural change — extend the registry/validator pattern batch-1 established. Order the batch by decode-readiness (corpus-determined):
   - **Batch 2a (decodable now):** **CHM** — grammar fully decoded this session: key `ITSF`; gates dword@+8==0x60, u64 pair {0x11D07BAA7C01FD10, 0xECE622C9A0000C9E} at +0x18/+0x20, u64 pair {0x11D07BAA7C01FD11, 0xECE622C9A0000C9E} at +0x28/+0x30, dword@+0x60==0x1FE, dword@+0x70==0, dword@+0x74==0; size u64@+0x68; no format-specific bounds (the global cap governs). **MTS/M2TS** — list-layer brute-force posture (same seam as TAR): 0x47 packet-sync walk (`47 41 4x` transport packets, 188-byte stride, the og's own walk bound read at `:30331-30338` region per the trail's MPEG-TS cap note), no magic key.
   - **Batch 2b (key sites readable, grammars to extract):** SWF (`FWS`/`CWS`, size u32@+4, ZWS NOT accepted — census :…; list-layer per SCAN-ENGINE-DELTA A4), JP2 (sig box + `jp2c` keys 0x80/0x81/0x82, gated), MOBI (BOOKMOBI, RawConfig-gated), 3DS (u16 `MM` key id 0x01, `==`@+16 chunk gate, max 200MB — the TIFF-BE collision needs the multimap-equivalent treatment the TTF/MDB shared key already models), LZH (structural "-lh", max 200MB), ONE (E4 52 5C 7B id 0x31, 20B..500MB), CRW (II/MM `\x1A\x00` id 0x12), MNY (u16 `MS` id 0x28 — decode the analyser's follow-up gates FIRST, FP-unsafe without).
   - **Batch 2c (text school):** HTML (u16 `<h`/`<H`/`<!`/UTF-16-BOM id 0x21), SVG/XML (u32 `<?xm`/BOM`<`/`<svg`/0x000001cd id 0x5b), VCF/GPX/HIK/OSP/POS/AEP/CHFP + OfficeTagFromXML — per-analyser decompile reads required before each row lands (the zero-invention bar).
2. Files: `include/cr/carve/cr_signatures.h` (append-only FormatIds), `src/core/carve/cr_registry.cpp` (rows with og-exact keys), `src/core/carve/cr_analysers.cpp` (validators), `src/core/carve/cr_carve.cpp` (kTrialOrder entries + kMaxTrialSlots headroom if needed), `tests/unit/test_carve.cpp` (plants/rejects/salvage per batch-1's pattern).
3. Constants: as cited above; CHM's GUIDs as the two u64 pairs (little-endian at +0x18/+0x28).
4. Tests per batch: plant/carve/reject-noise/salvage quadruples, fail-before proven by validator neutralization (batch-1's discipline); the shared-key collision cases (3DS-vs-TIFF-BE on u16 `MM`) get a discriminates-test like TTF-vs-MDB's.
5. Docs: refresh the S-007/S-041 rows in CROSS-REF-MATRIX as each batch lands; note the FH11 key-284 census erratum (E53) and the retdec constant-truncation artifact (E54) in CORPUS-ERRATA (landed this session).
6. Acceptance: for each landed format, a fixture planted per the og grammar carves; a hostile variant rejects; the census roster row is satisfiable by grep against the core registry; no DBF row ever (no dBase analyser exists in the og — session-3 correction stands).

## Tests to add

- `S041ChmItsfGuidPairGrammar` (the two u64 GUID pairs + 0x60/0x1FE gates + size u64@+0x68)
- `S041MtsListLayerPacketSync` (188-byte transport-packet walk, torn-stream salvage at last complete packet)
- `S041Batch2KeyedFormats` (per-format plant/carve/reject quadruples as they land)
- `S041SharedMmKeyDiscriminates` (3DS vs TIFF-BE on the shared u16 `MM` key)
- `S041TextSchoolRejectsNoise` (hostile XML/HTML variants)

## Related findings

- **S-007** (parent finding — batch 1 landed 27 formats; this file owns the remainder)
- **S-040** (the WMF placeable validator contradiction — batch-1 residue in the same validator file; fix together)
- **N-089** (SQLite/FCPEvt — already landed; the FCPEvt keyed registration is context for batch 2c)

## Open questions

The per-analyser gate grammars for batch 2b/2c formats (MNY's follow-up gates, ONE's gates, EMF's chain membership, SWF/JP2/MOBI key sites, the text school's accept predicates) remain to be decoded first-hand before their rows land — recorded as the fix plan's step-1 decode reads, not as unverifiable reference behavior (the analysers' existence is census-verified). The E4 52 5C 7B key's LZH-vs-OneNote census collision (census :78 vs SCAN-3's attribution) needs the registration-site re-read during batch 2b.
