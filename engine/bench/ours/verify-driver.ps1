# verify-driver.ps1 - prove the RDA driver is loaded and reachable on the namespace
# rrecov actually uses (\\.\RDA for service name 'rda'):
#   1. sc query state
#   2. CreateFileW device open on \\.\RDA (GLE recorded on failure)
#   3. IOCTL_RDA_GET_INFO  (0x60002440) -> version/flags/maxDisks/ioctlBase
#   4. IOCTL_RDA_GET_VERSION (0x60002448) -> (Major<<32)|Minor
#
# Note: the release's rdacli.exe hardcodes the drop-in namespace \\.\EPRDTDRV
# (reference-parity tool); when installed as service 'rda' the device is \\.\RDA,
# so this script performs the equivalent proof against the correct namespace.
#
# Output: driver-info.json + stdout summary. Non-zero exit on any failure.
#
# Licensed under the MIT license. See LICENSE.

param(
    [string] $ServiceName = 'rda',
    [string] $Device = 'RDA',
    [string] $OutJson = 'driver-info.json'
)

$ErrorActionPreference = 'Stop'

if (-not ('RdaVerify' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
public static class RdaVerify {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern SafeFileHandle CreateFileW(string name, uint access, uint share, IntPtr sec, uint disp, uint flags, IntPtr tmpl);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool DeviceIoControl(SafeFileHandle h, uint ctl, byte[] inBuf, uint inLen, byte[] outBuf, uint outLen, out uint ret, IntPtr ovl);
    const uint GENERIC_READ = 0x80000000;
    const uint OPEN_EXISTING = 3;
    public static SafeFileHandle Open(string device) {
        string p = @"\\.\" + device;
        SafeFileHandle h = CreateFileW(p, GENERIC_READ, 7, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (h.IsInvalid) throw new IOException("device open " + p + " failed GLE=" + Marshal.GetLastWin32Error());
        return h;
    }
    public static uint[] Info(SafeFileHandle h) {
        byte[] b = new byte[24];
        uint ret;
        if (!DeviceIoControl(h, 0x60002440, null, 0, b, 24, out ret, IntPtr.Zero) || ret < 24)
            throw new IOException("IOCTL_RDA_GET_INFO failed GLE=" + Marshal.GetLastWin32Error() + " ret=" + ret);
        return new uint[] {
            BitConverter.ToUInt32(b, 0), BitConverter.ToUInt32(b, 4), BitConverter.ToUInt32(b, 8),
            BitConverter.ToUInt32(b, 12), BitConverter.ToUInt32(b, 16), BitConverter.ToUInt32(b, 20) };
    }
    public static ulong Version(SafeFileHandle h) {
        byte[] b = new byte[8];
        uint ret;
        if (!DeviceIoControl(h, 0x60002448, null, 0, b, 8, out ret, IntPtr.Zero) || ret < 8)
            throw new IOException("IOCTL_RDA_GET_VERSION failed GLE=" + Marshal.GetLastWin32Error() + " ret=" + ret);
        return BitConverter.ToUInt64(b, 0);
    }
}
"@
}

$info = [ordered]@{
    service = $ServiceName
    device = "\\.\$Device"
    checked_utc = (Get-Date).ToUniversalTime().ToString('o')
}

$svc = (sc.exe query $ServiceName) -join ' '
$info.service_query = $svc
if ($svc -notmatch 'RUNNING') {
    $info | ConvertTo-Json -Depth 4 | Set-Content -Path $OutJson -Encoding utf8
    Write-Host $svc
    throw "service $ServiceName is not RUNNING"
}

$h = [RdaVerify]::Open($Device)
try {
    $di = [RdaVerify]::Info($h)
    $info.driver_version = "{0}.{1}" -f $di[1], $di[2]
    $info.flags = "0x{0:X}" -f $di[3]
    $info.max_disks = $di[4]
    $info.ioctl_base = "0x{0:X}" -f $di[5]
    $info.struct_size = $di[0]
    $ver = [RdaVerify]::Version($h)
    $info.version_ioctl = "0x{0:X16}" -f $ver
} finally { $h.Close() }

$info | ConvertTo-Json -Depth 4 | Set-Content -Path $OutJson -Encoding utf8
Get-Content $OutJson | Write-Host
Write-Host "DRIVER VERIFIED: service=$ServiceName device=\\.\$Device version=$($info.driver_version) (IOCTL GET_INFO + GET_VERSION)"
