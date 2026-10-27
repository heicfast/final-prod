# verify-scenario.ps1 - the acceptance proof for one staged scenario
# (docs/CI-SCENARIOS.md): scan the staged disk with the PREBUILT core
# through the REAL driver, compare found vs the staged ground truth
# (expected set derived from the seed manifest + deletion semantics -
# never from the tool's own output), recover to a fresh destination, and
# hash-verify every recovered file against the staged seed hashes.
# Evidence (logs, expected-vs-observed JSON, hashes) is uploaded by the
# workflow as the scenario's acceptance proof.
#
# Usage: verify-scenario.ps1 -Rrecov <path> -Staging <dir with staged-truth.json>
#        -OutDir evidence
param(
    [Parameter(Mandatory = $true)][string]$Rrecov,
    [Parameter(Mandatory = $true)][string]$Staging,
    [Parameter(Mandatory = $true)][string]$OutDir
)
$ErrorActionPreference = 'Continue'   # collect every result, verdict at the end

function Fail($msg) { Write-Host "::error::$msg"; exit 1 }

New-Item -ItemType Directory -Force $OutDir | Out-Null
$truth = Get-Content (Join-Path $Staging 'staged-truth.json') -Raw | ConvertFrom-Json
$disk = [int]$truth.disk_number
$env:PATH = "$((Split-Path -Parent $Rrecov));$env:PATH"

# ---- 1. the scan (real driver, real disk; the session is the truth input) ----
$session = Join-Path $OutDir 'session.crsf'
& $Rrecov scan --disk $disk --out $session *> (Join-Path $OutDir 'scan-log.txt')
$scanRc = $LASTEXITCODE
Get-Content (Join-Path $OutDir 'scan-log.txt') | Write-Host
if ($scanRc -ne 0) { Fail "scan exited $scanRc (expected 0 on a healthy staged disk)" }

# ---- 2. found-vs-expected (the tree JSON is the observed set) ----
& $Rrecov tree --in $session --json *> (Join-Path $OutDir 'tree.json')
if ($LASTEXITCODE -ne 0) { Fail "tree --json exited $LASTEXITCODE" }
$treeRaw = (Get-Content (Join-Path $OutDir 'tree.json') -Raw)
# the JSON is the last line of the output (progress/transport lines precede)
$jsonLine = ($treeRaw -split "`n" | Where-Object { $_ -match '^\s*\{"total"' } | Select-Object -Last 1)
if (-not $jsonLine) { Fail "no tree JSON found in the output" }
$tree = $jsonLine | ConvertFrom-Json
$observed = @($tree.rows | ForEach-Object { [IO.Path]::GetFileName($_.name) })

$expected = @($truth.expected_recoverable)
$missing = @($expected | Where-Object { $observed -notcontains $_ })
$unexpectedFound = @()   # informational: extra finds are never failures
$results = @()
foreach ($name in $expected) {
    $row = $tree.rows | Where-Object { [IO.Path]::GetFileName($_.name) -eq $name } | Select-Object -First 1
    $results += [ordered]@{
        file     = $name
        expected = $true
        found    = [bool]$row
        observed_size = if ($row) { [long]$row.size } else { $null }
    }
}
[ordered]@{
    expected_count = $expected.Count
    observed_count = $observed.Count
    missing        = $missing
    results        = $results
} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $OutDir 'expected-vs-observed.json')
if ($missing.Count -gt 0) {
    Write-Host "::error::MISSING expected recoverable files: $($missing -join ', ')"
}

# ---- 3. recover to a fresh destination ----
$dest = Join-Path $OutDir 'recovered'
New-Item -ItemType Directory -Force $dest | Out-Null
& $Rrecov recover --in $session --disk $disk --to $dest *> (Join-Path $OutDir 'recover-log.txt')
$recRc = $LASTEXITCODE
Get-Content (Join-Path $OutDir 'recover-log.txt') | Write-Host
if ($recRc -ne 0 -and $recRc -ne 2) { Fail "recover exited $recRc (0 or 2-partial expected)" }

# ---- 4. hash verification: every recovered expected file == its seed hash ----
$hashOk = 0; $hashBad = @()
foreach ($name in $expected) {
    $candidate = Get-ChildItem $dest -Recurse -Filter $name | Select-Object -First 1
    if (-not $candidate) { $hashBad += "$name (not recovered)"; continue }
    $got = (Get-FileHash $candidate.FullName -Algorithm SHA256).Hash
    if ($got -eq $truth.seed_hashes.$name) { $hashOk++ } else { $hashBad += "$name (hash mismatch)" }
}
[ordered]@{
    hash_verified_ok = $hashOk
    hash_failures    = $hashBad
} | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $OutDir 'hash-verification.json')

# ---- verdict ----
if ($missing.Count -gt 0 -or $hashBad.Count -gt 0) {
    Write-Host "SCENARIO VERDICT: FAIL - missing=$($missing.Count) hash_failures=$($hashBad.Count)"
    exit 1
}
Write-Host "SCENARIO VERDICT: PASS - $hashOk/$($expected.Count) expected files recovered + hash-verified against the staged seed truth"
exit 0
