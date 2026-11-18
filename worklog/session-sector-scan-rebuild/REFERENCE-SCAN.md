# REFERENCE-SCAN — Sector-Scan Ground Truth (Agent B Context Sheet)

> READ-ONLY survey of the EaseUS DRW reference corpus (`/home/z/my-project/repos/easeus-drw-research`).
> Every claim carries an artifact citation (path + section/line). Claims the corpus does not
> support are marked **UNVERIFIED**. Companion sheets in this session directory cover the
> product side; this sheet is the reference (ground truth) side only.

---

## 1. The exact scan flow (quick → deep/advanced → organize)

### 1.1 Phase order and engine wiring

1. **Scan start**: `CScanMgr::doScan` ("Scan drive:I, Partition type:3 (NTFS)") →
   `CDoScanPartition::PushScanDevice / PushFolderFilter / DoScan`; the vendor QA log shows
   `Quick scan StartSector=705794048, TotalSector=270977023` then
   `Deep scan StartSector=705794048 (same range)` — **quick and deep scan the SAME sector
   range; deep is not a subrange** [config-files/qa-logs--session-forensics.md §2, lines 25-29].
2. **Quick scan** = single-FS metadata pass. DRWUI maps the volume's FS type code to one
   factory: FS codes 0-2=FAT, 3=NTFS, 4-5=EXT2, 7=exFAT, 8=HFS, 9=EXT4, 10=ReFS, 13=ZFS
   (+Btrfs) → `GetProcAddress(h, "EUCreate<FS>QuickSearcher")(&p)` returning
   `IQuickFileSystemSearchController*` [architecture/04-recovery-workflow.md §3.2;
   DRWUI-exe--main-ui-architecture.md §9.2 table]. NTFS quick = $MFT record enumeration
   including deleted records (fixup-aware) with $Bitmap cross-check
   [SearchDLL/ntfs-engine--deep-dive.md §5 table, line 108].
3. **Quick completion** (~21 s on the vendor's 129 GB NTFS I: partition —
   [config-files/qa-logs--session-forensics.md §2, line 30]) raises
   `sigQuickScanFinished(int)` → `onQuickScanCompleted(int)` ×2 = "the two-phase gate"
   [UIUX/25-connect-wiring-graph.md §3.1, lines 69-70]. UI shows `CQuickScanCompleteWnd`:
   "Quick Scan Completed / You may now / Recover Found Deleted %1 files %2" with the tip
   "Advanced Scan is trying to find more other lost files for you while recove[ring…]" —
   the advanced scan **continues automatically in the background while the user may already
   recover** [UIUX/08-scan-phase-deep-dive.md §4].
4. **Advanced/Deep scan** (`FUN_140580720`) creates the FULL searcher set —
   `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher` — appends each to a `std::list`, plus
   `EUCreateRAWSearcher` when the settings flag at +0x60 enables carving, and registers a
   `std::function<string(string,string)>` naming/filter callback into the RAW engine
   (vtable+0x48). "Advanced Scan is searching files sector by sector..." (UI string).
   **Advanced = ALL FS engines in full mode + RAW carving engine** [architecture/04-recovery-workflow.md §3.3;
   DRWUI-exe--main-ui-architecture.md §9.2, lines 255-261]. The RAW searcher is registered
   LAST in the engine list [SearchDLL/rawsearch-carving--deep-dive.md §4].
5. **Lost partitions** are inserted as additional scan targets
   ("Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d"), sector size
   default 512 [architecture/04-recovery-workflow.md §3.4;
   DRWUI-exe--main-ui-architecture.md §9.2 lines 263-266].
6. **Finish**: `sigScanFinished()` fans out to retranslate, page re-broadcast, freeze
   protocol entry, `onScanFinishRefreshUI`, `onDoAutoRecover`, `onScanFinish`,
   `scanFreezeFinished` [UIUX/25-connect-wiring-graph.md §3.1 lines 71-76]. Completion
   pipeline (runtime log, verbatim order): cache-reset cascade → **Freeze** →
   `Build tree finish` → **auto-save `auto_<Volume>_<MM_DD_HH_MM_SS>.rsf`** → **Unfreeze** →
   `CDoScanPartition::ScanThread:483 → UpdateScanFinishedInfo:498 →
   CScanMgr::ScanFinished:636` → `bottomarea.cpp:838/856 nStatus=4, m_iScanType 3→4`
   (quick→advanced completion transition rendered live) → **recoverability pass**
   ("Quick scan matched=0, total=64" + "Recovery ability matched=0, total=127" ×4 runs →
   "Recoverability finish match..") [UIUX/30-interaction-driver-ground-truth.md §5 table,
   lines 60-94].
7. **"Organizing found files..."** phase after engines finish = tree building
   (`CDirectoryTreeManager`, `CSetClassifyThread`/`CSortNodeThread` workers) — modal offered
   as "Stop Organizing Found Files?" [UIUX/08-scan-phase-deep-dive.md §3].

### 1.2 What each phase reads

- **Quick**: FS metadata only. NTFS: DBR parse → $MFT datarun read in 64 KB batches →
  per-record fixup/magic validation → deleted branch (in-use flag clear) → bitmap
  cross-check of data runs [SearchDLL/ntfs-engine-internals.md §MFT acquisition +
  ntfs-engine--deep-dive.md §3]. Complete scan additionally builds the full $Bitmap volume
  bitmap, cross-checks the USN journal ($UsnJrnl strcmp sites 9070/19761/19868/20282) and
  $MFTMirr, and sweeps orphaned records [SearchDLL/ntfs-engine--deep-dive.md §5 table +
  verification ledger].
- **Deep/advanced**: all FS engines in full (complete) mode **in parallel** + the RAW
  carving pass reading sectors ("sector by sector") via the same volume IO
  [DRWUI-exe--main-ui-architecture.md §9.2; SearchDLL/rawsearch-carving--deep-dive.md §4].
- **Carve (RAW)**: signature-keyed (RB-tree `std::map<uint32 fourcc, Analyser*>`) +
  raw_signs.json header/tail signs with confidence + min/max size gates; per-format C++
  grammars compute the file END by walking structure
  [SearchDLL/rawsearch-carving--deep-dive.md §2-3].

### 1.3 UI-visible timeline (runtime captures)

| t | State | Evidence |
|---|---|---|
| t=0 | click "Search for Lost Data" → scan view opens, browse-while-scanning active | [UIUX/dynamic-ground-truth--scan-phase-captured.md §2] |
| t≈75 s | **"Advanced Scanning, 9%"**, Files Found 11xx; Quick pass already DONE (1.11M files in Windows (C:) branch); live "Searching: <file path>" ticker | [same §3, point 1 + status line] |
| t≈460 s | **34%**, Files Found 1,139,157 (143.74 GB); `Remaining time: 07m43s \| Reading sector: 134316032/31413598`; **Reconstructed branch grew 1→16 live (carving streams in)**; File Path Lost stable at 4.3-4.4K (structured-loss phase finished early); Pause/Stop controls + "Recover All..." available mid-scan | [same §7 addendum table + verified behaviors 1-3] |
| t≈880 s | **SCAN COMPLETE** modal ("Congratulations! Scan Completed!" + "highly recoverable" + Go to Recover); bottom bar "Scan Completed, Files Found: 1139286 (147.12 GB)" | [same §8] |

Full session 14.7 min for a 149.45 GB volume; ~98.5% of disk capacity enumerated; quick
pass ≈21 s (matches vendor QA log) [same §8 final numbers]. Status strings:
"Quick Scanning%1, Files Found:" / "Advanced Scanning%1, Files Found:" (string-table lines
10314/10318) [same §3 point 1]. Scan-phase chrome: circular % gauge, Pause (‖), Stop (■),
status line, remaining-time + sector ticker, "Searching:" strip, Recover All CTA —
pixel-verified positions [UIUX/35-redesign-reference-atlas.md §6, lines 170-182].

---

## 2. Sector read model

### 2.1 Volume-IO layers (who reads the disk)

```
Engine tree reader
  └─ CPartitionVirtualVolumeDevice (works on lost partitions)
       ├─ normal file API when volume is mounted & healthy
       ├─ raw IOCTL path → EPMNtDrv (\\.\EPRDTDRV\<d>\<p>) — MDL async sector IO, 512B sectors
       ├─ BitLocker: FVE keys (DRWService pipe) → XTS-AES decrypt
       └─ NAS: EuMrx redirector letter
```
[architecture/05-scan-engine-architecture.md §8, lines 118-125]

- `CStorageDeviceWithCache` — an IO **read cache layer** in front of `IStorageDevice`
  ("to survive MFT re-reads economically") [SearchDLL/ntfs-engine-internals.md class model
  line 25 + §IO; ntfs-engine--deep-dive.md §8 lines 145-149].
- Healthy volumes use the normal file API; damaged/locked volumes the raw IOCTL path
  (capa located the DeviceIoControl site @0x18002C0BA in NTFSSearchDLL)
  [architecture/04-recovery-workflow.md §4a; ntfs-engine-internals.md §IO].

### 2.2 Read sizes / granularity

- **MFT reads: 64 KB batches** (0x10000 stack buffers = 16×4KB or 64×1KB records),
  **cluster-strided** (aligned blocks, not per-record) [SearchDLL/ntfs-engine-internals.md
  §MFT acquisition line 33; ntfs-engine--deep-dive.md §8 lines 150-151].
- **FAT engine: 0x200-byte (512 B) sector staging buffer** at controller +0xA0; sector-size
  defaulting `param_3 == 0 ? 512 : …` [SearchDLL/fat-engine--deep-dive.md lines 13-16].
- Memory-adaptive: GlobalMemoryStatus logging — "batch sizes adapt to RAM"
  [SearchDLL/ntfs-engine-internals.md §IO & robustness].
- Record-size clamp ≥ 0x400 (1024 B, NTFS default) per record [ntfs-engine--deep-dive.md §3].

### 2.3 The EuRdr/EPMNtDrv driver protocol (eprdtdrv.sys)

- Device `\Device\EPMNTDRV`, symlink opened as `\\.\EPRDTDRV\...`; `DO_DIRECT_IO`
  (MDL-based) flag set; 39 functions, pool tag "esdr"
  [PartitionRecovery/eprdtdrv-sys--driver-internals.md §1].
- **Path resolution**: `\\.\EPRDTDRV\{DiskNum}\{PartNum}` resolves to
  `\Device\Harddisk{D}\Partition0` = **always the whole-disk device**; "All sector
  addressing is caller-side absolute LBA on the whole disk — the scan engines compute
  partition-relative offsets themselves". `...\Volume\{N}` resolves to the volume stack
  [same §2 branches A/B + meaning note].
- **IRP_MJ_READ/WRITE contract**: user `ReadFile()` length expressed in **sectors of 512
  bytes** (hardcoded), file offset = absolute byte offset (LBA×512); the driver maps the
  MDL, builds an **asynchronous FsdRequest** (`IoBuildAsynchronousFsdRequest`) to the
  target stack with completion routine `FltReadWriteSectorsCompletion`, waits on event if
  PENDING, and on success sets `IoStatus.Information = Length << 9`
  [same §3 code block + contract paragraph].
- Three custom IOCTLs (0x60002400 global target, 0x60002404 GetFileObjName 520-byte
  MAX_PATH out, 0x60002408 GetDiskContextMode) + **transparent passthrough of any other
  IOCTL to the disk stack** (geometry queries, SCSI passthrough) [same §4 table].
- 512-byte sector assumption: "user-mode must adapt to 4Kn/512e drives (sector-size
  normalization happens above the driver — `InitSectorSize` in EUDCPEPM)"
  [same §3 note]. Sector-size default 512 also set by CGenericRecoveryController
  [architecture/04-recovery-workflow.md §3.4].
- Four identical deployments (eprdtdrv.sys / eprdtdrv_win7older.sys / ebrntdrv.sys /
  epmdkdrv.sys — same PDB epmntdrv.pdb) [same §8].

### 2.4 Overlapped/async IO model above the driver — mostly UNVERIFIED

- Documented: engines use Vista+ **condition variables** for worker coordination
  (InitializeConditionVariable / SleepConditionVariableCS / WakeAllConditionVariable,
  GetProcAddress-resolved with pre-Vista fallback) [SearchDLL/ntfs-engine-internals.md
  §IO & robustness]; the driver-side IO is asynchronous-with-wait (see §2.3); DRWUI scan
  workers are QThreads (42-thread census) [MOM-modules/mom-modules--ui-bridge... §1
  architecture reading].
- **UNVERIFIED**: the corpus does not decode a user-mode OVERLAPPED/ReadFileEx loop, a
  queue depth, a read-ahead pipeline, or FILE_FLAG_NO_BUFFERING usage in the engines. No
  artifact states the concurrency factor of parallel sector reads. Any such claim for the
  rebuild must be marked as an engineering choice, not reference behavior.

---

## 3. Real-time streaming cadence (results during scan)

### 3.1 The event path (engine → UI)

Engine side → MOM service hub (`FUN_14026b8d0` singleton accessor) → message service →
`CMessageProxy` (singleton, MI: QObject + `INotifyObserver` @+0x10) → static notify shim
`FUN_14089f970` constructs a stack **`CMessageSend` envelope per message** (create →
connect → emit → destroy) → `CMessageSend::dispatch(int, void*, int&)` Qt signal →
`CMessageProxy::doDispatchMessage` → **15 UI subscriber classes** wired
`connect(bus, doDispatchMessage, this, onReceiveData)` [MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md
§1 diagram + §4]. The bridge is a **thread-safe adapter**: the `CMessageSend` ctor probes
thread affinity (`QThread::currentThread() == QCoreApplication::instance()->thread()`) and
selects direct-style vs queued-style marshaling — engine threads never touch Qt objects
directly [same §1 dual-branch note].

### 3.2 Cadence — push for events, poll for progress

- **Scan/finish events push** (signals); **progress percentages pull** via 38 named
  `timeout()` consumers (polling timers); "The only progress-push signal is
  `sigPercentChange` (CUISink's)" [UIUX/25-connect-wiring-graph.md §6 point 4].
- Engine callbacks post progress/message events into the service hub →
  `doDispatchMessage` fans out to page components; `ScaningRecoverMessage` updates status
  text; `AnalysisGridWidget` paints the sector map; file-count milestones emit telemetry
  [UIUX/13-ui-core-wiring-state.md §3].
- **Tree-build batching**: the vendor's own DefaultLog.txt shows
  `FinishSearch → "prepare to create 3 new file system tree" → CreateNewFileSystemTree →
  BuildTree cycles: "show file system node" ⇄ "build raw file node" (incremental batches
  ~1000 items)` — the builder processed a 223M-item cumulative workload in ~1000-item
  incremental batches [config-files/qa-logs--session-forensics.md §4]. (These cycles are
  at engine-finish tree assembly; the mid-scan streaming path is signal-driven.)
- **Behavioral proof of streaming**: Reconstructed branch grew 1→16 during 6 minutes of
  advanced scanning — "direct runtime proof that RAW carving results stream into the tree
  as found"; UI fully interactive mid-scan [UIUX/dynamic-ground-truth--scan-phase-captured.md
  §7 verified behavior 1; §3 point 2].
- Progress UI objects: `CUIScanProgressInfo` ("%1 file chunks scanned...", "%1 files %2
  have been found.") [architecture/04-recovery-workflow.md §5.6].
- **UNVERIFIED**: exact per-file vs per-batch emission rate for the found-count ticker
  (e.g. events/second) is not decoded anywhere in the corpus. The observable contract is:
  counters update "live" (see 1.3), tree grows in ≤~1000-item batches, progress percent is
  poll-driven.

### 3.3 Tree-build / classify / filter merge pipeline

1. Engine controllers ARE the tree builders (multiple inheritance; `DetourDirectoryTree`
   decorator intercepts node insertion — "used to feed progress + dedup + raw/structured
   merge") [architecture/05-scan-engine-architecture.md §3].
2. Merge: **structured nodes win; carved nodes attach as "Lost"/"Other deleted files"**
   (`LostTreeModel`/`LostTableModel`) [architecture/05-scan-engine-architecture.md §7.3;
   rawsearch-carving--deep-dive.md §4].
3. Classification `CSetClassifyThread` (FileFilter.xml 8 categories / 1,489 ext) →
   4. Validation `CCheckNodeThread` / `SearchDataSource::OnCheckNode` (per-node
   recoverability, extents readable through volume IO) →
   5. Filtering `CSetFilterThread` + `CAdvanceFileFilterMgr` (FilterFolder.ini junk rules;
   extension/size/date) →
   6. Sorting `CSortNodeThread`, selection `CSelNodeThread`, thumbnails
   `CGetListThumbnailThread` — all off the UI thread so the tree never blocks
   [architecture/05-scan-engine-architecture.md §7.4-7; UIUX/09-results-phase-deep-dive.md §7].
- The recoverability pass **re-fires during user browsing/selection** (async grading
  re-runs as rows are selected) and again on Go to Recover [UIUX/30-interaction-driver-ground-truth.md
  §5 table row 3 + §9.3].

---

## 4. Progress weighting

### 4.1 What the reference corpus actually establishes

- **Within-scan progress is sector-throughput based**: the remaining-time estimate
  (7m43s at 34%) "implies a linear sector-throughput model — matches the sector-based
  progress decoded in the engines (bitmap/sector progress objects)"
  [UIUX/dynamic-ground-truth--scan-phase-captured.md §7 verified behavior 2]. The live
  sector ticker `Reading sector: 134316032/31413598` is the numerator/denominator of that
  model [same §7 + §2 status bar]. (Note: the observed numerator exceeds the denominator —
  the capture records the verbatim UI text; whether it counts absolute disk LBA vs
  partition-relative sectors is **UNVERIFIED**.)
- **Phase-owned % ranges of the scan progress bar are NOT decoded in the reference
  corpus.** The dynamic captures only show "Advanced Scanning, 9%" at t=75 s and 34% at
  t≈460 s [same §3, §7]. The corpus contains no statement like "quick owns 0-40%".
  **UNVERIFIED (reference side).**
- **The 40→65% mapping is about .rsf SESSION SAVE, not the scan**: during record-4 tree
  serialization the save progress reports `start = 40% (0x28)`, `percent = 0x28 +
  (processed * 0x32) / (total * 2)` — climbs 40% → 65% across record 4; i.e. the SAVE
  progress bar spends 40% on records 1-3 + 25% on the tree walk + remainder on
  compression/finalize [SaveScan/rsf-record4--tree-node-grammar.md §5, lines 93-100].
  Do not conflate this with scan-phase weights.
- Quick pass duration ≈21 s on a 129 GB NTFS volume, whole session 14.7 min / 149 GB —
  i.e. quick ≈2-3% of wall time, advanced ≈97% [config-files/qa-logs--session-forensics.md
  §2 line 30 + dynamic-ground-truth--scan-phase-captured.md §8].

### 4.2 The product-side 40/35/25 weights (final-prod worklog, for disambiguation)

- The "quick(40)+deep(35)" note is the **product's own worklog** (BF-22): "75.0% is the
  EXACT quick(40)+deep(35) boundary" [final-prod worklog BEHAVIOR-FINDINGS.md line 36].
  The product's engine encodes `StageWeights { quick=40; deep=35; carve=25 }`
  (cr_scan.h:72-76), `weighted_progress = Σ(wᵢ·pmᵢ)/100`; with the product's stage ORDER
  quick→carve→deep: quick ends at 40%, carve ends at 65%, deep carries 65→100%
  [final-prod worklog CORE-API.md lines 47, 154, 222, 225; VERIFICATION.md §"V2 Product
  scan stages" lines 34-49].
- **No reference-corpus artifact states 40/35/25 or any per-phase percent ownership for
  the SCAN bar.** The reference evidence is only: (a) the sector-linear model above,
  (b) the observed 9%@75s / 34%@460s advanced-phase samples, (c) the rsf SAVE 40→65%
  mapping. If the rebuild keeps 40/35/25 it is inheriting the prior product-engine
  decision (itself not traceable to the reference corpus) — mark accordingly.

### 4.3 Progress display surfaces

- Circular % gauge (blue ring), status line "Advanced Scanning, Files Found: N (size)",
  "Remaining time:" + "Reading sector: X/Y", "Searching: …" strip
  [UIUX/35-redesign-reference-atlas.md §6 lines 174-179]. Paused state: "Scan Paused" +
  red "Scanning Paused." strip, gauge frozen at pause % [same §6 states]. Stop-confirm
  dialog with stat tiles (files found + used time) [same]. Partition-level smart scan
  uses "Scanning disk sectors: %1" [UIUX/08-scan-phase-deep-dive.md §2].

---

## 5. Result classification + system-file deprioritization

### 5.1 Category model and type ordering

- **FileFilter.xml** (54 KB): 8 FileCategories — Graphic, Document, Email, Audio, Video,
  Other, Archive, Bookmarks — with **1,489 `<Extention>` entries** (typo in the tag
  itself); includes 40+ camera RAW formats under Graphic/TIFF; `Selected=2` state flags
  (0=unchecked, 1=partial?, 2=checked) per node; consumer `CAdvanceFileFilterMgr` +
  `CClassify*` tree controllers [config-files/config-data-reference.md §Filtering lines
  28-33 + verification note lines 93-98].
  - NOTE: architecture/04 §5.2 and architecture/05 §7.4 quote the categories as
    "Graphics, Video, Audio, Document, Email, Web&Others, Archive, Folders" with counts
    (Graphics 213, Video 76, Audio 62, Document 302, Email 25, Web&Others 12, Archive 59
    = 1,489) [architecture/05-scan-engine-architecture.md §7.4]. The config-data-reference
    byte-verified parse says Graphic/Document/Email/Audio/Video/Other/Archive/Bookmarks.
    The **byte-verified parse (8 categories / 1,489 ext) is authoritative on names**; the
    per-category counts appear only in the architecture docs. Treat category-name spelling
    as: use the config parse; per-category counts as secondary evidence.
- **PriorityDisplayType.xml**: 5 categories, 35 extensions, each with an `alias` 2-digit
  sort key (`00jpg`, `01png`, `02jpeg`…) — the Type tree's ordering WITHIN each category;
  "FileFilter.xml = membership, PriorityDisplayType.xml = presentation order"
  [config-files/filtering-priority--config-layer.md §2]. Type-tab clicks emit 8-category
  telemetry (Click_Audio/Document/Photo/Video/Archives/Emails/Databases/Bookmarks)
  [UIUX/09-results-phase-deep-dive.md §3].
- **usefullostfiles.xml**: 27 "valuable formats" (photos 12 incl. cr2/cr3/nef/arw/dng,
  video 9, documents 6) — promotes carved RAW photos out of "Unimportant", ranks valuable
  files in lost-partition results [filtering-priority--config-layer.md §1;
  UIUX/09-results-phase-deep-dive.md §3 last line].

### 5.2 Noise layer (FilterFolder.ini) and per-format carve switches

- **FilterFolder.ini** `[Filter]`: FolderName blocklist (Windows upgrade scaffolding
  `Config.Msi`, `$Windows.~BT/WS/TMP`); FileName blocklist (desktop.ini, .DS_Store,
  $UsnJrnl, IndexerVolumeGuid, WPSettings.dat, MountPointManagerRemoteDatabase);
  **`FatFilterName` = the localized "New Folder" filter in 10 languages + System Volume
  Information + BitLocker artifacts (AadRecoveryPasswordDelete,
  ClientRecoveryPasswordRotation, FveDecryptedVolumeFolder)** — "an empty auto-named
  folder is junk in every locale"; TxtViewFiles (119 text-preview extensions);
  SumaryFileType container-family unification groups (e.g. `m4p_m4a_m4v_mp4`,
  `zip_docx_pptx_xlsx`) [filtering-priority--config-layer.md §3;
  config-data-reference.md §FilterFolder lines 35-41].
- **RawConfig.ini**: per-extension RAW-carve on/off (37 toggles; e.g. HEIC/WEBM/WEBP=1,
  STL/VPJ=0; video-editor project files DRP/HFP/OSP/WFP enabled) — support can toggle
  carving per format without a rebuild [filtering-priority--config-layer.md §4;
  rawsearch-carving--deep-dive.md §4 last bullet; config-data-reference.md line 48].
- The RAW engine also receives the UI-side naming/filter callback at discovery time
  ("File0001.jpg" naming policy lives in the UI, not the engine)
  [rawsearch-carving--deep-dive.md §4].

### 5.3 System-file handling ($MFT etc.)

- **NTFS metafiles are SKIPPED at walk time**: record filter `props->name[0] == '$' →
  skip (NTFS metafiles: $MFT, $Bitmap, $UsnJrnl …)` plus `props->flags & 4 → skip
  (temp/system flag)` [SearchDLL/ntfs-engine--deep-dive.md §3 step 7, lines 73-76;
  verification ledger "…$-prefixed metafile filter … line 21170 region"]. The metadata
  namespace ($RmMetadata/$Reparse/$Repair, $Extend) is "classified separately from user
  data" [SearchDLL/ntfs-engine-internals.md §Record processing lines 54-55].
- **Filesystem-noise files hidden by config**: $UsnJrnl, IndexerVolumeGuid, desktop.ini,
  System Volume Information (see 5.2) [filtering-priority--config-layer.md §3].
- The results tree also carries an **"Unimportant" branch** — "To help you find the
  desired files quickly, we organize some unimportant files here." (collapsed junk bucket)
  — and a "Versions" node (NTFS file-record multi-version recovery)
  [UIUX/09-results-phase-deep-dive.md §2 points 4-5].

### 5.4 Results tree organization (lost vs by-path)

Three-branch root structure, verified live + in shipped .rsf samples + in the vendor's
own log ("prepare to create 3 new file system tree"):
1. **`Windows (C:)` / `Local Disk(I:)`** — existing filesystem structure (by-path tree);
2. **`File Path Lost`** — deleted files whose paths couldn't be reconstructed (warning
   icon; coaching: "Files we found with Advanced Scan, but the file path was fully or
   partially missing…"); per the rsf census this root holds raw-carving results whose
   parentage could not be reconstructed;
3. **`Reconstructed`** — RAW-carved files regrouped / lost-file rebuilds.
[UIUX/dynamic-ground-truth--scan-phase-captured.md §3 table + §3 additional point 6;
SaveScan/rsf-format--scan-session.md §5.4.6 census (I_ sample: 3 roots, NTFS 248,727 ·
RAW 12,147 · Generic 6,959 · FAT 39 nodes; 99.999% lightweight CFileNode, full
CFileSystemNode only at roots); config-files/qa-logs--session-forensics.md §4].
Additional left-tree branches: **Tags Extracted** (EXIF/metadata tag grouping) and
**File Name Lost** ("files whose original name cannot be found, but are more likely to be
your desired files") [UIUX/09-results-phase-deep-dive.md §2 points 1-2]. Path/Type dual
tree modes via `IsTypeTree()` polymorphism on CLeftTreeView (CClassifyLeftTreeView =
override-only subclass) [dynamic-ground-truth--scan-phase-captured.md §3 point 3;
09-results-phase-deep-dive.md addendum].

---

## 6. Phantom-record handling (validation before display)

### 6.1 Engine-side gates (structured records)

- **NTFS per-record validation chain**: (1) "FILE" magic 0x454c4946; (2) USA/fixup
  validation (USN at +0x1E vs each 512-byte sector slot +0x1FE..0x1FF, 8 sectors for 4 KB
  records — "Records failing fixup are rejected"); (3) not a BAAD/"bad" record; (4)
  header checks; then property build. "Fixup failure → record skipped (never aborts the
  whole walk — a recovery product maximizes salvage over purity)"
  [SearchDLL/ntfs-engine-internals.md §MFT acquisition items 4-5 + §IO note;
  ntfs-engine--deep-dive.md §3 steps 1-5 + §9].
- **Deleted-record viability (the anti-phantom gate)**: for every deleted file the walker
  iterates data runs and tests each run's start sector against the volume bitmap +
  cached-node lookup → readable-extent ratio; "a record whose data-run clusters still
  read as allocated in the live $Bitmap means those clusters were re-used — the engine
  destroys such candidates when every cluster is re-used (fully gone), keeps
  overwritten-partial files at reduced chances/quality" — decided AT SCAN TIME
  [ntfs-engine-algorithms-v2--deleted-viability.md §"deleted-viability" lines 47-66 +
  ntfs-engine--deep-dive.md §4]. This ratio is what the UI later surfaces as recovery
  chances/quality.
- $MFTMirr cross-reference as repair/verification source when primary records fail fixup
  [ntfs-engine--deep-dive.md §5 + ledger].

### 6.2 Carve-side gates (RAW records)

- **Two-generation analyser chain**: every format has a modern analyser and, where one
  exists, a legacy `File*Analyser` fallback; "When the modern analyser rejects a
  candidate (validation failure), the chain falls to the legacy implementation before
  giving up. This is why carving rarely false-positives: two independent validators must
  both fail before a match is discarded, and one strong validator accepting is enough"
  [SearchDLL/rawsearchdll--analyser-registry-census.md §2 two-generation design].
- **Defense-in-depth re-validation**: e.g. the WPG magic 0x677077 is re-checked inside
  the FileInfo constructor and a canonical size stamped — "minimizes false positives on
  damaged media where random data can mimic magics" [same §6 note 2;
  rawsearchdll--zip-analyser-carving.md point 4 "Dual validation"].
- **Grammar-based end computation**: hardcoded C++ analysers compute the file END by
  walking structure ("the difference between a 'signature hit' and a recoverable file
  with size") — e.g. PDF `%PDF-x.y` version parse (41230-41233), JPEG SOF0 dimension
  check with 8-bit precision (39653), ZIP central-directory walk
  [rawsearch-carving--deep-dive.md §3 layer 3]. ZIP analyser: candidate must begin at one
  of three ZIP structures, "Anything else → param_2[2] = NULL; return (no match — the
  analyser rejects the candidate outright)"; carve end pointer NULL on total rejection
  [rawsearchdll--zip-analyser-carving.md lines 19, 62].
- **BruteForce-family salvage semantics**: "parsers that walk forward to EOF validating
  structure at every step… keeps consuming until the grammar breaks, then reports the
  last valid position as the file size… more false-positive tail, no premature
  truncation" [rawsearch-carving--deep-dive.md §5]. (Family rosters: 23 BruteForce
  classes, 12 Brance classes [rawsearchdll--family-bruteforce.md; --family-brance.md].)
- **Confidence + size gates**: raw_signs.json per-signature confidence (0.50-1.00),
  header+tail combined scoring "ranks overlapping carves", min/max size bounds "prune
  absurd candidates" [config-data-reference.md §raw_signs.json lines 8-12;
  rawsearch-carving--deep-dive.md §3 layer 1].
- **Signature-collision resolution**: the multimap view returns ALL chains claiming a
  magic (4 JPG magics; TIFF II* claimed by TIFF + camera-RAW analysers)
  [rawsearchdll--analyser-registry-census.md §1 multimap note + §3 table].
- **ForCellTest harness**: `EUCreateRAWSearcherForCellTest` constructs the controller
  statically (once-init + atexit) and binds `CRawFileCellTestStorageDevice` — an in-house
  unit-test harness where a fake storage device feeds synthetic sector images; the
  shipping product carries the engine's test scaffolding
  [rawsearch-carving--deep-dive.md §1 ForCellTest bullet + ledger;
  rawsearchdll--zip-analyser-carving.md line 65 (injects synthetic sector size at +0xc)].
- **UI-side validation pass**: `CCheckNodeThread`/`CheckAnsyncToListThread`
  ("SearchDataSource::OnCheckNode") verify each node's recoverability (extent
  readability, size sanity) before results are presented as recoverable
  [architecture/04-recovery-workflow.md §5.3]; runtime recoverability grading runs at
  completion AND re-fires during browsing/selection (see §3.3), visualized as
  "Recovery Chances: High" with a circular gauge [UIUX/30-interaction-driver-ground-truth.md
  §9.1].
- **UNVERIFIED**: an explicit numeric threshold (e.g. "discard if confidence < X" or
  "discard if readable-run ratio < Y") is not decoded in the corpus; the gates above are
  qualitative/structural. Also UNVERIFIED: any dedup rule between structured and carved
  copies beyond "structured wins, carved attaches as Lost"
  [architecture/05-scan-engine-architecture.md §7.3].

---

## Parity requirements (the behaviors the product must match)

| # | Behavior | Reference citation |
|---|---|---|
| 1 | Two-phase scan: quick (single-FS metadata pass, same StartSector/TotalSector range as deep) auto-continues into advanced; user may recover during advanced | qa-logs--session-forensics.md §2; 25-connect-wiring-graph.md §3.1; 08-scan-phase-deep-dive.md §4 |
| 2 | Advanced = ALL FS engines in full mode + RAW carving engine (RAW registered LAST, gated by settings flag +0x60), with UI-side naming/filter callback into the carver | 04-recovery-workflow.md §3.3; DRWUI-exe--main-ui-architecture.md §9.2 lines 255-261; rawsearch-carving--deep-dive.md §4 |
| 3 | Results stream DURING scan: browse-while-scanning, found-count ticks live, Reconstructed branch grows during carve; engine→UI via CMessageProxy/doDispatchMessage thread-marshaled bus, 15 subscribers | dynamic-ground-truth--scan-phase-captured.md §3 point 2 + §7 behavior 1; mom-modules--ui-bridge... §1; 25-connect-wiring-graph.md §2 |
| 4 | Progress is sector-linear (Reading sector X/Y + remaining time from throughput), not time-fake; percent + counters observable moving throughout | dynamic-ground-truth--scan-phase-captured.md §7 behaviors 2 + status format line 102 |
| 5 | Status strings: "Quick Scanning%1, Files Found:" / "Advanced Scanning%1, Files Found:"; completion: "Scan Completed, Files Found: N (size)" | dynamic-ground-truth--scan-phase-captured.md §3 point 1 + §8 |
| 6 | Scan bar phase-% ownership (0-40 quick etc.): **UNVERIFIED in reference** — only the .rsf SAVE progress 40→65% mapping is documented; if 40/35/25 is kept, cite it as product-engine lineage (BEHAVIOR-FINDINGS BF-22), not reference | rsf-record4--tree-node-grammar.md §5; BEHAVIOR-FINDINGS.md line 36 (product side) |
| 7 | Three-branch result tree: `<Volume>` (by-path) / `File Path Lost` / `Reconstructed`; plus Tags Extracted, File Name Lost, Versions, Unimportant branches | dynamic-ground-truth--scan-phase-captured.md §3; rsf-format--scan-session.md §5.4.6; 09-results-phase-deep-dive.md §2 |
| 8 | Classification: FileFilter.xml 8 categories / 1,489 extensions (membership) + PriorityDisplayType.xml alias ordering (presentation); Path/Type dual tree via IsTypeTree() | config-data-reference.md §Filtering (byte-verified); filtering-priority--config-layer.md §2; 09-results-phase-deep-dive.md addendum |
| 9 | Noise suppression: FilterFolder.ini blocklists (folder scaffolding, FS metadata files, localized New Folder ×10 locales, System Volume Information, BitLocker artifacts) | filtering-priority--config-layer.md §3 |
| 10 | System files ($-prefixed NTFS metafiles, flags&4 temp/system) skipped at walk time, never surfaced as results | ntfs-engine--deep-dive.md §3 step 7 + ledger |
| 11 | Deleted-record viability gate: data-run clusters tested against $Bitmap; fully-reused candidates destroyed, partial-overwrite kept at reduced chances | ntfs-engine-algorithms-v2--deleted-viability.md; ntfs-engine--deep-dive.md §4 |
| 12 | Carve validation: two-generation analyser chains (modern → legacy fallback; both must fail to discard), in-FileInfo magic re-validation, grammar-computed end + NULL-on-reject, confidence + min/max size gates from raw_signs.json | rawsearchdll--analyser-registry-census.md §2 + §6.2; rawsearchdll--zip-analyser-carving.md; config-data-reference.md §raw_signs.json |
| 13 | Sector read model: cached volume IO (CStorageDeviceWithCache) over file-API-or-raw path; EPMNtDrv raw protocol = 512-byte sector units, absolute LBA offsets, MDL async passthrough with completion routine; 64 KB cluster-strided MFT batches | 05-scan-engine-architecture.md §8; eprdtdrv-sys--driver-internals.md §2-3; ntfs-engine-internals.md §MFT |
| 14 | Merge rule: structured nodes win; carved nodes attach to Lost/Other-deleted-files views; recoverability grading (CCheckNodeThread) runs at completion and re-fires on selection | 05-scan-engine-architecture.md §7.3; 30-interaction-driver-ground-truth.md §5 + §9.3 |
| 15 | Completion sequence: Freeze → auto-save auto_<Volume>_<timestamp>.rsf → Unfreeze → ScanFinished → status-bar nStatus=4/m_iScanType 3→4 → recoverability pass; completion modal with stats grid + "Go to Recover" | 30-interaction-driver-ground-truth.md §5 table; 08-scan-phase-deep-dive.md §3; 35-redesign-reference-atlas.md §7 |

### Known reference gaps (do NOT invent parity here)

- Per-phase percent ownership of the scan bar (see #6) — UNVERIFIED.
- Exact event emission rate/throttle for found-file counters — UNVERIFIED (only "live"
  behavior + ~1000-item build batches).
- User-mode overlapped-IO concurrency/queue depth and no-buffering flags — UNVERIFIED.
- The Reading-sector numerator semantics (absolute disk LBA vs partition-relative) —
  UNVERIFIED (capture shows 134316032/31413598, numerator > denominator).
- Numeric confidence/readability discard thresholds — UNVERIFIED.
