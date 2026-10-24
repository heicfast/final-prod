# generate_seed.ps1 - deterministic, versioned seed corpus for the comparative bench.
#
# Seeds a volume with a known file matrix (types x sizes x attributes x depths
# x fragmentation), enriches the manifest with per-file extents
# (FSCTL_GET_RETRIEVAL_POINTERS) and NTFS record numbers, then emits:
#   seed-manifest.jsonl  (line 1 = _meta; then one JSON object per file)
#   manifest.csv         (rel,size,sha256,class,keep  - eyeball convenience)
#
# Deletion is NOT applied here - apply-scenario.ps1 owns deletion, so the same
# seed layout serves every scenario and both tools.
#
# Usage:
#   powershell -File bench\seed\generate_seed.ps1 -Root E:\seed `
#       -ManifestOut D:\work\seed-manifest.jsonl -CsvOut D:\work\manifest.csv `
#       -Profile full -FileSystem NTFS
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $Root,          # deletable corpus root, e.g. E:\seed
    [Parameter(Mandatory = $true)] [string] $ManifestOut,   # .jsonl out
    [string] $CsvOut = '',
    [ValidateSet('smoke', 'full')] [string] $Profile = 'full',
    [ValidateSet('NTFS', 'exFAT', 'FAT32', 'ReFS')] [string] $FileSystem = 'NTFS',
    [string] $VolumeMetaJson = '',   # optional volume.json for disk-level context
    [string] $DeletableTag = 'seed'   # manifest rel prefix; 'seedb' tags the 2nd volume of delete_partition runs
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\..\volume\volume.ps1"

$SEED_VERSION = '2.0.0'

# Deterministic PRNG (fixed seed => identical corpus across runners)
$script:Rng = [System.Random]::new(0x5EEDC0DE)
function Next-Bytes([byte[]] $buf) { $script:Rng.NextBytes($buf); return $buf }
function Next-Range([long] $min, [long] $max) {
    return [long]($min + $script:Rng.NextDouble() * ($max - $min + 1))
}

# deterministic timestamps (ground truth for preservation checks)
$T_CREATE = Get-Date '2026-01-15 08:09:10'
$T_WRITE = Get-Date '2026-01-16 09:10:11'
$T_ACCESS = Get-Date '2026-01-17 10:11:12'

# ---- type registry: real headers/footers over PRNG bodies --------------------
# hash: header bytes, footer bytes ('' = none)
$TypeMagic = @{
    'jpg'    = @([byte[]](0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01), [byte[]](0xFF,0xD9))
    'png'    = @([byte[]](0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52), [byte[]]@())
    'heic'   = @([byte[]](0x00,0x00,0x00,0x18,0x66,0x74,0x79,0x70,0x68,0x65,0x69,0x63,0x00,0x00,0x00,0x00,0x6D,0x66,0x68,0x64), [byte[]]@())
    'tiff'   = @([byte[]](0x49,0x49,0x2A,0x00,0x10,0x00,0x00,0x00,0x43,0x52), [byte[]]@())
    'cr2'    = @([byte[]](0x49,0x49,0x2A,0x00,0x10,0x00,0x00,0x00,0x43,0x52,0x02,0x00), [byte[]]@())
    'nef'    = @([byte[]](0x49,0x49,0x2A,0x00,0x10,0x00,0x00,0x00,0x4E,0x45,0x46,0x00), [byte[]]@())
    'arw'    = @([byte[]](0x49,0x49,0x2A,0x00,0x10,0x00,0x00,0x00,0x53,0x4F,0x4E,0x59), [byte[]]@())
    'dng'    = @([byte[]](0x49,0x49,0x2A,0x00,0x10,0x00,0x00,0x00,0x43,0x52,0x03,0x00), [byte[]]@())
    'mp4'    = @([byte[]](0x00,0x00,0x00,0x18,0x66,0x74,0x79,0x70,0x69,0x73,0x6F,0x6D), [byte[]]@())
    'mov'    = @([byte[]](0x00,0x00,0x00,0x14,0x66,0x74,0x79,0x70,0x71,0x74,0x20,0x20), [byte[]]@())
    'avi'    = @([byte[]](0x52,0x49,0x46,0x46,0x00,0x00,0x00,0x00,0x41,0x56,0x49,0x20), [byte[]]@())
    'mkv'    = @([byte[]](0x1A,0x45,0xDF,0xA3,0xA3,0x42,0x86,0x81,0x01,0x00,0x00,0x00), [byte[]]@())
    'pdf'    = @([System.Text.Encoding]::ASCII.GetBytes("%PDF-1.4`n% bench seed`n"), [System.Text.Encoding]::ASCII.GetBytes("`n%%EOF"))
    'zip'    = @([byte[]](0x50,0x4B,0x03,0x04,0x14,0x00,0x00,0x00,0x08,0x00), [byte[]]@())
    '7z'     = @([byte[]](0x37,0x7A,0xBC,0xAF,0x27,0x1C,0x00,0x04), [byte[]]@())
    'rar5'   = @([byte[]](0x52,0x61,0x72,0x21,0x1A,0x07,0x01,0x00), [byte[]]@())
    'sqlite' = @([System.Text.Encoding]::ASCII.GetBytes("SQLite format 3`0"), [byte[]]@())
    'pst'    = @([byte[]](0x21,0x42,0x44,0x4E,0x00,0x00,0x00,0x00), [byte[]]@())
    'txt'    = @([System.Text.Encoding]::ASCII.GetBytes("bench-seed text`n"), [byte[]]@())
    'bin'    = @([byte[]](0xB5,0x5E,0xF0,0x7D), [byte[]]@())
}
# docx/xlsx/pptx are ZIP containers (PK) - distinct extensions, same magic
$TypeMagic['docx'] = $TypeMagic['zip']
$TypeMagic['xlsx'] = $TypeMagic['zip']
$TypeMagic['pptx'] = $TypeMagic['zip']

$script:Entries = @()
function Write-SeedBody([string] $Path, [string] $Type, [long] $Size) {
    $magic = $TypeMagic[$Type]
    $fs = [System.IO.File]::Create($Path)
    try {
        if ($Size -lt $magic[0].Length) {
            # tiny file: truncate header to fit
            $h = New-Object byte[] $Size
            [Array]::Copy($magic[0], $h, $Size)
            $fs.Write($h, 0, $Size)
            return
        }
        $fs.Write($magic[0], 0, $magic[0].Length)
        $footerReserve = $magic[1].Length
        $remaining = $Size - $fs.Position - $footerReserve
        $buf = New-Object byte[] 65536
        while ($remaining -gt 0) {
            $n = [int][Math]::Min([long]65536, [long]$remaining)
            $filled = New-Object byte[] $n
            [void](Next-Bytes $filled)
            [Array]::Copy($filled, $buf, $n)
            $fs.Write($buf, 0, $n)
            $remaining -= $n
        }
        if ($footerReserve -gt 0) { $fs.Write($magic[1], 0, $footerReserve) }
    } finally { $fs.Close() }
}

function Add-Seed {
    param([string] $RelDir, [string] $Name, [string] $Type, [long] $Size,
           [string] $Class, [string[]] $Attrs = @(), [bool] $Keep = $false,
           [string] $Frag = 'contiguous', [string] $RootPrefix = 'seed')
    $dir = Join-Path $Root $RelDir
    if (-not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
    $path = Join-Path $dir $Name
    if ($Frag -eq 'contiguous') { Write-SeedBody $path $Type $Size }
    $item = Get-Item -LiteralPath $path
    $item.CreationTime = $T_CREATE
    $item.LastWriteTime = $T_WRITE
    $item.LastAccessTime = $T_ACCESS
    foreach ($a in $Attrs) {
        if ($a -eq 'readonly') { Set-ItemProperty -LiteralPath $path -Name IsReadOnly -Value $true }
        if ($a -eq 'hidden') { $item.Attributes = $item.Attributes -bor [IO.FileAttributes]::Hidden }
        if ($a -eq 'system') { $item.Attributes = $item.Attributes -bor [IO.FileAttributes]::System }
    }
    $rel = "$RootPrefix/$RelDir/$Name" -replace '\/', '/'
    $rel = $rel -replace '//', '/'
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:Entries += [pscustomobject]@{
        rel = $rel; name = $Name; type = $Type; size = $Size; sha256 = $hash
        class = $Class; attrs = $Attrs; keep = $Keep; frag = $Frag
        ads = @(); hardlink_group = 0; sparse = $false; long_path = $false
        cjk = $false; depth = ($RelDir -split '[\\/]').Count
        mft_record = $null; extents = $null; extents_error = $null
        ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
    }
    return $path
}

# ---- profile matrix -----------------------------------------------------------
$full = $false
if ($Profile -eq 'full') { $full = $true }

# capability map per filesystem (recorded skips - never silent)
# ADS: NTFS ONLY. ReFS has no named data streams (MS "Resilient File System
# overview"; CreateFile on <file>:<stream> fails with ERROR_INVALID_PARAMETER)
# - the map previously claimed ads on ReFS and every refs seeding would have
# died at Add-Content -Stream under ErrorActionPreference=Stop. Hard links:
# both NTFS and ReFS. Sparse: NTFS (ReFS sparse is supported by the fs but
# fsutil setrange semantics differ; kept NTFS-only conservatively).
$cap = @{
    ads = ($FileSystem -eq 'NTFS')
    hardlink = ($FileSystem -eq 'NTFS' -or $FileSystem -eq 'ReFS')
    sparse = ($FileSystem -eq 'NTFS')
    long_path = ($FileSystem -ne 'FAT32')
}

$types = @('jpg','png','heic','tiff','cr2','nef','arw','dng','mp4','mov','avi','mkv',
           'pdf','docx','xlsx','pptx','zip','7z','rar5','sqlite','txt','pst','bin')

# tiny (1-3.9 KB) - attribute ring
$tinyCount = 12; if ($full) { $tinyCount = 40 }
$attrRing = @(@(), @('readonly'), @('hidden'), @('system'))
for ($i = 0; $i -lt $tinyCount; $i++) {
    $t = $types[$i % $types.Length]
    Add-Seed 'tiny' ("t{0:d3}.{1}" -f $i, $t) $t (Next-Range 1024 3999) 'tiny' $attrRing[$i % 4] | Out-Null
}

# small (4 KB - 1 MB)
$smallCount = 20; if ($full) { $smallCount = 60 }
for ($i = 0; $i -lt $smallCount; $i++) {
    $t = $types[$i % $types.Length]
    $attrs = @()
    if ($i % 7 -eq 3) { $attrs = @('readonly') }
    if ($i % 11 -eq 5) { $attrs = @('hidden') }
    Add-Seed 'small' ("s{0:d3}.{1}" -f $i, $t) $t (Next-Range 4096 1048576) 'small' $attrs | Out-Null
}

# medium (1-8 MB)
$mediumCount = 8; if ($full) { $mediumCount = 30 }
for ($i = 0; $i -lt $mediumCount; $i++) {
    $t = $types[($i + 4) % $types.Length]
    Add-Seed 'media' ("m{0:d3}.{1}" -f $i, $t) $t (Next-Range 1MB 8MB) 'medium' | Out-Null
}

# large (50-200 MB) incl. the 200 MB single-file probe
Add-Seed 'big' 'big200.bin' 'bin' 200MB 'large-single' | Out-Null
Add-Seed 'big' 'l060.jpg' 'jpg' 60MB 'large' | Out-Null
Add-Seed 'big' 'l080.mp4' 'mp4' 80MB 'large' | Out-Null
Add-Seed 'big' 'l064.zip' 'zip' 64MB 'large' | Out-Null
if (-not $full) {
    # smoke: keep the volume small - drop big200, keep 3 larges
    $script:Entries = $script:Entries | Where-Object { $_.rel -ne 'seed/big/big200.bin' }
}

# cap-test (>500 MB: expected to hit the og free limit on recovery)
$capSize = 250MB; if ($full) { $capSize = 600MB }
Add-Seed 'big' 'cap600.bin' 'bin' $capSize 'cap-test' | Out-Null

# many-small-files
$manyCount = 500; if ($full) { $manyCount = 10000 }
$manyDir = Join-Path $Root 'manysmall'
New-Item -ItemType Directory -Path $manyDir -Force | Out-Null
$manyBuf = New-Object byte[] 4096
for ($i = 0; $i -lt $manyCount; $i++) {
    $p = Join-Path $manyDir ("f{0:d5}.txt" -f $i)
    $fs = [System.IO.File]::Create($p)
    try { [void](Next-Bytes $manyBuf); $fs.Write($manyBuf, 0, 4096) } finally { $fs.Close() }
    $item = Get-Item -LiteralPath $p
    $item.CreationTime = $T_CREATE; $item.LastWriteTime = $T_WRITE; $item.LastAccessTime = $T_ACCESS
    $hash = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:Entries += [pscustomobject]@{
        rel = "seed/manysmall/f{0:d5}.txt" -f $i; name = "f{0:d5}.txt" -f $i; type = 'txt'
        size = 4096; sha256 = $hash; class = 'many-small'; attrs = @(); keep = $false
        frag = 'contiguous'; ads = @(); hardlink_group = 0; sparse = $false
        long_path = $false; cjk = $false; depth = 2
        mft_record = $null; extents = $null; extents_error = $null
        ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
    }
}

# nested depth (6 levels)
$nestedCount = 6; if ($full) { $nestedCount = 12 }
for ($i = 0; $i -lt $nestedCount; $i++) {
    $sub = "nested/d{0}/d{1}/d{2}/d{3}/d{4}/d{5}" -f ($i % 2), ($i % 3), ($i % 2), ($i % 3), ($i % 2), ($i % 3)
    $t = $types[$i % $types.Length]
    Add-Seed $sub ("n{0:d2}.{1}" -f $i, $t) $t (Next-Range 4096 262144) 'nested' | Out-Null
}

# long path (>260 chars total) - via \\?\ extended paths
if ($cap.long_path) {
    $lpCount = 3; if ($full) { $lpCount = 6 }
    for ($i = 0; $i -lt $lpCount; $i++) {
        $seg = 'lvl{0:d2}_0123456789012345678901234567890123456789' -f $i
        $sub = "longpath/$seg/$seg/$seg/$seg"
        $dir = Join-Path $Root $sub
        $longDir = "\\?\$dir"
        [System.IO.Directory]::CreateDirectory($longDir) | Out-Null
        $name = "lp{0:d2}_0123456789012345678901234567890123456789.dat" -f $i
        $p = "\\?\$dir\$name"
        Write-SeedBody $p 'bin' (Next-Range 8192 131072)
        # hash via short 8.3-safe path is impossible; use the \\?\ path
        $hash = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()
        $item = Get-Item -LiteralPath $p
        $item.CreationTime = $T_CREATE; $item.LastWriteTime = $T_WRITE; $item.LastAccessTime = $T_ACCESS
        $script:Entries += [pscustomobject]@{
            rel = "seed/$sub/$name"; name = $name; type = 'bin'
            size = $item.Length; sha256 = $hash; class = 'long-path'; attrs = @()
            keep = $false; frag = 'contiguous'; ads = @(); hardlink_group = 0
            sparse = $false; long_path = $true; cjk = $false; depth = 5
            mft_record = $null; extents = $null; extents_error = $null
            ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
        }
    }
}

# CJK / Unicode names
$cjkNames = @('照片_风景001.jpg', '動画_クリップ002.mp4', '文档_报告003.docx', '音乐_歌曲004.bin',
              '数据_备份005.zip', 'архив_данные006.txt', '日本語_ファイル007.pdf', '한국어_문서008.xlsx')
$cjkTypes = @('jpg', 'mp4', 'docx', 'bin', 'zip', 'txt', 'pdf', 'xlsx')
$cjkDir = Join-Path $Root 'unicode/日本語のフォルダ'
New-Item -ItemType Directory -Path $cjkDir -Force | Out-Null
for ($i = 0; $i -lt $cjkNames.Count; $i++) {
    $p = Join-Path $cjkDir $cjkNames[$i]
    Write-SeedBody $p $cjkTypes[$i] (Next-Range 8192 524288)
    $item = Get-Item -LiteralPath $p
    $item.CreationTime = $T_CREATE; $item.LastWriteTime = $T_WRITE; $item.LastAccessTime = $T_ACCESS
    $hash = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:Entries += [pscustomobject]@{
        rel = "seed/unicode/日本語のフォルダ/$($cjkNames[$i])"; name = $cjkNames[$i]
        type = $cjkTypes[$i]; size = $item.Length; sha256 = $hash; class = 'cjk'
        attrs = @(); keep = $false; frag = 'contiguous'; ads = @(); hardlink_group = 0
        sparse = $false; long_path = $false; cjk = $true; depth = 3
        mft_record = $null; extents = $null; extents_error = $null
        ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
    }
}

# fragmentation groups: interleaved writers (frag measured later via extents)
$fragGroups = 2; if ($full) { $fragGroups = 6 }
$fragDir = Join-Path $Root 'frag'
New-Item -ItemType Directory -Path $fragDir -Force | Out-Null
$handles = @(); $meta = @()
for ($i = 0; $i -lt $fragGroups; $i++) {
    $p = Join-Path $fragDir ("frag{0:d2}.bin" -f $i)
    $handles += [System.IO.File]::Create($p)
    $meta += @{ path = $p; rel = "seed/frag/frag{0:d2}.bin" -f $i; size = 4MB }
}
$chunk = 65536
$cbuf = New-Object byte[] $chunk
$rounds = [int](4MB / $chunk)
for ($r = 0; $r -lt $rounds; $r++) {
    for ($f = 0; $f -lt $handles.Count; $f++) {
        [void](Next-Bytes $cbuf)
        $handles[$f].Write($cbuf, 0, $chunk)
    }
}
foreach ($h in $handles) { $h.Close() }
foreach ($m in $meta) {
    $item = Get-Item -LiteralPath $m.path
    $item.CreationTime = $T_CREATE; $item.LastWriteTime = $T_WRITE; $item.LastAccessTime = $T_ACCESS
    $hash = (Get-FileHash -LiteralPath $m.path -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:Entries += [pscustomobject]@{
        rel = $m.rel; name = [IO.Path]::GetFileName($m.path); type = 'bin'
        size = $item.Length; sha256 = $hash; class = 'fragmented'; attrs = @()
        keep = $false; frag = 'interleaved'; ads = @(); hardlink_group = 0
        sparse = $false; long_path = $false; cjk = $false; depth = 2
        mft_record = $null; extents = $null; extents_error = $null
        ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
    }
}

# highly fragmented: smaller chunks, more files
$hfCount = 3; if ($full) { $hfCount = 6 }
$hfDir = Join-Path $Root 'fraghigh'
New-Item -ItemType Directory -Path $hfDir -Force | Out-Null
$handles = @(); $meta = @()
for ($i = 0; $i -lt $hfCount; $i++) {
    $p = Join-Path $hfDir ("hfrag{0:d2}.bin" -f $i)
    $handles += [System.IO.File]::Create($p)
    $meta += @{ path = $p; rel = "seed/fraghigh/hfrag{0:d2}.bin" -f $i; size = 4MB }
}
$hchunk = 16384
$hbuf = New-Object byte[] $hchunk
$hrounds = [int](4MB / $hchunk)
for ($r = 0; $r -lt $hrounds; $r++) {
    for ($f = 0; $f -lt $handles.Count; $f++) {
        [void](Next-Bytes $hbuf)
        $handles[$f].Write($hbuf, 0, $hchunk)
    }
}
foreach ($h in $handles) { $h.Close() }
foreach ($m in $meta) {
    $item = Get-Item -LiteralPath $m.path
    $item.CreationTime = $T_CREATE; $item.LastWriteTime = $T_WRITE; $item.LastAccessTime = $T_ACCESS
    $hash = (Get-FileHash -LiteralPath $m.path -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:Entries += [pscustomobject]@{
        rel = $m.rel; name = [IO.Path]::GetFileName($m.path); type = 'bin'
        size = $item.Length; sha256 = $hash; class = 'high-fragmented'; attrs = @()
        keep = $false; frag = 'highfrag'; ads = @(); hardlink_group = 0
        sparse = $false; long_path = $false; cjk = $false; depth = 2
        mft_record = $null; extents = $null; extents_error = $null
        ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
    }
}

# ADS (NTFS/ReFS)
if ($cap.ads) {
    $adsCount = 4; if ($full) { $adsCount = 8 }
    for ($i = 0; $i -lt $adsCount; $i++) {
        $relDir = 'ads'; $name = "ads{0:d2}.txt" -f $i
        $p = Add-Seed $relDir $name 'txt' (Next-Range 4096 65536) 'ads'
        $streamName = 'ads1'
        if ($i % 3 -eq 0) { $streamName = 'zone_identifier' }
        $payload = "bench-ads-payload-{0:d4}-0123456789ABCDEF" -f $i
        Add-Content -LiteralPath $p -Stream $streamName -Value $payload
        $entry = $script:Entries | Where-Object { $_.rel -eq "seed/$relDir/$name" } | Select-Object -First 1
        $payloadBytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
        $shaProvider = [System.Security.Cryptography.SHA256]::Create()
        $streamHash = [BitConverter]::ToString($shaProvider.ComputeHash($payloadBytes)).Replace('-', '').ToLowerInvariant()
        $entry.ads = @(@{ name = $streamName; size = [System.Text.Encoding]::UTF8.GetByteCount($payload); sha256 = $streamHash })
    }
}

# hard links (NTFS/ReFS)
if ($cap.hardlink) {
    $hlCount = 2; if ($full) { $hlCount = 4 }
    for ($i = 0; $i -lt $hlCount; $i++) {
        $a = Add-Seed 'hardlinks' ("hl_a{0:d2}.bin" -f $i) 'bin' (Next-Range 65536 1048576) 'hardlink'
        $b = Join-Path $Root ("hardlinks\hl_b{0:d2}.bin" -f $i)
        New-Item -ItemType HardLink -Path $b -Target $a | Out-Null
        $hb = (Get-FileHash -LiteralPath $b -Algorithm SHA256).Hash.ToLowerInvariant()
        $script:Entries += [pscustomobject]@{
            rel = "seed/hardlinks/hl_b{0:d2}.bin" -f $i; name = "hl_b{0:d2}.bin" -f $i
            type = 'bin'; size = (Get-Item -LiteralPath $b).Length; sha256 = $hb
            class = 'hardlink'; attrs = @(); keep = $false; frag = 'contiguous'
            ads = @(); hardlink_group = 100 + $i; sparse = $false; long_path = $false
            cjk = $false; depth = 2; mft_record = $null; extents = $null; extents_error = $null
            ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
        }
        $ea = $script:Entries | Where-Object { $_.rel -eq "seed/hardlinks/hl_a{0:d2}.bin" -f $i } | Select-Object -First 1
        $ea.hardlink_group = 100 + $i
    }
}

# sparse files (NTFS)
if ($cap.sparse) {
    $spCount = 2; if ($full) { $spCount = 3 }
    for ($i = 0; $i -lt $spCount; $i++) {
        $p = Add-Seed 'sparse' ("sp{0:d2}.bin" -f $i) 'bin' 24MB 'sparse'
        # sparsify the middle 12 MB
        cmd /c "fsutil sparse setflag `"$p`"" | Out-Null
        cmd /c "fsutil sparse setrange `"$p`" 6291456 12582912" | Out-Null
        $entry = $script:Entries | Where-Object { $_.rel -eq "seed/sparse/sp{0:d2}.bin" -f $i } | Select-Object -First 1
        $entry.sparse = $true
    }
}

# doomed tree (folder-delete scenario target)
$doomCount = 8; if ($full) { $doomCount = 24 }
for ($i = 0; $i -lt $doomCount; $i++) {
    $sub = "doomed/g{0}/g{1}" -f ($i % 3), ($i % 2)
    $t = $types[$i % $types.Length]
    Add-Seed $sub ("d{0:d2}.{1}" -f $i, $t) $t (Next-Range 2048 65536) 'doomed' | Out-Null
}

# keep control group (never deleted; sibling root OUTSIDE the deletable root)
$keepRoot = Split-Path $Root -Parent
$keepDir = Join-Path $keepRoot 'keep'
New-Item -ItemType Directory -Path $keepDir -Force | Out-Null
$keepCount = 8; if ($full) { $keepCount = 12 }
for ($i = 0; $i -lt $keepCount; $i++) {
    $t = $types[$i % $types.Length]
    $p = Join-Path $keepDir ("k{0:d2}.{1}" -f $i, $t)
    Write-SeedBody $p $t (Next-Range 8192 262144)
    $item = Get-Item -LiteralPath $p
    $item.CreationTime = $T_CREATE; $item.LastWriteTime = $T_WRITE; $item.LastAccessTime = $T_ACCESS
    $hash = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:Entries += [pscustomobject]@{
        rel = "keep/k{0:d2}.{1}" -f $i, $t; name = "k{0:d2}.{1}" -f $i, $t; type = $t
        size = $item.Length; sha256 = $hash; class = 'keep'; attrs = @(); keep = $true
        frag = 'contiguous'; ads = @(); hardlink_group = 0; sparse = $false
        long_path = $false; cjk = $false; depth = 1
        mft_record = $null; extents = $null; extents_error = $null
        ts_create = $T_CREATE.ToString('o'); ts_write = $T_WRITE.ToString('o')
    }
}

# ---- enrichment: extents + NTFS record numbers

# remap the deletable prefix when tagging the second volume (delete_partition)
if ($DeletableTag -ne 'seed') {
    foreach ($e in $script:Entries) {
        if (-not $e.keep -and $e.rel.StartsWith('seed/')) {
            $e.rel = $e.rel -replace '^seed/', "$DeletableTag/"
        }
    }
}

# guarantee optional fields exist on every entry (inline literals and
# Add-Seed literals alike) so later property assignment cannot throw
foreach ($e in $script:Entries) {
    if (-not ($e.PSObject.Properties['frag_measured_extents'])) {
        Add-Member -InputObject $e -NotePropertyName frag_measured_extents -NotePropertyValue $null
    }
    if (-not ($e.PSObject.Properties['frag_measured_ratio'])) {
        Add-Member -InputObject $e -NotePropertyName frag_measured_ratio -NotePropertyValue $null
    }
    if (-not ($e.PSObject.Properties['frag_measured_anomaly'])) {
        Add-Member -InputObject $e -NotePropertyName frag_measured_anomaly -NotePropertyValue $null
    }
}
# extents + NTFS record numbers enrichment follows
$lettersOk = $true
foreach ($e in $script:Entries) {
    $winRel = ($e.rel -replace '^seed/', '' -replace '^keep/', '')
    $base = if ($e.rel.StartsWith('seed/')) { $Root } else { $keepRoot }
    $p = Join-Path $base $winRel
    if ($e.long_path) { $p = "\\?\$p" }
    try {
        $ext = [BenchRaw]::GetExtents($p)
        $e.extents = @()
        foreach ($x in $ext) { $e.extents += ,@([long]$x[0], [long]$x[1]) }
    } catch {
        $e.extents_error = $_.Exception.Message
    }
    try {
        $e.mft_record = [BenchRaw]::GetFileIndex($p) -band 0xFFFFFFFFFFFF
    } catch {
        $e.mft_record = $null
    }
}
# measured fragmentation (proof, not claim)
foreach ($e in $script:Entries) {
    if ($e.extents -and $e.extents.Count -gt 0) {
        [long]$clusters = 0
        foreach ($x in $e.extents) { [long]$clusters += [long]$x[1] }
        $e.frag_measured_extents = $e.extents.Count
        if ($clusters -gt 0) {
            $e.frag_measured_ratio = [math]::Round([double]$e.extents.Count / [double][math]::Max([double]1, [double]$clusters), 4)
        }
        if ($clusters -gt 2147483647) {
            # implausible cluster total recorded as an anomaly with the raw extents
            # (diagnosable from the manifest artifact; never silently clamped)
            $raw = ($e.extents | ForEach-Object { "[$($_[0]),$($_[1])]" }) -join ' '
            $e.frag_measured_anomaly = "cluster total $clusters exceeds int32 range; extents: $raw"
        }
    } else {
        $e.frag_measured_extents = $null; $e.frag_measured_ratio = $null
    }
}

# ---- emit ----------------------------------------------------------------------
$meta = [ordered]@{
    _meta = $true; seed_version = $SEED_VERSION; profile = $Profile
    filesystem = $FileSystem; root = $Root; keep_root = $keepRoot
    entries = $script:Entries.Count
    deterministic = 'System.Random(0x5EEDC0DE)'
    capabilities = $cap
    total_bytes = ($script:Entries | Measure-Object -Property size -Sum).Sum
}
$meta | ConvertTo-Json -Depth 4 -Compress | Set-Content -Path $ManifestOut -Encoding utf8
foreach ($e in $script:Entries) {
    $e | ConvertTo-Json -Depth 5 -Compress | Add-Content -Path $ManifestOut -Encoding utf8
}
if ($CsvOut) {
    $lines = @('rel,size,sha256,class,keep,frag')
    foreach ($e in $script:Entries) {
        $lines += "$($e.rel),$($e.size),$($e.sha256),$($e.class),$($e.keep),$($e.frag)"
    }
    $lines | Set-Content -Path $CsvOut -Encoding utf8
}
Write-Host "seeded $($script:Entries.Count) files ($((($script:Entries | Measure-Object -Property size -Sum).Sum / 1MB).ToString('0.0')) MiB), profile=$Profile fs=$FileSystem"
Write-Host "manifest: $ManifestOut"
