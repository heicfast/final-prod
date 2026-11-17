# REFERENCE-SCAN — the og's scan pipeline (corpus ground truth)

> Agent C, Phase-0 recon. Corpus: `/home/z/my-project/easeus-drw-research/` (install dump +
> RE workspace of EaseUS Data Recovery Wizard 21.0.0.0_free, build Oct 13 2026,
> git `40504ccb…` per `og-codebase/log/DRWUI.log` line 2, quoted in
> `re-workspace/artifacts/config-files/qa-logs--session-forensics.md` §1).
> Discipline: every claim tagged **[OBSERVED]** (with corpus path + exact quote/offset),
> **[INFERRED]** (reading consistent with cited evidence), or **[UNKNOWN]**.
> CORRECTIONS.md rows (corpus `re-workspace/artifacts/CORRECTIONS.md`) supersede older
> artifact text where cited. Product-side citations (`final-prod/…`) are verification notes,
> not ground truth.

---

## 1 The flow (click → completion)

**Entry point — "Search for Lost Data" clicked** [OBSERVED]:
- `scanClickedSlot` = `FUN_140833a90` (fully decoded, `re-workspace/artifacts/UIUX/04-userflow-state-machine.md` §4):
  1. `scanMgr = FUN_140149c30()` (CScanMgr singleton); `scanMgr->vft[0x1f0](0)` reset, `vft[0x1f8](1)` prepare.
  2. `CDetailPage::FUN_1406bb5f0(detail, device)` hands the target to the results page.
  3. `QStackedWidget::setCurrentIndex(this, 3)` — **the stack flips to CDetailPage; scan and results are the SAME page**.
  4. `FUN_1406bbfe0(detail)` starts the scan (NAS targets pre-attach `CNasNetErrorTipsWnd`).
- Runtime log (vendor QA, `og-codebase/log/DRWUI.log`, lines 35–49, timestamps 22:57:57):
  ```
  22:57:57:837 run to CScanMgr::doScan:254
  22:57:57:852 Scan drive:I / Partition type:3 / strFolder: / Scan Desktop:0
  22:57:57:869 run to CDoScanPartition::PushScanDevice:58   (ScanDevice= / ScanDriverLetter=I / FilterPath=)
  22:57:57:920 run to CDoScanPartition::PushFolderFilter:84
  22:57:57:932 run to CDoScanPartition::DoScan:94            (AdvanceFilter current time is :2026-10-14 22:57:57)
  22:57:57:961 run to CDoScanPartition::DoScan:153
  22:57:57:975 Quick scan StartSector=705794048, TotalSector=270977023
  22:57:57:987 Deep scan StartSector=705794048, TotalSector=270977023
  ```
  **[OBSERVED]** Both phases are set up at `DoScan` time with the SAME partition range; the
  deep phase is pre-declared, not chosen later.

**Phase 1 — quick scan (type=0)** [OBSERVED, same log lines 56–61]:
```
22:58:01:807 run to CScanMgr::doStartLogic:950
22:58:01:907 run to CDoScanPartition::ScanThread:364      ScanThread type=0
22:58:22:987 run to CDoScanPartition::ScanThread:483       (~21 s later)
22:58:23:003 run to CDoScanPartition::UpdateScanFinishedInfo:498
22:58:24:532 bottomarea.cpp, 838 nStatus=2, m_iScanType=1
22:58:24:548 bottomarea.cpp, 856 nStatus=2, m_iScanType=2
```
- Quick scan = FS-metadata pass driven by the **FS type → quick-searcher factory**
  `FUN_140577d40` (`re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md` §9.2):
  FS codes **0-2=FAT, 3=NTFS, 4-5=EXT2, 7=exFAT, 8=HFS, 9=EXT4, 10=ReFS, 13=ZFS** (+Btrfs)
  → `GetProcAddress(h, "EUCreate<FS>QuickSearcher")(&p)` → `IQuickFileSystemSearchController*`
  (`architecture/04-recovery-workflow.md` §3.2). I re-verified the exports myself with
  `objdump -p` on all 9 DLLs (`og-codebase/*.dll`): every engine exports exactly
  `EUCreate<FS>QuickSearcher` + `EUCreate<FS>Searcher`; Btrfs spells `EUCreateBtrfsComplateSearcher`
  [sic]; RAW adds `EUCreateRAWSearcher`, `EUCreateRAWSearcherForCellTest`, `EUGetAnalyserList`,
  `EUGetAnalyserMap`, `EUGetAnalyserMultimap`, `GetRawUserInfoMap`.
- `m_iScanType` state machine (status bar): 1 = quick running, 2 = advanced running,
  nStatus 2 = scanning / 4 = finished; emit sites `bottomarea.cpp:838/856/477`,
  signal `sigScanFinished(iScanTypeChanged, iScanType)`
  (`re-workspace/artifacts/UIUX/34-navigatebar-bottomarea-chrome.md` §2.1).

**Phase 2 — advanced/deep scan (type=1) — starts AUTOMATICALLY, no user prompt** [OBSERVED]:
```
22:58:24:656 run to CScanMgr::doStartLogic:950
22:58:25:840 run to CDoScanPartition::ScanThread:364      ScanThread type=1
22:58:25:894 Insert lost partition …recoverycontroller\genericrecoverycontroller.cpp, 130,
             StartSector=0, TotalSector=270977023
```
- The deep pass immediately **inserts the whole disk (StartSector=0) as an additional scan
  target** — partition-loss search over unallocated space runs as part of the same scan
  (`architecture/04-recovery-workflow.md` §3.4: `CGenericRecoveryController` "reads the
  partition list, inserts lost partitions as additional scan targets … sector size default 512").
- Advanced composition (`FUN_140580720`, `DRWUI-exe--main-ui-architecture.md` §9.2):
  creates the FULL searcher set `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher`, appends each
  to a `std::list`, **plus `EUCreateRAWSearcher` when the settings flag (+0x60) enables
  carving**, and registers a `std::function<string(string,string)>` naming/filter callback
  into the RAW engine (vtable+0x48). "Advanced scan = ALL FS engines in full mode + RAW
  carving engine, in parallel." [OBSERVED composition; "in parallel" is the artifact's reading
  of the list + type=1 single ScanThread — actual thread fan-out per engine is UNKNOWN, see §12.]
- The user is OFFERED recovery at quick completion but the deep scan continues in the
  background: `CQuickScanCompleteWnd` "Quick Scan Completed" / "You may now" / primary CTA
  **"Recover Found Deleted %1 files %2"** + coaching "Tips: Advanced Scan is trying to find
  more other lost files for you while recove[ring…]"
  (`re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md` §4;
  strings `qt_strings_english.txt` @0001d52b/@0002f962/@0001e445).

**Completion** [OBSERVED, log lines 23:12:44–23:13:23 + dynamic runs]:
```
23:12:44 -------------Start Freeze--------------
23:13:10 Build tree finish.
23:13:21 SaveResultPath = …\SaveScan\auto_Local Disk (I_)10_14_23_13_21.rsf   (AUTO-SAVE)
23:13:23 m_nFreeze=0, bUnfreeze=1 / Send unfreeze msg. / Unfreeze
23:13:23 CDoScanPartition::ScanThread:483 → UpdateScanFinishedInfo:498 → CScanMgr::ScanFinished:636
23:13:23 bottomarea.cpp, 838 nStatus=4, m_iScanType=3
23:13:23 bottomarea.cpp, 856 nStatus=4, m_iScanType=4
23:13:23 Quick scan matched=5682, total=9423 / Recovery ability matched=12351, total=46459
```
- UI: standalone **540×380 OS child window** "Congratulations! Scan Completed!" + "Great news:
  Your files are highly recoverable" + "Go to Recover" CTA; no ESC rejection
  (`re-workspace/artifacts/UIUX/30-interaction-driver-ground-truth.md` §1, §4;
  `dynamic-ground-truth--scan-phase-captured.md` §8).
- Timing references: quick ≈21 s on a 131 GB NTFS volume (270,977,023 sectors × 512 B)
  [OBSERVED, QA log]; full session ≈14.7–15.5 min, 1,139,286 files / 147.12 GB on a 149 GB
  volume (≈98.5% of capacity enumerated) [OBSERVED, `dynamic-ground-truth--scan-phase-captured.md` §8].
- Repair is POST-recovery (Fixo chain), but the completion dialog already carries
  repair-during-recovery counters ("Video Repaired / Video to be repaired",
  `UIUX/08-scan-phase-deep-dive.md` §3; `architecture/06-repair-chain.md`); scan results feed
  repair via the recover-with-repair toggle, not the other way round.

## 2 Quick scan

**What quick scan reads (per FS)** — all [OBSERVED] from engine decompilations
(`re-workspace/reversed/ghidra-searchdll/`, cited via artifacts):
- **NTFS** (`SearchDLL/ntfs-engine--deep-dive.md` §1-§5, §10; v2 algorithms doc):
  `CNTFSQuickSearch(Base)` reads **$MFT directly** — boot-sector → MFT bootstrap
  (FUN_18001a3c0: 64 KB buffer, fixup/USA validation FUN_180041bb0, "FILE" magic
  0x454c4946, record size clamped ≥ 0x400), $ATTRIBUTE_LIST (type 0x20) fragmented-MFT
  handler ("MFT file record has 0x20 attribute, MFT start sector is %I64d"), then a
  record-batch walk: fixup → base-record test (FUN_180041970 — **CORRECTIONS.md row 2: this
  is the base-record test, not the in-use bit**) → build `CNTFSFileProperties` → filters
  (`flags & 4` skip temp/system; `name == NULL` skip; `name[0] == '$'` skip metafiles:
  "$BITMAP", "$Bitmap", "$MFTMirr", "$UsnJrnl", "LostDir", "fastsearch"/"search" config keys
  are literal strings in NTFSSearchDLL) → **deleted records (not in-use) enter the
  deleted-file branch** with bitmap viability scoring (§8 below). Paths are reconstructed
  from parent-reference chains ($FILE_NAME attribute 0x30, parent ref + namespace + times).
  Config gate: `GetPrivateProfileIntW(L"search", L"fastsearch", 1, ini)` — **defaults ON**
  (decompiled line 7163).
- **FAT** (`SearchDLL/fat-engine--deep-dive.md`): `CFATQuickSearch` (0x1C0 bytes, embedded
  0x200-byte sector staging buffer) + nested **`CFATQuickSearch::CFATQuickSearchWorker`**
  (13+ ctor params, thread control at +0x38, sector-size default 512) — the quick scan is
  **threaded per-worker**. Reads the **FAT tables + directory entries**: polymorphic
  `CFAT12Table/CFAT16Table/CFAT32Table` chain readers; EOC family masks
  (`0xFFFFFFF8` / `(e & 0xFFFFFFF8) == 0xFFFFFFF8` / 24-bit variant / 0x0FFFFFF0 forms);
  deleted 8.3 entries = first byte `0xE5` (compiled `-0x1b`, 6 sites) kept with a
  **64 KB-chain-length sanity guard**; LFN reassembly (0x40 last-entry flag + 0x0F attr,
  checksum-verified against the 8.3 alias).
- **exFAT** (`SearchDLL/exfat-engine--deep-dive.md`): `CExFatQuickSearch` (0x8E0 bytes —
  embeds `CQuickSearchAndBuildBitmap`, the allocation-bitmap builder run DURING the quick
  walk); dentry model Volume Label 0x83 / Allocation Bitmap 0x81/0x82 / Up-case / File 0x85 +
  Stream Extension 0xC0 + File Name 0xC1 triplets; `NoFatChain` contiguous runs; reuses NTFS
  property/datarun classes (`CNTFSFileProperties`, `CNTFSFileRecordDataRunsAttribute`).
- **HFS+** (`architecture/05-scan-engine-architecture.md` §4): `CHFSPlusQuickFileSystemSearchController::QuickSearchFiles`
  — open/analyse volume → **`CPartitionVirtualVolumeDevice`** ("works for volumes the OS
  can't see (lost/deleted partitions)") → volume reader `FUN_18000af90(device, 0,
  totalSectors, …)` → attach to embedded tree (`vtable[0x18]`) → unwrap `DetourDirectoryTree`
  → build (`vtable[9]`). Engine errors are HRESULT-style `0x5002xxxx`; the release build
  carries a verbose `cout` channel: `"Return Error:0x%x … Fuction: [sic] … QuickSearchFiles"`.
- **ReFS**: `RefsQuickSearchFiles` (class is function-named; `SearchDLL/refs-engine--deep-dive.md` §1).
- **Deleted vs existing vs lost** [OBSERVED vocabulary]: NTFS in-use flag (FILE record flags
  bit → props bit) splits existing vs deleted records; UI strings "Deleted"/"Deleted Files"
  (@0000d305/@0000d336) and "Existing"/"Existing Files" (@00010193/@000101c7); preview panel
  shows `Status: "Existing"` (dynamic capture, `UIUX/30` §9 shot_26). "Lost" = anything not
  reachable from the live tree: deleted-with-unlinkable-path → "File Path Lost"; carved
  without any FS attribution → "Reconstructed"/"File Name Lost" (§8, §10 below).
- **Engine controller = tree builder** [OBSERVED, vtable decode]: each engine controller
  multiply-inherits `I<Quick|Complete>FileSystemSearchController` (primary) +
  `IDirecotryTreeBuilder` [sic] + `DetourDirectoryTree` (decorator that "intercepts node
  insertion (used to feed progress + dedup + raw/structured merge)")
  (`architecture/05-scan-engine-architecture.md` §3). Results stream as
  `CFileNode`/`CFileSystemNode` + `IFileProperties` content objects.

**What quick scan does NOT do**: no sector sweep, no carving, no $Bitmap build (that is the
complete controller's `CNTFSBuildVolumeBitmap` MI base), no USN cross-check
(`SearchDLL/ntfs-engine--deep-dive.md` §5 table).

## 3 The quick→deep transition

- **Automatic and immediate** [OBSERVED]: `doStartLogic:950` re-fires 1.7 s after
  `UpdateScanFinishedInfo` (QA log 22:58:23→22:58:25); `ScanThread type=1` follows; the
  bottom-area state machine flips `m_iScanType 1→2` at `bottomarea.cpp:838/856` with
  `nStatus=2` (still scanning). There is **no user prompt gating the deep scan** — the
  `CQuickScanCompleteWnd` is an *offer to recover now*, with copy explicitly saying advanced
  continues in the background ("Advanced Scan is trying to find more other lost files for you
  while recovering…", `UIUX/08-scan-phase-deep-dive.md` §4) [OBSERVED copy; INFERRED: it is
  non-blocking coaching, dynamic runs never show a modal at the quick boundary — the first
  modal is at full completion].
- Status bar switches stage label: "Quick Scanning %1, Files Found:" → "Advanced Scanning %1,
  Files Found:" (strings @0001d7d6 / @00007825, `qt_strings_english.txt`; also
  `strings_ascii.txt:10303-10326` with inline color spans `<font color='#5094ff'>%1</font>`,
  `#0560fd`, gray `#616777`).
- Deep scan widens the target set: `Insert lost partition … StartSector=0, TotalSector=<disk>`
  (QA log) — i.e. the advanced pass covers the partition PLUS the whole-disk region for
  lost-partition remnant scanning [OBSERVED log line; the exact per-target ordering is UNKNOWN].
- User can Pause (❚❚) / Stop (■) at any time; paused = "Scan Paused" + red "Scanning Paused."
  strip, gauge frozen at pause %; Stop → `CScanCommonPopDlg` confirm "Stop Searching for Lost
  Data?" with risk framing "Remaining data can be rescanned and recovered anytime later, but
  continuing now save you time." (`UIUX/35-redesign-reference-atlas.md` §6 states row;
  `UIUX/08-scan-phase-deep-dive.md` §3). Cancellation is cooperative: `CStopScanThread` +
  **manual-reset Win32 scan-abort event at `CScanMgr+0x108`** (`UIUX/13-ui-core-wiring-state.md`);
  engine-side batch loops check a cancel callback (`controller.cancel_callback.vt[0x10]()` per
  MFT record batch, `SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1) [OBSERVED].
- Recover-before-completion is offered mid-scan: "Recover All …" blue button active in the
  bottom bar during Advanced Scanning (`dynamic-ground-truth--scan-phase-captured.md` §7).

## 4 Deep/advanced scan + carving

**Two complementary mechanisms run under the type=1 pass** [OBSERVED decomposition,
`architecture/04-recovery-workflow.md` §4a/§4b]:
1. **Full FS-structured scan per engine** — e.g. NTFS `CNTFSSearchController` (0x940 bytes,
   dual vftable: primary `ICompleteFileSystemSearchController` + secondary
   `IBuildVolumeBitmap`/`CNTFSBuildVolumeBitmap`): full walk + **$Bitmap volume bitmap
   build** + **USN journal cross-check** (`$UsnJrnl` strcmp sites 9070/19761/19868/20282) +
   **orphaned-record sweep** + per-file attribute-stream assembly
   (`CResidentNTFSDataRunList`/`CNoneResidentNTFSDataRunList`) + `$MFTMirr` cross-reference
   (`SearchDLL/ntfs-engine--deep-dive.md` §5). Remnant filesystem scanning includes the
   whole-disk lost-partition target (§3) and per-candidate FS probing by
   PartitionRecovery analysers.
2. **RAW carving (RAWSearchDLL)** — "Advanced Scan is searching files sector by sector to
   ensure data recovery as much as possible." (UI string @0002a523; shipped variant with typo
   "sector bu sector" in `architecture/07-user-flow-uiux.md` §3):
   - **190+ `IRawFileAnalyser` format handlers** (`CommonRawAnalyser` → per-format
     `File<XXX>Analyser`; namespaces `Brance::` [sic, "branch generation"], `BruteForce::`,
     `RawSpeed::`; 488 vftable references) — `SearchDLL/rawsearch-carving--deep-dive.md` §1-§5,
     `rawsearchdll--analyser-registry-census.md`.
   - **Signature registry**: `std::map<u32 fourcc, Analyser*>` RB-tree (JPG keys
     0xFFD8FFE0/E1/DB/EE → `eeffd8ff` etc.; `%PDF` 0x46445025; PNG 0x474e5089; `Rar!`
     0x21726152; ZIP local header 0x04034b50; TIFF byte-order family ×4) PLUS a **second
     u16 first-2-bytes table (39 entries: `BM`→BMP, `MM`→TIFF-BE, 0x8B1F→gzip, MPEG sync
     variants, `<h`/`<H`/`<!`→HTML/XML…)** — `raw-dual-registry--u16-magic-keys.md` §1-§2.
     Chains: modern analyser + legacy `File*Analyser` fallback ("two independent validators
     must both fail before a match is discarded") — `rawsearchdll--analyser-registry-census.md` §2.
   - **Signature DB `og-codebase/raw_signs.json`** [OBSERVED, I parsed it]: 139 extension
     keys; every entry `{header: {signs: ["offset,hexbytes,confidence", …]}, tail?: {signs},
     min, max}` — e.g. `"apk": header 0,504b0304,0.59 + tail 28,504b050600000000,0.94,
     min 1, max 51200`; 46/139 entries carry tail signatures; confidence values 0.50–1.00;
     header sign-counts 1–6 per ext. Loaded at engine init via hand-rolled istream JSON
     tokenizer (NOT tinyxml2/CJsonObject for this file — correction inside
     `rawsearch-carving--deep-dive.md` §3); header+tail+size-gate confidences combine to
     rank overlapping candidates.
   - **Hardcoded analysers compute file END by walking structure** (JPEG marker walk incl.
     SOF0 dimension check `0xC0 && precision 8`; PDF `%PDF-x.y` version parse; ZIP central
     directory; PST node b-tree); `BruteForce::` analysers "walk forward to EOF validating
     structure at every step … report the last valid position as the file size"
     (`rawsearch-carving--deep-dive.md` §3, §5).
   - **Per-format carve toggles**: `og-codebase/config/RawConfig.ini` `[Filter] FORMAT=1/0`
     — **34 toggles** after CORRECTIONS.md row 5 (e.g. HEIC=1, M2TS=1, DRP=1, HFP=1, HIK=1;
     STL=0, VPJ=0) — video-editor project formats are first-class carve targets
     (`config-files/filtering-priority--config-layer.md` §4).
   - **Carve scope**: family doc says carving runs "across unallocated clusters (volume
     bitmap from FS engine) + free space" (`searchdll-family--architecture.md` §6.4)
     [INFERRED/UNVERIFIED — the bitmap hand-off is architecturally implied (IBuildVolumeBitmap
     MI + engine list) but the exact carve window is not decompiled; see §12].
   - **Naming/parenting**: RAW searcher receives the UI's
     `std::function<string(string,string)>` callback (vtable+0x48) — "e.g. 'File0001.jpg'
     naming policy lives in the UI, not the engine" (`rawsearch-carving--deep-dive.md` §4).
     Carved nodes feed `IRawDirectoryTreeBuilder` → **"Lost" flat views
     (`LostTreeModel`/`LostTableModel`) and are cross-correlated with structured results in
     the merge phase — structured wins; carved attach as 'Other deleted files'/'Reconstructed'**
     (`architecture/05-scan-engine-architecture.md` §6.3).
   - **`GetRawUserInfoMap` is a per-scan carve-STATISTICS map** (insert-or-increment from
     ~70 analyser detection paths, cleared at every searcher init, read one-way post-scan by
     DRWUI's "Trigger_Raw"/"FileSystem" report builder) — **scan OUTPUT, never input;
     EUUserData never reaches the RAW engine** — CORRECTIONS.md row 9 [OBSERVED correction,
     supersedes older artifact text].
- Live proof of carving streaming: the **"Reconstructed" branch grows during the advanced
  pass** — 1 file at t=75 s (9%), 16 at t≈460 s (34%), 87 at completion
  (`dynamic-ground-truth--scan-phase-captured.md` §3, §7; `UIUX/30` §2) [OBSERVED].

## 5 Progress model

- **Per-phase 0-100%**: the circular blue-ring gauge (`RadiusProgressBar`, `ScanProgress/`
  188 RCC assets) restarts per phase — at t=75 s the display read **"Advanced Scanning, 9%"**
  with quick already done (1.11M files found); at t≈460 s **34%**; whole scan ≈14.7 min
  [OBSERVED, `dynamic-ground-truth--scan-phase-captured.md` §3.1, §7 table].
- **Status bar format (verbatim)** [OBSERVED]:
  `Advanced Scanning , Files Found: 1139157 (143.74 GB)` + `Remaining time: 07m43s |
  Reading sector: 134316032/31413598` (§7 addendum). String templates:
  - `"Quick Scanning %1, Files Found:"` @0001d7d6; `"Advanced Scanning %1, Files Found:"`
    @00007825; `"Cloud Scanning %1, Files Found:"`; `"Scan Paused %1, Files found:"` @00022d56;
    `"Scan Completed, Files Found:"` @00022c5a; `"Scan Stopped, Files Found:"` @00022e48
    (`re-workspace/reversed/DRWUI.exe/qt_strings_english.txt`; also
    `strings_ascii.txt:10303-10326` with color spans, and `" %1<font color='#616777'>(%2)</font>"`
    for the GB suffix).
  - `"Remaining time: %1"` (`strings_ascii.txt:15216-15217`); `"Reading sector:"` @0001de88;
    `"Searching: <current file path>"` full-width light-blue strip; milestone line
    `"%1 file chunks scanned..."` @00003b9a; `"%1 files %2 have been found."` @00003c35;
    generic `"Scanning %1%"`; `"Estimated Time for Advanced Scan:"` @0000ff19.
  - Smart partition variant: `"Scanning disk sectors: %1"` @000237dc / stopped
    `"Scanned disk sectors: %1"` @00023684.
- **Sector ticker semantics**: `Reading sector: X/Y` observed with X > Y (134,316,032 vs
  31,413,598 on a 149 GB disk) — units/order of the two counters are **[UNKNOWN]** (neither
  cleanly equals total sectors at 512 B or 4 KiB); what IS observed: it ticks and moves
  during the advanced pass [OBSERVED verbatim; semantics UNKNOWN].
- **What drives the percent**: linear sector-throughput model [INFERRED] — "the remaining-time
  estimate (7m43s at 34%) implies a linear sector-throughput model — matches the sector-based
  progress decoded in the engines (bitmap/sector progress objects)"
  (`dynamic-ground-truth--scan-phase-captured.md` §7 "Verified behaviors" #2). Exact formula
  (bytes-read/total vs sub-stage weighting across the parallel engine list) is **[UNKNOWN]**
  (§12). Product-side note: `final-prod/docs/PARITY-DELTAS.md` row D-P6 already records this
  divergence (og restarts 0-100% per phase; our product ran one global 40/35/25 curve) —
  **verified against corpus: correct**; D-P6's "status bar `<Stage>%1, Files Found: N (GB)`"
  matches the verbatim captures above.
- **Other progress axes simultaneously** [OBSERVED]: cumulative files+size counter; the
  `AnalysisGridWidget` sector-grid disk-map (54 v-methods); "Searching:" path ticker showing
  the file currently examined (e.g. `...\Microsoft.CodeAnalysis.*.resources.dll` mid-scan);
  `ScaningRecoverMessage` rotating status line (coaching gated by
  `config.ini [RecoveringTips] scaningRecoverShowTips=true`).
- **Bad-sector progress handling** [OBSERVED strings]: "Program is trying to skip bad sectors
  to find data." @0001d05c; "There are %1 bad sectors in the disk." @0002a01b; "Invalid
  sector" @00015c34. Engine behavior: fixup failure → record skipped, never aborts the walk
  (`SearchDLL/ntfs-engine--deep-dive.md` §9); FAT follows FAT12 BAD cluster 4087 RAW
  (garbage-tolerant continuation, CORRECTIONS.md row 3); "Disk image … can improve the
  recovery performance when the disk has bad sectors" (image-mode option @0000d9c7).

## 6 Streaming (results appear during the scan)

- **Browse-while-scanning** [OBSERVED, dynamic]: at 9% the UI is fully interactive — 3-branch
  tree populated ("Windows (C:) 1.11M", "File Path Lost 4.4K", "Reconstructed 1"), list
  browsable, Path/Type tabs live, preview panel live, Filter funnel + search present,
  "Recover All ..." active (`dynamic-ground-truth--scan-phase-captured.md` §2-§3).
- **Incremental tree insertion in ~1000-item batches** [OBSERVED, vendor DefaultLog.txt]:
  `FinishSearch → "prepare to create 3 new file system tree" → CreateNewFileSystemTree →
  BuildTree cycles: "show file system node" ⇄ "build raw file node" (incremental batches
  ~1000 items)`; 223 M cumulative items on the stress QA disk
  (`config-files/qa-logs--session-forensics.md` §4).
- **Live grading passes stream during the scan** [OBSERVED, QA log]: `Quick scan matched=N,
  total=9423` and `Recovery ability matched=N, total=M` lines fire every few seconds
  throughout the deep pass (e.g. 22:58:24→23:02:39 matched 36→5682 of 9423; Recovery ability
  7940/15349), totals growing as results stream in [INFERRED: totals track the growing
  deleted/lost subset; the whole-disk 1.1M-file count on the runner shows these counters are
  NOT the total file count].
- **Worker fleet (UI never blocks)** [OBSERVED classes]: `CDirectoryTreeManager` +
  `CSetClassifyThread`, `CSetFilterThread`, `CSortNodeThread`, `CCheckNodeThread`(s),
  `CSelNodeThread`, `CGetListThumbnailThread`; several started at **QThread priority 7
  (TimeCritical)** (`UIUX/31-filterbar-family-decode.md` §5; `UIUX/32` "priority-7 worker
  fleet confirmed cross-line"); async expand-on-scroll with 1 s debounce
  (`UIUX/20-moc-surface-sink-architecture.md`).
- **Scan → filter interplay** [OBSERVED]: the filter condition is pushed INTO the scan
  controller at start (`FilterPath=` + `PushFolderFilter:84` + `AdvanceFilter current time
  is :…` in both QA and runner logs; `UIUX/31` §7) — pre-set filters constrain incremental
  tree insertion, not just the final list.
- Signal contract for progressive fill: `sigQuickScanFinished(int)`, `sigScanFinished` ×3,
  `sigLoadFinished(bool)` wired in CDetailPage ctor (`UIUX/09-results-phase-deep-dive.md` §1).
- Engine-side streaming hook: `DetourDirectoryTree` intercepts node insertion "to feed
  progress + dedup + raw/structured merge" (`architecture/05-scan-engine-architecture.md` §3)
  [OBSERVED vtable structure; dedup/merge timing INFERRED].

## 7 Sector access & performance

- **Volume IO layering** [OBSERVED, `architecture/05-scan-engine-architecture.md` §8]:
  ```
  Engine tree reader
    └─ CPartitionVirtualVolumeDevice (works on lost partitions)
         ├─ normal file API when volume is mounted & healthy
         ├─ raw IOCTL path → EPMNtDrv (\\.\EPRDTDRV\<d>\<p>) — MDL async sector IO, 512B sectors
         ├─ BitLocker: FVE keys (\\.\Pipe\DRWBitlocker, DRWService) → XTS-AES decrypt
         └─ NAS: EuMrx redirector (\\127.0.0.1 tunnel via libssh2)
  ```
  with a **read cache in front** (`CStorageDeviceWithCache` → `IStorageDevice`) "to survive
  MFT re-reads economically" (`SearchDLL/ntfs-engine--deep-dive.md` §8) [OBSERVED class + INFERRED role].
- **Driver contract** [OBSERVED, `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` §3]:
  `\\.\EPRDTDRV\{disk}\{part}` → `\Device\Harddisk{d}\Partition0` (absolute whole-disk LBA);
  control device `\Device\EPMNTDRV` DO_DIRECT_IO; IRP_MJ_READ → `IoBuildAsynchronousFsdRequest`
  with **read length expressed in sectors**, completion `FltReadWriteSectorsCompletion`,
  `Information = Length << 9` — **512-byte sector hardcoded**; 4Kn/512e normalization happens
  above the driver. Three custom IOCTLs (0x60002400/04/08) + passthrough; PID/process checks
  (anti-abuse). Installed by `PartitionRecovery/setupdrvx64.exe` on first raw-access need;
  preloaded in WinPE as ebrntdrv/epmdkdrv.
- **First-hand import census** [OBSERVED, my `objdump -p` run]: NTFSSearchDLL, EXT4SearchDLL,
  BtrfsSearchDLL, FATSearchDLL import `CreateFileW` + `DeviceIoControl` + `SetFilePointerEx`
  (+`CreateFileMappingW`; FAT also `CreateFileA`/`SetFilePointer`/`WriteFile`) — direct
  sector-IO capability. **RAWSearchDLL, ReFSSearchDLL, HFSPlusSearchDLL, ZfsSearchDLL import
  only KERNEL32/MSVCP140/VCRUNTIME(+crt)** — no direct Win32 IO imports; their reads ride the
  device abstraction handed to them by the host/DRWUI side [INFERRED from imports + §8 layering].
- **Read pattern (NTFS MFT)** [OBSERVED]: cluster-strided aligned MFT reads; batch loader
  reads **4 KiB (0x1000) pages** (`device.vt[0x08](device, offset, buf, 0x1000, 0)`) then
  record-size-strided fixup re-validation across the page; MFT bootstrap uses a **64 KB
  buffer** (`ntfs-engine-algorithms-v2` §5; `ntfs-engine--deep-dive.md` §2, §8).
- **Threading**: FAT quick search = threaded `CFATQuickSearchWorker` [OBSERVED]; the advanced
  engine set is a `std::list` of full searchers created by `FUN_140580720` [OBSERVED] — the
  concurrency/overlapped-IO fan-out across engines is **[UNKNOWN]** (§12). Driver IO is
  synchronous-from-caller (event wait after `IofCallDriver`) but async at the disk stack
  [OBSERVED driver decompile].
- **Cancellation / retry**: cooperative per-record/batch cancel callbacks [OBSERVED];
  `CStopScanThread` + scan-abort event `CScanMgr+0x108` [OBSERVED]; retry vocabulary:
  `CRemoteDevice::Connect` "retry %d" is NAS-side; engine read failure → skip-and-continue
  (maximize salvage) [OBSERVED]; no explicit sector-read retry loop is documented anywhere —
  **[UNKNOWN]** whether a per-read retry exists below the cache.

## 8 Result model, categories & prioritization

- **The 3 roots** (triple-verified: live screenshot = .rsf record-4 roots = vendor log
  "create 3 new file system tree") [OBSERVED]:
  1. **Existing filesystem branch** — named by volume ("Windows (C:)" / "Local Disk(I:)"),
     full directory tree of the live volume (1.11M files in the runner capture).
  2. **"File Path Lost"** — deleted files whose path could not be reconstructed; warning
     icon (folder + question mark); coaching: "Files that we found with Advanced Scan, but
     the file path was fully or partially missing. Although the file path is incomplete,
     it's possible that they are the lost files you want to recover." (@00010fa9).
  3. **"Reconstructed"** — puzzle-piece folder icon; lost-file rebuilds incl. RAW-carved
     regrouped (grows live during advanced scan; `dynamic-ground-truth--scan-phase-captured.md`
     §3; `SaveScan/rsf-format--scan-session.md` §5.4.6 census).
- **5 special branches with coaching copy** [OBSERVED, `UIUX/09-results-phase-deep-dive.md` §2]:
  Tags Extracted (EXIF/metadata tag grouping) / File Name Lost ("original name cannot be
  found") / File Path Lost / Versions ("another version of the file in the same directory…
  NTFS file-record multi-version recovery", @00015eb5) / Unimportant ("we organize some
  unimportant files here" @0002ad81 — collapsed junk bucket, `filetype_ico_Unimportant`).
- **Deleted vs live distinction** [OBSERVED]: separate UI strings "Deleted Files"/"Existing
  Files"; tree section "&Other deleted files" @0001b2c0; quick-complete CTA counts deleted
  specifically ("Recover Found Deleted %1 files %2"); per-file Status field ("Existing") in
  preview. In the persisted node model, `CFileSystemNode+0x50 type_flags` carries a
  `-0x10` "hidden" value filtered by the second root list, and each node inherits its
  parent's +0x50 type tag on restore (`rsf-format--scan-session.md` §5.4.2-§5.4.3) —
  exact flag-bit meanings **[UNKNOWN]** (soft labels per artifact §8).
- **Node model** [OBSERVED, .rsf grammar]: 99.999% lightweight `CFileNode` (147 B) +
  `CFileSystemNode` (656 B) roots only; refcounted `IFileProperties` content per node with
  per-FS serializers (NTFS/FAT/exFAT/EXT4/HFS/ReFS/Btrfs/ZFS/RAW/generic type codes);
  content type 0xB (runtime-only thumbcache/cloud properties) never saved.
- **Category model (FileFilter.xml)** [OBSERVED, my parse of `og-codebase/FileFilter.xml`]:
  8 `FileCategory` entries — **Graphic (icon 0, 320 ext), Document (icon 1, 432), Email
  (icon 2, 19), Audio (icon 3, 281), Video (icon 4, 217), Other (icon 5, 0 ext), Archive
  (icon 6, 219), Bookmarks (icon 7, 1)** = **1489 extension entries / 1481 unique** (8 dups:
  `.insp .md .ari .insv .gz .jar .xz .uzip`). CORRECTIONS.md row 5's recount of 1481 =
  unique count; both numbers reconcile. Each category/FileType carries `Selected=2`
  (selected-state triad, root `Selected="2"`, `RootName="All types"`). Labels e.g.
  Graphic = "common image files or digital camera photos, such as JPG, etc."; Archive =
  "other files such as archives."
- **Prioritization** [OBSERVED]:
  - `config/PriorityDisplayType.xml` — 5 categories × 35 extensions, each with an `alias`
    2-digit sort key (`00jpg`,`01png`,`02jpeg`…) = presentation order within category
    (`config-files/filtering-priority--config-layer.md` §2).
  - `config/usefullostfiles.xml` — 27 "valuable formats" (photos incl. camera RAW
    cr2/cr3/nef/arw/dng; videos mp4/mov/avi/mkv/mts/m2t/m2ts; docs pdf/office) promoted out
    of Unimportant (`filtering-priority--config-layer.md` §1; `UIUX/09` §3).
  - `SumaryFileType` family grouping for counts (m4p_m4a_m4v_mp4, zip_docx_pptx_xlsx …).
  - Category tab click telemetry = 8 categories (`Click_Audio/Document/Photo/Video/Archives/Emails/Databases/Bookmarks`).
  - AI layer (2024+): "AI Recommend is on. Showing %1 recommended files." ranked view +
    AI dedup (`UIUX/09` §5).
- **System/junk handling** [OBSERVED]: NTFS metafiles skipped at engine level (`$`-prefix
  filter); `FilterFolder.ini` noise suppression (§9); "Unimportant" bucket; `hidden` node
  flag; system folders like "System Volume Information" in FatFilterName exclusions.

## 9 Filtering

**Two layers** [OBSERVED]:
1. **Engine/config-side noise suppression** — `og-codebase/config/FilterFolder.ini` `[Filter]`:
   - `FolderName=;Config.Msi;$Windows.~BT;$Windows.~WS;$Windows.~TMP;`
   - `FileName=;desktop.ini;._.DS_Store;.DS_Store;$UsnJrnl;IndexerVolumeGuid;WPSettings.dat;MountPointManagerRemoteDatabase;`
   - `FatFilterName=` localized "New Folder" filter in **10 locales** (新建文件夹 /
     新しいフォルダー / 새 폴더 / Neuer Ordner / Nova pasta / Nuova cartella / Nueva carpeta /
     Nouveau dossier …) + `System Volume Information` + BitLocker rotation artifacts —
     default-named empty folders are junk in every locale
     (`config-files/filtering-priority--config-layer.md` §3).
   - `TxtViewFiles` = 119 text-family extensions for text preview; `SumaryFileType` = family
     unification for counts.
2. **UI-side advanced filter** (`UIUX/31-filterbar-family-decode.md`, full decode):
   - 5 filter buttons sharing one moc surface: **Type / Time (Date Modified) / Size /
     Advanced + CFilterLostFilesBtn ("lost files only" toggle)**; menu items are
     QCheckBox-backed QActions; special action-IDs `0x1a/0x105/0x1006` (select-all /
     lost-files / custom-range) re-assert rather than toggle.
   - Filter dropdown contents (static decode, `UIUX/35` §"filter dropdown"): File Type /
     Date / Size with Custom suffix entry + **toggles Existing / Hide 0KB / Hide System /
     Hide Temp / Hide Browser Cache** + per-category "Select All (%1)" live counts + Reset.
   - **`AdvanceFilterCondition` = 0x48 bytes with 7 sub-condition groups** (enable-bits;
     empty = all 7 set → NULL → clear filter) — dimensions type/time/size/state/tag/path/search.
   - TWO `CFilterBar` instances (Path mode / Type mode), hidden until mode switch; both
     signal `sigFilterChange(cond, bClear, bForce)` → `CRightContent::onFilterChanged`
     (FUN_14072b730) → spawns **`CSetFilterThread`** → resolves the view-indexed model
     (path/type/search) → `model->SetFilter` (virtual +0x10) or NULL to clear.
   - **Client-side (UI model) filtering, post-results but applicable during the scan** —
     during-scan applicability is proven by the scan-start `PushFolderFilter` push (§6) and
     by live match counts (`CFilterCountFrame`) while the scan runs [OBSERVED + INFERRED].
   - Search box: "Search files or folders" with type dropdown (Pictures/Videos/Documents/
     Audio/Archive/Emails/Bookmarks/**Unsaved File**); empty-results coach
     "How to find files if it searched nothing" with 3 strategies.
   - `CAdvanceFileFilterMgr` keeps 3 condition slots (one per view mode) — persistent
     "last filter per mode". `CRecoveryFilter` = recovery-time filter (core side).
   - Blocked-state copy: "The file was filtered. Please clear the filter and then [search]
     again."; "Filtered nothing..." empty state; guide "To find your files quickly and
     easily by 'Filter'".

## 10 Deleted-tree reconstruction

- **Deleted folders appear as folders** [OBSERVED]: NTFS deleted MFT records (not in-use)
  with $FILE_NAME attributes keep their parent-reference chains; `CNTFSQuickSearch(Base)`
  "reconstructs paths from parent-reference chains" (`SearchDLL/ntfs-engine--deep-dive.md`
  §5) — so a deleted directory subtree re-materializes as a directory subtree, not a flat
  list. FAT deleted entries (0xE5) feed `FUN_18000b430`/`FUN_180008d50` "process valid/invalid
  entry chains and feed the tree builders" (`fat-engine--deep-dive.md` §4) — deleted FAT
  directories are re-walked as chains. exFAT deleted = File dentries flagged invalid but
  "still paired when both fragments survive" (Stream+Name fragments).
- **Viability pruning at scan time (NTFS deleted files)** [OBSERVED algorithm,
  `SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1 + CORRECTIONS rows 2/7]:
  deleted-record batch loop FUN_18001acc0 — magic/fixup/corruption checks → base-record
  test FUN_180041970 [CORRECTED semantics] → system-file filter (`flags&4`, null name,
  `$`-prefix) → **viability walk only when `0x3ff < size` strictly** (files of exactly 1023
  bytes take the fast path with no bitmap check — CORRECTIONS row 7) → iterate data runs,
  `bitmap.test(sector)` + secondary map membership → **`intact == total` → destroy**
  (every cluster re-allocated = content overwritten); partially-overwritten files are KEPT.
  Fast-path small files (< 1 KiB, often resident $DATA) skip validation.
- **Orphan grouping** [OBSERVED]: records whose parent chain is broken land in "File Path
  Lost" (path fully/partially missing); nameless carves land in "File Name Lost"; RAW-carved
  regroup under "Reconstructed"; the complete scan adds an "orphaned record sweep"
  (NTFS) and the merge phase cross-correlates structured vs carved (structured wins,
  carved attaches as "Other deleted files"/Lost views).
- **"Deleted only" scoping** [OBSERVED]: `CFilterLostFilesBtn` scopes the view to lost-file
  branches; quick-complete CTA "Recover Found Deleted %1 files %2"; "Select unrecovered
  files" context item (`UIUX/35` §"context menu").
- **Edge cases documented in corpus** [OBSERVED strings/algos]: duplicate first-cluster
  directories (FAT: "Some thing must be wrong! We found two undeleted directory got the
  same first cluster number!" — anomaly logged, not fatal); BAAD/'FILE0' corrupted-record
  family discrimination (FUN_18002a780); fragmented-MFT ($ATTRIBUTE_LIST) reassembly;
  $MFTMirr repair source; multi-version records (Versions branch); fixup failure → record
  skipped, walk continues; FAT12 BAD cluster garbage-tolerant continuation (CORRECTIONS row 3).
- **.rsf persistence round-trips the tree** (3 roots + BFS child lists + per-node
  dataruns), validated byte-exact on 1,986,806 nodes across 3 shipped samples
  (`SaveScan/rsf-format--scan-session.md` §5.4.6) [OBSERVED].

## 11 Rebuild spec for our product (checkable behaviors)

Each row: behavior → evidence tag + citation. These are the acceptance criteria the
final-prod scan pipeline must satisfy.

1. **Quick phase MUST run and render first** — click → quick scan (type=0) with status line
   "Quick Scanning %1, Files Found:" and its OWN 0-100% gauge; ~seconds-per-100GB scale
   (21 s / 131 GB NTFS). No "jumps directly to 30-75% advanced". [OBSERVED: QA log + UIUX/04
   §4 + strings @0001d7d6]
2. **Advanced phase starts AUTOMATICALLY after quick** (no prompt, ~2 s later, type=1),
   inserting the whole-disk region (`Insert lost partition … StartSector=0`) as an extra
   target; stage label switches to "Advanced Scanning %1, Files Found:" and the gauge
   RESTARTS from low %. [OBSERVED: QA log 22:58:24-25; D-P6 verified]
3. **Status bar exact format**: `<Stage> %1, Files Found: N (S GB)` + `Remaining time:
   HHmMMs` + `Reading sector: X/Y` + `Searching: <path>` strip + "%1 file chunks scanned..."
   milestones; completion = "Scan Completed, Files Found: N (S GB)". [OBSERVED verbatim
   captures; product's PARITY-DELTAS D-P6 row verified correct]
4. **Status-bar state machine**: `m_iScanType` 1→2 at quick→advanced (nStatus=2), 3→4 at
   completion (nStatus=4); emit `sigScanFinished(iScanTypeChanged, iScanType)`.
   [OBSERVED bottomarea.cpp:838/856/477]
5. **Results stream live**: tree fills during scan in ~1000-item batches; 3 roots (existing
   volume / File Path Lost / Reconstructed) visible mid-scan; Reconstructed grows during
   carving; browse/filter/preview/recover-CTA all live mid-scan. [OBSERVED: DefaultLog +
   dynamic captures]
6. **Quick scan reads FS metadata only**: NTFS $MFT record walk incl. deleted (fixup-aware,
   $-metafile filter, base-record test, parent-chain path reconstruction); FAT FAT-table +
   dir-entry walk (0xE5 kept, LFN reassembly, 64 KB chain guard); exFAT dentry triplets +
   bitmap build during quick. [OBSERVED engine decompiles]
7. **Deleted-file viability scoring at scan time**: deleted records with `0x3ff < size`
   strictly get data-run bitmap cross-check; fully-reallocated → dropped; partial → kept.
   Small/resident files fast-pathed. [OBSERVED + CORRECTIONS row 7]
8. **Deep scan = full FS searchers + RAW carving engine** (all in one type=1 pass):
   carving gated by a settings flag (+0x60 analog), signature dispatch = u32 fourcc map +
   u16 magic table + raw_signs.json (139 ext: offset,hex,confidence header/tail + min/max),
   per-format RawConfig toggles, two-generation analyser chains, structure-walking EOF
   computation. [OBSERVED]
9. **Carved-file naming/parenting**: engine emits raw nodes; UI naming callback assigns
   "FileNNNN.ext"-style names; carved nodes land under Reconstructed/File Name Lost flat
   views; structured results win on merge conflicts. [OBSERVED wiring; INFERRED merge order]
10. **Categories must cover the FileFilter.xml model**: 8 categories, 1489 entries/1481
    unique ext (Graphic 320 / Document 432 / Audio 281 / Video 217 / Archive 219 / Email 19
    / Bookmarks 1 / Other 0); PriorityDisplayType alias ordering; usefullostfiles promotion
    out of Unimportant. Images/documents/archives MUST be found+classified — the corpus's
    own category weights show these are the majority classes. [OBSERVED parse]
11. **Filters must be live**: 0x48-byte 7-dimension condition (type/time/size/state/tag/
    path/search); type/size/date dropdowns + Existing/Hide-0KB/Hide-System/Hide-Temp/Hide-
    Browser-Cache toggles + lost-files-only scope; refilter in CSetFilterThread-analog
    worker (never UI thread); filter state also pushed into the scan controller at start.
    [OBSERVED]
12. **Deleted-tree reconstruction**: deleted dirs reappear as dirs via parent-chain/chained
    entry walk; orphans grouped into File Path Lost / File Name Lost / Reconstructed /
    Other deleted files; Versions branch for multi-version records. [OBSERVED]
13. **Sector access**: cached device layer; healthy volume = file API, damaged = raw driver
    path with 512-byte sector contract; 4 KiB page reads for record batches; skip-not-abort
    on bad sectors with user-visible "skipping bad sectors" copy; per-batch cooperative
    cancellation. [OBSERVED]
14. **Completion pipeline**: cache-reset cascade → Freeze → auto-save .rsf session →
    Unfreeze → ScanFinished → 540×380 completion window with stats grid + recoverability
    verdict; recoverability grading passes re-fire during browsing. [OBSERVED]
15. **Confidence/quality marking**: per-file "Recovery Chances" grade (High + circular
    gauge) + Status (Existing/…) in the preview pane; recovery-chance column in the list;
    engine-side readable-extent ratio computed at scan time. [OBSERVED]

## 12 Open questions for Agent E

1. **Percent formula**: exact mapping from engine progress → the per-phase 0-100% gauge
   (bytes-read/total? sector-position/total-sectors? weighted sub-stages across the parallel
   searcher list?). Only the linear-throughput implication is observed.
2. **`Reading sector: X/Y` semantics**: observed X > Y (134,316,032 vs 31,413,598); units,
   order, and which window (disk vs partition vs phase) are undecoded. Also the ticker's
   update cadence.
3. **Advanced-pass concurrency**: the `std::list` of full searchers + RAW — how many
   threads, what scheduling, how progress aggregates across engines; whether the lost-
   partition whole-disk target runs interleaved or after the partition scan.
4. **RAW carve scope**: unallocated-clusters-only (via the FS engine's volume bitmap) vs
   whole-volume sweep — the family doc says bitmap+free space but no decompile confirms the
   window hand-off.
5. **Tree-insertion batching exact contract**: ~1000-item batches observed in DefaultLog;
   the signal/slot that paces UI insertion (and the interval) is not decoded.
6. **`Quick scan matched=N, total=M` / `Recovery ability matched=N, total=M` exact
   semantics** — which node sets the two passes grade, and what "matched" means (the totals
   track a growing subset, not total files).
7. **Node flag bits**: `CFileNode/CFileSystemNode` flag-byte business meanings (deleted vs
   existing marker bits, the `-0x10` hidden semantics, +0x71 enum remap 2↔5) are soft
   labels in the rsf artifact §8.
8. **Per-engine quick-search internals for EXT4/ReFS/Btrfs/ZFS** (only NTFS/FAT/exFAT/HFS+
   have algorithm-level deep dives; EXT4 has 0xEF53 superblock + journal gating per
   CORRECTIONS row 6).
9. **Read-retry policy**: any per-read retry/sector-reallocation below `CStorageDeviceWithCache`
   is undocumented; only skip-and-continue is observed.
10. **u16/u32/raw_signs dispatch order** is INFERRED (u16 first, then 4CC, then signature
    DB) — confirm from the carve loop.
11. **Existing-file display policy**: whether the existing branch shows ALL live files
    always, or is gated (runner showed 1.11M = everything; the QA I: scan's rsf had 248,727
    NTFS nodes vs "Recovery ability total=46459" — reconcile).
12. **Reconstructed vs File Path Lost split rule**: which engine output goes to which branch
    (rsf census says "Reconstructed holds lost-file rebuilds"; live run shows carved files
    growing there; the precise classifier is not decoded).

---
*Sources index (primary): `architecture/04-recovery-workflow.md`,
`architecture/05-scan-engine-architecture.md`, `architecture/05-uiux-architecture.md`,
`architecture/07-user-flow-uiux.md`, `architecture/02-kernel-driver-architecture.md`,
`re-workspace/artifacts/{SearchDLL/*, UIUX/04,08,09,30,31,34,35, dynamic-ground-truth--scan-phase-captured.md,
config-files/{filtering-priority--config-layer, qa-logs--session-forensics}.md,
SaveScan/rsf-format--scan-session.md, DRWUI-exe--main-ui-architecture.md,
PartitionRecovery/eprdtdrv-sys--driver-internals.md, CORRECTIONS.md}`,
`og-codebase/{FileFilter.xml, raw_signs.json, EUUserData, config/*, log/DRWUI.log, *.dll}`,
`re-workspace/reversed/DRWUI.exe/{qt_strings_english.txt, strings_ascii.txt}`,
`re-workspace/reversed/ghidra-searchdll/*` (via artifacts).
Product-side verified: `final-prod/worklog/REFERENCE-BEHAVIOR.md` §6-7 (citations spot-checked
PASS), `final-prod/docs/PARITY-DELTAS.md` D-P6 (PASS), `final-prod/engine/AUDIT-PIPELINE.md`
(scan-stage claims consistent with corpus; not re-audited line-by-line here).*
