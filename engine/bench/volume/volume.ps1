# volume.ps1 - VHDX volume management + raw-volume access for the bench.
#
# Dot-source:  . $PSScriptRoot\volume.ps1
#
# Design note ( FileStream-over-SafeFileHandle owns the handle on Dispose ):
# every multi-step raw operation is a SINGLE C# composite call that opens,
# locks, dismounts, does all IO on one FileStream, then closes. PS wrappers
# never share one handle across several C# calls.
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

    // ---- handle helpers ----------------------------------------------------

    private static SafeFileHandle OpenVolumeRaw(string letter)
    {
        string name = @"\\.\" + letter.TrimEnd(':') + ":";
        SafeFileHandle h = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_ALL, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("OpenVolumeRaw " + name + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }

    private static SafeFileHandle OpenDiskRaw(int diskNumber)
    {
        string name = @"\\.\" + "PhysicalDrive" + diskNumber;
        SafeFileHandle h = CreateFileW(name, GENERIC_READ, 0x3, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("OpenDiskRaw " + name + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }

    private static SafeFileHandle OpenFileForInfo(string path)
    {
        SafeFileHandle h = CreateFileW(path, GENERIC_READ, FILE_SHARE_ALL, IntPtr.Zero, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("OpenFileForInfo " + path + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }

    private static void LockAndDismount(SafeFileHandle h)
    {
        uint ret;
        if (!DeviceIoControl(h, FSCTL_LOCK_VOLUME, null, 0, null, 0, out ret, IntPtr.Zero))
            throw new IOException("FSCTL_LOCK_VOLUME failed GLE=" + Marshal.GetLastWin32Error());
        if (!DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, null, 0, null, 0, out ret, IntPtr.Zero))
            throw new IOException("FSCTL_DISMOUNT_VOLUME failed GLE=" + Marshal.GetLastWin32Error());
    }

    // ---- per-file info (single DeviceIoControl per handle: no FileStream) --

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
            /* RETRIEVAL_POINTERS_BUFFER true layout (BENCH-F3): ExtentCount
             * u32 @0, 4 bytes padding, StartingVcn i64 @8, then per extent
             * @16+16i: NextVcn i64, Lcn i64. The first draft read
             * StartingVcn@4 / Extents@12 - a 4-byte shift that composed
             * garbage: every 1-cluster extent decoded with length exactly
             * 2^32 (evidence: run 40073093997 manifests, t000.jpg
             * [[9092445765632, 4294967296]] for a 2849-byte file). All
             * manifest extents were wrong -> partial_overwrite and
             * mft_corrupt targeted the wrong disk locations. */
            long prevVcn = BitConverter.ToInt64(outBuf, 8);
            List<long[]> list = new List<long[]>();
            for (int i = 0; i < count; i++)
            {
                long nextVcn = BitConverter.ToInt64(outBuf, 16 + 16 * i);
                long lcn = BitConverter.ToInt64(outBuf, 16 + 16 * i + 8);
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

    // ---- MFT record helpers (called under one stream) ----------------------

    private static void ApplyFixup(byte[] rec)
    {
        int usaOff = BitConverter.ToUInt16(rec, 0x04);
        int usaCount = BitConverter.ToUInt16(rec, 0x06);
        int sectors = (usaCount > 0) ? usaCount - 1 : 0;
        for (int i = 0; i < sectors; i++)
        {
            int sectorEnd = (i + 1) * 512;
            if (sectorEnd - 2 < rec.Length && usaOff + 2 + 2 * (i + 1) <= rec.Length)
            {
                rec[sectorEnd - 2] = rec[usaOff + 2 + 2 * (i + 1)];
                rec[sectorEnd - 1] = rec[usaOff + 3 + 2 * (i + 1)];
            }
        }
    }

    private static byte[] ReadRange(FileStream fs, long offset, int len)
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

    private static void WriteRange(FileStream fs, long offset, byte[] data)
    {
        fs.Seek(offset, SeekOrigin.Begin);
        fs.Write(data, 0, data.Length);
        fs.Flush();
    }

    private static long[] NtfsData(SafeFileHandle h)
    {
        byte[] b = new byte[256];
        uint ret;
        if (!DeviceIoControl(h, FSCTL_GET_NTFS_VOLUME_DATA, null, 0, b, (uint)b.Length, out ret, IntPtr.Zero))
            throw new IOException("GET_NTFS_VOLUME_DATA failed GLE=" + Marshal.GetLastWin32Error() + " (NTFS only?)");
        return new long[] {
            BitConverter.ToInt64(b, 40),   // bytesPerCluster
            BitConverter.ToInt64(b, 48),   // clustersPerFRS (signed)
            BitConverter.ToInt64(b, 56),   // mftStartLcn
            BitConverter.ToInt64(b, 16) }; // totalClusters
    }

    private static int MftRecordSize(long clustersPerFRS, long bytesPerCluster)
    {
        if (clustersPerFRS < 0) return 1 << (int)(-clustersPerFRS);
        return (int)(clustersPerFRS * bytesPerCluster);
    }

    private static long[][] GetDataRuns(byte[] rec, int recordSize, long bytesPerCluster)
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

    // ---- composite public operations ---------------------------------------
    // Each: open -> lock -> dismount -> all IO on ONE FileStream -> close.

    public static List<string> HashVolume(string letter, long volumeBytes, long blockBytes)
    {
        List<string> lines = new List<string>();
        using (SafeFileHandle h = OpenVolumeRaw(letter))
        {
            LockAndDismount(h);
            using (SHA256 sha = SHA256.Create())
            using (FileStream fs = new FileStream(h, FileAccess.Read, 4096, false))
            {
                for (long off = 0; off < volumeBytes; off += blockBytes)
                {
                    int n = (int)Math.Min(blockBytes, volumeBytes - off);
                    fs.Seek(off, SeekOrigin.Begin);
                    byte[] buf = new byte[n];
                    int got = 0;
                    while (got < n)
                    {
                        int r = fs.Read(buf, got, n - got);
                        if (r <= 0) break;
                        got += r;
                    }
                    if (got < n)
                    {
                        byte[] t = new byte[got];
                        Array.Copy(buf, t, got);
                        buf = t;
                    }
                    lines.Add(off + "," + Convert.ToHexString(sha.ComputeHash(buf)));
                }
            }
        }
        return lines;
    }

    public static List<string> HashDisk(int diskNumber, long diskBytes, long blockBytes)
    {
        List<string> lines = new List<string>();
        using (SafeFileHandle h = OpenDiskRaw(diskNumber))
        using (SHA256 sha = SHA256.Create())
        using (FileStream fs = new FileStream(h, FileAccess.Read, 4096, false))
        {
            for (long off = 0; off < diskBytes; off += blockBytes)
            {
                int n = (int)Math.Min(blockBytes, diskBytes - off);
                fs.Seek(off, SeekOrigin.Begin);
                byte[] buf = new byte[n];
                int got = 0;
                while (got < n)
                {
                    int r = fs.Read(buf, got, n - got);
                    if (r <= 0) break;
                    got += r;
                }
                if (got < n)
                {
                    byte[] t = new byte[got];
                    Array.Copy(buf, t, got);
                    buf = t;
                }
                lines.Add(off + "," + Convert.ToHexString(sha.ComputeHash(buf)));
            }
        }
        return lines;
    }

    // cluster size from the boot sector: NTFS/FAT u16 bps @0x0B + u8 spc @0x0D;
    // exFAT shift bytes @0x78/@0x79. Returns -1 when unrecognized.
    public static long GetBootClusterSize(string letter)
    {
        using (SafeFileHandle h = OpenVolumeRaw(letter))
        {
            LockAndDismount(h);
            using (FileStream fs = new FileStream(h, FileAccess.Read, 512, false))
            {
                byte[] b = ReadRange(fs, 0, 512);
                string oem = Encoding.ASCII.GetString(b, 3, 8).Trim();
                if (oem == "EXFAT")
                {
                    int s1 = b[0x78]; int s2 = b[0x79];
                    return (long)(Math.Pow(2, s1) * Math.Pow(2, s2));
                }
                int bps = BitConverter.ToUInt16(b, 0x0B);
                int spc = b[0x0D];
                if (bps <= 0 || spc <= 0) return -1;
                return (long)bps * (long)spc;
            }
        }
    }

    // zero the given MFT records (+adjacent) and optionally XOR 4 KiB of $Bitmap;
    // returns a JSON-ready summary string
    public static string CorruptMft(string letter, long[] records, int adjacent, bool alsoBitmap)
    {
        var zeroed = new List<string>();
        string bitmapJson = "null";
        long bpc = 0; int recSize = 0; long mftOff = 0;
        using (SafeFileHandle h = OpenVolumeRaw(letter))
        {
            LockAndDismount(h);
            long[] nd = NtfsData(h);
            bpc = nd[0];
            recSize = MftRecordSize(nd[1], nd[2]);
            mftOff = nd[2] * bpc;
            using (FileStream fs = new FileStream(h, FileAccess.Read | FileAccess.Write, 4096, false))
            {
                var seen = new HashSet<long>();
                foreach (long r0 in records)
                {
                    for (int k = 0; k <= adjacent; k++)
                    {
                        long r = r0 + k;
                        if (r < 24) continue;              // never touch system records 0..23
                        if (!seen.Add(r)) continue;
                        byte[] prev = ReadRange(fs, mftOff + r * recSize, recSize);
                        string head = BitConverter.ToString(prev, 0, 16).Replace("-", "");
                        WriteRange(fs, mftOff + r * recSize, new byte[recSize]);
                        zeroed.Add("{\"record\":" + r + ",\"prev_head\":\"" + head + "\"}");
                    }
                }
                if (alsoBitmap)
                {
                    byte[] rec6 = ReadRange(fs, mftOff + 6 * recSize, recSize);
                    ApplyFixup(rec6);
                    long[][] runs = GetDataRuns(rec6, recSize, bpc);
                    if (runs.Length > 0)
                    {
                        long off = runs[0][0];
                        int len = 4096;
                        if (runs[0][1] < len) len = (int)runs[0][1];
                        byte[] blk = ReadRange(fs, off, len);
                        string prevHash;
                        using (SHA256 sha = SHA256.Create()) { prevHash = Convert.ToHexString(sha.ComputeHash(blk)); }
                        for (int i = 0; i < len; i++) blk[i] ^= 0xFF;
                        WriteRange(fs, off, blk);
                        bitmapJson = "{\"offset\":" + off + ",\"len\":" + len + ",\"prev_sha256\":\"" + prevHash + "\"}";
                    }
                    else
                    {
                        bitmapJson = "{\"error\":\"no $Bitmap data runs parsed\"}";
                    }
                }
            }
        }
        return "{\"bytes_per_cluster\":" + bpc + ",\"mft_record_size\":" + recSize +
               ",\"mft_start_offset\":" + mftOff + ",\"records_zeroed\":[" + string.Join(",", zeroed.ToArray()) +
               "],\"bitmap\":" + bitmapJson + "}";
    }

    // overwrite the first N sectors of each extent with a pattern byte
    public static string PartialOverwrite(string letter, long[][] extents, int sectors, byte pattern)
    {
        var written = new List<string>();
        using (SafeFileHandle h = OpenVolumeRaw(letter))
        {
            LockAndDismount(h);
            using (FileStream fs = new FileStream(h, FileAccess.Read | FileAccess.Write, 4096, false))
            {
                foreach (long[] e in extents)
                {
                    if (e == null || e.Length < 2) continue;
                    long off = e[0];
                    long avail = e[1];
                    int len = sectors * 512;
                    if (avail < len) len = (int)avail;
                    if (len <= 0) continue;
                    byte[] b = new byte[len];
                    for (int i = 0; i < len; i++) b[i] = pattern;
                    WriteRange(fs, off, b);
                    written.Add("{\"offset\":" + off + ",\"len\":" + len + "}");
                }
            }
        }
        return "[" + string.Join(",", written.ToArray()) + "]";
    }

    public static long[] NtfsGeometry(string letter)
    {
        using (SafeFileHandle h = OpenVolumeRaw(letter))
        {
            LockAndDismount(h);
            return NtfsData(h);
        }
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
        # two ~equal partitions: A = corpus target, B = stays (delete_partition scenario deletes both)
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
    try { Dismount-VHD -Path $VhdPath -ErrorAction Stop } catch {
        $dp = "select vdisk file=`"$VhdPath`"`r`ndetach vdisk"
        $tmp = [System.IO.Path]::GetTempFileName()
        $dp | Set-Content -Path $tmp -Encoding ascii
        diskpart /s $tmp 2>$null | Out-Null
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath $VhdPath -Force -ErrorAction SilentlyContinue
}

# ---- raw operation wrappers (single composite C# call each) ------------------

function Invoke-VolumeHashCapture {
    param([string] $Letter, [string] $OutFile, [long] $VolumeBytes, [int] $BlockMB = 1)
    $lines = [BenchRaw]::HashVolume($Letter, $VolumeBytes, [long]$BlockMB * 1MB)
    $lines | Set-Content -Path $OutFile -Encoding utf8
    Write-Host "hash map: $((Get-Item $OutFile).Length) bytes -> $OutFile"
}

function Invoke-DiskHashCapture {
    param([int] $DiskNumber, [string] $OutFile, [int] $BlockMB = 1)
    $size = [long](Get-Disk -Number $DiskNumber).Size
    $lines = [BenchRaw]::HashDisk($DiskNumber, $size, [long]$BlockMB * 1MB)
    $lines | Set-Content -Path $OutFile -Encoding utf8
    Write-Host "disk hash map: $((Get-Item $OutFile).Length) bytes -> $OutFile"
}

function Get-BootClusterSize {
    param([string] $Letter)
    return [BenchRaw]::GetBootClusterSize($Letter)
}

function Invoke-PartialOverwrite {
    param([string] $Letter, [array] $Extents, [int] $Sectors = 8, [byte] $Pattern = 0xAA)
    # Extents: flat [volumeByteOffset, lengthBytes] pairs (lcn * bytesPerCluster precomputed)
    $pairs = New-Object 'System.Collections.Generic.List[long[]]'
    foreach ($e in $Extents) {
        $pairs.Add(([long[]]@([long]$e[0], [long]$e[1])))
    }
    $json = [BenchRaw]::PartialOverwrite($Letter, $pairs.ToArray(), $Sectors, $Pattern)
    return ($json | ConvertFrom-Json)
}

function Invoke-MftCorrupt {
    param([string] $Letter, [array] $Records, [int] $AdjacentRecords = 0, [switch] $AlsoBitmap)
    $recs = [long[]]@($Records | ForEach-Object { [long]$_ })
    $json = [BenchRaw]::CorruptMft($Letter, $recs, $AdjacentRecords, [bool]$AlsoBitmap)
    return ($json | ConvertFrom-Json)
}

function Get-BenchNtfsGeometry {
    param([string] $Letter)
    $nd = [BenchRaw]::NtfsGeometry($Letter)
    return @{ bytes_per_cluster = $nd[0]; clusters_per_frs = $nd[1]; mft_lcn = $nd[2]; total_clusters = $nd[3] }
}
