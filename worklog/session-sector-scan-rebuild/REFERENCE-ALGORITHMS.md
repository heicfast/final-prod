# REFERENCE-ALGORITHMS — Detection-Algorithm Ground Truth (Sector-Scan Rebuild, Agent C)

> READ-ONLY survey of the reference corpus (`easeus-drw-research`) covering the DETECTION
> ALGORITHMS: signature tables, analyser families, grammar-aware validation, metadata
> parsers, recovery-chance model, classification rules, and the phantom-record gates.
> Every claim carries an artifact + section/line citation, or a decompile `file:line`
> cite (decompile root: `re-workspace/reversed/ghidra-searchdll/`). CORRECTIONS.md rows
> supersede their original artifacts; every affected claim below is marked
> **[CORRECTIONS row N]** and states the corrected reading. Key decompile spot-checks
> performed this session are marked **[verified this session]**.

---

## 1. Signature tables

### 1.1 The dual registry (u32 fourcc keys + u16 magic keys)

RAWSearchDLL builds ONE registry structure populated with TWO key families
(`raw-dual-registry--u16-magic-keys.md` §1; corpus `RAWSearchDLL_decompiled.c`
lines 2517-2918, registry builder FUN_180004320):

1. **u32 4CC keys** — the first 4 bytes of a candidate read as a little-endian DWORD
   (`rawsearchdll--analyser-registry-census.md` §3: 70 unique keys, byte-decoded).
   Representative verified keys (census §3 table; carving deep-dive §2 verification
   ledger at lines 1578/1595/2053/2131/16128/41230):
   - `FF D8 FF E0/E1/DB/EE` → JPEG (4 magics) — `searchdll-family--architecture.md` §4.2
   - `%PDF` (0x46445025) → `BruteForce::PdfAnalyzer`
   - `II*`/`MM*`/`IIU`/`IIRO` → TIFF family (incl. Olympus ORF variants `IIRO`/`IIRS`/`MMOR`, Fujifilm `FUJI`, Minolta MRW both byte orders)
   - `06 0E 2B 34` (0x342b0e06) → **MXF** (SMPTE KLVP) → `Brance::MXFAnalyser` — **[verified this session: registry insert at RAWSearchDLL_decompiled.c:1910-1913]**
   - `30 26 B2 75` (0x75b22630) → **ASF/WMA/WMV** → `BruteForce::CAsfAnalyser` — **[verified this session: :1922-1931, analyser ID 6]**
   - `SQLi` (0x694c5153) → **SQLite** (DB chain) — **[verified this session: :1972-1980 (FCPEvt cluster adjacent)]**
   - `AC10` (0x30314341) → **AutoCAD DWG** → `BruteForce::CDWGAnalyser` — **[verified this session: :1997, follow-on CAsf-style block; DWG analyser block at :2000+ with 0xc6d3d0c5 dBase preceding]**
   - `BZh1` (0x31685a42) → bzip2 → `Brance::DMGAnalyser → FileDMGAnalyser` (DMG payload; census §3, behavioural note §6.1)
   - `FOVb` → X3F; `OggS`, `.RMF`, `MAC `, `C5 D0 D3 C6` (dBase), `GIF8`, `MThd`, `.PNG`, `8BPS`, `{rtf`, `Pain` (PSP), `Rar!`, `Stuf`, `wpg`, `PK\x03\x04`, `\xFF WPC`/`D7 CD C6 9A` (WordPerfect), ICO/CUR, `RIFF`, `D0 CF 11 E0` (OLE), `MSCF`/`mscf` (case-duplicate keys, census §6.3), `.snd`, `1A 45 DF A3` (EBML→MKV), `ITSF` (CHM), `CPT7/8/9` (three-generation PhotoPaint, census §6.4)
2. **u16 first-2-bytes keys** (`raw-dual-registry--u16-magic-keys.md` §2, 39 decoded
   entries): `BM`(0x4D42)→BMP, `MM`(0x4D4D)→TIFF-BE, `%!`(0x2125)→PostScript/EPS,
   `FL`(0x4C46)→FLV, `<h`/`<H`/`<!`→HTML/XML, `1F 8B`(0x8B1F)→gzip, `MS`→MS Money,
   `ID`→AIFF, MPEG sync families 0xFFF1..0xFFF/0xFFE2..0xFFE7, `00 01` structure-code,
   and `0x0000` default/fallback chain head. Dispatch tries the u16 key first (cheap),
   then the 4CC table, then the signature DB — order [INFERRED per artifact §1].

**Registry mechanics** (dual-registry §4; census §2): find-or-create node
(FUN_18000ac20 / map `operator[]` FUN_180006c80) + append analyser to the node's chain
(FUN_18000ac70). The registry is a `std::map<uint32, Analyser*>` **red-black tree**:
RB descent by key comparison, `_Tree::_Insert` = FUN_18000acf0, rebalance/rotate =
FUN_18000ad50 (`rawsearch-carving--deep-dive.md` §2, construction decoded at
RAWSearchDLL_decompiled.c:1560-1700). Three export projections of one singleton:
`EUGetAnalyserList` (all), `EUGetAnalyserMap` (first-match), `EUGetAnalyserMultimap`
(all matches for signature collisions, e.g. 4 JPG magics, TIFF `II*.` claimed by TIFF +
camera-RAW chains) — census §1; carving deep-dive §1 (72170-72205).

**Analyser object construction** (census §2): 0x18-byte chain node; two-step vtable
pattern (base `IRawFileAnalyser` vftable first, then concrete class vftable — the
inlined base-then-derived C++ ctor); numeric format-ID at +4 (0xFFFFFFFF unassigned
sentinel until the concrete ID lands). **Two-generation fallback chains**: every
format has a modern namespaced analyser plus a legacy `File*Analyser` fallback;
when the modern analyser rejects a candidate the chain falls to the legacy impl —
"two independent validators must both fail before a match is discarded" (census §2).

### 1.2 RTTI census

182 RTTI types (`.?AV` strings), 106 instantiated analyser classes, 488 vftable
references, 6 DLL exports (`rawsearchdll--analyser-registry-census.md` header + §4).
Class families: `BruteForce::` (24 classes/20 analysers — frame-sync scanners),
`RawSpeed::` (20 — index-first archive/document school; namespace borrowed from the
open-source RawSpeed camera library, EaseUS implementations in its naming style,
census §4), `@RawFormat_xjq` (8 — developer initials; HEIC/JP2/MOBI/M1V/CommonRaw),
`@Brance` (22 — developer handle; media/Apple ecosystem), first-generation `File*`
(46 — dual role primary + fallback validators), standalone/un-namespaced
(AEP/CHFP/CHIK/COSP/CPOS/CGPX/CVCF/C7ZIP/AAC/AIF/Ogg), plus infrastructure
(IRawFileAnalyser, ISpecialRawFileAnalyser — extended validation contract used by
DXF/ZIP/Dbx chains, CRawFileInfo, CZeroEndRawFileInfo for NUL-terminated names,
CRAWSearchController, CNeedRawMgr, CRawFileCellTestStorageDevice, tinyxml2,
neb::CJsonObject) — census §4.

### 1.3 raw_signs.json (the external signature DB)

139 extensions; per-extension `{"header": {"signs": ["offset,HEXBYTES,confidence"]]},
"tail": {...}, "min": bytes, "max": bytes}`; 139/139 have header signs, 46 have tail
signs (footer detection → exact file end); confidence 0.50-1.00 per signature;
header+tail combined scoring ranks overlapping carves; min/max size bounds "prune
absurd candidates" (`config-files/config-data-reference.md` §"raw_signs.json";
example `ifo` header `0,44564456,0.96`). Loaded at engine init via
`std::ifstream("raw_signs.json")` (RAWSearchDLL_decompiled.c:23836 —
**[verified this session: `FUN_1800098d0(local_2e8,"raw_signs.json")` at :23836]**),
parsed by a **hand-rolled istream JSON tokenizer** (FUN_180021460/21380) — NOT
tinyxml2, NOT CJsonObject for this file (carving deep-dive §3, correction note vs
family doc §4.1 folded inline there). The JSON covers formats WITHOUT dedicated C++
analysers (`itc2`, `prproj`, `nes`, `rpgmvo`, `apk`, `blp`, `resw`...); the engine
consults the magic-key registry first, then JSON signs for unclaimed extensions
(census §5).

**[CORRECTIONS row 9]**: EUUserData is NOT a RAW-carving input. The carving
deep-dive §3's "EUUserData magic/MIME DB — secondary typing" and the family doc
§4.3's listing of it as signature source #2 are superseded: `EUUserData` never
reaches the RAW carving engine (zero references across 80,614 lines —
**[verified this session: grep count 0]**). Its sole consumer is DRWUI's
`CHKRepairFileSign` repair chain. `GetRawUserInfoMap` (export at
RAWSearchDLL_decompiled.c:72213 — **[verified this session]**) returns the per-scan
carve-STATISTICS map (insert-or-increment from ~70 detection paths, FUN_18000ed00
:11376-11525; cleared at searcher init FUN_18000beb0 :9044-9051; read post-scan by
DRWUI's Trigger_Raw/FileSystem report builder) — scan OUTPUT, never scan input.

**Per-format carve gating**: `config/RawConfig.ini` `[Filter]` per-extension on/off
(carving deep-dive §4: loaded at :13133). **[CORRECTIONS row 5]**: RawConfig.ini
ships **34 toggles** (not 37). Enabled examples: HEIC, WEBM, WEBP, M1V, M2T, M2TS,
JP2, M4P, XMIND, RVT, MOBI, MMAP, POS, EDDX, WFP, OSP, HFP, DRP, HIK, ODP, ODT;
disabled: STL, VPJ (`filtering-priority--config-layer.md` §4). The toggles map 1:1
onto the standalone analyser classes (CHFP/CHIK/COSP/CPOS/CVCF/AEP — the
video-editor-project-file niche).

### 1.4 The ForCellTest harness

`EUCreateRAWSearcherForCellTest` constructs the controller statically
(`_Init_thread_header/footer` once-init + atexit teardown at :72142-72165) and
FUN_18000c650 binds `CRawFileCellTestStorageDevice` (112 vftable refs) — an in-house
unit-test harness feeding synthetic sector images to carvers; the shipping product
carries its engine's test scaffolding (carving deep-dive §1; census §4).

---

## 2. Analyser families

Family artifacts: `rawsearchdll--family-{brance,bruteforce,filegen,rawspeed,xjq,special}.md`
(roster + vftable-ref counts). All cite the registry census for magic bindings and
share the two-generation fallback-chain design (each family artifact "Notable decode
facts"; census §2).

| Family | Classes | Formats | Carving school / validation |
|---|---|---|---|
| `BruteForce::` | 23 (20 analysers + FileInfo pairs) | ADTS, APE, ASF, DWG, DXF, EPS, FCPEvt, FH11, GIF, M2TS, MTS, JPEG, MP3, MOV, PDF, TIFF | Frame-sync scanners for formats without internal indexes: "signature+structure walking without a directory index"; walk forward validating structure at every step, report last valid position as file size — "maximize salvage" trade-off: more false-positive tail, no premature truncation (family-bruteforce §description; carving deep-dive §5) |
| `RawSpeed::` | 15 | TAR, RAR, SIT, GZ, CPT, HTML, RTF, PNG, PSP, OneNote, MDB, Dbx, New3DS, Wpg, MID | **Index-first carving school**: wherever a format has an internal index (RAR/TAR/SIT/CPT/CHM/PST), walk the index to reconstruct exact sizes (family-rawspeed §description; zip-analyser §5.1) |
| `Brance::` | 12 | CRW, DMG, FLV, LZH, MPG, MXF, OGG, RIFF, RM_RMVB, SVG, X3F, OfficeTagFromXML | Media/Apple-ecosystem set by developer "Brance" (family-brance) |
| `RawFormat_xjq::` | 5 | CommonRaw, HEIC, JP2, M1V, MOBI | Newest-generation formats by developer "xjq" (family-xjq) |
| `File*` (first gen) | 47 | FileJPG…FileZIP | Dual role: primary analysers AND fallback-chain validators for the modern generation (family-filegen) |
| Standalone/special | 29 | AAC/AIF/Ogg + AEP, C7ZIP, GPX, VCF, HFP, HIK, OSP, POS + infrastructure (IRaw/ISpecial/CRaw/CJsonObject/MemPool/tinyxml2) | Un-namespaced modern wave + engine infrastructure; `ISpecialRawFileAnalyser` = extended validation contract (family-special; census §4) |

### 2.1 Granular validators (batch 1) — `analyser-granular-batch1--pst-rar-mkv-heic.md`

- **FilePSTAnalyser** (validator FUN_180041eb0, §1): format byte at header+0x0A —
  0x0E ANSI → size u32@+0xA8; 0x17 UNICODE → size u32@+0xB8; other → floor 1 MiB
  (0x100000); accept type-ID 0x38. **[verified this session: FUN_180041eb0 at
  RAWSearchDLL_decompiled.c:50000-50017 reads exactly `param_2+10` == 0x0e/0x17,
  sizes at +0xa8/+0xb8, default 0x100000, ID 0x38]**
- **RawSpeed::RARAnalyser** (validator FUN_180012bb0, §2): `Rar!` (0x21726152) +
  u16@+4 == 0x071A + buf[6]==0x00 → RAR4, type-ID 0x3A. **[verified this session:
  :14726-14733 — local_18=0x21726152, local_14=0x71a, buf[6]=='\0', ID 0x3a at
  param_3+0x18]**. RARRawFileInfo (0x128 alloc): version bounds [+0x1B]=7/[+0x22]=7,
  flags [+0x23]=0x40000000; walk FUN_180012d00 carries a 272-byte filename buffer
  (entry names for the results tree). RAR5 rides the 4CC/fallback tables
  [INFERRED per artifact §7].
- **FileMKVAnalyser** (FUN_180016c70, §3): full EBML variable-length-integer
  decoder — count leading zero bits from mask 0x80 (max 8 bytes), strip marker bit,
  shift in payload; element bounds `n + value + 4 ≤ 0x200`; DocType discrimination
  via 0x6D626577 ("webm", **[verified this session: :17796]**) and 0x67805318.
  "The only analyser in the set with a true variable-length-integer grammar" —
  element sizes are self-describing, so MKV/WebM carving survives fragmentation.
- **RawFormat_xjq::HEICAnalyser** (validator ~21278, §4): brand u32 == 0x68656963
  ("heic") → accept type-ID 0x7C label L"HEIC"; HEIF-family brands → label L"HEIF"
  (brand slot rewritten for display). **[verified this session: :21275
  `if (uVar7 == 0x68656963)`]**. 0x7C is the newest ID in the table.

Type-ID rollup (batch1 §5 + dual-registry §3): 0x0B BMP, 0x19 DXF, 0x1B EPS, 0x1F
gzip, 0x27 MS Access, 0x28 MS Money, 0x2A QuickTime, 0x2D AAC ADTS, 0x38 PST,
0x3A RAR4, 0x5A FLV, 0x7C HEIC. Registry inserts also carry their own IDs
(**[verified this session: CAsfAnalyser ID 6 at :1930, CFCPEvt 0x73 at :1946,
CEPS 0x1b at :1965, DWG-family 0x18 near :1998]**).

### 2.2 The ZIP analyser — `rawsearchdll--zip-analyser-carving.md`

FUN_1800176e0 (main), FUN_1800176b0 (probe), FUN_180045690 (per-entry parser); lines
18290-18560. **[verified this session: entry dispatch at :18383-18390 — CD 0x2014b50 /
  LFH 0x4034b50 (registry key, census §3; insert at :2246) / EOCD 0x6054b50, anything
  else → `param_2[2] = NULL` reject]**

1. **Three PK anchors accepted** (§1): LFH `PK\x03\x04` with sanity checks
   `version_needed ≤ 10` (+8 ushort) and extradata/flags at +0x1a in [1, 0x102];
   CD entry `PK\x01\x02` (preferred anchor — index-based walk); EOCD `PK\x05\x06`
   (truncated/EOCD-first zips). The registry binds the chain to LFH only, but the
   analyser handles all three once invoked.
2. **CD walk / per-entry inventory** (§2): loop over structures; per-entry parser
   FUN_180045690 reads the fixed header (name len +0x1c, extra +0x1e, comment +0x20
   for CD; compressed/uncompressed sizes, local-header offset) and accumulates exact
   total size — "the final carve size is computed from the SUM of parsed entries,
   not guessed". Entry names accumulate into vectors (FUN_180007060 merge) → complete
   inner-file inventory returned to the results tree. **[verified this session:
   loop re-dispatch at :18425-18430; FUN_180045690 call sites :18411/:18446]**
3. **Sector-aligned zero-padding extension** (§3): after the walk breaks, align DOWN
   to sector boundary; if the last 4 bytes of the sector are non-zero, forward-scan
   through zero padding while each sector's trailing 16 bytes (4 dwords at s-4/-8/-12/-16)
   are all zero, capped at 0x500001 (5,242,881 ≈ 5.24 MB). **[verified this session:
   :18488-18502 — `uVar15 < 0x500001`, dword checks at puVar9+(uVar16-4/-8/-0xc/-0x10)]**
   Produces clean file boundaries on deleted ZIPs followed by slack-space zeros.
4. **Result contract** (§4): `param_2[2]` = carve end (aligned+extended), NULL on
   total rejection; size slot; entry list via FUN_1800173f0; indexless (LFH-only)
   result carries status 0xd vs 0x48 for full-CD carve — UI presents
   "recoverable but structure incomplete" differently.
5. **Generalizable patterns** (§5): index-first carving for indexed formats;
   brute-force fallback for indexless; **sector-awareness** (analysers query the
   storage device's real sector size via vtable slot 5 — correct on 4Kn disks);
   **dual validation** (registry magic + internal re-validation, e.g. WPG's
   0x677077 re-check inside the FileInfo ctor which also stamps canonical size 0x45
   — census §6.2).

---

## 3. Grammar-aware validation

### 3.1 Carving-side grammars — `rawsearch-carving--deep-dive.md`

- **PDF grammar** (§3, verification ledger): `(*param_2 == 0x46445025) &&
  (param_2[1]=='-')` then `%PDF-x.y` version parse (:41230-41233).
- **JPEG grammar** (§3, ledger): marker walk incl. SOF0 `bVar1==0xc0 &&
  pbVar25[4]==8` (0xC0 with 8-bit precision, :39653); census §3 notes the 4 JPEG
  magics dispatched via std::map.
- **Hardcoded analysers compute the file END by walking structure** — "the
  difference between a 'signature hit' and a *recoverable file with size*" (§3):
  ZIP central directory, PST node b-tree, etc.
- **Confidence combination** (§3, family doc §4.4): header+tail+size-gate
  confidence ranks overlapping candidates (raw_signs.json confidence fields).

### 3.2 Repair-side format grammars — `FileRepair/image-repair--format-grammar.md`

These are the validation grammars per format (the repair engine, but they define
what "structurally valid" means for each image format — the same corruption-first
philosophy as the carving engines, artifact §12.5.1):

- **JPEG** (§4): `JPEG::CJpegInfo` ctor seeds a minimal valid skeleton
  `obj[0x1b]=0xFFD8FF01` (SOI + JPG0 marker) and `obj[0x1c]=0xD9` (EOI) — the
  FFD8/FF01/FFD9 marker state machine template; marker-level parsing reuses the
  LibRaw LJPEG reader family (SOF0/1/2/3, DHT with make_decoder_ref, SOS
  per-component, DQT 64-entry, DRI) with a **0x400 marker-count sanity bound** and
  **0x7FFF length clamps** (§5 `LibRaw::ljpeg_start` 0x271c10: 0xFFC0/C1/C2/C5-CF
  state machine, max 0x400 segments, SOF length==9 resync hook).
- **PNG** (§12.2): `CPngInfo` embeds the 8-byte signature twice, a 13-chunk known
  table (IHDR, PLTE, IDAT, sRGB, fcTL, acTL, fdAT — APNG first-class) plus the
  terminator pair "IEND" + its **canonical CRC 0xAE426082** — "the driver searches
  for the literal end-of-PNG 12 bytes, not a parsed IEND". `ScanCriticalChunks`
  (FUN_00526ac0) = corruption-tolerant parallel byte-pattern search (5-9
  simultaneous scans); `BasisRepair` phase pipeline (progress 20/30/50/80) with
  IHDR validation (width/height nonzero, bit-depth/color-type/interlace combos);
  `SampleRepair` salvage path requires `m_idatFlagsOffsetList` non-empty and
  synthesizes a header around the salvaged IDAT stream.
- **WebP** (§12.4): validation = **full decode** — `ValidateWebP` calls the
  statically-linked libwebp `WebPDecode`; "a WebP file is 'valid' iff the actual
  decoder accepts it".
- **HEIC** (§12.3): box-walk engine reads {u32 size, u32 type}; `meta` skip 0xC
  recurse, `moov` skip 8, bound-checked against region start/end; scan patterns
  "hdlr"/"pict"/"infe"; ipco/ipma property dispatch; "every walk is range-checked
  against the mapped region — corrupted sizes cannot walk off the buffer".
- **JXL** (§6): `IsCompletionFile` event loop (event 0x40 basic-info, event 1
  error); `IsCompletionBoxInfo` validates the ISOBMFF box chain.
- **IImageInfo 5-slot virtual contract** (§12.1), identical across all 29 classes:
  +0x00 destructor, +0x04 `AnalysisData` (async analysis+repair entry),
  +0x08 **CheckMagic(buf,len)** (format signature validation, e.g. PNG
  `89 50 4E 47 0D 0A 1A 0A`), +0x0C `CreateInstance()`, +0x10 `SaveData()`.
- **Cross-format finding** (§12.5.1): "Scan-then-validate, never trust structure" —
  locate elements by content search, then hand-validate; the same school as the
  RAWSearchDLL RB-tree signature registry.

**VALID vs phantom, per format**: a detection is VALID when (a) the magic matches a
registered key, (b) the format grammar walk succeeds (structural walk success —
PDF version parse, JPEG SOF dimension check with 8-bit precision, EBML vint bounds,
ZIP CD walk with sum-of-entries size, WebP decoder acceptance), (c) terminators are
present where the format defines them (JPEG EOI FFD9 in the repair skeleton; PNG
IEND+CRC literal search; raw_signs.json tail signs for 46 extensions), and (d) size
sanity holds (raw_signs min/max bounds; PST 1 MiB floor for unknown versions;
element-bounds caps like EBML ≤0x200, JPEG 0x400 marker bound / 0x7FFF length
clamps). A signature hit WITHOUT a computable structural end is at best a
brute-force candidate whose size is "last valid grammar position" (carving
deep-dive §5), and indexless ZIPs are explicitly downgraded to status 0xd
(zip-analyser §3).

---

## 4. Metadata parsers + name/path recovery

### 4.1 NTFS — `ntfs-engine--deep-dive.md`, `ntfs-engine-algorithms-v2--deleted-viability.md`, `ntfs-engine-internals.md`

**MFT bootstrap** (deep-dive §2): boot sector → DBR values; $MFT located via the
0x20 ($DATA) attribute of record 0; 64 KB read batches; fixup validation
FUN_180041bb0 at every record boundary; header sanity FUN_1800411d0 (magic
0x454C4946 "FILE"); **$-skip / attribute-list fork**: FUN_180041850 probes for
$ATTRIBUTE_LIST (0x20) — present → fragmented-MFT handler FUN_18001a540 (follows
the attribute list to reassemble $DATA runs across records, log string "MFT file
record has 0x20 attribute..."); absent → FUN_180041cf0 parses the runlist.
**$MFTMirr** cross-reference (first 4 records' backup at volume mid-point) as a
repair/verification source when primary records fail fixup (deep-dive §5;
sites 52032/52102).

**MFT record walk** (deep-dive §3; v2 §1 [CODE] FUN_18001acc0): record size from
$MFT clamped ≥ 0x400 (1024 bytes); per record:
1. magic == "FILE" (:21152 **[verified this session]**; BAAD-family skipped,
   'FILE0' variant accepted — internals §MFT acquisition, :34153-34161);
2. fixup valid (FUN_180041bb0; wrapper FUN_180041cf0 has copy-on-wrap semantics so
   fixup application cannot corrupt the shared scan buffer — v2 §2);
3. secondary corruption check FUN_180034b20 == 0;
4. **[CORRECTIONS row 2]**: FUN_180041970 is the **BASE-RECORD test**
   (`base_record != 0`, i.e. `*(longlong*)(param_1+0x20) != 0` — **[verified this
   session: FUN_180041970 body is exactly `return *(longlong *)(param_1 + 0x20) != 0;`]**),
   NOT the in-use bit test. Extension-record skip logic keys on it. In the deleted
   batch loop the test is applied as "if FUN_180041970(rec) != 0 → skip": records
   with a non-zero base-record field (extension records) are skipped, leaving
   base records for property building.
5. CNTFSFileProperties build (FUN_1800431d0) + validate (FUN_180043030) + resolve
   into tree (FUN_180006db0, 0x200 sector-size assumption).
6. **$-skip system-file filter** (deep-dive §3 step 7; v2 §1): skip when
   `props->flags & 4` (temp/system), `name == NULL`, or `name[0] == '$'`
   ($MFT/$Bitmap/$UsnJrnl metadata) — **[verified this session: :21186-21189
   `(uVar7 & 4) || pcVar15==NULL || *pcVar15=='$'` → Release + skip]**.
7. **Deleted branch**: `(props->flags & 0x10) == 0` — the NTFS in-use flag maps to
   props bit 4; NOT-in-use = deleted record (deep-dive §3 line 21186 area — note
   the artifact's line cite for this branch is the flags test region; the
   `(uVar7 & 0x10) == 0` non-folder check at :21191 **[verified this session]**).
   Names are recovered from $FILE_NAME (0x30, CNTFSFileRecordFileNameAttribute —
   names, parent references, timestamps, DOS+Win32 namespace variants); deleted
   records keep names even when parent dirs are gone → reparented into the "lost"
   tree (internals §Record processing).

**Runlist nibble math** (deep-dive §7): `len += (run[i] >> 4) + 1 + (run[i] & 0xF)`
while `run[len] != 0` — high nibble = length-field width, low nibble = offset-field
width (FUN_180003440 :2378-2384). Runlists are kept in hex-text form across
serialization (CNoneResidentNTFSDataRunList ↔ DRWUI hex parse — shared interface).

**Bitmap extent scoring** (deep-dive §4; v2 §1/§3 [CODE]): for each deleted file's
data runs (FUN_1800340b0 iterator), `sector = cluster * sectorsPerCluster`;
`hit = bitmap->Test(sector)` (FUN_18002c2e0: plain bit-array with base-offset
re-basing — `buffer[pos>>3] & (1<<(pos&7))` with bounds guard) OR secondary-map
contains (controller +0x60 virtual check); count intact vs total runs → the
per-file **readable-extent ratio** surfaced as recovery chances.

**Viability gate** — **[CORRECTIONS row 7]**: og is **strict `0x3ff < size`**
(Dec :21208 — **[verified this session: `if (0x3ff < lVar18)` at
NTFSSearchDLL_decompiled.c:21208]**): only files with size > 1023 bytes get the
bitmap viability walk; files of exactly 1023 bytes take the fast path with NO
bitmap check. Any `>=` reading is wrong. When the walk runs: if `total > 0 and
intact == total` → ALL clusters re-allocated → the candidate is DESTROYED
(`props.Release(); continue`) — content overwritten since deletion; partially-intact
ones are kept (offered for recovery; the RAW analyser and preview validation grade
actual content) (v2 §1). The 0x400 minimum also appears in the fixup validator
(`size < 0x400 → false`, v2 §2).

**Quick vs complete** (deep-dive §5; internals): Quick = direct $MFT enumeration
including deleted records (fixup-aware), path reconstruction from parent-reference
chains, CResident/CNoneResidentDataRunList; Complete = full walk + $Bitmap build
(IBuildVolumeBitmap secondary vftable at controller +0x37/+0x38 — multiply
inherited) + $UsnJrnl cross-check (strcmp sites 9070/19761/19868/20282) + orphaned
record sweep + attribute-stream assembly. **fastsearch default ON**:
`GetPrivateProfileIntW(L"search", L"fastsearch", 1, …)` at :7163 — **[verified this
session]**.

**[CORRECTIONS row 1]**: the "resident $DATA cap 0xFFF" claim is VOID — the
constant belongs to an MSVC std::string destructor sequence, not the attribute
parser. There is NO 0xFFF resident gate in the engine. (The deep-dive §6 note
"0x80-resident data length checks ... clamp to ≤ 0xFFF" is superseded.)

### 4.2 FAT — `fat-engine--deep-dive.md`, `fat-exfat-engine-algorithms-v2.md`, `fat-exfat-engine-internals.md`

- **Table polymorphism** (deep-dive §2; v2 §1): CFAT12Table/CFAT16Table/CFAT32Table
  behind IFATTable; type detection via OEM label `_strnicmp(bootsector+0x36,
  "FAT16   ", 8)` (:38048) and "FAT12"/"FAT16"/"FAT32" strings (9860-9863,
  27904-27907, 34341-34345). **CDoubleFATTable** composes two child tables
  (mirrored FAT #1/#2; corruption in one falls to the other — fallback switch
  [INFERRED per v2 §1]).
- **Chain walking + EOC/BAD** (deep-dive §3): FAT32 termination family
  `0xFFFFFFF8` max, mask-28 EOC family 0x0FFFFFF8-0x0FFFFFFF, mask-24 secondary,
  alternate 0x0FFFFFF0-7 forms (:8035-8060 — **[verified this session: the
  0xffffff8/0x7fffff8/0xffffff0 mask family at FATSearchDLL_decompiled.c:8035-8045]**).
  FAT16 `Next` (FUN_180041c80 :54125-54145 **[verified this session]**):
  bounds check vs cluster_count; u16 slot; `0xFFF7 → 0xFFFFFF7` (canonical BAD);
  `< 0xFFF8 → next`; else `0xFFFFFF8` (canonical EOC) — every flavor normalized to
  a 32-bit space.
- **[CORRECTIONS row 3]**: **FAT12 BAD-cluster handling is RAW, not canonicalized.**
  Og follows FAT12 BAD cluster 4087 (0xFF7) RAW — garbage-tolerant continuation —
  with NO canonicalization at the 12-bit boundary. **[verified this session: the
  CFAT12Table Next (FUN_1800416f0 at FATSearchDLL_decompiled.c:53772-53806) reads
  the 12-bit entry, maps `0xfff → 0xffffffff` (EOC) and `>=0xff8 → 0xffffff8`, but
  passes 0xff7 (BAD) through as the raw value 0xff7 — no 0xffffff7 translation.]**
  (Core deliberately canonicalizes; that is a registered delta, NOT og behavior.)
  Note the v2 §1 table labels FUN_180041c80 "FAT12/16" — the function body is the
  u16-slot reader (FAT16); the FAT12 12-bit reader is FUN_1800416f0.
- **Chain-follow loop** (v2 §2 [CODE], corpus line ~11200): `while cluster <
  0xFFFFFF8`; cluster→sector `(c-2)*SPC + first_data_sector`; whole-run batched
  reads; **visited-range red-black tree** = cycle-breaker so corrupt FAT loops
  cannot hang the scan; "."-self-entry detection anchors directory identity;
  volume-bounds and capacity guards.
- **Deleted entries + LFN** (deep-dive §4): deleted 8.3 entries first byte 0xE5
  (compiled `-0x1b`) at 6 sites (11742…32155) with a 64KB-chain-length sanity
  guard (`< 0x10000` clusters) bounding corrupt chains; LFN 0x40 last-entry flag +
  0x0F attribute checks (11442, 28446, 33978, 44355), fragments reassembled
  checksum-verified against the 8.3 alias. **Name recovery**: LFN reassembly from
  VFAT slots; deleted-entry salvage + chain reconstruction even when parent
  directory entries are gone → entries feed the "lost" tree; chain truncation
  tolerated (partial-file recovery, size = min(entry size, chain walk))
  (internals §Recovery-relevant behaviors).
- **exFAT** (v2 §3; exfat deep-dive): dentries 0x20 bytes; type = low 7 bits,
  **bit 0x80 = InUse** — deleted entries keep type with 0x80 cleared; `(t-X)&0x7F`
  class tests for volume-label triple (0x03/0x83 + 0x81 TX + 0x82 label),
  allocation-bitmap (0x05/0x85), stream-extension (0x40/0xC0: reserved fields
  zero, first-cluster 1 or EOC-family), filename (0x41/0xC1 → UTF-16 fragment
  validation); **backward triple reconstruction** from the stream dentry rebuilding
  {file → stream-ext → name-fragments} chains, deleted chains identically.
  `CQuickSearchAndBuildBitmap` builds the allocation bitmap during quick scan;
  NoFatChain contiguous files resolved purely from dentries (internals §exFAT flow).

### 4.3 ext4 — `ext4-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md`

- Superblock magic 0xEF53 at +0x38 (u16) — **[verified this session:
  EXT4SearchDLL_decompiled.c:3222 `uVar2 = 0xef53`]**; read 0x200 bytes at offset
  0x400 (v2 §2).
- **Superblock validation gate** (v2 §1 FUN_180004710 [CODE]): reject if
  s_inodes_count==0, s_blocks_count_lo==0, s_blocks_per_group==0,
  s_inodes_per_group==0; the two kernel first_data_block rules (1 KiB blocks →
  first_data_block=1; larger → 0; any other combination rejected);
  `block_size = 0x400 << (s_log_block_size & 0x1F)`; 64-bit total blocks;
  groups invariants (0 < groups ≤ total_blocks; GDT byte sanity).
- **[CORRECTIONS row 4]**: the deep-dive §2 claim of backup-superblock positions
  (8193, 32768, …) is an overreach from kernel knowledge — **no such constants
  exist in the EXT4SearchDLL decompile** (verified this session: grep for
  8193/32768 over EXT4SearchDLL_decompiled.c returns nothing). Treat as
  UNVERIFIED for the engine; the engine probes the primary superblock at 0x400.
- **[CORRECTIONS row 6]**: journal salvage gates ONLY on jsb magic 0xC03B3998 +
  blocktype {3,4} — every block in [s_first, s_maxlen) feeds the directory parser;
  NO commit/revoke/descriptor semantics exist engine-side. **[verified this
  session: EXT4SearchDLL_decompiled.c:12207-12215 — byteswapped u32 == 0xc03b3998
  then `(uVar13 == 3) || (uVar13 == 4)`]** (FUN_18000e5b0 + FUN_18000e890,
  :12159-12368).
- **CatchBuffer bad-sector proxy** (deep-dive §3): CEXT4CatchBufferDeviceProxy
  retries failed reads with a shifted window (0xE00 fresh buffer, 0x400 backtrack)
  — skip-and-salvage ("Program is trying to skip bad sectors to find data").
- Structures walked (deep-dive §4): group descriptors (32/64-byte GDT via 64-bit
  feature flag 0x10 → desc_size, v2 §2), inode tables, extents
  (CExt4DataRunList; ext3 indirect-block fallback), linear + htree directories.
  Journal traversal traces "GotJournalDir/TraverseJournalDir" (remaining-engines
  artifact §EXT4). **Name recovery**: directory-entry walk + orphan-file/list
  handling for deleted inodes (remaining-engines §EXT4).

### 4.4 HFS+ — `hfsplus-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md` §3-4

- **Volume-header validation** (deep-dive §1 :10005-10020; v2 §3 [CODE]):
  signature "H+" (0x482B) or "HX" (0x4858, HFSX); block size multiple of 512;
  version < 0x11; allocFile/extentsFile block counts ≤ totalBlocks. Big-endian
  decode first (rotating byte-swap loop).
- **B-tree machinery** (deep-dive §2): CHFSPlusCatalog (Catalog B-tree; cnid
  parent chains reconstruct the folder hierarchy), CHFSPlusExtents (Extents
  Overflow B-tree; >8-extent forks), CHFSPlusAttribute (decmpfs), journal-aware
  raw file reader. Catalog leaf walker (v2 §4 [CODE] FUN_18000c8f0): child node
  placement `base + 1 + record_count + attr_count` (thread-record math), per-node
  cancel/EOF gates, sibling-list recursion.
- **Deleted files**: catalog records are overwritten on delete (unlike NTFS), so
  the deep pass relies on **orphaned-cnid sweeps** + the RAW carving engine
  (deep-dive §2) — the "Advanced Scan" combines engines + RAW. Names/paths from
  catalog thread records where they survive.

### 4.5 ReFS — `refs-engine--deep-dive.md`, `refs-btrfs-zfs-engine-algorithms-v2.md` §1

- `refs::` namespace: RefsMetadataBlockHeaderImpl / RefsMetadataBlockRefrenceImpl
  [sic] / RefsBaseDataImpl(Manager<T>) / RefsCheckPointImpl / RefsDbrImpl /
  RefsDirectoryValuesImpl / RefsFileValuesImpl (refs deep-dive §1; remaining-engines
  §ReFS).
- **MSB chain validation** (v2 §1 [CODE], corpus line ~11670): size ≥ 0x20;
  page_type 1 → "SUPBMSB+ReFSFSRS" (superblock), 2 → "CHKPSUPBMSB+ReFSFSRS"
  (checkpoint), 3 → "MSB+ReFSFSRS" (metadata session block); signature dword must
  match; RefsMetadataBlockRefrenceImpl::parse(page+8, size-0x20); on valid ref,
  accumulate size and copy {valid, block_id, flags +0x2C, size +0x30, next +0x38}
  into the manager. CHKP→SUPB→MSB page chaining.
- Metadata in B+ trees under fixed system identity dirs; 32-byte object IDs →
  directory/parent resolution; CReFSDataRunList / CRefsResidentDataRunList (the
  only per-FS runlist class DRWUI re-materializes by name in .rsf restore,
  FUN_1405f5150). OS-version gating picks the metadata layout (refs deep-dive
  §2-3).

### 4.6 Btrfs — `btrfs-engine--deep-dive.md`, `refs-btrfs-zfs-engine-algorithms-v2.md` §2

- Superblock at 0x10000, magic `_BHRfS_M` (0x4D5F53665248425F at +0x40), mirror
  offsets probed for damaged volumes.
- **Newest-generation selection** (v2 §2 [CODE], corpus line ~10290): for each
  candidate mirror offset: dev-size sufficiency; magic check; sectorsize != 0 and
  power of two; nodesize in [0x86, 0x86+0xFF7B]; nodesize % sectorsize == 0;
  keep the copy with the **highest generation** (+0x48) — the crash-consistency
  rule.
- Chunk tree (logical↔physical mapping via CBtrfsNoResidentDataRunList), root →
  FS trees per subvolume (DRWUI CBtrfsQuickTreeBuilder renders each subvolume as
  a separate root), CoW leaf walking (inode_item, dir_item, dir_index,
  extent_item refs). zlib inflate for compressed extents; crc32c/adler32
  checksums; BtrfsCache.txt metadata cache for re-scans (deep-dive §2-3;
  remaining-engines §Btrfs).
- **[CORRECTIONS row 8]**: og does **NO chunk-tree-uuid validation** (name memcmps
  only); the kernel SB field at +0x60 is log_root-adjacent, not a uuid source.
  The btrfs deep-dive's implied chunk-tree-uuid validation is superseded.

### 4.7 ZFS — `zfs-engine--deep-dive.md`, `refs-btrfs-zfs-engine-algorithms-v2.md` §3

- **Uberblock walk** (v2 §3 [CODE], corpus line ~32300; deep-dive §2): magic
  0x00BAB10C (4 sites: 15049, 32321, 32365, 35244); 128-slot × 1 KiB (0x80-qword
  stride) uberblock rings in vdev labels (4 labels × 128 KB front/back per vdev);
  **txg-gated selection** — newest-transaction-wins (comparison polarity decoded;
  walk direction [INFERRED per v2 §6]).
- Uberblock → MOS → DMU object walk (DSL dir/child maps, ZAP objects) via
  CStreamBase (gang blocks, indirect blocks, compression lzjb/gzip, checksum-aware
  reads). `zfs_struct::` layer (UberData, VdevLoc/VdevDisk, objset, ZplNode(ZFile),
  per-object compression registry `std::map<int64, zio_compress>`). Snapshots
  surface as dataset roots. Pool-level reconstruction survives FS-level damage
  (deep-dive §2-4; remaining-engines §ZFS).

**Shared engine facts**: all 9 engines export the Quick/Complete factory pair
(Btrfs's `EUCreateBtrfsComplateSearcher` typo preserved); DRWUI FS codes 0-2 FAT,
3 NTFS, 4-5 EXT2/3, 7 exFAT, 8 HFS+, 9 EXT4, 10 ReFS, 13 ZFS
(`engine-matrix--factory-contract-verification.md` §1-2). Shared base library
(CFileSystemNode/CFileNode/IFileProperties/IDataRunList compiled per-DLL; NTFS
runlist classes reused by ext4/HFS+/exFAT) — remaining-engines §Common. RAW output
correlates with structured results in the merge phase: **structured wins; carved
attaches as "Other deleted files"** (carving deep-dive §4).

---

## 5. Recovery-chance model — `DRWUI-subsystems--assistant-grading-preview.md`

**RecoveryAssistant** (§1, FUN at DRWUI 330837-330960): post-scan statistics —
TotalFileCount, rawCount (carved), lost-partition exist/delete counts, quick-scan
exist/delete counts, DeepScanFileCount; per-category counts via category IDs:
Graphics=5, Document=6, Archive=7, Video=8, Audio=9, Other=0xB — the same type-ID
space persisted in IFileProperties +0x60.

**CRecoverability — the per-file grading engine** (§2, FUN at 605660+ / 14038ac90):
background-thread pipeline; per node:
```
if size < 1:                        grade = 0
else if node.type == 0xC:           grade = 2   # ZFS special-case
else if node.type in {3, 10, 0xB}:              # NTFS / ReFS / Btrfs
    deep validation path (device + fixer-aware)
else:                               generic path (FUN_1403895c0/400, tree ops)
grade adjustment: device.kind ∈ {1, 4} → offset −2
set grade via FUN_140089630(node, grade)
```
Grades observed at runtime: 0 = poor/none, 2 = high ("Your files are highly
recoverable" verdict string at 1072691) [verdict-string↔grade mapping INFERRED per
artifact §6]. `ICheckRecoverable` (vftables 438476/439200) is the pluggable
checker interface. The grading consumes the engine-side inputs: NTFS
readable-extent ratio from the bitmap cross-check (ntfs deep-dive §4), RAW
analyser content validation, and device/fixer state.

**UI-side evidence** (UIUX artifacts): the recoverability pass is
**CCheckNodeThread** spawned at TimeCriticalPriority (ctor FUN_140ad1100, 0x28
bytes) with sigFinished→onCheckFinished; runtime logs "Recovery ability matched=N,
total=M" + "Recoverability finish match.." fire on scan completion AND re-fire on
Go-to-Recover (`UIUX/31-filterbar-family-decode.md` §5; `UIUX/30-interaction-driver-ground-truth.md`
§5 — "matched=0" because the pass grades repairable-beyond-recovery candidates).
The results list has a **recovery-chance column** (CListControl/CListHeaderView
sortable headers: Name/Size/Type/Date Modified + recovery chance — `UIUX/09-results-phase-deep-dive.md`
§5). Preview bridges to the same model: `PreviewFailedWnd` grades damaged-header
files "But this file is likely recoverable based on its structure." with a
"Recovery Chances: High"-style rating; `RecoveryChancesTipsWidget` coaches on list
rows (`UIUX/10-preview-subsystem.md` §"structure grading"). The completion modal
banner "Great news: Your files are highly recoverable" (`UIUX/30` §1).

---

## 6. Classification rules

`config-files/filtering-priority--config-layer.md` (the config layer) +
`config-files/config-data-reference.md`:

1. **Noise layer — FilterFolder.ini** (filtering-priority §3): FolderName blocklist
   (Windows upgrade scaffolding $Windows.~BT/WS/TMP, Config.Msi); FileName
   blocklist (desktop.ini, .DS_Store, $UsnJrnl, IndexerVolumeGuid,
   WPSettings.dat, MountPointManagerRemoteDatabase); **FatFilterName = the
   localized "New Folder" filter (10 locales) + System Volume Information +
   BitLocker recovery-password artifacts** — "an empty auto-named folder is junk
   in every locale"; TxtViewFiles text-preview whitelist (119 extensions);
   SumaryFileType extension-unification groups for counts (m4p_m4a_m4v_mp4,
   zip_docx_pptx_xlsx — container identity, not just extension).
2. **Type ordering — PriorityDisplayType.xml** (§2): 5 categories, 35 extensions,
   each with an `alias` 2-digit sort key (`00jpg`, `01png`, `02jpeg`…) ordering
   extensions within each category in the Type tree.
3. **Family groupings** (§3 item 5 + config-reference): SumaryFileType groups.
4. **Per-format carve switches — RawConfig.ini** (§4): 34 toggles
   (**[CORRECTIONS row 5]** — recount; the config-reference's "37 types" and the
   carving deep-dive §4's "37 extension toggles" are superseded).
5. **Membership — FileFilter.xml** (§2/config-reference): **8 FileCategories**
   (Graphic, Document, Email, Audio, Video, Other, Archive, Bookmarks) /
   **1,481 `<Extention>` entries** — **[CORRECTIONS row 5]**: the config-reference's
   "1489" (and its "independent verification note" recount of 1489) is superseded
   by the mechanical recount of 1,481. 40+ camera-RAW formats under Graphic/TIFF
   group; `Selected=2` tri-state UI flags. Consumers: DRWUI
   CAdvanceFileFilterMgr + CClassify* tree controllers.
6. **Valuable-formats whitelist — usefullostfiles.xml** (§1): 27 extensions
   (photos incl. cr2/cr3/nef/arw/dng/heic; videos; documents) — promotes carved
   RAW photos out of "Unimportant" (UIUX/09 §3).
7. **Results-tree organization** (UIUX/09 §2; carving deep-dive §4): 5 special
   branches — Tags Extracted (EXIF/metadata tag grouping), **File Name Lost**
   (raw-carved by content, original name unknowable), **File Path Lost**
   (advanced-scan finds with fully/partially missing paths), **Versions** (NTFS
   file-record multi-version recovery), **Unimportant** (collapsed junk bucket).
   Merge rule: structured results win; carved output attaches as "Other deleted
   files" (carving deep-dive §4) feeding LostTreeModel/LostTableModel flat views
   (DRWUI-exe--main-ui-architecture §3.4). System-file handling: NTFS `$`-metafiles
   filtered at the engine (name[0]=='$'); $Extend hierarchy ($RmMetadata/$Reparse/
   $Repair) "classified separately from user data" (ntfs-internals §Record
   processing); recycle-bin items surfaced via IRecycleBinNode2UI with $I/$R
   pairing (fat-exfat-internals §FAT flow).

---

## 7. The phantom-record question — gate synthesis

The og pipeline prevents phantom records (results that look like recoverable files
but are noise) through the following gates, each cited:

**A. Registry-level gates (RAW carving)**
1. *Exact-key dispatch*: a candidate only invokes an analyser chain when its first
   bytes match a registered u32 fourcc or u16 magic key (census §1/§3; dual-registry
   §1/§2) — random data rarely matches, and case-duplicate keys (MSCF/mscf) are
   deliberate, not fuzzy.
2. *Two-generation dual validation*: modern analyser AND legacy `File*Analyser`
   fallback must both fail before a match is discarded — conversely one STRONG
   validator accepting is enough; "this is why carving rarely false-positives"
   (census §2).
3. *In-analyser re-validation*: e.g. WPG re-checks magic 0x677077 inside the
   FileInfo ctor and stamps a canonical size (census §6.2); ZIP sanity-checks
   LFH fields (`version_needed ≤ 10`, extradata window [1,0x102]) before walking
   (zip-analyser §1).

**B. Grammar-walk gates (structure must parse)**
4. *Structural walk success with computable end*: the hardcoded analysers compute
   the file END by walking structure — "the difference between a 'signature hit'
   and a recoverable file with size" (carving deep-dive §3). PDF version grammar
   (:41230-41233), JPEG SOF0 8-bit-precision dimension check (:39653), EBML vint
   bounds ≤ 0x200 (batch1 §3), ZIP sum-of-entries exact size (zip-analyser §2),
   PST per-generation size fields (batch1 §1).
5. *Terminator presence*: raw_signs.json tail signs for 46 of 139 extensions give
   exact file ends (config-reference); PNG repair grammar searches the literal
   IEND+CRC 0xAE426082 (image-repair §12.2); JPEG skeleton carries EOI 0xD9
   (image-repair §4); brute-force carvers "keep consuming until the grammar breaks,
   then report the last valid position" (carving deep-dive §5) — no infinite tail.
6. *Decoder-acceptance validation*: WebP valid iff libwebp's decoder accepts it
   (image-repair §12.4) — the strongest possible grammar gate.
7. *Indexless degradation, not phantom*: LFH-only ZIPs get status 0xd (not 0x48)
   and are presented as structure-incomplete (zip-analyser §3) — the product
   distinguishes "partial truth" from phantoms.

**C. Size-sanity gates**
8. *Min/max size bounds* from raw_signs.json "prune absurd candidates"
   (config-reference; census §5).
9. *PST 1 MiB floor* for unknown format versions (batch1 §1 — verified :50002).
10. *Marker-count/length bounds* in the JPEG grammars: 0x400 segments max, 0x7FFF
    length clamps (image-repair §4/§5).
11. *NTFS viability gate*: strict `0x3ff < size` — only files > 1023 bytes get the
    bitmap walk; exactly-1023 files fast-path WITHOUT the bitmap check
    (**[CORRECTIONS row 7]**, Dec :21208 — verified this session). Files < 1 byte
    grade 0 in CRecoverability (grading §2).

**D. Extent-viability gates (metadata engines)**
12. *NTFS bitmap extent scoring*: deleted files whose EVERY data-run cluster reads
    allocated in the live $Bitmap (intact == total, total > 0) are DESTROYED at
    scan time — fully-overwritten content cannot surface as a phantom "recoverable"
    record; partially-intact files survive and are graded later (v2 §1; deep-dive
    §4). Fixup validation (USN per sector, min 0x400) rejects torn/garbage records
    before property building (v2 §2; deep-dive §2).
13. *FAT chain guards*: 64KB chain-length sanity on deleted entries; EOC-family
    mask tests; visited-range RB-tree cycle-breaker; volume-bounds and capacity
    guards (fat deep-dive §4; fat-exfat v2 §2). Chain truncation tolerated but
    sized honestly (min of entry size and chain walk — internals §Recovery).
14. *FS sanity gates before any walk*: ext4 superblock field-set validation
    (nonzero counts, first_data_block rules, groups invariants — v2 §1); HFS+
    signature/512-align/version/field-clamp acceptance (hfs deep-dive §1); Btrfs
    pow2 sectorsize + nodesize band + generation selection (refs-btrfs-zfs v2 §2);
    ZFS 0xBAB10C magic + txg selection (v2 §3); ReFS page-signature chain (v2 §1).
    These prevent mounting a phantom filesystem off random data.

**E. Name/path sanity gates**
15. *NTFS $-skip*: name == NULL or name[0] == '$' → skipped (deep-dive §3 step 7 —
    verified :21186-21189); flags & 4 (temp/system) skipped.
16. *FAT/LFN checksum verification* of reassembled long names against their 8.3
    alias (fat deep-dive §4) — garbage LFN fragments do not synthesize names.
17. *exFAT triple field validation* (reserved-zero fields, EOC-family first
    cluster, UTF-16 fragment validation — v2 §3).
18. *Carved-file naming policy lives in the UI* (vtable+0x48 callback,
    "File0001.jpg" policy — carving deep-dive §4), so the engine never invents
    fake original names; name-less carves land in "File Name Lost" (UIUX/09 §2).

**F. Classification-stage gates (de-prioritization, not deletion)**
19. *Noise layer* FilterFolder.ini hides OS junk, $UsnJrnl, localized
    "New Folder"s, System Volume Information (filtering-priority §3).
20. *"Unimportant" bucket* collapses junk; usefullostfiles.xml whitelist promotes
    the 27 valuable formats (UIUX/09 §2-3; filtering-priority §1).
21. *Merge rule*: structured (FS-engine) results win over carved; carved attaches
    as "Other deleted files" (carving deep-dive §4) — duplicates and unrooted
    carves never displace validated metadata records.
22. *RawConfig.ini per-format switches* (34) can disable weak/niche carvers
    entirely (filtering-priority §4; **[CORRECTIONS row 5]** count).

**G. Post-scan grading gate**
23. *CRecoverability* grades every node (0 for size<1; deep validation for
    NTFS/ReFS/Btrfs; device-kind adjustment) and the recovery-chance column /
    "Recovery Chances: High" verdicts surface the grade (grading §2; UIUX/09 §5;
    UIUX/10) — the final user-visible filter between a real record and a phantom.

**Net synthesis**: a phantom record must survive (1) an exact magic-key hit,
(2) two independent validator generations, (3) a structural grammar walk that
computes a bounded size with min/max sanity, (4) for metadata engines, a valid
fixup + non-fully-overwritten extent set + non-$ name, and (5) the noise/
unimportant classification layer — and even then it is graded by CRecoverability
before the user trusts it. The deliberate asymmetries (keep partially-intact,
destroy only fully-overwritten; degrade indexless to 0xd rather than drop) show
the design favors salvage over purity at the record level while pushing final
credibility to the grading and classification layers.

---

## Algorithm parity matrix

| Subsystem | Exact rule the product must implement | Citation |
|---|---|---|
| **Signature registry** | One RB-tree `std::map<u32 fourcc, Analyser*>` + a second u16 first-2-bytes key family into the same registry; find-or-create + chain-append node construction (0x18 nodes, two-step vtable, 0xFFFFFFFF unassigned ID sentinel); 3 export projections (List/Map/Multimap) over one singleton; 70 u32 keys incl. MXF 0x342b0e06, ASF 0x75b22630, SQLite 0x694c5153, DWG 0x30314341, JPEG ×4, DMG `BZh1`; 39 u16 keys incl. BM/MM/%!/1F8B/FL/<h/MPEG syncs; dispatch u16 → u32 → JSON signs; case-duplicate keys not case-folding | census §1-§3; dual-registry §1-§4; carving deep-dive §2; verified :1910/:1922/:1972/:1997 |
| **raw_signs.json pipeline** | Load at init via ifstream; parse with a dedicated istream tokenizer (NOT CJsonObject); per-ext header/tail sign triples `offset,hex,confidence` + min/max; header+tail+size confidence combination ranks overlaps; magic-key registry consulted FIRST, JSON for unclaimed extensions; 139 extensions, 46 with tails | carving deep-dive §3; config-reference; census §5 |
| **EUUserData** | NOT a carving input — do not wire it into the scan engine; it feeds only DRWUI's CHKRepairFileSign repair chain. GetRawUserInfoMap = per-scan carve-statistics accumulator (insert-or-increment, cleared per init, read post-scan) | **CORRECTIONS row 9**; verified grep=0 refs |
| **Per-family validation** | BruteForce = frame-sync walk to grammar break, size = last valid position; RawSpeed = index-first exact size from internal index; File* = fallback validators (match discarded only when modern AND legacy both fail); Brance/xjq = media/newest-generation with brand/type discrimination (HEIC 0x68656963→0x7C, webm 0x6D626577); special = ISpecialRawFileAnalyser extended contract | family artifacts ×6; census §2/§4; carving deep-dive §5; batch1 §3-§4 |
| **Granular validators** | PST: ver@+0x0A 0x0E→size@+0xA8, 0x17→size@+0xB8, else 1 MiB floor, ID 0x38. RAR4: `Rar!`+u16 0x071A+ver 0, ID 0x3A. MKV: EBML vint (mask-0x80 descent ≤8 bytes, marker-strip, bounds ≤0x200), DocType webm/matroska. HEIC: brand 0x68656963 → 0x7C | batch1 §1-§4; verified :50000/:14726/:17796/:21275 |
| **ZIP carve** | Accept 3 PK anchors (LFH with `ver≤10` + extradata∈[1,0x102], CD preferred, EOCD); CD-walk with per-entry fixed-header parse, size = SUM of entries; entry-name inventory returned; sector-aligned zero-padding extension checking trailing 16 bytes/sector, cap 0x500001; indexless → status 0xd vs 0x48; query real sector size via device vtable | zip-analyser §1-§4; verified :18383/:18425/:18488 |
| **Grammar walks** | PDF `%PDF-x.y` version parse; JPEG marker walk + SOF0 precision-8 dimension check; PNG chunk table + IEND+CRC 0xAE426082 literal + parallel ScanCriticalChunks + IHDR field validation; WebP = full libwebp decode acceptance; HEIC bound-checked box walk (meta skip 0xC, moov skip 8) + ipco/ipma; JPEG bounds 0x400 segments / 0x7FFF lengths; IImageInfo 5-slot contract with CheckMagic slot | carving deep-dive §3; image-repair §4/§5/§12.1-§12.4 |
| **NTFS metadata parser** | MFT bootstrap via DBR → record-0 $DATA, 64 KB batches, $ATTRIBUTE_LIST(0x20) fork to fragmented-MFT handler, $MFTMirr cross-check; record walk: magic FILE, fixup (USN per sector, min 0x400, copy-on-wrap), corruption check, **base-record test FUN_180041970 = `+0x20 != 0` for extension-record skip (NOT in-use)**, deleted = props bit-4 clear; $-skip + flags&4 + null-name filters; runlist nibble math (hi=len-width, lo=off-width); properties field map {flags,name,size,times,attrs,hex-runlist} | ntfs deep-dive §2-§3/§6-§7; v2 §1-§2/§4; **CORRECTIONS row 2** (FUN_180041970); verified :21152/:21186/:21208 |
| **NTFS viability gate** | Strict `0x3ff < size` (files > 1023 bytes only; exactly-1023 → fast path, NO bitmap check); walk runs: sector = cluster×SPC, bitmap bit test + secondary map; `total>0 && intact==total` → destroy candidate; partial-intact kept. No 0xFFF resident cap exists | v2 §1/§3; **CORRECTIONS rows 1 & 7**; verified :21208 |
| **NTFS quick/complete** | fastsearch config default 1 (ON) via GetPrivateProfileIntW(L"search",L"fastsearch",1); Quick = deleted-inclusive MFT enumeration + parent-chain path rebuild; Complete = + $Bitmap build (MI at +0x37/+0x38) + $UsnJrnl cross-check + orphan sweep | ntfs deep-dive §5; verified :7163 |
| **FAT chain walker** | IFATTable polymorphism (CFAT12/16/32, CDoubleFATTable mirror); FAT16 Next canonicalizes BAD 0xFFF7→0xFFFFFF7, EOC ≥0xFFF8→0xFFFFFF8; **FAT12 BAD 4087 followed RAW (no canonicalization — og behavior)**; EOC mask families (28/24-bit, 0xFFFFFF0 forms); chain loop with (c-2)*SPC+first_data_sector, batched run reads, visited-range RB-tree cycle guard, "."-entry anchor; deleted 0xE5 ×6 sites + 64KB chain guard; LFN 0x40/0x0F checksum-verified reassembly | fat deep-dive §2-§4; fat-exfat v2 §1-§2; **CORRECTIONS row 3**; verified :8035/:54125/:53772 |
| **exFAT dentry triples** | Type low-7-bits + InUse bit 0x80 (deleted = cleared); (t-3)/(t-5)/(t-0x40)/(t+0x3F) & 0x7F class tests; stream-ext reserved-zero + first-cluster 1-or-EOC; backward 0x40-strided triple reconstruction into parallel list builders; CQuickSearchAndBuildBitmap; NoFatChain support | fat-exfat v2 §3; exfat deep-dive §2-§3 |
| **ext4 engine** | SB magic 0xEF53@+0x38 read at 0x400; field-set validation (nonzero inodes/blocks/bpg/ipg, both first_data_block rules, block_size=0x400<<log, groups invariants, 64-bit GDT via feature 0x10→desc_size); journal salvage = jsb magic 0xC03B3998 + blocktype {3,4} ONLY (no commit/revoke); CatchBuffer retry (0xE00 buffer, 0x400 backtrack); NO backup-SB constants | ext4 deep-dive §2-§3; ext4-hfs v2 §1-§2; **CORRECTIONS rows 4 & 6**; verified :3222/:12207 |
| **HFS+ engine** | Volume gate: "H+"/"HX", blockSize%512==0, version<0x11, alloc/extents ≤ totalBlocks; BE decode; Catalog/Extents/Attribute B-trees, cnid parent chains, +1+record_count+attr_count leaf math; deleted → orphaned-cnid sweeps + RAW carve (catalog overwritten on delete) | hfs deep-dive §1-§2; ext4-hfs v2 §3-§4 |
| **ReFS engine** | MSB chain: page_type 1/2/3 signatures "SUPBMSB+ReFSFSRS"/"CHKPSUPBMSB+ReFSFSRS"/"MSB+ReFSFSRS", size≥0x20, RefsMetadataBlockRefrenceImpl parse, {block_id, flags, size, next} accumulation via RefsBaseDataImplManager; CRefs(Resident)DataRunList; OS-version gating | refs deep-dive §1-§3; refs-btrfs-zfs v2 §1 |
| **Btrfs engine** | SB at 0x10000 magic `_BHRfS_M`@+0x40; mirror table walk; sectorsize pow2 & nonzero, nodesize ∈[0x86,0x86+0xFF7B] & %sectorsize==0; **max(generation)@+0x48 wins**; chunk-tree logical↔physical translation; subvolume roots; zlib inflate + crc32c/adler32; **NO chunk-tree-uuid validation (name memcmps only)** | btrfs deep-dive §2-§3; refs-btrfs-zfs v2 §2; **CORRECTIONS row 8** |
| **ZFS engine** | Uberblock magic 0x00BAB10C; vdev labels (4×128 KB front/back); 128×1 KiB ring, 0x80-qword stride; txg-gated newest-wins selection; MOS→DMU→ZAP→ZPL walk via CStreamBase (gang blocks, lzjb/gzip, checksum verify); snapshots as dataset roots | zfs deep-dive §2-§4; refs-btrfs-zfs v2 §3 |
| **Grading model** | RecoveryAssistant category IDs {5,6,7,8,9,0xB} = IFileProperties +0x60 type space; CRecoverability per node: size<1→0, type 0xC (ZFS)→2, types {3,10,0xB}→deep validation, else generic; device.kind∈{1,4}→−2 offset; grades 0=poor, 2=high; ICheckRecoverable pluggable; CCheckNodeThread at TimeCriticalPriority re-fires on Go-to-Recover; recovery-chance column + "Recovery Chances: High" verdicts | grading §1-§2; UIUX/31 §5; UIUX/30 §5; UIUX/09 §5; UIUX/10 |
| **Classification** | Pipeline: FilterFolder.ini noise layer (blocklists + localized New-Folder + SVI + BitLocker artifacts) → FileFilter.xml membership (8 categories / **1,481** extensions) → PriorityDisplayType.xml alias ordering → SumaryFileType family groups → RawConfig.ini carve switches (**34**) → usefullostfiles.xml whitelist; results tree: Tags Extracted / File Name Lost / File Path Lost / Versions / Unimportant; structured-wins merge, carved → "Other deleted files"; NTFS $-files engine-filtered, $Extend separately classified, recycle bin via $I/$R pairing | filtering-priority §1-§6; config-reference; **CORRECTIONS row 5**; UIUX/09 §2; carving deep-dive §4 |
| **Phantom gates** | (1) exact-key dispatch; (2) two-generation dual validation; (3) in-analyser re-validation (WPG 0x677077); (4) grammar walk with computable end; (5) terminator presence (tail signs / IEND+CRC / EOI); (6) decoder acceptance (WebP); (7) indexless → 0xd degradation; (8) min/max size bounds; (9) PST 1 MiB floor; (10) JPEG 0x400/0x7FFF bounds; (11) NTFS strict 0x3ff<size viability + destroy-if-fully-reallocated + fixup rejection; (12) FAT chain guards (64KB, EOC masks, cycle tree, bounds); (13) FS sanity gates (ext4 field set, HFS+ sig/align/version, Btrfs geometry+generation, ZFS magic+txg, ReFS signatures); (14) name sanity ($-skip, LFN checksum, exFAT triple fields, UI-side carved naming); (15) noise/Unimportant de-prioritization + whitelist promotion; (16) structured-wins merge; (17) CRecoverability grading as the final user-visible gate | synthesis of all above; per-gate citations in §7 A-G |
