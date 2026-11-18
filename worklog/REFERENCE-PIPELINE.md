# REFERENCE-PIPELINE — How the Reference (EaseUS Data Recovery Wizard) Streams Results, Identifies, Sizes, Grades, Names, Persists, and Caches

> Phase 0 parallel read (Agent B). Ground-truth corpus: `/home/z/my-project/repos/easeus-drw-research`
> (architecture/ numbered docs, re-workspace/artifacts/ RE artifacts, og-codebase/ original binaries).
> Every claim carries a citation (corpus path + section/heading). Where the corpus does not
> cover a question, it says so explicitly. Written 2026-11-18.

---

## 1. End-to-end scan→result flow (sector read → engine → detection → tree → UI)

Cast of components and the full pipeline, stage by stage:

1. **Supervisor**: `DRW.exe` (DRWLoader) launches `DRWUI.exe`, watchdogs it via
   `DRW_Crash_{GUID}` events, restarts on crash (architecture/04-recovery-workflow.md §0 "Cast
   of components"; artifacts/DRW-exe--launcher.md).
2. **UI + orchestration**: `DRWUI.exe` (Qt 5 Widgets, 119,182 functions) hosts everything:
   `CScanMgr` (IScanMgr, singleton FUN_140149c30) drives a strategy family
   `CDoScanDeviceBase → CDoScanPartition / CDoScanLostPartition / CDoScanCloudDevice /
   CDoScanRemoteDevice` (44 v-methods each) which run the engine loop
   (artifacts/UIUX/08-scan-phase-deep-dive.md §6; artifacts/DRWUI-exe--main-ui-architecture.md §3.3).
3. **Engine loading (in-proc plugin)**: DRWUI `LoadLibraryW`s all 10 engine DLLs
   (NTFS/FAT/exFAT/Ext2/EXT4/HFSPlus/ReFS/Btrfs/ZFS/RAW SearchDLL) into globals
   `DAT_1425e6d30..78` (FUN_14056c0d0); quick scan resolves the FS code
   (0-2=FAT, 3=NTFS, 4-5=EXT2, 7=exFAT, 8=HFS, 9=EXT4, 10=ReFS, 13=ZFS, +Btrfs) to
   `GetProcAddress(h, "EUCreate<FS>QuickSearcher")(&p)` (FUN_140577d40); advanced scan
   (FUN_140580720) creates the full searcher set + `EUCreateRAWSearcher` (gated by settings
   +0x60) and registers a naming/filter `std::function<string(string,string)>` callback into
   the RAW engine via vtable+0x48 (artifacts/DRWUI-exe--main-ui-architecture.md §9.2;
   architecture/05-scan-engine-architecture.md §2).
4. **Volume IO (sector reads)**: engines read through `CPartitionVirtualVolumeDevice` →
   `CStorageDeviceWithCache` → `IStorageDevice`: normal file API on healthy mounted volumes,
   raw IOCTL path `\\.\EPRDTDRV\<disk#>\<part#>` via kernel driver `eprdtdrv.sys` (EPMNtDrv)
   for damaged/lost volumes — MDL async sector IO, 512-byte units; BitLocker volumes get FVE
   keys via `\\.\Pipe\DRWBitlocker` (DRWService.exe) and XTS-AES decryption; NAS via EuMrx
   redirector (architecture/05-scan-engine-architecture.md §8; architecture/04-recovery-workflow.md
   §4a; artifacts/SearchDLL/ntfs-engine-internals.md "IO & robustness").
5. **Detection (per FS engine)**: structured FS walks — NTFS MFT walk
   (CNTFSSearchController, 0x940 bytes, fixup-aware record enumeration, $ATTRIBUTE_LIST fork,
   $Bitmap cross-check); FAT/exFAT FAT-chain + directory-entry walks; ext inode/extent trees;
   HFS+ catalog B-trees; ReFS/Btrfs/ZFS metadata walks (architecture/04-recovery-workflow.md
   §4a; artifacts/SearchDLL/ntfs-engine--deep-dive.md §2-§4). In parallel, RAWSearchDLL
   carves by signatures "sector by sector" (§4b; artifacts/SearchDLL/rawsearch-carving--deep-dive.md).
6. **Tree building (engine side)**: each engine controller IS its tree builder via multiple
   inheritance — primary base `ICompleteFileSystemSearchController`, secondary base
   `IDirecotryTreeBuilder` [sic] → `DetourDirectoryTree` decorator (intercepts node insertion
   to feed progress + dedup + raw/structured merge) (architecture/05-scan-engine-architecture.md
   §3, constructor vtable sequence in HFSPlusSearchDLL).
7. **Tree ingestion + organize (UI side)**: `CDirectoryTreeManager`/`CDirectoryTreeBuilder`
   (+ `CBtrfsQuickTreeBuilder`, `IRawDirectoryTreeBuilder` for carved) ingest engine trees;
   then "Organizing Found Files": classification `CSetClassifyThread` (FileFilter.xml),
   validation `CCheckNodeThread(s)` ("SearchDataSource::OnCheckNode"), filtering
   `CSetFilterThread` + `CAdvanceFileFilterMgr`, sorting `CSortNodeThread`, thumbnails
   `CGetListThumbnailThread` — all background QThreads so the UI never blocks
   (architecture/04-recovery-workflow.md §5; architecture/05-scan-engine-architecture.md §7;
   artifacts/UIUX/09-results-phase-deep-dive.md §7).
8. **Presentation**: `CDetailPage` (stack idx 3) hosts both the scan view and results — left
   tree with 5 special branches (Tags Extracted / File Name Lost / File Path Lost / Versions
   / Unimportant) + classify tabs, right list, browse-while-scanning
   (artifacts/UIUX/09-results-phase-deep-dive.md §2/§7;
   artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md §2-§3).
9. **IPC inventory between components**: engines are in-proc DLLs (vtable contracts, no
   IPC); cross-process channels exist only for: DRWLoader↔DRWUI (named events + 2MB shared
   log, artifacts/DRW-exe--launcher.md), DRWService FVE pipe (BitLocker keys,
   artifacts/DRWService-exe--bitlocker-service.md), media preview helper over localhost TCP
   20171/20172 + ZeroMQ (VAPreviewListener, §2 below), EuRdr `\\.\EuRdrDN` IOCTL protocol for
   NAS (artifacts/DRWUI-exe--main-ui-architecture.md §9.3), and out-of-proc office/PDF viewers.

## 2. How results stream during scan (mechanism, cadence, ordering)

- **The engine→UI event bridge (the core streaming mechanism)**: MOM-side engine callbacks
  post `(int msg, void* data, int& ret)` messages into the service hub
  (FUN_14026b8d0 → FUN_14026bc20); `CMessageProxy` (singleton QObject + `INotifyObserver`
  MI) receives them in its static notify shim (FUN_14089f970), constructs a temporary
  `CMessageSend` envelope (24 B), and emits `CMessageSend::dispatch(int, void*, int&)` —
  a Qt signal whose connect is chosen by a thread-affinity probe
  (`QThread::currentThread() == QCoreApplication::instance()->thread()`): same-thread =
  direct-style, cross-thread = queued marshaling. The envelope is destroyed per message
  ("create → connect → emit → destroy"), so engine threads never touch Qt objects directly
  (artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md §1 — "the engine→UI
  bridge, CMessageProxy, INotifyObserver, CMessageSend", with all function cites).
- **Fan-out**: `doDispatchMessage(int,void*,int&)` is the single app-wide bus signal; 15 UI
  classes subscribe identically via `connect(bus, doDispatchMessage, this, onReceiveData)`
  (wiring sites listed in artifacts/UIUX/25-connect-wiring-graph.md §2). Engine messages =
  "progress, file counts, tree updates" (same doc §2).
- **Progress is pull-polled, events are pushed**: 38 named `timeout()` consumers poll
  progress (`onGet*Progress`); scan/finish/completion events are push signals
  (`sigQuickScanFinished(int)` ×2, `sigScanFinished()` wired to 7 slots,
  `sigLoadFinished(bool)`) (artifacts/UIUX/25-connect-wiring-graph.md §3.1/§3.3 and §6 item 4).
- **Cadence / batching**: engines read MFT in 64 KB batches (0x10000 buffers) and process
  records in 0x1000-granular pages; per-batch walkers (e.g. NTFS deleted-record batch loop
  FUN_18001acc0) check a cooperative cancel callback per record; batch sizes adapt to RAM
  (GlobalMemoryStatus) (artifacts/SearchDLL/ntfs-engine-internals.md "MFT acquisition"/"IO &
  robustness"; artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §1).
- **Incremental/live update proven at runtime**: browse-while-scanning is fully interactive;
  the "Reconstructed" (RAW carve) branch grows live from 1 → 16 files during the advanced
  scan, while the structured branches are already stable — "direct runtime proof that RAW
  carving results stream into the tree as found" (artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md
  §7 ADDENDUM: long-run capture, table t=75s vs t≈460s). Tree/list updates run through the
  worker-thread family so the models update off the UI thread
  (artifacts/UIUX/13-ui-core-wiring-state.md §3-§4).
- **Ordering (two-phase, not priority-weighted categories)**: quick pass (FS metadata) runs
  first and its results render immediately; the advanced pass (all FS engines full-mode +
  RAW carving in parallel) then streams additional results into the separate "File Path
  Lost"/"Reconstructed" branches. Priority ordering of *categories* is a display concern —
  the Type tree order is config-driven (PriorityDisplayType.xml aliases), not scan-emission
  order (artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md §3 item 1 + §7;
  artifacts/config-files/filtering-priority--config-layer.md §2).
- **Scan-completion pipeline** (runtime-verified verbatim log): ClearCache cascade →
  `Start Freeze` → auto `.rsf` save → `Unfreeze` → `CDoScanPartition::ScanThread:483 →
  UpdateScanFinishedInfo:498 → CScanMgr::ScanFinished:636` → recoverability pass
  (CCheckNodeThread "Recovery ability matched=..") (artifacts/UIUX/30-interaction-driver-ground-truth.md §5).
- **Not covered by corpus**: an explicit numeric batching interval (e.g. "emit every N
  nodes" or a fixed ms cadence for tree refresh) is not documented; only the batch sizes of
  the disk reads and the signal architecture are evidenced.

## 3. File-type identification (signature registry, precedence, validators)

Three cooperating layers (architecture/05-scan-engine-architecture.md §6; artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md §1/§5):

1. **C++ analyser registry (magic-key dispatch)** — `std::map<uint32 magic_dword, Analyser*>`
   (red-black tree) built by a static initializer; 106 instantiated analyser classes across
   namespaces (BruteForce 24, RawSpeed 20, xjq 8, Brance 22, first-generation File*Analyser 46,
   standalone modern); 70 unique magic keys byte-decoded (e.g. 0x46445025 `%PDF`,
   0x474e5089 PNG, 0x04034b50 ZIP, 0xeeffd8ff JPEG). Exposed via `EUGetAnalyserList/Map/Multimap`
   (artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md §3/§4;
   artifacts/SearchDLL/rawsearch-carving--deep-dive.md §2). Each registry node carries a
   format/type ID (e.g. DMG=0x71) and an optional **two-generation fallback chain** (modern
   namespaced analyser → legacy File*Analyser) — two independent validators must both fail
   before a candidate is discarded (census §2).
2. **raw_signs.json (139 extensions, 34 KB)** — per-extension
   `{"header": signs["offset,HEXBYTES,confidence"], "tail": signs, "min": bytes, "max": bytes}`;
   139/139 have header signs, 46 also tail signs; confidence 0.50–1.00 per signature; loaded
   at engine init by a hand-rolled istream JSON tokenizer (FUN_180021460/21380 — not
   tinyxml2/CJsonObject for this file). It drives header/tail scanning for formats WITHOUT
   dedicated C++ analysers; the engine consults BOTH: magic-key registry first, then the JSON
   signs for unclaimed extensions (artifacts/config-files/config-data-reference.md "raw_signs.json";
   artifacts/SearchDLL/rawsearch-carving--deep-dive.md §3; census §5).
3. **EUUserData (16,891 B, 111 extensions)** — user-extensible signature DB,
   `{".ext": {"signs": ["offset,hex"], "mime": "..."}}`, same offset,hex grammar as
   raw_signs.json but runtime-extensible (the "add custom file type" feature); libmagic-style
   magic+MIME typing used for preview/repair routing (artifacts/config-files/userdata-remoteconfig--signature-and-flags.md §1;
   config-data-reference.md "EUUserData").
4. **Validators / structure walking**: hardcoded analysers compute the file END by walking
   structure — PDF `%PDF-x.y` version parse, JPEG marker walk incl. SOF0 dimension checks
   (0xC0, precision 8), ZIP central-directory walk with three accepted anchors (PK0304/0102/0506)
   and 5.24 MB zero-padding extension cap, PST generation-specific size fields (0x0E ANSI @+0xA8 /
   0x17 Unicode @+0xB8), EBML vint grammar for MKV/WebM, HEIC brand discrimination
   (artifacts/SearchDLL/rawsearch-carving--deep-dive.md §3;
   artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §1-§3;
   artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md §1-§4).
5. **Precedence (magic > structure > extension)**: dispatch is magic-key first, then
   structural validation (dual validation: registry magic + internal re-validation, e.g. WPG
   0x677077 re-check) — extension-based typing is the LAST resort and is a UI-side concern
   (FileFilter.xml categorization), not a carving decision (census §6 note 2;
   zip-analyser §5 "Dual validation"). **Not covered by corpus**: a formal written
   "precedence: magic > structure > extension" statement; this ordering is inferred from the
   dispatch architecture above (census §5 explicitly: "magic-key registry first (C++ chain),
   then the JSON signs for unclaimed extensions").

## 4. Size computation (FS metadata vs carver vs format fields vs fallback)

- **FS metadata (structured results)**: size comes from the FS record itself — NTFS
  $DATA attribute (resident length checks clamped ≤ 0xFFF; non-resident datarun extents),
  $FILE_NAME attribute fields; FAT/exFAT from chain/FAT-table semantics; the shared
  `IFileProperties` object carries `+0x1B` = size for NTFS/FAT (serializer field map)
  (artifacts/SearchDLL/ntfs-engine--deep-dive.md §6; artifacts/SaveScan/rsf-format--scan-session.md §5.4.5 table).
- **Carver header/footer**: raw_signs.json tail signs give exact file ends for the 46
  extensions that have them; min/max size gates "prune absurd candidates"
  (artifacts/config-files/config-data-reference.md "raw_signs.json").
- **Format-specific fields**: PST reads the spec's dwTotalFileSize at +0xA8 (ANSI) / +0xB8
  (Unicode); ZIP sums parsed entries ("the final carve size is computed from the SUM of
  parsed entries, not guessed") with sector-aligned padding extension; ZIP indexless result
  carries status 0xd (partial) vs 0x48 (full carve with CD); BruteForce analysers walk to
  EOF validating structure at every step and report the last valid position as the size
  (the "maximize salvage" trade-off) (artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md §1;
  artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md §2-§4;
  artifacts/SearchDLL/rawsearch-carving--deep-dive.md §5).
- **Fallback**: unknown PST versions floor at 1 MiB; ZIP initial size estimate is
  sector_size << 7 (64 KB) as an initial guess only; CRAWFileProperties persists a u64
  file size at +0x1A (corrected from an earlier FILETIME misread)
  (analyser-granular-batch1 §1; zip-analyser §2; rsf-format §5.4.5 corrections note).

## 5. Recovery-chance model (grading inputs)

- **Engine-side at scan time (NTFS)**: every deleted file's data runs are cross-checked
  against the volume $Bitmap (CNTFSBuildVolumeBitmap, bit test FUN_18002c2e0):
  `intact/total` readable-extent ratio per file; if ALL clusters are re-allocated → candidate
  destroyed; partially-intact ones still surface. Files < 1 KiB skip the walk (often
  resident). This is "the per-file readable-extent ratio that the UI later surfaces as
  recovery chances/quality" (artifacts/SearchDLL/ntfs-engine--deep-dive.md §4;
  artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md §1/§3).
- **UI-side grader**: `CRecoverability` — background-thread pipeline per node:
  size < 1 → grade 0; node type 0xC (ZFS) → 2; types 3/10/0xB (NTFS/ReFS/Btrfs) → deep
  validation path (device+fixer-aware); else generic path; grade offset −2 for device kind
  ∈ {1,4}; grade stored via FUN_140089630. Grades observed at runtime: 0 = poor/none,
  2 = high ("Your files are highly recoverable"). `ICheckRecoverable` is the pluggable
  checker interface; the CCheckNodeThread recoverability pass runs at scan finish and
  re-fires per selection ("re-grading telemetry") (artifacts/DRWUI-subsystems--assistant-grading-preview.md §2;
  artifacts/UIUX/30-interaction-driver-ground-truth.md §5 recoverability pass rows).
- **Display**: the "Recovery Chances: High" circular gauge in the preview panel is this
  grading visualized; there is also a recovery-chance column in the list header
  (artifacts/UIUX/09-results-phase-deep-dive.md §5; artifacts/UIUX/05-uiux-architecture.md
  §12 v2 addendum "Recovery Chances: High" circular gauge = CCheckNodeThread grading visualized).
- **Inputs**: overwritten sectors (bitmap cross-check), fragmentation (data-run extent
  counts), device kind, node/FS type, size (artifacts/DRWUI-subsystems--assistant-grading-preview.md §2;
  ntfs-algorithms-v2 §1). **Not covered by corpus**: a fully enumerated grade ladder with
  all intermediate values/labels (only grades 0 and 2 are runtime-observed; the
  verdict-string↔grade mapping is flagged INFERRED in the artifact's Confidence section).

## 6. Name/metadata/path reconstruction

- **NTFS**: names from the $FILE_NAME attribute (0x30) parsed by
  `CNTFSFileRecordFileNameAttribute` — "names, parent references, timestamps, DOS+Win32
  namespace variants"; paths reconstructed from parent-reference chains in quick scan;
  "names recovered from $FILE_NAME even when parent dirs are gone (reparenting into lost
  tree)"; orphaned-record sweep in complete scan; MFT-mirror ($MFTMirr) cross-reference as
  repair/verification source (artifacts/SearchDLL/ntfs-engine-internals.md "Record
  processing"/"Quick vs Complete"; artifacts/SearchDLL/ntfs-engine--deep-dive.md §5;
  searchdll-family--architecture.md §5).
- **FAT**: LFN reassembly — 0x40 last-logical-entry flag + 0x0F attribute checks,
  "LFN fragments are reassembled checksum-verified against their 8.3 alias"; deleted 8.3
  entries first byte 0xE5 (six decompiled sites) with a 64 KB chain-length sanity guard
  (artifacts/SearchDLL/fat-engine--deep-dive.md §4).
- **exFAT**: dentry triplets File (0x85) + Stream Extension (0xC0) + File Name (0xC1) —
  "the parser state machine pairs Stream+Name entries"; deleted = invalid-flagged File
  dentries still paired when both fragments survive; NoFatChain contiguous-run support
  (artifacts/SearchDLL/exfat-engine--deep-dive.md §3).
- **Carver naming**: carved files are named at discovery time by a DRWUI-supplied
  callback — `std::function<string(string,string)>` registered via vtable+0x48; the
  "File0001.jpg"-style naming policy lives in the UI, not the engine
  (artifacts/SearchDLL/rawsearch-carving--deep-dive.md §4; artifacts/DRWUI-exe--main-ui-architecture.md §9.2).
  Carved entries carry extracted names where the format self-describes them (RAR analyser
  walk carries a 272-byte filename buffer extracting entry names; ZIP analyser returns the
  complete inner file inventory) (analyser-granular-batch1 §2; zip-analyser §2).
- **Tree/path assembly**: the result tree has three verified roots — "Local Disk(I:)"/volume
  root, "File Path Lost" (carved/deleted files whose parentage could not be reconstructed)
  and "Reconstructed" (lost-file rebuilds) — live-verified matching the .rsf record-4 parse
  (artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md §3;
  artifacts/SaveScan/rsf-format--scan-session.md §5.4.6). UI branches: "File Name Lost"
  ("original name cannot be found, but are more likely to be your desired files" — carved
  by content) and "File Path Lost" ("the file path was fully or partially missing");
  a "Versions" node for NTFS file-record multi-version recovery
  (artifacts/UIUX/09-results-phase-deep-dive.md §2). Merge rule: "structured nodes win;
  carved nodes attach as 'Lost'/'Other deleted files'" (architecture/05-scan-engine-architecture.md §7.3).
  Engine-side "LostDir" naming string present in NTFS engine strings
  (artifacts/SearchDLL/searchdll-family--architecture.md §5).
- **Collision rules**: the only documented collision handling is the .rsf file-NAME
  sanitizer — `/ * : ? " < > |` replaced with `_` for save-file names
  (rsf-format--scan-session.md §5.5, FUN_140286930), and the multimap signature-collision
  resolution for carving (several analysers claim the same magic — e.g. 4 JPG magics, TIFF
  II* claimed by TIFF + camera-RAW chains) (analyser-registry-census §1). **Not covered by
  corpus**: on-disk-result duplicate-name collision policy (e.g. "file(1).jpg" renaming
  during recovery export) is not documented.

## 7. Local persistence (.rsf session format, checkpoints, resume, database)

- **Format (fully reversed + validated)**: magic `EA F4 B3 1B…` (16 B) + header fields; a
  session = up to 4 records, each with a 36-byte record header (magic + SessionId[16] +
  chain size) followed by chunked data. Chunks: `u16le header || payload ≤ 4096 B`, bit 15
  = compressed (zlib deflate level 5 via `CZlibCompressor`/`czlibdecompressor`), bits 0-11 =
  size−1; stored form used when compression isn't smaller; decompression failure falls
  back to memcpy of raw payload (graceful degradation) (artifacts/SaveScan/rsf-format--scan-session.md §2-§3).
  **Not encrypted** (corrections register §1 — the prior CryptoAPI hypothesis disproven).
- **Record schemas**: record 1 session state (12 B sample); record 2 volume/session info
  (147 B: 0x7F sig + volumeName[100] + deviceType + SYSTEMTIME + 2 stat qwords + flags);
  record 3 partition + volume-device context (7,185 B: 1781 B partition struct + 5400 B
  device blob incl. 512 B boot-sector snapshot — the "device identity card" for restore
  verification); record 4 = the FULL found-file tree (BFS serialization
  `CFileNodeRS::SaveFileTree`/`RestoreFileTree`, CFileNode 0x93 B / CFileSystemNode 0x290 B,
  IFileProperties content family — 10 per-FS classes with per-FS grammars, NTFS-style hex
  dataruns; validated byte-exact on 3 shipped samples: 1,986,806 nodes total)
  (rsf-format--scan-session.md §5.1-§5.4; artifacts/SaveScan/rsf-record4--tree-node-grammar.md).
- **Auto-save + resume**: scan completion triggers auto-save `auto_<Volume>_<MM_DD_HH_MM_SS>.rsf`
  in the SaveScan folder (runtime log verbatim; FUN_140273940 enumerates auto_*.rsf);
  "Continue last scanning" banner on the home page resumes .rsf sessions; "Load Scan
  Session"/"(1-Import Scan Result" import paths (artifacts/UIUX/30-interaction-driver-ground-truth.md §5;
  rsf-format §5.5; artifacts/UIUX/05-uiux-architecture.md §2.2 "Continue last scanning").
- **Checkpoints / cancellation**: the record-4 save walks the tree with a progress callback
  (40% + 50·done/(2·total) — the save progress maps 40→65%) and an IsCancelled check after
  every child (rsf-format §5.4.4; rsf-record4--tree-node-grammar.md "progress"). The
  CRecoveryMgr export chain has its own `P[%04d]:T[%04d]` progress checkpoints
  (DRWUI-subsystems--assistant-grading-preview.md §3). **Not covered by corpus**: a
  mid-scan crash-resume/checkpoint behavior (resume is whole-session .rsf load, not
  partial-scan checkpointing); no SQL/embedded database is documented anywhere —
  persistence is the .rsf file format only.

## 8. Caching

- **`CStorageDeviceWithCache`** — the engine volume-IO read cache in front of raw/file IO,
  "to survive MFT re-reads economically" (artifacts/SearchDLL/ntfs-engine--deep-dive.md §8;
  ntfs-engine-internals.md "IO & robustness").
- **Runtime result caches with explicit clear**: `SearchDataSource::ClearCache`,
  `PreviewDataSource::ClearCache`, `ThumbnailFactory::ClearCache` (×3 sites),
  `OfficePreviewStub::doKillAllOpenProcess` — fired as a cascade at scan completion before
  the Freeze/auto-save (runtime log, artifacts/UIUX/30-interaction-driver-ground-truth.md §5).
- **Thumbnails**: async per-row `CGetListThumbnailThread`; media thumbnails stream over the
  localhost TCP preview channel ([602-byte MediaInfo header][≤1 MiB BMP]) with
  request/response id correlation and 10-second bounded wait; `ThumbnailFactory` is the
  producer (artifacts/UIUX/13-ui-core-wiring-state.md §3;
  artifacts/DRWUI-subsystems--assistant-grading-preview.md §4).
- **Signature/config caches**: raw_signs.json + RawConfig.ini loaded once at engine init;
  DRWCfg.bin = AES-256-CBC (hardcoded key) persistent config cache of 0x80-byte records
  (per-scan-state, feature flags, MRU values that survive restarts; obfuscation, not
  security) (artifacts/SearchDLL/rawsearch-carving--deep-dive.md §3;
  artifacts/config-files/drwcfg-bin--aes-config-cache.md §2-§3).
- **Remote-config cache**: `remote_config_data` = shipped Firebase Remote-Config snapshot
  (~600 flags, 1-byte-length-prefixed values), the offline fallback
  (artifacts/config-files/userdata-remoteconfig--signature-and-flags.md §2).
- **Runtime-only node properties**: IFileProperties type 0xB (thumbcache/cloud classes) are
  never persisted in .rsf — writer filters GetType()==0xB children (rsf-format §5.4.5 table).
- **Not covered by corpus**: eviction policies/TTLs/sizes for any of these caches are not
  documented.

## 9. Classification (category model, prioritization, system-file deprioritization)

- **Category model**: FileFilter.xml — 8 FileCategories (Graphic, Document, Email, Audio,
  Video, Other, Archive, Bookmarks; UI shows Graphics/Video/Audio/Document/Email/Web&Others/
  Archive/Folders) with 1,489 `<Extention>` [sic] entries; UI tab clicks emit per-category
  telemetry (`Click_Audio/Document/Photo/Video/Archives/Emails/Databases/Bookmarks`)
  (artifacts/config-files/config-data-reference.md "FileFilter.xml";
  architecture/05-scan-engine-architecture.md §7.4; artifacts/UIUX/09-results-phase-deep-dive.md §3).
- **Two-level type system**: top-level category IDs 5=Graphics, 6=Document, 7=Archive,
  8=Video, 9=Audio, 0xB=Other (RecoveryAssistant space, same IDs persisted in
  IFileProperties +0x60) with per-format sub-IDs beneath (e.g. PST 0x38, RAR4 0x3A, HEIC
  0x7C, BMP 0x0B) (artifacts/DRWUI-subsystems--assistant-grading-preview.md §1;
  artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md §5).
- **Prioritization**: PriorityDisplayType.xml — 5 display categories, 35 extensions, each
  with an `alias` 2-digit sort key (`00jpg`, `01png`, `02jpeg`…) ordering extensions within
  a category in the Type tree; usefullostfiles.xml = the 27-extension "valuable formats"
  whitelist (photos incl. camera RAW, videos, documents) that promotes carved RAW photos
  out of "Unimportant" (artifacts/config-files/filtering-priority--config-layer.md §1-§2;
  artifacts/UIUX/09-results-phase-deep-dive.md §3).
- **System-file deprioritization**: engine-side $-prefixed NTFS metafiles skipped during
  the MFT walk (`name[0] == '$'`); flags & 4 (temp/system) records skipped; $Extend
  hierarchy ($RmMetadata/$Reparse/$Repair) "system files classified separately from user
  data" (artifacts/SearchDLL/ntfs-engine--deep-dive.md §3 step 7;
  ntfs-engine-internals.md "Record processing"). UI-side: the "Unimportant" collapsed junk
  branch ("To help you find the desired files quickly, we organize some unimportant files
  here") (artifacts/UIUX/09-results-phase-deep-dive.md §2 item 5); FilterFolder.ini hides
  OS noise (desktop.ini, .DS_Store, $UsnJrnl, $Windows.~BT/WS/TMP, localized "New Folder"
  in 10 languages, BitLocker recovery artifacts, System Volume Information)
  (artifacts/config-files/filtering-priority--config-layer.md §3).
- **Junk/config-driven pipeline order**: noise suppression → category membership →
  presentation order → family grouping (SumaryFileType, e.g. m4p_m4a_m4v_mp4 as one family)
  → carve scope (RawConfig.ini per-format toggles) → valuable-formats emphasis
  (filtering-priority--config-layer.md §6 synthesis diagram).
- **Per-format carve scope**: RawConfig.ini `[Filter]` FORMAT=1/0 per extension — maps 1:1
  onto the analyser registry's standalone classes; "letting support toggle carving per
  format without a rebuild" (filtering-priority §4; rawsearchdll--analyser-registry-census.md cross-ref).

---

## Source index (all citations resolve under /home/z/my-project/repos/easeus-drw-research/)

| Corpus path | Used for |
|---|---|
| architecture/00-INDEX.md | artifact map |
| architecture/04-recovery-workflow.md | §0 cast, §3 wiring, §4 scan, §5 organize, §8 persistence |
| architecture/05-scan-engine-architecture.md | §2 plugin contract, §3 controller=tree-builder, §6 RAW, §7 merge/classify, §8 IO |
| architecture/05-uiux-architecture.md | §2 journey, §12 v2 gauge, §12 waves |
| architecture/07-user-flow-uiux.md | §3 scan screen, §4 results screen |
| re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md | §1-§7 progress wiring, CScanCommonPopDlg |
| re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md | §2 branches, §3 classify, §7 progressive fill |
| re-workspace/artifacts/UIUX/13-ui-core-wiring-state.md | §3 engine→UI path, §4 threads |
| re-workspace/artifacts/UIUX/20-moc-surface-sink-architecture.md | §6 Freeze protocol, CUISink hub |
| re-workspace/artifacts/UIUX/25-connect-wiring-graph.md | §2 MOM bus, §3.1 scan flow, §3.3 polling convention |
| re-workspace/artifacts/UIUX/30-interaction-driver-ground-truth.md | §5 completion pipeline log |
| re-workspace/artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md | §3 3-branch tree, §7-§8 live streaming/complete |
| re-workspace/artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md | §1 CMessageProxy/CMessageSend bridge |
| re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md | §3.3/§3.4 classes, §9.2 plugin contract, §9.5 preview IPC |
| re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md | §1 category IDs, §2 CRecoverability, §4 preview protocol |
| re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md | §1-§7 family contract, engine table |
| re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md + ntfs-engine-internals.md + ntfs-engine-algorithms-v2--deleted-viability.md | MFT walk, bitmap scoring, quick-vs-complete |
| re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md | §1-§5 registry, raw_signs, naming callback |
| re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md | §1-§6 magic table, analyser census |
| re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md | §1-§5 size reconstruction model |
| re-workspace/artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md | §1-§5 per-format validators + type IDs |
| re-workspace/artifacts/SearchDLL/fat-engine--deep-dive.md + exfat-engine--deep-dive.md | §4 LFN/0xE5; §2-§3 bitmap/dentries |
| re-workspace/artifacts/SaveScan/rsf-format--scan-session.md + rsf-record4--tree-node-grammar.md | §2-§5 format, records, tree grammar, auto-save |
| re-workspace/artifacts/config-files/config-data-reference.md | raw_signs.json, FileFilter.xml, EUUserData, Dict |
| re-workspace/artifacts/config-files/filtering-priority--config-layer.md | §1-§6 filter pipeline |
| re-workspace/artifacts/config-files/userdata-remoteconfig--signature-and-flags.md | §1 EUUserData, §2 remote config |
| re-workspace/artifacts/config-files/drwcfg-bin--aes-config-cache.md | §2-§3 config cache |

## Explicitly NOT covered by the corpus

- A numeric emit cadence / batch interval for tree updates (only architecture + read-batch sizes evidenced).
- A written "magic > structure > extension" precedence statement (inferred from dispatch architecture).
- The full recovery-grade ladder beyond observed 0/2 (verdict mapping flagged INFERRED in the artifact).
- Duplicate-name collision policy at recovery export (only .rsf save-name sanitizer and carving multimap collision resolution documented).
- Mid-scan crash-resume/checkpointing (resume = whole-session .rsf); no database of any kind.
- Cache eviction/TTL policies (caches exist, policies not documented).
