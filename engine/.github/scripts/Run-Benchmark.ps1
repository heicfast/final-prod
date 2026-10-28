# Run-Benchmark.ps1 - the test-benchmark orchestrator.
#
# Executes the versioned scenario matrix (.github/scenarios/benchmark-matrix.json)
# against a staged core_recovery release, through BOTH lanes:
#   CLI  - bench/ours/run-ours.ps1 (rrecov scan/tree/recover per mode, process
#          sampling, transport assertion, preservation probes)
#   API  - .github/scripts/api_harness.exe (the cr_ui C-ABI lifecycle: open ->
#          scan(+events) -> save/reload session -> query -> preview ->
#          recovery -> G-09 report) - the strategic product surface, the one
#          the future UI binds to
#
# Phases:
#   0. simulated environment: New-TestDisk.ps1 builds the matrix disks
#      (fixed VHDX, MBR+GPT, FAT32/NTFS/exFAT, multi-partition) + seeds every
#      volume with bench/seed/generate_seed.ps1 (23 real-magic file formats,
#      CJK names, long paths, ADS, hard links, sparse, fragmentation, nested
#      trees, many-small, keep-control group - the deterministic corpus)
#   1. feature probes: health, fix dry-run, user-signs registration,
#      CLI-vs-API carve-registry parity, the cancel+resume session probe
#   2. clean-volume benchmark: scan timings per mode + the driver bench verb
#      (info/seq/rand/open) on the benchmark disk - both lanes timed
#   3. the scenario loop: reseed (deterministic) -> apply-scenario (the
#      deletion) -> CLI lane -> API lane -> inventory+metrics -> gate ->
#      post-actions (the lost-partition APPLY probe) -> cleanup
#   4. the verdict: gate summary (06-comparison) + perf summary
#      (07-performance); exit 1 iff any HARD gate failed
#
# Disciplines baked in (each from a recorded failure):
#   - hashtable splats for every named-parameter invocation (M-069: the
#     array-splat positional binding killed a whole lane)
#   - ground truth is ONLY the seed manifest + the deletion method - never
#     a tool's own output; extra finds are recorded, never failures
#   - $ErrorActionPreference = 'Continue' inside the scenario loop: collect
#     every result, verdict at the END (a failed scenario never hides the
#     ones after it)
#   - per-run time caps (ScanCapSec/RecoverCapSec) with force-kill + the
#     capped flag recorded - nothing may hang (run-ours' sampler owns this)
#   - recovered payloads are deleted after inventory (byte samples kept);
#     the evidence artifact carries inventories/reports/logs, not gigabytes
#   - every step lands in the structured evidence tree
#
# Licensed under the MIT license. See LICENSE.
param(
    [Parameter(Mandatory = $true)] [string] $MatrixPath,
    [Parameter(Mandatory = $true)] [string] $RepoRoot,        # the checkout (bench/, tools/)
    [Parameter(Mandatory = $true)] [string] $CoreStaged,      # staged release (bin/rrecov.exe + bin/cr_ui.dll)
    [Parameter(Mandatory = $true)] [string] $ApiHarness,      # compiled api_harness.exe
    [Parameter(Mandatory = $true)] [string] $WorkRoot,        # <drive>:\cr-bench (disks\, work\)
    [Parameter(Mandatory = $true)] [string] $EvidenceRoot,    # the structured evidence tree
    [Parameter(Mandatory = $true)] [ValidateSet('smoke', 'full')] [string] $Profile,
    [string[]] $ScanModes = @('quick', 'deep', 'deepcarve'),
    [int] $ScanCapSec = 600,
    [int] $RecoverCapSec = 900,
    [int] $Threads = 4
)

# 'Continue' DELIBERATELY (the collect-every-result, verdict-at-the-end
# discipline): the child bench scripts carry their own EAP=Stop and still
# throw; native invocations whose stderr is captured (2>&1) must NOT throw
# at the redirect under Stop (the pwsh trap class) - every phase checks
# $LASTEXITCODE explicitly instead.
$ErrorActionPreference = 'Continue'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

# ---- fixed layout -----------------------------------------------------------------
$RrecovExe = Join-Path $CoreStaged 'bin\rrecov.exe'
$SeedScript = Join-Path $RepoRoot 'bench\seed\generate_seed.ps1'
$ScenarioScript = Join-Path $RepoRoot 'bench\scenarios\apply-scenario.ps1'
$RunOursScript = Join-Path $RepoRoot 'bench\ours\run-ours.ps1'
$NewDiskScript = Join-Path $RepoRoot '.github\scripts\New-TestDisk.ps1'
$InvScript = Join-Path $RepoRoot 'bench\metrics\inventory_recovered.py'
$MetricsScript = Join-Path $RepoRoot 'bench\metrics\compute_metrics.py'
$WorkDir = Join-Path $WorkRoot 'work'
$DiskStatePath = Join-Path $WorkRoot 'disk-state.json'
$SummariesDir = Join-Path $EvidenceRoot '06-comparison'
$PerfDir = Join-Path $EvidenceRoot '07-performance'

foreach ($d in @($WorkDir, $SummariesDir, $PerfDir,
        (Join-Path $EvidenceRoot '00-environment'), (Join-Path $EvidenceRoot '01-driver'),
        (Join-Path $EvidenceRoot '02-disk-setup'), (Join-Path $EvidenceRoot '03-baseline'),
        (Join-Path $EvidenceRoot '04-scenarios'), (Join-Path $EvidenceRoot '05-feature-probes'))) {
    New-Item -ItemType Directory -Force -Path $d | Out-Null
}

$script:HardFailures = @()      # gate failures + phase-fatal errors
$script:SoftRecords = @()       # observe-only / informational records

function Fail($msg) { Write-Host "::error::$msg" -ForegroundColor Red; $script:HardFailures += $msg }
function Note($msg) { Write-Host "[note] $msg" -ForegroundColor Yellow; $script:SoftRecords += $msg }

function Copy-Evidence {
    param([string[]] $Items, [string] $DestDir)
    New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
    foreach ($f in $Items) {
        if (Test-Path -LiteralPath $f) {
            Copy-Item -LiteralPath $f -Destination $DestDir -Force -Recurse -ErrorAction SilentlyContinue
        }
    }
}

# resolve a matrix volume -> @{ disk=state-entry; vol=volume-entry }
function Resolve-Volume {
    param([string] $DiskId, [string] $VolumeLabel, $DiskState)
    $disk = $DiskState.disks | Where-Object { $_.id -eq $DiskId }
    if (-not $disk) { throw "matrix references unknown disk '$DiskId'" }
    $vol = $disk.volumes | Where-Object { $_.label -eq $VolumeLabel }
    if (-not $vol) { throw "disk '$DiskId' has no volume labeled '$VolumeLabel'" }
    return @{ disk = $disk; vol = $vol }
}

# ---- phase 0: the simulated environment --------------------------------------------

Write-Host '=== phase 0: simulated disk environment ===' -ForegroundColor Cyan
if (-not (Test-Path $DiskStatePath)) {
    $p = @{ MatrixPath = $MatrixPath; WorkRoot = $WorkRoot; OutJson = $DiskStatePath }
    & $NewDiskScript @p
    if ($LASTEXITCODE -ne 0) { throw 'New-TestDisk failed' }
}
$DiskState = Get-Content -LiteralPath $DiskStatePath -Raw | ConvertFrom-Json
Copy-Item $DiskStatePath (Join-Path $EvidenceRoot '02-disk-setup\disk-state.json') -Force

$matrix = Get-Content -LiteralPath $MatrixPath -Raw | ConvertFrom-Json

# seed every volume; per-volume seed profile (follow-run-profile resolves here)
$script:ManifestPath = @{}   # volume label -> manifest path (the per-run truth)
$script:FirstSeeded = @{}    # volume label -> $true once seeded this run
function Invoke-Seed {
    param($VolEntry, [string] $SeedProfile)
    $letter = [string]$VolEntry.letter
    $root = "$letter`:\seed"
    $manifest = Join-Path $WorkDir "seed-manifest-$($VolEntry.label).jsonl"
    $p = @{
        Root = $root; ManifestOut = $manifest; Profile = $SeedProfile
        FileSystem = $VolEntry.fs
    }
    & $SeedScript @p
    $script:ManifestPath[$VolEntry.label] = $manifest
    $script:FirstSeeded[$VolEntry.label] = $true
    Copy-Item $manifest (Join-Path $EvidenceRoot "03-baseline\seed-manifest-$($VolEntry.label).jsonl") -Force
    # human-readable pre-deletion baseline listing
    Get-ChildItem -LiteralPath "$letter`:\" -Recurse -File -Force -ErrorAction SilentlyContinue |
        Select-Object FullName, Length, LastWriteTime |
        ConvertTo-Json -Depth 2 | Set-Content (Join-Path $EvidenceRoot "03-baseline\pre-deletion-listing-$($VolEntry.label).json") -Encoding utf8
    $count = @(Get-ChildItem -LiteralPath "$letter`:\" -Recurse -File -Force -ErrorAction SilentlyContinue).Count
    Write-Host ("  seeded {0}: {1} ({2} files)" -f $VolEntry.label, $SeedProfile, $count)
}

foreach ($d in $DiskState.disks) {
    $matrixDisk = $matrix.disks | Where-Object { $_.id -eq $d.id }
    foreach ($v in $d.volumes) {
        $mv = $matrixDisk.volumes | Where-Object { $_.label -eq $v.label }
        $profile = [string]$mv.seed_profile
        if ($profile -eq 'follow-run-profile') { $profile = $Profile }
        Invoke-Seed $v $profile
    }
}

# ---- phase 1: feature probes ---------------------------------------------------------

Write-Host '=== phase 1: feature probes ===' -ForegroundColor Cyan
$probeDir = Join-Path $WorkDir 'probes'
$probeEv = Join-Path $EvidenceRoot '05-feature-probes'
New-Item -ItemType Directory -Force -Path $probeDir, $probeEv | Out-Null

# health (driver-independent classification (b) surface): exit 0 or 3 both honest
$multiDisk = ($DiskState.disks | Where-Object { $_.id -eq 'multi-gpt' }).disk_number
$healthOut = & $RrecovExe health --disk $multiDisk 2>&1
$healthExit = $LASTEXITCODE
$healthOut | Set-Content (Join-Path $probeEv 'health.txt') -Encoding utf8
if ($healthExit -eq 0 -or $healthExit -eq 3) {
    Write-Host "PASS health --disk ${multiDisk}: exit $healthExit (0 = sources available, 3 = every source unavailable - both honest)"
} else {
    Fail "health --disk ${multiDisk} exited $healthExit (expected 0 or 3)"
}

# fix dry-run on an undamaged disk: REFUSED/clean is exit 0
$fixOut = & $RrecovExe fix --disk $multiDisk 2>&1
$fixExit = $LASTEXITCODE
$fixOut | Set-Content (Join-Path $probeEv 'fix-dryrun.txt') -Encoding utf8
if ($fixExit -eq 0) {
    Write-Host 'PASS fix (dry-run): exit 0 (REFUSED/clean-volume is the expected verdict on an undamaged disk)'
} else {
    Fail "fix --disk ${multiDisk} dry-run exited $fixExit (expected 0)"
}

# user-signs registration (the M-048/M-065 API surface, shipped defaults)
$userSignsJson = Join-Path $CoreStaged 'bin\userdata-defaults.json'
if (Test-Path $userSignsJson) {
    $usOut = & $ApiHarness --user-signs $userSignsJson 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASS user-signs (API): $usOut"
        $usOut | Set-Content (Join-Path $probeEv 'user-signs.txt') -Encoding utf8
    } else {
        Fail "api_harness --user-signs failed: $usOut"
    }
} else {
    Note 'userdata-defaults.json absent from the staged release - user-signs probe skipped (recorded, not silent)'
}

# CLI-vs-API carve-registry parity (the same registry must project identically)
$cliCarve = & $RrecovExe list-carve-formats 2>&1
$cliCount = $null
if ($cliCarve -match '(\d+) carve format') { $cliCount = [int]$Matches[1] }
$apiCarve = & $ApiHarness --carve-count 2>&1
$apiCount = $null
if ($apiCarve -match '"carve_formats":(\d+)') { $apiCount = [int]$Matches[1] }
"CLI: $cliCount`nAPI: $apiCount" | Set-Content (Join-Path $probeEv 'carve-parity.txt') -Encoding utf8
if ($null -ne $cliCount -and $null -ne $apiCount -and $cliCount -gt 0 -and $cliCount -eq $apiCount) {
    Write-Host "PASS carve-registry parity: CLI=$cliCount API=$apiCount"
} else {
    Fail "carve-registry parity failed: CLI=$cliCount API=$apiCount"
}

# the cancel + resume session probe (the exit-3/--resume contract via the API)
$cancelCfg = $matrix.cancel_resume_probe
if ($cancelCfg) {
    $rv = Resolve-Volume -DiskId $cancelCfg.disk -VolumeLabel $cancelCfg.volume -DiskState $DiskState
    $cancelLetter = [string]$rv.vol.letter
    $cancelDir = Join-Path $probeDir 'cancel-resume'
    New-Item -ItemType Directory -Force -Path $cancelDir | Out-Null
    & $ApiHarness --scan --target volume --letter $cancelLetter `
        --dest (Join-Path $cancelDir 'recovered') --out $cancelDir `
        --cancel-after ([int]$cancelCfg.cancel_after_seconds) --label 'cancel-probe' 2>&1 |
        Tee-Object -FilePath (Join-Path $cancelDir 'cancel.stdout') | Write-Host
    $cancelExit = $LASTEXITCODE
    $cancelResultPath = Join-Path $cancelDir 'api-result.json'
    $wasCancelled = $false
    if (Test-Path $cancelResultPath) {
        $cr = Get-Content $cancelResultPath -Raw | ConvertFrom-Json
        $wasCancelled = [bool]$cr.cancelled
    }
    $resumeExit = 1
    $resumeResultPath = Join-Path $cancelDir 'resume-api-result.json'
    if ($wasCancelled) {
        # rename the first result so --resume's api-result.json does not clobber it
        Move-Item $cancelResultPath (Join-Path $cancelDir 'cancel-api-result.json') -Force
        & $ApiHarness --resume --session (Join-Path $cancelDir 'session.crsf') `
            --target volume --letter $cancelLetter `
            --dest (Join-Path $cancelDir 'recovered-resumed') --out $cancelDir 2>&1 |
            Tee-Object -FilePath (Join-Path $cancelDir 'resume.stdout') | Write-Host
        $resumeExit = $LASTEXITCODE
    }
    Copy-Evidence @(
        (Join-Path $cancelDir 'cancel.stdout'), (Join-Path $cancelDir 'resume.stdout'),
        (Join-Path $cancelDir 'cancel-api-result.json'), (Join-Path $cancelDir 'api-result.json'),
        (Join-Path $cancelDir 'session.crsf'), (Join-Path $cancelDir 'report.json')
    ) (Join-Path $probeEv 'cancel-resume')
    $resumedFiles = 0
    if (Test-Path (Join-Path $cancelDir 'api-result.json')) {
        $rr = Get-Content (Join-Path $cancelDir 'api-result.json') -Raw | ConvertFrom-Json
        $resumedFiles = [int64]$rr.scan_stats.files_found
    }
    if ($cancelExit -eq 0 -and $wasCancelled -and $resumeExit -eq 0 -and $resumedFiles -gt 0) {
        Write-Host "PASS cancel+resume: cancelled mid-scan, session saved, resumed to completion ($resumedFiles files)"
    } else {
        Fail "cancel+resume probe: cancelExit=$cancelExit cancelled=$wasCancelled resumeExit=$resumeExit resumedFiles=$resumedFiles"
    }
}

# ---- phase 2: clean-volume benchmark -------------------------------------------------

Write-Host '=== phase 2: clean-volume benchmark ===' -ForegroundColor Cyan
$benchRv = Resolve-Volume -DiskId $matrix.benchmark_disk -VolumeLabel $matrix.benchmark_volume -DiskState $DiskState
$benchDisk = [int]$benchRv.disk.disk_number
$benchLetter = [string]$benchRv.vol.letter
$benchDir = Join-Path $WorkDir 'clean-benchmark'
New-Item -ItemType Directory -Force -Path $benchDir | Out-Null

$cleanPerf = [ordered]@{ captured_utc = (Get-Date).ToUniversalTime().ToString('o'); scans = @(); bench_verb = @(); api = $null }

# CLI scan timings per mode on the intact (pre-deletion) volume
foreach ($mode in $ScanModes) {
    $session = Join-Path $benchDir "session-$mode.crsf"
    $scanArgs = @('scan', '--volume', "$benchLetter`:", '--threads', "$Threads")
    if ($mode -eq 'quick') { $scanArgs += '--quick-only' }
    elseif ($mode -eq 'deep') { $scanArgs += '--no-carve' }
    $scanArgs += @('--out', $session, '--log', (Join-Path $benchDir "stage-$mode.log"), '--json')
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $out = & $RrecovExe @scanArgs 2>&1
    $sw.Stop()
    $summary = $null
    $jline = ($out | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1)
    if ($jline) { try { $summary = $jline | ConvertFrom-Json } catch { } }
    $cleanPerf.scans += [ordered]@{
        mode = $mode; wall_ms = $sw.ElapsedMilliseconds; exit = $LASTEXITCODE
        files = if ($summary) { $summary.files } else { $null }
        bytes_scanned = if ($summary) { $summary.bytes_scanned } else { $null }
        elapsed_ms = if ($summary) { $summary.elapsed_ms } else { $null }
        # OQ-S4-3's TTFR half: the CLI's event-stream probe logs ONE line at
        # the first positive FilesFound event (run-elapsed ms,
        # pause-compensated; the honest granularity - counters coalesce at
        # stage boundaries). Extracted per mode so the lane's dispatches
        # transcribe it (the probe-target correction: ride this lane).
        ttfr_ms = $null; ttfr_first_files = $null
    }
    $ttfrLine = Select-String -Path (Join-Path $benchDir "stage-$mode.log") `
        -Pattern 'ttfr_ms=(\d+) first_files=(\d+) stage=(\d+)' -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($ttfrLine) {
        $cleanPerf.scans[-1].ttfr_ms = [long]$ttfrLine.Matches[0].Groups[1].Value
        $cleanPerf.scans[-1].ttfr_first_files = [long]$ttfrLine.Matches[0].Groups[2].Value
        Write-Host ("  scan {0}: TTFR {1} ms (first {2} file(s))" -f $mode,
            $cleanPerf.scans[-1].ttfr_ms, $cleanPerf.scans[-1].ttfr_first_files)
    }
    $out | Set-Content (Join-Path $benchDir "scan-$mode.stdout") -Encoding utf8
    Write-Host ("  scan {0}: {1} ms (exit {2})" -f $mode, $sw.ElapsedMilliseconds, $LASTEXITCODE)
}

# the driver bench verb: info + seq + rand + open on the benchmark disk
foreach ($bm in @(@('info', @('--mode', 'info')), @('seq', @('--mode', 'seq', '--mb', '256')), @('rand', @('--mode', 'rand')), @('open', @('--mode', 'open')))) {
    $name = $bm[0]; $margs = $bm[1]
    $out = & $RrecovExe bench --disk $benchDisk @margs --json 2>&1
    $line = ($out | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1)
    $obj = $null
    if ($line) { try { $obj = $line | ConvertFrom-Json } catch { } }
    if ($obj) { $cleanPerf.bench_verb += $obj } else { Note "bench verb $name produced no JSON (recorded)" }
    $out | Set-Content (Join-Path $benchDir "bench-$name.stdout") -Encoding utf8
    Write-Host "  bench $name : $line"
}
$osDiskInfo = & $RrecovExe bench --disk 0 --mode info --json 2>&1
$osDiskInfo | Set-Content (Join-Path $benchDir 'bench-osdisk-info.stdout') -Encoding utf8

# API scan timing (the product surface, same volume, all stages)
$apiBenchDir = Join-Path $benchDir 'api'
New-Item -ItemType Directory -Force -Path $apiBenchDir | Out-Null
& $ApiHarness --scan --target volume --letter $benchLetter --dest (Join-Path $apiBenchDir 'recovered') --out $apiBenchDir --label 'clean-benchmark' 2>&1 |
    Tee-Object -FilePath (Join-Path $apiBenchDir 'harness.stdout') | Write-Host
$apiBenchExit = $LASTEXITCODE
if (Test-Path (Join-Path $apiBenchDir 'api-result.json')) {
    $cleanPerf.api = Get-Content (Join-Path $apiBenchDir 'api-result.json') -Raw | ConvertFrom-Json
}
if ($apiBenchExit -ne 0 -and $apiBenchExit -ne 2) {
    Fail "clean-volume API benchmark exited $apiBenchExit"
}

$cleanPerf | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $PerfDir 'clean-volume-benchmark.json') -Encoding utf8
Copy-Evidence @(Get-ChildItem $benchDir -File | ForEach-Object FullName) $PerfDir
Copy-Evidence @((Join-Path $apiBenchDir 'api-result.json'), (Join-Path $apiBenchDir 'harness.stdout')) $PerfDir
# the clean-benchmark recovery wrote files - remove them (inventory not needed; timings recorded)
Remove-Item -LiteralPath (Join-Path $apiBenchDir 'recovered') -Recurse -Force -ErrorAction SilentlyContinue

# ---- phase 3: the scenario loop -------------------------------------------------------

Write-Host "=== phase 3: scenario matrix (profile=$Profile) ===" -ForegroundColor Cyan
$scenarioVerdicts = @()
$volumeSeen = @{}   # volume label -> seeded at least once this run

function Get-ScenarioScanModes {
    param($Scenario)
    if ($Scenario.scan_modes -eq 'all') { return $ScanModes }
    return @($Scenario.scan_modes -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
}

function Invoke-Reseed {
    param($VolEntry, $MatrixVol)
    $letter = [string]$VolEntry.letter
    # clear the bin for this volume (state hygiene between bin-touching scenarios)
    try { Clear-RecycleBin -DriveLetter $letter -Force -ErrorAction SilentlyContinue } catch { }
    try { Write-VolumeCache -DriveLetter $letter -ErrorAction SilentlyContinue | Out-Null } catch { }
    $profile = [string]$MatrixVol.seed_profile
    if ($profile -eq 'follow-run-profile') { $profile = $Profile }
    Invoke-Seed $VolEntry $profile
}

# evaluate one lane's inventory against the expected deleted set
function Get-Recall {
    param([string] $InventoryJson, [string] $ManifestPath)
    $meta = $null; $entries = @()
    Get-Content -LiteralPath $ManifestPath | ForEach-Object {
        if (-not $_.Trim()) { return }
        $o = $_ | ConvertFrom-Json
        if (-not $o._meta) { $entries += $o }
    }
    $expected = @($entries | Where-Object { -not $_.keep -and [long]$_.size -gt 0 })
    if (-not (Test-Path $InventoryJson)) { return @{ expected = $expected.Count; hash_matched = 0; recall_hash = 0.0; recall_name = 0.0 } }
    $inv = Get-Content -LiteralPath $InventoryJson -Raw | ConvertFrom-Json
    $records = @($inv.records)
    $hashSet = @{}
    $nameSet = @{}
    foreach ($r in $records) {
        if ($r.manifest_rel) {
            if ($r.hash_match) { $hashSet[$r.manifest_rel] = $true }
            $nameSet[$r.manifest_rel] = $true
        }
    }
    $hashHits = 0; $nameHits = 0
    foreach ($e in $expected) {
        if ($hashSet.ContainsKey($e.rel)) { $hashHits++ }
        if ($nameSet.ContainsKey($e.rel)) { $nameHits++ }
    }
    $recallHash = if ($expected.Count -gt 0) { [double]$hashHits / $expected.Count } else { 1.0 }
    $recallName = if ($expected.Count -gt 0) { [double]$nameHits / $expected.Count } else { 1.0 }
    return @{ expected = $expected.Count; hash_matched = $hashHits; recall_hash = $recallHash; recall_name = $recallName }
}

foreach ($sc in ($matrix.scenarios | Where-Object { $_.profile -eq 'smoke' -or $Profile -eq 'full' })) {
    $id = [string]$sc.id
    Write-Host ""
    Write-Host "--- scenario ${id} ($($sc.mode)) ---" -ForegroundColor Magenta
    $scDir = Join-Path $WorkDir "scenarios\$id"
    $scCli = Join-Path $scDir 'cli'
    $scApi = Join-Path $scDir 'api'
    $scEv = Join-Path $EvidenceRoot "04-scenarios\$id"
    New-Item -ItemType Directory -Force -Path $scCli, $scApi, $scEv | Out-Null
    $verdict = [ordered]@{
        scenario = $id; mode = $sc.mode; profile = $sc.profile
        gate_type = $sc.gate.type; gate_note = $sc.gate.note
        lanes = @{}; passed = $false; error = $null
    }

    try {
        $rv = Resolve-Volume -DiskId $sc.disk -VolumeLabel $sc.volume -DiskState $DiskState
        $vol = $rv.vol; $diskEntry = $rv.disk
        $matrixDisk = $matrix.disks | Where-Object { $_.id -eq $sc.disk }
        $matrixVol = $matrixDisk.volumes | Where-Object { $_.label -eq $sc.volume }
        $letter = [string]$vol.letter
        $diskNumber = [int]$diskEntry.disk_number

        # 3.1 deterministic reseed (every scenario starts from the same corpus)
        if ($volumeSeen.ContainsKey($vol.label)) {
            Write-Host "  reseeding $($vol.label) (deterministic corpus restore)"
            Invoke-Reseed $vol $matrixVol
        } else {
            $volumeSeen[$vol.label] = $true
        }
        $manifest = $script:ManifestPath[$vol.label]

        # 3.2 apply the deletion scenario (the ONLY mutation of ground truth)
        $scStateDir = Join-Path $scDir 'state'
        New-Item -ItemType Directory -Force -Path $scStateDir | Out-Null
        $p = @{
            Mode = [string]$sc.mode
            Letter = $letter
            Root = "$letter`:\seed"
            ManifestJsonl = $manifest
            OutDir = $scStateDir
            KeepRoot = "$letter`:\keep"
            FileSystem = [string]$vol.fs
            VolumeLabel = [string]$vol.label
            VolumeBytes = [long]$vol.bytes
        }
        if ($sc.mode -eq 'delete_partition') {
            $p.DiskNumber = $diskNumber
            $p.PartitionNumber = @([int]$vol.partition)
        }
        & $ScenarioScript @p
        $scenarioStatePath = Join-Path $scStateDir 'scenario-state.json'
        Copy-Evidence @($scenarioStatePath, (Join-Path $scStateDir 'hashmap-pre.csv')) (Join-Path $scEv 'state')

        # 3.3 the CLI lane
        $laneModes = Get-ScenarioScanModes $sc
        $p = @{
            RrecovExe = $RrecovExe
            Letter = $letter
            DiskNumber = $diskNumber
            ManifestJsonl = $manifest
            OutDir = $scCli
            Scenario = [string]$sc.mode
            ScanModes = $laneModes
            CapSec = $ScanCapSec
            Threads = $Threads
            DriverInfo = 'service=rda state=running'   # arms the transport assertion (GAP-BENCH-5)
            RecoverCapSec = $RecoverCapSec
            TargetMode = [string]$sc.target_mode
        }
        & $RunOursScript @p
        $cliRunPath = Join-Path $scCli 'ours-run.json'

        # 3.4 the API lane (the product surface)
        $apiDest = Join-Path $scApi 'recovered'
        $apiArgs = @('--scan', '--target', 'volume', '--letter', $letter, '--dest', $apiDest, '--out', $scApi, '--label', $id)
        if ($sc.target_mode -eq 'disk') {
            $apiArgs = @('--scan', '--target', 'disk', '--disk', "$diskNumber", '--whole-disk', '--dest', $apiDest, '--out', $scApi, '--label', $id)
        }
        & $ApiHarness @apiArgs 2>&1 | Tee-Object -FilePath (Join-Path $scApi 'harness.stdout') | Write-Host
        $apiExit = $LASTEXITCODE
        $apiResultPath = Join-Path $scApi 'api-result.json'

        # 3.5 inventories + metrics (ground truth = manifest + method, never tool output)
        $cliRecovered = Join-Path $scCli 'recovered-deepcarve'
        if (-not (Test-Path $cliRecovered)) { $cliRecovered = Join-Path $scCli 'recovered-deep' }
        $invCli = Join-Path $scCli 'recovered-inventory.json'
        $invApi = Join-Path $scApi 'recovered-inventory.json'
        if (Test-Path $cliRecovered) {
            python $InvScript --root $cliRecovered --manifest $manifest --out $invCli --tool ours --label "cli-$id" |
                Tee-Object -FilePath (Join-Path $scCli 'inventory.stdout') | Write-Host
        }
        if (Test-Path $apiDest) {
            python $InvScript --root $apiDest --manifest $manifest --out $invApi --tool ours --label "api-$id" |
                Tee-Object -FilePath (Join-Path $scApi 'inventory.stdout') | Write-Host
        }
        # the repo's own metrics (CLI lane: it carries tree csvs + ours-run.json)
        if ((Test-Path $invCli) -and (Test-Path $scenarioStatePath)) {
            python $MetricsScript --manifest $manifest --scenario $scenarioStatePath --inventory $invCli `
                --outdir $scCli --tool ours --fs ([string]$vol.fs) --mode deepcarve 2>&1 |
                Select-Object -Last 3 | Write-Host
        }

        # 3.6 gate evaluation
        $cliRun = $null
        if (Test-Path $cliRunPath) { $cliRun = Get-Content $cliRunPath -Raw | ConvertFrom-Json }
        $apiResult = $null
        if (Test-Path $apiResultPath) { $apiResult = Get-Content $apiResultPath -Raw | ConvertFrom-Json }
        $cliRecall = Get-Recall -InventoryJson $invCli -ManifestPath $manifest
        $apiRecall = Get-Recall -InventoryJson $invApi -ManifestPath $manifest

        $laneResults = @{}
        foreach ($lane in 'cli', 'api') {
            $recall = if ($lane -eq 'cli') { $cliRecall } else { $apiRecall }
            $res = [ordered]@{
                recall_hash = [math]::Round($recall.recall_hash, 4)
                recall_name = [math]::Round($recall.recall_name, 4)
                expected_deleted = $recall.expected
                hash_matched = $recall.hash_matched
                passed = $true; reasons = @()
            }
            switch ($sc.gate.type) {
                'floor' {
                    $floor = [double]$sc.gate.floor_hash_recall
                    $res.floor = $floor
                    if ($recall.recall_hash -lt $floor) {
                        $res.passed = $false
                        $res.reasons += "hash-recall $($recall.recall_hash) below floor $floor"
                    }
                }
                'discovery' {
                    $lost = 0
                    if ($lane -eq 'cli' -and $cliRun) {
                        $dc = @($cliRun.scan_modes) | Where-Object { $_.mode -eq 'deepcarve' } | Select-Object -First 1
                        if ($dc -and $dc.summary) { $lost = [int64]$dc.summary.lost_partitions }
                    } elseif ($lane -eq 'api' -and $apiResult) {
                        $lost = [int64]$apiResult.scan_stats.lost_partitions
                    }
                    $res.lost_partitions = $lost
                    if ($lost -lt 1) {
                        $res.passed = $false
                        $res.reasons += "lost-partition discovery found $lost (expected >= 1)"
                    }
                    # recover must have completed (CLI: exit 0/2 per mode; API: process_rc 0 or harness exit 0/2)
                    if ($lane -eq 'api' -and $apiExit -ne 0 -and $apiExit -ne 2) {
                        $res.passed = $false
                        $res.reasons += "api harness exit $apiExit"
                    }
                }
                default {
                    # observe: recorded, never gated
                    $res.reasons += 'observe-only (known gap or by-construction - see gate note)'
                }
            }
            # lane-global hard rules: the transport assertion + scan/recover completion
            if ($lane -eq 'cli' -and $cliRun) {
                foreach ($m in @($cliRun.scan_modes)) {
                    if ($null -ne $m.exit_code -and [int]$m.exit_code -ne 0) {
                        $res.passed = $false
                        $res.reasons += "scan mode $($m.mode) exited $($m.exit_code)$(if ($m.capped) { ' (capped at budget)' })"
                    }
                    if ($m.recover -and $m.recover.exit_code -notin @(0, 2)) {
                        $res.passed = $false
                        $res.reasons += "recover mode $($m.mode) exited $($m.recover.exit_code) (0 ok / 2 partial are the honest shapes)"
                    }
                    if ($m.rda_transport_assertion -and [int]$m.rda_transport_assertion.hits -lt 1) {
                        $res.passed = $false
                        $res.reasons += "mode $($m.mode) never used the RDA driver path (transport assertion)"
                    }
                }
            }
            if ($lane -eq 'api') {
                if ($apiExit -ne 0 -and $apiExit -ne 2) {
                    $res.passed = $false
                    $res.reasons += "api harness exit $apiExit (expected 0 or 2)"
                }
                if ($apiResult -and $apiResult.transport -and ([string]$apiResult.transport) -ne 'rda') {
                    $res.passed = $false
                    $res.reasons += "api transport was '$($apiResult.transport)' (expected rda)"
                }
            }
            $laneResults[$lane] = $res
        }
        $verdict.lanes = $laneResults
        $verdict.passed = ($laneResults.cli.passed -and $laneResults.api.passed)
        if (-not $verdict.passed) {
            $why = @()
            foreach ($k in $laneResults.Keys) { if (-not $laneResults[$k].passed) { $why += "$k`: $($laneResults[$k].reasons -join '; ')" } }
            Fail "scenario ${id} FAILED the '$($sc.gate.type)' gate - $($why -join ' | ')"
        } else {
            Write-Host ("PASS {0}: cli hash-recall {1}, api hash-recall {2}" -f $id, $laneResults.cli.recall_hash, $laneResults.api.recall_hash) -ForegroundColor Green
        }

        # 3.7 post-actions: the lost-partition APPLY probe (S-029 feature)
        if ($sc.gate.post_actions -contains 'apply_probe') {
            Write-Host '  post-action: lost-partition APPLY probe'
            $applyEv = @()
            $partBefore = @(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue).Count
            $sessionPath = Join-Path $scCli 'session-deepcarve.crsf'
            if (Test-Path $sessionPath) {
                # dry-run (plan print, no write): exit 0 whether planned or REFUSED - recorded either way
                $dryOut = & $RrecovExe apply --disk $diskNumber --session $sessionPath --candidate 1 2>&1
                $dryExit = $LASTEXITCODE
                $applyEv += "DRY-RUN exit ${dryExit}:" + ($dryOut -join ' | ')
                if ($dryExit -ne 0) {
                    Fail "apply dry-run exited $dryExit (expected 0 - a plan or an honest REFUSED both exit 0)"
                }
                $refused = (($dryOut -join ' ') -match 'REFUSED|refused')
                if (-not $refused) {
                    # the real write path (disposable VHD, last scenario on this disk, undo logged)
                    $undoPath = Join-Path $scCli 'apply-disk.undo'
                    $realOut = & $RrecovExe apply --disk $diskNumber --session $sessionPath --candidate 1 --apply --undo $undoPath 2>&1
                    $realExit = $LASTEXITCODE
                    $applyEv += "APPLY exit ${realExit}:" + ($realOut -join ' | ')
                    Start-Sleep -Seconds 2
                    try { Update-HostStorageCache -ErrorAction SilentlyContinue } catch { }
                    $rescanTmp = [System.IO.Path]::GetTempFileName()
                    'rescan' | Set-Content -Path $rescanTmp -Encoding ascii
                    diskpart /s $rescanTmp 2>&1 | Out-Null
                    Remove-Item $rescanTmp -Force -ErrorAction SilentlyContinue
                    $partAfter = @(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue).Count
                    $applyEv += "partitions before=$partBefore after=$partAfter"
                    if ($realExit -eq 0 -and $partAfter -gt $partBefore) {
                        Write-Host "PASS apply probe: partition restored ($partBefore -> $partAfter partitions)"
                    } else {
                        Note "apply probe: real apply exit $realExit, partitions $partBefore -> $partAfter (recorded - the dry-run planned a write but the apply path did not restore the entry; engine capability recorded, not a CI failure)"
                    }
                } else {
                    Note 'apply probe: planner REFUSED (recorded - an honest refusal is a valid capability state)'
                }
            } else {
                Note 'apply probe: no deepcarve session to plan from (recorded)'
            }
            $applyEv | Set-Content (Join-Path $scEv 'apply-probe.txt') -Encoding utf8
        }

        # 3.8 evidence + payload cleanup (samples kept, gigabytes dropped)
        Copy-Evidence @(
            $cliRunPath, (Join-Path $scCli 'scan-deepcarve.stdout'), (Join-Path $scCli 'scan-deepcarve.stderr'),
            (Join-Path $scCli 'stage-deepcarve.log'), (Join-Path $scCli 'tree-deepcarve.json'),
            (Join-Path $scCli 'tree-deepcarve.csv'), (Join-Path $scCli 'report-deepcarve.json'),
            (Join-Path $scCli 'inventory.stdout'), (Join-Path $scCli 'recovered-inventory.json'),
            (Join-Path $scCli 'metrics.json'), (Join-Path $scCli 'session-deepcarve.crsf'),
            (Join-Path $scCli 'preservation.json'), (Join-Path $scCli 'samples-deepcarve.csv'),
            (Join-Path $scCli 'sample-recovered')
        ) (Join-Path $scEv 'cli')
        Copy-Evidence @(
            $apiResultPath, (Join-Path $scApi 'report.json'), (Join-Path $scApi 'harness.stdout'),
            (Join-Path $scApi 'session.crsf'), (Join-Path $scApi 'recovered-inventory.json'),
            (Join-Path $scApi 'inventory.stdout')
        ) (Join-Path $scEv 'api')
        $verdict | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'verdict.json') -Encoding utf8
        $scenarioVerdicts += $verdict

        # drop the recovered payloads (evidence carries inventories/reports/samples)
        foreach ($rd in @(Get-ChildItem $scCli -Directory -Filter 'recovered-*' -ErrorAction SilentlyContinue)) {
            Remove-Item -LiteralPath $rd.FullName -Recurse -Force -ErrorAction SilentlyContinue
        }
        Remove-Item -LiteralPath $apiDest -Recurse -Force -ErrorAction SilentlyContinue
    } catch {
        $verdict.error = "exception: $($_.Exception.Message)"
        $verdict.passed = $false
        Fail "scenario ${id} threw: $($_.Exception.Message)"
        $verdict | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'verdict.json') -Encoding utf8
        $scenarioVerdicts += $verdict
    }
}

# ---- phase 4: the verdict --------------------------------------------------------------

Write-Host '=== phase 4: summaries ===' -ForegroundColor Cyan
$summary = [ordered]@{
    profile = $Profile
    captured_utc = (Get-Date).ToUniversalTime().ToString('o')
    scan_modes = $ScanModes
    scenarios = @($scenarioVerdicts | ForEach-Object {
        [ordered]@{
            id = $_.scenario; mode = $_.mode; gate = $_.gate_type; passed = $_.passed
            cli = $_.lanes.cli; api = $_.lanes.api; error = $_.error
        }
    })
    soft_records = $script:SoftRecords
    hard_failures = $script:HardFailures
}
$summary | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $SummariesDir 'summary.json') -Encoding utf8

# human-readable gate table
$lines = @('# Gate summary', '', '| scenario | gate | cli | api | verdict |', '|---|---|---|---|---|')
foreach ($v in $scenarioVerdicts) {
    $cliTxt = if ($v.lanes.cli) { "recall $($v.lanes.cli.recall_hash)" } else { '-' }
    $apiTxt = if ($v.lanes.api) { "recall $($v.lanes.api.recall_hash)" } else { '-' }
    $verdictTxt = if ($v.passed) { 'PASS' } else { 'FAIL' }
    $lines += "| $($v.scenario) | $($v.gate_type) | $cliTxt | $apiTxt | $verdictTxt |"
}
$lines += ''
if ($script:HardFailures.Count -gt 0) {
    $lines += '## Failures'
    $script:HardFailures | ForEach-Object { $lines += "- $_" }
} else {
    $lines += 'All hard gates green.'
}
$lines | Set-Content (Join-Path $SummariesDir 'gates.md') -Encoding utf8
$lines | Write-Host

# perf roll-up: per scenario/mode scan+recover walls + API timings
$perf = [ordered]@{ captured_utc = (Get-Date).ToUniversalTime().ToString('o'); scenarios = @() }
foreach ($v in $scenarioVerdicts) {
    $scCli = Join-Path $WorkDir "scenarios\$($v.scenario)\cli"
    $scApi = Join-Path $WorkDir "scenarios\$($v.scenario)\api"
    $entry = [ordered]@{ id = $v.scenario; cli_modes = @(); api = $null }
    $orp = Join-Path $scCli 'ours-run.json'
    if (Test-Path $orp) {
        $run = Get-Content $orp -Raw | ConvertFrom-Json
        foreach ($m in @($run.scan_modes)) {
            $entry.cli_modes += [ordered]@{
                mode = $m.mode; scan_wall_s = $m.wall_seconds; scan_exit = $m.exit_code; capped = $m.capped
                recover_wall_s = if ($m.recover) { $m.recover.wall_seconds } else { $null }
                files = if ($m.summary) { $m.summary.files } else { $null }
                lost_partitions = if ($m.summary) { $m.summary.lost_partitions } else { $null }
            }
        }
    }
    $arp = Join-Path $scApi 'api-result.json'
    if (Test-Path $arp) {
        $entry.api = Get-Content $arp -Raw | ConvertFrom-Json
    }
    $perf.scenarios += $entry
}
$perf | ConvertTo-Json -Depth 10 | Set-Content (Join-Path $PerfDir 'perf-summary.json') -Encoding utf8
Write-Host "perf summary -> $PerfDir\perf-summary.json"

if ($script:HardFailures.Count -gt 0) {
    Write-Host "::error::$($script:HardFailures.Count) hard gate failure(s) - see evidence/06-comparison/gates.md" -ForegroundColor Red
    exit 1
}
Write-Host 'ALL HARD GATES GREEN' -ForegroundColor Green
exit 0
