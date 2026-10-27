# stage-disk.ps1 - stage a VHD with a filesystem + the versioned seed
# manifest, then run the scripted deletion. The staged evidence (seed-hashes,
# deleted list, disk number) is the GROUND TRUTH the verification step
# compares against - derived from the manifest + deletion semantics, never
# from either tool's output (docs/CI-SCENARIOS.md).
#
# Usage: stage-disk.ps1 -VhdPath disk.vhdx -SizeMB 256 -Fs NTFS|FAT32|exFAT
#        [-QuickFormat] -Manifest .github/scenarios/seed-manifest.json
#        -OutDir staging
#
# Deterministic content: bytes generated from a per-(name,size) FNV-seeded
# LCG - identical across runs; category payloads start with the real magic
# bytes (JPEG SOI/EOI, ZIP EOCD, MP4 ftyp) so the carve layer can validate
# grammar, not just the engines' metadata paths.
param(
    [Parameter(Mandatory = $true)][string]$VhdPath,
    [Parameter(Mandatory = $true)][int]$SizeMB,
    [Parameter(Mandatory = $true)][ValidateSet('NTFS', 'FAT32', 'exFAT')][string]$Fs,
    [switch]$QuickFormat,   # S-FMT-*: format AFTER seeding (the quick format
                            # IS the deletion method - everything is deleted)
    [Parameter(Mandatory = $true)][string]$Manifest,
    [Parameter(Mandatory = $true)][string]$OutDir
)
$ErrorActionPreference = 'Stop'

function Fail($msg) { Write-Host "::error::$msg"; exit 1 }

New-Item -ItemType Directory -Force $OutDir | Out-Null
$manifest = Get-Content $Manifest -Raw | ConvertFrom-Json

# ---- deterministic content generator (FNV-seeded LCG over the payload) ----
function Get-SeedBytes([string]$name, [long]$size, [string]$category) {
    # FNV-1a over the name -> LCG state; category magic prefixes the stream
    $h = [uint32]0x811C9DC5
    foreach ($c in [IO.Encoding]::UTF8.GetBytes($name)) { $h = ($h -bxor $c) * [uint32]0x01000193 }
    $state = [uint64]$h
    $bytes = New-Object byte[] $size
    $magic = @{
        jpeg = [byte[]](0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46)
        zip  = [byte[]](0x50, 0x4B, 0x05, 0x06)   # EOCD record head
        mp4  = [byte[]](0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70)
    }
    $i = 0
    if ($magic.ContainsKey($category)) {
        foreach ($b in $magic[$category]) { if ($i -lt $size) { $bytes[$i++] = $b } }
    }
    while ($i -lt $size) {
        $state = ($state * [uint64]6364136223846793005 + [uint64]1442695040888963407) -band [uint64]0xFFFFFFFFFFFFFFFF
        $bytes[$i++] = [byte](($state -shr 33) -band 0xFF)
    }
    if ($category -eq 'jpeg' -and $size -ge 4) { $bytes[$size - 2] = 0xFF; $bytes[$size - 1] = 0xD9 }
    return $bytes
}

# ---- create + attach the VHD ----
$vhdDir = Split-Path -Parent $VhdPath
if ($vhdDir) { New-Item -ItemType Directory -Force $vhdDir | Out-Null }
$before = @(Get-Disk | ForEach-Object { $_.Number })
New-VHD -Path $VhdPath -SizeBytes ([long]$SizeMB * 1MB) -Fixed | Out-Null
Mount-VHD -Path $VhdPath
$after = @(Get-Disk | ForEach-Object { $_.Number })
$new = @($after | Where-Object { $before -notcontains $_ })
if ($new.Count -ne 1) { Fail "expected exactly one new disk after VHD attach, saw $($new.Count)" }
$disk = [int]($new | Select-Object -First 1)
Write-Host "staged disk: $disk ($VhdPath, $SizeMB MB, $Fs)"

# ---- initialize + format (the initial format; S-FMT re-formats after seeding) ----
Initialize-Disk -Number $disk -PartitionStyle MBR
$part = New-Partition -DiskNumber $disk -UseMaximumSize -AssignDriveLetter
$vol = Format-Volume -Partition $part -FileSystem $Fs -NewFileSystemLabel "SCRATCH" -Force -Confirm:$false
$letter = $vol.DriveLetter
if (-not $letter) { Fail "no drive letter assigned to the staged volume" }
$root = "$letter`:\"
Write-Host "formatted: disk=$disk letter=$letter fs=$Fs format-deletion=$([bool]$QuickFormat)"

# ---- seed the manifest (hashes recorded at stage time = the ground truth) ----
$seedHashes = [ordered]@{}
$deep = "$root\deep\nested"
New-Item -ItemType Directory -Force $deep | Out-Null
foreach ($f in $manifest.files) {
    $target = if ($f.name -like 'del_deep_*') { Join-Path $deep $f.name } else { Join-Path $root $f.name }
    $bytes = Get-SeedBytes $f.name ([long]$f.size) $f.category
    [IO.File]::WriteAllBytes($target, $bytes)
    foreach ($a in @($f.attrs)) {
        if ($a -eq 'readonly') { Set-ItemProperty $target -Name IsReadOnly -Value $true }
        if ($a -eq 'hidden') { (Get-Item $target -Force).Attributes += 'Hidden' }
    }
    $seedHashes[$f.name] = (Get-FileHash $target -Algorithm SHA256).Hash
}

# ---- the deletion method (recorded; expected set derives from it) ----
$deleted = @()
if ($QuickFormat) {
    # S-FMT-*: the quick format IS the deletion method - re-format the
    # SEEDED volume (metadata rebuilt; every seeded file is deleted)
    Format-Volume -DriveLetter $letter -FileSystem $Fs -Force -Confirm:$false
    foreach ($f in @($manifest.files | Where-Object { $_.size -gt 0 })) { $deleted += $f.name }
    Write-Host "deletion method: quick-format over the seeded volume ($($deleted.Count) file(s) deleted)"
} else {
    foreach ($f in @($manifest.files | Where-Object { $_.deleted })) {
        $target = if ($f.name -like 'del_deep_*') { Join-Path $deep $f.name } else { Join-Path $root $f.name }
        # readonly files must clear the bit before deletion (the recorded method)
        Set-ItemProperty $target -Name IsReadOnly -Value $false -ErrorAction SilentlyContinue
        Remove-Item $target -Force   # recycle-bypassing delete: the deletion semantics
        $deleted += $f.name
    }
}

# ---- the expected-recoverable subset (manifest + deletion semantics) ----
# Remove-Item -Force path: deleted && size > 0 (the FS metadata entry - MFT
# record / dirent - survives, marked deleted). Format path: every seeded
# file with size > 0 (the format deleted them all; recovery routes through
# the deep/carve layers). Zero-byte files are the documented exclusion (no
# data extent to prove recovery by hash).
$expected = if ($QuickFormat) {
    @($manifest.files | Where-Object { $_.size -gt 0 } | ForEach-Object { $_.name })
} else {
    @($manifest.files | Where-Object { $_.deleted -and $_.size -gt 0 } | ForEach-Object { $_.name })
}

[ordered]@{
    disk_number = $disk
    drive_letter = $letter
    filesystem = $Fs
    quick_format = [bool]$QuickFormat
    vhd = $VhdPath
    deleted = $deleted
    expected_recoverable = $expected
    seed_hashes = $seedHashes
    manifest_version = $manifest.version
} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $OutDir 'staged-truth.json')
Write-Host "staged truth: $($expected.Count) expected recoverable of $($deleted.Count) deleted"
exit 0
