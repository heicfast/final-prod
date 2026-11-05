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
#   2. clean-volume benchmark: scan timings per mode + the bench verb
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
        (Join-Path $EvidenceRoot '00-environment'),
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
        # $seedProfile (NOT $profile): the case-insensitive name would collide
        # with the [ValidateSet('smoke','full')] $Profile param — assigning the
        # matrix's 'follow-run-profile' sentinel to a validated variable throws
        # MetadataError and leaves the run profile stuck at the previous
        # volume's value (run 6: the CRBIG seed). Distinct name, no collision.
        $seedProfile = [string]$mv.seed_profile
        if ($seedProfile -eq 'follow-run-profile') { $seedProfile = $Profile }
        Invoke-Seed $v $seedProfile
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

# S-040 image-create round-trip (class (a), the direct read view): the
# produced flat image must hash byte-identical to the source device - the
# digest was computed over the bytes READ FROM the device, so
# sidecar-digest == Get-FileHash(image) proves the committed file equals
# the device stream; the size must equal the disk size exactly; and a
# scan --image of the produced file must exit 0 (the safe-scan posture,
# UIUX/11:17)
$imgPath = Join-Path $probeDir 'multi-gpt.img'
$imgOut = & $RrecovExe image --create --disk $multiDisk --out $imgPath 2>&1
$imgExit = $LASTEXITCODE
$imgOut | Set-Content (Join-Path $probeEv 'image-create.txt') -Encoding utf8
if ($imgExit -ne 0) {
    Fail "image --create --disk ${multiDisk} exited $imgExit (expected 0)"
} else {
    $sidecarPath = "$imgPath.sha256"
    if (-not (Test-Path $sidecarPath)) {
        Fail "image create left no .sha256 sidecar at $sidecarPath"
    } else {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $imgPath).Hash.ToLower()
        $sidecar = (Get-Content -LiteralPath $sidecarPath -Raw).ToLower()
        if ($sidecar -notmatch [regex]::Escape($hash)) {
            Fail "image hash mismatch: sidecar='$sidecar' file=$hash (the committed file must equal the device stream)"
        } else {
            $diskBytes = (Get-Disk -Number $multiDisk).Size
            $imgBytes = (Get-Item -LiteralPath $imgPath).Length
            if ($imgBytes -ne $diskBytes) {
                Fail "image size $imgBytes != disk size $diskBytes"
            } else {
                $imgScan = Join-Path $probeDir 'multi-gpt-scan.crsf'
                $scanOut = & $RrecovExe scan --image $imgPath --out $imgScan 2>&1
                $scanExit = $LASTEXITCODE
                $scanOut | Set-Content (Join-Path $probeEv 'image-scan.txt') -Encoding utf8
                if ($scanExit -ne 0) {
                    Fail "scan --image (of the created image) exited $scanExit"
                } else {
                    Write-Host ("PASS image-create round-trip: {0:N0} bytes, digest {1}.., scan --image exit 0" -f $imgBytes, $hash.Substring(0, 12))
                }
            }
        }
    }
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
# -join first: rrecov list-carve-formats emits the count line PLUS one line per
# format (239 formats = 240 lines), so the raw capture is an ARRAY — and an
# array -match filters instead of populating $Matches, leaving $cliCount null
# (run 6's "CLI= API=239" parity failure). Joined to a scalar, the regex
# populates $Matches exactly as the probe always intended.
$cliCarve = (& $RrecovExe list-carve-formats 2>&1) -join "`n"
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

# session-97 T6: process sampling for the clean-volume scans (the CPU-per-byte
# and working-set rows). The scenario lane always sampled (run-ours.ps1); the
# clean-volume phase never did - the driver-era CPU/WS baselines exist only for
# the scenario lane. Get-SampleStats is the shared parser (phase 4 reuses it).
function Get-SampleStats {
    param([string] $CsvPath)
    if (-not (Test-Path -LiteralPath $CsvPath)) { return $null }
    $rows = @(Get-Content -LiteralPath $CsvPath | Select-Object -Skip 1 | Where-Object { $_.Trim() })
    if ($rows.Count -lt 2) { return $null }
    $wsPeak = [long]0
    foreach ($r in $rows) {
        $c = $r -split ','
        if ($c.Count -ge 2) { $ws = [long]0; if ([long]::TryParse($c[1], [ref]$ws) -and $ws -gt $wsPeak) { $wsPeak = $ws } }
    }
    $c0 = $rows[0] -split ','; $c1 = $rows[-1] -split ','
    $cpu = $null
    if ($c0.Count -ge 3 -and $c1.Count -ge 3) {
        $v0 = 0.0; $v1 = 0.0
        if ([double]::TryParse($c0[2], [ref]$v0) -and [double]::TryParse($c1[2], [ref]$v1)) { $cpu = [math]::Round($v1 - $v0, 3) }
    }
    return @{ samples = $rows.Count; cpu_seconds = $cpu; ws_peak_bytes = $wsPeak }
}

function Invoke-SampledCli {
    param([string] $Exe, [string[]] $ArgList, [string] $StdoutPath, [string] $StderrPath, [string] $SamplesPath)
    $t0 = Get-Date
    $p = Start-Process -FilePath $Exe -ArgumentList $ArgList `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath `
        -PassThru -NoNewWindow
    $samples = New-Object System.Collections.Generic.List[string]
    $samples.Add('t_ms,working_set_bytes,cpu_seconds')
    while (-not $p.HasExited) {
        Start-Sleep -Milliseconds 500
        $age = [math]::Round(((Get-Date) - $t0).TotalMilliseconds)
        try {
            $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
            if ($proc) { $samples.Add(("{0:F0},{1},{2:F3}" -f $age, $proc.WorkingSet64, $proc.CPU)) }
        } catch { }
    }
    $samples | Set-Content -Path $SamplesPath -Encoding utf8
    # session-97 (the perf gate's first live catch, run 42012118585): the wall
    # is the PROCESS's own span (ExitTime - StartTime) - the 500 ms poll
    # quantum overstated sub-second walls by up to a full quantum (quick
    # measured 512 ms vs the true ~27 ms). Comparability with the driver-era
    # stopwatch walls (which never carried the quantum) requires this.
    $wallMs = [math]::Round(((Get-Date) - $t0).TotalMilliseconds)
    try {
        $p.Refresh()
        if ($p.ExitTime -and $p.StartTime) { $wallMs = [math]::Round(($p.ExitTime - $p.StartTime).TotalMilliseconds) }
    } catch { }
    return @{ exit = $p.ExitCode; wall_ms = $wallMs }
}

# CLI scan timings per mode on the intact (pre-deletion) volume
foreach ($mode in $ScanModes) {
    $session = Join-Path $benchDir "session-$mode.crsf"
    $scanArgs = @('scan', '--volume', "$benchLetter`:", '--threads', "$Threads")
    if ($mode -eq 'quick') { $scanArgs += '--quick-only' }
    elseif ($mode -eq 'deep') { $scanArgs += '--no-carve' }
    $scanArgs += @('--out', $session, '--log', (Join-Path $benchDir "stage-$mode.log"), '--json')
    $sr = Invoke-SampledCli -Exe $RrecovExe -ArgList $scanArgs `
        -StdoutPath (Join-Path $benchDir "scan-$mode.stdout") `
        -StderrPath (Join-Path $benchDir "scan-$mode.stderr") `
        -SamplesPath (Join-Path $benchDir "samples-$mode.csv")
    $summary = $null
    $so = ''
    if (Test-Path -LiteralPath (Join-Path $benchDir "scan-$mode.stdout")) {
        $so = (Get-Content -LiteralPath (Join-Path $benchDir "scan-$mode.stdout") -Raw)
    }
    $jline = ($so -split "`n" | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1)
    if ($jline) { try { $summary = $jline | ConvertFrom-Json } catch { } }
    $modeStats = Get-SampleStats -CsvPath (Join-Path $benchDir "samples-$mode.csv")
    $cleanPerf.scans += [ordered]@{
        mode = $mode; wall_ms = $sr.wall_ms; exit = $sr.exit
        files = if ($summary) { $summary.files } else { $null }
        bytes_scanned = if ($summary) { $summary.bytes_scanned } else { $null }
        elapsed_ms = if ($summary) { $summary.elapsed_ms } else { $null }
        # OQ-S4-3's TTFR half: the CLI's event-stream probe logs ONE line at
        # the first positive FilesFound event (run-elapsed ms,
        # pause-compensated; the honest granularity - counters coalesce at
        # stage boundaries). Extracted per mode so the lane's dispatches
        # transcribe it (the probe-target correction: ride this lane).
        ttfr_ms = $null; ttfr_first_files = $null
        # session-97 T6: the CPU/WS rows (same parser as the scenario lane)
        cpu_seconds = $modeStats.cpu_seconds; ws_peak_bytes = $modeStats.ws_peak_bytes; sample_count = $modeStats.samples
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
    Write-Host ("  scan {0}: {1} ms (exit {2}, cpu {3}s, ws-peak {4} MiB)" -f $mode, $sr.wall_ms, $sr.exit,
        $modeStats.cpu_seconds, [math]::Round($modeStats.ws_peak_bytes / 1MB, 1))
}

# the bench verb: info + the consumer-native size sweep + open on the benchmark disk
# (session-97 T6: the 1 MiB legs remain the headline rows; the 64 KiB / 256 KiB /
# 4 MiB seq legs and the 4 KiB rand leg are the CONSUMER-NATIVE sizes - the cache
# block fill, the og's 4 MiB chunking, the image chunk, the NTFS record granularity.
# The 'leg' label disambiguates the sweep in clean-volume-benchmark.json - the
# product's own "mode" field alone cannot.)
foreach ($bm in @(
    @('info',    @('--mode', 'info')),
    @('seq',     @('--mode', 'seq', '--mb', '256')),                        # headline: 1 MiB block (default)
    @('seq-64k', @('--mode', 'seq', '--mb', '256', '--block-kb', '64')),   # consumer: discovery windows, image chunks
    @('seq-256k',@('--mode', 'seq', '--mb', '256', '--block-kb', '256')),  # consumer: the CachedReader fill size
    @('seq-4m',  @('--mode', 'seq', '--mb', '256', '--block-kb', '4096')), # consumer: og 4 MiB chunks, remnant blocks
    @('rand',    @('--mode', 'rand')),                                      # headline: 1 MiB block (default)
    @('rand-4k', @('--mode', 'rand', '--block-kb', '4')),                   # consumer: NTFS record granularity
    @('open',    @('--mode', 'open'))
)) {
    $name = $bm[0]; $margs = $bm[1]
    $out = & $RrecovExe bench --disk $benchDisk @margs --json 2>&1
    $line = ($out | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1)
    $obj = $null
    if ($line) { try { $obj = $line | ConvertFrom-Json } catch { } }
    if ($obj) {
        $obj | Add-Member -NotePropertyName leg -NotePropertyValue $name -Force
        $cleanPerf.bench_verb += $obj
    } else { Note "bench verb $name produced no JSON (recorded)" }
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
    # Same $seedProfile naming as the phase-0 seed loop: never assign to a
    # name that collides with the validated $Profile param (this site sits
    # in function scope today, so it only works by accident — same shape,
    # same fix, before it copies the run-6 bug class into the reseed path).
    $seedProfile = [string]$MatrixVol.seed_profile
    if ($seedProfile -eq 'follow-run-profile') { $seedProfile = $Profile }
    Invoke-Seed $VolEntry $seedProfile
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

        # session-97 (S-PART-DELETE H1, run 41140212296): a raw whole-disk open
        # milliseconds after a table mutation can fail inside the PnP/volume-stack
        # reconfiguration window (the disk WAS raw-openable 35 ms earlier for the
        # post-state hash map; the scan died ~30 ms after that). Settle BEFORE the
        # scan lanes: wait for two consecutive identical partition-table snapshots
        # (500 ms apart), bounded at 30 s, plus a 1 s floor. HARNESS-SIDE ONLY -
        # the product's fail-fast open contract is driver parity and must not
        # change; if the scan still fails after this settle, the archived evidence
        # (the enriched throw + the catch-path archive) decides H2 (product defect)
        # with the exit code and stream tail in hand.
        if ($sc.mode -eq 'delete_partition') {
            $settleDeadline = (Get-Date).AddSeconds(30)
            $snapPrev = $null
            $settleStable = $false
            while ((Get-Date) -lt $settleDeadline) {
                $snap = @(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue |
                    Sort-Object PartitionNumber |
                    ForEach-Object { "$($_.PartitionNumber):$($_.StartingOffset):$($_.Size)" }) -join '|'
                if ($null -ne $snapPrev -and $snap -eq $snapPrev) { $settleStable = $true; break }
                $snapPrev = $snap
                Start-Sleep -Milliseconds 500
            }
            Start-Sleep -Seconds 1
            $settleCount = @(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue).Count
            Write-Host ("  table settle: stable={0} partitions={1} (S-PART-DELETE H1 guard)" -f $settleStable, $settleCount)
        }

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
            DriverInfo = 'transport=direct'   # arms the transport assertion (GAP-BENCH-5)
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
                    if ($m.direct_transport_assertion -and [int]$m.direct_transport_assertion.hits -lt 1) {
                        $res.passed = $false
                        $res.reasons += "mode $($m.mode) never used the direct transport (transport assertion)"
                    }
                }
            }
            if ($lane -eq 'api') {
                if ($apiExit -ne 0 -and $apiExit -ne 2) {
                    $res.passed = $false
                    $res.reasons += "api harness exit $apiExit (expected 0 or 2)"
                }
                # session-97: the transport is 'direct' since v0.7.0 - the stale
                # 'rda' expectation here false-failed every API lane in run
                # 41140212296. A MISSING transport is equally a failure (the
                # silent-fallback shape the assertion exists to catch).
                if (-not $apiResult -or -not $apiResult.transport -or ([string]$apiResult.transport) -ne 'direct') {
                    $res.passed = $false
                    $observedTransport = if ($apiResult) { "'$($apiResult.transport)'" } else { '<no api-result>' }
                    $res.reasons += "api transport was $observedTransport (expected direct)"
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
        # EVIDENCE FIRST (session-97, run 41140212296's S-PART-DELETE lesson):
        # the no-silent-skip path must preserve its own diagnosis - the throw
        # previously archived NO scan stdout/stderr/exit code, leaving the
        # H1-vs-H2 question unanswerable from the artifact. Best-effort copy of
        # every diagnostic the lanes managed to write before the throw (never
        # the recovered-* payload dirs - those are gigabytes).
        foreach ($lanePair in @(@($scCli, 'cli'), @($scApi, 'api'))) {
            $laneDir = $lanePair[0]; $laneName = $lanePair[1]
            if (Test-Path -LiteralPath $laneDir) {
                $diags = Get-ChildItem -LiteralPath $laneDir -File -ErrorAction SilentlyContinue |
                    Where-Object { $_.Name -match '^(scan-|recover-|stage-|samples-|tree-|report-|metrics|preservation|ours-run|harness\.stdout|api-result\.json|cancel\.stdout|resume\.stdout)' }
                foreach ($df in $diags) {
                    Copy-Evidence @($df.FullName) (Join-Path $scEv $laneName)
                }
            }
        }
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
            # session-97 T6: the CPU/WS transcription - the samples CSVs were
            # always captured, never rolled up (the -39% CPU finding lived only
            # in raw CSVs someone had to open by hand)
            $mStats = Get-SampleStats -CsvPath (Join-Path $scCli "samples-$($m.mode).csv")
            $rStats = Get-SampleStats -CsvPath (Join-Path $scCli "recover-samples-$($m.mode).csv")
            $entry.cli_modes += [ordered]@{
                mode = $m.mode; scan_wall_s = $m.wall_seconds; scan_exit = $m.exit_code; capped = $m.capped
                recover_wall_s = if ($m.recover) { $m.recover.wall_seconds } else { $null }
                files = if ($m.summary) { $m.summary.files } else { $null }
                lost_partitions = if ($m.summary) { $m.summary.lost_partitions } else { $null }
                scan_cpu_seconds = $mStats.cpu_seconds; scan_ws_peak_bytes = $mStats.ws_peak_bytes
                recover_cpu_seconds = $rStats.cpu_seconds; recover_ws_peak_bytes = $rStats.ws_peak_bytes
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

# ---- phase 5: the performance regression gate (session-97 T6; PERFORMANCE-TARGET §6) ----
# The versioned baseline (bench/baseline.json, perf section) carries the driver-
# parity rows with their tolerances. A row FAILS when the observed value violates
# its tolerance, and FAILS TOO when the metric cannot be found in this run's
# capture (no silent skip: a metric that stops being measured is a gate break,
# not a pass). The baseline changes only by reviewed commit with evidence.
$baselinePath = Join-Path $RepoRoot 'bench\baseline.json'
$gateVerdicts = @()
if (-not (Test-Path -LiteralPath $baselinePath)) {
    Note 'perf gate: bench/baseline.json not found - gate skipped (recorded, never silent)'
} else {
    $bl = $null
    try { $bl = Get-Content -LiteralPath $baselinePath -Raw | ConvertFrom-Json } catch { }
    if (-not $bl -or -not $bl.perf -or -not $bl.perf.rows) {
        Note 'perf gate: bench/baseline.json carries no perf.rows - gate skipped (recorded, never silent)'
    } else {
        foreach ($row in @($bl.perf.rows)) {
            $obs = $null
            switch ([string]$row.collection) {
                'bench_verb' {
                    $cands = @($cleanPerf.bench_verb | Where-Object { [string]$_.leg -eq [string]$row.leg })
                    if ($cands.Count -gt 0) { $obs = $cands[0].([string]$row.metric) }
                }
                'scans' {
                    $cands = @($cleanPerf.scans | Where-Object { [string]$_.mode -eq [string]$row.leg })
                    if ($cands.Count -gt 0) { $obs = $cands[0].([string]$row.metric) }
                }
                'api' {
                    if ($cleanPerf.api) { $obs = $cleanPerf.api.timings_ms.([string]$row.leg) }
                }
                'scan_cpu_ws' {
                    # cpu_s_per_gib: scan CPU over bytes_scanned (GiB) for the leg mode
                    $cands = @($cleanPerf.scans | Where-Object { [string]$_.mode -eq [string]$row.leg })
                    if ($cands.Count -gt 0 -and $cands[0].cpu_seconds -ne $null -and $cands[0].bytes_scanned -gt 0) {
                        if ([string]$row.metric -eq 'cpu_s_per_gib') {
                            $obs = [math]::Round([double]$cands[0].cpu_seconds / ([double]$cands[0].bytes_scanned / 1GB), 1)
                        } elseif ([string]$row.metric -eq 'ws_peak_mib') {
                            $obs = [math]::Round([double]$cands[0].ws_peak_bytes / 1MB, 1)
                        }
                    }
                }
            }
            $verdict = 'MISSING'
            $limit = $null
            $baseVal = [double]$row.value
            $tol = [double]$row.tol_pct
            if ([string]$row.op -eq '>=') { $limit = [math]::Round($baseVal * (1.0 - $tol / 100.0), 2) }
            else { $limit = [math]::Round($baseVal * (1.0 + $tol / 100.0), 2) }
            if ($null -ne $obs) {
                if ([string]$row.op -eq '>=') { $verdict = if ([double]$obs -ge $limit) { 'PASS' } else { 'FAIL' } }
                else { $verdict = if ([double]$obs -le $limit) { 'PASS' } else { 'FAIL' } }
            }
            $gateVerdicts += [ordered]@{
                id = $row.id; collection = $row.collection; leg = $row.leg; metric = $row.metric
                op = $row.op; baseline = $baseVal; tol_pct = $tol; limit = $limit
                observed = $obs; verdict = $verdict; source = $row.source
            }
        }
    }
}
if ($gateVerdicts.Count -gt 0) {
    $gateSummary = [ordered]@{
        captured_utc = (Get-Date).ToUniversalTime().ToString('o')
        rule = 'driver-parity tolerances from PERFORMANCE-TARGET.md section-1; baseline changes only by reviewed commit with evidence'
        rows = $gateVerdicts
    }
    $gateSummary | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $SummariesDir 'perf-gate.json') -Encoding utf8
    $gateLines = @('', '## Performance gate (driver parity)', '', '| id | metric | baseline | limit | observed | verdict |', '|---|---|---|---|---|---|')
    foreach ($g in $gateVerdicts) {
        $gateLines += "| $($g.id) | $($g.metric) $($g.op) | $($g.baseline) | $($g.limit) | $($g.observed) | $($g.verdict) |"
        if ($g.verdict -ne 'PASS') {
            Fail "perf gate $($g.id): observed $($g.observed) $($g.op) $($g.limit) (baseline $($g.baseline), tol $($g.tol_pct)%) - $($g.source)"
        }
    }
    Add-Content -Path (Join-Path $SummariesDir 'gates.md') -Value ($gateLines -join "`n") -Encoding utf8
    $gateLines | Write-Host
}

if ($script:HardFailures.Count -gt 0) {
    Write-Host "::error::$($script:HardFailures.Count) hard gate failure(s) - see evidence/06-comparison/gates.md" -ForegroundColor Red
    exit 1
}
Write-Host 'ALL HARD GATES GREEN' -ForegroundColor Green
exit 0
