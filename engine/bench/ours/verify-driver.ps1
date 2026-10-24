# verify-driver.ps1 - prove the RDA driver is loaded, reachable and versioned.
#
# Evidence layers (each recorded in driver-info.json; any hard failure exits non-zero):
#   1. sc query <service>              -> RUNNING
#   2. CreateFileW on \\\.<Device>      -> the namespace rrecov uses opens
#   3. IOCTL_RDA_GET_INFO/GET_VERSION on \\\.<Device> (recorded; GLE noted on failure)
#   4. PE FileVersion of the rda.sys binary (rda.rc FILEVERSION resource)
#   5. The release's own integration check: alias service 'eprdtdrv' + rdacli.exe
#      info (rdacli hardcodes the \\\.\EPRDTDRV reference namespace - the
#      drop-in name the release's run-integration.ps1 verifies against)
#
# Licensed under the MIT license. See LICENSE.

param(
    [string] $ServiceName = 'rda',
    [string] $Device = 'RDA',
    [string] $OutJson = 'driver-info.json',
    [string] $DriverSys = 'driver\rda.sys',
    [string] $Rdacli = 'driver\rdacli.exe'
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
    public static string Info(string device) {
        using (SafeFileHandle h = Open(device)) {
            byte[] b = new byte[24];
            uint ret;
            bool ok = DeviceIoControl(h, 0x60002440, null, 0, b, 24, out ret, IntPtr.Zero);
            int gle = Marshal.GetLastWin32Error();
            if (!ok || ret < 24) return "GET_INFO rejected (ok=" + ok + " ret=" + ret + " gle=" + gle + ")";
            uint major = BitConverter.ToUInt32(b, 4), minor = BitConverter.ToUInt32(b, 8);
            uint flags = BitConverter.ToUInt32(b, 12), maxDisks = BitConverter.ToUInt32(b, 16);
            uint base32 = BitConverter.ToUInt32(b, 20);
            byte[] v = new byte[8];
            uint ret2;
            bool ok2 = DeviceIoControl(h, 0x60002448, null, 0, v, 8, out ret2, IntPtr.Zero);
            int gle2 = Marshal.GetLastWin32Error();
            string ver = ok2 && ret2 >= 8 ? "0x" + BitConverter.ToUInt64(v, 0).ToString("X16") : ("GET_VERSION rejected (gle=" + gle2 + ")");
            return "v" + major + "." + minor + " flags=0x" + flags.ToString("X") + " maxDisks=" + maxDisks +
                   " ioctlBase=0x" + base32.ToString("X") + " version=" + ver;
        }
    }
}
"@
}

$info = [ordered]@{
    service = $ServiceName
    device = "\\.\$Device"
    checked_utc = (Get-Date).ToUniversalTime().ToString('o')
}

# 1. service state
$svc = (sc.exe query $ServiceName) -join ' '
$info.service_query_state = if ($svc -match 'RUNNING') { 'RUNNING' } else { 'NOT-RUNNING' }
if ($svc -notmatch 'RUNNING') {
    $info | ConvertTo-Json -Depth 4 | Set-Content -Path $OutJson -Encoding utf8
    throw "service $ServiceName is not RUNNING: $svc"
}

# 2 + 3. namespace open + extended IOCTLs (recorded either way)
try {
    $h = [RdaVerify]::Open($Device)
    $h.Close()
    $info.device_open = 'ok'
} catch {
    $info.device_open = "FAILED: $($_.Exception.Message)"
    $info | ConvertTo-Json -Depth 4 | Set-Content -Path $OutJson -Encoding utf8
    throw
}
try {
    $info.extended_ioctl = [RdaVerify]::Info($Device)
} catch {
    $info.extended_ioctl = "FAILED: $($_.Exception.Message)"
}

# 4. PE FileVersion from the binary
try {
    $info.pe_file_version = (Get-Item $DriverSys).VersionInfo.FileVersion
    $info.pe_product_version = (Get-Item $DriverSys).VersionInfo.ProductVersion
} catch {
    $info.pe_file_version = "unavailable: $($_.Exception.Message)"
}

# 5. the release's own integration check via the reference namespace alias
$alias = 'eprdtdrv'
$aliasResult = 'not-attempted'
try {
    Copy-Item $DriverSys "$env:SystemRoot\System32\drivers\eprdtdrv.sys" -Force
    sc.exe create $alias type= kernel start= demand binPath= "System32\drivers\eprdtdrv.sys" | Out-Null
    sc.exe start $alias | Out-Null
    Start-Sleep -Seconds 2
    $out = (& $Rdacli info) 2>$null
    $aliasResult = ($out -join ' ')
    if ($LASTEXITCODE -ne 0) { $aliasResult = "rdacli info exit=${LASTEXITCODE}: $aliasResult" }
} catch {
    $aliasResult = "alias check error: $($_.Exception.Message)"
} finally {
    sc.exe stop $alias 2>$null | Out-Null
    sc.exe delete $alias 2>$null | Out-Null
    Remove-Item "$env:SystemRoot\System32\drivers\eprdtdrv.sys" -Force -ErrorAction SilentlyContinue
}
$info.rdacli_reference_namespace = $aliasResult

$info | ConvertTo-Json -Depth 4 | Set-Content -Path $OutJson -Encoding utf8
Get-Content $OutJson | Write-Host

# verdict: service running + namespace opens + version evidence from EITHER the
# extended IOCTL or the rdacli reference-namespace check
$verdictOk = ($info.device_open -eq 'ok') -and
    (($info.extended_ioctl -notmatch 'FAILED|rejected') -or ($aliasResult -match 'PASS info'))
if (-not $verdictOk) {
    throw "driver verification FAILED - see $OutJson (device_open=$($info.device_open), extended=$($info.extended_ioctl), rdacli=$aliasResult)"
}
Write-Host "DRIVER VERIFIED: service=$ServiceName RUNNING; \\.\$Device opens; version evidence: extended=[$($info.extended_ioctl)] rdacli=[$aliasResult]"
