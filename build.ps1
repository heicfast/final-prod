#!/usr/bin/env pwsh
# build.ps1 — the single product build entry point (Windows full product).
# docs/BUILD.md documents every sub-build this drives.
#
# Usage:
#   .\build.ps1                 # full product: engine + ui + installer + portable + checksums
#   .\build.ps1 -Target engine  # one lane: engine | driver | ui | installer | portable | contracts | all
#   .\build.ps1 -Configuration Debug
#
# Version policy: the root VERSION file is the product version source of truth.
# The app artifacts (package.json / Cargo.toml / tauri.conf.json) and the
# installer + release tags derive from it; this script stamps them and FAILS if
# the committed state has drifted. Component wire-visible versions (driver
# FILEVERSION reported via IOCTL, engine ABI number 2) are component contracts
# and are documented as such in docs/BUILD.md — they are NOT restamped (that
# would be a behavior change, not a version change).
[CmdletBinding()]
param(
    [ValidateSet('all', 'contracts', 'engine', 'driver', 'ui', 'installer', 'portable')]
    [string]$Target = 'all',
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$RepoRoot = $PSScriptRoot
$Version = (Get-Content (Join-Path $RepoRoot 'VERSION') -Raw).Trim()
$OutDir = Join-Path $RepoRoot 'build\out'
$StageDir = Join-Path $OutDir "stage-$Version"
New-Item -ItemType Directory -Force -Path $OutDir, $StageDir | Out-Null

Write-Host "== final-prod build == version=$Version target=$Target config=$Configuration"

function Assert-VersionConsistency {
    Write-Host "[gate] VERSION consistency ($Version)"
    $packageJson = Get-Content (Join-Path $RepoRoot 'ui\package.json') -Raw | ConvertFrom-Json
    if ($packageJson.version -ne $Version) { throw "ui/package.json version $($packageJson.version) != VERSION $Version" }
    $cargoToml = Get-Content (Join-Path $RepoRoot 'ui\src-tauri\Cargo.toml') -Raw
    if ($cargoToml -notmatch "version\s*=\s*`"$Version`"") { throw "ui/src-tauri/Cargo.toml version != $Version" }
    $tauriConf = Get-Content (Join-Path $RepoRoot 'ui\src-tauri\tauri.conf.json') -Raw | ConvertFrom-Json
    if ($tauriConf.version -ne $Version) { throw "ui/src-tauri/tauri.conf.json version $($tauriConf.version) != VERSION $Version" }
    Write-Host "[gate] VERSION consistent across ui consumers"
}

function Invoke-Contracts {
    Write-Host '[contracts] boundary-truth validation'
    Push-Location $RepoRoot
    node contracts/validate.mjs
    if ($LASTEXITCODE -ne 0) { throw 'contracts validation failed' }
    Pop-Location
}

function Invoke-Engine {
    Write-Host "[engine] CMake win-x64-$Configuration (vcpkg manifest: zlib gtest lz4 zstd)"
    Push-Location (Join-Path $RepoRoot 'engine')
    cmake --preset "win-x64-$Configuration"
    if ($LASTEXITCODE -ne 0) { throw 'engine configure failed' }
    cmake --build --preset "win-x64-$Configuration" --parallel
    if ($LASTEXITCODE -ne 0) { throw 'engine build failed' }
    if (-not $SkipTests -and $Configuration -eq 'Release') {
        ctest --preset "win-x64-$Configuration" --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw 'engine tests failed' }
    }
    Pop-Location

    # PE gates: Win7SP1 loader contract (OS stamp + import allowlist), the
    # engine's own release discipline (tools/stamp_osversion_um.py +
    # tools/verify_pe_um.py; same gates its CI runs).
    $binDir = Join-Path $RepoRoot "engine\build\win-x64-$Configuration"
    foreach ($pe in @("$binDir\cr_ui.dll", "$binDir\rrecov.exe")) {
        if (-not (Test-Path $pe)) { throw "expected engine artifact missing: $pe" }
    }
    python (Join-Path $RepoRoot 'engine\tools\stamp_osversion_um.py') "$binDir\cr_ui.dll" "$binDir\rrecov.exe"
    if ($LASTEXITCODE -ne 0) { throw 'engine PE stamp failed' }
    python (Join-Path $RepoRoot 'engine\tools\verify_pe_um.py') "$binDir\cr_ui.dll" "$binDir\rrecov.exe"
    if ($LASTEXITCODE -ne 0) { throw 'engine PE verify failed' }

    # Assemble the engine kit: cr_ui.dll + import lib + vcpkg runtime DLLs.
    $engineKit = Join-Path $StageDir 'engine'
    New-Item -ItemType Directory -Force -Path $engineKit | Out-Null
    Copy-Item "$binDir\cr_ui.dll", "$binDir\cr_ui.lib" -Destination $engineKit
    Copy-Item "$binDir\rrecov.exe" -Destination $engineKit
    $vcpkgBin = Join-Path $RepoRoot 'engine\vcpkg_installed\x64-windows\bin'
    foreach ($dll in @('z.dll', 'lz4.dll', 'zstd.dll')) {
        Copy-Item (Join-Path $vcpkgBin $dll) -Destination $engineKit -ErrorAction Stop
    }
    Write-Host "[engine] kit staged -> $engineKit"
}

function Invoke-Driver {
    Write-Host '[driver] WDK build (rda.sln x64 Release) + PE gates'
    Push-Location (Join-Path $RepoRoot 'driver')
    nuget restore packages.config -SolutionDirectory . -NonInteractive
    if ($LASTEXITCODE -ne 0) { throw 'driver nuget restore failed' }
    msbuild rda.sln /p:Configuration=$Configuration /p:Platform=x64 /m /v:m
    if ($LASTEXITCODE -ne 0) { throw 'driver build failed' }
    Pop-Location

    $drvBin = Join-Path $RepoRoot "driver\bin\x64\$Configuration"
    python (Join-Path $RepoRoot 'driver\tools\stamp_osversion.py') "$drvBin\rda.sys"
    if ($LASTEXITCODE -ne 0) { throw 'driver PE stamp failed' }
    python (Join-Path $RepoRoot 'driver\tools\verify_pe.py') "$drvBin\rda.sys"
    if ($LASTEXITCODE -ne 0) { throw 'driver PE verify failed (unsigned/import gates)' }

    $driverStage = Join-Path $StageDir 'driver'
    New-Item -ItemType Directory -Force -Path $driverStage | Out-Null
    Copy-Item "$drvBin\rda.sys", "$drvBin\rdacli.exe" -Destination $driverStage
    Copy-Item (Join-Path $RepoRoot 'driver\src\driver\rda.inf') -Destination $driverStage
    Copy-Item (Join-Path $RepoRoot 'driver\scripts\install.ps1'), (Join-Path $RepoRoot 'driver\scripts\uninstall.ps1') -Destination $driverStage
    Write-Host "[driver] staged (unsigned by design; docs/RELEASE.md) -> $driverStage"
}

function Invoke-Ui {
    Write-Host '[ui] pnpm install + frontend build + cargo/tauri build (NSIS bundle)'
    Push-Location (Join-Path $RepoRoot 'ui')
    pnpm install --frozen-lockfile
    if ($LASTEXITCODE -ne 0) { throw 'ui pnpm install failed' }
    pnpm typecheck
    if ($LASTEXITCODE -ne 0) { throw 'ui typecheck failed' }
    pnpm lint
    if ($LASTEXITCODE -ne 0) { throw 'ui lint failed' }
    if (-not $SkipTests) {
        pnpm test
        if ($LASTEXITCODE -ne 0) { throw 'ui tests failed' }
    }

    # Stage the engine kit beside the shell so the bridge links/loads it.
    $shellTarget = Join-Path $RepoRoot 'ui\src-tauri\target\release'
    New-Item -ItemType Directory -Force -Path $shellTarget | Out-Null
    # cargo build is driven by tauri build (beforeBuildCommand runs pnpm build).
    pnpm tauri build
    if ($LASTEXITCODE -ne 0) { throw 'ui tauri build failed' }
    Pop-Location

    $bundleDir = Join-Path $RepoRoot 'ui\src-tauri\target\release\bundle\nsis'
    if (-not (Test-Path $bundleDir)) { throw "tauri NSIS bundle missing: $bundleDir" }
    Write-Host "[ui] NSIS bundle -> $bundleDir"
}

function Find-Makensis {
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'tauri\NSIS\makensis.exe'),
        'C:\Program Files (x86)\NSIS\makensis.exe',
        'C:\Program Files\NSIS\makensis.exe'
    )
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    $cmd = Get-Command makensis -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw 'makensis not found (install NSIS or run a tauri build once so the Tauri NSIS toolchain is present)'
}

function Invoke-Installer {
    Write-Host '[installer] NSIS combined installer (driver + engine + app)'
    $appBundle = Get-ChildItem (Join-Path $RepoRoot 'ui\src-tauri\target\release\bundle\nsis') -Filter '*-setup.exe' |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $appBundle) { throw 'no Tauri NSIS app bundle found; run the ui lane first' }
    $makensis = Find-Makensis
    & $makensis "/DPRODUCT_VERSION=$Version" "/DAPP_BUNDLE=$($appBundle.FullName)" `
        "/DENGINE_KIT=$(Join-Path $StageDir 'engine')" `
        "/DDRIVER_STAGE=$(Join-Path $StageDir 'driver')" `
        "/DOUT_DIR=$OutDir" (Join-Path $RepoRoot 'installer\recoverywizard.nsi')
    if ($LASTEXITCODE -ne 0) { throw 'installer build failed' }
    Write-Host "[installer] -> $OutDir\RecoveryWizard-$Version-setup.exe"
}

function Invoke-Portable {
    Write-Host '[installer] portable zip (no driver installed — app + engine only)'
    & (Join-Path $PSScriptRoot 'installer\portable.ps1') -RepoRoot $RepoRoot -Version $Version -StageDir $StageDir -OutDir $OutDir
    if ($LASTEXITCODE -ne 0) { throw 'portable build failed' }
}

function Invoke-Checksums {
    Write-Host '[out] SHA256SUMS'
    Get-ChildItem $OutDir -Filter '*.exe' -ErrorAction SilentlyContinue | Out-Null
    Get-ChildItem $OutDir -Filter '*.zip' -ErrorAction SilentlyContinue | Out-Null
    $sums = Get-ChildItem $OutDir -File | Where-Object { $_.Name -ne 'SHA256SUMS' }
    $sums | Get-FileHash -Algorithm SHA256 | ForEach-Object { "$($_.Hash)  $(Split-Path $_.Path -Leaf)" } |
        Set-Content (Join-Path $OutDir 'SHA256SUMS')
    Write-Host "[out] artifacts: $($sums.Name -join ', ')"
}

Assert-VersionConsistency
switch ($Target) {
    'contracts' { Invoke-Contracts }
    'engine'    { Invoke-Contracts; Invoke-Engine }
    'driver'    { Invoke-Driver }
    'ui'        { Invoke-Contracts; Invoke-Ui }
    'installer' { Invoke-Installer }
    'portable'  { Invoke-Portable; Invoke-Checksums }
    'all' {
        Invoke-Contracts
        Invoke-Driver
        Invoke-Engine
        Invoke-Ui
        Invoke-Installer
        Invoke-Portable
        Invoke-Checksums
    }
}
Write-Host "== build complete ($Target) =="
