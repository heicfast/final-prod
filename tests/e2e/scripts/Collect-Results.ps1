# Collect-Results.ps1 - assemble the committed results tree for one e2e run.
#
# The e2e contract: results are COMMITTED to the repo under
# tests/e2e/results/<run-id>/ for at least one full run (tests/e2e/README.md
# documents the layout; tests/e2e/RESULTS-SCHEMA.md is the schema). Each shard
# job of e2e-full produces a fragment (summary.json + 04-scenarios/<id>/...);
# this script merges the fragments into the run layout and enforces
# completeness: every matrix scenario must appear, else FAIL (no silent drops).
#
# Modes:
#   -MergeRoots <dir>[,<dir>...]  merge fragment dirs into -ResultsRoot
#   -ResultsRoot <dir>            (validate-only: check completeness in place)
#
# Outputs into -ResultsRoot:
#   run.json        run metadata + the per-scenario index (the schema anchor)
#   summary.json    the merged shard summaries
#   gates.md        the human-readable gate table for the WHOLE run
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $ResultsRoot,
    [ValidateSet('full', 'smoke')] [string] $Profile = 'full',
    [string] $MergeRoots = '',
    [string] $MatrixPath = '',
    [string] $RunId = '',
    [string] $RunUrl = '',
    [string] $Commit = '',
    [string] $ProductSha256 = ''
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

New-Item -ItemType Directory -Force -Path $ResultsRoot | Out-Null

# ---- merge fragments -----------------------------------------------------------------------
if ($MergeRoots) {
    foreach ($frag in ($MergeRoots -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })) {
        if (-not (Test-Path -LiteralPath $frag)) { throw "fragment dir not found: $frag" }
        Write-Host "merging fragment: $frag"
        # 04-scenarios/<id> trees
        $fragScen = Join-Path $frag '04-scenarios'
        if (Test-Path -LiteralPath $fragScen) {
            foreach ($sd in @(Get-ChildItem -LiteralPath $fragScen -Directory)) {
                $dst = Join-Path $ResultsRoot "04-scenarios\$($sd.Name)"
                New-Item -ItemType Directory -Force -Path (Split-Path $dst -Parent) | Out-Null
                Copy-Item -LiteralPath $sd.FullName -Destination $dst -Recurse -Force
            }
        }
        foreach ($leaf in @('00-environment', '02-disk-setup', '03-baseline', '99-teardown')) {
            $src = Join-Path $frag $leaf
            if (Test-Path -LiteralPath $src) {
                $dst = Join-Path $ResultsRoot $leaf
                New-Item -ItemType Directory -Force -Path $dst | Out-Null
                Copy-Item (Join-Path $src '*') -Destination $dst -Recurse -Force
            }
        }
        # shard summary -> <shard>.summary.json (provenance per shard)
        $fragSummary = Join-Path $frag 'summary.json'
        if (Test-Path -LiteralPath $fragSummary) {
            $shardName = if ($frag -match '[\\/]?([a-z0-9-]+)[\\/]?$') { $Matches[1] } else { 'shard' }
            Copy-Item -LiteralPath $fragSummary -Destination (Join-Path $ResultsRoot "$shardName.summary.json") -Force
        }
    }
}

# ---- load the matrix + collect the per-scenario index ---------------------------------------
$matrix = $null
if ($MatrixPath -and (Test-Path -LiteralPath $MatrixPath)) {
    $matrix = Get-Content -LiteralPath $MatrixPath -Raw | ConvertFrom-Json
}
# run 45916806570's assemble lesson: the expectation set is PROFILE-SCOPED -
# a full run carries the 45 full-matrix rows, a smoke run the 4 smoke rows;
# mixing both lists into every run's expectation false-failed the all-green
# full run as 'missing 4 scenarios' (the smoke rows are a different lane)
$expectedIds = @()
if ($matrix) {
    if ($Profile -eq 'smoke') {
        $expectedIds += @($matrix.smoke.scenarios | ForEach-Object { $_.id })
    } else {
        $expectedIds += @($matrix.scenarios | ForEach-Object { $_.id })
    }
}

$scenarioDirs = @(Get-ChildItem -LiteralPath (Join-Path $ResultsRoot '04-scenarios') -Directory -ErrorAction SilentlyContinue)
$foundIds = @($scenarioDirs | ForEach-Object { $_.Name })
$missing = @()
if ($expectedIds.Count -gt 0) {
    # validate against the requested profile's matrix rows
    $missing = @($expectedIds | Where-Object { $foundIds -notcontains $_ })
}

# ---- run.json + merged summary ---------------------------------------------------------------
$verdictRows = @()
$hardFailures = @()
$softRecords = @()
foreach ($vd in @(Get-ChildItem -LiteralPath $ResultsRoot -Filter '*.summary.json')) {
    $s = Get-Content -LiteralPath $vd.FullName -Raw | ConvertFrom-Json
    foreach ($row in @($s.scenarios)) {
        $verdictRows += [ordered]@{
            shard = $s.shard; id = $row.id; mode = $row.mode; gate = $row.gate
            passed = $row.passed; cli = $row.cli; api = $row.api; error = $row.error
        }
    }
    if ($s.hard_failures) { $hardFailures += @($s.hard_failures) }
    if ($s.soft_records) { $softRecords += @($s.soft_records) }
}

# if no shard summaries were merged, build verdict rows from the scenario dirs directly
if ($verdictRows.Count -eq 0 -and $scenarioDirs.Count -gt 0) {
    foreach ($sd in $scenarioDirs) {
        $vp = Join-Path $sd.FullName 'verdict.json'
        if (Test-Path -LiteralPath $vp) {
            $v = Get-Content -LiteralPath $vp -Raw | ConvertFrom-Json
            $verdictRows += [ordered]@{
                shard = ''; id = $v.scenario; mode = $v.mode; gate = $v.gate_type
                passed = $v.passed; cli = $v.lanes.cli; api = $v.lanes.api; error = $v.error
            }
        }
    }
}

# hashtable literals take pre-computed values only (an if/try inside the
# literal is a parse error - the runner's AST gate owns this class)
$matrixVersion = if ($matrix) { $matrix.version } else { $null }
$run = [ordered]@{
    run_id = $RunId
    run_url = $RunUrl
    commit = $Commit
    product_sha256 = $ProductSha256
    captured_utc = (Get-Date).ToUniversalTime().ToString('o')
    suite = 'tests/e2e'
    matrix_path = $MatrixPath
    matrix_version = $matrixVersion
    scenarios_expected = $expectedIds.Count
    scenarios_present = $foundIds.Count
    scenarios_missing = $missing
    results_schema = 'tests/e2e/RESULTS-SCHEMA.md'
    scenarios = $verdictRows
}
$run | ConvertTo-Json -Depth 10 | Set-Content (Join-Path $ResultsRoot 'run.json') -Encoding utf8

$merged = [ordered]@{
    captured_utc = (Get-Date).ToUniversalTime().ToString('o')
    hard_failures = $hardFailures
    soft_records = $softRecords
    scenarios = $verdictRows
}
$merged | ConvertTo-Json -Depth 10 | Set-Content (Join-Path $ResultsRoot 'summary.json') -Encoding utf8

# ---- the whole-run gate table ---------------------------------------------------------------
$lines = @(
    "# e2e run $RunId - gate table",
    '',
    "commit: $Commit",
    "run: $RunUrl",
    '',
    '| scenario | gate | cli recall | api recall | verdict |', '|---|---|---|---|---|'
)
foreach ($row in $verdictRows) {
    $cliTxt = if ($row.cli) { "recall $($row.cli.recall_hash)" } else { '-' }
    $apiTxt = if ($row.api) { "recall $($row.api.recall_hash)" } else { '-' }
    $verdictTxt = if ($row.passed) { 'PASS' } else { 'FAIL' }
    $lines += "| $($row.id) | $($row.gate) | $cliTxt | $apiTxt | $verdictTxt |"
}
$lines += ''
if ($missing.Count -gt 0) {
    $lines += "## Missing scenarios (no silent drops): $($missing -join ', ')"
}
if ($hardFailures.Count -gt 0) {
    $lines += '## Failures'
    $hardFailures | Select-Object -First 50 | ForEach-Object { $lines += "- $_" }
} else {
    $lines += 'All hard gates green.'
}
$lines | Set-Content (Join-Path $ResultsRoot 'gates.md') -Encoding utf8

Write-Host "results collected: expected=$($expectedIds.Count) present=$($foundIds.Count) missing=$($missing.Count) -> $ResultsRoot"

# ---- the collector's own gates ---------------------------------------------------------------
# 1. completeness (only when the caller passed the matrix): every scenario present
if ($expectedIds.Count -gt 0 -and $missing.Count -gt 0) {
    Write-Host "::error::missing scenarios: $($missing -join ', ') (no silent drops)" -ForegroundColor Red
    exit 1
}
# 2. every scenario dir carries the mandatory evidence files
$broken = @()
foreach ($sd in $scenarioDirs) {
    foreach ($req in @('scenario.json', 'verdict.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $sd.FullName $req))) { $broken += "$($sd.Name)/$req" }
    }
}
if ($broken.Count -gt 0) {
    Write-Host "::error::scenario evidence incomplete: $($broken -join ', ')" -ForegroundColor Red
    exit 1
}
exit 0
