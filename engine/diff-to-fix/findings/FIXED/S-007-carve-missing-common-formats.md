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

## Session-2 update (2026-10-25) — signature payload completed, both directions, dispatch resolved

The full decompile read of RAWSearchDLL (main-auditor leg; quotes in
`../evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`) upgrades this
finding's evidence:

**All 8 session-1 formats confirmed present in the og registry and absent from the core**
(GIF `47 49 46 38` u32 0x38464947 `:2009`; BMP `42 4D` u16 `:2898`; headerless MP3
MPEG-sync family `FF F2..FF FF`/`FF E2..FF E7` `:2565-2721` + 2-consecutive-frame grammar
`:37152-92`; AAC ADTS `FF F1`/`FF F9` `:2761/2775`; RTF `7B 5C 72 74` u32 0x74725c7b
`:2087`; OLE `D0 CF 11 E0` `:2341` + Word6 `DB A5 2D 00`/`94 A6 2E 00`; MPEG `00 00 01 B3`/
`00 00 01 BA` `:1843/1851`; DWG `41 43 31 30` "AC10" u32 0x30314341 `:1997`).

**~26 additional og-only formats session 1 missed, all decompile-verified:** CHM `ITSF`;
AU `.snd`; MIDI `MThd`; APE `MAC `; dBase `C5 D0 D3 C6`; EPS `%!`; DXF `"  0\r"` 0xd302020;
DMG `BZh1`; SWF; TAR; LZH `E4 52 2C 7B`; WPG; PSP `Pain`; **3DS `MM`** (`:2885-88` — the
census artifact's "MM = TIFF-BE" is an erratum, see I-011); MRW `II\x1A`/`MM\x1A`/`\0MRM`;
X3F `FOVb`; HLP `3F 5F 03 00`; ICO/CUR `00 01 00 00`/`00 02 00 00`; WMF/EMF `01 00 09 00`
+ `D7 CD C6 9A`; TTF; WPD; MNY `MS`; MDB `00 01`; MTS/M2TS; 3GP; AIF/AIFF `FORM`; AEP; GPX;
VCF; HIK/POS/OSP/HFP; FCPEVENT; RealMedia `.RMF`; OGG `OggS`; MXF `06 0E 2B 34`;
HTML/XML/SVG `<h`/`<H`/`<!`/UTF-16 `FF FE`/`<?xm`/`<svg`; gzip `1F 8B`; StuffIt `Stuf`;
CAB lowercase `mscf`; Dbx `JMF6`/`FE 12 AD CF`/`F7 89 34 2C`. **One core-only superset:**
ISO9660 `CD001`@0x8001 — the og cannot carve ISO images at all (informational; register).

**Dispatch resolved (OQ-T8):** sector-aligned probing only (headers starting mid-sector are
invisible to the og — the core byte-steps and is a detection superset, registered I-009);
3-layer first-accept-wins (u32 RB-tree → u16 multimap → list sweep), per-key
modern→legacy analyser chains, no cross-layer overlap resolution; **carve is
allocation-bitmap-gated when a bitmap exists** (`:9446-60` — decompile-proof for M-005).
ZIP validator semantics corrected in N-027d (padding inversion + two-tier version gates).
Fix-plan ordering note: implement the missing formats with the og's exact bytes above.

---

## Session-3 CORRECTION addendum (2026-10-26 — two rows of the addendum above are wrong; decompile re-verified first-hand)

The session-3 S3-CARVE registry decode (`evidence/decompiled-quotes/s3-rawsearch-inventory.md`
§3–§4, §8) — re-verified first-hand by the session-3 lead at the cited lines — corrects
two rows in the addendum above (the "~26 additional og-only formats" list):

1. **"dBase `C5 D0 D3 C6`" is WRONG.** That key is
   `BruteForce::CEPSAnalyser` (format id **0x1b**), registered in the u32 map at
   `RAWSearchDLL_decompiled.c:1975-1987` (`local_res10[0] = 0xc6d3d0c5` immediately
   after the CEPSAnalyser vftable store) — Adobe Illustrator **AI/EPS**, not dBase.
   **No dBase/DBF analyser exists anywhere in the decoded registry** (all 35 u16
   registrations + 33 u16 keys + the u32 map + the list layer were enumerated).
   Consequence for this finding's fix plan: **drop the DBF validator from the
   og-parity list**; add EPS/AI (magic `C5 D0 D3 C6` → CEPS grammar) if format
   parity is pursued beyond S-007's original families.
2. **"LZH `E4 52 2C 7B`" is WRONG twice.** The key at that registration site is
   `0x7b5c52e4` = bytes **E4 52 5C 7B** (not 2C), and it maps to
   `RawSpeed::OneAnalyser` + `FileONEAnalyser` (format id **0x31**) — the OneNote
   `{E4525C7B-…}` GUID prefix (`RAWSearchDLL_decompiled.c:2156-2177`, vftable stores
   at :2162/:17144). **LZH lives in the LIST layer** as `Brance::LZHAnalyser`
   (vftable at :3097, class bodies at :46896+) — a structural/text analyser, not a
   4CC-keyed one. Fix-plan consequence: the OneNote magic is `E4 52 5C 7B`; LZH
   must be implemented as a list-layer structural validator (or via raw-signs), not
   as a signature key.

The addendum's other rows (3DS `MM` vs TIFF-BE, MRW `\0MRM`, HLP `3F 5F 03 00`,
PST `!BDN`, DBX keys, OneNote) are consistent with the session-3 decode where both
legs overlapped; the six census-row mis-attributions are cataloged as corpus errata
(session-3 rows in `CORPUS-ERRATA.md`).

## Resolution

**Status:** FIXED (the primary common-format set + the full MPEG-audio sync family; the ~25 niche og-only formats are the precisely-scoped remainder below)
**Fixed in commit:** (this commit — see git log)
**Verified by:** `Carve.S007CommonFormatsCarve` (the finding's falsifiable claim end-to-end: gif/bmp/headerless-mp3/rtf/OLE/mpg/dwg + aac all carve on a synthetic slice), `Carve.S007ValidatorsRejectNoise` (bad-DIB BMP, bad-version GIF, 3-frame MP3, 4-frame ADTS, bad-shift OLE, unknown DWG version, lone pack header all reject), `Carve.S007TruncatedSalvageSemantics` (truncated GIF salvages past its images; unclosed-group RTF salvages to the slab end) — 3/3 FAIL on the implementation-reverted build (enum kept for compilation; git-stash proof). Suite 436/436 Linux ASan. Hostile ASan smoke (scripts/hostile_smoke_s007.cpp): 300 mutated multi-format fixtures through carve_slice — zero sanitizer findings.
**Resolution notes:** Implemented with the og's exact corpus keys (re-read first-hand at the cited registrations): GIF8 (:2009), BM (:2898), the headerless MPEG-audio u16 family FF E2..E7 / FF F2..F7 / FF FA..FF (18 keys, :2565-2721), AAC ADTS FF F1 / FF F9 (:2761/:2775 — the 0xf9ff key decompiler-rendered as decimal 63999), {\rt (:2087), OLE D0CF11E0 (:2341), MPEG 00 00 01 BA / 00 00 01 B3 (:1843/:1851), DWG AC10 (:1997). 27 new registry entries (ids appended; serialized carve_format values stable) with 9 new validators (validate_gif/bmp/mp3_raw/aac_adts/rtf/ole/mpegps/mpeges/dwg — reimplemented from the format grammar, og behavior as the model). Design decisions, each corpus- or finding-grounded: the headerless audio gate is 10 consecutive frames (the finding's fix plan: the og's 2-frame gate at :37152-92 is sector-aligned-probe-calibrated; the core's byte-granular scan needs the stricter gate at the finer grid — registered in the D-57 note); the audio family entries carry `suppress_overlap` (a mid-stream re-sync of an emitted stream is the SAME stream — the og's walker runs once per stream start; the core's byte-granular scan would otherwise emit overlapping per-frame candidates — pinned by the exactly-one-node assertions); OLE walks the DIFAT head + directory chain with the CFB (N+1)*sector_size offset rule (sector N's data at (N+1)*ssz — the header occupies the first sector slot) and extends to the last referenced sector; DWG is version-gated over the known AC10xx release set with the finding's sanctioned bounded-extent heuristic (64 MiB cap, raw-sign-class confidence, honest note) — the R13-R2000 section-locator walk and the R2004+ compressed section map are the recorded follow-up; OLE surfaces as ".doc" with the CLSID classification (Word/Excel/PowerPoint) left to the report layer (single-ext-per-signature shape; scoped simplification). DBF is NOT implemented (the session-3 correction: no dBase analyser exists in the og registry — CEPS at that key is AI/EPS); OneNote E4 52 5C 7B and list-layer LZH are in the niche remainder, not the primary set. The niche-format tail (~25 og-only formats: CHM/AU/MIDI/APE/EPS-AI/DXF/DMG/SWG/TAR/LZH-list-layer/WPG/PSP/3DS/MRW/X3F/HLP/ICO/CUR/WMF/EMF/TTF/WPD/MNY/MDB/MTS/3GP/AIFF/OGG/MXF/HTML-XML/gzip/RMF/Dbx) remains OPEN with the session-2/3-corrected magic table in this file as the implementation list — each is a bounded validator of the same class; next step: batch them per og analyser school (BruteForce vs RawSpeed vs list-layer) with the same test pattern. The bench seed-matrix extension (fix plan step 4) needs the Windows comparative-bench runner — recorded as the bench leg's follow-up.
---

## Addendum (session 6 — the RAWSearchDLL family layer finally read; roster + dispatch sharpened, 2026-10-26)

The six carving-family artifacts (`rawsearchdll--family-{brance,bruteforce,filegen,rawspeed,special,xjq}.md`) and the analyser-registry census/dual-registry decode were **never cited by any prior session** — this session read them end-to-end (context sheet: scratch, not committed; quotes preserved in `evidence/decompiled-quotes/s6-carve-family-layer.md`). They sharpen this finding's roster with first-hand citations:

- **Bare MPEG audio is unreachable in the core**: the og's dual registry dispatches 18+ MPEG frame-sync u16 keys (`raw-dual-registry--u16-magic-keys.md:38-41` — 0xFFF2..0xFFFF ×12, 0xFFE2..0xFFE7 ×6, 0xFFF1 ADTS, 0xF9FF) into the BruteForce `CMP3` frame-sync scanner; the core's MP3 anchor is `ID3`-only (`cr_registry.cpp:197`) AND `validate_mp3` itself rejects non-ID3 headers (`cr_analysers.cpp:1247-1248`) — bare-stream MP3s (no ID3 tag — a large fraction of real-world files) are a complete recall gap. This is the single highest-value addition to this finding's roster.
- **The ~45-format family census** (BruteForce/RawSpeed/Brance/FileGen/Special/xjq rosters, `rawsearchdll--analyser-registry-census.md:45-117`) — GIF, BMP, SWF, TTF, WMF/EMF, WPD, OLE, MDB, TAR, GZ, LZH, HTML/SVG, RM, MPG, MXF, OGG, ADTS, APE, DWG/DXF, EPS, ICO, CHM, HLP, AU/AIF, VCF, GPX, MOBI, JP2, CRW, X3F, DMG, ONE, dBase, … — extends the roster this finding already registered, now with per-family provenance instead of aggregate claims.
- **Case-variant keys**: og registers `MSCF` AND `mscf` (census:94,127) → split to **N-069** (own finding, single-format scope).
- **ZIP anchor set + LFH sanity gates**: EOCD-first zips + version_needed/extra gates → split to **N-068**.
- **Overlap scoring + two-generation fallback chain** (census:43, searchdll-family--architecture:73) → split to **N-070**.
- **Archive inner-file inventories** (zip-carve:38,64) → split to **S-032** (MAJOR — capability, not just coverage).
- Refinements that are MATCH/superset, no finding: RAR4 fast-accept on signature alone (core stricter — full block walk); PST "MS"-magic + AMap gates (core superset); RAR5 fully implemented in core vs og-rejects (core superset); raw_signs tail-scope per-format + 8 MiB bound (already FIXED via S-015 with og range documented).

The core-side grep for every census format against `cr_registry.cpp` + `cr_rawsigns_data.cpp` is in the evidence file. This finding remains SEVERE and active; the addendum's new sub-items carry their own finding files so the fix legs can take them independently.

### Post-resolution correction (session 6, after the fix landed)

This addendum was authored against the pre-fix tree in the same window the parallel
fix leg landed `587a98a` ("S-007 primary set"). Re-verified at the merged tree
(`5b0bc7b`-lineage): **the bare-MPEG headline sub-item is FIXED** — the registry now
carries the full headerless MPEG-audio sync family (FMT_MPGA_E2..E7 / F2..FF rows +
ADTS, each sharing the 10-frame BruteForce walker `validate_mp3_raw`,
`cr_registry.cpp:365-500`). The addendum's REMAINING live sub-items after the fix:
the **~25 niche og-only formats** (the Resolution's own precisely-scoped remainder —
the family census is the work list), **N-069** (CAB `mscf` case key — re-verified
absent at the merged tree), **N-068** (ZIP EOCD anchor + LFH sanity gates — re-verified
absent), **N-070** (overlap scoring + two-generation fallback — mechanism-level, not
format coverage), and **S-032** (archive inventories). The addendum's roster value
stands: it converts the aggregate claim into per-family provenance for the remainder
work.
