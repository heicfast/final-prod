# Build surface — Session-84 Phase 0 (baseline e8dcaba8)

Agent A research record (Task T2-A). Scope: every CMake file, preset, toolchain,
workflow, warning/analyzer/sanitizer setting, fuzz harness, local gate, and the
Win7/import gate — read exhaustively at HEAD `e8dcaba8` (branch `main`,
confirmed via `git log --oneline -5`; remote `github.com/heicfast/core_recovery`).
Every claim carries file:line. No tracked file was modified; this is the only
output file.

CMake file census (exactly 7 exist): `CMakeLists.txt`, `src/CMakeLists.txt`,
`src/core/CMakeLists.txt`, `src/ui/CMakeLists.txt`, `src/win/CMakeLists.txt`,
`src/app/CMakeLists.txt`, `tests/CMakeLists.txt`. **No** CMakeLists under
fuzz/ (Makefile-driven), bench/, tools/, installer/ (PowerShell-driven),
third_party/ (header-only vendored stb).

## Targets

| Target | Type | Dir | Sources | Key options / links |
|---|---|---|---|---|
| `cr_core` | STATIC lib | src/core | 81 .cpp (src/core/CMakeLists.txt:5-87) | PIC ON (:92); PUBLIC include/ + `ZLIB::ZLIB` + LZ4/ZSTD via `PkgConfig::LZ4`/`PkgConfig::ZSTD` with `CR_HAVE_LZ4=1`/`CR_HAVE_ZSTD=1` (:102-110); PRIVATE third_party/stb include (:97-98) + `CR_ENGINE_VERSION="${PROJECT_VERSION}"` (:100-101); ASan `-fsanitize=address` when `CR_ENABLE_ASAN AND NOT MSVC` (:112-115) |
| `cr_win` | STATIC lib | src/win | 10 .cpp (src/win/CMakeLists.txt:2-12) | Windows-only; PUBLIC `cr::core` + `cfgmgr32 Advapi32 Shlwapi` (:15-17) |
| `rrecov` | exe | src/app | rrecov.cpp (src/app/CMakeLists.txt:1) | Windows-only; links `cr::core cr::win` (:2); `cr_deploy_vcpkg_binaries` (:5) |
| `cr_ui` | SHARED lib | src/ui | cr_ui.cpp (src/ui/CMakeLists.txt:11) | `CR_UI_BUILD` (:14); links `cr::core`; Win32: `Psapi` + `cr::win` (:20-25); hidden visibility + PIC (:35-38); MSVC-only `cr_ui_exports.def` (:32-34); ASan (:40-43); DLL deploy (:46) |
| `cr_tests` | exe (GTest) | tests/unit | 65 TU = main.cpp + 64 test files (tests/CMakeLists.txt:5-71) | links `cr::core GTest::gtest_main` (:73); POST_BUILD fixture copy of data/images, data/exfat, data/thumbnails (:77-88); `CR_SOURCE_DIR` define (:93-94); DLL deploy (:98); `gtest_discover_tests` (:197) |
| `cr_destio_tests` | exe (GTest, WIN32-only) | tests/unit/test_destio_win.cpp | tests/CMakeLists.txt:105-113 | links `cr::win`; `add_test destio` (:113) |
| `cr_bitlocker_tests` | exe (GTest, WIN32-only) | tests/unit/test_bitlocker_win.cpp | tests/CMakeLists.txt:117-120 | links `cr::win`; `add_test bitlocker` (:120) |
| `cr_image_create_tests` | exe (GTest, WIN32-only) | tests/unit/test_image_create_win.cpp | tests/CMakeLists.txt:126-132 | links `cr::win`; `add_test image_create` (:132) |
| `cr_ui_tests` | exe (GTest) | tests/unit/test_cr_ui.cpp | tests/CMakeLists.txt:150-151 | links `cr::ui` — tests the shim as a host consumes it; Win32 additionally links static `cr::core` for direct core-API fixtures (:153-163); `CR_SOURCE_DIR` (:165-166); `gtest_discover_tests` (:198) |
| `cr_ui_abi_check` | exe (plain C) | tests/ui_abi_check.c | tests/CMakeLists.txt:168-169 | links `cr::ui`; `add_test cr_ui_abi_check <binary-dir>/cr_ui_abi_check.img` (:181-182); Win32 copies cr_ui.dll beside it (:172-179) |
| `win_syntax_check` | ctest test (not a target) | tools/win_syntax_check.sh | tests/CMakeLists.txt:213-217 | Linux **gcc-only** (`NOT WIN32 AND NOT Clang`, :213); runs g++ `-fsyntax-only` over src/win/*.cpp + src/app/*.cpp + src/ui/*.cpp against the declaration-only SDK stubs tests/win-stub/{windows,psapi,ntddscsi,share,winioctl,cfgmgr32,shlwapi}.h (tools/win_syntax_check.sh:47-77; flags `-fshort-wchar -D_WIN32 -D__declspec(x)=` at :47) |

Global project facts: `cmake_minimum_required(3.25)` (CMakeLists.txt:1);
project `core_recovery` VERSION 0.4.0, C CXX (CMakeLists.txt:3-6); C++20
Required, extensions OFF; C11 (CMakeLists.txt:13-17). Options:
`CR_ENABLE_ASAN` OFF, `CR_BUILD_TOOLS` ON, `CR_BUILD_APP` ON, `CR_BUILD_UI`
ON, `CR_ENABLE_TESTS` ON (CMakeLists.txt:38-42). NOTE: `CR_BUILD_TOOLS` is
declared but consumed by nothing (dead option; no tools/ CMake targets).
Hard-fail dependency posture: zlib REQUIRED (CMakeLists.txt:48-53); lz4+zstd
REQUIRED with 3-tier discovery CONFIG→pkg-config→find_library and the
PkgConfig::LZ4 aliasing wave (CMakeLists.txt:78-147, FB-X01). GTest:
`find_package(GTest)` else FetchContent googletest v1.15.2 tarball
(CMakeLists.txt:172-178). `cr_deploy_vcpkg_binaries()` POST_BUILD hook copies
every config-matching DLL from `vcpkg_installed` next to the target
(CMakeLists.txt:156-167; cmake/DeployVcpkgBinaries.cmake:15-45 — Debug copies
release+debug trees both, :28-35).

## Presets & toolchain

`CMakePresets.json` (version 6; cmakeMinimumRequired 3.25.0):

| Preset | Generator | Cache vars / notes |
|---|---|---|
| `base` (hidden) | — | binaryDir `${sourceDir}/build/${presetName}` (:10-13) |
| `linux-gcc` | Unix Makefiles | RelWithDebInfo, `CR_BUILD_APP=OFF`, `CR_ENABLE_ASAN=ON` (:15-23) — the documented "CI parity ASan lane" but run by NO CI workflow and NOT by ci-local.sh (see Findings §12) |
| `linux-release` | Unix Makefiles | Release (:25-31) — the ci-local.sh default build (:89) |
| `win-x64-debug` | VS 17 2022 x64 | toolchainFile `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`, Debug (:33-41) — ORPHAN: no workflow, script, or test preset consumes it |
| `win-x64-release` | VS 17 2022 x64 | vcpkg toolchain, Release (:43-51) — the workhorse: checks.yml:214/216, installer.yml:97/99, release.yml:168/170, ci-local.ps1:71/73 |
| `linux-ubsan` | Unix Makefiles | Debug, `CR_BUILD_APP=OFF`, ASAN OFF, `-fsanitize=undefined -fno-sanitize-recover=undefined -g -fPIE` on C+CXX, `-pie` exe linker (:53-65) |

buildPresets for all 5 (:67-90). testPresets: `linux-gcc`, `win-x64-release`,
`linux-ubsan` only (:91-113) — no `linux-release` test preset (ci-local.sh runs
ctest directly inside build/linux-release, ci-local.sh:99). Toolchain files:
the ONLY toolchain reference is vcpkg's stock
`scripts/buildsystems/vcpkg.cmake` via the win presets; `cmake/` contains
exactly one file, `DeployVcpkgBinaries.cmake`. `vcpkg.json`: name
core-recovery, version-string 0.4.0, deps zlib/gtest/lz4/zstd (vcpkg.json:2-9)
— manifest mode is the pinning policy, asserted at checks.yml:120 and
ci-local.sh:130-134 / ci-local.ps1:95-96.

## Warning policy per compiler

Global (identical strictness doctrine, CMakeLists.txt:8-36):

- **MSVC** (every target, every config): `/W4 /WX /permissive- /Zc:preprocessor
  /utf-8 /external:W0 /wd28251` (CMakeLists.txt:20-21) + definitions
  `_CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0601 WINVER=0x0601
  NTDDI_VERSION=0x06010000` (CMakeLists.txt:29-30). `/wd28251` is the ONLY
  `/wd` in any CMake file; no warning pragmas exist in CMake.
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
`#pragma warning`, `pragma GCC diagnostic`, `pragma clang diagnostic`,
`_Pragma`, `__pragma` — all negative except this):

- `src/core/preview/cr_thumbnail.cpp:33-56` — wraps ONLY the vendored
  `#include "stb_image.h"` (with `STBI_NO_STDIO=1`, `STBI_MAX_DIMENSIONS
  32768`, `STB_IMAGE_IMPLEMENTATION`, :48-51). MSVC side: `#pragma
  warning(push)` + `disable : 4100 4189 4244 4245 4267 4456 4457 4701 4702
  4706` (:34-35), `pop` (:53). GCC side: `#pragma GCC diagnostic push` (:37),
  `ignored "-Wmaybe-uninitialized"` under `__GNUC__ && !__clang__` only
  (:41-43), `ignored "-Wunused-but-set-variable"` (:44), `"-Wshadow"` (:45),
  `"-Wpedantic"` (:46), `pop` (:55). Registered as **S-1** in
  docs/BUILD-SUPPRESSIONS.md:19. First-party code in the same TU is outside
  the push/pop. `third_party/stb/stb_image.h` itself contains no pragmas; it
  is excluded from clang-format by the pre-commit filter
  (.githooks/pre-commit:24) and by CI/local format scopes (src/include/tests
  only — checks.yml:93, ci-local.sh:39, ci-local.ps1:36).

Zero first-party suppressions otherwise; /WX + -Werror are global and every
preset build is the Werror build.

## Analyzers & sanitizers

- **clang-format**: root `.clang-format` (:1-12): `BasedOnStyle: LLVM`,
  Language Cpp, Standard c++20, IndentWidth 4, ColumnLimit 100,
  AccessModifierOffset -4, SortIncludes Never, IncludeBlocks Preserve,
  AllowShortFunctionsOnASingleLine Inline, AllowShortIfStatementsOnASingleLine
  Never, PointerAlignment Left, DerivePointerAlignment false. Version PINNED
  to `clang-format==18.1.8` in CI (checks.yml:91) and in ci-local.ps1:33
  (auto-pip-install). ci-local.sh does NOT pin (presence check only,
  ci-local.sh:38-50) — see Findings §15.
- **clang-tidy**: NO config anywhere (absence). Acknowledged honestly at
  ci-local.sh:83-85, ci-local.ps1:59, docs/CI-LOCAL.md:42-46 ("not configured
  yet — tracked, not silenced").
- **cppcheck**: none (same acknowledgment).
- **CodeQL**: none in `.github/` (verified by search). CONTRIBUTING.md:67
  still claims "CodeQL is present but deferred (`if: false`)" — stale.
- **Sanitizer lanes**: ASan via `CR_ENABLE_ASAN` → `-fsanitize=address` on
  cr_core (src/core/CMakeLists.txt:112-115), cr_ui (src/ui/CMakeLists.txt:40-43),
  cr_tests + cr_ui_tests + cr_ui_abi_check (tests/CMakeLists.txt:184-194);
  `linux-gcc` preset turns it on. UBSan via `linux-ubsan` preset. TSan via a
  manual cmake invocation in ci-local.sh:186-193 (build/tsan, gcc).
  Fuzz lane: `-fsanitize=fuzzer,address,undefined` (fuzz/Makefile:28).
  **No CI workflow runs any sanitizer lane** (windows-only consolidation,
  checks.yml:3-8). No `continue-on-error` anywhere in any workflow.

## Fuzz harness

- 16 libFuzzer targets built by `fuzz/Makefile` (TARGETS, fuzz/Makefile:39-44):
  fuzz_pure, fuzz_engines, fuzz_carve, fuzz_container, fuzz_ntfs, fuzz_fat,
  fuzz_exfat, fuzz_ext4, fuzz_hfsplus, fuzz_btrfs, fuzz_zfs, fuzz_refs,
  fuzz_preview, fuzz_reghive, fuzz_image, fuzz_thumbnail (16 harness .cpp +
  fuzz_parser_common.h). The whole core is compiled into `build/libcrfuzz.a`
  from ALL ../src/core/*.cpp (fuzz/Makefile:34-37).
- Build: `make CXX=clang++`; CXXFLAGS `-std=c++20 -g -O1
  -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -I../include
  -I../third_party/stb -Wall -Wextra -Werror` + optional CR_HAVE_LZ4/ZSTD via
  pkg-config (fuzz/Makefile:19-31).
- Smoke: `make run` — 45 s per target (20 s for the per-FS harnesses),
  `-max_len` 64 KiB–2 MiB, `-timeout=25 -rss_limit_mb=2560`
  (fuzz/Makefile:72-87). `make run-long` — 60–120 s per target (:89-103).
- Corpus: committed seeds under fuzz/corpus/pure (1 seed:
  enum-attrs-window-overrun-513.seed) and fuzz/corpus/thumbnail (4 seeds:
  bomb_dims.png, huge_dims_exif_thumb.jpg, quad64x48.jpg, tiff_packbits.tif);
  all other corpus dirs are created empty by `make corpus`
  (fuzz/Makefile:64-68).
- Where run: LOCAL ONLY — `./ci-local.sh --fuzz` (ci-local.sh:200-208, make
  all + run with clang++). No CI lane (dropped by the owner's windows-only
  directive, checks.yml:4-8). Drifts inside the Makefile: Findings §5.

## CI workflows

### checks.yml — "Checks" (the pre-merge gate; MUST BE GREEN)
- Triggers: push+PR to main with symmetric `paths-ignore: ['**.md',
  'docs/**', 'worklog.md', 'worklog/**', 'AUDIT*.md']` (checks.yml:43-49);
  `workflow_call: {}` so release.yml can call it (:50). Permissions
  `contents: read` (:52-53). Concurrency `checks-<event>-<ref>`
  cancel-in-progress (event-scoped so a release's nested validate is never
  cancelled, :55-64).
- **Job `lint`** (windows-2022, timeout 15 m, :67-70): actions/checkout@v4;
  pip wheel cache via actions/cache@v4 key `pip-clangformat-win2022` (:74-79);
  [1/2] clang-format 18.1.8 `--dry-run -Werror` over src/include/tests
  (:88-102); [2/2] N-041 dead-surface lint (RdaClient::/volume_letter_to_disk
  over src,include,tests,tools,bench,fuzz) + driver-only architecture lint
  (rda_available/win_pe_environment/open_harddisk_volume/prefer_rda over
  src,include) + vcpkg.json presence (:105-123); PowerShell AST parse gate
  over ci-local.ps1 + .github/scripts/*.ps1 + bench/*.ps1 (:125-148); YAML
  parse gate via PyYAML over all workflows (:150-162); M-088 CI-shape
  doc-truth gate `python tools/doc_truth_gate.py` (:164-174).
- **Job `build-and-test`** (windows-2022, timeout 90 m, needs lint,
  :176-180): git `core.longpaths true` (:187-189); **vcpkg binary cache**
  actions/cache@v4 key `vcpkg-bin-win2022-${{ hashFiles('vcpkg.json') }}`
  (:197-203); `cmake --preset win-x64-release` + build (:206-218); full
  `ctest --preset win-x64-release -C Release --output-on-failure` (:220-225);
  **Win7 posture gate**: stamp+verify on rrecov.exe, cr_tests.exe, cr_ui.dll
  (stamp BEFORE verify; :229-244); G-09 report-schema validation of the
  TEMP-dir sample (:249-259); driverless refusal-contract probes — bare
  rrecov exit 0, `list`/`list-carve-formats` exit 0, `driver --json` exit 4
  + `"state":"absent"`, `scan --disk 0` exit 4 + "driver required" with
  nothing leaking past the gate; explicit `exit 0` verdict (:266-311); M-071
  CLI junk-gate contract over tests/data/images/junk-gate-fat16.raw — scan
  tree, session round-trip, A-013 resume-without-out, recover-all, M-079
  auto-save label naming (:323-431); failure diagnostics artifact
  (upload-artifact@v4, 7 days, :433-442).
- MUST BE GREEN: yes — this lane is the pre-merge gate AND the release gate
  (release.yml:74-78 calls it via workflow_call; "no release without green").

### installer.yml — "Installer gates (S-038)"
- Triggers: push main + PR with paths `['installer/**',
  'tools/test_installer_osgate.py', '.github/workflows/installer.yml']`
  (:30-36); workflow_dispatch. Permissions contents: read.
- **Job `osgate-matrix`** (windows-2022, 15 m, :42-46): checkout@v4; Gate 3
  `python tools/test_installer_osgate.py` (OSGATE-V1 sync pin + injectable
  version-tuple matrix) (:48-52); AST parse gate over installer/osgate.ps1,
  build-installer.ps1, run-gates.ps1 (:53-66).
- **Job `silent-roundtrip`** (windows-2022, 60 m, needs osgate-matrix,
  :68-72): checkout@v4; Inno Setup 6.3+ detected from the runner image, choco
  fallback only when absent (:76-90); **full core build**
  `cmake --preset/build win-x64-release` with **no vcpkg binary cache**
  (:92-100 — see Findings §8); Win7 posture gate on the two SHIPPED PEs
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
- **Job `resolve`** (windows-2022, 10 m): tag+SHA resolution; semver
  convention check; dispatch-path no-overwrite check; explicit exit 0
  (:81-137).
- **Job `build-core`** (windows-2022, 90 m, needs validate+resolve, runs if
  validate success OR skip_validation, :139-144): checkout at the exact
  released SHA (:147-149); longpaths; vcpkg binary cache (actions/cache@v4,
  :155-160); build + ctest (:162-173); Win7 posture gate on rrecov.exe +
  cr_ui.dll (:175-187); stage the SDK+CLI bundle (bin/lib/data/include/docs,
  PDBs, deployed DLLs, VC143 CRT + UCRT redist probed by content,
  userdata-defaults.json, SHA256SUMS, zip) (:192-258); upload core-bundle
  (upload-artifact@v4, if-no-files-found: error, :260-264).
- **Job `driver-tier`** (windows-2022, 30 m, :266-269): fetch prebuilt driver
  at floor v0.2.4 via fetch-release.ps1 (:280-300) with source-build fallback
  (clone + MSBuild, :302-316); persistent dev-sign with secrets
  DEV_SIGN_CERT_PFX_B64/DEV_SIGN_CERT_PASSWORD, CN=RRECOV-DEV-SIGN, signtool
  SHA256, no /tr timestamp (N-084 posture, :321-383); parse-gates the driver
  release's install scripts (:332-340); upload driver-tier artifact
  (:385-397).
- **Job `assemble-and-publish`** (windows-2022, 30 m, :399-409): download
  artifacts (download-artifact@v4, :412-418); assemble MANIFEST.json +
  SHA256SUMS (:420-476); upload combined-release-set (:478-491); cut the tag
  via gh api on the dispatch-publish path (:493-503); publish via
  **softprops/action-gh-release@v2** with the 8-asset set (:505-546);
  dispatch test-benchmark.yml with core_tag (:551-558).
- MUST BE GREEN: all jobs for a publish (validate is skipped only by the
  DRILL-ONLY skip_validation input, :59-62).

### test-benchmark.yml — "Test & Benchmark (prebuilt release, CLI + API)"
- Trigger: workflow_dispatch only (core_tag/profile smoke|full/scan_modes/
  reason; :64-83). Permissions contents: read. Concurrency `test-benchmark`
  cancel-in-progress (:88-90).
- **Single job `test-and-benchmark`** (windows-2022 — the execution-validated
  HVCI-off driver class; timeout 120 smoke / 180 full, :93-95). Steps:
  checkout@v4; longpaths; environment manifest + elevation assertion +
  best-free-space work drive (:112-153); release fetch hash-verified by
  convention, `rda-test-sign.cer` REQUIRED (:156-179); core staging +
  app-local PATH + CLI smoke (:182-210); driver install+verify via
  Install-ReleaseDriver.ps1 (:213-223); **API harness compiled against the
  release SDK** (vswhere → vcvars64 → cl /W4 /WX, :226-262); API probes
  (ABI version + driver status through cr_ui, :264-280); the scenario matrix
  via Run-Benchmark.ps1 over .github/scenarios/benchmark-matrix.json (3 fixed
  VHDX: 2 GiB MBR FAT32 / 5 GiB GPT NTFS+exFAT / 8 GiB GPT NTFS; CLI + API
  lanes; :283-299); teardown `if: always()` (uninstall contract M-061, VHD
  detach, cert cleanup; :302-319); evidence artifact `if: always()`
  (upload-artifact@v4, 30 days, :322-329).
- MUST BE GREEN: post-publish consumer (dispatched automatically after every
  publish, release.yml:551-558) — not a merge gate.

### Action pins (exact refs; NONE are SHA-pinned)
`actions/checkout@v4` (10 uses: checks.yml:72,182; installer.yml:47,74;
release.yml:147,278,411; test-benchmark.yml:103), `actions/cache@v4` (3:
checks.yml:75,198; release.yml:156), `actions/upload-artifact@v4` (6:
checks.yml:435; installer.yml:155; release.yml:260,385,479;
test-benchmark.yml:324), `actions/download-artifact@v4` (2: release.yml:412,415),
`softprops/action-gh-release@v2` (1: release.yml:507 — the only third-party
proper action, running with contents:write). Plus the local
`./.github/workflows/checks.yml` workflow_call (release.yml:76). All refs are
mutable tags. Runners: `windows-2022` pinned on every job (never
windows-latest; rationale checks.yml:37-40). No matrix axes anywhere; no
continue-on-error anywhere; skip conditions: release validate (skip_validation
input), checks/installer paths-ignore filters.

## Local gates

**ci-local.sh** (Unix entry point; docs/CI-LOCAL.md) — 10 components in
order, then optional lanes:
1. clang-format over src/include/tests (presence check only — unpinned;
   ci-local.sh:37-50)
2. Lints: N-041 dead-surface (:55-61); driver-only architecture
   (:64-70); **N-210 live conflict-marker grep** over src/include/tests/
   tools/bench/fuzz/docs/scripts/diff-to-fix/worklog (:74-82 — NOT mirrored
   in CI); static-analysis absence INFO (:83-85)
3. Werror build `cmake --preset linux-release` (:88-95)
4. Full ctest in build/linux-release (incl. cr_ui ABI + win_syntax_check)
   (:97-108)
5. Repo-truth self-tests: validate_report.py, test_verify_pe_um.py,
   test_ceiling_metric.py (:111-126)
6. vcpkg.json manifest presence+validity (:129-134)
7. Doc-link check over docs/ + README (:137-149)
8. Secrets scan of the diff vs origin/main (:152-161)
9. Commit-message convention type(scope): (:164-173)
10. core.hooksPath == .githooks presence (:176-181)

Optional: `--san` = TSan (manual cmake build/tsan, :186-193) + UBSan
(linux-ubsan preset, :195-198) — **no ASan lane**; `--fuzz` = make -C fuzz
all+run with clang++ (:200-208); `--all` = both.

**ci-local.ps1** (Windows entry point; the ci-local.ps1 component numbering
is what checks.yml's header mirrors): [1] clang-format (pip-installs the
pinned 18.1.8 when missing, :26-45); [2] the same two lints + static-analysis
INFO (:48-59); `-Fast` exits HERE (:61-65 — see Findings §14); [3] MSVC
win-x64-release build (:67-75); [4] ctest (:77-79); [5] the REAL Win7 posture
gate — stamp+verify over rrecov.exe/cr_tests.exe/cr_ui.dll (:82-91); [6]
vcpkg.json (:94-96); [7] doc links (:99-109); [8] secrets (:112-117); [9]
commit convention (:120-129); [10] hooks path (:132-134).

**.githooks/pre-commit** (fast subset, seconds): staged secrets scan
(:13-16) + staged-file clang-format excluding third_party/ (:19-32; exclusion
rationale :20-23). **.githooks/pre-push**: dispatches the FULL gate — pwsh
ci-local.ps1 on Windows shells, ./ci-local.sh otherwise (:8-11). Install via
`git config core.hooksPath .githooks` (checked only as INFO at component 10).
Bypass with --no-verify is a documented contributor violation.

## Win7 / import gate

Four cooperating layers (the "GAP-W6-WIN7 / F-D-4" posture, target envelope
Win7 SP1 → Win11, x64):

1. **Compile-time**: `_WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000`
   on every MSVC compilation (CMakeLists.txt:29-30; rationale :22-28 — the
   SDK macros gate every header declaration to what Win7 ships).
2. **Post-link stamp**: `tools/stamp_osversion_um.py` sets
   Major/MinorOperatingSystemVersion and Major/MinorSubsystemVersion to
   6.1, preserves the subsystem value, recomputes the PE checksum
   (stamp_osversion_um.py:72-78). Idempotent; MUST run after link and BEFORE
   verify.
3. **Verify gate**: `tools/verify_pe_um.py` — asserts machine 0x8664, OS
   version (6,1), subsystem version ≥ (6,1) (verify_pe_um.py:195-197,
   333-347); gates the import DLL set via `ALLOWED_DLLS` (:208-246:
   kernel32/advapi32/cfgmgr32/user32 strict; ucrtbase/vcruntime140/
   msvcp140/z[libd]1/lz4/liblz4/zstd/libzstd/gtest + UCRT api-sets
   name-only); strict function allowlist = `tools/ref_imports_um.json`
   (27 kernel32 + 6 advapi32 + 2 cfgmgr32 functions,
   ref_imports_um.json:4-45) UNION the in-script `ALLOWLIST_ADDITIONS`
   (~100 reasoned entries incl. the SRWLock/CV/InitOnce families and
   K32GetProcessMemoryInfo, verify_pe_um.py:32-193). Any NEW kernel32/
   advapi32 import fails the build ("forcing a conscious allowlist update
   with a reason, never a silent drift", verify_pe_um.py:19-20). Windows-8+-
   only imports (WaitOnAddress/WakeByAddress*) are deliberately NOT
   allowlisted — the guard trips if the toolset ever links them (:159-161).
4. **Installer OS-gate**: `installer/osgate.ps1` — OSGATE-V1 decision table:
   admit 6.1.7601+ / 6.2 / 6.3 / 10.0+ on x64 only, REFUSE everything else
   with exit 3 (osgate.ps1:44-76). The SAME table is embedded in
   `installer/core-recovery.iss:116` behind the OSGATE-V1 marker line; the
   sync pin + injectable-tuple matrix are enforced by
   `tools/test_installer_osgate.py` (Gate 3, installer.yml:48-52).

**Where the PE gate runs**: checks.yml:229-244 (rrecov.exe, cr_tests.exe,
cr_ui.dll — the CI windows lane); installer.yml:102-112 (the two shipped
PEs); release.yml:175-187 (the two shipped PEs, every shipped Windows PE of
the release); ci-local.ps1:82-91 (the same 3 PEs locally). Self-test
(Linux-visible): `tools/test_verify_pe_um.py` synthesizes minimal PEs and
exercises both sides — PASS path, stamp path ((10,0) fails then passes after
stamping), and FAIL paths (WaitOnAddress, gdi32.dll, a user32 function).

## Findings

Each with file:line evidence; severity is the auditor's read, ordered
roughly by importance.

1. **RED TREE AT HEAD — live conflict markers in 5 files; the class is
   invisible to CI.** `docs/PARITY-DELTAS.md:4677` and `:4707`;
   `diff-to-fix/FIXED-LEDGER.md:656`; `diff-to-fix/CROSS-REF-MATRIX.md:39`;
   `diff-to-fix/findings/FIXED/I-048-three-registers-selected-dead-sumary-remote-ext-size-32bit.md:52`;
   `worklog.md:12069` (verified by running the exact ci-local.sh:74-82 grep
   at HEAD). The N-210 conflict-marker lint exists ONLY in ci-local.sh
   (:74-82) — checks.yml's lint job has no equivalent step — AND
   checks.yml:46/:49 `paths-ignore` ('**.md', docs/**, worklog/**) means
   these .md-only files never trigger CI at all. The local pre-push gate is
   RED right now while every CI lane stays green. (HEAD's own commit message
   acknowledges this as "the N-210 class, Phase 3 target".)
2. **Suppression-registry self-contradiction.** docs/BUILD-SUPPRESSIONS.md:7
   declares "**This file intentionally carries ZERO entries.**" while
   BUILD-SUPPRESSIONS.md:19 carries row S-1 (the stb pragma block). Three
   sites cite the stale "zero": checks.yml:8 ("docs/BUILD-SUPPRESSIONS.md:
   zero suppressions"), CONTRIBUTING.md:75-76 ("currently zero entries"),
   docs/CI-LOCAL.md:45-46 ("carries zero rows").
3. **CONTRIBUTING.md describes a CI that no longer exists** (the
   pre-consolidation 7-job era): "CI's `local-mirror` job"
   (CONTRIBUTING.md:26-27 — checks.yml's jobs are `lint` and
   `build-and-test`); "Linux ASan (CI parity) … the lane CI runs"
   (CONTRIBUTING.md:49 — no Linux lane exists); "the `local-mirror` job
   (your gate, verbatim), Linux + TSan + UBSan + the ONE Windows MSVC
   Release build … the libFuzzer campaign (`make -C fuzz`, `-Werror`) …
   CodeQL is present but deferred (`if: false`)" (CONTRIBUTING.md:63-67 —
   none of these are in the current workflows; no CodeQL job exists
   anywhere in .github); "the correctness workflows (driver-install,
   core-install, staged scenarios)" cited in current tense
   (CONTRIBUTING.md:68-70) although doc_truth_gate.py:61-64 lists them as
   DELETED_SURFACE. Root cause: `doc_truth_gate.check_deleted_citations`
   scans only docs/*.md + README.md (doc_truth_gate.py:138) —
   CONTRIBUTING.md is outside every gate file-set.
4. **Phantom tool citation in the CI doc of record.** docs/CI-CURRENT.md:54
   and :153 cite `tools/gen_fat16_junk_fixture.py` and fixture
   `fat16-junk.img` — neither exists on disk. The real script is
   `tools/gen_junk_gate_fixture.py` and the real fixture is
   `tests/data/images/junk-gate-fat16.raw` (checks.yml:328-329,
   gen_junk_gate_fixture.py:3-5). doc_truth_gate's phantom-citation check
   covers only `scripts/` citations under diff-to-fix/ (N-160,
   doc_truth_gate.py:75,21-28), not `tools/` citations under docs/.
5. **Fuzz campaign coverage drifts.** (a) `fuzz_reghive` is BUILT (TARGETS,
   fuzz/Makefile:43) and its corpus dir created (:67) but is executed by
   NEITHER `make run` (:72-87) NOR `make run-long` (:89-103) — the reghive
   harness never runs in any campaign. (b) `run-long` silently drops
   `fuzz_image` (present in `run` at :86; absent from :89-103) — the longer
   campaign exercises FEWER targets than the smoke. (c) the `corpus` mkdir
   list (:64-68) omits `corpus/thumbnail`, which `run`/`run-long` consume
   (:87/:103) — works only because the 4 thumbnail seeds are committed.
6. **Stale fuzz-target count in the architecture parity doc.**
   docs/PARITY-ARCH.md:127 says "12 libFuzzer targets via fuzz/Makefile
   (fuzz/Makefile:31-35)" — the Makefile defines 16 (fuzz/Makefile:39-44;
   also stale line reference).
7. **Installer-lane timeout drift in the CI doc.** docs/CI-CURRENT.md:22
   says installer.yml timeouts "15/75"; actuals are 15
   (osgate-matrix, installer.yml:45) and 60 (silent-roundtrip,
   installer.yml:72).
8. **Installer lane reintroduces the known fixed cost it was built to
   avoid.** installer.yml:92-100 builds the FULL core
   (`cmake --preset win-x64-release`) with NO vcpkg binary cache, while
   checks.yml:197-202 and release.yml:155-160 both cache
   (`vcpkg-bin-win2022-${{ hashFiles('vcpkg.json') }}`) — zlib/gtest/lz4/
   zstd are rebuilt from source on every installer-lane run (the exact cost
   CI-CURRENT.md:59-60 says was eliminated).
9. **No third-party action is SHA-pinned.** All refs are mutable tags:
   actions/checkout@v4, actions/cache@v4, actions/upload-artifact@v4,
   actions/download-artifact@v4, and softprops/action-gh-release@v2
   (release.yml:507 — third-party proper, executing with `contents: write`).
   Supply-chain posture: tag-only pinning.
10. **Static analyzers absent (clang-tidy/cppcheck/CodeQL).** No config
    anywhere; the absence is honestly acknowledged at ci-local.sh:83-85,
    ci-local.ps1:59, docs/CI-LOCAL.md:42-46 — but CONTRIBUTING.md:67's
    CodeQL claim (see §3) contradicts the record.
11. **Fuzz clang lane is strictly weaker than the CMake gcc lane.**
    fuzz/Makefile:28-30 uses `-Wall -Wextra -Werror` without `-Wshadow` and
    without `-Wpedantic` (CMakeLists.txt:32 has both). Code that shadows
    can pass the fuzz lane while failing the CMake lane.
12. **No gate anywhere runs the ASan preset.** CI is windows-only
    (checks.yml:3-8); ci-local.sh's default gate builds `linux-release`
    (ci-local.sh:89); `--san` runs TSan+UBSan only (ci-local.sh:184-198).
    The `linux-gcc` ASan preset (CMakePresets.json:15-23) — the documented
    "CI parity" lane (CONTRIBUTING.md:49) — is executed by nothing. The
    strongest memory-error checker is now manual-only. checks.yml:6-7's
    claim that sanitizer coverage is kept "locally via ./ci-local.sh --san"
    overstates: --san has no ASan.
13. **Orphan preset.** `win-x64-debug` (CMakePresets.json:33-41) is consumed
    by no workflow, no script, and has no test preset (testPresets cover
    linux-gcc/win-x64-release/linux-ubsan only, :91-113).
14. **ci-local.ps1 -Fast comment vs behavior.** The header says "-Fast #
    format/lint/secrets/commits only" (ci-local.ps1:8-10) but the -Fast exit
    at :61-65 fires after component 2 — the secrets scan ([8]) and commit
    convention ([9]) never run under -Fast.
15. **Local/CI clang-format pin asymmetry (Unix side).** CI pins
    clang-format==18.1.8 (checks.yml:91) and ci-local.ps1 pip-installs the
    pin (ci-local.ps1:33), but ci-local.sh only checks presence
    (ci-local.sh:38-50) — a developer with clang-format 23.x can be locally
    green and CI red (the exact drift class the pin exists for; the comment
    at checks.yml:81-87 documents run 40692407577).
16. **GTest is dual-sourced and the fallback is hash-unpinned.**
    vcpkg.json:6 declares gtest (manifest) while CMakeLists.txt:172-178
    falls back to FetchContent of the v1.15.2 tarball by URL with no SHA256
    — a different gtest can be compiled in fallback environments than in
    vcpkg environments.
17. **CR_BUILD_TOOLS is a dead option.** Declared (CMakeLists.txt:39) but
    referenced by nothing — no tools/ CMake targets exist; the fixture
    generator is a standalone python script (tools/gen_junk_gate_fixture.py).
18. **checks.yml lint does not mirror the full local gate (by design, but
    asymmetric in both directions).** CI-only: AST parse gate, YAML parse
    gate, M-088 doc-truth gate (checks.yml:125-174). Local-only: N-210
    conflict-marker lint (ci-local.sh:74-82), doc-link check, secrets scan,
    commit-message convention, hooks check. The old "local-mirror job runs
    ci-local.ps1 verbatim" doctrine (docs/CI-LOCAL.md:59-67) no longer holds
    — no CI job executes either ci-local script.
19. *(Observation, not a defect)* The Win7 PE gate's coverage varies by
    lane: checks.yml:232-236 + ci-local.ps1:83-85 gate 3 PEs (incl.
    cr_tests.exe); installer.yml:105-112 and release.yml:178-181 gate only
    the 2 shipped PEs — consistent with "every shipped Windows PE"
    (verify_pe_um.py:4) but worth knowing when auditing.

— Agent A, Session-84 Phase 0. End of record.

---

## Session-90 coherence addendum (the post-S89 close, pre-v0.5.3 release)

Re-verified at the session HEAD (374c54cb + this session's commits) by
the coherence lane's parallel read agents. Deltas against the
Session-84 record above:

- **Target set +1 (the only real drift):** `cr_fsck_win_tests`
  (tests/CMakeLists.txt:126-129, WIN32-only block, the M-075 chkdsk
  wiring battery) postdates this sheet; every sheet-listed target still
  exists (no reverse drift). The tests/CMakeLists.txt line references
  below the fsck block shifted by the insertion (cr_destio_tests now
  :107-115, cr_bitlocker_tests :119-122, cr_image_create_tests
  :135-141, cr_ui_tests :159, cr_ui_abi_check :177,
  gtest_discover_tests :206-207, win_syntax_check :223-226).
- **Workflows: no drift** — still the four-lane set (checks, installer,
  release, test-benchmark); triggers/paths/concurrency as recorded.
  Note for every future reader: the `branches: [main]` lines in
  checks.yml:45/:48 and installer.yml:32 DISPLAY as `branches: ain]`
  through this container's tool-output (the literal `[m` is eaten as an
  SGR reset); hex-verified `5b 6d 61 69 6e 5d` = `[main]` — the OQ-S4-3
  refutation stands, do not re-chase.
- **Warning policy: no drift** — CMakeLists.txt:20 (MSVC /W4 /WX
  ...), :32 (-Wall -Wextra -Wpedantic -Werror -Wshadow), :34 (GNU-only
  -Wno-missing-field-initializers), fuzz/Makefile:30 as recorded; zero
  new suppressions.
- **NEW gate coverage (this session, N-228):** win_syntax_check.sh now
  parse-gates every `tests/unit/test_*_win.cpp` (5 files) in addition
  to src/win + src/app + src/ui — 18/18 green at this session's HEAD;
  the win-stub SDK grew direct.h, io.h, stdio.h, string.h, stdlib.h,
  process.h, sys/stat.h + windows.h registry/BYTE/GetCurrentDirectoryW
  surface (declaration-only, never linked).
- **Version stamps:** CMakeLists.txt:4 + vcpkg.json:3 now track the
  release tag (0.5.3) — they had been frozen at 0.4.0 through the
  v0.5.x line (v0.5.2's published MANIFEST records core_version 0.4.0,
  verified first-hand).

— Session-90 coherence lane. End of addendum.

### Post-window correction (the racing S92 absorption, same session)

The racing window landed mid-session (7bd3ed8b..d0475641, 30+ commits);
this lane's addendum above was written against 374c54cb and re-verified
against the merged HEAD before the v0.6.0 cut: the fsck name-resolution
repair itself was CONCEDED (7bd3ed8b's identical one-liner won the
seat); this lane's surviving contribution is the gate extension +
stubs (dc5aac67) and the SIT concession restoration (1cc9e045, the
5ed0f07f merge had resurrected the withdrawn five tests - the build was
red at the merged HEAD first-hand). Test count at the cut: 1720/1720
(1725 with the resurrected five, minus the concession's five).
