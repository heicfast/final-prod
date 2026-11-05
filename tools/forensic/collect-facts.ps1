# collect-facts.ps1 - snapshot machine + product state (elevation posture,
# services, install dir, registry, the installed module set) to
# facts-<label>.json. Used before install, after install, and after the run.
#
# Usage: collect-facts.ps1 -OutDir <dir> -Label <preinstall|postinstall|postrun>

param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $true)][string]$Label
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$instDir = 'C:\Program Files\HeicFast\Recovery Wizard'

function Safe([scriptblock]$b) {
    try { & $b } catch { "ERROR: $($_.Exception.Message)" }
}

$f = [ordered]@{
    label        = $Label
    capturedUtc  = (Get-Date).ToUniversalTime().ToString('o')
    os           = Safe { (Get-CimInstance Win32_OperatingSystem).Caption + ' ' + (Get-CimInstance Win32_OperatingSystem).Version }
    physDisks    = Safe { @(Get-Disk | Sort-Object Number | ForEach-Object { [ordered]@{ n = $_.Number; model = $_.Model; sizeGB = [math]::Round($_.Size/1GB,1); bus = $_.BusType; partitions = $_.NumberOfPartitions } }) }
    volumes      = Safe { @(Get-Volume | Where-Object DriveLetter | Sort-Object DriveLetter | ForEach-Object { [ordered]@{ letter = $_.DriveLetter; fs = $_.FileSystem; label = $_.FileSystemLabel; sizeGB = [math]::Round($_.Size/1GB,1) } }) }
    # The product's privilege posture: this probe shell's own token state
    # (the runner is elevated by construction - recorded as the fact it is)
    # and, once installed, the app manifest's requested execution level.
    elevated     = Safe { $id = [Security.Principal.WindowsIdentity]::GetCurrent(); $p = New-Object Security.Principal.WindowsPrincipal($id); [ordered]@{ isElevated = $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator); user = $id.Name } }
    integrityLevel = Safe { (whoami /groups /fo csv | Select-String 'Mandatory Level').Line }
    appManifest  = Safe { $exe = Join-Path $instDir 'recovery-wizard-ui.exe'; if (Test-Path $exe) { $bytes = [IO.File]::ReadAllBytes($exe); $s = [Text.Encoding]::ASCII.GetString($bytes); if ($s -match 'requestedExecutionLevel[^/]*requireAdministrator') { 'requireAdministrator' } elseif ($s -match 'requestedExecutionLevel') { 'as-invoker-or-highestAvailable' } else { 'no-manifest-match' } } else { 'app not installed' } }
    installDir   = Safe { if (Test-Path $instDir) { Get-ChildItem $instDir -Recurse -File | Select-Object @{n='path';e={$_.FullName.Substring($instDir.Length+1)}}, Length | Out-String } else { 'missing' } }
    installDirCount = Safe { if (Test-Path $instDir) { (Get-ChildItem $instDir -Recurse -File).Count } else { 0 } }
    productKey   = Safe { if (Test-Path 'HKLM:\SOFTWARE\HeicFast') { Get-ChildItem 'HKLM:\SOFTWARE\HeicFast' -Recurse | Out-String } else { 'missing' } }
    uninstallKey = Safe { Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*' -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match 'Recovery Wizard' } | Select-Object DisplayName, DisplayVersion, UninstallString | Out-String }
    systemEvents = Safe { Get-WinEvent -FilterHashtable @{LogName='System'; StartTime=(Get-Date).AddHours(-2)} -ErrorAction SilentlyContinue | Where-Object { $_.Message -match 'Recovery Wizard|HeicFast' } | Select-Object -First 10 TimeCreated, Id, ProviderName, Message | Out-String }
}

$dest = Join-Path $OutDir "facts-$Label.json"
$f | ConvertTo-Json -Depth 6 | Set-Content $dest -Encoding UTF8
Write-Host "[collect-facts] $Label -> $dest"
# NOTE: native probes inside Safe{} may leave $LASTEXITCODE nonzero; pwsh
# -command propagates the last native exit code, which would fail the CI
# step spuriously. The step's contract is "the facts file was written" -
# per-field errors are recorded IN the file.
if (Test-Path $dest) { exit 0 } else { Write-Error "facts file was not written"; exit 1 }
