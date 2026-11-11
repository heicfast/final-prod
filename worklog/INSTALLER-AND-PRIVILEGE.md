# INSTALLER + PRIVILEGE — the full Phase-0 Agent F audit (v0.9.2 tree)

> READ-ONLY audit of `/home/z/my-project/work/final-prod` @ VERSION 0.9.2.
> This file OVERWRITES the prior (v0.8.4-era) sheet — that sheet described
> asInvoker, the ElevationBanner, and the `_popen` fsutil; all three are
> gone in the current tree. Every claim carries a `path:line` citation.
> Companion history: `worklog/lane-b/INSTALLER-AND-PRIVILEGE.md` (the
> v0.8.4 audit), `worklog/REPRODUCTION/D-PRIV-01/README.md`,
> `worklog/REPRODUCTION/D-CONS-01/README.md`.

## 0. Executive verdict

| Question | Answer as built today (0.9.2) |
|---|---|
| Does the installer elevate? | Yes — `RequestExecutionLevel admin`, one UAC at setup launch (`installer/recoverywizard.nsi:162`) |
| Does the shipped app exe carry requireAdministrator? | **Yes, wired and gated** — `ui/src-tauri/app.manifest:25` embedded via tauri-build (`ui/src-tauri/build.rs:45-48`), asserted by the build gate (`build.ps1:169-190`), by the release lane (`release.yml` runs `build.ps1 -Target all`, `.github/workflows/release.yml:62-67`), and by the installed-product probe (`forensic-repro.yml:109-123`, `unelevated-launch.ps1:113-125`). The v0.9.0 forensic run recorded `appManifest = requireAdministrator` GREEN on the real installed release (`worklog/RELEASE-VALIDATION/RELEASE-RUN.md:501-508`). |
| Can a console flash on disk health in THIS tree? | **No reachable path.** The health read uses per-device IOCTL TRIM (`engine/src/ui/cr_ui.cpp:4977-4984` → `engine/src/win/cr_health.cpp:431-459`); the only fsutil spawn left is `query_trim_state` (`engine/src/win/cr_health.cpp:392-394`) with `CREATE_NO_WINDOW + SW_HIDE` + piped stdio, and it is NOT reachable from the UI (the bridge never passes `CR_ENUM_FLAG_TRIM` — `ui/src-tauri/src/bridge/util.rs:158,165` call the plain `cr_ui_enumerate_disks`). |
| Does a restart-as-admin prompt exist? | **No.** The banner + `system_relaunch_elevated` were removed with the 0.9.0 manifest fix (`ui/src-tauri/src/commands/system.rs:4-13`; no `elevation-banner.tsx` anywhere — glob `ui/src/**/*elevation*` = 0 hits). Residual `errors.elevationRequired` copy still tells the user to "Restart the app as administrator" (`ui/src/i18n/en.ts:543-544`) with **no action button** — dead vocabulary by design. |
| Why does the user still see all three symptoms? | The symptom triad (no UAC at launch + off-theme restart prompt + cmd/powershell flash on health) is **exactly the v0.8.4-or-earlier binary** — all three defects were fixed in the 0.9.0 train (`RELEASE-RUN.md:460-531`). The most probable explanations, ranked, are in §7. |

---

## 1. The installer (`installer/` — every file read in full)

### 1.1 `recoverywizard.nsi` (543 lines) — the combined product installer

- **Identity/artifact**: `Name "HeicFast Recovery Wizard ${PRODUCT_VERSION}"` (:144), `OutFile "${OUT_DIR}…\RecoveryWizard-<v>-setup.exe"` (:145), Unicode + SOLID LZMA (:51-52). Driven by build.ps1 with `/DPRODUCT_VERSION /DAPP_BUNDLE /DENGINE_KIT /DOUT_DIR /DPATHSEP` (`build.ps1:222-224`).
- **Privilege**: `RequestExecutionLevel admin` (**:162**) — the installer exe itself embeds requireAdministrator; ONE UAC at setup launch. All-users only: `SetShellVarContext all` in `.onInit` (:417) and in the uninstaller (:426, :436); `SetRegView 64` (:399, :425). Per-user installs are not offered (:158-160 comment).
- **Win10+ gate**: registry `CurrentMajorVersionNumber` probe, `MessageBox … /SD IDOK` + `Abort` (:400-413) — silent-safe.
- **Pages**: MUI Welcome → License (repo `LICENSE`, :179) → **custom Privilege page** (`PrivilegePageCreate` :319-341 — three static nsDialogs labels: user-mode product/no driver/no service/no cert; raw-disk needs administrator; installer elevates once and uninstall keeps sessions unless explicitly deleted) → Directory → InstFiles → Finish with `MUI_FINISHPAGE_RUN "$INSTDIR\recovery-wizard-ui.exe"` (:175-176).
- **Install dir**: `$PROGRAMFILES64\HeicFast\Recovery Wizard` (:146).
- **SEC_APP** (:191-274, RO):
  - Inner Tauri NSIS bundle staged to `$PLUGINSDIR\app-setup.exe` (:208), install-location seed `WriteRegStr SHCTX "Software\heicfast\Recovery Wizard" "" "$INSTDIR"` (:219), then driven **silently**: `ExecWait '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR' $0` (**:221**) — ExecWait deliberately (nsExec re-quotes `/D=`, :197-203). Nonzero exit aborts with a trace file (:223-226).
  - Integrity/adoption: exe must exist; falls back to the inner's own default dir (:233-242).
  - Engine kit lands **beside the exe** at `$INSTDIR` — `SetOutPath "$INSTDIR"` + `File /x cr_ui.lib "${ENGINE_KIT}${PATHSEP}*"` (:254-255). Static-import loader truth: `cr_ui.dll` must sit in the exe's directory or the app 0xc0000135s (:246-250). Gates: `cr_ui.dll` present (:257-259) and ≥2 DLLs total (runtime-DLL presence, :264-273).
- **"-post" hidden section** (:276-313):
  - ProgramData shared state: `C:\ProgramData\HeicFast\Recovery Wizard\{sessions,logs}` (:140-142, :280-281). Default ACL inheritance, no custom SDDL (documented deviation #2, `installer/README.md:89`).
  - Product SW key `HKLM\Software\HeicFast\Recovery Wizard` {InstallDir, Version, InstallMode=AllUsers} with read-back facts (:285-296).
  - **Uninstall-key takeover**: `RemoveInnerUninstallKeys` sweeps any HKLM+HKCU Uninstall key whose UninstallString points into `$INSTDIR` (:344-392), then the single product entry `HKLM\…\Uninstall\Recovery Wizard` is written (:303-310) with NoModify=1/NoRepair=1 (repair = rerun installer).
  - `WriteUninstaller "$INSTDIR\HeicFastUninstall.exe"` (:312).
- **Logging**: `InstallFact` appends to BOTH `${PROGRAMDATA_LOGS}\install.log` (persistent, user-discoverable) and `%TEMP%\HeicFastRecoveryWizard-install-facts.txt` (:98-122). `AbortWithReason` writes `%TEMP%\HeicFastRecoveryWizard-install-abort.txt` before `Abort` (:83-96).
- **Shortcuts**: the outer script creates **none** — Start-Menu/Desktop shortcuts come from the INNER Tauri setup (all-users), and are removed by the inner uninstaller (`docs/INSTALL-FOOTPRINT-CURRENT.md:97` — "the outer .nsi contains no CreateShortCut").
- **Silent `/SD` discipline**: every MessageBox on the silent path carries `/SD` — Win10 gate `IDOK` (:411), uninstall ask-close `IDCANCEL` (:450), sessions prompt `IDNO` + a `${Silent}` guard that skips it entirely (:468-472).
- **No console spawns in the installer**: the only child processes are the two `ExecWait`s (:221 install, :486 uninstall), both GUI-subsystem NSIS executables driven `/S`. The driver-era `powershell`/`certutil` flashes (FORENSIC-PROBE.md S1) died with the driver removal.

### 1.2 Uninstaller (`Section "Uninstall"`, :434-542)

1. Ask-close via `FindWindow "" "Recovery Wizard"` (the window's real text, :443-455; `/SD IDCANCEL` refuses silently when the app runs).
2. **Sessions kept by default**: interactive `MB_YESNO /SD IDNO` + `${Silent}` guard — silent uninstalls NEVER delete user data (:457-480; `RMDir /r` only on explicit IDYES).
3. Inner Tauri uninstaller run silently: `CopyFiles` to `$TEMP\RW-inner-uninst.exe` then `ExecWait '… /S _?=$INSTDIR'` (:483-488).
4. Kit deletes: explicit list (cr_ui.dll, rrecov.exe, zlib1/z/lz4/liblz4/liblz4d/zstd/zstdd/libzstd) + catch-all `Delete "$INSTDIR\*.dll"` (:495-512).
5. Registry: product Uninstall key + SW key deleted (:515-517); the install-time seed key `Software\heicfast\Recovery Wizard` deleted (the inner uninstaller leaves it) + empty parent (:525-530).
6. Final tidy: `${PROGRAMDATA_LOGS}\install.log` deleted, `RMDir` logs dir, `HeicFastUninstall.exe`, `$INSTDIR`, `$PROGRAMFILES64\HeicFast` (:537-541).

**Uninstall gaps found**:
- `ProgramData\…\logs\app.log` (+ `app.log.old`, NG-15 rotation, `ui/src-tauri/src/bridge/logging.rs:3-9`) is NOT deleted; the `RMDir "${PROGRAMDATA_LOGS}"` at :538 silently fails on a non-empty dir → the logs folder survives an uninstall (orphaned diagnostic data; contradicts "removes EVERYTHING this installer placed", README.md:45-48 — the sessions-keep is consented, the app-log-keep is not).
- T15's runtime-matrix evidence in-tree is **stale** (v0.6.0-rc1 driver era — `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/02-install-facts.txt` still shows cert/driver rows); no in-tree 0.9.x uninstall-footprint evidence.

### 1.3 `installer/devsign.ps1` (177 lines)

- Three actions: `new-cert | sign | verify` (:48-56). Ephemeral self-signed CodeSigningCert `CN=HeicFast Recovery Wizard Development Signing` (:60), created in `Cert:\CurrentUser\My` so it works elevated AND unelevated (:118-125); .cer/.pfx + random password + thumbprint staged beside the payloads (:107-141); the store copy is removed after export (:137).
- `sign`: `signtool sign /fd SHA256` per file (:152-153) + store-independent verification (signer subject must match, :157-164).
- **What gets signed**: `cr_ui.dll` + `rrecov.exe` at engine-lane time (`build.ps1:130-131`), the outer setup exe at installer-lane time (`build.ps1:233`). The **Tauri app exe/bundle is NOT signed** — the documented production-signing gap (`docs/DEV-SIGNING.md`; release notes state it, `release.yml:204-216`).
- No certificate-store import on target machines (user-mode product; :25-30).

### 1.4 `installer/portable.ps1` (86 lines)

- Zip = `recovery-wizard-ui.exe` from `ui/src-tauri/target/release` (:32, :45) + the whole engine kit beside it (:47) + `README-PORTABLE.txt` (:49-70) + the portable marker `recovery-wizard.portable` (:76) → sessions/logs resolve beside the exe (`ui/src-tauri/src/bridge/util.rs:266-288`).
- The README's first instruction: "RUN AS ADMINISTRATOR… Launch from an elevated prompt, **or use the app's restart-as-administrator action when it reports the elevation-required state**" (:57-60) — **STALE**: that action was removed in 0.9.0 (see §6). The portable exe carries the same requireAdministrator manifest (same binary as the installed app), so the OS prompts at launch anyway; the stale sentence should be deleted.
- No devsign step in the portable lane (the app exe is unsigned — same production gap).

### 1.5 What the installer requests, in one line

Admin, per-machine, all-users, Program Files + ProgramData + HKLM; the app it installs *also* requests admin at every launch via its own manifest (§2). One UAC for setup; then one UAC per app launch (except the Finish-page launch, which inherits the installer's token — no second prompt, by design).

---

## 2. The app manifest — content and the embedding chain (D-PRIV-01/D1)

### 2.1 `ui/src-tauri/app.manifest` (49 lines, read completely)

- `<requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>` (**:25**) inside `<trustInfo>` (:22-28) — requireAdministrator, no UIAccess.
- Common-Controls v6 dependency restated verbatim (:29-40) — a custom manifest REPLACES tauri-build's default, and `tauri-plugin-dialog`'s native pickers need it (:16-19).
- `<compatibility>` supportedOS: **Windows 10/11** `{8e0f7a12-…}` and **Windows 8.1** `{1f676c76-…}` (:41-48).
- **DPI awareness: ABSENT** — no `<dpiAware>`/`<dpiAwareness>` windowsSettings block. Tauri v2 sets per-monitor DPI awareness programmatically at runtime (tao's `SetProcessDpiAwarenessContext`), so this is not a user-visible defect, but it is a deviation from tauri-build's stock manifest (which declares it) and worth recording: a custom manifest silently drops every default section not restated.
- Header comment (:2-19) documents the model: token required AT PROCESS CREATION, no in-app relaunch, reference parity (og DRW.exe + DRWUI.exe both requireAdministrator).

### 2.2 How the manifest reaches the shipped exe

1. `ui/src-tauri/build.rs:45-48` — `tauri_build::WindowsAttributes::new().app_manifest(include_str!("app.manifest"))` passed to `tauri_build::try_build(Attributes::new().windows_attributes(...))`. tauri-build (pinned **2.7.4**, `ui/src-tauri/Cargo.lock:3436-3439`) embeds the manifest as the exe's RT_MANIFEST resource through `tauri-winres` 0.3.7 (in the lock graph, `Cargo.lock:3668-3670`) + `embed-resource` 3.0.12 (`Cargo.lock:845-848`). Inert on non-Windows targets (:44).
2. **Build gate (D1)** — `build.ps1:169-190`: after `pnpm tauri build` (`build.ps1:165`), the lane byte-scans `ui\src-tauri\target\release\recovery-wizard-ui.exe` for the ASCII needle `requireAdministrator` and **throws** if absent ("the privilege model broke"). A build without the embedded manifest cannot pass the lane.
3. **Release gate** — `.github/workflows/release.yml:62-67` runs `./build.ps1 -Target all`, so the D1 gate is in the shipping path.
4. **Installed-product gates** — `.github/workflows/forensic-repro.yml:109-123` (post-install: `facts.appManifest` must be `requireAdministrator`; classification regex in `tools/forensic/collect-facts.ps1:32`) and `:174-196` + `tools/forensic/unelevated-launch.ps1:80-125` (an unelevated `CreateProcessWithLogonW` start of the app exe MUST be refused by the OS — if the app ever STARTS unelevated, the gate throws).
5. **Historical truth note**: through v0.8.4 the exe had NO manifest at all (D-PRIV-01, `worklog/REPRODUCTION/D-PRIV-01/README.md:16-37`); the fix landed in 0.9.0 and the v0.9.0 forensic run validated it end to end (`RELEASE-RUN.md:507-508`).

**Verdict: requireAdministrator is actually embedded in the shipped exe as built today.** Evidence chain: source wiring (build.rs:45-48) + lane gate (build.ps1:169-190) + release lane (release.yml:62-67) + on-machine manifest classification + OS-refusal twin (forensic-repro.yml:109-123, 174-196) + the recorded GREEN v0.9.0 result (RELEASE-RUN.md:507). No binary exists on this Linux host to byte-verify 0.9.2 directly (UNVERIFIED-LOCALLY beyond the gate chain).

---

## 3. `build.ps1` (275 lines) — lanes, gates, artifacts

| Lane | Function | What it does | Artifact | Gates | Continue-on-error |
|---|---|---|---|---|---|
| contracts | `Invoke-Contracts` (:48-54) | `node contracts/validate.mjs` (error map, event map, command surface both directions) | — | exit≠0 throws | none |
| engine | `Invoke-Engine` (:56-132) | CMake `win-x64-<cfg>` preset + build (+ ctest Release unless `-SkipTests`, :72-81); PE gates per artifact: `stamp_osversion_um.py` + `verify_pe_um.py --allowlist ref_imports_um.json` (Win7 import allowlist — CreatePipe/SetHandleInformation/CreateProcessW etc. are consciously allowlisted entries, `engine/tools/ref_imports_um.json:2,25`); kit staged `stage-<v>/engine` (cr_ui.dll+lib, rrecov.exe, runtime DLLs; gtest/gmock excluded, :112-121); devsign new-cert + sign cr_ui.dll/rrecov.exe (:127-131) | `build\out\stage-<v>\engine\` | PE stamp/verify (:91-97), kit presence (:119) | none |
| ui | `Invoke-Ui` (:134-195) | pnpm install/typecheck/lint/test (:137-146); stages the kit into `target\{release,debug}` so the bridge links AND loads (:151-163); `pnpm tauri build` (:165); **D1 manifest byte-scan gate** (:169-190) | Tauri NSIS bundle in `target\release\bundle\nsis\` | version lockstep (:37-46), typecheck/lint/test, D1 (:189) | none |
| installer | `Invoke-Installer` (:209-236) | picks the **newest `*-setup.exe`** in `bundle\nsis` by LastWriteTime (:211-213), makensis on `recoverywizard.nsi` (:222-224), devsign-signs the setup exe (:231-234) | `build\out\RecoveryWizard-<v>-setup.exe` | makensis exit (:225), devsign $? (:234) | none |
| portable | `Invoke-Portable` (:238-247) | runs `installer/portable.ps1`; **`if (-not $?)`** not `$LASTEXITCODE` (the cmdlet-vs-native lesson, :241-245) | `build\out\RecoveryWizard-<v>-portable.zip` | $? (:246) | none |
| all | (:266-273) | contracts → engine → ui → installer → portable → checksums | + `SHA256SUMS` (:249-257) | all of the above | none |

- `Assert-VersionConsistency` (:37-46) checks SOURCE files only (ui/package.json, Cargo.toml, tauri.conf.json vs root VERSION).
- **Lane hazards (real, citable)**:
  1. `Invoke-Installer` selects the app bundle by **LastWriteTime, not version** (:211-213). A `-Target installer` run after source changes but without a fresh ui lane silently wraps the STALE bundle; nothing ties the bundle's version to `$Version`.
  2. `-Target portable` similarly copies whatever `target\release\recovery-wizard-ui.exe` sits there (`portable.ps1:32`) — no freshness/version gate.
  3. `-SkipTests` skips ctest + vitest but NOT the D1/PE/devsign gates (those are unconditional) — the privilege gates cannot be skipped.

---

## 4. Elevation flow, end to end (as built today)

1. **Shortcut → exe**: Start-Menu/Desktop shortcut (created by the inner Tauris setup, all-users) → `recovery-wizard-ui.exe` → loader reads RT_MANIFEST ID 1 → `requireAdministrator` → **the OS shows one UAC consent before any UI exists** (or auto-elevates under UAC-off/already-elevated-admin).
2. **Finish-page launch** (`nsi:175`): the app is executed by the still-elevated installer → token inherited, **no second prompt** (by design; the only launch path that never asks).
3. **User declines UAC**: `CreateProcess` fails with ERROR_ELEVATION_REQUIRED (740) — the app never starts; Windows shows its own "administrator permission required" message. Nothing in-app (no banner — `system.rs:8-13`).
4. **In-app posture probes: NONE.** No `IsUserAnAdmin`, no TokenElevation query, no `system_security_posture`, no `system_relaunch_elevated` in the bridge (removed with 0.9.0; `system.rs:4-13`, command list `lib.rs:33-90`). The Rust side keeps NO Win32 elevation code — only `GetDiskFreeSpaceExW` probes (`system.rs:133-155`, `devices.rs:52-75`).
5. **Engine-side refusal vocabulary (the backstop for any host without the shell)**: `ERR_ELEVATION_REQUIRED = 0x500C0001` (`engine/include/cr/cr_error.h:120`); gates at `open_disk` (`cr_ui.cpp:1439-1440`), `open_volume` (:1494-1495), `open_volume_with_key` (:1581-1582), `image_create` (:1731-1732), and health (`cr_ui.cpp:4910-4916`, the BF-28 uniform code — an unelevated ACCESS_DENIED on `\\.\PhysicalDriveN` returns the CODE, not a message-string). `win::is_elevated()` = `OpenProcessToken`+`TokenElevation` (`engine/src/win/cr_elevate.cpp:12-21`). BitLocker store denial carries the separate `ERR_FVE_ELEVATION_REQUIRED 0x50060002` (`cr_error.h:87`; diagnosis seam `cr_bitlocker.cpp:552-555`).
6. **Bridge mapping**: `scan_start` rethrows 0x500C0001 as the engine error (`scan.rs:198-205`), `lost_partition_apply` likewise (`lost_partition.rs:158`); the connector maps it to `'elevation-required'` → reasonKey `errors.elevationRequired` (`tauri-connector.ts:72,138`; `contract.ts:59`). The UI then shows a sonner toast ("Administrator privileges are required for direct disk access. Restart the app as administrator.", `en.ts:543-544`) or, on the health page, an EmptyState (`health-page.tsx:215-219`) — **no restart action exists** (dead vocabulary; unreachable while the manifest holds).
7. **CLI non-elevated**: `require_privilege()` prints a 3-line stderr refusal and the verb exits **4** (`engine/src/app/rrecov.cpp:519-537`; usage text :3906). Image targets (`--image`) and file-only verbs skip the gate (:539-547).
8. **The unelevated twin proof**: `unelevated-launch.ps1:80-125` asserts the OS refuses a standard-user start; if the app process appears, the gate FAILS ("requireAdministrator is not in effect", :113-118).

---

## 5. SUBPROCESS AUDIT — the complete spawn-site inventory

Method: ripgrep over `engine/src/`, `ui/src-tauri/src/`, `ui/src/`, `installer/`, `build.ps1`, `tools/` for `CreateProcess|ShellExecute|_popen|_wsystem|WinExec|system(|Command::new|process::Command|fsutil|powershell|wmic|smartctl|manage-bde|_exec|_spawn`. The **Rust bridge and the frontend spawn NOTHING** (`Command::new`: 0 hits in `ui/`), so every product spawn lives in the C++ engine or the NSIS scripts.

### 5.1 Shipped-runtime spawn sites

| # | Site (file:line) | What it runs | Creation flags / window discipline | Reachable from the UI? | Console can flash? |
|---|---|---|---|---|---|
| 1 | `engine/src/win/cr_health.cpp:392-394` (`query_trim_state`, D-CONS-01/D2 fix) | `fsutil behavior query disabledeletenotify` DIRECTLY (no cmd.exe) | `CreatePipe` + `SetHandleInformation` (:379-383); `STARTF_USESTDHANDLES \| STARTF_USESHOWWINDOW`, `wShowWindow=SW_HIDE`, stdout+stderr→pipe (:386-390); **`CREATE_NO_WINDOW`** (:393); inherit=TRUE only for the write end; bounded 10 s wait + `TerminateProcess` (:402-408) | **NO** — only callers are `cr_ui_enumerate_disks` with `CR_ENUM_FLAG_TRIM` (`cr_ui.cpp:1850-1851`; the bridge calls the plain export, `util.rs:158,165`) and the CLI `--trim` chip (`rrecov.cpp:807`) | **No** — correctly hidden (belt+braces: CREATE_NO_WINDOW *and* SW_HIDE *and* piped stdio; the D2 regression pins assert all of it, `engine/tests/unit/test_no_console_spawn.cpp:80-99`) |
| 2 | `engine/src/win/cr_fsck.cpp:50-51` (`live_spawn`) | `chkdsk.exe` (built by `build_chkdsk_command`, native System32 via WoW64-redirection disable, :39-54) | **`CREATE_NO_WINDOW`** (:51); no stdio redirect (output discarded); bounded wait, NOT killed on timeout (chkdsk-safety, :57-69) | Yes — `cr_ui_fsck_volume` (repair check-phase); requires elevation else `ERR_UNSUPPORTED` before any spawn (:85-86) | **No** |
| 3 | `engine/src/win/cr_bitlocker.cpp:64-65` (`run_command`) | `powershell -NoProfile -NonInteractive -Command "Unlock-BitLocker/Disable-BitLocker …"` (:358-360, :376-381) and `manage-bde` fallbacks (:360, :381) | **`CREATE_NO_WINDOW`** (:65); no stdio redirect; bounded wait (:69-78); fixed-format commands, key material quoted (:81-91) | Yes — the user-initiated unlock/turn-off/dictionary surfaces only (`unlock` :140-151, `turn_off` :159-166, dictionary :175-207) | **No** |
| 4 | `installer/recoverywizard.nsi:221` | inner Tauri setup `/S /allusers /D=…` (GUI-subsystem NSIS exe, silent) | `ExecWait` from a GUI installer — no console semantics apply | install | **No** |
| 5 | `installer/recoverywizard.nsi:486` | inner Tauri uninstaller `/S _?=$INSTDIR` (GUI exe, silent) | `ExecWait` | uninstall | **No** |
| 6 | `ui/src-tauri/src/commands/system.rs:307-328` (`system_open_path`) | explorer reveal/open via `tauri-plugin-opener` (`reveal_item_in_dir` / `open_path`) | plugin-internal ShellExecute-class launch of Explorer — GUI, user-initiated | Yes — "Open location" context action | **No** (no console binary involved) |
| 7 | `tauri-plugin-dialog` (`system.rs:284-304` `system_pick_directory`) | native folder picker | OS dialog | Yes | **No** |
| 8 | WebView2 runtime | `msedgewebview2.exe` child processes | GUI browser processes | always | **No** |
| 9 | `tauri.conf.json:49-51` (`webviewInstallMode: downloadBootstrapper`) | `MicrosoftEdgeWebview2Setup.exe` — **install-time only**, when the runtime is absent | Microsoft bootstrapper UI (standard installer chrome; possibly visible) | install on a machine without WebView2 | **No console; possible standard installer UI** (flagged in `docs/INSTALL-FOOTPRINT-CURRENT.md:332`) |

### 5.2 Non-UI / build-time / diagnostic spawns (not shipped-runtime surfaces)

| Site | What | Notes |
|---|---|---|
| `engine/src/app/rrecov.cpp:807` | fsutil TRIM chip (CLI `--trim`) | console app by design; same hidden discipline via `query_trim_state` |
| `rrecov.exe` itself (`engine/src/app/CMakeLists.txt:1` — `add_executable(rrecov rrecov.cpp)`, no WIN32 flag) | console-subsystem CLI | shipped in `$INSTDIR` (kit) but **never spawned by the app** (0 `Command::new` in the bridge) — terminal-only |
| `build.ps1` (:51, :68-80, :93-96, :128-133, :152-165, :222-224) | node/cmake/ctest/python/pnpm/makensis/devsign | build-time only |
| `installer/devsign.ps1:152` | `signtool sign` | build-time only |
| `installer/portable.ps1:80` | `Compress-Archive` | build-time only |
| `tools/forensic/*.ps1`, `.github/workflows/*` | pwsh watchers, seeders, net.exe cleanup | diagnostic/CI tooling, never shipped |

### 5.3 The build-time no-console gate

`engine/tests/unit/test_no_console_spawn.cpp` (162 lines) pins, on every engine lane (incl. the Linux stub lane):
- **No `_popen`/`_pclose`/`WinExec`/`ShellExecute*`** in the 13 shipping source files (list at :34-40) — the only sanctioned vehicle is `CreateProcessW + CREATE_NO_WINDOW` (:30-55).
- **No CRT `system(`** in the win layer (:57-64).
- `health_read_impl` MUST source TRIM from the per-device IOCTL `query_trim_state_handle` (:66-78).
- `query_trim_state` MUST carry `CREATE_NO_WINDOW` + `SW_HIDE` + `STARTF_USESTDHANDLES` + `TerminateProcess` (:80-99).
- `cr_fsck.cpp` / `cr_bitlocker.cpp` MUST keep `CREATE_NO_WINDOW` (:101-112).
- The health path's unelevated refusal MUST carry `ERR_ELEVATION_REQUIRED` (:114-130).

**Verdict: every shipped spawn site sets the correct hidden-window flags; no site lacks them.** The one pre-0.9.0 offender (`_popen("fsutil … 2>nul")`, v0.8.4's `cr_health.cpp:306`) is gone, and its class is regression-pinned.

---

## 6. The restart-as-admin prompt — theming diagnosis

- **In the current tree the component does not exist.** The 0.8.4 surface was `ElevationBanner` (`ui/src/features/consent/elevation-banner.tsx`) + `system_relaunch_elevated` (`ShellExecuteW` verb "runas"); both were REMOVED in the 0.9.0 manifest fix (`worklog/REPRODUCTION/D-PRIV-01/README.md:39-47`; `system.rs:8-13` "NO banner, and NO ShellExecuteW runas relaunch"; glob `ui/src/**/*elevation*` = 0 hits; no relaunch/posture command in `lib.rs:33-90`).
- **What the user saw (the "broken theme")** was that 0.8.x banner: an **amber** palette (`border-amber-300 bg-amber-50 text-amber-900` with a ShieldAlert icon) mounted on every route, inside a product whose shell is blue (`#EBF2FF` frame, `#1677FF` accent — the current theme tokens at `ui/src/theme/adopted/tokens.css`, theme application via `ui/src/app/theme.tsx:74-123` `data-theme`/`data-scheme`). It was NOT a standalone window and not a system message box — it was an in-app React component whose hardcoded amber classes ignored the product's token system (recorded as D-UI-01, `worklog/REPRODUCTION/D-PRIV-01/README.md:24-27` "the off-theme banner surface").
- **What remains in 0.9.2**: only the i18n refusal copy `errors.elevationRequired` = "…Restart the app as administrator." (`en.ts:543-544`, `zh-CN.ts:527`) rendered as a sonner toast (`Toaster` mounted inside `ThemeProvider`, `ui/src/app/providers.tsx:22-27` — theme-consistent) or a health-page EmptyState (`health-page.tsx:215-219`). **There is no button/action to actually restart** — if the vocabulary ever fires (a host running the engine without the product shell), the user gets advice, not an affordance.
- **Stale references to the removed action (should be trued)**:
  - `installer/recoverywizard.nsi:331` — the privilege page tells the user the app shows "a clear elevation-required notice and a restart-as-administrator action" (the action no longer exists).
  - `installer/portable.ps1:58-59` — README-PORTABLE: "or use the app's restart-as-administrator action when it reports the elevation-required state."
  - `ui/src-tauri/Cargo.toml:22-23` — comment still cites "the relaunch-as-admin verb" while `Win32_UI_Shell`/`Win32_UI_WindowsAndMessaging`/`Win32_System_Threading` windows-sys features (:27-34) are now unused by any elevation code.
  - `ui/src-tauri/src/bridge/error.rs:18` — comment still describes "the elevation-required banner + relaunch flow".

---

## 7. The console flash on disk health — ranked explanation

The user reports consoles (cmd or powershell) flashing when opening disk health, and prior sessions claimed it fixed. Current-tree facts:

1. **The 0.9.2 health path spawns NOTHING.** `health_read` (bridge `health.rs:19-67`) → `cr_ui_health_read` → `health_read_impl` (`cr_ui.cpp:4888-4990`): opens `\\.\PhysicalDriveN`, reads SMART/NVMe via IOCTL, `looks_like_ssd` via IOCTL, TRIM via `query_trim_state_handle` IOCTL (`cr_ui.cpp:4977-4984` → `cr_health.cpp:431-459`). The old `_popen` call is gone; `FORENSIC-PROBE.md:90-91`'s "(now replaced with the native IOCTL)" is now TRUE (it was stale at v0.8.4).
2. **Ranked explanations for the user's observation**:
   - **#1 (most likely): the machine is running a pre-0.9.0 build (v0.8.4 or earlier).** That binary (a) had no manifest → no UAC at launch, (b) had the amber restart-as-admin banner (off-theme), and (c) flashed `cmd.exe /c fsutil … 2>nul` on EVERY health read (the `_popen`, `worklog/REPRODUCTION/D-CONS-01/README.md:14-25`). All three reported symptoms are the v0.8.4 signature, simultaneously. The user needs to install the current release (or the 0.9.x install never reached that machine — old installer artifact, unreplaced portable zip, or an in-place update that silently failed; note the inner-setup failure path aborts with a %TEMP% trace, `nsi:223-226`, which a user would never see).
   - **#2: a stale-bundle installer.** `build.ps1:211-213` picks the newest `*-setup.exe` by LastWriteTime — a locally-built installer over an old ui-lane output would wrap the 0.8.x app exe inside a 0.9.x-versioned installer. (The official release lane builds all lanes fresh, so this applies to hand-built artifacts.)
   - **#3: a debug/dev build in the wild.** `main.rs:2` only sets `windows_subsystem = "windows"` for `not(debug_assertions)` — a debug build attaches a console by design; and `cargo run`/`tauri dev` sessions show consoles for the app itself.
   - **#4: WebView2 bootstrapper** (install-time, only when the runtime is missing) — Microsoft's standard installer UI, not a console, and not tied to disk health.
   - **#5: AV/SmartScreen or explorer-shell interference** on the unsigned app exe (the production-signing gap) — speculative, last resort.
3. **If the report is against a VERIFIED 0.9.2 install**, then the current code cannot produce it (no reachable spawn on the health path — §5.1 row 1's only UI-reachable cousins are IOCTL-only), and the next diagnostic step is the shipped app log (`%ProgramData%\HeicFast\Recovery Wizard\logs\app.log`, NG-15) plus `tools/forensic/io-probe.ps1` (a child-process census keyed to the app PID, sampling 2 s — with the honest caveat that a millisecond flash can slip sampling; `D-CONS-01/README.md:26-32`).

---

## 8. Launch chain + PE subsystem audit

- **Chain**: Start-Menu/Desktop shortcut (inner Tauri setup, all-users) → `$INSTDIR\recovery-wizard-ui.exe` → RT_MANIFEST #1 `requireAdministrator` → UAC → GUI window (frameless, `decorations: false`, `tauri.conf.json:25`).
- **Subsystem of every shipped binary**:
  - `recovery-wizard-ui.exe` — **windows subsystem in release** (`ui/src-tauri/src/main.rs:2`); debug builds are console by design (dev-only).
  - `cr_ui.dll` — DLL (no subsystem).
  - `rrecov.exe` — **console subsystem** (`engine/src/app/CMakeLists.txt:1`, no WIN32 flag) — intentional (a terminal CLI), shipped beside the app, never spawned by it.
  - Outer installer + inner Tauri setup/uninstaller — NSIS GUI executables.
  - No `/SUBSYSTEM` directives anywhere else in `engine/CMakeLists.txt` (grep: only `_WIN32_WINNT` defines :30 and the vcpkg deploy function :156).
- **No `_popen`/`system` anywhere in shipping code** (§5.3 gate) — the CRT console-allocation class is structurally banned.

---

## 9. Installer/uninstall gap list (the honest residue)

| # | Gap | Evidence | Severity |
|---|---|---|---|
| 1 | Stale claims about the removed restart-as-admin action, shipped to users in the installer's privilege page and the portable README | `nsi:331`, `portable.ps1:58-59` | low (text), but it is user-facing untruth |
| 2 | `app.log`/`app.log.old` survive uninstall (logs dir RMDir fails non-empty) | `nsi:537-541` vs `logging.rs:3-9` | low |
| 3 | Installer lane can wrap a stale Tauri bundle (newest-by-LastWriteTime, no version tie) | `build.ps1:211-213` | medium (release-lane-safe; hand-build hazard) |
| 4 | Portable lane has no freshness/D1 gate of its own (trusts the ui lane's exe) | `portable.ps1:32-45` | low |
| 5 | Tauri app exe + portable zip unsigned (production-signing gap; SmartScreen warnings on first run) | `docs/DEV-SIGNING.md`, `release.yml:204-216` | medium (UX/trust, not privilege) |
| 6 | T15 runtime-matrix evidence in-tree is v0.6.0-rc1-era (driver rows) — no current-tree uninstall footprint proof | `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/02-install-facts.txt` | low (evidence debt) |
| 7 | Custom manifest drops DPI-awareness declaration (runtime-compensated by tao) and adds only Win8.1+10 GUIDs (no Win8/Win7 GUIDs — consistent with the Win10 gate, `nsi:400-413`) | `app.manifest:41-48` | informational |
| 8 | Dead vocabulary: `errors.elevationRequired` copy advises a manual restart with no affordance, and `Win32_UI_Shell`-class windows-sys features are unused | `en.ts:543-544`, `Cargo.toml:22-34`, `error.rs:18` | informational |

---

## 10. Source index (files read in full for this audit)

installer/{recoverywizard.nsi (543L), devsign.ps1 (177L), portable.ps1 (86L), README.md} · build.ps1 (275L) · ui/src-tauri/{app.manifest, build.rs, Cargo.toml, tauri.conf.json, main.rs, Cargo.lock (pins)} · ui/src-tauri/src/{lib.rs, bridge/{util.rs, logging.rs (head), error.rs (grep)}, commands/{system.rs, devices.rs, health.rs, scan.rs (excerpt)}} · engine/src/win/{cr_health.cpp (TRIM section), cr_fsck.cpp (full), cr_bitlocker.cpp (full), cr_elevate.cpp (full)} · engine/src/ui/cr_ui.cpp (enumerate :1829-1887, health :4880-5023, elevation gates :1434-1732) · engine/src/app/rrecov.cpp (:519-547, :3906) · engine/tests/unit/test_no_console_spawn.cpp (full) · engine/src/app/CMakeLists.txt + engine/CMakeLists.txt (subsystem/deploy) · ui/src/{connector/tauri/tauri-connector.ts (full), connector/contract.ts (grep), i18n/en.ts + zh-CN.ts (grep), features/health/health-page.tsx (excerpt), app/providers.tsx + app/theme.tsx (grep)} · .github/workflows/{release.yml (full), forensic-repro.yml (:95-214)} · tools/forensic/{collect-facts.ps1 (full), unelevated-launch.ps1 (:80-126)} · contracts/README.md · docs/{FORENSIC-PROBE.md, INSTALL-FOOTPRINT-CURRENT.md (grep), INTEGRATION-VALIDATION/t15-runtime-matrix/* (heads)} · worklog/{REPRODUCTION/D-PRIV-01/README.md, REPRODUCTION/D-CONS-01/README.md, lane-b/INSTALLER-AND-PRIVILEGE.md, RELEASE-VALIDATION/RELEASE-RUN.md (:460-531)}.

*No file was modified except this one. No git commands were run.*
