# build-installer.ps1 - stage the payload and compile the Inno Setup
# installer (S-038 step 2; the packaging milestone's build entry point).
#
# Inputs:
#   -BuildDir    the cmake build directory (default build\win-x64-release;
#                bin\ = src\app\Release\rrecov.exe + src\ui\Release\cr_ui.dll
#                + the vcpkg runtime DLLs the build placed beside them)
#   -Version     the version string baked into AppVersion + the version key
#   -OutputDir   where core-recovery-setup.exe lands (default dist)
#   -Iscc        explicit ISCC.exe path (default: the standard
#                %ProgramFiles(x86)%\Inno Setup 6 location)
#
# The payload layout is the release SDK's shape (bin/ include/ docs/ data/
# LICENSE); the release workflow remains the real packager - this script is
# the CI-gate and standalone-reproducibility path. session-96: the kernel
# driver is REMOVED from the product (direct raw I/O from user mode); the
# payload carries the core only.
#
# Exit codes: 0 built; 1 missing input; 2 ISCC not found; 3 ISCC failed.

param(
    [string] $BuildDir = 'build\win-x64-release',
    [string] $Version = '1.0.0',
    [string] $OutputDir = 'dist',
    [string] $Iscc = ''
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Fail([int] $Code, [string] $Msg) {
    Write-Host "::error::$Msg"
    exit $Code
}

# ---- validate inputs ----
$appExe = Join-Path $RepoRoot "$BuildDir\src\app\Release\rrecov.exe"
$uiDll = Join-Path $RepoRoot "$BuildDir\src\ui\Release\cr_ui.dll"
if (-not (Test-Path $appExe)) { Fail 1 "rrecov.exe not built: $appExe (run: cmake --preset win-x64-release; cmake --build --preset win-x64-release)" }
if (-not (Test-Path $uiDll)) { Fail 1 "cr_ui.dll not built: $uiDll" }

# ---- stage the payload (clean first: no stale files ever ship) ----
$Payload = Join-Path $PSScriptRoot 'payload'
if (Test-Path $Payload) { Remove-Item -Recurse -Force $Payload }
New-Item -ItemType Directory -Force -Path "$Payload\bin" | Out-Null

# bin/ = the built engine (exe + dll + the vcpkg runtime the build placed
# beside them - z.dll/lz4.dll/zstd.dll when the manifest resolved them)
Copy-Item $appExe "$Payload\bin\" -Force
Copy-Item $uiDll "$Payload\bin\" -Force
Get-ChildItem (Split-Path -Parent $appExe) -Filter '*.dll' -ErrorAction SilentlyContinue |
    Copy-Item -Destination "$Payload\bin\" -Force

# include/ + docs/ + data/ + LICENSE (the release SDK shape)
Copy-Item (Join-Path $RepoRoot 'include') "$Payload\include" -Recurse -Force
Copy-Item (Join-Path $RepoRoot 'docs') "$Payload\docs" -Recurse -Force
if (Test-Path (Join-Path $RepoRoot 'data')) {
    Copy-Item (Join-Path $RepoRoot 'data') "$Payload\data" -Recurse -Force
}
Copy-Item (Join-Path $RepoRoot 'LICENSE') "$Payload\LICENSE" -Force

# ---- locate ISCC ----
if (-not $Iscc) {
    $Iscc = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
    if (-not (Test-Path $Iscc)) {
        $alt = Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe'
        if (Test-Path $alt) { $Iscc = $alt }
    }
}
if (-not (Test-Path $Iscc)) {
    Fail 2 "ISCC.exe not found at '$Iscc' - install Inno Setup 6.3+ (the runner image ships it; choco fallback: choco install innosetup)"
}

# ---- compile ----
New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot $OutputDir) | Out-Null
& $Iscc "/DAppVersion=$Version" "/DPayloadDir=$Payload" "/O$(Join-Path $RepoRoot $OutputDir)" `
    (Join-Path $PSScriptRoot 'core-recovery.iss')
if ($LASTEXITCODE -ne 0) { Fail 3 "ISCC failed (exit $LASTEXITCODE)" }

$Out = Join-Path $RepoRoot "$OutputDir\core-recovery-setup.exe"
if (-not (Test-Path $Out)) { Fail 3 "ISCC reported success but $Out is missing" }
$size = [math]::Round((Get-Item $Out).Length / 1MB, 2)
Write-Host "built: $Out ($size MB), version $Version"
exit 0
