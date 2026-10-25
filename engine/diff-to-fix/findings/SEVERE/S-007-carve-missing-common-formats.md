# SEVERE S-007 — Carve coverage gap: GIF, BMP, headerless MP3, AAC ADTS, RTF, OLE legacy Office, MPEG video, DWG (+ ~10 niche) not carved

**Severity:** SEVERE
**Module:** carve
**Parameters affected:** B1 (detection correctness — carve set), B3 (false positives, both directions), A28 (carving validator logic — missing validators), C19 (magic tables)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md` §4 (106 analyser classes: BruteForce 24, RawSpeed 20, xjq 8, Brance 22, legacy File*Analyser 46); `raw-dual-registry--u16-magic-keys.md` §2–3 (70 u32 keys + 39 u16 keys, selected keys quoted: GIF8 0x38464947, BM 0x4D42, {\rt 0x74725c7b, OLE 0xe011cfd0, MPEG audio syncs 0xFFF2–0xFFFF, AAC ADTS 0xFFF1, MThd, MAC, ITSF, .snd, dBase, AC10/DWG); `og-codebase/raw_signs.json` (139 extensions — **does not contain** gif/bmp/rtf/doc/xls/ppt/mp3/aac/mid/dwg; parsed this session)
**Core references:** `src/core/carve/cr_registry.cpp` kTable (31 grammar signatures — grep: no GIF/BMP/RTF/OLE/ADTS/MPEG-video/DWG entries), `src/core/carve/cr_analysers.cpp:214` (MP3 = 'I','D','3' magic only), `src/core/carve/cr_rawsigns_data.cpp` (139 formats, mirrors og raw_signs) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's carve surface is a dual magic registry (70 u32 + 39 u16 first-byte keys) backed by
106 analyser classes with two-generation fallback chains. Our core has 31 grammar
validators + the shared raw_signs layer. Executed set-comparison (og registry keys ∪
raw_signs vs core grammar ∪ raw_signs) shows the following **user-common formats are not
carved by the core at all**: GIF, BMP, headerless MP3 (frame-sync streams without ID3),
AAC (ADTS), RTF, OLE compound documents (.doc/.xls/.ppt — legacy Office), MPEG video
(.mpg/.m1v), and AutoCAD DWG; plus ~10 niche formats (CHM, AU, MIDI, APE, DBF, EPS, DXF,
JP2, MOBI, DMG). `raw_signs.json` does not cover them (its 139 extensions are a disjoint,
mostly-exotic set — verified by parsing).

## What the reference does

The og hard-codes the common formats in the dual registry (quoted keys with census
citations): GIF8, BM (u16), {\rt, OLE D0CF11E0 (all legacy Office + MSI), MPEG audio sync
family (12 u16 codes 0xFFF2–0xFFFF → BruteForce frame walkers that walk headerless MP3
streams), AAC ADTS 0xFFF1, MThd, ITSF, .snd, MAC, dBase, AC10. The BruteForce school
(24 classes) does frame-sync scanning that maximizes salvage on headerless streams; the
two-generation chain (modern analyser → legacy File*Analyser) means a candidate is
discarded only when both validators fail.

## What our core does

`cr_registry.cpp` kTable covers: JPEG PNG TIFF CR2 DNG WebP ZIP DOCX 7z SQLite MP4 MOV M4A
HEIC AVI WAV MKV RAR4 RAR5 PST ORF RAF RW2 PSD MP3 ASF FLV CAB ISO ZIP_CD. MP3 requires an
ID3v2 header (`cr_registry.cpp:214` magic `{'I','D','3'}`). There is no GIF, BMP, RTF,
OLE, ADTS, MPEG-video, or DWG entry, and the raw-sign layer cannot supply them (absent
from raw_signs.json — the og has the same 139-extension set for that layer, so the
gap is exactly the hardcoded-set difference).

## Gap

Falsifiable claim: a disk containing only `photo.gif`, `img.bmp`, `song.mp3` (no ID3),
`note.rtf`, `letter.doc` (OLE), `clip.mpg`, `drawing.dwg` in unallocated space yields
carve results for all of them in the og and **zero** of them in the core.

## Impact

- Directly user-visible recovery-yield loss on the most common file categories after
  JPEG/Office-XML: GIF and BMP are ubiquitous in older data; headerless MP3 is the norm
  for carved fragments; .doc/.xls/.ppt dominate pre-2007 Office data.
- Bench recall on mixed-format fixtures diverges mechanically.
- DWG matters for the CAD user base (a named EaseUS vertical).

## Reproduction / verification

- Executed set-comparison (strength 1, this session): parsed `og-codebase/raw_signs.json`
  (139 extensions listed in evidence) and grepped the core registry — the missing set is
  the computed difference; no GIF/BMP/RTF/OLE/ADTS/MPEG/DWG magic exists anywhere in
  `cr_registry.cpp`/`cr_analysers.cpp`/`cr_rawsigns_data.cpp`.
- Corpus-derived: census + dual-registry artifacts (keys quoted with citations).

## Fix plan

1. Add grammar entries + validators, in priority order (each is a small, bounded
   validator — reuse existing helpers):
   - **GIF** — magic GIF87a/GIF89a, walk Logical Screen Descriptor + blocks to trailer
     0x3B (skip sub-image data via LZW-min codes), max 64 MiB.
   - **BMP** — "BM", DIB header size ∈ {12..124}, bfOffBits sanity, dimensions from
     BITMAPINFOHEADER, extent = offBits + computed image size (uncompressed), floor 512.
   - **MP3 headerless** — new registry entry keyed on the frame-sync byte pair
     (0xFF 0xFB/0xF3/0xF2/0xE3... family): run the existing `validate_mp3` frame walker
     from the sync position with a ≥10-frame validity gate (vs 2 for ID3-headed) to
     control false positives.
   - **AAC ADTS** — 0xFFF1 sync + profile/sample-index/channel tables, frame walk
     (ADTS fixed header), ≥10 frames.
   - **RTF** — `{\rtf` + walk groups counting braces to the matching close (cap 16 MiB).
   - **OLE (doc/xls/ppt/msi sibling)** — D0CF11E0 + minor-version/sector-size fields,
     walk the FAT sector chain from the directory entry, extent = last referenced sector
     × sector size (bounded); classify by directory CLSID prefix tables (Word/Excel/
     PowerPoint) for the report.
   - **MPEG video** — 0x000001BA (PS pack) / 0x000001B3 (video ES): packet walk to the
     first invalid start code (BruteForce-style, cap 2 GiB).
   - **DWG** — "AC10xx" version string, walk the section-locator records (r13+ format) —
     or a bounded heuristic extent (header-decoded section map when parseable, else
     min(file-slab, 64 MiB) with a MetadataOnly-style honesty cap).
2. Add the matching `FMT_*` ids, kTrialOrder placements (BMP/RTF/OLE early — distinct
   magics; MPEG after ZIP_CD), and registry F8 validation rows.
3. Tests: one validator test per format (valid + truncated + corrupt-magic reject) +
   a carve-pipeline test per format on a synthetic slice.
4. Bench: extend the seed corpus's type matrix (bench/seed/generate_seed.ps1 22-type set)
   with gif/bmp/rtf/doc/mpg/mp3-no-id3 and re-measure recall.

## Tests to add

(fix plan step 3 — all fail before, pass after)

## Related findings

- S-006 (lost-partition probes — same users affected at the volume level).
- N-008 (ZIP padding semantics — same module).

## Open questions

The og's dispatch order (u16 → u32 → raw_signs) is corpus-INFERRED (OQ-T8); the coverage
gap here does not depend on it.
