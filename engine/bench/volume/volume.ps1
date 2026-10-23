# volume.ps1 - VHDX volume management + raw-volume access for the bench.
#
# Dot-source:  . $PSScriptRoot\volume.ps1
#
# Provides:
#   New-BenchVolume / Remove-BenchVolume / Get-BenchVolumeJson
#   [BenchRaw] C# helpers: raw open/lock/dismount, FSCTL_GET_RETRIEVAL_POINTERS,
#   BY_HANDLE file id, NTFS volume data (cluster size / MFT LCN), MFT-record
#   read+fixup, $Bitmap run parse, pattern writes, volume hash-map capture.
#
# Licensed under the MIT license. See LICENSE.

$ErrorActionPreference = 'Stop'

if (-not ('BenchRaw' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using Microsoft.Win32.SafeHandles;

public static class BenchRaw
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern SafeFileHandle CreateFileW(string name, uint access, uint share, IntPtr security, uint disposition, uint flags, IntPtr template);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool DeviceIoControl(SafeFileHandle h, uint control, byte[] inBuf, uint inLen, byte[] outBuf, uint outLen, out uint returned, IntPtr overlapped);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetFileInformationByHandle(SafeFileHandle h, out BY_HANDLE_FILE_INFORMATION info);

    private const uint GENERIC_READ = 0x80000000;
    private const uint GENERIC_WRITE = 0x40000000;
    private const uint FILE_SHARE_ALL = 0x7;
    private const uint OPEN_EXISTING = 3;
    private const uint FILE_FLAG_BACKUP_SEMANTICS = 0x02000000;
    private const uint FSCTL_LOCK_VOLUME = 0x00090018;
    private const uint FSCTL_DISMOUNT_VOLUME = 0x00090020;
    private const uint FSCTL_GET_RETRIEVAL_POINTERS = 0x00090073;
    private const uint FSCTL_GET_NTFS_VOLUME_DATA = 0x00090064;

    [StructLayout(LayoutKind.Sequential)]
    private struct BY_HANDLE_FILE_INFORMATION
    {
        public uint FileAttributes;
        public System.Runtime.InteropServices.ComTypes.FILETIME CreationTime;
        public System.Runtime.InteropServices.ComTypes.FILETIME LastAccessTime;
        public System.Runtime.InteropServices.ComTypes.FILETIME LastWriteTime;
        public uint VolumeSerialNumber;
        public uint FileSizeHigh;
        public uint FileSizeLow;
        public uint NumberOfLinks;
        public uint FileIndexHigh;
        public uint FileIndexLow;
    }

    public static SafeFileHandle OpenVolumeRaw(string letter)
    {
        string name = @"\\.\" + letter.TrimEnd(':') + ":";
        SafeFileHandle h = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_ALL, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("OpenVolumeRaw " + name + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }

    public static SafeFileHandle OpenFileForInfo(string path)
    {
        SafeFileHandle h = CreateFileW(path, GENERIC_READ, FILE_SHARE_ALL, IntPtr.Zero, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("OpenFileForInfo " + path + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }

    public static void LockVolume(SafeFileHandle h)
    {
        uint ret;
        if (!DeviceIoControl(h, FSCTL_LOCK_VOLUME, null, 0, null, 0, out ret, IntPtr.Zero))
            throw new IOException("FSCTL_LOCK_VOLUME failed GLE=" + Marshal.GetLastWin32Error());
    }

    public static void DismountVolume(SafeFileHandle h)
    {
        uint ret;
        if (!DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, null, 0, null, 0, out ret, IntPtr.Zero))
            throw new IOException("FSCTL_DISMOUNT_VOLUME failed GLE=" + Marshal.GetLastWin32Error());
    }

    // extents: array of [lcn, lengthInClusters] pairs (volume-relative, clusters)
    public static long[][] GetExtents(string path)
    {
        using (SafeFileHandle h = OpenFileForInfo(path))
        {
            byte[] inBuf = new byte[8];
            byte[] outBuf = new byte[16 + 16 * 4096];
            uint ret;
            if (!DeviceIoControl(h, FSCTL_GET_RETRIEVAL_POINTERS, inBuf, 8, outBuf, (uint)outBuf.Length, out ret, IntPtr.Zero))
                throw new IOException("GET_RETRIEVAL_POINTERS " + path + " failed GLE=" + Marshal.GetLastWin32Error());
            int count = BitConverter.ToInt32(outBuf, 0);
            long prevVcn = BitConverter.ToInt64(outBuf, 4);
            List<long[]> list = new List<long[]>();
            for (int i = 0; i < count; i++)
            {
                long nextVcn = BitConverter.ToInt64(outBuf, 12 + 16 * i);
                long lcn = BitConverter.ToInt64(outBuf, 12 + 16 * i + 8);
                if (lcn >= 0)
                    list.Add(new long[] { lcn, nextVcn - prevVcn });
                prevVcn = nextVcn;
            }
            return list.ToArray();
        }
    }

    public static long GetFileIndex(string path)
    {
        using (SafeFileHandle h = OpenFileForInfo(path))
        {
            BY_HANDLE_FILE_INFORMATION info;
            if (!GetFileInformationByHandle(h, out info)) throw new IOException("GetFileInformationByHandle " + path + " failed");
            return ((long)info.FileIndexHigh << 32) | (long)info.FileIndexLow;
        }
    }

    public static uint GetNumberOfLinks(string path)
    {
        using (SafeFileHandle h = OpenFileForInfo(path))
        {
            BY_HANDLE_FILE_INFORMATION info;
            if (!GetFileInformationByHandle(h, out info)) throw new IOException("GetFileInformationByHandle " + path + " failed");
            return info.NumberOfLinks;
        }
    }

    // NTFS_VOLUME_DATA_BUFFER: {bytesPerCluster, clustersPerFRS(signed), mftStartLcn, totalClusters}
    public static long[] GetNtfsVolumeData(SafeFileHandle h)
    {
        byte[] b = new byte[256];
        uint ret;
        if (!DeviceIoControl(h, FSCTL_GET_NTFS_VOLUME_DATA, null, 0, b, (uint)b.Length, out ret, IntPtr.Zero))
            throw new IOException("GET_NTFS_VOLUME_DATA failed GLE=" + Marshal.GetLastWin32Error());
        long bytesPerCluster = BitConverter.ToInt64(b, 40);
        long clustersPerFRS = BitConverter.ToInt64(b, 48);
        long mftStartLcn = BitConverter.ToInt64(b, 56);
        long totalClusters = BitConverter.ToInt64(b, 16);
        return new long[] { bytesPerCluster, clustersPerFRS, mftStartLcn, totalClusters };
    }

    public static int MftRecordSize(long clustersPerFRS, long bytesPerCluster)
    {
        if (clustersPerFRS < 0) return 1 << (int)(-clustersPerFRS);
        return (int)(clustersPerFRS * bytesPerCluster);
    }

    private static FileStream R(SafeFileHandle h) { return new FileStream(h, FileAccess.Read, 4096, false); }
    private static FileStream W(SafeFileHandle h) { return new FileStream(h, FileAccess.Write, 4096, false); }

    public static byte[] ReadRange(SafeFileHandle h, long offset, int len)
    {
        using (FileStream fs = R(h))
        {
            fs.Seek(offset, SeekOrigin.Begin);
            byte[] buf = new byte[len];
            int got = 0;
            while (got < len)
            {
                int n = fs.Read(buf, got, len - got);
                if (n <= 0) break;
                got += n;
            }
            if (got != len) throw new IOException("short read " + got + "/" + len + " @" + offset);
            return buf;
        }
    }

    public static void WriteRange(SafeFileHandle h, long offset, byte[] data)
    {
        using (FileStream fs = W(h))
        {
            fs.Seek(offset, SeekOrigin.Begin);
            fs.Write(data, 0, data.Length);
            fs.Flush();
        }
    }

    // apply USA fixups in place so attribute parsing sees true bytes
    private static void ApplyFixup(byte[] rec)
    {
        int usaOff = BitConverter.ToUInt16(rec, 0x04);
        int usaCount = BitConverter.ToUInt16(rec, 0x06);
        int sectorSize = 512;
        int sectors = (usaCount > 0) ? usaCount - 1 : 0;
        for (int i = 0; i < sectors; i++)
        {
            int sectorEnd = (i + 1) * sectorSize;
            if (sectorEnd - 2 < rec.Length && usaOff + 2 + 2 * (i + 1) <= rec.Length)
            {
                rec[sectorEnd - 2] = rec[usaOff + 2 + 2 * (i + 1)];
                rec[sectorEnd - 1] = rec[usaOff + 3 + 2 * (i + 1)];
            }
        }
    }

    public static byte[] ReadMftRecord(SafeFileHandle h, long mftStartOffset, int recordSize, long recordNumber)
    {
        byte[] rec = ReadRange(h, mftStartOffset + recordNumber * recordSize, recordSize);
        ApplyFixup(rec);
        return rec;
    }

    // $DATA runlist of a fixed-up MFT record: [volumeByteOffset, lengthBytes][]
    public static long[][] GetDataRuns(byte[] rec, int recordSize, long bytesPerCluster)
    {
        List<long[]> runs = new List<long[]>();
        int off = BitConverter.ToUInt16(rec, 0x14);
        while (off < recordSize - 8)
        {
            uint type = BitConverter.ToUInt32(rec, off);
            if (type == 0xFFFFFFFF) break;
            uint len = BitConverter.ToUInt32(rec, off + 4);
            if (len == 0 || off + (int)len > recordSize) break;
            if (type == 0x80)
            {
                byte nonRes = rec[off + 8];
                if (nonRes != 0)
                {
                    int runOff = off + BitConverter.ToUInt16(rec, off + 0x20);
                    int p = runOff;
                    long prevLcn = 0;
                    while (p < off + (int)len && rec[p] != 0)
                    {
                        int szLen = rec[p] & 0x0F;
                        int szOff = (rec[p] >> 4) & 0x0F;
                        p++;
                        long rl = 0;
                        for (int i = 0; i < szLen; i++) rl |= ((long)rec[p + i]) << (8 * i);
                        p += szLen;
                        long dl = 0;
                        bool neg = (szOff > 0 && (rec[p + szOff - 1] & 0x80) != 0);
                        for (int i = 0; i < szOff; i++) dl |= ((long)rec[p + i]) << (8 * i);
                        if (neg && szOff < 8) dl -= (1L << (8 * szOff));
                        p += szOff;
                        prevLcn += dl;
                        runs.Add(new long[] { prevLcn * bytesPerCluster, rl * bytesPerCluster });
                    }
                }
                break;
            }
            off += (int)len;
        }
        return runs.ToArray();
    }

    // zero an MFT record (targeted corruption); returns previous 16 bytes as hex for the record
    public static string ZeroMftRecord(SafeFileHandle h, long mftStartOffset, int recordSize, long recordNumber)
    {
        byte[] rec = ReadRange(h, mftStartOffset + recordNumber * recordSize, recordSize);
        string prev = BitConverter.ToString(rec, 0, 16).Replace("-", "");
        byte[] zero = new byte[recordSize];
        WriteRange(h, mftStartOffset + recordNumber * recordSize, zero);
        return prev;
    }

    // XOR a byte range with a pattern (bitmap corruption); returns prev-hash
    public static string XorRange(SafeFileHandle h, long offset, int len)
    {
        byte[] b = ReadRange(h, offset, len);
        string prev;
        using (SHA256 sha = SHA256.Create()) { prev = Convert.ToHexString(sha.ComputeHash(b)); }
        for (int i = 0; i < len; i++) b[i] ^= 0xFF;
        WriteRange(h, offset, b);
        return prev;
    }

    // fill a byte range with a pattern byte
    public static void FillRange(SafeFileHandle h, long offset, int len, byte pattern)
    {
        byte[] b = new byte[len];
        for (int i = 0; i < len; i++) b[i] = pattern;
        WriteRange(h, offset, b);
    }

    // volume hash map: lines "offset,sha256" per block
    public static List<string> HashVolumeBlocks(SafeFileHandle h, long volumeBytes, long blockBytes)
    {
        List<string> lines = new List<string>();
        using (SHA256 sha = SHA256.Create())
        {
            for (long off = 0; off < volumeBytes; off += blockBytes)
            {
                int n = (int)Math.Min(blockBytes, volumeBytes - off);
                byte[] buf;
                using (FileStream fs = R(h))
                {
                    fs.Seek(off, SeekOrigin.Begin);
                    buf = new byte[n];
                    int got = 0;
                    while (got < n)
                    {
                        int r = fs.Read(buf, got, n - got);
                        if (r <= 0) break;
                        got += r;
                    }
                    if (got < n) { byte[] t = new byte[got]; Array.Copy(buf, t, got); buf = t; }
                }
                string hh = Convert.ToHexString(sha.ComputeHash(buf));
                lines.Add(off + "," + hh);
            }
        }
        return lines;
    }

    public static SafeFileHandle OpenDiskRaw(int diskNumber)
    {
        string name = @"\\.\PhysicalDrive" + diskNumber;
        SafeFileHandle h = CreateFileW(name, GENERIC_READ, 0x3, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("OpenDiskRaw " + name + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }

    public static long VolumeBytesFromQuery(string letter)
    {
        // not used by callers; volume size comes from Get-Volume (PS side)
        return 0;
    }
}
"@
}

# ---- volume lifecycle --------------------------------------------------------

function New-BenchVolume {
    param(
        [Parameter(Mandatory = $true)] [string] $VhdPath,
        [int] $SizeMB = 3072,
        [ValidateSet('NTFS', 'exFAT', 'FAT32', 'ReFS')] [string] $FileSystem = 'NTFS',
        [string] $Label = 'BENCH',
        [switch] $SecondVolume,
        [string] $OutJson
    )
    $method = 'New-VHD'
    $ok = $false
    try {
        New-VHD -Path $VhdPath -SizeBytes ([long]$SizeMB * 1MB) -Dynamic -ErrorAction Stop | Out-Null
        $ok = $true
    } catch {
        Write-Host "New-VHD unavailable ($($_.Exception.Message)) - diskpart fallback"
        $method = 'diskpart'
    }
    if (-not $ok) {
        $dp = "create vdisk file=`"$VhdPath`" type=expandable maximum=$SizeMB`r`nattach vdisk"
        $tmp = [System.IO.Path]::GetTempFileName()
        $dp | Set-Content -Path $tmp -Encoding ascii
        diskpart /s $tmp
        if ($LASTEXITCODE -ne 0) { throw "diskpart create vdisk failed ($LASTEXITCODE)" }
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
    } else {
        try {
            Mount-VHD -Path $VhdPath -ErrorAction Stop
        } catch {
            Write-Host "Mount-VHD failed ($($_.Exception.Message)) - diskpart attach fallback"
            $method = "$method+diskpart-attach"
            $dp = "select vdisk file=`"$VhdPath`"`r`nattach vdisk"
            $tmp = [System.IO.Path]::GetTempFileName()
            $dp | Set-Content -Path $tmp -Encoding ascii
            diskpart /s $tmp
            if ($LASTEXITCODE -ne 0) { throw "diskpart attach failed ($LASTEXITCODE)" }
            Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        }
    }
    Start-Sleep -Seconds 4

    $disk = (Get-Disk | Sort-Object Number | Select-Object -Last 1).Number
    $serial = (Get-Disk -Number $disk).SerialNumber
    Initialize-Disk -Number $disk -PartitionStyle GPT
    $letters = @()
    $parts = @()
    if ($SecondVolume) {
        # two ~equal partitions: A = corpus target, B = stays living (delete_partition scenario)
        $free = [long](Get-Disk -Number $disk).LargestFreeExtent
        $half = [long]($free / 2)
        $p1 = New-Partition -DiskNumber $disk -Size $half -AssignDriveLetter
        $p2 = New-Partition -DiskNumber $disk -UseMaximumSize -AssignDriveLetter
        $v2 = Format-Volume -Partition $p2 -FileSystem $FileSystem -NewFileSystemLabel ($Label + '2') -Force
        $letters += @("$($v2.DriveLetter):")
        $parts += @($p2.PartitionNumber)
    } else {
        $p1 = New-Partition -DiskNumber $disk -UseMaximumSize -AssignDriveLetter
    }
    $v1 = Format-Volume -Partition $p1 -FileSystem $FileSystem -NewFileSystemLabel $Label -Force
    $vbytes = [long](Get-Volume -DriveLetter $v1.DriveLetter).Size
    $info = [ordered]@{
        vhd = $VhdPath; disk = $disk; serial = $serial; fs = $FileSystem
        letter = "$($v1.DriveLetter):"; volume_bytes = $vbytes
        partition = $p1.PartitionNumber; letters = @("$($v1.DriveLetter):") + $letters
        partitions = @($p1.PartitionNumber) + $parts
        create_method = $method; size_mb = $SizeMB
    }
    $json = $info | ConvertTo-Json -Depth 4
    if ($OutJson) { $json | Set-Content -Path $OutJson -Encoding utf8 }
    Write-Host $json
    return $json
}

function Remove-BenchVolume {
    param([Parameter(Mandatory = $true)] [string] $VhdPath)
    foreach ($l in @('Dismount-VHD')) {
        try { Dismount-VHD -Path $VhdPath -ErrorAction Stop } catch {
            $dp = "select vdisk file=`"$VhdPath`"`r`ndetach vdisk"
            $tmp = [System.IO.Path]::GetTempFileName()
            $dp | Set-Content -Path $tmp -Encoding ascii
            diskpart /s $tmp 2>$null | Out-Null
            Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        }
    }
    Remove-Item -LiteralPath $VhdPath -Force -ErrorAction SilentlyContinue
}

# ---- raw operations (lock + dismount wrappers) --------------------------------

function Invoke-VolumeHashCapture {
    param([string] $Letter, [string] $OutFile, [long] $VolumeBytes, [int] $BlockMB = 1)
    $h = [BenchRaw]::OpenVolumeRaw($Letter)
    try {
        [BenchRaw]::LockVolume($h)
        [BenchRaw]::DismountVolume($h)
        $lines = [BenchRaw]::HashVolumeBlocks($h, $VolumeBytes, [long]$BlockMB * 1MB)
        $lines | Set-Content -Path $OutFile -Encoding utf8
    } finally { $h.Close() }
    Write-Host "hash map: $((Get-Item $OutFile).Length) bytes -> $OutFile"
}

function Invoke-PartialOverwrite {
    param([string] $Letter, [array] $Extents, [int] $Sectors = 8, [byte] $Pattern = 0xAA, [long] $BytesPerCluster)
    # Extents: @{ lcn = X; clusters = Y } entries from the manifest
    $h = [BenchRaw]::OpenVolumeRaw($Letter)
    $written = @()
    try {
        [BenchRaw]::LockVolume($h)
        [BenchRaw]::DismountVolume($h)
        foreach ($e in $Extents) {
            $off = [long]$e.lcn * $BytesPerCluster
            $len = [int]($Sectors * 512)
            if ($e.clusters * $BytesPerCluster -lt $len) { $len = [int]($e.clusters * $BytesPerCluster) }
            if ($len -gt 0) {
                [BenchRaw]::FillRange($h, $off, $len, $Pattern)
                $written += @{ offset = $off; len = $len }
            }
        }
    } finally { $h.Close() }
    return ,$written
}

function Invoke-MftCorrupt {
    param([string] $Letter, [array] $Records, [int] $AdjacentRecords = 0, [switch] $AlsoBitmap)
    $h = [BenchRaw]::OpenVolumeRaw($Letter)
    $result = [ordered]@{ records_zeroed = @(); bitmap = $null; bytes_per_cluster = 0; mft_record_size = 0; mft_start_offset = 0 }
    try {
        [BenchRaw]::LockVolume($h)
        [BenchRaw]::DismountVolume($h)
        $ntfs = [BenchRaw]::GetNtfsVolumeData($h)
        $bpc = $ntfs[0]; $cpfrs = $ntfs[1]; $mftLcn = $ntfs[2]
        $recSize = [BenchRaw]::MftRecordSize($cpfrs, $bpc)
        $mftOff = $mftLcn * $bpc
        $result.bytes_per_cluster = $bpc
        $result.mft_record_size = $recSize
        $result.mft_start_offset = $mftOff
        $all = New-Object System.Collections.Generic.List[long]
        foreach ($r in $Records) {
            $all.Add([long]$r)
            for ($i = 1; $i -le $AdjacentRecords; $i++) { $all.Add([long]$r + $i) }
        }
        $seen = @{}
        foreach ($r in $all) {
            if ($r -lt 24) { continue }   # never touch system records 0..23 ($Mft..$Secure)
            if ($seen.ContainsKey($r)) { continue }
            $seen[$r] = $true
            $prev = [BenchRaw]::ZeroMftRecord($h, $mftOff, $recSize, $r)
            $result.records_zeroed += @{ record = $r; prev_head = $prev }
        }
        if ($AlsoBitmap) {
            $rec6 = [BenchRaw]::ReadMftRecord($h, $mftOff, $recSize, 6)
            $runs = [BenchRaw]::GetDataRuns($rec6, $recSize, $bpc)
            if ($runs.Length -gt 0) {
                $off = $runs[0][0]
                $len = 4096
                if ($runs[0][1] -lt $len) { $len = [int]$runs[0][1] }
                $prevHash = [BenchRaw]::XorRange($h, $off, $len)
                $result.bitmap = @{ offset = $off; len = $len; prev_sha256 = $prevHash }
            } else {
                $result.bitmap = @{ error = 'no $Bitmap data runs parsed' }
            }
        }
    } finally { $h.Close() }
    return ($result | ConvertTo-Json -Depth 5)
}

function Get-BenchNtfsGeometry {
    param([string] $Letter)
    $h = [BenchRaw]::OpenVolumeRaw($Letter)
    try {
        [BenchRaw]::LockVolume($h)
        [BenchRaw]::DismountVolume($h)
        $ntfs = [BenchRaw]::GetNtfsVolumeData($h)
        return @{ bytes_per_cluster = $ntfs[0]; clusters_per_frs = $ntfs[1]; mft_lcn = $ntfs[2]; total_clusters = $ntfs[3] }
    } finally { $h.Close() }
}


function Invoke-DiskHashCapture {
    param([int] $DiskNumber, [string] $OutFile, [int] $BlockMB = 1)
    $size = [long](Get-Disk -Number $DiskNumber).Size
    $h = [BenchRaw]::OpenDiskRaw($DiskNumber)
    try {
        $lines = [BenchRaw]::HashVolumeBlocks($h, $size, [long]$BlockMB * 1MB)
        $lines | Set-Content -Path $OutFile -Encoding utf8
    } finally { $h.Close() }
    Write-Host "disk hash map: $((Get-Item $OutFile).Length) bytes -> $OutFile"
}

function Get-BootClusterSize {
    # NTFS/FAT(12/16/32): bytes/sector u16 @0x0B, sectors/cluster u8 @0x0D
    # exFAT: BytesPerSectorShift u8 @0x78, SectorsPerClusterShift u8 @0x79
    param([string] $Letter)
    $h = [BenchRaw]::OpenVolumeRaw($Letter)
    try {
        [BenchRaw]::LockVolume($h)
        [BenchRaw]::DismountVolume($h)
        $b = [BenchRaw]::ReadRange($h, 0, 512)
    } finally { $h.Close() }
    $oem = [System.Text.Encoding]::ASCII.GetString($b, 3, 8).Trim()
    if ($oem -eq 'EXFAT') {
        $s1 = [int]$b[0x78]; $s2 = [int]$b[0x79]
        return [int][math]::Pow(2, $s1) * [int][math]::Pow(2, $s2)
    }
    $bps = [int][BitConverter]::ToUInt16($b, 0x0B)
    $spc = [int]$b[0x0D]
    return $bps * $spc
}
