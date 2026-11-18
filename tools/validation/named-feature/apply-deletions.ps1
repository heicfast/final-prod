# apply-deletions.ps1 - apply the named-feature fixture's deletion matrix.
#
# Reads the manifest emitted by generate-fixture.ps1 and performs, in
# apply_order: the double-delete recreate dance (delete v1 -> restore v2
# from the .nffx stash with its declared metadata -> delete v2), the plain
# file deletes, and the whole-tree deletes. Volume is flushed BEFORE
# (lazy-writer guard) and AFTER (metadata flush) - the testlab lesson
# (HALIM-FEATURES §2.19 / Run-TestLab.ps1). No remount happens here by
# construction; the volume must not be remounted-and-written before the
# scan (see README).
#
# Emits a JSON apply-log: every action, its outcome, flush results.
#
# Usage:
#   powershell -File apply-deletions.ps1 -Root X:\fixture `
#       -ManifestOut D:\work\manifest.jsonl -LogOut D:\work\apply-log.json
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)][string] $Root,
    [Parameter(Mandatory = $true)][string] $ManifestOut,
    [string] $LogOut = ''
)

$ErrorActionPreference = 'Stop'

function Write-Log([string] $msg) {
    Write-Host ("[nffx-del] {0} {1}" -f (Get-Date -Format o), $msg)
}

function Get-VolumeLetter([string] $root) {
    return (Split-Path -Qualifier $Root).TrimEnd(':')
}

function Flush-Volume([string] $root) {
    $letter = Get-VolumeLetter $root
    try {
        Write-VolumeCache -DriveLetter $letter -ErrorAction Stop
        return 'flushed'
    }
    catch {
        return "flush-failed: $($_.Exception.Message)"
    }
}

if (-not (Test-Path -LiteralPath $ManifestOut)) { throw "manifest not found: $ManifestOut" }
$rows = [System.IO.File]::ReadAllLines($ManifestOut) |
    Where-Object { $_ -and ($_ -notmatch '^\s*_meta') } |
    ForEach-Object { $_ | ConvertFrom-Json }

$actions = [System.Collections.Generic.List[object]]::new()

function Add-Action([string] $kind, [string] $target, [string] $outcome) {
    $script:actions.Add([ordered]@{
        kind    = $kind
        target  = $target
        outcome = $outcome
        at_utc  = (Get-Date).ToUniversalTime().ToString('o')
    })
}

# 0. flush BEFORE any deletion (lazy-writer guard)
$flushBefore = Flush-Volume $Root
Write-Log "flush-before: $flushBefore"

$ordered = $rows | Sort-Object -Property apply_order

foreach ($r in $ordered) {
    if ($r.special -eq 'double-delete-v1') {
        # delete v1, then restore v2 from the stash with its declared metadata
        $abs = Join-Path $Root ($r.rel -replace '/', '\')
        if (Test-Path -LiteralPath $abs) {
            Remove-Item -LiteralPath $abs -Force
            Add-Action 'delete' $r.rel 'ok'
        }
        else {
            Add-Action 'delete' $r.rel 'already-absent'
        }
        continue
    }
    if ($r.special -eq 'double-delete-v2') {
        # v2 is recreated from the stash, then deleted (apply_order > v1's)
        continue   # handled below after the stash copy step
    }
}

# recreate v2 from the stash (between the two delete passes), with declared metadata
$stash = Join-Path $Root '.nffx'
$stashFile = Join-Path $stash 'draft-v2.txt'
$v2rows = @($rows | Where-Object { $_.special -eq 'double-delete-v2' })
if ($v2rows.Count -ne 1) { throw "expected exactly one double-delete-v2 row, got $($v2rows.Count)" }
$v2 = $v2rows[0]
if (-not (Test-Path -LiteralPath $stashFile)) { throw "stash missing: $stashFile (run generate-fixture.ps1 first)" }
$v2abs = Join-Path $Root ($v2.rel -replace '/', '\')
if (Test-Path -LiteralPath $v2abs) { throw "v2 target already exists before recreation: $v2abs" }
[System.IO.File]::Copy($stashFile, $v2abs)
$item = Get-Item -LiteralPath $v2abs
$item.CreationTime   = [datetime] $v2.created
$item.LastWriteTime  = [datetime] $v2.modified
$item.LastAccessTime = [datetime] $v2.accessed
if ($v2.attributes -and $v2.attributes -ne 'Directory') {
    $item.Attributes = [IO.FileAttributes] ($v2.attributes -replace ', ', ',')
}
$sha = (Get-FileHash -LiteralPath $v2abs -Algorithm SHA256).Hash.ToLowerInvariant()
if ($sha -ne $v2.sha256) {
    throw "v2 content mismatch after recreation: manifest $($v2.sha256) vs actual $sha"
}
Add-Action 'recreate-from-stash' $v2.rel "ok sha-verified ($sha)"

# now apply every remaining action in apply_order (v2 delete + files + trees)
foreach ($r in ($rows | Sort-Object -Property apply_order)) {
    if (-not $r.deleted) { continue }
    if ($r.special -eq 'double-delete-v1') { continue }        # done above
    if ($r.special -eq 'double-delete-v2') {
        if (Test-Path -LiteralPath $v2abs) {
            Remove-Item -LiteralPath $v2abs -Force
            Add-Action 'delete' "$($r.rel) (v2)" 'ok'
        }
        continue
    }
    if ($r.delete_class -eq 'file-delete') {
        $abs = Join-Path $Root ($r.rel -replace '/', '\')
        if (Test-Path -LiteralPath $abs) {
            # ReadOnly attributes need -Force; Remove-Item -Force covers them
            Remove-Item -LiteralPath $abs -Force
            Add-Action 'delete' $r.rel 'ok'
        }
        else {
            Add-Action 'delete' $r.rel 'already-absent'
        }
    }
    elseif ($r.delete_class -eq 'tree-delete') {
        if ($r.is_dir) {
            $abs = Join-Path $Root ($r.rel -replace '/', '\')
            if (Test-Path -LiteralPath $abs) {
                Remove-Item -LiteralPath $abs -Recurse -Force
                Add-Action 'delete-tree' $r.rel 'ok'
            }
            else {
                Add-Action 'delete-tree' $r.rel 'already-absent'
            }
        }
        # child rows of a tree-delete are covered by their parent's
        # recursive delete - no individual action (they are already gone)
    }
}

# remove the scaffolding stash (it must not appear in the damaged image)
if (Test-Path -LiteralPath $stash) {
    Remove-Item -LiteralPath $stash -Recurse -Force
    Add-Action 'remove-stash' '.nffx' 'ok'
}

# flush AFTER all deletions (metadata flush)
$flushAfter = Flush-Volume $Root
Write-Log "flush-after: $flushAfter"

$log = [ordered]@{
    fixture_root = $Root
    manifest     = $ManifestOut
    applied_utc  = (Get-Date).ToUniversalTime().ToString('o')
    flush_before = $flushBefore
    flush_after  = $flushAfter
    actions      = $actions
}
$json = $log | ConvertTo-Json -Depth 5
if ($LogOut) {
    $d = Split-Path -Parent $LogOut
    if ($d -and -not (Test-Path -LiteralPath $d)) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
    [System.IO.File]::WriteAllText($LogOut, $json)
    Write-Log "apply-log: $LogOut"
}
Write-Log "actions: $($actions.Count)"
Write-Log "done"
