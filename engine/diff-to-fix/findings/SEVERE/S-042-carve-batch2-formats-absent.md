# SEVERE S-042 — Carve batch-2 formats absent: CHM, MNY, EMF, ONE, MPEG-TS, 3DS, SWF, JP2, MOBI, LZH, HTML, SVG/XML, the text school (VCF/GPX/HIK/OSP/POS/AEP/HFP) + the OfficeTagFromXML id-split

**Severity:** SEVERE
**Module / Surface:** carve (the RAW-signature layer — the og's registered analyser set)
**Parameters affected:** B1 (detection correctness — carve set), A28 (validator logic — missing validators), C19 (magic tables), P29 (format identity/ids — the og's per-format ids and the OOXML id split)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` (per-format registrations + gates — cites per row below); `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c` (ctor bounds); `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md` (the roster); `rawsearchdll--family-{filegen,brance,rawspeed,special}.md` (the family contracts)
**Core references:** `src/core/carve/cr_registry.cpp` (the 101-row registry — audited inventory at `evidence/executions/s9-carve-registry-inventory.md`), `src/core/carve/cr_analysers.cpp` — audited commit `35adcb9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** expanded on prior finding S-007 (FIXED) — the S-007 addendum recorded this remainder as "the batch-2 remainder with per-format causes" outside an active finding; session-8's T2 mandated it become a proper finding. This finding owns the remainder with the session-9 decodes (the CHM/MPEG-TS/EMF/LZH puzzles resolved).

## Summary

The og's RAWSearchDLL registers a set of formats that our carve engine does
not implement at all: CHM, MS Money (MNY), EMF, OneNote (ONE), MPEG-TS
(MTS/M2TS), 3DS, SWF, JPEG 2000 (JP2), MOBI, LZH, HTML, SVG/XML, and the
seven ext-gated "text school" formats (VCF, GPX, HIK/DAV, OSP, POS, AEP,
HFP). Additionally the og's OfficeTagFromXML classifier distinguishes
DOCX/XLSX/PPTX inside ZIP carving with three distinct format ids (0x49/0x4A/
0x4B) where we carry a single FMT_DOCX. Every format below is now decoded
at its registration site (keys, gates, bounds), so this finding is directly
executable — no further RE is required except where noted.

## What the reference does

Per-format decode (registration + gate, with cites; verbatim quotes in
`evidence/decompiled-quotes/s9-batch2-registrations.md`):

| Format | Key / dispatch | Id | Gate (decoded) | Bounds | Cite (ghidra DC / retdec RC) |
|---|---|---|---|---|---|
| CHM | u32 `ITSF` 0x46535449 | 0x0E | ITSF + hdr_len 0x60 + GUID1 {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC}@+0x18 + GUID2 {…FD11…}@+0x28 (full 16-byte u64-pair compares — instruction-verified) + HS0 0x1FE@+0x60 + zeros@+0x70/+0x74; extent = file size u64@+0x68 | self-sized | DC:2488-2499, :17703-17740; asm 0x180016b40 |
| MNY | u16 `MS` 0x534D | 0x28 | "MSISAM Database" 15-byte memcmp @+4 | fixed 1 MB | census; gate per B-decode |
| EMF | list-only | 0x1A | `" EMF"`@+0x28 + sReserved@+0x3A==0 + nBytes@+0x30!=0 → size=nBytes. NO key, NO record-type-1 check (addendum superseded) | self-sized | B-decode |
| ONE | u32 0x7B5C52E4 (`E4 52 5C 7B`) | 0x31 | the OneNote GUID {7B5C52E4-D8D8-4DA7-AEB1-5378D02996D3} | 20 B..500 MB | DC:2160-2177 |
| MPEG-TS | list | CMTS 0x76 / CM2TS 0x74 | 0x47 sync @+0 or +4; second sync @+188/+192; PID≠0x1FFF. ("key 284" resolved: that is FreeHand 11, id 0x1D — our FMT_FH11 already correct) | packet walk | DC:3143-3175 |
| 3DS | u16 `MM` 0x4D4D | 0x01 | "MM"@0 + "=="@+16 (chunk grammar) OR "NCSD"@+0x100 + 0x200000@+0x104 (Nintendo 3DS ROM) | max 200 MB | B-decode |
| SWF | list | 0x3E | "FWS"/"CWS" only (ZWS rejected); size=u32@+4; version∈[3,20]; RECT parse | self-sized | DC:3067-3068 |
| JP2 | list, ext-gated "JP2" | 0x80/0x81/0x82 | "jP  " + 0x0D0A870A + "ftyp"; brand {"jp2 ","jpx ","jpm "} selects the id | — | B-decode |
| MOBI | list, ext-gated "MOBI" | 0x85 | BE "BOOK"@+0x3C + "MOBI"@+0x40 (PalmDOC header) | — | B-decode |
| LZH | list/structural | 0x24 | u32@+2 ∈ {-lh0,-lh5,-lh7,-lhd} + '-'@+6. NOT the 0x7B5C52E4 key (that is ONE) | max 200 MB | B-decode |
| HTML | u16 ×4: `<h`/`<H`/`<!`/UTF-16 BOM | 0x21 | memicmp "<HTML"/"<!DOCTYPE HTML" or BOM+'<'+L"<HTML>"/'!' | 1 MB cap | DC:2844-2879, gate :14329-14376 |
| SVG/XML | u32 ×4: `<?xm`/BOM`<`/`<svg`/0x1CD | 0x5B | "<?xml version\"" + "<svg" strstr ≤2 KiB; extent via "\n</svg>" tail scan | tail-anchored | DC:1806-1837, gate :63924-64024 |
| VCF | list, ext-gated | 0x88 | "BEGIN:VCARD" | — | DC gate :45329-45334 |
| GPX | list, ext-gated | 0x97 | "<?xml version\"" (+0 or BOM+3) + "<gpx" scan after +0x16 | — | DC gate :20876-20891 |
| HIK (Hikvision DAV) | list, ext-gated | 0x92 | MPEG-PS pack BA (+ optional system hdr BC) + private stream BD; size from BE u16 PES len | PES-derived | DC gate :30429-30456 |
| OSP | list, ext-gated | 0x8F | JSON `{\r\n "id": ` | — | DC gate :50123-50126 |
| POS | list, ext-gated | 0x8B | JSON `{"diagram":` | — | DC gate :20216-20221 |
| AEP | list, ext-gated | 0x9C | "RIFX" u32 | — | DC gate :11568-11570 |
| HFP | list, ext-gated | 0x90 | "<?xml version\"" + "<BiffProject"@+0x28 | — | DC gate :19960-19968 |
| OfficeTagFromXML | inside ZIP carve | 0x49/0x4A/0x4B | ZIP entry names "word/_rels/document.xml.rels" → 0x49; "xl/_rels/workbook.xml.rels" → 0x4A; "ppt/_rels/presentation.xml.rels" → 0x4B (singleton, 0x2800 buffer) | — | DC:53155-53195, ctor :53202-53224 |

## What our implementation does

The registry (`cr_registry.cpp`) carries 101 FMT_* ids. Mechanical inventory
(this session, `evidence/executions/s9-carve-registry-inventory.md`): no row
exists for any format in the table above except the OOXML half-row —
`FMT_DOCX` exists with a single id, gated by `zip_common(..., office=true)`
requiring the first local entry name to be exactly `[Content_Types].xml`
(`cr_analysers.cpp:846-856`). XLSX and PPTX files (which also carry
`[Content_Types].xml` as the first entry) are carved under the single
"docx"/"Office Open XML" identity — the og's three-way split (and its
rels-name classification for damaged archives) is not replicated.

## Gap

For every format F in the table: the og's carve acceptance set includes F's
real files; ours is empty for F (no key, no list dispatch, no validator).
For OfficeTagFromXML: the og reports three distinct format ids for the three
OOXML flavors; we report one. Both claims are mechanically verifiable via
the inventory evidence.

## Impact

- **User-observable recall:** every scan silently omits this format set —
  including mainstream types (CHM help files, EMF vector images, SWF,
  MOBI e-books, MPEG-TS camcorder footage (MTS/M2TS), JPEG 2000, HTML/SVG,
  vCards, GPX tracks, Hikvision DAV footage). This is the S-007 class of
  gap, continued: S-007 batch-1 fixed 27 formats; this is the recorded
  remainder now fully decoded.
- **Format identity:** recovered OOXML documents are mislabeled docx when
  they are xlsx/pptx (file-extension naming downstream will be wrong for
  two of three flavors).
- **Ext-gated formats** (JP2/MOBI/text school) additionally require the
  list-layer extension dispatch the og implements — see the fix plan.

## Reproduction / verification

1. Corpus: the per-format registration/gate cites above; the load-bearing
   registrations (CHM, ONE, SWF, MPEG-TS, CRW-family, OfficeTagFromXML)
   re-read first-hand by the lead this session; CHM additionally verified
   at the instruction level (capstone on `og-codebase/RAWSearchDLL.dll`).
2. Product: the 101-row inventory + per-magic greps (all zero) in
   `evidence/executions/s9-carve-registry-inventory.md`.
3. The prior session's deferral causes are now resolved: CHM (instruction-
   level compare-width read — see `s9-chm-gate-asm.md`), MPEG-TS (key 284 =
   FH11, not MTS), EMF (no keyed registration exists — list-only, the
   addendum's "record-type-1" phrasing superseded), LZH/ONE key attribution
   (0x7B5C52E4 = ONE), CRW/MRW (S-041).

## Fix plan (for the engineering team)

1. No architectural change for the keyed/list formats; one small mechanism
   for the ext-gated set (the og dispatches those from a list layer keyed
   by the carve candidate's *extension context* — implement as a
   post-scan classification pass over carved files whose format id is
   unknown-generic, or as registry rows with an ext-match precondition,
   matching the existing FMT_DOCX pattern's spirit).
2. Files to modify: `src/core/carve/cr_registry.cpp` (new rows),
   `src/core/carve/cr_analysers.cpp` (new validators), the FMT_* enum
   (append-only), `tests/unit/test_carve.cpp`, `docs/FORMATS.md`.
3. Per-format implementation order (risk-weighted, common formats first):
   a. **MPEG-TS** (CMTS/CM2TS — camcorder footage; two ids 0x76/0x74 for
      the 188/192-byte packet variants; reuse the MPEGPS/MPEGES frame-walk
      infrastructure);
   b. **HTML + SVG/XML** (u16/u32 keys; 1 MB cap for HTML; the SVG
      "\n</svg>" tail scan for extent);
   c. **SWF** (list; FWS/CWS only; size=u32@+4; version 3..20; RECT parse
      for a minimal gate);
   d. **EMF** (list; the " EMF"@+0x28 signature field + nBytes extent);
   e. **CHM** (u32 ITSF; the two full GUID compares at +0x18/+0x28; HS0
      0x1FE@+0x60; extent = u64@+0x68 — self-sizing, the strongest gate);
   f. **JP2, MOBI, LZH** (JP2/MOBI ext-gated; LZH structural u32@+2);
   g. **ONE, MNY, 3DS** (u32/u16 keyed; ONE's GUID; MNY's MSISAM memcmp
      — resolve the +4 prologue question first, see Open questions);
   h. **the text school** (VCF/GPX/HIK/OSP/POS/AEP/HFP — seven small
      ext-gated validators, each a single signature check per the table);
   i. **OfficeTagFromXML split** (inside zip_common's office branch and the
      CD-anchored salvage path: classify by rels entry names → three ids;
      keep `[Content_Types].xml` as the primary gate, add the rels-name
      classification as the damaged-archive fallback exactly as the og).
4. Trials/registry mechanics: extend kTrialOrder for every new keyed row
   (the static_assert(kTrialCount == FMT_COUNT) guard added in e018bf4
   enforces this); list-layer formats need the list dispatch hook.
5. Tests: one plant per format (both key variants where the og registers
   two), hostile-noise variants per the e018bf4 discipline, salvage tests
   for the self-sizing formats (CHM torn-header bound, SWF truncated RECT),
   and the OOXML three-way split test (a docx/xlsx/pptx plant trio carving
   to three distinct ids).
6. Docs: FORMATS.md rows; the S-007 FIXED addendum's remainder list flips
   to landed as each batch ships.

## Tests to add

- Per format: `S042<Fmt>Carves`, `S042<Fmt>RejectsNoise` (following the
  S-007 batch-1 test names).
- `S042ChmGuidGateFull` — both GUIDs corrupted by one bit each → rejected;
  canonical ITSF plant → carved with extent from +0x68.
- `S042MpegTsBothPacketPeriods` — 188 and 192-byte variants, both ids.
- `S042SwfZwsRejected` — a ZWS (LZMA-compressed) SWF is NOT carved.
- `S042OfficeOoxmlSplit` — docx/xlsx/pptx plants → three distinct format
  ids via the rels names; a damaged-zip xlsx (head destroyed, CD anchored)
  still classified 0x4A.
- `S042TextSchoolSignatures` — one plant per text format.
- Salvage: `S042ChmTornHeaderBound`, `S042LzhTruncatedMember`.

## Related findings

- S-007 (FIXED) — the parent batch; this is its recorded remainder.
- S-041 — CRW, the other member of the recorded remainder, split out
  because its keys are occupied by dead rows (a different fix shape).
- N-092 — cap drift on already-landed batch-1 formats.

## Open questions

- MNY: the og's key sits at +0 with the "MSISAM Database" signature at +4
  — confirm the 4-byte prologue semantics (MS Money v14+ header carries a
  4-byte "MSISAM" version/sentinel prefix) before pinning the gate. One
  first-hand read of the MNY analyser's memcmp offsets settles it; do not
  guess (the u16 `MS` key registration itself is verified).
- SVG: the numeric u32 key 0x1CD's semantics (the 4th registration in
  DC:1806-1837) — undecoded; non-blocking (the three string keys carry the
  format; implement those and track 0x1CD as a follow-up decode).
- The og's FileInfo ctor bounds for the ext-gated text school are not
  individually decoded (each analyser is a single signature check); the
  implementing session should mirror the DWG-pattern bounded extent with
  the low-confidence note, per the batch-1 convention, unless a ctor read
  pins real bounds.
