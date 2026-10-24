# run-og.ps1 - drive the og EaseUS DRW free build through one full scenario.
#
# Flow (every step screenshotted + logged + timed; any hard failure throws with
# artifacts retained - NO silent skips):
#   state reset -> launch DRW.exe -> wait landing -> select our volume (candidate
#   tiles, self-verified via DRWUI.log 'Scan drive:<letter>') -> Search for Lost Data
#   -> wait for the auto-saved .rsf (completion signal) -> Go to Recover -> Recover All
#   -> destination dialog (Local tab, Windows(C:), file-repair toggle OFF, Save)
#   -> progress / 500MB free-cap gate (Give Up) -> harvest .rsf + logs -> record
#   recovered destination roots (C:\ before/after diff) -> quota accounting.
#
# The og free cap is client-side (HKLM\SOFTWARE\DRWNewFree\Metadata + recoveryFree.ini);
# each scenario starts from a reset state (= fresh-install semantics for the quota).
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $CorpusDir,
    [Parameter(Mandatory = $true)] [string] $Letter,
    [Parameter(Mandatory = $true)] [string] $ManifestJsonl,
    [Parameter(Mandatory = $true)] [string] $OutDir,
    [Parameter(Mandatory = $true)] [string] $Scenario,
    [Parameter(Mandatory = $true)] [string] $CompareOgRsfPy,
    [int] $ScanWaitSec = 900,
    [int] $RecoverWaitSec = 1200,
    [string] $PythonExe = 'python',
    [ValidateSet('volume', 'disk')] [string] $TargetMode = 'volume',
    [string] $SecondLetter = ''
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\og-driver.ps1"
. "$PSScriptRoot\..\volume\volume.ps1"

$shotDir = Join-Path $OutDir 'screenshots'
New-Item -ItemType Directory -Force -Path $shotDir | Out-Null

$run = [ordered]@{
    tool = 'og'; scenario = $Scenario; target_mode = $TargetMode
    corpus = $CorpusDir; started_utc = (Get-Date).ToUniversalTime().ToString('o')
    attempts = @(); notes = @(); scan_phase = @{}; recover_phase = @{}
    cap_gate = @{ gate_seen = $false; action = 'not-hit' }
}

# ---- pristine state backup (once per job; corpus checkout is fresh anyway) ----------
$pristineIni = Join-Path $CorpusDir 'recoveryFree.ini.pristine'
if (-not (Test-Path $pristineIni)) {
    Copy-Item (Join-Path $CorpusDir 'recoveryFree.ini') $pristineIni -Force
}

Reset-OgState -CorpusDir $CorpusDir -PristineIni $pristineIni
$quotaBefore = Get-OgQuota
$rsfBaseline = Get-OgRsfBaseline -CorpusDir $CorpusDir
$run.notes += "rsf baseline count: $($rsfBaseline.Count) (corpus-shipped QA samples excluded from harvest)"

# ---- C:\ top-level snapshot (destination discovery by diff) -------------------------
function Get-RootSnapshot {
    $d = @(Get-ChildItem 'C:\' -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
    $f = @(Get-ChildItem 'C:\' -File -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
    return @{ dirs = $d; files = $f }
}
$preC = Get-RootSnapshot
$preC | ConvertTo-Json -Depth 3 | Set-Content -Path (Join-Path $OutDir 'c-root-before.json') -Encoding utf8

# ---- launch --------------------------------------------------------------------------
$t0 = Get-Date
$app = Start-OgApp -WorkDir $CorpusDir
if (-not (Wait-OgLanding -TimeoutSec 180 -ShotDir $shotDir)) {
    Get-OgWindowCensus | Set-Content -Path (Join-Path $OutDir 'window-census-fail.txt') -Encoding utf8
    Get-OgLogs -CorpusDir $CorpusDir -OutDir $OutDir | Out-Null
    throw 'og landing never appeared (180 s) - diagnostic artifacts retained'
}
$run.launch_seconds = [math]::Round(((Get-Date) - $t0).TotalSeconds, 1)

# ---- volume selection with self-verification ------------------------------------------
# Landing layout (VLM-verified against run-40048192077 screenshots, 1024x768):
# 'Internal drives' cards, 2 per row - Windows(C:) row1-left (~440,295),
# Temporary Storage(D:) row1-right (~730,275), BENCH(E:) row2-left (~440,455).
# Verification: the og appends 'Scan drive:<letter>' lines to DRWUI.log - we
# snapshot the log length at launch and only trust lines appended after it
# (the corpus checkout carries the vendor's own QA lines with stale stamps).
$logPath = Join-Path $CorpusDir 'log\DRWUI.log'
$script:LogOffset = 0
if (Test-Path $logPath) { $script:LogOffset = (Get-Item $logPath).Length }
function Get-ScanDriveLine {
    if (-not (Test-Path $logPath)) { return @() }
    $out = @()
    try {
        $fs = [System.IO.File]::Open($logPath, 'Open', 'Read', 'ReadWrite')
        try {
            if ($fs.Length -gt $script:LogOffset) {
                $fs.Seek($script:LogOffset, 'Begin') | Out-Null
                $sr = New-Object System.IO.StreamReader($fs)
                $text = $sr.ReadToEnd()
                $out = @($text -split "`r?`n" | Where-Object { $_ -match 'Scan drive:' })
            }
        } finally { $fs.Close() }
    } catch { }
    return ,$out
}

$tiles = @(
    @{ x = 440; y = 455 },   # BENCH(E:) - row2-left (VLM-verified primary)
    @{ x = 440; y = 295 },   # Windows(C:) row1-left (fallback ordering)
    @{ x = 730; y = 275 },   # Temporary Storage(D:) row1-right
    @{ x = 400; y = 266 }, @{ x = 860; y = 266 },
    @{ x = 400; y = 366 }, @{ x = 860; y = 366 },
    @{ x = 860; y = 466 }
)
if ($TargetMode -eq 'disk') {
    # whole-disk / lost-partition candidates: landing tile area then the
    # 'Lost Partition Rescue' sidebar page (atlas: sidebar ~ (135,374))
    $tiles = @(
        @{ x = 440; y = 455 }, @{ x = 440; y = 560 },
        @{ x = 440; y = 620 }, @{ x = 860; y = 560 }
    )
}

$selected = $false
$selIdx = 0
foreach ($tile in $tiles) {
    $selIdx++
    Invoke-OgGateDismiss
    New-OgShot ("02_select_attempt_{0:d2}" -f $selIdx) $shotDir | Out-Null
    Invoke-OgClickRel $tile.x $tile.y ("tile candidate $selIdx") | Out-Null
    Start-Sleep -Seconds 2
    Invoke-OgClickRel 897 678 'Search for Lost Data CTA' | Out-Null
    # wait for the scan to start (log line) or not
    $tSel = Get-Date
    $line = $null
    while (((Get-Date) - $tSel).TotalSeconds -lt 45) {
        $lines = Get-ScanDriveLine
        if ($lines.Count -gt 0) { $line = $lines[-1]; break }
        Start-Sleep -Seconds 3
    }
    $run.attempts += @{ attempt = $selIdx; tile = $tile; scan_line = $line }
    if ($TargetMode -eq 'volume' -and $line -and ($line -match "Scan drive:$Letter(\W|$)")) {
        $selected = $true
        $run.notes += "volume selection verified by fresh log line: $line"
        break
    }
    if ($TargetMode -eq 'disk' -and $line) {
        # a whole-disk scan logs a non-system drive line; the .rsf found-set
        # then proves what was actually scanned
        if ($line -notmatch 'Scan drive:C(\W|$)' -and $line -notmatch 'Scan drive:D(\W|$)') {
            $selected = $true
            $run.notes += "disk-mode selection: fresh non-system log line: $line"
            break
        }
    }
    # wrong tile: stop the scan, return HOME, dismiss gates before next attempt
    Invoke-OgClickRel 127 698 'Stop' | Out-Null
    Start-Sleep -Seconds 3
    New-OgShot ("03_stop_confirm_{0:d2}" -f $selIdx) $shotDir | Out-Null
    Invoke-OgClickRel 512 420 'stop confirm' | Out-Null
    Start-Sleep -Seconds 5
    Invoke-OgClickRel 135 120 'sidebar home' | Out-Null
    Start-Sleep -Seconds 3
    Invoke-OgGateDismiss
}
if (-not $selected -and $TargetMode -eq 'disk') {
    # last resort for delete_partition: the Lost Partition Rescue sidebar page
    Invoke-OgGateDismiss
    Invoke-OgClickRel 135 374 'sidebar Lost Partition Rescue' | Out-Null
    Start-Sleep -Seconds 5
    New-OgShot '02b_lost_partition_page' $shotDir | Out-Null
    Invoke-OgClickRel 440 295 'lost-partition page: first disk row' | Out-Null
    Start-Sleep -Seconds 2
    Invoke-OgClickRel 897 678 'Search CTA (lost-partition page)' | Out-Null
    $tLp = Get-Date
    while (((Get-Date) - $tLp).TotalSeconds -lt 60) {
        $lines = Get-ScanDriveLine
        if ($lines.Count -gt 0) {
            $run.notes += "lost-partition rescue flow produced: $($lines[-1])"
            $selected = $true
            break
        }
        Start-Sleep -Seconds 5
    }
}
if (-not $selected) {
    New-OgShot '99_selection_failed' $shotDir | Out-Null
    Get-OgWindowCensus | Set-Content -Path (Join-Path $OutDir 'window-census-fail.txt') -Encoding utf8
    Get-OgLogs -CorpusDir $CorpusDir -OutDir $OutDir | Out-Null
    throw "og volume selection failed after $($tiles.Count) tile candidates - diagnostic artifacts retained"
}
New-OgShot '04_scan_started' $shotDir | Out-Null

# ---- scan phase --------------------------------------------------------------------------
$tScan = Get-Date
$rsfResult = Wait-OgRsf -Baseline $rsfBaseline -CorpusDir $CorpusDir -TimeoutSec $ScanWaitSec `
    -ShotDir $shotDir -SamplesCsv (Join-Path $OutDir 'og-samples.csv')
$run.scan_phase = @{
    wall_seconds = [math]::Round(((Get-Date) - $tScan).TotalSeconds, 1)
    timeout = ($null -eq $rsfResult)
    scan_drive_lines = @(Get-ScanDriveLine)
}
if ($null -eq $rsfResult) {
    New-OgShot '99_scan_timeout' $shotDir | Out-Null
    Get-OgLogs -CorpusDir $CorpusDir -OutDir $OutDir | Out-Null
    Get-OgWindowCensus | Set-Content -Path (Join-Path $OutDir 'window-census-fail.txt') -Encoding utf8
    Stop-OgApp
    throw "og scan did not complete within $ScanWaitSec s (no new .rsf) - diagnostic artifacts retained"
}
$run.scan_phase.rsf = $rsfResult.rsf
$run.scan_phase.rsf_seconds = $rsfResult.seconds

# ---- results + Go to Recover ----------------------------------------------------------------
$w = 1024; $h = 735
$r = Get-OgMainRect
if ($r) { $w = $r.R - $r.L; $h = $r.B - $r.T }
Invoke-OgClickRel ([int]($w * 0.4824)) ([int]($h * 0.6569)) 'Go to Recover (completion modal CTA)' | Out-Null
Start-Sleep -Seconds 6
Invoke-OgGateDismiss
New-OgShot '11_results_baseline' $shotDir | Out-Null

# ---- FlaUiProbe (auxiliary UIA evidence; failure is recorded, not masked) ------------------
$fla = @{ attempted = $false; ok = $false; reason = '' }
$probeDir = Join-Path $OutDir 'uia'
$probeProj = Join-Path $PSScriptRoot 'FlaUiProbe'
try {
    $fla.attempted = $true
    & dotnet run --project $probeProj -- (Join-Path $probeDir 'uia-tree.json') 2> (Join-Path $OutDir 'flauiprobe-stderr.txt')
    if ($LASTEXITCODE -eq 0 -and (Test-Path (Join-Path $probeDir 'uia-tree.json'))) { $fla.ok = $true }
    else { $fla.reason = "dotnet exit=$LASTEXITCODE (UIA dump incomplete or FlaUI unavailable)" }
} catch {
    $fla.reason = "dotnet/FlaUI unavailable: $($_.Exception.Message)"
}
$run.flauiprobe = $fla
if (-not $fla.ok) {
    $run.notes += "FlaUiProbe (auxiliary evidence only) failed: $($fla.reason) - primary results come from the coordinate driver + .rsf + destination diff"
}

# ---- Recover All ---------------------------------------------------------------------------
$censusBefore = Get-OgWindowCensus
$dialogSeen = $false
foreach ($cta in @(@(916, 684), @(892, 668))) {
    Invoke-OgGateDismiss
    Invoke-OgClickRel $cta[0] $cta[1] 'Recover All CTA' | Out-Null
    Start-Sleep -Seconds 5
    New-OgShot '20_recover_cta_clicked' $shotDir | Out-Null
    $censusAfter = Get-OgWindowCensus
    if ($censusAfter.Count -gt $censusBefore.Count) { $dialogSeen = $true; break }
    # also accept if a screenshot changed materially - census count is primary
}
if (-not $dialogSeen) {
    # last resort: the select-all checkbox + contextual Recover
    Invoke-OgClickRel 40 140 'select-all checkbox' | Out-Null
    Start-Sleep -Seconds 2
    Invoke-OgClickRel 892 668 'contextual Recover' | Out-Null
    Start-Sleep -Seconds 5
    New-OgShot '20b_recover_via_selectall' $shotDir | Out-Null
}
Start-Sleep -Seconds 3
New-OgShot '21_destination_dialog' $shotDir | Out-Null

# ---- destination dialog: Local tab, Windows(C:), repair toggle OFF, Save -------------------
Invoke-OgClickRel 410 222 'destination: Local tab' | Out-Null
Start-Sleep -Seconds 2
Invoke-OgClickRel 511 279 'destination: Windows(C:) row' | Out-Null
Start-Sleep -Seconds 2
Invoke-OgClickRel 255 613 'destination: file-repair toggle OFF' | Out-Null
Start-Sleep -Seconds 1
New-OgShot '22_destination_configured' $shotDir | Out-Null
Invoke-OgClickRel 707 613 'destination: Save' | Out-Null

# ---- recovery progress + free-cap gate ------------------------------------------------------
$tRec = Get-Date
$capGateSeen = $false
$recoverDone = $false
$lastFiles = -1
$i = 0
while (((Get-Date) - $tRec).TotalSeconds -lt $RecoverWaitSec) {
    Start-Sleep -Seconds 10
    Invoke-OgGateDismiss
    $post = Get-RootSnapshot
    $newDirs = @($post.dirs | Where-Object { $preC.dirs -notcontains $_ })
    $fileCount = 0
    foreach ($d in $newDirs) {
        $fileCount += @(Get-ChildItem (Join-Path 'C:\' $d) -Recurse -File -ErrorAction SilentlyContinue).Count
    }
    if ($fileCount -eq $lastFiles -and $fileCount -gt 0) {
        # quiescent for one full poll interval - take one more look then finish
        Start-Sleep -Seconds 10
        $post2 = Get-RootSnapshot
        $fc2 = 0
        foreach ($d in ($post2.dirs | Where-Object { $preC.dirs -notcontains $_ })) {
            $fc2 += @(Get-ChildItem (Join-Path 'C:\' $d) -Recurse -File -ErrorAction SilentlyContinue).Count
        }
        if ($fc2 -eq $fileCount) { $recoverDone = $true; break }
    }
    $lastFiles = $fileCount
    # cap-gate detection: a new modal window appears mid-recovery (CRecoverLimitDlg)
    $census = Get-OgWindowCensus
    if (-not $capGateSeen -and $census.Count -gt 2) {
        New-OgShot '30_cap_gate_candidate' $shotDir | Out-Null
        $capGateSeen = $true
        $run.cap_gate.gate_seen = $true
        $run.cap_gate.action = 'give-up-clicked'
        $run.cap_gate.screenshot = '30_cap_gate_candidate.png'
        # 'Give Up' = the left button of the gate pair (atlas: buttons Give Up | Recover Remaining)
        Invoke-OgClickRel 432 470 'cap gate: Give Up (left button)' | Out-Null
        Start-Sleep -Seconds 5
        New-OgShot '31_cap_gate_after' $shotDir | Out-Null
    }
    if (($i++ % 6) -eq 0) { New-OgShot ("25_recover_progress_{0:d2}" -f $i) $shotDir | Out-Null }
}
$run.recover_phase = @{
    wall_seconds = [math]::Round(((Get-Date) - $tRec).TotalSeconds, 1)
    completed = $recoverDone
}
New-OgShot '26_recover_end' $shotDir | Out-Null

# ---- harvest ----------------------------------------------------------------------------------
$rsfPath = $rsfResult.rsf
Copy-Item $rsfPath (Join-Path $OutDir (Split-Path $rsfPath -Leaf)) -Force
& $PythonExe $CompareOgRsfPy (Join-Path $OutDir (Split-Path $rsfPath -Leaf)) (Join-Path $OutDir 'og-found.csv')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $OutDir 'og-found.csv'))) {
    Get-OgLogs -CorpusDir $CorpusDir -OutDir $OutDir | Out-Null
    throw "compare_og_rsf.py failed on the harvested .rsf (exit=$LASTEXITCODE) - artifacts retained"
}
$run.notes += "og found-set extracted from $(Split-Path $rsfPath -Leaf) via compare_og_rsf.py"

# ---- destination discovery by diff -------------------------------------------------------------
$postC = Get-RootSnapshot
$postC | ConvertTo-Json -Depth 3 | Set-Content -Path (Join-Path $OutDir 'c-root-after.json') -Encoding utf8
$newDirs = @($postC.dirs | Where-Object { $preC.dirs -notcontains $_ })
$newFiles = @($postC.files | Where-Object { $preC.files -notcontains $_ })
$rootFilesDir = Join-Path $OutDir 'recovered-root-files'
if ($newFiles.Count -gt 0) {
    New-Item -ItemType Directory -Force -Path $rootFilesDir | Out-Null
    foreach ($nf in $newFiles) {
        Copy-Item (Join-Path 'C:\' $nf) $rootFilesDir -Force -ErrorAction SilentlyContinue
    }
}
$roots = @($newDirs | ForEach-Object { Join-Path 'C:\' $_ })
if ($newFiles.Count -gt 0) { $roots += $rootFilesDir }
$run.recovered_roots = $roots
$run.notes += "recovered destination discovered by C:\ diff: $($newDirs -join ', ') $(if ($newFiles.Count) { '+ root files' })"

# ---- preservation probes -------------------------------------------------------------------------
$pres = [ordered]@{ ads = @(); hardlink = @(); sparse = @() }
$entries = @()
Get-Content -LiteralPath $ManifestJsonl | ForEach-Object {
    if (-not $_.Trim()) { return }
    $o = $_ | ConvertFrom-Json
    if (-not $o._meta) { $entries += $o }
}
$allRecovered = @()
foreach ($root in $roots) {
    $allRecovered += @(Get-ChildItem $root -Recurse -File -ErrorAction SilentlyContinue)
}
foreach ($e in ($entries | Where-Object { $_.ads -and $_.ads.Count -gt 0 })) {
    $cand = $allRecovered | Where-Object { $_.Name -eq $e.name -and $_.Length -eq $e.size } | Select-Object -First 1
    $found = $false
    if ($cand) {
        try {
            $streams = Get-Item -LiteralPath $cand.FullName -Stream * -ErrorAction Stop |
                Where-Object { $_.Stream -ne ':$DATA' -and $_.Stream -ne '::$DATA' }
            $found = [bool](@($streams) | Where-Object { $_.Stream -eq $e.ads[0].name })
        } catch { $found = $false }
    }
    $pres.ads += @{ rel = $e.rel; expected_stream = $e.ads[0].name; preserved = $found }
}
foreach ($e in ($entries | Where-Object { $_.sparse })) {
    $cand = $allRecovered | Where-Object { $_.Name -eq $e.name -and $_.Length -eq $e.size } | Select-Object -First 1
    $isSparse = $false
    if ($cand) { $isSparse = (($cand.Attributes -band [IO.FileAttributes]::SparseFile) -ne 0) }
    $pres.sparse += @{ rel = $e.rel; preserved_as_sparse = $isSparse }
}
foreach ($g in ($entries | Where-Object { $_.hardlink_group -gt 0 } | Group-Object hardlink_group)) {
    $a = $g.Group[0]; $b = $g.Group[1]
    if (-not $b) { continue }
    $la = $allRecovered | Where-Object { $_.Name -eq $a.name -and $_.Length -eq $a.size } | Select-Object -First 1
    $lb = $allRecovered | Where-Object { $_.Name -eq $b.name -and $_.Length -eq $b.size } | Select-Object -First 1
    $links = 0
    if ($la) { try { $links = [int][BenchRaw]::GetNumberOfLinks($la.FullName) } catch { $links = -1 } }
    $pres.hardlink += @{ group = $g.Name; both_recovered = [bool]($la -and $lb); links_on_first = $links }
}
$pres | ConvertTo-Json -Depth 5 | Set-Content -Path (Join-Path $OutDir 'preservation.json') -Encoding utf8

# ---- quota accounting ----------------------------------------------------------------------------
$quotaAfter = Get-OgQuota
$quota = [ordered]@{}
foreach ($k in @('enable_total_size', 'enable_total_file_count')) {
    $quota[$k] = $quotaAfter[$k]
}
$quota.recovered_total_size_before = $quotaBefore.recovered_total_size_before
$quota.recovered_total_size_after = $quotaAfter.recovered_total_size_before
$quota.recovered_total_file_count_after = $quotaAfter.recovered_total_file_count_before
$quota | ConvertTo-Json -Depth 3 | Set-Content -Path (Join-Path $OutDir 'og-quota.json') -Encoding utf8

# ---- logs + census + close -------------------------------------------------------------------------
Get-OgLogs -CorpusDir $CorpusDir -OutDir $OutDir | Out-Null
Get-OgWindowCensus | Set-Content -Path (Join-Path $OutDir 'window-census-final.txt') -Encoding utf8
Stop-OgApp

$run.ended_utc = (Get-Date).ToUniversalTime().ToString('o')
$run | ConvertTo-Json -Depth 8 | Set-Content -Path (Join-Path $OutDir 'og-run.json') -Encoding utf8
Write-Host "og run complete: scenario=$Scenario roots=$($roots -join '; ')"
if (-not $recoverDone) {
    Write-Host "NOTE: recovery progress did not quiesce within $RecoverWaitSec s (capped-state evidence retained)"
}
