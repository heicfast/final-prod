# S11/L1 — RAWSearchDLL INFRASTRUCTURE function-level walk (RAWINFRA lane)

- **Task ID:** S11/L1 · **Date:** 2026-10-28/29 · **Auditor:** L1 (read-only; no repo writes; this sheet is the only write)
- **Repos/commits:** `core_recovery @ 2498b9fbc7f274d7336166794a48dc59e22cfcd4` · `easeus-drw-research @ be0cfe95ecf22ac67e0f3a823dd003163b196d38`
- **Sources:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` (80,614 lines; cites = file line numbers), `RAWSearchDLL_functions.txt` (1,921 rows), `RAWSearchDLL_imports.txt` (186 rows), `og-codebase/RAWSearchDLL.dll` (PE walk: vtable + RTTI dump, this session), `diff-to-fix/` (the board), core `src/core/carve/`, `src/core/orchestrator/`, `include/cr/`.
- **Method:** every named export read in full; the controller vtable recovered **from the binary** (RVA 0x66e38, RTTI-verified `.?AVCRAWSearchController@@` via COL@RVA 0x757d8) and all 36 slots resolved to functions, then every non-stub slot's body read; every infra-cluster function read in full; each behavior cross-checked against the board before any new-divergence claim.

---

## §1 Function census

**Total functions (functions.txt): 1,921**

| Class | Count | Notes |
|---|---|---|
| Named engine exports | **6** | EUCreateRAWSearcher (0x18005e220), EUCreateRAWSearcherForCellTest (0x18005e260), EUGetAnalyserList (0x18005e320), EUGetAnalyserMap (0x18005e340), EUGetAnalyserMultimap (0x18005e360), GetRawUserInfoMap (0x18005e380) |
| CRT/SEH/STL noise (named) | **~715** | 539 `Unwind@`, 89 `Catch@/Catch_All@`, 15 `__scrt_*`, 5 `_Init_thread_*`, plus memcpy/memmove/memset/memchr/memcmp/free/deallocate×2/snprintf×2/sscanf/sprintf_s(FID)/pow/powf/strcmp thunks, `_Reset_back`, `_guard_check_icall/_guard_dispatch_icall`, `__CxxFrameHandler3/__RTDynamicCast/_purecall/terminate/_CxxThrowException`, `eh_vector_*`×3, `__ArrayUnwind`, `dllmain_crt_dispatch/dllmain_dispatch/entry`, `__report_*`, `__security_*`, `__GSHandlerCheckCommon`, `_alloca_probe`, `capture_current/previous_context`, `__isa_available_init`, `__scrt_fastfail`, `_onexit/atexit`, streambuf/locale facet stubs (`_Lock/_Unlock/showmanyc/uflow/xsgetn/xsputn/setbuf/sync/imbue`), `CAtlBaseModule/~CAtlBaseModule`, `_RTC_Initialize`×2 (FID), `IsProcessorFeaturePresent`, `InitializeCriticalSectionEx` (local copy) |
| `FUN_`-prefixed (domain code) | **1,195** | the analyser metropolis |
| — of which INFRASTRUCTURE (read this session, in full) | **64** | enumerated below |
| — of which CARVE-GRAMMAR (prior sessions' trail) | **≈1,131** | analyser ctors/validators, FileInfo classes, registry-builder per-format blocks (FUN_180002a50 :1436-2515, FUN_180004320 :2517-2914, FUN_180004fb0 :2919-4208), raw_signs.json loader (FUN_18001ea40 :23765, ifstream "raw_signs.json" :23836), tokenizer (FUN_180021460/FUN_180021380), tinyxml2/CJsonObject, CRawFileCellTestStorageDevice (112 vftable refs) |

**Imports telling the infra story (RAWSearchDLL_imports.txt):** NO CreateThread, NO CreateFileW/ReadFile — the engine owns **no threads and no direct disk I/O**; all reads flow through the host-injected `IStorageDevice` vtable. Kernel32 sync = `CreateEventW/WaitForSingleObjectEx/ResetEvent/SetEvent/CloseHandle` — used **only** by the CRT's `_Init_thread_*` magic-static machinery (:72694-72874); the engine's own lock is one `InitializeCriticalSection` at controller+0x128 (:8889). `LoadLibraryA/GetProcAddress` = the **EUVideoRepair.dll** bind (:9210-9220). `GetPrivateProfileSectionA` = RawConfig.ini `[Filter]` (:13137). `GlobalMemoryStatusEx` + `PUBLOG.DLL::LogInfo` = the OOM diagnostic path (:970-978). `GetModuleFileNameA` ×2 (EUVideoRepair path :9212; RawConfig dir :13129). `WS2_32.DLL::Ordinal_14` (ntohl) — endianness util at :53900 (TIFF-BE family — grammar).

### The CRAWSearchController vtable (RVA 0x66e38, dumped from the binary this session — NEW trail datum; previously only slots +0x48/+0x68 were on record)

| Slot | Function | Infra role (verified by body read) |
|---|---|---|
| +0x00 | FUN_18000cf20 (:9698) | set this+0xe8 (analyser-filter interface; feeds FUN_180006b30) |
| +0x08,+0x40,+0x78,+0x90,+0xa8,+0xc0,+0xc8,+0xf0,+0xf8,+0x108 | 0x180001310 stub | pure/dummy slots |
| +0x10, +0x70 | FUN_18000b760 (:8562) | `return 0` stub |
| +0x18 | FUN_18000b870 (:8639) | set this+0x08 |
| +0x20 | FUN_18000b880 (:8649) | set dword this+0x10 |
| +0x28 | FUN_18000b890 (:8659) | set this+0x18 + this+0x20 |
| +0x30 | FUN_18000b8a0 (:8670) | set this+0x28 |
| +0x38 | FUN_18000b8b0 (:8680) | call this+0x28 → vt+0x28 (notify) |
| +0x48 | FUN_18000b8d0 (:8695) | set naming std::function @this+0x30 (OQ-S4-2) |
| +0x50 | FUN_18000bcb0 (:8899) | deleting dtor |
| +0x58 | FUN_18000be90 (:8984) | SetStorageDevice → this+0xc0 |
| +0x60 | FUN_18000bea0 (:8994) | SetSectorsPerChunk → dword this+0xc8 |
| +0x68 | FUN_18000beb0 (:9006) | **Init/Start** (see §2) |
| +0x80 | FUN_18000ce30 (:9658) | **Stop** (see §2) |
| +0x88 | FUN_18000ba20 (:8768) | set byte this+0x70 |
| +0x98 | FUN_18000c820 (:9385) | **ScanChunk**(start_sector, count, bitmap) (see §2) |
| +0xa0 | FUN_18000ce20 (:9648) | SetResultQueue → this+0xd0 |
| +0xb0,+0xb8 | FUN_18000cf30 (:9708) | `return 1` |
| +0xd0 | FUN_18000b790 (:8585) | `return 2` |
| +0xd8 | FUN_18000cf40 (:9717) | **SetObserver** → global DAT_1801183c0 (the cancel/continue object) |
| +0xe0 | FUN_18000bae0 (:8819) | `return 0` |
| +0xe8 | FUN_18000ce10 (:9638) | set this+0xb8 |
| +0x100 | FUN_18000ba30 (:8778) | set 2nd std::function @this+0x78 (no DRWUI registration found — OQ-S4-2) |
| +0x110 | FUN_18000c430 (:9245) | **SetSectorSize + buffer/result-queue init** (see §2) |
| +0x118 | FUN_18000c650 (:9337) | **Search(range)** — the in-DLL chunk-loop driver (see §2) |

(Adjacent .rdata object: `ListIterator<CRawFileInfo*>` vtable at +0x128 — COL@0x757b0 — de70/dac0/dab0/da90/da80 = dtor/Reset/Next/IsEnd/Current; the result-queue's iterator wrapper.)

### The 64 infra functions (all read in full)

**Factory/exports (6):** EUCreateRAWSearcher (:72122-72136 `thunk_FUN_1800020c0(0x160)` → ctor → out) · EUCreateRAWSearcherForCellTest (:72141-72165 static once-init into DAT_180096240 + atexit + FUN_18000c650 bind) · EUGetAnalyserList/Map/Multimap (:72170-72208 = FUN_180006a80 + member +0x30/+0x40/+0x50 master views) · GetRawUserInfoMap (:72213-72221 `return FUN_180001350()+0x90`).

**Controller lifecycle (12):** FUN_18000bba0 ctor (:8859-8894; 0x160 bytes; sector default 0x200 at +0xf0 dword; list init at +0xf8; `InitializeCriticalSection` at +0x128) · FUN_18000bcb0 deleting dtor / FUN_18000bdb0 dtor (:8899-8979: free buffer +0xe0, DeleteCS, free +0x118, unlink+free every node of the pending list +0xf8, base dtor FUN_18000b9a0) · FUN_18000b9a0 base dtor (:8745-8763, releases shared_ptrs at +0xb0/+0x68) · FUN_18000b7a0 format-name getter (:8594-8621: `&DAT_18008f070 + *(uint*)(this+0x18)*9` — **9-byte-stride id→name table**) · FUN_18000b760/b790/bae0 stubs · FUN_18000ba20 (+0x70 byte) · FUN_18000bac0 DeleteCS wrapper.

**Init/Start (1, 1401 bytes):** FUN_18000beb0 :9006-9240 — (a) resets the user-info singleton: `*(puVar3+0x60)=1; *(puVar3+0x70)=200000000; *(puVar3+0x68)=0; FUN_180001500(puVar3+0x90)` (:9047-9051); (b) if the naming hook registered (this+0x68): invokes it with `"filter_generalraw"` (FUN_18000e160 :9090), builds the filter tree (FUN_180057320/57b80), collects type-6 child names into a vector, assigns to **CNeedRawMgr+0x18** (:9093-9140); (c) once-init FUN_180006730 (registry + CNeedRawMgr::LoadConfig) :9169-9179; (d) if this+0xe8: fetch the filter list (vt+0x90) and **FUN_180006b30 rebuilds the working registry minus disabled analysers** (:9180-9183); (e) sector size from device (CellTest +0xc or vt[5]; floor 0x200) → this+0xf0 (:9184-9204); (f) buffer = sectors_per_chunk(this+0xc8) × sector × 2, alloc `FUN_1800020c0(size+0x1000)` → this+0xe0 (:9205-9209); (g) **LoadLibraryA(".\EUVideoRepair.dll") + GetProcAddress("GetIMovMp4Repair")** → this+0x158, then repair->vt+0x30(repair, device, 0, sector×size) (:9210-9237).

**Setup/Search driver (2):** FUN_18000c430 :9245-9329 — clears result list (+0xf8 via FUN_180008290), sets sector size this+0xf0, **refuses (returns 0) sector sizes not a multiple of 512** (:9265-9268 `((param_2 + ((param_2>>63)&0x1ff)) & 0x1ff) != carry`), sectors-per-chunk = 0x400000/sector when unset (:9269-9288), buffer 2× + 0x1000 slack, **allocates the 0x10048-byte result queue** = 4096 × 0x10 per-format {list-head,count} buckets (`_eh_vector_constructor_iterator_(ptr, 0x10, 0x1000, FUN_18000db00, FUN_180006890)` :9304) + all-results list @+0x10000 + `IIterator<ListIterator<CRawFileInfo*>>` wrapper @+0x10010 + two offset-keyed std::maps @+0x10028/@+0x10038 (:9305-9324) → this+0xd0. FUN_18000c650 :9337-9380 — validates range, `vt+0x110` prepare, clears result list when flag==0, `vt+0x58` start, then **loops `vt+0x98`(chunk) over `range_sectors / sectors_per_chunk`** (:9361-9376).

**Chunk scan (1, 1516 bytes):** FUN_18000c820 :9385-9633 — device vt[1] Read fills `buffer + half` (:9434-9438); **continue-check**: `plVar10 = FUN_1800424b0(); if (*plVar10 == 0 || (*plVar10)->vt+0x10() != 0) { scan }` (:9439-9441 — observer==NULL or vt+0x10()!=0 proceeds; set via vtable+0xd8/FUN_18000cf40, cleared by Stop); per-sector loop with (i) **volume-bitmap gate** FUN_180058df0 + cluster skip `local_1e0 += spc-1` (:9446-9460); (ii) device-size bound + CellTest size fallback (:9463-9481); (iii) **leading/trailing zero metrics** (first-dword-nonzero flag uVar20=4, backward 7-dword trailing-zero count iVar17, :9498-9510); (iv) dispatch FUN_18000cf60 (:9533) → on match: CreateFileInfo vt+0x38, stamp {position, size, sector, analyser, id}, insert into queue list @+0x10000 + map @+0x10028 (:9551-9563); (v) **pending model**: size unknown → vt+0x10()==pending → **if pending count ≥ 16 (0x10 < count+1) force-finalize+enqueue+notify+evict the OLDEST** (:9573-9591), else append (:9593-9605); (vi) tail: `memmove(buffer, buffer+half, half)` ping-pong roll (:9620-9622); (vii) **per-chunk feed to EUVideoRepair**: `(this+0x158)->vt+0x40(repair, sector_size*start, buffer_half, half)` (:9623-9628).

**Per-sector dispatch (1) + u16 layer (1):** FUN_18000cf60 :9730-9925 (once-init reads config/config.ini `[only_common]` via ifstream-based FUN_180007890 :9766-9841; zero-4CC guard; u32 map lower_bound → analyser vt+0x40 special/plain vt+0x10 → FUN_18000d920 enqueue; then FUN_18000d3e0 u16 multimap equal_range; then full working-list walk when pos ≥ 0) · FUN_18000d3e0 :9933-10063 (u16 equal_range, insertion-order chains, special-slot 6-arg AnalyseEx incl. sector_size/pos/scanctx).

**Pending continuation walker (1):** FUN_18000d670 :10068-10152 — per pending FileInfo: set device, analyser ctx; drop nodes with analyser==0; `vt+0x20 ContinueScan(header, sector_size, pos, scanctx)` → on 1: enqueue FUN_18000d920, **host sink (this+0x108)->vt[0](sink, fileinfo)** (:10124-10125), **video-repair submit**: `vt+0x18 GetVideoInfo(&4 dwords)` → on true `(this+0x158)->vt+0x48(repair, ...)` (:10130-10136); node removed.

**Enqueue (2):** FUN_18000d920 :10158-10178 / FUN_18000d980 :10183-10200 — map find-or-insert on queue+0x10038 keyed by offset (FUN_18000e980 node-alloc + FUN_18000e9d0 lower_bound emplace).

**Stop (1):** FUN_18000ce30 :9658-9693 — `this+0x150 = 1` always; if device idle (device+0x14==0 && vt+0x70()==false): walk pending list, `vt+0x10(node, 0, -1)` force-finalize each, notify sink this+0x108, clear list (FUN_18000db40), **free the read buffer**, null the global observer (`*FUN_1800424b0() = 0`).

**Setters/stubs (8):** FUN_18000be90/bea0/ce10/ce20/cf20/cf30/cf40/ba20/b770 (documented in the vtable table above).

**Registry cluster (9):** FUN_180006730 :4216-4249 (once-init of DAT_1801181a0: 6 containers + CNeedRawMgr::LoadConfig + builders) · FUN_1800068c0 atexit teardown (:4265-4311) · FUN_180006a80 getter (:4317-4336) · FUN_180006b00/b10/b20 view selectors (:4341-4381: map=+0x20/+0x50, multimap=+0x10/+0x40, list=+0x00/+0x30) · **FUN_180006b30 ApplyFilter** :4386-4434 (clears+rebuilds working views; prune loop keeps analyser ids where `id−0x30 ∈ {0,15,20}` (mask 0x1000801) **or** filter-callback accepts; deletes rejected analysers) · FUN_180006c80 map operator[] (:4450-4484).

**Config/stats singletons (7):** FUN_180001350 :65-100 (once-init user-info singleton @DAT_1800939a0: +0x60 sub-object {1, 0, 200000000, vector<uint> ids **1..169** @+0x78, stats-map @+0x90, byte @+0xa0, string @+0xb0, 0xff @+0xc0}; atexit FUN_180064040) · FUN_18000ed00 :11376-11531 (stats-map insert-or-increment: existing → `node[8] += 1` :11466; new → value 1 :11525-11526; key = wide ext string) · FUN_180010a40 CNeedRawMgr singleton (:13040-13059) · FUN_180010b10 CNeedRawMgr::LoadConfig (:13066-13220: RawConfig.ini `[Filter]` via GetPrivateProfileSectionA :13137, key=value split, value=="1" → enabled{byte=1,tag=4} else disabled{byte=0,tag=8}, into map<string,flag> at +8) · FUN_180007890 :5233+ (ifstream-based ini reader — the config/config.ini `[only_common]` test-mode gate) · FUN_180057320/57b80/579b0/57e80/57610 (XML/JSON doc helpers — the filter_generalraw fetch AND the raw_signs.json loader both route through them).

**Observer/global (3):** FUN_1800424b0 :50323-50336 (magic-static getter of DAT_1801183c0 — the host-installed continue/cancel object) · FUN_18000cf40 setter · cleared in Stop.

**Iterator/queue accessors (5):** FUN_18000da80 (Current) · da90 (IsEnd) · dab0 (Next) · dac0 (Reset) · dad0/de70/dec0/db30 (dtors/vtable stamps) · FUN_18000db00 bucket ctor (:10285-10295: {FUN_180007710 list-head, 0}) · FUN_18000db40 list-clear (:10310-10329).

**Allocator (1):** FUN_1800020c0 :958-983 — `malloc(size)`; **on NULL: `LogInfo("Failed to alloc 0x%08x bytes")`, `GlobalMemoryStatusEx`, 3 more LogInfo lines (total/available physical + pagefile MB), then `RaiseException(0xe0000001, 0xc0000025 /*EXCEPTION_NONCONTINUABLE*/, 0, NULL)`** — the DLL's global operator new (thunk used by EUCreateRAWSearcher and all engine allocations).

**STL helpers implementing the infra structures (~14):** FUN_180001500 (map/list clear) · FUN_180008290/81f0 (list clear) · FUN_180009f00/1800077d0/180007710/180007310/180007770 (list node ops) · FUN_18000acf0/ad50 (RB insert/rebalance) · FUN_18000afb0/18000b010/b2c0 (map insert) · FUN_18000e1b0/e3a0/e420/e620/df80/def0/dd90/dd30/dbc0/e0f0/e060/e0a0 (vector/shared_ptr machinery) · FUN_18000e980/e9d0/e4e0/e6c0/eb70 (the queue's map emplace/rebalance) — read, confirmed STL semantics, no engine policy inside.

---

## §2 Per-function disposition table (infra)

| Function (addr) | Behavior | Disposition | Evidence |
|---|---|---|---|
| EUCreateRAWSearcher | 0x160 controller factory | **MATCH** (ABI-shape; core has no DLL factory — N-033/D-100 ABI row) | :72122-72136; core: no factory (deliberate, D-100) |
| EUCreateRAWSearcherForCellTest | static-instance + test-device bind | **COVERED-BY** rawsearch-carving--deep-dive.md §1 (:98) | :72141-72165 |
| EUGetAnalyserList/Map/Multimap | master-registry views | **COVERED-BY** s3-rawsearch-inventory.md §1 + N-033 FIXED/D-100 | :72170-72208 |
| GetRawUserInfoMap | returns singleton+0x90 stats map | **COVERED-BY** CORRECTIONS #9, D-104, SCAN-ENGINE-DELTA F2; **core-side absence of the surface = NEW candidate C2** | :72213-72221 |
| FUN_18000bba0 ctor | 0x160 layout, CS at +0x128, sector default 0x200 | **COVERED-BY** s3 §5 (buffer/field map) + M-025 | :8859-8894 |
| FUN_18000bcb0/bdb0/b9a0 dtors | teardown incl. pending-list node free | **MATCH** (RAII equivalents; no finding) | :8899-8979 |
| FUN_18000beb0 Init | singleton reset + filter fetch + registry rebuild + EUVideoRepair bind + buffer sizing | **COVERED-BY** M-021 (FIXED, incl. per-chunk addendum per SUMMARY:5165), M-024 (FIXED), OQ-S4-2, SCAN-ENGINE-DELTA A4/session-2 log | :9006-9240 |
| FUN_18000c430 setup | 512-multiple sector gate; 4 MiB chunk; 8 MiB buffer + 4 KiB slack; 0x10048 result queue | **COVERED-BY** s3 §5.1, M-025 (FIXED), I-009; queue shape refined (see §4 notes) | :9245-9329 |
| FUN_18000c650 Search | in-DLL chunk-loop driver (vtable+0x118) | **COVERED-BY** s3 §5.7 (wording "external virtual" imprecise — see §4) | :9337-9380 |
| FUN_18000c820 ScanChunk | read→gate→dispatch→emit→pending→roll→repair-feed | **COVERED-BY** M-005 (FIXED), N-031 (FIXED), s3 §5, M-021 addendum; **pending 16-cap eviction = NEW detail C3** | :9385-9633 |
| FUN_18000cf60 dispatch | u32→u16→list order; only_common; zero guard | **COVERED-BY** s3 §2 (OQ-T8), N-031 addendum | :9730-9925 |
| FUN_18000d3e0 u16 layer | equal-range chains | **COVERED-BY** s3 §2.2 (grammar-adjacent) | :9933-10063 |
| FUN_18000d670 pending walker | ContinueScan + enqueue + sink notify + repair submit | **COVERED-BY** s3 §5.5, M-021 (FIXED), D-101 | :10068-10152 |
| FUN_18000d920/d980 enqueue | offset-keyed map insert | **COVERED-BY** s3 §5.5 | :10158-10200 |
| FUN_18000ce30 Stop | +0x150 flag; idle→flush pending+free buffer+clear observer | **partially covered** (N-017 cancel-watcher FIXED; D-101 pending model) — **flush semantics = C4 (INFO)**; +0x150 write-only (see §4) | :9658-9693 |
| FUN_18000cf40 / FUN_1800424b0 observer | host cancel/continue object, checked per chunk | **COVERED-BY** N-017/D-78 (core: CancelToken per window — finer granularity = superset) | :9717-9725, :50323-50336, :9439-9441 |
| Registry cluster (6730/6a80/6b00-b30/6c80/68c0) | master/working views, filter rebuild, RB insert | **COVERED-BY** s3 §1 + M-024 (FIXED) | :4216-4484 |
| FUN_180001350 singleton | user-info object; ids 1..169; stats map | **COVERED-BY** SCAN-ENGINE-DELTA A4 + session-2 log (+0x70 cap; +0x60); full layout refined (§4) | :65-100 |
| FUN_18000ed00 stats | insert-or-increment per-extension counter | **COVERED-BY** CORRECTIONS #9 / D-104; **core-side absence = C2** | :11376-11531 |
| FUN_180010a40/180010b10 CNeedRawMgr | RawConfig [Filter] load | **COVERED-BY** M-024 (FIXED) | :13040-13220 |
| FUN_1800020c0 operator new | malloc; OOM → 5× LogInfo + GlobalMemoryStatusEx + RaiseException(0xE0000001) | **NOT COVERED anywhere** (CORPUS-ERRATA #102 is the NTFS-side note) — **NEW candidate C1** | :958-983 |
| FUN_180007890 ini reader | ifstream ini parse (only_common test mode) | **COVERED-BY** s3 §2.6 | :5233+, call :9834 |
| FUN_18000b7a0 name getter | 9-byte-stride id→name table @DAT_18008f070 | **COVERED-BY** S-049 (naming contract; mechanism now pinned — §4 note) | :8594-8621 |
| progress reporting | engine emits NO progress; host-side only (observer = continue-check only) | **COVERED-BY** D-78 (product-level progress model registration); core per-window callback = superset — **C5 (INFO note)** | imports; :9439-9441 only observer call |
| STL helpers (list above) | container machinery | **MATCH/ignore** (no engine policy) | various |

---

## §3 New divergence candidates (the deliverable)

### C1 (MINOR) — Carve-engine OOM policy + diagnostics: og logs a 5-line PUBLOG memory dump then dies by `RaiseException(0xE0000001)`; core fails silently (`new(nothrow)` → `ERR_NO_MEMORY`, or an uncaught `std::bad_alloc` from the window buffer) with zero logging

- **og:** `FUN_1800020c0` (the DLL's global operator new; every engine allocation including the controller itself via `thunk_FUN_1800020c0(0x160)` at :72128), RAWSearchDLL_decompiled.c :966-980:
  ```c
  pvVar2 = malloc(param_1);
  if (pvVar2 == (void *)0x0) {
    LogInfo("Failed to alloc 0x%08x bytes");
    local_58.dwLength = 0x40;
    BVar1 = GlobalMemoryStatusEx(&local_58);
    if (BVar1 == 0) { LogInfo("Failed to query memory status."); }
    LogInfo("Memory status:");
    LogInfo("Total physical memory:%4I64d MB\tAvailable physical memory:%4I64d MB");
    LogInfo("Total   page    files:%4I64d MB\tAvailable    page   files:%4I64d MB");
    RaiseException(0xe0000001,0xc0000025,0,(ULONG_PTR *)0x0);
  }
  ```
  Policy: allocation failure = logged diagnostics (requested size + full memory status via PUBLOG.DLL::LogInfo — imports :184) then a **non-continuable SEH exception 0xE0000001** → host process death. There is no in-engine recovery path anywhere (no caller of FUN_1800020c0 checks for null).
- **core:** `src/core/carve/cr_carve.cpp` — every emission-site allocation is `new (std::nothrow)` → on null: `ctx.aborted.store(true); return ERR_NO_MEMORY;` (:432-435, :520, :540-543, :561, :643-646, :679, :824-827, :890); `carve_slice` propagates the code. **No OOM-specific diagnostic exists in the carve path** (cr_carve.cpp includes no logging header; grep-verified) — the only trace is the orchestrator's generic scan-end line `"...scan end: rc=%u files=... "` (src/core/orchestrator/cr_scan.cpp:1146-1151), which records the rc but neither the failed allocation size nor any memory status. Additionally the window buffer `std::vector<u8> win(want)` (**:917**, scan_window_index) and the tail-scan buffer (:755) are **throwing** allocations with no try/catch in `carve_slice` — an OOM there propagates `std::bad_alloc` out of the engine rather than returning `ERR_NO_MEMORY` (inconsistent with the nothrow sites).
- **Gap (falsifiable):** force an allocation failure during a carve scan. og: PUBLOG receives the 5 diagnostic lines and the process terminates with exception 0xE0000001. core: either a silent `ERR_NO_MEMORY` (emit sites) or an uncaught C++ exception (window buffer) — no diagnostic record of the failed size or memory status is ever produced.
- **Severity suggestion:** **MINOR** (diagnosability/doc drift; no wrong recovery results — the core's graceful path is arguably safer, but it is an *unregistered* behavioral divergence and the core's own two paths disagree with each other).
- **Lead re-verify first-hand:** (1) confirm no wrapper upstream of `carve_slice` catches `std::bad_alloc` (orchestrator/CLI); (2) confirm the og's 0xE0000001 is not caught by DRWUI (search DRWUI decompile for 0xe0000001 handlers); (3) check whether the core's logging seam (cr_log.h; PARITY-DELTAS D-81/D-86 "one host sink" register) was meant to carry engine OOM events and never wired into the carve path.

### C2 (MINOR) — Per-extension carve-statistics surface (the og's `GetRawUserInfoMap` output that feeds the post-scan per-format report) has no core counterpart: `CarveStats` is scalar-only, the report carries 8 category counts, and the query Row omits `carve_format`

- **og:** `GetRawUserInfoMap` :72213-72221 returns `FUN_180001350()+0x90` — the per-scan `map<wstring,count>`; `FUN_18000ed00` :11376-11531 insert-or-increments per analyser detection (~60 call sites: :11571, :14336, :17856, :18418, :19626, :19969, :20222, :20794, :20892, :21226, :21574, :21926, :24942, :29402, :29919, :30331 … :64079); cleared at every searcher init (:9047-9051); read one-way post-scan by DRWUI's "Trigger_Raw"/"FileSystem" report builder (DRWUI :227538-:227625, counts formatted `%lld` — the per-extension "found N .jpg / M .png" breakdown).
- **core:** `CarveStats` (include/cr/carve/cr_carve.h:27-57) = 13 scalar counters; the orchestrator copies 5 scalars into `ScanStats` (src/core/orchestrator/cr_scan.cpp:1873-1887: `carved_found`, `bytes_scanned`, `carve_bitmap_skipped_bytes`, `carve_zero_strides`, `carve_occupancy_skipped`); the report carries 8 **category** counts (include/cr/recovery/cr_report.h:104 `u64 category_counts[8]`); the query `Row` (include/cr/query/cr_query.h:162-172) exposes `category` but **not** `carve_format`. No aggregation surface for per-format carve counts exists in the engine API, ABI, or CLI.
- **Gap (falsifiable):** after a scan that carves 405 JPG + 42 PNG, ask the engine (not the tree) for the per-format breakdown. og: the map holds {L"jpg":405, L"png":42} and the og report renders it. core: no API returns it; the closest is a full-tree walk reading `props->carve_format` per node (no helper), and the report's scan_analysis shows category totals only.
- **Severity suggestion:** **MINOR** (report/telemetry-content gap; data exists on nodes — an aggregation pass is missing, not the information).
- **Lead re-verify first-hand:** (1) confirm the og's report section truly renders per-**extension** counts (DRWUI :227538-:227625 — CORRECTIONS #9's decode); (2) confirm no core API (cr_ui.h / CLI `report`) already aggregates carve_format (grep `carve_format` over src/cli + src/core/recovery: zero hits this session).

### C3 (INFO — D-101 addendum) — The pending-list 16-entry force-finalize eviction cap (never documented in the trail)

- **og:** FUN_18000c820 :9573-9591: a new size-unknown candidate goes pending only after the list is trimmed — `if ((count != 0) && (0x10 < count + 1))` → the OLDEST pending node (head+0x10) is force-finalized (`vt+0x10(node, sector, pos)` :9579), enqueued (`FUN_18000d980` :9582), host-notified (:9584-9585), unlinked and freed (:9587-9591). At most **16** multi-sector files are tracked concurrently; overflow force-closes the oldest at its best-known size.
- **core:** no pending model at all — **COVERED-BY D-101** (docs/PARITY-DELTAS.md:2459+, "stateless single-shot validators", REGISTERED ARCHITECTURAL DIVERGENCE). The 16-cap constant and the oldest-first eviction semantics are a **new detail** for D-101's register text (they matter if a future streaming analyser is ported behind the M-021 seam: the og's bound caps memory AND truncates the least-recent file on overflow).
- **Severity:** INFO (register addendum; no core behavior to change today).

### C4 (INFO) — Stop() flush semantics: the og force-finalizes in-flight pending files on Stop (partial files are emitted); the core's cancel discards un-emitted candidates

- **og:** FUN_18000ce30 :9658-9693 — `this+0x150 = 1`; when the device is idle (device+0x14==0 && vt+0x70()==false) each pending FileInfo is finalized (`vt+0x10(node, 0, -1)` :9676) and the host sink (+0x108) is notified per file (:9677-9679) before the list is cleared and the buffer freed.
- **core:** cancel → `ERR_CANCELLED`; candidates already emitted stay in the tree; anything mid-window is dropped (no pending state exists — D-101).
- **Disposition:** the difference is inherent to the registered D-101 architecture (no pending objects to flush) plus N-017 (cancel watcher, FIXED). Worth one sentence in D-101's boundary note; not a standalone finding. **INFO.**

### C5 (INFO) — Engine-side progress: the og's engine emits NO progress events (its only observer interaction is the per-chunk continue-check); the core's carve reports per-1-MiB-window progress — a deliberate superset already registered at product level

- **og:** the only observer calls in the entire DLL: the per-chunk continue-check FUN_18000c820 :9439-9441 (`observer->vt+0x10()`), the setter (vtable+0xd8), and Stop's clear. No percent/position reporting from the engine; DRWUI computes progress host-side (its timer farm — D-78's subject).
- **core:** `carve_slice(..., bool (*progress)(u64, u64, void*), ...)` invoked per window (cr_carve.cpp, single-thread path `progress(wi * WINDOW, ctx.total, ...)`; parallel path `progress(done*WINDOW, total, ...)`; final 100% tick).
- **Disposition:** **COVERED-BY D-78** (docs/PARITY-DELTAS.md:2366+ registers the push-events progress model as a deliberate divergence). Carve-layer note only. **INFO.**

### C6 (INFO — I-009-adjacent) — The og's setup refuses device sector sizes that are not a multiple of 512

- **og:** FUN_18000c430 :9265-9268 — `if ((param_2 == 0) || ((param_2 + ((param_2>>63)&0x1ff)) & 0x1ff) != carry)) return 0;` — Search (FUN_18000c650 :9349-9353) then fails: **no carve at all** on e.g. 520-byte-sector SCSI/RAID layouts.
- **core:** byte-granular, geometry-agnostic (the I-009 superset — no sector concept at the carve layer).
- **Disposition:** subsumed by I-009's registered geometry superset; the explicit *refusal* behavior (vs. merely sector-aligned candidates) is a nuance worth one clause in I-009's register text. **INFO.**

---

## §4 Coverage notes (what I could not read/verify, and trail-precision corrections)

1. **The controller's +0x150 stop flag is write-only inside RAWSearchDLL** (written at :9667; the other `+0x150` hits at :39551/:39686/:39733/:39757 belong to a different object — an analyser at another base). UNVERIFIED: whether the host reads it via an interface outside this binary, or it is vestigial. Question for the lead: dump DRWUI for reads of the controller at +0x150 / a GetState vtable slot.
2. **The user-info singleton's +0x60 (=1) and +0x68 (=0) resets** (:9048, :9050) have **no in-DLL reader** (grep over all 70 FUN_180001350 call sites: only +0x70 [the 200 MB cap, 12+ readers] and the stats-map base are read). UNVERIFIED: their semantics (likely host-visible state via the DRWUI telemetry read path — CORRECTIONS #9's consumer). The full object layout (this sheet §1) extends SCAN-ENGINE-DELTA A4's "+0x70 cap" note: {+0x00=1, +0x08=0, +0x10=200000000, +0x18=vector<uint> ids 1..169, +0x30=stats map, +0x40=byte, +0x50=string, +0x60=0xff}.
3. **s3-rawsearch-inventory.md §5.7 wording correction (CORPUS-ERRATA-grade):** "the controller's chunk driver (external virtual)" — the chunk loop is **inside the DLL**: FUN_18000c650 at vtable slot **+0x118** (the same function the CellTest export calls directly). The host calls Search(range) once; the loop over `range/sectors_per_chunk` chunks (:9361-9376) is engine-internal.
4. **s3 §5.5 shape correction (CORPUS-ERRATA-grade):** "searcher+0xd0 array of 0x1000 maps" — precisely: a 0x10048-byte object = **4096 slots × 0x10 bytes** (per-format {intrusive-list-head, count} buckets, ctor FUN_18000db00) + all-results `std::list<CRawFileInfo*>` @+0x10000 + `IIterator<ListIterator<CRawFileInfo*>>` wrapper @+0x10010 + **two** offset-keyed `std::map`s @+0x10028 and +0x10038 (allocation :9293-9327).
5. **FUN_18000b7a0** (:8594-8621) pins the DLL-side format-namespace mechanism the trail described only from DRWUI: a **9-byte-stride NUL-padded name table at DAT_18008f070, indexed by the FileInfo's format id** (`&DAT_18008f070 + id*9`). Supporting evidence for S-049's `EXT_%06d` default prefix; no separate finding.
6. **The controller vtable dump (§1) is new trail datum** — previously only slots +0x48 (naming hook setter) and +0x68 (Init) were on record (s5-oq-s4-2). RTTI-verified: COL@RVA 0x757d8 → `.?AVCRAWSearchController@@`; adjacent COL@0x757b0 → `.?AV?$ListIterator@PEAVCRawFileInfo@@@@`.
7. **Not re-walked (grammar, per the lane split):** the ~1,131 analyser/validator/FileInfo/builder functions, the u32/u16/list registry contents (s3 §3/§4/§4b own them), CRawFileCellTestStorageDevice internals (112 vftable refs; the test-harness device — deep-dive §1 covers its role), the raw_signs tokenizer (deep-dive §3.1), tinyxml2/CJsonObject bodies.
8. **CRT events/critical-sections:** all CreateEventW/WaitForSingleObjectEx/SetEvent/ResetEvent usage in the binary belongs to the CRT's `_Init_thread_*` magic-static machinery (:72682-72874) — the engine's only own synchronization is the controller CS at +0x128 and the CRT heap lock. Confirms: no engine threads, no engine locks beyond result-list guarding.
9. **WS2_32 Ordinal_14 (ntohl)** at :53900/:53908 sits inside a grammar helper (byte-order util) — left to the grammar lane.
10. **Core-side re-verification done this session:** cr_carve.h/cr_carve.cpp read in full (WINDOW=1 MiB / OVERLAP=4096; nothrow sites; progress sites; per-window cancel; no logging include); cr_scan.cpp:1860-1897 (carve stage wiring + stats copy); cr_query.h Row; cr_report.h AssistantStats; PARITY-DELTAS D-78/D-100/D-101/D-104 checked; board findings S-048/S-049/S-050/S-051/M-073/M-106/M-107/M-108/M-109/N-111/N-112/N-113 and modules/carve.md checked for overlap.

---

## LEAD VERIFICATION LOG (S11, first-hand — appended on filing)

- **C1 core-side cites re-pinned at `46b81573`** (the M-096/N-093 landings shifted
  cr_carve.cpp by 2 lines): the throwing window buffer is now **:919**
  (`std::vector<u8> win(static_cast<size_t>(want), 0);`), the tail-scan buffer
  **:757**; the nothrow sites confirmed at :434-437/:522/:542-545 (+ 5 more).
  `grep -rn "catch|bad_alloc" src/app/` → **zero** (no handler upstream of
  `carve_slice`; an OOM in the two throwing vectors escapes the engine as an
  uncaught exception).
- **C1 og-side**: the FUN_1800020c0 body re-read at :958-983 (the five LogInfo
  lines + GlobalMemoryStatusEx + `RaiseException(0xe0000001, 0xc0000025, 0, 0)`
  quoted verbatim in §3 above). **DRWUI carries the SAME policy, not a handler**:
  `grep 0xe0000001 ghidra-drwui/DRWUI_decompiled.c` → :94179 = DRWUI's own copy
  of the identical operator-new RaiseException (the product-wide OOM policy:
  log-then-die; the crash reporter owns the aftermath). No handler exists — the
  og's OOM path is process death with diagnostics, as filed.
- **C2 re-scoped after a first-hand correction**: the original candidate text's
  "no aggregation surface … grep carve_format over src/cli + src/core/recovery:
  zero hits" was scoped too narrowly — **`carve_format` DOES exist** in
  `src/ui/cr_ui.cpp:3224` (the per-node ABI field) and :3467-3490
  (`cr_ui_carve_format_count`/`cr_ui_carve_format_info` — the **format-REGISTRY
  enumeration**, not scan results). The filed finding (N-140) is scoped to what
  is genuinely absent: the per-scan **aggregated** extension→count surface (the
  og's GetRawUserInfoMap + DRWUI report reader).
- **C3/C4/C5/C6** filed as the I-031 register (unchanged from the sheet).
- **The controller-vtable map (§1)** and the five §4 precision notes are new
  trail data — the §4 notes carry forward to CORPUS-ERRATA.

## TERRITORY NOTE (for the session-11-lead lane)

The session-11-lead's worklog claim (83d154b5) lists "(1) the RAWSearchDLL
infrastructure + six-family function-level walk" as its wave-1 territory. This
walk was executed COMPLETE by this lane's L1 agent before that claim landed
(agents spawned 17:50, results in hand 18:0x; the claim at 18:15:38). Per the
earlier-push-wins convention the filings here (N-139/N-140/I-031 + this evidence
file) own the territory; the 11-lead's wave-1 should reconcile
extend-don't-refile against them (the six-family carve-grammar half of their
wave-1 remains open to them — this walk dispositioned the INFRASTRUCTURE half
only, the grammar half belongs to the S-007/M-106/M-107 lineage).
