# New-Baseline.ps1 - build a tests/e2e/baseline.json CANDIDATE from a completed run.
#
# The baseline policy (tests/e2e/BASELINE.md, inherited from
# engine/bench/metrics/make_baseline.py): the baseline is versioned and updated
# ONLY by a human-reviewed commit. This script EMITS THE CANDIDATE - it never
# writes tests/e2e/baseline.json directly and is never wired into CI
# auto-commit. A human reviews the numbers (the run's gates.md + the analysis),
# then copies the candidate over baseline.json in a reviewed commit.
#
# Schema (engine/bench/metrics/regression_gate.py contract - the same consumer):
#   scenarios.<scenario>.{recall, precision, perf.<mode>.scan_wall_seconds}
# Scan-time rows are taken ONLY from uncapped modes (a capped scan's wall is
# the cap, not the engine's speed - a capped baseline row would be a lie).
# The CLI lane is the baseline lane (the deepcarve headline + per-mode walls);
# the API lane's agreement is enforced per-run by Compare-Lanes.ps1 instead.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $ResultsRoot,   # tests/e2e/results/<run-id>
    [string] $OutJson = '',                                 # candidate output ('' = stdout)
    [string] $RunId = ''
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

$scenRoot = Join-Path $ResultsRoot '04-scenarios'
if (-not (Test-Path -LiteralPath $scenRoot)) { throw "no 04-scenarios under $ResultsRoot" }

$scenarios = [ordered]@{}
$notes = @()
foreach ($sd in @(Get-ChildItem -LiteralPath $scenRoot -Directory | Sort-Object Name)) {
    $cliDir = Join-Path $sd.FullName 'cli'
    if (-not (Test-Path -LiteralPath $cliDir)) {
        $notes += "$($sd.Name): no cli lane dir - EXCLUDED"
        continue
    }
    $headline = Join-Path $cliDir 'metrics-deepcarve.json'
    if (-not (Test-Path -LiteralPath $headline)) {
        $notes += "$($sd.Name): no metrics-deepcarve.json (lane failed) - EXCLUDED"
        continue
    }
    $m = Get-Content -LiteralPath $headline -Raw | ConvertFrom-Json
    $entry = [ordered]@{
        recall = $m.recall
        precision = $m.precision
        perf = [ordered]@{}
    }
    foreach ($mp in @(Get-ChildItem -LiteralPath $cliDir -Filter 'metrics-*.json' | Sort-Object Name)) {
        $mm = Get-Content -LiteralPath $mp.FullName -Raw | ConvertFrom-Json
        $mode = $mp.BaseName -replace '^metrics-', ''
        $perf = $mm.perf
        $pmode = if ($perf) { $perf.$mode } else { $null }
        if (-not $pmode) { continue }
        $wall = $pmode.scan_wall_seconds
        if ($null -eq $wall) { continue }
        if ($pmode.scan_capped) {
            $notes += "$($sd.Name)/${mode}: scan was CAPPED (${wall}s) - wall time excluded from baseline"
            $entry.perf.$mode = [ordered]@{ scan_wall_seconds = $null; capped = $true }
            continue
        }
        $entry.perf.$mode = [ordered]@{ scan_wall_seconds = $wall }
    }
    $scenarios[$sd.Name] = $entry
}

$out = [ordered]@{
    _comment = 'Versioned regression baseline for the tests/e2e suite. Updated ONLY by human-reviewed commits (tests/e2e/BASELINE.md). Format: scenarios.<scenario>.{recall,precision,perf.<mode>.scan_wall_seconds} as produced by engine/bench/metrics/compute_metrics.py via the e2e CLI lane. Thresholds enforced by engine/bench/metrics/regression_gate.py: recall -2%, precision -2%, scan wall +25%.'
    schema = 1
    source_run_id = $RunId
    scenarios = $scenarios
}
if ($notes.Count -gt 0) { $out.exclusions = $notes }

$json = $out | ConvertTo-Json -Depth 8
if ($OutJson) {
    $json | Set-Content -LiteralPath $OutJson -Encoding utf8
    Write-Host "baseline candidate written: $OutJson ($($scenarios.Count) scenarios)"
    foreach ($n in $notes) { Write-Host "NOTE $n" }
} else {
    $json | Write-Output
}
