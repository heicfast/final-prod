# INSTALLER-CURRENT — the product's installer and installed footprint as it exists at HEAD

> **Task ID 2-a (Phase 0, Agent A)** — Installer Integration & API Surface
> Expansion session, 2026-11-18. READ-ONLY research: static reading only; the
> ONLY file written by this task is this sheet. No build, no tests, no commit.
> Product repo: `/home/z/my-project/repos/final-prod`, read at the **8217bc26**
> frontier ("docs(T19): the two footprint docs point to the current
> reconciliation"). The sandbox checkout carries exactly one later commit
> (`0c2c3c52`, worklog-only — the T1 session charter; zero product-code delta),
> so every product file cited below is byte-identical to 8217bc26.
> Sources read exhaustively: `installer/recoverywizard.nsi` (549 lines),
> `installer/{README.md,devsign.ps1,portable.ps1}`,
> `ui/src-tauri/{tauri.conf.json,app.manifest,build.rs,src/**}`,
> `build.ps1`, `build.sh`, `docs/{BUILD,INSTALLER,INSTALL-FOOTPRINT-CURRENT,
> INSTALL-FOOTPRINT-DELTA,INTEGRATION-VALIDATION,DEV-SIGNING,FORENSIC-PROBE}.md`,
> `docs/INTEGRATION-VALIDATION/{installer-compile-check.md,t15-runtime-matrix/*,
> t14-measurements/*}`, `tools/validation/installer-matrix.ps1`,
> `tools/forensic/*.ps1` + `drive-ui.mjs`, `engine/{CMakeLists.txt,
> cmake/DeployVcpkgBinaries.cmake,docs/PACKAGING.md,docs/PREBUILT-ARTIFACTS.md}`,
> `contracts/README.md`, `.github/workflows/{release,forensic-repro,
> validation-evidence}.yml`, `worklog/{FOOTPRINT-DELTA.md,INSTALLER-AND-PRIVILEGE.md}`
> + the D-PRIV-01 reproduction evidence.
> Anything unverifiable is marked NOT-FOUND or UNKNOWN. Nothing is invented.

**Executive summary.** The product ships one combined NSIS installer
(`RecoveryWizard-<version>-setup.exe`, built from `installer/recoverywizard.nsi`)
that wraps the stock Tauri NSIS bundle as a silent inner payload and adds three
product concerns the inner bundle knows nothing about: the native engine kit
(`cr_ui.dll` + `rrecov.exe` + the vcpkg runtime DLLs, laid BESIDE the app exe
because the Rust bridge statically imports `cr_ui.dll`), the machine-level
shared state (`%ProgramData%\HeicFast\Recovery Wizard\{sessions,logs}`), and a
complete uninstaller (`HeicFastUninstall.exe`) that removes everything the
installer placed, keeping user sessions unless an interactive user consents to
their deletion. The installed footprint as defined is a flat ~8-file
`C:\Program Files\HeicFast\Recovery Wizard` plus the ProgramData tree plus three
HKLM registry keys — no driver, no service, no certificate store, no
`System32` writes (the driver era ended at commit 318b9655, 2026-11-05). The
build is one command on Windows (`build.ps1 -Target all`: contracts → engine →
ui → installer → portable → checksums) and a Linux subset (`build.sh`). The
posture that matters for this session's mandate: **every committed runtime
validation evidence (T15 matrix, T14 measurements, forensic captures) is
driver-era v0.6.0-rc1 (2026-10-27) and predates the driver-free rewrite** —
the tools have been rewritten to assert the driver-free footprint, but no
committed evidence records a run of them against the current 0.9.3 installer;
and one real placement gap exists: `userdata-defaults.json` (the 111-entry
default user-signature set) is probed by both the CLI and the bridge exe-adjacent
at startup, exists in the repo at `engine/data/`, and is placed by nothing.

---

## 1. Build & packaging pipeline

Entry points: `build.ps1` (Windows, full product — `build.ps1:2-9`) and
`build.sh` (Linux subset: contracts + engine Linux presets + ui static checks;
**no installer and no portable lane exist on Linux** — `build.sh:2-9,45-51`).
Version source of truth: root `VERSION` = `0.9.3` (read `VERSION:1`); the first
gate `Assert-VersionConsistency` fails on drift in `ui/package.json`,
`ui/src-tauri/Cargo.toml`, `ui/src-tauri/tauri.conf.json` (`build.ps1:37-46`).

Targets (`build.ps1:21`): `all | contracts | engine | ui | installer |
portable`. The `all` lane order (`build.ps1:277-284`):

| # | Lane | What it does | Where output lands |
|---|---|---|---|
| 0 | VERSION gate | lockstep check (`build.ps1:37-46`) | — |
| 1 | contracts | `node contracts/validate.mjs` (`build.ps1:48-54`) | gate only |
| 2 | engine | `cmake --preset win-x64-<cfg>` + ctest (Release only, `build.ps1:66-81`) → PE gates `stamp_osversion_um.py` + `verify_pe_um.py --allowlist ref_imports_um.json` on `rrecov.exe` + `cr_ui.dll` (`build.ps1:84-97`) → **kit staging** (§2) → `devsign.ps1 new-cert` + sign `cr_ui.dll`,`rrecov.exe` (`build.ps1:112-131`) | `build\out\stage-<v>\engine\` (kit); engine build tree `engine\build\win-x64-<cfg>\src\{app,ui}\<Config>\` |
| 3 | ui | `pnpm install/typecheck/lint/test` → copy the staged kit into `src-tauri\target\{release,debug}\` + `CR_UI_LIB_DIR=<kit>` (`build.ps1:148-163`) → `pnpm tauri build` (`build.ps1:165-166`) → **D1 gate**: byte-scan for `requireAdministrator` in the built exe (`build.ps1:169-190`) | `ui\src-tauri\target\release\recovery-wizard-ui.exe` + the Tauri NSIS bundle at `ui\src-tauri\target\release\bundle\nsis\*-setup.exe` (`build.ps1:192-194`) |
| 4 | installer | pick the version-locked bundle (`*-<v>-setup.exe`, **I-STALE-01**: never LastWriteTime, hard stop on mismatch — `build.ps1:211-224`) → `Find-Makensis` (Tauri-bundled `%LOCALAPPDATA%\tauri\NSIS\makensis.exe`, system NSIS fallback — `build.ps1:197-207`) → `makensis /DPRODUCT_VERSION /DAPP_BUNDLE /DENGINE_KIT /DOUT_DIR /DPATHSEP=\` on `installer\recoverywizard.nsi` (`build.ps1:233-236`) → `devsign.ps1 sign` on the setup exe (`build.ps1:242-246`) | **`build\out\RecoveryWizard-<v>-setup.exe`** (`.nsi:145`) |
| 5 | portable | `installer/portable.ps1` (§4 note): app exe + kit beside it + `recovery-wizard.portable` marker + `README-PORTABLE.txt` → zip (`portable.ps1:41-86`) | `build\out\RecoveryWizard-<v>-portable.zip` |
| 6 | checksums | SHA256SUMS over every artifact in `build\out` (`build.ps1:260-268`) | `build\out\SHA256SUMS` |

Staging locations: `$OutDir = build\out`, `$StageDir = build\out\stage-<version>`
(`build.ps1:31-33`); the devsign stage certificate (`devsign.pfx`,
`devsign-pfx-password.txt`, `devsign.cer`, `devsign-thumbprint.txt`) lives in
the stage dir beside the kit (`devsign.ps1:109-135`), and `release.yml`
deliberately excludes `stage-*/` + `devsign.*` from artifact upload
(`.github/workflows/release.yml:276-282` — the closed NG-17 hygiene fix).

**The one-command-from-clean-checkout claim vs reality.** Claim:
`docs/INSTALLER.md:117-120` ("One command from a clean checkout —
`.\build.ps1 -Target all`") and `docs/BUILD.md:4-13`. Reality:

- True **only on Windows** with the full toolchain present (VS2022 MSVC v143 +
  Windows SDK, CMake ≥ 3.21 + vcpkg manifest mode, Node 24 + pnpm 12.15, Rust
  stable, NSIS/makensis — `docs/BUILD.md:51-59`). `build.sh` cannot produce an
  installer (`build.sh:3-4`).
- The single lanes `installer` and `portable` are **not standalone**:
  `-Target installer` runs only `Invoke-Installer` (`build.ps1:275`) and hard-
  stops if the ui lane has not produced a version-matching bundle
  (`build.ps1:219-223`); `portable.ps1` throws "run the ui lane first" /
  "run the engine lane first" on missing inputs (`portable.ps1:33-39`). Only
  `-Target all` (and `engine`/`ui`, which prepend `Invoke-Contracts`) is
  self-contained from a clean checkout.
- The claim is version-locked and stale-proof (I-STALE-01) and its output is
  deterministic in shape — but the WebView2 runtime is **not** built by the
  product: the inner Tauri setup uses `downloadBootstrapper`
  (`tauri.conf.json:49-51`), so an install on a machine without WebView2 and
  without network cannot complete from this one command's output alone
  (UNKNOWN-at-evidence level; see §8 G-7).

---

## 2. The staged engine kit

The engine lane stages the kit into `build\out\stage-<v>\engine\`
(`build.ps1:112-121`). Every file class:

| File class | Source | Staged by | Placed on machine by | Notes |
|---|---|---|---|---|
| **`cr_ui.dll`** — the host-binding C ABI DLL (92 `cr_ui_*` exports, ABI v4 — `contracts/README.md:13`, `engine/include/cr/cr_ui.h:81`) | `engine\build\win-x64-<cfg>\src\ui\<Config>\cr_ui.dll` (`build.ps1:114`) | `Copy-Item` (`build.ps1:114`) | outer `.nsi` `File /x cr_ui.lib "${ENGINE_KIT}${PATHSEP}*"` (`recoverywizard.nsi:255`) | dev-signed (`build.ps1:130`); links `cr_core` + `cr_win` (`engine/src/ui/CMakeLists.txt:3-11`) |
| **`cr_ui.lib`** — the link-time import library | same dir (`build.ps1:114`) | `Copy-Item` | **NOT placed** — the installer excludes it (`recoverywizard.nsi:255`); it IS shipped inside the portable zip (`portable.ps1:47` copies the whole kit) | link-time artifact only; the bridge needs it at build (`ui/src-tauri/build.rs:33`) |
| **`rrecov.exe`** — the engine CLI (scan/tree/recover/health/fix/apply/bench) | `src\app\<Config>\rrecov.exe` (`build.ps1:115`) | `Copy-Item` | same `File` (`recoverywizard.nsi:255`) | dev-signed (`build.ps1:130`); placed but NOT invoked by the UI (documented: `docs/INSTALL-FOOTPRINT-CURRENT.md` §A.1 row 4) |
| **vcpkg runtime DLLs** (zlib/lz4/zstd classes; names vary by port/config: `z.dll`/`zlib1.dll`, `lz4.dll`/`liblz4.dll`, `zstd.dll`/`libzstd.dll`) | deployed beside `cr_ui.dll` by the engine's own POST_BUILD `cr_deploy_vcpkg_binaries` (`engine/CMakeLists.txt:156-167`; `engine/src/ui/CMakeLists.txt:46`; `engine/cmake/DeployVcpkgBinaries.cmake` — copies every matching-config DLL from `vcpkg_installed/<triplet>/bin`, deterministic, name-agnostic) | glob-staged with the **gtest/gmock exclusion** (`build.ps1:116-118`) | same `File` (`recoverywizard.nsi:255`) | the 0.6.x observed set was `z.dll`,`lz4.dll`,`zstd.dll` (`t15-runtime-matrix/03-after-install.json` inventory); the .nsi only asserts "≥1 DLL beside cr_ui.dll" (`recoverywizard.nsi:264-273`) |

The deploy mechanism's own rationale (why POST_BUILD copies instead of
z-applocal, why single deployer): `engine/cmake/DeployVcpkgBinaries.cmake:1-20`.
The kit curation rationale (the round-13 gtest/gmock leak that shipped test
runtimes to customers and broke the uninstaller's delete list):
`build.ps1:99-111`.

**NOT staged, NOT placed — the include/ and data-defaults classes.** The
engine repo's own release convention ships `include/` and
`userdata-defaults.json` beside `rrecov.exe`
(`engine/docs/PREBUILT-ARTIFACTS.md:51` — "carries `userdata-defaults.json`
beside `rrecov.exe` (M-065 …111 entries, the engine-init default user-sign
set; absent file = the empty registry + one log line, never an error)"). In
final-prod's kit, neither class is staged: `build.ps1:112-121` copies exactly
the four classes above. Consequences on the product:

- `engine/data/userdata-defaults.json` (747 lines, 111 extensions) exists in
  the repo but no lane stages it, `tauri.conf.json` declares no `resources`
  (grep over `ui/src-tauri/tauri.conf.json` — no `resources` key), and the
  `.nsi` copies only the staged kit. **The installed product never receives
  the default user-signature set**, while BOTH consumers probe for it
  exe-adjacent at startup: the CLI (`engine/src/app/rrecov.cpp:1354`) and the
  bridge (`ui/src-tauri/src/commands/system.rs:85-112`,
  `USER_SIGN_DEFAULTS_FILE` at `:34`, invoked from `lib.rs:133`). The
  documented degradation is a logged no-op (`system.rs:83-84`) — the About
  row's `userSigns` count will read 0 on every clean install. See §8 G-1.

---

## 3. NSIS placement rules (`installer/recoverywizard.nsi`)

Header contract: one artifact installs the entire product — app (inner Tauri
NSIS setup, driven silent), engine kit, machine-level shared state
(`recoverywizard.nsi:4-8`). Defines required from `build.ps1`:
`PRODUCT_VERSION`, `APP_BUNDLE`, `ENGINE_KIT`, `OUT_DIR` (`!error` guards
`recoverywizard.nsi:54-65`); `PATHSEP` defaults `/` (Linux cross-compile
validation form) and Windows passes `\` (`recoverywizard.nsi:73-81`).

Global posture:

- `Unicode true`, `SetCompressor /SOLID lzma` (`:51-52`).
- `Name "HeicFast Recovery Wizard <v>"`, `OutFile ${OUT_DIR}\RecoveryWizard-<v>-setup.exe` (`:144-145`).
- `InstallDir "$PROGRAMFILES64\HeicFast\Recovery Wizard"` (`:146`).
- `RequestExecutionLevel admin` (`:162`) — ONE UAC at installer launch; rationale `.nsi:30-36`, `installer/README.md` deviation #1.
- **All-users ONLY**: `SetShellVarContext all` in `.onInit` (`:417`) and uninstall (`:436`), `SetRegView 64` (`:399`,`:425`); MultiUser.nsh gone, no per-user context offered (`.nsi:158-160`; `installer/README.md` deviation #5).
- Win10+ gate in `.onInit` via `CurrentMajorVersionNumber` (`:404-413`; `/SD IDOK` + `Abort` — exit 2, truthful refusal on Win7/8; the engine keeps its own Win7 envelope).

Pages (MUI2): Welcome (`:178`) → License, repo-root MIT `LICENSE` (`:179`) →
custom informational **Privilege page** (`:180`; `PrivilegePageCreate`
`:319-337` — three nsDialogs labels: user-mode product, launch-time elevation
posture, uninstall-keeps-sessions) → Directory (`:181`) → InstFiles (`:182`) →
Finish with "Launch HeicFast Recovery Wizard" run checkbox
(`MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"`, `:175-176`).

Sections:

- **`SEC_APP` "HeicFast Recovery Wizard (required)"** (`SectionIn RO`, `:191-274`):
  1. Stage the inner bundle: `File "/oname=$PLUGINSDIR\app-setup.exe" "${APP_BUNDLE}"` (`:208`).
  2. **Seed the inner installer's install-location registry**: `WriteRegStr SHCTX "Software\heicfast\Recovery Wizard" "" "$INSTDIR"` (`:219`) — steers Tauri's `RestorePreviousInstallLocation` (identifier `com.heicfast.recoverywizard` → manufacturer `heicfast`); the key is owned by the inner but cleaned by ours.
  3. Drive the inner setup **silent**: `ExecWait '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR' $0` (`:221`) — ExecWait, deliberately NOT nsExec (nsExec re-quotes `/D=`, which the NSIS parser then ignores — validation round 8's diagnosis, `.nsi:197-203`); the fact line records the exit code (`:222`); nonzero → `AbortWithReason` (`:223-226`).
  4. Integrity + **adoption**: `$INSTDIR\recovery-wizard-ui.exe` must exist; if the inner landed in its own default `"$PROGRAMFILES64\${PRODUCT_NAME}"`, `$INSTDIR` adopts that (`:233-242`).
  5. **Engine kit**: `SetOutPath "$INSTDIR"` + `File /x cr_ui.lib "${ENGINE_KIT}${PATHSEP}*"` (`:254-255`) — the DLLs ride BESIDE the app exe (loader truth: static imports resolve from the exe's directory; an `engine\` subfolder would 0xc0000135, `.nsi:244-250`). Gates: `cr_ui.dll` present (`:257-259`); ≥2 DLLs total in `$INSTDIR` (`:264-273`).
- **`-post` hidden section** (`:276-313`) — shared state, registry, uninstaller:
  - `CreateDirectory "${PROGRAMDATA_SESSIONS}"` / `"${PROGRAMDATA_LOGS}"` (`:280-281`) — default ACL inheritance (deviation #2, `installer/README.md:89`).
  - Product SW key `HKLM\Software\HeicFast\Recovery Wizard` ← `InstallDir`, `Version`, `InstallMode=AllUsers` (`:285-287`) **with read-backs into the facts file** (`:291-296`; the round-13 lesson).
  - `Call RemoveInnerUninstallKeys` (`:302`; function `:344-392`): sweeps **both** HKLM and HKCU `Uninstall` trees, deleting any key whose `UninstallString` contains `$INSTDIR` — then the single product entry is written.
  - Add/Remove entry `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Recovery Wizard`: `DisplayName`, `DisplayVersion`, `Publisher`, `InstallLocation`, `UninstallString "$INSTDIR\HeicFastUninstall.exe"`, `QuietUninstallString … /S`, `NoModify=1`, `NoRepair=1` (`:303-310`).
  - `WriteUninstaller "$INSTDIR\HeicFastUninstall.exe"` (`:312`).

Macros: `AbortWithReason` (`:89-96`) — writes
`%TEMP%\HeicFastRecoveryWizard-install-abort.txt` (the ONLY trace a silent
abort leaves) then `Abort`. `InstallFact` (`:111-122`) — appends one line to
TWO sinks: the persistent `${PROGRAMDATA_LOGS}\install.log` and the
`%TEMP%\HeicFastRecoveryWizard-install-facts.txt` evidence contract.

**Registry total (install)**: 3 HKLM trees — the SW key (3 values), the
Uninstall key (8 values), and the seeded `Software\heicfast\Recovery Wizard`
default value. **Shortcuts**: the outer script contains **no `CreateShortCut`
at all** (grep over `installer/recoverywizard.nsi` — zero matches); every
shortcut comes from the INNER Tauri setup's own template behavior
(`docs/INSTALL-FOOTPRINT-CURRENT.md` §A.3 shortcut row: "created by the INNER
Tauri setup … not asserted by T15"). The exact shortcut set is therefore
UNKNOWN from the repo alone (the Tauri NSIS template is not committed).

**Uninstaller (`Section "Uninstall"`, `:434-548`)** — removes everything this
installer placed:

1. `SetShellVarContext all` (`:436`).
2. Ask-close: `FindWindow $0 "" "Recovery Wizard"` — the window's REAL text
   (`tauri.conf.json` `app.windows[0].title`, `.nsi:139,439-442`); `MessageBox
   MB_RETRYCANCEL /SD IDCANCEL` — silent uninstall with the app running
   **refuses** (`Abort`), removes nothing (`:443-455`).
3. Sessions: `${FileExists} sessions\*.*` → `${Silent}` guard skips the
   question (silent ALWAYS keeps) → interactive `MB_YESNO /SD IDNO`
   (default KEEP); YES → `RMDir /r "${PROGRAMDATA_ROOT}"` (`:458-480`).
   Reference parity: user data survives uninstall.
4. Inner Tauri uninstaller, silent: `CopyFiles /SILENT` to
   `$TEMP\RW-inner-uninst.exe` + `ExecWait '"$TEMP\RW-inner-uninst.exe" /S _?=$INSTDIR'` + `Delete /REBOOTOK` (`:483-488`).
5. Our own files: explicit `/REBOOTOK` deletes of `cr_ui.dll`, `rrecov.exe`,
   and the known zlib/lz4/zstd name variants (`:495-504`) PLUS the catch-all
   `Delete "$INSTDIR\*.dll"` (`:512`) — the round-13 lesson (a hardcoded list
   leaked gtest/gmock; by this point every remaining DLL is ours).
6. Registry: `DeleteRegKey` the Uninstall key + the SW key (`:515-516`), the
   seeded `Software\heicfast\Recovery Wizard` key **and its empty `heicfast`
   parent** (`:525-530` — the inner uninstaller does NOT remove the seed; run-13
   evidence).
7. Final tidy: `install.log`, `app.log`, `app.log.old` (I-CLEAN-01, `:537-543`),
   `RMDir` logs dir, `Delete /REBOOTOK HeicFastUninstall.exe`, `RMDir $INSTDIR`,
   `RMDir "$PROGRAMFILES64\HeicFast"` (`:544-547`).

**Program Files vs ProgramData vs per-user split** (complete):

| Tree | Placed by | Contents |
|---|---|---|
| `C:\Program Files\HeicFast\Recovery Wizard\` (`$INSTDIR`, `.nsi:146`) | inner setup (silent) + outer kit `File` + `WriteUninstaller` | app exe, inner `uninstall.exe`, engine kit, `HeicFastUninstall.exe` — flat, no subdirectories (`tools/validation/installer-matrix.ps1:256-259` asserts flatness) |
| `C:\ProgramData\HeicFast\Recovery Wizard\{sessions,logs}\` (`.nsi:140-142`, `:280-281`; constants defined ONCE in `contracts/README.md:47-48`) | outer installer (`CreateDirectory`), the app at runtime (sessions `.crsf`, `app.log`) | machine-level shared state; default inherited ACLs |
| Per-user anything | **nothing** — no per-user context, no HKCU writes, no per-user dirs (`.nsi:158-160`; `installer/README.md` deviation #5) | — |

---

## 4. Tauri bundling (`ui/src-tauri/tauri.conf.json` + `app.manifest`)

What `tauri.conf.json` contributes to the bundle:

- `productName "Recovery Wizard"`, `version "0.9.3"` (lockstep-gated,
  `tauri.conf.json:3-4`), `identifier "com.heicfast.recoverywizard"` (`:5`) —
  the identifier's manufacturer segment `heicfast` is what the .nsi seed key
  mirrors (`.nsi:210-219`).
- App binary name: **`recovery-wizard-ui.exe`** — the Cargo package name;
  no `mainBinaryName` override (`tauri.conf.json` grep — absent; `.nsi:138`
  records the rationale).
- Window: title "Recovery Wizard" (the uninstaller's FindWindow text,
  `tauri.conf.json:16`), 1240×840 frameless maximized (`:14-26`).
- `bundle.targets: ["nsis"]` (`:34`) — the ONLY bundle target; icons
  `32x32/128x128/128x128@2x/icon.ico/icon.png` (`:35-41`) — also the MUI
  icon of the outer installer (`MUI_ICON/MUI_UNICON` reference
  `ui/src-tauri/icons/icon.ico`, `.nsi:173-174`).
- `bundle.windows.nsis`: `installerIcon icons/icon.ico`, `installMode
  "both"`, `languages [English, SimpChinese]`, `displayLanguageSelector
  true` (`:43-48`). Note: `installMode "both"` is dead config surface — the
  outer drives `/allusers` exclusively (`.nsi:221`); the outer UI is
  English-only while the inner carries EN+zh-CN (deviation #3,
  `installer/README.md:90`).
- `webviewInstallMode: downloadBootstrapper` (`:49-51`) — if the WebView2
  runtime is missing, the inner setup downloads Microsoft's bootstrapper at
  install time (network dependency; no offline installer is bundled).
- **No `resources` key** — nothing besides the app exe (and the NSIS
  uninstaller the template generates) ships from the ui lane. The engine kit
  does NOT ride in the inner bundle; the outer installer lays it beside
  (§3). This is exactly why `userdata-defaults.json` has no placement path
  (§2, §8 G-1).

The privilege model (`ui/src-tauri/app.manifest`):
`requestedExecutionLevel requireAdministrator uiAccess=false` (`app.manifest:25`)
+ the Common-Controls v6 dependency restated verbatim (a custom manifest
replaces tauri-build's default, `app.manifest:16-19,29-40`) + supportedOS
Win10/11 + 8.1 (`:41-48`). Embedded as RT_MANIFEST via
`tauri_build::WindowsAttributes::app_manifest` (`ui/src-tauri/build.rs:45-47`)
and **verified in the shipped binary** by build.ps1's D1 gate byte-scan
(`build.ps1:169-190`). History: through v0.8.x the docs claimed this while the
exe shipped asInvoker (the D-PRIV-01 hallucination; truth note `.nsi:37-40`,
erratum `installer/README.md:55-61`).

How the bundle interacts with the `.nsi`: the **wrapper architecture** —
`build.ps1` consumes the stock Tauri NSIS bundle as a silent payload and the
outer script adds the product concerns; the plan's §7.2 "custom Tauri NSIS
template" was rejected to avoid version-locking Tauri's template
(`installer/README.md:17-27`). Interaction points: staged into `$PLUGINSDIR`
(`.nsi:208`), seed-key steering (`:219`), silent `/S /allusers /D=` drive
(`:221`), integrity check + adoption (`:233-242`), uninstall-key takeover
(`:298-310`), silent inner uninstall with the `_?=` choreography (`:483-488`).

The portable zip is the same product without the installer:
`recovery-wizard-ui.exe` + kit beside it + `recovery-wizard.portable` marker
(NG-18 — the bridge's portable-mode probe, `ui/src-tauri/src/bridge/util.rs:285-307`;
sessions/logs resolve beside the exe, `state.rs:595-620`, `logging.rs:131-154`)
+ `README-PORTABLE.txt` (`installer/portable.ps1:41-77`). No registry, no
ProgramData, no shortcuts by the packaging itself.

---

## 5. The as-defined installed footprint

Everything placed after a full (all-users, default-dir) install — derived
from `.nsi` placement rules (§3) + the inner Tauri bundle's contract +
`docs/INSTALLER.md:41-51` + `contracts/README.md:47-48`:

```
C:\Program Files\HeicFast\Recovery Wizard\          ($INSTDIR — .nsi:146; flat, no subdirs)
├── recovery-wizard-ui.exe          inner Tauri setup, silent (/S /allusers /D=)      .nsi:221
│                                   (requireAdministrator manifest; the only CRT carrier)
├── uninstall.exe                   inner Tauri setup (its own NSIS uninstaller)
├── HeicFastUninstall.exe           WriteUninstaller                                .nsi:312
├── cr_ui.dll                       engine kit File (/x cr_ui.lib)                  .nsi:255
├── rrecov.exe                      engine kit (CLI; not invoked by the UI)
├── <vcpkg runtime DLLs>             engine kit — z.dll|zlib1.dll, lz4.dll|liblz4.dll,
│                                   zstd.dll|libzstd.dll (class; names vary)          build.ps1:116-120
( + Start Menu / optional Desktop shortcuts from the INNER Tauri template —
  set UNKNOWN from the repo; not asserted by any committed evidence)

C:\ProgramData\HeicFast\Recovery Wizard\            ($COMMONPROGRAMDATA root — .nsi:140)
├── sessions\                                        .nsi:280 (empty at install;
│   └── *.crsf …                                    user scan sessions, runtime-written)
└── logs\
    ├── install.log                                 InstallFact macro               .nsi:113-117
    └── app.log / app.log.old                       bridge logger, 1 MiB rotation
                                                    ui/src-tauri/src/bridge/logging.rs:3-9,234-235

HKLM\SOFTWARE\HeicFast\Recovery Wizard               .nsi:285-287
    InstallDir = "<dir>", Version = "0.9.3", InstallMode = "AllUsers"
HKLM\SOFTWARE\heicfast\Recovery Wizard              .nsi:219 (seed; default value = $INSTDIR)
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Recovery Wizard   .nsi:303-310
    DisplayName, DisplayVersion, Publisher, InstallLocation,
    UninstallString, QuietUninstallString, NoModify=1, NoRepair=1

%TEMP%\HeicFastRecoveryWizard-install-facts.txt     .nsi:118-121 (lifecycle log, both sinks)
%TEMP%\HeicFastRecoveryWizard-install-abort.txt     .nsi:91 (abort path only)
```

That is the complete list (the `docs/INSTALLER.md:49-51` claim): nothing in
`System32`, no service, no driver file, no certificate store, no per-user
tree. As-observed (the only committed runtime observation — driver-era
v0.6.0-rc1): 8 files in the flat root — exe 5,188,608 B; `uninstall.exe`
87,737 B; `cr_ui.dll` 1,192,912 B; `rrecov.exe` 1,230,288 B; `z.dll` 91,136 B;
`lz4.dll` 126,464 B; `zstd.dll` 657,920 B; `HeicFastUninstall.exe` 61,973 B
(`t15-runtime-matrix/03-after-install.json` inventory, with SHA-256 each) —
the module SET is unchanged by the driver removal
(`docs/INSTALL-FOOTPRINT-CURRENT.md:109-114` corrected totals). The runtime
footprint also includes whatever the app writes into `sessions\` and
`logs\` (above) — nothing else.

---

## 6. Install / uninstall / update / repair semantics + the captured evidence

Semantics as implemented (each documented in `installer/README.md:29-82` and
`docs/INSTALLER.md:53-92`):

- **Install**: §3's section order; failure path = `AbortWithReason`
  (file + `Abort`, no partial product claimed — but see §8 G-9 for the
  partial-install edge the success-path evidence never exercises).
- **Uninstall**: §3's uninstall order — ask-close (silent refuses if app
  running) → sessions kept by default (silent ALWAYS keeps; interactive
  default KEEP) → inner uninstaller silent → kit DLL catch-all → registry
  (unkey + swkey + seed + empty parent) → logs + final RMDirs.
- **Update**: rerun the installer in place — files overwritten, ProgramData
  sessions survive, version fields restamped from `VERSION`
  (`installer/README.md:70`; `docs/INSTALLER.md:81-86`). The matrix's
  honest note: the validated row is reinstall-over-**same**-version; the
  version-bump update mechanics get exercised at the RC→final sequence
  (`tools/validation/installer-matrix.ps1:30-32`).
- **Repair**: `NoRepair=1` (`:310`) — the honest story is rerun-the-installer
  (reinstall-over); `installer/README.md:71`; `docs/INSTALLER.md:88-92`.

**The captured evidence (`docs/INTEGRATION-VALIDATION/t15-runtime-matrix/`) —
all of it driver-era v0.6.0-rc1, run 2026-10-27 (validation-evidence runs
40437334177 / re-run 40442056802, `docs/INTEGRATION-VALIDATION.md:29`):**

| File | What it shows |
|---|---|
| `matrix.json` | 5 rows, all ok: baseline `clean` (`driverServicePresent:false`); install `ok` (exit 0, `files:15`, `driverService:"Running"`, `driverSigned:true`, `certInRoot/certInTP:true`, `testDlls:0`, `installMode:"AllUsers"`, `regVersion:"0.6.0-rc1"`, 9384 ms); update(reinstall) `ok` (`markerSurvived:true`); repair-over-corruption `ok` (`restored:true`); uninstall(silent) `ok` (`leftoverFiles:0`, `swKeyGone`, `unKeyGone`, `driverGone`, `sysDriverGone`, `certGoneRoot/TP`, `sessionsKept:true`). |
| `01-baseline.json` | pre-install truth: `installDirExists:false`, `swKey/uninstallKey/driverService:null`, `sessionsDirExists:false`. |
| `02-install-facts.txt` | the .nsi's own 12-line lifecycle log for one install: `onInit … mode=AllUsers`, `app-setup exit=0 …`, `cert Root/TrustedPublisher addstore exit=0`, `cert thumbprint=6462…`, `driver install.ps1 exit=0`, `driver ImagePath readback=\??\C:\Windows\System32\drivers\RDA.sys`, `driver sc start exit=1056`, `post InstallDir/Version/InstallMode readback`s, `post DriverInstalled readback=1`. **Every driver/cert line is the retired era's choreography — the current .nsi writes none of them.** |
| `02-install-transcript.txt` | `setup.exe /S /allusers`, exit 0, 9384 ms. |
| `03-after-install.json` | the exact 15-file inventory with SHA-256 (8 root + 7 `driver\`), the SW key incl. `DriverInstalled`/`DriverCertThumbprint`, the full Uninstall key value set, `driverService:4`, `testDllsShipped:null`, `certInRoot/certInTP:true` — the driver-era layout snapshot. |
| `04-reinstall-facts.txt` / `04-reinstall-transcript.txt` | two appended install cycles in the facts file (append-only contract); reinstall exit 0, 4028 ms. |
| `05-after-reinstall.json` | `markerSurvived:true` (a session marker written into the REAL sessions dir survived reinstall-over-install), `regVersion` unchanged, the Uninstall key re-written intact. |
| `06-repair-facts.txt` / `06-repair-transcript.txt` | three install cycles appended; repair run exit 0, 3902 ms. |
| `07-after-repair.json` | the repair proof: `hashOriginal == hashRepaired` (byte-exact restore of a `cr_ui.dll` corrupted with `CORRUPTED-BY-T15`), `hashCorrupted` differs, `restored:true`. |
| `08-uninstall-facts.txt` | the whole lifecycle (3 installs) + `un.onInit` + `uninstall driver uninstall.ps1 exit=0` + cert delstores + `uninstall registry deleted unkey+swkey`. |
| `08-after-uninstall.json` | exit 0, 2451 ms; `installDirExists:false`, `leftovers:0`, `swKeyExists/uninstallKeyExists:false`, `driverService:"gone"`, `sysDriverFile:false`, `markerSurvived:true`, `sessionsKept:true`, certs gone from both stores. |
| `t15-runtime-matrix.md` | the 5-row table rendering + the generator footer. |

**The critical dating fact.** Every file above was captured against the
v0.6.0-rc1 driver-era installer — BEFORE the consolidation session removed the
driver (commit 318b9655, 2026-11-05). The current 549-line `.nsi` and the
rewritten `tools/validation/installer-matrix.ps1` (driver-free posture
asserted at `:195-239` — a `driver\` dir, an RDA service, a
`System32\drivers\RDA.sys`, or any HeicFast machine-store certificate FAILS
the install; exact module-set assertion at `:241-267`) have **no committed
evidence of a run**. `installer/README.md:94-104` says it outright: the
compile check and the full T15 lane are to be "re-proven for the driver-free
rewrite before the next tagged release." Same for the compile evidence:
`docs/INTEGRATION-VALIDATION/installer-compile-check.md` documents the
0.5.0-era script (it still lists the `DRIVER_STAGE` define, MultiUser flow,
and a 517-line script — `:8-13,27-41`), not the current one. The T14
measurement evidence (`t14-measurements/`, run 40442056802 at `9e188154`) is
also the mandatory-driver tree (`t14-measurements.md:4-10`). The v0.9.3
runtime story therefore rests on: the code as read (§3), the D1/D-PRIV-01
posture gates codified in `forensic-repro.yml` (dispatchable, default tag
v0.9.3 — `.github/workflows/forensic-repro.yml:47-50,113-122,174`), and
Actions history — not on any committed artifact in this repo.

---

## 7. No-console / no-visible-dialog posture

- **Installer elevation**: `RequestExecutionLevel admin` (`recoverywizard.nsi:162`)
  — one standard UAC at launch; no script windows, no secondary elevation
  prompts (`docs/INSTALLER.md:17-20`).
- **Zero helper processes**: the entire lifecycle runs `powershell.exe` zero
  times — file placement, registry, directory prep are native NSIS operations;
  the ONLY child process anywhere is the inner Tauri setup, which runs
  silently inside the progress page (`docs/INSTALLER.md:31-37`, the
  consolidation-era rewrite; the driver-era `ExecWait` console spawns were the
  NG-16 defect, now structurally gone with the driver).
- **`/S` silent mode**: documented (`docs/INSTALLER.md:38-39`); every
  silent-path `MessageBox` carries `/SD` with the safe default — the Win10
  gate `IDOK` before its `Abort` (`.nsi:408-413`), uninstall ask-close
  `IDCANCEL` = refuse (`.nsi:443-455`), sessions prompt `IDNO` under the
  `${Silent}` guard (`.nsi:458-472`); rationale + round-11 evidence at
  `installer/README.md:73-82`. `AbortWithReason` is the silent-safe loud
  failure (`.nsi:83-96`). The inner setup is driven with `/S /allusers /D=`
  via `ExecWait` (chosen over nsExec for verbatim command-line handoff —
  `.nsi:197-203`).
- **First launch**: the app exe embeds `requireAdministrator`
  (`ui/src-tauri/app.manifest:25`) via tauri-build (`build.rs:45-47`), byte-
  scanned by the D1 gate (`build.ps1:169-190`) — the OS consent fires before
  any UI exists; an unelevated start is refused BY THE OS (win32 740). The
  refusal is asserted by `tools/forensic/unelevated-launch.ps1` (rewritten
  post-0.9.0: direct unelevated `CreateProcessWithLogonW` of the exe MUST
  throw the elevation refusal and no process may appear —
  `unelevated-launch.ps1:1-22,83-118`) and gated in
  `.github/workflows/forensic-repro.yml:174` (the manifest gate at `:113-122`).
  The committed `worklog/REPRODUCTION/D-PRIV-01/` capture is the **defect-era**
  v0.8.4 evidence (`facts-postinstall-appmanifest.json`:
  `"no-manifest-match"`); no committed artifact records the fixed-posture
  green run (Actions history only, per `docs/INTEGRATION-VALIDATION.md:4-6`).
- **App's own console**: `#![cfg_attr(not(debug_assertions),
  windows_subsystem = "windows")]` (`ui/src-tauri/src/main.rs:2`) — the
  release build is GUI-subsystem (no console); debug builds show one by
  design.
- **Engine subprocess spawns** (inside the app, all hidden): exactly three
  `CreateProcessW` sites, all `CREATE_NO_WINDOW`, pinned by a build-time
  source-scan gate (`engine/tests/unit/test_no_console_spawn.cpp:53-111`):
  - `engine/src/win/cr_health.cpp:511` — `fsutil behavior query
    disabledeletenotify` DIRECT (no cmd.exe), stdout+stderr piped,
    `STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW`, `wShowWindow=SW_HIDE`,
    bounded 10 s wait with `TerminateProcess` (the D2/D-CONS-01 fix; header
    rationale `cr_health.cpp:483-493`); reachable only via the
    `CR_ENUM_FLAG_TRIM` enumeration leg the bridge never sets — the health
    reads use the per-device IOCTL path.
  - `engine/src/win/cr_fsck.cpp:51` — chkdsk (repair check-phase), native
    System32 via WoW64-redirection disable.
  - `engine/src/win/cr_bitlocker.cpp:65` — `powershell -NoProfile
    -NonInteractive` Unlock/Disable-BitLocker + `manage-bde` fallbacks
    (user-initiated unlock/turn-off only).
  - Census: no `_popen`/`system`/`WinExec`/`ShellExecute` in the shipping
    engine sources (the gate's allowlist, `test_no_console_spawn.cpp:33`).
- **No hidden-child-process sites in the Rust bridge**: zero
  `std::process::Command` in `ui/src-tauri/src/**` (grep clean — the bridge
  talks to the engine in-process through the static import).
- **Residual console risks** (scoped): the WebView2 downloadBootstrapper leg
  (`tauri.conf.json:49-51`) on a machine missing the runtime — its window
  posture is UNKNOWN and uncovered by committed evidence (T15/forensic
  runners all had WebView2 preinstalled); debug builds of the app show a
  console by design (`main.rs:2`).

---

## 8. GAPS — every limitation observed, each scoped

Placement/product-completeness gaps (the integration matrix's raw material):

- **G-1 (missing placement — real capability loss).**
  `engine/data/userdata-defaults.json` (the 111-entry default user-signature
  set, M-065) is probed exe-adjacent by BOTH consumers at startup — the CLI
  (`engine/src/app/rrecov.cpp:1354`) and the bridge
  (`ui/src-tauri/src/commands/system.rs:85-112`, invoked `lib.rs:133`) — and
  the file exists in the repo, but NO lane places it: the engine kit staging
  copies only dll/lib/exe/runtime-DLLs (`build.ps1:112-121`), `tauri.conf.json`
  declares no `resources`, and the `.nsi` copies only the staged kit. Every
  clean install therefore starts with `userSigns: 0` (the About row reads
  zero) — the og's EUUserData install-tree posture silently absent. The
  engine's own release convention ships it beside `rrecov.exe`
  (`engine/docs/PREBUILT-ARTIFACTS.md:51`). This is exactly the "files the
  product needs but nothing places" class the session's Phase-1 mandate
  names.
- **G-2 (unreferenced placement — inverse class).** `cr_ui.lib` ships in the
  PORTABLE zip but is excluded from the install (`portable.ps1:47` vs
  `.nsi:255`) — a documented harmless inconsistency
  (`docs/INSTALL-FOOTPRINT-CURRENT.md` §B row 3). Also in this class:
  `rrecov.exe` is placed by the installer but invoked by nothing in the UI
  (CLI-only surface; the Lost-Partition page's `rrecov apply` note is the
  one honest reference — `docs/INSTALL-FOOTPRINT-DELTA.md` §C row 2). No
  orphan-on-uninstall risk (the uninstaller deletes it, `.nsi:496`).
- **G-3 (shortcuts unaudited).** The outer `.nsi` has no `CreateShortCut`;
  the inner Tauri template's shortcut set (Start Menu; desktop unknown) is
  neither documented in the repo nor asserted by any validation lane
  (`docs/INSTALL-FOOTPRINT-CURRENT.md` §A.3 shortcut row: "not asserted by
  T15 (its inventory covers `$INSTDIR` only)"). Shortcut creation AND removal
  ride entirely on the inner uninstaller's manifest — UNKNOWN, no committed
  evidence.
- **G-4 (orphan risk on the abort path).** If the inner setup SUCCEEDS but a
  later outer gate aborts (engine-kit gates, `.nsi:257-273`), the machine
  keeps the inner app + the inner's own uninstall entry, with no engine kit,
  no product key, no Add/Remove takeover (those land in `-post`, which never
  runs). The `-post`-only `RemoveInnerUninstallKeys` sweep and the seed key
  are also never cleaned on abort. No committed failure-path evidence
  covers this (the matrix runs success + one repair row only). Related
  history: NG-19's residue class (`docs/INTEGRATION-GAPS.md:198-201`).
- **G-5 (evidence staleness — the big one).** All committed runtime evidence
  (t15-runtime-matrix, installer-compile-check, t14-measurements,
  FORENSIC-PROBE curated captures, D-PRIV-01 reproduction) predates the
  driver-free rewrite (v0.6.0-rc1 / v0.8.4 era, 2026-10/11-06); the current
  .nsi, the rewritten matrix tool, and the forensic gates have no committed
  run artifacts. `installer/README.md:96-104` queues the re-proof "before the
  next tagged release." The driver-free install/uninstall/update/repair
  story of 0.9.3 is asserted-by-code, unproven-by-committed-evidence.
- **G-6 (seed-key cleanup unasserted).** The uninstaller deletes
  `Software\heicfast\Recovery Wizard` + the empty parent (`.nsi:525-530`),
  but the current matrix asserts only `swKeyExists`/`uninstallKeyExists`
  (`installer-matrix.ps1:334-364`) — the seed-key and empty-parent removals
  have no probe (the driver-era `08-after-uninstall.json` did not probe them
  either).
- **G-7 (WebView2 posture).** `downloadBootstrapper`
  (`tauri.conf.json:49-51`): network dependency at install when the runtime
  is absent; no offline installer is bundled; the portable README requires
  WebView2 present (`portable.ps1:66`). The bootstrapper leg's console/UX
  posture is UNKNOWN and uncovered by evidence.
- **G-8 (signing gap).** Only the outer `setup.exe` + the engine kit modules
  carry the development signature; the Tauri app bundle (inner app-setup +
  `recovery-wizard-ui.exe`) is NOT signed — the documented production-signing
  gap (`docs/DEV-SIGNING.md:19`); production signing (real CA) remains
  pending for everything (`installer/README.md:105-106`).
- **G-9 (update semantics).** The validated update row is
  reinstall-over-same-version only (`installer-matrix.ps1:30-32`); a real
  version-bump update (0.9.3 → next) has never been evidenced — the
  registry `Version` restamp, the inner-bundle overwrite-over-old-version,
  and stale-module cleanup across versions are all unexercised. (Known
  hazard class: I-STALE-01 guarded the build side only, `build.ps1:211-224`.)

Smaller, scoped residuals: outer installer UI English-only vs inner
EN+zh-CN (deviation #3, `installer/README.md:90`); ProgramData default ACLs,
no per-user model (deviation #2, `:89`); `installMode: "both"` dead config
(`tauri.conf.json:45`); the `%TEMP%` facts/abort files are deliberately kept
after uninstall (evidence contract, `docs/INSTALLER.md:110-113`); debug-build
console by design (`main.rs:2`); `ui/src-tauri/nsis-header.bmp` is an orphan
asset referenced by nothing (grep clean repo-wide); the 13-parameter Inno
contract of the engine lane's own packaging milestone
(`engine/docs/PACKAGING.md` §1, §4 — `/SILENT`//`VERYSILENT`/
`/SUPPRESSMSGBOXES`/`/NORESTART`… flag parity) does NOT bind the product
installer, which exposes only NSIS `/S` — a deliberate divergence recorded
in the wrapper decision, but a flag any enterprise-deployment consumer of
the reference's flag set will not find.

---

*End of sheet — task 2-a. The integration matrix (Agent E) should start from
§5 (as-defined footprint), §8 G-1/G-3/G-4/G-5/G-6/G-9 (the placement and
evidence holes), and the reference census in `worklog/REFERENCE-FOOTPRINT.md`
/ `worklog/FOOTPRINT-DELTA.md` (the per-module reconciliation already
recorded: our 8.2 MiB / 8-file install vs the reference's 494.7 MiB live
tree, with exactly three user-perceivable functional gaps — rendered
previews, the file-repair workbench, WinPE — all recorded, none installer
placement bugs).*
