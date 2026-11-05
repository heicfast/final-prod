# Build surface — post-remediation coherence session (HEAD 3b203fe1, 2026-10-30)

Task T2-A re-verification record. Scope: every CMake file, preset, toolchain,
workflow, warning/analyzer/sanitizer setting, fuzz harness, local gate, and the
Win7/import gate — re-read exhaustively at HEAD `3b203fe1` (branch `main`,
verified `git rev-parse --short HEAD`; remote `github.com/heicfast/core_recovery`;
working tree clean except this session's worklog files). This record supersedes
the Session-84 baseline (`e8dcaba8`) and the Session-90 addendum (both
summarized in Lineage below); every
claim carries file:line against the CURRENT tree. No tracked file was modified;
this sheet is the only output.

CMake file census (exactly 7 exist): `CMakeLists.txt`, `src/CMakeLists.txt`,
`src/core/CMakeLists.txt`, `src/ui/CMakeLists.txt`, `src/win/CMakeLists.txt`,
`src/app/CMakeLists.txt`, `tests/CMakeLists.txt`. **No** CMakeLists under
fuzz/ (Makefile-driven), bench/, tools/, scripts/, installer/ (PowerShell+Inno),
third_party/ (header-only vendored stb). `cmake/` contains exactly one helper:
`DeployVcpkgBinaries.cmake` (45 lines).

## Targets

| Target | Type | Dir | Sources | Key options / links |
|---|---|---|---|---|
| `cr_core` | STATIC lib | src/core | 83 .cpp (src/core/CMakeLists.txt:5-89) | PIC ON (:94); PUBLIC include/ (:95-96) + `ZLIB::ZLIB` (:104) + LZ4/ZSTD via `PkgConfig::LZ4`/`PkgConfig::ZSTD` with `CR_HAVE_LZ4=1`/`CR_HAVE_ZSTD=1` (:105-112); PRIVATE third_party/stb include (:99-100) + `CR_ENGINE_VERSION="${PROJECT_VERSION}"` (:102-103); ASan `-fsanitize=address` when `CR_ENABLE_ASAN AND NOT MSVC` (:114-117) |
| `cr_win` | STATIC lib | src/win | 11 .cpp incl. cr_fsck.cpp (src/win/CMakeLists.txt:2-13) | Windows-only; PUBLIC `cr::core` (:16) + `cfgmgr32 Advapi32 Shlwapi` (:18 — Shlwapi for PathIsNetworkPathW, the M-074/N-013 landing) |
| `rrecov` | exe | src/app | rrecov.cpp (src/app/CMakeLists.txt:1) | Windows-only; links `cr::core cr::win` (:2); `cr_deploy_vcpkg_binaries` (:5) |
| `cr_ui` | SHARED lib | src/ui | cr_ui.cpp (src/ui/CMakeLists.txt:11) | `CR_UI_BUILD` (:14); links `cr::core` (:17); Win32: `Psapi` (:20-22) + `cr::win` (:23-25); hidden visibility + PIC (:35-38); MSVC-only `cr_ui_exports.def` (:32-34); ASan (:40-43); DLL deploy (:46) |
| `cr_tests` | exe (GTest) | tests/unit | 68 TUs = main.cpp + 67 test files (tests/CMakeLists.txt:5-74) | links `cr::core GTest::gtest_main` (:76); POST_BUILD fixture copy of data/images, data/exfat, data/thumbnails (:80-91); `CR_SOURCE_DIR` define (:96-97); DLL deploy (:101); `gtest_discover_tests` (:207) |
| `cr_destio_tests` | exe (GTest, WIN32-only) | tests/unit/test_destio_win.cpp | tests/CMakeLists.txt:108-116 | links `cr::win` + CR_SOURCE_DIR + deploy (:109-115); `add_test destio` (:116) |
| `cr_bitlocker_tests` | exe (GTest, WIN32-only) | tests/unit/test_bitlocker_win.cpp | tests/CMakeLists.txt:120-123 | links `cr::win`; `add_test bitlocker` (:123) |
| `cr_fsck_win_tests` | exe (GTest, WIN32-only) | tests/unit/test_fsck_win.cpp | tests/CMakeLists.txt:127-130 | the M-075 chkdsk seam battery; links `cr::win`; `add_test fsck_win` (:130) |
| `cr_image_create_tests` | exe (GTest, WIN32-only) | tests/unit/test_image_create_win.cpp | tests/CMakeLists.txt:136-142 | the S-040 flat-raw producer contract; links `cr::win` + CR_SOURCE_DIR; `add_test image_create` (:142) |
| `cr_ui_tests` | exe (GTest) | tests/unit/test_cr_ui.cpp | tests/CMakeLists.txt:160-161 | links `cr::ui` — tests the shim as a host consumes it; Win32 additionally links static `cr::core` for direct core-API fixtures (:163-173); `CR_SOURCE_DIR` (:175-176); `gtest_discover_tests` (:208) |
| `cr_ui_abi_check` | exe (plain C) | tests/ui_abi_check.c | tests/CMakeLists.txt:178-179 | links `cr::ui`; `add_test cr_ui_abi_check <binary-dir>/cr_ui_abi_check.img` (:191-192); Win32 copies cr_ui.dll beside it (:182-189) |
| `win_syntax_check` | ctest test (not a target) | tools/win_syntax_check.sh | tests/CMakeLists.txt:223-227 | Linux **gcc-only** (`NOT WIN32 AND NOT Clang`, :223); g++ `-fsyntax-only -std=c++20 -fshort-wchar -D_WIN32 -D__declspec(x)=` (tools/win_syntax_check.sh:55) over src/win/*.cpp (:58) + src/app/*.cpp + src/ui/*.cpp (:76) + tests/unit/test_*_win.cpp (:123, the N-228 extension; GTest include + CR_SOURCE_DIR at :122) against the declaration-only stubs tests/win-stub/{windows,psapi,ntddscsi,share,winioctl,cfgmgr32,shlwapi,direct,io,stdio,string,stdlib,process,sys/stat}.h |
| `test_name_uniqueness` | ctest test (not a target) | scripts/verify/verify_test_names.py | tests/CMakeLists.txt:235-238 | NEW (N-231, the M-120/N-230 duplicate-gtest-name build-red class): full-tree duplicate TEST()/TEST_F()/TEST_P() lint, `find_package(Python3 REQUIRED)` (:235), LABELS lint (:238); runs on BOTH lanes (no platform gate) |

Global project facts: `cmake_minimum_required(3.25)` (CMakeLists.txt:1);
project `core_recovery` **VERSION 0.6.1** (CMakeLists.txt:3-6 — the f621b37f
release-truth stamp), C CXX; C++20 Required, extensions OFF; C11
(CMakeLists.txt:13-17). Options: `CR_ENABLE_ASAN` OFF, `CR_BUILD_TOOLS` ON,
`CR_BUILD_APP` ON, `CR_BUILD_UI` ON, `CR_ENABLE_TESTS` ON (CMakeLists.txt:38-42).
`CR_BUILD_TOOLS` remains a dead option (declared, consumed by nothing).
Hard-fail dependency posture: zlib REQUIRED (CMakeLists.txt:48-53); lz4+zstd
REQUIRED with the 3-tier CONFIG→pkg-config→find_library discovery and the
`PkgConfig::LZ4`/`PkgConfig::ZSTD` aliasing (CMakeLists.txt:78-136), FB-X01
fatal without both (CMakeLists.txt:141-147). GTest: `find_package(GTest)` else
FetchContent googletest v1.15.2 tarball, still hash-unpinned
(CMakeLists.txt:172-178). `cr_deploy_vcpkg_binaries()` POST_BUILD hook copies
every config-matching DLL from `vcpkg_installed` next to the target
(CMakeLists.txt:156-167; cmake/DeployVcpkgBinaries.cmake:15-45 — Debug copies
release+debug trees both, :28-38). `src/CMakeLists.txt` dispatches core, ui
(gated CR_BUILD_UI), then win+app on WIN32 (src/CMakeLists.txt:4-15).

## Presets & toolchain

`CMakePresets.json` (version 6; cmakeMinimumRequired 3.25.0):

| Preset | Generator | Cache vars / purpose |
|---|---|---|
| `base` (hidden) | — | binaryDir `${sourceDir}/build/${presetName}` (:10-13) |
| `linux-gcc` | Unix Makefiles | RelWithDebInfo, `CR_BUILD_APP=OFF`, `CR_ENABLE_ASAN=ON` (:14-23) — the documented "CI parity ASan lane" but run by NO CI workflow and NOT by ci-local.sh (see Findings §6) |
| `linux-release` | Unix Makefiles | Release (:24-31) — the ci-local.sh default build (ci-local.sh:99-100) |
| `win-x64-debug` | VS 17 2022 x64 | toolchainFile `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`, Debug (:32-41) — ORPHAN: no workflow, script, or test preset consumes it |
| `win-x64-release` | VS 17 2022 x64 | vcpkg toolchain, Release (:42-51) — the workhorse: checks.yml:214/216, installer.yml:97/99, release.yml:168/170, ci-local.ps1:71/73 |
| `linux-ubsan` | Unix Makefiles | Debug, `CR_BUILD_APP=OFF`, ASAN OFF, `-fsanitize=undefined -fno-sanitize-recover=undefined -g -fPIE` (+ `-DCR_SANITIZER_LANE` on CXX) on C+CXX, `-pie` exe linker (:52-65) |

buildPresets for all 5 (:67-90). testPresets: `linux-gcc`, `win-x64-release`,
`linux-ubsan` only (:91-113) — no `linux-release` test preset (ci-local.sh runs
ctest directly inside build/linux-release, ci-local.sh:109). Toolchain files:
the ONLY toolchain reference is vcpkg's stock
`scripts/buildsystems/vcpkg.cmake` via the two win presets. `vcpkg.json`: name
core-recovery, **version-string 0.6.1** (f621b37f), deps zlib/gtest/lz4/zstd
(vcpkg.json:2-9) — manifest mode is the pinning policy, asserted at
checks.yml:120-121 and ci-local.sh:140-144 / ci-local.ps1:95-96.

## Warning policy per compiler

Global (identical-strictness doctrine, CMakeLists.txt:8-36):

- **MSVC** (every target, every config): `/W4 /WX /permissive- /Zc:preprocessor
  /utf-8 /external:W0 /wd28251` (CMakeLists.txt:20-21) + definitions
  `_CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0601 WINVER=0x0601
  NTDDI_VERSION=0x06010000` (CMakeLists.txt:29-30 — the GAP-W6-WIN7/F-D-4
  compile-time posture; rationale :22-28). `/wd28251` is the ONLY `/wd` in any
  CMake file; no warning pragmas exist in CMake. No per-target deviations.
- **GCC/Clang (CMake)**: `-Wall -Wextra -Wpedantic -Werror -Wshadow`
  (CMakeLists.txt:32); GNU-only addition `-Wno-missing-field-initializers`
  (CMakeLists.txt:33-35).
- **Clang (fuzz Makefile lane)**: `-Wall -Wextra -Werror` only — **no
  `-Wshadow`, no `-Wpedantic`** (fuzz/Makefile:28-30).
- **CI one-off compile**: the test-benchmark API harness builds with
  `cl.exe /nologo /W4 /WX /utf-8 /D_CRT_SECURE_NO_WARNINGS` against the
  release SDK (test-benchmark.yml:244).

### #pragma suppression census (complete)

Exactly ONE site exists in src/ include/ tests/ third_party/ (searched
`#pragma warning`, `pragma GCC diagnostic`, `pragma clang diagnostic` — all
negative except this):

- `src/core/preview/cr_thumbnail.cpp:33-56` — wraps ONLY the vendored
  `#include "stb_image.h"` (with `STBI_NO_STDIO=1`, `STBI_MAX_DIMENSIONS
  32768`, `STB_IMAGE_IMPLEMENTATION`, :47-51). MSVC side: `#pragma
  warning(push)` + `disable : 4100 4189 4244 4245 4267 4456 4457 4701 4702
  4706` (:34-35), `pop` (:53). GCC side: `#pragma GCC diagnostic push` (:37),
  `ignored "-Wmaybe-uninitialized"` under `__GNUC__ && !__clang__` only
  (:42-44), `ignored "-Wunused-but-set-variable"` (:45), `"-Wshadow"` (:46),
  `"-Wpedantic"` (:47), `pop` (:55). Registered as **S-1** in
  docs/BUILD-SUPPRESSIONS.md (the S-1 row; the file now correctly headlines
  "exactly ONE registered entry" — the old "ZERO entries" self-contradiction
  was corrected in Session-84). First-party code in the same TU is outside
  the push/pop. `third_party/stb/stb_image.h` itself contains no pragmas; it
  is excluded from clang-format by the pre-commit filter
  (.githooks/pre-commit:24) and by CI/local format scopes (src/include/tests
  only — checks.yml:93, ci-local.sh:39, ci-local.ps1:36).

Zero first-party suppressions otherwise; /WX + -Werror are global and every
preset build is the Werror build. The MSVC lane is proving this live: the
racing-window telemetry residue that broke cr_tests at /W4 /WX (run
40965919891, commit 624db00d's message) was repaired before HEAD.

## Analyzers & sanitizers

- **clang-format**: root `.clang-format` (:1-13): `BasedOnStyle: LLVM`,
  Language Cpp, Standard c++20, IndentWidth 4, ColumnLimit 100,
  AccessModifierOffset -4, SortIncludes Never, IncludeBlocks Preserve,
  AllowShortFunctionsOnASingleLine Inline, AllowShortIfStatementsOnASingleLine
  Never, PointerAlignment Left, DerivePointerAlignment false. Version PINNED
  to `clang-format==18.1.8` in CI (checks.yml:88-102, pin at :91 — "the pin
  and the tree move TOGETHER", :81-87, the run-40692407577 drift record) and
  in ci-local.ps1:33 (auto-pip-install of the pin). ci-local.sh does NOT pin
  (presence check only, ci-local.sh:38-50) — see Findings §13.
- **clang-tidy**: NO config anywhere (absence — itself a finding to record).
  Acknowledged honestly at ci-local.sh:93-95, ci-local.ps1:59, docs/CI-LOCAL.md:42-47.
- **cppcheck**: none (same acknowledgment). **CodeQL**: none in `.github/`.
  `.editorconfig`: does not exist. `cppcheck.cfg`/`.clang-tidy`: do not exist.
- **Sanitizer lanes**: ASan via `CR_ENABLE_ASAN` → `-fsanitize=address` on
  cr_core (src/core/CMakeLists.txt:114-117), cr_ui (src/ui/CMakeLists.txt:40-43),
  cr_tests + cr_ui_tests + cr_ui_abi_check (tests/CMakeLists.txt:194-204);
  `linux-gcc` preset turns it on. UBSan via `linux-ubsan` preset. TSan via a
  manual cmake invocation in ci-local.sh:196-203 (build/tsan, gcc,
  `-DCR_SANITIZER_LANE`). Fuzz lane:
  `-fsanitize=fuzzer,address,undefined` (fuzz/Makefile:28). **No CI workflow
  runs any sanitizer lane** (windows-only consolidation, checks.yml:3-8). No
  `continue-on-error` anywhere in any workflow.

## Fuzz harness

- 16 libFuzzer targets built by `fuzz/Makefile` (TARGETS, fuzz/Makefile:39-44):
  fuzz_pure, fuzz_engines, fuzz_carve, fuzz_container, fuzz_ntfs, fuzz_fat,
  fuzz_exfat, fuzz_ext4, fuzz_hfsplus, fuzz_btrfs, fuzz_zfs, fuzz_refs,
  fuzz_preview, fuzz_reghive, fuzz_image, fuzz_thumbnail (16 harness .cpp +
  fuzz_parser_common.h). The whole core compiles into `build/libcrfuzz.a` from
  ALL ../src/core/*.cpp (fuzz/Makefile:34-37).
- Build: `make CXX=clang++`; CXXFLAGS `-std=c++20 -g -O1
  -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -I../include
  -I../third_party/stb -Wall -Wextra -Werror` + optional CR_HAVE_LZ4/ZSTD via
  pkg-config (fuzz/Makefile:19-31).
- Smoke: `make run` — 45 s per harness (20 s for the per-FS ones),
  `-max_len` 64 KiB–2 MiB, `-timeout=25 -rss_limit_mb=2560`
  (fuzz/Makefile:72-87). `make run-long` — 60–120 s per harness (:89-103).
- Corpus: **1467 committed seed files** under fuzz/corpus/ (git ls-files),
  heavily grown since the S84 record (was 5 seeds in 2 dirs): pure 255,
  engines 85, carve 176, ntfs 29, preview 402, thumbnail 485, image 5, refs 5,
  ext4 4, hfsplus 4, fat 9, btrfs 2, container 2, exfat 2, zfs 2 — all 15
  corpus dirs now carry seeds (seeds/gen scripts live in scripts/, e.g.
  gen_zfs_fuzz_seeds.py).
- Where run: LOCAL ONLY — `./ci-local.sh --fuzz` (ci-local.sh:210-218, make
  all + run with clang++). No CI lane (checks.yml:3-8). Known Makefile drifts
  preserved in Findings §4.

## CI workflows

### checks.yml — "Checks" (the pre-merge gate; MUST BE GREEN)
- Triggers: push+PR to main with symmetric `paths-ignore: ['**.md',
  'docs/**', 'worklog.md', 'worklog/**', 'AUDIT*.md']` (checks.yml:43-49);
  `workflow_call: {}` so release.yml can call it (:50). Permissions
  `contents: read` (:52-53). Concurrency `checks-<event>-<ref>`
  cancel-in-progress (event-scoped so a release's nested validate is never
  cancelled, :55-64).
- **Job `lint`** (windows-2022, timeout 15 m, :67-70): actions/checkout@v4
  (:72); pip wheel cache via actions/cache@v4 key `pip-clangformat-win2022`
  (:74-79); [1/2] clang-format 18.1.8 `--dry-run -Werror` over src/include/tests
  (:88-102); [2/2] N-041 dead-surface lint (RdaClient::/volume_letter_to_disk
  over src,include,tests,tools,bench,fuzz) + driver-only architecture lint
  (rda_available/win_pe_environment/open_harddisk_volume/prefer_rda over
  src,include) + vcpkg.json presence (:105-123); PowerShell AST parse gate
  over ci-local.ps1 + .github/scripts/*.ps1 + bench/*.ps1 (:125-148); YAML
  parse gate via PyYAML over all workflows (:150-162); M-088 CI-shape
  doc-truth gate `python tools/doc_truth_gate.py` (:164-174).
- **Job `build-and-test`** (windows-2022, timeout 90 m, needs lint,
  :176-180): checkout (:182); git `core.longpaths true` (:187-189); **vcpkg
  binary cache** actions/cache@v4 key `vcpkg-bin-win2022-${{ hashFiles('vcpkg.json') }}`
  (:197-202); `cmake --preset win-x64-release` + build (:206-218); full
  `ctest --preset win-x64-release -C Release --output-on-failure` (:220-225);
  **Win7 posture gate** — stamp BEFORE verify on rrecov.exe, cr_tests.exe,
  cr_ui.dll (:229-244; THE CURRENT RED STEP — see CURRENT CI STATE); G-09
  report-schema validation of the TEMP-dir sample (:249-259); driverless
  refusal-contract probes — bare rrecov exit 0, `list`/`list-carve-formats`
  exit 0, `driver --json` exit 4 + `"state":"absent"`, `scan --disk 0` exit 4
  + "driver required" with nothing leaking past the gate; explicit `exit 0`
  verdict (:266-311); M-071 CLI junk-gate contract over
  tests/data/images/junk-gate-fat16.raw — scan tree, session round-trip,
  A-013 resume-without-out, recover-all, M-079 auto-save label naming
  (:323-431); failure diagnostics artifact (upload-artifact@v4, 7 days,
  :433-442).
- MUST BE GREEN: yes — the pre-merge gate AND the release gate
  (release.yml:74-78 calls it via workflow_call).

### installer.yml — "Installer gates (S-038)"
- Triggers: push main + PR with paths `['installer/**',
  'tools/test_installer_osgate.py', '.github/workflows/installer.yml']`
  (:30-36); workflow_dispatch. Permissions contents: read (:38-39).
- **Job `osgate-matrix`** (windows-2022, 15 m, :42-46): checkout@v4; Gate 3
  `python tools/test_installer_osgate.py` (OSGATE-V1 sync pin + injectable
  version-tuple matrix) (:48-52); AST parse gate over installer/osgate.ps1,
  build-installer.ps1, run-gates.ps1 (:53-66).
- **Job `silent-roundtrip`** (windows-2022, 60 m, needs osgate-matrix,
  :68-72): checkout@v4; Inno Setup 6.3+ detected from the runner image, choco
  fallback only when absent (:76-90); **full core build**
  `cmake --preset/build win-x64-release` with **no vcpkg binary cache**
  (:92-100 — Findings §7); Win7 posture gate on the two SHIPPED PEs
  (rrecov.exe, cr_ui.dll) (:102-112); driver fetch via
  `.github/scripts/fetch-release.ps1 -Repo heicfast/rda-driver -FloorVersion
  v0.2.4` (:114-119); test-sign rda.sys with a job-local cert CN=RRECOV-CI
  (:121-137); build installer (build-installer.ps1, version
  `1.0.0-gate.<run_number>`) (:139-145); Gates 1+2 via installer/run-gates.ps1
  (silent round-trip residue contract + version-key read-back) (:147-151);
  installer artifact uploaded `if: always()` (upload-artifact@v4, 14 days)
  (:153-160).
- MUST BE GREEN: on installer-path changes (pre-merge for the packaging
  surface).

### release.yml — "Release (combined core + driver)"
- Triggers: tag push `v[0-9]+.[0-9]+.[0-9]+` (:46-48); workflow_dispatch with
  inputs version/publish/skip_validation (:49-62). Permissions
  `contents: write` + `actions: write` (:64-66). Concurrency
  `release-<ref>` **cancel-in-progress: false** (:68-70).
- **Job `validate`**: `if: ${{ !inputs.skip_validation }}`; `uses:
  ./.github/workflows/checks.yml` with contents:read (:73-78) — the gate.
- **Job `resolve`** (windows-2022, 10 m, :81-137): tag+SHA resolution via
  ref_type (tag-push detection), plain-semver convention check, auto
  next-patch from latest release, dispatch-path no-overwrite check; explicit
  exit 0 (:88-137).
- **Job `build-core`** (windows-2022, 90 m, needs validate+resolve, runs if
  validate success OR skip_validation, :139-144): checkout at the exact
  released SHA (:147-149); longpaths (:151-153); vcpkg binary cache
  (actions/cache@v4, :155-160); build + ctest (:162-173); Win7 posture gate
  on rrecov.exe + cr_ui.dll (:175-187); stage the SDK+CLI bundle (bin/lib/
  data/include/docs, PDBs, deployed DLLs minus gtest/gmock, VC143 CRT + UCRT
  redist probed by content, userdata-defaults.json, SHA256SUMS, zip)
  (:192-258); upload core-bundle (upload-artifact@v4, if-no-files-found:
  error, :260-264).
- **Job `driver-tier`** (windows-2022, 30 m, :266-270): fetch prebuilt driver
  at floor v0.2.4 via fetch-release.ps1 (:280-300) with source-build fallback
  (clone + MSBuild, :302-316); persistent dev-sign with secrets
  DEV_SIGN_CERT_PFX_B64/DEV_SIGN_CERT_PASSWORD, CN=RRECOV-DEV-SIGN, signtool
  SHA256, no /tr timestamp (N-084 posture, :321-383); parse-gates the driver
  release's install scripts (:332-340); records unsigned+signed SHA256;
  upload driver-tier artifact (:385-397).
- **Job `assemble-and-publish`** (windows-2022, 30 m, :399-409): download
  artifacts (download-artifact@v4, :412-418); assemble MANIFEST.json +
  SHA256SUMS (:420-476); upload combined-release-set (:478-491); cut the tag
  via gh api on the dispatch-publish path (:493-503); publish via
  **softprops/action-gh-release@v2** with the 8-asset set
  (zip/rda.sys/rda.inf/install.ps1/uninstall.ps1/rda-test-sign.cer/
  MANIFEST.json/SHA256SUMS) (:505-546); dispatch test-benchmark.yml with
  core_tag (:551-558).

### test-benchmark.yml — "Test & Benchmark (prebuilt release, CLI + API)"
- Trigger: workflow_dispatch only (core_tag/profile smoke|full/scan_modes/
  reason; :64-83). Permissions contents: read (:85-86). Concurrency
  `test-benchmark` cancel-in-progress (:88-90).
- **Single job `test-and-benchmark`** (windows-2022 — the execution-validated
  HVCI-off driver class; timeout 120 smoke / 180 full, :93-95). Steps:
  checkout@v4 (:103); longpaths (:105-106); environment manifest + elevation
  assertion + best-free-space work drive (:112-153); release fetch
  hash-verified by convention, `rda-test-sign.cer` REQUIRED (:156-179); core
  staging + app-local PATH + CLI smoke (:182-210); driver install+verify via
  Install-ReleaseDriver.ps1 (:213-223); **API harness compiled against the
  release SDK** (vswhere → vcvars64 → cl /W4 /WX, :226-262); API probes
  (ABI version + driver status through cr_ui, :264-280); the scenario matrix
  via Run-Benchmark.ps1 over .github/scenarios/benchmark-matrix.json
  (:283-299); teardown `if: always()` (uninstall contract M-061, VHD detach,
  cert cleanup; :302-319); evidence artifact `if: always()`
  (upload-artifact@v4, 30 days, :322-329).
- MUST BE GREEN: post-publish consumer (dispatched automatically after every
  publish, release.yml:551-558) — not a merge gate.

### Action pins (exact refs; NONE are SHA-pinned)
`actions/checkout@v4` (8 uses: checks.yml:72,182; installer.yml:47,74;
release.yml:147,278,411; test-benchmark.yml:103), `actions/cache@v4` (3:
checks.yml:75,198; release.yml:156), `actions/upload-artifact@v4` (6:
checks.yml:435; installer.yml:155; release.yml:260,385,479;
test-benchmark.yml:324), `actions/download-artifact@v4` (2: release.yml:412,415),
`softprops/action-gh-release@v2` (1: release.yml:507 — the only third-party
proper action, running with contents:write). Plus the local
`./.github/workflows/checks.yml` workflow_call (release.yml:76). All refs are
mutable tags; verified no 40-hex SHA pins exist in any workflow. Runners:
`windows-2022` pinned on every job (never windows-latest; rationale
checks.yml:37-40). No matrix axes anywhere; no continue-on-error anywhere;
skip conditions: release validate (skip_validation input),
checks/installer path filters.

## Local gates

**ci-local.sh** (Unix entry point; docs/CI-LOCAL.md) — 10 components in
order, then optional lanes:
1. clang-format over src/include/tests (presence check only — unpinned;
   ci-local.sh:37-50)
2. Lints: N-041 dead-surface (:55-61); driver-only architecture (:64-70);
   N-210 live conflict-marker grep over src/include/tests/tools/bench/fuzz/
   docs/scripts/diff-to-fix/worklog (:74-82 — NOT mirrored in CI);
   **N-231 duplicate-gtest-name lint** via `python3
   scripts/verify/verify_test_names.py` (:83-92 — NEW since S84, before the
   Werror build so the cheapest gate catches the class first);
   static-analysis absence INFO (:93-95)
3. Werror build `cmake --preset linux-release` (:98-105)
4. Full ctest in build/linux-release (incl. cr_ui ABI + win_syntax_check +
   test_name_uniqueness) (:107-118)
5. Repo-truth self-tests: validate_report.py, test_verify_pe_um.py,
   test_ceiling_metric.py (:121-136)
6. vcpkg.json manifest presence+validity (:139-144)
7. Doc-link check over docs/ + README (:147-159)
8. Secrets scan of the diff vs origin/main (GitHub-PAT/AWS/private-key
   token patterns; :162-171)
9. Commit-message convention type(scope): (:174-183)
10. core.hooksPath == .githooks presence (:186-191)

Optional: `--san` = TSan (manual cmake build/tsan, :196-203) + UBSan
(linux-ubsan preset, :205-208) — **no ASan lane**; `--fuzz` = make -C fuzz
all+run with clang++ (:210-218); `--all` = both.

**ci-local.ps1** (Windows entry point; the component numbering checks.yml's
header mirrors): [1] clang-format (pip-installs the pinned 18.1.8 when
missing, :26-45); [2] the same two lints + static-analysis INFO (:48-59) —
NO N-210 and NO N-231 mirror (Unix-only asymmetry); `-Fast` exits HERE
(:61-65 — Findings §12); [3] MSVC win-x64-release build (:68-75); [4] ctest
(:77-79); [5] the REAL Win7 posture gate — stamp+verify over
rrecov.exe/cr_tests.exe/cr_ui.dll (:82-91); [6] vcpkg.json (:94-96); [7] doc
links (:99-109); [8] secrets (:112-117); [9] commit convention (:120-129);
[10] hooks path (:132-134).

**.githooks/pre-commit** (fast subset, seconds): staged secrets scan
(:13-16) + staged-file clang-format excluding third_party/ (:19-32) + the
FULL-TREE N-231 duplicate-gtest-name check (:34-42 — full-tree, not
staged-only, because the class's real shape is a staged file colliding with
an already-committed one). **.githooks/pre-push**: dispatches the FULL gate —
pwsh ci-local.ps1 on Windows shells, ./ci-local.sh otherwise (:8-11). Install
via `git config core.hooksPath .githooks` (checked only as INFO at component
10). Bypass with --no-verify is a documented contributor violation.

## Win7 / import gate

Four cooperating layers (the "GAP-W6-WIN7 / F-D-4" posture, target envelope
Win7 SP1 → Win11 + Server 2025, x64-only):

1. **Compile-time**: `_WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000`
   on every MSVC compilation (CMakeLists.txt:29-30; rationale :22-28).
2. **Post-link stamp**: `tools/stamp_osversion_um.py` sets
   Major/MinorOperatingSystemVersion and Major/MinorSubsystemVersion to 6.1
   (stamp_osversion_um.py:72-75), PRESERVES the subsystem value (:71),
   recomputes the PE checksum (:77-78). Idempotent; MUST run after link and
   BEFORE verify.
3. **Verify gate**: `tools/verify_pe_um.py` — asserts machine 0x8664, OS
   version (6,1), subsystem version ≥ (6,1) (verify_pe_um.py:195-197,
   333-347); gates the import DLL set via `ALLOWED_DLLS` (:208-246:
   kernel32/advapi32/cfgmgr32/user32 strict; ucrtbase/vcruntime140(_1)/
   msvcp140/z[libd]1|z[|d].dll/lz4/liblz4/zstd/libzstd/gtest + UCRT api-sets
   name-only); strict function allowlist = `tools/ref_imports_um.json`
   — **29 kernel32 + 10 advapi32 + 2 cfgmgr32 + 1 shlwapi (PathIsNetworkPathW)
   functions** (ref_imports_um.json:4-53; the +6 M-118/N-178 wave landed in
   2619e2e4, the shlwapi section in 3b203fe1) UNION the in-script
   `ALLOWLIST_ADDITIONS` (**103 reasoned entries** incl. the SRWLock/CV/
   InitOnce families, the <filesystem> set, and K32GetProcessMemoryInfo,
   verify_pe_um.py:32-193). Any NEW kernel32/advapi32 import fails the build
   ("forcing a conscious allowlist update with a reason, never a silent
   drift", verify_pe_um.py:19-20). Windows-8+-only imports
   (WaitOnAddress/WakeByAddress*) are deliberately NOT allowlisted
   (:159-161). **NOTE the two-stage gate order**: the DLL NAME gate runs
   FIRST (:363-367) — a DLL missing from ALLOWED_DLLS fails before its
   function allowlist is ever consulted; shlwapi.dll is in the JSON but NOT
   in ALLOWED_DLLS at HEAD (Findings §1).
4. **Installer OS-gate**: `installer/osgate.ps1` — OSGATE-V1 decision table:
   admit 6.1.7601+ / 6.2 / 6.3 / 10.0+ on x64 only (osgate.ps1:24,47-70),
   REFUSE everything else. The SAME table is embedded in
   `installer/core-recovery.iss:116` behind the OSGATE-V1 marker line; the
   sync pin + injectable-tuple matrix are enforced by
   `tools/test_installer_osgate.py` (Gate 3, installer.yml:48-52).

**Where the PE gate runs**: checks.yml:229-244 (rrecov.exe, cr_tests.exe,
cr_ui.dll — the CI windows lane); installer.yml:102-112 (the two shipped
PEs); release.yml:175-187 (the two shipped PEs); ci-local.ps1:82-91 (the
same 3 PEs locally). Self-test (Linux-visible): `tools/test_verify_pe_um.py`
synthesizes minimal PE32+ images and exercises both sides — PASS path (the
exact 2026-10-23 MSVC import surface), stamp path ((10,0) fails then passes
after stamping), and FAIL paths (WaitOnAddress, gdi32.dll, a user32
function). Also in the loop: `tools/validate_report.py` (G-09 report-schema
checker), `tools/gen_junk_gate_fixture.py` (regenerates the M-071 fixture),
`tools/doc_truth_gate.py` (583 lines: FloorVersion discipline :98,
deleted-citation scan over docs/*.md+README :135-161, cited-scripts-exist
:176, finding-ID uniqueness :213, ABI count claims :332, filed-as citations
:390, bench ioctl codes :428, parity-delta numbering :469, header contract
phrases :531).

## CURRENT CI STATE (2026-10-30)

**The last 8 Checks runs on main are RED (runs 40965919891 through
40981611198).** Five build-and-test logs are captured under
worklog/ci-failures/ (the README there predates them — Findings §9). The
run-by-run map (verified from the captured logs' checkout lines and FAIL
lines):

| Run (head commit) | Failed step | Evidence |
|---|---|---|
| 40965919891 (pre-624db00d) | cr_tests unbuildable — MSVC C2065 'settled' undeclared (telemetry conflict-resolution residue; repaired by 624db00d) | 624db00d's commit message (no local log) |
| 40973324217 (cbe54bd7) | ctest: 1 failed of 1748 — `Telemetry.CountersMoveDuringStage` (N-239 freeze-observe race; repaired by 7af4c974) | log: `99% tests passed, 1 tests failed out of 1748` |
| 40975641537 (?) | log unretrievable (Azure BlobNotFound — the 215-byte XML stub in ci-failures/) | checks-40975641537-build-and-test.log |
| 40977711470 (c57986d6) | Win7 posture gate on rrecov.exe: 12 FAILs — 4 advapi32 (RegQueryValueExA/RegOpenKeyExA/RegOpenKeyA/RegEnumKeyA), SHLWAPI.dll unexpected, 7 kernel32 (GetConsoleMode, ReadConsoleInputW, SetFileAttributesW, SetFileTime, GetVolumePathNameW, Wow64DisableWow64FsRedirection, Wow64RevertWow64FsRedirection); ctest 1747/1747 green | log FAIL lines + `100% tests passed, 0 tests failed out of 1747` |
| 40979687718 (2619e2e4) | posture gate on rrecov.exe: 6 FAILs — SHLWAPI.dll + SetFileAttributesW, SetFileTime, GetVolumePathNameW, Wow64 pair (the 2619e2e4 extension covered the advapi32 quartet + GetConsoleMode/ReadConsoleInputW); ctest 1747/1747 green | log |
| 40981611198 (3b203fe1 = HEAD) | SAME 6 FAILs on rrecov.exe | log :4095-4120 |

The exact FAIL lines of the LATEST run (worklog/ci-failures/
checks-40981611198-build-and-test.log:4095-4120): `stamped: OS 6.1
subsystem 3 checksum 0x00186381` (:4095) — the stamp works; `ctest green`
(:4078); then on rrecov.exe — `FAIL: unexpected import DLL 'SHLWAPI.dll'`
(:4100), `FAIL: import 'SetFileAttributesW' (from KERNEL32.dll) is not in
the Win7 allowlist` (:4101), same for `SetFileTime` (:4102),
`GetVolumePathNameW` (:4103), `Wow64DisableWow64FsRedirection` (:4104),
`Wow64RevertWow64FsRedirection` (:4105); `verify_pe_um: GATE FAILED`
(:4118); `##[error]import allowlist failed:
build\win-x64-release\src\app\Release\rrecov.exe` (:4119); exit 1 (:4120).
**Build + full ctest (1747/1747) PASS** — the Win7 posture gate
(stamp + import allowlist) step is the only red step.

**Why HEAD is still red (the live remediation target):** 3b203fe1 added a
`shlwapi.dll`/PathIsNetworkPathW section to tools/ref_imports_um.json:51-53
but did NOT add `shlwapi.dll` to `ALLOWED_DLLS` (verify_pe_um.py:208-246) —
the name-gate at :363-367 fires first, so the JSON entry is unreachable as
written. The five kernel32 imports failing since 40979687718 —
SetFileAttributesW + SetFileTime (the S-060 restoration tail,
src/core/recovery/cr_safewriter.cpp:822-823 via the win layer),
GetVolumePathNameW (cr_transport.cpp:1066), the Wow64 pair (the M-075
chkdsk launch) — are in NEITHER ref_imports_um.json NOR ALLOWLIST_ADDITIONS
at HEAD (searched; zero hits under tools/). ALSO: 3b203fe1's commit message
says "the first two PEs clean, the third carries SHLWAPI.dll" — the captured
log shows rrecov.exe (the FIRST PE) failing; both PEs that link cr::win
(rrecov.exe, cr_ui.dll) carry the import (src/win/CMakeLists.txt:18).

**v0.6.0 release workflow failed twice** (runs 40954956609, 40960961632 —
per the session brief; no local logs). release.yml's job chain explains the
failure surface: `validate` (the nested checks.yml call — inherits the same
Win7-gate red), `resolve`, `build-core`, `driver-tier`,
`assemble-and-publish` — any of validate/build-core failing stops the
publish; the v0.6.0 tag itself exists (latest tag), so the runs died before
a green publish or mid-chain (the release notes record the run-27 refusal
repairs: 08456571, 92650e5e, fb2981f6 fixed the Windows-red test set the
release validation exposed).

**The exact job set that must be green at session end:**
- checks.yml: `lint` + `build-and-test` (the posture-gate step included).
- installer.yml: `osgate-matrix` + `silent-roundtrip`.
- release.yml on the coming v0.6.1 tag: `validate` (nested checks) +
  `resolve` + `build-core` + `driver-tier` + `assemble-and-publish`
  (tag-push path publishes: PUBLISH=true via
  `startsWith(github.ref,'refs/tags/')`, release.yml:408).
- test-benchmark.yml: `test-and-benchmark` (auto-dispatched by the publish,
  release.yml:551-558; also manual).

## Release machinery (the v0.6.1 posture)

- Trigger: tag push `vX.Y.Z` (release.yml:46-48) or workflow_dispatch
  (version/publish/skip_validation, :49-62). Chain: **validate**
  (workflow_call into checks.yml, :74-78) → **resolve** (tag+SHA identity,
  :81-137) → **build-core** ∥ **driver-tier** (both need validate+resolve,
  run on validate success OR skip_validation, :139-144/:266-268) →
  **assemble-and-publish** (needs build-core+driver-tier+resolve, :399-409;
  MANIFEST.json + SHA256SUMS :420-476, tag cut on dispatch-publish :493-503,
  softprops publish :505-546, test-benchmark dispatch :551-558).
- **The v0.6.1 version stamps already landed** (commit f621b37f): vcpkg.json
  version-string 0.6.1 (vcpkg.json:3) + CMake project VERSION 0.6.1
  (CMakeLists.txt:4) — the release-truth rule restored at the v0.6.0 close
  (v0.5.0–v0.5.2 had shipped with a stale 0.4.0 stamp in their MANIFESTs).
- **The v0.6.1 tag itself does not exist yet** — `git tag --sort=-
  creatordate` shows v0.6.0 as latest (v0.5.2, v0.5.1, v0.5.0,
  v0.4.1-parity follow). The v0.6.1 notes are staged (524a786e, RELEASE-NOTES.md).
  The publish will assemble MANIFEST.json's core_version from vcpkg.json's
  version-string (release.yml:434), so the stamps and the tag must agree —
  they do at HEAD.

## Findings

Each with file:line evidence; statuses re-verified at HEAD 3b203fe1.

1. **THE LIVE ONE — the Win7 posture gate is provably still red at HEAD.**
   shlwapi.dll is in ref_imports_um.json:51-53 but NOT in ALLOWED_DLLS
   (verify_pe_um.py:208-246); the name-gate (:363-367) fires first → "FAIL:
   unexpected import DLL 'SHLWAPI.dll'" recurs. The five kernel32 imports
   (SetFileAttributesW, SetFileTime, GetVolumePathNameW,
   Wow64DisableWow64FsRedirection, Wow64RevertWow64FsRedirection) are in no
   allowlist at HEAD. The remediation needs BOTH: an ALLOWED_DLLS
   shlwapi.dll entry (or a strict-function policy) AND the five kernel32
   entries (JSON or ALLOWLIST_ADDITIONS, with reasons — the gate's own
   doctrine, verify_pe_um.py:19-20). Plus the 3b203fe1 commit message's
   "third PE" description contradicts the captured logs (rrecov.exe, the
   first PE, fails) — worth a correction note in the commit trail.
2. **No third-party action is SHA-pinned.** All refs are mutable tags
   (see the census above). Supply-chain posture: tag-only pinning —
   unchanged since S84.
3. **Installer lane reintroduces the known fixed cost.** installer.yml:92-100
   builds the FULL core with NO vcpkg binary cache while checks.yml:197-202
   and release.yml:155-160 cache (`vcpkg-bin-win2022-${{
   hashFiles('vcpkg.json') }}`) — zlib/gtest/lz4/zstd rebuild from source on
   every installer-lane run.
4. **Fuzz campaign coverage drifts (verified still true).** (a) fuzz_reghive
   is BUILT (TARGETS, fuzz/Makefile:43) but executed by NEITHER `make run`
   (:72-87) NOR `make run-long` (:89-103). (b) `run-long` silently drops
   `fuzz_image` (in `run` at :86; absent from :89-103). (c) the `corpus`
   mkdir list (:64-68) omits `corpus/thumbnail`, which run/run-long consume
   (:87/:103) — works because 485 thumbnail seeds are committed.
5. **Fuzz clang lane strictly weaker than the CMake gcc lane**
   (fuzz/Makefile:28-30 vs CMakeLists.txt:32 — no -Wshadow/-Wpedantic).
6. **No gate anywhere runs the ASan preset.** CI is windows-only
   (checks.yml:3-8); ci-local.sh's default builds linux-release (:99);
   `--san` runs TSan+UBSan only (:196-208). The `linux-gcc` ASan preset
   (CMakePresets.json:14-23) — still called "the lane CI runs" by
   CONTRIBUTING.md:55 (residual stale fragment; the file's Session-84
   CI-shape note at :26-36 is otherwise accurate) — is executed by nothing.
7. **ci-local.ps1 -Fast comment vs behavior** — the header says
   "format/lint/secrets/commits only" (ci-local.ps1:9-10) but the -Fast exit
   at :61-65 fires after component 2; the secrets scan ([8]) and commit
   convention ([9]) never run under -Fast.
8. **Local/CI clang-format pin asymmetry (Unix side).** CI pins
   clang-format==18.1.8 (checks.yml:91) and ci-local.ps1 pip-installs the pin
   (ci-local.ps1:33), but ci-local.sh only checks presence (ci-local.sh:38-50).
9. **ci-failures/README.md does not describe the current RED set.** It
   documents the three older captures (checks-281, checks-282,
   test-benchmark-6) only; the four 2026-10-29/30 build-and-test logs
   (40973324217, 40977711470, 40979687718, 40981611198) and the BlobNotFound
   stub (40975641537) are undocumented there.
10. **GTest is dual-sourced and the fallback is hash-unpinned.** vcpkg.json:6
    declares gtest while CMakeLists.txt:172-178 falls back to FetchContent of
    the v1.15.2 tarball by URL with no SHA256.
11. **Static analyzers absent (clang-tidy/cppcheck/CodeQL) + no
    .editorconfig** — no config anywhere; the absence is acknowledged at
    ci-local.sh:93-95, ci-local.ps1:59, docs/CI-LOCAL.md:42-47.
12. **checks.yml lint does not mirror the full local gate (asymmetric both
    ways).** CI-only: AST parse gate, YAML parse gate, M-088 doc-truth gate
    (checks.yml:125-174). Local-only: N-210 conflict-marker lint, N-231
    duplicate-gtest-name lint (ci-local.sh:74-92), doc-link check, secrets
    scan, commit-message convention, hooks check. No CI job executes either
    ci-local script — though test_name_uniqueness (tests/CMakeLists.txt:235-238)
    now carries the N-231 check into BOTH ctest lanes.
13. **docs/CI-CURRENT.md:22 still says installer timeouts "15/75"** —
    actuals are 15 (installer.yml:45) and 60 (installer.yml:72). (The old
    phantom-tool citation there is FIXED: :54/:153 now cite
    tools/gen_junk_gate_fixture.py + junk-gate-fat16.raw correctly.)
14. **doc_truth_gate's deleted-citation scan covers only docs/*.md +
    README.md** (doc_truth_gate.py:135-161) — CONTRIBUTING.md at repo root is
    outside every gate file-set (its residual stale fragment, Finding §6,
    is invisible to the gate).
15. *(Observation)* The Win7 PE gate's coverage varies by lane: checks.yml
    + ci-local.ps1 gate 3 PEs (incl. cr_tests.exe); installer.yml and
    release.yml gate only the 2 shipped PEs — consistent with "every shipped
    Windows PE" (verify_pe_um.py:4) but worth knowing when auditing.
16. *(Resolved since S84, recorded for continuity)* The N-210 conflict-marker
    RED-tree finding is FIXED — the exact ci-local.sh:74-82 grep at HEAD
    returns clean; docs/BUILD-SUPPRESSIONS.md's "ZERO entries"
    self-contradiction is FIXED (now headlines "exactly ONE registered
    entry"); the docs/PARITY-ARCH.md stale fuzz-count target file no longer
    exists; ci-local.sh gained the N-231 lint; cr_tests grew to 68 TUs and
    the Windows ctest total is now 1747 (was 1720 at the v0.6.0 cut).

## Lineage

This sheet's prior records: the Session-84 Phase 0 baseline record (HEAD
e8dcaba8 — the "65-TU cr_tests, 81-file cr_core, 27+6+2-function allowlist,
5-seed corpus, 0.4.0 stamps" era), the Session-90 coherence addendum (HEAD
374c54cb — the cr_fsck_win_tests arrival, the win_syntax_check N-228
extension, the 0.5.3 stamps), and the post-window correction (the
7bd3ed8b..d0475641 racing window before the v0.6.0 cut, 1720/1720 at the
cut). Their per-line references describe those historical trees, not this
one. Deltas verified for THIS rewrite: cr_core 83 .cpp, cr_win 11 .cpp,
cr_tests 68 TUs + test_name_uniqueness, version stamps 0.6.1 (f621b37f),
allowlist 29+10+2+1 with the 2619e2e4/3b203fe1 waves, corpus 1467 seeds,
lint-fix wave 9b921ef8, the N-239 telemetry + release-27 test repairs
(08456571/92650e5e/fb2981f6/7af4c974/624db00d), and the CURRENT CI STATE
section above.

— Task 2-a, post-remediation coherence session. End of record.
