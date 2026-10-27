; =============================================================================
; recoverywizard.nsi — the HeicFast Recovery Wizard COMBINED product installer
; =============================================================================
; One artifact installs the entire product: the application (inner Tauri NSIS
; setup, driven silent), the native engine kit (cr_ui.dll + runtime DLLs), the
; RDA raw-disk kernel driver (consent-gated, surfaced, explained), and the
; machine-level shared state. Layout, service registration, and privilege
; model per docs/INTEGRATION-PLAN.md §7 and contracts/README.md (constants
; defined ONCE there: service name RDA, ProgramData sessions/logs paths).
;
; Invoked by build.ps1 (Invoke-Installer) with these defines:
;   /DPRODUCT_VERSION=<x.y.z>   from the root VERSION file
;   /DAPP_BUNDLE=<abs path>     the Tauri NSIS setup exe (ui lane output)
;   /DENGINE_KIT=<abs path>     staged engine kit dir (engine lane output)
;   /DDRIVER_STAGE=<abs path>   staged driver dir (driver lane output)
;   /DOUT_DIR=<abs path>        artifact output directory
;
; Design notes (installer/README.md has the full rationale):
;   - Wrapper architecture: the committed build.ps1 contract consumes the
;     Tauri bundle as the app payload; this script wraps it with the product
;     concerns (engine kit, driver, shared state, complete uninstall).
;   - Elevation: RequestExecutionLevel admin — ONE UAC prompt at launch; the
;     per-machine install and the driver step (SCM + System32\drivers) both
;     need it, and the app itself runs elevated (requireAdministrator,
;     INTEGRATION-PLAN §7.4). Deviation from the mandate's ideal
;     "unelevated launch" is recorded in installer/README.md.
;   - ALL-USERS ONLY (mandatory-driver session, 2026-10-27): the driver is
;     a REQUIRED component (machine-level: SCM + System32\drivers + the
;     certificate stores), so per-user installs are REMOVED - MultiUser.nsh
;     is gone, the shell context is fixed to 'all', and the driver section
;     is SectionIn RO (no consent checkbox: installing the product installs
;     the driver; the driver PAGE still explains it - surfaced, never
;     hidden).
;   - DEVELOPMENT TEST-SIGNING: the build embed-signs rda.sys + the engine
;     kit + this installer with the ephemeral build certificate
;     (installer/devsign.ps1; docs/DEV-SIGNING.md). Before the service
;     create the installer imports the staged public certificate into
;     LocalMachine Root + TrustedPublisher (the load prerequisite on
;     Secure-Boot-off machines); the uninstaller removes it by thumbprint.
;   - Uninstall removes EVERYTHING this installer placed: inner app files
;     (via the Tauri uninstaller, silent), engine kit, driver (service stop +
;     delete + file removal via the driver's own uninstall.ps1), registry,
;     shortcuts (inner), and — only with explicit consent — ProgramData
;     sessions (default KEEP, reference parity: user data survives uninstall).
; =============================================================================

Unicode true
SetCompressor /SOLID lzma

!ifndef PRODUCT_VERSION
  !error "PRODUCT_VERSION must be defined (build.ps1 passes it from the root VERSION file)"
!endif
!ifndef APP_BUNDLE
  !error "APP_BUNDLE must be defined (path to the Tauri NSIS setup exe)"
!endif
!ifndef ENGINE_KIT
  !error "ENGINE_KIT must be defined (path to the staged engine kit)"
!endif
!ifndef DRIVER_STAGE
  !error "DRIVER_STAGE must be defined (path to the staged driver files)"
!endif
!ifndef OUT_DIR
  !error "OUT_DIR must be defined (artifact output directory)"
!endif

; Repository root (this script lives in <repo>\installer\). ${__FILEDIR__} is
; the directory of THIS script — robust regardless of makensis working dir.
!ifndef REPO_ROOT
  !define REPO_ROOT "${__FILEDIR__}/.."
!endif

; PATHSEP — the separator used inside File/OutFile filespecs built from the
; build-passed defines. DEFAULT '/' (the Linux cross-compile validation
; proven form); build.ps1 passes /DPATHSEP=\ on Windows, where the Tauri-
; bundled makensis rejects forward-slash filespecs (validation rounds 2-4:
; 'D:\dir\kit/*' and even the space-free 'D:/a/.../app-setup.exe' both
; resolved to 'no files found', while the backslash forms resolve).
!ifndef PATHSEP
  !define PATHSEP "/"
!endif

; AbortWithReason — Abort, but leave a trace: silent installs show NO
; message box, so %TEMP%\HeicFastRecoveryWizard-install-abort.txt is the
; ONLY place the reason survives (the T15 runtime matrix copies it into
; its evidence; a real user's support request can attach it). Defined
; BEFORE the sections — NSIS macros must exist before their first
; !insertmacro (compile-time order).
!macro AbortWithReason REASON
  Push $0
  FileOpen $0 "$TEMP\HeicFastRecoveryWizard-install-abort.txt" w
  FileWrite $0 "${REASON}"
  FileClose $0
  Pop $0
  Abort "${REASON}"
!macroend

; InstallFact — append one line to the ALWAYS-ON install facts file
; (%TEMP%\HeicFastRecoveryWizard-install-facts.txt). The T15 matrix copies
; it into its evidence; a support request can attach it. Silent installs
; leave no other trace (no UI, no console), so the lifecycle records its
; own ground truth: resolved mode, child-process exit codes (the round-13
; driver lesson: install.ps1's failure left NO surviving exit code), and
; read-backs of every product-key write. Dedicated handle variable so fact
; lines can reference $0/$1/$2 freely; no commas in fact lines (NSIS macro
; parameter separators).
Var FactsHandle
!macro InstallFact LINE
  FileOpen $FactsHandle "$TEMP\HeicFastRecoveryWizard-install-facts.txt" a
  FileSeek $FactsHandle 0 END
  FileWrite $FactsHandle "${LINE}$\r$\n"
  FileClose $FactsHandle
!macroend

!define PRODUCT_NAME        "HeicFast Recovery Wizard"
!define PRODUCT_PUBLISHER   "HeicFast"
; Registry layout (INTEGRATION-PLAN §7 row 6, exactly):
;   product SW key    HKLM\Software\HeicFast\Recovery Wizard\{Version, InstallDir,...}
;   uninstall key     HKLM\...\Uninstall\Recovery Wizard  (key name = product name,
;                     NSIS standard; DisplayName carries the full publisher name)
; Validation round 13 caught the original single-key-with-space form
; ("Software\HeicFast Recovery Wizard" — a DIFFERENT location than every
; consumer probes): the T15 matrix reads HKLM:\SOFTWARE\HeicFast\Recovery
; Wizard (a PowerShell two-segment path) and saw only the /D-seed's default
; value, while our named values landed in the single-key form it never
; probed. One truth now: PRODUCT_SWKEY everywhere for the SW tree.
!define PRODUCT_UNKEY       "Recovery Wizard"             ; Uninstall\<name> (Add/Remove entry key)
!define PRODUCT_SWKEY       "Software\HeicFast\Recovery Wizard" ; SW product key (two-level, like ProgramData)
!define DRIVER_SERVICE      "RDA"            ; contracts/README.md: defined ONCE
;                                              (engine opens \\.\RDA\<token>\...;
;                                              device name = UPPERCASE(service name))
!define APP_EXE             "recovery-wizard-ui.exe" ; the Tauri-built binary name (Cargo package name — 'Built application at: ...\recovery-wizard-ui.exe'; tauri.conf has no mainBinaryName override)
!define APP_WINDOW_TITLE    "Recovery Wizard"  ; the Tauri main window's TEXT (tauri.conf.json app.windows[0].title) — FindWindow matches window text, not the product name (the frameless window still carries its title)
!define PROGRAMDATA_ROOT    "$COMMONPROGRAMDATA\HeicFast\Recovery Wizard"
!define PROGRAMDATA_SESSIONS "${PROGRAMDATA_ROOT}\sessions"
!define PROGRAMDATA_LOGS     "${PROGRAMDATA_ROOT}\logs"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${OUT_DIR}${PATHSEP}RecoveryWizard-${PRODUCT_VERSION}-setup.exe"
InstallDir "$PROGRAMFILES64\HeicFast\Recovery Wizard"

; -----------------------------------------------------------------------------
; Includes / libraries
; -----------------------------------------------------------------------------
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "StrFunc.nsh"
${Using:StrFunc} StrStr

; Per-user installs are REMOVED (mandatory driver: machine-level state -
; SCM + System32\drivers + cert stores cannot live per-user). All-users,
; always: SetShellVarContext all in .onInit, SHCTX = HKLM throughout.

RequestExecutionLevel admin

; -----------------------------------------------------------------------------
; Variables
; -----------------------------------------------------------------------------
Var DriverResult          ; "" | "installed" | "installed-not-running" | "failed"
Var DriverCertThumbprint  ; the development certificate's SHA1 (uninstall removes it)
Var EngineDllCount

; -----------------------------------------------------------------------------
; MUI configuration
; -----------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_ICON   "${REPO_ROOT}/ui/src-tauri/icons/icon.ico"
!define MUI_UNICON "${REPO_ROOT}/ui/src-tauri/icons/icon.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${REPO_ROOT}/LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
Page custom DriverPageCreate DriverPageLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

; =============================================================================
; INSTALLER SECTIONS
; =============================================================================

Section "${PRODUCT_NAME} (required)" SEC_APP
  SectionIn RO

  ; ---- 1. Application: drive the Tauri NSIS bundle silently --------------
  ; /S silent; /allusers or /currentuser follows the chosen context; /D=
  ; (last, unquoted by NSIS convention) pins the install dir to OUR dir page.
  ; ExecWait (NOT nsExec): nsExec re-tokenizes the command line and re-quotes
  ; the arguments — a QUOTED /D= is IGNORED by the NSIS parser (docs: 'must
  ; not contain quotes'), so the inner setup silently installed to its own
  ; default dir and our integrity check failed (validation round 8's
  ; diagnosis: the app landed in 'C:\Program Files\Recovery Wizard').
  ; ExecWait hands the string to CreateProcess verbatim; the silent inner
  ; setup writes no console output, so nothing is lost vs ExecToLog.
  SetDetailsPrint textonly
  DetailPrint "Installing application (Tauri bundle, silent)..."
  SetDetailsPrint both
  InitPluginsDir
  File "/oname=$PLUGINSDIR\app-setup.exe" "${APP_BUNDLE}"

  ; Seed the inner installer's install-location registry: Tauri's
  ; RestorePreviousInstallLocation reads the DEFAULT value of
  ; SHCTX\Software\<manufacturer>\<productName> (our identifier
  ; com.heicfast.recoverywizard -> manufacturer 'heicfast'). On the
  ; fresh-machine SILENT flow the /D= pin is not honored (validation
  ; rounds 8-9: the app landed in the inner's own default) — with the
  ; seed, the inner's placeholder-dir path adopts OUR dir. The key is
  ; OWNED by the inner (it records and deletes it in its own uninstaller),
  ; so seeding leaks nothing.
  WriteRegStr SHCTX "Software\heicfast\Recovery Wizard" "" "$INSTDIR"

  ExecWait '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR' $0
  !insertmacro InstallFact "app-setup exit=$0 mode=AllUsers dir=$INSTDIR"
  ${If} $0 != 0
    StrCpy $1 $0   ; $0 becomes the FileWrite handle inside the macro
    !insertmacro AbortWithReason "Application setup failed (exit code $1). The inner Tauri bundle is at $PLUGINSDIR\app-setup.exe."
  ${EndIf}

  ; Integrity + ADOPTION: $INSTDIR becomes WHERE THE PRODUCT IS. The seed
  ; above steers the inner to our dir; if a template change ever makes it
  ; land elsewhere anyway, probe the inner's own defaults and adopt the
  ; real location — the engine kit, driver stage, and registry keys then
  ; record the truth instead of a fiction.
  ${IfNot} ${FileExists} "$INSTDIR\${APP_EXE}"
    ${If} ${FileExists} "$PROGRAMFILES64\${PRODUCT_NAME}\${APP_EXE}"
      StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_NAME}"
      DetailPrint "Adopted the inner install location: $INSTDIR (its per-machine default)"
    ${EndIf}
  ${EndIf}
  ${IfNot} ${FileExists} "$INSTDIR\${APP_EXE}"
  ${AndIfNot} ${FileExists} "$INSTDIR\uninstall.exe"
    !insertmacro AbortWithReason "Application setup did not produce the expected files in $INSTDIR (missing ${APP_EXE} / uninstall.exe)."
  ${EndIf}

  ; ---- 2. Engine kit: cr_ui.dll + import lib + the vcpkg runtime DLLs ----
  ; staged by build.ps1 from the engine's own POST_BUILD-deployed set.
  ; LAYOUT TRUTH (bridge decision, docs/BUILD.md): the Rust bridge STATICALLY
  ; imports cr_ui.dll — the loader resolves static imports from the exe's
  ; directory at process start, so the DLLs ride BESIDE ${APP_EXE} at
  ; $INSTDIR (not an $INSTDIR\engine subfolder, which is off the loader's
  ; search path and would 0xc0000135 at app start).
  SetDetailsPrint textonly
  DetailPrint "Installing native recovery engine..."
  SetDetailsPrint both
  SetOutPath "$INSTDIR"
  File /x cr_ui.lib "${ENGINE_KIT}${PATHSEP}*"

  ${IfNot} ${FileExists} "$INSTDIR\cr_ui.dll"
    !insertmacro AbortWithReason "Engine kit incomplete: cr_ui.dll is missing."
  ${EndIf}

  ; At least one runtime DLL must ride beside cr_ui.dll (zlib1/lz4/liblz4/
  ; zstd naming varies) — count *.dll in the kit; cr_ui.dll alone is fatal
  ; (0xc0000135 at app start, engine CMakeLists.txt:151-154).
  StrCpy $EngineDllCount 0
  FindFirst $0 $1 "$INSTDIR\*.dll"
  ${DoUntil} $1 == ""
    IntOp $EngineDllCount $EngineDllCount + 1
    FindNext $0 $1
  ${Loop}
  FindClose $0
  ${If} $EngineDllCount < 2
    !insertmacro AbortWithReason "Engine kit incomplete: no runtime DLLs beside cr_ui.dll (found $EngineDllCount DLL(s))."
  ${EndIf}
SectionEnd

Section "RDA raw-disk kernel driver (required)" SEC_DRIVER
  ; REQUIRED component (mandatory-driver session, 2026-10-27): SectionIn RO -
  ; there is no product without it (the engine refuses every raw-disk open
  ; when \\.\RDA is absent: ERR_DRIVER_REQUIRED). The driver page explains
  ; it; it cannot be deselected.
  SectionIn RO
  SetDetailsPrint textonly
  DetailPrint "Installing the RDA kernel driver (required component)..."
  SetDetailsPrint both

  ; Stage the driver files (kept in <app>\driver — the signed-drop-in surface
  ; for later: replace .sys + reinstall without touching the app).
  SetOutPath "$INSTDIR\driver"
  File "${DRIVER_STAGE}${PATHSEP}rda.sys"
  File "${DRIVER_STAGE}${PATHSEP}rdacli.exe"
  File "${DRIVER_STAGE}${PATHSEP}rda.inf"
  File "${DRIVER_STAGE}${PATHSEP}install.ps1"
  File "${DRIVER_STAGE}${PATHSEP}uninstall.ps1"
  File "${DRIVER_STAGE}${PATHSEP}rda-testcert.cer"
  File "${DRIVER_STAGE}${PATHSEP}rda-testcert-thumbprint.txt"

  ; ---- Development-certificate import (BEFORE the service create) --------
  ; The build embed-signed rda.sys with the ephemeral build certificate
  ; (docs/DEV-SIGNING.md). A test-signed driver loads when (a) test signing
  ; is ON (the CI runners are imaged that way) or (b) the machine's
  ; LocalMachine Root + TrustedPublisher stores carry the certificate -
  ; THIS import. certutil needs the same elevation the service create does
  ; (the installer is admin-manifest), and ExecWait hands the command line
  ; to CreateProcess verbatim (the round-8 lesson; $INSTDIR has spaces).
  DetailPrint "Importing the development signing certificate (Root + TrustedPublisher)..."
  ExecWait 'certutil -addstore -f Root "$INSTDIR\driver\rda-testcert.cer"' $0
  !insertmacro InstallFact "cert Root addstore exit=$0"
  ${If} $0 != 0
    StrCpy $1 $0
    !insertmacro AbortWithReason "Certificate import failed (certutil Root exit code $1). The development certificate must be importable for the driver to load on this machine."
  ${EndIf}
  ExecWait 'certutil -addstore -f TrustedPublisher "$INSTDIR\driver\rda-testcert.cer"' $0
  !insertmacro InstallFact "cert TrustedPublisher addstore exit=$0"
  ${If} $0 != 0
    StrCpy $1 $0
    !insertmacro AbortWithReason "Certificate import failed (certutil TrustedPublisher exit code $1). The development certificate must be importable for the driver to load on this machine."
  ${EndIf}
  ; remember the thumbprint for the uninstaller (the staged file carries it)
  ClearErrors
  FileOpen $0 "$INSTDIR\driver\rda-testcert-thumbprint.txt" r
  ${IfNot} ${Errors}
    FileRead $0 $DriverCertThumbprint
    FileClose $0
    StrCpy $DriverCertThumbprint $DriverCertThumbprint 40
  ${EndIf}
  !insertmacro InstallFact "cert thumbprint=$DriverCertThumbprint"

  ; Update/repair semantics: an existing RDA service is stopped and removed
  ; first (stop -> delete -> recreate; no reboot — demand-start with a real
  ; DriverUnload, DRIVER-DEPLOYMENT §8).
  ClearErrors
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Services\${DRIVER_SERVICE}" "ImagePath"
  ${Unless} ${Errors}
    DetailPrint "Existing ${DRIVER_SERVICE} service found - updating (stop + delete + recreate)."
    nsExec::ExecToLog 'sc.exe stop ${DRIVER_SERVICE}'
    Pop $0
    Sleep 1500
    nsExec::ExecToLog 'sc.exe delete ${DRIVER_SERVICE}'
    Pop $0
    Sleep 500
  ${EndUnless}

  ; Register via the driver's own install.ps1 — the exact SCM semantics
  ; (type= kernel start= demand error= normal group= "Boot File System",
  ; file renamed to <ServiceName>.sys under System32\drivers, Parameters
  ; {IoctlBase=0, MaxDisks=200, DebugLevel=1, SessionsEnabled=1}).
  ; ExecWait (NOT nsExec): the command line must reach powershell VERBATIM —
  ; nsExec re-tokenizes and re-quotes arguments (the round-8 /D= lesson),
  ; and $INSTDIR ALWAYS contains spaces (C:\Program Files\...). The
  ; round-13 root cause was the dead elevation guard (fixed in the scripts
  ; themselves); ExecWait removes the quoting failure mode as a class. The
  ; console flash is the honest cost; the exit code lands in the facts file.
  DetailPrint "Registering the RDA service (driver install.ps1)..."
  ExecWait 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\driver\install.ps1" -ServiceName ${DRIVER_SERVICE} -Source "$INSTDIR\driver\rda.sys"' $0
  !insertmacro InstallFact "driver install.ps1 exit=$0 instdir=$INSTDIR"

  ClearErrors
  ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Services\${DRIVER_SERVICE}" "ImagePath"
  !insertmacro InstallFact "driver ImagePath readback=$1"
  ${If} ${Errors}
    StrCpy $DriverResult "failed"
    DetailPrint "RDA driver registration failed (exit code $0) - aborting the install."
    ; MANDATORY component: a product without its driver is a broken
    ; product - the install STOPS here (no degraded-mode fiction).
    StrCpy $1 $0
    !insertmacro AbortWithReason "The RDA driver service could not be registered (installer exit code $1). The driver is a REQUIRED component - the product is not installed half-way; correct the problem and rerun (see the Service event log)."
  ${Else}
    ; Start (idempotent): install.ps1 already tried; 0 = started,
    ; 1056 = already running, anything else = did not load (usually the
    ; unsigned-driver policy on stock Win10/11).
    nsExec::ExecToLog 'sc.exe start ${DRIVER_SERVICE}'
    Pop $0
    !insertmacro InstallFact "driver sc start exit=$0"
    ${If} $0 == 0
    ${OrIf} $0 == 1056
      StrCpy $DriverResult "installed"
      DetailPrint "RDA driver installed and running (device \\.\${DRIVER_SERVICE})."
    ${Else}
      StrCpy $DriverResult "installed-not-running"
      ; /SD IDOK — same round-11 lesson: without it the box BLOCKS /S installs.
      ; The service IS registered (demand-start): the machine's signing
      ; policy refused to LOAD it. TRUTHFUL mandatory-driver message: the
      ; install completes, the service starts after the user changes the
      ; policy - and the product REFUSES raw-disk work until it does
      ; (ERR_DRIVER_REQUIRED). No fallback mode exists.
      DetailPrint "RDA driver registered but not started (exit code $0 - machine signing policy; see docs/DEV-SIGNING.md)."
      MessageBox MB_OK|MB_ICONEXCLAMATION "The RDA driver is registered but Windows refused to load it (exit code $0).$\r$\n$\r$\nThis build's driver is DEVELOPMENT TEST-SIGNED. On machines with Secure Boot ON, enable test signing (bcdedit /set testsigning on) and reboot, or disable Secure Boot - the driver will start on the next boot (it is demand-start). On machines with Secure Boot OFF the staged certificate was imported, so it should load now.$\r$\n$\r$\nThe application is installed, but it REQUIRES the running driver for every scan - it will report the driver as missing until it loads. See docs/DEV-SIGNING.md." /SD IDOK
    ${EndIf}
  ${EndIf}
SectionEnd

Section "-post" ; hidden: shared state, registry, uninstaller
  ; ---- ProgramData shared state (contracts/README.md: defined ONCE) ------
  ; Default ACL inheritance: directory creators get ownership; the app runs
  ; elevated and writes freely (installer/README.md documents the ACL choice).
  CreateDirectory "${PROGRAMDATA_SESSIONS}"
  CreateDirectory "${PROGRAMDATA_LOGS}"
  DetailPrint "Shared state: ${PROGRAMDATA_ROOT}"

  ; ---- Product registry key (INTEGRATION-PLAN §7 row 6: two-level form) ---
  ; Mandatory driver: DriverInstalled is 1 whenever we reach here (the
  ; driver section is RO and its registration failure ABORTS the install);
  ; the RUNNING-vs-registered distinction lives in DriverResult below.
  WriteRegDWORD SHCTX "${PRODUCT_SWKEY}" "DriverInstalled" 1
  WriteRegStr   SHCTX "${PRODUCT_SWKEY}" "InstallDir" "$INSTDIR"
  WriteRegStr   SHCTX "${PRODUCT_SWKEY}" "Version" "${PRODUCT_VERSION}"
  WriteRegStr   SHCTX "${PRODUCT_SWKEY}" "InstallMode" "AllUsers"
  ${If} $DriverCertThumbprint != ""
    WriteRegStr SHCTX "${PRODUCT_SWKEY}" "DriverCertThumbprint" "$DriverCertThumbprint"
  ${EndIf}
  ; READ-BACKS into the facts file: in-process proof that every write
  ; landed where the plan says it lands (the round-13 lesson — the values
  ; were being written to a different tree than anything probed).
  ReadRegStr $2 SHCTX "${PRODUCT_SWKEY}" "InstallDir"
  !insertmacro InstallFact "post InstallDir written=$INSTDIR readback=$2"
  ReadRegStr $2 SHCTX "${PRODUCT_SWKEY}" "Version"
  !insertmacro InstallFact "post Version written=${PRODUCT_VERSION} readback=$2"
  ReadRegStr $2 SHCTX "${PRODUCT_SWKEY}" "InstallMode"
  !insertmacro InstallFact "post InstallMode written=AllUsers readback=$2"
  ReadRegDWORD $2 SHCTX "${PRODUCT_SWKEY}" "DriverInstalled"
  !insertmacro InstallFact "post DriverInstalled readback=$2 driverResult=$DriverResult"

  ; ---- Uninstall key: take over the inner installer's entry --------------
  ; The Tauri bundle wrote its own key (name varies across Tauri versions).
  ; Remove any key whose UninstallString points into $INSTDIR, then write the
  ; single, complete product entry pointing at OUR uninstaller.
  Call RemoveInnerUninstallKeys
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "DisplayName"     "${PRODUCT_NAME}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "UninstallString" "$\"$INSTDIR\HeicFastUninstall.exe$\""
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "QuietUninstallString" "$\"$INSTDIR\HeicFastUninstall.exe$\" /S"
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "NoModify" 1
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}" "NoRepair" 1 ; repair = rerun the installer (documented in installer/README.md)

  WriteUninstaller "$INSTDIR\HeicFastUninstall.exe"
SectionEnd

; -----------------------------------------------------------------------------
; Section descriptions (MUST follow the Section definitions: ${SEC_*} section
; index defines are created by the Section commands below.)
; -----------------------------------------------------------------------------
LangString DESC_APP    ${LANG_ENGLISH} "The ${PRODUCT_NAME} application and the native recovery engine. Required."
LangString DESC_DRIVER ${LANG_ENGLISH} "The RDA raw-disk kernel driver (rda.sys, development test-signed): the ONLY raw-disk transport - the engine refuses every scan without it. Required; machine-wide; the development certificate is imported so it can load."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} $(DESC_APP)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DRIVER} $(DESC_DRIVER)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; =============================================================================
; PAGES / CALLBACKS
; =============================================================================

Function DriverPageCreate
  ; The surfaced, explained driver step (the mandate's "no hidden driver
  ; install"). INFORMATIONAL now: the component is REQUIRED - there is no
  ; checkbox, the page explains what installs and under which signature.
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 40u "The RDA kernel driver is a REQUIRED component of ${PRODUCT_NAME}: it is the ONLY transport the engine uses for sector-level reads and writes of disks and volumes (\\.\RDA). It runs with SYSTEM privileges and its control device is restricted to Administrator access."
  Pop $0
  ${NSD_CreateLabel} 0 44u 100% 40u "The application does NOT work without the driver: every scan, repair and recovery operation requires it, and the engine refuses with an explicit 'driver required' error until it runs. There is no fallback mode and no degraded mode."
  Pop $0
  ${NSD_CreateLabel} 0 88u 100% 40u "This build's driver is DEVELOPMENT TEST-SIGNED. The installer imports its certificate into the machine's Root and Trusted Publishers stores. On machines with Secure Boot ON, enable test signing (bcdedit /set testsigning on) and reboot, or disable Secure Boot - the driver starts on the next boot. On Secure-Boot-off machines it loads immediately."
  Pop $0
  ${NSD_CreateLabel} 0 132u 100% 16u "The driver service is demand-start; it can be removed only by uninstalling the product."
  Pop $0

  nsDialogs::Show
FunctionEnd

Function DriverPageLeave
  ; No state to record: the component is required and was installed.
FunctionEnd

; -----------------------------------------------------------------------------
; RemoveInnerUninstallKeys — delete every Uninstall key (HKLM + HKCU) whose
; UninstallString points into $INSTDIR. The inner Tauri installer's entry
; disappears; ours replaces it. ONE entry in Add/Remove Programs, complete
; scope.
; -----------------------------------------------------------------------------
Function RemoveInnerUninstallKeys
  Push $R0 ; key index
  Push $R1 ; key name
  Push $R2 ; uninstall string
  Push $R3 ; match

  StrCpy $R0 0
  ${Do}
    EnumRegKey $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall" $R0
    ${If} $R1 == ""
      ${ExitDo}
    ${EndIf}
    ClearErrors
    ReadRegStr $R2 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R1" "UninstallString"
    ${IfNot} ${Errors}
    ${AndIf} $R2 != ""
      ${StrStr} $R3 $R2 "$INSTDIR"
      ${If} $R3 != ""
        DetailPrint "Removing inner uninstall entry: HKLM Uninstall\$R1"
        DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R1"
      ${EndIf}
    ${EndIf}
    IntOp $R0 $R0 + 1
  ${Loop}

  StrCpy $R0 0
  ${Do}
    EnumRegKey $R1 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall" $R0
    ${If} $R1 == ""
      ${ExitDo}
    ${EndIf}
    ClearErrors
    ReadRegStr $R2 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R1" "UninstallString"
    ${IfNot} ${Errors}
    ${AndIf} $R2 != ""
      ${StrStr} $R3 $R2 "$INSTDIR"
      ${If} $R3 != ""
        DetailPrint "Removing inner uninstall entry: HKCU Uninstall\$R1"
        DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\$R1"
      ${EndIf}
    ${EndIf}
    IntOp $R0 $R0 + 1
  ${Loop}

  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0
FunctionEnd

; =============================================================================
; INIT
; =============================================================================

Function .onInit
  SetRegView 64
  ; Windows 10+ gate (registry-based — robust across NSIS header versions):
  ; CurrentMajorVersionNumber exists (10+) on Win10 and later; absent on
  ; Win7/8/8.1. The WebView2 application shell requires Win10+; the engine
  ; and driver keep their own Win7 envelope (docs/PARITY-DELTAS.md).
  ClearErrors
  ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" "CurrentMajorVersionNumber"
  ${If} ${Errors}
  ${OrIf} $0 < 10
    ; /SD IDOK: silent /S on Win7/8 skips the box (NSIS manual §4.12 —
    ; boxes without /SD block silent installs) and the Abort below still
    ; refuses the install with exit code 2, which is the truthful outcome.
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires Windows 10 or newer (the WebView2 application shell does not exist on older Windows; the engine and driver retain their own Windows 7 support envelope — see docs/PARITY-DELTAS.md)." /SD IDOK
    Abort
  ${EndIf}

  ; ALL-USERS, always (the mandatory driver is machine-level state):
  ; SHCTX = HKLM, $SMPROGRAMS/shortcuts = all users, no per-user context.
  SetShellVarContext all

  ; The facts file starts here: every phase appends (install / reinstall /
  ; repair / uninstall — one lifecycle story per machine).
  !insertmacro InstallFact "onInit version=${PRODUCT_VERSION} mode=AllUsers dir=$INSTDIR"

  StrCpy $DriverResult ""
  StrCpy $DriverCertThumbprint ""
FunctionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
  !insertmacro InstallFact "un.onInit mode=AllUsers dir=$INSTDIR"
FunctionEnd

; =============================================================================
; UNINSTALLER — removes EVERYTHING this installer placed
; =============================================================================

Section "Uninstall"
  ; ---- 0. The install was all-users (the only mode); keep the context ----
  SetShellVarContext all

  ; ---- 1. Ask the user to close the app ----------------------------------
  ; APP_WINDOW_TITLE is the window's real text — the previous
  ; "${PRODUCT_NAME}" lookup never matched any window (the app window is
  ; titled "Recovery Wizard", tauri.conf.json app.windows[0].title), so the
  ; close-app prompt could never fire: a dead check. Live now.
  ask_close:
  FindWindow $0 "" "${APP_WINDOW_TITLE}"
  ${If} $0 != 0
    ; /SD IDCANCEL: in a silent uninstall the box would otherwise BLOCK
    ; (NSIS manual §4.12). The safe silent answer is CANCEL — the app is
    ; running, so the uninstall is refused (Abort) and NOTHING is removed;
    ; an interactive user still gets the retry/cancel choice.
    MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "Please close ${PRODUCT_NAME}, then click Retry to continue." /SD IDCANCEL IDRETRY ask_close IDCANCEL cancel_un
  ${EndIf}
  goto closed
  cancel_un:
  Abort
  closed:

  ; ---- 2. Driver teardown (only if a service is present) ------------------
  ClearErrors
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Services\${DRIVER_SERVICE}" "ImagePath"
  ${IfNot} ${Errors}
    ${If} ${FileExists} "$INSTDIR\driver\uninstall.ps1"
      ; ExecWait — same verbatim-quoting rule as the install side (nsExec
      ; re-tokenizes; $INSTDIR always has spaces; the round-8 /D= lesson
      ; applied to the uninstall direction too).
      DetailPrint "Removing the RDA service (driver uninstall.ps1)..."
      ExecWait 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\driver\uninstall.ps1" -ServiceName ${DRIVER_SERVICE}' $0
      !insertmacro InstallFact "uninstall driver uninstall.ps1 exit=$0"
    ${Else}
      StrCpy $0 1
    ${EndIf}
    ; Fallback (script missing/failed): stop, delete, remove the file.
    ${If} $0 != 0
      nsExec::ExecToLog 'sc.exe stop ${DRIVER_SERVICE}'
      Pop $0
      Sleep 1500
      nsExec::ExecToLog 'sc.exe delete ${DRIVER_SERVICE}'
      Pop $0
      Sleep 500
    ${EndIf}
    ; Belt and braces: the .sys must be gone from the REAL System32.
    ; This installer is a 32-bit process (makensis output PE32), so
    ; $SYSDIR resolves to SysWOW64 and file operations on System32 paths
    ; get WOW64-redirected; the Sysnative alias is the 32-bit process's
    ; view of the real System32 (the driver's uninstall.ps1 carries the
    ; full story + the canonical removal). Delete BOTH views: Sysnative
    ; when this is a WOW64 run, the literal path if a future 64-bit
    ; build makes Sysnative invisible (the alias does not exist for
    ; 64-bit callers - Delete on it is a harmless no-op then).
    ${If} ${FileExists} "$WINDIR\Sysnative\drivers\${DRIVER_SERVICE}.sys"
      Delete /REBOOTOK "$WINDIR\Sysnative\drivers\${DRIVER_SERVICE}.sys"
    ${EndIf}
    ${If} ${FileExists} "$WINDIR\System32\drivers\${DRIVER_SERVICE}.sys"
      Delete /REBOOTOK "$WINDIR\System32\drivers\${DRIVER_SERVICE}.sys"
    ${EndIf}
    DetailPrint "RDA driver removed (service + System32\\drivers\\${DRIVER_SERVICE}.sys)."
  ${EndIf}

  ; ---- 2b. The development certificate we imported --------------------------
  ; The installer added rda-testcert.cer to LocalMachine Root +
  ; TrustedPublisher; the thumbprint was recorded at install. Removing the
  ; product removes the trust anchor it brought in - no orphans.
  ClearErrors
  ReadRegStr $0 SHCTX "${PRODUCT_SWKEY}" "DriverCertThumbprint"
  ${IfNot} ${Errors}
  ${AndIf} $0 != ""
    DetailPrint "Removing the development signing certificate ($0)..."
    ExecWait 'certutil -delstore Root $0' $1
    !insertmacro InstallFact "uninstall cert Root delstore exit=$1 tp=$0"
    ExecWait 'certutil -delstore TrustedPublisher $0' $1
    !insertmacro InstallFact "uninstall cert TrustedPublisher delstore exit=$1 tp=$0"
  ${EndIf}

  ; ---- 3. Shared state: sessions kept by default (explicit consent) ------
  ${If} ${FileExists} "${PROGRAMDATA_SESSIONS}\*.*"
    ; SILENT mode must NEVER delete user data. Truth about NSIS semantics
    ; (round 11): a MessageBox without /SD is DISPLAYED and BLOCKS even in
    ; silent mode (NSIS manual §4.12 — "the script can still display
    ; whatever it wants, use MessageBox's /SD to specify a default for
    ; silent installers"); it is NOT auto-answered. Two independent layers
    ; keep silent uninstalls safe: (1) this ${Silent} guard skips the
    ; question entirely; (2) the box itself carries /SD IDNO — if the guard
    ; ever regresses, the silent default is KEEP. Only an interactive user
    ; can consent to deletion.
    ${If} ${Silent}
      DetailPrint "Silent uninstall: shared state kept (${PROGRAMDATA_ROOT}) — data deletion requires interactive consent."
      goto keep_state
    ${EndIf}
    MessageBox MB_YESNO|MB_ICONQUESTION "Also delete the saved scan sessions in$\r$\n${PROGRAMDATA_SESSIONS} ?$\r$\n$\r$\nChoose No to keep them (recommended — recovered files live wherever you saved them and are never touched)." /SD IDNO IDYES del_state IDNO keep_state
    del_state:
      RMDir /r "${PROGRAMDATA_ROOT}"
      DetailPrint "Shared state removed."
      goto state_done
    keep_state:
      DetailPrint "Shared state kept (${PROGRAMDATA_ROOT})."
    state_done:
  ${EndIf}

  ; ---- 4. Inner application uninstaller (Tauri), silent ------------------
  ${If} ${FileExists} "$INSTDIR\uninstall.exe"
    DetailPrint "Removing application (Tauri uninstaller, silent)..."
    CopyFiles /SILENT "$INSTDIR\uninstall.exe" "$TEMP\RW-inner-uninst.exe"
    ExecWait '"$TEMP\RW-inner-uninst.exe" /S _?=$INSTDIR' $0
    Delete /REBOOTOK "$TEMP\RW-inner-uninst.exe"
  ${EndIf}

  ; ---- 5. Our own files ----------------------------------------------------
  ; Engine kit DLLs live BESIDE the app exe (loader truth — see install):
  ; delete every file the kit staged that the inner uninstaller did not
  ; own (its manifest covers only the Tauri bundle). cr_ui.lib + the
  ; runtime DLLs are ours.
  Delete /REBOOTOK "$INSTDIR\cr_ui.dll"
  Delete /REBOOTOK "$INSTDIR\rrecov.exe"
  Delete /REBOOTOK "$INSTDIR\zlib1.dll"
  Delete /REBOOTOK "$INSTDIR\z.dll"
  Delete /REBOOTOK "$INSTDIR\lz4.dll"
  Delete /REBOOTOK "$INSTDIR\liblz4.dll"
  Delete /REBOOTOK "$INSTDIR\liblz4d.dll"
  Delete /REBOOTOK "$INSTDIR\zstd.dll"
  Delete /REBOOTOK "$INSTDIR\zstdd.dll"
  Delete /REBOOTOK "$INSTDIR\libzstd.dll"
  ; Catch-all: the kit's runtime-DLL set varies by vcpkg naming, and round 13
  ; proved a hardcoded list leaks (gtest/gmock rode along — 4 leftover files
  ; after uninstall). By this point the inner Tauri uninstaller has already
  ; removed ITS files, so every remaining DLL in $INSTDIR is ours; delete
  ; them all, then the explicit list above stays as documentation of the
  ; expected set. Also covers installs made by older installers whose kits
  ; carried the test DLLs.
  Delete "$INSTDIR\*.dll"
  RMDir /r "$INSTDIR\driver"

  ; ---- 6. Registry ---------------------------------------------------------
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}"
  DeleteRegKey SHCTX "${PRODUCT_SWKEY}"
  !insertmacro InstallFact "uninstall registry deleted unkey+swkey (incl. DriverCertThumbprint) mode=AllUsers"

  ; The install-time /D-seed key (Software\heicfast\Recovery Wizard — the
  ; inner setup's install-location key, case-insensitively the SAME tree the
  ; T15 matrix probes). The inner uninstaller does NOT remove it (run 13
  ; evidence: the default value survived our complete uninstall), so we
  ; clean it ourselves: WE seeded it on fresh machines. The empty manufacturer
  ; parent goes too when nothing else lives under it.
  DeleteRegKey SHCTX "Software\heicfast\Recovery Wizard"
  ClearErrors
  EnumRegKey $0 SHCTX "Software\heicfast" 0
  ${If} $0 == ""
    DeleteRegKey SHCTX "Software\heicfast"
  ${EndIf}

  ; ---- 7. Final tidy -------------------------------------------------------
  Delete /REBOOTOK "$INSTDIR\HeicFastUninstall.exe"
  RMDir "$INSTDIR"
  RMDir "$PROGRAMFILES64\HeicFast"
SectionEnd
