#!/usr/bin/env pwsh
# portable.ps1 — the portable (no-installer) build of HeicFast Recovery Wizard.
# Invoked by build.ps1 (Invoke-Portable): -RepoRoot -Version -StageDir -OutDir.
#
# Product shape (docs/INTEGRATION-PLAN.md §7.5):
#   RecoveryWizard-<version>-portable.zip
#   ├── recovery-wizard-ui.exe   the Tauri app (the Cargo binary name —
#   │                              'Built application at: ...\recovery-wizard-ui.exe';
#   │                              frontend embedded; WebView2 runtime must be
#   │                              present on the machine)
#   ├── cr_ui.dll + runtime DLLs    the complete host-binding kit, BESIDE the
#   │                              exe (static-import loader truth - the DLLs
#   │                              must sit in the exe's directory)
#   ├── portable marker            'recovery-wizard.portable' beside the exe —
#   │                              the bridge's portable-mode probe (NG-18):
#   │                              sessions/logs resolve beside the exe
#   └── README-PORTABLE.txt        the truthful label
#
# Portable mode behavior (contracts/README.md): the bridge writes sessions and
# logs BESIDE THE EXECUTABLE (.\sessions, .\logs) — decided by the portable
# marker file written below (NG-18); there is no configuration surface.
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$RepoRoot,
    [Parameter(Mandatory)] [string]$Version,
    [Parameter(Mandatory)] [string]$StageDir,
    [Parameter(Mandatory)] [string]$OutDir
)

$ErrorActionPreference = 'Stop'

$appExe = Join-Path $RepoRoot 'ui\src-tauri\target\release\recovery-wizard-ui.exe'
if (-not (Test-Path $appExe)) {
    throw "portable: app exe not found at $appExe - run the ui lane first (build.ps1 -Target ui)"
}
$engineKit = Join-Path $StageDir 'engine'
if (-not (Test-Path (Join-Path $engineKit 'cr_ui.dll'))) {
    throw "portable: engine kit not staged ($engineKit) - run the engine lane first (build.ps1 -Target engine)"
}

$portableRoot = Join-Path $OutDir "RecoveryWizard-$Version-portable"
if (Test-Path $portableRoot) { Remove-Item $portableRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $portableRoot | Out-Null

Copy-Item $appExe -Destination $portableRoot
# Kit DLLs ride BESIDE the exe (static-import loader truth - see recoverywizard.nsi)
Copy-Item (Join-Path $engineKit '*') -Destination $portableRoot -Force

$readme = @"
HeicFast Recovery Wizard $Version — PORTABLE BUILD

This is the no-installer build. Differences from the installed product:

  * RUN AS ADMINISTRATOR. The product is a pure user-mode application:
    it reads raw disks directly from user mode (\\.\PhysicalDriveN), and
    every raw-disk operation (scan, recover, health, repair) requires an
    elevated (administrator) process. The executable carries the same
    requireAdministrator manifest as the installed product: launching it
    from Explorer raises the one standard UAC prompt at start. Launch
    from an elevated prompt when scripting. A process that somehow runs
    non-elevated gets a clear refusal - never a degraded mode.

  * Sessions and logs are written BESIDE THIS EXECUTABLE (.\\sessions,
    .\\logs) — nothing is written to ProgramData or the registry.

  * WebView2 Runtime must be present (Windows 10/11 normally has it).

For the full product (installer, ProgramData state, Start Menu shortcuts,
complete uninstall), use RecoveryWizard-$Version-setup.exe.
"@
Set-Content -Path (Join-Path $portableRoot 'README-PORTABLE.txt') -Value $readme -Encoding UTF8

# NG-18: the portable-mode marker. The bridge probes for this file beside the
# exe (bridge util::portable_mode): present => sessions/logs resolve beside
# the executable; absent (installed builds) => ProgramData. Deterministic -
# the pre-fix ProgramData env-var probe never detected portable mode.
Set-Content -Path (Join-Path $portableRoot 'recovery-wizard.portable') -Value "portable build $Version" -Encoding ASCII

$zip = Join-Path $OutDir "RecoveryWizard-$Version-portable.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $portableRoot '*') -DestinationPath $zip -CompressionLevel Optimal
Remove-Item $portableRoot -Recurse -Force

$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
Write-Host "[portable] -> $zip"
Write-Host "[portable] SHA256 $hash"
