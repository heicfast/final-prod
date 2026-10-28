# S11/L3 — OQ traces + payload probe (Lane L3, OQTRACE)

**Task ID:** S11/L3 · **Date:** 2026-10-28 (session-11) · **Auditor role:** read-only (only this sheet is written)
**Audited trees:** `/home/z/my-project/audit/core_recovery` @ working tree (board carries OQ-S4-3's addenda through 2026-10-28 PARITY-FIX-5) · corpus `/home/z/my-project/audit/easeus-drw-research` @ `03ad46c`-lineage re-workspace
**Method:** direct decompile/string walks in `re-workspace/reversed/` (ghidra-mo, ghidra-partition, ghidra-drwui, ghidra-searchdll), rabin2 string/export tables, objdump/xxd/python for the payload probe. No file inside the two audited trees was touched.

---

## §1 OQ-S8-1 — CWindowsFS consumer trace → **VERDICT (a): consumer DECODED — a user-visible existing-file browsing surface in DRWUI.exe, served through PartitionRecovery.dll's ExplorerPartition; second minor consumer = HFS+ used-space computation**

### 1.0 Why the naive grep dead-ends (and what the real call path is)

`FUN_18000cf10` is **not called by name anywhere** — not even inside `FileSystem_decompiled.c` itself (only the definition at `:10650`; verified by grep over the whole `reversed/` tree). All same-name hits in other binaries are **address collisions, different functions** (each .mo is its own PE address space):

- `ghidra-searchdll/ZfsSearchDLL_decompiled.c:10842` `FUN_18000cf10(longlong*,undefined8*,longlong*,longlong*)` = 401-byte tree-node destructor (`ZfsSearchDLL_functions.txt:196`); callers at :10606/:10648/:30193/:30309/:32997 are its own destructor tree — **not** the FileSystem factory.
- `ghidra-mo/DeviceManager_decompiled.c:8757` = 56-byte 3-arg local function (`DeviceManager_functions.txt:500`); `ghidra-partition/PartitionRecovery_decompiled.c:8318` = 163-byte 3-arg (`PartitionRecovery_functions.txt:397`); `Burn_decompiled.c:4239` = 69-byte 3-arg; `RecoveryModule_decompiled.c:2281` = 28-byte void — all unrelated.

The real path is the MOM interface chain (per `artifacts/PartitionRecovery/mom-module-system--architecture.md:24-37`): modules are requested **by name string** (`FUN_1800017f0("<name>")` → name+".mo" → `LoadLibraryA` → `GetProcAddress("GetModule")`), and services are then called **through the module object's vtable**. `FileSystem.mo`'s only real export is `GetModule` (`reversed/PartitionRecovery/FileSystem.mo/rabin2_exports.txt:35` — `0x18000dae0 GetModule`), so the factory is a virtual method, reached as **module-vtable slot +0x38** (two independent call sites use exactly that slot, below).

### 1.1 Consumer call site #1 (the primary one): PartitionRecovery.dll's **ExplorerPartition** → `FUN_18000d220`

`ghidra-partition/PartitionRecovery_decompiled.c:8440-8503` (`FUN_18000d220`), the fs-wrapper attach inside the class whose ctor sets **`ExplorerPartition::vftable`** (`:8207`, dtor `:8263`; interface ctor sets `IExplorerPartition::vftable` `:8180`):

```c
8481:  (plVar8 = (longlong *)FUN_1800017f0("FileSystem"), plVar8 != (longlong *)0x0)) {
8482:    puVar5 = FUN_180001740(param_1 + 1,local_10);
8483:    plVar9 = (longlong *)FUN_180001730(puVar5);
8484:    iVar2 = (**(code **)(*plVar9 + 0x30))(plVar9);          // FS type
8485:    if ((iVar2 == 5) || ((iVar2 == 6 || (iVar2 == 0x18)))) local_50 = 1; else local_50 = 0;
8491:    lVar10 = (**(code **)(*plVar8 + 0x38))(plVar8,pIVar6,local_50);   // <-- FACTORY CALL (module vtable +0x38)
8492:    param_1[2] = lVar10;                                    // store IFSInterface at +0x10
```

- The flag (`local_50`) is the factory's `param_3` low byte — 1 for FS types 5/6/0x18, which in the factory (`FileSystem_decompiled.c:10695` `if ((cVar1 != '\0') && (cVar9 == '\0'))`) suppresses the early CWindowsFS branch; for NTFS(4)/HFS+(8)/other the flag is 0 and the mounted path (`FUN_18000d590` probe → CWindowsFS ctor `FUN_1800117d0`) can win.
- What the wrapper DOES with the IFSInterface: `FUN_18000d4d0` (`:8529-8565`) calls **vtable slot 0** of the stored interface with a converted path string + a find-data out-param (`uVar2 = (**(code **)**(undefined8 **)(param_1 + 0x10))(*(undefined8 *)(param_1 + 0x10),uVar2,param_3)`, `:8546-8547`) — the find-first enumeration entry. `FUN_18000d0d0` (`:8402-8422`) copies a **0x468-byte find-data record** and passes it on (`FUN_18000d940`), and `FUN_18000d440` (`:8508-8523`) is the Release-on-destruct. On the CWindowsFS side the find-first implementation is `FUN_1800119f0` (`FileSystem_decompiled.c:14132+`, local `_WIN32_FIND_DATAW`, `FindFirstFileW`) with `FUN_1800119e0` = `FindClose` (`:14120-14127`). The ctor resolves the volume root path via the `+0x38→+0x40→+0x68→+0x98` chain (`:14064-14076`, per the OQ decode).
- Interface names decoded from `FileSystem.mo` RTTI (`reversed/PartitionRecovery/FileSystem.mo/rabin2_strings_data.txt:211-218`): the factory queries **IPartitionEntry** (`:215`, 0x18001d298) and **IFileSystemPartition** (`:216`, 0x18001d2c0; `+0x30` returns the FS type), the early-branch gate is **IVolumeEntryInfo** (`:217`, 0x18001d2f0), the ctor uses **IManagerItem** (`:218`, 0x18001d318), and the mounted probe `FUN_18000d590` (`:10896-10919`) queries **IDiskDeviceType** (`:214`, 0x18001d270) and returns its `+0x28` boolean. (Factory flow: `:10673` GetInterfaceEx(IPartitionEntry) → `:10682-10684` GetInterfaceEx(IFileSystemPartition), `+0x30`=FStype, bitmask `0x145617e`, `<0x19` → `:10693-10695` GetInterfaceEx(IVolumeEntryInfo), `+0x30` gate → CWindowsFS.)

### 1.2 Who drives ExplorerPartition: **DRWUI.exe, via the exported factory — a Qt file-browsing surface**

- `ExplorerPartition` is created by PartitionRecovery.dll's export **`CreateExplorerPartitionObject`** (`PartitionRecovery_decompiled.c:890-906` → ctor `FUN_18000cc90` `:8200-8219` sets `ExplorerPartition::vftable`; export listed in `reversed/PartitionRecovery/PartitionRecovery.dll/rabin2_exports.txt:58`).
- **DRWUI.exe loads it explicitly**: `ghidra-drwui/DRWUI_decompiled.c:438825` `FUN_14027df40` builds `"<base>\\PartitionRecovery"`, `SetDllDirectoryA` (`:438861-438863`), `LoadLibraryA("...\\PartitionRecovery.dll")` (`:438864-438877`), then resolves **five** object factories by name (`:438885-438893`): `CreateEnumPartitionObject`, `CreateScanLostPartitionObject`, `CreateRecoveryPartitionObject`, **`CreateExplorerPartitionObject`** (`:438891`, stored at +0x28), `CreateCheckPartitionObject`.
- The stored pointer is invoked via `FUN_14027dc90` (`:438721-438733`: `if (*(longlong *)(param_1 + 0x28) != 0) uVar1 = (**(code **)(param_1 + 0x28))();`) — called at **`:1433572`** inside `FUN_14091c620` (`:1433564-1433587`): lazily creates the ExplorerPartition object, stores it at +0x70, creates a 0x80-byte callback (`FUN_140219bc0`), and registers it (`(**(code **)(**(longlong **)(param_1+0x70)+8))(explorer,callback)` `:1433582-1433583`).
- The **Qt browse API** on that object (all in the `FUN_14091c6xx` family, manager singleton `FUN_14076eea0`):
  - `FUN_14091ca70` (`:1433701-1433733`): `QString→toUtf8` path → ExplorerPartition **vtable+0x40** `(path, listA, listB)` → returns **file count** (`FUN_140933c50`).
  - `FUN_14091c800` (`:1433617-1433696`): same vtable+0x40 call, then copies the **0x468-byte file record** at index `param_3` (`for lVar4 = 0x468; ...` `:1433675-1433679`), converts the record's name to `QString::fromUtf8` — "get file at index from directory listing".
  - `FUN_14091cbb0` (`:1433738-1433766`): **vtable+0x30** `(path)` → boolean exists/is-dir probe.
  - `FUN_14091cca0` (`:1433773+`): full listing with `QFileInfo`/`QString` plumbing.
- **The user-visible surface**: `FUN_1408f3e00(QWidget*)` (`:1411664`, a Qt page/widget builder — QPushButtons/QLabels/QFrames/QTableView) constructs a **QTableView** with 4 columns (widths `0x183/0x96/0x50/0x6e`, `:1412346-1412352`) directly under a `"PartitionLostSep"` separator QFrame (`:1412268`), then populates it from the **root directory listing**:
  ```c
  1412353:  local_3c0 = FUN_14076eea0();                // PR-manager singleton
  1412354:  QString::QString(local_748,"\\");           // path = "\"
  1412355:  local_3b8 = FUN_14091cca0(local_3c0,local_3a0,local_748);  // ExplorerPartition root listing
  1412358:  FUN_1408f29f0(local_7e8,local_3b8);         // feed the table model
  ```
  i.e. a **partition-content browse table** in the lost-partition results page, fed by enumerating the partition's directory tree from its root. Adjacent plumbing: `FUN_140904fc0` (`:1421095-1421127`) either `ShellExecuteA(...,"open",<file>...)` (`:1421113`) or creates the ExplorerPartition session (`FUN_14091c620` at `:1421118`) — an open-file/browse entry point; `FUN_1408f9330` (`:1414842-1414852`) stores the per-directory file count (progress estimation).

### 1.3 Consumer call site #2 (minor): HFS+ **used-space** computation

`PartitionRecovery_decompiled.c:15806-15823`, inside `FUN_180017ca0` (fn start `:15454`) — a per-FS-type used-space switch: types 1/2/3 → `GetFatPartitionUsedSpace` (`:15735`), type 4 → `CNTFSLib::GetSpaceInformation` (`:15742`), **type 8 (HFS+) → the FileSystem module**:

```c
15806:  plStack_fb0 = (longlong *)FUN_1800017f0("FileSystem");
15812:  plStack_fd0 = (**(code **)(*plStack_fb0 + 0x38))(plStack_fb0,pIStack_f88,0);  // factory, flag=0
15814:  local_998 = (**(code **)(*plStack_fd0 + 0x20))(plStack_fd0);   // IFSInterface +0x20 = used space
15821:  uStack_ef8 = (**(code **)(*plStack_fd0 + 0x28))(plStack_fd0,1); // Release
```

Capacity/statistics (partition-list display), not file browsing — but note `flag=0` + HFS+ means the factory's HFS+ branch can select **CWindowsFS** when the HFS+ volume is OS-mounted (`FileSystem_decompiled.c:10751-10770`), so even this flow rides CWindowsFS for mounted volumes.

### 1.4 VERDICT

**(a) Consumer decoded — user-visible existing-file browsing.** The og flow that constructs CWindowsFS is: **DRWUI.exe (main product UI) → PartitionRecovery.dll export `CreateExplorerPartitionObject` → `ExplorerPartition` → MOM "FileSystem" module → factory vtable+0x38 (`FUN_18000cf10`) → CWindowsFS for OS-mounted volumes** (and the raw parsers for unmounted/lost ones — the same browse surface serves lost partitions through the raw engines, which is why the factory has both arms). The UI-side evidence is unambiguous about user visibility: a QTableView with 4 columns, populated from the partition root "\" listing, in the lost-partition results page, with per-index 0x468-byte file records, counts, existence probes and an open-file branch.

Per the OQ's decision rule: the consumer **implies a user-visible existing-file browsing surface** → the lead files the **capability finding** (the core serves volume content exclusively through the raw engines; its only FindFirstFile uses are the `*.crsf` session listings — per the OQ's own core-side verification, `cr_ui.cpp:396`, `rrecov.cpp:927`). Scope caveat for the lead: the finding's disposition depends on whether the merged product's UI scope includes a browse-existing-files surface at all (the core is engine-only, no-UI posture); the og-side fact is now proven either way. A second, minor consumer (HFS+ used-space via IFSInterface+0x20, `FUN_180017ca0`) is a capacity display flow — if the merged product keeps a partition-list capacity display, that half is a smaller parity consideration.

Residual unknown (does not block the verdict): the exact vtable-slot mapping inside `IExplorerPartition` (+0x30 exists-probe / +0x40 enumerate are decoded behaviorally from the UI call sites; the PR.dll-side slot table is data, not decompiled text).

---

## §2 OQ-S10-2 — VideoViewer/sfc payload probe → **VERDICT: format AND consumer DECODED — the " cfs" container: an XOR-obfuscated, custom-LZ-compressed database loaded (and rebuilt) by VideoFileRepairer.exe at repair-engine init. OQ hypothesis 1 CONFIRMED; 2 and 3 REFUTED**

### 2.1 The file

`og-codebase/VideoViewer/sfc` — 211,614 bytes (0x33A9E), `file` → "data". Head (`od -A x -t x1z`):

```
000000 20 63 66 73 2d f8 6c 97 46 41 4e 2e bd a6 c2 a5  > cfs-.l.FAN.....
000010 11 81 2a b2 7e 60 26 d4 04 05 7c 6d dd df c8 aa  >..*.~`&...|m....
```

Tail ends `...79 c5 03 eb 74 62 78 91 46 41 4e 2e 98 e6` (a "FAN." + `98 e6` at 0x33A98-0x33AA0, 6 bytes from EOF). Shannon entropy 7.886 bits/byte; byte-autocorrelation peaks at lag 16 (0.0915) then 32/48/64 — a 16-byte-periodic structure. "FAN." (`46 41 4E 2E`) occurs verbatim only 6× (offsets 8, 0x68, 0xD2E8, 0x196A8, 0x1CDF8, 0x33A98); per-position (mod 16) byte histograms show a dominant byte + ±1 neighbors at every position — the signature of **repeating-16-byte-key XOR over structured plaintext** (mode per position = `11 81 2A 8B 4C 58 60 97 46 41 4E 2E 98 E6 8C 93`).

### 2.2 The consumer (found without Ghidra: raw byte-grep + objdump + the corpus's own wave2 decompile)

Byte-grep of the three VideoViewer PEs for ` cfs`/`cfs`/`FAN.`/`sfc` — hits ONLY in **VideoFileRepairer.exe** (none in EaseUS_VideoViewer.exe / EaseUS_MediaGraph.dll → kills OQ hypothesis 2):

- **0x116a62 / 0x131725 (file off)**: the 16-byte XOR key built inline — objdump `movl $0x9760584c,-0x10(%ebp); movl $0x2e4e4146,-0xc(%ebp); movl $0x938ce698,-0x8(%ebp); movl $0x8b2a8111,-0x4(%ebp)` at VMA 0x517658-0x51766d → key bytes **`4C 58 60 97 46 41 4E 2E 98 E6 8C 93 11 81 2A 8B`** — i.e. **"FAN." is bytes 4-7 of the XOR key**, which is why the motif recurs wherever the plaintext XORs to zero there. Same key inlined at VMA 0x532315.
- **0x115c2c (file off)**: `mov [eax]," cfs"` (dword `20 63 66 73`) — the magic WRITER. **0x11472f**: `cmp` against the " cfs" family — the magic CHECK.
- **0x19c24c (file off, .rdata VMA 0x59D24C)**: the NUL-terminated string **"sfc"** — used as a PATH COMPONENT (adjacent to "m4a", "aiff", "rb"/"rb+"/"wb" mode strings).

Corpus decompile (`reversed/ghidra-wave2/VideoFileRepairer_decompiled.c`, per-binary census `artifacts/per-binary/VideoFileRepairer.md`):

- **`FUN_00517640` (:249943-250060) = the XOR codec**: `local_14[0]=0x9760584c; local_14[1]=0x2e4e4146; local_14[2]=0x938ce698; local_14[3]=0x8b2a8111;` (:249963-249966) then a chunked XOR (SSE `pxor` in the binary) of (ptr,len) with the repeating 16-byte key.
- **`FUN_00514f80` (:248131) = the sfc LOADER**: builds `<base>` + `\` (`DAT_0058d4ec`, 1 char) + **"sfc" (`DAT_0059d24c`, 3 chars)** (:248187-248208 — path concat calls `FUN_004180c0`); `stat` + size gate [4, 0x1000000] (:248238-248242); `_wfopen_s(..., "rb")` (`DAT_00591aa0`, :248273); reads whole file (:248307); **`if (*piVar1 == 0x73666320)`** — magic " cfs" (:248309); **`FUN_00517640(buf+4, size-4)`** (:248313) — XOR-decode everything after the 4-byte magic; reads decoded dword `uVar6` (alloc size, must be 0<uVar6≤0x1000001, :248327-248344); dispatches on the **version nibble at buffer+0xC**: `*pbVar9 >> 4 == 2` → **`FUN_00512fa0`** (v2 parser), `== 1` → **`FUN_00513350`** (v1) (:248357-248364).
- **`FUN_00512fa0` (:246026-246300) = a custom LZ decompressor** — literal tokens (length nibble `&0xf`, extended via LEB128-style varint `uVar17|=(b&0x7f)<<i; while(b&0x80)` :246075-246084, `+0x10` bias), 64-byte long-literal blocks (`_Src_00+0x40`, :246124), and match/back-reference tokens (the `else` branch :246175+) — writing into the 0xCA061-byte output buffer.
- **`FUN_00516270` (:249149) = the sfc BUILDER/WRITER** (first-run/rebuild): same `<base>\sfc` path (:249216-249228), `"wb"` (`DAT_00591fc8`, :249268), iterates the engine object's map (`param_1+0xc..+0x10`), serializes each record via **`FUN_00516a70(char*, FILE*)`** (:249499; `fwrite(rec+0x18c,1,4,f)` ×4 at +0x190/+0x194/+0x198, :249915-249918, plus strings via `std::ostringstream`), writes the " cfs" magic (:249399 `*(undefined4*)local_38 = 0x73666320`) and XORs the payload (:249405 `FUN_00517640(local_38+4, ...)`).
- **The caller = engine init**: the repair-engine ctor **`FUN_005126f0`** (:245536) ends with `cVar4 = FUN_00514f80(param_1); if (cVar4 == '\0') { puVar9 = FUN_00514f40(); if (puVar9 && !cVar2) FUN_00516270(param_1); }` (:246921-246931) — load the DB at startup; rebuild it when the load fails.

### 2.3 The sfc file decoded (bytes cited exactly)

Applying the code's exact semantics (`payload = file[4:] XOR key`, key phase 0 at payload start):

```
magic  = 20 63 66 73                                     " cfs"           (file[0:4])
payload[0:4]  = 61 a0 0c 00   -> LE uint32 0x000CA061 = 827,489  (uncompressed size)
payload[4:8]  = 00 00 00 00
payload[8]    = 25                -> high nibble 2 -> v2 parser FUN_00512fa0
payload[9:12] = 40 4e 36
then the custom-LZ compressed stream (211,598 compressed bytes -> 827,489, 3.91x)
```

(Consistency check: the mode-derived key `11 81 2A 8B 4C 58 60 97 46 41 4E 2E 98 E6 8C 93` equals the code key rotated by 12 — exactly the phase you get applying the code key at file offset 4 — and that decode also yields a table of plausible LE uint32 offsets/sizes at 0x1A0+, e.g. `49 01 00 00 / 20 9e 01 00 / 21 9e 01 00 / ...`.)

### 2.4 VERDICT

**Hypothesis 1 of the OQ is CONFIRMED**: `VideoViewer/sfc` is a live data asset of the video-repair line — an engine-init database in the proprietary " cfs" container (4-byte magic `20 63 66 73`, payload XOR-obfuscated with the 16-byte key `4C 58 60 97 46 41 4E 2E 98 E6 8C 93 11 81 2A 8B`, header = uncompressed-size dword + version byte, then a custom-LZ-compressed serialized map of ≥0x19C-byte records), loaded at VideoFileRepairer.exe's repair-engine construction (FUN_005126f0 → FUN_00514f80) and rebuilt on demand (FUN_00516270). Hypothesis 2 (viewer/Qt payload) is refuted (zero references in EaseUS_VideoViewer.exe / EaseUS_MediaGraph.dll); hypothesis 3 (dead payload) is refuted (actively loaded + rebuilt). Disposition for the register: **repair-family data asset, OOS by the repair exclusion (I-010/D-26) → a registration row (format identified as above), NOT a capability finding**. The "Ghidra magic-search" the OQ queued is no longer needed — the magic, key, grammar and consumer are all cited above. Residual unknown (non-blocking): the semantic content of the records (codec/sample info vs. statistics) would need FUN_00512fa0's decompressor ported; not required for the disposition since the repair line is out of scope.

---

## §3 OQ-S4-3 — chunking-relative-perf scoping → **VERDICT: NO — an executed comparison is NOT feasible in this Linux container (the remaining TTFR half is Windows-driver+og-product-bound by nature); the transport half is already measured-and-closed; the precise minimum environment is below**

### 3.1 What the OQ asks NOW (post-addenda state, re-verified on the live board)

The transport-chunking half is **MEASURED and closed** — alignbench run `40315255368` (2026-10-26, `win22`, `\\.\C:`, 512 MiB, qd=1, RDA service running) produced the `chunk_strategy_ratios` verdict transcribed in the OQ (og 4 MiB buffered-sync 197.32 MiB/s / cpu 609.4 ms vs core 64 KiB envelope 203.98 / 593.8 — og trails 3.3–3.8% — and core 256 KiB cache block 241.64 / 562.5 — core +18–19% at lower CPU; RDA-path legs 194.34/237.54 MiB/s confirm through the driver). The ONLY remaining open half is **scan-level time-to-first-result (TTFR)**: core TTFR is harness-ready, og-side TTFR is not yet captured. Acceptance = both numbers + transcription into the OQ + M-030's Resolution addendum + SUMMARY's performance section.

### 3.2 The harness state on the live tree (all verified this session)

- **Core probe (landed, PARITY-FIX-5)**: `src/app/rrecov.cpp:205-213` — `struct TtfrProbe` + `ttfr_event_fn` logging `ttfr_ms=%llu first_files=%llu stage=%u` at the first positive FilesFound event; wired before ScanController construction at `:1545-1547` (`cfg.event_fn = &ttfr_event_fn; cfg.event_user = &ttfr_probe;`).
- **Extraction (landed)**: `.github/scripts/Run-Benchmark.ps1:289-303` — regex `ttfr_ms=(\d+) first_files=(\d+) stage=(\d+)` over `stage-<mode>.log`, stored into `$cleanPerf.scans[-1].ttfr_ms/.ttfr_first_files`, printed per mode.
- **The lane**: `.github/workflows/test-benchmark.yml` — windows-2022, elevated, HVCI-off class, prebuilt release only (fetch-release.ps1: `rda.sys` + `rda-test-sign.cer` + `install.ps1` + `core-recovery-*.zip`), driver install/verify (`Install-ReleaseDriver.ps1`), API harness build, then `Run-Benchmark.ps1` over `.github/scenarios/benchmark-matrix.json` (fixed-size VHDX disks made by `New-TestDisk.ps1` — e.g. `small-fat32` 2048 MB MBR, `multi-gpt` 5120 MB GPT NTFS+…), teardown + structured evidence artifact (`07-performance/`). Triggers: manual `workflow_dispatch` (inputs `core_tag/profile/scan_modes/reason`) + automatic dispatch from `release.yml:551` after every publish. **No cron exists anymore** (grep `schedule|cron` over `.github/workflows/*.yml` → no hits) — the OQ's "daily cron 05:30" reference is stale (it described the mid-landing staged-scenarios lane the owner's 3-lane consolidation replaced). Also stale: `bench/README.md` still says "Everything runs on GitHub-hosted Windows runners via `.github/workflows/comparative-bench.yml`" — that workflow no longer exists (the OQ's 2026-10-28 probe-target correction is correct; workflows present: checks.yml, installer.yml, release.yml, test-benchmark.yml).
- **Transport harness (for reference, the already-run half)**: `bench/ours/run-alignbench.ps1:387-390` — `Strat-Pair 'og4m_vs_core64k' 4194304 65536` / `'og4m_vs_core256k' 4194304 262144` / `'og7m_vs_core64k' 7340032 65536` / `'og7m_vs_core256k' 7340032 262144`; `cpu_ms` per measurement (`:69`, `:211` — `Process.TotalProcessorTime` delta); `chunk_strategy_ratios` verdict block (`:397`). PowerShell/.NET — Windows-only.

### 3.3 Does the core build/run on Linux? (from build files only — nothing executed)

**Yes for the pure core + unit tests; no for anything that can produce the TTFR number:**

- `CMakeLists.txt:31-36` carries a full non-MSVC branch (`-Wall -Wextra -Wpedantic -Werror -Wshadow`); zlib "on Linux the system package" (`:48-53`); lz4/zstd required with apt fallbacks (`:141-147`); tests via GTest + FetchContent (`:170-180`).
- `CMakePresets.json` defines `linux-gcc` (RelWithDebInfo, ASan, `CR_BUILD_APP=OFF`), `linux-release`, `linux-ubsan` + matching build/test presets (`:15-65, 87-113`).
- `src/CMakeLists.txt:10-14` gates the app under `if(WIN32)` → **`rrecov` (the TtfrProbe carrier) is not built on Linux at all**.
- CI is windows-only by owner directive (`checks.yml:1-8`: "the ubuntu sanitizer/fuzz lanes were dropped by the owner's windows-only directive"); local Linux validation exists via `ci-local.sh` (`--san`/`--fuzz` only). `docs/PERFORMANCE-BASELINE.md:9-14` codifies the split: local Linux container = "logic-shape baselines, unit suites"; windows-2022 runner = "driver microbench, comparative bench, integration".
- Unit-suite relevance: `tests/unit/` (61 suites) incl. `test_query_perf.cpp` — logic-level perf only; nothing measures volume-I/O chunking on Linux, and the acceptance's TTFR is a **real-volume scan on the RDA driver** measurement.

### 3.4 Corpus evidence on the og's I/O chunking (re-verified first-hand)

- **4 MiB disk chunk**: `re-workspace/reversed/ghidra-mo/Device_decompiled.c` ~:56545 — `*(int *)(param_1 + 0x10) = (int)(0x400000 / (longlong)*(int *)(param_1 + 0x38))` (0x400000 = 4,194,304 = 4 MiB chunk budget / sector size → read count).
- **7 MiB volume chunk**: same file ~:56912, `FUN_180062480` = the `CVolumeDevice` ctor — `*(undefined ***)param_1 = CVolumeDevice::vftable; param_1[0x10] = 0x20; *(int *)(param_1 + 0x18) = (int)(0x700000 / (longlong)param_2)` (0x700000 = 7,340,032 = 7 MiB / sector size).
- **No corpus prose artifact on chunking exists**: grep over `re-workspace/artifacts/` for `chunk|4 MiB|7 MiB|0x400000|0x700000|M-030|ReadFile chunk` → zero I/O-chunking hits (only unrelated RIFF/rsf-chunk and btrfs-mirror-offset matches). The corpus evidence for M-030 is the Device.mo decompile itself (the two cites above), which the board already carries.

### 3.5 The og-side TTFR gap (found this session — the real remaining engineering)

The og lane's wait loop (`bench/og/og-driver.ps1:361-412`, `Wait-OgRsf`) samples only `t_ms,working_set_bytes,cpu_seconds` (`:367-368`) and detects completion via a NEW auto-saved `.rsf` or DRWUI.log markers (`'Build tree finish'` / `'CScanMgr::ScanFinished'`, :385/:399). **No first-found timestamp is captured on the og side** (grep `first_found|FilesFound|files_found` over `bench/og/`+`bench/ours/`+`Run-Benchmark.ps1` → only the core-side `scan_stats.files_found` total at `Run-Benchmark.ps1:251` and the core TTFR extraction at `:289-303`). So the acceptance's "og-side TTFR" number needs either (a) a small og-driver addition (poll the UI's found-count label / parse DRWUI.log for a first-found marker with a timestamp — the corpus log artifacts do not document such a line: grep of `config-files/vendor-logs--qa-session-forensics.md` + `qa-logs--session-forensics.md` for found-file lines → no hits), or (b) an owner decision to close the half with the core-side TTFR + the already-measured transport numbers as the operative verdict (the OQ itself notes the measured verdict "already answers the finding's operative question").

### 3.6 VERDICT — feasibility this session

**NO-BECAUSE (not executable from this Linux container):**
1. The TTFR probe lives in `rrecov` (Windows-only target, `src/CMakeLists.txt:10-14`); Linux presets build core+tests only.
2. The measurement is real-volume scan I/O through the **RDA Windows kernel driver** on seeded VHDXs, elevated, HVCI-off — the OQ's own session-14/37 addenda already ruled the Linux container out ("the question is real-hardware I/O behavior by nature"), and the transport half needed exactly that environment (run 40315255368 on `win22`).
3. The og-side comparison additionally needs the og DRW v21.0.0.0 GUI + UI automation (Windows-only, `bench/og/`).
4. Nothing to try lightly: a Linux core build is possible but produces no acceptance-relevant number.

**Minimum environment for the remaining half (precise):**
1. A windows-2022-class GitHub runner (elevated, HVCI off — the execution-validated class, `checks.yml:33-35`/`docs/PREBUILT-ARTIFACTS.md`).
2. Dispatch **Test & Benchmark** (`workflow_dispatch` on `test-benchmark.yml`, profile `smoke`, `scan_modes=quick,deep,deepcarve`; or let `release.yml:551` dispatch it on the next publish) — it self-contains steps 2-4: fetch+verify the latest combined release, install/verify the test-signed RDA driver, build the fixed-VHD matrix (`benchmark-matrix.json` + `New-TestDisk.ps1`), run the scans with `--log stage-<mode>.log`.
3. Post-run: read `ttfr_ms`/`ttfr_first_files` from the evidence artifact (`07-performance/`, `$cleanPerf.scans[]`) and transcribe into OQ-S4-3 + M-030's Resolution addendum + SUMMARY's performance section.
4. For the og-side TTFR (the acceptance's second leg): a Windows machine with the og product + the `bench/og/` lane, PLUS the small og-driver probe addition from §3.5 (or the owner's scoped closure decision).

**Plan if the lead wants maximum progress from Linux this session (already delivered by this lane):** the scoping above + the stale-reference corrections (no cron; comparative-bench.yml gone; bench/README.md stale) — those belong in the OQ's next addendum. No execution path exists from here.

---

## §4 What I could not resolve (and what would resolve it)

1. **OQ-S8-1 residual**: the exact `IExplorerPartition` vtable slot table on the PartitionRecovery.dll side (+0x30 exists-probe / +0x40 enumerate / +8 set-callback are decoded behaviorally from the DRWUI call sites; the PR.dll vtable is data, not decompiled text — Ghidra data-table listing of `IExplorerPartition::vftable` would name them). Does not affect the verdict (the UI-side behavioral decode + the factory chain is complete).
2. **OQ-S8-1 scope question left to the lead**: whether the merged product's UI scope includes a browse-existing-files surface (the og-side fact — user-visible QTableView browsing fed by CWindowsFS for mounted volumes — is now proven; the disposition, capability finding vs deliberate-absence registration, is the lead's scope call per the OQ's own decision rule).
3. **OQ-S10-2 residual**: the semantic content of the sfc records (codec/sample DB vs statistics) — would require porting `FUN_00512fa0`'s custom LZ decompressor (~275 lines of decompile) and dumping the 827,489-byte decoded stream; NOT needed for the disposition (repair line OOS).
4. **OQ-S4-3 residuals**: (a) the TTFR numbers themselves — execution-bound (Windows lane dispatch; §3.6); (b) the og-side first-found log-line format — the corpus's qa-log artifacts don't document one; a live og run or a deeper DRWUI.log capture would resolve it; (c) two stale references on the board/harness worth correcting at next touch: the OQ's "daily cron 05:30" (no schedule exists in any workflow) and `bench/README.md`'s pointer to the deleted `comparative-bench.yml`.

## Read-only discipline note

No file under `/home/z/my-project/audit/core_recovery` or `/home/z/my-project/audit/easeus-drw-research` was created, modified or deleted this session; all tools were read-only (Read/Grep/Glob/LS/objdump/od/python-statistics over the trees). The only write is this scratch sheet.
