# S8 evidence — the og's carve batch-2 format grammars (decode sheet)

Session-8 wave-2 decode (parallel agents V-A1/V-A2, grep-verified region
reads) of `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
(corpus `be0cfe9`). All `:NNNNN` cites are line numbers in that file.
Core-absence verified against `src/core/carve/` (FMT_ enum list,
cr_registry.cpp, cr_analysers.cpp, cr_rawsigns_data.cpp) this session —
every format below is ABSENT from the core's 100-format registry.

## Registry architecture (corrected)

Three dispatch layers, one registry: u32 magic map (59 bindings,
:1728-2509, `FUN_180006c80` = map::operator[]), u16 multimap (35
bindings, :2536-2911, find-or-create :18000ac20 + chain-append
:18000ac70), and a `std::list` layer (:2919-4214) with an UNCONDITIONAL
zone (~2930-3350) and a CONFIG-GATED zone (3356-4110: each analyser
registers only if `FUN_180010f50(config, ext)` finds its extension
enabled in RawConfig.ini — 18 gated sites). List-layer analysers are
self-probing and bind their display extension at runtime through
`FUN_18000ed00` (the per-scan carve-statistics map insert).

## The keyed six (V-A1)

| Format | Registration | Grammar (each gate cited) | Size-walk |
|---|---|---|---|
| CHM | u32 `ITSF` 0x46535449, chain :2488-2498, FileCHMAnalyser ID 0x0e | u32@0=='ITSF' AND u32@+8==0x60 AND u64@+0x18==0x11D07BAA7C01FD10 AND u64@+0x20==0xECE622C9A0000C9E (GUID1 {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC}) AND u64@+0x28/+0x30 == GUID2 {7C01FD11-…} AND u32@+0x60==0x1FE AND u32@+0x70==0 AND u32@+0x74==0 (:17727-17732) | size = u64@+0x68 (:17734) — no walk |
| MNY | u16 `MS` 0x534D :2909-2911, FileMNYAnalyser ID 0x28 | memcmp(+4, "MSISAM Database", 15) (:49968-72) | fixed 0x100000 (1 MiB) (:49978) |
| EMF | LIST-only (no key — confirmed; vftable 3×: :3194-3213 append, dtor :14190) ID 0x1a | u32@+0x28==' EMF' AND u16@+0x3A==0 AND u32@+0x30!=0 (:14207-08) | size = u32@+0x30 (nBytes) (:14210) |
| ONE | u32 0x7B5C52E4 :2175-77, RawSpeed::OneAnalyser + FileONEAnalyser fallback, ID 0x31 | u64@+4==0x7853B1AE4DA7D88C AND u32@+0xC==0xD39629D0 (:17177-78, legacy :52303) — full GUID {7B5C52E4-D88C-4DA7-AEB1-5378D02996D3} | cluster/chunk walker :52316-52358: per-chunk GUID re-check, chunk size u64@+0xC4 (fallback u64@+0x64), 0<size≤2×device; ONERawFileInfo max 500,000,000 (:17203) |
| MPEG-TS | LIST-only brute: CMTSAnalyser ID 0x76 (:3134-54), CM2TSAnalyser ID 0x74 (:3164-84) — NO magic key | prober :35578-35669: byte@+4==0x47→192-byte packets, byte@+0==0x47→188-byte; second byte ∈{0x40,0x41,0x50,0x10}; validator :38456-38619: 2nd-packet sync (byte@+0xC4 or +0xBC), u64@+8 and +0xC8/+0xC4 not all-FF; needs >0x1FF bytes (:35606) | packet walk :38457-38536 (188/192 stride, 32-packet batches), reject <0x5000 (20 KiB) :38525-27, cap = device×10 :38516-21, stamps L"M2TS" :38530-33 |
| 3DS | u16 `MM` 0x4D4D :2885-88, RawSpeed::New3DSAnalyser + File3DSAnalyser (Autodesk .3ds), ID 1 | (u16@0==0x4D4D AND u16@+0x10==0x3D3D) OR (u32@+0x100==DAT-const AND u32@+0x104==0x200000) (:14535-45, legacy :50906-16) | chunk walk :50921-71 (stride u16@+2, 0x58 shortcut when u16@(chunk+0x58)==0x4D4D), cap device×5, max 200,000,000 (:14559) |

Census corrections from this decode: `E4 52 5C 7B` is ONE (census says
"LZH arc"); `0x11C` (284) is FreeHand-11 CFH11Analyser ID 0x1d
(:1960-62, gates :32612-34) — NOT MPEG-TS; "New3DS" is Autodesk 3D
Studio, not Nintendo; CHM's GUID compares live at :17728-31 (not
:22635 — that region is std::map rotation code).

## The list-layer and text school (V-A2)

| Format | Registration | Grammar | Size-walk |
|---|---|---|---|
| SWF | list, unconditional :3067, idx 0x3e, FileSWFAnalyser | u16@0=='FW'/'CW' + byte@2=='S' (:29975-76); FWS: version byte@3 ∈[3,20] AND FileLength u32@4>8 AND RECT bit-unpack@8 (:30029-58) | size = u32@+4 (:29991) |
| JP2 | list, config-gated ext "jp2" :3552, idx 0x80, RawFormat_xjq::JP2Analyser | BE u32@4=='jP  ' AND BE u32@8==0x0d0a870a AND BE u32@0x10=='ftyp' (:21875); brand map jp2→0x80 / jpx→0x81 / jpm→0x82 (:21876-21909) | FileInfo ctor :21941 |
| MOBI | list, config-gated ext "mobi" :3608, idx 0x85 | BE dword@0x38=='BOOK' AND BE dword@0x3C=='MOBI' (:29394-400) | EndTag strategy: tag 0xE98E0D0A, max 0x1f400000 (500 MB), step 0x200 (:29454-71) |
| LZH | list, unconditional :3097/:3110 (dual-gen), idx 0x24 | u32@+2 ∈ {-lh5,-lh0,-lhd,-lh7} AND byte@+6=='-' (:46962-65; legacy :62927-44) | member-header walk :46992+, cap 0xc800000 (200 MB) :46927 |
| HTML | u16 multimap keys `<h`/`<H`/`<!`/BOM FFFE :2849-2876, RawSpeed::HTMLAnalyser idx 0x21 + FileHTMAnalyser | _memicmp "<HTML" 5 OR "<!DOCTYPE HTML" 14 OR UTF-16LE BOM+'<' variants (:14335-71) | max size 0x100000 (1 MiB) :14396 |
| XML/SVG | u32 keys `<?xm`/UTF8-BOM'<'/0x1cd/`<svg` :1811-1835 → Brance::SVGAnalyser idx 0x5b + FileSVGAnalyser | memcmp "<?xml version=" at 0 or +3 (:49642-44), then string-probe router: `<svg` battery→0x5b, `<gpx`-family→0x13 (:49680-702) | shared text tail strategy |
| CRW | u32 keys 0x1a4949 ('II'+0x1A) and 0x1a4d4d ('MM'+0x1A) :1751/:1759 → Brance::CRWAnalyser idx 0x12 + FileCRWAnalyser (0x418-byte state) | "HEAP"@+6, "CC"@+8, `…DR` sub-magic + version char @+0xe (:46148-80) — HEAPCCDR/CIFF | CRWRawFileInfo ctor :46190 |
| VCF | list, config-gated ext "vcf" :3423, idx 0x88 | _memicmp "BEGIN:VCARD" 11 (:45330) | tail scan "END:VCARD" in sector<<7 windows (:45363-99) |
| GPX | list, config-gated ext "gpx" :3984, idx 0x97 | "<?xml version=" 15B probe then u32 '<gpx' scan from +0x16 (:20878-90) | text tail strategy |
| HIK | list, config-gated :3856, idx 0x92 | MPEG-PS: u32@0==0xBA010000, system header 0xBC010000 or private 0xBD010000 (:30429-52) | offset+6+BE-u16 length, <0x400001 (4 MB) (:30435-41) |
| OSP | list, config-gated :3728, idx 0x8f | u64@0==0x22646922200a0d7b + u16@8==0x203a — `{\r\n "di": ` JSON (:50123-25) | chunk walk |
| POS | list, config-gated :3664, idx 0x8b | _memicmp "{\"diagram\":" 11 (:20217) | chunk walk |
| AEP | list, config-gated :4047, idx 0x9c | u32@0=='RIFX' (:11569) — AE project | chunk-tree walk |
| CHFP | list, config-gated :3792, idx 0x90 | "<?xml version=" AND _memicmp(+0x28,"<BiffProject",12) (:19963-66) | text tail strategy |
| FCPEvt | u32 key `SQLi` 0x694c5153 :1972 → BruteForce::CFCPEvtAnalyser idx 0x73 (FCP event = SQLite DB) | SQLite magic + deep-walk memcmp "tableactiveactive" 17 → relabels subtype 0x75 (:35461-64), cap 200,000,000 (:35507) | core has the N-089 relabel only — no standalone 0x73/0x75 format |

Shared text-tail mechanism `FUN_18000f0d0` (:11603): forward scan in
sector<<7 windows for a per-format tail marker; last marker = carved end.

Core coverage verdicts: ALL of the above ABSENT except FCPEvt (partial —
the SQLite relabel, cr_analysers.cpp:1202-48) and OfficeTagFromXML
(functionally covered by zip_common's [Content_Types].xml branch,
cr_analysers.cpp:827-54). The core has no config-gated registration
zone at all (no RawConfig extension-toggle equivalent for carve rows).
