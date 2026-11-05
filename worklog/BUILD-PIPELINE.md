# BUILD-PIPELINE — the full build/CI/installer/release survey of final-prod @ a6cdead9

Agent C (T2-c), read-only survey. Every claim cites `file:line` from the tree at
commit `a6cdead9df281d6cd31144fbdb8a6ccb4dff8a76` (`VERSION` = `0.6.0-rc2`,
VERSION:1). Purpose: the kernel driver (`driver/`, 63 files, service `RDA`,
`rda.sys`) is being deleted; `engine/` advances to core_recovery@12f3edf7
(user-mode direct raw access, `ERR_ELEVATION_REQUIRED`), `ui/` is replaced by
recovery-wizard-ui-new@dd2782c. This document is the complete driver-touchpoint
map so the pipeline can be reborn driver-free.

Action-pinning note (applies to every workflow): **no action is SHA-pinned**.
All 52 `uses:` references are tag-pinned (`git grep "uses:" -- .github/`):
`actions/checkout@v4` (17), `actions/setup-node@v4` (6), `actions/upload-artifact@v4`
(10), `dtolnay/rust-toolchain@stable` (4), `ilammy/msvc-dev-cmd@v1` (2),
`microsoft/setup-msbuild@v2` (4), `nuget/setup-nuget@v2` (4), `pnpm/action-setup@v4`
(4), `softprops/action-gh-release@v2` (1). There is no `actions/download-artifact`
anywhere (0 matches) — no workflow consumes another's artifacts; uploads are
evidence only.

---

## 1. Workflow-by-workflow analysis

### 1.1 contracts-ci.yml (43 lines)

- **Trigger**: push to `main` filtered on `contracts/**`, `engine/include/**`,
  `ui/src/connector/**`, `ui/src-tauri/src/**`, `ui/scripts/gen-filefilter.mjs`,
  the workflow itself (contracts-ci.yml:8-16); pull_request on the same paths
  minus the workflow file (:17-23).
- **Permissions**: `contents: read` (:25-26). No concurrency block, no timeout,
  no caches, no artifacts.
- **Job `validate`**, `runs-on: ubuntu-24.04` (:29-30):
  1. `actions/checkout@v4` (:32)
  2. `actions/setup-node@v4` with `node-version: 24` (:34-36)
  3. "Boundary-truth validation (validate.mjs, phase 1 + 2)" — `node contracts/validate.mjs` (:38-39)
  4. "filefilter mirror freshness" — `node ui/scripts/gen-filefilter.mjs --check` (:41-42)
- **Driver touchpoints**: none in the workflow body. The *validated data* is
  driver-bound: `contracts/command-surface.json:29` (`system_driver_status` →
  "SCM probe + \\.\RDA open + IOCTL GET_VERSION"), `contracts/error-codes.json:65-70`
  (`ERR_DRIVER_REQUIRED` / uiCode `driver-required`),
  `contracts/type-maps.md:207-209` (`DriverStatus` = SCM service `RDA` exists /
  `\\.\RDA` opens), `contracts/README.md:47` (service-name constant `RDA`).

### 1.2 driver-ci.yml (208 lines) — 100% driver; DELETE-FILE

- **Trigger**: push to `main` on `driver/**`, `installer/devsign.ps1`, the
  workflow (driver-ci.yml:7-12); pull_request on `driver/**` +
  `installer/devsign.ps1` (:13-16). Note: devsign.ps1 changes trigger this lane.
- **Permissions**: `contents: read` (:18-19). **Defaults**: `working-directory: driver` (:21-23).
- **Concurrency** (the only workflow with one): group
  `${{ github.workflow }}-${{ github.ref }}`, `cancel-in-progress: true` (:25-27).
- **Job `lint`** (`ubuntu-latest`, :30-57): checkout@v4 (:34); clang-format
  `--dry-run --Werror` over `src/**/*.c|h` via apt clang-format (:36-42);
  markdown trailing-newline sanity (:44-52); portable contract-layer unit tests
  `scripts/run-unit-tests.sh` with apt gcc (:54-57).
- **Job `fuzz`** (`ubuntu-latest`, :59-79): libFuzzer grammar/parser round
  (`make -C fuzz fuzz` + `run`, clang, :65-71); invariant selftest
  (`make -C fuzz selftest`, :73-75); unit tests under ASan/UBSan
  (`make -C fuzz unit`, :77-79).
- **Job `unit`** (`windows-2022`, :81-102): checkout@v4 (:85);
  `ilammy/msvc-dev-cmd@v1` arch x64 (:87-90); MSVC `/W4 /WX /wd4127 /std:c11
  /external:W0 -I src\shared\inc tests\unit\test_compat.c` → `bin\test_compat.exe`,
  then run it (:92-102).
- **Job `build`** — matrix `runner: [windows-2022]`, `wdk: [26100.6584]`,
  `tag: [wdk26100]`, `configuration: [Debug, Release]`, `fail-fast: false`
  (:104-113):
  1. checkout@v4 (:115)
  2. `nuget/setup-nuget@v2` (:117-118); `nuget restore .\packages.config
     -PackagesDirectory .\packages\` — the pinned WDK 10.0.26100.6584 (:120-122)
  3. `microsoft/setup-msbuild@v2` (:124-125); `msbuild .\rda.sln -m -v:minimal
     -p:Configuration=<matrix> -p:Platform=x64` (:127-134)
  4. "Stamp Win7 loader contract" — `python tools\stamp_osversion.py
     bin\x64\<cfg>\rda.sys` (:136-141)
  5. "Win7 import/PE gate" — `python tools\verify_pe.py <rda.sys>` (:143-149)
  6. "Verify unsigned artifact" — raw PE parse asserting the security-directory
     RVA is zero (SignMode stays Off; no accidental build-time signing) (:151-163)
  7. "Dev-sign smoke leg" — copies `rda.sys` to `$RUNNER_TEMP\devsign-smoke`,
     runs `installer\devsign.ps1 -Action new-cert|sign|verify` on the copy, then
     asserts the signed copy's PE security directory is NON-zero (:173-198).
     Proves the packaging-time signature path on every driver push.
  8. `actions/upload-artifact@v4` name `rda-<tag>-<configuration>` (so
     `rda-wdk26100-Debug` / `-Release`), paths `driver/bin/x64/<cfg>/rda.sys`,
     `rdacli.exe`, `driver/src/driver/rda.inf`, `if-no-files-found: error`,
     default retention (:199-207). **Never downloaded by anything.**
- **Fate**: entire file is driver-only → [DELETE-FILE]. Its dev-sign smoke leg
  is the only CI proof of devsign.ps1's `sign` action; that proof moves to the
  release lane (release.yml:92-95 already verifies signed artifacts).

### 1.3 engine-ci.yml (183 lines) — lane survives; one driver-era lint inside

- **Trigger**: push `main` on `engine/**` + workflow; PR `engine/**` (:9-17).
  `contents: read` (:19-20); defaults `working-directory: engine` (:22-24). No concurrency.
- **Job `lint`** (`ubuntu-24.04`, :27-45): clang-format-18 over src/include/tests
  (:31-35); **"dead-surface absence lint (N-041)"** greps production code for
  `RdaClient::`, `class RdaClient`, `volume_letter_to_disk` and FAILS if found
  (:37-45) — a driver-era guard asserting the old engine-side driver client
  stays deleted. [REWRITE] when engine/ is replaced (the symbols no longer exist
  to guard).
- **Job `linux`** (`ubuntu-24.04`, :47-65): apt zlib1g-dev/liblz4-dev/libzstd-dev/libgtest-dev
  (:51-52); `cmake --preset linux-gcc` + build + `ctest --preset linux-gcc`
  (:53-57); G-09 report-schema validation via `tools/validate_report.py` (:59-61);
  Win7 PE gate self-test `tools/test_verify_pe_um.py` (:63-65).
- **Job `linux-tsan`** (:67-83): manual cmake with `-fsanitize=thread` flags,
  ctest `-j2`. **Job `linux-ubsan`** (:85-95): `cmake --preset linux-ubsan`.
- **Job `windows`** (`windows-2022`, :97-157): `VCPKG_ROOT ← VCPKG_INSTALLATION_ROOT`;
  configure+build `win-x64-debug` and `win-x64-release` (:101-114); diagnostics
  step `if: always()` — dumpbin `/imports` on `cr_tests.exe` + `rrecov.exe`
  (:115-135); "Win7 posture gate" — `stamp_osversion_um.py` +
  `verify_pe_um.py --allowlist tools/ref_imports_um.json` over
  `rrecov.exe`, `cr_tests.exe`, `cr_ui.dll` (:136-151); `ctest --preset
  win-x64-release -C Release` (:152-157).
- **Job `fuzz`** (`ubuntu-24.04`, **timeout-minutes: 30** — the only job-level
  timeout in this workflow, :159-161): clang+zlib; `make -C fuzz all CXX=clang++`;
  smoke campaign `make -C fuzz run` (45 s per target, zero-crash gate, :170-171);
  `actions/upload-artifact@v4` `fuzz-crash-corpus` **if: failure()**, retention
  7, `if-no-files-found: ignore` (:172-183).
- **Driver touchpoints**: only the N-041 RdaClient lint at :40. The engine
  binary itself probes `\\.\RDA` (cr_transport), but that is engine/ code
  (replaced wholesale), not workflow code.

### 1.4 ui-ci.yml (104 lines)

- **Trigger**: push `main` on `ui/**`, `contracts/**`, workflow; PR same minus
  workflow (ui-ci.yml:7-17). `contents: read` (:19-20).
- **Job `ci`**, `runs-on: windows-latest`, defaults `working-directory: ui` (:23-27):
  1. checkout@v4 (:29)
  2. `pnpm/action-setup@v4` version 12.15.0 (:31-33)
  3. `actions/setup-node@v4` node 24, **cache: pnpm**, cache-dependency-path
     `ui/pnpm-lock.yaml` (:35-39)
  4. `pnpm install --frozen-lockfile` (:41-42)
  5. Typecheck / Lint / Format check (:44-51)
  6. Architecture rules: `pnpm arch:lint` (A1-A7) and `pnpm sg:lint`
     (ast-grep) (:53-57)
  7. `pnpm filefilter:check` — the engine-table ↔ bridge mirror gate (:59-60)
  8. `pnpm knip` with env `KNIP_DISABLE_RAW_TRANSFER: '1'` (:62-69)
  9. `pnpm jscpd` (:71-72); `pnpm test:coverage` (thresholds 70/60/70/70) (:74-75)
  10. `pnpm build` — production mode, real connector (alias contract keeps the
      test double out) (:77-79); `pnpm bundle:check` with env
      `BUDGET_MAIN_KB: '200'`, `BUDGET_TOTAL_KB: '500'` (:81-85)
  11. `dtolnay/rust-toolchain@stable` (:87-89); `cargo check --tests` and
      `cargo clippy -- -D warnings` in `ui/src-tauri` — no link step, no engine
      kit needed (:91-97)
  12. upload `web-dist` (ui/dist/), retention 7 (:99-104)
- **Driver touchpoints**: none in the workflow. The compiled bridge has them:
  `ui/src-tauri/Cargo.toml:21` (windows-sys for "SCM/\\.\RDA driver status"),
  `contracts/command-surface.json:29` (`system_driver_status`). Those ride the
  ui/ replacement.

### 1.5 ui-e2e.yml (67 lines)

- **Trigger**: push `main` on `ui/**` + workflow; PR `ui/**` (:9-17).
  `contents: read` (:19-20). Job `e2e`, `windows-latest`, defaults `ui` (:23-27).
- Steps: checkout@v4 (:29); pnpm/action-setup@v4 12.15.0 (:31-33); setup-node@v4
  node 24 **cache: pnpm** (:35-39); `pnpm install --frozen-lockfile` (:41-42);
  `pnpm exec playwright install --with-deps chromium` (:44-45); `pnpm e2e`
  (12 specs; webServer builds `--mode test` so the bundle carries the
  tests/fakes connector — header :3-8) (:47-48); upload `e2e-step-screenshots`
  (ui/test-results/*.png) `if: always()` retention 14 ignore-if-empty (:50-57);
  upload `e2e-traces` `if: failure()` retention 7 (:59-66).
- **Driver touchpoints**: none.

### 1.6 forensic-repro.yml (141 lines) — the Windows validation vehicle (T2)

- **Purpose** (header :1-26): reproduce every reported symptom of the INSTALLED
  release on a real Windows runner with captured evidence at every layer, then
  hand-curate artifacts into docs/FORENSIC-PROBE.md.
- **Trigger**: `workflow_dispatch` with input `release_tag` (default
  `v0.6.0-rc2`) (:27-33); push `main` on the workflow + `tools/forensic/**`
  (:34-38). `contents: read` (:40-41).
- **Job `repro`**, `runs-on: windows-2022`, **timeout-minutes: 90** (:43-47):
  1. checkout@v4 (:49); setup-node@v4 **node '20'** (:51-53)
  2. `npm install playwright-core@1.49.1` inside tools/forensic (CDP client
     only, no browser) (:55-61)
  3. "Download the shipped release assets" — curl
     `https://github.com/heicfast/final-prod/releases/download/<tag>/RecoveryWizard-<ver>-setup.exe`
     + `SHA256SUMS`; parses the sums file and **verifies setup.exe's SHA256**
     before use (:63-79)
  4. "Machine facts (pre-install)" — `tools/forensic/collect-facts.ps1 -Label
     preinstall` (:81-83)
  5. "Seed disks" — `tools/forensic/seed-disks.ps1`: diskpart creates two
     dynamic VHDXs — disk1 64 GB GPT/NTFS X: (~190 files, real JPEG/PNG/PDF/
     MP4/ZIP signatures, deliberate deletions), disk2 8 GB MBR/exFAT Y:
     (~60 files); writes seed-manifest.json + disk-map.json (:85-87;
     seed-disks.ps1:117-119, 168-218)
  6. **S1** "Install the product with full process/window capture" —
     `install-probe.ps1 -Silent`: runs setup.exe /S while sampling every child
     spawn (watchlist includes `certutil.exe`, `sc.exe`, `powershell.exe`,
     `fsutil.exe` — install-probe.ps1:65) and every top-level window at 400 ms;
     writes install-process-events.jsonl + install-window-events.jsonl +
     install-result.json (:89-91; install-probe.ps1:61-124)
  7. "Machine facts (post-install)" (:93-95)
  8. **S2+S3+S4** ONE step on purpose (the runner's job object kills
     step-spawned processes when the step ends — comment :100-104):
     `launch-app.ps1` starts the installed app with
     `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=9222`
     (launch-app.ps1:19) and waits for the CDP endpoint; `io-probe.ps1` samples
     the app's IO counters/handles/child processes every 2 s for 3000 s;
     `node drive-ui.mjs` drives the REAL shipped UI over CDP — S2 health per
     drive, S3 scan of the seeded 64 GB vdisk (up to 28 min timeline, including
     the NG-21 pre-scan repair consent gate: answers Deny and records the plan
     text, drive-ui.mjs:188-213), S4 every other surface (lost-partition,
     repair, sd-card, backup, settings) (:97-124; drive-ui.mjs:107-281)
  9. "Final facts (postrun)" (:126-128)
  10. upload `forensic-repro-evidence` (C:\forensic\evidence\**, seed-manifest,
      disk-map) `if: always()`, retention 30, `if-no-files-found: error`
      (:130-140)
- **Driver dependencies** (all in the facts layer):
  - collect-facts.ps1:29 `rdaService` (Get-Service RDA + StartType); :30
    `rdaServiceReg` (HKLM:\SYSTEM\CurrentControlSet\Services\RDA); :31
    `driverFile` (System32\drivers\RDA.sys); :34-35 `rootCert`/`tpCert`
    (HeicFast-subject certs in LocalMachine Root/TrustedPublisher); :36
    `rdacli` (runs `<install>\driver\rdacli.exe info`); :39 `systemEvents`
    filtered on `RDA|rda.sys`; :28 `testSigning` (bcdedit).
  - Workflow header :18 promises "rdacli liveness during the scan".
  - drive-ui.mjs itself is driver-agnostic (pure CDP DOM observation).
- **What it proves**: the shipped product's observable behavior (installer
  popups, SMART surface, scan progress/results, all pages) plus machine-level
  before/after delta (services, driver file, certs, registry, System log).
  After the driver removal the S1-S4 mechanics stay 100% valid; the facts
  probes need rewriting (drop RDA service/file/cert/rdacli rows; add elevation
  posture instead).

### 1.7 release.yml (190 lines) — the release assembly

- **Trigger**: `push: tags: ['v*']` (:21-24). **Permissions: `contents: write`**
  (release upload) (:26-27). Job `full-product`, `windows-2022`,
  **timeout-minutes: 90** (:30-32).
- Step order:
  1. checkout@v4 (:34)
  2. **Version gate** — tag must equal `v<VERSION>` from the root VERSION file,
     else Write-Error + exit 1 (:36-45)
  3. **Contracts gate** — `node contracts/validate.mjs` (:47-48)
  4. setup-node@v4 node 24 (:50-52); pnpm/action-setup@v4 12.15.0 (:54-56);
     dtolnay/rust-toolchain@stable (:58-60); nuget/setup-nuget@v2 (:62-63);
     microsoft/setup-msbuild@v2 (:65-66)
  5. **Full product build** — `./build.ps1 -Target all` with
     `$env:CR_UI_LIB_DIR = ""` (build.ps1 stages the kit itself) (:68-73)
  6. **Verify artifacts + dev signature** — asserts the four artifacts exist
     (`build\out\RecoveryWizard-<v>-setup.exe`, `-portable.zip`,
     `-testsigning.cer`, `SHA256SUMS`, :79-84), then
     `devsign.ps1 -Action verify` on the **setup.exe** (:92-93) and on
     **`build\out\stage-<v>\driver\rda.sys`** (:94-95) — both must carry the
     build certificate (:75-97)
  7. **Compose truthful release notes** — a here-string written to
     `release-notes.md`: product description naming the "REQUIRED RDA raw-disk
     kernel driver" (:107-110), "The driver is MANDATORY" section with
     `ERR_DRIVER_REQUIRED` (:112-119), signing-status section (ephemeral cert,
     Root+TrustedPublisher import, Secure-Boot/bcdedit guidance, Tauri bundle
     unsigned, :121-142), artifacts list (:144-156), and the SHA256SUMS content
     pasted verbatim (:158-162) (:99-163)
  8. **Upload to the release** — `softprops/action-gh-release@v2` with
     `body_path: release-notes.md` and files `RecoveryWizard-*-setup.exe`,
     `-portable.zip`, `-testsigning.cer`, `SHA256SUMS`,
     `fail_on_unmatched_files: true` (:165-174)
  9. **Build-log evidence** — upload `release-build-evidence` `if: always()`,
     paths `build/out/*` **excluding** `stage-*/` and `devsign.*` (NG-17: the
     PFX + its password file must never ride an artifact), retention 14
     (:176-189)
- **Release asset set (what a tag publishes)**: `RecoveryWizard-<v>-setup.exe`
  (combined NSIS installer: driver + engine + app), `-portable.zip` (app +
  engine + staged-not-installed driver), `-testsigning.cer` (the build's public
  dev certificate), `SHA256SUMS`.
- **Driver touchpoints**: :9-15 header; :94-95 devsign verify of rda.sys;
  :107-156 the notes text (mandatory-driver sections, cert import explanation,
  portable-driver description). Signing of the *installer* and the *engine kit*
  (build.ps1:130-131, 261-262) is driver-*motivated* but applies to driver-free
  payloads too.

### 1.8 validation-evidence.yml (266 lines) — T14 + T15 (workflow_dispatch only)

- **Trigger**: `workflow_dispatch` ONLY (:28-29). `contents: read` (:31-32).
  Header :3-27 documents intent; artifacts are downloaded and committed under
  docs/INTEGRATION-VALIDATION/ by hand; "No masking" (:23-26).
- **Job `t14-measurements`** ("T14 engine latency + bridge filter/memory
  measurements", `windows-2022`, timeout 60, :35-38):
  1. checkout@v4 (:40); nuget/setup-nuget@v2 (:44); microsoft/setup-msbuild@v2
     (:46) — the comment :42-43 says the WDK stack is needed for "the driver
     lane"; the engine lane needs only cmake+vcpkg.
  2. **Engine kit + driver** — `./build.ps1 -Target engine -SkipTests` then
     `./build.ps1 -Target driver` (:48-56; "the driver lane is here so the
     harness can run against the REAL mandatory transport", :51-52).
  3. **"Load the development-signed RDA driver (the mandatory transport)"**
     (:58-123) — the single biggest driver block in the pipeline:
     - devsign.ps1 `-Action install-cert` on `stage-<v>\driver\rda-testcert.cer`
       (:80-81);
     - if service RDA exists, `driver\scripts\uninstall.ps1 -ServiceName RDA`
       (:83-85);
     - `powershell.exe -File "$stage\driver\install.ps1" -ServiceName RDA
       -Source "$stage\driver\rda.sys"` (:86-87);
     - sleep 2, `Get-Service RDA` must be `Running` — else records the System
       event log (filter `RDA|driver`) into t14-driver-info.json and FAILS
       (:88-100);
     - **proves `\\.\RDA` opens**: Add-Type CreateFileW probe with GENERIC_READ
       (`[uint32]2147483648` decimal — the load-bearing cast, :104-112); GLE
       recorded on failure (:113-119); t14-driver-info.json written (:121-123).
  4. `ilammy/msvc-dev-cmd@v1` x64 (:125-128)
  5. **Compile + run the harness** — `cl /std:c17 /O2 /W4
     tools\validation\measure-cr-ui.c /I engine\include /link
     <kit>\cr_ui.lib psapi.lib`; copied into the kit dir and run
     `--quick-only` → `t14-engine.jsonl` (streaming per-phase JSON lines)
     (:130-148). measure-cr-ui.c (362 lines) measures enumerate/health/scan/
     snapshot/preview latencies + PeakWorkingSet against the retired
     simulator's budgets (measure-cr-ui.c:1-39) — it links only cr_ui, no
     driver import, but every scan it measures rode `\\.\RDA`.
  6. **Bridge-side measurements** — dtolnay/rust-toolchain@stable (:150-153);
     `cargo test --release -- --ignored --nocapture --test-threads=1` in
     ui/src-tauri with `CR_UI_LIB_DIR` + kit DLLs staged into
     target/release{,\deps} (G-7 filter cost at 10^5/10^6, NodeMeta memory);
     `T14JSON:` lines extracted to t14-bridge.jsonl (:155-176).
  7. **Runner context** — imageOs/runnerName/cpu/cores/ramGb/ranAtUtc/commit/
     runUrl → runner-context.json (:178-190).
  8. upload `t14-measurements` (4 files) `if: always()` retention 30
     if-no-files-found error (:192-203).
- **Job `t15-installer-matrix`** (`windows-2022`, timeout 100, :205-208):
  checkout; setup-node@v4 node 24; pnpm/action-setup@v4; dtolnay/rust-toolchain@stable;
  nuget/setup-nuget@v2; microsoft/setup-msbuild@v2 (:210-226); **full product
  build** `./build.ps1 -Target all -SkipTests` (:228-233); **run the runtime
  matrix** `tools/validation/installer-matrix.ps1 -InstallerPath
  build\out\RecoveryWizard-<v>-setup.exe -OutDir t15-evidence`, failure check
  via `$?` (the $LASTEXITCODE-null asymmetry lesson, :235-248); upload
  `t15-installer` (the setup.exe itself) retention 30 (:250-256); upload
  `t15-runtime-matrix` `if: always()` retention 30 (:258-265).
- **installer-matrix.ps1 rows** (the T15 contract, tools/validation/installer-matrix.ps1):
  1. baseline (:132-142);
  2. install `/S /allusers` — exit code, duration, registry InstallDir as
     source of truth (:147-185); asserts app exe + cr_ui.dll present
     (:229-231); **DriverCertThumbprint must exist in the SW key** (:235-238);
     **RDA service must be Running** (:245-247); **cert must be in LocalMachine
     Root AND TrustedPublisher** (:248-250); **staged rda.sys must carry a
     signature** (:251-253); exact module set — 12 named modules incl.
     `driver\rda.sys`, `driver\rdacli.exe`, `driver\rda.inf`,
     `driver\install.ps1`, `driver\uninstall.ps1`,
     `driver\rda-testcert.cer`, `driver\rda-testcert-thumbprint.txt`
     (:261-272); **driver dir must carry exactly 7 files** (:273-276); ≥1
     vcpkg runtime DLL (:277-280); no unexpected non-DLL root files (:281-284);
     no gtest/gmock leakage (:286-292);
  3. session preservation across reinstall-over-install (:297-313);
  4. repair over corruption (corrupt cr_ui.dll, rerun, hash restored)
     (:318-336);
  5. uninstall — app dir empty, SW key gone, uninstall key gone, **driver
     service gone**, **System32\drivers\RDA.sys gone**, **cert gone from Root +
     TrustedPublisher**, sessions KEPT (:341-386); throws on any failing row
     (:406-408).

---

## 2. build.ps1 / build.sh

### 2.1 build.ps1 (306 lines) — the Windows full-product entry point

- **Params** (:18-25): `-Target` ∈ `all|contracts|engine|driver|ui|installer|portable`
  (default `all`), `-Configuration` ∈ `Release|Debug`, `-SwitchTests` switch
  (`-SkipTests`). `$Version` from `VERSION` (:29); `$OutDir = build\out`,
  `$StageDir = build\out\stage-<Version>` (:30-32).
- **Version-lockstep gate** `Assert-VersionConsistency` (:36-45): reads
  `ui/package.json` `version` (:38-39), `ui/src-tauri/Cargo.toml` (regex
  `version = "<Version>"`, :40-41), `ui/src-tauri/tauri.conf.json` `version`
  (:42-43) — any drift throws. Component wire-visible versions (driver
  FILEVERSION via IOCTL, engine ABI 2) are explicitly NOT restamped (:13-17).
- **`Invoke-Contracts`** (:47-53): `node contracts/validate.mjs`, throw on fail.
- **`Invoke-Engine`** (:55-132):
  - VCPKG_ROOT fallback to VCPKG_INSTALLATION_ROOT (:59-62); `cmake --preset
    win-x64-<cfg>` + `--build --parallel` (:65-70);
  - ctest when Release and not SkipTests — `-C` is required on the VS
    multi-config generator (:71-80);
  - Win7 PE gates on the BUILD-tree files: `engine\tools\stamp_osversion_um.py`
    + `verify_pe_um.py --allowlist engine\tools\ref_imports_um.json` over
    `rrecov.exe` and `cr_ui.dll` (:83-96);
  - **engine kit staging** to `$StageDir\engine` (:111-120): `cr_ui.dll`,
    `cr_ui.lib`, `rrecov.exe`, plus every deployed `*.dll` beside cr_ui.dll
    EXCEPT `cr_ui.dll` itself and `gtest/gmock*` (the round-13 test-runtime
    exclusion, :98-119). Throws if no vcpkg runtime DLLs were deployed (:118).
  - **dev-signing of the staged kit**: `devsign.ps1 -Action new-cert` (idempotent
    so `-Target engine` is self-contained) then `-Action sign` on
    `$engineKit\cr_ui.dll`, `$engineKit\rrecov.exe` (:122-131).
- **`Invoke-Driver`** (:134-181) — [DELETE] wholesale:
  - `nuget restore packages.config` + `msbuild rda.sln /p:Configuration=<cfg>
    /p:Platform=x64` (:137-140);
  - `$drvBin = driver\bin\x64\<cfg>`; `driver\tools\stamp_osversion.py` on
    rda.sys (:143-145); `driver\tools\verify_pe.py ... --allowlist
    driver\tools\ref_imports.json` (explicit allowlist because the default is
    CWD-relative, :146-150);
  - devsign `new-cert` + `sign` on `$drvBin\rda.sys` (:161-165) — the WDK
    output stays SignMode=Off; the embed signature is a packaging step
    (:152-160);
  - **staging** to `$StageDir\driver`: `rda.sys`, `rdacli.exe`, `rda.inf`,
    `driver\scripts\install.ps1`, `driver\scripts\uninstall.ps1`,
    `rda-testcert.cer`, `rda-testcert-thumbprint.txt` (:167-175); the public
    cert is also promoted to the first-class artifact
    `build\out\RecoveryWizard-<Version>-testsigning.cer` (:176-179).
- **`Invoke-Ui`** (:183-221): `pnpm install --frozen-lockfile` → `pnpm typecheck`
  → `pnpm lint` → (`pnpm test` unless SkipTests) (:186-195); requires the
  staged engine kit (`cr_ui.dll` present, else "run the engine lane first",
  :197-203); copies the kit into `ui\src-tauri\target\{release,debug}` and sets
  `$env:CR_UI_LIB_DIR` (:204-212); `pnpm tauri build` (beforeBuildCommand runs
  `pnpm build`; cargo links cr_ui.lib — static import, :213-215); asserts the
  NSIS bundle dir exists (:218-220).
- **`Find-Makensis`** (:223-233): `%LOCALAPPDATA%\tauri\NSIS\makensis.exe`,
  then Program Files (x86)/Program Files NSIS, then PATH.
- **`Invoke-Installer`** (:235-264): picks the newest `*-setup.exe` from the
  Tauri nsis bundle (:237-239); runs makensis with
  `/DPRODUCT_VERSION=<v> /DAPP_BUNDLE=<tauri setup> /DENGINE_KIT=<stage engine>
  /DDRIVER_STAGE=<stage driver> /DOUT_DIR=<out> /DPATHSEP=\\` (:248-251) —
  native backslash paths because the Tauri-bundled makensis rejects
  forward-slash filespecs (:241-247); then **devsign `sign` on
  `build\out\RecoveryWizard-<v>-setup.exe`** (:254-263).
- **`Invoke-Portable`** (:266-275): delegates to `installer\portable.ps1`
  (-RepoRoot -Version -StageDir -OutDir); success checked with `$?` (the
  cmdlet-only $LASTEXITCODE-null trap, :269-274).
- **`Invoke-Checksums`** (:277-285): SHA256SUMS over every file in `build\out`
  except SHA256SUMS itself.
- **Lane switch** (:287-304): `contracts`; `engine` (contracts+engine);
  `driver`; `ui` (contracts+ui); `installer`; `portable` (+checksums);
  `all` = **contracts → driver → engine → ui → installer → portable →
  checksums** (:295-303) — note driver runs FIRST in the `all` lane (the
  driver stage must exist before portable.ps1 runs, which hard-requires it).
- **Driver references with lines**: :6-8 (usage comment), :14-16 (FILEVERSION
  note), :134-181 (whole Invoke-Driver), :250 (DDRIVER_STAGE define),
  :255-257 (comment: payloads inside the installer incl. rda.sys),
  :291/:297 (switch entries).

### 2.2 build.sh (54 lines) — the Linux developer subset

- Targets `all|contracts|engine|ui` only — **no driver lane, no installer,
  no portable** (header :2-9; case :45-51). `VERSION` read at :15.
- `run_contracts` = `node contracts/validate.mjs` (:21-24);
  `run_engine` = `cmake --preset linux-release` + build + ctest (:26-33);
  `run_ui_static` = `pnpm install --frozen-lockfile; typecheck; lint; test`
  (:35-43). Zero driver references. [NO CHANGE needed beyond the ui swap].

---

## 3. Installer footprint

### 3.1 recoverywizard.nsi (809 lines) — combined NSIS product installer

- **Defines required at compile time** (`!error` guards, :50-64):
  PRODUCT_VERSION, APP_BUNDLE, ENGINE_KIT, **DRIVER_STAGE** (:59-61), OUT_DIR.
  PATHSEP defaults `/` (Linux cross-compile validation), build.ps1 passes `\`
  (:72-80). REPO_ROOT from `${__FILEDIR__}/..` (:68-70).
- **Constants** (:123-144): `PRODUCT_NAME "HeicFast Recovery Wizard"`,
  `PRODUCT_PUBLISHER "HeicFast"`, `PRODUCT_UNKEY "Recovery Wizard"`,
  `PRODUCT_SWKEY "Software\HeicFast\Recovery Wizard"`,
  **`DRIVER_SERVICE "RDA"`** (:137, "contracts/README.md: defined ONCE"),
  `APP_EXE "recovery-wizard-ui.exe"` (:140), `APP_WINDOW_TITLE "Recovery
  Wizard"` (:141), `PROGRAMDATA_ROOT "$COMMONPROGRAMDATA\HeicFast\Recovery
  Wizard"`, `PROGRAMDATA_SESSIONS`, `PROGRAMDATA_LOGS` (:142-144).
- **Globals**: `RequestExecutionLevel admin` (:164); `SetRegView 64` + Win10+
  gate (CurrentMajorVersionNumber, :605-620); `SetShellVarContext all`
  (all-users only, :622-624). Variables `DriverResult`, `DriverCertThumbprint`,
  `EngineDllCount` (:169-171).
- **Pages** (:176-190): MUI2 welcome → license (repo LICENSE) → components →
  **custom driver page** (`Page custom DriverPageCreate DriverPageLeave`,
  :185) → directory → instfiles → finish (MUI_FINISHPAGE_RUN = the app).
- **Macros**: `AbortWithReason` writes
  `%TEMP%\HeicFastRecoveryWizard-install-abort.txt` then Abort (:88-95);
  `InstallFact` appends to BOTH
  `${PROGRAMDATA_LOGS}\install.log` and
  `%TEMP%\HeicFastRecoveryWizard-install-facts.txt` (:97-121).
- **Section 1 — `${PRODUCT_NAME} (required)` SEC_APP** (:196-279, SectionIn RO):
  - stages the Tauri bundle to `$PLUGINSDIR\app-setup.exe` (:213);
  - seeds the inner installer's install-location registry:
    `WriteRegStr SHCTX "Software\heicfast\Recovery Wizard" "" $INSTDIR` (:224);
  - `ExecWait '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR'` (:226)
    (ExecWait, not nsExec — the quoted-/D= re-tokenization hazard, :200-208);
    non-zero exit → AbortWithReason (:228-231);
  - adoption logic if the inner setup landed at its own default (:233-247);
  - engine kit: `SetOutPath $INSTDIR`; **`File /x cr_ui.lib
    "${ENGINE_KIT}${PATHSEP}*"`** — kit DLLs BESIDE the app exe (static-import
    loader truth, :249-260); integrity gates: cr_ui.dll exists (:262-264) and
    ≥2 DLLs in $INSTDIR (the 0xc0000135 guard, :266-278).
- **Section 2 — "RDA raw-disk kernel driver (required)" SEC_DRIVER**
  (:281-453, **SectionIn RO**, :286) — the driver footprint:
  - `SetOutPath $INSTDIR\driver`; **7 File instructions** (:297-304):
    `rda.sys`, `rdacli.exe`, `rda.inf`, `install.ps1`, `uninstall.ps1`,
    `/nonfatal rda-testcert.cer`, `/nonfatal rda-testcert-thumbprint.txt`
    (nonfatal so a production-signed build stages no dev cert, :291-296);
  - **certificate import BEFORE the service create** (gated on the staged
    .cer): `nsExec::ExecToLog 'certutil -addstore -f Root
    "$INSTDIR\driver\rda-testcert.cer"'` (:321) and the same for
    `TrustedPublisher` (:328); either failing → AbortWithReason (:324-334);
    no staged cert → fact "cert import skipped (production-signed posture)"
    (:335-339); thumbprint read from the staged file into $DriverCertThumbprint
    (:340-348);
  - **update/repair**: existing RDA service (HKLM ImagePath probe, :354) →
    `sc.exe stop` + sleep 1500 + `sc.exe delete` + sleep 500 (:357-362);
  - **WOW64-safe copy**: `${DisableX64FSRedirection}` +
    `CopyFiles /SILENT "$INSTDIR\driver\rda.sys" "$SYSDIR\drivers\RDA.sys"`
    (:384-386) with fatal verification (SysWOW64 trap → ERROR_FILE_NOT_FOUND,
    :387-396), then `${EnableX64FSRedirection}` (:397);
  - **service create**: `sc.exe create RDA binPath= "$SYSDIR\drivers\RDA.sys"
    type= kernel start= demand error= normal group= "Boot File System"`
    (:399-402);
  - **Parameters block** (registry): `HKLM\SYSTEM\CurrentControlSet\Services\
    RDA\Parameters` — `IoctlBase=0`, `MaxDisks=200`, `DebugLevel=1`,
    `SessionsEnabled=1` (WriteRegDWORD, :405-408);
  - ImagePath read-back fact (:411-413);
  - **registration failure** → $DriverResult="failed", best-effort cert
    removal (`certutil -delstore Root/TrustedPublisher $tp`, :421-428),
    AbortWithReason (:429-430) — "the product is not installed half-way";
  - **start**: `sc.exe start RDA` (:434); exit 0/1056 → "installed"
    (:437-440); anything else → "installed-not-running" + truthful
    MessageBox (Secure Boot / `bcdedit /set testsigning on` guidance, /SD IDOK,
    :441-450) — the documented non-fatal start-failure branch
    (installer/README.md deviation #6).
- **Section 3 — "-post" (hidden)** (:455-501):
  - `CreateDirectory` PROGRAMDATA_SESSIONS + PROGRAMDATA_LOGS (:459-460);
  - product registry `SHCTX ${PRODUCT_SWKEY}`: `DriverInstalled=1` (DWORD,
    :467), `InstallDir` (:468), `Version` (:469), `InstallMode="AllUsers"`
    (:470), `DriverCertThumbprint` (conditional, :471-473); read-back facts
    (:474-484);
  - `Call RemoveInnerUninstallKeys` (:490; function :551-599 — deletes every
    HKLM+HKCU Uninstall key whose UninstallString points into $INSTDIR);
  - Add/Remove entry `SHCTX "Software\Microsoft\Windows\CurrentVersion\
    Uninstall\Recovery Wizard"`: DisplayName, DisplayVersion, Publisher,
    InstallLocation, UninstallString (`"$INSTDIR\HeicFastUninstall.exe"`),
    QuietUninstallString (`... /S`), NoModify=1, NoRepair=1 (:491-498);
  - `WriteUninstaller "$INSTDIR\HeicFastUninstall.exe"` (:500).
- **Section descriptions** (:507-513); **DriverPageCreate/Leave**
  (:519-543) — informational page: only transport, SYSTEM privileges,
  admin-only DACL, test-signing truth, demand-start.
- **Uninstaller `Section "Uninstall"`** (:644-808):
  1. ask-close via FindWindow on APP_WINDOW_TITLE, MB_RETRYCANCEL /SD IDCANCEL
     (:648-665);
  2. **driver teardown** (if the service key exists): `sc.exe stop` + sleep
     1500 + `sc.exe delete` + sleep 500 (:676-683); `DeleteRegKey HKLM
     "SYSTEM\CurrentControlSet\Services\RDA"` (:686); `Delete /REBOOTOK` of
     `$WINDIR\Sysnative\drivers\RDA.sys` AND `$WINDIR\System32\drivers\RDA.sys`
     (both WOW64 views, :694-699);
  3. **certificate removal** by thumbprint from the SW key:
     `certutil -delstore Root` + `-delstore TrustedPublisher` (:709-720);
  4. ProgramData sessions: silent → keep unconditionally; interactive asks
     MB_YESNO `/SD IDNO`; yes → `RMDir /r "${PROGRAMDATA_ROOT}"` (:722-745);
  5. inner Tauri uninstaller, copied to $TEMP and run `/S _?=$INSTDIR`
     (:747-753);
  6. our files: explicit `Delete /REBOOTOK` of cr_ui.dll, rrecov.exe, and the
     known runtime DLL names (zlib1, z, lz4, liblz4, liblz4d, zstd, zstdd,
     libzstd) + catch-all `Delete "$INSTDIR\*.dll"` + **`RMDir /r
     "$INSTDIR\driver"`** (:755-778);
  7. registry: DeleteRegKey the Uninstall key + SW key (:781-782); the seeded
     `Software\heicfast\Recovery Wizard` key + empty parent (:791-796);
  8. final tidy: delete install.log, RMDir logs dir, delete
     HeicFastUninstall.exe, RMDir $INSTDIR, RMDir "$PROGRAMFILES64\HeicFast"
     (:798-807).
- **Driver-era logic embedded**: the WOW64 redirection dance (:384-397,
  :694-699), the start-failure branch (:441-451), the cert orphan cleanup on
  the abort path (:421-428), the DriverPage (:519-543), the DriverResult/
  DriverCertThumbprint state (:169-170), the `driver\` staging surface
  (:291-304), and the registry values DriverInstalled/DriverCertThumbprint
  (:467, :471-473).
- **Counts**: 4 `Section` blocks in the script (SEC_APP :196, SEC_DRIVER :281,
  "-post" :455, "Uninstall" :644); the compile-check evidence records "7
  sections" per NSIS's own counting (installer/README.md:118-121). File
  instructions: 9 payload `File` commands (1 app :213, 1 engine kit :260, 7
  driver :298-304) + 2 `CopyFiles` (:386 driver→System32, :750 uninstaller→TEMP).
  Registry hives/keys touched: HKLM SYSTEM\...\Services\RDA{,\Parameters};
  SHCTX Software\heicfast\Recovery Wizard; SHCTX Software\HeicFast\Recovery
  Wizard; SHCTX ...\Uninstall\Recovery Wizard; HKLM SOFTWARE\Microsoft\Windows
  NT\CurrentVersion (read); HKLM+HKCU Uninstall enumeration (RemoveInnerUninstallKeys).

### 3.2 portable.ps1 (106 lines)

- Hard requires: the built app exe (:36-39), the staged engine kit (:40-43),
  **and the staged driver — throws if `driver\rda.sys` is missing** (:44-47).
- Assembles `RecoveryWizard-<v>-portable\`: app exe (:53), engine kit `*`
  beside it (:55), **driver stage `*` → `driver\` subdir (STAGED, NOT
  INSTALLED)** (:56).
- `README-PORTABLE.txt` (:58-90): the driver-required label with manual
  install instructions (`certutil -addstore Root/TrustedPublisher
  .\driver\rda-testcert.cer`, `.\driver\install.ps1 -ServiceName RDA -Source
  .\driver\rda.sys`, Secure-Boot testsigning guidance, :69-77) + "RUN AS
  ADMINISTRATOR" + sessions-beside-exe + WebView2 note.
- **Portable-mode marker** `recovery-wizard.portable` beside the exe (NG-18:
  the bridge's `util::portable_mode` probe — present ⇒ sessions/logs resolve
  beside the executable, :92-96).
- `Compress-Archive` → `RecoveryWizard-<v>-portable.zip`, SHA256 printed
  (:98-105). [REWRITE]: drop the driver requirement/staging/README section;
  the marker + kit-beside-exe mechanics survive.

### 3.3 devsign.ps1 (220 lines) — the development test-signing tool

- Actions: `new-cert | sign | verify | install-cert | remove-cert` (:59).
  Cert subject `CN=HeicFast Recovery Wizard Development Signing` (:71).
- **new-cert** (:118-153): `New-SelfSignedCertificate -Type CodeSigningCert`
  RSA-2048/SHA256, 3 years, in `Cert:\CurrentUser\My` (works elevated or not,
  :129-137); exports `rda-testcert.cer` (DER — certutil's native form, :139),
  `devsign.pfx` with an ephemeral random password written to
  `devsign-pfx-password.txt` (:141-145), `rda-testcert-thumbprint.txt` (:146);
  removes the store copy (:148). Idempotent unless `-Force` (:124-127).
- **sign** (:155-176): `Find-Signtool` walks `Windows Kits\10\bin` newest-kit
  first, x64 then x86 (:77-94); `signtool sign /f <pfx> /p <pw> /fd SHA256 /v
  <file>` (:163); verifies via `Get-AuthenticodeSignature` signer-SUBJECT
  match (store-independent, :168-175).
- **verify** (:178-186): asserts a signature is present.
- **install-cert** (:188-204): elevated; `certutil -addstore -f Root` +
  `TrustedPublisher`; prints thumbprint. **remove-cert** (:206-218): elevated;
  `certutil -delstore` both stores by thumbprint (the uninstaller's leg).
- What gets signed (call sites): rda.sys (build.ps1:164), staged cr_ui.dll +
  rrecov.exe (build.ps1:130), setup.exe (build.ps1:261); verified at
  release.yml:92-95; the .cer ships as the `-testsigning.cer` asset
  (build.ps1:179). The Tauri app bundle is NOT signed (DEV-SIGNING.md §1
  table, docs/DEV-SIGNING.md:11-17).
- [REWRITE]: the sign/verify/new-cert actions are payload-agnostic and could
  survive for installer+kit dev-signing; `install-cert`/`remove-cert` exist
  only for the driver load policy and become dead code; the filenames
  (`rda-testcert.*`) are driver-branded.

---

## 4. contracts-ci and the boundary validation

`contracts/validate.mjs` (135 lines) is the boundary-truth gate, run by
contracts-ci.yml:38-39, build.ps1 Invoke-Contracts (:47-53), build.sh
(:21-24), and release.yml:47-48 (before any build):

- **Check 1** (:39-55): every `cr::Err` value parsed from
  `engine/include/cr/cr_error.h` has a row in `contracts/error-codes.json`,
  and every mapped row references a real header value with the right number.
- **Check 2** (:57-77): every `uiCode` (mapped + bridgeOnlyCodes) exists in
  `ui/src/connector/contract.ts`'s `ConnectorErrorCode`, and every vocabulary
  member is accounted for.
- **Check 3** (:79-88): every `CR_EVT_*` define in
  `engine/include/cr/cr_ui.h` (both single-line and `\`-continued forms) is
  mapped in `contracts/event-mapping.md` with the right number.
- **Check 4** (:90-97): `command-surface.json` well-formedness — unique names,
  name/threading/serves present, threading ∈ {sync, spawn-join, async-job}.
- **Phase 2** (auto-activates when `ui/src-tauri/src/bridge/abi.rs` exists,
  :99-131): every manifest command has a Rust `fn` in
  `ui/src-tauri/src/{commands,bridge}` and vice versa (:109-116); every
  `invoke('...')` from `ui/src/connector/` is in the manifest (:118-130).
- contracts-ci.yml additionally runs `node ui/scripts/gen-filefilter.mjs
  --check` (:41-42) — the engine filefilter table ↔ bridge mirror freshness
  gate (also `pnpm filefilter:check` in ui-ci.yml:59-60).
- **Driver-bound contract data** (the script is generic; the data is not):
  `command-surface.json:29` `system_driver_status`; `error-codes.json:65-70`
  `ERR_DRIVER_REQUIRED` → uiCode `driver-required` with the mandatory-driver
  rule text; `type-maps.md:207-209` DriverStatus; `README.md:47` service name
  `RDA`. These rows must change with the engine/ui swap (the new engine's
  refusal code is `ERR_ELEVATION_REQUIRED = 0x500C0001`,
  core_recovery/include/cr/cr_error.h:120).

---

## 5. Secrets, caches, runners inventory

- **Secrets**: **none referenced anywhere**. `git grep "secrets\." -- .github/`
  returns nothing; no `GITHUB_TOKEN` explicit use; the ephemeral dev
  certificate is deliberately secret-free (docs/DEV-SIGNING.md:19-28). The
  only signing-adjacent future rails live inside `driver/signing/ossign/`:
  `sign.yml` references repo variables `SOURCE_REPO` / `OSSIGN_SERVER` /
  `OSSIGN_KEY` and the protected environment `signing`
  (driver/signing/ossign/sign.yml:22-33) — **dead the moment `driver/` is
  deleted** (the file dies with the directory; no final-prod workflow
  references it). No driver-signing secret exists to revoke.
- **Caches**: exactly two — `actions/setup-node` `cache: pnpm` with
  `cache-dependency-path: ui/pnpm-lock.yaml` in ui-ci.yml:35-39 and
  ui-e2e.yml:35-39. No cargo cache, no vcpkg cache, no nuget cache (every
  WDK restore re-downloads; driver-ci.yml:120-122 and build.ps1:137).
- **Runners**: `ubuntu-24.04` (contracts-ci validate; engine-ci lint/linux/
  linux-tsan/linux-ubsan/fuzz), `ubuntu-latest` (driver-ci lint/fuzz),
  `windows-2022` (driver-ci unit + build matrix; engine-ci windows;
  forensic-repro repro; release full-product; validation-evidence t14 + t15),
  `windows-latest` (ui-ci ci; ui-e2e e2e).
- **Timeouts**: engine-ci fuzz 30m (:161); forensic-repro 90m (:47); release
  90m (:32); validation-evidence t14 60m (:38) + t15 100m (:208). Others
  default.
- **Artifacts**: driver-ci `rda-wdk26100-{Debug,Release}` (default retention,
  :199-207, never consumed); engine-ci `fuzz-crash-corpus` if:failure() 7d
  (:172-183); ui-ci `web-dist` 7d (:99-104); ui-e2e screenshots 14d + traces
  7d (:50-66); forensic-repro `forensic-repro-evidence` 30d (:130-140);
  release `release-build-evidence` 14d with the NG-17 stage-/devsign exclusion
  (:176-189); validation-evidence `t14-measurements` 30d (:192-203),
  `t15-installer` 30d (:250-256), `t15-runtime-matrix` 30d (:258-265).
- **Dead-on-driver-removal**: the OSSign variables/environment (inside
  driver/), driver-ci's entire artifact set, and the WDK nuget restore cache
  semantics. Nothing else.

---

## 6. Driver touchpoint master list (exhaustive)

Classification legend: [DELETE-FILE] exists only for the driver · [REWRITE]
must be edited to drop driver parts and keep the rest · [REPLACE-WITH-CORE-
EQUIVALENT] superseded by the core_recovery lane/asset · [AUDIT-TRAIL-
PRESERVE] evidence/history, kept untouched.

| File | Line(s)/step | What it does driver-wise | Class |
|---|---|---|---|
| `driver/` (63 files: rda.sln, src/driver/*, src/client/rdacli.c, src/bench/rdabench.c, src/shared/inc/rda_ioctl.h+rda_compat.h, fuzz/*, tests/unit/test_compat.c, scripts/{install,uninstall,run-integration,run-unit-tests}.ps1/.sh, tools/{stamp_osversion.py,verify_pe.py,ref_imports.json}, packages.config, signing/ossign/{sign,review}.yml, docs/* incl. benchmarks/2026-10-22-runner-comparison.md, winpe/regdrv-rda.reg) | whole tree | The driver: rda.sys build+PE gates+bench+fuzz+install scripts+OSSign rail | [DELETE-FILE] |
| `.github/workflows/driver-ci.yml` | all 208 lines | lint/fuzz/unit/WDK-matrix + unsigned assert + dev-sign smoke + rda-* artifacts | [DELETE-FILE] |
| `.github/workflows/release.yml` | :9-15 header; :94-95 devsign verify of `stage\<v>\driver\rda.sys`; :107-156 notes text (mandatory driver, ERR_DRIVER_REQUIRED, testsigning.cer) | release assembly asserts the signed driver + publishes driver narrative | [REWRITE] |
| `.github/workflows/validation-evidence.yml` | :42-46 (WDK stack for the driver lane); :48-56 (`build.ps1 -Target driver`); :58-123 (cert import + install.ps1 + RDA Running + `\\.\RDA` CreateFileW probe) | T14 loads the driver before measuring | [REWRITE] |
| `.github/workflows/engine-ci.yml` | :37-45 (N-041 lint greps `RdaClient::`/`class RdaClient`/`volume_letter_to_disk`) | asserts the deleted engine-side driver client stays deleted | [REPLACE-WITH-CORE-EQUIVALENT] (core_recovery checks.yml is the successor lane; the RdaClient lint is obsolete there) |
| `build.ps1` | :134-181 Invoke-Driver (nuget+msbuild+stamp+verify_pe+devsign+stage 7 files+cer promote); :250 `/DDRIVER_STAGE`; :291 `driver` case; :297 driver-first in `all` | the driver build lane + installer define + lane wiring | [REWRITE] (drop the lane, the define, reorder `all`) |
| `build.sh` | — | no driver content | no change |
| `installer/recoverywizard.nsi` | :59-61 DRIVER_STAGE define; :137 DRIVER_SERVICE; :169-170 vars; :185 driver page; :281-453 SEC_DRIVER (7 Files, cert import, WOW64 copy, sc create/start, Parameters, abort path); :467/:471-473 DriverInstalled+DriverCertThumbprint; :508 DESC_DRIVER; :519-543 DriverPage; :667-701 uninstall driver teardown; :703-720 cert removal; :778 RMDir driver | the entire driver install/uninstall surface | [REWRITE] (keep SEC_APP + -post + kit layout + sessions consent + inner-uninstaller choreography) |
| `installer/portable.ps1` | :14-21 header; :44-47 driver-stage requirement; :56 driver copy; :58-90 README driver text | stages the not-installed driver + manual-install instructions | [REWRITE] |
| `installer/devsign.ps1` | :25-34 install-cert/remove-cert docs; :188-218 those actions; rda-testcert.* filenames | driver load-policy support actions + driver-branded names | [REWRITE] (sign/verify survive if installer dev-signing is kept; install-cert/remove-cert die) |
| `installer/README.md` | :3-9, :29-45, :56-70, :79-87, :105-114 (deviations 1/5/6), :116-128 | documents the driver flow + mandatory-driver deviations | [REWRITE] |
| `tools/forensic/collect-facts.ps1` | :28-31, :34-36, :39 | facts: RDA service, service reg, System32 RDA.sys, HeicFast certs, rdacli, System-log RDA filter | [REWRITE] |
| `tools/forensic/forensic-repro.yml` header | :18 "rdacli liveness" | narrative | [REWRITE] |
| `tools/forensic/{seed-disks,install-probe,io-probe,launch-app,drive-ui}.mjs/.ps1` | — (install-probe watchlist :65 includes certutil/sc.exe — generic) | driver-agnostic mechanics | no change (keep) |
| `tools/validation/installer-matrix.ps1` | :57 DriverSvc; :191-208 cert/driverSigned facts; :235-253 Running+cert+signed asserts; :261-276 12-module set incl. 7 driver files + driverCount==7; :362-375 uninstall driverGone/sysDriverGone/certGone asserts | the T15 driver contract | [REWRITE] (new expected-module set: 5 named + DLL class) |
| `tools/validation/measure-cr-ui.c` | — (links only cr_ui) | no direct driver refs; measured the RDA-backed transport | [REPLACE-WITH-CORE-EQUIVALENT] (core_recovery has its own benchmark lanes: benchmark-src.yml, test-benchmark.yml) |
| `contracts/command-surface.json` | :29 `system_driver_status` | bridge command probing SCM + `\\.\RDA` | [REWRITE] (with the new UI's real wiring; becomes elevation-status or dies) |
| `contracts/error-codes.json` | :65-70 `ERR_DRIVER_REQUIRED` → `driver-required` | the mandatory-driver refusal contract | [REWRITE] (→ `ERR_ELEVATION_REQUIRED` 0x500C0001, core_recovery cr_error.h:120) |
| `contracts/type-maps.md` | :207-209 DriverStatus | SCM/device/IOCTL GET_VERSION mapping | [REWRITE] |
| `contracts/README.md` | :47 (service name RDA), :50 (driver rda_version.h in lockstep note) | constant definitions | [REWRITE] |
| `contracts/validate.mjs` | — | mechanism is driver-free | no change (data changes) |
| `ui/` (replaced wholesale) | `src-tauri/Cargo.toml:21` windows-sys SCM/\\.\RDA comment; `commands/system.rs` get_driver_status; `src/connector/*` driver-required vocab | bridge-side driver status | dies with the ui swap (recovery-wizard-ui-new@dd2782c has no driver surface) |
| `engine/` (replaced wholesale) | cr_transport `\\.\RDA` probe, ERR_DRIVER_REQUIRED emission | engine-side driver transport | dies with the engine swap (core_recovery@12f3edf7: direct raw access) |
| `docs/DEV-SIGNING.md` | whole file (§1-§5) | the driver test-signing design; §5 production path names Hardware Dev Center + the ossign rail | [AUDIT-TRAIL-PRESERVE] + a successor note (the installer-only signing story is a new, much smaller doc) |
| `docs/RELEASE.md` | :5 (all lanes incl. driver), :13, :18-32 (artifact descriptions), :34-37 (MSIX refusal cites the driver), :39-61 signing status | release contract | [REWRITE] |
| `docs/BUILD.md` | :9-10 lane list, :30-34 driver lane, :54-55 WDK prerequisite, :81-82 driver-ci mapping | build contract | [REWRITE] |
| `docs/INSTALLER.md` | :25-34 placement table (System32\drivers row, cert stores row, driver\ in $INSTDIR), :36-55 driver flow, :57-68 uninstall, :84-92 signing posture | installer contract | [REWRITE] |
| `docs/FORENSIC-PROBE.md`, `docs/INTEGRATION-VALIDATION/**` (t14-measurements, t15-runtime-matrix, installer-compile-check.md), `worklog*`, `ui/worklog*`, `driver/worklog.md` (dies with driver/) | — | measured evidence of the driver-era product | [AUDIT-TRAIL-PRESERVE] |
| `docs/context-sheets/DRIVER-DEPLOYMENT.md`, `docs/INTEGRATION-PLAN.md` §7/§8.3, `docs/INTEGRATION-GAPS.md`, `docs/PARITY-DELTAS.md`, `docs/MSIX-ASSESSMENT.md`, `docs/REFERENCE-*.md`, `docs/INSTALL-FOOTPRINT-*.md`, `docs/INSTALLER-DECISION.md` | various | historical analysis referencing the driver | [AUDIT-TRAIL-PRESERVE] (supersede with new context sheets; do not edit history) |

---

## 7. What the pipeline must become

Grounded in the read: engine → core_recovery@12f3edf7 (user-mode direct raw
access; `ERR_ELEVATION_REQUIRED = 0x500C0001` at core_recovery/include/cr/
cr_error.h:120; its own CI already exists: checks.yml — Windows-only
consolidation with the elevation refusal-contract probes — plus
benchmark-src.yml, test-benchmark.yml, installer.yml, release.yml); ui →
recovery-wizard-ui-new@dd2782c (pnpm@12.15.0/Node 24/Tauri 2; package.json
scripts: typecheck/lint/format:check/arch:lint/knip/jscpd/test/test:coverage/
e2e/tauri/bundle:check/visual:compare — **no `sg:lint`, no `filefilter:check`**,
and scripts/ has arch-lint, bundle-check, i18n-sweep, visual-compare but **no
gen-filefilter.mjs**); driver → gone.

- **contracts-ci.yml — SURVIVES (data edit)**. validate.mjs's mechanism is
  engine/ui-agnostic. The data rows change: `ERR_DRIVER_REQUIRED` → the new
  engine's `ERR_ELEVATION_REQUIRED` vocabulary; `system_driver_status`/
  DriverStatus removed or replaced by an elevation/status surface. The
  `gen-filefilter.mjs --check` step (:41-42) must be dropped unless the
  filefilter mirror is ported into the new UI (it has no such script today).
- **driver-ci.yml — DELETE** (driver-ci.yml is 100% driver). No other workflow
  consumes its artifacts, so deletion is consequence-free. The WDK matrix, the
  unsigned-output assertion, and the dev-sign smoke leg all end; release.yml's
  existing verify gate (:92) becomes the only signature proof for setup.exe.
- **engine-ci.yml — REPLACE-WITH-CORE-EQUIVALENT**. core_recovery's own
  checks.yml is the proven successor (clang-format, N-041 dead-surface lints,
  MSVC /W4 /WX, full ctest, Win7 posture gates, G-09 + the elevation
  refusal probes). If final-prod keeps a monorepo engine lane, it should be
  re-derived from that file; the current engine-ci.yml's RdaClient lint
  (:40) is meaningless against the new engine. The fuzz lane maps to
  core_recovery's local `ci-local.sh --san|--fuzz` posture or its benchmark/
  test lanes.
- **ui-ci.yml / ui-e2e.yml — SURVIVE with step edits**: the new package.json
  lacks `sg:lint` and `filefilter:check`, so ui-ci.yml:56-60 loses those two
  steps (or the scripts get ported); everything else (pnpm setup, caches,
  knip, jscpd, coverage, bundle budget, cargo check/clippy) maps 1:1 onto the
  new repo's script surface. The "real wiring" of the mock connector is a
  code change inside src-tauri, not a workflow change — cargo check/clippy
  keep working either way (no link step, ui-ci.yml:91-97).
- **release.yml — REWRITE (the big one)**. Kept: tag trigger `v*`, version
  gate, contracts gate, `build.ps1 -Target all`, SHA256SUMS, gh-release
  upload. Changed: (a) drop the `devsign verify` of `stage\<v>\driver\rda.sys`
  (:94-95); (b) the artifact set shrinks to `setup.exe` + `portable.zip` +
  `SHA256SUMS` — the `-testsigning.cer` asset existed to make the test-signed
  DRIVER loadable (release notes :130-133; recoverywizard.nsi:306-339), and
  with no kernel code-integrity gate there is no machine-level reason to
  import a cert; the installer's cert import/uninstall removal and the
  Root/TrustedPublisher writes all go; (c) whether devsign of setup.exe +
  cr_ui.dll/rrecov.exe survives is a product decision — devsign.ps1's
  sign/verify are payload-agnostic (devsign.ps1:155-186), and an unsigned
  user-mode installer only costs SmartScreen noise (the already-documented
  gap, docs/DEV-SIGNING.md:17); (d) the notes text is rewritten (no
  "mandatory driver", no bcdedit guidance; state the elevation-required
  posture instead). The T18 RC discipline (docs/RELEASE.md:70-75) stays.
- **build.ps1 — REWRITE**: delete Invoke-Driver (:134-181), the `driver`
  target, `/DDRIVER_STAGE` (:250), and the `all`-lane driver step (:297); the
  engine kit staging, ui lane, installer lane, portable lane, checksums, and
  the version-lockstep gate are untouched. portable.ps1 drops its driver
  hard-requirement (:44-47) and the driver\ payload.
- **recoverywizard.nsi — REWRITE**: delete SEC_DRIVER (:281-453), the driver
  page (:185, :519-543), DriverInstalled/DriverCertThumbprint (:467,
  :471-473), the uninstall driver teardown + cert removal (:667-720), and
  `RMDir $INSTDIR\driver` (:778). SEC_APP, the -post section, the kit layout,
  ProgramData, the sessions-consent choreography, and the inner-uninstaller
  dance all survive. NOTE: core_recovery ships its own Inno Setup installer
  ("session-96: the kernel-driver component is REMOVED — the installer never
  touches the service control manager", core_recovery .github/workflows/
  installer.yml header) — adopting it versus keeping the NSIS wrapper is an
  open consolidation decision; either way the NSIS wrapper minus SEC_DRIVER
  is the minimal-diff path for final-prod.
- **forensic-repro.yml — SURVIVES (mechanics) + REWRITE (facts)**: S1-S4 are
  driver-free by construction (CDP DOM driving, process/window sampling,
  seeded VHDs). collect-facts.ps1's driver probes (:28-36, :39) are replaced
  by elevation-posture facts. The scan probes remain meaningful: the new
  engine does direct raw access under an elevated token — the installed app
  runs elevated (requireAdministrator posture), so S3 still measures real
  scans of the seeded vdisk; the interesting new failure mode to probe is the
  ERR_ELEVATION_REQUIRED refusal, exactly what core_recovery's checks.yml
  probes at the engine level.
- **validation-evidence.yml — REWRITE**: T14 loses the entire driver-load
  step (:58-123) and the driver build lane (:55-56) — the harness then
  measures the real user-mode transport directly (measure-cr-ui.c needs no
  change: it links only cr_ui.lib); the driver benchmarks are replaced by
  core_recovery's own benchmark lanes (benchmark-src.yml, test-benchmark.yml)
  — the driver-era rdabench/runner comparison dies with driver/. T15's
  matrix re-baselines: expected modules become the 5 product files +
  runtime-DLL class (installer-matrix.ps1:261-276), the Running-service /
  cert-store / signed-rda.sys asserts (:245-253) are deleted, and the
  uninstall row asserts files+registry+sessions only (:373-386).
- **The single riskiest transformation**: the release.yml ↔ build.ps1 ↔
  recoverywizard.nsi triad. They unwind together — `build.ps1 -Target all`
  currently runs driver FIRST (:297) and portable.ps1 hard-fails without the
  driver stage (portable.ps1:44-47); release.yml hard-fails without the
  signed rda.sys (:94-95) and the `-testsigning.cer` asset (:79-84); the
  installer aborts the whole install if the service cannot register
  (recoverywizard.nsi:414-430). Any one of them left driver-aware while the
  others are stripped means every tag build reds or ships a broken artifact —
  and because forensic-repro installs the TAGGED release (forensic-repro.yml:
  63-79), a bad first tag poisons the whole validation chain downstream.

— Agent C, T2-c. Sources: all 8 workflows, build.ps1/build.sh, installer/*
(recoverywizard.nsi read in full, 809 lines), contracts/validate.mjs,
tools/forensic/* (6 files), tools/validation/* (2 files), docs/{BUILD,RELEASE,
DEV-SIGNING,INSTALLER}.md, VERSION; cross-checked against core_recovery@12f3edf7
and recovery-wizard-ui-new@dd2782c for section 7 grounding.
