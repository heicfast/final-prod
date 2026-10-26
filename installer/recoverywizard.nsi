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
;   - Per-user installs remain available (MultiUser.nsh); the driver section
;     is force-disabled in per-user mode with a truthful explanation.
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

!define PRODUCT_NAME        "HeicFast Recovery Wizard"
!define PRODUCT_PUBLISHER   "HeicFast"
!define PRODUCT_UNKEY       "HeicFast Recovery Wizard"
!define DRIVER_SERVICE      "RDA"            ; contracts/README.md: defined ONCE
;                                              (engine opens \\.\RDA\<token>\...;
;                                              device name = UPPERCASE(service name))
!define APP_EXE             "recovery-wizard-ui.exe" ; the Tauri-built binary name (Cargo package name — 'Built application at: ...\recovery-wizard-ui.exe'; tauri.conf has no mainBinaryName override)
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

; MultiUser: offer per-user + per-machine; with the admin manifest this still
; launches with one UAC prompt and honors the user's context choice.
; (All MULTIUSER_* defines MUST precede the include.)
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY       "Software\${PRODUCT_UNKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "InstallMode"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY       "Software\${PRODUCT_UNKEY}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallDir"
!include "MultiUser.nsh"

RequestExecutionLevel admin

; -----------------------------------------------------------------------------
; Variables
; -----------------------------------------------------------------------------
Var DriverResult          ; "" | "installed" | "installed-not-running" | "failed" | "skipped"
Var DriverPageCheckbox
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
!define MUI_PAGE_CUSTOMFUNCTION_PRE ComponentsPre
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
  SetDetailsPrint textonly
  DetailPrint "Installing application (Tauri bundle, silent)..."
  SetDetailsPrint both
  InitPluginsDir
  File "/oname=$PLUGINSDIR\app-setup.exe" "${APP_BUNDLE}"

  ${If} $MultiUser.InstallMode == "AllUsers"
    nsExec::ExecToLog '"$PLUGINSDIR\app-setup.exe" /S /allusers /D=$INSTDIR'
  ${Else}
    nsExec::ExecToLog '"$PLUGINSDIR\app-setup.exe" /S /currentuser /D=$INSTDIR'
  ${EndIf}
  Pop $0
  ${If} $0 != 0
    StrCpy $1 $0   ; $0 becomes the FileWrite handle inside the macro
    !insertmacro AbortWithReason "Application setup failed (exit code $1). The inner Tauri bundle is at $PLUGINSDIR\app-setup.exe."
  ${EndIf}

  ; Integrity: the inner installer must have produced the app + its uninstaller
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

Section "RDA raw-disk kernel driver (recommended)" SEC_DRIVER
  ; Surfaces as its own component; the driver page explains + confirms.
  SetDetailsPrint textonly
  DetailPrint "Installing the RDA kernel driver (consent given)..."
  SetDetailsPrint both

  ; Stage the driver files (kept in <app>\driver — the signed-drop-in surface
  ; for later: replace .sys + reinstall without touching the app).
  SetOutPath "$INSTDIR\driver"
  File "${DRIVER_STAGE}${PATHSEP}rda.sys"
  File "${DRIVER_STAGE}${PATHSEP}rdacli.exe"
  File "${DRIVER_STAGE}${PATHSEP}rda.inf"
  File "${DRIVER_STAGE}${PATHSEP}install.ps1"
  File "${DRIVER_STAGE}${PATHSEP}uninstall.ps1"

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
  nsExec::ExecToLog 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\driver\install.ps1" -ServiceName ${DRIVER_SERVICE} -Source "$INSTDIR\driver\rda.sys"'
  Pop $0

  ClearErrors
  ReadRegStr $1 HKLM "SYSTEM\CurrentControlSet\Services\${DRIVER_SERVICE}" "ImagePath"
  ${If} ${Errors}
    StrCpy $DriverResult "failed"
    MessageBox MB_OK|MB_ICONEXCLAMATION "The RDA driver service could not be registered (installer exit code $0).$\r$\n$\r$\nThe application is fully installed and will use Windows direct disk access instead (slower on some media; locked volumes cannot be opened). You can retry the driver later by rerunning this installer."
  ${Else}
    ; Start (idempotent): install.ps1 already tried; 0 = started,
    ; 1056 = already running, anything else = did not load (usually the
    ; unsigned-driver policy on stock Win10/11).
    nsExec::ExecToLog 'sc.exe start ${DRIVER_SERVICE}'
    Pop $0
    ${If} $0 == 0
    ${OrIf} $0 == 1056
      StrCpy $DriverResult "installed"
      DetailPrint "RDA driver installed and running (device \\.\${DRIVER_SERVICE})."
    ${Else}
      StrCpy $DriverResult "installed-not-running"
      MessageBox MB_OK|MB_ICONEXCLAMATION "The RDA driver is installed but could not be started (exit code $0).$\r$\n$\r$\nThis build's driver is UNSIGNED by design (release signing is pending). Windows 10/11 with Driver Signature Enforcement will not load it until it is properly signed, or until test signing is enabled (bcdedit /set testsigning on) — see docs/RELEASE.md.$\r$\n$\r$\nThe application works without it via Windows direct disk access."
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

  ; ---- Product registry key ----------------------------------------------
  ${If} $DriverResult == "installed"
  ${OrIf} $DriverResult == "installed-not-running"
    WriteRegDWORD SHCTX "Software\${PRODUCT_UNKEY}" "DriverInstalled" 1
  ${Else}
    WriteRegDWORD SHCTX "Software\${PRODUCT_UNKEY}" "DriverInstalled" 0
  ${EndIf}
  WriteRegStr   SHCTX "Software\${PRODUCT_UNKEY}" "InstallDir" "$INSTDIR"
  WriteRegStr   SHCTX "Software\${PRODUCT_UNKEY}" "Version" "${PRODUCT_VERSION}"
  WriteRegStr   SHCTX "Software\${PRODUCT_UNKEY}" "InstallMode" "$MultiUser.InstallMode"

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
LangString DESC_DRIVER ${LANG_ENGLISH} "The RDA raw-disk kernel driver (rda.sys): fast, reliable sector-level reads via \\.\RDA. Not available in per-user installs; the app falls back to Windows direct disk access without it."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} $(DESC_APP)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DRIVER} $(DESC_DRIVER)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; =============================================================================
; PAGES / CALLBACKS
; =============================================================================

Function ComponentsPre
  ; Per-user installs cannot take the driver (machine-level: SCM +
  ; System32\drivers). Force it off, make it read-only, and say so.
  ${If} $MultiUser.InstallMode == "CurrentUser"
    SectionSetFlags ${SEC_DRIVER} 0            ; deselect
    SectionSetFlags ${SEC_DRIVER} ${SF_RO}     ; read-only
    SectionSetText ${SEC_DRIVER} "RDA raw-disk kernel driver (not available in per-user installs)"
    StrCpy $DriverResult "skipped"
  ${EndIf}
FunctionEnd

Function DriverPageCreate
  ; The surfaced, explained driver step. Skipped when the component is off.
  ${IfNot} ${SectionIsSelected} ${SEC_DRIVER}
    Abort
  ${EndIf}

  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 40u "The RDA kernel driver gives ${PRODUCT_NAME} fast, reliable sector-level reads of disks and volumes through the \\.\RDA device. It runs with SYSTEM privileges and its control device is restricted to Administrator access."
  Pop $0
  ${NSD_CreateLabel} 0 44u 100% 40u "Without the driver the application still works, using Windows direct disk access instead: somewhat slower on some media, and BitLocker-locked volumes cannot be opened. Nothing about your data changes either way — both transports are read-only for scans."
  Pop $0
  ${NSD_CreateLabel} 0 88u 100% 28u "This build's driver is UNSIGNED (release signing is pending). Windows 10/11 with Driver Signature Enforcement will not load it until signing is in place; the installer registers it either way and the app falls back gracefully."
  Pop $0
  ${NSD_CreateLabel} 0 120u 100% 16u "The driver can be added or removed later at any time by rerunning this installer."
  Pop $0

  ${NSD_CreateCheckbox} 0 142u 100% 12u "Install the RDA kernel driver now (recommended)"
  Pop $DriverPageCheckbox
  ${If} ${SectionIsSelected} ${SEC_DRIVER}
    ${NSD_Check} $DriverPageCheckbox
  ${Else}
    ${NSD_Uncheck} $DriverPageCheckbox
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function DriverPageLeave
  ${NSD_GetState} $DriverPageCheckbox $0
  ${If} $0 == ${BST_CHECKED}
    SectionSetFlags ${SEC_DRIVER} 1
  ${Else}
    SectionSetFlags ${SEC_DRIVER} 0
    StrCpy $DriverResult "skipped"
  ${EndIf}
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
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires Windows 10 or newer (the WebView2 application shell does not exist on older Windows; the engine and driver retain their own Windows 7 support envelope — see docs/PARITY-DELTAS.md)."
    Abort
  ${EndIf}
  !insertmacro MULTIUSER_INIT

  ; Per-user default location (multiuser init set the context by now)
  ${If} $MultiUser.InstallMode == "CurrentUser"
    StrCpy $INSTDIR "$LOCALAPPDATA\Programs\HeicFast\Recovery Wizard"
  ${EndIf}

  StrCpy $DriverResult ""
FunctionEnd

Function un.onInit
  SetRegView 64
  !insertmacro MULTIUSER_UNINIT
FunctionEnd

; =============================================================================
; UNINSTALLER — removes EVERYTHING this installer placed
; =============================================================================

Section "Uninstall"
  ; ---- 0. Shell context follows the recorded install mode ----------------
  ReadRegStr $0 SHCTX "Software\${PRODUCT_UNKEY}" "InstallMode"
  ${If} $0 == "AllUsers"
    SetShellVarContext all
  ${Else}
    SetShellVarContext current
  ${EndIf}

  ; ---- 1. Ask the user to close the app ----------------------------------
  ask_close:
  FindWindow $0 "" "${PRODUCT_NAME}"
  ${If} $0 != 0
    MessageBox MB_RETRYCANCEL|MB_ICONQUESTION "Please close ${PRODUCT_NAME}, then click Retry to continue." IDRETRY ask_close IDCANCEL cancel_un
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
      nsExec::ExecToLog 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$INSTDIR\driver\uninstall.ps1" -ServiceName ${DRIVER_SERVICE}'
      Pop $0
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
    ; Belt and braces: the .sys must be gone (delete-on-reboot if in use).
    ${If} ${FileExists} "$SYSDIR\drivers\${DRIVER_SERVICE}.sys"
      Delete /REBOOTOK "$SYSDIR\drivers\${DRIVER_SERVICE}.sys"
    ${EndIf}
    DetailPrint "RDA driver removed (service + $SYSDIR\drivers\${DRIVER_SERVICE}.sys)."
  ${EndIf}

  ; ---- 3. Shared state: sessions kept by default (explicit consent) ------
  ${If} ${FileExists} "${PROGRAMDATA_SESSIONS}\*.*"
    ; SILENT mode must NEVER delete user data: NSIS auto-answers a
    ; MessageBox with its default button (MB_YESNO -> IDYES), so the
    ; original prompt silently WIPED sessions on every /S uninstall —
    ; the opposite of this block's own "No recommended" copy. Silent
    ; keeps state (recorded in the uninstall log); only an interactive
    ; user can consent to deletion.
    ${If} ${Silent}
      DetailPrint "Silent uninstall: shared state kept (${PROGRAMDATA_ROOT}) — data deletion requires interactive consent."
      goto keep_state
    ${EndIf}
    MessageBox MB_YESNO|MB_ICONQUESTION "Also delete the saved scan sessions in$\r$\n${PROGRAMDATA_SESSIONS} ?$\r$\n$\r$\nChoose No to keep them (recommended — recovered files live wherever you saved them and are never touched)." IDYES del_state IDNO keep_state
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
  RMDir /r "$INSTDIR\driver"

  ; ---- 6. Registry ---------------------------------------------------------
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_UNKEY}"
  DeleteRegKey SHCTX "Software\${PRODUCT_UNKEY}"

  ; ---- 7. Final tidy -------------------------------------------------------
  Delete /REBOOTOK "$INSTDIR\HeicFastUninstall.exe"
  RMDir "$INSTDIR"
  RMDir "$PROGRAMFILES64\HeicFast"
SectionEnd
