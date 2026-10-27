# INSTALL-FOOTPRINT-CURRENT.md — what the current product places on a Windows machine, and how it is built

> Task T4-B (Agent B, build & packaging). READ-ONLY analysis of
> `/home/z/my-project/final-prod` @ `af5c838ab71d37a129f81e327e632dff6a717e4e` (branch
> `main`, working tree clean; shipped release **v0.6.0-rc1**, development test-signed).
> Every claim cites `path:line`. Sources read exhaustively: `installer/recoverywizard.nsi`
> (736 lines), `driver/scripts/{install,uninstall}.ps1`, `installer/{devsign,portable}.ps1`,
> `build.ps1`, `.github/workflows/{release,validation-evidence,driver-ci}.yml`,
> `ui/src-tauri/{tauri.conf.json,Cargo.toml,build.rs,src/**}`, `tools/validation/installer-matrix.ps1`,
> `docs/INTEGRATION-VALIDATION/t15-runtime-matrix/*`, and the prior context sheets.
> No file was modified by this task except this sheet + the shared worklog.

---

## A. The complete installer footprint (`RecoveryWizard-0.6.0-rc1-setup.exe`)

The outer NSIS installer (32-bit PE, Unicode, LZMA-SOLID — `installer/recoverywizard.nsi:47-48`)
is `RequestExecutionLevel admin` (`recoverywizard.nsi:154`), all-users only
(`SetShellVarContext all`, `recoverywizard.nsi:556`; `SetRegView 64`, `:538`). Default
install dir `C:\Program Files\HeicFast\Recovery Wizard` (`$PROGRAMFILES64\HeicFast\Recovery Wizard`,
`recoverywizard.nsi:139`). Observed layout + sizes are the T15 runtime-matrix evidence
(`docs/INTEGRATION-VALIDATION/t15-runtime-matrix/03-after-install.json`), run 2026-10-27 on a
clean windows-2022 runner (test signing ON).

### A.1 Files in `$INSTDIR` (root — 8 files, T15-observed)

| # | File (on machine) | Source in build | Size (v0.6.0-rc1) | Placed by | What it is |
|---|---|---|---|---|---|
| 1 | `$INSTDIR\recovery-wizard-ui.exe` | `ui/src-tauri/target/release/recovery-wizard-ui.exe` via the inner Tauri NSIS bundle | 5,188,608 | inner Tauri setup, driven silent (`recoverywizard.nsi:216`) | Tauri app shell; statically imports `cr_ui.dll` (`ui/src-tauri/build.rs:35`); binary name = Cargo package name (`ui/src-tauri/Cargo.toml:2`, `recoverywizard.nsi:131`) |
| 2 | `$INSTDIR\uninstall.exe` | inner Tauri bundle | 87,737 | inner Tauri setup | the INNER uninstaller (Tauri/NSIS; removes only the app's own files/registry/shortcuts) |
| 3 | `$INSTDIR\cr_ui.dll` | staged engine kit `build\out\stage-<v>\engine\cr_ui.dll` (`build.ps1:113`) | 1,192,912 | `File /x cr_ui.lib "${ENGINE_KIT}${PATHSEP}*"` (`recoverywizard.nsi:250`) | the C++ recovery engine host-binding ABI (78 exports, ABI v2) — dev test-signed (`build.ps1:130`) |
| 4 | `$INSTDIR\rrecov.exe` | kit (`build.ps1:114`) | 1,230,288 | same `File` | engine CLI (diagnostic/headless tool; not invoked by the UI) — dev test-signed |
| 5–7 | `$INSTDIR\z.dll`, `$INSTDIR\lz4.dll`, `$INSTDIR\zstd.dll` | kit: vcpkg runtime DLLs deployed beside `cr_ui.dll` by the engine's POST_BUILD `cr_deploy_vcpkg_binaries` (`engine/CMakeLists.txt:156-167`, `engine/cmake/DeployVcpkgBinaries.cmake:1-18`), curated by `build.ps1:115-119` (gtest/gmock excluded) | 91,136 / 126,464 / 657,920 | same `File` | decompressor runtime DLLs (zlib/lz4/zstd) — **names vary by vcpkg port/config** (`build.ps1:100-103`); the .nsi only asserts "≥1 DLL beside cr_ui.dll" (`recoverywizard.nsi:256-268`) |
| 8 | `$INSTDIR\HeicFastUninstall.exe` | generated | 61,973 | `WriteUninstaller` (`recoverywizard.nsi:432`) | the OUTER product uninstaller (complete scope) |

Note: `cr_ui.lib` is staged into the kit (`build.ps1:113`) but **excluded** from the install
(`File /x cr_ui.lib`, `recoverywizard.nsi:250`) — it is a link-time artifact. No
`WebView2Loader.dll`, no VCRT/UCRT DLLs ship (T15 inventory is the complete set; the app exe
is the only CRT carrier, statically linked).

### A.2 Files in `$INSTDIR\driver\` (7 files — `recoverywizard.nsi:283-290`)

| # | File | Source | Size | What it is |
|---|---|---|---|---|
| 9 | `driver\rda.sys` | `driver\bin\x64\Release\rda.sys`, embed-signed (`build.ps1:161-165`) | 50,640 | the RDA raw-disk kernel driver (WDM, x64) — development TEST-SIGNED with the ephemeral build certificate |
| 10 | `driver\rdacli.exe` | same driver lane (`build.ps1:169`) | 48,640 | driver client/contract harness CLI |
| 11 | `driver\rda.inf` | `driver\src\driver\rda.inf` verbatim (`build.ps1:170`) | 2,038 | INF profile of the driver — **staged but UNUSED by the installer** (registration goes through `install.ps1`; `recoverywizard.nsi:338-349`); kept as the manual/PnP documentation surface. Declares `CatalogFile = rda.cat` (`driver/src/driver/rda.inf:21`) which is never built |
| 12 | `driver\install.ps1` | `driver\scripts\install.ps1` (`build.ps1:171`) | 6,945 | SCM service installer (see D.3) |
| 13 | `driver\uninstall.ps1` | `driver\scripts\uninstall.ps1` (`build.ps1:171`) | 2,743 | SCM service remover |
| 14 | `driver\rda-testcert.cer` | stage cert (`build.ps1:175`) | 834 | PUBLIC half of the ephemeral dev signing cert (also published as a release asset) |
| 15 | `driver\rda-testcert-thumbprint.txt` | stage (`build.ps1:175`) | 40 | SHA-1 thumbprint; read at install into `DriverCertThumbprint` (`recoverywizard.nsi:314-321`) |

The driver staging dir is the "signed-drop-in surface": replace `driver\rda.sys` + reinstall
without touching the app (`recoverywizard.nsi:281-282`).

### A.3 Machine-level effects outside `$INSTDIR`

| Effect | Exact target | Mechanism (cite) | Evidence |
|---|---|---|---|
| Driver file | `C:\Windows\System32\drivers\RDA.sys` (renamed from `rda.sys` to the service name) | `driver\install.ps1:58-59, 83` — copy with WOW64 FS-redirection disabled (`install.ps1:72-88`) + redirection-free verification (`:89-100`) | T15 facts: `driver ImagePath readback=\??\C:\Windows\System32\drivers\RDA.sys` (`02-install-facts.txt:7`) |
| SCM service `RDA` | `HKLM\SYSTEM\CurrentControlSet\Services\RDA`: `type= kernel start= demand error= normal group= "Boot File System"`, `binPath= C:\Windows\System32\drivers\RDA.sys` | `sc.exe create` (`install.ps1:103`), spawned by the .nsi via `powershell.exe -File install.ps1` (`recoverywizard.nsi:349`) | service `Running` (`03-after-install.json:24`) |
| Service Parameters | `...\Services\RDA\Parameters`: `IoctlBase=0`, `MaxDisks=200`, `DebugLevel=1`, `SessionsEnabled=1` (DWORDs) | `install.ps1:106-111` | `driver/scm` semantics per `docs/context-sheets/DRIVER-DEPLOYMENT.md` §4.3 |
| Kernel device | `\Device\RDA` + `\DosDevices\RDA` → user path `\\.\RDA` | derived from service name at DriverEntry (DRIVER-DEPLOYMENT §2.1); device name = UPPER(service name) (`recoverywizard.nsi:128-130`) | `driverService: 4 (Running)` (`03-after-install.json:24`) |
| Certificate stores | `LocalMachine\Root` + `LocalMachine\TrustedPublisher` ← `rda-testcert.cer` | `certutil -addstore -f Root` (`recoverywizard.nsi:301`) and `-f TrustedPublisher` (`:307`), BEFORE the service create | `certInRoot/certInTP: true` (`matrix.json:19-21`) |
| ProgramData dirs | `C:\ProgramData\HeicFast\Recovery Wizard\sessions\` and `...\logs\` (empty, default inherited ACLs) | `CreateDirectory` ×2 (`recoverywizard.nsi:391-392`); ACL deviation recorded (`installer/README.md:110`) | created; `sessionsKept: true` after uninstall (`matrix.json:60`) |
| Product SW registry key | `HKLM\Software\HeicFast\Recovery Wizard`: `DriverInstalled`=1 (DWORD), `InstallDir`, `Version`="0.6.0-rc1", `InstallMode`="AllUsers", `DriverCertThumbprint` | `recoverywizard.nsi:399-405` (read-backs `:409-416`) | full snapshot `03-after-install.json:6-13` |
| Uninstall key | `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\Recovery Wizard`: DisplayName/DisplayVersion/Publisher/InstallLocation/UninstallString/QuietUninstallString/NoModify=1/NoRepair=1 | `recoverywizard.nsi:423-430`; any inner-Tauri uninstall key pointing into `$INSTDIR` is deleted first (`:422`, `:483-531`) | snapshot `03-after-install.json:14-23` |
| Inner-setup seed key | `HKLM\Software\heicfast\Recovery Wizard` default value = `$INSTDIR` | `WriteRegStr` (`recoverywizard.nsi:214`) — steers the Tauri bundle's `RestorePreviousInstallLocation`; key is owned by the inner uninstaller AND cleaned by ours (`:724-729`) | default value visible in `03-after-install.json:7` |
| Shortcuts (Start Menu) | all-users Start Menu (Tauri template behavior) | created by the INNER Tauri setup (the outer .nsi contains **no** `CreateShortCut` — verified across all 736 lines); removed by the inner uninstaller | not asserted by T15 (its inventory covers `$INSTDIR` only) |
| `%TEMP%` lifecycle log | `%TEMP%\HeicFastRecoveryWizard-install-facts.txt` — ALWAYS written, appended at every phase (install/update/repair/uninstall) | `InstallFact` macro (`recoverywizard.nsi:107-112`), started in `.onInit` (`:560`) | `02-install-facts.txt` (12 lines for one install) |
| `%TEMP%` abort reason | `%TEMP%\HeicFastRecoveryWizard-install-abort.txt` — only on Abort | `AbortWithReason` macro (`recoverywizard.nsi:88-95`) | consumed by the T15 failure path (`tools/validation/installer-matrix.ps1:152-153`) |

**Footprint total (silent install, observed): 15 files under `$INSTDIR` (8 root + 7 in
`driver\`) + 1 file in `System32\drivers` + 2 empty ProgramData dirs + 4 registry keys
(SW key, uninstall key, inner seed key, service key tree incl. Parameters) + the cert in 2
stores + 2 `%TEMP%` log files.** Install wall-time 9,384 ms silent (`02-install-transcript.txt:4`).

---

## B. The complete portable.zip contents (`RecoveryWizard-0.6.0-rc1-portable.zip`)

Assembled by `installer/portable.ps1` (invoked at `build.ps1:266-275`). The zip flattens the
portable root (`portable.ps1:46-53`, compressed `:89-92`); the T15 matrix does **not**
inventory the zip (scope note), so contents are derived from the script + the kit contract:

| # | File in zip | Source | Size (v0.6.0-rc1) | Notes |
|---|---|---|---|---|
| 1 | `recovery-wizard-ui.exe` | `ui/src-tauri/target/release/recovery-wizard-ui.exe` (`portable.ps1:33, 50`) | 5,188,608 | same binary as installed (T15 hash `4A16…84A2`) |
| 2 | `cr_ui.dll` | engine kit (`portable.ps1:52`) | 1,192,912 | beside the exe — loader truth |
| 3 | `cr_ui.lib` | engine kit | varies (build output) | **ships in the zip but NOT in the install** (`recoverywizard.nsi:250` excludes it) — footprint inconsistency, harmless |
| 4 | `rrecov.exe` | engine kit | 1,230,288 | |
| 5–7 | `z.dll`, `lz4.dll`, `zstd.dll` | engine kit (deployed vcpkg set; names vary) | 91,136 / 126,464 / 657,920 | |
| 8–14 | `driver\{rda.sys, rdacli.exe, rda.inf, install.ps1, uninstall.ps1, rda-testcert.cer, rda-testcert-thumbprint.txt}` | driver stage (`portable.ps1:53`) | 50,640 / 48,640 / 2,038 / 6,945 / 2,743 / 834 / 40 | **STAGED, NOT INSTALLED** — the README documents the manual elevated install (cert import ×2 + `install.ps1`, `portable.ps1:66-69`) |
| 15 | `README-PORTABLE.txt` | generated (`portable.ps1:55-87`) | varies | the truthful label: no driver installed (product requires it), run as admin, WebView2 must be present, sessions/logs "beside this executable" (**see E — this claim is not what the code does**) |

**Portable total: 15 files** (1 exe + 7 kit files incl. `cr_ui.lib` + 7 driver + 1 README).
No uninstaller, no registry writes, no ProgramData writes **by the packaging itself**.
The zip's SHA-256 is printed at build (`portable.ps1:94-96`) and folded into `SHA256SUMS`.

---

## C. The build chain (single command → lanes → outputs)

**Single entry point:** `.\build.ps1 -Target all` (default `all`, `build.ps1:20, 288-304`;
documented in `docs/BUILD.md:8-13`). Version read from the root `VERSION` file
(`build.ps1:29`); stage dir `build\out\stage-<version>` (`build.ps1:30-32`).

```
.\build.ps1 -Target all                      (build.ps1:295-303)
├── Assert-VersionConsistency                (build.ps1:36-45)  ui/package.json + Cargo.toml + tauri.conf.json must equal VERSION
├── Invoke-Contracts   node contracts/validate.mjs   (build.ps1:47-53)
├── Invoke-Driver                             (build.ps1:134-181)
│     nuget restore (WDK 10.0.26100.6584) → msbuild rda.sln x64      (:137-140)
│     stamp_osversion.py + verify_pe.py (import allowlist)           (:144-150)
│     devsign new-cert (ephemeral, idempotent) + sign rda.sys        (:161-165; installer/devsign.ps1:118-176)
│     stage → stage-<v>\driver\{rda.sys,rdacli.exe,rda.inf,install.ps1,uninstall.ps1,rda-testcert.cer,rda-testcert-thumbprint.txt}  (:167-175)
│     publish cert copy → build\out\RecoveryWizard-<v>-testsigning.cer (:179)
├── Invoke-Engine                             (build.ps1:55-132)
│     cmake --preset win-x64-<cfg> + build + ctest (Release only)    (:65-80)
│     PE Win7 gates on rrecov.exe + cr_ui.dll (stamp_osversion_um / verify_pe_um) (:83-96)
│     KIT CURATION → stage-<v>\engine\: cr_ui.dll + cr_ui.lib + rrecov.exe
│       + every deployed vcpkg runtime DLL EXCLUDING ^(gtest|gmock)  (:111-119; throws if zero runtime DLLs)
│     devsign new-cert (reuse) + sign staged cr_ui.dll + rrecov.exe  (:127-131)
├── Invoke-Ui                                 (build.ps1:183-221)
│     pnpm install/typecheck/lint/test       (:186-195)
│     stage kit into src-tauri/target/{release,debug} + CR_UI_LIB_DIR (:200-212)
│     pnpm tauri build → NSIS bundle (beforeBuildCommand runs pnpm build) (:214; bundle dir :218-220)
├── Invoke-Installer                          (build.ps1:235-264)
│     makensis /DPRODUCT_VERSION /DAPP_BUNDLE /DENGINE_KIT /DDRIVER_STAGE /DOUT_DIR /DPATHSEP=\  (:248-251)
│     → build\out\RecoveryWizard-<v>-setup.exe                       (:259)
│     devsign sign the setup.exe                                     (:261-262)
├── Invoke-Portable                           (build.ps1:266-275 → installer/portable.ps1)
│     → build\out\RecoveryWizard-<v>-portable.zip
└── Invoke-Checksums → build\out\SHA256SUMS   (build.ps1:277-285; all top-level files in build\out)
```

**Outputs into `build\out`** (asserted by `release.yml:75-97`):
`RecoveryWizard-0.6.0-rc1-setup.exe`, `RecoveryWizard-0.6.0-rc1-portable.zip`,
`RecoveryWizard-0.6.0-rc1-testsigning.cer`, `SHA256SUMS`, plus the stage dir
`stage-0.6.0-rc1\` holding `devsign.pfx`, `devsign-pfx-password.txt` (private key material,
gitignored build output — `.gitignore:2` `build/`; **but see G.10: the release workflow
uploads all of `build/out/` as a workflow artifact**).

**Version single-source-of-truth status: ENFORCED.** `VERSION` (currently `0.6.0-rc1`,
`VERSION:1`) gates three lockstep consumers at build time (`build.ps1:36-45`) and the
release tag at release time (tag must equal `v<VERSION>`, `release.yml:36-45`). Component
wire versions (driver FILEVERSION 1.0, engine ABI 2) are deliberately NOT restamped
(`build.ps1:13-17`, `docs/BUILD.md:20-23`). Stale doc value: `contracts/README.md:50` still
says `VERSION` = `0.5.0`.

**What the Tauri bundler puts beside the exe:** nothing — no `resources`, no `externalBin`,
no sidecars (`ui/src-tauri/tauri.conf.json:32-53`); `bundle.targets = ["nsis"]` only
(`:34`); WebView2 ships as `downloadBootstrapper` (`:49-51`), i.e. the INNER setup
downloads/installs the WebView2 runtime at install time when the machine lacks it (network
dependency, not exercised by T15 — the runner has WebView2). Install dir name and product
name come from the inner template: `productName` "Recovery Wizard" (`tauri.conf.json:3`),
identifier `com.heicfast.recoverywizard` (`:5` → manufacturer key `heicfast`,
`recoverywizard.nsi:205-214`), `installMode: "both"` (`:45`) — the outer drives `/allusers`.

---

## D. Installer UX flow, mechanisms, uninstall coverage, update/repair, logging

### D.1 Pages and elevation

1. **UAC prompt at launch** — `RequestExecutionLevel admin` (`recoverywizard.nsi:154`); ONE
   prompt for the whole install (deviation #1, `installer/README.md:109`). Win10+ gate with
   `/SD IDOK` MessageBox + Abort (`recoverywizard.nsi:544-552`).
2. Welcome → License (repo `LICENSE`, MIT) → Components → **custom Driver page** →
   Directory → InstFiles → Finish (`recoverywizard.nsi:172-178`).
3. Components: "HeicFast Recovery Wizard (required)" + "RDA raw-disk kernel driver
   (required)" — **both `SectionIn RO`** (`recoverywizard.nsi:186-187, 271-276`); no consent
   checkbox for the driver (mandatory-driver decision, `recoverywizard.nsi:27-33`).
4. Driver page (`recoverywizard.nsi:451-471`): 4 informational labels (raw-disk transport
   `\\.\RDA`, SYSTEM privileges + admin-only DACL, no-fallback truth, development
   test-signing + Secure-Boot instructions). No state (`:473-475`).
5. Finish: optional launch checkbox `MUI_FINISHPAGE_RUN` → `$INSTDIR\recovery-wizard-ui.exe`
   (`recoverywizard.nsi:169-170`). The app itself carries `requireAdministrator`
   (INTEGRATION-PLAN §7.4 decision (a), `docs/INTEGRATION-PLAN.md:926-931`).

### D.2 Install sequence (SEC_APP → SEC_DRIVER → "-post")

1. Inner Tauri setup staged to `$PLUGINSDIR\app-setup.exe` (`recoverywizard.nsi:203`), seed
   registry value written (`:214`), driven `ExecWait '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR'` (`:216`). Non-zero exit → abort with reason (`:218-221`). Integrity:
   `recovery-wizard-ui.exe`/`uninstall.exe` must exist, with adoption of the inner's own
   default dir as fallback (`:228-237`).
2. Engine kit `File /x cr_ui.lib` into `$INSTDIR` (`:249-250`); gates: `cr_ui.dll` present
   (`:252-254`), ≥2 DLLs total in `$INSTDIR` (`:259-268`).
3. Driver files staged to `$INSTDIR\driver` (7 `File`s, `:283-290`).
4. **Cert import** (before service create): `certutil -addstore -f Root` (`:301`),
   `certutil -addstore -f TrustedPublisher` (`:307`); failure aborts (`:303-312`);
   thumbprint read from staged file into `$DriverCertThumbprint` (`:314-321`).
5. **Update pre-step** (if `HKLM\…\Services\RDA\ImagePath` exists): `sc.exe stop` + sleep
   1500 ms + `sc.exe delete` + sleep 500 ms, both via `nsExec::ExecToLog` (`:327-336`).
6. **Driver registration**: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
   "$INSTDIR\driver\install.ps1" -ServiceName RDA -Source "$INSTDIR\driver\rda.sys"` via
   `ExecWait` (`:349`) — VERBATIM quoting (nsExec re-tokenizes; round-8 lesson, `:342-347`).
   `install.ps1` (elevated context) does: elevation guard (`driver/scripts/install.ps1:45-49`),
   existence preflight (`:52-55`), WOW64-safe copy to `System32\drivers\RDA.sys`
   (`:58-99`), `sc.exe create … type= kernel start= demand error= normal group= "Boot File
   System"` (`:103`), Parameters `{IoctlBase=0, MaxDisks=200, DebugLevel=1,
   SessionsEnabled=1}` (`:106-111`), `sc.exe start` (`:114-128`, start failure = WARNING
   only), explicit `exit 0` (`:142`). Registration verified by ImagePath read-back
   (`recoverywizard.nsi:352-361`); failure ABORTS the install (mandatory component, no
   half-product, `:355-361`).
7. Start check: `nsExec sc.exe start` (`:366`); 0/1056 → `installed` (`:369-372`); anything
   else → `installed-not-running` + truthful `MessageBox /SD IDOK` naming the Secure-Boot/
   test-signing remedy (`:373-383`) — install completes, product refuses raw-disk work
   (`ERR_DRIVER_REQUIRED`).
8. "-post": ProgramData dirs (`:391-392`), SW key + read-backs (`:399-416`), inner
   uninstall keys swept + product uninstall key written (`:418-430`), `WriteUninstaller`
   (`:432`).

### D.3 Uninstaller coverage (what `HeicFastUninstall.exe /S` removes) vs placed

Order: ask-close (FindWindow on window text "Recovery Wizard", `/SD IDCANCEL` silent-refusal,
`:585-597`) → driver teardown via `uninstall.ps1` (`ExecWait … uninstall.ps1 -ServiceName
RDA`, `:608`; fallback `sc stop/delete` `:614-621`; belt-and-braces `Sysnative` +
literal `System32\drivers\RDA.sys` deletes `/REBOOTOK`, `:631-636`) → **cert removal by
thumbprint** (`certutil -delstore Root/TrustedPublisher`, `:649, 651`) → sessions consent
(silent ⇒ always KEEP, `:666-669`; interactive YESNO `/SD IDNO`, `:670-678`) → inner Tauri
uninstaller silent (`CopyFiles` to `%TEMP%` + `ExecWait '"$TEMP\RW-inner-uninst.exe" /S
_?=$INSTDIR'`, `:681-686`) → kit deletes (explicit list `:693-702` + catch-all
`Delete "$INSTDIR\*.dll"` `:710` + `RMDir /r "$INSTDIR\driver"` `:711`) → registry
(uninstall key + SW key `:714-716`; seed key + empty parent `:724-729`) → final tidy
(`:732-734`).

**Coverage diff (placed vs removed):** T15 observed a perfect silent uninstall —
`leftovers: 0`, `installDirExists: false`, service gone, `System32\drivers\RDA.sys` gone,
both registry keys gone, cert gone from both stores, sessions kept
(`08-after-uninstall.json`, `matrix.json:48-61`). Remaining gaps:

- **Orphan (by design):** `%TEMP%\HeicFastRecoveryWizard-install-facts.txt` is never
  deleted (lifecycle log, `recoverywizard.nsi:97-112`); same for the abort-reason file.
- **Orphan (edge):** if the sessions dir is EMPTY at uninstall, the `${If} ${FileExists}
  "${PROGRAMDATA_SESSIONS}\*.*"` guard (`:656`) is false → nothing is asked or deleted →
  the empty `C:\ProgramData\HeicFast\Recovery Wizard\{sessions,logs}` tree remains.
- **Orphan (abort path):** if the install ABORTS at driver-registration failure
  (`:361`), the certificate imported at `:301/:307` stays in Root+TrustedPublisher and the
  inner Tauri uninstall entry stays (RemoveInnerUninstallKeys never ran; the outer
  uninstaller was never written) — an orphaned trust anchor + a partial product until a
  later successful install/uninstall cycle.
- **Shortcuts:** removed by the INNER uninstaller only; never asserted by T15.
- **Maintenance hazard:** the explicit delete list (`:693-702`) enumerates 9 legacy DLL
  names; a future kit file that is neither `*.dll` nor in the list (e.g. a `.pdb`/config)
  would survive (only `RMDir $INSTDIR` would fail silently). Round-13 history documents the
  gtest/gmock leak that motivated the catch-all (`:703-709`).

### D.4 Update / repair semantics

Update = rerun the same installer in place: inner setup overwrites app files, driver step
does stop → delete → recreate (no reboot; demand-start with real DriverUnload,
`recoverywizard.nsi:323-336`), cert import is `-f` idempotent, ProgramData untouched,
uninstall key rewritten. T15 update row: exit 0, session marker survived, registry intact
(`matrix.json:29-37`, `05-after-reinstall.json`). Version-bump updates were NOT exercised
(reinstall-over-same-version only — honest note, `tools/validation/installer-matrix.ps1:29-31`).
Repair = identical reinstall-over (`NoRepair=1`, `recoverywizard.nsi:430`; documented
`installer/README.md:89-92`); T15 corrupted `cr_ui.dll` and proved byte-exact restoration
(`matrix.json:38-46`, `07-after-repair.json`).

### D.5 Installer logging

- **Always-on facts file** `%TEMP%\HeicFastRecoveryWizard-install-facts.txt`: mode, child
  exit codes, cert thumbprint, ImagePath read-back, registry read-backs
  (`recoverywizard.nsi:97-112`); uninstall appends to the same file (`:569`, `:609`,
  `:650-652`, `:716`).
- **Abort file** `%TEMP%\HeicFastRecoveryWizard-install-abort.txt` (`:88-95`).
- Interactive runs additionally show `DetailPrint` lines on the InstFiles page
  (e.g. `:200, 247, 278, 300, 348`); there is NO NSIS log file (no `LogSet`/`/LOG`), and
  nothing is written to `$INSTDIR` or ProgramData by the installer itself.

---

## E. Runtime-expectation cross-check (app expects vs installer places)

| Runtime expectation | Code cite | Installer places | Verdict |
|---|---|---|---|
| `cr_ui.dll` + runtime DLLs in the EXE's directory (static import resolved at process start) | `ui/src-tauri/build.rs:35` (`rustc-link-lib=dylib=cr_ui`); `ui/src-tauri/src/bridge/abi.rs:462-` extern block; layout truth `recoverywizard.nsi:241-245` | kit beside `recovery-wizard-ui.exe` at `$INSTDIR` | ✅ match (T15: app + cr_ui + 3 runtime DLLs) |
| `cr_ui` ABI version 2 | `ui/src-tauri/src/lib.rs:76-83` (startup gate, eprintln on mismatch) | kit from the same build | ✅ (single-build kit) |
| SCM service `RDA` existing + running; `\\.\RDA` opens; `IOCTL_RDA_GET_VERSION` 0x60002448 | `ui/src-tauri/src/commands/system.rs:20-22, 34-99`; engine `engine/src/win/cr_transport.cpp:396-408`; `engine/src/ui/cr_ui.cpp:1161, 1209, 1282` (hardcoded `L"RDA"`) | service RDA created + started; device `\\.\RDA` | ✅ match (T15: Running; ImagePath read-back) |
| `System32\drivers\RDA.sys` at the ImagePath | `install.ps1:58-59, 83` | copied + renamed, WOW64-safe | ✅ (T15 `sysDriverFile: false` after uninstall) |
| Sessions dir `%ProgramData%\HeicFast\Recovery Wizard\sessions` (created on demand) | `ui/src-tauri/src/bridge/state.rs:513-530` | pre-created by installer (`recoverywizard.nsi:391`) | ✅ match |
| **Logs dir** `%ProgramData\HeicFast\Recovery Wizard\logs` — contracts claim a "bridge logger" writes it | claim: `contracts/README.md:49`; plan `docs/INTEGRATION-PLAN.md:836` | installer creates it (`recoverywizard.nsi:392`) | ❌ **MISMATCH**: the shipped app has NO file logger — `lib.rs` only `eprintln!`s the ABI mismatch (`lib.rs:78-82`); nothing in `ui/src-tauri/src/**` ever writes the logs dir (grep: zero `logs` path references). The installer places a directory no runtime consumer exists for; the contracts row is stale. |
| **Portable mode: sessions beside the exe** — README-PORTABLE claims "nothing is written to ProgramData"; contracts claim an "app-dir writability probe at startup" decides | claims: `installer/portable.ps1:79-80` (README text), `contracts/README.md:53-58`, `docs/INTEGRATION-PLAN.md:936-939` | n/a (portable) | ❌ **MISMATCH/DEFECT**: `sessions_dir()` (`state.rs:513-522`) prefers `%ProgramData%\HeicFast\Recovery Wizard\sessions` whenever the `ProgramData` env var is non-empty — it is ALWAYS set on Windows. There is no writability probe and no install-mode detection: the portable build writes sessions to ProgramData exactly like the installed build. The README-PORTABLE statement and the contracts/plan prose are both false for the shipped code. |
| `rrecov.exe` beside the app | no runtime consumer (engine CLI) | placed (`recoverywizard.nsi` via kit; `build.ps1:114`) | ➖ informational (diagnostic surface) |
| `cr_ui.lib` | link-time only | excluded from install; included in portable zip | ➖ inconsistency (B.3) |
| `DriverCertThumbprint` in the product SW key | uninstaller only (`recoverywizard.nsi:645-653`) | written `:404` | ✅ |
| WebView2 Runtime present on the machine | Tauri shell; `webviewInstallMode: downloadBootstrapper` (`tauri.conf.json:49-51`) | INNER setup downloads+runs the bootstrapper when missing (network at install time) | ⚠️ not covered by T15 (runner has WebView2); Win10/11 normally preinstalled |

---

## F. ALL process spawns in `installer/recoverywizard.nsi` + visibility analysis

`ExecWait` hands the command line to CreateProcess verbatim; a console-subsystem child
(`certutil`, `powershell`) launched from this GUI-subsystem installer gets a **NEW, VISIBLE
console window** on the user's desktop. `nsExec::ExecToLog` runs the child hidden with
stdout captured into the install log. The installer is a 32-bit process (PE32,
`recoverywizard.nsi:623-624`), so bare `powershell.exe`/`certutil` resolve to the 32-bit
SysWOW64 binaries (noted at `driver/scripts/install.ps1:62-65`).

**Install path:**

| # | Line | Command | Visibility on a user's desktop |
|---|---|---|---|
| 1 | `recoverywizard.nsi:216` | `"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR` (ExecWait) | **No window** — inner Tauri/NSIS setup is GUI-subsystem and silent. (Its own template may spawn the WebView2 bootstrapper — visible UI possible, only when WebView2 is missing.) |
| 2 | `:301` | `certutil -addstore -f Root "<…>\rda-testcert.cer"` (ExecWait) | **VISIBLE console window** (flash) |
| 3 | `:307` | `certutil -addstore -f TrustedPublisher "<…>\rda-testcert.cer"` (ExecWait) | **VISIBLE console window** (flash) |
| 4 | `:330` | `sc.exe stop RDA` (nsExec::ExecToLog; only when an existing service was found) | Hidden |
| 5 | `:333` | `sc.exe delete RDA` (nsExec) | Hidden |
| 6 | `:349` | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "<…>\driver\install.ps1" -ServiceName RDA -Source "<…>\driver\rda.sys"` (ExecWait) | **VISIBLE PowerShell console window** — the known "console flash" root cause; the .nsi acknowledges it as "the honest cost" of verbatim quoting (`:347`). Inside this same window `install.ps1` runs its children: `Add-Type` (in-proc), `sc.exe create` (`install.ps1:103`), `sc.exe start` (`:115`) — no additional windows. |
| 7 | `:366` | `sc.exe start RDA` (nsExec) | Hidden |

**Uninstall path (`HeicFastUninstall.exe`):**

| # | Line | Command | Visibility |
|---|---|---|---|
| 8 | `:608` | `powershell.exe … -File "<…>\driver\uninstall.ps1" -ServiceName RDA` (ExecWait) | **VISIBLE PowerShell console window** (hosts `sc.exe stop`/`sc.exe delete`, `uninstall.ps1:28-31`) |
| 9–10 | `:615, :618` | `sc.exe stop` / `sc.exe delete` (nsExec; fallback only, script missing/failed) | Hidden |
| 11 | `:649` | `certutil -delstore Root <thumbprint>` (ExecWait) | **VISIBLE console window** |
| 12 | `:651` | `certutil -delstore TrustedPublisher <thumbprint>` (ExecWait) | **VISIBLE console window** |
| 13 | `:684` | `"$TEMP\RW-inner-uninst.exe" /S _?=$INSTDIR` (ExecWait) | No window (silent NSIS GUI-subsystem child) |

Also on the install path: the MUI finish-page launch checkbox runs the app
(`recoverywizard.nsi:169-170`) — a GUI window, user-initiated.

**Complete list of visible console spawns (user-visible):**
install → `certutil` ×2 (`:301`, `:307`) + `powershell install.ps1` ×1 (`:349`);
uninstall → `powershell uninstall.ps1` ×1 (`:608`) + `certutil` ×2 (`:649`, `:651`).
**Five distinct console windows across the product lifecycle; three during a single
interactive install.** (T15 runs silent via `Start-Process -Wait` on a headless runner —
these windows were never seen/verified by the matrix.)

---

## G. Discrepancies vs prior context sheets and vs the T15 evidence

1. **`docs/INTEGRATION-PLAN.md` §7.1 row 2** — plan: engine kit → `<app>\engine\` incl.
   "app-local VC143 CRT + UCRT + api-ms-win-crt-*" (`docs/INTEGRATION-PLAN.md:858`).
   Actual: kit DLLs ride BESIDE the exe at `$INSTDIR` (loader truth,
   `recoverywizard.nsi:241-250`), and NO CRT/UCRT DLLs ship (T15 inventory). Corrected in
   `installer/README.md:50-55` + `docs/BUILD.md:63-74` — plan text is stale.
2. **§7.1 row 1** — plan names the app `RecoveryWizard.exe` + "WebView2 bootstrapper" beside
   it (`INTEGRATION-PLAN.md:857`); actual binary is `recovery-wizard-ui.exe`
   (`recoverywizard.nsi:131`) and WebView2 is downloaded by the inner setup on demand
   (`tauri.conf.json:49-51`), not shipped.
3. **§7.2** — plan retains per-user installs + an optional driver checkbox
   (`INTEGRATION-PLAN.md:873-884`); actual: all-users ONLY, driver `SectionIn RO`
   (`recoverywizard.nsi:27-33, 150-154`). Recorded as deviation #5 (`installer/README.md:113`).
4. **§7.4 resolution note** — plan says the installer "honors surfaced elevation via Tauri
   NSIS `installMode: both` (launches asInvoker…)" (`INTEGRATION-PLAN.md:929-930`); actual
   outer installer is `RequestExecutionLevel admin` — one UAC at launch
   (`recoverywizard.nsi:154`), recorded as deviation #1 (`installer/README.md:109`).
5. **`contracts/README.md:49`** — "Logs directory … bridge logger" — no bridge logger
   exists in the shipped code (see E). Stale contract row.
6. **`contracts/README.md:53-58`, `INTEGRATION-PLAN.md §7.5`, `portable.ps1:79-80`** — the
   portable-mode "app-dir writability probe" does not exist; `state.rs:513-522` is a
   `ProgramData`-env-var check, so the portable build writes sessions to ProgramData (see E).
7. **`contracts/README.md:50`** — still cites `VERSION` = `0.5.0`; actual `0.6.0-rc1`.
8. **`docs/context-sheets/DRIVER-DEPLOYMENT.md` §5.1** — describes `install.ps1` with a
   plain `Copy-Item` to System32 (§5.1 step 3, sheet line ~253) and §7 states "rda.sys
   ships unsigned" / "The build never signs" (sheet `DRIVER-DEPLOYMENT.md:294-295`). Both
   superseded by the mandatory-driver + dev-signing session: the copy is now WOW64-guarded
   and verified (`driver/scripts/install.ps1:61-100`), and `rda.sys` is embed-signed at
   packaging time (`build.ps1:161-165`; `docs/DEV-SIGNING.md:13`). The sheet's §13 checklist
   ("ship unsigned", "Install via install.ps1") is otherwise exactly what the product does.
9. **`DRIVER-DEPLOYMENT.md` §3 / rda.inf** — `rda.cat` declared, never built
   (`driver/src/driver/rda.inf:21`); still true; the INF remains staged-but-unused by the
   installer (A.2 #11).
10. **Release evidence artifact carries the private key** — `release.yml:176-182` uploads
    `build/out/` (whole tree, 14-day retention) as `release-build-evidence`, which includes
    `build\out\stage-<v>\devsign.pfx` + `devsign-pfx-password.txt` (`build.ps1:111`,
    `devsign.ps1:120-123, 141-145`). Not shipped to the GitHub Release (files list
    `release.yml:169-174` is the four public artifacts), but the ephemeral signing key is
    downloadable by anyone with repo-artifact read for 14 days.
11. **T15 vs the .nsi — what the matrix actually verified:** silent `/S /allusers` install
    (exit 0, 15 files, exact module set incl. 7-file driver dir, service **Running**, cert
    in both stores, dev-signed rda.sys), reinstall-over (marker survived), repair
    (corrupted `cr_ui.dll` hash restored), silent uninstall (0 leftovers, service+file+keys
    + cert gone, sessions kept) — `tools/validation/installer-matrix.ps1:255-292`,
    `matrix.json`. **NOT verified by T15:** the interactive UX (pages, driver page, message
    boxes), the visible console spawns (headless), Start Menu shortcuts, ProgramData `logs`
    usage, the portable zip (never installed/inventoried), the `installed-not-running`
    Secure-Boot branch (`recoverywizard.nsi:374-383`), the empty-sessions uninstall edge
    (D.3), the abort-path orphans (D.3), WebView2 download, and the installed app actually
    launching from the installed layout (ui-e2e ran against a test-mode build — session
    worklog T1). The install facts show `driver sc start exit=1056`
    (`02-install-facts.txt:8`) — `install.ps1` had already started the service; the .nsi's
    own start returned "already running", consistent with `driverResult=installed`.
12. **Worklog context confirmed:** the session's root-cause note (visible PowerShell popup
    from `ExecWait … install.ps1`) is exactly spawn #6 above (`recoverywizard.nsi:342-349`),
    and the two `certutil` spawns per direction add four more visible consoles that the
    original triage did not enumerate.

---

## Quick totals

- **Installed product:** 15 files in `$INSTDIR` + `System32\drivers\RDA.sys` + 2 ProgramData
  dirs + 4 registry keys + service `RDA` (demand-start) + cert in 2 stores + 2 `%TEMP%`
  lifecycle logs. Uninstall leaves only the (by-design) `%TEMP%` logs — and, on the empty-
  sessions edge, the empty ProgramData tree.
- **Portable zip:** 15 files (app + 7-file engine kit incl. `cr_ui.lib` + 7-file driver
  stage + README), nothing installed, driver manual.
- **Visible windows during install (interactive):** 3 consoles (certutil ×2, PowerShell ×1);
  during uninstall: 3 (PowerShell ×1, certutil ×2).
- **Runtime mismatches (defects to route):** (1) no bridge logger vs contracts/`logs` dir;
  (2) portable sessions still go to ProgramData (no writability probe) vs README/contracts/
  plan claims; (3) `cr_ui.lib` ships in the zip but not the install (cosmetic).
