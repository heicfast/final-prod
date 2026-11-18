# generate-fixture.ps1 - deterministic seeded fixture for the named-feature
# validation standard (session: cross-repo harvest, T5).
#
# Builds the fixture tree on any NTFS root with FIXED timestamps, FIXED
# attributes and PRNG content seeded from a salted hash of each file's
# relative path (same discipline as engine/bench/seed/generate_seed.ps1):
# the tree is byte-identical on every run, every machine. Deletion is NOT
# applied here - apply-deletions.ps1 owns the deletion matrix, so the
# manifest records the ground truth BEFORE damage (session Phase 3).
#
# Emits manifest.jsonl: line 1 = _meta, then one JSON object per file:
#   {rel, size, sha256, created, modified, accessed, attributes,
#    deleted, delete_class, apply_order, expect{...}}
#
# Usage:
#   powershell -File generate-fixture.ps1 -Root X:\fixture `
#       -ManifestOut D:\work\manifest.jsonl
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)][string] $Root,          # e.g. X:\fixture (an NTFS volume)
    [Parameter(Mandatory = $true)][string] $ManifestOut,   # .jsonl out path
    [string] $FixtureId = 'named-feature-v1'
)

$ErrorActionPreference = 'Stop'

$FIXTURE_VERSION = '1.0.0'
$CONTENT_SALT    = 0x5EEDF1AF

# deterministic timestamps (ground truth for preservation checks;
# same convention as engine/bench/seed/generate_seed.ps1)
$T_CREATE = [datetime] '2026-01-15 08:09:10'
$T_WRITE  = [datetime] '2026-01-16 09:10:11'
$T_ACCESS = [datetime] '2026-01-17 10:11:12'

function Write-Log([string] $msg) {
    Write-Host ("[nffx-gen] {0} {1}" -f (Get-Date -Format o), $msg)
}

# ---- deterministic content -------------------------------------------------
# PRNG seed = salted FNV-style fold over the rel path: same rel path =>
# same bytes, independent of creation order or host.
function Get-SeedFor([string] $rel) {
    # long math + mask: uint32 would overflow-promote to double under -band
    $h = [long] $CONTENT_SALT
    foreach ($ch in $rel.ToCharArray()) {
        $h = (($h * 31) + [int][char] $ch) -band 0x7FFFFFFF
    }
    return [int] $h
}

function Get-ContentBytes([string] $rel, [long] $size, [byte[]] $header, [byte[]] $footer) {
    $ms = [System.IO.MemoryStream]::new()
    if ($header.Length -gt 0) { $ms.Write($header, 0, $header.Length) }
    $bodyLen = $size - $header.Length - $footer.Length
    if ($bodyLen -gt 0) {
        $rng = [System.Random]::new((Get-SeedFor $rel))
        $body = [byte[]]::new($bodyLen)
        $rng.NextBytes($body)
        $ms.Write($body, 0, $body.Length)
    }
    if ($footer.Length -gt 0) { $ms.Write($footer, 0, $footer.Length) }
    # comma operator: prevents PowerShell from unrolling an EMPTY byte[0]
    # into zero pipeline output (which the caller would receive as $null -
    # the run-1 failure: WriteAllBytes ArgumentNullException on empty.bin)
    return , $ms.ToArray()
}

# real signature bytes so later carve-dedup checks have true positives
# (pattern from tools/forensic/seed-disks.ps1, our own MIT code)
$MAGIC = @{
    jpg  = @([byte[]](0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01), [byte[]](0xFF,0xD9))
    png  = @([byte[]](0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52), [byte[]](0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82))
    zip  = @([byte[]](0x50,0x4B,0x03,0x04,0x14,0x00,0x00,0x00,0x08,0x00), [byte[]](0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00))
    txt  = @([byte[]]@(), [byte[]]@())
    bin  = @([byte[]]@(), [byte[]]@())
    json = @([byte[]]@(), [byte[]]@())
}

# ---- the fixture matrix ----------------------------------------------------
# delete_class: 'none' (live) | 'file-delete' | 'tree-delete' (dir row carries
# the recursive delete; its children are covered by the parent).
# apply_order: ascending execution order for apply-deletions.ps1.
# expect.output_rel: the exact path a deleted-only, structure-preserving
# reconstruction MUST produce (null for the collision-set pair).
$Files = @(
    # rel,                                            size,      magic, attrs,        deleted, delete_class, apply_order, output_rel
    @{ rel = 'Docs/readme.txt';                        size = 2048;    magic = 'txt';  attrs = 'Archive';             deleted = $false; delete_class = 'none';         apply_order = 0;  output_rel = $null }
    @{ rel = 'Docs/report.docx';                       size = 40960;   magic = 'zip';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 10; output_rel = 'Docs/report.docx' }
    @{ rel = 'Docs/archive-2025.zip';                  size = 131072;  magic = 'zip';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 11; output_rel = 'Docs/archive-2025.zip' }
    @{ rel = 'Docs/Old';                               size = 0;       magic = $null;  attrs = 'Directory';           deleted = $true;  delete_class = 'tree-delete';  apply_order = 40; output_rel = 'Docs/Old'; is_dir = $true }
    @{ rel = 'Docs/Old/notes.txt';                     size = 3072;    magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Docs/Old/notes.txt' }
    @{ rel = 'Docs/Old/sub';                           size = 0;       magic = $null;  attrs = 'Directory';           deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Docs/Old/sub'; is_dir = $true }
    @{ rel = 'Docs/Old/sub/deep.bin';                  size = 614400;  magic = 'bin';  attrs = 'Archive';             deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Docs/Old/sub/deep.bin' }
    @{ rel = 'Photos/keep.jpg';                        size = 92160;   magic = 'jpg';  attrs = 'Archive';             deleted = $false; delete_class = 'none';         apply_order = 0;  output_rel = $null }
    @{ rel = 'Photos/vacation';                        size = 0;       magic = $null;  attrs = 'Directory';           deleted = $true;  delete_class = 'tree-delete';  apply_order = 41; output_rel = 'Photos/vacation'; is_dir = $true }
    @{ rel = 'Photos/vacation/day1.jpg';               size = 122880;  magic = 'jpg';  attrs = 'Archive';             deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Photos/vacation/day1.jpg' }
    @{ rel = 'Photos/vacation/day2.png';               size = 81920;   magic = 'png';  attrs = 'Archive';             deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Photos/vacation/day2.png' }
    @{ rel = 'Photos/vacation/inner';                  size = 0;       magic = $null;  attrs = 'Directory';           deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Photos/vacation/inner'; is_dir = $true }
    @{ rel = 'Photos/vacation/inner/meta.json';        size = 1024;    magic = 'json'; attrs = 'Archive';             deleted = $true;  delete_class = 'tree-delete';  apply_order = 0;  output_rel = 'Photos/vacation/inner/meta.json' }
    @{ rel = 'Collisions/draft.txt';                   size = 2048;    magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 20; output_rel = $null; special = 'double-delete-v1' }
    @{ rel = 'Collisions/draft.txt';                   size = 4096;    magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 22; output_rel = $null; special = 'double-delete-v2' }
    @{ rel = 'Names/file with spaces.txt';             size = 2048;    magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 12; output_rel = 'Names/file with spaces.txt' }
    @{ rel = 'Names/üñíçødé-名前- файл.txt';           size = 3072;    magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 13; output_rel = 'Names/üñíçødé-名前- файл.txt' }
    @{ rel = 'Sizes/empty.bin';                        size = 0;       magic = 'bin';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 14; output_rel = 'Sizes/empty.bin' }
    @{ rel = 'Sizes/resident.txt';                     size = 512;     magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 15; output_rel = 'Sizes/resident.txt' }
    @{ rel = 'Sizes/onerecord.bin';                    size = 4096;    magic = 'bin';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 16; output_rel = 'Sizes/onerecord.bin' }
    @{ rel = 'Sizes/nonresident.bin';                  size = 2097152; magic = 'bin';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 17; output_rel = 'Sizes/nonresident.bin' }
    @{ rel = 'Attributes/hidden-note.txt';             size = 1024;    magic = 'txt';  attrs = 'Archive, Hidden';      deleted = $true;  delete_class = 'file-delete';  apply_order = 18; output_rel = 'Attributes/hidden-note.txt' }
    @{ rel = 'Attributes/readonly-diary.txt';          size = 2048;    magic = 'txt';  attrs = 'Archive, ReadOnly';   deleted = $true;  delete_class = 'file-delete';  apply_order = 19; output_rel = 'Attributes/readonly-diary.txt' }
    @{ rel = 'Deep/a/b/c/d/e/nested-deep.txt';         size = 5120;    magic = 'txt';  attrs = 'Archive';             deleted = $true;  delete_class = 'file-delete';  apply_order = 21; output_rel = 'Deep/a/b/c/d/e/nested-deep.txt' }
)

$StashDir = '.nffx'   # scaffolding for the double-delete v2 bytes; removed by apply-deletions
$V2Size   = 4096      # MUST match the double-delete-v2 row's size in $Files

# ---- build -----------------------------------------------------------------
Write-Log "fixture $FixtureId v$FIXTURE_VERSION -> $Root"
New-Item -ItemType Directory -Force -Path $Root | Out-Null

$manifest = [System.Collections.Generic.List[string]]::new()
$meta = [ordered]@{
    _meta         = $true
    fixture_id    = $FixtureId
    fixture_ver   = $FIXTURE_VERSION
    content_salt  = $CONTENT_SALT
    t_create      = $T_CREATE.ToString('o')
    t_write       = $T_WRITE.ToString('o')
    t_access      = $T_ACCESS.ToString('o')
    root          = $Root
    generated_utc = (Get-Date).ToUniversalTime().ToString('o')
    note          = 'ground truth BEFORE deletion; apply-deletions.ps1 owns the deletion matrix'
}
$manifest.Add(($meta | ConvertTo-Json -Compress))

$rowIndex = 0
foreach ($f in $Files) {
    $abs = Join-Path $Root ($f.rel -replace '/', '\')
    $dir = Split-Path -Parent $abs
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }

    if ($f.is_dir) {
        # directory row (already created by the child or explicitly here)
        if (-not (Test-Path -LiteralPath $abs)) { New-Item -ItemType Directory -Force -Path $abs | Out-Null }
        $item = Get-Item -LiteralPath $abs
        $sha = ''
        $size = 0
    }
    else {
        $m = $MAGIC[$f.magic]
        if ($f.special -eq 'double-delete-v2') {
            # v2 bytes come from the stash copy written below; content is
            # deterministic via the stash rel path seed and DIFFERS from v1.
            $bytes = $null   # created by apply-deletions from the stash
        }
        else {
            $bytes = Get-ContentBytes -rel $f.rel -size $f.size -header $m[0] -footer $m[1]
        }
        if ($f.special -eq 'double-delete-v1') {
            # write v1 now; stash v2 bytes for apply-deletions
            [System.IO.File]::WriteAllBytes($abs, $bytes)
            $stashAbs = Join-Path $Root ($StashDir -replace '/', '\')
            if (-not (Test-Path -LiteralPath $stashAbs)) { New-Item -ItemType Directory -Force -Path $stashAbs | Out-Null }
            $v2bytes = Get-ContentBytes -rel "$StashDir/draft-v2.txt" -size $V2Size -header $m[0] -footer $m[1]
            [System.IO.File]::WriteAllBytes((Join-Path $stashAbs 'draft-v2.txt'), $v2bytes)
        }
        elseif ($f.special -ne 'double-delete-v2') {
            [System.IO.File]::WriteAllBytes($abs, $bytes)
        }
        if ($f.special -eq 'double-delete-v2') {
            # v2 does NOT exist yet (v1 is still on disk at this rel path!) -
            # hash the STASHED bytes, which is what apply-deletions will copy
            $v2 = [System.IO.File]::ReadAllBytes((Join-Path (Join-Path $Root $StashDir) 'draft-v2.txt'))
            $size = $v2.Length
            $sha = [System.BitConverter]::ToString([System.Security.Cryptography.SHA256]::Create().ComputeHash($v2)).Replace('-', '').ToLowerInvariant()
            if ($size -ne $f.size) { throw "v2 stash size mismatch: $($f.rel) declared $($f.size), stash $size" }
        }
        elseif (Test-Path -LiteralPath $abs) {
            $item = Get-Item -LiteralPath $abs
            $size = $item.Length
            $sha = (Get-FileHash -LiteralPath $abs -Algorithm SHA256).Hash.ToLowerInvariant()
        }
        else {
            throw "generated file missing after write: $($f.rel)"
        }
    }

    # deterministic metadata on everything that exists right now
    if ($f.special -ne 'double-delete-v2' -and $item) {
        $item.CreationTime = $T_CREATE
        $item.LastWriteTime = $T_WRITE
        $item.LastAccessTime = $T_ACCESS
        if ($f.attrs -and $f.attrs -ne 'Directory') {
            $item.Attributes = [IO.FileAttributes] ($f.attrs -replace ', ', ',')
        }
        if ($f.is_dir) { $sha = ''; $size = 0 }
    }

    $row = [ordered]@{
        idx          = $rowIndex
        rel          = $f.rel
        size         = $size
        sha256       = $sha
        created      = $T_CREATE.ToString('o')
        modified     = $T_WRITE.ToString('o')
        accessed     = $T_ACCESS.ToString('o')
        attributes   = $f.attrs
        is_dir       = [bool] $f.is_dir
        deleted      = [bool] $f.deleted
        delete_class = $f.delete_class
        apply_order  = $f.apply_order
        special      = $f.special
        expect       = [ordered]@{
            output_rel = $f.output_rel
            class      = 'EXACT'
        }
    }
    if ($f.special -like 'double-delete*') {
        $row.expect.class = 'COLLISION-SET'
        $row.expect.dir   = 'Collisions'
        $row.expect.stem  = 'draft'
        $row.expect.ext   = 'txt'
    }
    $row.expect.mode = $row.expect.class
    $manifest.Add(($row | ConvertTo-Json -Compress))
    $rowIndex++
}

$outDir = Split-Path -Parent $ManifestOut
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) { New-Item -ItemType Directory -Force -Path $outDir | Out-Null }
[System.IO.File]::WriteAllLines($ManifestOut, $manifest)
Write-Log "manifest: $ManifestOut ($($manifest.Count) lines incl _meta)"
Write-Log "done"
