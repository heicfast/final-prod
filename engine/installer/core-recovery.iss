; core-recovery.iss - the app installer (S-038 step 2, built against
; docs/PACKAGING.md - the 13-parameter contract; never invented from
; scratch).
;
; Contract mapping (parameter -> section):
;   1 /SILENT                native Inno flag (no ISS work)
;   2 /VERYSILENT            native Inno flag
;   3 /SUPPRESSMSGBOXES      native Inno flag (documented default per box)
;   4 /NORESTART             native Inno flag + this script never reboots
;   5 /NORESTARTAPPLICATIONS native Inno flag (no CloseApplications use)
;   6 admin-required         PrivilegesRequired=admin (UAC manifest) + the
;                            uninstaller inherits it (unins000 enforces it)
;   7 ARP UninstallString    native (Inno writes it under
;                            ...Uninstall\{#AppId}_is1)
;   8 ARP QuietUninstallString  [Code] CurStepChanged - written + VERIFIED
;                            post-install, the reference's exact form ending
;                            " /SILENT" (Inno does not write it natively)
;   9 unins%.3d multi-inst.  SINGLE-INSTANCE policy (PACKAGING.md section 9):
;                            one frozen AppId, in-place upgrade
;  10 Inno metadata keys     native (Inno writes them all)
;  11 install telemetry      OMIT - PACKAGING.md section 8 decision (no
;                            endpoint; a stub verb that pretends to upload is
;                            worse than absence)
;  12 uninstall telemetry    OMIT - same decision
;  13 version registry key   [Code] CurStepChanged - write+VERIFY (the M-064
;                            write-and-verify lesson) of
;                            HKLM\SOFTWARE\heicfast\core_recovery\Version
;
; The OS gate (PACKAGING.md section 6, REGISTERED divergence: the modernized
; envelope Win7 SP1 -> Win11/Server 2025, x64-only) lives in InitializeSetup
; with the SAME decision table as installer/osgate.ps1 - the OSGATE-V1 marker
; line below is the sync pin tools/test_installer_osgate.py enforces.
;
; Build: installer/build-installer.ps1 (stages the payload, runs ISCC).
; Requires Inno Setup 6.3+ (x64compatible); the windows-2022 runner image
; ships 6.7.1 preinstalled (build-installer autodetects, choco is the
; fallback).
; session-96: the kernel-driver component is REMOVED (the direct raw I/O
; replacement) - the product is a user-mode engine requiring administrator
; privilege for raw disk access at RUN time; the installer ships the core
; only and never touches the service control manager.

#define AppName "core_recovery"
#ifndef AppVersion
#define AppVersion "1.0.0"
#endif
; AppIdRaw is the literal GUID (single braces) - use it in [Code] string
; building (the ARP key Inno creates is named `<GUID>_is1`); AppId carries
; the ISS `{{` escape for the [Setup] directive only
#define AppIdRaw "{2F4B9C31-7A08-4D6E-9B5C-3E1A8F0D4C72}"
#define AppId "{{2F4B9C31-7A08-4D6E-9B5C-3E1A8F0D4C72}"
#define Vendor "heicfast"
#define Product "core_recovery"
#define ProductName "core_recovery"
#ifndef PayloadDir
#define PayloadDir "payload"
#endif

[Setup]
AppId={#AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#Vendor}
DefaultDirName={autopf}\{#Product}
; single instance (PACKAGING.md section 9): one AppId, one ARP entry, in-place
LicenseFile={#PayloadDir}\LICENSE
; parameter 6: install requires elevation (the UAC manifest)
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
; parameters 1-5: the reference's exact silent flag set is native Inno
; behavior; /NORESTART semantics are honored (this script never requests a
; reboot) and no closed applications are restarted (CloseApplications=no).
CloseApplications=no
RestartApplications=no
; parameter 9: the single-instance policy - a second install over the first
; is an in-place upgrade (the frozen AppId keeps one ARP entry)
UsePreviousAppDir=yes
SetupLogging=yes
; the ARP surface (parameters 7-8 + the standard values PACKAGING.md
; section 3 requires); QuietUninstallString is written by [Code] post-install
UninstallDisplayName={#AppName}
Uninstallable=yes
CreateUninstallRegKey=yes
; no Start-menu/desktop surface needed for a console product; the group page
; stays disabled so silent installs take the default without a prompt
DisableProgramGroupPage=yes
DefaultGroupName={#ProductName}
OutputBaseFilename=core-recovery-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
; session-96: the driver component is REMOVED with the kernel driver itself
; (direct raw I/O from user mode replaced it; raw disk access needs the
; elevated token at RUN time, not an installed service)
Name: "engine"; Description: "core_recovery engine (rrecov CLI + cr_ui ABI)"; Types: full custom; Flags: fixed

[Files]
Source: "{#PayloadDir}\bin\*"; DestDir: "{app}\bin"; Components: engine; Flags: ignoreversion recursesubdirs
Source: "{#PayloadDir}\include\*"; DestDir: "{app}\include"; Components: engine; Flags: ignoreversion recursesubdirs
Source: "{#PayloadDir}\docs\*"; DestDir: "{app}\docs"; Components: engine; Flags: ignoreversion recursesubdirs
Source: "{#PayloadDir}\data\*"; DestDir: "{app}\data"; Components: engine; Flags: ignoreversion recursesubdirs
Source: "{#PayloadDir}\LICENSE"; DestDir: "{app}"; DestName: "LICENSE"; Flags: ignoreversion

[Code]
// OSGATE-V1: admit (6.1.7601+ | 6.2 | 6.3 | 10.0+ on x64); refuse (below 6.1.7601, any x86/arm64)

function InitializeSetup(): Boolean;
var
  Version: TWindowsVersion;
begin
  Result := False;
  // native, WOW64-aware probing (PACKAGING.md section 6 keeps the
  // reference's mechanism; Inno's ProcessorArchitecture is
  // GetNativeSystemInfo-backed and GetWindowsVersionEx is unvirtualized)
  GetWindowsVersionEx(Version);
  if ProcessorArchitecture <> paX64 then
  begin
    MsgBox(
      'This product requires a 64-bit (x64) Windows system.',
      mbCriticalError, MB_OK);
    Exit;
  end;
  // the modernized envelope: Win7 SP1 -> Win11 / Server 2025
  if not (((Version.Major = 6) and (Version.Minor = 1) and (Version.Build >= 7601)) or
          ((Version.Major = 6) and (Version.Minor = 2)) or
          ((Version.Major = 6) and (Version.Minor = 3)) or
          (Version.Major >= 10)) then
  begin
    MsgBox(
      'This product requires Windows 7 SP1 or newer (through Windows 11 and' + #13#10 +
      'Windows Server 2025). The detected version ' +
      IntToStr(Version.Major) + '.' + IntToStr(Version.Minor) + '.' +
      IntToStr(Version.Build) + ' is outside the supported envelope.' + #13#10#13#10 +
      'Install refused - no changes were made to this system.',
      mbCriticalError, MB_OK);
    Exit;
  end;
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  ReadBack: String;
  QuietUninstall: String;
  UninstallKey: String;
  UninstallExe: String;
begin
  if CurStep <> ssPostInstall then
    Exit;

  // ---- parameter 13: the version key, write + VERIFY (never write-and-hope)
  RegWriteStringValue(HKLM, 'SOFTWARE\{#Vendor}\{#Product}', 'Version', '{#AppVersion}');
  if (not RegQueryStringValue(HKLM, 'SOFTWARE\{#Vendor}\{#Product}', 'Version', ReadBack)) or
     (ReadBack <> '{#AppVersion}') then
  begin
    RaiseException('version key write+verify FAILED: HKLM\SOFTWARE\{#Vendor}\{#Product}\Version did not read back as "{#AppVersion}"');
  end;

  // ---- parameter 8: QuietUninstallString, write + VERIFY (the reference's
  // exact form: quoted uninstaller path + " /SILENT"). Inno writes
  // UninstallString natively (parameter 7); the quiet form is ours.
  UninstallKey := 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#AppIdRaw}_is1';
  if not RegQueryStringValue(HKLM, UninstallKey, 'UninstallString', UninstallExe) then
  begin
    RaiseException('ARP UninstallString missing - Inno did not create the uninstall entry');
  end;
  // Inno writes UninstallString QUOTED when the path contains spaces
  // ("C:\Program Files\..."); strip the surrounding quotes so the quiet
  // form is the canonical single-quoted shape, never ""path"" (the
  // round-trip gate parses it - run 40682713693's gate-1d lesson)
  if (Length(UninstallExe) >= 2) and (Copy(UninstallExe, 1, 1) = '"') and
     (Copy(UninstallExe, Length(UninstallExe), 1) = '"') then
    UninstallExe := Copy(UninstallExe, 2, Length(UninstallExe) - 2);
  QuietUninstall := '"' + UninstallExe + '" /SILENT';
  RegWriteStringValue(HKLM, UninstallKey, 'QuietUninstallString', QuietUninstall);
  if (not RegQueryStringValue(HKLM, UninstallKey, 'QuietUninstallString', ReadBack)) or
     (ReadBack <> QuietUninstall) then
  begin
    RaiseException('ARP QuietUninstallString write+verify FAILED (the value must end " /SILENT")');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    // the version key was written by [Code] (write+verify), so [Code] owns
    // its removal too - Inno only uninstalls [Registry]-section entries;
    // leaving it behind would be exactly the residue the round-trip gate
    // exists to catch
    RegDeleteValue(HKLM, 'SOFTWARE\{#Vendor}\{#Product}', 'Version');
    RegDeleteKeyIfEmpty(HKLM, 'SOFTWARE\{#Vendor}\{#Product}');
  end;
end;
