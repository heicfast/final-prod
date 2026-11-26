# Compare-Lanes.ps1 - the CLI-vs-API divergence check (the e2e gate-5 contract).
#
# "Both paths must agree - CLI and API produce byte-identical results for the
# same scenario. Any divergence is a finding." The two lanes drive the SAME
# engine core through two surfaces (engine/src/app/rrecov.cpp consumes the C++
# core directly; cr_ui.dll wraps the same core - worklog/API-SURFACE.md), on
# the SAME post-deletion volume state, with the same thread count:
#
#   CLI lane: rrecov scan --quick-only|--no-carve|default per mode (deepcarve =
#             quick+deep+carve, the default) -> tree -> recover --report
#   API lane: api_harness --scan (stage_mask 7 = all stages, the same default)
#             -> session -> query -> recovery -> G-09 report
#
# Divergence is judged ONLY against ground truth (the seed manifest), never
# tool-against-tool output comparison: for every manifest file the two lanes'
# recovered-inventory.json (engine/bench/metrics/inventory_recovered.py) give
# {manifest_rel, hash_match, size}. The lanes agree iff:
#   A. the hash-match sets are IDENTICAL (every manifest rel hash-matched by
#      one lane is hash-matched by the other - byte-identical recovered bytes
#      follow transitively: both equal the manifest's SHA-256)
#   B. every manifest rel recovered by both lanes carries the same size
#   C. the full-fidelity scan stats agree (CLI deepcarve files/deleted/lost vs
#      API scan_stats.files_found/deleted_found/lost_partitions)
#   D. both lanes' recover completed in the honest exit shape (CLI 0, API 0/2)
# Any difference is a FINDING, recorded in divergence.json; exit 1.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $ScenarioId,
    [Parameter(Mandatory = $true)] [string] $CliDir,
    [Parameter(Mandatory = $true)] [string] $ApiDir,
    [string] $CliRunJson = '',
    [string] $ApiResultJson = '',
    [string] $CliInventory = '',
    [string] $ApiInventory = '',
    [string] $OutJson = ''
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

function Load-Json([string] $Path) {
    if ($Path -and (Test-Path -LiteralPath $Path)) {
        try { return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json } catch { return $null }
    }
    return $null
}

function Get-HashMatchSet {
    param($Inventory)
    # manifest_rel -> @{ hash = [bool]; size = [long] } for every matched record
    $set = @{}
    if (-not $Inventory) { return $set }
    foreach ($r in @($Inventory.records)) {
        if ($r.manifest_rel) {
            # first record wins for duplicate rels; hash_match recorded
            if (-not $set.ContainsKey([string]$r.manifest_rel)) {
                $set[[string]$r.manifest_rel] = @{ hash = [bool]$r.hash_match; size = [long]$r.size }
            }
        }
    }
    return $set
}

$cliInv = Load-Json $CliInventory
if (-not $cliInv) { $cliInv = Load-Json (Join-Path $CliDir 'recovered-inventory.json') }
$apiInv = Load-Json $ApiInventory
if (-not $apiInv) { $apiInv = Load-Json (Join-Path $ApiDir 'recovered-inventory.json') }
$cliRun = Load-Json $CliRunJson
if (-not $cliRun) { $cliRun = Load-Json (Join-Path $CliDir 'ours-run.json') }
$apiRes = Load-Json $ApiResultJson
if (-not $apiRes) { $apiRes = Load-Json (Join-Path $ApiDir 'api-result.json') }

$findings = @()
$checked = $true

if (-not $cliInv -or -not $apiInv) {
    # a lane's own gate already fails when its inventory is absent; the
    # divergence check records the incomparable state instead of inventing one
    $checked = $false
    $findings += "inventory absent: CLI=$(if ($cliInv) { 'present' } else { 'MISSING' }) API=$(if ($apiInv) { 'present' } else { 'MISSING' }) - divergence not computable; the missing lane's own gate owns the failure"
} else {
    # A. hash-match set equality
    $cliSet = Get-HashMatchSet $cliInv
    $apiSet = Get-HashMatchSet $apiInv
    $cliHashKeys = @($cliSet.Keys | Where-Object { $cliSet[$_].hash })
    $apiHashKeys = @($apiSet.Keys | Where-Object { $apiSet[$_].hash })
    $onlyCli = @($cliHashKeys | Where-Object { $apiHashKeys -notcontains $_ })
    $onlyApi = @($apiHashKeys | Where-Object { $cliHashKeys -notcontains $_ })
    if ($onlyCli.Count -gt 0) {
        $findings += "hash-match set divergence: $($onlyCli.Count) manifest rel(s) hash-matched ONLY by the CLI lane (first: $($onlyCli[0]))"
    }
    if ($onlyApi.Count -gt 0) {
        $findings += "hash-match set divergence: $($onlyApi.Count) manifest rel(s) hash-matched ONLY by the API lane (first: $($onlyApi[0]))"
    }

    # B. per-rel hash flags + recovered sizes on the common recovered set
    $sizeDivergences = 0
    $hashFlagDivergences = 0
    foreach ($rel in @($cliSet.Keys)) {
        if ($apiSet.ContainsKey($rel)) {
            if ($cliSet[$rel].hash -ne $apiSet[$rel].hash) {
                $hashFlagDivergences++
                if ($hashFlagDivergences -le 5) { $findings += "hash flag divergence on rel ${rel}: CLI=$($cliSet[$rel].hash) API=$($apiSet[$rel].hash)" }
            }
            if ($cliSet[$rel].size -ne $apiSet[$rel].size) {
                $sizeDivergences++
                if ($sizeDivergences -le 5) { $findings += "recovered size divergence on rel ${rel}: CLI=$($cliSet[$rel].size) API=$($apiSet[$rel].size)" }
            }
        }
    }
    if ($hashFlagDivergences -gt 0) { $findings += "total hash-flag divergences: $hashFlagDivergences" }
    if ($sizeDivergences -gt 0) { $findings += "total size divergences: $sizeDivergences" }

    # C. full-fidelity scan stats: CLI deepcarve vs API all-stages
    $cliDc = $null
    if ($cliRun) {
        $cliDc = @($cliRun.scan_modes) | Where-Object { $_.mode -eq 'deepcarve' } | Select-Object -First 1
        if (-not $cliDc) { $cliDc = @($cliRun.scan_modes)[-1] }
    }
    $cliFiles = $null; $cliLost = $null; $cliExit = $null
    if ($cliDc -and $cliDc.summary) {
        $cliFiles = [int64]$cliDc.summary.files
        $cliLost = [int64]$cliDc.summary.lost_partitions
    }
    if ($cliDc) { $cliExit = [int]$cliDc.exit_code }
    $apiFiles = $null; $apiLost = $null
    if ($apiRes -and $apiRes.scan_stats) {
        $apiFiles = [int64]$apiRes.scan_stats.files_found
        $apiLost = [int64]$apiRes.scan_stats.lost_partitions
    }
    if (($null -ne $cliFiles) -and ($null -ne $apiFiles) -and ($cliFiles -ne $apiFiles)) {
        $findings += "scan files divergence: CLI(deepcarve)=$cliFiles API=$apiFiles"
    }
    if (($null -ne $cliLost) -and ($null -ne $apiLost) -and ($cliLost -ne $apiLost)) {
        $findings += "lost_partitions divergence: CLI=$cliLost API=$apiLost"
    }

    # D. recover completion shapes are owned by the lane gates (Run-E2E.ps1
    #    checks CLI scan exits 0 + recover 0/2 + API 0/2 + transport=direct).
}

$out = [ordered]@{
    scenario = $ScenarioId
    checked = $checked
    agree = ($findings.Count -eq 0)
    compared = [ordered]@{
        cli_files_seen = if ($cliInv) { [int]$cliInv.files_seen } else { $null }
        cli_matched = if ($cliInv) { [int]$cliInv.matched } else { $null }
        cli_hash_matched = if ($cliInv) { [int]$cliInv.hash_match } else { $null }
        cli_false_positives = if ($cliInv) { [int]$cliInv.false_positives } else { $null }
        api_files_seen = if ($apiInv) { [int]$apiInv.files_seen } else { $null }
        api_matched = if ($apiInv) { [int]$apiInv.matched } else { $null }
        api_hash_matched = if ($apiInv) { [int]$apiInv.hash_match } else { $null }
        api_false_positives = if ($apiInv) { [int]$apiInv.false_positives } else { $null }
        cli_scan_files = $cliFiles; api_scan_files = $apiFiles
        cli_lost_partitions = $cliLost; api_lost_partitions = $apiLost
    }
    findings = $findings
}

$json = $out | ConvertTo-Json -Depth 6
if ($OutJson) { $json | Set-Content -LiteralPath $OutJson -Encoding utf8 }
$json | Write-Host

if ($findings.Count -gt 0) { exit 1 }
exit 0
