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

function Get-VolumeLetter([string] $rootPath) {
    # regex on the LOCAL param (run-2 postmortem: the Split-Path/TrimEnd
    # derivation fed Write-VolumeCache a ':' letter at the after-flush while
    # the identical before-flush worked; derive defensively + loudly)
    if ($rootPath -match '^([A-Za-z]):[\\/]') { return $Matches[1] }
    throw "cannot derive a drive letter from root: '$rootPath'"
}

function Flush-Volume([string] $rootPath) {
    $letter = Get-VolumeLetter $rootPath
    try {
        $vol = Get-Volume -DriveLetter $letter -ErrorAction Stop
        if ($vol) {
            $vol | Write-VolumeCache -ErrorAction Stop
            return "flushed (letter $letter)"
        }
        return "flush-failed: volume $letter not found"
    }
    catch {
        return "flush-failed (letter $letter): $($_.Exception.Message)"
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

# 0a. D-PROBE-01: create the flush probe's MFT record NOW, before any
# deletion. Run 44391734922 proved the hard way that a post-deletion
# create can take a JUST-FREED record (report.docx's - its recovery was
# eaten by the probe's own file record); created here, while every
# fixture file is still live, its record is freshly allocated and
# cannot be any deleted file's record. The write-through byte + delete
# still happen AFTER all deletions (the durability step below).
$probePath = Join-Path $Root '.nffx-flush-probe'
$probeKeep = [System.IO.FileStream]::new($probePath, [System.IO.FileMode]::Create,
    [System.IO.FileAccess]::Write, [System.IO.FileShare]::None, 1,
    [System.IO.FileOptions]::WriteThrough)
try { $probeKeep.Close() } catch { $probeKeep.Close() }

# 0. flush BEFORE any deletion (lazy-writer guard)
$flushBefore = Flush-Volume $Root
Write-Log "flush-before: $flushBefore"

$ordered = $rows | Sort-Object -Property apply_order

foreach ($r in $ordered) {
    if ($r.special -eq 'double-delete-v1') {
        # delete v1, then restore v2 from the stash with its declared metadata
        $abs = Join-Path $Root ($r.rel -replace '/', '\')
        Write-Log "action delete-v1: $($r.rel)"
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
# D-V2-DURABILITY (the 44391734922 root cause): the v2 copy is the ONLY
# mid-apply cached write on the volume, and this lane's own history proves
# the volume cache unreliable here (Write-VolumeCache second-invocation
# failures, runs 44354788432/44355793332) - the copy's DATA clusters were
# lost on the detach (the VHDX read zeros at v2's extent while every
# generation-time write survived). The recreate now goes through a
# WriteThrough FileStream (the same ctor the flush probe already uses),
# so the bytes reach the disk before the delete can orphan them.
$stash = Join-Path $Root '.nffx'
$stashFile = Join-Path $stash 'draft-v2.txt'
$v2rows = @($rows | Where-Object { $_.special -eq 'double-delete-v2' })
if ($v2rows.Count -ne 1) { throw "expected exactly one double-delete-v2 row, got $($v2rows.Count)" }
$v2 = $v2rows[0]
if (-not (Test-Path -LiteralPath $stashFile)) { throw "stash missing: $stashFile (run generate-fixture.ps1 first)" }
$v2abs = Join-Path $Root ($v2.rel -replace '/', '\')
if (Test-Path -LiteralPath $v2abs) { throw "v2 target already exists before recreation: $v2abs" }
$v2bytes = [System.IO.File]::ReadAllBytes($stashFile)
$fs = [System.IO.FileStream]::new($v2abs, [System.IO.FileMode]::Create,
    [System.IO.FileAccess]::Write, [System.IO.FileShare]::None, 65536,
    [System.IO.FileOptions]::WriteThrough)
try { $fs.Write($v2bytes, 0, $v2bytes.Length); $fs.Flush($true) } finally { $fs.Close() }
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
        Write-Log "action delete: $($r.rel)"
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
            Write-Log "action delete-tree: $($r.rel)"
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

# flush AFTER all deletions (metadata flush).
# Truth from runs 44354788432/44355793332: the Storage-module Write-VolumeCache
# misbound DriveLetter ':' on the SECOND invocation (identical first call
# succeeded; both the -DriveLetter char form and the piped-volume form), so the
# after-flush is best-effort by construction and NEVER fatal to the evidence:
#   (a) a WriteThrough create+flush+delete probe forces the write-through path
#       (durability before the diskpart detach, which flushes again itself);
#       (b) one -ErrorAction SilentlyContinue Write-VolumeCache attempt, outcome
#       recorded truthfully in the apply-log.
# D-PROBE-01: the probe is now a DECLARED manifest row (size 1, content
# 0x00, sha256 of b"\x00") - its honest recovery is accounted by I4. The
# probe's create may reuse ANY freed MFT record (lowest-free allocation),
# which can eat a just-deleted file's record; the apply-log records the
# fixture's MFT state after the probe so a missing I2 row is diagnosable
# from evidence (the run-44391734922 lesson: report.docx's record was
# taken by the probe's create and the failure was undebuggable).
$flushAfter = 'not attempted'
try {
    # the probe's record was allocated BEFORE the deletions (0a above);
    # this step only writes its 1 content byte write-through and deletes
    # it - the delete frees the probe's OWN record, nothing else's
    $probe = $probePath
    # FileStream ctor (run-4 evidence: File.Open's 4-arg overload takes
    # FileShare, not FileOptions - the ctor form pins WriteThrough correctly)
    $fs = [System.IO.FileStream]::new($probe, [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::Write, [System.IO.FileShare]::None, 1,
        [System.IO.FileOptions]::WriteThrough)
    try { $fs.WriteByte(0); $fs.Flush($true) } finally { $fs.Close() }
    $probeSha = (Get-FileHash -LiteralPath $probe -Algorithm SHA256).Hash.ToLowerInvariant()
    Remove-Item -LiteralPath $probe -Force
    $letter = Get-VolumeLetter $Root
    Write-VolumeCache -DriveLetter $letter -ErrorAction SilentlyContinue
    $flushAfter = "write-through probe ok (sha $probeSha); volume-cache best-effort (letter $letter)"
}
catch {
    $flushAfter = "flush-failed: $($_.Exception.Message)"
}
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
