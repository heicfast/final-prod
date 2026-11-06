# REPO-STATE — the complete CURRENT-STATE sheet of final-prod @ c8aef05a (v0.8.4)

> Agent A (Phase 0, T1). READ-ONLY survey; every claim cites `path:line` or a
> commit SHA in the tree at HEAD `c8aef05ab841abcfc3b2eb2e1de76d128869565e`
> (branch `main`, verified `git rev-parse HEAD`; origin
> `https://github.com/heicfast/final-prod.git`). `VERSION:1` = `0.8.4`.
> Working tree clean except one untracked parallel-agent sheet
> (`worklog/INSTALLER-AND-PRIVILEGE.md`, in flight at 2026-11-06 16:04 UTC).
> Badge/live-status checks were made 2026-11-06 ~16:00 UTC.

---

## Commit graph summary

- **2362 commits total** on main (`git log --oneline | wc -l`). Tags: v0.5.0
  (+3 rcs), v0.6.0-rc1/rc2, v0.7.0, v0.8.0..v0.8.4.
- **v0.7.0..HEAD = 26 commits, all dated 2026-11-05/06** — the entire v0.8.x
  line is one ~17-hour session. v0.6.0-rc2..v0.7.0 = 1123 commits (the
  consolidation session: driver removal + core re-merge + UI replacement).
- **v0.7.0** = `68ab3fcd` 2026-11-05 22:58 "evidence(P6 prep)…" — the
  user-mode/driverless rebase release (run 42126551916, RELEASE-RUN.md:61).
- **v0.8.0** = `f1c0418f` (05:04) "session-save visibility" — beneath:
  `db600f4c` (refused save now shows onError toast), `d3982b01` (prettier),
  `95d5d1de` (BF-25 save/resume gates re-vehicled).
- **v0.8.1** = `97893c15` (06:17) "backup-list truth" — beneath: `73419713`
  (BF-26: whole-disk session no longer "(unreadable session)").
- **v0.8.2** = `ff9e4c13` (07:35) "stopped-scan banner" — beneath: `f76c9941`
  (state poller froze at 'stopping' forever), `23ec07c6` (BF-25 drain-window
  gate generalized).
- **v0.8.3** = `939dd483` (08:26) "invisible-resume" — beneath: `1cd9bc64`
  (`/scan?scanId=` was not a route; resumed scan ran behind fallback page).
- **v0.8.4** = `4b297849` (09:49) "whole-disk session resume" — beneath:
  `559080ae` (BF-27: re-attach by size among identical-model disks),
  `a5e6ae7a` (resume-hang instrumentation).
- **Post-v0.8.4 (6 commits)**: forensic-probe tooling only — `44d166e6`
  (unelevated twin user-data dir), `c0e590a4` (WebView2 sweep pre-uninstall),
  `f48f0396` (completion cap 960→carve variance), `2f89f833` (NSIS `_?=`
  self-survivor), `0d09bca7` (emptied install dir), `4c522cd3` (FORENSIC #29
  evidence), then HEAD `c8aef05a` (T1 session opens; names the four spot-check
  root causes — worklog.md:2156-2208).
- **Last commits per dir** (`git log -5 -- <dir>`):
  - `engine/` → `318b9655` (P3a driver removal) — **engine/ has NOT changed
    since the v0.7.0 era; every v0.8.x fix is ui/-side only**.
  - `ui/` → `4b297849` (v0.8.4 tag commit itself).
  - `installer/` → `318b9655` — untouched since driver removal.
  - `contracts/` → `8e61f4a8` (P4 UI wiring) — untouched since.
  - `.github/workflows/` → `0d09bca7` (forensic uninstall leg).
  - `tools/` → `f48f0396`; `docs/` → `a5cfa11f` (P5); `worklog/` → `c8aef05a`.

## Workflows (7 lanes, all files read; live badge = "passing" for all 7)

Live status method: fetched
`https://github.com/heicfast/final-prod/actions/workflows/<name>.badge.svg`
anonymously at ~16:00 UTC — all seven returned `passing`. Per-run history was
NOT obtainable from this sandbox (Actions HTML requires JS; the REST API was
rate-limited for this IP) — badge status only, recorded explicitly. What the
files guarantee is stated per lane below.

### contracts-ci.yml (44 lines) — push(main)+PR; ubuntu-24.04
Paths: `contracts/**`, `engine/include/**`, `ui/src/connector/**`,
`ui/src-tauri/src/**`, `ui/scripts/gen-filefilter.mjs`, itself
(contracts-ci.yml:8-24). One job `validate` (node 24):
`node contracts/validate.mjs` (error-code map ↔ cr_error.h, event map ↔
CR_EVT_*, command manifest ↔ both implementations) + `gen-filefilter.mjs
--check` (contracts-ci.yml:39-43). ~1 min. **Green proves**: boundary JSON/docs
match headers and both impls. **Proves nothing about runtime.**

### engine-ci.yml (506 lines) — push(main)+PR+workflow_call+dispatch; windows-2022
Paths `engine/**` + itself, negating `**.md`, `engine/docs/**`,
`engine/worklog*` (engine-ci.yml:43-59; the `[main]` in the file is verified
by od -c — an earlier console artifact stripped `[m`). Two jobs:
`lint` (15 min timeout; clang-format; dead-surface + architecture lints;
PowerShell AST gate; ROOT workflow YAML parse gate; doc-truth gate M-088) →
`build-and-test` (needs lint, 90 min; vcpkg cache; CMake win-x64-release
/W4 /WX; full ctest incl. cr_ui ABI; Win7 posture gate
stamp+verify_pe_um; G-09 report schema; **elevation refusal-contract
probes** engine-ci.yml:343-381 — `rrecov` unelevated must exit 4
(ERR_ELEVATION_REQUIRED) and elevated `bench --disk 0 --mode info` must pass;
M-071 CLI junk-filter contract). CI scripts live in
`engine/.github/scripts/` (Probe-Unelevated.ps1 et al.) resolved via the
job's `working-directory: engine` (engine-ci.yml:257-259). **Stale comment**:
engine-ci.yml:60 claims "release.yml calls this lane as its validation gate"
— release.yml contains no `uses: ./.github/workflows/engine-ci.yml`; the
release runs `build.ps1 -Target all` with ctest inside instead. **Green
proves**: the ENGINE compiles clean and its 1741 ctest cases pass on a
runner; the CLI refuses unelevated raw-disk work. **Says nothing about the
Tauri exe's launch privilege** — the probes exercise `rrecov.exe`, not
`recovery-wizard-ui.exe`.

### ui-ci.yml (117 lines) — push(main)+PR; windows-latest
Paths `ui/**`, `contracts/**` (ui-ci.yml:9-19). One job `ci` (wd `ui/`):
pnpm install → typecheck → lint → format:check → arch-lint A1-A7 → ast-grep
(raw `invoke()` only in `src/connector/**`; connector factory import only
from queries/connector — ui-ci.yml:58-70) → filefilter mirror → knip → jscpd
→ `test:coverage` (thresholds 70/60/70/70) → build → bundle:check
(main 200 KB / total 500 KB) → `cargo check --tests` + clippy `-D warnings`
in `ui/src-tauri` (**no link step — no engine kit needed**, ui-ci.yml:5-6).
**Green proves**: TS/Rust static quality + mock-connector unit/component
tests. **Proves nothing about the real bridge at runtime.**

### ui-e2e.yml (74 lines) — push(main)+PR; windows-latest
Paths `ui/**`. One job `e2e`: builds with **`VITE_CONNECTOR=mock`** and runs
12 Playwright tests (3 spec files: smoke 6, responsive 4, consent 2) against
`vite preview`. The file's own comment: "the e2e specs are mock-pinned by
design in this state; the real connector needs the Tauri shell + the engine
kit, which is the release lane's business" (ui-e2e.yml:3-8). **Green proves**
the mock-driven web app navigates. **Zero coverage of cr_ui.dll.**

### release.yml (214 lines) — tags `v*` ONLY; windows-2022; 90 min
Version gate (tag == VERSION), contracts gate, `build.ps1 -Target all`
(contracts→engine incl. ctest→ui+tauri→NSIS→portable→checksums), artifact +
dev-sign verification (setup.exe, staged `cr_ui.dll`), truthful release notes
(self-signed status; the notes themselves describe the **asInvoker +
restart-as-admin** posture at release.yml:111-117), gh-release upload,
evidence artifact excluding signing material (release.yml:205-212). Measured:
22.0 min (v0.8.0, run 42167758364), 22.8 min (v0.7.0, run 42126551916)
(RELEASE-RUN.md:19,61). Five v0.8.x release runs green: 42167758364,
42174434974, 42182876659, 42188868989, 42199111948 (worklog.md:2145-2146).
**Green proves**: the product builds, self-signs, and its engine passes ctest
inside the build. **Never runs on main pushes.**

### forensic-repro.yml (310 lines) — workflow_dispatch ONLY; windows-2022; 90 min
Input `release_tag` (default v0.8.4). Downloads the shipped release assets
(checksum-verified), `collect-facts` pre/post, seeds 64 GB NTFS + 8 GB exFAT
VHDX disks, S1 install-probe (400 ms window/process sampling — install only),
launches the app with CDP + io-probe + `drive-ui.mjs` (S2 health, S3 scan,
S4 surfaces on the real shipped DOM), the **unelevated twin**
(`runas /trustlevel`, CDP 9223) asserting the elevation banner + scan-start
refusal (forensic-repro.yml:142-155), app log/sessions snapshot, silent
uninstall + no-leftover assertions. Dispatch-only by design
(forensic-repro.yml:30-36). FORENSIC #29 (run 42226613660 @ 0d09bca7 on
v0.8.4) = the first fully-green run (RELEASE-RUN.md:386-439). **The blind
spot**: the runner shell is elevated and `launch-app.ps1:50-54` documents
that the MAIN instance is expected elevated; the unelevated twin EXPECTS the
banner. The probe therefore validates the app when pre-elevated by its
launcher, and validates the refusal UI when not — **it never requires a
launch-time UAC prompt, so a missing requireAdministrator manifest is
invisible to this gate by construction.**

### validation-evidence.yml (211 lines) — workflow_dispatch ONLY; windows-2022
`t14-measurements` (60 min): `build.ps1 -Target engine -SkipTests`, elevation
facts recorded (TokenElevation asserted true — validation-evidence.yml:50-71),
`measure-cr-ui.c` harness `--quick-only`, bridge `cargo test --release --
--ignored`. `t15-installer-matrix` (100 min): full build, then
`installer-matrix.ps1` install/update/repair/uninstall rows on a clean runner.
Dispatch-only (numbers are runner-dependent; artifacts curated by hand). **No
launch-privilege gate either** — elevation is a precondition the workflow
asserts about ITSELF, not a property it tests of the app.

## Directory map

- `engine/` — the C++20 user-mode recovery core (project `core_recovery`
  VERSION 0.6.1, engine/CMakeLists.txt:3-6). `src/core/` = 29 module dirs
  (ntfs fat exfat ext4 btrfs zfs refs hfsplus carve health tree session
  storage recovery preview reghive query orchestrator check repair table lost
  compress crypto err bcdfix nvramfix apply + cr_parallel.cpp);
  `src/win/` = 12 Win32 modules (cr_enumerate cr_health cr_fsck cr_transport
  cr_image_create cr_image_win cr_bitlocker cr_destio cr_mountpoints
  cr_elevate cr_bootfix); `src/ui/cr_ui.cpp` (5086 lines; 92 `CR_UI_API`
  exports — count verified `rg -c "^CR_UI_API" cr_ui.h` = 92; export tail
  cr_ui_exports.def); `src/app/rrecov.cpp` (CLI); `include/cr/` = 36 module
  header dirs + cr_ui.h (1602 lines); `tests/unit/` = 76 GoogleTest files +
  tests/win-stub (Linux-buildable Win32 stubs) + tests/data; `tools/` =
  CI verifier scripts (verify_pe_um.py, doc_truth_gate.py,
  stamp_osversion_um.py…); `scripts/` = dev/repro scripts;
  `diff-to-fix/` = the reverse-engineering audit corpus; `worklog/` =
  ~100 historical session sheets; `docs/` (ABI-CONTRACT, CI-CURRENT…);
  `vcpkg.json` (zlib/gtest/lz4/zstd).
- `ui/` — React 19 + TS + Vite + Tauri 2 app. `src/app/` (router, shell,
  providers, error-boundary); `src/features/` = 11 features (backup consent
  dev-panel device-selection health lost-partition preview recovery-queue
  repair results-explorer scan-progress settings-about); `src/queries/` = 11
  domain clients; `src/connector/` = contract.ts + mock/ (test double) +
  tauri/ (the real connector); `src/stores/` (5 zustand stores);
  `src/components/ui/`, hooks, i18n, lib, theme, workers (hex worker);
  `e2e/` = 3 Playwright specs (12 tests); `scripts/` = arch-lint,
  bundle-check, gen-filefilter, i18n-sweep, visual-compare.
  `src-tauri/`: `src/bridge/` (abi.rs consent.rs error.rs events.rs
  filefilter.rs logging.rs state.rs util.rs), `src/commands/` = 11 domain
  command files (devices health lost_partition preview recovery repair
  results scan sessions system), `lib.rs`/`main.rs` (GUI subsystem —
  `windows_subsystem = "windows"`, main.rs:1-2), `build.rs`, `tauri.conf.json`,
  `Cargo.toml`, `capabilities/default.json`, icons.
- `contracts/` — the boundary truth: `command-surface.json` (v4, 45 commands,
  threading classes sync/spawn-join/async-job, abiRefs+serves per command),
  `error-codes.json` (40 engine↔UI code mappings), `validate.mjs` (the gate),
  `event-mapping.md`, `type-maps.md`, `README.md`.
- `installer/` — `recoverywizard.nsi` (531 lines; outer all-users NSIS
  wrapper around the Tauri bundle; RequestExecutionLevel admin, nsi:151;
  MUI_FINISHPAGE_RUN launches the app, nsi:164-165), `devsign.ps1`
  (ephemeral self-sign), `portable.ps1`, `README.md`.
- `tools/` — `forensic/` (8 scripts: collect-facts, seed-disks, install-probe,
  launch-app, io-probe, drive-ui.mjs, unelevated-launch, unelevated-probe —
  see "prior sheets" below) + `validation/` (installer-matrix.ps1 T15,
  measure-cr-ui.c T14).
- `docs/` — 19 product docs (ARCHITECTURE BUILD DEV-SIGNING FORENSIC-PROBE
  INSTALL-FOOTPRINT-{CURRENT,DELTA} INSTALLER* INTEGRATION-* PARITY-DELTAS
  PRODUCT-WIRING REFERENCE-* RELEASE TESTING) + INTEGRATION-VALIDATION/.
- `worklog/` — the consolidation/release session sheets (detailed below).
- `.github/workflows/` — 7 lanes above; CI scripts ride in
  `engine/.github/scripts/` (engine-ci.yml:6-7).

## Product-determining configs

- `VERSION` = `0.8.4`; lockstep gate `build.ps1:37-47` (Assert-VersionConsistency)
  checks ui/package.json:4, ui/src-tauri/Cargo.toml:3, tauri.conf.json:4 — all
  `0.8.4` (verified). Engine carries its own `0.6.1` (CMakeLists.txt:4,
  vcpkg.json:3) — an engine-internal version, decoupled from product VERSION.
- `ui/src-tauri/tauri.conf.json` — window "main" 1240×840 (min 800×600),
  maximized, **decorations: false** (custom titlebar), center; bundle targets
  `["nsis"]` only; nsis `installMode: "both"`, languages EN + SimpChinese,
  displayLanguageSelector, installerIcon; webviewInstallMode
  `downloadBootstrapper`; csp: null; identifier com.heicfast.recoverywizard
  (tauri.conf.json:13-52). NOTE: `installMode: "both"` is dead config in the
  shipped product — the outer wrapper forces all-users (nsi:149-151, and the
  inner Tauri setup is driven `/S /allusers` at nsi:210).
- `ui/src-tauri/Cargo.toml` — tauri 2 (+dialog, opener plugins), windows-sys
  0.59 (Foundation, Storage_FileSystem, Security, System_Threading, UI_Shell,
  UI_WindowsAndMessaging — the TokenElevation probe + ShellExecuteW relaunch
  verbs, Cargo.toml:21-34); release profile: codegen-units 1, lto, opt-level
  "s", panic abort, strip (Cargo.toml:36-41).
- `ui/src-tauri/build.rs` — sets cr_ui.lib link search (CR_UI_LIB_DIR or
  target/{release,debug}), `cargo:rustc-link-lib=dylib=cr_ui` (static import
  of cr_ui.dll, ABI v4), then **bare `tauri_build::build()`** (build.rs:38).
- `build.ps1` (252 lines) — targets `contracts|engine|ui|installer|portable|
  all` (+ `-SkipTests`, `-Configuration`); stages
  `build/out/stage-<v>/engine`; Find-Makensis; SHA256SUMS checksums.
- `engine/CMakeLists.txt` — C++20; MSVC /W4 /WX /permissive-/ /Zc:preprocessor
  /utf-8; **_WIN32_WINNT=0x0601 WINVER=0x0601 (Win7 SP1 target envelope)**
  (CMakeLists.txt:19-30); zlib/lz4/zstd all REQUIRED-fatal (FB-X01);
  GTest 1.15.2; POST-BUILD vcpkg DLL deployment (cmake/DeployVcpkgBinaries).
  `CMakePresets.json` — win-x64-release (vcpkg toolchain $env{VCPKG_ROOT},
  Release) / win-x64-debug / linux-gcc / linux-release / linux-ubsan;
  binaryDir `${sourceDir}/build/${presetName}`.
- `contracts/*.json` — command-surface.json (v4): every Tauri command the
  bridge exposes ↔ abiRefs it calls ↔ queries it serves; error-codes.json:
  cr::Err numeric space ↔ ConnectorErrorCode vocabulary. Both gated by
  validate.mjs in contracts-ci + release.
- `.gitignore` — build/, dist/, out/, target/, node_modules/, vcpkg_installed/,
  ui/src-tauri/gen/, IDE dirs; `*.log` except engine test fixtures. Posture:
  outputs-only; no tracked build product; signing material additionally
  excluded from release evidence artifacts (release.yml:205-212, NG-17).

## The manifest question — verified answer

**NO file in this repository configures a Windows application manifest /
requestedExecutionLevel for the UI exe. The shipped
`recovery-wizard-ui.exe` launches with the invoker's token (asInvoker
semantics) — no UAC at launch.** Evidence:

1. `Glob **/*.manifest` → **zero files** in the whole tree.
2. Grep `requireAdministrator|requestedExecutionLevel|asInvoker|
   highestAvailable` → hits ONLY in docs/, worklog/, engine historical notes,
   and the forensic fact collector. **Zero hits in any build input.**
3. `ui/src-tauri/build.rs:38` = bare `tauri_build::build()` — no
   `WindowsAttributes`, no `.app_manifest(...)` override.
4. `ui/src-tauri/tauri.conf.json` — no elevation field (Tauri 2's config has
   none; the manifest is a build.rs concern).
5. Upstream truth (fetched from tauri-apps/tauri @ tag `tauri-build-v2.7.4`,
   matching Cargo.lock:3436-3438): `WindowsAttributes::new()` embeds
   `crates/tauri-build/src/windows-app-manifest.xml` by default
   (lib.rs:344-361), and that file's entire content is the Common-Controls
   v6 dependency — **it contains no `<trustInfo>`/`<requestedPrivileges>`/
   `<requestedExecutionLevel>` at all**. An exe whose manifest declares no
   requestedExecutionLevel is created with the caller's token.
6. Consequence (matches the user's report exactly): launched from the Start
   menu the app is unelevated; it shows the in-app banner and the
   `system_relaunch_elevated` command — `ShellExecuteW("runas", own-exe)` +
   `app.exit(0)` (ui/src-tauri/src/commands/system.rs:84-124) — the "ugly
   restart-as-admin prompt".

**Claims in the tree that contradict the binary** (documentation
hallucination, already named by the T1 spot-check at worklog.md:2173-2182):
- `installer/recoverywizard.nsi:27-29` — "the app itself runs elevated
  (requireAdministrator)". FALSE in the build.
- `docs/INTEGRATION-PLAN.md:936-945` — §7.4 "DECIDED (a) … ADOPTED. The app
  manifest is requireAdministrator". Never implemented.
- `docs/INSTALL-FOOTPRINT-CURRENT.md:205` — "The app itself carries
  requireAdministrator". FALSE.
- The reference-product sheets (worklog/context-sheets/REFERENCE-INSTALLER.md:
  163-164, 617-622) describe the EaseUS DRW reference's requireAdministrator
  manifests — the model §7.4 cited — but that is reference evidence, not this
  product.
- `release.yml:111-117` (the shipped release notes) describes the **asInvoker
  + restart-as-admin** posture truthfully — the release notes match the
  binary; the plan/footprint/NSIS comments do not.

**Why every gate stayed green with the manifest missing**:
- engine-ci probes `rrecov.exe` (the CLI), not the UI exe (engine-ci.yml:343-381).
- forensic-repro's main app instance is launched from the ELEVATED runner
  (launch-app.ps1:50-54 records "expected true"), and the installer's finish
  page launches the app from the elevated installer (nsi:164-165) — every
  automated launch path pre-elevates.
- The unelevated twin EXPECTS the banner + refusal (forensic-repro.yml:142-155)
  — i.e. the probe's gates codify the asInvoker behavior as CORRECT.
- `collect-facts.ps1:32` even classifies the installed exe's embedded manifest
  (`'as-invoker-or-highestAvailable'` regex) into the facts JSON — but no
  assertion anywhere consumes `appManifest` (grep in docs/FORENSIC-PROBE.md +
  RELEASE-VALIDATION/* → zero hits).

## What the prior sheets cover vs the user's defects

Sheets in `worklog/` (one line each):
- `FINAL-PROD-INVENTORY.md` — Agent A T2-a @ a6cdead9: the OLD 0.6.0-rc2
  driver-era tree inventory (pre-consolidation).
- `UI-INTEGRATION-SURFACE.md` — Agent B T2-b: the old Rust bridge (ABI v2)
  wiring record.
- `BUILD-PIPELINE.md` — Agent C T2-c @ a6cdead9: the driver-era build/CI/
  installer/release survey.
- `CORE-CURRENT-STATE.md` — Agent D T2-d @ core 12f3edf7: the ABI v4 engine
  contract.
- `UI-REPO-STATE.md` — Agent F T2-f @ dd2782c: the fresh UI repo inventory.
- `INTEGRATION-PLAN.md` — Agent E T3: the consolidation plan (contains the
  unimplemented §7.4 requireAdministrator decision).
- `context-sheets/` (6 files, all ARCHIVED) — driver-era reference evidence;
  REFERENCE-INSTALLER.md documents the EaseUS requireAdministrator model.
- `RELEASE-VALIDATION/` — the §1-§9 gate evidence: DRIVER-REMOVAL, CORE-MERGE,
  BUILDS, UI-WIRING, CAPABILITY-MAP, and RELEASE-RUN.md (the release +
  forensic campaign ledger, v0.7.0→v0.8.4, FORENSIC #29 at :386-439,
  behavior-findings section at :441-455).
- `BEHAVIOR-FINDINGS.md` — the BF-20..BF-27 register: BF-22 (deep-scan stall
  at 76.1%), BF-23 (SMART absent on Azure VM disks), BF-24 (save race),
  BF-25 (unbounded cancel latency inside the carve matcher), BF-26
  (unreadable-session mislabel), BF-27 (identical-model resume refusal).
- `session-*-prompt.md` (3 files) — the verbatim session prompts.
- Root `worklog.md` (2213 lines) — master log; the v0.8.x campaign ledger at
  :2145-2154; the T1 debug-session opening with the four spot-checks at
  :2156-2208. Historical gold: the 0.6.x-era gap register at :1305-1335 —
  **NG-8 was "every health_read spawns fsutil via _popen (a VISIBLE console
  per health read, twice per drive)" and it was FIXED natively (IOCTL
  StorageDeviceTrimProperty) — then the engine re-merge (283a5d90) replaced
  the fixed file with core_recovery's `_popen` version (cr_health.cpp:306):
  the console-flash defect was known, fixed once, and REGRESSED by the
  consolidation.** NG-7 of the same era was "only one drive listed" (old-UI
  filter bug) — a historical parallel to the user's 1-of-3 symptom.
- Untracked `worklog/INSTALLER-AND-PRIVILEGE.md` — being written by a
  parallel Phase 0 agent right now (not read, not duplicated).

Coverage vs the user's four defects:
1. **No elevation request at launch / ugly restart prompt** — NOT covered by
   any prior sheet as a defect; the sheets claim the opposite (§7.4,
   INSTALL-FOOTPRINT-CURRENT.md:205, nsi:27-29). Only the T1 entry
   (worklog.md:2173-2182) names it. Prior validation encoded the asInvoker+
   banner behavior as the PASS condition.
2. **Console popups on disk health** — NOT covered as a shipped-product defect.
   The probe collects the raw material (install-window sampling at 400 ms is
   install-only, install-probe.ps1:87-95; io-probe.ps1 samples app child
   processes — its header literally says "the fsutil _popen flash evidence")
   but NO gate fails on a visible console during the app's lifetime, and
   drive-ui.mjs's "console" capture is the WEBVIEW console (drive-ui.mjs:102).
   Historical fix lost (NG-8, above). Engine cause verified at
   cr_health.cpp:306 (`_popen("fsutil behavior query disabledeletenotify
   2>nul", "r")` — visible console in a GUI-subsystem process; the correct
   CreateProcessW+CREATE_NO_WINDOW pattern already exists in cr_fsck.cpp /
   cr_bitlocker.cpp).
3. **1 of 3 disks enumerated** — NOT covered. BF-27 fixed the identical-model
   RESUME refusal (by-size pick), but no sheet audits enumeration on real
   multi-disk hardware; the runner has 2 virtual disks + 2 seeds, all
   "Virtual Disk"/empty-serial (worklog.md:2201-2208 hints the identity axes
   are weak). Enumeration code: engine/src/win/cr_enumerate.cpp (257 lines,
   two stacks; identity axes product@+16/serial@+24 with a historical
   offset-misread fix note at cr_enumerate.cpp:63-68); UI row builder
   ui/src-tauri/src/commands/devices.rs.
4. **Scan stuck at 75% "advanced scanning", "files found 21 (0 B)"** —
   PARTIALLY covered then DISMISSED: BF-22 records the 76.1% stall and labels
   it "environmental/known-perf (the carve matcher over free space …), not a
   wiring regression" (BEHAVIOR-FINDINGS.md:36; RELEASE-RUN.md:441-455);
   FORENSIC #29 explicitly "recorded not gated" (RELEASE-RUN.md:414-417).
   The user's report upgrades it to a real product defect (T1 concedes,
   worklog.md:2193-2199). The "files found 21 (0 B)" counter detail is in NO
   sheet (runner recorded 189/197 items); the status-line surface is
   ui/src/features/scan-progress/scan-progress-page.tsx:168-179 ("Advanced
   Scanning, Files Found: N (size)").

## Open questions for other agents

- **Agent B (installer/privilege)**: the fix decision — embed a
  requireAdministrator manifest via `tauri_build::Attributes::windows_
  attributes(WindowsAttributes::app_manifest(...))` in ui/src-tauri/build.rs
  (the option is documented upstream; lib.rs:425-445 shows the pattern), vs
  keep the relaunch flow and fix the three lying docs (nsi:27-29,
  INTEGRATION-PLAN.md:943, INSTALL-FOOTPRINT-CURRENT.md:205). Also: is
  `installMode: "both"` (tauri.conf.json:46) reachable at all through the
  all-users-only wrapper (nsi:149-151, 210)?
- **Agent C/D (engine)**: cr_health.cpp:306 `_popen` → the native IOCTL
  (the 0.6.x NG-8 fix, worklog.md:1329-1335, is the in-repo precedent) or
  CreateProcessW+CREATE_NO_WINDOW (cr_fsck.cpp pattern). Enumeration axes on
  real hardware (two internal disks + SD): cr_enumerate.cpp's
  query_disk_handle offsets + the UI's devices.rs row builder. BF-22 root
  cause: does the carve matcher reach a cancellation checkpoint
  (BEHAVIOR-FINDINGS.md:39, BF-25's family), and why do found-count/bytes
  freeze at "21 (0 B)" — check the event chain cr_ui.cpp → bridge/events.rs
  → scan-progress store.
- **Agent E (workflows/validation)**: gates to add — (a) assert the installed
  exe's manifest classification (collect-facts.ps1:32 already produces it);
  (b) a cold unelevated launch gate that expects UAC (only meaningful after
  the manifest decision); (c) an app-lifetime no-visible-console gate (extend
  install-probe's window sampling beyond the install step, or assert
  io-timeline has zero cmd.exe/fsutil children); (d) fix the stale
  engine-ci.yml:60 comment (release.yml does not call the lane).
- **All**: the release notes (release.yml:111-117) vs the NSIS/plan docs
  disagree about the elevation posture — whichever way the product decision
  goes, three docs must be reconciled, and the forensic probe's unelevated-
  twin gates re-specified to match the decided posture.
