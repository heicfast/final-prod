# CI-CURRENT — the CI/build/release/workflow state of core_recovery at HEAD `22e9657` (main)

> Task 2-d audit deliverable (2026-10-27). Scope: every workflow, every
> build lane, the release path, the driver release-pinning mechanism, staged
> test data, the win-stub parse gate, the failure-mode history, and the
> factual wipe-vs-keep classification for the planned "consume prebuilt
> releases; no from-scratch build in the correctness path" rebuild. Every
> claim carries a file:line citation at this exact tree.

## 0. Inventory (what exists)

- **Workflows (4, only 4):** `.github/workflows/ci.yml` (223 lines),
  `api-benchmark.yml` (161 lines), `comparative-bench.yml` (773 lines),
  `release.yml` (137 lines). Nothing else lives under `.github/` (no
  dependabot, no issue templates, no branch-protection config in-repo).
- **Deleted predecessors:** `benchmark.yml` (stray-brace YAML, 5 consecutive
  failed dispatches), `comparison-ours.yml`, `comparison-easeus.yml` — all
  removed in commit `144c71d` and superseded by `comparative-bench.yml`
  (worklog.md:1458; comparative-bench.yml:25-26).
- **Build graph:** root `CMakeLists.txt` (180 lines, project version 0.4.0,
  CMake ≥ 3.25, C++20, MSVC `/W4 /WX /permissive-`, gcc
  `-Wall -Wextra -Wpedantic -Werror -Wshadow` — CMakeLists.txt:19-36);
  `CMakePresets.json` (6 configure presets: `linux-gcc` (ASan),
  `linux-release`, `linux-ubsan`, `win-x64-debug`, `win-x64-release` —
  CMakePresets.json:14-65); `vcpkg.json` manifest = `zlib, gtest, lz4, zstd`
  (vcpkg.json:4-9). Translation units: `src/core` 69 cpp (cr_core STATIC,
  PIC — src/core/CMakeLists.txt:5-80), `src/win` 8 cpp (cr_win STATIC,
  MSVC-only — src/win/CMakeLists.txt:2-15), `src/app` 1 cpp (rrecov —
  src/app/CMakeLists.txt:1-5), `src/ui` 1 cpp (cr_ui SHARED —
  src/ui/CMakeLists.txt:11-46), tests 56 unit cpp + 3 win-only targets +
  2 UI targets + 1 C ABI consumer (tests/CMakeLists.txt:5-59, 88-104,
  121-141). Fuzz: 15 harnesses over a rebuild of all core TUs
  (fuzz/Makefile:36-40).
- **Zero caching anywhere:** no `actions/cache`, no ccache, in any workflow
  (grep over `.github/workflows/` finds none). Every Windows job re-runs the
  vcpkg manifest dependency build (zlib/gtest/lz4/zstd) from source; every
  Linux job re-installs apt dev packages. This is the single biggest
  per-job fixed cost.
- **Test scale:** 926 tests on the Linux lanes at this tree
  (docs/testing.md:4-5; RELEASE-NOTES.md:58-61).

>
> **POST-REBUILD STATE (driver-only session, commit 8943993+):** this sheet
> is the pre-rebuild census. comparative-bench.yml and api-benchmark.yml are
> WIPED; ci.yml is rebuilt as the validation lane (local-mirror first, one
> Windows build); driver-install.yml / core-install.yml / staged-scenarios.yml
> consume PREBUILT releases by [PREBUILT-ARTIFACTS.md](PREBUILT-ARTIFACTS.md);
> release.yml is the combined sole-source-of-builds workflow ([RELEASE.md](RELEASE.md)).
---

## 1. Workflow-by-workflow dump

### 1.1 `ci.yml` — "CI" (the correctness path)

**Triggers** (ci.yml:3-6): `push` to `main` with
`paths-ignore: ['**.md', 'docs/**', 'worklog.md', 'worklog/**', 'AUDIT*.md']`
+ **all** `pull_request`s (no path filter). Permissions: `contents: read`
(ci.yml:9-10). **No concurrency group, no caching, no job-level timeouts
except fuzz** (default 360 min applies).

**Jobs in order:**

1. **`lint`** — `runs-on: ubuntu-24.04` (ci.yml:14).
   - `clang-format check`: apt-installs `clang-format-18`, runs
     `clang-format-18 --dry-run --Werror` over every `*.h|*.hpp|*.cpp|*.c`
     under `src include tests` (ci.yml:17-21). Format spec: `.clang-format`
     (LLVM base, 4-space indent, 100-col, `SortIncludes: Never`).
   - `dead-surface absence lint (N-041)`: greps production trees for the
     deleted symbols `RdaClient::` / `class RdaClient` /
     `volume_letter_to_disk`; presence fails the run (ci.yml:23-36).
2. **`linux`** — `runs-on: ubuntu-24.04` (ci.yml:39). Steps: apt deps
   (`zlib1g-dev liblz4-dev libzstd-dev libgtest-dev`, ci.yml:43) →
   `cmake --preset linux-gcc` + `cmake --build --preset linux-gcc -j$(nproc)`
   + `ctest --preset linux-gcc` (pure core + ASan; CR_BUILD_APP=OFF per
   CMakePresets.json:20) → `tools/validate_report.py` over the CI-emitted
   report sample (G-09, ci.yml:50-56) → `tools/test_verify_pe_um.py`
   (the Win7 gate's own self-test, GAP-W6-WIN7, ci.yml:58-65).
3. **`linux-tsan`** — `ubuntu-24.04` (ci.yml:68). Raw cmake configure with
   `-fsanitize=thread` flags (NOT a preset; ci.yml:78-84), build, `ctest -j2`
   (ci.yml:85-86).
4. **`linux-ubsan`** — `ubuntu-24.04` (ci.yml:91). `linux-ubsan` preset
   (`-fsanitize=undefined -fno-sanitize-recover=undefined`,
   CMakePresets.json:53-65), build + `ctest --preset linux-ubsan -j2`
   (ci.yml:99-101). This lane exists because of finding N-018
   (ci.yml:88-90).
5. **`windows`** — `runs-on: windows-2022` (ci.yml:104). Steps:
   - *Configure + build (Debug + Release, manifest vcpkg)*: sets
     `VCPKG_ROOT=$env:VCPKG_INSTALLATION_ROOT`, then **builds BOTH
     `win-x64-debug` AND `win-x64-release` presets from source**
     (ci.yml:107-119) — the whole tree twice (~158 MSVC TU compiles + 2×
     vcpkg manifest dependency builds).
   - *Diagnostics* (`if: always()`): dumps the vcpkg installed bin trees,
     DLLs deployed next to the exes, and `dumpbin /imports` of
     `cr_tests.exe`/`rrecov.exe` — the 0xc0000135 post-mortem evidence trail
     (ci.yml:120-143).
   - *Win7 posture gate*: `python tools/stamp_osversion_um.py` rewrites PE
     OS/subsystem version to 6.1 on `rrecov.exe`, `cr_tests.exe`,
     `cr_ui.dll`; `python tools/verify_pe_um.py --allowlist
     tools/ref_imports_um.json` fails on any kernel32/advapi32 import
     outside the 27+6-function allowlist (ci.yml:144-165; allowlist at
     tools/ref_imports_um.json: kernel32 27 entries, advapi32 6, cfgmgr32 2;
     audited additions live in tools/verify_pe_um.py:31+).
   - *Tests*: `ctest --preset win-x64-release -C Release` (ci.yml:166-170)
     — runs the full gtest suite + `destio` + `bitlocker` + `cr_ui_abi_check`
     Windows-only targets (tests/CMakeLists.txt:96, 103, 152).
6. **`fuzz`** — `ubuntu-24.04`, `timeout-minutes: 30` (ci.yml:177-181).
   apt `clang zlib1g-dev liblz4-dev libzstd-dev` → `make -C fuzz all
   CXX=clang++` (**rebuilds all 69 core TUs + 15 harnesses with
   `-fsanitize=fuzzer,address,undefined -Werror`**, fuzz/Makefile:26-27)
   → `make -C fuzz run` = 45 s/target smoke, zero-crash gate
   (fuzz/Makefile:68-82) → uploads `fuzz-crash-corpus` artifact
   `if: failure()` with `retention-days: 7` (ci.yml:191-202). Re-enabled by
   the diff-to-fix session after the bench-session skip (M-006;
   ci.yml:172-178; docs/testing.md:35-43).
7. **`codeql`** — **`if: false`** (deliberately deferred, recorded not
   silent; ci.yml:204-205; rationale in docs/testing.md:44-48). Would build
   the whole tree a third time on windows-2022.

**Cost drivers:** the windows job (double full build, no cache) dominates;
each Linux sanitizer lane is a separate full 79-TU build; the fuzz job is
another full 69-TU clang build.

### 1.2 `api-benchmark.yml` — "API Benchmark" (alignbench)

**Triggers** (api-benchmark.yml:28-46): `workflow_dispatch` (input
`span_mib`, default 512); `push` to `main` **only** on paths
`src/win/**`, `src/core/storage/**`, `src/ui/**`, `include/cr/win/**`,
`include/cr/storage/**`, `include/cr/cr_ui.h`, `bench/ours/run-alignbench.ps1`,
`.github/workflows/api-benchmark.yml`; weekly `schedule: cron '0 4 * * 1'`
(the comment says "04:05" — the cron actually fires 04:00; trivial comment
drift, api-benchmark.yml:45-46). Permissions `contents: read`. **No
concurrency group, no cache.**

**Job `alignbench`** — `runs-on: windows-2022`,
`timeout-minutes: 45` (api-benchmark.yml:57-59). Steps in order:

1. `actions/checkout@v4` (:61).
2. *Configure + build (Release, manifest vcpkg)*: **full Release build of
   the whole tree from source** via `cmake --preset win-x64-release` +
   `cmake --build --preset win-x64-release` (:63-70) — this is the lane
   that validates `cr_transport.cpp`/`cr_ui.dll` on real MSVC
   (worklog.md:5227-5229).
3. *Fetch RDA driver release + verify checksums* (:72-91): queries
   `https://api.github.com/repos/heicfast/rda-driver/releases/tags/$env:RDA_RELEASE_TAG`
   with the job's `GITHUB_TOKEN`, downloads **every asset of the release**
   (rda.sys, rdacli.exe, rda.inf, SHA256SUMS, scripts/install.ps1,
   scripts/uninstall.ps1 — asset names from the driver repo's
   `.github/workflows/release.yml:80-85`), then verifies each file against
   the release's `SHA256SUMS` (`Get-FileHash` mismatch → exit 1).
4. *Test-sign + install + verify the RDA driver (service 'rda')* (:93-123):
   job-local `New-SelfSignedCertificate` (CN=RRECOV-API-BENCH), imported
   into `LocalMachine\Root` + `TrustedPublisher`, `signtool sign` on
   `driver\rda.sys`, then the release's `install.ps1 -ServiceName rda`
   with a manual `sc.exe create rda type= kernel` fallback, `sc.exe start
   rda`, then `bench\ours\verify-driver.ps1 -ServiceName rda -Device RDA
   -OutJson driver-info.json` (service RUNNING + `\\.\RDA` opens +
   GET_INFO/GET_VERSION evidence + PE FileVersion + the release's own
   rdacli reference-namespace check — bench/ours/verify-driver.ps1:1-134).
5. *Run the alignbench harness* (:125-142): prepends
   `build\win-x64-release\vcpkg_installed\x64-windows\bin` to PATH (the
   LoadLibraryW fix, run 40227375329), then `bench\ours\run-alignbench.ps1`
   against `\\.\C:` + the RDA grammar path + cr_ui.dll cheap-op latency
   (2 M iterations/op) → `alignbench.json`.
6. *Uninstall the driver* (`if: always()`, :144-150).
7. *Upload benchmark artifacts* (`if: always()`, :152-160): artifact
   `alignbench-<run_id>` = `alignbench.json` + `driver-info.json`,
   `if-no-files-found: error`.

**Build-from-source:** the entire core (Release). **Downloaded:** the
pinned rda-driver release (see §4). **Caches:** none. **Masking:** none.

### 1.3 `comparative-bench.yml` — "Comparative Bench" (the 90-job evidence machine)

**Triggers** (comparative-bench.yml:28-71): `workflow_dispatch` with inputs
`profile` (smoke|full), `tools` (ours|og|both), `fs_filter`,
`scenarios_filter`, `upload_images` (bool), `driver_mode`
(present|absent), `scan_target` (volume|disk); `pull_request` on
`bench/**` + the workflow file; `push` to main on the same paths;
`schedule: cron '0 3 * * *'` (**nightly FULL both-legs run**).

**Concurrency** (comparative-bench.yml:73-75): group
`comparative-bench-${{ github.ref }}`, `cancel-in-progress: false`.

**Global env** (:80-82): `RDA_RELEASE_TAG: 'v0.2.1'` (⚠ **one version
behind api-benchmark's v0.2.2**) and `RESEARCH_REPO:
'heicfast/easeus-drw-research'`.

**Jobs:**

1. **`prepare`** — `ubuntu-latest` (:85-170). Pinned-SHA actions
   (`actions/checkout@11d5960a326750d5838078e36cf38b85af677262 # v4`).
   Steps: *Parse-check the bench PS scripts* via `scripts/ps_parsecheck.ps1`
   (the F4 lesson: bench-script defects must fail BEFORE the matrix
   dispatches; scripts/ps_parsecheck.ps1:1-25) → *Compute matrix*: PR/push
   force `PROFILE=smoke, TOOLS=ours`; schedule forces `full/both`;
   full = `["ntfs","exfat","fat32","refs"]` × 12 scenarios
   (`recycle_bin, permanent_delete, unlink_native, folder_delete,
   quick_format, full_format, delete_partition, decay_50, decay_80,
   decay_100, partial_overwrite, mft_corrupt`) with `mft_corrupt`
   NTFS-only → **45 fs×scenario pairs per leg** (:125-145); outputs
   `ours_matrix`/`og_matrix`/counts/flags → *Upload skip reasons* artifact
   `bench-skip-reasons` (90 d, :165-170).
2. **`bench-ours`** — matrix over the pairs, `runs-on: windows-2022`,
   `timeout-minutes: full?240:150`, `fail-fast: false` (:173-182). Steps:
   checkout (pinned SHA) → **Build rrecov (Release) from source**
   (`cmake --preset win-x64-release`, `-j 2`, :188-198) + rrecov SHA256
   recorded → *Fetch RDA driver release (v0.2.1) + verify checksums*
   (only `driver_mode == present`; same release-fetch + SHA256SUMS
   verification as api-benchmark, :200-222; also `driver-sha.txt`) →
   *PE provenance* (`python bench\tools\pe_info.py` over rda.sys +
   rrecov.exe, :224-238) → *Test-sign + install + verify the RDA driver*
   (:240-275; same cert dance + install.ps1 + sc.exe fallback +
   verify-driver.ps1) → *Run the scenario (ours)* (:277-374): builds a
   fresh VHDX volume per job on `C:\bench` (1.5 GiB smoke / 3 GiB full; 5
   GiB for delete_partition), seeds via
   `bench\seed\generate_seed.ps1`, applies the deletion scenario via
   `bench\scenarios\apply-scenario.ps1`, runs
   `bench\ours\run-ours.ps1` (quick/deep/deepcarve + `recover --report`),
   inventories + computes metrics per mode (`bench\metrics\inventory_recovered.py`,
   `compute_metrics.py`), optionally zips the VHDX (`upload_images`), cleans
   up in `finally` → *Cleanup driver (always)* (:376-387) → *Upload ours
   artifacts (90 d)*: `bench-ours-<fs>-<scenario>` (benchmark-results JSONs/
   CSVs/logs + rrecov-sha.txt + driver-sha.txt + driver-pe.json,
   :389-409).
3. **`ours-behavior`** — single windows-2022 job, 90 min timeout
   (:412-461). Builds rrecov (Release, from source) → driver-absent probe +
   cancellation + resume-equivalence probes via
   `bench\ours\run-behavior.ps1` against a 1.5 GiB NTFS VHDX → uploads
   `ours-behavior` artifact (90 d).
4. **`bench-og`** — matrix over the same pairs, windows-2022,
   `timeout-minutes: full?240:180` (:464-477). Steps: checkout core (pinned
   SHA) → **checkout the research corpus** `heicfast/easeus-drw-research`
   sparse (`og-codebase` only, :479-485) → record og build identity
   (version.ini + DRWUI.log head + `pe_info.py` over DRW.exe/DRWUI.exe,
   :487-494) → *Run the scenario (og)* (:496-592): same volume/seed/
   scenario machinery, then drives the og **UI by mouse-coordinate
   injection** (`bench\og\run-og.ps1` + `bench\og\og-driver.ps1`), harvests
   the auto-saved `.rsf` (parsed by `scripts/compare_og_rsf.py`), runs the
   Recover-All funnel with the 500 MB free-cap gate; cleanup kills
   DRWUI/DRW + the og's own `eprdtdrv` service if installed (:594-608) →
   uploads `bench-og-<fs>-<scenario>` artifacts (90 d, :610-629). **No
   build of ours; no rda-driver install on this leg.**
5. **`compare`** — `ubuntu-latest`, `needs: [prepare, bench-ours,
   bench-og, ours-behavior]`, `if: always() && !cancelled()`,
   `permissions: contents: write` (:632-637). Downloads all leg artifacts
   (pinned-SHA download-artifact), reorganizes them, records missing-leg
   reasons (:641-709), builds `docs/COMPARATIVE-BENCHMARK.md` +
   `docs/benchmark-summary.json` via `bench/metrics/compare.py`
   (:711-722), runs the regression gate against `bench/baseline.json`
   (`bench/metrics/regression_gate.py`, full profile requires all baseline
   scenarios, :724-737), writes the job summary (:739-749), **commits the
   report to main** via bench-bot on dispatch+full runs (:751-760), uploads
   `comparative-report-<run_label>` (90 d, :762-772).

**Masking policy: NO `continue-on-error` anywhere** (header comment
:21-23). **One scenario per job** so a per-job timeout can only lose that
scenario (:10-12).

**Estimated cost:** nightly full both-legs = 45 (ours) + 45 (og) + 1
(behavior) + 2 (prepare/compare) = **93 jobs**, 90 of them on
windows-2022 with per-job full Release builds (ours leg) — plus the
`upload_images` option adding VHDX zips to artifacts. PR/push runs are
smoke = 3 ours jobs + behavior + prepare + compare ≈ 6 jobs. **No caching
anywhere** — each ours-leg job rebuilds the full tree + vcpkg deps.

### 1.4 `release.yml` — "Release"

**Trigger** (release.yml:3-5): `push: tags: ['v*']`. Permissions
`contents: write` (:7-8). Single job `release`, `runs-on: windows-2022`
(:11-12). Steps:

1. *Build (Release, vcpkg manifest)*: **full Release build + full test run
   from source** — `cmake --preset win-x64-release`, `cmake --build`,
   `ctest --preset win-x64-release -C Release` (:15-24).
2. *Win7 posture gate on every shipped PE*: stamp + verify `rrecov.exe`
   and `cr_ui.dll` against the import allowlist (:26-42).
3. *Stage the complete SDK + CLI bundle* (:44-131): existence-checks then
   stages `bin/rrecov.exe`, `bin/cr_ui.dll`, `lib/cr_ui.lib`,
   `lib/cr_core.lib`, `lib/cr_win.lib` (:57-71); optional PDBs when
   present (:73-78); vcpkg runtime DLLs (z/zstd/lz4) minus gtest/gmock
   (:80-91); VC143 CRT redist DLLs from the VS layout (:93-97); UCRT
   `ucrtbase.dll` + `api-ms-win-crt-*.dll` selected **by content** from
   three candidate SDK layouts (Win7 RTM ships no UCRT; :99-112);
   `include/`, `docs/`, README/LICENSE/RELEASE-NOTES (:114-117); then a
   `SHA256SUMS` integrity manifest over every staged file (:124-129) and
   `Compress-Archive` → **`core-recovery-<tag>.zip`** (:131).
4. *Create GitHub release* (:133-137): `softprops/action-gh-release@v2`
   with `files: core-recovery-${{ github.ref_name }}.zip` and
   `body_path: RELEASE-NOTES.md`.

**Signing: none.** The release PEs are *stamped* to OS version 6.1 and
import-gated, but there is no Authenticode signing step in the workflow
(driver releases are likewise consumed test-signed in CI). Checksums: the
in-zip `SHA256SUMS` file (release.yml:124-129); GitHub's per-asset digests
come implicitly from the release page. Release notes: the repo-root
`RELEASE-NOTES.md` (currently titled `core_recovery v0.4.1-parity`,
RELEASE-NOTES.md:1).

**Existing tags:** `v0.1.0`, `v0.2.1-parity`, `v0.3.0-parity`,
`v0.4.0-parity`, `v0.4.1-parity` (`git tag -l`). `v0.4.1-parity` is an
annotated tag on HEAD `22e9657`. **Naming convention: `vX.Y.Z` or
`vX.Y.Z-parity`.** The release path was hardened through five fix
commits (545c838 → 2140eb4; UCRT layout moves, parse errors, PDB honesty,
no test-framework DLLs — git log for `.github/workflows/release.yml`).

---

## 2. Dependency graph (what consumes what)

```
rda-driver releases (github.com/heicfast/rda-driver)
  └─ v0.2.2  ← api-benchmark.yml      (env RDA_RELEASE_TAG, api-benchmark.yml:51-54)
  └─ v0.2.1  ← comparative-bench.yml  (env RDA_RELEASE_TAG, comparative-bench.yml:80-81)
       both: REST fetch of ALL release assets with GITHUB_TOKEN,
       SHA256SUMS-verified, signtool test-signed with a job-local cert,
       install.ps1 (sc.exe fallback) → service 'rda' → verify-driver.ps1
       (driver-info.json evidence) → run → sc.exe stop/delete cleanup.
       NO workflow ever clones/builds the rda-driver repo itself.

easeus-drw-research (og corpus)
  └─ comparative-bench bench-og leg only: sparse checkout of og-codebase/
     (comparative-bench.yml:479-485) to drive the og UI + harvest .rsf.
     (bench-og runs the ORIGINAL EaseUS binaries; ours is not built there.)

core_recovery self-builds (from source, every time):
  ci.yml windows (Debug+Release), ci.yml linux/tsan/ubsan (core+tests),
  ci.yml fuzz (clang, all core TUs), api-benchmark (Release),
  comparative-bench bench-ours + ours-behavior (Release, per matrix job!),
  release.yml (Release + ctest).

Staged fixtures (committed, no download):
  tests/data/images/qemu-disk1.{vhd,vhdx,vmdk} + SHA256SUMS
  tests/data/exfat/upcase-table-mkfs-exfat-1.2.9.bin
  fuzz/corpus/pure/enum-attrs-window-overrun-513.seed

Runtime (production) dependency — not a build dep:
  the shipped core opens \\.\RDA\<token>\<disk>\<part> at RUNTIME with
  service name "RDA" (docs/CONNECTION-GUIDE.md:77, :160-161); if the
  driver is absent it falls back to direct \\.\PhysicalDrive handles
  (DRIVER-UTILIZATION.md §1.9). So the release zip ships NO driver.
```

## 3. The driver release-pinning mechanism (exact mechanics)

Authoritative doc: **docs/DRIVER-UTILIZATION.md:241-252** ("Driver release
pinning (2026-10-26 addendum)"):

- `api-benchmark.yml` `RDA_RELEASE_TAG` is **v0.2.2** "as of the
  v0.4.1-parity tree (previously v0.2.1)" — the actual variable sits at
  **api-benchmark.yml:54** inside `env:` with the comment "v0.2.2 = the
  driver's current release (the sc.exe installer fix). Functional floor is
  >= v0.2.1 (the error-87 fix); pinned to the newest."
  (**comparative-bench.yml:81 still pins v0.2.1** — the addendum's "the
  bench lane's driver pin" move was applied only to the API-benchmark
  lane; the comparative lane was left at v0.2.1.)
- Functional floor: **≥ v0.2.1**; v0.2.0 fails every bare `\\.\RDA` open
  with error 87 (driver classifier ordering bug, fixed in the driver's
  `7e88e38`), which would silently disable the whole RDA transport via
  `rda_available()` (DRIVER-UTILIZATION.md:246-249).
- Wire contract is version-decoupled (driver PE/wire version 1.0.0.0 by
  design); per-run verification = service install + device open recorded
  in `driver-info.json` (DRIVER-UTILIZATION.md:250-252).
- Pin history in commits: `095aab5 fix(warnings): ... api-benchmark pins
  rda-driver v0.2.2` (v0.2.1 → v0.2.2); the addendum landed in `22e9657
  docs(v0.4.1-parity): ... DRIVER-UTILIZATION release-pinning addendum
  (v0.2.2)`; the pin mechanism itself was created in `a4053c4
  feat(transport): ... Windows CI alignbench` (worklog.md:5213-5215:
  "rda-driver release fetch+SHA verify+test-sign+install+verify-driver").
- How verified per run (identical code in both workflows): release assets
  downloaded via the GitHub REST API with `secrets.GITHUB_TOKEN`
  (api-benchmark.yml:75-80; comparative-bench.yml:204-209), each verified
  against the release's own `SHA256SUMS` (api-benchmark.yml:81-90;
  comparative-bench.yml:211-220), then `bench\ours\verify-driver.ps1`
  proves RUNNING service + `\\.\RDA` open + GET_INFO/GET_VERSION + PE
  FileVersion + the release's rdacli integration check
  (bench/ours/verify-driver.ps1:66-134).
- Driver release asset names (from the driver repo's release workflow):
  `rda.sys`, `rdacli.exe`, `rda.inf`, `SHA256SUMS`, `scripts/install.ps1`,
  `scripts/uninstall.ps1`.

## 4. Release state summary

- Trigger: tag push `v*` (release.yml:3-5). Asset: exactly one zip —
  `core-recovery-<tag>.zip` (release.yml:131, 136) containing `bin/`
  (rrecov.exe, cr_ui.dll, vcpkg + VC CRT + UCRT runtime DLLs), `lib/`
  (cr_ui.lib, cr_core.lib, cr_win.lib), `include/`, `docs/`,
  README/LICENSE/RELEASE-NOTES, `SHA256SUMS`. Created by
  `softprops/action-gh-release@v2` with `body_path: RELEASE-NOTES.md`
  (release.yml:133-137). No signing; no separate checksum asset; no
  draft/prerelease flags. Tag convention: `vX.Y.Z[-parity]`.

## 5. Staged test-data inventory (committed fixtures)

`tests/data/images/` (S-030 real-world image fixtures; README.md:1-19):

| File | Size | SHA256 (SHA256SUMS:1-4) | Consumed by |
|---|---|---|---|
| `qemu-disk1.vmdk` | 262,144 B | `20d43ec7b2339734db70b81e8ff3b335b8b55b8980b00a7856e831786e84bc6d` | `Image.Qemu*Compat` tests |
| `qemu-disk1.vhd` | 4,197,888 B | `ff57c7e0e9e71f9e8a3ece7f1112de1db85370d0a2315d3598fff12be3d86ba2` | `Image.Qemu*Compat` tests |
| `qemu-disk1.vhdx` | 16,777,216 B | `93097d82d18bcfd1aa6b48cb13ebee5142452bdbe8cf91150246162e1c3b9dfa` | `Image.Qemu*Compat` tests (incl. the post-close log entry path) |

Generated by `qemu-img/qemu-io 10.0.13`: 4 MiB virtual disks carrying
0xAB @0..64K, 0xCD @1MiB..+64K, 0xEF @3MiB..+8K, zeros elsewhere
(README.md:8-19). Consumers: `tests/unit/test_image.cpp` —
`load_fixture("data/images/<name>")` with `GTEST_SKIP` if absent
(test_image.cpp:830, 891-941); the images are POST_BUILD-copied next to
the test binary (tests/CMakeLists.txt:65-68). Additional staged data:
`tests/data/exfat/upcase-table-mkfs-exfat-1.2.9.bin` (consumed by
tests/unit/test_exfat_volume.cpp, the N-067 upcase validation) and
`fuzz/corpus/pure/enum-attrs-window-overrun-513.seed`. **No Windows disk
images with real filesystems are staged** — the comparative-bench harness
creates its VHDX volumes live on the runner (bench/volume/volume.ps1).

## 6. The win-stub mechanism (Linux unit tests faking Windows APIs)

Files: `tests/win-stub/{windows.h,winioctl.h,cfgmgr32.h,psapi.h,
shlwapi.h,share.h,ntddscsi.h}` (7 declaration-only headers; windows.h is
446 lines). Driver: `tools/win_syntax_check.sh`, wired as the ctest
`win_syntax_check` **only on non-Windows, non-Clang CMake builds**
(tests/CMakeLists.txt:184-188).

Mechanics (tools/win_syntax_check.sh:47-78):

```
g++ -std=c++20 -fshort-wchar -D_WIN32 -D__declspec(x)= -fsyntax-only \
    -I tests/win-stub -I include   <each src/win/*.cpp, src/app/*.cpp, src/ui/*.cpp>
```

- `-fshort-wchar`: wchar_t is 16-bit like MSVC/x64; `-D_WIN32` makes the
  `#ifdef _WIN32` branches parse as MSVC sees them;
  `-D__declspec(x)=` neutralizes the ELF-illegal storage class (parse
  only, never linked — win_syntax_check.sh:32-46).
- The stubs **declare, never link or run**: inline bodies exist only so
  TUs parse; shapes are SDK-faithful (ULONG_PTR width, OVERLAPPED,
  STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR field order "load-bearing for the
  512e/4Kn geometry probe" — winioctl.h:1-8; ATA_PASS_THROUGH_EX
  DataBufferOffset ULONG_PTR-width load-bearing — ntddscsi.h:1-10).
  Stubbed surface: CreateFileW/ReadFile/WriteFile/GetOverlappedResult,
  device/volume/registry/process/token APIs, critical sections, MSVC
  secure CRT (`_wfopen_s`, `swprintf_s`, ...), cfgmgr32 (N-071), psapi
  (N-062 working-set sample), shlwapi `PathIsNetworkPathW` (N-013),
  share.h `_SH_*` (S-008), ntddscsi ATA passthrough.
- Failure mode = the gate working: **a new SDK symbol consumed by src/win
  grows the stub or the Linux suite goes red** (windows.h:14-17; M-055
  rationale in tests/CMakeLists.txt:172-183 — two win-layer breaks
  survived multiple pushes before this gate existed).

**Implication for cost:** structural/parse correctness of the whole
Windows-only layer (win/app/ui) is validated **on cheap Linux runners**;
what genuinely needs a Windows runner is (a) MSVC real compilation
(`/W4 /WX`), (b) the Win7 PE import gate + stamping, (c) the
destio/bitlocker runtime contract suites, (d) real CreateFile/IOCTL
behavior, (e) the RDA grammar path end-to-end.

## 7. Failure-mode history (top recurring CI failure classes)

From worklog.md (17 k lines of session history) + git log:

1. **MSVC-only compile reds pushed from Linux lanes** (the dominant
   class): `gmtime_r` is POSIX-only (MSVC: `gmtime_s`), C4244 narrowing
   casts under `/W4 /WX`, missing `<algorithm>` for `std::count_if`
   (MSVC has no transitive includes), missing `psapi.h`/`CR_SOURCE_DIR`
   define, `u16string→wstring` widening, C2065 undeclared identifiers —
   worklog.md:16652-16667 ("the parallel window's 89196ac push left main
   CI RED (3 consecutive failed runs): windows build (gmtime_r + two
   C4244 narrowings)…"), worklog.md:17011, git commits `0e6a642`,
   `be0150e`, `251b2dc`, `3577bab`.
2. **0xc0000135 DLL-not-found at test startup** (three CI cycles to root
   cause): pkg-config/find_library release-bias made MSVC Debug link the
   release import lib → exe imports `liblz4.dll` while the debug tree only
   has `liblz4d.dll`; applocal's silent name-skip + startup = death —
   worklog.md:421-449 (the full postmortem), root CMakeLists.txt:62-77
   discovery-order comment. Countermeasures now in-tree: CONFIG-first
   discovery + `cr_deploy_vcpkg_binaries` POST-BUILD backstop
   (CMakeLists.txt:150-167; src/app/CMakeLists.txt:4-5) + the
   `if: always()` diagnostics step (ci.yml:120-125).
3. **clang-format lint drift from parallel lanes** — recurring whole-tree
   format sweeps (62 violations at worklog.md:5827-5829; 91 files at
   `f7b62a6`; 106 files at worklog.md:16665-16666). The lint gate
   (ci.yml:17-21) is the most frequently red lane.
4. **Committed merge-conflict markers / semantic clobber from parallel
   merge storms**: the `5aaf044 "style(lint)"` commit that was actually a
   four-finding semantic rollback (worklog.md:8870-8910); committed
   conflict markers in nvramfix/bootfix (`c830743`, `b95de7d`); the
   reverted `stopped_early` API (`2fb7035`). Lesson recorded: "never
   absorb a style/lint commit without diffing it" (worklog.md:8903-8905).
5. **Fuzz-lane build breaks**: `fuzz_image.cpp`'s map-vs-set API drift
   (`insert` on a `map<u64,u64>`), `-Winconsistent-missing-override`,
   the 48-warning clang backlog (closed by `095aab5`, which added
   `-Werror` to fuzz/Makefile:26-27).
6. **Windows CI integration failures in the bench stack** (13 root-caused
   iterations for comparative-bench alone — worklog.md:1459): PowerShell
   parse errors ([byte[]]() empty parens, ordered-cast, `$VAR:`
   interpolation, op_Multiply inside array literals), CreateProcessW
   charset, D: disk headroom (dynamic-VHDX growth failure → moved to
   `C:\bench`), FSCTL_LOCK GLE=5 retry, LoadLibraryW module-not-found for
   cr_ui.dll's vcpkg runtime (api-benchmark PATH fix,
   api-benchmark.yml:130-134). Mitigation now standard: the pre-dispatch
   `ps_parsecheck.ps1` gate (comparative-bench.yml:101-106).
7. **Broken workflow YAML itself**: the old `benchmark.yml` died of a
   stray brace (5 consecutive failed dispatches) before deletion
   (worklog.md:1458); release.yml needed four fix rounds for UCRT redist
   layout changes (commits 545c838→2140eb4).
8. **Sanitizer-only reds**: TSan race in the test event recorder
   (worklog.md:16667), ASan/LSan leaks escaping local runs made with
   `detect_leaks=0` (worklog.md:5822-5826 — "always run the ASan lane
   with leaks ON before pushing memory-touching fixes").

## 8. Commit-message convention

`type(scope): lowercase imperative subject` — long, evidence-dense,
often multi-clause with `+`/`;` separators; body explains why + proof.
Types in live use: `fix`, `repair`, `docs`, `worklog`, `bench`, `build`,
`ci`, `style`, `audit`, `convergence`, `merge` (plain), plus occasional
`feat`/`perf` (from earlier sessions). Scopes: subsystem or finding-ID
(`warnings`, `windows tests`, `win7-gate`, `Windows CI`, `MSVC`,
`CI red`, `N-041`, `M-040 stages b+win+wiring`, `S-034`, `release`,
`v0.4.1-parity`, `the overhaul`). Ten verbatim examples:

1. `095aab5 fix(warnings): the clang fuzz lane's 48-warning backlog zeroed + the lane gets -Werror; S-023 merge-storm duplicate deleted; api-benchmark pins rda-driver v0.2.2`
2. `394a102 fix(windows tests): the advisory clean-state pin was Linux-contract-only; the S-014 auto-unlock enumeration SKIPPED oversized Data values`
3. `23739b2 fix(win7-gate): allowlist K32GetProcessMemoryInfo - the N-062 memory monitor's kernel32 binding`
4. `7a76c86 fix(Windows CI): cr_ui_tests' direct core-API uses need the static core on MSVC`
5. `5077afc repair(CI layer 3): the fuzz smoke's ASan crash + the src/ui parse-gate gap`
6. `68580e6 repair(CI red): the four failing gates fixed — lint format sweep (incl. the zfs/test pair), fuzz_image's never-compiling map insert, MSVC portability (gmtime_r) + C4244 casts, and the TSan race in the test event recorder`
7. `be0150e fix(MSVC): std::count_if needs <algorithm> — MSVC has no transitive include (test_refs_volume.cpp:1490, C2039/C3861 on the Windows lane; gcc's transitive include hid it)`
8. `22e9657 docs(v0.4.1-parity): the OQ-S4-3 transport verdict transcribed (alignbench 40315255368: core 64 KiB envelope meets og 4/7 MiB chunking, 256 KiB beats it ~19%); CONTRIBUTING + SECURITY created; CONNECTION-GUIDE host-projects section; DRIVER-UTILIZATION release-pinning addendum (v0.2.2); SUMMARY Session-38 delta; RELEASE-NOTES retitled v0.4.1-parity`
9. `6552322 fix(M-040 stages b+win+wiring): the UEFI NVRAM Boot#### updater + the bootfix win runtime + the post-apply wiring — the boot-chain family completed`
10. `merge: union with origin/main — ours for source, keep-both for append-only docs`

CONTRIBUTING.md:54-60 states it as policy: "Evidence-citing subject lines
in the imperative, lowercase family … with the body explaining the *why*
and the proof."

## 9. Branch protection / CI expectations (documented)

- CONTRIBUTING.md:22-34: "Tests and gates (all must be green before a
  push)" — `ctest --preset linux-gcc` + `tools/win_syntax_check.sh`
  locally; CI per push runs clang-format-18, linux + linux-tsan +
  linux-ubsan + windows MSVC lane, the libFuzzer campaign, and the Win7
  PE import gate; CodeQL present but deferred.
- README.md:124-131 repeats the same matrix as the repo's public CI
  contract ("CI runs the full matrix on every push…").
- No branch-protection / required-checks config exists in-repo (that
  lives in GitHub repo settings, not visible here); nothing in
  docs/roadmap.md or docs/requirements.md adds CI requirements beyond
  the non-functional build/test matrix (docs/requirements.md:94-101).
- Masking policy is an explicit norm: no `continue-on-error`
  (comparative-bench.yml:21-23; api-benchmark.yml:21-22); skips are
  recorded, never silent (`if: false` on codeql is "recorded, not
  silent" — ci.yml:205; docs/testing.md:44-48).

## 10. WIPED vs KEPT under "no from-scratch build in the correctness path; consume prebuilt releases"

Factual classification of every from-source build site:

**Builds the whole core from source (candidates to restructure):**

| Site | What it builds | Notes for the rebuild |
|---|---|---|
| ci.yml `windows` job | Full tree, **Debug + Release** (ci.yml:107-119) + ctest | The correctness path's only true Windows compile+test lane. Debug config exists only to catch config-mapping DLL bugs (the 0xc0000135 class); its retention under the new policy is a decision point. |
| ci.yml `linux`/`linux-tsan`/`linux-ubsan` | Core + tests ×3 sanitizer flavors | Cheap Linux lanes; the win layer is parse-gated, not compiled (win-stub). Natural KEEP as the fast correctness path. |
| ci.yml `fuzz` | All core TUs + 15 harnesses (clang) | Separate build system (Makefile); zero-crash gate. |
| api-benchmark.yml | Full Release build per run (:63-70) | **Benchmark lane that builds from source on every transport-touching push + weekly cron.** Under the policy this is a prime wipe candidate: consume a prebuilt rrecov/cr_ui release artifact instead (it already consumes the *driver* as a pinned release). |
| comparative-bench.yml `bench-ours` | Full Release build **per matrix job** — up to 45 jobs nightly + PR/push smoke (:188-198) | **The biggest aggregate build cost in the repo** (45× nightly full MSVC builds, no cache). Prime wipe candidate: one build artifact (or release download) fanned out to the matrix. |
| comparative-bench.yml `ours-behavior` | Full Release build (:420-427) | Same as above. |
| release.yml | Full Release build + ctest (the release gate) (:15-24) | **KEEP — this IS the release path.** It must build from source once to produce the tagged artifact; it also runs the full test suite + Win7 gate + staging + SHA256SUMS. |

**Already consumes prebuilt releases (KEEP as-is — the pattern to extend):**

- api-benchmark.yml: rda-driver **v0.2.2** release (fetch + SHA256SUMS
  verify + test-sign + install + verify-driver; :51-54, :72-123).
- comparative-bench.yml bench-ours: rda-driver **v0.2.1** release (same
  mechanics; :80-81, :200-275) — should be moved to v0.2.2 to match the
  documented floor/newest pin.
- comparative-bench.yml bench-og: the og EaseUS binaries from the
  research corpus (no build at all).
- The `prepare` job's `ps_parsecheck.ps1` gate (no build, catches the
  F4 class before dispatch).

**Test data / fixtures (no builds, no downloads — all committed):**
qemu image fixtures + SHA256SUMS, exFAT upcase table, fuzz seed — all
KEEP. The bench volumes are synthesized live per-job (VHDX creation on
the runner) — reproducible by construction, nothing to pin.

**Net classification:** under the stated policy the correctness path
(ci.yml) keeps its from-source builds (that is its purpose: compile +
sanitize + test the tree under change); the *benchmark* workflows
(api-benchmark, comparative-bench ours legs) are the from-scratch builds
that can be wiped in favor of consuming prebuilt core release artifacts,
exactly as they already consume prebuilt driver releases; the release
workflow stays as the single from-source production build. The driver
release-pinning mechanism (§3) is the proven template: pinned
`RDA_RELEASE_TAG` env + full-asset SHA256SUMS verification + per-run
service/device verification, duplicated today in two workflows (api-benchmark.yml:51-54/72-123 and comparative-bench.yml:80-81/200-275) — a shared composite action or reusable workflow is the obvious dedup point.

## 11. Noted drift / surprises (factual, for the rebuild plan)

- **Pin skew:** comparative-bench pins driver v0.2.1 while
  api-benchmark pins v0.2.2 and docs/DRIVER-UTILIZATION.md:244 describes
  only the api-benchmark move ("`api-benchmark.yml` `RDA_RELEASE_TAG` is
  v0.2.2 … previously v0.2.1") — the comparative lane is the stale one.
- **docs/benchmarking.md §2/§3 still documents the DELETED workflows**
  (`.github/workflows/benchmark.yml`, `comparison-ours.yml`,
  `comparison-easeus.yml`) and a v0.2.0 driver pin
  (docs/benchmarking.md:24-104) — pre-comparative-bench harness doc, a
  registered N-020-class drift.
- **No caching at all** in any workflow (§1) — every Windows job pays the
  vcpkg manifest build + full MSVC compile; the ours-leg matrix pays it up
  to 45× per nightly run.
- api-benchmark's cron comment says "04:05 UTC" but the cron fires 04:00
  (api-benchmark.yml:45-46).
- `bench/baseline.json` is currently an empty scenarios map (baseline.json:1-5)
  — the regression gate has nothing to enforce against yet;
  `baseline-candidate.json` holds the first candidate awaiting human
  review (worklog.md:1464).
- The nightly comparative-bench full run was repeatedly cancelled by an
  **external canceller** during the bench session (worklog.md:1465) — an
  environmental hazard recorded, not worked around.
- ci.yml `pull_request` has **no path filter** (ci.yml:7) while push has
  one — every PR runs the full matrix including the double Windows build.
