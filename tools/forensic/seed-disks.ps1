# seed-disks.ps1 - forensic reproduction fixture: create + attach seeded VHDs.
#
# Creates two dynamic VHDXs on the runner:
#   disk1: 64 GB, GPT, one NTFS partition (X:), ~190 seeded files across
#          categories + deliberate deletions (the recovery targets)
#   disk2:  8 GB, MBR, one exFAT partition (Y:), ~60 seeded files
# Writes seed-manifest.json (every file: path, size, category, deleted flag)
# and disk-map.json (vhdx -> disk number, style, fs, letter).
#
# The corpus uses REAL signature bytes (JPEG/PNG/PDF/ZIP/MP4) so deep-scan
# carving has true positives to find; NTFS metadata stays intact for the
# quick-scan pass. This is evidence tooling: every step logs, every failure
# throws (no silent skips).

param(
    [string]$WorkDir = 'C:\forensic',
    [int]$Disk1GB = 64,
    [int]$Disk2GB = 8
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

function Write-Log([string]$msg) {
    Write-Host ("[seed] {0} {1}" -f (Get-Date -Format o), $msg)
}

# --- real-signature payload generators -------------------------------------

function New-JpegBytes([int]$kb) {
    # Minimal JFIF with real SOI/APP0 markers + payload + EOI.
    $ms = [System.IO.MemoryStream]::new()
    $ms.Write([byte[]](0xFF,0xD8,0xFF,0xE0,0,0x10,0x4A,0x46,0x49,0x46,0,1,1,0,0,1,0,1,0,0),0,20) # SOI+APP0 JFIF
    $payload = [byte[]]::new([Math]::Max(1, $kb * 1024 - 24))
    (New-Object Random 1234).NextBytes($payload)
    $ms.Write($payload, 0, $payload.Length)
    $ms.Write([byte[]](0xFF,0xD9),0,2) # EOI
    return $ms.ToArray()
}

function New-PngBytes([int]$kb) {
    $ms = [System.IO.MemoryStream]::new()
    $ms.Write([byte[]](0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A),0,8) # signature
    # IHDR chunk (length + type + 13 data bytes)
    $ihdr = [byte[]](0,0,0,13,0x49,0x48,0x44,0x52,0,0,1,0,0,0,1,0,8,2,0,0,0)
    $ms.Write($ihdr,0,$ihdr.Length)
    $payload = [byte[]]::new([Math]::Max(1, $kb * 1024 - 40))
    (New-Object Random 4321).NextBytes($payload)
    $ms.Write($payload, 0, $payload.Length)
    $ms.Write([byte[]](0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82),0,8) # IEND
    return $ms.ToArray()
}

function New-PdfBytes([int]$kb) {
    $head = [System.Text.Encoding]::ASCII.GetBytes("%PDF-1.4`n%evidence-fixture`n")
    $ms = [System.IO.MemoryStream]::new()
    $ms.Write($head,0,$head.Length)
    $payload = [byte[]]::new([Math]::Max(1, $kb * 1024 - $head.Length - 8))
    (New-Object Random 999).NextBytes($payload)
    $ms.Write($payload, 0, $payload.Length)
    $tail = [System.Text.Encoding]::ASCII.GetBytes("`n%%EOF`n")
    $ms.Write($tail,0,$tail.Length)
    return $ms.ToArray()
}

function New-Mp4Bytes([int]$kb) {
    # ftyp box + payload
    $ms = [System.IO.MemoryStream]::new()
    $ftyp = [byte[]](0,0,0,0x18,0x66,0x74,0x79,0x70,0x69,0x73,0x6F,0x6D,0,0,2,0,0x69,0x73,0x6F,0x6D,0x6D,0x70,0x34,0x31)
    $ms.Write($ftyp,0,$ftyp.Length)
    $payload = [byte[]]::new([Math]::Max(1, $kb * 1024 - 32))
    (New-Object Random 777).NextBytes($payload)
    $ms.Write($payload, 0, $payload.Length)
    return $ms.ToArray()
}

function New-ZipFile([string]$path, [int]$innerKB) {
    $tmp = Join-Path $env:TEMP ("zipsrc-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Force -Path $tmp | Out-Null
    $inner = Join-Path $tmp 'inner.txt'
    $bytes = [byte[]]::new($innerKB * 1024)
    (New-Object Random 555).NextBytes($bytes)
    [System.IO.File]::WriteAllBytes($inner, $bytes)
    Compress-Archive -Path $inner -DestinationPath $path -Force
    Remove-Item -Recurse -Force $tmp
}

function New-TxtBytes([string]$seedText, [int]$kb) {
    $sb = [System.Text.StringBuilder]::new()
    while ($sb.Length -lt ($kb * 1024)) {
        [void]$sb.AppendLine($seedText)
    }
    return [System.Text.Encoding]::UTF8.GetBytes($sb.ToString())
}

# --- vdisk creation via diskpart --------------------------------------------

function New-Vhd([string]$file, [int]$gb, [string]$style, [string]$fs, [string]$label, [string]$letter) {
    $script = Join-Path $WorkDir ("dp-" + [IO.Path]::GetFileNameWithoutExtension($file) + ".txt")
    $lines = @(
        "create vdisk file=`"$file`" type=expandable maximum=$($gb * 1024)",
        "select vdisk file=`"$file`"",
        "attach vdisk",
        "convert $style",
        "create partition primary",
        "format fs=$fs label=`"$label`" quick",
        "assign letter=$letter"
    )
    $lines | Set-Content -Path $script -Encoding ASCII
    $out = & diskpart /s $script 2>&1 | Out-String
    Write-Log "diskpart for $file ->`n$out"
    if ($LASTEXITCODE -ne 0) { throw "diskpart failed for $file (exit $LASTEXITCODE)" }
    $okCount = ([regex]::Matches($out, 'DiskPart successfully')).Count
    if ($okCount -lt 5) { throw "diskpart for $file reported only $okCount successes (expected >= 5): $out" }
}

Write-Log "creating disk1.vhdx ($Disk1GB GB GPT NTFS -> X:) and disk2.vhdx ($Disk2GB GB MBR exFAT -> Y:)"
New-Vhd -File "$WorkDir\disk1.vhdx" -gb $Disk1GB -style gpt -fs ntfs -label SeedNTFS -letter X
New-Vhd -File "$WorkDir\disk2.vhdx" -gb $Disk2GB -style mbr -fs exfat -label SeedEXFAT -letter Y

# --- record disk numbers -----------------------------------------------------

$disks = Get-Disk | Where-Object { $_.BusType -eq 'File Backed Virtual' } | Sort-Object Number
$diskMap = @()
$n = 0
foreach ($d in $disks) {
    $part = Get-Partition -DiskNumber $d.Number -ErrorAction SilentlyContinue | Select-Object -First 1
    $vol = if ($part) { Get-Volume -Partition $part } else { $null }
    $diskMap += [ordered]@{
        diskNumber = $d.Number
        style      = $d.PartitionStyle.ToString()
        sizeGB     = [math]::Round($d.Size / 1GB, 1)
        vhdx       = "disk$(1 + $n).vhdx"
        letter     = if ($vol) { $vol.DriveLetter } else { $null }
        fs         = if ($vol) { $vol.FileSystem } else { $null }
    }
    $n++
}
$diskMap | ConvertTo-Json -Depth 3 | Set-Content "$WorkDir\disk-map.json" -Encoding UTF8
Write-Log ("disk map: " + ($diskMap | ConvertTo-Json -Compress -Depth 3))

# --- seed corpus --------------------------------------------------------------

$manifest = [System.Collections.Generic.List[object]]::new()

function Add-SeedFile([string]$dir, [string]$name, [int]$kb, [string]$category, [string]$kind, [bool]$deleteAfter) {
    $path = Join-Path $dir $name
    switch ($kind) {
        'jpeg' { [System.IO.File]::WriteAllBytes($path, (New-JpegBytes $kb)) }
        'png'  { [System.IO.File]::WriteAllBytes($path, (New-PngBytes $kb)) }
        'pdf'  { [System.IO.File]::WriteAllBytes($path, (New-PdfBytes $kb)) }
        'mp4'  { [System.IO.File]::WriteAllBytes($path, (New-Mp4Bytes $kb)) }
        'zip'  { New-ZipFile -Path $path -innerKB ([Math]::Max(1, $kb / 2)) }
        'txt'  { [System.IO.File]::WriteAllBytes($path, (New-TxtBytes "forensic seed $name" $kb)) }
        default { throw "unknown kind $kind" }
    }
    $script:manifest.Add([ordered]@{
        disk     = if ($dir.StartsWith('X:')) { 'disk1' } else { 'disk2' }
        path     = $path
        name     = $name
        sizeKB   = $kb
        category = $category
        kind     = $kind
        deleted  = $deleteAfter
    })
}

# disk1 (X:, NTFS): documents / photos / videos / archives / work
$dirs = @('X:\Documents', 'X:\Documents\Reports', 'X:\Photos', 'X:\Photos\Summer2024', 'X:\Photos\Winter2025', 'X:\Videos', 'X:\Archives', 'X:\Work')
foreach ($d in $dirs) { New-Item -ItemType Directory -Force -Path $d | Out-Null }

# photos: 48 jpegs (16 deleted), Summer2024 pngs (all deleted), Winter2025 jpegs (kept)
for ($i = 1; $i -le 48; $i++) {
    Add-SeedFile 'X:\Photos' ("IMG_{0:d4}.jpg" -f $i) (200 + ($i % 7) * 130) 'graphics' 'jpeg' ($i % 3 -eq 0)
}
for ($i = 1; $i -le 12; $i++) {
    Add-SeedFile 'X:\Photos\Summer2024' ("SUM_{0:d3}.png" -f $i) (150 + $i * 40) 'graphics' 'png' $true
}
for ($i = 1; $i -le 10; $i++) {
    Add-SeedFile 'X:\Photos\Winter2025' ("WIN_{0:d3}.jpg" -f $i) (300 + $i * 50) 'graphics' 'jpeg' $false
}
# videos: 6 mp4s (2 deleted), including large ones
Add-SeedFile 'X:\Videos' 'clip_intro.mp4' 4096 'video' 'mp4' $false
Add-SeedFile 'X:\Videos' 'clip_deleteme.mp4' 8192 'video' 'mp4' $true
Add-SeedFile 'X:\Videos' 'clip_mid.mp4' 3072 'video' 'mp4' $false
Add-SeedFile 'X:\Videos' 'clip_deleteme2.mp4' 2048 'video' 'mp4' $true
Add-SeedFile 'X:\Videos' 'clip_long.mp4' 16384 'video' 'mp4' $false
Add-SeedFile 'X:\Videos' 'clip_short.mp4' 1024 'video' 'mp4' $false
# documents
for ($i = 1; $i -le 40; $i++) {
    Add-SeedFile 'X:\Documents' ("note_{0:d3}.txt" -f $i) (8 + $i) 'document' 'txt' ($i % 4 -eq 0)
}
for ($i = 1; $i -le 20; $i++) {
    Add-SeedFile 'X:\Documents\Reports' ("report_{0:d3}.pdf" -f $i) (120 + $i * 15) 'document' 'pdf' ($i % 5 -eq 0)
}
# archives
for ($i = 1; $i -le 10; $i++) {
    Add-SeedFile 'X:\Archives' ("backup_{0:d2}.zip" -f $i) (256 + $i * 100) 'archive' 'zip' ($i -le 3)
}
# work: mixed, including one big recoverable target (deleted)
for ($i = 1; $i -le 30; $i++) {
    Add-SeedFile 'X:\Work' ("file_{0:d3}.txt" -f $i) (16 + $i * 2) 'other' 'txt' ($i % 6 -eq 0)
}
Add-SeedFile 'X:\Work' 'BIG_deleted_video.mp4' 32768 'video' 'mp4' $true
Add-SeedFile 'X:\Work' 'presentation.pdf' 512 'document' 'pdf' $false

# disk2 (Y:, exFAT)
$dirs2 = @('Y:\Docs', 'Y:\Media', 'Y:\Misc')
foreach ($d in $dirs2) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
for ($i = 1; $i -le 20; $i++) {
    Add-SeedFile 'Y:\Docs' ("exf_note_{0:d3}.txt" -f $i) (8 + $i) 'document' 'txt' ($i % 7 -eq 0)
}
for ($i = 1; $i -le 25; $i++) {
    Add-SeedFile 'Y:\Media' ("exf_img_{0:d3}.jpg" -f $i) (120 + $i * 30) 'graphics' 'jpeg' ($i % 8 -eq 0)
}
for ($i = 1; $i -le 15; $i++) {
    Add-SeedFile 'Y:\Misc' ("exf_zip_{0:d2}.zip" -f $i) (180 + $i * 60) 'archive' 'zip' ($i -le 3)
}

# --- perform the deletions -----------------------------------------------------

$deletedPaths = @($manifest | Where-Object { $_.deleted } | ForEach-Object { $_.path })
foreach ($p in $deletedPaths) {
    Remove-Item -LiteralPath $p -Force
    if (Test-Path -LiteralPath $p) { throw "failed to delete $p" }
}

# --- manifest -------------------------------------------------------------------

$sumKB = ($manifest | Measure-Object -Property sizeKB -Sum).Sum
$out = [ordered]@{
    created  = (Get-Date -Format o)
    disk1    = [ordered]@{ vhdx = 'disk1.vhdx'; sizeGB = $Disk1GB; style = 'GPT'; fs = 'NTFS'; letter = 'X:' }
    disk2    = [ordered]@{ vhdx = 'disk2.vhdx'; sizeGB = $Disk2GB; style = 'MBR'; fs = 'exFAT'; letter = 'Y:' }
    files    = $manifest
    totals   = [ordered]@{
        total   = $manifest.Count
        deleted = @($manifest | Where-Object { $_.deleted }).Count
        bytes   = $sumKB * 1KB
    }
}
$out | ConvertTo-Json -Depth 5 | Set-Content "$WorkDir\seed-manifest.json" -Encoding UTF8
Write-Log ("manifest: {0} files, {1} deleted, {2:N0} KB total -> seed-manifest.json" -f $out.totals.total, $out.totals.deleted, $sumKB)
Write-Log "seed complete"
