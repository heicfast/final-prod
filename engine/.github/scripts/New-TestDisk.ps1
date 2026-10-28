# New-TestDisk.ps1 - create the simulated virtual-disk environment for the
# test-benchmark lane from the versioned matrix (.github/scenarios/benchmark-matrix.json).
#
# Design decisions (each paid for by a recorded failure):
#   - DISKPART-ONLY for vdisk create/attach/detach: New-VHD/Mount-VHD need
#     the Hyper-V PowerShell module, whose availability on GitHub runner
#     images is not guaranteed (bench/volume/volume.ps1 carries a fallback
#     for exactly this reason). diskpart is present on every Windows image;
#     ONE code path = deterministic behavior, and the method is recorded
#     in the output JSON anyway.
#   - FIXED-size VHDX: deterministic footprint (no dynamic-growth failure
#     mid-run - the recorded D:-drive headroom class), and the total
#     matrix footprint (~15 GiB) fits the probed work drive.
#   - Disk numbers are resolved by BEFORE/AFTER DIFF of Get-Disk numbers
#     (never "the highest number" - a second attach or a race would grab
#     the wrong disk; stage-disk.ps1's proven pattern).
#   - Drive letters are EXPLICIT (-DriveLetter), taken from the matrix
#     letter_hint after asserting they are free; no auto-assignment races.
#   - The work drive is PROBED at runtime (free space), never hardcoded:
#     C:/D: layouts differ across runner generations and the recorded
#     failure ("dynamic-VHDX growth failure -> moved to C:\bench") came
#     from assuming the wrong drive had headroom.
#   - MBR and GPT both supported (the matrix exercises both partition
#     styles); multiple volumes per disk (the multi-partition matrix disk).
#
# Output: a merged disk-state.json the orchestrator and teardown consume:
#   { "work_root": ..., "disks": [ { "id", "vhd", "disk_number",
#     "partition_style", "size_mb", "volumes": [ { "label", "fs", "letter",
#     "partition", "bytes" } ] } ] }
#
# Licensed under the MIT license. See LICENSE.
param(
    [Parameter(Mandatory = $true)] [string] $MatrixPath,
    [Parameter(Mandatory = $true)] [string] $WorkRoot,   # disks land in <WorkRoot>\disks
    [Parameter(Mandatory = $true)] [string] $OutJson
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

function Invoke-Diskpart {
    param([string[]] $Lines, [string] $What)
    $tmp = [System.IO.Path]::GetTempFileName()
    ($Lines -join "`r`n") | Set-Content -Path $tmp -Encoding ascii
    $out = diskpart /s $tmp 2>&1
    $code = $LASTEXITCODE
    Remove-Item $tmp -Force -ErrorAction SilentlyContinue
    if ($code -ne 0) {
        Write-Host "::error::diskpart $What failed (exit $code):"
        $out | Select-Object -Last 15 | ForEach-Object { Write-Host "  $_" }
        throw "diskpart $What failed"
    }
    return $out
}

function Test-LetterFree {
    param([char] $Letter)
    $l = "$Letter`:"
    if (Get-PSDrive -Name $Letter -ErrorAction SilentlyContinue) { return $false }
    if (Get-Volume -DriveLetter $Letter -ErrorAction SilentlyContinue) { return $false }
    return $true
}

function Get-FreeLetter {
    param([char] $Preferred)
    if (Test-LetterFree $Preferred) { return $Preferred }
    foreach ($c in 'U', 'V', 'T', 'S', 'R', 'Q', 'P', 'O', 'N') {
        if (Test-LetterFree $c) { return $c }
    }
    throw 'no free drive letters in the probe range'
}

# ---- load the matrix + probe the work drive ------------------------------------

$matrix = Get-Content -LiteralPath $MatrixPath -Raw | ConvertFrom-Json
$totalMb = 0
foreach ($d in $matrix.disks) { $totalMb += [int]$d.size_mb }

$drive = Split-Path $WorkRoot -Qualifier   # e.g. 'C:\'
$freeBytes = (Get-PSDrive -Name ($drive.TrimEnd(':\')) -ErrorAction Stop).Free
$needBytes = ([long]$totalMb * 1MB) + 20GB   # VHDs + recovery transient + margin
if ($freeBytes -lt $needBytes) {
    throw ("work drive {0} has {1:N1} GiB free but the matrix needs ~{2:N1} GiB (VHDs {3} MiB + 20 GiB transient margin)" -f $drive, ($freeBytes / 1GB), ($needBytes / 1GB), $totalMb)
}
Write-Host ("work drive: {0} ({1:N1} GiB free, need ~{2:N1} GiB)" -f $drive, ($freeBytes / 1GB), ($needBytes / 1GB))

$diskDir = Join-Path $WorkRoot 'disks'
New-Item -ItemType Directory -Force -Path $diskDir | Out-Null

$state = [ordered]@{
    work_root = $WorkRoot
    created_utc = (Get-Date).ToUniversalTime().ToString('o')
    method = 'diskpart create vdisk type=fixed'
    disks = @()
}

foreach ($d in $matrix.disks) {
    Write-Host "== disk '$($d.id)': $($d.size_mb) MiB $($d.partition_style) =="
    $vhd = Join-Path $diskDir "$($d.id).vhdx"

    # free letters first (explicit, no auto-assign races)
    $letters = @()
    foreach ($v in $d.volumes) {
        $letters += @(Get-FreeLetter ([char]$v.letter_hint[0]))
    }

    # disk number: BEFORE snapshot
    $before = @(Get-Disk | ForEach-Object { $_.Number })

    Invoke-Diskpart @(
        "create vdisk file=`"$vhd`" maximum=$($d.size_mb) type=fixed",
        'attach vdisk'
    ) -What "create+attach $($d.id)" | Out-Null

    # resolve the new disk by diff (retry up to 30 s: PnP settle)
    $diskNumber = -1
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep -Seconds 1
        $after = @(Get-Disk | ForEach-Object { $_.Number })
        $new = @($after | Where-Object { $before -notcontains $_ })
        if ($new.Count -eq 1) { $diskNumber = $new[0]; break }
        if ($new.Count -gt 1) { throw "ambiguous disk attach: $($new -join ',')" }
    }
    if ($diskNumber -lt 0) { throw "disk $($d.id) did not appear within 30 s of attach" }
    Write-Host "  attached as disk $diskNumber"

    Set-Disk -Number $diskNumber -IsOffline $false -ErrorAction SilentlyContinue
    Set-Disk -Number $diskNumber -IsReadOnly $false -ErrorAction SilentlyContinue
    Initialize-Disk -Number $diskNumber -PartitionStyle $d.partition_style -ErrorAction Stop

    # partitions: all but the last sized by percent, the last takes the rest
    $volumes = @()
    $partCount = @($d.volumes).Count
    for ($i = 0; $i -lt $partCount; $i++) {
        $v = $d.volumes[$i]
        $letter = $letters[$i]
        if ($i -lt $partCount - 1) {
            $free = [long](Get-Disk -Number $diskNumber).LargestFreeExtent
            $size = [long]([math]::Floor($free * ([double]$v.size_percent / 100.0)))
            if ($size -lt 64MB) { throw "volume $($v.label): computed size $size bytes is below the 64 MiB sanity floor" }
            $part = New-Partition -DiskNumber $diskNumber -Size $size -DriveLetter $letter -ErrorAction Stop
        } else {
            $part = New-Partition -DiskNumber $diskNumber -UseMaximumSize -DriveLetter $letter -ErrorAction Stop
        }
        $vol = Format-Volume -Partition $part -FileSystem $v.fs -NewFileSystemLabel $v.label -Force -ErrorAction Stop
        # settle: wait until the volume reports its filesystem (format completes async)
        for ($w = 0; $w -lt 20; $w++) {
            $probe = Get-Volume -DriveLetter $letter -ErrorAction SilentlyContinue
            if ($probe -and $probe.FileSystem -eq $v.fs -and $probe.Size -gt 0) { break }
            Start-Sleep -Milliseconds 500
        }
        $final = Get-Volume -DriveLetter $letter -ErrorAction Stop
        $volumes += [ordered]@{
            label = $v.label; fs = $v.fs; letter = "$letter"
            partition = $part.PartitionNumber
            bytes = [long]$final.Size
        }
        Write-Host ("  volume {0}: {1} {2} ({3:N0} MiB, partition {4})" -f "$letter`:", $v.fs, $v.label, ($final.Size / 1MB), $part.PartitionNumber)
    }

    $state.disks += [ordered]@{
        id = $d.id
        vhd = $vhd
        disk_number = $diskNumber
        partition_style = $d.partition_style
        size_mb = [int]$d.size_mb
        volumes = $volumes
    }
}

$state | ConvertTo-Json -Depth 6 | Set-Content -Path $OutJson -Encoding utf8
Write-Host "disk-state.json -> $OutJson"
Get-Content $OutJson | Write-Host
