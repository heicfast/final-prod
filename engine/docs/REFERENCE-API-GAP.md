# REFERENCE-API-GAP — the og engine→UI operation inventory vs. our core

> Written by agent C2 (Task 2-c2). Corpus = `easeus-drw-research` @ `03ad46c` (11,495 files;
> RE artifacts + og deployment tree). Gap baseline = [CORE-API-CURRENT.md](CORE-API-CURRENT.md)
> (our v0.4.1-parity tree @ `22e9657`: 78 cr_ui exports, 9 CLI verbs, ~25 C++ modules).
> Every reference claim carries a corpus citation (`file:line` in the corpus repo, paths
> relative to `easeus-drw-research/`). Our-side claims cite the cr_ui export / CLI verb and
> the CORE-API-CURRENT.md section. Only **corpus-documented** operations are listed (§d
> records the corpus-silent expectations so we do not invent them).
> Method note: the Qt slot→class index (`scripts/slot_class_index.json`) is EMPTY (`{}`); the
> operation inventory was therefore assembled from the decoded-moc / connect-wiring / RSF /
> MOM-bridge / architecture artifacts (the corpus's own resolved inventories), not from raw scans.

---

## (a) REFERENCE OPERATION INVENTORY

### A1. Scan lifecycle

| # | Operation (corpus term) | Semantic shape | Corpus citation |
|---|---|---|---|
| R-01 | `startScan(device, 0)` — orchestrator vtable+0x18 | UI hands a `ScanDeviceInfo` to `CDetailPage::setScanDevice` → scan orchestrator singleton starts the scan on the target; page stack switches to detail (scan+results same page). Async; progress via MOM bus (R-03). | `re-workspace/artifacts/UIUX/13-ui-core-wiring-state.md:33`; full chain `:20-40`; `UIUX/04-userflow-state-machine.md:44-66` |
| R-02 | Pre-scan prep + version gating — `CScanMgr::vft+0x1f0(0)` / `vft+0x1f8(1)` | Scan-path preparation calls on the scan manager + `CVersionCtrl` (licensing/version) prep **wired into the scan path** before engines run. Synchronous pre-conditions. | `UIUX/13-ui-core-wiring-state.md:11-12,24-25`; `UIUX/04-userflow-state-machine.md:46-49` |
| R-03 | Engine→UI event bus — `doDispatchMessage(int,void*,int&)` | One Qt signal fans out engine messages (progress, file counts, tree updates) to **15 subscriber classes** (`onReceiveData`); bridge is thread-safe (same-thread/queued marshaling in `CMessageProxy`/`CMessageSend`). Push model for events. | `UIUX/25-connect-wiring-graph.md:41-62`; `re-workspace/artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md:16-58` |
| R-04 | Scan progress (pull-polled) — 38 `timeout()` consumers; `sigPercentChange` | Progress **percentages are pull-polled** via QTimers (`onGet*Progress`), only finish-events push; the sole progress-push signal is `CPEUISink::sigPercentChange`. | `UIUX/25-connect-wiring-graph.md:104-116,186-188`; `UIUX/20-moc-surface-sink-architecture.md:144` |
| R-05 | Scan status text / sector map — `ScaningRecoverMessage`, `AnalysisGridWidget` | Live status line ("%1 file chunks scanned..." milestones) + sector-grid (disk-map) painting fed by engine callbacks. | `UIUX/08-scan-phase-deep-dive.md:8-15,49-54` |
| R-06 | Two-phase quick→advanced scan — `sigQuickScanFinished(int)` ×2 → `onQuickScanCompleted(int)` | Quick pass ends → `CQuickScanCompleteWnd` offers recover-now while **Advanced Scan continues in background** ("Advanced Scan is trying to find more other lost files for you while recove[ring…]"). Engine-side: advanced = ALL full searchers + RAWSearcher in parallel. | `UIUX/25-connect-wiring-graph.md:69-78`; `UIUX/08-scan-phase-deep-dive.md:34-36`; `re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md:255-261` |
| R-07 | Stop scan — `CStopScanThread` + manual-reset abort event @ `CScanMgr+0x108` | Cooperative cancel: confirm dialog ("Stop searching for lost data?" risk framing) then thread + `CreateEventA` abort signal. | `UIUX/08-scan-phase-deep-dive.md:26-28,55`; `UIUX/13-ui-core-wiring-state.md:42` |
| R-08 | Scan finish — `sigScanFinished()` (7 consumers) | Full end → completion dialog with stats grid (Files found/scanned/used time/size/organized/video repaired/to-be-repaired) + "Organizing found files..." tree-building phase. | `UIUX/08-scan-phase-deep-dive.md:31-33,54`; `UIUX/25-connect-wiring-graph.md:70-77` |
| R-09 | Progressive results during scan | Files stream into the tree while engines run (`CDirectoryTreeManager` + classify/filter/sort worker threads); preview + recover available mid-scan. | `UIUX/09-results-phase-deep-dive.md:62-64`; `UIUX/10-preview-subsystem.md:37-38` |
| R-10 | Engine plugin loading — `EUCreate<FS>Searcher` / `EUCreate<FS>QuickSearcher` | `LoadLibraryW` × 10 engine DLLs → `GetProcAddress` factories returning `I{Quick,}CompleteFileSystemSearchController*`; FS-type code → factory map (0-2 FAT, 3 NTFS, 4-5 EXT2, 7 exFAT, 8 HFS, 9 EXT4, 10 ReFS, 13 ZFS, +Btrfs). | `re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md:9-34`; `architecture/05-scan-engine-architecture.md:29-43`; `DRWUI-exe--main-ui-architecture.md:239-253` |
| R-11 | Deep-scan RAW gate — settings `+0x60` toggle | User/config toggle consulted at complete-scan build: gates `EUCreateRAWSearcher` creation. | `DRWUI-exe--main-ui-architecture.md:287-290` |
| R-12 | Carved-file naming/filter callback — `std::function<string(string,string)>` @ RAW vtable+0x48 | DRWUI registers a per-file naming/filter hook into the carving engine at scan setup. | `architecture/05-scan-engine-architecture.md:93-94`; `DRWUI-exe--main-ui-architecture.md:256-260` |
| R-13 | Carver/analyser enumeration — `EUGetAnalyserList/Map/Multimap`, `GetRawUserInfoMap` | RAWSearchDLL exports let the host enumerate all carvers + capabilities ("which formats can we carve"); 190+ analysers, raw_signs.json 139-ext signature DB with confidence scoring. | `SearchDLL/searchdll-family--architecture.md:17,50-73`; `architecture/05-scan-engine-architecture.md:86-92` |
| R-14 | Scan-time filter constraint — `FilterPath=` / `CDoScanPartition::PushFolderFilter` | The advanced-filter condition is **pushed INTO the scan controller at scan start** so pre-set filters constrain incremental tree insertion, not just post-hoc list filtering (runtime-verified). | `UIUX/31-filterbar-family-decode.md:140-143`; `UIUX/30-interaction-driver-ground-truth.md:96` |
| R-15 | Lost-partition "smart scan" — `CDoScanLostPartition`, `CPartitionScanFrame` | Whole-disk scan for lost partitions ("Smart Partition Scanning", "Scanning disk sectors: %1"); found partitions listed w/ Disk/Basic/Dynamic/MBR/GPT/FS properties; controls Recover/Stop/Home; "Preview for disk %1". | `UIUX/08-scan-phase-deep-dive.md:19-23`; `architecture/04-recovery-workflow.md:63-69` |
| R-16 | Drive-disconnect error states | Mid-scan target removal: "Drive Disconnected — Unable to continue scanning. Reconnect your drive and try again." Retry/Cancel; same triad for NAS/Linux/Cloud offline. | `UIUX/08-scan-phase-deep-dive.md:39-47` |
| R-17 | Deep-scan marketing copy / sector-by-sector framing | "Advanced Scan is searching files sector by sector..." — the advanced pass UI contract (all engines + RAW in parallel). | `DRWUI-exe--main-ui-architecture.md:135-137` |

### A2. Results-tree query / filter / navigation

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-18 | Tree fill + worker pipeline — `CSetClassifyThread / CSetFilterThread / CSortNodeThread / CCheckNode(Thread/s) / CSelNodeThread` | Post-scan "Organizing" phases run classify → check → filter → sort → selection on worker QThreads off the UI thread; tree never blocks. | `UIUX/13-ui-core-wiring-state.md:49`; `UIUX/09-results-phase-deep-dive.md:62-64`; `architecture/04-recovery-workflow.md:99-113` |
| R-19 | Dual tree modes — Path tree vs Type/classify tree (`IsTypeTree()`) | One class (`CLeftTreeView`, 47 moc methods) specialized by virtual dispatch; two filter bars (one per mode) toggled by `sigShowFilterBar(bool)`. | `UIUX/20-moc-surface-sink-architecture.md:30-37`; `UIUX/31-filterbar-family-decode.md:78-92` |
| R-20 | Special result branches | 5 distinguished roots: **Tags Extracted** (EXIF/tag grouping), **File Name Lost** (raw-carved by content), **File Path Lost** (carved w/o reconstructable path), **Versions node** (NTFS file-record multi-version), **Unimportant** (collapsed junk bucket). | `UIUX/09-results-phase-deep-dive.md:27-36` |
| R-21 | Search — `CSearchLineedit` w/ "Optional Type" dropdown | Inline search by name + type scope (Pictures/Videos/Documents/Audio/Archive/Emails/Bookmarks/**Unsaved File**); NLT natural-language search funnel (`CSearchNltPopWnd` coach); empty-results strategies. | `UIUX/09-results-phase-deep-dive.md:49`; `UIUX/25-connect-wiring-graph.md:146-151` |
| R-22 | Advanced filter — `AdvanceFilterCondition` (0x48 B, 7 condition groups) | Typed condition struct (type/time/size/state/tag/path/search) broadcast by value via `sigFilterChange(cond,bClear,bForce)` → `CSetFilterThread` → model `SetFilter` virtual (+0x10); empty condition → NULL = clear. Per-view-mode persistent store (`CAdvanceFileFilterMgr`, 3 slots). Live match counts (`CFilterCountFrame`). | `UIUX/31-filterbar-family-decode.md:55-64,94-125,144-147`; `UIUX/09-results-phase-deep-dive.md:51` |
| R-23 | Sort — `sigSort(int,bool)` → `onDoSort`; async worker sort | Column sort requests; worker-thread sort completion relay `sigAsyncSortRet(int,bool,bool)`; sortable headers Name/Size/Type/Date Modified + recovery-chance column. | `UIUX/25-connect-wiring-graph.md:132-135`; `UIUX/09-results-phase-deep-dive.md:53` |
| R-24 | Lazy tree expand — `sigPathItemExpand`, async expand-on-scroll | Path-node lazy load on expand; scroll-driven deep-tree expansion on a worker QThread at TimeCritical priority (1 s debounce) re-entering UI via `finished()`. | `UIUX/20-moc-surface-sink-architecture.md:56,194-202` |
| R-25 | Selection / state queries — `sigGetRightIndex(int&,int&)` | By-reference synchronous state queries (selection checkpoint, busy checks `sigIsBusy(bool&)`) — cross-component polling without field exposure. | `UIUX/09-results-phase-deep-dive.md:20`; `UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:41` |
| R-26 | Recoverability grading — `CRecoverability` + `CCheckNodeThread` | Background std::thread pass grades every node (0=poor, 2=high; NTFS/ReFS/Btrfs take deep validation path; device.kind ∈ {1,4} → grade −2); **re-runs per selection on Go-to-Recover** (runtime-verified "Recovery ability matched=… " re-fire). `ICheckRecoverable` = pluggable checker. | `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md:33-52`; `UIUX/31-filterbar-family-decode.md:103-105`; `UIUX/30-interaction-driver-ground-truth.md:76-77,91-92` |
| R-27 | Post-scan statistics — `RecoveryAssistant` | At scan completion computes/logs TotalFileCount, rawCount, lostPartition counts, quick/deep counts, per-category counts (IDs 5-0xB — same type-ID space as `IFileProperties+0x60`). | `DRWUI-subsystems--assistant-grading-preview.md:9-31` |
| R-28 | Thumbnails — `CGetListThumbnailThread` | Async per-row thumbnail fetch; list-side icon streamer for remote lists pushes pixmaps by index (`sigFinished(int,QPixmap)` / `sigPushPixmapGetIndex(int)`). | `UIUX/13-ui-core-wiring-state.md:50`; `UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:20` |
| R-29 | AI layer — "AI Recommend" + dedup | AI-ranked results view ("AI Recommend is on. Showing %1 recommended files."), AI dedup window (`DrwAIDedupWindow`), "AI Recovery Insight:" cards. | `UIUX/09-results-phase-deep-dive.md:55`; `UIUX/11-recovery-phase-funnel.md:51` |
| R-30 | Freeze/Unfreeze coordinated-UI protocol | During model-destructive phases (partition apply, recover-all): `sigBeforeFreeze` → 4-stage logged prep → frozen shell → `sigUnfreeze`. Runtime-verified at auto-save. | `UIUX/20-moc-surface-sink-architecture.md:176-192`; `UIUX/30-interaction-driver-ground-truth.md:64-68` |
| R-31 | Config data layer | FileFilter.xml (8 categories / **1,481** extensions after CORRECTIONS #5), PriorityDisplayType.xml display priority, usefullostfiles.xml whitelist promoting carved RAW photos, RawConfig.ini **34** carve toggles, FilterFolder.ini junk rules, config.ini `[RecoveringTips]`. | `UIUX/09-results-phase-deep-dive.md:44-45`; `re-workspace/artifacts/CORRECTIONS.md:5`; `architecture/05-scan-engine-architecture.md:104-112` |

### A3. Preview

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-32 | Preview open per type — `PreviewMainDialog` + `IPreviewStub` family | Double-click / row action → `CPreviewController`/`CPreviewModel` resolve previewer by file type → embedded per-type window (Image/Video/RichText/Archive/PDF); shell carries its own Recover CTA. | `UIUX/10-preview-subsystem.md:8`; `UIUX/18-preview-architecture.md:8-19,32-34` |
| R-33 | Streaming preview — `PreviewDataSource` / `IPreviewSink` (N-088: `IInWindowStream` removed from this row — M-028 proved it is the 7-Zip/LZMA SDK input interface used in the recovery export loop, not og's preview surface) | Large media previews read ranges on demand instead of whole-file load (scrubbing, 100 MB-cap edge). | `UIUX/10-preview-subsystem.md:33-34` |
| R-34 | Preview caps & gating | Size cap "Files larger than 100MB are not available for preview."; video time cap "15s preview only" (QTimer on `VideoPreviewWnd`); trial gating "Trial version is preview-only."; capability gap "Preview is not supported, recover to view this partition". | `UIUX/10-preview-subsystem.md:22-27`; `UIUX/18-preview-architecture.md:15` |
| R-35 | Damage-aware preview grading | Failed preview still grades the file ("But this file is likely recoverable based on its structure." / "Recovery Chances: High") — bridges preview → repair. | `UIUX/10-preview-subsystem.md:29-30` |
| R-36 | Preview during scan | "You can preview and recover your files during the scanning process." — preview reads from the live scan session's in-memory metadata + engine sector reads. | `UIUX/10-preview-subsystem.md:37-38` |
| R-37 | Archive browse-inside preview | Full MVC over damaged zip contents (`ArchivePreviewModel` QAbstractItemModel invokables: index/row/column/parent/child/rowCount/columnCount/data). | `UIUX/10-preview-subsystem.md:17`; `UIUX/20-moc-surface-sink-architecture.md:170-174` |
| R-38 | Media helper IPC — `VAPreviewListener` | TCP `127.0.0.1:20171/20172` + ZeroMQ REP control channel to spawned `EaseUS_VideoViewer.exe`; protocol `[602-byte MediaInfo header][≤1 MiB BMP]`, request-id correlation, 10 s bounded wait, HGLOBAL handoff. | `DRWUI-exe--main-ui-architecture.md:291-313`; `DRWUI-subsystems--assistant-grading-preview.md:66-98` |
| R-39 | External viewer processes | Office/PDF/HWP previews are separate processes (`EUOfficeViewer.exe`, `PDFViewer.exe`, `EUHwpViewer.exe`) launched per file type with process-tree termination on close. | `UIUX/10-preview-subsystem.md:4,16`; `UIUX/13-ui-core-wiring-state.md:82` |

### A4. Recovery

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-40 | Recover selected — `sigRecoverBtnClicked` → destination → `IRecoveryController` family | Selection → `ChooseDestinationPathDialog` (Local/Cloud tabs) → strategy controller (`CGeneric/CDeleted/CComplete/CRecycleBin/CCloud RecoveryController`) → `RecoverProgressDialog` → `RecoverResultDlg`. | `UIUX/11-recovery-phase-funnel.md:53-56`; `DRWUI-exe--main-ui-architecture.md:98-105` |
| R-41 | Destination validation | Hard block "Cannot save to original partition" (source partition); overwrite-warning ladder for same-disk; `PathIsNetworkPathW` branch; local: opens `\\.\X:` raw volume of destination (writability + same-volume detection); path errors ("recovery path is too long", numbered taxonomy 1-6+). | `UIUX/11-recovery-phase-funnel.md:13-17`; `architecture/04-recovery-workflow.md:136-141` |
| R-42 | Recovery engine loop — `CRecoveryMgr::ExportThread` | Worker thread: PID/TID trace → **cancel checks** → destination validation → per-target export loop (CreateDirectoryW, path reconstruction, engine-stream source reads via IFileProperties/IInWindowStream, normal file writes); failures collected for the recovery log; progress/completion callbacks (progress forced to 100.0 on completion). | `architecture/04-recovery-workflow.md:136-154`; `DRWUI-subsystems--assistant-grading-preview.md:54-64` |
| R-43 | Recovery cancel — `doCancelRecovery` | Named cancel path on `CRecoveryMgr` (checkpoint line 0xBF). | `DRWUI-subsystems--assistant-grading-preview.md:56-57` |
| R-44 | Result summary + recovery log | "(%1 files / %2 failed; for details, you may check the recovery log)" — `EndRecoverThread` finalizes and writes the log; failed list surfaced. | `UIUX/11-recovery-phase-funnel.md:19-21,48`; `DRWUI-exe--main-ui-architecture.md:103,144-146` |
| R-45 | Free-quota gate + upgrade flow | `CRecoverLimitDlg` counters ("Recovered: %1/ %2 files", "To be recovered: %1 files"), Give Up vs Recover Remaining; hard-stop "No free recovery remaining."; recovery data-source **type 6 = upgrade flow** ("Recovering_Click_Upgrade") — quota check embedded in the recovery data-source dispatch. | `UIUX/11-recovery-phase-funnel.md:25-30`; `architecture/04-recovery-workflow.md:131-135` |
| R-46 | Repair-during-recovery | "Enable file repair" toggle in destination dialog — corrupted video/PDF/doc files queued to the Fixo chain as they recover; "Video Repaired / Video to be repaired" counters in the completion stats. | `UIUX/11-recovery-phase-funnel.md:8-10`; `UIUX/08-scan-phase-deep-dive.md:32` |
| R-47 | Recover-All / Auto-Recover plan | `sigAllFinished → slotDoAutoRecover → slotBtnAutoRecover` wired state machine; `CAutoRecoverMgr` settings: scope (Suggested Recovery / All Files / Select file), destination w/ validation, **email notification**, "Auto-Recover after Advanced Scan Completed". | `UIUX/25-connect-wiring-graph.md:189-192`; `UIUX/09-results-phase-deep-dive.md:58-59` |
| R-48 | ACL preservation on copy | `CAccessControlList`/`CFileDaclProc`/`CFileSecurityInfo` (shared epmpublic) — recovered files/partitions keep original NTFS security descriptors. | `re-workspace/artifacts/PartitionRecovery/pending-operation-engine.md:45-51` |
| R-49 | Cloud destination upload | Cloud scans can save to cloud (onedrive_cloud_stream / `ICloudDriveUploadTask`, quota-checked). | `architecture/04-recovery-workflow.md:152-154` |
| R-50 | Recovery-funnel telemetry | `Result_Recover`, `Result_Restore`, `Click_Continuerecover`/`Click_Dontrecover`, `Preview_Click_Recover{,_Nas,_Linux,_SD}` per-destination analytics. | `UIUX/02-string-inventory.md:55`; `DRWUI-subsystems--assistant-grading-preview.md:62-64` |

### A5. Session persistence (SaveScan)

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-51 | Save session — `SaveScanSession` / `CSaveAndRestoreResult::SaveResult` | Writer `CRSDataEx` (mode 1) appends 4 records: session state, volume info (name+SYSTEMTIME+elapsed stats), partition context (**device identity card**: model/serial/fw/WWN + 512-byte boot-sector snapshot), full result tree (BFS, `CFileNodeRS::SaveFileTree`). Trace `"SaveResultPath = %S"`. | `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:11,55-62,133-160,237-239` |
| R-52 | Auto-save on completion | Full session auto-written at scan end with naming `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` into the module-dir `SaveScan\` folder (runtime-verified: `auto_Windows (C_)10_17_16_25_59.rsf`); auto_* enumeration `FUN_140273940`; volume-name sanitization (`/ * : ? " < > \|` → `_`). | `SaveScan/rsf-format--scan-session.md:443-451`; `UIUX/30-interaction-driver-ground-truth.md:64-67,86` |
| R-53 | Load/restore session — `RestoreScanSession` / `CFileNodeRS::RestoreFileTree` | Reader `CRSDataEx` (mode 0) chain-walks record headers by 16-byte session id, decompresses chunked-zlib, rebuilds the tree; restore path **with progress** ("Start restore result."); `sigLoadFinished(bool)` → `onLoadScanRetFinished`. Record-3 identity card lets restore **verify the same volume is re-attached** and re-materialize partition context without re-enumerating. | `SaveScan/rsf-format--scan-session.md:47-52,122-123,237-239`; `UIUX/25-connect-wiring-graph.md:77` |
| R-54 | Save/load progress + cancel | Save reports **40%→~65%** progress (`40 + 50·processed/(2·remaining)`); restore mirror `40 + 50·done/(total+done)`; **cancellation checked after every child** via `IUIProgressInfo::IsCancelled` — on cancel writer tears down and returns 0. | `SaveScan/rsf-format--scan-session.md:344-376` |
| R-55 | Corrupt-file graceful degradation | On any decompression failure the reader **falls back to memcpy of the raw payload** (recovery-product requirement). | `SaveScan/rsf-format--scan-session.md:98-99` |
| R-56 | Session list / "Continue last scanning" | `LastScanRecordView` UI page listing past sessions; home banner "Continue last scanning · Scan time: · Scan files: · Scan disk: · Load" (telemetry `Result_Loadsession`); import-from-file `"(1-Import Scan Result"`. | `SaveScan/rsf-format--scan-session.md:11`; `UIUX/07-home-phase-deep-dive.md:21`; `architecture/04-recovery-workflow.md:156-163` |
| R-57 | Adjacent config cache | `DRWCfg.bin` AES-256-CBC config cache (hardcoded key) — persisted app config, separate from .rsf. | `SaveScan/rsf-format--scan-session.md:453-461` |

### A6. Report-export

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-58 | Recovery log | **HTML report** (N-159 decode: `CSaveFailedFilesWithHtml` — per-failure collection slot DRWUI :642729-642746, the row grammar with the per-file tree-classification column :642755-642822, opened via ShellExecuteW :642885) written at export completion; surfaced as "(%1 files / %2 failed; for details, you may check the recovery log)"; numbered failure-cause taxonomy (items 1-6+) shared with destination validation copy. **This is the ONLY corpus-documented results-export surface — no CSV/list-export verb is documented.** | `UIUX/11-recovery-phase-funnel.md:16,19-21`; `DRWUI-exe--main-ui-architecture.md:103`; DRWUI_decompiled.c :642669-642892 (the generator class family, N-159); grep-negative: no export-list/csv verb in artifacts |
| R-59 | Crash/diagnostic report | `DRW.exe` watchdog → `DRWReport.exe` (minidump + Elapsed/Error/Result JSON + send_dmp_with_aliyun upload) → DRWUI restart; 2 MB shared log buffer flushed every 5 s. | `architecture/04-recovery-workflow.md:177-180`; `re-workspace/artifacts/DRW-exe--launcher.md` (per index) |

### A7. Health diagnostics

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-60 | SMART attribute polling — `CAtaSmart` multi-transport | Per-disk attribute polling with transport-specific accessors (SAT/ATA passthrough, CSMI for Intel RST, NVMe incl. Realtek bridge); USB-bridge table (CrystalDiskInfo lineage). | `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md:15-23` |
| R-61 | Periodic refresh — `RefreshThread` + `DiskInfoMgr` | Background periodic SMART refresh; QML bridge invokables `diskInfoMgr`: `showWindow()`, `refreshDisks()`, `firstCautionIndex()`, `isTrayExist()`, `setWindowIcon()`; tray-resident guarding; notification-launched console. | `DiskHealth/diskhealth--smart-companion.md:20-28`; `UIUX/19-diskhealth-quick-ui.md:9-13,34` |
| R-62 | Health verdicts | "Good" / "Caution!" / "Normal" states, caution card "Your disk may have physical failure:" + "View Solutions"; jumps to first caution disk. | `UIUX/19-diskhealth-quick-ui.md:24-31` |
| R-63 | TRIM status per home row | "TRIM OFF" / "TRIM ON" chip on every partition row — SSD-aware recovery-chance indicator paired with DRW's recovery advisories. | `UIUX/07-home-phase-deep-dive.md:17`; `DiskHealth/diskhealth--smart-companion.md:29` |
| R-64 | Bad-sector skip visibility | `BadSectorTipsDlg` — "Program is trying to skip bad sectors to find data." coaching during scan. | `re-workspace/artifacts/config-files/config-data-reference.md:55`; `UIUX/03-window-class-map.md:125` |
| R-65 | Volume/disk check — `FileSystemCheck.mo` + `WinChkdsk.exe` | chkdsk-style checks (`CCheckFAT`, `CCheckNTFSVolume`, `CFatChain`, `CHardDriveIO`) in the MOM engine; `WinChkdsk.exe` = console chkdsk wrapper "Launches/monitors chkdsk (legacy mod)". | `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:65`; `re-workspace/artifacts/per-binary/WinChkdsk.md` (Role) |

### A8. Partition recovery

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-66 | Found-partition presentation + recover | Smart-scan results rows (Disk/Basic/Dynamic/MBR/GPT/FS) with Recover/Stop/Home; success copy "%1 partition and its data were successfully recovered!"; lost-partition icon click routes into the flow (`sigLostPartitionIconClicked`). | `UIUX/08-scan-phase-deep-dive.md:21-23`; `UIUX/11-recovery-phase-funnel.md:50`; `UIUX/20-moc-surface-sink-architecture.md:63` |
| R-67 | Partition-table apply (transactional) — `COperationManager` | Every partition action = Controller→Operation→PendingOperation queued transaction w/ apply/undo + composition; writes new table via MainModule ops so found partitions can be recovered to the table, or scanned in place. | `PartitionRecovery/mom-module-system--architecture.md:77-89,96`; `PartitionRecovery/pending-operation-engine.md:16-28` |
| R-68 | EPM editing vocabulary | Full partition-manager op set: create/delete/move-resize/split/merge/align, letter/label/hide/active, format/cluster-size/FS-convert, MBR↔GPT init/convert/rebuild, copy-disk/partition, wipe, extend-boot, RAID-5 rebuild, BitLocker-turn-off. | `PartitionRecovery/pending-operation-engine.md:30-43` |
| R-69 | Boot repair | `CBcdAccess/CBcdBootProc/CBcdEditProc`, `CGenUEFISystemFile/CGenBIOSSystemFile` (EFI/BIOS bootfile synthesis), `CRebuildMBR`, `CExtendBootPart` — so a restored OS boots. | `PartitionRecovery/pending-operation-engine.md:41,59-61` |
| R-70 | LDM dynamic disks | `LdmManager.mo` DmrpDisk/Disk_EP_Cache dynamic-volume config parsing. | `PartitionRecovery/mom-module-system--architecture.md:59` |
| R-71 | RAID-5 rebuild | `CRepairRaid5Controller` reconstructs RAID-5 arrays (pairs with LDM parsing). | `PartitionRecovery/pending-operation-engine.md:64-65` |

### A9. File repair (Fixo chain)

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-72 | Add files for repair | `FileRepairScanPopWnd`: "Add Files"/"Remove All", drag-drop from source chips; purpose "Fix damaged or unreadable files." | `UIUX/12-repair-ui-flows.md:13-18` |
| R-73 | Video repair wizard | Added → auto-repair (header/moov regroup) → damaged? → **Advanced Repair w/ sample video** ("Add Sample Video for Advanced Repair", "Analyzing sample video...") → reconstruct → Repaired/Original/Failed triage → save (gated); "Deep Video Reconstruct" mode; enhanced repair **with pause** states. | `UIUX/12-repair-ui-flows.md:22-32,53` |
| R-74 | PDF repair wizard | Added → encrypted? → password decrypt → repair → page-preview (pixmaps) → save (gated). | `UIUX/12-repair-ui-flows.md:36-45,54` |
| R-75 | Repair progress (polling) | `timeout() → onUpdateRepairProgress` ×8; `onGetAddProgress` ×4 / `onGetSaveProgress` ×4; `doNextRepair` ×2 — `_MsgData` IPC funnels into timer slots. | `UIUX/25-connect-wiring-graph.md:104-110` |
| R-76 | Repair stop | "Stop Repairing for corrupted video?" confirm integrated into the universal scan dialog. | `UIUX/08-scan-phase-deep-dive.md:29` |
| R-77 | Repair-preview + compare | Page-by-page preview of repaired output; before/after compare view (`RepairPreviewCompareWnd`). | `UIUX/12-repair-ui-flows.md:30,42`; `UIUX/18-preview-architecture.md:19` |
| R-78 | Repair save gating + host bridge | Repair+preview runs FREE; saving repaired output is Pro-gated; `EuRepairHost.exe` = out-of-proc activation/licensing bridge spawning the Fixo engines. | `UIUX/12-repair-ui-flows.md:48-49` |

### A10. Video viewer

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-79 | Media preview helper — `EaseUS_VideoViewer.exe` | Spawned with the preview port on its command line; ffmpeg-based player (685 fns); streams thumbnails/MediaInfo back over the TCP+ZeroMQ channel (R-38). | `DRWUI-exe--main-ui-architecture.md:299-305`; `re-workspace/artifacts/per-binary/EaseUS_VideoViewer.md` (Role) |

### A11. License / activation

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-80 | Activation | `euactiveonline 3.1` (`COnlineActivation`): auto-activate against activation.easeus.com[.cn]; fallback Manual-Activation (serial entry `LicenseInputEdit`); result dialogs ("Activate Successful"/"Activate failed."/"Activate Manually"); states Expired/Invalid length/Invalid/**Locked**/**Pirate**; `writeActivateData` lifts gates. | `re-workspace/artifacts/Licensing/licensing-activation-chain.md:9-20,33-47`; `UIUX/11-recovery-phase-funnel.md:44` |
| R-81 | Free-quota counters | "%1 free recovery capacity." / "Recovered: %1/ %2 files" — `CVersionCtrl`-mediated counters + registry license keys. | `Licensing/licensing-activation-chain.md:24-30`; `UIUX/13-ui-core-wiring-state.md:74` |
| R-82 | Update/version check | `CVersionCtrl::doCheckUpdate` on `CheckUpdateThread` + firebase remote-config A/B gating of monetization surfaces. | `architecture/04-recovery-workflow.md:35-37`; `Licensing/licensing-activation-chain.md:14` |

### A12. Driver / service installer status

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-83 | Service install/start/stop/delete — DRWService | `CreateServiceA("Read FVE Server", SERVICE_AUTO_START)` + immediate `StartServiceA`; uninstall = stop+delete via SCM; strings "Service stopping…", "Uninstall service succeed/failed". (This is the **BitLocker key service**, not the IO driver.) | `re-workspace/artifacts/DRWService-exe--bitlocker-service.md:9-30` |
| R-84 | Driver registration (install-time) | eprdtdrv.sys family installed by `setupdrvx64.exe`/installers + WinPE `regdrv.reg`; **on-demand EPMNtDrv load path lives in Device.mo** (engine-internal, not a UI query). | `re-workspace/artifacts/PartitionRecovery/getdriverinfo--tb-module-decode.md:23`; `PartitionRecovery/mom-module-system--architecture.md:98` |
| R-85 | Drive-topology query — `GetDriverInfo_GetDriverInfo` | MOM module (`TBCreateObject` contract) returning drive-letter topology (GetLogicalDrives/GetDriveTypeW) with async completion events. **Naming trap: NOT kernel-driver info; NO SCM APIs (verified).** | `getdriverinfo--tb-module-decode.md:9-29` |

### A13. Misc / cross-cutting

| # | Operation | Semantic shape | Corpus citation |
|---|---|---|---|
| R-86 | Device enumeration + refresh | Device-manager singleton (`FUN_14076df60`); refresh = modal dialog + `FUN_14076e1d0(dev,1)` start → `refreshFinished()`; home lists rebuild on `deviceEnumFinished()`. | `UIUX/13-ui-core-wiring-state.md:14`; `UIUX/04-userflow-state-machine.md:36-39,92`; `UIUX/07-home-phase-deep-dive.md:61-63` |
| R-87 | Hot-plug notify | `CDevNotifyManager`/`CDevNotifyWnd` WM_DEVICECHANGE; device-arrival events flow the MOM pipe (`doReceivedEvent`). | `PartitionRecovery/pending-operation-engine.md:43`; `MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md:113-114` |
| R-88 | BitLocker unlock (in-product) | `CBitlockerStartWnd` → `BitlockerUnlockDialog` → `BitlockerUnlockThread`; password/recovery-key entry; FVE auto-unlock keys via `\\.\Pipe\DRWBitlocker` (SYSTEM-privileged service push); BitLockerLib PowerShell `manage-bde` fallback + registry FVE reads; content decrypt XTS-AES; `CBitlockerTurnOffOperation`. | `UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:33`; `DRWService-exe--bitlocker-service.md:50-89`; `architecture/04-recovery-workflow.md:179-183` |
| R-89 | NAS connect/enumerate | Login → SSH enable guidance → agent download → share enumeration; signals `sigConnectRet(int)`, `SigEnumRet(bool)`, cancel `sigCancelRet(bool)`; retry logic ("retry %d"); transport = EuMrx kernel redirector + user-mode IOCTL session mgmt (0x140320 add / 0x140324 dump / 0x140328 remove / 0x140640 cancel-all). | `UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:8-25`; `DRWUI-exe--main-ui-architecture.md:268-284` |
| R-90 | Linux RAID/box connect | Same agent pattern (zip download); `CLinuxConnectThread`/`CLinuxCancelConnectThread`; Win8.1/8/7 unsupported notice. | `UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:27-29` |
| R-91 | Cloud drive scan | `cloud_session.dll` OAuth2 (Dropbox/GDrive/OneDrive/iCloud) + `EuICloudRecovery.dll` full iCloud client; `CDoScanCloudDevice`; cloud thumbnails. | `DRWUI-exe--main-ui-architecture.md:33`; `architecture/04-recovery-workflow.md:152-154` |
| R-92 | WinPE media creation | `CPECreateThread`/`CPEGetInfoThread`; EuPEMaker (~20 files: makeiso/makepe/grabpe…); ToolsModule WinPE/burn/imaging (PE_Tools, Burn.dll, libvmdk). | `UIUX/15-scenario-flows-nas-bitlocker-linux-pe.md:35-37`; `DRWUI-exe--main-ui-architecture.md:218-222`; `PartitionRecovery/mom-module-system--architecture.md:87` |
| R-93 | Disk-image creation | "Create Image" / "Select a drive or a partition to create a backup disk image" → `DiskBackupDialog` chain; recovery side recommends scanning an image ("We recommend using the image file of this disk for scanning…"); libvmdk.dll shipped. | `UIUX/07-home-phase-deep-dive.md:36-37`; `UIUX/11-recovery-phase-funnel.md:17` |
| R-94 | WeChat recovery | `CWeChat*` window family + `CWeChatThread` + Apowersoft WeChat SDK. | `DRWUI-exe--main-ui-architecture.md:64`; `architecture/04-recovery-workflow.md:33-34` |
| R-95 | Engine error codes | HRESULT-style `0x5002xxxx` engine errors (e.g. open-fail, read-fail) with verbose `Return Error:… File:… Line:… Fuction:` diag channel in release builds. | `architecture/05-scan-engine-architecture.md:61-70` |

---

## (b) GAP CHECK vs `CORE-API-CURRENT.md`

Status legend: **PRESENT** (our equivalent exists), **PARTIAL** (exists with a named delta),
**GAP** (missing). "Ours" cites cr_ui exports (CORE-API-CURRENT §b), CLI verbs (§c), or C++
modules (§a). Corpus-side refs use the R-numbers above.

### Scan lifecycle

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-01 startScan(device) | `cr_ui_scan_create/attach_device/run` (§b.4); CLI `scan` (§c.3) | PRESENT | Shape differs (explicit device handle + attach vs. one ScanDeviceInfo) — capability equal. |
| R-02 pre-scan version/license gating | none (no licensing layer) | GAP | Deliberate scope: licensing is product layer, not recovery core. No hook point exists in `ScanConfig` for a pre-scan gate callback (nearest: `pre_scan_check` M-041). |
| R-03 engine→UI event bus | `cr_ui_scan_set_event_fn` + 10 ABI event types (§b.13) | PRESENT | Ours is typed events, og is `(int,void*,int&)` untyped — ours is the cleaner contract; capability equal. |
| R-04 progress (poll vs push) | `cr_ui_scan_telemetry` any-thread + `SCAN_PROGRESS` event (§b.4/b.13) | PRESENT | Ours pushes progress; og mostly polls. Ours exceeds (telemetry snapshot pollable AND events push). |
| R-05 status text / sector map | `cr_ui_disk_map` (GAP-S6-11 overlay producer, §b.9) | PRESENT | Disk-map data producer exists; text/status strings are UI-side. |
| R-06 two-phase quick→advanced (background deep while usable results) | stages quick→deep→carve in ONE synchronous run; `SCAN_STAGE` events; mid-scan `cr_ui_scan_snapshot` (G-02) lets UI show/recover results while run continues (§b.4) | PARTIAL | No dedicated "quick complete → user gate → deep continues" orchestration verb; the capability is reachable via stage events + mid-scan snapshot, but the og's decoupled background-advanced model (dialog CTA + counters) is a UI-orchestration pattern, not an engine op. |
| R-07 stop scan | `cr_ui_scan_cancel` + `_stopping` + `CStopScan`-equivalent PausableToken (§b.4); CLI exit 3 | PRESENT | Equivalent. |
| R-08 scan finish + stats grid | `cr_ui_scan_get_stats` + completion counters + `cr_ui_scan_assistant` (§b.4) | PRESENT | Equivalent (stats data; grid rendering is UI). |
| R-09 progressive results | `FILES_FOUND` events + mid-scan snapshot + preview/recovery-from-snapshot (§b.4/b.10/b.11) | PRESENT | Ours is designed for exactly this (G-02). |
| R-10 engine plugin factories / FS dispatch | engines are compiled-in; `probe_family` dispatch; `fs_hint` (§a orchestrator) | PRESENT | No dynamic plugin loading (irrelevant — single-vendor core). FS coverage superset (og: 9 engines + RAW; ours: same families incl. BitLocker/UDF). |
| R-11 RAW gate toggle | `--no-carve`, `stage_mask` in `cr_ui_scan_config` (§c.3/d.5) | PRESENT | Equivalent. |
| R-12 carved-file naming/filter callback | `CarvePostProcessor`/`default_namer` + `raw_category_name` in ScanConfig (§a carve) | PRESENT | Equivalent seam. |
| R-13 carver enumeration | `list-carve-formats` CLI + `cr_ui_carve_format_count/info` (N-033 two-layer, §b.8) | PRESENT | Ours: grammar + 139 rawsigns, stable order — matches `EUGetAnalyserList/Map` purpose. |
| R-14 scan-time filter constraint (`PushFolderFilter`) | none — filtering is post-scan only (`cr_ui_query_run` G-12) | GAP | No filter input on `ScanConfig`; og constrains incremental tree insertion with the pre-set `AdvanceFilterCondition` at scan start. |
| R-15 lost-partition smart scan | `--whole-disk` + `lost::discover` + `LOST_CANDIDATE` events + Candidate/Evidence data (§a engines) | PRESENT | Equivalent (fs kind/extents/score exposed; MBR/GPT/Basic/Dynamic labels derivable). |
| R-16 drive-disconnect retry states | none — target loss surfaces as `ERR_READ_FAILED`/engine errors | GAP | No disconnect detection/retry surface; no reattach-and-continue model. |
| R-17 (copy only) | — | — | UI copy, not an operation. |

### Results-tree query / filter

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-18 tree worker pipeline | query runs synchronously server-side (`cr_ui_query_run`, §b.7) | PRESENT | Threading split differs (ours: one query call vs. og's 5 worker classes); engine capability equal. |
| R-19 dual tree modes (Path/Type) | `Query.Category` + `category_for_extension` + `filefilter_entry_count` (§a query) | PRESENT | Data level present; view models are UI-side. |
| R-20 special branches | `Origin/DeletedState` enums + `raw_category_name` (§a tree) | PARTIAL | "Unimportant" = the og's **usefullostfiles.xml extension gate at tree-build** (N-215 re-truth: the reader FUN_1401b8380 loads `config\usefullostfiles.xml` into a 27-ext set with a 101-entry cap — DRWUI:320396; the bucket builder FUN_1403bb970 at the "Build tree finish." handler creates the "Unimportant files" node and partitions the lost branch — `_strlwr` + set-membership for files, `strlen<3` folders, the recursive any-useful-descendant walk for the rest — DRWUI:635764-635910; **NOT the M-018 junk filter**, which is the separate scan-time name-blocklist hide — two different data sources, timing, and visibility; disposition OOS-UI per `PARITY-OG-CORE-INVENTORY.md`'s usefullostfiles.xml row). "File Name/Path Lost" ≈ raw-category + Origin. **Missing: "Versions node" (NTFS multi-version siblings — M-101) and "Tags Extracted" (EXIF tag grouping) — no tree concept for either.** |
| R-21 search + type scope | `tree --filter/--deleted-only`, `Query` filter + Category (§c.4/b.7) | PARTIAL | Name/substring/type present; **no "Unsaved File" (office unsaved-doc) category; no NLT search** (NLT is UI product feature; engine side = free-text query). |
| R-22 advanced filter (7 groups) | `cr_ui_query_run` (G-12 filter/sort/page) + `cr_ui_query_apply_buckets` (M-029 date/size buckets) (§b.7) | PARTIAL | type/time/size PRESENT (buckets og-parity). **Missing condition axes: path, state (deleted-state beyond boolean), tag.** |
| R-23 sort | `Query/SortKey/SortDir` (§a query) | PRESENT | Includes recovery-chance (grade) ordering. |
| R-24 lazy expand | snapshot `_node/_dfs` enumeration + query paging (§b.6) | PRESENT | Full tree is materialized in session; lazy load is a UI pattern over our APIs. |
| R-25 selection/state queries | selection is caller-owned; no by-ref query | PRESENT (n/a) | Selection state is UI-owned by design in our ABI; no engine state to query. |
| R-26 recoverability grading + re-grade | S-023: `grade_tree`, `cr_ui_scan_assistant`, `cr_ui_session_regrade` (per-selection re-grade, §b.6) | PRESENT | Direct parity incl. the re-grade-on-selection behavior (runtime-verified og behavior). |
| R-27 RecoveryAssistant stats | `ScanStats` + assistant stats + report `ScanAnalysis` (§a/b) | PRESENT | Category counts via query totals. |
| R-28 thumbnails | engine landed (`cr_thumbnail`, 05e6786: JPEG/PNG/BMP/GIF/TIFF + PSD/PNM/HDR; dims-first bomb guard; named refusals incl. WMF/EMF/ICO per N-092; EXIF orientation per S-043; 200 MB (0xC800000) max_read default per M-109); `cr_ui_preview_read` remains the read seam; the cr_ui thumbnail ABI export LANDED (cr_ui.h:1233-1297: `cr_ui_thumbnail_format_supported`/`_generate`/`_dimensions`, cr_ui.cpp:3873-3975) | PRESENT (N-180 re-truth: engine + ABI) | async per-row fetch is host-side by design (the og's dedicated thread is UI-side; cr_ui is free-threaded per ABI rules — PLAN-API-EXPANSION:81). |
| R-29 AI recommend/dedup/insight | none | GAP | Product feature; engine side would need a ranking/dedup pass — nothing in core. |
| R-30 freeze/unfreeze | n/a (UI protocol; our model is immutable snapshots — G-02 makes freezing unnecessary) | PRESENT (by design) | Snapshot immutability replaces the freeze protocol's purpose (model safety during mutation). |
| R-31 config layer | `filefilter_entry_count`/`category_for_extension` (FileFilter equivalent); `cr_ui_register_user_signs` (M-048, EUUserData JSON) (§a/b.8); RawConfig 34 carve toggles (`cr_carve` CarveFlags, M-024 + D-97); PriorityDisplayType ordering (the query/filefilter data layers, API-DELTA row 45) | PRESENT | The one dispositioned remainder: usefullostfiles.xml whitelist promotion is OOS-UI (verified — PARITY-OG-CORE-INVENTORY:64 carries the row). |

### Preview

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-32 preview open per type | `cr_ui_preview_open_snapshot/_session` + `cr_ui_preview_metadata` (capped kind/format/EXIF/codec/text) (§b.10) | PRESENT | Engine-level equivalent; rendering is UI. |
| R-33 streaming preview | `cr_ui_preview_read` w/ `read_at`, 4 MiB/call cap, unreadable-span map (§b.10) | PRESENT | Range reads + truthful failure map. |
| R-34 preview caps | `Caps` config in `cr_preview.h:108` (§a) with shipped defaults (N-180 re-truth per M-077: `max_file_bytes = 100 MiB` / `max_decode_ms = 15000`, cr_preview.h:125-126) | PRESENT | The 100 MB / 15 s lines ship as defaults (M-077); trial gating = licensing (absent by scope). |
| R-35 damage-aware grading | node `Grade`/`Confidence` survives preview failure; `cr_ui_preview_unreadable` (§b.10) | PRESENT | Same bridge (grade + failure coexist). |
| R-36 preview during scan | `cr_ui_preview_open_snapshot` mid-scan (§b.10) | PRESENT | G-02 designed for this. |
| R-37 archive browse-inside preview | the ENGINE surface exists: cr_preview.h's format/enumeration inventory (the archive analyser family); the real gap is the ABI/CLI exposure | PARTIAL (N-150's re-shape) | the archive-list verb + cr_ui_archive_entry_count/at LANDED (M-116: the handle-cached extraction over the ABI; the rrecov archive-list verb; ABI-CONTRACT §5's archive flow) — the residual is the browse-inside PREVIEW depth (inner-file preview through the inventory), tracked by M-116's follow-up (would need an archive-listing preview mode). |
| R-38 media helper IPC | none | GAP | No media-helper process/protocol (video/audio decode out of core scope; flag for UI-shim planning). |
| R-39 external viewer processes | none (library API only) | GAP | Deliberate: process spawning is a UI/CLI concern; note for the shim (it will own viewer launching). |

### Recovery

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-40 recover selected | `cr_ui_recovery_create/set_destination/enqueue_*/process` (§b.11); CLI `recover` (§c.5) | PRESENT | Strategy split (generic/deleted/recycle-bin/cloud) collapses into one writer + query — capability equal for local recovery. |
| R-41 destination validation | `set_destination` arms F12 overlap refusal; `ERR_DEST_OVERLAP/UNWRITABLE/FULL/NAME_EXHAUSTED`; CLI `[F12]` same-disk warning (§b.11/§d.1) | PARTIAL | Overlap/unwritable/full present (N-015 dest-full = completed-with-failures). **Missing: path-too-long / invalid-path as distinct error codes (og numbered taxonomy 1-6); network-path branch.** |
| R-42 recovery engine loop + progress | `cr_ui_recovery_process` synchronous + `WriterStats` progress callbacks + per-item outcomes (§b.11) | PRESENT | Equivalent (ours adds post-write hash verification — `ERR_HASH_MISMATCH`, og not documented as verifying). |
| R-43 recovery cancel | `cr_ui_recovery_cancel` — ERR_CANCELLED keeps committed items (§b.11) | PRESENT | Equivalent. |
| R-44 result summary + recovery log | `cr_ui_recovery_report_json` (G-09; valid any time) + `get_stats` (§b.11) | PRESENT | Ours is a structured JSON report over every item (with the N-159 tree-classification axis + failed-bytes split) — exceeds the og's failed-only HTML funnel. |
| R-45 free-quota gate / upgrade flow | none | GAP | Product/monetization layer — out of core scope by design; recorded so the UI shim knows the og behavior. |
| R-46 repair-during-recovery | none | GAP | No repair engine (see A9). |
| R-47 auto-recover plan | none | GAP | No auto-recover orchestration (scope, destination validation reuse, email notify — product layer). |
| R-48 ACL preservation | none — `SafeWriter`/`WinDestIo` write plain files (§a recovery) | GAP | Recovered files lose NTFS DACLs; og preserves them (`CFileDaclProc`). |
| R-49 cloud destination upload | none | GAP | No cloud upload path. |
| R-50 recovery telemetry | none | GAP | Out of scope (telemetry is product layer). |

### Session persistence

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-51 save session (manual) | `cr_ui_scan_save_session` (N-060 atomic tmp+rename) (§b.4); CLI `--out` | PRESENT | Format differs by design (own CRSF vs. og .rsf — D-01); record coverage matches (state, volume info + identity card incl. image path N-066, partition context, disk layout M-042, tree, error map, free map, assistant stats, fs-check, lost candidates). |
| R-52 auto-save on completion | landed (W1c, c95335f): `ScanConfig.autosave_dir` (empty = off, zero-init safe) writes `auto_<vol>_<ts>.crsf` at completion via the existing save path; event + stats field; CLI `scan --autosave-dir D`; the S-044 gates (results-found / not-import / not-already-saved) landed `7b7e095` | PRESENT | Naming per N-061; the og's fixed module-adjacent SaveScan folder vs our caller-dir is the registered D-12 delta; the auto-name sanitizer superset (N-091 addendum) registered. |
| R-53 load/restore session | `cr_ui_session_load` + `cr_ui_session_resume` (mask-validated) (§b.6); CLI `--resume` | PRESENT | Identity revalidation = M-042 + `cr_ui_scan_verify_identity` (boot-hash → `ERR_BAD_CHECKSUM`) — direct parity with og's record-3 identity-card verify-on-restore. |
| R-54 save/load progress + cancel | save: `SAVE_PROGRESS` event (9) + `cr_ui_scan_save_cancel` (§b.4/b.13); load (W1b, ce6bcb0): `cr_ui_session_load_with_events` fires `CR_EVT_SESSION_LOAD_PROGRESS` (stage permille + the byte fields) and takes the `cancel_flag` arg (polled after every child; a torn load never half-claims state) | PRESENT | Load progress model = own stage-permille + 0xFFFFFFFF overall sentinel (the bytes_done/bytes_total fields permit host recomputation of the og's 40+50·done/(total+done) window — the D-12 registration, N-091). |
| R-55 corrupt-record degradation | skip-forward salvage F10/G-01..G-03 (§d.2) | PRESENT | Same spirit (corruption-tolerant reader). |
| R-56 session list / last-session banner | landed (W1a, 78a6552 + eb16335): `cr_ui_session_list` (light-records-only rows; two-call size pattern; newest-first by save_unix with path tie-break per N-116) + the CLI `sessions [--dir D] [--json]` verb + the C-gate block | PRESENT | Row-scope/shape deltas (no `auto_` filter; label[32]; 1 s save time; no device-type field) are N-090's registered set. |
| R-57 config cache | none (no config file persistence) | GAP | Out of scope unless the UI shim needs it (DRWCfg.bin is og's own AES cache). |

### Report-export

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-58 recovery log | `cr_ui_recovery_report_json` (G-09) + `tree --csv/--json` (§c.4) | PRESENT | Ours exceeds: structured JSON report + frozen CSV/JSON tree projections; og documents only the text recovery log. |
| R-59 crash report / watchdog | none in core (CI/launcher concern) | GAP | Out of core scope; note for the host app. |

### Health diagnostics

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-60 SMART/NVMe read | `cr_ui_health_read` (SMART + NVMe + SSD heuristic + TRIM one call) (§b.12); CLI `health` (§c.6); C++ `CAtaSmart`-equivalent multi-path parsing incl. CSMI (§a health) | PRESENT | Includes CSMI + NVMe + ATA passthrough families. |
| R-61 periodic refresh / tray / notifications | one-shot pull only | PARTIAL | No periodic monitor, tray, or event-driven surface; `grade_smart/grade_nvme` C++ seams exist but no refresh loop/daemon in core. |
| R-62 health verdicts | `health::grade_smart/grade_nvme` pure seams + CLI verdict print (§a) | PRESENT | Good/Caution-equivalent grading present at C++ level (not a dedicated cr_ui export; verdict derivable from `SmartReport`). |
| R-63 TRIM chip per row | TRIM state in `cr_ui_health_read` (per disk) | PARTIAL | `cr_ui_enumerate_volumes` carries letter/disk/FVE flag only — **no TRIM/SSD flag in enumeration output** (requires a second health call per disk). |
| R-64 bad-sector skip visibility | `ERROR_RANGE` events w/ M-049 module/operation/attempts + retry Skip (§b.13) | PRESENT | Equivalent (ours richer: attribution triple). |
| R-65 disk check | `check::run_fscheck` (M-041) + CLI `fix --check` + `FS_CHECK` event (§a/§c.7) + the M-075 escalation endpoint: `cr_ui_fsck_volume` ABI + `fix --chkdsk C:` CLI (the direct `chkdsk.exe <vol> /f` form WinChkdsk itself wraps) | PRESENT | The gap closed by M-075 (2026-10-30): the external chkdsk launch is in core, elevated + synchronous + timeout-bounded, WoW64-native (the og's IsWow64Process branch realized as the redirection disable), loud-refusal on absent volumes. The og's WinChkdsk.exe wrapper layer (its .ini config, LogMsg.txt results, VSS coordination) is host-side UI chrome around the same chkdsk — the core's contract is the launch + the raw exit code. |

### Partition recovery

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-66 found-partition present + recover | lost candidates + `cr_ui_apply_lost_partition` (S-029) (§b.5); CLI `apply` (§c.8) | PRESENT | Transactional apply + undo + consent default-deny. |
| R-67 partition-table apply (transactional) | `apply::plan/execute/undo` + `CRUNDO01` persistence (§a/§b.5) | PRESENT | Direct parity (plan → consent → apply → undo). |
| R-68 EPM editing vocabulary | only lost-partition table apply | GAP | Full editing ops (create/delete/resize/format/convert/copy/wipe) out of recovery scope; recorded as corpus-documented but out-of-mission. |
| R-69 boot repair | `bcdfix`/`nvramfix` (M-040a/b) + `reghive::surgery` (M-113: MountedDevices rewrite, BCD device-element re-point, bootstat contract) + CLI `apply --boot-fix/--boot-fix-bcd` (§a/§c.8) | PARTIAL | BCD offset surgery + UEFI NVRAM Boot#### update + the offline hive-surgery family (the recovered-OS write side) landed; the apply-transaction pending-op integration and the Windows-lab boot drill are the recorded follow-ups (M-113's Resolution). |
| R-70 LDM | `parse_ldm_database` + `LdmDiskResolver` + LDM member arm (§a engines) | PRESENT | Dynamic-disk decode present. |
| R-71 RAID-5 rebuild | none | GAP | No RAID reconstruction. |

### File repair

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-72..R-78 Fixo chain (video/PDF repair wizards, progress, stop, compare, save gating) | none — `fix` is FS-structure repair (RawFixer), a DIFFERENT thing which IS covered: `repair::analyze/execute/undo` (S-008) + CLI `fix --apply/--undo` (§a/§c.7) ≈ og `RawFixer.dll`/FileSystemCheck | GAP (file-content repair) / PRESENT (FS repair) | Entire file-content repair category (video/PDF/office/archive/image, sample-based advanced repair, repair preview/compare) missing. The og FS-repair surface (RawFixer + chkdsk-style checks) has direct parity in our core. |

### Video viewer

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-79 media helper | none | GAP | Media decode/render out of core scope; the UI shim must own this (or preview stays byte/metadata-level). |

### License / activation

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-80 activation | none | GAP | Out of core scope by design (product layer). |
| R-81 free-quota counters | none | GAP | Product layer. |
| R-82 update/version check | none | GAP | Product layer. |

### Driver / service installer status

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-83 service install/start/stop/delete | none in core (driver install lives in CI/release scripts; CORE-API §f: no driver-status surface) | GAP | No SCM/service lifecycle ops in the core; og performs them from the app. Our install is release-asset + install.ps1 (driver repo). |
| R-84 driver registration / on-demand load | **PRESENT — the 6-facet `cr_ui_driver_status_t`** (`cr_ui_driver_status` export + `include/cr/win/cr_driver_status.h`'s facet classifier; the old boolean probe REMOVED at src/win/cr_transport.cpp:13 — N-150's re-verified flip; **no version query, no loaded/started/authorized distinction** (CORE-API-CURRENT.md §e/§f, :482-521) | PARTIAL | Presence probe exists; the status/detail surface is a documented core gap (N-041 deleted the client; wire structs doc-only). Note: the corpus does NOT document a UI driver-status query for the og either (see §d) — og's on-demand load is engine-internal. |
| R-85 drive-topology query | `cr_ui_enumerate_disks/volumes` + two-stack C++ enumeration (§b.3) | PRESENT | Equivalent (and the og module is drive-letter topology, not driver info — naming trap). |

### Misc

| Ref | Ours | Status | Delta |
|---|---|---|---|
| R-86 enumeration + refresh | `cr_ui_enumerate_disks/volumes` (§b.3); C++ `win::refresh_disk_properties` (S-029 IOCTL_DISK_UPDATE_PROPERTIES) exists but is NOT exported via cr_ui/CLI | PRESENT (pull model, registered) | **Each `cr_ui_enumerate_disks` call re-queries the OS — no cache, no retained state; calling again IS the refresh** (the N-098 contract sentence: cr_ui.h:472-484 + CORE-API-CURRENT's b.3 row). The og's modal refresh flow re-runs the same enumeration machinery; the modal chrome is host-side. The update-disk-metadata case (the OS seeing a changed disk) is the C++-only refresh IOCTL — deliberately unexported (apply-context only). |
| R-87 hot-plug notify | none — pull-only enumeration | GAP | No WM_DEVICECHANGE-equivalent event; no device-arrival notification. |
| R-88 BitLocker unlock | `cr_ui_open_volume` (auto-unlock → `ERR_FVE_LOCKED`), `cr_ui_open_volume_with_key` (M-033 FVEK XTS), `cr_ui_unlock_volume_with_dictionary` (N-064, 228+user dict), `try_auto_unlock(_offline)` (§b.2/§a win), `cr_ui_turn_off_bitlocker` (M-078, the CBitlockerTurnOffOperation endpoint) + CLI `unlock --turn-off C:` | PRESENT | Strong parity incl. in-stack XTS-AES decrypt; dictionary unlock exceeds og's documented surface. The turn-off op landed (M-078, 2026-10-30): the consented destructive twin (decrypt, never bypass; dual Disable-BitLocker/manage-bde -off chain, the unlock surface's own duality); the OS decrypt continues in the background (the accepted-not-plaintext contract). The service-broker equivalent stays a host concern (ours reads FVE directly when privileged). |
| R-89 NAS connect/enumerate | none | GAP | No remote transports. |
| R-90 Linux connect | none | GAP | — |
| R-91 cloud drive scan | none | GAP | — |
| R-92 WinPE media creation | none (we RUN under WinPE: `win_pe_environment` gate M-030, §e) | GAP | Media creation out of scope; PE-awareness present. |
| R-93 disk-image creation | `cr_ui_image_create` (ABI, class (a)); CLI `image --create` (S-040); reader `cr_ui_open_image` (.vmdk/.vhd/.vhdx/.img/.dmg/.raw; S-030) (§b.2) | PRESENT | Open/parse PRESENT (superset of og's libvmdk VMDK-only); create-image PRESENT (S-040: flat-raw producer, atomic commit + .sha256 sidecar + truthful .partial; VHD/VHDX writers out of scope — D-147). |
| R-94 WeChat recovery | none | GAP | Product scenario, out of scope. |
| R-95 engine error codes | 34+OK error space across 9 families 0x5002..0x500A (§d.1) | PRESENT | Same HRESULT-family shape (0x5002xxxx engine errors) — parity by construction. |

---

## (c) SUMMARY TABLE

Status counts over the 88 inventory rows (R-01..R-95, minus 3 copy-only rows):
**PRESENT 45 · PARTIAL 10 · GAP 27** (recounted 2026-10-29, the S56 lane, by the awk verdict-column census — the table's own rows; R-84 flipped to PRESENT, R-37 reshaped to PARTIAL; of which ~14 are product-layer/licensing/monetization or
process/UI concerns deliberately out of core scope — flagged below).

| Op (R#) | Reference evidence (corpus) | Ours | Status | Gap delta (one line) |
|---|---|---|---|---|
| startScan (R-01) | UIUX/13:33 | scan_create/attach/run; CLI scan | PRESENT | — |
| pre-scan license gate (R-02) | UIUX/13:11-12 | none | GAP | product layer; no pre-scan gate hook in ScanConfig |
| event bus (R-03) | UIUX/25:41-62 | set_event_fn + 10 event types | PRESENT | — |
| progress (R-04) | UIUX/25:186-188 | telemetry + SCAN_PROGRESS | PRESENT | — |
| status/sector map (R-05) | UIUX/08:8-15 | cr_ui_disk_map | PRESENT | — |
| quick→advanced 2-phase (R-06) | UIUX/25:69-78 | stage events + mid-scan snapshot | PARTIAL | no dedicated quick-complete gate verb; reachable via events+snapshot |
| stop scan (R-07) | UIUX/08:55 | scan_cancel/_stopping | PRESENT | — |
| scan finish/stats (R-08) | UIUX/08:31-33 | get_stats + assistant | PRESENT | — |
| progressive results (R-09) | UIUX/09:62-64 | FILES_FOUND + snapshot | PRESENT | — |
| engine dispatch (R-10) | SearchDLL arch:9-34 | probe_family + fs_hint | PRESENT | compiled-in vs plugin — capability equal |
| RAW gate (R-11) | DRWUI arch:287-290 | --no-carve/stage_mask | PRESENT | — |
| carve naming cb (R-12) | scan-engine arch:93-94 | default_namer + raw_category_name | PRESENT | — |
| carver enumeration (R-13) | SearchDLL arch:17,50-73 | list-carve-formats + format_info | PRESENT | — |
| scan-time filter (R-14) | UIUX/31:140-143 | none | GAP | no filter input on ScanConfig |
| lost-part smart scan (R-15) | UIUX/08:19-23 | --whole-disk + lost::discover | PRESENT | — |
| disconnect retry (R-16) | UIUX/08:39-47 | none | GAP | no disconnect detection/retry surface |
| tree workers (R-18) | UIUX/13:49 | query_run (sync) | PRESENT | — |
| dual tree modes (R-19) | UIUX/20:30-37 | Query.Category + filefilter count | PRESENT | — |
| special branches (R-20) | UIUX/09:27-36 | Origin/raw_category ("Unimportant" = the usefullostfiles.xml gate, OOS-UI — N-215) | PARTIAL | Versions node + Tags-Extracted grouping missing |
| search+type scope (R-21) | UIUX/09:49 | tree --filter; Query filter+Category | PARTIAL | no unsaved-file category; NLT is UI-only |
| advanced filter 7 axes (R-22) | UIUX/31:55-125 | query_run + apply_buckets | PARTIAL | path/state/tag filter axes missing |
| sort (R-23) | UIUX/25:132-135 | Query SortKey/Dir | PRESENT | — |
| lazy expand (R-24) | UIUX/20:194-202 | snapshot _node/_dfs + paging | PRESENT | — |
| state queries (R-25) | UIUX/09:20 | caller-owned selection | PRESENT (n/a) | — |
| grading + re-grade (R-26) | DRWUI-subsystems:33-52 | S-023 + session_regrade | PRESENT | — |
| assistant stats (R-27) | DRWUI-subsystems:9-31 | ScanStats/assistant/ScanAnalysis | PRESENT | — |
| thumbnails (R-28) | UIUX/13:50 | cr_thumbnail engine landed (05e6786; S-043/N-092/M-109-era updates: 200 MB max_read); cr_ui thumbnail ABI landed (cr_ui.h:1233-1297) | PRESENT (N-180 re-truth) | async per-row fetch is host-side by design (free-threaded ABI) |
| AI recommend/dedup (R-29) | UIUX/09:55 | none | GAP | product feature; no ranking/dedup pass |
| freeze protocol (R-30) | UIUX/20:176-192 | immutable snapshots (G-02) | PRESENT (by design) | — |
| config layer (R-31) | UIUX/09:44-45; CORRECTIONS:5 | filefilter_entry_count + user signs; RawConfig 34 carve toggles (M-024); PriorityDisplayType + SumaryFileType (N-132's query data layers) | PRESENT (agrees with the primary row) | the one dispositioned remainder: usefullostfiles whitelist = OOS-UI (PARITY-OG-CORE-INVENTORY:64) |
| preview open (R-32) | UIUX/10:8 | preview_open_* + metadata | PRESENT | — |
| streaming preview (R-33) | UIUX/10:33-34 | preview_read (read_at, 4 MiB) | PRESENT | — |
| preview caps (R-34) | UIUX/10:22-27 | Caps seam + shipped 100 MB/15 s defaults (cr_preview.h:125-126, M-077) | PRESENT (N-180 re-truth) | — |
| damage grading (R-35) | UIUX/10:29-30 | node Grade + unreadable spans | PRESENT | — |
| preview during scan (R-36) | UIUX/10:37-38 | preview_open_snapshot | PRESENT | — |
| archive preview (R-37) | UIUX/10:17 | none | GAP | no archive-content enumeration |
| media IPC (R-38) | DRWUI arch:291-313 | none | GAP | UI-shim concern |
| external viewers (R-39) | UIUX/13:82 | none | GAP | UI-shim concern |
| recover selected (R-40) | UIUX/11:53-56 | recovery_* family; CLI recover | PRESENT | — |
| destination validation (R-41) | arch/04:136-141 | F12 + DEST_* errors | PARTIAL | path-too-long/invalid-path + network branch missing |
| recovery loop (R-42) | arch/04:136-154 | recovery_process + WriterStats | PRESENT | ours adds hash verify |
| recovery cancel (R-43) | DRWUI-subsys:56-57 | recovery_cancel | PRESENT | — |
| summary/log (R-44) | UIUX/11:19-21 | report_json (G-09) | PRESENT | ours structured JSON (exceeds) |
| quota gate (R-45) | arch/04:131-135 | none | GAP | product layer |
| repair-during-recovery (R-46) | UIUX/11:8-10 | none | GAP | no repair engine |
| auto-recover (R-47) | UIUX/25:189-192 | none | GAP | product orchestration layer |
| ACL preservation (R-48) | pending-op:45-51 | none | GAP | SafeWriter drops NTFS DACLs |
| cloud upload (R-49) | arch/04:152-154 | none | GAP | no cloud dest |
| recovery telemetry (R-50) | UIUX/02:55 | none | GAP | product layer |
| session save (R-51) | SaveScan:55-62,133-160 | scan_save_session (N-060) | PRESENT | own CRSF format (D-01) |
| auto-save (R-52) | SaveScan:443-451; UIUX/30:64-67 | autosave_dir + completion auto-write + S-044 gates | PRESENT | W1c c95335f + 7b7e095; folder convention = registered delta |
| session load/restore (R-53) | SaveScan:47-52,122-123 | session_load + resume | PRESENT | identity revalidation parity (M-042) |
| save/load progress+cancel (R-54) | SaveScan:344-376 | SAVE_PROGRESS evt + save_cancel + load events/cancel_flag | PRESENT | W1b ce6bcb0 |
| corrupt tolerance (R-55) | SaveScan:98-99 | skip-forward F10/G-01..03 | PRESENT | — |
| session list (R-56) | SaveScan:11; UIUX/07:21 | cr_ui_session_list + sessions verb (N-116 time order) | PRESENT | W1a 78a6552/eb16335; N-090 carries the row-shape deltas |
| config cache (R-57) | SaveScan:453-461 | none | GAP | out of scope (shim may need it) |
| recovery log/report (R-58) | UIUX/11:16,19-21 | report_json + tree --csv/--json | PRESENT | ours exceeds (structured export; og has none documented) |
| crash report (R-59) | arch/04:177-180 | none | GAP | host-app concern |
| SMART/NVMe read (R-60) | DiskHealth:15-23 | health_read (one call) | PRESENT | — |
| periodic monitor (R-61) | DiskHealth:20-28 | one-shot pull | PARTIAL | no refresh loop/tray/notify |
| health verdicts (R-62) | UIUX/19:24-31 | C++ grade_smart/grade_nvme | PRESENT | — |
| TRIM per row (R-63) | UIUX/07:17 | health_read per disk | PARTIAL | no TRIM/SSD flag in enumerate_volumes output |
| bad-sector visibility (R-64) | config-data-ref:55 | ERROR_RANGE events (M-049) | PRESENT | — |
| disk check (R-65) | mom-module-system:65 | fix --check (M-041) | PARTIAL | no external chkdsk launch |
| found-partition recover (R-66) | UIUX/08:21-23 | apply_lost_partition (S-029) | PRESENT | — |
| table apply txn (R-67) | mom-module-system:77-89 | apply::plan/execute/undo | PRESENT | — |
| EPM editing ops (R-68) | pending-op:30-43 | none | GAP | out of recovery mission (corpus-documented) |
| boot repair (R-69) | pending-op:41,59-61 | bcdfix/nvramfix (M-040) + reghive::surgery (M-113) | PARTIAL | the hive-surgery family landed core-side; the apply-chain rider + the boot drill remain |
| LDM (R-70) | mom-module-system:59 | parse_ldm + resolver | PRESENT | — |
| RAID-5 rebuild (R-71) | pending-op:64-65 | none | GAP | no RAID reconstruction |
| file repair chain (R-72..78) | UIUX/12:22-55 | none (fix = FS repair, PRESENT vs RawFixer) | GAP | entire Fixo content-repair category missing |
| media helper (R-79) | per-binary/EaseUS_VideoViewer | none | GAP | UI-shim concern |
| activation (R-80) | licensing:9-20 | none | GAP | product layer |
| quota counters (R-81) | licensing:24-30 | none | GAP | product layer |
| update check (R-82) | arch/04:35-37 | none | GAP | product layer |
| service lifecycle (R-83) | DRWService:9-30 | none in core | GAP | install lives in driver-repo CI scripts |
| driver status query (R-84) | getdriverinfo:23 | the 6-facet cr_ui_driver_status_t (present/attached/loaded/started/authorized + version) | PRESENT (N-150 flip) | the facet set covers the og's display axes; the version/loaded/started/authorized axes landed (M-102's surface) — but see §d: og exposes no UI driver-status query either |
| drive topology (R-85) | getdriverinfo:9-29 | enumerate_disks/volumes | PRESENT | — |
| enumeration+refresh (R-86) | UIUX/13:14 | enumerate_*; refresh C++-only | PARTIAL | no standalone refresh export/verb |
| hot-plug notify (R-87) | pending-op:43 | none | GAP | pull-only enumeration |
| BitLocker unlock (R-88) | UIUX/15:33; DRWService:50-89 | open_volume/auto/FVEK/dict/turn-off (M-078) | PRESENT | full family incl. the turn-off op |
| NAS connect (R-89) | UIUX/15:8-25 | none | GAP | no remote transport |
| Linux connect (R-90) | UIUX/15:27-29 | none | GAP | — |
| cloud scan (R-91) | DRWUI arch:33 | none | GAP | — |
| WinPE media creation (R-92) | UIUX/15:35-37 | none (PE-aware run: M-030) | GAP | out of scope |
| image create (R-93) | UIUX/07:36-37 | cr_ui_image_create + `image --create` (S-040; flat-raw, D-147 boundary) | PRESENT | create-image landed (S-040) |
| WeChat (R-94) | DRWUI arch:64 | none | GAP | product scenario |
| engine error codes (R-95) | scan-engine arch:61-70 | 34+OK codes 0x5002..0x500A | PRESENT | — |

---

## (d) CORPUS-SILENT LIST — expected engine→UI ops the corpus does NOT document

(Recorded so we do not invent them as "og parity" items.)

1. **Scan pause/resume** — the corpus documents cancel (`CStopScanThread`, abort event) and
   pause only for *enhanced video repair* ("Enhanced video Paused"), never for scans
   (UIUX/08:55; UIUX/12:28). Our `cr_ui_scan_pause/_resume` exceeds the documented og surface.
2. **Driver status/version query surfaced to the UI** — no DRWUI-side driver-status query is
   documented anywhere in the corpus; og's EPMNtDrv load is engine-internal in Device.mo
   (getdriverinfo--tb-module-decode.md:23; mom-module-system:98). Our missing driver-status
   surface (CORE-API §f) is therefore a *robustness* gap, not an og-parity gap.
3. **Structured export of the results list (CSV/JSON file save)** — the only documented export
   is the text recovery log (UIUX/11:19-21); no list-export verb exists in the og artifacts.
   Our `tree --csv/--json` + G-09 JSON report exceed the documented surface.
4. **Post-write hash verification of recovered files** — no corpus evidence og verifies hashes
   after recovery; our `ERR_HASH_MISMATCH`/`HashStatus` is an undocumented-by-corpus extra.
5. **Resumable partial scan after a crash** — og's .rsf auto-save happens at scan *completion*
   (UIUX/30:64-67); no corpus evidence of mid-scan crash-resume. Our stage-boundary
   checkpoints + `--resume` exceed the documented og behavior.
6. **Scan to a byte-range/sub-region of a volume** — no corpus evidence of a user-facing
   partial-region scan verb.
7. **Concurrent multi-target scans in one app instance** — corpus shows one scan target per
   scan (`m_scanTarget` QString field, UIUX/13:61); no multi-scan API documented.
8. **User-defined carve signature installation at runtime** — the og signature DBs
   (raw_signs.json / EUUserData) ship read-only; no UI flow documents *adding* signatures
   (SearchDLL arch:67-70). Our M-048 `cr_ui_register_user_signs` has no documented og counterpart.
9. **Per-file password attempt on encrypted archives/PDFs during scan** — decrypt flows exist
   only in the repair wizards (PDF password, UIUX/12:40) and BitLocker unlock; no scan-time
   archive-password API is documented.
10. **License seat/offline-grace management beyond the state strings** — corpus documents
    activation states/endpoints only (licensing artifact).
11. **Deduplication/AI ranking engine parameters or API** — only UI strings exist
    (UIUX/09:55); no engine-side AI API is documented (flagged GAP above as product layer).

---

## Appendix: corpus evidence index (files relied on)

- `re-workspace/artifacts/UIUX/` 04, 07, 08, 09, 10, 11, 12, 13, 14, 15, 18, 19, 20, 25, 30, 31 (+ 02, 03 cited)
- `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` (session persistence, fully reversed + validated)
- `re-workspace/artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md` (the engine→UI bridge)
- `re-workspace/artifacts/DRWUI-exe--main-ui-architecture.md`, `DRWUI-subsystems--assistant-grading-preview.md`, `DRWService-exe--bitlocker-service.md`, `DRW-exe--launcher.md` (index)
- `re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md`
- `re-workspace/artifacts/PartitionRecovery/` mom-module-system, pending-operation-engine, getdriverinfo
- `re-workspace/artifacts/Licensing/licensing-activation-chain.md`; `DiskHealth/diskhealth--smart-companion.md`; `per-binary/{WinChkdsk,EaseUS_VideoViewer}.md`
- `re-workspace/artifacts/CORRECTIONS.md` (errata supersede: FileFilter 1,481 ext / RawConfig 34 toggles)
- `architecture/` 04-recovery-workflow, 05-scan-engine-architecture
- Not used (empty): `scripts/slot_class_index.json` = `{}`.
