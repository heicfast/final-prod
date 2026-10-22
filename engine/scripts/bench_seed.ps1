# bench_seed.ps1 - deterministic ground-truth seeding for the recovery benchmark.
#
# Seeds a volume with a manifest of files with known names, sizes and
# SHA-256 hashes, then applies deletion scenarios:
#   keep    - control group, never deleted (sanity: must be found active)
#   recycle - deleted via the Recycle Bin (VB FileSystem SendToRecycleBin)
#   perm    - permanently deleted (Remove-Item, no recycle)
#   wipe    - content overwritten with zeros, then permanently deleted
#             (honest floor: no product can recover wiped content)
#   folder  - an entire subtree removed (Remove-Item -Recurse)
#
# Output: manifest.csv (relpath,size,sha256,scenario) at -ManifestOut.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $Root,     # e.g. E:\seed
    [Parameter(Mandatory = $true)] [string] $ManifestOut,
    [int] $SmallCount = 60,       # 4-64 KiB files
    [int] $MediumCount = 25,      # 1-8 MiB files
    [int] $LargeCount = 6         # 16-48 MiB files
)

$ErrorActionPreference = 'Stop'

# Deterministic PRNG (LCG) so every run seeds identical content.
$script:State = [uint64]0x2A5F1E39
function Next-U64 {
    $script:State = ([uint64]$script:State * [uint64]6364136223846793005 + [uint64]1442695040888963407) -band [uint64]0xFFFFFFFFFFFFFFFF
    return $script:State
}
function Next-Range([uint64] $min, [uint64] $max) {
    return [uint64]($min + (Next-U64 % ($max - $min + 1)))
}

# Real-shape file content: correct headers/footers per type so carving
# sees genuine structure (JPEG, PNG, PDF, ZIP, MP4 shapes) over a
# deterministic pseudo-random body.
function Write-SeedFile([string] $Path, [string] $Type, [long] $Size) {
    $fs = [System.IO.File]::Create($Path)
    try {
        switch ($Type) {
            'jpg' { $h = [byte[]](0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01,0x01,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0xFF,0xDB,0x00,0x04,0x00,0x00); $fs.Write($h, 0, $h.Length) }
            'png' { $h = [byte[]](0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52); $fs.Write($h, 0, $h.Length) }
            'pdf' { $h = [System.Text.Encoding]::ASCII.GetBytes("%PDF-1.4`n% bench seed`n"); $fs.Write($h, 0, $h.Length) }
            'zip' { $h = [byte[]](0x50,0x4B,0x03,0x04,0x14,0x00,0x00,0x00,0x08,0x00); $fs.Write($h, 0, $h.Length) }
            'mp4' { $h = [byte[]](0x00,0x00,0x00,0x18,0x66,0x74,0x79,0x70,0x69,0x73,0x6F,0x6D); $fs.Write($h, 0, $h.Length) }
            default { $h = [byte[]](0xB5, 0x5E, 0xF0, 0x7D); $fs.Write($h, 0, $h.Length) }
        }
        $buf = New-Object byte[] 65536
        $remaining = $Size - $fs.Position
        while ($remaining -gt 0) {
            $n = [int][Math]::Min($buf.Length, $remaining)
            for ($i = 0; $i -lt $n; $i += 8) {
                $v = Next-U64
                for ($b = 0; $b -lt 8 -and $i + $b -lt $n; $b++) {
                    $buf[$i + $b] = [byte](($v -shr (8 * $b)) -band 0xFF)
                }
            }
            $fs.Write($buf, 0, $n)
            $remaining -= $n
        }
        switch ($Type) {
            'jpg' { $fs.Seek(-2, 'End') | Out-Null; $h = [byte[]](0xFF,0xD9); $fs.Write($h, 0, 2) }
            'pdf' { $h = [System.Text.Encoding]::ASCII.GetBytes("`n%%EOF"); $fs.Write($h, 0, $h.Length) }
        }
    } finally {
        $fs.Close()
    }
}

$manifest = New-Object System.Collections.Generic.List[string]
$script:Files = @()

function Add-Seed([string] $RelDir, [string] $Name, [string] $Type, [long] $Size, [string] $Scenario) {
    $dir = Join-Path $Root $RelDir
    if (-not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
    $path = Join-Path $dir $Name
    Write-SeedFile $path $Type $Size
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    $rel = ($path.Substring($Root.Length + 1)).Replace('\', '/')
    $script:Files += [pscustomobject]@{
        Path = $path; Rel = $rel; Size = $Size; Hash = $hash; Scenario = $Scenario
    }
    $manifest.Add("$rel,$Size,$hash,$Scenario")
}

$types = @('jpg', 'png', 'pdf', 'zip', 'bin')

# ---- small files (4-64 KiB): recycle / perm / wipe groups ----
for ($i = 0; $i -lt $SmallCount; $i++) {
    $t = $types[$i % $types.Length]
    $size = Next-Range 4096 65536
    $scenario = @('recycle', 'perm', 'wipe')[$i % 3]
    Add-Seed "small" ("s{0:d4}.{1}" -f $i, $t) $t ([long]$size) $scenario
}

# ---- medium files (1-8 MiB) ----
for ($i = 0; $i -lt $MediumCount; $i++) {
    $t = $types[$i % $types.Length]
    $size = Next-Range 1MB 8MB
    $scenario = if ($i % 4 -eq 3) { 'recycle' } else { 'perm' }
    Add-Seed "media" ("m{0:d3}.{1}" -f $i, $t) $t ([long]$size) $scenario
}

# ---- large files (16-48 MiB): perm + folder tree ----
for ($i = 0; $i -lt $LargeCount; $i++) {
    $size = Next-Range 16MB 48MB
    $scenario = if ($i % 2 -eq 0) { 'perm' } else { 'folder' }
    Add-Seed ("big/tree{0}" -f ($i % 3)) ("l{0:d2}.bin" -f $i) 'bin' ([long]$size) $scenario
}

# ---- keep group (control): must be found ACTIVE ----
for ($i = 0; $i -lt 10; $i++) {
    $t = $types[$i % $types.Length]
    Add-Seed "keep" ("k{0:d2}.{1}" -f $i, $t) $t ([long](Next-Range 8192 262144)) 'keep'
}

# ---- nested folder tree that gets deleted wholesale ----
for ($i = 0; $i -lt 12; $i++) {
    $sub = "doomed/n{0}/n{1}" -f ($i % 3), ($i % 2)
    Add-Seed $sub ("d{0:d2}.txt" -f $i) 'bin' ([long](Next-Range 2048 32768)) 'folder'
}

# ---- deletion scenarios ----

# recycle: send to the recycle bin (headless-safe VB API)
Add-Type -AssemblyName Microsoft.VisualBasic
foreach ($f in ($script:Files | Where-Object { $_.Scenario -eq 'recycle' })) {
    [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteFile(
        $f.Path, [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
        [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin)
}

# wipe: zero the content first, then permanently delete
foreach ($f in ($script:Files | Where-Object { $_.Scenario -eq 'wipe' })) {
    $fs = [System.IO.File]::OpenWrite($f.Path)
    try {
        $zero = New-Object byte[] 65536
        $left = $f.Size
        while ($left -gt 0) { $n = [int][Math]::Min(65536, $left); $fs.Write($zero, 0, $n); $left -= $n }
    } finally { $fs.Close() }
    Remove-Item -LiteralPath $f.Path -Force
}

# perm: straight permanent delete
foreach ($f in ($script:Files | Where-Object { $_.Scenario -eq 'perm' })) {
    Remove-Item -LiteralPath $f.Path -Force
}

# folder: delete the doomed subtree and big/tree* folders wholesale
$doomed = Join-Path $Root 'doomed'
if (Test-Path -LiteralPath $doomed) {
    Remove-Item -LiteralPath $doomed -Recurse -Force
}
Get-ChildItem (Join-Path $Root 'big') -Directory -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-Item -LiteralPath $_.FullName -Recurse -Force }

# flush the volume cache so the on-disk state is what the scanner sees
Write-VolumeCache -DriveLetter $Root[0] -ErrorAction SilentlyContinue | Out-Null

$manifest | Set-Content -Path $ManifestOut -Encoding utf8
$counts = $script:Files | Group-Object Scenario | ForEach-Object { "{0}={1}" -f $_.Name, $_.Count }
Write-Host "seeded $($script:Files.Count) files: $($counts -join ' ')"
Write-Host "manifest: $ManifestOut"
