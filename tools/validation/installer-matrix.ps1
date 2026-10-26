#Requires -Version 7
<#
.SYNOPSIS
    installer-matrix.ps1 — T15 runtime install/uninstall/update/repair matrix.

.DESCRIPTION
    Runs the built combined installer on a CLEAN Windows runner and captures
    evidence for docs/INTEGRATION-VALIDATION/ (docs/INTEGRATION-VALIDATION.md
    "Pending evidence" — T15). Every phase writes a transcript and machine-
    readable JSON; the workflow commits the evidence directory.

    Matrix rows (in order):
      1. baseline    — pre-install registry + Program Files + ProgramData
      2. install     — setup.exe /S /allusers (exit code, duration, uninstall
                       registry values, installed-file inventory, driver
                       service state — the graceful-degradation row)
      3. session-preservation — a marker written into the REAL sessions dir
                       survives a reinstall-over-install (the update row)
      4. repair      — corrupt an installed engine DLL, rerun setup.exe /S,
                       verify the hash is restored (the NoRepair=1 rerun path,
                       documented in installer/README.md)
      5. uninstall   — HeicFastUninstall.exe /S: app dir + registry + driver
                       service removed; ProgramData sessions PRESERVED (the
                       .nsi's silent-keeps-state contract — the interactive
                       Only consent can delete)

    Honest notes: the update row exercises reinstall-over-same-version (the
    NSIS overwrite semantics are version-independent; the version-bump update
    mechanics get exercised for real at the RC -> final sequence).

.PARAMETER InstallerPath
    Path to the built RecoveryWizard-<version>-setup.exe.

.PARAMETER OutDir
    Evidence output directory (transcripts + JSON land here).

.EXAMPLE
    .\installer-matrix.ps1 -InstallerPath build\out\RecoveryWizard-0.5.0-setup.exe `
                           -OutDir docs\INTEGRATION-VALIDATION\t15-runtime-matrix
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$InstallerPath,
    [Parameter(Mandatory)] [string]$OutDir
)

$ErrorActionPreference = 'Stop'

$InstallDir = "$env:ProgramFiles\HeicFast\Recovery Wizard"
$AppExeName = 'recovery-wizard-ui.exe'   # the Tauri-built binary name (Cargo package name; APP_EXE in recoverywizard.nsi)
$Unkey = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Recovery Wizard'  # INTEGRATION-PLAN §7 row 6: Uninstall\<product name> (round 13: was 'HeicFast Recovery Wizard' — the .nsi and this probe now carry the plan-exact name)
$SwKey = 'HKLM:\SOFTWARE\HeicFast\Recovery Wizard'  # INTEGRATION-PLAN §7 row 6: two-level product key (round 13 taught the .nsi to write HERE — a PowerShell two-segment path, NOT the single 'HeicFast Recovery Wizard' key with a space)
$FactsFile = "$env:TEMP\HeicFastRecoveryWizard-install-facts.txt"  # the .nsi's always-on lifecycle log (InstallFact macro): mode resolution + child exit codes + registry write read-backs
$SessionsDir = "$env:ProgramData\HeicFast\Recovery Wizard\sessions"
$DriverSvc = 'RDA'

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$timeline = [System.Collections.Generic.List[object]]::new()

function Write-Evidence {
    param([string]$Name, [string]$Content)
    $path = Join-Path $OutDir $Name
    $Content | Set-Content -Path $path -Encoding utf8
    Write-Host "[evidence] $Name ($($Content.Length) bytes)"
}

function Add-Row {
    param([string]$Phase, [string]$Result, [hashtable]$Facts = @{})
    $row = [ordered]@{
        phase    = $Phase
        result   = $Result
        atUtc    = (Get-Date).ToUniversalTime().ToString('o')
        facts    = $Facts
    }
    $timeline.Add($row)
    Write-Host ("[matrix] {0,-24} {1}" -f $Phase, $Result)
}

function Get-RegSnapshot {
    param([string]$Path)
    if (-not (Test-Path $Path)) { return $null }
    $props = Get-ItemProperty -Path $Path
    $out = [ordered]@{}
    foreach ($p in $props.PSObject.Properties | Where-Object { $_.Name -notlike 'PS*' }) {
        $out[$p.Name] = "$($p.Value)"
    }
    return $out
}

function Get-DirInventory {
    param([string]$Path, [switch]$Hash)
    if (-not (Test-Path $Path)) { return $null }
    $files = Get-ChildItem -Path $Path -Recurse -File
    $out = @()
    foreach ($f in $files) {
        $row = [ordered]@{
            rel  = $f.FullName.Substring($Path.Length + 1)
            size = $f.Length
        }
        if ($Hash) { $row.sha256 = (Get-FileHash -Path $f.FullName -Algorithm SHA256).Hash }
        $out += [pscustomobject]$row
    }
    return $out
}

function Invoke-Setup {
    param([string]$LogName)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = Start-Process -FilePath $InstallerPath -ArgumentList '/S', '/allusers' -Wait -PassThru
    $sw.Stop()
    $transcript = @(
        "installer: $InstallerPath"
        "args: /S /allusers"
        "exitCode: $($p.ExitCode)"
        "durationMs: $($sw.ElapsedMilliseconds)"
        "ranAtUtc: $((Get-Date).ToUniversalTime().ToString('o'))"
    ) -join "`n"
    Write-Evidence -Name $LogName -Content $transcript
    # The installer's own lifecycle log rides along at every setup boundary
    # (mode resolution + child exit codes + registry read-backs).
    if (Test-Path $FactsFile) {
        Write-Evidence -Name ($LogName -replace '-transcript\.txt$', '-facts.txt') -Content (Get-Content $FactsFile -Raw)
    }
    return @{ exitCode = $p.ExitCode; durationMs = $sw.ElapsedMilliseconds }
}

# ---------------------------------------------------------------------------
# 1. baseline
# ---------------------------------------------------------------------------
$baseline = [ordered]@{
    installDirExists   = Test-Path $InstallDir
    swKey              = Get-RegSnapshot -Path $SwKey
    uninstallKey       = Get-RegSnapshot -Path $Unkey
    driverService      = Get-Service -Name $DriverSvc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Status
    sessionsDirExists  = Test-Path $SessionsDir
}
Write-Evidence -Name '01-baseline.json' -Content ($baseline | ConvertTo-Json -Depth 4)
Add-Row -Phase 'baseline' -Result ($baseline.installDirExists ? 'DIR-ALREADY-PRESENT' : 'clean') -Facts @{
    driverServicePresent = [bool]$baseline.driverService
}

# ---------------------------------------------------------------------------
# 2. install (silent, all-users)
# ---------------------------------------------------------------------------
$install = Invoke-Setup -LogName '02-install-transcript.txt'
if ($install.exitCode -ne 0) {
    # Silent installs Abort with NO message box — the installer writes the
    # reason to %TEMP%\HeicFastRecoveryWizard-install-abort.txt (the .nsi's
    # AbortWithReason); surface it + probe where the inner setup landed.
    $abortFile = "$env:TEMP\HeicFastRecoveryWizard-install-abort.txt"
    $abortReason = if (Test-Path $abortFile) { Get-Content $abortFile -Raw } else { '(no abort-reason file — the Abort came from .onInit or a plugin)' }
    $probes = @(
        "$env:ProgramFiles\HeicFast",
        "$env:ProgramFiles\HeicFast\Recovery Wizard",
        "$env:ProgramFiles\Recovery Wizard",
        "D:\Program",
        "$env:LOCALAPPDATA\Programs\HeicFast\Recovery Wizard"
    ) | ForEach-Object {
        $exists = Test-Path $_
        if ($exists) {
            [pscustomobject]@{ path = $_; children = @((Get-ChildItem $_ -ErrorAction SilentlyContinue | Select-Object -First 12 -ExpandProperty Name)) }
        }
    }
    Write-Evidence -Name '02-install-failure.json' -Content ([ordered]@{
        exitCode    = $install.exitCode
        durationMs  = $install.durationMs
        abortReason = $abortReason
        dirProbes   = $probes
    } | ConvertTo-Json -Depth 5)
    Add-Row -Phase 'install' -Result "FAILED exitCode=$($install.exitCode)" -Facts @{ abortReason = $abortReason.Trim() }
    Write-Evidence -Name 'matrix.json' -Content ($timeline | ConvertTo-Json -Depth 5)
    throw "installer exited $($install.exitCode): $($abortReason.Trim())"
}

# The installer's registry InstallDir is the SOURCE OF TRUTH for the
# location: the .nsi records the ADOPTED dir when the inner setup lands
# elsewhere (the /D= bypass), so resolve from the registry — the expected
# default above only serves the baseline probe.
$regInstallDir = Get-RegSnapshot -Path $SwKey
if ($regInstallDir -and $regInstallDir.InstallDir) {
    $InstallDir = $regInstallDir.InstallDir
    Write-Host "[matrix] install dir (registry): $InstallDir"
}

$appExe = Join-Path $InstallDir $AppExeName
$crUi = Join-Path $InstallDir 'cr_ui.dll'
$installed = [ordered]@{
    appExePresent = Test-Path $appExe
    crUiPresent   = Test-Path $crUi
    rrecovPresent = Test-Path (Join-Path $InstallDir 'rrecov.exe')
    driverDirFiles = @(Get-ChildItem (Join-Path $InstallDir 'driver') -ErrorAction SilentlyContinue).Count
    swKey         = Get-RegSnapshot -Path $SwKey
    uninstallKey  = Get-RegSnapshot -Path $Unkey
    driverService = if (Get-Service -Name $DriverSvc -ErrorAction SilentlyContinue) { (Get-Service -Name $DriverSvc).Status } else { $null }
    inventory     = Get-DirInventory -Path $InstallDir -Hash
    testDllsShipped = @(Get-ChildItem $InstallDir -Filter '*.dll' -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like 'gtest*' -or $_.Name -like 'gmock*' }).Name
}
Write-Evidence -Name '03-after-install.json' -Content ($installed | ConvertTo-Json -Depth 5)
Add-Row -Phase 'install' -Result 'ok' -Facts @{
    exitCode      = $install.exitCode
    durationMs    = $install.durationMs
    appExe        = $installed.appExePresent
    crUi          = $installed.crUiPresent
    regVersion    = $installed.swKey.Version
    installMode   = $installed.swKey.InstallMode
    files         = @($installed.inventory).Count
    driverService = "$($installed.driverService)"
    # Truthful count: the collection is .Name (null on a clean kit — the
    # null IS the dirt signal for the guard below); @($null).Count would
    # report 1 for a PERFECTLY CLEAN kit (run 15's facts row said 1 while
    # the guard correctly passed). Null-safe count here.
    testDlls      = if ($null -ne $installed.testDllsShipped) { @($installed.testDllsShipped).Count } else { 0 }
}

if (-not ($installed.appExePresent -and $installed.crUiPresent)) {
    throw 'install verification failed: app exe or cr_ui.dll missing'
}
# NOT @(...).Count: when the kit is clean, .Name on the empty pipeline is
# $null, and @($null).Count is 1 (array-wrapping a null yields a one-element
# array) — run 15's own guard threw on a PERFECTLY CLEAN kit with an empty
# list "()". Null-ness IS the dirt signal: any actual match returns a
# non-null string/array of names.
if ($null -ne $installed.testDllsShipped) {
    throw "install verification failed: test-framework DLLs shipped to the customer layout ($($installed.testDllsShipped -join ' ')) - the kit exclusion in build.ps1 failed"
}

# ---------------------------------------------------------------------------
# 3. session preservation across reinstall (the update row)
# ---------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $SessionsDir | Out-Null
$marker = Join-Path $SessionsDir 't15-preservation-marker.txt'
"t15 marker $(Get-Date -AsUTC -Format o)" | Set-Content -Path $marker

$reinstall = Invoke-Setup -LogName '04-reinstall-transcript.txt'
$markerSurvived = Test-Path $marker
Write-Evidence -Name '05-after-reinstall.json' -Content ([ordered]@{
    exitCode        = $reinstall.exitCode
    durationMs      = $reinstall.durationMs
    markerSurvived  = $markerSurvived
    regVersion      = (Get-RegSnapshot -Path $SwKey).Version
    uninstallKey    = Get-RegSnapshot -Path $Unkey
} | ConvertTo-Json -Depth 4)
Add-Row -Phase 'update(reinstall)' -Result $(if ($reinstall.exitCode -eq 0 -and $markerSurvived) { 'ok' } else { 'FAILED' }) -Facts @{
    exitCode       = $reinstall.exitCode
    markerSurvived = $markerSurvived
}

# ---------------------------------------------------------------------------
# 4. repair over corruption
# ---------------------------------------------------------------------------
$crUiHashBefore = (Get-FileHash -Path $crUi -Algorithm SHA256).Hash
Set-Content -Path $crUi -Value 'CORRUPTED-BY-T15' -NoNewline
$corruptHash = (Get-FileHash -Path $crUi -Algorithm SHA256).Hash
if ($corruptHash -eq $crUiHashBefore) { throw 'corruption injection failed to change the file' }

$repair = Invoke-Setup -LogName '06-repair-transcript.txt'
$crUiHashAfter = (Get-FileHash -Path $crUi -Algorithm SHA256).Hash
$repaired = ($crUiHashAfter -eq $crUiHashBefore)
Write-Evidence -Name '07-after-repair.json' -Content ([ordered]@{
    exitCode       = $repair.exitCode
    hashOriginal   = $crUiHashBefore
    hashCorrupted  = $corruptHash
    hashRepaired   = $crUiHashAfter
    restored       = $repaired
} | ConvertTo-Json -Depth 3)
Add-Row -Phase 'repair-over-corruption' -Result $(if ($repaired) { 'ok' } else { 'FAILED' }) -Facts @{
    exitCode = $repair.exitCode
    restored = $repaired
}

# ---------------------------------------------------------------------------
# 5. uninstall (silent) — everything the installer placed removed; sessions KEPT
# ---------------------------------------------------------------------------
$un = "$InstallDir\HeicFastUninstall.exe"
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$p = Start-Process -FilePath $un -ArgumentList '/S' -Wait -PassThru
$sw.Stop()
Start-Sleep -Seconds 5
# The uninstaller appends its side of the lifecycle to the same facts file.
if (Test-Path $FactsFile) {
    Write-Evidence -Name '08-uninstall-facts.txt' -Content (Get-Content $FactsFile -Raw)
}

$after = [ordered]@{
    exitCode          = $p.ExitCode
    durationMs        = $sw.ElapsedMilliseconds
    installDirExists  = Test-Path $InstallDir
    leftovers         = @(Get-ChildItem $InstallDir -Recurse -File -ErrorAction SilentlyContinue).Count
    leftoverNames     = @(Get-ChildItem $InstallDir -Recurse -File -ErrorAction SilentlyContinue |
        ForEach-Object { $_.FullName.Substring($InstallDir.Length + 1) })
    swKeyExists       = Test-Path $SwKey
    uninstallKeyExists = Test-Path $Unkey
    driverService     = if (Get-Service -Name $DriverSvc -ErrorAction SilentlyContinue) { 'present' } else { 'gone' }
    sysDriverFile     = Test-Path "$env:SystemRoot\System32\drivers\$DriverSvc.sys"
    markerSurvived    = Test-Path $marker
    sessionsKept      = Test-Path $SessionsDir
}
Write-Evidence -Name '08-after-uninstall.json' -Content ($after | ConvertTo-Json -Depth 4)
$uninstallOk = ($after.exitCode -eq 0) -and (-not $after.swKeyExists) -and (-not $after.uninstallKeyExists) -and
    ($after.driverService -eq 'gone') -and $after.markerSurvived
Add-Row -Phase 'uninstall(silent)' -Result $(if ($uninstallOk) { 'ok' } else { 'CHECK-LEFTOVERS' }) -Facts @{
    exitCode      = $after.exitCode
    leftoverFiles = $after.leftovers
    swKeyGone     = -not $after.swKeyExists
    unKeyGone     = -not $after.uninstallKeyExists
    driverGone    = $after.driverService -eq 'gone'
    sysDriverGone = -not $after.sysDriverFile
    sessionsKept  = $after.sessionsKept
}

# ---------------------------------------------------------------------------
# evidence footer
# ---------------------------------------------------------------------------
Write-Evidence -Name 'matrix.json' -Content ($timeline | ConvertTo-Json -Depth 6)
$md = @"
# T15 runtime install matrix — evidence transcript

Installer: $(Split-Path $InstallerPath -Leaf) (silent /S /allusers; runner: $env:RUNNER_NAME, image: $env:ImageOS)

| Phase | Result | Facts |
|---|---|---|
$($timeline | ForEach-Object { "| $($_.phase) | $($_.result) | $((($_.facts | ConvertTo-Json -Compress -Depth 4)) -replace '\|', '/') |" } | Out-String)

Raw transcripts: `01-baseline.json` .. `08-after-uninstall.json`, `matrix.json`.
Generated by `tools/validation/installer-matrix.ps1` (validation-evidence workflow).
"@
Write-Evidence -Name 't15-runtime-matrix.md' -Content $md

if ($timeline | Where-Object { $_.result -in 'FAILED', 'CHECK-LEFTOVERS' }) {
    throw 'T15 matrix has failing rows — inspect the evidence files'
}
Write-Host '[matrix] all rows ok'
