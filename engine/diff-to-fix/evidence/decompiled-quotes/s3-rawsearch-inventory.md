# S3 evidence — RAWSearchDLL analyser/format inventory + scan-dispatch decode (OQ-T8 resolved)

**Agent:** S3-CARVE · **Session:** 3 · **Corpus:** `easeus-drw-research @ 03ad46c` ·
**Core:** `core_recovery @ 34b3441`
**Primary corpus artifact:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
(80,614 lines; cite = the line numbers in that file) + `RAWSearchDLL_functions.txt`
(symbol→address) + `og-codebase/RAWSearchDLL.dll` (bytes probed with python PE-walks).
This file supersedes the registry/key tables in
`artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md` §3 and
`artifacts/SearchDLL/raw-dual-registry--u16-magic-keys.md` §2 wherever they disagree
(see §8 — six census rows are corrected here).

---

## 1. Registry architecture (decoded — refines the session-2 model)

The registry singleton lives at `DAT_1801181a0` (init `FUN_180006730`,
RAWSearchDLL_decompiled.c:4216-4248; getter `FUN_180006a80`, :4317-4336,
thread-safe magic-static). It holds **two parallel SETS of three containers**:

| offset | container | built by | role |
|---|---|---|---|
| +0x00 | `std::list` | `FUN_180006b30` (:4386-4434) → `FUN_180004fb0` mode 0 | **working** analyser list (rebuilt per-search with host filter) |
| +0x10 | `std::map<u32,…>` | `FUN_180006b30` → `FUN_180002a50` (:1436-2515) | **working** u32 (4CC) map |
| +0x20 | `std::multimap<u16,…>` | `FUN_180006b30` → `FUN_180004320` (:2517-2914) | **working** u16 map |
| +0x30 | `std::list` | `FUN_180006730` → `FUN_180004fb0` mode 1 | **master** list → `EUGetAnalyserList` |
| +0x40 | `std::map<u32,…>` | `FUN_180006730` → `FUN_180002a50` | **master** u32 map → `EUGetAnalyserMap` |
| +0x50 | `std::multimap<u16,…>` | `FUN_180006730` → `FUN_180004320` | **master** u16 multimap → `EUGetAnalyserMultimap` |

- `EUGetAnalyserList/Map/Multimap` (:72170-72208) = `FUN_180006a80()` + member
  offset (+0x30 / +0x40 / +0x50 — `FUN_180006b20/b00/b10`, :4341-4381). They expose
  the **master** (unfiltered) copies; the **scan uses the working copies** (+0x10/+0x20/+0x00),
  which `FUN_180006b30` rebuilds on every searcher init, applying the host's
  analyser-filter interface (searcher+0xe8 → vtable slot 0x90 → filter ptr; call at
  :9180-9182, filter loop :4411-4431 removes analysers whose id−0x30 ∈ mask 0x1000801
  or that the filter callback rejects).
- **u32 map value semantics:** `FUN_180006c80` (:4450-4484) = lower_bound find-or-create;
  builders write `*plVar3 = analyser` — a re-registration of the same u32 key
  **overwrites**. One analyser per u32 key.
- **u16 map value semantics:** `FUN_18000ac20` (:7916-7928) always allocates a NEW
  node (copy key u16 + analyser ptr), `FUN_18000ac70` (:7933-7961) does a plain
  tree insert walking right on key-equal — a **multimap**: equal keys chain as
  separate nodes, and in-order traversal visits equal keys in **insertion order**.
- Node layout (both maps): `_Left +0, _Right +8, _Parent +0x10, _Isnil byte +0x19,
  key +0x20, analyser ptr +0x28` (verified by the walks at :4460-4478 and :9959-9993).

### 1.1 The two-generation analyser object model

Two object shapes (constructed in the builders):

- **Dual-vtable (ISpecialRawFileAnalyser)**, 0x20 bytes:
  `{IRawFileAnalyser vtable @0, format-id @8, ISpecialRawFileAnalyser vtable @0x10,
  legacy File*Analyser @0x18}` — e.g. `FUN_180033620` (BruteForce::JpegAnalyzer +
  FileJPGAnalyser, :39280-39305), `BruteForce::CDXFAnalyser` (:1925-1935),
  `FileZIPAnalyser` (:2210-2232 region).
- **Single-vtable + fallback/state ptr**, 0x18 bytes:
  `{IRawFileAnalyser vtable @0, format-id @8, ptr @0x10}` — the ptr is either the
  legacy analyser (PdfAnalyzer+FilePDFAnalyser :41117-41136, GZ :2785-2799,
  PNG :2040-2061 region) or a parser-state object (TiffAnalyzer's 0x188-byte IFD
  state, :42450-42475).

`IRawFileAnalyser` vtable slots used by the dispatch: **slot 8 (+0x40)** = returns
the special/fallback view (0 = none), **slot 2 (+0x10)** = plain
`Analyse(header[,ctx])`, special slot 0 = `AnalyseEx(header, ctx, sector_size, …)`;
slot 0 (+0x00) of the IRawFileAnalyser vtable is the scalar deleting destructor
(`(**vt)(obj,1)` at :4429). Slot 4 (+0x20) = `ContinueScan` (pending-list, §5);
slot 7 (+0x38) = `CreateFileInfo`; slot 3 (+0x18) = stream-info for the video-repair
hook; slot 9 (+0x48) = repair submit (see §7).

---

## 2. Scan dispatch — THE OQ-T8 ANSWER (decoded, decompiled evidence)

### 2.1 Consult order: u32 map FIRST, then u16 multimap, then the list layer

`FUN_18000cf60` (:9730-9925) — the per-candidate dispatch
(`searcher, header_ptr, analysis_ctx, position, scan_ctx`):

```c
9842  if (DAT_180118204 == 0) {            // "only_common" mode OFF (shipped default, §6)
9843    uVar2 = *param_2;                  // u32 LE read of candidate's first 4 bytes
9844    if (uVar2 != 0) {                  // all-zero 4-byte header skips the u32 layer
9845      puVar5 = FUN_180006a80(...);      // registry singleton
9846      puVar4 = *(puVar5 + 0x10);       // WORKING u32 map
          ... lower_bound walk (9847-9864, key at node+0x20) ...
9865      if (found exact key) {
9866        plVar7 = node->analyser;
9867        lVar6 = slot8(plVar7);          // GetSpecial/GetFallback
9868        if (lVar7 == 0) { iVar3 = slot2(plVar7, header, ctx); if (iVar3==1) → EMIT; return; }
9877        else { special = slot8(plVar7); iVar3 = special->slot0(header, ctx, sector_size);
               if (iVar3) → EMIT; return;                    // accept short-circuits below
9880          iVar3 = slot2(plVar7, header);                 // legacy/plain fallback
9881          if (iVar3) → FUN_18000d920(...); }             // record, analyser=0 (fmt id 8, §2.3)
9891      }
9891    plVar10 = FUN_18000d3e0(searcher, (u16*)header, ctx, pos, scanctx);  // u16 layer
9892    if (plVar10 != 0) goto done;       // u16 accept stops everything
9893  }
9894  if (-1 < (longlong)pos) {            // list layer ALWAYS runs (even in only_common)
9896    for (each node of the working LIST, insertion order) { … §2.2 … }
      }
```

**Order = u32 (4CC) map → u16 multimap → list.** Each earlier layer's ACCEPT
short-circuits all later layers; earlier-layer REJECTS fall through. The
session-2/corpus inference "u16 first (cheap), then 4CC" is **refuted** (errata §8).
This matters when a candidate matches both a u32 and a u16 key: the u32 analyser
gets first claim (e.g. `00 01 00 00` is a u32 key → FileTTFAnalyser *and* a u16 key
0x0100 → FileMDBAnalyser; TTF wins because the u32 map is consulted first —
note the two og layers disagree about what `00 01 00 00` is).

### 2.2 u16 equal-key chains: insertion order, first-accept-wins

`FUN_18000d3e0` (:9933-10063) = `multimap::equal_range(key)` (the two-phase walk
:9959-9993 is MSVC `_Eqrange`+`_Upper_bound`) then iteration
`[lower, upper)` with successor-advance (:10041-10061):

```c
9994  do {
9995    if (lower == upper) return 0;         // no analyser for this u16 key
9998    plVar10 = lower->analyser;
9999    lVar7 = slot8(plVar10);
10001   if (lVar7 == 0) { if (slot2(plVar10, header) == 1) → accept-path (10004-10016); }
10020    else { special = slot8(plVar10);
              if (special->slot0(header, ctx, sector, pos, …)) → accept-path;
10026        else if (slot2(plVar10, header)) {   // PLAIN accept after SPECIAL reject:
10028-38          record {pos → ctx->id or literal 8} in the searcher's position map
                   and CONTINUE the chain (no return) } }
        lower = successor(lower); }
```

**Collision resolution (OQ-T8 core question):** within one u16 key the analysers
are tried **in builder-insertion order**; the **first accept wins** and stops.
The one wrinkle: a *plain-slot accept after a special-slot reject* only records the
position with format id `ctx->+0x18` or **literal 8** (generic) and **keeps trying
later analysers** in the chain. Real chains decoded: `'%!'` (0x2125) →
[BruteForce::PdfAnalyzer, BruteForce::CEPSAnalyser] (:2536, :2549 — a PostScript
candidate tries the PDF analyser first, EPS second); `0xfeff` →
[BruteForce::MP3Analyzer (:2657), RawSpeed::HTMLAnalyser (:2878)] — an FF FE
(UTF-16LE BOM) candidate tries the MP3 frame-sync walker first, the UTF-16 HTML
analyser second.

### 2.3 TIFF-family collision resolution (the OQ-T8 example case)

There is **no cross-analyser TIFF collision at all**: the whole family is keyed on
DISTINCT u32 magics, each mapping to its **own instance of the same class**
(BruteForce::TiffAnalyzer, id 0x40, ctor `FUN_180037b50` :42450):

| og u32 key | bytes | → class |
|---|---|---|
| 0x002a4949 | `II*\0` | TiffAnalyzer (LE TIFF / CR2 / DNG relabel inside) |
| 0x2a004d4d | `MM\0*` | TiffAnalyzer (BE TIFF) |
| 0x00554949 | `IIU\0` | TiffAnalyzer (RW2) |
| 0x49494f52* | `IIRO` | TiffAnalyzer (ORF) — *LE-read shown as key 0x4f524949 at :1717* |
| 0x49494f52→0x53524949 | `IIRS` | TiffAnalyzer (ORF) (:1728) |
| 0x494a5546 | `FUJI` | TiffAnalyzer (RAF — validates `FUJIFILMCCD-RAW ` inside) (:1736) |
| 0x524f4d4d | `MMOR` | TiffAnalyzer (ORF BE) (:1744) |

The relabeling (TIFF vs CR2 vs DNG vs RW2/ORF/RAF) happens INSIDE the analyser
(which sets the analysis-ctx format id at ctx+0x18 — read back at :10005-10008 and
:10167-10170), not by chain ordering. The core's split entries
(FMT_TIFF/FMT_CR2/FMT_DNG first-validated-wins at the same position,
`cr_carve.cpp:35-41` kTrialOrder) are functionally equivalent for these formats —
the core checks "CR" at +8 / DNGVersion tag inside its validators
(`cr_analysers.cpp:399-407, 1023-1050`), exactly the sub-dispatch the og does in
one analyser. **MATCH** for the shared TIFF family.

### 2.4 JPEG family (the OQ-T8 second example)

Four u32 keys — 0xe0ffd8ff / 0xe1ffd8ff / 0xdbffd8ff / 0xeeffd8ff (:1458-1572) —
each → its own **BruteForce::JpegAnalyzer** (id 0x23) with a FileJPGAnalyser
fallback (ctor :39280-39305). The og therefore carves JPEGs whose 4th byte is
E0/E1/DB/EE only. See finding S-013 for the accept-criteria comparison (no EOI
requirement in the og; `0xd9` appears **zero** times in the JpegAnalyzer body
:39489-41110 and the FileJPGAnalyser body :57080-57567).

### 2.5 Zero-header guard

`if (*(int*)param_2 == 0) return 0;` guards both layers (u32: :9843-9844;
u16: :9954-9956) — candidates whose first 4 bytes are zero skip u32+u16 entirely.
Consequence: the u16 registration `0x0000 → BruteForce::MovAnalyzer` (:2748) is
**dead**; MOV/MP4 detection lives in the LIST layer (§4) plus the video-repair
feed (§7).

### 2.6 `only_common` mode (internal test mode)

`DAT_180118204` = ini read `FUN_180007890(section="only_common", key="User",
file="config/config.ini")` (strings at RVA 0x66bcc: `only_common\0User\0` and
`config/config.ini`, byte-probed from `og-codebase/RAWSearchDLL.dll` file offset
0x651c0; call :9834). The shipped `og-codebase/config/config.ini` has **no
[only_common] section** (grep: absent) → mode off → full u32+u16 dispatch. In
`FUN_180004fb0` mode 0 the same read gates an early-out list build (:3017-3055):
if the section exists, the working list = [CommonRawAnalyser] ONLY (goto :4205);
otherwise the full structural list of §4 is built with **CommonRawAnalyser near the
END** — so structural analysers claim positions before the raw-signs engine. The
core's layering (grammar first, raw-signs last, `cr_carve.cpp:186-300`) matches
the shipped og configuration.

---

## 3. Complete u32 (4CC) registry — 68 registrations, 65 unique keys

Extraction: builder `FUN_180002a50` (:1436-2515); each block = construct analyser →
set key → `FUN_180006c80` find-or-create → store. "id" = the analyser's format id
(field at +8; `0xffffffff` = the pre-ctor sentinel seen in the block, omitted).
"core" column = `core_recovery/src/core/carve/cr_registry.cpp` kTable @ 34b3441:
MATCH / MISSING / superset.

| og key (u32 LE) | on-disk bytes | format | og analyser (id) | core status |
|---|---|---|---|---|
| 0xe0ffd8ff | FF D8 FF E0 | JPEG JFIF | BruteForce::JpegAnalyzer (0x23) +FileJPG | **MATCH+superset** (FMT_JPG magic FF D8 FF any-4th; I-009) |
| 0xe1ffd8ff | FF D8 FF E1 | JPEG EXIF | BruteForce::JpegAnalyzer (0x23) | **MATCH+superset** |
| 0xdbffd8ff | FF D8 FF DB | JPEG quant-first | BruteForce::JpegAnalyzer (0x23) | **MATCH+superset** |
| 0xeeffd8ff | FF D8 FF EE | JPEG JFIF-ee/Adobe | BruteForce::JpegAnalyzer (0x23) | **MATCH+superset** |
| 0x46445025 | %PDF | PDF | BruteForce::PdfAnalyzer (0x32) +FilePDF | MATCH (FMT_PDF) |
| 0x2a004d4d | MM\0* | TIFF BE | BruteForce::TiffAnalyzer (0x40) | MATCH (FMT_TIFF_BE, CARVE-3) |
| 0x002a4949 | II*\0 | TIFF LE/CR2/DNG | TiffAnalyzer (0x40) | MATCH (family split, §2.3) |
| 0x00554949 | IIU\0 | RW2 | TiffAnalyzer (0x40) | MATCH (FMT_RW2) |
| 0x4f524949 | IIRO | ORF | TiffAnalyzer (0x40) | MATCH (FMT_ORF) |
| 0x53524949 | IIRS | ORF | TiffAnalyzer (0x40) | MATCH (FMT_ORF_IIRS) |
| 0x494a5546 | FUJI | RAF | TiffAnalyzer (0x40) | MATCH (FMT_RAF 16-byte magic) |
| 0x524f4d4d | MMOR | ORF BE | TiffAnalyzer (0x40) | MATCH (FMT_ORF_MMOR) |
| 0x001a4949 | II\x1a\0 | Canon CRW (CIFF) | Brance::CRWAnalyser (ctor FUN_18003c9f0) | **MISSING** (N-026) |
| 0x001a4d4d | MM\x1a\0 | Canon CRW BE | Brance::CRWAnalyser | **MISSING** (N-026) |
| 0x31685a42 | BZh1 | bzip2 (DMG payload) | Brance::DMGAnalyser (0x71) +FileDMG | **MISSING** (S-007 DMG) |
| 0x62564f46 | FOVb | Sigma X3F | Brance::X3FAnalyser (0x6c) +FileX3F | **MISSING** (N-026) |
| 0x6d783f3c | `<?xm` | XML | Brance::SVGAnalyser (ctor FUN_180041580) | **MISSING** (N-026) |
| 0x3cbfbbef | EF BB BF `<` | XML UTF-8 BOM | Brance::SVGAnalyser | **MISSING** (N-026) |
| 0x000001cd | CD 01 00 00 | XML variant | Brance::SVGAnalyser | **MISSING** (N-026) |
| 0x6776733c | `<svg` | SVG | Brance::SVGAnalyser | **MISSING** (N-026) |
| 0x000001b3 | 00 00 01 B3 | MPEG-1/2 video ES | Brance::MPGAnalyser (0x6d) | **MISSING** (S-007 MPEG) |
| 0x000001ba | 00 00 01 BA | MPEG-PS pack | Brance::MPGAnalyser (0x6d) | **MISSING** (S-007 MPEG) |
| 0x5367674f | OggS | Ogg | Brance::OGGAnalyser +CFileOggAnalyser (0x52) | **MISSING** (N-026) |
| 0x2e464d52 | .RMF | RealMedia | Brance::RM_RMVBAnalyser +FileRM (0x5e) | **MISSING** (N-026) |
| 0x060e2b34 | 06 0E 2B 34 | MXF (SMPTE) | Brance::MXFAnalyser | **MISSING** (N-026) |
| 0x75b22630 | 30 26 B2 75 | ASF/WMV/WMA | BruteForce::CAsfAnalyser (id 6) | MATCH (FMT_ASF) |
| 0x0d302020 | `  0\r` | AutoCAD DXF (ASCII) | BruteForce::CDXFAnalyser (0x19, ISpecial) | **MISSING** (S-007 DXF) |
| 0x4d414320 | `MAC ` | Monkey's Audio | BruteForce::CAPEAnalyser (0x51) | **MISSING** (S-007 APE) |
| 0x0000011c | 1C 01 00 00 | FreeHand FH11 | BruteForce::CFH11Analyser (0x1d) | **MISSING** (N-026) |
| 0x694c5153 | SQLi | SQLite | BruteForce::CFCPEvtAnalyser (0x73)* | MATCH (FMT_SQLITE) — *class name is RTTI-odd; magic+id decoded at :1963-1974 |
| 0xc5d0d3c6 | C5 D0 D3 C6 | Adobe Illustrator AI/EPS | BruteForce::CEPSAnalyser (0x1b) | **MISSING** (N-026; census called this dBase — errata §8) |
| 0x41433030 | AC10 | AutoCAD DWG | BruteForce::CDWGAnalyser (0x18) | **MISSING** (S-007 DWG) |
| 0x47494638 | GIF8 | GIF | BruteForce::CGifAnalyser | **MISSING** (S-007 GIF) |
| 0x6468544d | MThd | MIDI | RawSpeed::MIDAnalyser +FileMID (0x26) | **MISSING** (S-007 MIDI) |
| 0x89504e47 | 89 PNG | PNG | RawSpeed::PNGAnalyser +FilePNG (0x33) | MATCH (FMT_PNG) |
| 0x38504253 | 8BPS | Photoshop PSD | FilePSDAnalyser | MATCH (FMT_PSD) |
| 0x7b5c7274 | {\rt | RTF | RawSpeed::RTFAnalyser +FileRTF (0x3c) | **MISSING** (S-007 RTF) |
| 0x6e696150 | `Pain` | Paint Shop Pro | RawSpeed::PSPAnalyser +FilePSP (0x37) | **MISSING** (N-026) |
| 0x52617221 | Rar! | RAR4 | RawSpeed::RARAnalyser +FileRAR (0x3a) | MATCH (FMT_RAR4); core adds RAR5 (clean-room superset) |
| 0x53747566 | Stuf | StuffIt | RawSpeed::SITAnalyser +FileSIT (0x5d) | **MISSING** (N-026) |
| 0x7b5c52e4 | E4 52 5C 7B | OneNote `.one` (GUID {E4525C7B…} LE) | RawSpeed::OneAnalyser +FileONE (0x31) (:2156-2177) | **MISSING** (N-026; census called this "LZH arc" — errata §8) |
| 0x00777067 | `wpg\0` | WordPerfect WPG | RawSpeed::WpgAnalyser (0x33) (:2178-2191) | **MISSING** (N-026) |
| 0x36464d4a | JMF6 | Outlook Express DBX variant | RawSpeed::DbxAnalyser | **MATCH** (FMT_DBX_JMF6 — landed with M-118/S42; the never-filed N-026 residue re-pointed) |
| 0xcf12adfe | CF AD 12 FE | Outlook Express DBX | RawSpeed::DbxAnalyser | **MATCH** (FMT_DBX — landed with M-118/S42; the never-filed N-026 residue re-pointed) |
| 0x2c3489f7 | F7 89 34 2C | DBX folder magic | RawSpeed::DbxAnalyser | **MATCH** (FMT_DBX_MARKED — landed with M-118/S42; the never-filed N-026 residue re-pointed) |
| 0x04034b50 | PK\0\3\4 | ZIP local header | FileZIPAnalyser (ISpecial) | MATCH (FMT_ZIP + FMT_DOCX + FMT_ZIP_CD) |
| 0xff575043 | FF WPC | WordPerfect WPD | FileWPDAnalyser | **MISSING** (N-026) |
| 0x00090001 | 01 00 09 00 | Windows WMF | FileWMFAnalyser | **MISSING** (N-026) |
| 0x9ac6cdd7 | D7 CD C6 9A | placeable WMF | FileWMFAnalyser | **MISSING** (N-026) |
| 0x00000100 | 00 01 00 00 | TrueType TTF (versus u16 0x0100=MDB — the u32 layer wins, §2.1) | FileTTFAnalyser | **MISSING** (N-026) |
| 0x52494646 | RIFF | RIFF family (AVI/WAV sub-dispatch) | Brance::RIFFAnalyser +FileRIFF (0x3b) | MATCH (FMT_AVI/FMT_WAV split) |
| 0x214e4442 | !BDN | Outlook PST/OST | FilePSTAnalyser | MATCH (FMT_PST) |
| 0xe011cfd0 | D0 CF 11 E0 | OLE compound (doc/xls/ppt/msi) | FileOLEAnalyser | **MISSING** (S-007 OLE) |
| 0x002da5db | DB A5 2D 00 | legacy "LowDoc" | FileLowDocAnalyser | **MISSING** (N-026) |
| 0x0094a62e | 94 A6 2E 00 | legacy "LowDoc" | FileLowDocAnalyser | **MISSING** (N-026) |
| 0x00010000 | 00 00 01 00 | ICO | FileICOAnalyser | **MISSING** (N-026) |
| 0x00020000 | 00 00 02 00 | CUR | FileICOAnalyser | **MISSING** (N-026) |
| 0x00035f3f | 3F 5F 03 00 | Windows Help | FileHLPAnalyser | **MISSING** (N-026) |
| 0x464f524d | FORM | IFF/AIFF | CFileAIFAnalyzer (0x50) | **MISSING** (N-026) |
| 0x43505437 | CPT7 | Corel PhotoPaint | RawSpeed::CPTAnalyser (ctor FUN_1800132c0) | **MISSING** (N-026) |
| 0x43505438 | CPT8 | Corel PhotoPaint | RawSpeed::CPTAnalyser | **MISSING** (N-026) |
| 0x43505439 | CPT9 | Corel PhotoPaint | RawSpeed::CPTAnalyser | **MISSING** (N-026) |
| 0x4d534346 | MSCF | CAB | FileCABAnalyser (0xc) | MATCH (FMT_CAB) |
| 0x6d736366 | mscf | CAB lowercase | FileCABAnalyser (0xc) | **MISSING key** (core has MSCF only — N-026) |
| 0x2e646e73 | .snd | Sun/NeXT AU | FileAUAnalyser | **MISSING** (S-007 AU) |
| 0x1a45dfa3 | 1A 45 DF A3 | EBML/MKV/WebM | FileMKVAnalyser (0x60) | MATCH (FMT_MKV) |
| 0x49535446 | ITSF | CHM | FileCHMAnalyser (0xe) | **MISSING** (S-007 CHM) |
| 0x004d524d | \0MRM | Minolta MRW | FileMRWAnalyser (0x66) | **MISSING** (N-026) |

Builder line references: JPEG quartet :1458-1572; PDF :1573-1602; TIFF-family
:1603-1745; CRW :1746-1761; DMG :1762-1783; X3F :1784-1803; XML/SVG :1804-1843;
MPG :1844-1861; OGG :1862-1879; RM :1880-1903; MXF :1904-1917; ASF :1913-1924;
DXF :1925-1938; APE :1939-1950; FH11 :1951-1962; SQLite :1963-1974; AI/EPS
:1975-1987; DWG :1988-1999; GIF :2000-2011; MID :2012-2031; PNG :2032-2053;
PSD :2054-2065; RTF :2066-2087; PSP :2088-2109; RAR :2110-2131; SIT :2132-2153; OneNote :2156-2177; WPG :2178-2191;
DBX×3 :2192-2231; ZIP :2232-2246; WPD :2247-2258;
WMF×2 :2259-2282; TTF :2283-2294; RIFF :2295-2316; PST :2317-2328; OLE :2329-2341;
LowDoc :2342-2365; ICO/CUR :2366-2389; HLP :2390-2401; IFF :2402-2415;
CPT7/8/9 :2416-2439; CAB :2440-2461; AU :2462-2473; MKV :2474-2485; CHM
:2486-2497; MRW :2498-2510.

---

## 4. Complete u16 registry — 35 registrations, 33 unique keys

Builder `FUN_180004320` (:2517-2914), keys = first two bytes LE:

| og key (u16) | bytes | format | og analyser (id) | core status |
|---|---|---|---|---|
| 0x2125 | %! | PostScript/EPS (chain: PDF first, EPS second, §2.2) | PdfAnalyzer; BruteForce::CEPSAnalyser (0x1b) | **MISSING** (S-007 EPS) |
| 0x4944 | ID | MP3 via ID3 | BruteForce::MP3Analyzer (0x2b) +FileMP3 | MATCH (FMT_MP3 magic "ID3") |
| 0xfff2–0xffff (12 keys) | FF F2…FF FF | headerless MPEG audio (MP3 layers) | BruteForce::MP3Analyzer ×12 | **MISSING** (S-007 headerless MP3) |
| 0xffe2–0xffe7 (6 keys) | FF E2…FF E7 | MPEG-2/2.5 low-rate audio | BruteForce::MP3Analyzer ×6 | **MISSING** (S-007) |
| 0xfff1, 0xfff9 | FF F1 / FF F9 | AAC ADTS (2 syncs) | BruteForce::CADTSAnalyser (0x2d) | **MISSING** (S-007 AAC) |
| 0x3939 | `99` | DXF variant | BruteForce::CDXFAnalyser (0x19) | **MISSING** (S-007 DXF) |
| 0x0000 | \0\0 | (DEAD — zero-header guard, §2.5) | BruteForce::MovAnalyzer (0x2a) | n/a (list layer does MOV) |
| 0x8b1f | 1F 8B | gzip | RawSpeed::GZAnalyzer +FileGZ (0x1f) | **MISSING** (N-026) |
| 0x0100 | 00 01 | MS Access JET4 MDB | RawSpeed::CMDBAnalyser +FileMDB (0x27) | **MISSING** (N-026) |
| 0x464c | FL | FLV | Brance::FLVAnalyser +FileFLV (0x5a) | MATCH (FMT_FLV magic "FLV") |
| 0x3c68 | `<h` | HTML | RawSpeed::HTMLAnalyser (ctor FUN_180012180) | **MISSING** (N-026) |
| 0x3c48 | `<H` | HTML | RawSpeed::HTMLAnalyser | **MISSING** (N-026) |
| 0x3c21 | `<!` | HTML/DOCTYPE | RawSpeed::HTMLAnalyser | **MISSING** (N-026) |
| 0xfffe | FF FE | HTML UTF-16LE BOM (chain: MP3 sync first, §2.2) | MP3Analyzer; HTMLAnalyser | **MISSING** (N-026) |
| 0x4d4d | MM | Autodesk 3DS (MAIN chunk 0x4D4D) | RawSpeed::New3DSAnalyser (ctor FUN_1800126c0) | **MISSING** (N-026; dual-registry artifact called this TIFF-BE — errata §8) |
| 0x424d | BM | BMP | FileBMPAnalyser (0xb) | **MISSING** (S-007 BMP) |
| 0x4d53 | MS | MS Money MNY | FileMNYAnalyser (0x28) | **MISSING** (N-026) |

*(The corpus artifact's headline "39 u16 keys" overcounts its own table; the builder
performs exactly 35 registrations — 33 unique keys, 2 doubled: 0x2125, 0xfeff.)*

## 4b. The LIST layer — 25 analysers (FUN_180004fb0, :2919-4208)

Insertion order (master build; working build is identical when config lacks
[only_common], §2.6): FileSWFAnalyser (0x3e) · Brance::LZHAnalyser (0x24)
+FileLZH · BruteForce::CMTSAnalyser (MPEG-TS, ISpecial) · BruteForce::CM2TSAnalyser
(M2TS, ISpecial) · FileEMFAnalyser · RawSpeed::TARAnalyser +FileTAR ·
**BruteForce::MovAnalyzer (0x2a — THE MOV/MP4 carver)** ·
**RawFormat_xjq::HEICAnalyser** · CVCFAnalyser (vCard) · RawFormat_xjq::M1VAnalyser
· RawFormat_xjq::JP2Analyser · RawFormat_xjq::MOBIAnalyser · CPOSAnalyser ·
COSPAnalyser · CHFPAnalyser · CHIKAnalyser · **C7ZIPAnalyser** · CGPXAnalyser ·
AEPAnalyser · **RawFormat_xjq::CommonRawAnalyser (0xa8 — the raw_signs.json
engine; sits AFTER the structural analysers)** · File3GPAnalyser ·
CFileAIFAnalyzer · CFileAACAnalyzer · FileFCPEVENTAnalyser.

These list-layer analysers receive `(header_ptr, analysis_ctx, sector_size)` via
the special interface (or `(header_ptr)` via slot 2) and scan the 8 MiB buffer
themselves (BruteForce style). Core status: HEIC/7z/MOV exist as core grammar
validators but **HEIC/MOV/M4A/MP4 signatures are unreachable** (S-012 — executed);
M1V/JP2/MOBI/AAC missing (S-007); the rest missing (N-026).

---

## 5. Scan loop + read model (the og's carve READ model — decoded)

`FUN_18000c820` (searcher virtual, :9385-9633) — called with
`(searcher, start_LBA, sector_count, bitmap_ptr)`:

1. **Read:** device vtable slot 1 fills `buffer + read_size/2` with this chunk
   (:9434-9438). `read_size = blocks × sector × 2` where `blocks = 4 MiB / sector`
   (FUN_18000c430 :9269-9275, init FUN_18000beb0 :9195-9209) → **8 MiB buffer for
   512-B sectors, filled 4 MiB at a time, +0x1000 (4 KiB) tail** (:9207). After the
   chunk: `memmove(buffer, buffer+half, half)` (:9620-9622) — a ping-pong
   double-buffer; the previous 4 MiB stay readable as left-context (analysers read
   across the boundary), and every chunk is scanned once.
2. **Candidate granularity: SECTORS.** `candidate = buffer + half + sector_index ×
   sector_size` (:9496), positions = `start_LBA + sector_index` (:9479-9481);
   the dispatch (§2) runs once per sector-aligned position. **The og never tries
   mid-sector candidate offsets** (vs the core's byte-granular scan — I-009).
3. **Bitmap gating (M-005 confirmation at loop level):** if a bitmap object is
   supplied, `FUN_180058df0` tests `(pos)/cluster` (:9451-9459) and allocated
   clusters are skipped whole (`local_1e0 += cluster_sectors−1`, :9458) — unless
   the "full scan" override at bitmap+0x60 ([0xc] vtable slot 4) says otherwise
   (:9455-9456).
4. **Per-sector pre-filters before dispatch:** a leading-zero flag (4-byte test,
   :9498-9501) and a trailing-zero-words count scanning back from the sector end
   (7 words max, :9502-9510) are computed into the analysis context
   (local_1a0/local_19c) — cheap "is this sector empty/padding" hints the analysers
   consult. No entropy filter.
5. **Emission + pending model:** dispatch accept → the analyser's slot-7
   (`CreateFileInfo`, :9541) produces a `CRawFileInfo {vtable, position, size,
   format-id @+0x18, analyser @+0x1a, id dup @+0x1c, last-pos @+0x1d}`; recorded
   in the found-list + position map (searcher+0xd0 array of 0x1000 maps,
   entry map at +0x10038 — 0x10048-byte alloc :9298-9309). **If the size is not
   yet known (info[2]==0) and `info->slot1()` says pending, the object moves to
   the pending list (searcher+0xf8)** (:9573-9605); every subsequent sector
   iteration calls `FUN_18000d670` (:10068-10152), which re-feeds each pending
   analyser `ContinueScan(header, sector_size, position, scanctx)` (vtable slot 4,
   :10114-10115) until it reports done, then finalizes (record + emit through the
   +0x108 callback + optional video-repair submit :10123-10136). **Streaming carve
   state across chunk boundaries** — the core's validators instead re-read whole
   extents from the device statelessly (`cr_analysers.cpp` rd/probe helpers).
6. **Chunk feed to the video-repair engine:** after each chunk the
   `searcher+0x158` interface (EUVideoRepair.dll `GetIMovMp4Repair`,
   LoadLibraryA at :9210-9220) gets `(position, buffer_half, half_size)`
   (vtable slot 8 at :9623-9628) — see N-024.
7. **Progress:** not in this function — the controller's chunk driver (external
   virtual) reports per 4 MiB chunk; the core reports per 1 MiB window
   (`cr_carve.cpp:375`).

---

## 6. Runtime format filter (RawConfig.ini) — the working-registry gate

`og-codebase/config/RawConfig.ini` `[Filter]` section (read: 37 toggles — **CORRECTED to 34 by CORRECTIONS.md #5, recounted first-hand 2026-10-28; see M-024's addendum**, e.g.
`HEIC=1 VCF=1 WEBM=1 WEBP=1 M1V=1 M2T=1 M2TS=1 JP2=1 STL=0 M4P=1 XMIND=1 RVT=1
MOBI=1 MMAP=1 POS=1 EDDX=1 WFP=1 VPJ=0 OSP=1 HFP=1 DRP=1 HIK=1 ODP=1 ODT=1 ODS=1
7Z=1 INSP=1 INSV=1 GPX=1 CRM=1 AEP=1 FFF=1 DPX=1 MUSX=1 …`) — per-format
enable/disable consumed via `GetRawUserInfoMap` (`FUN_180001350()+0x90`,
:72213-72221) and the searcher+0xe8 config interface; the searcher init fetches
the filter (vtable+0x90, :9181) and `FUN_180006b30` **rebuilds the working
registries minus disabled analysers** (removal loop :4411-4431). The names match
the list-layer analysers 1:1 (HEIC/VCF/M1V/M2TS/JP2/MOBI/POS/OSP/HFP/HIK/7Z/GPX/
AEP) — the "newest generation" formats are runtime-toggleable, and **the og ships
with STL and VPJ disabled**. The core has no equivalent surface (M-011).

---

## 7. Registry exports and their consumers (A42/A45 → P42 evidence)

`EUGetAnalyserList/Map/Multimap` (exports 3-5, `rabin2_exports.txt`) are consumed
**inside the SearchDLL family itself**: FATSearchDLL does
`LoadLibraryA("RAWSearchDLL.dll")` + `GetProcAddress("EUGetAnalyserList"/"Map"/
"Multimap")` and **clones the returned master containers into its own members**
(list @+0xb8, map @+0xd0, multimap @+0xe0) —
`FATSearchDLL_decompiled.c:13872-13977` and a second copy at :29290-29361 (the
clone walks are std::map/multimap copy-ctors with leftmost/rightmost fixups,
:13903-13930 and :13946-13975). The FS engines keep a snapshot of the carve
registry for capability enumeration. The carve stream additionally feeds
`EUVideoRepair.dll` (`GetIMovMp4Repair` :9220, per-chunk slot-8 calls :9625-9627,
per-finalize slot-9 submit :10130-10136) for MOV/MP4 salvage/repair (N-024).
The core exposes no registry-enumeration ABI and no repair hook
(`cr_registry.cpp:300-305` accessors are internal; `docs/ABI-CONTRACT.md` has no
carve-registry query; `src/core/repair/` contains only volume rawfixer) — N-025/N-024.

---

## 8. Corpus-document corrections (CORPUS-ERRATA candidates)

| # | Artifact claim | Decoded reality (this session) |
|---|---|---|
| 1 | census §3 + dual-registry §1: "dispatch tries the u16 magic-key first, then falls through to the 4CC table" [INFERRED] | **u32 4CC map first, then u16, then list** (§2.1, :9842-9893) |
| 2 | census row "II\x1a\x00/MM\x1a\x00 = Minolta MRW" | **Brance::CRWAnalyser** = Canon CRW/CIFF (:1746-1761); MRW is the separate `\0MRM` key (:2498-2510) |
| 3 | census row "C5 D0 D3 C6 = dBase/DBF" | **BruteForce::CEPSAnalyser** = Adobe Illustrator AI/EPS (:1975-1987). No dBase analyser exists anywhere in the registry decode — **S-007's "DBF" entry should be dropped from the og set** |
| 4 | census row "!BDN = Bentley/BDN document" | **FilePSTAnalyser** = Outlook PST/OST (:2317-2328) |
| 5 | census rows "CF AD 12 FE / F7 89 34 2C = MS-DOS exec / thumbnail-repair variant" | both → **RawSpeed::DbxAnalyser** = Outlook Express DBX (:2190-2231) |
| 6 | census row "3F 5F 03 00 = Nikon NEF/other" | **FileHLPAnalyser** = Windows Help (:2390-2401) |
| 7 | dual-registry §2 "0x4D4D MM = TIFF big-endian" | **RawSpeed::New3DSAnalyser** = Autodesk 3DS (MM = 0x4D4D MAIN chunk); TIFF-BE rides the u32 key `MM\0*` (:2887-2900) |
| 8 | dual-registry §2 "0x4449 ID = AIFF/IANA-family [INFERRED]" | **BruteForce::MP3Analyzer** = ID3/MP3 (:2558) |
| 9 | dual-registry headline "39 u16 keys" | 35 registrations / 33 unique keys (§4) |
| 10 | census §1 "Map = magic DWORD → analyser chain; Multimap = magic DWORD → all matches" | Map = **u32 4CC → single analyser**; Multimap = **u16 → chain**; List = the raw-signs/structural list (§1) |
| 11 | census row "E4 52 5C 7B = LZH arc"; dual-registry §2 same | **RawSpeed::OneAnalyser** (OneNote, id 0x31) — E4 52 5C 7B is the {E4525C7B…} OneNote GUID prefix (:2156-2177); LZH lives in the LIST layer (Brance::LZHAnalyser) |

---

## 9. Core comparison summary (per shared format)

**MATCH (grammar-layer equivalence, both directions verified):** JPEG (magic; but
see S-013 for accept policy), PNG, TIFF LE/BE, CR2/DNG relabeling, RW2, ORF
(IIRO/IIRS/MMOR), RAF, PDF, ZIP/DOCX/ZIP-CD (N-008 tracks padding deltas), 7z,
SQLite, MP4/MOV/HEIC/M4A **would** match if reachable (S-012), AVI/WAV (RIFF
split vs og sub-dispatch), MKV, RAR4 (+core RAR5 superset), PST, PSD, MP3 (ID3),
ASF, FLV, CAB (MSCF), ISO (core-only superset), MP3-ID3.

**MISSING in core (S-007, already filed):** GIF, BMP, headerless MP3 (12+6 sync
keys), AAC ADTS (2 sync keys), RTF, OLE, MPEG video (2 keys), DWG, CHM, AU, MIDI,
APE, EPS (u32 AI-key + u16 %!-chain), DXF (2 keys), JP2, MOBI, DMG, M1V.

**MISSING in core (NEW this session — N-026):** CRW (2 keys), X3F, XML/SVG (4
keys), OGG, RM/RMVB, MXF, FH11, PSP, SIT, **OneNote**, WPG, DBX (3 keys), WPD,
WMF (2 keys), TTF, LowDoc (2 keys), ICO/CUR (2 keys), HLP, AIFF/IFF, CPT7/8/9,
MRW, HTML (4 keys incl. UTF-16 BOM), GZ, MDB, MNY, 3DS, SWF, EMF, MTS, M2TS, TAR,
LZH (list layer), VCF, GPX, AEP, POS/OSP/HFP, HIK, FCPEVENT, 3GP,
lowercase-mscf CAB key. Plus the **DBF removal** (§8.3). ≈38 formats.

**Core superset over og (I-009):** byte-granular candidates (og = sector-aligned
only, §5.2); JPEG any-4th-byte magic (og = E0/E1/DB/EE only, §2.4); RAR5; ORF
full-magic precision is a wash; ZIP-CD anchor is parity (CARVE-2).

**DIFFERENT-VALIDATOR (findings):** JPEG accept policy (S-013: og = no EOI
requirement, SOF-dimension size; core = EOI+SOF+SOS, EOI-bounded extent);
ZIP padding/version-gate (N-008, prior); BMFF signatures unreachable (S-012).
