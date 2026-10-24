# apply-scenario.ps1 - apply a deletion scenario to a seeded volume.
#
# Modes:
#   recycle_bin           shell delete to the Recycle Bin (VB SendToRecycleBin), bin NOT emptied
#   permanent_delete      Remove-Item -Force (no bin)
#   unlink_native         native DeleteFileW via P/Invoke (no bin, distinct code path)
#   folder_delete         whole seed tree shell-deleted to the bin, then bin emptied
#   quick_format          Format-Volume (quick)
#   full_format           Format-Volume -Full (zero-fill)
#   delete_partition      GPT partition entry removed (data sectors untouched)
#   decay_50/80/100       permanent delete, then fill volume to N%, then delete the fillers
#   partial_overwrite     permanent delete, then 0xAA over first 8 sectors of each extent
#   mft_corrupt           permanent delete, then zero the deleted files' MFT records + XOR $Bitmap
#
# Emits scenario-state.json with the exact method, per-file results, pre/post
# volume hash maps and changed-block counts. Ground truth is NEVER derived
# from tool output - only from the manifest + this recorded method.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $Mode,
    [Parameter(Mandatory = $true)] [string] $Letter,          # e.g. 'E'
    [Parameter(Mandatory = $true)] [string] $Root,            # e.g. E:\seed
    [Parameter(Mandatory = $true)] [string] $ManifestJsonl,
    [Parameter(Mandatory = $true)] [string] $OutDir,
    [long] $VolumeBytes = 0,
    [string] $KeepRoot = '',
    [string] $FileSystem = 'NTFS',
    [string] $VolumeLabel = 'BENCH',
    [int] $DiskNumber = -1,
    [int[]] $PartitionNumber = @(-1),
    [int] $AdjacentRecords = 0
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\..\volume\volume.ps1"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# ---- load manifest -------------------------------------------------------------
$entries = @()
Get-Content -LiteralPath $ManifestJsonl | ForEach-Object {
    $o = $_ | ConvertFrom-Json
    if (-not $o._meta) { $entries += $o }
}
$deletable = @($entries | Where-Object { -not $_.keep })
$keepEntries = @($entries | Where-Object { $_.keep })

function EntryPath($e) {
    $base = if ($e.rel -match '^seed') { $Root } else { $KeepRoot }
    $winRel = ($e.rel -replace '^seedb?/', '' -replace '^keep/', '')
    $p = Join-Path $base $winRel
    if ($e.long_path) { $p = "\\?\$p" }
    return $p
}

$state = [ordered]@{
    mode = $Mode
    letter = "$Letter`:"
    filesystem = $FileSystem
    started_utc = (Get-Date).ToUniversalTime().ToString('o')
    deletable_count = $deletable.Count
    keep_count = $keepEntries.Count
    deletable_bytes = ($deletable | Measure-Object -Property size -Sum).Sum
    delete_failures = @()
    notes = @()
    extra = $null
}

# ---- pre-state hash map ---------------------------------------------------------
$preMap = Join-Path $OutDir 'hashmap-pre.csv'
Invoke-VolumeHashCapture -Letter $Letter -OutFile $preMap -VolumeBytes $VolumeBytes

$t0 = Get-Date

# ---- deleters --------------------------------------------------------------------
function Clear-ReadOnlyQuiet($p) {
    try {
        $it = Get-Item -LiteralPath $p -Force
        if ($it.IsReadOnly) { Set-ItemProperty -LiteralPath $p -Name IsReadOnly -Value $false; return $true }
    } catch { }
    return $false
}

function Invoke-PermanentDelete {
    $failed = @()
    foreach ($e in $deletable) {
        $p = EntryPath $e
        if (-not (Test-Path -LiteralPath $p)) { continue }
        try { Remove-Item -LiteralPath $p -Force -ErrorAction Stop }
        catch { $failed += @{ rel = $e.rel; error = $_.Exception.Message } }
    }
    return ,$failed
}

if ($Mode -eq 'recycle_bin') {
    Add-Type -AssemblyName Microsoft.VisualBasic
    $cleared = @()
    foreach ($e in $deletable) {
        $p = EntryPath $e
        if (-not (Test-Path -LiteralPath $p)) { continue }
        $didClear = $false
        try {
            [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteFile(
                $p, [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
                [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin)
        } catch {
            # read-only files refuse the shell move until the bit is cleared;
            # record it (method honesty) and retry once
            $didClear = Clear-ReadOnlyQuiet $p
            try {
                [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteFile(
                    $p, [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
                    [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin)
            } catch {
                $state.delete_failures += @{ rel = $e.rel; error = $_.Exception.Message }
                continue
            }
        }
        if ($didClear) { $cleared += $e.rel }
    }
    $state.notes += "shell delete via VB SendToRecycleBin; bin NOT emptied; readonly-cleared-before-delete: $($cleared.Count)"
}
elseif ($Mode -eq 'permanent_delete') {
    $state.delete_failures += Invoke-PermanentDelete
    $state.notes += 'Remove-Item -Force (bypasses the recycle bin)'
}
elseif ($Mode -eq 'unlink_native') {
    if (-not ('BenchDelete' -as [type])) {
        Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class BenchDelete {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool DeleteFileW(string path);
}
"@
    }
    $cleared = @()
    foreach ($e in $deletable) {
        $p = EntryPath $e
        if (-not (Test-Path -LiteralPath $p)) { continue }
        $didClear = $false
        if (-not [BenchDelete]::DeleteFileW($p)) {
            $didClear = Clear-ReadOnlyQuiet $p
            if (-not [BenchDelete]::DeleteFileW($p)) {
                $state.delete_failures += @{ rel = $e.rel; error = "DeleteFileW GLE=$([Runtime.InteropServices.Marshal]::GetLastWin32Error())" }
                continue
            }
        }
        if ($didClear) { $cleared += $e.rel }
    }
    $state.notes += "native DeleteFileW P/Invoke (no bin); readonly-cleared-before-unlink: $($cleared.Count)"
}
elseif ($Mode -eq 'folder_delete') {
    Add-Type -AssemblyName Microsoft.VisualBasic
    [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory(
        $Root, [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
        [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin)
    Start-Sleep -Seconds 1
    Clear-RecycleBin -DriveLetter $Letter -Force -ErrorAction SilentlyContinue
    Write-VolumeCache -DriveLetter $Letter -ErrorAction SilentlyContinue | Out-Null
    $state.notes += 'whole seed tree shell-deleted to the bin (VB DeleteDirectory), then Clear-RecycleBin -Force; keep root untouched'
}
elseif ($Mode -eq 'quick_format') {
    Format-Volume -DriveLetter $Letter -FileSystem $FileSystem -NewFileSystemLabel $VolumeLabel -Force | Out-Null
    $state.notes += 'Format-Volume quick format (metadata reinit, no zero-fill)'
}
elseif ($Mode -eq 'full_format') {
    Format-Volume -DriveLetter $Letter -FileSystem $FileSystem -NewFileSystemLabel $VolumeLabel -Full -Force | Out-Null
    $state.notes += 'Format-Volume -Full (zero-fills the data area)'
}
elseif ($Mode -eq 'delete_partition') {
    if ($DiskNumber -lt 0 -or $PartitionNumber[0] -lt 0) { throw 'delete_partition requires -DiskNumber and -PartitionNumber' }
    $removed = @()
    foreach ($pn in $PartitionNumber) {
        if ($pn -lt 0) { continue }
        try {
            Remove-Partition -DiskNumber $DiskNumber -PartitionNumber $pn -Confirm:$false -ErrorAction Stop
        } catch {
            $dp = "select disk $DiskNumber`r`nselect partition $pn`r`ndelete partition override"
            $tmp = [System.IO.Path]::GetTempFileName()
            $dp | Set-Content -Path $tmp -Encoding ascii
            diskpart /s $tmp
            if ($LASTEXITCODE -ne 0) { throw "partition $pn delete failed: $_" }
            Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        }
        $removed += $pn
    }
    # the disk is left fully unallocated: both seeded partitions' entries are gone
    # (data sectors untouched) - lost-partition recovery must rebuild from raw scan
    $state.notes += "GPT entries removed on disk ${DiskNumber}: partitions $($removed -join ',') (Remove-Partition; data sectors untouched); disk left unallocated"
}
elseif ($Mode.StartsWith('decay_')) {
    $pct = [int]($Mode -replace 'decay_', '')
    $state.delete_failures += Invoke-PermanentDelete
    Write-VolumeCache -DriveLetter $Letter -ErrorAction SilentlyContinue | Out-Null
    $targetRemaining = [long]($VolumeBytes * (100 - $pct) / 100)
    $fillerDir = Join-Path $Root 'filler'
    New-Item -ItemType Directory -Path $fillerDir -Force | Out-Null
    $frng = [System.Random]::new(0xDECA0501 + $pct)
    $buf = New-Object byte[] 65536
    $fillerCount = 0; $fillerBytes = [long]0; $fullHit = $false
    while ($true) {
        $rem = [long](Get-Volume -DriveLetter $Letter).SizeRemaining
        if ($rem -le $targetRemaining) { break }
        if ($rem -lt 40MB) { $fullHit = $true; break }
        $fp = Join-Path $fillerDir ("fill{0:d4}.dat" -f $fillerCount)
        $fs = [System.IO.File]::Create($fp)
        try {
            $size = [long]32MB
            if ($rem - $targetRemaining -lt $size) { $size = [long]($rem - $targetRemaining) }
            $left = $size
            while ($left -gt 0) {
                $n = [int][Math]::Min(65536, $left)
                $frng.NextBytes($buf)
                $fs.Write($buf, 0, $n); $left -= $n
            }
            $fillerBytes += $size
        } catch { $fullHit = $true; $fs.Close(); break }
        finally { if ($fs) { $fs.Close() } }
        $fillerCount++
    }
    Start-Sleep -Seconds 1
    Remove-Item -LiteralPath $fillerDir -Recurse -Force
    Write-VolumeCache -DriveLetter $Letter -ErrorAction SilentlyContinue | Out-Null
    $decayMap = Join-Path $OutDir 'hashmap-post-decay.csv'
    Invoke-VolumeHashCapture -Letter $Letter -OutFile $decayMap -VolumeBytes $VolumeBytes
    $state.extra = @{
        fill_pct = $pct; filler_files = $fillerCount; filler_bytes = $fillerBytes
        disk_full_hit = $fullHit; target_remaining_bytes = $targetRemaining
    }
    $state.notes += "permanent delete -> fillers written to $pct% -> fillers deleted (sector reuse decay); post-decay hash map captured"
}
elseif ($Mode -eq 'partial_overwrite') {
    $state.delete_failures += Invoke-PermanentDelete
    Write-VolumeCache -DriveLetter $Letter -ErrorAction SilentlyContinue | Out-Null
    $bpc = Get-BootClusterSize -Letter $Letter
    if ($bpc -le 0) { throw "bad cluster size $bpc" }
    $exts = @()
    foreach ($e in $deletable) {
        if (-not $e.extents -or $e.extents.Count -eq 0) { continue }
        $first = $e.extents[0]
        $exts += @{ lcn = [long]$first[0]; clusters = [long]$first[1]; rel = $e.rel }
    }
    $written = Invoke-PartialOverwrite -Letter $Letter -Extents $exts -Sectors 8 -Pattern 0xAA -BytesPerCluster $bpc
    $state.extra = @{ bytes_per_cluster = $bpc; files_overwritten = $exts.Count; sectors_per_file = 8; pattern = '0xAA' }
    $state.notes += "permanent delete -> first 8 sectors (4096 B) of each deleted file's first extent overwritten with 0xAA via locked-volume raw IO"
}
elseif ($Mode -eq 'mft_corrupt') {
    if ($FileSystem -ne 'NTFS') {
        throw "mft_corrupt requires NTFS (got $FileSystem) - caller must record NOT-COMPARABLE instead of running this"
    }
    $state.delete_failures += Invoke-PermanentDelete
    Write-VolumeCache -DriveLetter $Letter -ErrorAction SilentlyContinue | Out-Null
    $records = @()
    foreach ($e in $deletable) { if ($null -ne $e.mft_record) { $records += [long]$e.mft_record } }
    $corruptJson = Invoke-MftCorrupt -Letter $Letter -Records $records -AdjacentRecords $AdjacentRecords -AlsoBitmap
    $corrupt = $corruptJson | ConvertFrom-Json
    $state.extra = @{
        ntfs = $corrupt
        records_requested = $records.Count
    }
    $state.notes += "permanent delete -> $($corrupt.records_zeroed.Count) MFT records zeroed (+$AdjacentRecords adjacent each) and the first 4 KiB of the dollar-Bitmap XOR-0xFFed via locked-volume raw IO"
}
else {
    throw "unknown scenario mode: $Mode"
}

$state.elapsed_delete_seconds = [math]::Round(((Get-Date) - $t0).TotalSeconds, 2)
$state.ended_utc = (Get-Date).ToUniversalTime().ToString('o')

# ---- post-state hash map ----------------------------------------------------------
if ($Mode -eq 'delete_partition') {
    $postMap = Join-Path $OutDir 'hashmap-post-disk.csv'
    Invoke-DiskHashCapture -DiskNumber $DiskNumber -OutFile $postMap
    $state.notes += 'post-state captured at DISK level (volume no longer exists)'
} else {
    $postMap = Join-Path $OutDir 'hashmap-post.csv'
    Invoke-VolumeHashCapture -Letter $Letter -OutFile $postMap -VolumeBytes $VolumeBytes
}

# ---- changed-block proof -----------------------------------------------------------
function Get-MapStats($path) {
    if (-not (Test-Path $path)) { return @{ blocks = 0; changed = -1 } }
    $pre = Get-Content -LiteralPath (Join-Path $OutDir 'hashmap-pre.csv') -ErrorAction SilentlyContinue
    $cur = Get-Content -LiteralPath $path
    $changed = 0
    $n = [Math]::Min($pre.Count, $cur.Count)
    for ($i = 0; $i -lt $n; $i++) { if ($pre[$i] -ne $cur[$i]) { $changed++ } }
    return @{ blocks = $cur.Count; changed = $changed; compared = $n }
}
$state.pre_map = Get-MapStats (Join-Path $OutDir 'hashmap-pre.csv')
$state.post_map = Get-MapStats $postMap
if ($Mode -eq 'delete_partition') {
    # pre-map is volume-level, post-map is disk-level - a numeric diff would be
    # meaningless; flag it instead of implying comparability
    $state.post_map.not_comparable = $true
}

$state | ConvertTo-Json -Depth 8 | Set-Content -Path (Join-Path $OutDir 'scenario-state.json') -Encoding utf8
Write-Host "scenario $Mode applied: failures=$($state.delete_failures.Count) notes=$($state.notes -join ' | ')"
