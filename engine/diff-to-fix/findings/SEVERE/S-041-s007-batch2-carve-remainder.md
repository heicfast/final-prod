# SEVERE S-041 — The S-007 batch-2 carve remainder: ~24 corpus-documented analyser formats absent (a live recall gap recorded outside an active finding)

**Severity:** SEVERE
**Module:** carve
**Parameters affected:** B1 (detection correctness), B3 (false-positive rate — several absences are FP-gated by undecoded og follow-up logic), A28 (carving validator logic), C19 (magic bytes), Q65 (test coverage of the above)
**Corpus references:** the roster artifacts — `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:97-115` (per-family rosters: ITSF→CHM :97; BruteForce incl. M2TS/CMTS/CFCPEvt :105; RawSpeed incl. HTML :107; xjq = JP2/MOBI :109; Brance = CRW/LZH/SVG/OfficeTagFromXML :111; File* = FileSWF/FileEMF/FileCHM/FileMNY/File3DS/FileHTM/FileLZH/FileONE/FileCRW/FileFCPEVENT :113; standalone AEP/CHFP/CHIK/COSP/CPOS/CGPX/CVCF :115); the S-007 batch-2 addendum `diff-to-fix/findings/FIXED/S-007-carve-missing-common-formats.md:250-262` (per-format causes); `diff-to-fix/SCAN-ENGINE-DELTA.md:162-189` (the corrected census table with og-exact keys and ids)
**Core references:** audited `35adcb9`. Absence verified first-hand this session: `rg -c "FMT_CHM|FMT_MNY|FMT_EMF|FMT_ONE|FMT_MTS|FMT_M2TS|FMT_3DS|FMT_SWF|FMT_JP2|FMT_MOBI|FMT_HTML|FMT_XML|FMT_SVG|FMT_LZH|FMT_FCP|FMT_VCF|FMT_GPX|FMT_HIK|FMT_OSP|FMT_POS|FMT_AEP|FMT_CHFP"` over `src/core/carve/cr_registry.cpp` + `include/cr/carve/cr_signatures.h` → **0 hits for every name**; the raw-sign layer `cr_rawsigns_data.cpp` and the og's own `og-codebase/raw_signs.json` contain none of the extensions (both grep-verified) — this is analyser/grammar work, not raw-sign work.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

S-007 (FIXED) landed its batch-1 (27 formats, `e018bf4`) and pinned the batch-2 remainder's per-format causes in an addendum — but the remainder itself has lived since as "a live recall gap recorded outside an active finding" (worklog.md:822, the session-8 T1 baseline's own scope determination). The batch-2 set is ~24 corpus-documented analyser formats (CHM MNY EMF ONE MPEG-TS 3DS SWF JP2 MOBI + the list-layer/text school HTML/XML/SVG/LZH/FCPEvt/OfficeTagFromXML/CRW/VCF/GPX/HIK/OSP/POS/AEP/CHFP). None of them carves in the core today; the og carves all of them. Per the session mandate ("mark all remaining features, implementations, functions as findings too"), this finding gives the gap a tracked owner.

## What the reference does

The og's RAWSearchDLL registers ~106 analyser classes across six families with a dual registry (u32 fourcc RB-tree + u16-key multimap + a list layer). The batch-2 formats with corpus-exact keys (SCAN-ENGINE-DELTA:162-189, census re-verified at the registration sites):

- **CHM** — `ITSF` (u32 0x46535449, registry id 0x0e), full GUID grammar (the :22635 region), size = u64@+0x68; FileCHMAnalyser (:97, :113).
- **MNY** — u16 key `MS` (id 0x28); the analyser's follow-up gates are undecoded (FP-unsafe to key alone — batch-1's zero-invention bar).
- **EMF** — record-type-1 header (id 0x1a); FileEMFAnalyser (:113); no keyed registration decoded (the `01 00 00 00` first-u16 = 0x0001 is not a registered key).
- **ONE** — `E4 52 5C 7B` (id 0x31, 20B..500MB); FileONEAnalyser (:113).
- **MPEG-TS (MTS/M2TS)** — list-layer, numeric key 284 (semantics undecoded; note the s3 census ties 0x11C/284 to the FH11 family — the key claim needs the re-read); CM2TS/CMTS BruteForce classes (:105).
- **3DS** — u16 `MM` (id 0x01, `==`@+16 chunk gate, max 200MB; NOT TIFF-BE — the collision census row was the erratum).
- **SWF** — list-layer `FWS`/`CWS` (id 0x3e, size=u32@+4, ZWS NOT accepted); FileSWFAnalyser (:113).
- **JP2** — sig box + `jp2c` keys 0x80/0x81/0x82 (id 0x80, RawConfig-gated); xjq family (:109).
- **MOBI** — `BOOKMOBI` (id 0x85, RawConfig-gated); xjq family (:109).
- **List-layer/text school** — HTML (u16 `<h`/`<H`/`<!`/UTF-16-BOM, id 0x21), SVG/XML (u32 `<?xm`/BOM`<`/`<svg`/0x000001cd, id 0x5b), LZH ("-lh" structural, id 0x24, max 200MB), FCPEvt (XML front for the SQLite analyser, id 0x73), OfficeTagFromXML, CRW (II/MM`\x1A\x00` id 0x12 — see M-081: the keys exist in core but mislabeled/misgated), VCF/GPX/HIK/OSP/POS/AEP/CHFP (standalone analysers, :115).

## What our core does

The core's registry carries 100 `FMT_` rows (batch-1 complete and key-verified — the session-8 spot-verification of 12 representative batch-1 formats found all magics byte-exact). None of the batch-2 formats has a row, a validator, or a trial-order member (the `static_assert(kTrialCount == FMT_COUNT)` gate would catch a half-landing). The raw-sign layer cannot close this: neither our compiled-in table nor the og's own raw_signs.json carries these extensions — they are analyser-grammar formats in the og too.

## Gap

A disk whose unallocated space contains only `help.chm`, `money.mny`, `pic.emf`, `note.one`, `clip.mts`, `model.3ds`, `banner.swf`, `photo.jp2`, `book.mobi`, `page.html`, `data.xml`, `icon.svg`, `arc.lzh`, `proj.fcpevent`, `shot.crw`, `v.vcf`, `t.gpx`, `cam.hik`, `x.osp`, `p.pos`, `a.aep`, or `c.chfp` carves every one of them in the og and none of them in the core (grep-verified absence above; the CRW half is M-081's misgate, listed here because its recall is equally lost).

## Impact

User-observable recall loss on common file types (HTML/SWF/EMF/CHM/MTS are mainstream; the niche school is the tail). The core's capability row on the carve surface rests on the census, and this residue was known-but-unowned — the exact "recorded outside an active finding" state the session-8 mandate closes. No wrong results are produced (nothing is falsely carved — the absences are pure misses, not misvalidations).

## Reproduction / verification

- Corpus-derived: the roster artifacts (:97-115) + the census corrections in SCAN-ENGINE-DELTA:162-189 (keys re-read at the RAWSearchDLL registration sites by the prior session; batch-1's 27 magics independently re-verified byte-exact this session — see the session-8 verification record in worklog).
- Core absence: the per-format grep loop over `cr_registry.cpp`/`cr_signatures.h`/`cr_rawsigns_data.cpp` (0 hits each) — executed this session, results in the finding's Core references.
- The og-side raw_signs.json cross-check (0 hits) proves the gap cannot be closed at the raw-sign layer on either side.

## Fix plan (for the engineering team)

1. **Precondition per format (the zero-invention bar the S-007 addendum itself pins):** each landing must re-read its og registration/analyser site first-hand — CHM (asm-level u64 GUID compare-width at RAWSearchDLL :22635); MNY (the MS-key follow-up gates); EMF (find the registration; census :113); ONE (analyser gates); MPEG-TS (resolve the 284/0x11C key claim against the FH11 family); 3DS (MM + `==`@+16 chunk gate); SWF (FWS/CWS, size=u32@+4, ZWS rejected); JP2 (sig box + jp2c keys, RawConfig-gated); MOBI (BOOKMOBI, RawConfig-gated); the list-layer/text school (per-analyser decompile reads).
2. Land per analyser school (BruteForce → RawSpeed/xjq → Brance → list-layer), each format = `FMT_*` enum append (`cr_signatures.h`) + kTable row with og-exact key bytes (`cr_registry.cpp`) + validator (`cr_analysers.cpp`) + `kTrialOrder` membership (the shipped `static_assert` gate enforces it).
3. Fix M-081 first (the CRW re-attribution unblocks two of the keys this finding's recall depends on).
4. RawConfig-gated formats (JP2/MOBI) ride the M-024 `disabled_formats` mechanism; the og's default-off set applies (STL/VPJ precedent).
5. Acceptance criteria: per-format carve test (valid plant + truncated salvage + corrupt-magic reject) in the `S007NicheBatch1Carve`/`RejectsNoise`/`SalvageSemantics` pattern; hostile ASan smoke extension over the new validators; no FP regression on the noise fixture set (21 hostile variants re-run).

## Tests to add

- One `NicheBatch2Carve.<FMT>` test per landed format (plant/carve/reject triplet).
- `RejectsNoise` extension: the new keys' collision surfaces (MM-vs-TIFF-BE multimap treatment for 3DS; the HTML u16 `<` keys vs arbitrary text).
- `SalvageSemantics` for the frame-walk formats (torn MTS at the last complete unit).
- RawConfig gate tests for JP2/MOBI (disabled-by-default posture).

## Related findings

- S-007 (FIXED — batch-1; this finding is its pinned remainder, now owned).
- M-081 (the CRW misattribution — overlapping keys, different defect).
- N-093 (batch-1's undocumented bounds deviations — the same registration-bounds discipline this plan step 1 enforces).
- M-024 (FIXED — the RawConfig toggle surface the gated formats ride).

## Open questions

The MPEG-TS numeric-key-284 claim conflicts with the s3 census's FH11 tie (0x11C) — resolve at the decompile before implementing (the fix plan's step 1 covers it). The og's per-format size-walk decodes remain the recorded follow-ups for WPG-class formats (the batch-1 addendum's DWG-pattern note).

## Session-8 wave-2 addendum (the parallel continuation lane — the full grammar decode, cross-verified)

This lane's independent decode of every batch-2 format at its registration
and validator site **confirms the roster above and completes the decode**
(the fix plan's step-1 preconditions are now discharged — every grammar is
first-hand). Full per-format table with citations:
`../../evidence/decompiled-quotes/s8-batch2-formats.md`. What this addendum
adds beyond the body and the SCAN-4 log:

1. **The seven text-school grammars the SCAN-4 list did not enumerate**:
   VCF (config-gated ext "vcf", `_memicmp "BEGIN:VCARD"` 11 bytes, tail
   walk for "END:VCARD" in sector<<7 windows, :45316-99); GPX (gated,
   `<?xml version="` probe then a u32 `<gpx` scan from +0x16, :20860-90);
   HIK (gated, **not text** — MPEG-PS: pack header 00 00 01 BA, system
   header/private-stream gates, <4 MB, :30429-52); OSP (gated, the JSON
   probe `{\r\n "di": ` as u64+u16 literals, :50104-25); POS (gated,
   `_memicmp "{\"diagram\":"`, :20203-17); AEP (gated, 'RIFX', chunk-tree
   walk, :11550-69); CHFP (gated, `<?xml version="` + `<BiffProject` at
   fixed +0x28, :19939-66).
2. **The config-gated registration zone, enumerated**: the list layer's
   gated zone (:3356-4110) registers an analyser only when
   `FUN_180010f50(config, ext)` finds the extension enabled in
   RawConfig.ini — **18 gated sites** (JP2 MOBI VCF GPX HIK OSP POS AEP
   CHFP M1V HEIC among them). The core has no config-gated registration
   surface at all (build-time registry only) — the fix plan's step 4
   (M-024 `disabled_formats`) is the right ride; note the og's default
   posture per extension is undecoded for most of the 18 (the RawConfig
   artifact's 34 toggles name the surface, not per-analyser defaults).
3. **Registry geometry, corrected for all future work**: u32 block
   entries :1728-2509 (**59 bindings**), u16 block :2536-2911 (**35**,
   including the Unicode-BOM battery that routes UTF-16 text), list
   layer :2919-4214 (unconditional ~2930-3350 + gated 3356-4110);
   total **94 keyed bindings** (erratum E55). The batch-1 landing's
   cited regions ":5450-6600"/":6205-6590" are INI/streambuf code
   (N-099).
4. **The u16 layer's BOM battery** (0xfeff, 0xe2ff-0xe7ff, 0xf1ff-0xf7ff,
   0xfaff-0xfdff, 0xffff, 0x100 — every Unicode BOM maps to text
   analysers) — the HTML UTF-16 variant rides it; a core port must
   decide whether the BOM keys are in scope (the og's text dispatch is
   encoding-aware from byte 0).
5. **FCPEvt**, decode-complete: u32 key `SQLi` (:1972) →
   BruteForce::CFCPEvtAnalyser id 0x73; the deep walk's
   "tableactiveactive" memcmp (:35461) relabels subtype 0x75; cap
   200,000,000 (:35507). The core's N-089 relabel covers the SQLite
   detection; the standalone 0x73/0x75 split remains open per the body.
6. **Census-corroborating corrections** (this lane's errata E56-E60):
   `E4 52 5C 7B` = ONE not "LZH arc" (E56); II/MM+0x1A = the CRW chain,
   corroborating **M-081** (E57); New3DS = Autodesk 3D Studio, not the
   console (E58); the four XML keys route through one SVG analyser whose
   probes discriminate SVG (0x5b) from GPX-class XML (0x13) (E59); LZH
   probes u32@+2 + byte@+6 (no u8 '-' key) (E60).

The two batch-1 defects this lane verified at the registration sites that
the body does not carry: the WMF-placeable grammar contradiction
(**S-042** — the batch-1 checksum claim is refuted) and the missing
"Standard ACE DB" ACCDB branch (**M-084**).

**Severity:** unchanged (SEVERE). **Status:** READ-ONLY — no fix applied.

---

## Addendum (session-8 third lane — absence-sweep evidence donation + convergent CHM/FH11 asm verification, merge of the duplicate S-041 filing)

A parallel session-8 lane independently swept the batch-2 absence (patterns →
0 hits logged in `evidence/decompiled-quotes/s8-batch2-absence-sweep.md`,
core `35adcb9`: CHM/ITSF, MNY, EMF, ONE, MPEG-TS/284/0x11C-as-MTS, 3DS,
SWF/FWS/CWS, JP2/jp2c, MOBI/BOOKMOBI, LZH/-lh, HTML, SVG, FCPEvt-keyed,
OfficeTagFromXML, CRW, VCF, GPX — plus a full-tree sanity sweep
`ITSF|BOOKMOBI|jp2c|OfficeTag` → 0) and independently resolved the two decode
blockers the finding's fix plan carried:

- **CHM**: the ":22635 u64-widened GUID conflict" is a **retdec constant-print
  artifact** — the analyser (`FUN_180016b40`, ghidra `:17705-17740`, asm
  `0x180016b40-0x180016c14` of `og-codebase/RAWSearchDLL.dll`) compares the
  canonical ITSF v3 GUID pair {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC} @+0x18 and
  {7C01FD11-…E6EC} @+0x28 as genuine 64-bit compares (retdec prints the qword
  constants truncated to sign-extended low dwords at :22635). Gates: dword@+8
  == 0x60, u64 pairs at +0x18/+0x20 and +0x28/+0x30, dword@+0x60 == 0x1FE,
  dword@+0x70 == 0, dword@+0x74 == 0; size = u64@+0x68 verbatim; NO bounds.
  The trail's claimed GUIDs (799C39D5 / D0ED6BCE) have zero occurrences in the
  binary. (Convergent with the SCAN-4 lane's `:17705-17740` read.)
- **Key 284 / 0x11C**: the registration site (`retdec :5654-5665`, ghidra
  `:1951-1962`, asm `0x1800035a1-0x1800035ea`) maps the magic dword `1C 01 00
  00` → `BruteForce::CFH11Analyser` (id 0x1D, FreeHand 11, u16@+0x22 ==
  "11") — NOT MPEG-TS; CMTS/CM2TS are list-only (no magic-key entry), the TAR
  posture. The census `:99` "(ICO sizes, MTS/ADTS sync)" gloss on the 0x11c
  row is a misattribution (corpus erratum E-E pending below; note the SCAN-4
  lane's E53/E54 cover different rows — !BDN→FilePST and 0x4449→AIFF).

The duplicate filing `S-041-niche-carve-batch2-remainder.md` (this lane, filed
in the same window after renumbering past the deep-audit lane's S-040) is
removed by this merge; its absence-sweep evidence file remains and is
referenced above. **Status after the merge: this file is the single owner of
the batch-2 remainder divergence.**

## SCAN-4 progress record (2026-10-28 — decode precondition satisfied; school 1 landed)

The A2 context-recovery re-run (SCAN-4) decoded the ENTIRE batch-2 set at
validator level — the session-5 grammar table in SCAN-ENGINE-DELTA.md is the
per-format decode record, so this finding's fix-plan step 1 precondition (the
zero-invention bar) is now SATISFIED for every listed format. FCPEvt CLOSES
as already-covered (FMT_SQLITE + the N-089 subtype relabel; NO XML side
exists in the DLL). M-081 (the CRW half) is RESOLVED (`9e5dedc`).

**School 1 LANDED (`b92ebef`):** CHM ONE MNY EMF SWF(FWS+CWS) 3DS(MM + the
NCSD list-layer twin) — 8 registry rows, every grammar re-verified first-hand
at the analyser body; the 3DS NCSD constant 0x4453434e extracted from the og
DLL's .data (VA 0x180093978); ONE/3DS carry the bounded-extent stand-in for
their undecoded walks. Tests: `S007NicheBatch2School1Carve` (8 plants,
og-exact size semantics) + `S007NicheBatch2School1RejectsNoise` (7 hostile
variants); suite 1023/1023 green.

**Remaining (school 2/3, grammars already decoded in the session-5 table):**
HTML (u16 ×4) · SVG/XML (u32 ×4) · LZH · MTS/M2TS · MOBI · JP2 · the text
school (VCF/GPX/HIK/OSP/POS/AEP/CHFP) · OfficeTagFromXML (the ZIP-entry
classifier — the archive-lane classification surface, decoded, to file with
S-032's consumer). **Update (fix session): MTS/M2TS is DONE — S-047 landed
the two sync scanners (FMT_TS/FMT_M2TS); see findings/FIXED/S-047.** The one-line batch-2 causes that this finding inherited
from the S-007 addendum are re-pinned there (CHM :22635 invalid; MPEG-TS
0x11C stale = FH11).


---

## Session-8 lead-lane reconciliation addendum #2 (2026-10-28, the third push-window merge —
IDs past the third lane's M-086..M-089/N-107/N-108, and the school-1 landing reconciled)

The lead lane's independent decode wave (five read legs + first-hand lead verification)
converged with SCAN-4 on every overlapped format and now contributes the **school-2/3
per-format fix units** with full grammars (children of this rollup, in the merged numbering):

- **S-047** MPEG-TS/M2TS — the list-layer sync scanners (0x47 sync; 188/192-byte
  auto-detect by sync position + the {0x40,0x41,'P',0x10} byte set; per-packet verify;
  null-packet tolerance; sync-loss run commit; **min extent 0x5000**; cap x10; the M2TS
  extension stamp) — :3143-3149/:3173-3179/:35578-35677/:38457-38536.
- **M-092** HTML — the FOUR u16 keys (`<h`/`<H`/`<!`/the FF FE UTF-16LE-BOM chain missing
  from the dual-registry table), the case-insensitive ASCII probes + the UTF-16 wide
  probe, the `</html>` end-tag walk (1 MiB cap) with sector round-up.
- **M-093** XML + SVG — the four u32 keys routing into the one analyser (per the remote
  erratum E59), the 512-byte `<svg` window, the generic-XML fallback marker
  (DAT_180093968 — one .data hexdump needed before landing), the `</svg>` family walk.
- **M-096** MOBI — BOOKMOBI at +0x3C with nothing before it validated (the og's own
  full-salvage posture — port the weak gate as-is), the 0x0E98E0D0A end-tag walk
  (0x200 step, 500 MB cap).
- **M-098** HIK — the Hikvision MPEG-PS pack -> private_stream_1 discriminator (the
  pack-length computation (buf[0xd]&7)+0xE, the BB->BD length gates, the direct-BD form),
  the structural walk to the 1 GiB cap. CCTV footage — a flagship scenario.
- **M-099** AEP — bare RIFX (the og's weak grammar, port as-is) + the BE-chunk length +
  the `<?xpacket end` XMP extension, ~30 MiB cap.
- **N-111** JP2 — the jP signature + the 0x0D0A870A payload + the ftyp brand map
  (jp2 /jpx /jpm); cross-ref the school-1 table's "jp2c as the walk box".
- **N-112** LZH — list-only (no u32 key; the E4 52 5C 7B key binds ONE per the remote
  erratum E56); the -lh0/-lh5/-lhd/-lh7 method field at +2 + the trailing '-', the
  local-header entry-chain walk (200 MiB cap).
- **N-113** the text school — VCF (BEGIN:VCARD probe, END:VCARD+CRLF walk, **multi-card
  continuation**, 20 MiB cap), GPX (declaration + the <gpx window up to 0x200 iterations,
  </gpx> walk), OSP (the `{
 "di": ` 10-byte prefix + the "libopenshot": needle —
  **reject-on-missing**, the family's only one, 2 MiB cap), POS (`{"diagram":` probe),
  CHFP (declaration + `<BiffProject`@+0x28, `</BiffProject>` walk with NUL trim, 100 MiB cap).
- **N-115** OfficeTagFromXML — the og's ZIP-entry classifier (per-entry decompress <=10 KiB;
  the rels-path split for DOCX/XLSX/PPTX; the ODF/EPUB/iWork/HWP/CDR tag corpus; the
  dc:title/dc:creator metadata channel) vs core's first-entry [Content_Types].xml gate only.
  File with S-032's consumer per the school-2 note.

Also: the lead lane's school-1 duplicates (CHM/ONE/MNY/EMF/SWF/3DS/FCPEvt) are dropped in
this merge — the formats landed via this rollup's school 1 (`b92ebef`) and the lead lane's
decode evidence is convergent with the SCAN-4 table + the remote errata E56/E57/E58/E65/E66;
no separate defect remains. The one batch-1 verification failure the parallel waves did NOT
cover is filed as **M-090** (DMG: the og's FileDMGAnalyser walks the "koly" UDIF trailer
+0x200 with DMG's own 0xC800000 bound :49436-49440/:63806 — core walks the bzip2 EOS +10
with the global 0xBEBC200 cap, truncating carved DMGs before the mandatory footer; erratum
E67) and **M-091** (the MDB page-chain size walk decoded: 0x1000 strides, page-type word
<= 0x901, cap 0x6400000, :52001-52039 — the landed row's "undecoded" comment is falsifiable;
complements the parallel M-084 ACCDB second-signature finding).


## SA-9 addendum (2026-10-28 — the third lane's consolidated decode table; merged at the push-time reconciliation)

The SA-9 lane decoded the batch-2 set at the registration sites in parallel
(audited `35adcb9`). Its consolidated table — with the per-format key/id/
class/bounds/gate one-liners and the product-absence marks taken at
`35adcb9` (immediately pre-`b92ebef`) — is preserved verbatim at
`evidence/decompiled-quotes/s9-batch2-registrations.md`, with the
instruction-level CHM disassembly (capstone on the og binary,
`evidence/decompiled-quotes/s9-chm-gate-asm.md`) corroborating erratum
E66's tooling caveat and the school-1 landing's gate. The lane's
product-side registry inventory (the 101 FMT_* rows + the per-magic
absence greps) is at
`evidence/executions/s9-carve-registry-inventory.md` — the pre-landing
baseline the school-1/2/3 children can diff against.

Decodes this lane contributed beyond the then-current record (now
convergent with the children): MNY's "MSISAM Database" 15-byte memcmp@+4
gate (the +4 prologue question stays the child's open item); EMF's
`" EMF"`@+0x28 + sReserved + nBytes gate with NO keyed registration and
NO iType-1 check; the ONE ctor's own bounds {20, 500,000,000}; SWF's
version ∈ [3,20] + RECT parse with ZWS rejected; the OfficeTagFromXML
rels-name classifier quote (word/xl/ppt → 0x49/0x4A/0x4B,
FUN_180046120 :53155-53195); and the MMOR/IIRS/FUJI → TiffAnalyzer
attribution correction (see the M-081 SA-9 addendum).
