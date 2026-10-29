# S68 lane sheet L1 — the RAWSearchDLL static registry initializer, fully decoded (66 keys)

The corpus census doc (`re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md`
§3, 70-key table) left several chains ambiguous ("—") and misattributed two keys. This
sheet records the FIRST-HAND decode of the construct-then-insert initializer
(`RAWSearchDLL_decompiled.c` :1440–:2520, `FUN_180002a50` = the registry builder;
`FUN_180006c80` = the std::multimap<u32, analyser*> insert; pairing verified by the
CEPS↔0xc6d3d0c5 cross-check the core itself registered in session 3).

## Pairing rule (verified)

Each block: `[alloc 0x10/0x18] [set IRawFileAnalyser::vftable] [set ConcreteAnalyser::vftable]
[set id byte at +8]` — then `local_res10[0] = KEY; FUN_180006c80(map, &key) -> slot =
analyser`. The constructed analyser binds to the key assigned immediately after the block.
Constructor helpers wrap shared chains: `FUN_180033620` = JpegAnalyzer→FileJPG fallback;
`FUN_180037b50` = TiffAnalyzer→FileTIF; `FUN_180035e00` = PdfAnalyzer→FilePDF;
`FUN_18003c9f0` = Brance::CRWAnalyser→FileCRW; `FUN_180041580` = Brance::SVGAnalyser→FileSVG;
`FUN_18003f2b0`-class = HTML chains.

## The 66-key table (og key -> LE bytes -> og chain -> core row -> verdict)

| og key | LE bytes | og analyser chain (id) | core row | verdict |
|---|---|---|---|---|
| 0xe0ffd8ff/e1/db/ee | FF D8 FF xx | BruteForce::JpegAnalyzer→FileJPG | FMT_JPG (3B magic FF D8 FF) | MATCH |
| 0x46445025 | 25 50 44 46 | BruteForce::PdfAnalyzer→FilePDF | FMT_PDF (%PDF- 5B) | MATCH (core longer magic, see L2) |
| 0x2a004d4d / 0x2a4949 / 0x554949 | MM\0* / II*\0 / IIU\0 | TiffAnalyzer→FileTIF | FMT_TIFF/_BE/_EP | MATCH |
| 0x4f524949 / 0x53524949 / 0x524f4d4d | IIRO/IIRS/MMOR | TiffAnalyzer→FileTIF (ORF variants) | FMT_ORF/_IIRS/_MMOR | MATCH |
| 0x494a5546 | FUJI | TiffAnalyzer chain (RAF) | FMT_RAF | MATCH |
| 0x1a4949 / 0x1a4d4d | II\x1a\0 / MM\x1a\0 | Brance::CRWAnalyser→FileCRW (MRW) | FMT_CRW_MM (M-081 2-row) + FMT_MRW | MATCH (M-081 landed) |
| 0x31685a42 | BZh1 | Brance::DMGAnalyser→FileDMG | FMT_DMG | MATCH |
| 0x62564f46 | FOVb | Brance::X3FAnalyser→FileX3F | FMT_X3F | MATCH |
| 0x6d783f3c / 0x3cbfbbef / 0x6776733c | <?xm / BOM< / <svg | HTML/SVG chains | FMT_XML/_BOM, FMT_SVG | MATCH |
| **0x1cd** | **CD 01 00 00** | **Brance::SVGAnalyser→FileSVG (the CD01-compressed SVG key)** | **FMT_SVG_CD01** | **MATCH (M-140's family)** |
| 0xb3010000 / 0xba010000 | 00 00 01 B3/BA | MPEGES/MPEGPS chains | FMT_MPEGES/MPEGPS | MATCH |
| 0x5367674f | OggS | CFileOggAnalyser | FMT_OGG | MATCH |
| 0x464d522e | .RMF | Brance::RM_RMVB→FileRM_RMVB | FMT_RM | MATCH |
| 0x342b0e06 | 06 0E 2B 34 | Brance::MXFAnalyser | FMT_MXF | MATCH |
| 0x75b22630 | 30 26 B2 75 | BruteForce::CAsfAnalyser (id 6) | FMT_ASF (16B GUID) | MATCH (core longer magic, L2) |
| **0xd302020** | **20 20 30 0D ("  0\\r")** | **BruteForce::CDXFAnalyser (id 0x19)** | **FMT_DXF {' ',' ','0','\\r'}** | **MATCH — the census doc's "MPEG audio frame sync variant" label WRONG (doc-side erratum)** |
| 0x2043414d | MAC␣ | BruteForce::CAPEAnalyser (id 0x51) | FMT_APE | MATCH |
| 0x11c | 1C 01 00 00 | BruteForce::CFH11Analyser (id 0x1d) | FMT_FH11 | MATCH |
| **0x694c5153** | **SQLi** | **BruteForce::CFCPEvtAnalyser (id 0x73)** | FMT_SQLITE (12B "SQLite forma") + the N-089 FCP-event relabel | **MATCH — the census doc's "SQLite → DB chain" was imprecise: the DB-chain analyser IS the FCP-event analyser (FCPX events are SQLite DBs; the binary carries "SQLite format 3" + "FCPEVENT" strings)** |
| 0xc6d3d0c5 | C5 D0 D3 C6 | BruteForce::CEPSAnalyser (id 0x1b) | FMT_AI (session-3 correction) | MATCH |
| 0x30314341 | AC10 | BruteForce::CDWGAnalyser (id 0x18) | FMT_DWG | MATCH |
| 0x38464947 | GIF8 | BruteForce::CGifAnalyser (id 0x1e) | FMT_GIF | MATCH |
| 0x6468544d | MThd | FileMIDAnalyser | FMT_MID | MATCH |
| 0x474e5089 | .PNG | FilePNGAnalyser (id 0x33) | FMT_PNG (8B) | MATCH (L2) |
| 0x53504238 | 8BPS | FilePSDAnalyser (id 0x36) | FMT_PSD | MATCH |
| 0x74725c7b | {\\rt | FileRTFAnalyser | FMT_RTF | MATCH |
| 0x6e696150 | Pain | FilePSPAnalyser | FMT_PSP | MATCH |
| 0x21726152 | Rar! | FileRARAnalyser | FMT_RAR4 (7B) | MATCH (L2) |
| 0x66757453 | Stuf | FileSITAnalyser | FMT_SIT | MATCH |
| 0x7b5c52e4 | E4 52 5C 7B | FileONEAnalyser (id 0x20?) | FMT_ONE | MATCH |
| 0x677077 | wpg\\0 | RawSpeed::WpgAnalyser (id 0x33) | FMT_WPG (4B wpg\\0) | MATCH |
| 0x36464d4a | JMF6 | RawSpeed::DbxAnalyser (id 0x15) | FMT_DBX_JMF6 | MATCH (M-117/M-121) |
| 0xfe12adcf | CF AD 12 FE | RawSpeed::DbxAnalyser | FMT_DBX | MATCH |
| 0x2c3489f7 | F7 89 34 2C | RawSpeed::DbxAnalyser | FMT_DBX_MARKED | MATCH |
| 0x04034b50 | PK\\x03\\x04 | FileZIPAnalyser (id 0x48) | FMT_ZIP (4B) + FMT_ZIP_CD | MATCH |
| **0x435057ff** | **FF WPC** | **FileWPDAnalyser (id 0x44)** | **FMT_WPD (FF 57 50 43)** | **MATCH — census doc's "WordPerfect (WPG/doc)" refined: the \\xFF WPC key is WPD proper** |
| **0x90001** | **01 00 09 00** | **FileWMFAnalyser (id 0x46)** | **FMT_WMF (01 00 09 00 = standard WMF hdr)** | **MATCH — census doc's "Windows thumbnail cache" label WRONG: 01 00 09 00 is the std WMF header (type=1, headersize=9); doc-side erratum** |
| **0x9ac6cdd7** | **D7 CD C6 9A** | **FileWMFAnalyser (id 0x46)** | **FMT_WMF_PL (placeable)** | **MATCH — both WMF keys bind the same analyser, as the core rows do** |
| **0x100** | **00 01 00 00** | **FileTTFAnalyser (id 0x41)** | **FMT_TTF (00 01 00 00 = sfntVersion)** | **MATCH — census doc's "ICO/CUR (01/02 + count)" conflated: 0x100 is the TTF sfnt key; ICO is 0x10000** |
| **0x10000 / 0x20000** | **00 00 01 00 / 00 00 02 00** | **FileICOAnalyser (id 0x22) ×2** | **FMT_ICO / FMT_CUR (both validate_ico)** | **MATCH — the og binds both ICO and CUR to the same analyser, exactly the core's two rows** |
| 0x46464952 | RIFF | Brance::RIFFAnalyser→FileRIFF (id 0x3b) | FMT_WAV/AVI family dispatch | MATCH |
| **0x4e444221** | **!BDN** | **FilePSTAnalyser (id 0x38)** | **FMT_PST ('!','B','D','N')** | **MATCH — census doc's "Bentley/BDN document" label WRONG: !BDN is the PST magic; doc-side erratum** |
| 0xe011cfd0 | D0 CF 11 E0 | FileOLEAnalyser (id 0x30) | FMT_OLE | MATCH |
| 0x2da5db / 0x2ea694 | DB A5 2D 00 / 94 A6 2E 00 | FileLowDocAnalyser (id 0x17) ×2 | FMT_LOWDOC/LOWDOC2 | MATCH |
| 0x35f3f | 3F 5F 03 00 | FileHLPAnalyser (id 0x20?) | FMT_HLP | MATCH |
| 0x4d524f46 | FORM | CFileAIFAnalyzer (id 0x50) | FMT_AIFF | MATCH |
| 0x37545043/38/39 | CPT7/8/9 | RawSpeed::CPTAnalyser (id 0x33?) | FMT_CPT7/8/9 | MATCH |
| 0x4643534d / 0x6663736d | MSCF / mscf | FileCABAnalyser (id 0xc) ×2 | FMT_CAB / FMT_CAB_LC | MATCH |
| 0x646e732e | .snd | FileAUAnalyser (id 9) | FMT_AU | MATCH |
| 0xa3df451a | 1A 45 DF A3 | FileMKVAnalyser (id 0x60) | FMT_MKV | MATCH |
| 0x46535449 | ITSF | FileCHMAnalyser (id 0xe) | FMT_CHM | MATCH |
| 0x4d524d00 | \\0MRM | FileMRWAnalyser (id 0x66) | FMT_MRW (M-081 1-row) | MATCH |

## The raw_signs.json min/max ×1024 question — RESOLVED (core CORRECT)

The corpus `raw_signs.json` stores min/max in KiB (e.g. itc2: min 1, max 51200). The
core's generated table (`cr_rawsigns_data.cpp`, gen_rawsigns.py) carries min/max ×1024
(all 139 formats). **First-hand decode of the og's scan-time consumption proves the
×1024 is the og's own behavior** — `FUN_1800205b0` (the layer-1 sign walker):
- `:25020+159` — `if (uVar24 < (ulonglong)(**(longlong **)(param_1 + 0x118) << 10))` — the max compare is `max << 10`
- `:25020+221` — `*(longlong *)(param_1 + 0x10) = lVar19 << 10;` — the stored size is `min << 10`
- `:25020+174` — `(ulonglong)(lVar17 * 0x400)` — the zero-max default compares `0x7d000 * 0x400` (500 MB)
- `:25020+177` — the absent-max default `lVar17 = 0x7d000` (512,000 KiB)

**Verdict: MATCH — the core's KiB→byte conversion at generation time reproduces the og's
scan-time `<<10` exactly, and the 0x7d000 zero-max default is documented in the core's
`cr_rawsigns.h:50-52`. No divergence. The near-finding was disproven by first-hand decode.**

## Census-doc errata candidates (corpus-side, read-only — recorded, not filed as code findings)

1. `0xd302020` "MPEG audio frame sync variant" → actually the **DXF header key "  0\\r"** (BruteForce::CDXFAnalyser, id 0x19). The MPEG audio sync keys are the MPGA_F*/MPGA_E* family (separate inserts).
2. `0x90001` "Windows thumbnail cache" → actually the **standard WMF header** (01 00 09 00: type=1, hsize=9) binding FileWMFAnalyser.
3. `0x4e444221` "Bentley/BDN document" → actually **PST** ('!BDN' magic, FilePSTAnalyser id 0x38).
4. `0x100` conflated with ICO/CUR → **0x100 is the TTF sfnt key**; ICO = 0x10000, CUR = 0x20000.
5. `0x694c5153` "SQLite → DB chain" → the bound analyser is BruteForce::CFCPEvtAnalyser (id 0x73) — the FCP-event SQLite chain the core already implements (N-089).

These are CORPUS-ERRATA rows (the census doc is the corpus's own artifact; the core's
code matches the binary, not the doc's labels).
