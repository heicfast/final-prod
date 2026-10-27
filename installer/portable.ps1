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
#   ├── driver\                    rda.sys + rdacli.exe + rda.inf + the SCM
#   │                              install/uninstall scripts — STAGED, NOT
#   │                              INSTALLED. Manual, consent-explicit install
#   │                              (elevated PowerShell) if the user wants it.
#   └── README-PORTABLE.txt        the truthful label
#
# Portable mode behavior (contracts/README.md): the bridge writes sessions and
# logs BESIDE THE EXECUTABLE (.\sessions, .\logs) — the app-dir-writability
# probe at startup decides; there is no configuration surface.
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
$driverStage = Join-Path $StageDir 'driver'
if (-not (Test-Path (Join-Path $driverStage 'rda.sys'))) {
    throw "portable: driver stage not found ($driverStage) - run the driver lane first (build.ps1 -Target driver)"
}

$portableRoot = Join-Path $OutDir "RecoveryWizard-$Version-portable"
if (Test-Path $portableRoot) { Remove-Item $portableRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $portableRoot 'driver') | Out-Null

Copy-Item $appExe -Destination $portableRoot
# Kit DLLs ride BESIDE the exe (static-import loader truth - see recoverywizard.nsi)
Copy-Item (Join-Path $engineKit '*') -Destination $portableRoot -Force
Copy-Item (Join-Path $driverStage '*') -Destination (Join-Path $portableRoot 'driver') -Recurse

$readme = @"
HeicFast Recovery Wizard $Version — PORTABLE BUILD

This is the no-installer build. Differences from the installed product:

  * NO DRIVER IS INSTALLED - AND THE PRODUCT REQUIRES IT. The RDA raw-disk
    kernel driver is the ONLY raw-disk transport: the engine refuses every
    scan, repair and recovery operation with an explicit 'driver required'
    error (ERR_DRIVER_REQUIRED) until the driver runs. There is no
    fallback mode. The driver is staged in .\driver\ (development
    test-signed rda.sys + its public certificate rda-testcert.cer) - to
    install it manually (elevated PowerShell):
        certutil -addstore -f Root .\driver\rda-testcert.cer
        certutil -addstore -f TrustedPublisher .\driver\rda-testcert.cer
        .\driver\install.ps1 -ServiceName RDA -Source .\driver\rda.sys
    The certificate import is what lets the test-signed driver load on
    Secure-Boot-OFF machines; with Secure Boot ON, also enable test
    signing (bcdedit /set testsigning on) and reboot. For the full
    experience use the installer (RecoveryWizard-$Version-setup.exe),
    which does all of this for you.

  * RUN AS ADMINISTRATOR. The driver path (\\.\RDA) and the app's
    elevated operations require elevation.

  * Sessions and logs are written BESIDE THIS EXECUTABLE (.\\sessions,
    .\\logs) — nothing is written to ProgramData or the registry.

  * WebView2 Runtime must be present (Windows 10/11 normally has it).

For the full product (installer, driver step, ProgramData state, Start Menu
shortcuts, complete uninstall), use RecoveryWizard-$Version-setup.exe.
"@
Set-Content -Path (Join-Path $portableRoot 'README-PORTABLE.txt') -Value $readme -Encoding UTF8

$zip = Join-Path $OutDir "RecoveryWizard-$Version-portable.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $portableRoot '*') -DestinationPath $zip -CompressionLevel Optimal
Remove-Item $portableRoot -Recurse -Force

$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
Write-Host "[portable] -> $zip"
Write-Host "[portable] SHA256 $hash"
