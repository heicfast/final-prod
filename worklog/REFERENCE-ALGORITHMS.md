# REFERENCE-ALGORITHMS — Detection, Validation, Scoring, Persistence, Cache (RAW/decompiled level)

> Agent C Phase 0 sheet. Corpus root: `/home/z/my-project/repos/easeus-drw-research/`.
> Every claim cites a corpus file (artifact path + section, or decompiled function). Where the
> corpus is silent, the sheet says "not covered". Confidence tags reuse the corpus convention:
> [CODE] = decompiled, [CENSUS] = value distribution over shipped .rsf samples,
> [INFERRED] = interpretation flagged by the source artifact.
> Raw stats in sections 6/7 were recomputed this session directly against
> `og-codebase/raw_signs.json`, `og-codebase/EUUserData`, `og-codebase/FileFilter.xml`,
> `og-codebase/config/PriorityDisplayType.xml` (scripts inline, results below).

---

## 1. The RAW carving engine (RAWSearchDLL.dll)

Primary sources:
- `re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md`
- `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md`
- `re-workspace/artifacts/SearchDLL/raw-dual-registry--u16-magic-keys.md`
- `re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md`
- `re-workspace/artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md`
- `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` (1,921 functions / 80,614 lines)

### 1.1 Controller and exports [CODE]
- `EUCreateRAWSearcher` → `new(0x160)` → FUN_18000bba0 = **CRAWSearchController** ctor (352 B,
  lean: no FS metadata state). (rawsearch-carving--deep-dive.md §1; decompiled lines 72122-72230)
- `EUCreateRAWSearcherForCellTest` → static once-init (`_Init_thread_header/footer` + atexit) into
  DAT_180096240; FUN_18000c650 binds `CRawFileCellTestStorageDevice` — an in-house unit-test
  harness (fake storage feeding synthetic sector images) shipped in the product.
- `EUGetAnalyserList / EUGetAnalyserMap / EUGetAnalyserMultimap` → all fetch the global registry
  singleton (FUN_180006a80) and project it into list (FUN_180006b20) / map (FUN_180006b00,
  magic→first-match) / **multimap (FUN_180006b10, magic→ALL matches)** views.
- `GetRawUserInfoMap` → FUN_180001350()+0x90 — per-extension user preference table
  (ties to `config/RawConfig.ini`, loaded at decompiled line 13133).

### 1.2 The signature registries [CODE]
Registry = `std::map<u32 fourcc_magic, Analyser*>` built per-format at static init
(RB-tree descent by key comparison, FUN_18000acf0 = `_Tree::_Insert`, FUN_18000ad50 =
rebalance/rotate; construction decoded at decompiled lines 1560-1700). Each binding is a
0x18-byte chain node: base `IRawFileAnalyser` vftable first, `idx=0xFFFFFFFF` sentinel, then
concrete vftable overwrite + numeric format-ID, plus optional legacy fallback link
(`File*Analyser` first-generation class) appended via FUN_180006c80 / FUN_18000ac20 /
FUN_18000ac70.

Two key families feed ONE registry (raw-dual-registry--u16-magic-keys.md §1-2,
registry builder FUN_180004320 at decompiled lines 2517-2918):
1. **u32 4CC keys** (first 4 bytes, LE): e.g. 0xeeffd8ff (JPEG SOI+APP14), 0x46445025 (`%PDF`),
   0x474e5089 (PNG), 0x21726152 (`Rar!`), 0x04034b50 (ZIP), 0x2a004d4d/0x2a4949/0x554949/
   0x4f524949 (TIFF byte-order family), 0x494a5546 (FUJI RAF), 0x31685a42 (BZh1 → DMG),
   0x62564f46 (FOVb X3F), 0xa3df451a (EBML/MKV), 0xe011cfd0 (OLE), 0x46464952 (RIFF),
   0x46535449 (ITSF/CHM), 0x5367674f (OggS), 0x75b22630 (ASF GUID), 0x694c5153 (SQLi),
   0x30314341 (AC10 DWG), 0x38464947 (GIF8), 0x74725c7b (`{\rt` RTF), 0x4643534d/mscf (CAB,
   both cases = duplicate keys, not case folding). ~70 unique keys
   (rawsearchdll--analyser-registry-census.md §3).
2. **u16 first-2-bytes keys** (39 entries): 0x2125 `%!` (PS/EPS), 0x4D42 `BM` (BMP),
   0x4D4D `MM` (TIFF BE), 0x4C46 `FL` (FLV), 0x683C `<h` / 0x483C `<H` / 0x213C `<!` (HTML),
   0x8B1F (gzip), 0xFFF1 (AAC ADTS) + 0xFFF2..0xFFFF MPEG syncs + 0xFFE2..0xFFE7 + 0xF9FF,
   0x534D `MS`, 0x4449 `ID`, 0x0100, 0x0000 fallback chain head.
   (raw-dual-registry--u16-magic-keys.md §2)

Dispatch order (u16 key first, then 4CC table, then raw_signs.json signature DB) is
**[INFERRED]** in raw-dual-registry §1 — the tables are [CODE], the ordering is not.

Analyser census: 106 instantiated classes / 488 vftable references; namespaces:
`BruteForce::` (24 classes — frame-sync scanners), `RawSpeed::` (20 — homage naming, not the
library), `Brance::` (22 — media/Apple ecosystem), `RawFormat_xjq::` (8 — newest formats
HEIC/JP2/MOBI), first-generation flat `File*Analyser` (46), plus standalone
(AEP/CHFP/CHIK/COSP/CPOS/CGPX/CVCF/C7ZIP/CFileAAC/CFileAIF/CFileOgg — note vendor
Analyser/Analyzer spelling inconsistency). Infrastructure: `IRawFileAnalyser`,
`ISpecialRawFileAnalyser`, `CRawFileInfo`, `CZeroEndRawFileInfo`, `CNeedRawMgr` (decides
which formats need raw analysis per scan mode), tinyxml2, `neb::CJsonObject`.
(rawsearchdll--analyser-registry-census.md §4)

### 1.3 How a candidate is validated (the grammar-aware validators) [CODE]
Three-layer pipeline (rawsearch-carving--deep-dive.md §3; searchdll-family--architecture.md §4):
1. `raw_signs.json` (139 extensions) — loaded at engine init (decompiled line 23836:
   `std::ifstream("raw_signs.json")` → seekg(end)/tellg → whole-file read → **hand-rolled
   istream tokenizer FUN_180021460 / FUN_180021380** — *not* tinyxml2, *not* CJsonObject for
   this file; both exist in the binary for other configs).
2. `EUUserData` magic/MIME DB — secondary typing (see §6.2).
3. **Hardcoded C++ analysers** — per-format parsers that compute the file END by walking
   structure. Verified examples:
   - **PDF**: `(*param_2 == 0x46445025) && (param_2[1]=='-')` then `%PDF-x.y` version parse
     (decompiled 41230-41233).
   - **JPEG**: marker walk incl. SOF0 `bVar1==0xc0 && pbVar25[4]==8` (precision 8 dimension
     check, decompiled 39653).
   - **ZIP** (the reference "walk-the-index" carver, rawsearchdll--zip-analyser-carving.md):
     anchors accepted for ALL three structures — PK\x03\x04 (LFH, sanity: version_needed ≤ 10
     at +8, extradata/flags at +0x1a ∈ [1,0x102]), PK\x01\x02 (CD entry, preferred anchor),
     PK\x05\x06 (EOCD); loop FUN_1800176e0 walks entries (per-entry parser FUN_180045690:
     name len +0x1c, extra +0x1e, comment +0x20), exact carve size = SUM of parsed entries;
     end-of-archive handling aligns down to sector, then forward-scans zero padding while the
     last 16 bytes (4 dwords) of each sector are zero, **cap 0x500001 ≈ 5.24 MB**; indexless
     result carries status 0xd vs 0x48 for full carve with CD; entry list merged via
     FUN_1800173f0 (inner file inventory surfaces in the results tree).
   - **PST** (FUN_180041eb0): ver byte @+0x0A == 0x0E (ANSI) → size u32 @+0xA8; == 0x17
     (Unicode) → size u32 @+0xB8; else floor 1 MiB; type-ID 0x38.
   - **RAR4** (FUN_180012bb0): `"Rar!"` + u16@+4 == 0x071A + buf[6]==0 → type 0x3A; entry
     iterator FUN_180012d00 with 272-byte filename buffer. RAR5 rides the 4CC/fallback tables
     [INFERRED from signature delta].
   - **MKV/WebM** (FUN_180016c70): full EBML variable-length-integer decoder — mask-descent
     leading-zero count (max 8 bytes), value = strip marker bit + shift in payload, element
     bounds `n + value + 4 ≤ 0x200`, DocType discrimination via 0x6D626577 ("webm").
   - **HEIC** (~21278): ftyp brand == 0x68656963 → type 0x7C, label L"HEIC"; HEIF-family brands
     relabeled L"HEIF".
   - **WPG**: defence-in-depth — magic 0x677077 re-checked inside the FileInfo ctor, canonical
     size 0x45 stamped.
   (rawsearch-carving--deep-dive.md §3; analyser-granular-batch1--pst-rar-mkv-heic.md;
   rawsearchdll--zip-analyser-carving.md §1-3; rawsearchdll--analyser-registry-census.md §6)
   - **BruteForce namespace**: parsers that keep consuming until the grammar breaks and report
     the last valid position as file size — "maximize salvage" for formats without crisp tails
     (rawsearch-carving--deep-dive.md §5).
   - **Two-generation design**: every format has a modern namespaced analyser and (where one
     exists) a legacy `File*Analyser` fallback on the same chain node — "two independent
     validators must both fail before a match is discarded" (rawsearchdll--analyser-registry-census.md §2).

### 1.4 How confidence is computed and the winner picked
- raw_signs.json carries **explicit per-signature confidence** (see §6.1; observed range
  0.50-1.00). The engine "accumulates header + tail confidences to rank candidate carves
  (multiple overlapping candidates resolved by score)" and "Header+tail+size-gate confidence
  combination ranks overlapping candidates" (searchdll-family--architecture.md §4.4;
  rawsearch-carving--deep-dive.md §3). **The exact numeric combination function is NOT
  decompiled anywhere in the corpus — not covered.**
- Collision resolution structures [CODE]: the **multimap** projection exists precisely
  because several keys are claimed by more than one chain (4 JPEG magics; TIFF `II*.` claimed
  by TIFF + camera-RAW analysers) (rawsearchdll--analyser-registry-census.md §1/§3).
- Chains allow multiple analysers per key (e.g. gzip: `RawSpeed::GZAnalyzer` +
  `FileGZAnalyser` — one for header-validation carving, one for full validation; chain
  semantics [INFERRED], pair construction [CODE]) (raw-dual-registry--u16-magic-keys.md §4).
- Per-extension runtime gate: `config/RawConfig.ini` (37 toggles, `[Filter]` FORMAT=1/0)
  loaded at decompiled line 13133 via the ini machinery; maps 1:1 onto the standalone
  niche-format analysers (filtering-priority--config-layer.md §4).

### 1.5 Scan integration contract
- RAW searcher is registered LAST in the advanced-scan engine list (DRWUI FUN_140580720),
  gated by a settings flag at +0x60; receives a `std::function<string(string,string)>`
  naming/filter callback via vtable+0x48 — the "File0001.jpg" naming policy lives in the UI,
  not the engine (rawsearch-carving--deep-dive.md §4).
- Carved nodes feed `IRawDirectoryTreeBuilder` → LostTreeModel/LostTableModel flat "Lost data"
  views; merge phase: structured (FS) results win, carved attaches as "Other deleted files"
  (same source §4).

---

## 2. Per-FS metadata parsers

### 2.1 NTFS (`re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md`,
`ntfs-engine-algorithms-v2--deleted-viability.md`, `ntfs-engine-internals.md`;
decompile `NTFSSearchDLL_decompiled.c`, 1,931 fns / 62,394 lines)
- Factories: `EUCreateNTFSSearcher` → `new(0x940)` CNTFSSearchController (FUN_180007ba0),
  multiply-inheriting `IBuildVolumeBitmap` (secondary vftable at obj+0x37/+0x38);
  `EUCreateNTFSQuickSearcher` → FUN_1800014a0 (CNTFSQuickSearchBase).
- **$MFT bootstrap** (FUN_18001a3c0): 64 KB buffer read; record validation:
  `fixup = FUN_180041bb0(buffer, recSize*param_2)` (USA/fixup apply), header sanity
  FUN_1800411d0 (magic 0x454c4946 "FILE", flags, sequence). **$ATTRIBUTE_LIST (type 0x20)
  fork**: FUN_180041850(buffer, 0x20, 0) probes; present → log "MFT file record has 0x20
  attribute, MFT start sector is %I64d" → FUN_18001a540 fragmented-MFT handler; absent →
  FUN_180041cf0 parses record 0 $DATA runlist (FUN_180041c50 validates, FUN_180007550 installs
  the MFT stream). Detector for 0x20: FUN_1800419e0 (`find_attribute(rec,0x20,0)!=NULL`).
- **Fixup chain** (FUN_180041bb0) [CODE]: min record parse size 0x400; fixup offset (FUN_180046160)
  must be nonzero; FUN_180041a20 applies fixups across sectors; FUN_180041370 validates USN per
  sector; FUN_180041290 restores. Wrapper FUN_180041cf0 implements copy-on-wrap (copy_flag
  duplicates the buffer via thunk_FUN_18002be40 so fixup cannot corrupt the shared scan buffer).
  MFT reads are cluster-strided, 0x1000-granular pages unpacked into record-size slots.
- **MFT walk** (record enumeration, FUN at decompiled line 21120; record size clamped ≥0x400):
  per record: magic=="FILE" (21152) → fixup valid (FUN_180041bb0) → not "bad"
  (FUN_180034b20==0) → header checks (FUN_180041970==0) → build CNTFSFileProperties
  (FUN_1800431d0) → resolve into result tree FUN_180006db0 (0x200 sector size assumption).
  Filters: `flags & 4` skip (temp/system, line 21170); `name == NULL` skip; `name[0]=='$'`
  skip ($MFT/$Bitmap/$UsnJrnl metafiles). Deleted branch: in-use flag clear
  (`(props->flags & 0x10) == 0`, line 21186) → bitmap cross-check (§3.1 below).
  A second 'FILE' site FUN_18002a780 checks a '0' suffix at rec+0x5C (BAAD/'FILE0' corrupted
  family discrimination).
- **Deleted-record batch loop** — FUN_18001acc0 (the "File Path Lost" feed):
  `record_size = device.vt[0x28]()` (fallback 0x200), buf_size = max(0x400, record_size);
  per record: cancel check → 'FILE' magic → fixup/secondary checks → **only NOT-in-use
  (deleted) records pass** (FUN_180041970: parsed+0x20 != 0 → in use → skip) → parse ctx
  (FUN_1800431d0) → validate (FUN_180043030) → build props (FUN_180006db0) → system-file
  filter (`flags&4`, name NULL, name[0]=='$') → **if not a folder (flags&0x10)==0 and
  size ≥ 0x3FF (1023 B)**: iterate data runs (FUN_1800340b0 iterator, FUN_1800341c0 next),
  for each run `sector = sectors_per_cluster * cluster`; `bitmap.test(sector)`
  (FUN_18002c2e0) OR secondary-map membership (controller +0x60 virtual check) counts as
  re-allocated; **total>0 && intact==total → props.Release() (ALL clusters re-allocated →
  candidate destroyed)**; partial → kept. Files <1 KiB skip the viability walk (often
  resident data). [CODE for mechanics; $Bitmap reading INFERRED-but-consistent]
  (ntfs-engine-algorithms-v2--deleted-viability.md §1)
- **Bitmap test** FUN_18002c2e0 [CODE]: `pos = base(+0x50)+sector_index; bounds vs limit(+0x10);
  return buffer(+0x00)[pos>>3] & (1<<(pos&7))` — plain bit array with base-offset rebasing;
  two bitmaps at controller +0x40/+0x48 with secondary virtual check at +0x60.
- **Attributes**: 0x20 $ATTRIBUTE_LIST (fragmented reassembly); 0x30 $FILE_NAME parsed by
  `CNTFSFileRecordFileNameAttribute` (type checks at decompiled 52190/53409) — parent-reference
  + namespace + filename + times; 0x40 $OBJECT_ID / 0x80 $DATA (checks at 51006/54487) split
  `CNTFSFileRecordDataRunsAttribute` (non-resident) vs resident payload, resident cap
  `≤ 0xFFF` (region 450-474).
- **Runlist nibble math** (FUN_180003440, lines 2378-2384):
  `len += (run[i] >> 4) + 1 + (run[i] & 0xF)` while `run[len] != 0` — high nibble = offset-field
  width, low nibble = length-field width. Runlists kept in hex text form across serialization
  (CNoneResidentNTFSDataRunList ↔ hex parse in DRWUI FUN_1405f5150 — shared engine/UI model).
- **CNTFSFileProperties field map** (serializer FUN_180003440):
  `{+0x19 u16, +0x11 str, +0x1b u64, +0x23 u16, +0x25 u64, +0x2d u64, +0x35 u64, +0x3d u8,
  +0x40 u64, +0x64 u32, +0x60 u32, +0x48 hexrunlist, +0x50 runlist-map}` =
  {name, size, flags, times, attributes, runlist-in-hex}.
- **$MFTMirr resilience** (NTFSLib `InitMFTMirror`, ntfsmft.cpp, ghidra-mo/NTFSLib_decompiled.c
  17310-17430; zfs-vdev--mftmirr--calcpartition.md §2): fixed-location estimate
  `mirror_sector = clusters_per_record(+0x24) * mft_start(+0x18/+0x10) + volume_base(+0x38)`;
  on runlist failure, fallback reads **MFT record 1 (=$MFTMirr)** and its non-resident unnamed
  $DATA runlist. $UsnJrnl cross-check: strcmp sites at 9070/19761/19868/20282.
- Quick vs complete: quick = direct $MFT enumeration incl. deleted records (fixup-aware),
  path reconstruction from parent-reference chains, `GetPrivateProfileIntW(L"search",
  L"fastsearch", 1, ini)` default ON (line 7163); complete = + $Bitmap volume bitmap build +
  USN cross-check + orphaned-record sweep + attribute-stream assembly.
- IO: `CStorageDeviceWithCache` → IStorageDevice; damaged/locked volumes via
  `\\.\EPRDTDRV\{d}\{p}` EPMNtDrv IOCTL path; memory-adaptive batching
  (GlobalMemoryStatus logging); condition-variable worker coordination.
- Thread safety: `_Ref_count_base` refcounting with interlocked LOCK()/UNLOCK() at
  21170-21205 (async UI workers).

### 2.2 FAT (`fat-engine--deep-dive.md`, `fat-exfat-engine-internals.md`,
`fat-exfat-engine-algorithms-v2.md`; decompile 1,265 fns / 64,658 lines)
- Factories: `EUCreateFATQuickSearcher` → `new(0x1C0)` CFATQuickSearch (448 B, embedded 0x200
  sector staging buffer at +0xA0); nested `CFATQuickSearch::CFATQuickSearchWorker`
  (threaded per-worker, 13+ ctor params, sector-size defaulting 512);
  `EUCreateFATSearcher` → CFATSearchController. Type dispatch: partition-context FAT flavor
  0=FAT12, 1=FAT16, 2=FAT32 → labels ("UNKNOWN" fallback); detection via
  `_strnicmp(bootsector+0x36, "FAT16   ", 8)` (line 38048) and OEM label offsets 0x36
  (FAT12/16) / 0x52 (FAT32).
- **IFATTable polymorphism** [CODE]: FAT16 `Next` (FUN_180041c80): `cluster ≥
  cluster_count(+0x1C) → 0`; `entry = *(u16*)(entries(+0x20)+cluster*2)`;
  `entry == 0xFFF7 → 0xFFFFFF7` (canonical BAD); `entry < 0xFFF8 → next`; else `0xFFFFFF8`
  (canonical EOC). FAT32 EOC family masks (8035-8060): `0xFFFFFFF8` EOC, `(e & 0x0FFFFFF8)
  == 0xFFFFFFF8` family, `(e & 0xFFFFFFF0) == 0xFFFFFFF0` alternate forms, mask-24 secondary —
  tolerant termination. `CDoubleFATTable` (ctor FUN_180041d00) composes two child IFATTables
  (mirrored FAT1/FAT2, fields +8/+0x10, cascading Release); primary/mirror fallback switch
  [INFERRED].
- **Chain-follow / directory accumulator** (FUN at corpus line 11200) [CODE]:
  `cluster→sector = (cluster-2)*SPC + first_data_sector` (param fields +4/+8); whole-run
  batching (one device.read per FAT run); **visited-range red-black tree at P+0x10 as
  cycle guard** (each cluster range inserted once; repeat → stop); `.`-entry detection
  (`0x202020202020202E` + space at +0xA + attr 0x10) anchors directory identity.
- **Deleted entries**: first byte 0xE5 (compiled as `-0x1b`) at **6 sites** (11742, 14634,
  14825, 29978, 30178, 32155) with a `< 0x10000` cluster sanity guard bounding corrupt chains;
  deleted entries retained with deleted flag.
- **LFN**: 0x40 last-logical-entry flag + 0x0F attribute checks at 11442/28446/33978/44355;
  "LFN fragments are reassembled checksum-verified against their 8.3 alias (the standard
  algorithm — visible as paired short/long entry state machines)" (fat-exfat-engine-internals.md
  §3). **The byte-exact LFN checksum routine is not decompiled — not covered.**
- Recycle Bin: `$RECYCLE.BIN` / `Recycled` / `Recycler` byte-exact strings; `$I…/$R…`
  info/restore pairing by index across bin subfolders via `IRecycleBinNode2UI`.
- Complete scan adds deleted-entry chain reconstruction + cross-FAT recovery + orphan
  cluster re-linking; chain truncation tolerated (size = min(entry size, chain walk)).
- Geometry: `Fat32CalcPartition` (RTTI, 39 vftable refs) — lost-partition candidate scorer
  (see §2.6).

### 2.3 exFAT (`exfat-engine--deep-dive.md`, `fat-exfat-engine-algorithms-v2.md`;
decompile 1,299 fns / 58,043 lines)
- `EUCreateEXFATQuickSearcher` → `new(0x8E0)` CExFatQuickSearch (2,240 B — embeds the
  allocation-bitmap builder); classes incl. `CExFatVolumeAnalyer` [sic], `CExFatPartition(Ex)`,
  `CExFatCompleteSearch`, `CExFatDataRunList`, `CExFatFileProperties`,
  **`CQuickSearchAndBuildBitmap`** (4 vftable sites 10523/10577/11120/11328) — builds the
  volume bitmap while walking dentries; reuses `CNTFSFileProperties` /
  `CNTFSFileRecordDataRunsAttribute` (NTFS-like extent model). NoFatChain contiguous runs
  resolved purely from directory entries (no FAT walk).
- **Dentry triple validation** (FUN at corpus line 1840) [CODE]: entries 0x20 bytes; first
  byte low 7 bits = type, **bit 0x80 = InUse** (deleted keeps type with 0x80 cleared —
  `(t - X) & 0x7F` tests accept live+deleted):
  - `(t-3)&0x7F==0` (0x03/0x83) volume-label triple: entry itself, +0x20 must be 0x81 (TX),
    +0x40 must be 0x82 (label) — root's opening sequence;
  - `(t-5)&0x7F==0` (0x05/0x85) allocation-bitmap dentry → FUN_180002f90(entry+1) checks the
    bitmap descriptor fields;
  - `(t-0x40)&0x7F==0` (0x40/0xC0) stream-extension: reserved fields at +2, +6/+7,
    +0x10..0x13 must be zero; `u32@+0x14` (first cluster / no-fat-chain) must be 1 or
    ≥ 0xFFFFFF7 (EOC-family);
  - `(t+0x3F)&0x7F==0` (0x41/0xC1) filename dentry → FUN_180003030(entry+1) validates the
    UTF-16 name fragment.
- **Triple reconstruction** (FUN at line 1912) [CODE]: walks 0x40-strided entries BACKWARD
  from the stream dentry (`param_1 - 0x40`), dispatching each type class into three parallel
  std::list builders (FUN_18000b760), rebuilding the {file-dentry → stream-ext →
  name-fragments} chain; deleted chains (0x80 cleared on all members) reconstructed
  identically.
- Volume validation: boot + up-case-table checks (CExFatVolumeAnalyer); config filtering
  `GetPrivateProfileStringA("Filter","FATFilterName",…)`; `$RECYCLE.BIN` handling mirrored
  from FAT; logs `c:\log\euExfat.log` / `DelexFat.log`. Geometry scorer:
  `ExFatCalcPartition` (54 vftable refs — heaviest single class in its engine; SPC
  re-derivation heuristics; detailed scoring walk = open item, not covered).

### 2.4 EXT4 (`ext4-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md`;
decompile 475 fns / 18,071 lines)
- Exports `EUCreateEXT2Searcher`/`EUCreateEXT2QuickSearcher` (+ EXT4 variants; one family
  covers ext2/3/4).
- **Superblock validation FUN_180004710** [CODE]: `s_magic(+0x38,u16) == 0xEF53`;
  `s_inodes_count(+0x00) != 0`; `s_blocks_count_lo(+0x04) != 0`; `s_blocks_per_group(+0x20)
  != 0`; `s_inodes_per_group(+0x28) != 0`; first_data_block cross-checks
  (`s_first_data_block(+0x14)==0 XOR s_log_block_size(+0x18)!=0` reject pair — the kernel
  1-KiB-block rule); `block_size = 0x400 << (s_log_block_size & 0x1F)`;
  `total_blocks = (u64)s_blocks_count_hi(+0x150) << 32 | lo`;
  `groups = total/blocks_per_group`; geometry invariants (groups>0, ≤ total, GDT bytes sane).
  Backup superblocks probed at sparse positions (8193, 32768…).
- **Mount/geometry FUN_1800047c0** [CODE]: reads 0x200 bytes at **offset 0x400**; copies an
  0x400-byte window into the volume object at +0x98; feature-driven corrections — flag 0x04
  at +0xF4 remaps a field from +0x178; **flag 0x10 at +0xF8 → u16 desc_size (+0x196)**
  (64-bit GDT feature; both 32- and 64-byte GDT entries understood); `strncpy(label,
  s_volume_name, 0xF)`; 2×block_size GDT read buffer via device block API (vtable+0x78).
- **Bad-sector resilience — `CEXT4CatchBufferDeviceProxy`** [CODE]: reads strided by geometry;
  on failure allocates a fresh 0xE00-byte buffer and retries with a 0x400 (1 KB) backtrack
  window — skip-and-salvage (matches the "Program is trying to skip bad sectors to find data"
  UX copy).
- Journal traversal: traces "GotJournalDir: id=%I64d,parentId:%I64d,sector:%I64d" and
  "TraverseJournalDir: fileName:%s,id=%I64d,parentId=%I64d" — walks the ext4 journal
  (inode 8, .journal) for recently-modified/deleted state not in the main trees
  (remaining-engines--ext4-hfs-refs-btrfs-zfs.md EXT4 section).
- Structures walked: superblock → group descriptors → inode tables → extents
  (`CExt4DataRunList`; ext3 fallback = indirect blocks) → directory entries "linear + htree
  (dx_dir) hashed directories" (ext4-engine--deep-dive.md §4 — from class census + family
  conventions; **a decompiled htree walk is not covered**). Orphan-file/list handling for
  deleted inodes. Deleted-file fields surfaced via `CEXT4FileProperties` (type 8): name
  (UTF-8 std::string), size +0x31, FLAGS +0x39, creation +0x3B / write +0x43 / access +0x4B
  (FILETIME qwords), marker +0x53, inode/parent anchor +0x55, extent runlist
  (ifileproperties-part2 §1).

### 2.5 HFS+ / ReFS / Btrfs / ZFS
- **HFS+** (`hfsplus-engine--deep-dive.md`, `ext4-hfs-engine-algorithms-v2.md` §3-4):
  volume header validation [CODE] at corpus line 10005 — big-endian decode first; signature
  `0x482B ("H+")` or `0x4858 ("HX")` (HFSX); `block_size & 0x1FF == 0` (multiple of 512);
  `version < 0x11`; `total_blocks >= start_of_alloc_block`, `free_blocks <= total_blocks`.
  Catalog leaf walker FUN_18000c8f0 [CODE]: first record offset inside a B-tree leaf =
  `volume.base(+0x60) + 1 + node.record_count(+0x24) + node.attr_count(+0x20)`; per-node
  cancel/EOF virtual checks; sibling-list walk = B-tree traversal driver (field naming of
  +0x24/+0x20 [INFERRED from usage]). Class family: CHFSPlusCatalog / Extents / Attribute /
  BTreeOptionBase / FileNode / FileCache (hot-node cache) / Rawfile /
  CPartitionVirtualVolumeDevice; embedded source paths hfsplus{volume,btree,...,journal}.cpp
  (journal replay support). Deleted-file fields via `HFSPlusFileProperties` (type 5): name,
  size +0x8, FLAGS +0x9, created +0x4C / write +0x54 / access +0x5C, mode/flags +0xE, type
  id +0x74, marker +0xF, extents as `[u32 count][count×16 B {u64,u64} pairs]` from an
  std::map→std::list conversion (serializer FUN_1405e4180); runlist class
  `HFSPlusFileDataRun` (alloc 0x48, AddExtent via vtable+0x38). Second variant
  `HFSPlusSystemNodeProperties` (marker 1, alloc 0x40) for catalog file IDs.
  (ifileproperties-part2 §2)
- **ReFS** (`refs-engine--deep-dive.md`, `refs-btrfs-zfs-engine-algorithms-v2.md` §1):
  `refs::` RTTI namespace — RefsDbrImpl, RefsMSTreeHeaderDescImpl, RefsMetadataBlockHeaderImpl,
  RefsMetadataBlockRefrenceImpl [sic], RefsCheckPointImpl, RefsDirectoryValuesImpl /
  RefsFileValuesImpl, RefsBaseDataImpl(+Manager<T>).
  Block-header parser (FUN at ReFS decompile line 11670) [CODE]: `size < 0x20` reject;
  page_type 1 → sig "SUPBMSB+ReFSFSRS", 2 → "CHKPSUPBMSB+ReFSFSRS", 3 → "MSB+ReFSFSRS";
  `RefsMetadataBlockRefrenceImpl::parse(page+8, size-0x20)`; on valid ref:
  `header.size(+0x20) += ref.field(+0x30)`; copies {valid, block_id, flags(+0x2C),
  size(+0x30), next(+0x38)} into the RefsBaseDataImplManager. Resident small files:
  `CRefsResidentDataRunList` (vftable literally reused from
  CNoneResidentNTFSDataRunList). OS-version gating in imports.
- **Btrfs** (`refs-btrfs-zfs-engine-algorithms-v2.md` §2; btrfs-engine--deep-dive.md):
  superblock selection (FUN at corpus line 10290) [CODE]: candidate offset table DAT_180018d60;
  `*(u64*)(buf+0x40) == "_BHRfS_M"` (0x4D5F53665248425F); `sectorsize = u32@+0x90` must be
  power-of-2; `nodesize = u32@+0x94` ∈ [0x86, 0x86+0xFF7B] and divisible by sectorsize;
  **newest generation (+0x48) wins across mirrors**. Decompression: ZLIB inflate + RC4 PRGA
  (encrypted datasets) + CRC32/adler32 checksums. Persistence: `BtrfsCache.txt` (relative
  path, 4 memcpy sites in decompile at 1991/2003/7266/7272) — metadata cache in the DLL's
  directory to speed re-scans (subvolume/chunk trees cached); **content schema/eviction of
  the cache file: not covered**. Export typo `EUCreateBtrfsComplateSearcher`.
- **ZFS** (`refs-btrfs-zfs-engine-algorithms-v2.md` §3; zfs-vdev--mftmirr--calcpartition.md §1;
  remaining-engines §ZFS): uberblock walk (FUN at corpus line 32300) [CODE]: staged
  0x400-byte window; magic `0x00BAB10C`; **txg (u64 @ +0x10) newest-wins**; 0x80-qword
  (1 KiB) stride = one uberblock slot, walking the 128-slot array (direction semantics
  [INFERRED from comparison polarity]). vdev/nvlist parser (FUN at 21040) [CODE]:
  `device.read(0x1C000, 0x4000)` (label nvlist region); entries 0x40-stride {name,type,value};
  `type == 9 (NVLIST)` + name "vdev_tree" → recursion; `type == 2 (UINT64)` id/asize-family →
  big-endian byte accumulation. `std::map<int64, zio_compress>` per-object compression
  registry (LZJB/LZ4/zle/gzip…). `zfs_struct::` layer: UberData, VdevLoc/VdevDisk, objset,
  ZplNode/ZplNodeFile, CStreamBase.

### 2.6 Lost-partition scoring (CalcPartition family)
`Fat32CalcPartition` (39 vftable refs, FATSearchDLL), `ExFatCalcPartition` (54 refs,
exFATSearchDLL — heaviest single class in its engine), NTFS counterpart rides
CNTFSSearchController's partition logic. These are the per-FS lost-partition candidate
scorers participating in the polymorphic candidate pipeline (`__RTDynamicCast` sites);
QA log evidence `CalcPartition.log` shows `AdjustPartitionInfoOrder` ranking 15+ overlapping
NTFS candidates. **Detailed scoring walk remains an open item — not covered.**
(zfs-vdev--mftmirr--calcpartition.md §3)

### 2.7 What fields each parser extracts for a deleted file
Per the IFileProperties family (the engines construct these objects themselves — statically
linked shared props library compiled into every engine DLL; handoff to DRWUI is by object,
not field copies — fat-exfat-engine-algorithms-v2.md §4):
**name, size (u64 bytes), FLAGS u16 (0x10 = has-children/folder, 0x20 = deleted, 0x01
folder-ish, 0x02/0x04 attribute-ish), creation/last-write/last-access times, marker byte,
parent/directory anchor, type-registry ID, runlist.** Timestamps: NTFS/EXT4/ReFS/HFS+/FAT/
exFAT persist **FILETIME qwords (100ns units since 1601)** — [CENSUS] 100% of NTFS creation
values in 1.19e17..1.34e17 = valid FILETIME range for 2017-2026 dates (ifileproperties-part1
§2); the question "100ns FILETIME?" is therefore YES for structured FS results. **RAW
(carved) files persist NO raw FILETIME** — only a formatted string `"%d-%d-%d %d-%d-%d"`
(WYear/Month/Day/H/M/S) produced by FUN_140c41aa0 from a FILETIME argument
(ifileproperties-part3 §3 + corrections §5.1). Per-FS grammar table: see §4.3 below.

---

## 3. Recovery-chance model (two sides)

### 3.1 Engine-side — bitmap cross-check (NTFS, FUN_18001acc0)
[CODE] (ntfs-engine-algorithms-v2--deleted-viability.md §1; ntfs-engine--deep-dive.md §4)
```
size < 0x3FF (1023 B)     → skip viability walk entirely (fast path; often resident)
size ≥ 0x3FF, not folder  → for each data-run cluster:
    sector = sectors_per_cluster * cluster
    hit    = bitmap.test(sector)          # FUN_18002c2e0 on CNTFSBuildVolumeBitmap
           || secondary_map.contains     # controller +0x60 virtual check
    if hit: reallocated++
if total > 0 and reallocated == total:   # ALL clusters re-allocated
    props.Release(); continue            # → candidate DESTROYED (content overwritten)
# partial → kept and surfaced (RAW analyser + preview validation grade content later)
```
Reading [INFERRED-but-consistent]: a deleted MFT record whose clusters read as allocated in
the live $Bitmap means reuse by other files; the `intact==total → destroy` asymmetry means
partially-overwritten files still surface. The quick-vs-complete split: quick builds
CNTFSBuildVolumeBitmap from $Bitmap to know which clusters are in use; complete adds USN
cross-check + orphan sweep. TRIM status chip "TRIM OFF"/"TRIM ON" on the home list =
SSD-aware chance indicator (UIUX/07-home-phase-deep-dive.md §List rows).

### 3.2 UI-side — CRecoverability (DRWUI.exe)
Decompiled this session from
`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (FUN_14038ac90 @ 605760-605900,
launch FUN_14038aad0 @ 605640-605668) and cross-read with
`re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` §2:
- Pipeline: background threads via `_LaunchPad<tuple<void(__cdecl*)(void*), CRecoverability*>>`
  (std::thread on member fn, decompile line 609369); logged `"Recoverability analyze begin."`
  to `RecoveryAssistant.log` (line 605664); re-fires per selection and on Go-to-Recover
  (UIUX/31-filterbar-family-decode.md — CCheckNodeThread ctor FUN_140ad1100, 0x28 B, started
  with `QThread::start(thread, 7)` = TimeCriticalPriority, `sigFinished→onCheckFinished`).
- Grade dispatch inside FUN_14038ac90 (verified in decompile):
```
if size < 1:                       grade = 0        # FUN_140089170 (size via props vt+0x18)
else if node-type == 0xC           # FUN_140088f00 = *(byte*)(node+0x70) >> 2; ZFS
     and current-grade != 2:       grade = 2        # FUN_140089670 reads grade
else if content-type in {3, 10, 0xB}:               # FUN_140088770 = props vt+0xa0 GetType
     deep-validation path           # (type 0xB = runtime-only props; never persisted)
else:                              generic path (FUN_1403895c0 / FUN_140389400 tree ops)
grade adjustment: device.kind ∈ {1,4} → grade offset −2
                   # device vtable+0xa0 result == 1 or == 4 → local_a8 -= 2 (decompile 605780)
set grade via FUN_140089630(node, grade)
```
- Grade storage [CODE]: 3-bit field in `*(byte*)(node+0x73) >> 5` (FUN_140089630 writes
  `bVar1 & 0x1F | grade << 5` — monotonic, only raises unless grade 0 clears;
  FUN_140089670 reads it). Observed values: 0 = poor/none, 2 = high ("Your files are highly
  recoverable" verdict string adjacent at decompile 1072691; mapping [INFERRED] from UI
  strings + runtime observation).
- Pluggable checker interface: `ICheckRecoverable` (vftables at decompile 438476 / 439200).
- Runtime telemetry: "Quick scan matched=0, total=64", "Recovery ability matched=0,
  total=127" (runs 4×), "Recoverability finish match.." (UIUX/30-interaction-driver-ground-truth.md §77-91).
- Related UI surfaces: recovery-chance column in CListControl header (sortable columns
  Name/Size/Type/Date Modified + chance, UIUX/09-results-phase-deep-dive.md §55);
  `RecoveryChancesTipsWidget` coaching overlays; PreviewFailedWnd grades "But this file is
  likely recoverable based on its structure." with "Recovery Chances: High" (UIUX/10-preview-subsystem.md §30).
- Device-kind −2 semantics beyond the {1,4} membership: **not covered** (which device kinds
  the values 1/4 name is not decoded).
- RecoveryAssistant post-scan stats (FUN at decompile 330837-330960): counts TotalFileCount,
  rawCount, lostPartitionExist/DeleteFileCount, QuickScanExist/DeleteFileCount,
  DeepScanFileCount, per-category counts via FUN_1401c8940 with category IDs
  **Graphics=5, Document=6, Archive=7, Video=8, Audio=9, Other=0xB** — the same type-ID space
  persisted in IFileProperties +0x60.

---

## 4. Session/persistence schema — the .rsf format (FULLY REVERSED)

Primary source: `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` (status
COMPLETE; parser `scripts/rsf_parser.py` + `scripts/rsf_record4_grammar.py` validated against
all 3 shipped samples — 293 MB of decompressed trees, 1,986,806 nodes, byte-exact to EOF).
Companion: `rsf-record4--tree-node-grammar.md`; per-FS field maps:
`ifileproperties-part{1,2,3}*.md`.

### 4.1 Container [CODE, byte-verified on all 3 samples]
```
+0x000  Magic[16]  EA F4 B3 1B 05 43 B6 4A B4 B8 24 4A 33 D9 13 BB
+0x010  u32 0x00001F43 (8003, constant across samples; flags/count)
+0x014  u32 0x0000D739 (55097, constant; likely header CRC/size)
+0x018  u32 1 (format version)
+0x01C..0x083 zero padding (reserved)
+0x084  u32 0x200 (512 — unit size)
+0x088  u32 record count (4)
+0x08C  RECORD STREAM
```
Record header = **36 bytes (0x24)**: Magic[16] + SessionId[16] (4 dwords copied from the
caller's id struct; a session identifier, NOT a crypto nonce) + u32 chain (= byte size of this
record's chunked data area). Next header at `this_header + 0x24 + chain`. Writer FUN_14051af90
(seeks EOF, writes header with chain=0, remembers offset at obj+0x10029; finalize
FUN_14051b2a0 re-writes the 36-byte header with the real chain). Reader FUN_14051b0b0 (seek
0x8C, chain-walk until SessionId matches).

### 4.2 Chunked compression [CODE]
```
chunk := u16le header || payload
header bit 15    = 1 → zlib (deflate) payload; 0 → stored raw
header bits 0-11 = payload_size − 1   (payload ≤ 4096 B)
chunk on disk   = 2 + payload_size
```
Reader FUN_140519ac0: `total = (hdr & 0xfff) + 3`. Writer FUN_14051a190: `hdr = (size−1)
| 0x8000` when compressed. `CZlibCompressor` (factory FUN_1406277e0) init **level 5**
(`init(5, 0x800000)`); input 4 KB blocks from a 512 KB staging buffer at obj+0x120; output
8,190-byte buffer; compressed chunk only emitted if smaller than raw. Decompressor
`czlibdecompressor` (FUN_140627820, 0x58 B state; inflate port constants: state 0x2548,
wbits 15, 32 KB window). Every observed compressed payload begins `78 5E` (valid CMF/FLG,
level-1 class header). On decompression failure the reader **falls back to memcpy of the raw
payload** (graceful degradation). NOT encrypted (the prior CryptoAPI hypothesis was
disproven; the only CryptImportKey+CryptEncrypt site FUN_140c35150 is an LM-hash "KGS!@#$%"
DES generator for NAS auth).

### 4.3 The four records + the 10 IFileProperties classes
Record IDs are compiled GUID constants in .rdata (byte-exact):
| Record | GUID | Content (sample I_) |
|---|---|---|
| 1 session state | `f144ae6e a75cf849 bd8d7126 d446aaca` (DAT_141160c30) | 12 B: u32 entryCount + entries{u64,u64,u32,u8,u64,u32 len,name} + trailer + 4 flag bytes |
| 2 volume/session | `2565c07d 8a638c4a b7d16419 641247db` (DAT_141160c20) | 147 B: u32 sig 0x7F; volumeName[100]; deviceType u32; SYSTEMTIME 16B (GetLocalTime at save); stat1 u64 (elapsed ms); stat2 u64 (hi-res tick counter, ratio ≈ 3.431 GHz); flags |
| 3 partition context | `16f4f594 17436745 ae4415b3 21ed998` (DAT_1414d47c8) | 7,185 B: u32 partitionCount + per partition {0x6F5 B raw partition struct memcpy; +0x14 u32 FS type code (3=NTFS); +0x389 "ST500DM0 02-1BD142 USB Device"} + {0x1518 B volume-device blob: bus type 7, model, serial, firmware, WWN, "USB Device", 512 B boot-sector snapshot incl. 0x55AA} |
| 4 result tree | `6cf208d2 8c07344e 9d71d91f 535f7b57` (DAT_140f64c80) | 36,001,842 B decompressed (I_) — the full found-file tree |

(A 5th constant DAT_1414d47a0 = `76a672b0…` drives a `[552-B struct]+[5400-B blob]` record
via FUN_1403f1180 — a variant save path, absent from the shipped samples.)

**Record 4 tree grammar** (writer `CFileNodeRS::SaveFileTree` = FUN_1400d40c0 @ 186446-186717;
reader `CFileNodeRS::RestoreFileTree` = FUN_1400d4870 @ 186756-187150):
- Two node classes: `CFileNode` (0x93 B, vftable 0x14187fec8, base 34-B grammar) and
  `CFileSystemNode : CFileNode` (0x290 B, vftable 0x14187ff60, full grammar). Marker byte
  before each child = node class (0 = CFileNode, 1 = CFileSystemNode); roots always marker 1
  (reader enforces `!= 1 → abort` @186882). End-of-children sentinel `u32 0xFFFFFFFF`.
- BFS from each root through the intrusive sibling list (children at +0x10 first-child,
  +0x18 next-sibling); children written as marker + node_body + optional content_object;
  `content.flags & 0x10` (has-children) → queue.push. Filter: `content == NULL ||
  GetType() == 0xB` (runtime-only thumbcache/cloud properties) NEVER saved. Roots' inline
  `std::vector<CHN*>` at +0x210 carries volume-shortcut children named `" (I:)"`.
- CFileNode base grammar (34 B): u64 type_flags(+0x50), u64 f_48, 3 flag bits, u32 reserved
  (always 0), 2×u16 defaults 0xFFFF, u8 enum (+0x71; value 2 remapped to 5 on write/back on
  read), duplicate flag byte, 2 more flag bits. CFileSystemNode extension: type_flags
  re-serialized, 40-B stat blob, f_1c8/f_1d0/f_1d4, name_len + name (UTF-8, reader
  NUL-terminates), f_1e4/f_1ec(root-role)/f_1e8/f_1f0/f_200/f_208/f_288/f_289, then inline
  vector recursion. Progress: 40% + 50·processed/(2·remaining) (writer),
  40 + 50·done/(total+done) (reader); cancel via IUIProgressInfo IsCancelled per child.
- **Content objects** — `[u32 type_code][body]` (writer FUN_1400d6fa0 @188981: type from
  vtable+0xa0, body via vtable+0x98; reader factory FUN_1400d6e50 @188913 dispatches to ctor
  + vtable+0xa8):

| Code | Class (vftable) | Serialized grammar (in-memory offsets) |
|---|---|---|
| 0 | CNTFSFileProperties (0x1418944f8, 0x73 B) | u16 len+name(8.3); u64 size(+0x1B); u16 FLAGS(+0x23); u64 created(+0x25); u64 write(+0x2D); u64 access(+0x35); u8(+0x3D); u64 parent-anchor(+0x40); u32(+0x64); u32 type-registry ID(+0x60); u32 hasRun→DataRunList; u64 cnt→cnt×{u64,u64} extent pairs (FUN_140520d20) |
| 1 | CFATFileProperties (0x141897198) | same order; trailing ints at +0x3E (first-cluster/dir-entry anchor) and +0x42 (parent-dir band); runlist ptr +0x46 guarded by std::mutex +0x50 |
| 3 | CRAWFileProperties (0x140f5f528) | u8 shortNameLen+name (analyser-assigned, e.g. shot0017.jpg); u8 tsLen + timestamp STRING "%d-%d-%d %d-%d-%d" (FUN_140c41aa0; raw FILETIME never persisted); u64 size(+0x1A); u16 FLAGS(+0x22) — constant 0x20 (deleted) in all 12,147 census records; u8 marker; u32 analyser checksum/validation(+0x56, 0xFFFFFFFF = invalid); second string; qword header/carve anchor A(+0x8C); qword tail anchor B(+0x94); u32 bool(+0xF4); u32 type-registry ID(+0xF8); runlist UNCONDITIONAL |
| 4 | CExFatFileProperties (0x141896bb8, 0xB8 B) | name @+0x18/+0x20 (UTF-8); u64 size(+0x28); u16 FLAGS(+0x30); created(+0x34)/write(+0x3C)/access(+0x44); u8 marker(+0x4C); u32(+0x50); u32(+0x54); hasRun→runlist (+0x60, mutex +0x68) |
| 5 | HFSPlusFileProperties / HFSPlusSystemNodeProperties (0x141898720 / 0x1418985d8, pair selected by node marker) | name (u32 len); u64 size; u16 FLAGS; created/write/access; u16 secondary; u32 type id; u8 marker; u32 count + count×16 B extent pairs (HFSPlusFileDataRun) |
| 6 | RefsFileProperties (0x1418995a8) | u16 name; u64 size(+0x11); u32 objectId/parent(+0x39); created(+0x19)/write(+0x21)/access(+0x29); u8 marker(+0x3D); hasRun→DataRunList (reader 0x100 name cap) |
| 7 | CBtrfsFileProerties [sic] (0x141893378, 0xB6 B) | name(+0x53); u64 size(+0x4B); FLAGS(+0x77); times(+0x5B/+0x63/+0x6B); u8(+0x73); qword inode/parent(+0x79); u8 +0xB5 RUNLIST-FORMAT SWITCH (0 = DataRunList blob; != 0 = raw inline compressed-extent blob {u32 id, u32 len, bytes}); u32×2 secondary ids; qword generation/transid(+0x81) |
| 8 | CEXT4FileProperties (0x141896ff8) | u16 strLen(+0x21)+std::string SSO name; u64 size(+0x31); FLAGS(+0x39); times(+0x3B/+0x43/+0x4B); u8(+0x53); u64 inode/parent(+0x55); hasRun→DataRunList |
| 9 | CGenericFileProperties (0x1418885c0) | u32 strLen + section name; u16 FLAGS(+0x38) — constant 0x10 (folder); u16 secondary(+0x3A) |
| 0xB | (never persisted — writer filters GetType()==0xB; reader has no case) | runtime-only thumbcache/cloud family |
| 0xC | ZfsFileProperties (0x1418849f8) | ONE leading u64 (name length — corrected); name (SSO); u64 size; u16 FLAGS; created(+0x48)/write(+0x50)/access(+0x40); u16 secondary; polymorphic `zfs_struct::ZfsDataRunlist` (0x80 B, own vtable +0x18/+0x20) |

- **FLAGS u16 semantics**: bit 0x10 = has-children (drives the BFS queue; assembly-verified
  `movzwl 0x23(%rcx)` etc.); bit 0x20 = deleted (84% of I_ NTFS census); 0x01/0x02/0x04
  attribute-ish [CENSUS+CODE, labels partially INFERRED].
- **DataRunList sub-format** (FUN_14051e3a0 write @851580 / FUN_14051e440 read @851612):
  `[u32 byte_len][runlist bytes][u16 flags]` where the runlist is a **raw NTFS-style datarun
  byte string** (header nibbles: high = offset-field size, low = length-field size; walker
  `pos += (b & 0xF) + (b >> 4) + 1`; 0x00 terminator). Runs stored verbatim so restore can
  replay them against the still-present volume.
- Empirical census: I_ = 267,886 nodes (248,727 NTFS + 12,147 RAW + 6,959 Generic + 39 FAT
  content objects; 260,843 dataruns; roots "Local Disk(I:)", "File Path Lost",
  "Reconstructed"); H_ = 1,500,407 nodes / 226.7 MB. Two type-code spaces coexist (important
  for replication): **FS codes at tree construction (FUN_14063b4e0 "File Path Lost" root
  factory): 0/1/2=FAT, 3=NTFS, 6=RAW, 7=exFAT, 8=HFS+, 9=EXT4, 10=ReFS, 0xB=Btrfs** vs
  **record-4 persistence codes: 0=NTFS, 1=FAT, 3=RAW, 4=exFAT, 5=HFS+×2, 6=ReFS, 7=Btrfs,
  8=EXT4, 9=Generic, 0xC=ZFS**.
- File naming: `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` auto-saves; SaveScan folder =
  module dir + "SaveScan" (FUN_1402737f0); FUN_140286930 sanitizes `/ * : ? " < > |` → `_`.

---

## 5. Cache policies

### 5.1 CStorageDeviceWithCache (volume IO cache)
- Class is RTTI-present in NTFSSearchDLL (decompile line 7167: `__RTDynamicCast(…,
  &IStorageDevice::RTTI_Type_Descriptor, &CStorageDeviceWithCache::RTTI_Type_Descriptor)`)
  and named in the NTFS RTTI census (ntfs-engine--deep-dive.md §RTTI; ntfs-engine-internals.md
  "IO cache layer"). Volume reads flow `CStorageDeviceWithCache → IStorageDevice`, which is
  the `\\.\EPRRTDRV\{d}\{p}` EPMNtDrv IOCTL path for damaged/locked volumes and the normal
  file API for healthy ones, "with a read cache in front to survive MFT re-reads
  economically" (ntfs-engine--deep-dive.md §8). RAWSearchDLL reads flow through the same
  CStorageDeviceWithCache-style volume IO (rawsearch-carving--deep-dive.md §4).
- **Buffer sizes / TTL / eviction policy: not covered** — no decompiled cache
  implementation, LRU structure, or TTL constant appears anywhere in the corpus. Known
  adjacent buffer facts (NOT a cache policy): NTFS MFT reads in 64 KB batches / 0x1000
  pages; quick-search 0x200 sector staging; 4 KB chunk buffers in .rsf; CHFSPlusFileCache =
  "hot-node cache" (class-census only, no internals).

### 5.2 Thumbnail pipeline
- UI side: `CGetListThumbnailThread` (async, per-row thumbnail batch fetcher;
  DRWUI-exe--main-ui-architecture.md §86; DRWUI-exe--thread-architecture.md §21 — "every
  non-trivial operation (sort/filter/classify/select/thumbnail/navigation) is off-thread").
- Preview socket protocol (VAPreviewListener, DRWUI-subsystems--assistant-grading-preview.md
  §4) [CODE]: request `SyncWait4DataBack` (654520) — send via ws2_32, poll up to 100×100 ms
  (10 s bounded wait), response id correlation, `payloadSize = resp.size(+0x4) − 0x25A ∈
  (0, 0x100000]` (≤ 1 MiB), `GlobalAlloc(GMEM_MOVEABLE, (size+0xDA5) & ~0xFFF)` page-rounded
  HGLOBAL, 602-byte header stripped. Receive `RecvThumbnailBmpData` (654595): fixed 602-byte
  MediaInfo header first (`recv(fd, buf, 0x25A)`; short read → 0xFFFFFFFF), size validation
  (bad → 0xFFFFFFFE), streaming continuation loop (incomplete → 0xFFFFFFFD). Contract:
  **[602-byte media-info header][≤ 1 MiB BMP thumbnail]**, own _LaunchPad thread.
- **Thumbnail on-disk cache / factory / TTL / eviction: not covered** (no thumbnail cache
  store is documented in the corpus; the pipeline is socket-fetched per request).

### 5.3 DRWCfg.bin (AES-256-CBC config cache)
Source: `re-workspace/artifacts/config-files/drwcfg-bin--aes-config-cache.md` (byte-exact;
decompile sites FUN_1404f4a90 ctor @ ~826087 / 398147 / 398301; writer/reader at
DRWUI_decompiled.c 398090-398400, DRWCfg.bin literal at lines 398112/398264).
- Class `AES_CBC256`: **hardcoded key (32 B) = `XZJE151628AED2A6ABF7158809CF4F3C`**,
  **IV (16 B) = `XZJ2030405060708`** (48 ASCII bytes written into the object after the
  vftable; encrypt reads IV from +0x28). Key schedule FUN_14032d4a0(this+8, 0x100, 256-B
  expansion). Obfuscation, not security (key embedded in every shipped copy).
- Writer: serializes config items into a heap buffer of **0x80-byte (128) records**
  (iterator over the map at param_1+0x1b8), pads to next 16-byte multiple
  (`((size+0xF)/0x10)*0x10`), encrypts, `CreateFileA("…\DRWCfg.bin", GENERIC_READ|WRITE,
  share 3, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL)` → WriteFile → FlushFileBuffers. On failure
  the plaintext buffer is freed and the file is not written. Reader mirrors: decrypt,
  unpad, iterate 0x80-byte records (0x14-byte key identifiers memcmp'd, 0x32-byte value
  update loop).
- Stored semantics: persistent settings map (per-scan state, feature flags, MRU values
  surviving restarts). **TTL: none (explicit CREATE_ALWAYS rewrite); eviction: not covered
  (the corpus does not decode the item map's replacement policy).**

### 5.4 Other persistence-side caches
- `BtrfsCache.txt` — BtrfsSearchDLL persists a metadata cache file in the DLL's directory to
  speed re-scans of the same volume (subvolume/chunk trees cached); string sites
  BtrfsSearchDLL_decompiled.c 1991/2003/7266/7272. **Content schema / TTL / eviction: not
  covered.**
- `remote_config_data` (62,579 B) — Firebase Remote-Config snapshot cache: `fetched`
  timestamp + token stream, keys plain strings, values **1-byte-length-prefixed**
  (`\x04true`, `\x05false`); ~600 key/value flags; the local fallback when fetch fails
  (userdata-remoteconfig--signature-and-flags.md §2). TTL = startup re-fetch attempt,
  fallback to snapshot.

---

## 6. Signature tables

### 6.1 raw_signs.json (RAW signature DB #1)
File: `og-codebase/raw_signs.json` (34 KB). Recomputed this session (direct JSON parse):
- **139 extensions** (matches config-data-reference.md §"raw_signs.json" and its independent
  verification note).
- Per-extension schema: `{"header": {"signs": ["offset,HEXBYTES,confidence", …]},
  "tail": {"signs": [...]}, "min": bytes, "max": bytes}`.
- **139/139 have header signs (199 header sign rows total); 46 extensions also have tail
  signs (62 tail rows). Confidence range 0.50-1.00.**
- Example rows (verbatim):
  - `"itc2"`: header `["2,011c69746368000000020000000200000002000000006172747700000000,1.00"]`,
    min 1, max 51200 (no tail).
  - `"cpr"`: header `["8,4e554e44524f4f54,0.54"]`, tail
    `["1,01000000000000000400000006466c61677300000100000000000000,0.69"]`, min 10240,
    max 204800.
  - `"ifo"`: header `["0,44564456,0.96", "0,445644564944454f2d,0.90",
    "0,445644564944454f2d56,0.84"]`, min 10240, max 204800 (DVD-Video IFO: "DVDV" at
    offset 0, graded confidences 0.96/0.90/0.84).
- Semantics: header/tail hex patterns + confidence + min/max size bounds prune absurd
  candidates; tail = footer detection → exact file end; combined header+tail scoring ranks
  overlapping carves (searchdll-family--architecture.md §4.4; config-data-reference.md
  §raw_signs). Consumer: RAWSearchDLL via the dedicated istream tokenizer (NOT CJsonObject —
  correction in rawsearch-carving--deep-dive.md §3); gated per-extension by
  config/RawConfig.ini (37 toggles). No `jpg`/`jpeg` entry exists in raw_signs.json
  (recomputed: `d.get('jpg')` → None) — JPEG is handled purely by the hardcoded
  analyser/registry path; the JSON covers formats WITHOUT dedicated C++ analysers
  (itc2, prproj, nes, rpgmvo, apk, blp, resw…).

### 6.2 EUUserData (RAW signature DB #2 — magic + MIME)
File: `og-codebase/EUUserData` (16,891 B). Recomputed this session: **111 per-extension
entries** (config-data-reference says "111 entries" in the userdata-remoteconfig artifact
and "16KB"; the byte-verified note in config-data-reference gives 16,891 B). Format:
`{".ext": {"signs": ["offset,HEX"], "mime": "..."}}` — libmagic-style, same `offset,hex`
grammar as raw_signs.json but **extensible at runtime** (the file behind the product's
"add custom file type / signature" feature; the UI custom-format flow feeds this JSON).
Examples (verbatim): `"doc": signs ["0,0D444F43" (DF in Cyrillic OEM = Word6), "0,CF11E0A1B11AE100"
(OLE GUID)], mime application/msword (plus a `512,ECA5C100` variant per
config-data-reference); `"epub": ["0,504B03040A000200"]`; `"gz": ["0,1F8B08"]`;
`"mxf": ["0,060E2B34020501010D0102010102" (SMPTE KLVP), "0,3C435472616E7354696D656C696E6533E"
(<CTransTimeline> XML fallback)]; `"123": ["0,00001A00051004"]` (Lotus 1-2-3). The
signature choices are sharper than raw_signs.json (MXF full KLVP key + XML fallback; DOC
gets the OLE GUID). MIME types included for display/classification. Legacy long tail:
Lotus/Notes/WordPro, Palm, mac-binhex40, tcpdump pcap.
Consumer: file-type identification for preview + repair routing
(userdata-remoteconfig--signature-and-flags.md §1; config-data-reference.md §EUUserData).

---

## 7. FileFilter.xml + PriorityDisplayType.xml decode

### 7.1 FileFilter.xml — result-category tree (membership)
File: `og-codebase/FileFilter.xml` (54 KB). Recomputed this session (ElementTree parse):
- **8 FileCategory elements, 1,489 `<Extention>` entries total** (the "Extention" typo is in
  the XML tag itself; count matches config-data-reference's independent verification).
- Per-category `<Extention>` counts (recomputed): Graphic 320 (ShowComment file.jpeg,
  iconIndex 0; 40+ camera RAW formats nef/arw/cr2/cr3/rw2/dng/heic under Graphic/TIFF group),
  Document 432 (file.pmd, iconIndex 1), Email 19 (file.eml, iconIndex 2; pst/dbx),
  Audio 281 (file.wav, iconIndex 3), Video 217 (file.WMA, iconIndex 4), **Other 0**
  (file.dll, iconIndex 5, `IsMutex=1` — catch-all/mutex category with no direct extensions),
  Archive 219 (file.zip, iconIndex 6), Bookmarks 1 (file.bookmarks, iconIndex 7).
- Each node carries a `Selected` state flag (0=unchecked, 1=partial, 2=checked) — UI tree
  state. Consumer: DRWUI `CAdvanceFileFilterMgr` (Filter dropdown) + `CClassify*` tree
  controllers (Type view).

### 7.2 PriorityDisplayType.xml — Type-tree ordering contract
File: `og-codebase/config/PriorityDisplayType.xml`. Recomputed this session:
- **5 categories, 35 `<ext>` entries, each with an `alias` attribute encoding a 2-digit sort
  key** (`00jpg`, `01png`, …). Structure: `<root>` → `<type Description="…">` →
  `<ext alias="NNxxx">xxx</ext>`.
- Ordered extensions per category: **Picture (9)**: jpg(00), png(01), jpeg(02), gif(03),
  tif(04), eps(05), psd(06), bmp(07), ico(08). **Video (7)**: mp4, mov, avi, swf, m4v, fla,
  wmv (00-06). **Document (7)**: doc, docx, pdf, xls, xlsx, ppt, pptx (00-06). **Audio (7)**:
  mp3, wav, wma, m4a, ape, flac, aac (00-06). **Archive (5)**: zip, rar, 7z, tar, gzip (00-04).
- Decoded semantics (filtering-priority--config-layer.md §2): the alias strings are the sort
  keys the Type tree uses to order extensions WITHIN each category — this is why the results
  Type-tab always shows jpg before png before jpeg. The 5 categories here are the *priority
  tier*; FileFilter.xml's 8-category/1489-ext map is the *complete* filter set:
  **FileFilter.xml = membership, PriorityDisplayType.xml = presentation order.**

### 7.3 The full filtering pipeline (config → behavior)
```
scan results
  → FilterFolder.ini        (noise suppression: $Windows.~BT/WS/TMP, Config.Msi, desktop.ini,
                             .DS_Store, $UsnJrnl, MountPointManagerRemoteDatabase; the
                             localized "New Folder" filter in 10 languages; System Volume
                             Information; BitLocker recovery-password artifacts
                             AadRecoveryPasswordDelete/ClientRecoveryPasswordRotation/
                             FveDecryptedVolumeFolder; TxtViewFiles text-preview whitelist
                             ~119 extensions; SumaryFileType family groups
                             m4p_m4a_m4v_mp4, zip_docx_pptx_xlsx)
  → FileFilter.xml         (category membership: 8 categories / 1489 ext)
  → PriorityDisplayType.xml (presentation order within category)
  → SumaryFileType          (family grouping for counts)
  → RawConfig.ini           (deep-scan carve scope, per-format toggles: HEIC/WEBM/7Z/MOBI/…
                             =1; STL/VPJ =0; DRP/OVP/HFP/OSP/WFP = video-editor project files)
  → usefullostfiles.xml     (27 ext "valuable formats" headline set: 12 photos incl. RAW
                             camera formats, 9 videos, 6+ documents)
```
Each stage is config-driven (hot-tunable by support without code changes)
(filtering-priority--config-layer.md §6). Type-registry IDs roll up into top-level categories
5=Graphics/6=Document/7=Archive/8=Video/9=Audio/0xB=Other (RecoveryAssistant census; the .rsf
census showed `.jpg`→6/26/3 = multiple signature variants per extension).

---

## 8. Replication checklist — what the product must reproduce

1. **Type detection**: two-layer dispatch (u16 magic key → u32 4CC registry → raw_signs.json
   signs → EUUserData); per-analyser numeric type-IDs (0x0B BMP … 0x7C HEIC) feeding
   IFileProperties +0x60 → category buckets 5/6/7/8/9/0xB → FileFilter.xml membership +
   PriorityDisplayType alias ordering.
2. **Size detection**: computed by grammar walk (ZIP sum-of-entries, PST 0xA8/0xB8,
   brute-force last-valid-position), bounded by raw_signs min/max, extended through
   sector-aligned zero padding (ZIP 0x500001 cap).
3. **Chance**: engine-side bitmap cross-check (all-reallocated→destroy, partial→keep,
   <0x3FF skip) + UI-side CRecoverability (size<1→0; type 0xC→2; content types {3,10,0xB}
   deep-validation; device kind ∈{1,4} → −2; 3-bit grade at node+0x73 bits 5-7).
4. **Name**: 8.3 (NTFS/FAT), UTF-8 (exFAT/EXT4), LFN reassembly (FAT 0x40/0x0F),
   exFAT UTF-16 fragments via backward triple walk, carved names assigned by the UI
   callback ("File0001.jpg" policy in DRWUI, not the engine), sanitizer `/ * : ? " < > |`→`_`.
5. **Metadata**: per-FS props = name/size/FLAGS/times(FILETIME 100ns)/marker/parent-anchor/
   type-ID + runlist (NTFS nibble dataruns verbatim).
6. **Path**: parent-reference chains (NTFS quick scan), directory recursion (FAT), "File
   Path Lost" root for unreconstructable parentage (FPL roots get SetName("File Path Lost"),
   SetFlags(0x10), +0x3d=9), "Reconstructed" root, LostDir naming `%s%02d`.

## 9. Explicit "not covered" register
- Numeric confidence-combination formula among overlapping signature candidates (only the
  field semantics + "combined header+tail scoring ranks" statements exist).
- Dispatch ORDER between the u16/u32 registry tables and raw_signs.json ([INFERRED] in
  raw-dual-registry §1).
- LFN checksum routine byte-level decode (standard algorithm referenced, state machines
  cited by line only).
- ext4 htree (dx_dir) walk — class census + family conventions only.
- CStorageDeviceWithCache internals (sizes/TTL/eviction).
- Thumbnail cache store (none documented; socket pipeline only).
- DRWCfg.bin item-map replacement policy; BtrfsCache.txt content schema.
- ExFatCalcPartition / Fat32CalcPartition detailed scoring walks (open items in the corpus).
- Device-kind values {1,4} semantics in CRecoverability's −2 adjustment.
- ZFS uberblock walk direction rationale ([INFERRED] from comparison polarity).
- CDoubleFATTable primary/mirror fallback switch semantics ([INFERRED]).
- The 10th/5th analyser-ID tables beyond the census batches (106 classes counted; IDs
  decoded only for the batch-1 set + dual-registry set).
