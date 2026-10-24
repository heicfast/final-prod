# run-ours.ps1 - run the rrecov CLI against one scenario with full instrumentation.
#
# Per scan mode (quick | deep | deepcarve):
#   rrecov scan  (--quick-only | --no-carve | default) --volume X: [--deep for disk targets]
#                --out session.crsf --log stage.log --json        (capped poll + kill)
#   rrecov tree  --in session.crsf --csv / --json
#   rrecov recover --in session.crsf --disk N --to dest --report report.json
# Process sampling (working set / CPU) every 500 ms while scanning.
#
# Outputs into -OutDir: scan-<mode>.{stdout,stderr,json-summary}, stage-<mode>.log,
# session-<mode>.crsf, tree-<mode>.csv, tree-<mode>.json, report-<mode>.json,
# samples-<mode>.csv, recovered-<mode>/, preservation-<mode>.json, ours-run.json.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $RrecovExe,
    [Parameter(Mandatory = $true)] [string] $Letter,           # volume letter, e.g. 'E' (unused for disk mode)
    [Parameter(Mandatory = $true)] [int] $DiskNumber,          # source disk (recover --disk)
    [Parameter(Mandatory = $true)] [string] $ManifestJsonl,
    [Parameter(Mandatory = $true)] [string] $OutDir,
    [Parameter(Mandatory = $true)] [string] $Scenario,
    [ValidateSet('quick', 'deep', 'deepcarve')] [string[]] $ScanModes = @('quick', 'deep', 'deepcarve'),
    [int] $CapSec = 900,
    [int] $Threads = 4,
    [string] $CoreSha = '',
    [string] $DriverInfo = '',
    [int] $RecoverCapSec = 1800
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }
. "$PSScriptRoot\..\volume\volume.ps1"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$run = [ordered]@{
    tool = 'ours'; scenario = $Scenario; core_sha = $CoreSha
    driver = $DriverInfo; rrecov = $RrecovExe
    rrecov_sha256 = (Get-FileHash -LiteralPath $RrecovExe -Algorithm SHA256).Hash.ToLowerInvariant()
    scan_modes = @(); started_utc = (Get-Date).ToUniversalTime().ToString('o')
}

function Invoke-SampledProcess {
    param([string] $Exe, [string[]] $ArgList, [string] $StdoutPath, [string] $StderrPath,
           [string] $SamplesPath, [int] $CapSec)
    $t0 = Get-Date
    $p = Start-Process -FilePath $Exe -ArgumentList $ArgList `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath `
        -PassThru -NoNewWindow
    $samples = New-Object System.Collections.Generic.List[string]
    $samples.Add('t_ms,working_set_bytes,cpu_seconds')
    $capped = $false
    while (-not $p.HasExited) {
        Start-Sleep -Milliseconds 500
        $age = ((Get-Date) - $t0).TotalMilliseconds
        if ($age -gt ($CapSec * 1000)) {
            $capped = $true
            Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
            Start-Sleep -Seconds 2
            break
        }
        try {
            $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
            if ($proc) {
                $samples.Add(("{0:F0},{1},{2:F3}" -f $age, $proc.WorkingSet64, $proc.CPU))
            }
        } catch { }
    }
    $samples | Set-Content -Path $SamplesPath -Encoding utf8
    $wall = [math]::Round(((Get-Date) - $t0).TotalSeconds, 2)
    return @{ pid = $p.Id; exit = $p.ExitCode; wall_seconds = $wall; capped = $capped }
}

foreach ($mode in $ScanModes) {
    Write-Host "=== ours scan mode: $mode ==="
    $session = Join-Path $OutDir "session-$mode.crsf"
    $stageLog = Join-Path $OutDir "stage-$mode.log"
    $scanArgs = @('scan')
    if ($Scenario -eq 'delete_partition') {
        $scanArgs += @('--disk', "$DiskNumber", '--deep', '--threads', "$Threads")
        if ($mode -eq 'quick') { $scanArgs += '--quick-only' }
        elseif ($mode -eq 'deep') { $scanArgs += '--no-carve' }
    } else {
        $scanArgs += @('--volume', "$Letter`:", '--threads', "$Threads")
        if ($mode -eq 'quick') { $scanArgs += '--quick-only' }
        elseif ($mode -eq 'deep') { $scanArgs += '--no-carve' }
    }
    $scanArgs += @('--out', $session, '--log', $stageLog, '--json')

    $sr = Invoke-SampledProcess -Exe $RrecovExe -ArgList $scanArgs `
        -StdoutPath (Join-Path $OutDir "scan-$mode.stdout") `
        -StderrPath (Join-Path $OutDir "scan-$mode.stderr") `
        -SamplesPath (Join-Path $OutDir "samples-$mode.csv") -CapSec $CapSec

    $summary = $null
    try {
        $so = (Get-Content -LiteralPath (Join-Path $OutDir "scan-$mode.stdout") -Raw).Trim()
        $jline = ($so -split "`n" | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1)
        if ($jline) { $summary = $jline | ConvertFrom-Json }
    } catch { }

    $modeInfo = [ordered]@{
        mode = $mode; exit_code = $sr.exit; wall_seconds = $sr.wall_seconds; capped = $sr.capped
        command = "$RrecovExe $($scanArgs -join ' ')"
        session_exists = (Test-Path $session)
        summary = $summary
        transport_lines = @()
    }
    if (Test-Path $stageLog) {
        $modeInfo.transport_lines = @(Get-Content $stageLog | Select-String -SimpleMatch '[transport]' |
            ForEach-Object { $_.Line.Trim() })
    }
    if (-not $modeInfo.session_exists -and -not $sr.capped) {
        throw "scan $mode produced NO session and was not capped - refusing to continue (no silent skip)"
    }

    if ($modeInfo.session_exists) {
        & $RrecovExe tree --in $session --csv | Out-File -FilePath (Join-Path $OutDir "tree-$mode.csv") -Encoding utf8
        & $RrecovExe tree --in $session --json | Out-File -FilePath (Join-Path $OutDir "tree-$mode.json") -Encoding utf8

        # recovery of everything the session reports (dest is on the runner workspace disk,
        # never the source VHD - destination-overlap refusal stays quiet by construction)
        $dest = Join-Path $OutDir "recovered-$mode"
        New-Item -ItemType Directory -Force -Path $dest | Out-Null
        $rr = Invoke-SampledProcess -Exe $RrecovExe `
            -ArgList @('recover', '--in', $session, '--disk', "$DiskNumber", '--to', $dest, '--report', (Join-Path $OutDir "report-$mode.json")) `
            -StdoutPath (Join-Path $OutDir "recover-$mode.stdout") `
            -StderrPath (Join-Path $OutDir "recover-$mode.stderr") `
            -SamplesPath (Join-Path $OutDir "recover-samples-$mode.csv") -CapSec $RecoverCapSec
        $modeInfo.recover = @{
            exit_code = $rr.exit; wall_seconds = $rr.wall_seconds; capped = $rr.capped
            report = (Test-Path (Join-Path $OutDir "report-$mode.json"))
            dest = $dest
        }
        # transport assertion (GAP-BENCH-5): with the rda service present the scan must
        # actually have used \\.\RDA - a silent direct fallback would measure the wrong transport
        if ($DriverInfo -match 'service=rda' -and $DriverInfo -match 'running') {
            $rdaLine = ($modeInfo.transport_lines | Where-Object { $_ -match 'RDA driver path' } | Measure-Object).Count
            $modeInfo.rda_transport_assertion = @{ expected = $true; hits = $rdaLine }
            if ($rdaLine -lt 1) {
                Write-Host "TRANSPORT ASSERTION FAILED for $mode - scan never used the RDA driver path"
            }
        }
    }
    $run.scan_modes += $modeInfo
}

# ---- attribute preservation probes (best effort, recorded) ------------------------
$presFile = Join-Path $OutDir 'preservation.json'
$pres = [ordered]@{ ads = @(); hardlink = @(); sparse = @(); timestamps = @() }
$entries = @()
Get-Content -LiteralPath $ManifestJsonl | ForEach-Object {
    if (-not $_.Trim()) { return }
    $o = $_ | ConvertFrom-Json
    if (-not $o._meta) { $entries += $o }
}
$destDeep = Join-Path $OutDir 'recovered-deepcarve'
if (-not (Test-Path $destDeep)) { $destDeep = Join-Path $OutDir 'recovered-deep' }
if (Test-Path $destDeep) {
    # ADS: look for the stream on the recovered counterpart (exact path match)
    foreach ($e in ($entries | Where-Object { $_.ads -and $_.ads.Count -gt 0 })) {
        $winRel = ($e.rel -replace '^seed/', '' -replace '^keep/', '')
        $cand = Get-ChildItem -LiteralPath $destDeep -Recurse -Filter $e.name -ErrorAction SilentlyContinue |
            Where-Object { $_.Length -eq $e.size } | Select-Object -First 1
        $found = $false
        if ($cand) {
            try {
                $streams = Get-Item -LiteralPath $cand.FullName -Stream * -ErrorAction Stop |
                    Where-Object { $_.Stream -ne ':$DATA' -and $_.Stream -ne '::$DATA' }
                $names = @($streams | ForEach-Object { $_.Stream })
                $found = ($names -contains $e.ads[0].name)
            } catch { $found = $false }
        }
        $pres.ads += @{ rel = $e.rel; expected_stream = $e.ads[0].name; preserved = $found }
    }
    # sparse: check the SparseFile attribute on recovered counterparts
    foreach ($e in ($entries | Where-Object { $_.sparse })) {
        $cand = Get-ChildItem -LiteralPath $destDeep -Recurse -Filter $e.name -ErrorAction SilentlyContinue |
            Where-Object { $_.Length -eq $e.size } | Select-Object -First 1
        $isSparse = $false
        if ($cand) { $isSparse = (($cand.Attributes -band [IO.FileAttributes]::SparseFile) -ne 0) }
        $pres.sparse += @{ rel = $e.rel; preserved_as_sparse = $isSparse; size_ok = [bool]$cand }
    }
}
# hardlink: count links on recovered counterparts (group files share content)
foreach ($g in ($entries | Where-Object { $_.hardlink_group -gt 0 } | Group-Object hardlink_group)) {
    $a = $g.Group[0]; $b = $g.Group[1]
    if (-not $b) { continue }
    $la = Get-ChildItem -LiteralPath $destDeep -Recurse -Filter $a.name -ErrorAction SilentlyContinue |
        Where-Object { $_.Length -eq $a.size } | Select-Object -First 1
    $lb = Get-ChildItem -LiteralPath $destDeep -Recurse -Filter $b.name -ErrorAction SilentlyContinue |
        Where-Object { $_.Length -eq $b.size } | Select-Object -First 1
    $links = 0
    if ($la) {
        try { $links = [int][BenchRaw]::GetNumberOfLinks($la.FullName) } catch { $links = -1 }
    }
    $pres.hardlink += @{ group = $g.Name; both_recovered = [bool]($la -and $lb); links_on_first = $links }
}
$pres | ConvertTo-Json -Depth 5 | Set-Content -Path $presFile -Encoding utf8

$run.ended_utc = (Get-Date).ToUniversalTime().ToString('o')
$run | ConvertTo-Json -Depth 8 | Set-Content -Path (Join-Path $OutDir 'ours-run.json') -Encoding utf8
Write-Host "ours run complete: $($run.scan_modes.Count) mode(s)"
