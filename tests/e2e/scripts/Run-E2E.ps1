# Run-E2E.ps1 - the tests/e2e master orchestrator (one shard of the e2e matrix).
#
# The e2e contract (tests/e2e/README.md): for every scenario in the shard, on a
# REAL Windows runner, on REAL VHDX virtual disks, with REAL seeded files:
#   1. create the shard's disk environment (engine/.github/scripts/New-TestDisk.ps1
#      - diskpart-only vdisk create/attach; the CI-proven no-Hyper-V pattern)
#   2. seed the deterministic corpus (engine/bench/seed/generate_seed.ps1,
#      corpus v2.0.0) -> seed-manifest.jsonl (THE ground truth)
#   3. apply the deletion scenario (engine/bench/scenarios/apply-scenario.ps1 -
#      12 modes, hash-map capture pre/post, changed-block proof)
#   4. run the CLI lane (engine/bench/ours/run-ours.ps1: rrecov scan/tree/recover
#      per mode with full stdout/stderr/exit/session/report/timing capture)
#   5. run the API lane (engine/.github/scripts/api_harness.exe: the cr_ui C-ABI
#      lifecycle open->scan->session->query->recovery, QPC-timed)
#   6. inventory both lanes' recovered trees against the manifest
#      (engine/bench/metrics/inventory_recovered.py) and compute metrics
#      (engine/bench/metrics/compute_metrics.py: recall/precision/F1/FP)
#   7. prove the two paths agree (Compare-Lanes.ps1 - CLI-vs-API divergence)
#   8. lay every artifact down in the results tree (tests/e2e/results schema)
#   9. teardown the VHDs (engine/.github/scripts/Teardown-Benchmark.ps1, always)
#
# Reuse discipline (worklog/TEST-INVENTORY.md): the proven bench primitives are
# INVOKED, not forked - one copy of the battle-tested code, e2e orchestrates it.
# New code in this suite: this orchestrator, Compare-Lanes.ps1 (divergence),
# Collect-Results.ps1 (results schema), New-Baseline.ps1 (baseline builder).
#
# Ground truth is ONLY the seed manifest + the recorded deletion method - never
# a tool's own output. Extra finds are recorded, never failures.
#
# Verdict policy: exit 1 iff (a) a hard gate failed (floor/discovery), (b) a
# lane could not execute (scan produced no session, recover exit outside 0/2,
# API exit outside 0/2), (c) CLI-vs-API divergence, (d) an exception. There is
# no continue-on-error inside a scenario: every result is COLLECTED, the verdict
# lands at the END (the collect-every-result discipline), and a failed scenario
# never hides the ones after it.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $MatrixPath,
    [Parameter(Mandatory = $true)] [string] $RepoRoot,        # the monorepo checkout
    [Parameter(Mandatory = $true)] [string] $CoreStaged,      # staged kit: <core>\bin\rrecov.exe + cr_ui.dll + userdata-defaults.json
    [Parameter(Mandatory = $true)] [string] $WorkRoot,         # <drive>:\e2e-bench (disks\, work\)
    [Parameter(Mandatory = $true)] [string] $ResultsRoot,     # the shard results fragment (committed layout)
    [Parameter(Mandatory = $true)] [ValidateSet('smoke', 'full')] [string] $Profile,
    [string] $ShardId = '',                                    # '' = all scenarios (single-machine mode)
    [string] $ApiHarness = '',                                # compiled api_harness.exe; '' + -IncludeApi = hard error
    [bool] $IncludeApi = $true,
    [string[]] $ScanModes = @('quick', 'deep', 'deepcarve'),
    [int] $ScanCapSec = 1200,
    [int] $RecoverCapSec = 1800,
    [int] $Threads = 4,
    [string] $RunId = '',
    [string] $RunUrl = ''
)

$ErrorActionPreference = 'Continue'   # collect-every-result, verdict-at-the-end (the proven discipline)
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

# ---- fixed layout ----------------------------------------------------------------------
$EngineRoot = Join-Path $RepoRoot 'engine'
$RrecovExe = Join-Path $CoreStaged 'bin\rrecov.exe'
$NewDiskScript = Join-Path $EngineRoot '.github\scripts\New-TestDisk.ps1'
$TeardownScript = Join-Path $EngineRoot '.github\scripts\Teardown-Benchmark.ps1'
$SeedScript = Join-Path $EngineRoot 'bench\seed\generate_seed.ps1'
$ScenarioScript = Join-Path $EngineRoot 'bench\scenarios\apply-scenario.ps1'
$RunOursScript = Join-Path $EngineRoot 'bench\ours\run-ours.ps1'
$InvScript = Join-Path $EngineRoot 'bench\metrics\inventory_recovered.py'
$MetricsScript = Join-Path $EngineRoot 'bench\metrics\compute_metrics.py'
$CompareLanesScript = Join-Path $PSScriptRoot 'Compare-Lanes.ps1'
$WorkDir = Join-Path $WorkRoot 'work'
$DiskStatePath = Join-Path $WorkRoot 'disk-state.json'

$script:HardFailures = @()
$script:SoftRecords = @()

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

# ---- runner context + sanity gates (fail fast with diagnostics, never silently) ---------
New-Item -ItemType Directory -Force -Path $WorkDir, $ResultsRoot, (Join-Path $ResultsRoot '00-environment') | Out-Null
$ctxCpu = $null; $ctxCores = $null; $ctxRam = $null; $ctxElevated = $null
try { $ctxCpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name) } catch { }
try { $ctxCores = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty NumberOfLogicalProcessors) } catch { }
try { $ctxRam = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 1) } catch { }
try { $ctxElevated = (New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator) } catch { }
$ctxCommit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { try { (git -C $RepoRoot rev-parse HEAD).Trim() } catch { '' } }
$ctx = [ordered]@{
    imageOs = $env:ImageOS; runnerName = $env:RUNNER_NAME
    cpu = $ctxCpu; cores = $ctxCores; ramGb = $ctxRam; isElevated = $ctxElevated
    ranAtUtc = (Get-Date).ToUniversalTime().ToString('o')
    runId = $RunId; runUrl = $RunUrl; profile = $Profile; shard = $ShardId
    commit = $ctxCommit
}
$ctx | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $ResultsRoot '00-environment\runner-context.json') -Encoding utf8

foreach ($required in @($RrecovExe, $NewDiskScript, $TeardownScript, $SeedScript, $ScenarioScript, $RunOursScript, $InvScript, $MetricsScript)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "required component missing: $required" }
}
if ($IncludeApi -and -not $ApiHarness) { throw 'API lane enabled but -ApiHarness not supplied (no silent skip)' }

# ---- load the matrix; select the shard --------------------------------------------------
$matrix = Get-Content -LiteralPath $MatrixPath -Raw | ConvertFrom-Json
$isSmoke = ($Profile -eq 'smoke')
$matrixDisks = if ($isSmoke) { $matrix.smoke.disks } else { $matrix.disks }
$matrixScenarios = if ($isSmoke) { $matrix.smoke.scenarios } else { $matrix.scenarios }

$shard = $null
if (-not $isSmoke -and $ShardId) {
    $shard = @($matrix.shards | Where-Object { $_.id -eq $ShardId })
    if ($shard.Count -ne 1) { throw "shard '$ShardId' not found in matrix.shards" }
    $shard = $shard[0]
    $shardDisks = @($shard.disks)
    $shardScenarioIds = @($shard.scenarios)
    # ORDER BY THE SHARD LIST (run 45826003339's exfat-b lesson: selecting by
    # the matrix array's order silently ran PARTDEL first on its disk, forced
    # the reseed's partition recreation, and surfaced the exFAT extent garbage
    # finding - the shard's declared order is the contract; delete_partition
    # is always last per disk by matrix design)
    $selected = @($shardScenarioIds | ForEach-Object {
        $sid = $_
        $matrixScenarios | Where-Object { $_.id -eq $sid } | Select-Object -First 1
    })
    $selectedIds = @($selected | ForEach-Object { $_.id })
    $missing = @($shardScenarioIds | Where-Object { $selectedIds -notcontains $_ })
    if ($missing.Count -gt 0) { throw "shard references unknown scenarios: $($missing -join ', ')" }
    $nullIds = @($selected | Where-Object { -not $_ } | ForEach-Object { 'null' })
    if ($nullIds.Count -gt 0) { throw "shard references unresolved scenario entries" }
    $selectedDisks = $matrixDisks | Where-Object { $shardDisks -contains $_.id }
} else {
    $selected = $matrixScenarios
    $selectedDisks = $matrixDisks
}

# a shard-local matrix file for New-TestDisk (it reads matrix.disks only)
$shardMatrixPath = Join-Path $WorkDir 'shard-matrix.json'
[ordered]@{ version = 1; disks = $selectedDisks } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $shardMatrixPath -Encoding utf8

Write-Host "=== tests/e2e: profile=$Profile shard=$(if ($ShardId) { $ShardId } else { 'ALL' }) scenarios=$(@($selected).Count) api=$IncludeApi ===" -ForegroundColor Cyan

# ---- phase 0: the disk environment ------------------------------------------------------
Write-Host '=== phase 0: virtual disk environment ===' -ForegroundColor Cyan
try {
    if (-not (Test-Path $DiskStatePath)) {
        $p = @{ MatrixPath = $shardMatrixPath; WorkRoot = $WorkRoot; OutJson = $DiskStatePath }
        & $NewDiskScript @p
        if ($LASTEXITCODE -ne 0) { throw 'New-TestDisk failed (diagnostics above are the artifact)' }
    }
} catch {
    # the cannot-execute contract: a failed VHD setup fails the shard WITH its
    # diagnostics recorded in the results tree - never a silent skip
    $diagDir = Join-Path $ResultsRoot '02-disk-setup'
    New-Item -ItemType Directory -Force -Path $diagDir | Out-Null
    "disk environment creation failed: $($_.Exception.Message)" | Set-Content (Join-Path $diagDir 'FAILURE.txt') -Encoding utf8
    try {
        diskpart 2>&1 | Select-Object -Last 30 | Set-Content (Join-Path $diagDir 'diskpart-interactive-tail.txt') -Encoding utf8
        Get-Disk | Select-Object Number, FriendlyName, OperationalStatus, PartitionStyle, Size |
            ConvertTo-Json -Depth 3 | Set-Content (Join-Path $diagDir 'get-disk.json') -Encoding utf8
        Get-Volume | Select-Object DriveLetter, FileSystem, HealthStatus, Size |
            ConvertTo-Json -Depth 3 | Set-Content (Join-Path $diagDir 'get-volume.json') -Encoding utf8
    } catch { }
    Fail "phase 0 failed: $($_.Exception.Message)"
    exit 1
}
$DiskState = Get-Content -LiteralPath $DiskStatePath -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Force -Path (Join-Path $ResultsRoot '02-disk-setup') | Out-Null
Copy-Item $DiskStatePath (Join-Path $ResultsRoot '02-disk-setup\disk-state.json') -Force

# ---- seeding ----------------------------------------------------------------------------
$script:ManifestPath = @{}    # volume label -> manifest path (the per-run ground truth)
$script:FirstSeeded = @{}
$script:VolumeFs = @{}        # volume label -> filesystem
$script:VolumeBytes = @{}

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
    # generate_seed signals failure by THROWING (its own EAP=Stop) - the
    # caller's catch owns it; $LASTEXITCODE would only hold a stale native
    # exit here (the run-6 trap class)
    $script:ManifestPath[$VolEntry.label] = $manifest
    $script:FirstSeeded[$VolEntry.label] = $true
    $script:VolumeFs[$VolEntry.label] = [string]$VolEntry.fs
    $script:VolumeBytes[$VolEntry.label] = [long]$VolEntry.bytes
    New-Item -ItemType Directory -Force -Path (Join-Path $ResultsRoot '03-baseline') | Out-Null
    Copy-Item $manifest (Join-Path $ResultsRoot "03-baseline\seed-manifest-$($VolEntry.label).jsonl") -Force
    $count = @(Get-ChildItem -LiteralPath "$letter`:\" -Recurse -File -Force -ErrorAction SilentlyContinue).Count
    Write-Host ("  seeded {0}: {1} ({2} files)" -f $VolEntry.label, $SeedProfile, $count)
}

# reseed with volume-restore. Two distinct absent-volume states (run
# 45874368709 exFAT PARTDEL - the last lesson): (a) after PARTOVR's
# locked-volume raw IO the volume is DISMOUNTED/OFFLINE with the partition
# INTACT (Get-Volume reports no FileSystem) - re-attaching the access path
# restores it; blindly recreating the partition then hits 'The requested
# access path is already in use' on the still-assigned letter; (b) after
# delete_partition the partition is TRULY gone - recreate it (defense-in-
# depth; the deterministic order makes PARTDEL last on every disk).
function Invoke-Reseed {
    param($VolEntry, $MatrixVol, $DiskEntry)
    $letter = [string]$VolEntry.letter
    $diskNumber = [int]$DiskEntry.disk_number
    $partNumber = [int]$VolEntry.partition
    $vol = Get-Volume -DriveLetter $letter -ErrorAction SilentlyContinue
    $fsOk = ($vol -and -not [string]::IsNullOrEmpty([string]$vol.FileSystem))
    if (-not $fsOk) {
        $part = Get-Partition -DiskNumber $diskNumber -PartitionNumber $partNumber -ErrorAction SilentlyContinue
        if ($part) {
            # state (a): the partition is intact, the volume is offline
            Write-Host "  volume ${letter}: offline with the partition intact (post-locked-raw-IO state) - re-attaching the access path"
            try {
                Add-PartitionAccessPath -DiskNumber $diskNumber -PartitionNumber $partNumber `
                    -AccessPath "$letter`:\" -ErrorAction Stop
            } catch {
                Write-Host "  re-attach: $($_.Exception.Message) (recorded - waiting for the volume to settle)"
            }
            for ($w = 0; $w -lt 20; $w++) {
                $probe = Get-Volume -DriveLetter $letter -ErrorAction SilentlyContinue
                if ($probe -and -not [string]::IsNullOrEmpty([string]$probe.FileSystem) -and $probe.Size -gt 0) { $fsOk = $true; break }
                Start-Sleep -Milliseconds 500
            }
        }
    }
    if (-not $fsOk) {
        # state (b): the partition is truly gone - recreate it. CONFIRM the
        # absence first: a still-offline-but-intact volume must FAIL honestly,
        # never be formatted blind (ground-truth destruction risk)
        $part2 = Get-Partition -DiskNumber $diskNumber -PartitionNumber $partNumber -ErrorAction SilentlyContinue
        if ($part2) {
            throw "volume ${letter}: offline with the partition intact and it did not come back after the access-path re-attach - refusing to reseed (no blind format)"
        }
        Write-Host "  volume ${letter}: absent (partition removed by an earlier scenario) - recreating partition before reseed"
        $part = New-Partition -DiskNumber $diskNumber -UseMaximumSize -DriveLetter $letter -ErrorAction Stop
        $null = Format-Volume -Partition $part -FileSystem $VolEntry.fs -NewFileSystemLabel $VolEntry.label -Force -ErrorAction Stop
        for ($w = 0; $w -lt 20; $w++) {
            $probe = Get-Volume -DriveLetter $letter -ErrorAction SilentlyContinue
            if ($probe -and $probe.FileSystem -eq $VolEntry.fs -and $probe.Size -gt 0) { break }
            Start-Sleep -Milliseconds 500
        }
    }
    try { Clear-RecycleBin -DriveLetter $letter -Force -ErrorAction SilentlyContinue } catch { }
    try { Write-VolumeCache -DriveLetter $letter -ErrorAction SilentlyContinue | Out-Null } catch { }
    $seedProfile = [string]$MatrixVol.seed_profile
    if ($seedProfile -eq 'follow-run-profile') { $seedProfile = $Profile }
    Invoke-Seed $VolEntry $seedProfile
}

try {
    foreach ($d in $DiskState.disks) {
        $matrixDisk = $matrixDisks | Where-Object { $_.id -eq $d.id }
        foreach ($v in $d.volumes) {
            $mv = $matrixDisk.volumes | Where-Object { $_.label -eq $v.label }
            $seedProfile = [string]$mv.seed_profile
            if ($seedProfile -eq 'follow-run-profile') { $seedProfile = $Profile }
            Invoke-Seed $v $seedProfile
        }
    }
} catch {
    $diagDir = Join-Path $ResultsRoot '03-baseline'
    New-Item -ItemType Directory -Force -Path $diagDir | Out-Null
    "seeding failed: $($_.Exception.Message)" | Set-Content (Join-Path $diagDir 'FAILURE.txt') -Encoding utf8
    Fail "phase 0 seeding failed: $($_.Exception.Message)"
    exit 1
}

# ---- the scenario loop -------------------------------------------------------------------
Write-Host "=== phase 1: scenario matrix (profile=$Profile) ===" -ForegroundColor Cyan
$scenarioVerdicts = @()
$volumeSeen = @{}

function Resolve-Volume {
    param([string] $DiskId, [string] $VolumeLabel)
    $disk = $DiskState.disks | Where-Object { $_.id -eq $DiskId }
    if (-not $disk) { throw "matrix references unknown disk '$DiskId'" }
    $vol = $disk.volumes | Where-Object { $_.label -eq $VolumeLabel }
    if (-not $vol) { throw "disk '$DiskId' has no volume labeled '$VolumeLabel'" }
    return @{ disk = $disk; vol = $vol }
}

foreach ($sc in $selected) {
    $id = [string]$sc.id
    Write-Host ""
    Write-Host "--- scenario ${id} ($($sc.mode)) ---" -ForegroundColor Magenta
    $scDir = Join-Path $WorkDir "scenarios\$id"
    $scCli = Join-Path $scDir 'cli'
    $scApi = Join-Path $scDir 'api'
    $scEv = Join-Path $ResultsRoot "04-scenarios\$id"
    New-Item -ItemType Directory -Force -Path $scCli, $scApi, $scEv | Out-Null
    $verdict = [ordered]@{
        scenario = $id; mode = $sc.mode; profile = $Profile
        gate_type = $sc.gate.type; gate_note = $sc.gate.note
        lanes = @{}; passed = $false; error = $null
    }

    try {
        $rv = Resolve-Volume -DiskId $sc.disk -VolumeLabel $sc.volume
        $vol = $rv.vol; $diskEntry = $rv.disk
        $matrixDisk = $matrixDisks | Where-Object { $_.id -eq $sc.disk }
        $matrixVol = $matrixDisk.volumes | Where-Object { $_.label -eq $sc.volume }
        $letter = [string]$vol.letter
        $diskNumber = [int]$diskEntry.disk_number

        # deterministic reseed (every scenario starts from the same corpus)
        if ($volumeSeen.ContainsKey($vol.label)) {
            Write-Host "  reseeding $($vol.label) (deterministic corpus restore)"
            Invoke-Reseed $vol $matrixVol $diskEntry
        } else {
            $volumeSeen[$vol.label] = $true
        }
        $manifest = $script:ManifestPath[$vol.label]

        # apply the deletion scenario (the ONLY mutation of ground truth)
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
            # the access-path detach before the partition removal lives
            # INSIDE apply-scenario.ps1 (after its pre-deletion hash map -
            # a Run-E2E-level pre-step detached the letter before the hash
            # capture and broke it: 'OpenVolumeRaw \\.\Z: failed GLE=2',
            # run 45866040158 fat32-b/refs-b)
        }
        & $ScenarioScript @p
        # apply-scenario signals failure by THROWING (its own EAP=Stop);
        # delete_partition's diskpart fallback owns its own exit check


        # ---- the unsupported-format path (expect=unsupported_fs) -------------------------
        # The row pins the product's honest behavior on a filesystem its volume
        # scan refuses (the prompt's 'every unsupported-format path' coverage):
        # the environment is REAL (disk + seed + deletion all happen); the scan
        # must refuse CLEANLY - exit 1, the error surfaced, NO session, NO
        # crash - and that clean refusal is the gate. Recorded alongside it:
        # the capability FINDING (see the matrix note + worklog).
        if ([string]$sc.expect -eq 'unsupported_fs') {
            Write-Host "  unsupported-format probe: the scan is EXPECTED to refuse cleanly"
            $refusalDir = Join-Path $scEv 'cli'
            New-Item -ItemType Directory -Force -Path $refusalDir | Out-Null
            $sessionPath = Join-Path $scCli 'session-quick.crsf'
            $isPartDel = ([string]$sc.mode -eq 'delete_partition')
            if ($isPartDel) {
                # after delete_partition the VOLUME is gone (run 45845188954:
                # --volume on the removed letter refused in the BitLocker
                # state probe, not the FS probe). The honest unsupported-FS
                # shape on a partition-deleted disk is the WHOLE-DISK scan
                # completing (exit 0 + session) and finding ZERO lost
                # partitions - the ReFS lost-partition discovery gap (the
                # run 45826003339 refs-b evidence: completed, lost=0).
                $scanArgs = @('scan', '--disk', "$diskNumber", '--deep', '--threads', "$Threads",
                              '--out', $sessionPath, '--log', (Join-Path $scCli 'stage-whole.log'), '--json')
            } else {
                $scanArgs = @('scan', '--volume', "$letter`:", '--threads', "$Threads", '--quick-only',
                              '--out', $sessionPath, '--log', (Join-Path $scCli 'stage-quick.log'), '--json')
            }
            $t0 = Get-Date
            $pinfo = Start-Process -FilePath $RrecovExe -ArgumentList $scanArgs `
                -RedirectStandardOutput (Join-Path $refusalDir 'scan-quick.stdout') `
                -RedirectStandardError (Join-Path $refusalDir 'scan-quick.stderr') `
                -PassThru -NoNewWindow -Wait
            $cliExit = [int]$pinfo.ExitCode
            $cliWall = [math]::Round(((Get-Date) - $t0).TotalSeconds, 2)
            $cliOut = ''
            if (Test-Path -LiteralPath (Join-Path $refusalDir 'scan-quick.stdout')) {
                $cliOut = (Get-Content -LiteralPath (Join-Path $refusalDir 'scan-quick.stdout') -Raw)
            }
            if ($isPartDel) {
                # honest-empty gate: completed + session + zero lost partitions
                $jline = ($cliOut -split "`n" | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1)
                $lost = -1
                if ($jline) { try { $lost = [int64](($jline | ConvertFrom-Json).lost_partitions) } catch { } }
                $cliRefused = ($cliExit -eq 0) -and (Test-Path -LiteralPath $sessionPath) -and ($lost -eq 0)
                $cliExpected = 'completed whole-disk scan, session produced, lost_partitions == 0 (the ReFS discovery gap)'
            } else {
                $cliRefused = ($cliExit -eq 1) -and ($cliOut -match 'bad magic|0x50030001') -and (-not (Test-Path -LiteralPath $sessionPath))
                $cliExpected = 'clean refusal (exit 1 + bad magic surfaced + no session)'
            }
            $apiExitRef = $null
            $apiRefused = $false
            if ($IncludeApi) {
                $apiRefDir = Join-Path $scEv 'api'
                New-Item -ItemType Directory -Force -Path $apiRefDir | Out-Null
                # capture into a variable AND write the evidence file (run
                # 45836204263: Tee'd to the evidence dir but read back from
                # the work dir - the refusal text was never seen and every
                # API probe false-failed; one source of truth from here on)
                $apiTargets = @('--scan', '--target', 'volume', '--letter', $letter,
                    '--dest', (Join-Path $scApi 'recovered'), '--out', $scApi, '--label', $id,
                    '--threads', "$Threads")
                if ($isPartDel) {
                    $apiTargets = @('--scan', '--target', 'disk', '--disk', "$diskNumber", '--whole-disk',
                        '--dest', (Join-Path $scApi 'recovered'), '--out', $scApi, '--label', $id,
                        '--threads', "$Threads")
                }
                $apiOutLines = & $ApiHarness @apiTargets 2>&1
                $apiOutLines | Set-Content -LiteralPath (Join-Path $apiRefDir 'harness.stdout') -Encoding utf8
                $apiExitRef = $LASTEXITCODE
                $apiOut = ($apiOutLines -join "`n")
                if ($isPartDel) {
                    # honest-empty: harness completes (0 or 2) and reports zero lost partitions
                    $apiLost = $null
                    $apiResultPathLocal = Join-Path $scApi 'api-result.json'
                    if (Test-Path -LiteralPath $apiResultPathLocal) {
                        try { $apiLost = [int64](Get-Content -LiteralPath $apiResultPathLocal -Raw | ConvertFrom-Json).scan_stats.lost_partitions } catch { }
                    }
                    $apiRefused = ($apiExitRef -eq 0 -or $apiExitRef -eq 2) -and ($null -ne $apiLost) -and ($apiLost -eq 0)
                } else {
                    $apiRefused = ($apiExitRef -ne 0) -and ($apiOut -match 'bad magic|0x50030001|50030001')
                }
            }
            $resCli = [ordered]@{
                expected = $cliExpected
                exit_code = $cliExit; wall_seconds = $cliWall
                passed = $cliRefused; reasons = @()
            }
            if (-not $cliRefused) {
                $resCli.reasons += "CLI refusal shape wrong: exit=$cliExit session=$(Test-Path -LiteralPath $sessionPath) badmagic=$($cliOut -match 'bad magic|0x50030001')"
            }
            $resApi = [ordered]@{ expected = 'the same shape as the CLI lane'; exit_code = $apiExitRef; passed = $true; reasons = @() }
            if ($IncludeApi) {
                $resApi.passed = $apiRefused
                if (-not $apiRefused) { $resApi.reasons += "API refusal shape wrong: exit=$apiExitRef" }
            } else {
                $resApi.reasons += 'skipped: smoke profile is CLI-only by design (recorded)'
            }
            $verdict.lanes = @{ cli = $resCli; api = $resApi }
            $verdict.passed = $resCli.passed -and (($resApi.passed) -or (-not $IncludeApi))
            $verdict.finding = [string]$sc.gate.note
            if (-not $verdict.passed) {
                Fail "scenario ${id}: the unsupported-format refusal was NOT clean - see 04-scenarios/$id"
            } else {
                Write-Host ("PASS {0}: clean refusal (CLI exit {1}, API exit {2}) - the finding is recorded, not masked" -f $id, $cliExit, $apiExitRef)
            }
            [ordered]@{
                scenario = $id; mode = $sc.mode; filesystem = [string]$vol.fs
                expect = 'unsupported_fs'
                disk = [string]$sc.disk; volume = [string]$sc.volume
                deletion_applied = $true
                seed_manifest = (Join-Path '03-baseline' "seed-manifest-$($vol.label).jsonl")
                seed_manifest_sha256 = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLower()
                finding = [string]$sc.gate.note
            } | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'scenario.json') -Encoding utf8
            [ordered]@{ scenario = $id; checked = $false; reason = 'unsupported-format probe: nothing is recovered by construction; the refusal shape is the gate' } |
                ConvertTo-Json -Depth 4 | Set-Content (Join-Path $scEv 'divergence.json') -Encoding utf8
            $verdict | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'verdict.json') -Encoding utf8
            $scenarioVerdicts += $verdict
            continue
        }

        # the S-PART-DELETE H1 settle guard (raw whole-disk opens can hit the
        # PnP reconfiguration window milliseconds after a table mutation)
        if ($sc.mode -eq 'delete_partition') {
            $settleDeadline = (Get-Date).AddSeconds(30)
            $snapPrev = $null
            while ((Get-Date) -lt $settleDeadline) {
                $snap = @(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue |
                    Sort-Object PartitionNumber |
                    ForEach-Object { "$($_.PartitionNumber):$($_.StartingOffset):$($_.Size)" }) -join '|'
                if ($null -ne $snapPrev -and $snap -eq $snapPrev) { break }
                $snapPrev = $snap
                Start-Sleep -Milliseconds 500
            }
            Start-Sleep -Seconds 1
            Write-Host "  table settle: partitions=$(@(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue).Count)"
        }

        $scenarioStatePath = Join-Path $scStateDir 'scenario-state.json'
        Copy-Evidence @($scenarioStatePath, (Join-Path $scStateDir 'hashmap-pre.csv'), (Join-Path $scStateDir 'hashmap-post.csv'), (Join-Path $scStateDir 'hashmap-post-disk.csv')) (Join-Path $scEv 'state')

        # the CLI lane
        $laneModes = if ([string]$sc.scan_modes -eq 'all') { $ScanModes } else { @([string]$sc.scan_modes -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ }) }
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
            DriverInfo = 'transport=direct'
            RecoverCapSec = $RecoverCapSec
            TargetMode = [string]$sc.target_mode
        }
        & $RunOursScript @p
        # run-ours signals failure by THROWING (its own diagnostics-carrying
        # throw on a sessionless scan - the no-silent-skip contract)
        $cliRunPath = Join-Path $scCli 'ours-run.json'

        # the API lane (the product surface)
        $apiExit = $null
        $apiResultPath = Join-Path $scApi 'api-result.json'
        if ($IncludeApi) {
            $apiDest = Join-Path $scApi 'recovered'
            $apiArgs = @('--scan', '--target', 'volume', '--letter', $letter, '--dest', $apiDest, '--out', $scApi, '--label', $id, '--threads', "$Threads")
            if ([string]$sc.target_mode -eq 'disk') {
                $apiArgs = @('--scan', '--target', 'disk', '--disk', "$diskNumber", '--whole-disk', '--dest', $apiDest, '--out', $scApi, '--label', $id, '--threads', "$Threads")
            }
            & $ApiHarness @apiArgs 2>&1 | Tee-Object -FilePath (Join-Path $scApi 'harness.stdout') | Write-Host
            $apiExit = $LASTEXITCODE
        } else {
            Note "API lane skipped for ${id}: smoke profile is CLI-only by design (recorded reason, prompt contract)"
        }

        # inventories + metrics for BOTH lanes (ground truth = manifest + method)
        $cliRecovered = Join-Path $scCli 'recovered-deepcarve'
        if (-not (Test-Path $cliRecovered)) { $cliRecovered = Join-Path $scCli "recovered-$(@($laneModes)[-1])" }
        $invCli = Join-Path $scCli 'recovered-inventory.json'
        $invApi = Join-Path $scApi 'recovered-inventory.json'
        if (Test-Path $cliRecovered) {
            python $InvScript --root $cliRecovered --manifest $manifest --out $invCli --tool ours --label "cli-$id" |
                Tee-Object -FilePath (Join-Path $scCli 'inventory.stdout') | Write-Host
            if ($LASTEXITCODE -ne 0) { throw "CLI inventory failed for $id (exit $LASTEXITCODE)" }
        }
        if ($IncludeApi -and (Test-Path (Join-Path $scApi 'recovered'))) {
            python $InvScript --root (Join-Path $scApi 'recovered') --manifest $manifest --out $invApi --tool ours --label "api-$id" |
                Tee-Object -FilePath (Join-Path $scApi 'inventory.stdout') | Write-Host
            if ($LASTEXITCODE -ne 0) { throw "API inventory failed for $id (exit $LASTEXITCODE)" }
        }
        # per-mode metrics for the CLI lane + the headline for the API lane
        foreach ($m in $laneModes) {
            if (Test-Path $invCli) {
                python $MetricsScript --manifest $manifest --scenario $scenarioStatePath --inventory $invCli `
                    --outdir $scCli --tool ours --fs ([string]$vol.fs) --mode $m --outname "metrics-$m.json" 2>&1 |
                    Select-Object -Last 2 | Write-Host
            }
        }
        if ($IncludeApi -and (Test-Path $invApi)) {
            python $MetricsScript --manifest $manifest --scenario $scenarioStatePath --inventory $invApi `
                --outdir $scApi --tool ours --fs ([string]$vol.fs) --mode deepcarve --outname 'metrics-deepcarve.json' 2>&1 |
                Select-Object -Last 2 | Write-Host
        }

        # CLI-vs-API divergence (gate 5: both paths must agree)
        $divergence = $null
        if ($IncludeApi) {
            $p = @{
                ScenarioId = $id
                CliDir = $scCli; ApiDir = $scApi
                CliRunJson = $cliRunPath; ApiResultJson = $apiResultPath
                CliInventory = $invCli; ApiInventory = $invApi
                OutJson = (Join-Path $scEv 'divergence.json')
            }
            $divergence = & $CompareLanesScript @p
            if ($LASTEXITCODE -ne 0) { Fail "scenario ${id}: CLI-vs-API divergence - see 04-scenarios/$id/divergence.json" }
        } else {
            [ordered]@{ scenario = $id; checked = $false; reason = 'smoke profile is CLI-only by design (recorded reason)' } |
                ConvertTo-Json -Depth 4 | Set-Content (Join-Path $scEv 'divergence.json') -Encoding utf8
        }

        # gate evaluation
        $cliRun = $null
        if (Test-Path $cliRunPath) { $cliRun = Get-Content $cliRunPath -Raw | ConvertFrom-Json }
        $apiResult = $null
        if (Test-Path $apiResultPath) { $apiResult = Get-Content $apiResultPath -Raw | ConvertFrom-Json }
        $headline = 'deepcarve'
        $cliMetricsPath = Join-Path $scCli "metrics-$headline.json"
        if (-not (Test-Path $cliMetricsPath)) { $cliMetricsPath = Join-Path $scCli "metrics-$(@($laneModes)[-1]).json" }
        $cliMetrics = $null
        if (Test-Path $cliMetricsPath) { $cliMetrics = Get-Content $cliMetricsPath -Raw | ConvertFrom-Json }

        $laneResults = @{}
        foreach ($lane in 'cli', 'api') {
            $res = [ordered]@{
                recall_hash = $null; precision = $null; f1 = $null
                expected_deleted = $null; hash_matched = $null
                passed = $true; reasons = @()
            }
            $m = if ($lane -eq 'cli') { $cliMetrics } else { $null }
            if ($IncludeApi -and $lane -eq 'api') {
                $apiMetricsPath = Join-Path $scApi 'metrics-deepcarve.json'
                if (Test-Path $apiMetricsPath) { $m = Get-Content $apiMetricsPath -Raw | ConvertFrom-Json }
            }
            if ($m) {
                $res.recall_hash = [math]::Round([double]$m.recall, 4)
                $res.precision = if ($null -ne $m.precision) { [math]::Round([double]$m.precision, 4) } else { $null }
                $res.f1 = [math]::Round([double]$m.f1, 4)
                $res.expected_deleted = $m.ground_truth_files
                $res.hash_matched = $m.recovered_hash_ok
            }
            if ($lane -eq 'api' -and -not $IncludeApi) {
                $res.reasons += 'skipped: smoke profile is CLI-only by design (recorded)'
            }
            if ($lane -eq 'cli') {
                if ($cliRun) {
                    foreach ($mm in @($cliRun.scan_modes)) {
                        if ($null -ne $mm.exit_code -and [int]$mm.exit_code -ne 0) {
                            $res.passed = $false
                            $res.reasons += "scan mode $($mm.mode) exited $($mm.exit_code)$(if ($mm.capped) { ' (capped at budget)' })"
                        }
                        if ($mm.recover -and $mm.recover.exit_code -notin @(0, 2)) {
                            $res.passed = $false
                            $res.reasons += "recover mode $($mm.mode) exited $($mm.recover.exit_code) (0 ok / 2 partial are the honest shapes)"
                        }
                    }
                } else {
                    $res.passed = $false
                    $res.reasons += 'no ours-run.json - the CLI lane did not complete'
                }
                # the floor gate (CLI headline metrics)
                if ($sc.gate.type -eq 'floor' -and $m) {
                    $floor = [double]$sc.gate.floor_hash_recall
                    $res.floor = $floor
                    if ([double]$m.recall -lt $floor) {
                        $res.passed = $false
                        $res.reasons += "hash-recall $($m.recall) below floor $floor"
                    }
                }
            }
            if ($lane -eq 'api' -and $IncludeApi) {
                if ($apiExit -ne 0 -and $apiExit -ne 2) {
                    $res.passed = $false
                    $res.reasons += "api harness exit $apiExit (expected 0 or 2)"
                }
                if (-not $apiResult -or [string]$apiResult.transport -ne 'direct') {
                    $res.passed = $false
                    $res.reasons += "api transport was '$(if ($apiResult) { $apiResult.transport } else { '<no api-result>' })' (expected direct)"
                }
            }
            # discovery gate (both lanes): lost partition must be found
            if ($sc.gate.type -eq 'discovery') {
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
            }
            $laneResults[$lane] = $res
        }
        $verdict.lanes = $laneResults
        $verdict.passed = $laneResults.cli.passed -and (($laneResults.api.passed) -or (-not $IncludeApi))
        if (-not $verdict.passed) {
            $why = @()
            foreach ($k in $laneResults.Keys) { if (-not $laneResults[$k].passed) { $why += "$k`: $($laneResults[$k].reasons -join '; ')" } }
            Fail "scenario ${id} FAILED the '$($sc.gate.type)' gate - $($why -join ' | ')"
        } else {
            Write-Host ("PASS {0}: cli recall {1}, api recall {2}" -f $id, $laneResults.cli.recall_hash, $laneResults.api.recall_hash) -ForegroundColor Green
        }

        # post-actions: the lost-partition APPLY probe (S-029 feature)
        if ($sc.gate.post_actions -contains 'apply_probe' -or ($sc.mode -eq 'delete_partition')) {
            Write-Host '  post-action: lost-partition APPLY probe'
            $applyEv = @()
            $partBefore = @(Get-Partition -DiskNumber $diskNumber -ErrorAction SilentlyContinue).Count
            $sessionPath = Join-Path $scCli 'session-deepcarve.crsf'
            if (Test-Path $sessionPath) {
                $dryOut = & $RrecovExe apply --disk $diskNumber --session $sessionPath --candidate 1 2>&1
                $dryExit = $LASTEXITCODE
                $applyEv += "DRY-RUN exit ${dryExit}:" + ($dryOut -join ' | ')
                if ($dryExit -ne 0) {
                    Fail "apply dry-run exited $dryExit (expected 0 - a plan or an honest REFUSED both exit 0)"
                } else {
                    $refused = (($dryOut -join ' ') -match 'REFUSED|refused')
                    if (-not $refused) {
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
                            Note "apply probe: real apply exit $realExit, partitions $partBefore -> $partAfter (recorded)"
                        }
                    } else {
                        Note 'apply probe: planner REFUSED (recorded - an honest refusal is a valid capability state)'
                    }
                }
            } else {
                Note 'apply probe: no deepcarve session to plan from (recorded)'
            }
            $applyEv | Set-Content (Join-Path $scEv 'apply-probe.txt') -Encoding utf8
        }

        # evidence + payload cleanup (samples kept, gigabytes dropped)
        Copy-Evidence @(
            $cliRunPath,
            (Join-Path $scCli 'scan-deepcarve.stdout'), (Join-Path $scCli 'scan-deepcarve.stderr'),
            (Join-Path $scCli 'scan-quick.stdout'), (Join-Path $scCli 'scan-quick.stderr'),
            (Join-Path $scCli 'scan-deep.stdout'), (Join-Path $scCli 'scan-deep.stderr'),
            (Join-Path $scCli 'stage-deepcarve.log'),
            (Join-Path $scCli 'tree-deepcarve.json'), (Join-Path $scCli 'tree-deepcarve.csv'),
            (Join-Path $scCli 'report-deepcarve.json'), (Join-Path $scCli 'report-quick.json'), (Join-Path $scCli 'report-deep.json'),
            (Join-Path $scCli 'inventory.stdout'), (Join-Path $scCli 'recovered-inventory.json'),
            (Join-Path $scCli 'metrics-quick.json'), (Join-Path $scCli 'metrics-deep.json'), (Join-Path $scCli 'metrics-deepcarve.json'),
            (Join-Path $scCli 'preservation.json'), (Join-Path $scCli 'samples-deepcarve.csv'),
            (Join-Path $scCli 'sample-recovered')
        ) (Join-Path $scEv 'cli')
        Copy-Evidence @(
            $apiResultPath, (Join-Path $scApi 'report.json'), (Join-Path $scApi 'harness.stdout'),
            (Join-Path $scApi 'session.crsf'), (Join-Path $scApi 'recovered-inventory.json'),
            (Join-Path $scApi 'inventory.stdout'), (Join-Path $scApi 'metrics-deepcarve.json')
        ) (Join-Path $scEv 'api')
        # the scenario row for the results schema
        [ordered]@{
            scenario = $id; mode = $sc.mode; filesystem = [string]$vol.fs
            disk = [string]$sc.disk; volume = [string]$sc.volume
            target_mode = [string]$sc.target_mode; scan_modes = [string]$sc.scan_modes
            gate = $sc.gate
            seed_manifest = (Join-Path '03-baseline' "seed-manifest-$($vol.label).jsonl")
            seed_manifest_sha256 = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLower()
            scenario_state = (Test-Path $scenarioStatePath)
        } | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'scenario.json') -Encoding utf8
        $verdict | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'verdict.json') -Encoding utf8
        $scenarioVerdicts += $verdict

        foreach ($rd in @(Get-ChildItem $scCli -Directory -Filter 'recovered-*' -ErrorAction SilentlyContinue)) {
            Remove-Item -LiteralPath $rd.FullName -Recurse -Force -ErrorAction SilentlyContinue
        }
        Remove-Item -LiteralPath (Join-Path $scApi 'recovered') -Recurse -Force -ErrorAction SilentlyContinue
    } catch {
        # EVIDENCE FIRST: archive every diagnostic the lanes wrote before the throw
        foreach ($lanePair in @(@($scCli, 'cli'), @($scApi, 'api'))) {
            $laneDir = $lanePair[0]; $laneName = $lanePair[1]
            if (Test-Path -LiteralPath $laneDir) {
                $diags = Get-ChildItem -LiteralPath $laneDir -File -ErrorAction SilentlyContinue |
                    Where-Object { $_.Name -match '^(scan-|recover-|stage-|samples-|tree-|report-|metrics|preservation|ours-run|harness\.stdout|api-result\.json)' }
                foreach ($df in $diags) { Copy-Evidence @($df.FullName) (Join-Path $scEv $laneName) }
            }
        }
        $verdict.error = "exception: $($_.Exception.Message)"
        $verdict.passed = $false
        Fail "scenario ${id} threw: $($_.Exception.Message)"
        $verdict | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $scEv 'verdict.json') -Encoding utf8
        $scenarioVerdicts += $verdict
    }
}

# ---- the shard verdict --------------------------------------------------------------------
Write-Host '=== phase 2: shard summary ===' -ForegroundColor Cyan
$summary = [ordered]@{
    suite = 'tests/e2e'
    profile = $Profile; shard = $ShardId
    captured_utc = (Get-Date).ToUniversalTime().ToString('o')
    runId = $RunId; runUrl = $RunUrl
    scan_modes = $ScanModes; include_api = $IncludeApi
    scenarios = @($scenarioVerdicts | ForEach-Object {
        [ordered]@{
            id = $_.scenario; mode = $_.mode; gate = $_.gate_type; passed = $_.passed
            cli = $_.lanes.cli; api = $_.lanes.api; error = $_.error
        }
    })
    soft_records = $script:SoftRecords
    hard_failures = $script:HardFailures
}
$summary | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $ResultsRoot 'summary.json') -Encoding utf8

$lines = @('# e2e shard gate summary', '', '| scenario | gate | cli | api | verdict |', '|---|---|---|---|---|')
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
$lines | Set-Content (Join-Path $ResultsRoot 'gates.md') -Encoding utf8
$lines | Write-Host

# ---- teardown (always; recorded into the results tree) -------------------------------------
$teardownEv = Join-Path $ResultsRoot '99-teardown'
New-Item -ItemType Directory -Force -Path $teardownEv | Out-Null
try {
    & $TeardownScript -ReleaseDir (Join-Path $WorkRoot 'release') -DiskStatePath $DiskStatePath -EvidenceDir $teardownEv
} catch {
    Note "teardown threw (recorded): $($_.Exception.Message)"
}

if ($script:HardFailures.Count -gt 0) {
    Write-Host "::error::$($script:HardFailures.Count) hard gate failure(s) - see $($ResultsRoot)\gates.md" -ForegroundColor Red
    exit 1
}
Write-Host 'ALL HARD GATES GREEN' -ForegroundColor Green
exit 0
