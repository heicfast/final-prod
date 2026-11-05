# PARITY-ARCH — Architecture & Structure Parity Audit (Task 2-d)

<!-- session-96: the driver era ended - the kernel driver was replaced by direct raw I/O; the driver-era rows below are historical records. -->

**Auditor:** parity sub-agent 2-d (read-only; no repo modifications)
**Date:** 2026-10-23

## Pins (audit basis)

| Repo | Commit / tag | Role |
|---|---|---|
| core_recovery | `2473c03` + `d42bb00` (HEAD, clean tree) | ours — C++20 engine core |
| rda-wizard | `c2eaad6` + `c083bdd` (HEAD, clean tree) | ours — WPF wizard (Core+Integration shipped; App in-flight) |
| rda-driver | `7e88e38` = tag `v0.2.1` | ours — kernel transport (read-only reference) |
| easeus-drw-research | `18d6f14` | RE corpus (evidence base) |

**Coverage:** module boundaries, dependency direction, abstraction seams, repo layout, build-target shape, CI posture, resource organization, localization structure, hygiene. Wire-level transport detail and per-engine algorithm parity are owned by other agents (2-a/2-b/2-c class); only boundary *shape* audited here.

**Method:** include-graph built by `rg` over `src/` + `include/` (not from docs); corpus `architecture/00-05` + `MOM-modules/*` + `mo-module-map` read in full; targeted `og-codebase/` layout inspection; every claim below carries file:line citations. Prior agents' docs (AUDIT.md, AUDIT-UI.md, ADR-1..12, README) were treated as *claims to verify*, not evidence.

---

## 1. og ↔ ours module map

| og element (corpus evidence) | ours (file:line) | Verdict |
|---|---|---|
| `DeviceAdapter.mo` — `IStorageDevice`, `IVolumeDevice`, `IBlockDevice`, `IDiskDevice`, `IPartitionDevice` (RTTI, DeviceAdapter.md:9) | `cr::storage::IStorageDevice` + `Geometry`/`ReadErrorMap`/`CancelToken` (include/cr/storage/cr_device.h:86-100) | **MATCH** |
| `Device.mo` — opens `\\.\EPRDTDRV\%d\%d`, `\\.\EPRDTDRV\%d\Volume\%d`, `\\.\PHYSICALDRIVE%d`, `\\.\HarddiskVolume%d` (Device.md:39-44) | `cr::win` transport duality: `open_rda_partition/open_rda_volume` + `open_physical_drive/open_harddisk_volume` (include/cr/win/cr_transport.h:27-34); RDA-first with direct fallback (docs/benchmarking.md:19 "the Device.mo duality") | **MATCH** |
| `DeviceManager.mo` — enumeration, WMI, hot-plug (DeviceManager.md:1) | `cr::win::enumerate` (src/win/cr_enumerate.cpp; geometry 4Kn/512e + GPT/MBR via pure core per docs/architecture.md:35) | **MATCH** |
| `CStorageDeviceWithCache` (corpus ntfs-engine; cited cr_device.h:11) | `cr::storage::CachedReader` (include/cr/storage/cr_cached.h) | **MATCH** |
| `Partition.mo` + `LdmManager.mo` (MBR/GPT/EBR, LDM dynamic disks; mo-module-map.md:26-27) | `cr::table` — GPT/MBR/EBR (cr_table_parse.cpp) + full LDM decode (cr_ldm.cpp, roadmap v0.2 "LDM dynamic-disk layer… DONE") | **MATCH** |
| `FileSystemAnalyser.mo` family — `CFileSystemAnalyser` + IFSAnalyser (FileSystemAnalyser.md:13) | FS probe + dispatch in orchestrator: `probe_fs` FsFamily switch (src/core/orchestrator/cr_scan.cpp:85-131) | **MATCH** (shape: function API, not vtable interface — consistent with in-process design) |
| `*SearchDLL.dll` × 9, `EUCreate<FS>Searcher`/`QuickSearcher` factory exports, `LoadLibraryW` plugin load (05-scan-engine-architecture.md:28-43) | in-process engines + `FsFamily` switch dispatch (cr_scan.cpp:268+); no DLLs | **DELIBERATE-DELTA** — documented README.md:39 "no engine DLLs — in-process registry (crCreate* factories)" (naming inaccuracy: F-D-10) |
| Engine controllers = tree builders (MI `ICompleteFileSystemSearchController` + `DetourDirectoryTree`, 05:45-57) | engines fill `tree::ResultTree` via explicit refs (cr_mftwalk.h:77-80); orchestrator merges (docs/architecture.md:12-16) | **MATCH** (same responsibility, no MI — cleaner seam) |
| `PartitionRecoveryD.mo` deep-scan block parsers, lost-partition scan (mom-module-system §3) | `cr::lost` candidates (src/core/lost/cr_candidates.cpp) | **MATCH** |
| `NTFSLib.mo`/`NTFSUtil.mo`/`FatLib.mo` format libs (mo-module-map.md:30-33) | `cr::ntfs` internals (cr_mftwalk/cr_attributes/cr_runlist/cr_bitmap…), `cr::fat` chains/LFN | **MATCH** |
| `RefsFileSystemAnalyser.mo` MSB engine (mo-module-map.md:29) | `cr::refs` (cr_refs_engine.cpp; roadmap "ReFS FULL engine… DONE") | **MATCH** |
| `RAWSearchDLL` 190+ analysers + `raw_signs.json` external DB (05:84-93) | `cr::carve` versioned registry, compiled-in signature table (cr_rawsigns_data.cpp; cr_signatures.h:2-5 "versioned signature registry") | **DELIBERATE-DELTA** partially: 30-format compiled-in table vs og's external 139-ext JSON — externalization gap folded into F-D-7 |
| `RawFixer.dll` transactional FS repair (rawfixer-fs-repair.md) | `cr::repair` (cr_rawfixer.cpp; S1-S6 safety standard cr_rawfixer.h) | **MATCH** |
| `RecoveryModule.dll` recovery orchestration (mom-module-system §2:85) | `cr::recovery::SafeWriter` (cr_safewriter.cpp) + orchestrator recover stage | **MATCH** |
| `.rsf` SaveScan sessions (rsf-format artifact) | CRSF own format, `.rsf`-proven chunked-zlib framing (docs/architecture.md:63-64; docs/crsf-format.md) | **DELIBERATE-DELTA** — documented ("own format") |
| `EPMNtDrv`/`EUEDKEPM`/`EUDCPEPM` 7 driver binaries (02-kernel-driver-architecture.md:9-17) | `rda-driver` one merged `rda.sys`, separate repo v0.2.1 (rda-driver README.md:1-6) | **DELIBERATE-DELTA** — documented (rda-driver README "Why" section) |
| `EuMrx.sys` NAS redirector + `cloud_session`/`EuICloudRecovery` | out of scope by decision (core README.md:28-32; roadmap "Deferred") | **DELIBERATE-DELTA** — documented |
| Fixo repair chain, ToolsModule (WinPE/burn/imaging) | roadmap v0.3 candidates (docs/roadmap.md:66-70) | **DELIBERATE-DELTA** — documented as future |
| `MainModule.dll` partition-**editing** engine — `CBaseOperation`/pending ops, writes new partition tables for found partitions (mom-module-system §2:77-84, §3:96) | **absent** — lost partitions are virtual candidates scanned in place; no table-write path; not in roadmap or deferred list | **MISSING** → F-D-5 |
| `Log.mo` (`IEsLog`, `CFileMappingLog`, `EPMLOG.log`, Log.md:12-15,23) + `Logger.dll` + `PubLog.dll` + DRWLoader 2 MB shared log | **absent** — no logging seam in core; only CLI `--log` `vfprintf` (src/app/rrecov.cpp:114) | **MISSING** → F-D-6 |
| `Error.mo` (`CErrorManager`/`IErrorBase`, Error.md:9) | `cr::Err` enum 0x5002xxxx-family + stable `err_text` (include/cr/cr_error.h:18-63) | **MATCH** (functionally; error-*object* bus unnecessary in-process — delta implied by README:39) |
| `Thread.mo` threading kit (mo-module-map.md:20) | `cr::cr_parallel` worker pool (include/cr/cr_parallel.h) | **MATCH** |
| `CallbackOperator.mo` + `CMessageProxy`/`CMessageSend` engine→UI bridge (mom-modules--ui-bridge §1) | CLI stdout progress → `IProgress<T>` events in Integration (ADR-1 consequence) | **DELIBERATE-DELTA** — ADR-1 |
| `CheckVersion.mo` Windows-install detection across partitions (mo-module-map.md:18) | absent | **MISSING** (minor, unlisted) |
| `RegLib.mo` offline registry-hive parser (mo-module-map.md:16) | absent — live-OS APIs only | note: covered by "BitLocker behavior subset" claim (README.md:26)? Not explicit — see UNVERIFIED |
| `DRWLoader` watchdog + `DRWReport` crash reporter (01:§3.1, DRW-exe artifact) | absent; wizard handles engine crashes via process-isolation exit codes (AUDIT-UI.md:47-49) | **MISSING** as a named delta (crash *reporting* unplanned; isolation substitute documented) |
| `DRWService.exe` FVE pipe service (DRWService artifact) | absent — BitLocker detect + legitimate unlock only (README.md:26; cr_bitlocker.cpp) | **DELIBERATE-DELTA** — documented subset |
| `UILogic.dll` UI logic layer (mom-module-system §2:89) | rda-wizard Core VMs/services | **MATCH** (in-flight, see §4) |
| `DiskHealth.exe` SMART companion | `rrecov health` + `HealthViewModel` (rda-wizard Core/ViewModels/HealthViewModel.cs:10) | **MATCH** |
| `FileFilter.xml` 8 categories / 1,489 entries (1,481 unique — CORRECTIONS #5) + `EUUserData` (config-data-reference; 05:106-108) | `ResultFilter` + hardcoded 7 categories / ~85 ext (rda-wizard ResultNode.cs:130-171) | **MISMATCH** (structure + coverage) → F-D-7 |
| `language/` 25 locales + 4 OEM + qtbase pairs, QM (Languages census §1) | `LocalizationService` JSON tables, en + zh-CN (LocalizationService.cs:28-33) | **DELIBERATE-DELTA** (EN-first documented UI-MAP.md:875) + **MISSING** RTL → F-D-9 |
| `Resource/` + `ResourceDark/` theme `.rcc` (og-codebase layout) | DESIGN-SYSTEM.md tokens; App resources not yet built | **UNVERIFIED** (App in-flight) |

---

## 2. Include-graph verification (claimed layering vs actual)

**Claim under test** (docs/architecture.md:6-44): app → orchestrator → engines → tree/session/writer → storage → win platform; `src/core` has "no OS headers" (architecture.md:43); `src/core/CMakeLists.txt:3` repeats "No OS headers allowed anywhere below this line".

**Commands + counts** (run at core_recovery HEAD):

```
rg -c '#include' src/            → 216 includes across 46 files
  src/core:  40 .cpp, 161 includes      (breakdown: cr/* + std only, + zlib.h/lz4.h/zstd.h)
  src/win:    5 .cpp,  32 includes      (windows.h/winioctl.h/ntddscsi.h here — allowed)
  src/app:    1 .cpp,  23 includes      (windows.h + full cr/ stack — allowed, top layer)
  include/cr: 45 headers, 174 includes  (std + cr/* only outside win/)

rg -n '#include\s*[<"](cr/win/|windows\.h|winioctl|ntddscsi|unistd|sys/|linux/|io\.h|process\.h|direct\.h)' src/core/
  → 0 matches                                  # engines/core: no win headers, no OS headers
rg -n 'cr/win/' include/cr/ -g '!**/win/**'    → 0 matches  # core headers never pull win/
rg -n 'cr/win/' src/core/                      → 0 matches
```

**Direction rules verified:**

| Rule | Result | Evidence |
|---|---|---|
| engines must NOT include `cr/win/*` | **PASS (0)** | rg above |
| core must not include OS headers | **PASS (0)** | rg above; full unique include list of src/core contains only `cr/*`, std, `zlib.h`, `lz4.h`, `zstd.h` |
| engines must not include orchestrator | **PASS (0)** | `rg 'cr/orchestrator/' src/core/` matches only orchestrator's own `cr_scan.cpp:13` |
| storage must not include engines/tree/session | **PASS (0)** | `rg 'cr/(ntfs|fat|…|orchestrator|tree|session|recovery)/' src/core/storage/` → none |
| tree/session must not include engines | **PASS (0)** | same pattern over `src/core/tree/`, `src/core/session/` → none |
| app sits on top (may include everything) | **PASS** | rrecov.cpp:20-42 includes orchestrator, session, table, win/*, windows.h |
| win layer includes only downward (core) | **PASS** | src/win pulls only `cr/cr_error.h`, `cr/table/cr_table_parse.h`, `cr/storage/cr_slice.h`, `cr/repair/cr_rawfixer.h` (cr_transport.cpp:17, cr_health.cpp:7, cr_bitlocker.cpp:7, cr_enumerate.cpp:9-10, cr_destio.h→recovery) — never engines/orchestrator |
| engines → storage (downward) | **PASS** | carve→`cr_device.h` (cr_registry.cpp:7, cr_analysers.cpp:10); lost→`cr_slice.h` (cr_candidates.cpp:10) |
| engines → tree (downward, result graph) | **PASS** | 13 engine/orchestrator headers include `tree/cr_node.h` (e.g. cr_mftwalk.h:24, cr_carve.h via include) |

**Lateral edges found (within the engines band, allowed by the diagram but worth recording):**
- `lost → ntfs/fat/exfat`: cr_candidates.cpp:7-13 (signature probes reuse boot validators)
- `repair → fat/exfat`: cr_rawfixer.cpp:15-17 (fixers reuse engine wire structs)
- `zfs → compress`, `safewriter → compress`, `session → table(crc32)`, `repair → table`

These mirror og practice (PartitionRecoveryD.mo and RawFixer.dll reuse NTFS/FAT libs — mo-module-map.md:25,32), so they are parity-consistent reuse, not violations. docs/architecture.md:17-18 lists `lost`/`repair` inside the engines band, so the diagram permits them; the doc does not call the reuse out explicitly (minor doc nit, folded into F-D-10's theme).

**Layering verdict: MATCH** — the documented layering is real in the include graph, in both `.cpp` and header sides, with zero violations found.

---

## 3. UI-side layering (rda-wizard)

**Claim** (AUDIT-UI.md ADR-1, :31-50): Integration drives `rrecov.exe` as a subprocess behind `IEngineHost`-shaped interfaces; UI never links C++ classes, never reimplements engine logic; ViewModels stay engine-agnostic. og comparison: DRWUI loads engines in-process via `LoadLibraryW` + MOM modules (05-scan-engine-architecture.md:31-33) — subprocess is the documented delta.

**Verified in code:**

| Check | Result | Evidence |
|---|---|---|
| No CLI strings/ProcessStartInfo above Integration | **PASS** | `rg 'ProcessStartInfo|Arguments|ExitCode|StandardOutput'` in `src/RdaWizard.Core/` → 0 code matches (only XML doc comments mention `rrecov` flags, e.g. ScanOptions.cs:33-39 — documentation, not invocation) |
| CLI argument construction confined to Integration | **PASS** | EngineServices.cs:23 (`["list"]`), :151-196 (`"scan"`, `--deep`, `--json`); SessionRecoveryServices.cs:48 (`["tree","--in",…,"--csv"]`), :160 (`"recover"`), :371 (`"health" --disk`), :425 (`"fix" --disk`) |
| No engine/filesystem logic in VMs | **PASS** | `rg 'magic|signature|MFT|boot sector|GPT|MBR'` over Core → only doc comments (ScanOptions.cs:44,50; DiskInfo.cs:21,35-36) |
| Core is pure (no WPF/platform refs) | **PASS** | Core csproj + source inspection; platform P/Invoke lives in Integration (DriverStatusDetector.cs:99-100 `CreateFileW`) |
| Driver probe does not bypass core transport | **PASS** | DriverStatusDetector.cs:12-13 "NEVER issues IOCTLs (transport is…)"; CreateFileW presence probe only (:43) |
| Engine-crash isolation (ADR-1 consequence) | **PASS by design** | subprocess boundary; AUDIT-UI.md:47-49 |

**App layer:** `src/RdaWizard.App` **does not exist yet** (only `RdaWizard.Core` + `RdaWizard.Integration` on disk). Worklog documents this as in-flight (rda-wizard worklog.md:80-81 "then App/Controls/Design + tests + CI"). Consequence: the WPF-side layering (Views/App resources/themes vs og `qtdrwui` + `Resource/*.rcc`) is **UNVERIFIED** at code level.

**UI-side verdict: MATCH for the shipped layers** (discipline is real, delta documented); App = MISSING-in-flight (tracked, see F-D-3/F-D-11 for the structural defects around it).

---

## 4. Build / repo structure

**CMake targets (core_recovery):** `cr_core` STATIC pure core (src/core/CMakeLists.txt:5), `cr_win` STATIC platform layer linking `cr::core` + cfgmgr32/Advapi32 (src/win/CMakeLists.txt:2-12), `rrecov` exe linking both (src/app/CMakeLists.txt:1-2), `cr_tests` GTest exe (tests/CMakeLists.txt:5), 12 libFuzzer targets via `fuzz/Makefile` (fuzz/Makefile:31-35). Strictness: `/W4 /WX /permissive-` MSVC, `-Wall -Wextra -Wpedantic -Werror -Wshadow` gcc (CMakeLists.txt:19-28). vcpkg manifest: zlib/gtest/lz4/zstd (vcpkg.json:4-8). Presets for linux-gcc (ASan), win debug/release (CMakePresets.json). **Internally consistent — MATCH.**

**CI matrix (core_recovery/.github/workflows/):** `ci.yml` = lint(clang-format) + linux(gcc ASan) + linux-tsan + windows(MSVC Debug+Release) + fuzz(smoke, zero-crash gate) + codeql; `benchmark.yml` (seeded VHD + RDA driver + recall/precision scoring); `comparison-ours.yml` + `comparison-easeus.yml` (split-leg head-to-head, same seed/methodology); `release.yml` (tag-driven, artifacts + hashes). This matches and exceeds the README claim (README.md:63-65). **MATCH** — except the Win7 posture gap (F-D-4) and the version-pin drift (F-D-12).

**DLL-per-engine delta:** og ships 9 `*SearchDLL.dll` (01-build-tree-intelligence.md:14; 05:31-37); ours is one static lib + exe. Documented deliberately at README.md:39. **DELIBERATE-DELTA (documented).**

**rda-driver posture (reference point):** Win7 gates are machine-enforced — `NTDDI_VERSION=0x06010000`, `_WIN32_WINNT=0x0601`, PE import allowlist, `/osversion:6.1` (rda-driver docs/win7-compat.md:22-37; tools/verify_pe.py per docs/testing.md:25; ground-truth-contracts.md:323-328). core_recovery has **no equivalent user-mode gate** despite claiming a Win7 SP1 test matrix (docs/testing.md:22) → F-D-4.

**Repo hygiene findings:** `fuzz/clang++` (broken symlink → `/home/z/.local/bin/clang++`, git-tracked via `git ls-files fuzz/`) breaks repo-wide search tooling — verified live: `rg`/Glob/LS over the repo root fail with `rg: fuzz/clang++: No such file or directory` → F-D-1. `temp_data.txt` (701 lines, stray copy of rawfixer content) tracked at root → F-D-8. `docs/UI-READINESS-PLAN.md` + `inc/cr_ui.h` referenced by the parity mandate (worklog/session-parity-prompt.md:64,81) but absent — acknowledged in worklog.md:582 ("do NOT exist yet at 2473c03e") → F-D-11. `AUDIT.md` (253 lines) at root vs `docs/` organization; rda-wizard `AUDIT-UI.md` also root; rda-driver keeps audits under `docs/audit/` → F-D-13. `worklog/` prompt files committed in core_recovery (session-05-prompt.md, session-parity-prompt.md) and rda-wizard (2 files) but no such dir in rda-driver (root `worklog.md` only) — convention inconsistency, folded into F-D-13.

---

## 5. Resource & localization organization

- **og:** `language/` 37 QM (25 product locales incl. Arabic=RTL, 4 Mainland OEM variants, 8 qtbase; Languages census §1); `Resource/` + `ResourceDark/` per-theme `.rcc` + `ImageResource.xml`; `FileFilter.xml` (8 categories/1,489 entries, 1,481 unique — CORRECTIONS #5; localized labels — 05:106-108); `EUUserData` MIME/magic; `Dict/`; `raw_signs.json` (139 ext, external).
- **ours (wizard):** `LocalizationService` — JSON key→string tables beside the exe, `en` + `zh-CN`, fallback current→English→key-as-text, malformed-format and missing-file degradation never crashes (LocalizationService.cs:54-67, 119-141). Original strings only (clean-room; header comment :6-8). Key discipline: dotted stable ids (`Home.Title`), `Format()` invariant culture. String table files themselves do not exist yet (App-owned, in-flight).
- **ours (core):** `err_text` stable English strings keyed by code (cr_error.h:62-63, cr_error.cpp:10+); error taxonomy reproduces the og 0x5002xxxx convention (cr_error.h:4-6 citing 05-scan-engine §4).

Structure parity where reasonable holds (externalized table + fallback ≈ QM + QTranslator + English boot seed, Languages census §3); deltas: 2 locales vs 25, no RTL, and the classification/filter data is compiled-in C# rather than a resource file (F-D-7, F-D-9).

---

## 6. Driver boundary (shape only)

- og: 7 kernel binaries / 4 roles, WDK `fre_wlh_amd64`, shared with EPM (02:9-19); user-mode opens via Device.mo (02:108-118).
- ours: one repo, one `rda.sys` merging EPMNtDrv + EUEDKEPM + EUDCPEPM roles — documented consolidation (rda-driver README.md:1-6,16-19). core_recovery consumes it exclusively through the legacy PID-token grammar path (open_rda_partition / open_rda_volume — N-041 deleted the structured-open `RdaClient` in session-12: zero callers since introduction, and this row's earlier "consumes it exclusively through RdaClient / structured OPEN_TARGET preferred" claim was refuted by grep at audit time). The F11 `Information=Length×512` unscale (:54-56) rides the grammar path; the copy-session protocol stays documented wire-contract surface only. The contract is *restated* from `rda-driver/src/shared/inc/rda_ioctl.h` (cr_rda.h:4-5) rather than vendored — values cross-check (0x6002440/0x910, 0x6002444/0x911 on both sides; rda_ioctl.h:74-75 — the wire-truth fix, commit 3bda8e2, corrected the phantom-zero 0x6000244x literals), and wire structs are size/offset-frozen with `static_assert` (cr_rda.h:73,93,154-159,174-178,196-200). Direct-handle fallback preserves the og Device.mo duality.
- Version coupling: benchmark.yml fetches the `v0.2.1` release assets and pins their SHA-256 (benchmark.yml:51-64); core CI does not otherwise gate on a driver version (acceptable — capability query at runtime). Comment drift v0.2.0 vs v0.2.1 → F-D-12.

**Boundary verdict: MATCH** (separate repo, single merged driver as documented delta, clean client seam, contract frozen on both sides).

---

## 7. Audit grid

| Axis | Verdict | Key evidence |
|---|---|---|
| Core layering (pure core / platform split) | **MATCH** | §2 include graph: 0 OS/win includes in src/core + include/cr (non-win) |
| Dependency direction (app→orch→engines→storage) | **MATCH** | §2 direction rules, all PASS |
| Abstraction seams (IStorageDevice, CancelToken, IWriteDevice) | **MATCH** | cr_device.h:86-100; cr_transport.h:47 (repair::IWriteDevice) |
| Module bus / plugin seam | **DELIBERATE-DELTA** (documented README.md:39) | MOM loading contract (mom-module-system §1.1) vs compile-time linking |
| Logging seam | **MISSING** | F-D-6 |
| Error model | **MATCH** | cr_error.h:18-63 ↔ 05:61-62 (0x5002xxxx family) |
| Engine dispatch seam | **MATCH** (shape differs by design) | cr_scan.cpp:85-131,268+ ↔ 05:38-43 |
| UI layer discipline (Core/Integration) | **MATCH** | §3 checks all PASS |
| UI App layer | **MISSING (in-flight)** | §3; worklog.md:80-81 |
| UI↔engine transport | **DELIBERATE-DELTA** (ADR-1 documented) | AUDIT-UI.md:31-50 |
| Build target shape (CMake/Make) | **MATCH** | §4 |
| DLL-per-engine layout | **DELIBERATE-DELTA** (documented) | README.md:39 ↔ 01:14 |
| CI matrix (core) | **MATCH** | ci.yml:12-145 |
| CI (wizard) | **MISSING** | no `.github/` in rda-wizard → F-D-2 |
| Win7-inclusive posture (user mode) | **MISMATCH** | F-D-4 |
| Repo hygiene | **MISMATCH** | F-D-1, F-D-8, F-D-11, F-D-13 |
| Resource organization (filter/classification data) | **MISMATCH** | F-D-7 |
| Localization structure | **DELIBERATE-DELTA + MISSING (RTL)** | F-D-9 |
| Driver boundary shape | **MATCH** | §6 |
| Session/persistence placement | **MATCH** (own-format delta documented) | architecture.md:63-64 |
| Partition-table editing path | **MISSING** | F-D-5 |

---

## 8. Findings

### F-D-1 — P1 — hygiene/build — `fuzz/clang++` broken symlink is git-tracked and breaks repo tooling
- **Ours:** `core_recovery/fuzz/clang++` → `/home/z/.local/bin/clang++` (nonexistent); tracked (`git ls-files fuzz/` lists it; not in .gitignore).
- **Impact (verified live during this audit):** any `rg`/Glob/LS over the repo root fails (`rg: /…/fuzz/clang++: No such file or directory`) — this broke the auditor's own tooling and will break any script that walks the tree. It also leaks a developer-machine absolute path.
- **Suggested fix:** `git rm fuzz/clang++` (the Makefile already takes `CXX=clang++` from CI, ci.yml:112 — the symlink is not used by CI); add `fuzz/clang++` to .gitignore if it's a local convenience.

### F-D-2 — P2 — CI posture — rda-wizard has no CI at all; README claims a full matrix
- **Ours:** no `.github/` directory exists in rda-wizard (verified: `ls .github/` fails; `git ls-files` shows no workflow files). README.md:43 lists ".github/workflows/ — ci.yml, ui-smoke.yml, release.yml" and README.md:57-58 claims "CI runs the full matrix (build, unit, contract, FlaUI smoke, artifact publish)".
- **Corpus contrast:** the RE corpus itself runs CI (00-INDEX.md:99 `.github/workflows/ — ghidra-decompile.yml, multitool-static.yml`); our other two repos run full matrices.
- **Mitigation:** worklog.md:80-81 documents CI as the next in-flight step, and the Linux-box/Windows-runner division of labor is recorded (worklog.md:12-18). Doc-vs-reality mismatch remains until workflows land.
- **Suggested fix:** land the three workflows (or trim README) with the next wizard commit; the promised FlaUI smoke suite needs Windows runners per worklog.

### F-D-3 — P2 — build structure — `RdaWizard.sln` references 4 nonexistent projects
- **Ours:** RdaWizard.sln:9-19 declares `RdaWizard.Controls`, `RdaWizard.Design`, `RdaWizard.App`, `RdaWizard.Ui.Tests`; none of the four `.csproj` files exist on disk (`find . -name "*.csproj"` → only Core, Integration, 2 test projects). `dotnet build RdaWizard.sln` (README.md:53) fails on missing project files. README.md:32-37 presents the full 5-project layout as if present.
- **Suggested fix:** either commit scaffold `.csproj`s for the four projects (matching ADR-11 pins) or trim the solution until they land; keep README's layout table marked "planned".

### F-D-4 — P2 — CI posture / Win7 — user-mode stack claims Win7 in the test matrix but enforces no Win7 gate
- **Ours:** docs/testing.md:22 claims matrix "Win7 SP1 / Win10 / Win11 / WinPE". There is no `_WIN32_WINNT`/`WINVER`/`NTDDI` pin anywhere in core_recovery source or CMake (`rg` over src/ and .github → 0); the Windows CI job only *prints* PE imports for diagnostics (ci.yml:72-95, "Diagnostics — deployed DLLs + PE imports", `if: always()`, no assertions).
- **Contrast (ours, driver):** rda-driver machine-verifies the Win7 contract on every build — header pins `NTDDI_VERSION=0x06010000`/`_WIN32_WINNT=0x0601` (docs/win7-compat.md:23-24), PE import allowlist gate (docs/win7-compat.md:30-37; tools/verify_pe.py, docs/testing.md:25). The two repos therefore diverge in posture while the product claims one Win7→Win11 envelope.
- **Note:** a spot-check found no obvious Win8+-only user-mode APIs in `src/win` (rg for `GetDiskSpaceInformation|SetFileInformationByHandle|PathCch|…` → 0), so this is a *verification* gap, not (yet) a known API violation.
- **Suggested fix:** mirror the driver gate in user mode: pin `_WIN32_WINNT=0x0601` in the Windows presets and add a dumpbin-driven import-allowlist step to ci.yml; or explicitly re-scope docs/testing.md to Win10+ and document that as a delta.

### F-D-5 — P2 — module boundaries / scope — partition-table WRITE path absent and undeclared
- **Ours:** lost-partition discovery produces virtual candidates only (docs/architecture.md:28-30 "PartitionSlice - engines CANNOT read outside their candidate partition"; cr::lost). No code writes MBR/GPT entries; `rg` over src/ finds no partition-table writer; roadmap v0.3 candidates (docs/roadmap.md:64-70) list UI/repair/WinPE/wipe/vm-mount — no table editing; the deferred list (roadmap.md:57-59) covers only remote features; README's out-of-scope (README.md:28-32) likewise only remote.
- **og evidence:** found partitions can be *written back* — `MainModule.dll` operation engine `CBaseOperation`→…/`CGPTToMBROperation` etc. (mom-module-system--architecture.md:77-84), "rebuilds partition records → user can recover found partitions (writes new table via MainModule.dll ops!)" (§3:96); Device.mo carries `CDiskDriveLayoutReadWriter` ("raw layout R/W!", mo-module-map.md:24).
- **Suggested fix:** decide and document: either add a `cr::table` write path (transactional, RawFixer-style S1-S6 safety) to the roadmap, or record "no partition-table editing (scan-in-place only)" in the deferred/delta list. Today the delta is silent.

### F-D-6 — P2 — abstraction seams — no logging seam (og Log.mo / Logger.dll / EPMLOG)
- **Ours:** no logging facility in core — `rg -l 'log'` over src/core+include/cr finds only domain uses (rawfixer undo *log*, health NVMe *log* page); the only general logging is the CLI's `--log` `vfprintf` (src/app/rrecov.cpp:114, :404). Nothing in README/docs/roadmap acknowledges the absence.
- **og evidence:** dedicated infra modules — `Log.mo` (`IEsLog`, `CFileMappingLog`, `\\EPMLOG.log`, Log.md:12-15,23), `Logger.dll` + `PubLog.dll` in the install tree (01-build-tree-intelligence.md:48), DRWLoader's 2 MB shared crash log (00-INDEX.md DRW-exe row); the MOM dependency matrix shows Log.mo = 15+ importers (00-INDEX.md:53).
- **Impact:** engine-side diagnostics (retry exhaustion, fixer refusals, cache stats) currently surface only through CLI stdout/exit codes; the wizard consumes progress lines but has no engine log channel to correlate.
- **Suggested fix:** either document "no engine logging module (CLI --log is the seam)" as a delta, or add a minimal `cr::log` sink interface injected at the orchestrator boundary (keeps core pure — interface in include/cr, implementation in app/win).

### F-D-7 — P2 — resource organization — classification/filter data hardcoded and much smaller than og's external resources
- **Ours:** `FileCategory` = 7 categories (Photos/Videos/Documents/Audio/Archives/Email/Other) with ~85 extensions in hardcoded C# HashSets (rda-wizard src/RdaWizard.Core/Models/ResultNode.cs:130-171). Core's carve signature DB is likewise compiled-in (cr_rawsigns_data.cpp; 30 formats per roadmap v0.2).
- **og evidence:** `FileFilter.xml` — 8 top categories (Graphics 213, Video 76, Audio 62, Document 302, Email 25, Web&Others 12, Archive 59, Folders) = 1,489 entries (1,481 unique after CORRECTIONS #5's dedup — 8 duplicated extension rows), with per-category localized labels/descriptions (05-scan-engine-architecture.md:106-108; config-data-reference; og-codebase/FileFilter.xml verified present, 54,597 bytes); plus `EUUserData` MIME/magic typing; plus external `raw_signs.json` (139 ext, 05:90-92).
- **Doc status:** UI-MAP.md:592 records the og surface ("8 categories / 1,489 extensions") but AUDIT-UI's ADR/G lists do not flag the implementation gap; ResultNode.cs:129 claims "og parity: the category taxonomy" while the taxonomy differs (no Folders/Web&Others; adds Other).
- **Suggested fix:** externalize the category/extension map to a resource file (JSON/XML beside the app, localizable labels — mirrors FileFilter.xml's role), align taxonomy (add Folders + Web-ish bucket or document the merge), and track extension-coverage parity as a measurable number (85/1,481 unique today).

### F-D-8 — P3 — hygiene — `temp_data.txt` committed at repo root
- **Ours:** core_recovery/temp_data.txt, 701 lines, git-tracked; content is a stale copy of rawfixer-related C++ (header comment block matching include/cr/repair/cr_rawfixer.h).
- **Suggested fix:** delete from git; add to .gitignore.

### F-D-9 — P3 — localization — 2 locales, no RTL; EN-first documented, the cap and RTL are silent
- **Ours:** `LocalizationService` supports exactly `en` + `zh-CN` (LocalizationService.cs:107-117 — anything not `zh*` resolves to `en`); no RTL/FlowDirection concept anywhere (App absent). Fallback chain current→English→key and never-crash degradation are good structural parity with og's QM+English-seed model (Languages census §3).
- **og evidence:** 25 product locales incl. Arabic (RTL) + 4 OEM variants + qtbase pairs (Languages census §1); UI claim "supports 20+ languages".
- **Doc status:** UI-MAP.md:875-876 documents "ships EN first; keep the string-table discipline" — the EN-first part is a documented delta; the 2-locale cap and RTL absence are not called out.
- **Suggested fix:** record "v1 ships en+zh-CN; RTL and further locales deferred" in AUDIT-UI/README so the delta is deliberate rather than silent; keep keys stable for later tables.

### F-D-10 — P3 — docs — README names a factory mechanism that doesn't exist
- **Ours:** README.md:39 "in-process registry (crCreate* factories)" — `rg 'crCreate'` over src/include → 0 matches; actual dispatch is a `FsFamily` switch (cr_scan.cpp:268+). Also minor: docs/architecture.md:17-18 doesn't mention the intra-band reuse edges (lost→ntfs/fat/exfat, repair→fat/exfat) that the include graph shows (§2).
- **Suggested fix:** reword README ("in-process FsFamily dispatch"); add one sentence to architecture.md noting intra-engine-band reuse is allowed (lost/repair reuse boot validators/wire structs — mirroring og's PartitionRecoveryD/RawFixer reuse of NTFS/FAT libs, mo-module-map.md:25,32).

### F-D-11 — P3 — docs/hygiene — dangling references to in-flight artifacts
- **Ours:** core_recovery worklog/session-parity-prompt.md:64,81 references `docs/UI-READINESS-PLAN.md` and `inc/cr_ui.h` — neither exists (acknowledged at worklog.md:582). rda-wizard README.md:71-73 references `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/ROADMAP.md` — none exist (docs/ contains CORE-CONTRACT, DESIGN-SYSTEM, TRANSPORT-CONTRACT, UI-MAP only).
- **Suggested fix:** mark the README table entries "planned" or land stub docs; keep the UI-readiness items tracked in issues so the references resolve.

### F-D-12 — P3 — version pins — benchmark workflow comment says v0.2.0, fetches v0.2.1
- **Ours:** benchmark.yml:6 ("fetch the RDA driver from the rda-driver v0.2.0 release") and step name :51 vs the actual fetch of `tags/v0.2.1` (:55).
- **Suggested fix:** update the comment/step name; consider moving the pin to a workflow input or env var so it is single-sourced.

### F-D-13 — P3 — repo organization — cross-repo convention drift (audits, worklog dirs)
- **Ours:** core_recovery keeps `AUDIT.md` (253 lines, session-5 module audit) at root alongside `docs/` (9 docs); rda-wizard keeps `AUDIT-UI.md` at root; rda-driver keeps audits under `docs/audit/` (e.g. docs/audit/2026-10-22-security-review.md) with only `worklog.md` at root. `worklog/` prompt directories are committed in core_recovery (2 files) and rda-wizard (2 files) but not in rda-driver. Committed prompts are redacted (worklog.md:107 "token redacted") — acceptable as an audit-trail convention, but it should be uniform.
- **Suggested fix:** adopt one convention org-wide (recommend rda-driver's: `docs/audit/` + root worklog.md; prompts under `worklog/` if kept at all) and move the root AUDIT files.

---

## 9. UNVERIFIED (silent — could not be checked from citations alone)

1. **rda-wizard App/Controls/Design layering vs og DRWUI view-manager/QSS stack** — the projects don't exist yet; only the Core/Integration boundary was auditable. Re-audit when App lands (og reference: 05-uiux-architecture.md, mom-modules--ui-bridge §1).
2. **Theme/resource organization parity** (og `Resource/`+`ResourceDark/` `.rcc` + `ImageResource.xml` vs our DESIGN-SYSTEM tokens) — no App resources on disk to compare; DESIGN-SYSTEM.md exists but token→resource-file mapping is unimplemented.
3. **`strings/en.json` / `zh-CN.json` content and key coverage** — files are App-owned and not yet in the repo; only the loader was auditable.
4. **Actual Win7 API-level compatibility of `src/win` + `rrecov.exe`** — requires a Windows build with import analysis (dumpbin) or a Win7 VM; only a spot-check for known Win8+ APIs was possible here (negative, see F-D-4).
5. **`RegLib.mo` offline-hive functionality coverage** — og parses offline SYSTEM/SOFTWARE hives for FVE keys/OS detection (mo-module-map.md:16); our cr_bitlocker.cpp live-API behavior was not compared feature-by-feature (owned by the BitLocker/transport agent).
6. **CI green status at HEAD** — no runners were executed by this audit; matrix *structure* was verified from workflow files only.

---

## 10. Deliberate-delta candidates (documented; recommend a single deltas registry)

Already documented today (scattered): in-process engines instead of DLL-per-engine + MOM bus (README.md:39); CLI-subprocess UI integration instead of in-proc MOM bridge (AUDIT-UI.md ADR-1, :31-50); CRSF instead of .rsf (architecture.md:63-64); one merged rda.sys instead of 7 driver binaries (rda-driver README); remote features out of scope (README.md:28-32); BitLocker behavior subset (README.md:26); EN-first localization (UI-MAP.md:875).
**Silent today — should be either documented or scheduled:** partition-table editing path (F-D-5); engine logging seam (F-D-6); locale cap + RTL (F-D-9); launcher/crash-reporter watchdog (module map row); 2-locale + classification-resource externalization (F-D-7/F-D-9). Recommend consolidating all of the above into the deltas registry planned for `docs/PARITY-AUDIT.md` (worklog.md:106-107) so "deliberate" is provable.
