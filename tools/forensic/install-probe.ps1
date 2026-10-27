# install-probe.ps1 - run the product installer while capturing every child
# process spawn and every top-level window that appears (the PowerShell-popup
# reproduction), then record the install result.
#
# Usage: install-probe.ps1 -SetupExe <path> -OutDir <dir> [-Silent]
# Outputs:
#   install-process-events.jsonl  one line per sampled child process spawn
#   install-window-events.jsonl   one line per sampled visible top-level window
#   install-result.json           exit code, duration, silent flag
#
# Evidence tooling: failures throw; nothing is skipped silently.

param(
    [Parameter(Mandatory = $true)][string]$SetupExe,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [switch]$Silent
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Add-Type -TypeDefinition @"
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;
public static class WinEnum {
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lp);
    [DllImport("user32.dll")] static extern int GetWindowTextW(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll")] static extern int GetClassNameW(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lp);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    public static List<string> Snapshot() {
        var list = new List<string>();
        EnumWindows((h, lp) => {
            var sb = new StringBuilder(512);
            GetWindowTextW(h, sb, 512);
            string title = sb.ToString();
            var cb = new StringBuilder(256);
            GetClassNameW(h, cb, 256);
            string cls = cb.ToString();
            uint pid; GetWindowThreadProcessId(h, out pid);
            bool vis = IsWindowVisible(h);
            RECT r; GetWindowRect(h, out r);
            if (title.Length > 0 || vis)
                list.Add(pid + "|" + vis + "|" + cls + "|" + title.Replace('|','/').Replace('\n',' ').Replace('\r',' ') + "|" + (r.R - r.L) + "x" + (r.B - r.T));
            return true;
        }, IntPtr.Zero);
        return list;
    }
}
"@

function Write-Jsonl([string]$path, [object]$obj) {
    ($obj | ConvertTo-Json -Compress -Depth 4) | Add-Content -Path $path -Encoding UTF8
}

$procLog = Join-Path $OutDir 'install-process-events.jsonl'
$winLog  = Join-Path $OutDir 'install-window-events.jsonl'
Remove-Item $procLog, $winLog -ErrorAction SilentlyContinue

$watchNames = @('powershell.exe','pwsh.exe','cmd.exe','certutil.exe','fsutil.exe','conhost.exe','sc.exe','net.exe','rundll32.exe','msiexec.exe','setup*.tmp')

Write-Host "[install-probe] launching: $SetupExe $(if ($Silent) {'/S'} else {''})"

$t0 = Get-Date
$p = if ($Silent) { Start-Process -FilePath $SetupExe -ArgumentList '/S' -PassThru } else { Start-Process -FilePath $SetupExe -PassThru }
$installerPid = $p.Id

# Poll until the installer process tree exits (the NSIS outer exe may spawn
# children; wait for the outer process itself, then a settle period).
while (-not $p.HasExited) {
    $now = Get-Date
    # child processes of interest created since install start
    $procs = Get-CimInstance Win32_Process -Filter "CreationDate >= '$($t0.ToUniversalTime().ToString('yyyy-MM-dd HH:mm:ss'))'" -ErrorAction SilentlyContinue |
        Where-Object { $watchNames -contains $_.Name.ToLower() -or ($_.Name -like 'setup*.tmp') }
    foreach ($pr in $procs) {
        Write-Jsonl $procLog ([ordered]@{
            t = $now.ToString('o'); pid = $pr.ProcessId; name = $pr.Name
            ppid = $pr.ParentProcessId; cmd = $pr.CommandLine
        })
    }
    # top-level windows
    foreach ($w in [WinEnum]::Snapshot()) {
        $parts = $w -split '\|', 5
        Write-Jsonl $winLog ([ordered]@{
            t = $now.ToString('o'); pid = $parts[0]; visible = ($parts[1] -eq 'True')
            class = $parts[2]; title = $parts[3]; size = $parts[4]
        })
    }
    Start-Sleep -Milliseconds 400
}

# a final settle sample (late children, e.g. the uninstaller registration)
Start-Sleep -Seconds 3
$procs2 = Get-CimInstance Win32_Process -Filter "CreationDate >= '$($t0.ToUniversalTime().ToString('yyyy-MM-dd HH:mm:ss'))'" -ErrorAction SilentlyContinue |
    Where-Object { $watchNames -contains $_.Name.ToLower() -or ($_.Name -like 'setup*.tmp') }
foreach ($pr in $procs2) {
    Write-Jsonl $procLog ([ordered]@{
        t = (Get-Date).ToString('o'); pid = $pr.ProcessId; name = $pr.Name
        ppid = $pr.ParentProcessId; cmd = $pr.CommandLine
    })
}

$t1 = Get-Date
$result = [ordered]@{
    setupExe    = $SetupExe
    silent      = [bool]$Silent
    startedUtc  = $t0.ToUniversalTime().ToString('o')
    endedUtc    = $t1.ToUniversalTime().ToString('o')
    durationSec = [math]::Round(($t1 - $t0).TotalSeconds, 1)
    exitCode    = $p.ExitCode
    installerPid = $installerPid
}
$result | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $OutDir 'install-result.json') -Encoding UTF8
Write-Host ("[install-probe] installer exited {0} after {1:N1}s (exit code {2})" -f $result.endedUtc, $result.durationSec, $result.exitCode)

if ($p.ExitCode -ne 0) {
    Write-Warning ("installer exit code {0} - captured evidence retained" -f $p.ExitCode)
}
