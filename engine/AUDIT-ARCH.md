# AUDIT-ARCH — Deep Architecture-Parity Audit (Refactor-A)

**Auditor:** Refactor-A (architecture-parity auditor; read-only + this one file)
**Mode:** READ-ONLY audit. No source modified. No git state changed.

## Pins (audit basis)

| Repo | Commit | Role |
|---|---|---|
| core_recovery | `08b44ac28bdcdd6263e7fe20fe0b597c141f87d9` (main; 1 commit after the T1 pin `2ea5d45`) | ours — C++20 engine core |
| easeus-drw-research | `18d6f14a1add9fc761ed7b3732bb1715b15aa79f` (main) | RE corpus (ground truth) |
| rda-driver | `7e88e38c` (v0.2.x, per T1 pin; not re-pinned — not read this session) | transport reference |

## Methodology

- Our files read line-by-line (100%): `src/core/orchestrator/cr_scan.cpp` (1042) + `include/cr/orchestrator/cr_scan.h` (301); `src/core/storage/{cr_cached,cr_devmap,cr_retry,cr_slice}.cpp` + `include/cr/storage/{cr_device,cr_slice,cr_cached,cr_retry}.h`; `src/core/tree/cr_node.cpp` (293) + `include/cr/tree/cr_node.h` (248); `src/core/cr_parallel.cpp` (153) + `include/cr/cr_parallel.h` (107); `src/core/err/cr_error.cpp` + `include/cr/cr_error.h`, `include/cr/cr_types.h`; `src/win/{cr_transport,cr_enumerate,cr_bitlocker,cr_destio,cr_health}.cpp` + all `include/cr/win/*.h` incl. `cr_rda.h` (299); `src/app/rrecov.cpp` (1271); all 5 CMakeLists.
- Corpus read in full: `architecture/00-INDEX.md`, `04-recovery-workflow.md`, `05-scan-engine-architecture.md`; SearchDLL `searchdll-family--architecture.md`, `engine-matrix--factory-contract-verification.md`, `per-engine-function-map.md`; MOM-modules `Mom/Device/DeviceManager/DeviceAdapter/FileSystem/FileSystemAnalyser/Thread/Partition/CallbackOperator.md`, `mom-modules--ui-bridge-and-dependency-matrix.md`; PartitionRecovery `mom-module-system--architecture.md`, `mo-module-map.md`.
- Decompiled-tree verification (targeted Grep + bounded sed, never full-file reads): `ghidra-drwui/DRWUI_decompiled.c` (FUN_140580720 advanced-scan composition @916443+, `EUCreate*Searcher` GetProcAddress sites @905470–917273), `ghidra-searchdll/FATSearchDLL_decompiled.c` (`CFATQuickSearch::CFATQuickSearchWorker` vftable @26931/27130).
- Layering re-verified independently with fresh include-graph greps at HEAD (not trusted from prior docs).
- Dead-code claims verified by Grep across `src/`, `include/`, `tests/`, `fuzz/` (win-layer targets additionally checked against the app layer; note `cr_win` builds only on WIN32 so tests cannot reference it).
- Prior-audit claims (PARITY-ARCH, PARITY-ORCH, PARITY-DELTAS D-18/D-21/U-12) treated as claims, re-verified against code/corpus.

---

## Dimension 1 — Module boundaries & layering

Reference stack: DRWUI (host) → engine DLLs (plugin contract: `LoadLibraryW` + `EUCreate<FS>{Quick,}Searcher` → `ISearcher`/`IQuickFileSystemSearchController`/`ICompleteFileSystemSearchController`/`IRawDirectoryTreeBuilder`) → per-FS controller-is-tree-builder (`DetourDirectoryTree` decorator, 05 §3) → `CPartitionVirtualVolumeDevice` → raw IOCTL/driver (05 §2/§4/§8).
Ours: rrecov CLI → ScanController orchestrator → static engines → ResultTree → PartitionSlice/CachedReader/RetryingDevice → cr::win transports (docs/architecture.md:8-38).

| # | Boundary | og (corpus evidence) | ours (file:line) | Verdict |
|---|---|---|---|---|
| 1 | Host→engine plugin seam | `LoadLibraryW`×10 into globals `DAT_1425e6d30..78`, `GetProcAddress("EUCreate<FS>Searcher")` (05 §2; verified in DRWUI_decompiled.c:917068+); 9/9 factory pairs uniform (engine-matrix §1) | static link + `FsFamily` switch dispatch (cr_scan.cpp:532-617) + per-candidate re-probe (cr_scan.cpp:755) | **DIVERGENT — documented (D-21), justified** (single product, no third-party engine plugins). Registry wording defect: D-21/PARITY-ARCH:152 say "crCreate*-style factories"/"structured OPEN_TARGET preferred" — no `crCreate` exists (Grep: 0) and OPEN_TARGET is never called (see ARCH-1) |
| 2 | Engine controller = tree builder | MI `ICompleteFileSystemSearchController` + `IDirecotryTreeBuilder`/`DetourDirectoryTree` decorator intercepting insertion for progress+dedup+merge (05 §3, constructor vtable seq) | engines emit into a shared `ResultTree` via explicit `tree&,root` params (cr_scan.cpp:542, 552, …); progress/dedup/merge duties split into orchestrator bridge (cr_scan.cpp:218-232), tree hashed indexes (cr_node.cpp:156-188), carve dedup (cr_carve.cpp:117-119) | **STRUCTURALLY-DIFFERENT-BUT-EQUIVALENT** — decorator's three duties all present, relocated; no MI |
| 3 | FS detection/analyser layer | `FileSystemAnalyser.mo` family (mo-module-map.md:29-33) validates candidates pre-scan | `ScanController::probe_family` static (cr_scan.cpp:287-336); lost-partition per-format probes (cr_candidates.cpp:7-13) | **MATCH** (function API vs vtable interface — consistent with in-process design) |
| 4 | Device abstraction | `DeviceAdapter.mo`: `IStorageDevice`, `IVolumeDevice`, `IBlockDevice`, `IDiskDevice`, `IPartitionDevice` (DeviceAdapter.md:9) | `cr::IStorageDevice` + `Geometry`/`ReadErrorMap`/`CancelToken` (cr_device.h:89-121) | **MATCH** (single flattened interface vs og's 5-interface family — narrower but sufficient; no IVolumeDevice split) |
| 5 | Virtual volume for OS-invisible partitions | `CPartitionVirtualVolumeDevice`, "Virtual = works for volumes the OS can't see" (05 §4.2; per-engine census: ×15 in HFS+) | `PartitionSlice` bounded view (cr_slice.h:30-49, bounds enforced cr_slice.cpp:43-50) + `SliceShim` non-owning adapter (cr_slice.h:56-71) | **MATCH** |
| 6 | Read cache | `CStorageDeviceWithCache` "in front to survive MFT re-reads" (ntfs-engine-internals, cited cr_cached.h:4-5) | `CachedReader` LRU 8 MiB/64 KiB (cr_scan.cpp:373; cr_cached.cpp) | **MATCH** |
| 7 | Transport duality | `Device.md` strings: `\\.\EPRDTDRV\%d\%d`, `\\.\EPRDTDRV\%d\Volume\%d`, `\\.\PHYSICALDRIVE%d`, `\\.\HarddiskVolume%d` (Device.md:39-44) | `open_rda_partition`/`open_rda_volume` + `open_physical_drive`/`open_harddisk_volume` (cr_transport.cpp:515-574) | **MATCH** — but the `Volume\%d` leg is dead (ARCH-DEAD-3) and og's *mounted-volume file-API* read path (05 §8: "normal file API when volume is mounted & healthy") has no counterpart: we always read raw (see ARCH-14) |
| 8 | Enumeration | `DeviceManager.mo` (WMI + setupapi + hot-plug; mo-module-map.md:21; MainModule `CDevNotifyManager`/WM_DEVICECHANGE) | `cr_enumerate.cpp` (GetLogicalDriveStringsW + IOCTL_STORAGE_QUERY_PROPERTY + pure-core table parse) | **MATCH minus hot-plug notify** (no WM_DEVICECHANGE equivalent anywhere; unregistered gap, P3) |
| 9 | Partition tables / LDM | `Partition.mo` (MBR/GPT/EBR) + `LdmManager.mo` 91K lines, `Dmrp*`, `EditLdm2*` (mo-module-map.md:24) | `cr::table` GPT/MBR/EBR (cr_table_parse.cpp) + full LDM decode + volume reconstruction incl. RAID-5 XOR (cr_ldm.cpp:1367-1372; LdmVolumeDevice cr_ldm.h) wired unconditionally before FS dispatch (cr_scan.cpp:497-530) + cross-disk GUID resolver (rrecov.cpp:447-479) | **MATCH** for scan/read scope; og's LDM *editing* (`EditLdm2*`) and MainModule table-write path absent (known F-D-5) |
| 10 | Carving | `RAWSearchDLL` 190+ analysers + external `raw_signs.json` + `EUGetAnalyserList/Map/Multimap` host-enumeration exports (05 §6) | `cr::carve` compiled-in versioned registry (cr_rawsigns_data.cpp), 30 formats; no analyser-enumeration seam | **STRUCTURALLY-DIFFERENT — partially documented** (F-D-7 externalization gap); missing host "which formats can we carve" enumeration seam (og exposes it via exports) |
| 11 | Classification/filter/sort | DRWUI-side threads: `CSetClassifyThread` (FileFilter.xml 8 cats/1489 ext), `CSetFilterThread`, `CSortNodeThread` (05 §7.4-7.7) | core-side `cr::query` module (cr_query.cpp) + wizard-side filtering | **STRUCTURALLY-DIFFERENT-BUT-EQUIVALENT** — post-processing moved from UI threads into the core (justified by subprocess UI, ADR-1); coverage gap F-D-7 stands |
| 12 | Recovery copy-out | `CRecoveryMgr::ExportThread` (04 §7.3): dest validation via `\\.\X:` raw open, engine-stream source reads, vtable progress | `SafeWriter` + `WinDestIo` (atomic temp+rename), dest-overlap refusal, `report::build_report` (cr_safewriter/cr_destio/cr_report) | **MATCH-with-deltas** (D-19; ExportThread is off-thread — our `Writer::process` is synchronous) |
| 13 | Session persistence | SaveScan `.rsf`, `SaveRestoreThread` off-thread writer (04 §8; F-B-18) | CRSF synchronous `session::Writer` (cr_scan.cpp:869-928; rrecov.cpp:521-535) | **DIVERGENT — documented (D-01)**; no save-progress callback (F-B-18 open) |
| 14 | Threading kit | `Thread.mo` (`CThread`,`CMutex`,`CEvent`,`CCriticalSection`,`CTimer`) | `cr::ThreadPool` + `PausableToken` (cr_parallel.h; cr_device.h:130-196) | **MATCH** (shape) |
| 15 | Error module | `Error.mo` (`CErrorManager`/`IErrorBase`), `CErrorObject` in MOM | `cr::Err` + `err_text` (cr_error.h:18-63) | **MATCH** functional; og logging seam (Log.mo, 15+ importers) absent = F-D-6 |
| 16 | Engine→UI bridge | `CMessageProxy`/`CMessageSend` thread-marshaling envelope → 15 Qt subscribers (mom-modules--ui-bridge §1) | stdout progress lines + `telemetry()` pull from any thread (cr_scan.cpp:234-258) | **DIVERGENT — documented (ADR-1 subprocess)**; pull-model telemetry is a clean equivalent for a CLI host |
| 17 | Launcher/watchdog/crash reporter | DRW.exe + DRWReport.exe (04 §1, §10) | none | **MISSING — known** (PARITY-ARCH module-map row; process-isolation substitute in wizard) |
| 18 | App layer | DRWUI (Qt) | rrecov CLI + rda-wizard subprocess (other repo) | **MATCH** for core scope |

**Layering verdict: MATCH.** The documented layering (app → orchestrator → engines → tree/session/writer → storage → win) is real in the include graph at HEAD — see Dimension 2.

---

## Dimension 2 — Dependency-direction violations

Fresh include-graph verification at `08b44ac` (commands re-run this session, not copied):

| Rule | Result | Evidence |
|---|---|---|
| core must not include OS/win headers | **PASS (0)** | Grep `#include [<""](cr/win/|windows\.h|winioctl|ntddscsi|unistd|sys/|linux/|io\.h|process\.h|direct\.h)` over `src/core/` + `include/cr/` (non-win) → 0 |
| core headers never pull `cr/win/` | **PASS (0)** | Grep `cr/win/` in `src/core/`, `include/cr/` outside `win/` → 0 |
| engines must not include orchestrator | **PASS (0)** | Grep `cr/orchestrator/` in `src/core/` → only orchestrator's own cr_scan.cpp:13 |
| storage must not include engines/tree/session | **PASS (0)** | Grep over `src/core/storage/` → only cr_device/cr_slice/cr_retry/cr_cached/cr_devmap internal |
| tree/session must not include engines | **PASS (0)** | Grep over `src/core/tree/`, `src/core/session/` → none (session→table crc32 wire only) |
| win layer only downward | **PASS** | src/win pulls `cr/cr_error.h`, `cr/table/*`, `cr/storage/cr_slice.h`, `cr/repair/cr_rawfixer.h` (IWriteDevice interface), `cr/health/cr_nvme.h` — never engines/orchestrator |
| app sits on top | **PASS** | rrecov.cpp:20-42 includes orchestrator/session/table/win stack + windows.h |
| Lateral engine-band edges (allowed by diagram) | recorded | lost→ntfs/fat/exfat (cr_candidates.cpp:7-13), repair→fat/exfat (cr_rawfixer.cpp:15-18), zfs/safewriter→compress, session→table(crc32) — mirrors og reuse (mo-module-map.md: PartitionRecoveryD/RawFixer reuse NTFSLib/FatLib) |

**Zero layering violations found — the prior PARITY-ARCH claim is re-confirmed at HEAD.**
Nit: the diagram (docs/architecture.md:17-18) still doesn't call out the intra-band reuse edges (F-D-10 doc item, still open).

---

## Dimension 3 — Interface shapes

| Interface | og | ours | Verdict |
|---|---|---|---|
| Storage device | `IStorageDevice` ReadAt (engine-side; CPartitionVirtualVolumeDevice wraps partition context); engine chooses file-API vs raw-IOCTL per volume health (05 §8) | `IStorageDevice::read_at(u64 off, void*, u32 len)` byte contract + `geometry()` + `absolute_base()` (cr_device.h:89-99); transport chosen once by CLI | **MATCH on shape; DIVERGENT on read-path selection** — we always read raw; og could use the mounted file API on healthy volumes (ARCH-14, P3, unregistered) |
| Engine entry | factory → controller vtable (Quick/Complete pair per FS, engine-matrix §1) | `scan_<fs>(slice, flags, cancel, progress, tree, roots…, stats)` free functions (cr_fat_engine.h etc.; dispatched cr_scan.cpp:532-617) | **STRUCTURALLY-DIFFERENT-BUT-EQUIVALENT** (function API; Quick/Complete duality collapsed — see Dimension 7) |
| Progress callback | `ulong (*)(OC_PARAM*, void*)` message-style, 3-level ops (start/end/child), large counters, cancel gating (CallbackOperator decode, PARITY-ORCH F-B-6) | `bool (*)(u64 done, u64 total, void*)` engine-level + `bool (*)(u32 permille, void*)` host-level, bridged with stage ids via `StageBridge` (cr_scan.cpp:218-232, 479-484) | **DIVERGENT (known F-B-6)** — no stage-id param on the public callback; stage info only via `telemetry()` pull |
| Cancel | per-record `controller.callback.vt[0x10]` poll (ntfs-engine-algorithms-v2 §1) | `CancelToken::checkpoint()`/`cancelled()` at loop heads + `PausableToken` pause (cr_device.h:110-196) | **MATCH+** (we add pause + pause-compensated elapsed, RE parity) |
| Carve naming | DRWUI registers `std::function<string(string,string)>` naming/filter callback into RAW engine vtable+0x48 (04 §3.3, 05 §6) | `carve::NameCallback` seam exists (cr_carve.h:42) but the namer is hardcoded in the orchestrator (`"Carved%06u.%s"`, cr_scan.cpp:846-854) — the CLI/UI cannot supply it | **DIVERGENT (small)** — naming responsibility moved host→core; seam exists, wiring doesn't (ARCH-13) |
| Write device (repair) | RawFixer.dll internal | `repair::IWriteDevice` (cr_rawfixer.h:54-63) implemented by `RwDevice` (cr_transport.cpp:633-717) | **MATCH** |
| Analyser enumeration | `EUGetAnalyserList/Map/Multimap` host-facing exports (05 §6) | none | **MISSING seam** (host cannot enumerate carve formats) |
| Error values | 0x5002xxxx open/read classes (05 §4.1) | 0x5002xxxx family + 0x5003-0x5009 (cr_error.h) | **MATCH** (D-15 extends) |

---

## Dimension 4 — Lifecycle & ownership

| Aspect | og | ours | Verdict |
|---|---|---|---|
| Engine/module lifetime | DLLs loaded at startup into globals, process lifetime; controllers heap-allocated per scan via factory (CNTFSSearchController 0x940 bytes) | static link; engines are stateless functions; `ScanController` per scan (cr_scan.h:153) | EQUIVALENT (in-process delta D-21) |
| Result objects | refcounted `IFileProperties` family, per-DLL static "engine SDK" duplication (D-21 og-side) | `FileProperties` refcounted (cr_node.h:86-96) shared by hard links; single shared lib | MATCH (improved — kills per-DLL duplication) |
| Node ownership | engine trees ingested by DRWUI `CDirectoryTreeManager`; merge later (05 §7) | `ResultTree` arena owns all nodes, bulk destruction (cr_node.cpp:84-101, 264-287); one shared tree, roots pre-created per stage config (cr_scan.cpp:72-81) | STRUCTURALLY-DIFFERENT-BUT-EQUIVALENT (merge step eliminated; mutex-guarded appends instead) |
| Device handles | Device.mo opens per use; RDA grammar per open | transports opened once per CLI command (`OpenResult` holds parent + slice, rrecov.cpp:124-199); run() wraps `SliceShim→RetryingDevice→CachedReader` unique_ptr chain destroyed at run() end; raw-pointer publication of the retry pipe documented & race-analyzed (cr_scan.cpp:363-377) | MATCH; ownership dance is subtle but documented |
| Session lifecycle | SaveRestoreThread writer (off-thread) | `session::Writer` per save/checkpoint; checkpoint = fresh container per emit (cr_scan.cpp:265-285) | MATCH-with-delta (D-01, F-B-18) |
| Resume | .rsf flags semantics undecoded (UNVERIFIED-4) | `stage_done_mask` truth + `resume()` factory (cr_scan.cpp:1019-1038) — core-complete, CLI-unwired (ARCH-2) | ours ahead; og side unknown |

---

## Dimension 5 — Threading model

| Aspect | og (evidence) | ours (evidence) | Verdict |
|---|---|---|---|
| Engine-internal threads | **YES** — FAT `CFATQuickSearch::CFATQuickSearchWorker` (FATSearchDLL_decompiled.c:26931; fat-engine--deep-dive "threaded QuickSearchWorker"); exFAT `CQuickSearchAndBuildBitmap`; scan workers run in QThreads (mom-modules--ui-bridge.md:49, 42/45-thread census) | engines single-threaded per unit; **host/orchestrator fans out** (cr_scan.cpp:3-8; cr_parallel.h:4-14) | **DIVERGENT-but-registered** (D-18) — the unqualified "engines single-threaded" corpus characterization is wrong (see claim verification #3); behavioral impact none for correctness (both models produce merged results), but our claim text is stale in 3 places (ARCH-4) |
| Host-side pool | CommonThreadPoolMgr + per-stage DRWUI threads (DRWUI-exe--thread-architecture §1) | `cr::ThreadPool` fixed workers, auto hw−1 clamp 1..8, cap 64 (cr_parallel.cpp:13-31; resolve_threads cr_scan.cpp:41-51) | MATCH (shape) |
| Fan-out points | searcher list, UI post-processing threads | carve windows, NTFS remnant chunks, lost-partition candidates (cr_scan.cpp:699-734, 647-662; cr_carve threads flag) | MATCH (equivalent parallelism at the same granularity) |
| Post-processing threads | CSetClassifyThread/CCheckNodeThread/CSetFilterThread/CSortNodeThread/CSelNodeThread (05 §7.4-7.7) — progressive, UI-visible | synchronous core `cr_query` + wizard-side; results visible at stage boundaries (G-02) | DIVERGENT (known F-B-21 browse-while-scanning gap) |
| Ordering/dedup under parallelism | merge phase post-hoc ("structured wins", 05 §7.3) | shared-tree mutex appends + hashed dedup indexes keyed by (record_ref / first-extent+size-class) — order-independent by key (cr_node.cpp:156-188; cr_carve.cpp:117-119) | EQUIVALENT (dedup is key-based, not order-based) |
| Cancellation in threads | per-record cancel poll; CStopScanThread UI-side | token polled at loop heads; `ThreadPool::abort()` drops queue; **but `PoolCancelToken` never wired into the candidate fan-out** (cr_scan.cpp:712 — constructed, unused; F-B-10 open at HEAD) and CLI passes `cancel=nullptr` (rrecov.cpp:487) so no product-level cancel exists | DIVERGENT (wiring gap, ARCH-2/ARCH-6) |
| Pause | og Pause (❚❚) control (dynamic-ground-truth §7) | `PausableToken` parks workers at `checkpoint()`; pause-compensated elapsed (cr_device.h:130-196; cr_scan.cpp:143-206) — core-complete, CLI-unwired | ours ahead in core, unwired in product |

---

## Dimension 6 — Error propagation

| Aspect | og | ours | Verdict |
|---|---|---|---|
| Code family | HRESULT-style 0x5002xxxx, open-fail/read-fail classes (05 §4.1) | `cr::Err` 0x50020001-06 (open/read/write/range/cancelled/retry) + 0x5003-0x5009 families (cr_error.h:18-60) | MATCH (D-15 extends; registered) |
| Diagnostics channel | `cout << "Return Error:0x…" << File/Line/Function` compiled into release engines (05 §4.5) | none in core; CLI `--log` vfprintf only (rrecov.cpp:109-118) | DIVERGENT — F-D-6 logging seam missing (unregistered as a *parityEngine* diagnostic gap) |
| Error object bus | Error.mo CErrorManager + CErrorObject propagation (mo-module-map) | plain u32 codes + `err_text` | MATCH functional |
| Bad-sector tracking | none located in corpus (UNVERIFIED-3) | `ReadErrorMap` persistent through stats+session (cr_devmap.cpp; cr_scan.cpp:388-415 — merged on **every** exit path incl. cancel; F-B-8 now FIXED at HEAD) | ours ahead (R1.6 addition) |
| Abort semantics | first error aborts (PARITY-ORCH row 2) | stage rc != OK → `stamp(rc)` immediate return (cr_scan.cpp:419-440) | MATCH |
| Deterministic vs retriable classification | not located | retry layer distinguishes hard errors (bounds/param — never retried) from IO failures (retried) (cr_retry.cpp:50-53) | ours-only addition |
| Style nit | — | session-load maps failures via raw literals `0x50030002`/`0x50040001` instead of enum names (cr_scan.cpp:947, 954) | P3 hygiene |

---

## Dimension 7 — Orchestration wiring

| Aspect | og | ours | Verdict |
|---|---|---|---|
| Quick/Complete duality | Quick scan = `EUCreate<FS>QuickSearcher`; Advanced = **separate complete-searcher generation** — `FUN_140580720` builds a list of `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher` + `EUCreateRAWSearcher` (verified in DRWUI_decompiled.c:916443+, GetProcAddress sites 917068-917273); quick results carry over ("Advanced Scanning 9% … 1.11M files", dynamic-ground-truth) | **single-pass-with-deep-settings**: quick stage runs the engine pass with deep flags when deep is configured (cr_scan.cpp:450-457, "single walk" design); deep stage adds remnant sweep + lost discovery; carve is a third weighted stage | **DIVERGENT — justified but UNREGISTERED** (PARITY-ORCH §5.1 candidate only; docs/PARITY-DELTAS.md lacks an entry → per that registry's own rule, "a bug, not a delta" until registered) → ARCH-3 |
| Default scan depth | quick by default; Advanced is user opt-in | default = quick+deep+carve; `--quick-only` opts out; `--deep` only enables whole-disk lost discovery (rrecov.cpp:385-397, 430-435) | DIVERGENT (CLI semantics; PARITY-ORCH terminology delta #7) |
| Stage model | Quick pass → Advanced (searchers+RAW inside one advanced pass, live streaming) | quick → deep → carve, weighted 40/35/25, monotonic, stage-boundary visibility (cr_scan.cpp:419-447) | MATCH ordering; packaging delta F-B-21 (open) |
| Lost-partition hook | CGenericRecoveryController inserts lost partitions as scan targets ("Insert lost partition %s…", 04 §3.4) | deep stage, whole_disk mode: discover → materialize slices → per-candidate scans (parallel above 1 candidate) (cr_scan.cpp:664-739) | MATCH |
| LDM hook | partition-layer LdmManager before engine dispatch | probed **unconditionally** before fs_hint switch (cr_scan.cpp:497-530; aliasing rationale in-code; F-B-2 contract drift open) | MATCH behaviorally; contract drift open |
| Carve hook | RAWSearcher appended to advanced searcher list, naming callback registered at wiring time (04 §3.3) | third stage after deep; dedup vs structured tree via `dedup_tree=&tree_` (cr_scan.cpp:833-867) | MATCH hook point; naming seam divergence (ARCH-13) |
| Checkpoint/resume | none located (UNVERIFIED-4) | G-13 stage-boundary checkpoints via `CheckpointSink`; G-07 `resume()` — **core-only, CLI never sets a sink or calls resume** (rrecov.cpp has no wiring; tests cover both, test_pause_resume.cpp:491-599) | ours ahead in core; **product wiring gap** (ARCH-2) |
| Save-on-cancel | n/a (GUI) | docs/architecture.md:80-81 claims "the CLI saves on cancel" — **false at HEAD**: cmd_scan returns 1 on rc!=OK before the save block (rrecov.cpp:503-506 vs 521-535); UI-READINESS-PLAN.md:36 acknowledges the gap | **doc-vs-code contradiction** (ARCH-2) |

---

## Dimension 8 — DEAD / DUPLICATE code register

Verification method per entry: Grep over `src/`, `include/`, `tests/`, `fuzz/` (win-layer targets also checked against `src/app`; `cr_win` cannot appear in Linux tests). "test-only" = referenced exclusively by unit tests.

### 8a. Dead in production (and tests)

| ID | Item | Location | Evidence of deadness | Size |
|---|---|---|---|---|
| ARCH-DEAD-1 | ~~`RdaClient` class~~ **RESOLVED (N-041, session-12): DELETED** — the wire-or-delete decision executed as delete (zero-caller grep re-verified at deletion time; the REFACTOR-PLAN's KEEP ruling was superseded — its cited TRANSPORT-CONTRACT §8 no longer exists and its "UI reads through it" claim is grep-disproven). The class + methods + `rda_bind_structured` + the device's structured-open plumbing + `volume_letter_to_disk` deleted; cr_rda.h narrative corrected; a CI absence-lint guards the symbols' return. The grammar path is the sole production surface.** | deleted | Zero references outside cr_transport.cpp itself (Grep `RdaClient` over src/tests/fuzz → only impl+header+docs). Production opens go through free functions `open_rda_partition`/`open_disk` (rrecov.cpp:138, 456; cr_enumerate.cpp:160). Also **refutes** PARITY-ARCH.md:152 ("core_recovery consumes it exclusively through RdaClient… structured OPEN_TARGET preferred") — the refuted claim is corrected | ~284 impl lines + ~67 decl lines (removed) |
| ARCH-DEAD-2 | cr_rda.h extended wire surface (kept BY DESIGN after N-041: the size-frozen structs/constants are the client-side mirror of the DRIVER's wire protocol - the same ground truth rda_ioctl.h documents on the driver side; the CONSUMER (RdaClient) is deleted, the contract record stays): `kIoctlGetProcName`, `kIoctlGetFileObjName`, `kIoctlGetDiskContextMode`, `kIoctlGetInfo`, `kIoctlGetVersion`, `kIdentInLen/kIdentMinOutLen`, `kInfoFlagFilterMode`, `kVersionMajor/Minor`, `OpenSession`, `StartBackupIn`, `GetNextBlock`, `SessionHandleIn`, `kGetProgressOutLen`, all `kSess*` codes, session-state consts | cr_rda.h:41-48, 70-71, 106-132, 138-209, 211-213 | Per-constant Grep: 0 uses outside cr_rda.h except inside dead RdaClient methods (cr_transport.cpp) | ~110 header lines |
| ARCH-DEAD-3 | `open_rda_volume` (the `Volume\%n` grammar leg) | cr_transport.cpp:540-562 | 0 callers anywhere | 23 lines |
| ARCH-DEAD-4 | `volume_letter_to_disk` | cr_transport.cpp:761-804 | 0 callers (CLI uses `volume_extent` instead) | 44 lines |
| ARCH-DEAD-5 | `win::open_disk_view` | cr_enumerate.cpp:156-164 + cr_enumerate.h:56 | 0 callers — rrecov defines its own *local* `open_disk_view(u32, OpenResult&)` (rrecov.cpp:132) with different semantics | 9+2 lines |
| ARCH-DEAD-6 | `bitlocker::status_text` | cr_bitlocker.cpp:220-231 | 0 callers | 12 lines |
| ARCH-DEAD-7 | `SmartReport::trim_enabled` / `trim_known` fields | cr_health.h:55-56 | never read or written anywhere (CLI keeps its own local bool from `query_trim_state`) | 2 fields |
| ARCH-DEAD-8 | `RetryingDevice::failed_bytes()` accessor (counter `failed_bytes_` maintained but never consumed) | cr_retry.h:53, cr_retry.cpp:40-43, 84 | accessor 0 callers | 4 lines |
| ARCH-DEAD-9 | `CachedReader::inner()` | cr_cached.h:30 | 0 callers | 1 line |
| ARCH-DEAD-10 | `ResultTree::index_node` (public wrapper) | cr_node.h:197, cr_node.cpp:151-154 | 0 external callers (add_file uses `index_node_locked` directly) | 4 lines |
| ARCH-DEAD-11 | Unreachable branch `if (!blocks_[0].valid)` | cr_cached.cpp:83-87 | miss path either returns early (rc!=OK) or pushes valid=true; hit path only matches valid blocks — the check can never fire | 5 lines |
| ARCH-DEAD-12 | `PoolCancelToken pool_cancel(&pool)` constructed, never passed | cr_scan.cpp:712 | workers get the outer `cancel` (cr_scan.cpp:714-716); F-B-10 re-confirmed open at HEAD | 1 line + class |
| ARCH-DEAD-13 | `round_up` utility | cr_types.h:53-55 | 0 uses (only `round_down` is used, cr_cached.cpp:39) | 3 lines |
| ARCH-DEAD-14 | `FsFamily::CarvedOnly` enum value | cr_scan.h:37 | never produced by `probe_family` (cr_scan.cpp:287-336) nor matched by the dispatch switch; only a hand-crafted session could inject it (then silently yields no structured results via default case) | 1 enum |
| ARCH-DEAD-15 | `Out.deleted` collected in the parallel candidate path, never merged | cr_scan.cpp:703-722 | reduction loop adds only `o.files` (F-B-11 open at HEAD — same scan, different `threads` ⇒ different `deleted_found`) | — |
| ARCH-DEAD-16 | `temp_data.txt` stray tracked file | repo root | stale rawfixer content copy (F-D-8, still present) | 25 KB |

### 8b. Test-only API surface (production-unused, keep or prune consciously)

| ID | Item | Location | Referenced only by |
|---|---|---|---|
| ARCH-TEST-1 | `RetryingDevice::error_map()` const-ref accessor | cr_retry.h:43 | test_retry.cpp:60-110 (production uses `error_map_copy()`) |
| ARCH-TEST-2 | `FileNode::prev_sibling()` | cr_node.h:108 | test_node_tree.cpp:33 |
| ARCH-TEST-3 | `ReadErrorMap::total_bytes()` | cr_device.h:60 | test_retry.cpp:97,109 (production prints only `ranges().size()`) |

### 8c. Duplicated logic

| ID | Duplication | Locations | Note |
|---|---|---|---|
| ARCH-DUP-1 | OVERLAPPED ReadFile loop ×3 (DirectDevice / RdaTargetDevice / RwDevice) | cr_transport.cpp:109-128, 172-198, 641-669 | near-identical; only the F11-unscale tail differs |
| ARCH-DUP-2 | RDA grammar open duplicated: free functions vs RdaClient methods | cr_transport.cpp:515-562 vs 375-385 | one of the two pairs is dead (ARCH-DEAD-1/3) |
| ARCH-DUP-3 | `u32_from_bytes_le` re-implemented locally | cr_transport.cpp:32-34 vs cr_types.h:29-32 | anonymous-namespace shadow |
| ARCH-DUP-4 | engine dispatch duplicated: `run_engine_pass` switch vs `scan_candidate` if/else chain (8 engines each) | cr_scan.cpp:532-617 vs 752-831 | og has a single factory dispatch (FUN_140577d40); ours pays the dispatch twice with drift risk (flags already differ: candidates never set deep flags) |
| ARCH-DUP-5 | `open_disk_view` name collision: local CLI vs dead win-layer function | rrecov.cpp:132-159 vs cr_enumerate.cpp:156 | one is dead (ARCH-DEAD-5) |
| ARCH-DUP-6 | `ReadErrorMap::add` first merge loop redundant with the sort+normalize pass | cr_devmap.cpp:19-29 vs 35-47 | push_back-then-normalize alone is equivalent |
| ARCH-DUP-7 | `fs_kind` assignment style: raw literals vs `FsKindCode` enum | cr_mftwalk.cpp:534 (`=3`), cr_exfat_quick.cpp:353 (`=7`), cr_ext4_engine.cpp:517 (`=9`), cr_hfsplus_engine.cpp:504 (`=8`), cr_zfs_engine.cpp:1333 (`=13`) vs cr_btrfs_engine.cpp:579 + cr_carve.cpp:137 (`FsKindCode::…`) | one codebase, two conventions |

**Dead/duplicate totals: 16 dead, 3 test-only, 7 duplicated ≈ 26 register entries; ~520+ lines of win-layer code are production-dead (RdaClient block + orphan exports).**

---

## Dimension 9 — Win7/CI posture

| Check | Status | Evidence |
|---|---|---|
| `_WIN32_WINNT`/`WINVER`/`NTDDI_VERSION` pin | **ABSENT** | Grep over all CMake/presests/sources → 0 matches |
| Import-allowlist gate in CI | **ABSENT** | ci.yml:77-98 "Diagnostics — deployed DLLs + PE imports" prints dumpbin output only, `if: always()`, no assertions |
| Claimed matrix | Win7 SP1 / Win10 / Win11 / WinPE | docs/testing.md:22 |
| Contrast (rda-driver) | machine-enforced (NTDDI 0x06010000, `_WIN32_WINNT=0x0601`, verify_pe.py allowlist) | per PARITY-ARCH F-D-4 (driver repo docs) |
| Verdict | **F-D-4 STILL OPEN at HEAD** — the user-mode side claims a Win7 envelope it never verifies | |

---

## Findings

### ARCH-1 — P1 — dead code / stale architecture claim — the entire `RdaClient` surface is dead and PARITY-ARCH's driver-boundary claim is false
- **Ours:** cr_transport.cpp:226-509 (impl), cr_rda.h:100-290 (surface) — zero production/test/fuzz references. The used RDA path is the free-function legacy grammar (`open_rda_partition`, rrecov.cpp:138; cr_enumerate.cpp:160; cr_scan path via open_disk → open_rda_partition, cr_transport.cpp:616-626).
- **Reference context:** og opens via `\\.\EPRDTDRV\%d\%d` grammar (Device.md:39-44) — the grammar path itself is faithful; but PARITY-ARCH.md:152 claims "core_recovery consumes it exclusively through cr::win::rda::RdaClient … structured OPEN_TARGET preferred over legacy PID-token grammar" — **REFUTED at HEAD**: OPEN_TARGET (`kIoctlOpenTarget`, cr_rda.h:47) is called only inside dead code.
- **Impact:** ~520 dead lines; a stale architecture claim in the prior audit; the capability-gated structured open (which self-rejects old drivers) is designed but unwired.
- **Proposed action:** either wire `open_disk()`/`open_disk_view()` through `RdaClient::open()`+`open_target()` (making the doc true and gaining GET_INFO capability gating), or delete the class and correct PARITY-ARCH.md:152 + cr_rda.h header narrative. Register the outcome.

### ARCH-2 — P1 — orchestration wiring — pause/cancel/checkpoint/resume are core-complete but product-invisible; architecture.md asserts a false CLI behavior
- **Ours:** `ScanController::run(slice, nullptr /*cancel*/, …)` (rrecov.cpp:486-491); no `PausableToken` construction anywhere in src/app (Grep); no `--resume`/checkpoint sink ever set (cfg.checkpoint untouched in cmd_scan, rrecov.cpp:360-424); save-on-cancel not implemented — `if (rc != OK) return 1;` precedes the save block (rrecov.cpp:503-506 vs 521-535).
- **Contradicting claim:** docs/architecture.md:80-81: "Save-on-cancel: a cancelled run persists a resumable session (the CLI saves on cancel)" — **false at HEAD**. docs/UI-READINESS-PLAN.md:36 correctly states the opposite ("passes CancelToken* = nullptr and never saves on cancel").
- **Reference:** og Pause/Stop controls (dynamic-ground-truth §7); CStopScanThread (DRWUI threads). Our core is *ahead* of og (pause + durable resume + checkpoints, test_pause_resume.cpp:491-599) but the only host (rrecov → wizard subprocess) cannot reach any of it.
- **Proposed action:** wire a Ctrl-C handler + PausableToken + checkpoint sink + `resume` subcommand into rrecov (tracked as UI-READINESS W1-2/W1-6), and fix architecture.md:80-81 to describe current truth.

### ARCH-3 — P2 — orchestration — single-pass quick+deep is an UNREGISTERED deliberate delta
- **Ours:** cr_scan.cpp:2-8, 450-457 ("the old code walked the MFT twice… single walk").
- **Reference:** og runs Quick THEN a separate Complete-searcher generation (FUN_140580720 @ DRWUI_decompiled.c:916443+, searcher-list composition verified; engine-matrix §1 Quick/Complete factory pairs; dynamic-ground-truth: quick results carry into Advanced).
- **Doc status:** PARITY-ORCH §5.1 lists it as a *candidate*; the authoritative registry (docs/PARITY-DELTAS.md, "A delta that is not in this registry is a bug, not a delta") has **no entry**.
- **Proposed action:** register as D-3x (what og does / what we do / why: output-equivalent, half the walking, both passes' semantics preserved via flags+extras) or re-introduce the two-generation shape if the wizard needs quick-then-deep UX staging.

### ARCH-4 — P2 — threading claim — "engines single-threaded, host fans out" misquote persists in 3 places
- **Ours:** roadmap.md:19-20, cr_parallel.h:4-9 ("the reference engines themselves are single-threaded; ALL parallelism is host-side"), cr_scan.cpp:3-4.
- **Reference (corpus):** FAT `CFATQuickSearch::CFATQuickSearchWorker` vftable — FATSearchDLL_decompiled.c:26931, 27130; fat-engine--deep-dive ("threaded QuickSearchWorker", searchdll-family--architecture.md:115); QThread scan workers (mom-modules--ui-bridge-and-dependency-matrix.md:49).
- **Status:** D-18 (PARITY-DELTAS.md:396-419) already ruled the wording a "citation-hygiene note" feeding U-12 (:830-833) with the action "reword" — **not applied** to any of the three sites at HEAD.
- **Proposed action:** reword all three to "extends the corpus's host-fans-out model with a first-class pool; the corpus also shows engine-internal workers (FAT QuickSearchWorker)".

### ARCH-5 — P2 — first-class scoping — stale "recognition-only"/"IN PROGRESS"/"report-only" texts vs shipped first-class LDM/ReFS
- **Ours (code, first-class confirmed):** full LDM decode + simple/spanned/striped/mirrored/RAID-5 reconstruction (cr_ldm.cpp:1-36 header contract, 1321-1372 reconstruction; 1469 lines total); wired unconditionally (cr_scan.cpp:497-530) with cross-disk resolver (rrecov.cpp:447-479). ReFS FULL engine wired in both pass and candidate paths (cr_scan.cpp:604-612, 821-828).
- **Stale texts:** docs/ground-truth-contracts.md:85 "(recognition only through v0.2)"; docs/roadmap.md:42-46 "LDM dynamic-disk layer … IN PROGRESS (session 5; recognition shipped in v0.2, full decode + reconstruction landing this session)" — the landing happened; the row was never flipped to DONE. include/cr/refs/cr_refs_detect.h:35 + cr_refs_detect.cpp:1 say "Report-only" — accurate for the *detection helper* component, but per the session mandate ("any doc/code claiming 'ReFS report-only'… is stale and must be flagged") the phrasing invites misreading next to the full engine.
- **Proposed action:** flip roadmap LDM row to DONE; reword ground-truth-contracts §6 parenthetical as history ("v0.2 shipped recognition; session 5+ ships full decode"); scope the cr_refs_detect "report-only" comment to "detection probe only — full engine in cr_refs_engine".

### ARCH-6 — P2 — prior-findings regression audit — PARITY-ORCH statuses at HEAD 08b44ac
| Prior finding | Status at HEAD | Evidence |
|---|---|---|
| F-B-4 (fix --undo-apply disk-0 write) | **FIXED** | rrecov.cpp:1082-1089 unconditional `if (!have_disk)` guard, comment cites F-B-4 |
| F-B-8 (error-map merge on abort path) | **FIXED** | stamp() merges on every exit (cr_scan.cpp:388-415) |
| F-B-1/F-B-2/F-B-3 (LDM contract drift) | code present; wizard-side CORE-CONTRACT re-pin not verifiable from this repo | cr_scan.cpp:497-530; rrecov.cpp:447-479 |
| F-B-10 (dead PoolCancelToken) | **OPEN** | cr_scan.cpp:712 |
| F-B-11 (parallel path drops deleted_found) | **OPEN** | cr_scan.cpp:703-722 |
| F-B-15 (failed scan prints "100.0% done") | **OPEN** | rrecov.cpp:492 before rc check at 503 |
| F-B-16 (cr_device.h "engines AND transports" cancel overclaim) | **OPEN** | cr_device.h:101-102; no src/win transport polls a token |
| F-B-5/F-B-6/F-B-21 (progress richness / stage ids / live carve) | OPEN (product-level; partially mitigated by telemetry() pull) | cr_scan.h:170-171 callback shape unchanged |

### ARCH-7 — P3 — type defect — `quick_done_` declared `u32`, initialized/used as bool
- include/cr/orchestrator/cr_scan.h:268 `u32 quick_done_ = false;` — semantics are boolean at all four use sites (cr_scan.cpp:123, 459, 631, 878). Misleading; should be `bool`.

### ARCH-8 — P3 — interface shape — carve naming seam exists but the host cannot use it
- og: DRWUI registers the naming/filter callback into the RAW engine at wiring time (04 §3.3; 05 §6, vtable+0x48). Ours: `carve::NameCallback` (cr_carve.h:42) is a real seam, but the namer is hardcoded in the orchestrator (cr_scan.cpp:846-854). The wizard cannot localize/customize carved-file naming (og's host could).
- Action: thread an optional namer through ScanConfig (tiny, keeps og's host-owns-naming shape).

### ARCH-9 — P3 — transport shape — og's mounted-volume file-API read path has no counterpart
- og volume IO selects: normal file API when mounted & healthy, else EPRDTDRV raw, else FVE decrypt (05 §8). Ours always reads raw handles (RDA-first or direct). Justified by the uniform bounded-slice contract and lost-partition support, but the divergence is not registered anywhere.
- Action: one paragraph in docs/PARITY-DELTAS.md (or architecture.md §platform transports).

### ARCH-10 — P3 — build/version drift — CMake `VERSION 0.1.0` vs roadmap "v0.2 (shipped)"
- CMakeLists.txt:4 `VERSION 0.1.0`; `CR_ENGINE_VERSION="${PROJECT_VERSION}"` (src/core/CMakeLists.txt:56-57) bakes "0.1.0" into every recovery report (G-09), while docs/roadmap.md:17 declares v0.2 shipped (and cr_scan.h:38-39 documents post-0.1.0 enum appenders). Reports understate the engine.

### ARCH-11 — P3 — hygiene — `temp_data.txt` still tracked (F-D-8), and F-D-1 (fuzz/clang++ symlink) is now RESOLVED
- temp_data.txt present at root (25 KB stale rawfixer copy). Conversely, the git-tracked broken `fuzz/clang++` symlink reported by PARITY-ARCH F-D-1 no longer exists (verified: not on disk, not in `git ls-files fuzz/`) — that item can be closed.

### ARCH-12 — P3 — consistency — mixed fs_kind literal/enum style and raw hex error literals
- See ARCH-DUP-7 and Dimension 6 style nit (cr_scan.cpp:947,954). One-line cleanups.

### ARCH-13 — P3 — enumeration — no hot-plug/device-change notification
- og: DeviceManager.mo hot-plug notify + MainModule `CDevNotifyManager`/WM_DEVICECHANGE (mo-module-map.md:21; mom-module-system §2). Ours: enumeration is poll-only (`list` command). Unregistered gap; likely fine for a CLI wizard (re-enumerate on demand) but should be a conscious delta.

---

## Prior-session claims — verification verdicts

| Claim | Verdict | Evidence |
|---|---|---|
| "PARITY-ARCH reported 0 layering violations" | **CONFIRMED (both the report and the fact)** | PARITY-ARCH.md:100 states it; independently re-verified at HEAD 08b44ac — all include-graph rules PASS (Dimension 2 table; fresh greps, 0 matches on every violation pattern) |
| "in-process engine registry is delta D-21" | **CONFIRMED — with a wording defect** | docs/PARITY-DELTAS.md:464-479 (D-21, ACCEPTED). Defect: D-21 says "crCreate*-style factories" — `Grep crCreate` over src/include/tests/fuzz → **0 matches**; the actual mechanism is the `FsFamily` switch (cr_scan.cpp:532-617). Same defect family as F-D-10 (README wording), now propagated into the authoritative registry |
| "engines single-threaded per unit, host fans out (D-18/U-12 says corpus shows engine-side threads too)" | **The parenthetical is CONFIRMED; the unqualified claim is REFUTED as a corpus description** | Corpus: FATSearchDLL_decompiled.c:26931/27130 (`CFATQuickSearch::CFATQuickSearchWorker` vftable, engine-side worker); searchdll-family--architecture.md:115 ("threaded QuickSearchWorker"); mom-modules--ui-bridge-and-dependency-matrix.md:49 (scan workers in QThreads, 42-thread census). D-18 (PARITY-DELTAS.md:404-419) already recorded the correction + "reword" action; U-12 (:830-833) tracks it. The stale wording still lives at roadmap.md:19-20, cr_parallel.h:4-9, cr_scan.cpp:3-4 (ARCH-4) |

---

## UNVERIFIED register (never guessed)

1. **Per-searcher invocation policy inside og's complete-searcher list** — FUN_140580720's list composition is verified (DRWUI_decompiled.c:916443+), but whether every complete searcher runs against every target or only the FS-matching one is not decoded (would need the searcher-list consumer in DRWUI).
2. **og RAWSearchDLL internal threading** — the corpus does not document whether the carve engine fans out internally; our parallel carve windows have no confirmed og counterpart.
3. **og-side retry/bad-sector isolation** — no counterpart to RetryingDevice located beyond `CStorageDeviceWithCache` (carried from PARITY-ORCH UNVERIFIED #3; possibly in eprdtdrv internals).
4. **Whether og `.rsf` sessions ever resume partial scans** — "resume/recover later" (04 §8) is display/recover-ambiguous (PARITY-ORCH #2).
5. **Actual Win7 API compatibility of `src/win`/rrecov** — requires a Windows build + dumpbin import gate or Win7 VM (F-D-4/ARCH dimension 9).
6. **Destination of og's engine `cout` diagnostics** — the "Return Error:0x… File/Line/Function" strings are verified in decompilation (05 §4.5) but whether the GUI process surfaces or swallows them is not decoded.
7. **`CCallBackOperate` OC_PARAM payload layout** (opcodes 0,1,3-6) — method names verified, bodies not fully decoded (PARITY-ORCH #1).
8. **rda-wizard-side contract re-pin for the LDM trio (F-B-1/2/3)** — CORE-CONTRACT.md lives in the other repo; not verifiable here.

---

## Summary counts

- Dimensions audited: 9 (all with tables above).
- Findings: 13 (ARCH-1..ARCH-13). Severity: **P1 ×2** (ARCH-1 dead RdaClient + false prior claim; ARCH-2 orchestration wiring / false architecture.md claim), **P2 ×4** (ARCH-3, ARCH-4, ARCH-5, ARCH-6 rollup), **P3 ×7**.
- Dead/duplicate register: **26 entries** (16 dead, 3 test-only, 7 duplicated); ~520+ lines of win-layer code production-dead.
- Prior claims: 3/3 re-verified — 2 CONFIRMED (one with wording defect), 1 REFUTED-as-corpus-description (correction already registered in D-18/U-12 but not applied to source comments).
- Layering: 0 violations at HEAD (re-confirmed).
- ReFS/LDM first-class: confirmed in code; 3 stale doc texts flagged (ARCH-5).
