# io-probe.ps1 - sample the app process's IO counters, handle count, and any
# CHILD processes it spawns (the fsutil _popen flash evidence), for the app's
# whole observed lifetime. Writes JSONL.
#
# Usage: io-probe.ps1 -AppPid <pid> -OutDir <dir> [-DurationSec 2400] [-IntervalMs 2000]
# Output: io-timeline.jsonl

param(
    [Parameter(Mandatory = $true)][int]$AppPid,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$DurationSec = 2400,
    [int]$IntervalMs = 2000
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$ioLog = Join-Path $OutDir 'io-timeline.jsonl'
Remove-Item $ioLog -ErrorAction SilentlyContinue

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class IoCounters {
    [StructLayout(LayoutKind.Sequential)]
    public struct IO_COUNTERS {
        public ulong ReadOperationCount, WriteOperationCount, OtherOperationCount;
        public ulong ReadTransferCount, WriteTransferCount, OtherTransferCount;
    }
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool GetProcessIoCounters(IntPtr h, out IO_COUNTERS c);
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
    public static object Sample(int pid) {
        IntPtr h = OpenProcess(0x0010 /*PROCESS_VM_READ*/ | 0x0400 /*PROCESS_QUERY_INFORMATION*/, false, pid);
        if (h == IntPtr.Zero) return null;
        try {
            IO_COUNTERS c;
            if (!GetProcessIoCounters(h, out c)) return null;
            return new {
                readOps = c.ReadOperationCount, writeOps = c.WriteOperationCount,
                otherOps = c.OtherOperationCount,
                readBytes = c.ReadTransferCount, writeBytes = c.WriteTransferCount,
                otherBytes = c.OtherTransferCount
            };
        } finally { CloseHandle(h); }
    }
}
"@

$deadline = (Get-Date).AddSeconds($DurationSec)
Write-Host "[io-probe] sampling pid $AppPid every ${IntervalMs}ms for ${DurationSec}s"

while ((Get-Date) -lt $deadline) {
    $p = Get-Process -Id $AppPid -ErrorAction SilentlyContinue
    if (-not $p) { Write-Host "[io-probe] app pid $AppPid gone; stopping"; break }
    $io = [IoCounters]::Sample($AppPid)
    # child processes of the app (fsutil/powershell/cmd flashes)
    $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $AppPid" -ErrorAction SilentlyContinue |
        ForEach-Object { [ordered]@{ pid = $_.ProcessId; name = $_.Name; cmd = $_.CommandLine } })
    $row = [ordered]@{
        t           = (Get-Date).ToString('o')
        handleCount = $p.HandleCount
        wsMB        = [math]::Round($p.WorkingSet64 / 1MB, 1)
        io          = $io
        children    = $children
    }
    ($row | ConvertTo-Json -Compress -Depth 4) | Add-Content -Path $ioLog -Encoding UTF8
    Start-Sleep -Milliseconds $IntervalMs
}
Write-Host "[io-probe] done"
