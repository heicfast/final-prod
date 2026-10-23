param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$Session,
    [Parameter(Mandatory = $true)][string]$WorkDir
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $WorkDir | Out-Null

function Invoke-Cli {
    param([int]$Expected, [string[]]$CliArgs)
    $script:LastCliOutput = (& $Exe @CliArgs 2>&1 | Out-String)
    if ($LASTEXITCODE -ne $Expected) {
        throw "rrecov $($CliArgs -join ' ') exited $LASTEXITCODE, expected $Expected`n$script:LastCliOutput"
    }
    return $script:LastCliOutput
}

$usage = Invoke-Cli 0 @()
foreach ($command in @('list', 'scan', 'tree', 'load', 'verify', 'save', 'recover', 'health', 'repair', 'bench')) {
    if ($usage -notmatch "rrecov $command") { throw "usage omits $command" }
}
Invoke-Cli 1 @('definitely-not-a-command') | Out-Null
Invoke-Cli 0 @('list') | Out-Null
Invoke-Cli 1 @('scan', '--definitely-invalid') | Out-Null
Invoke-Cli 1 @('recover', '--definitely-invalid') | Out-Null
Invoke-Cli 1 @('health', '--definitely-invalid') | Out-Null
Invoke-Cli 1 @('bench', '--definitely-invalid') | Out-Null
Invoke-Cli 1 @('repair') | Out-Null
Invoke-Cli 1 @('fix') | Out-Null

$tree = Invoke-Cli 0 @('tree', '--in', $Session, '--json') | ConvertFrom-Json
if ($tree.total -lt 1) { throw 'tree --json returned no fixture rows' }
$loaded = Invoke-Cli 0 @('load', '--in', $Session, '--json') | ConvertFrom-Json
if ($loaded.verified -ne $false -or $loaded.files -lt 1) { throw 'load summary is not truthful' }
$verified = Invoke-Cli 0 @('verify', '--in', $Session, '--json') | ConvertFrom-Json
if ($verified.verified -ne $true -or $verified.files -ne $loaded.files) {
    throw 'verify/load summaries disagree'
}

$saved = Join-Path $WorkDir 'saved.crsf'
Invoke-Cli 0 @('save', '--in', $Session, '--out', $saved) | Out-Null
if ((Get-FileHash $Session).Hash -ne (Get-FileHash $saved).Hash) {
    throw 'save did not preserve the CRSF byte stream'
}
Invoke-Cli 1 @('save', '--in', $Session, '--out', $Session) | Out-Null
Invoke-Cli 0 @('verify', '--in', $saved) | Out-Null

$corrupt = Join-Path $WorkDir 'corrupt.crsf'
Copy-Item $Session $corrupt -Force
$bytes = [System.IO.File]::ReadAllBytes($corrupt)
if ($bytes.Length -lt 16) { throw 'fixture session is unexpectedly small' }
$bytes[[int]($bytes.Length / 2)] = $bytes[[int]($bytes.Length / 2)] -bxor 0x5a
[System.IO.File]::WriteAllBytes($corrupt, $bytes)
Invoke-Cli 1 @('verify', '--in', $corrupt) | Out-Null
Invoke-Cli 1 @('save', '--in', $corrupt, '--out', (Join-Path $WorkDir 'bad-save.crsf')) | Out-Null
if (Test-Path (Join-Path $WorkDir 'bad-save.crsf')) { throw 'save exposed an invalid destination' }

Write-Output 'cli_contract: OK (all commands, session grammar, strict verification, atomic save)'
