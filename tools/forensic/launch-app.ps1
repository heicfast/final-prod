# launch-app.ps1 - launch the INSTALLED product with WebView2 CDP enabled so
# the real, shipped UI can be driven and observed (no product code changes).
# Captures the app's stderr/stdout (the only logging the shell has) to files.
#
# Usage: launch-app.ps1 -AppExe <path> -OutDir <dir> [-CdpPort 9222]
# Outputs: app-pid.txt, app-stderr.log, app-stdout.log, cdp-ready.json

param(
    [string]$AppExe = 'C:\Program Files\HeicFast\Recovery Wizard\recovery-wizard-ui.exe',
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$CdpPort = 9222
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if (-not (Test-Path $AppExe)) { throw "app exe not found: $AppExe" }

$env:WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS = "--remote-debugging-port=$CdpPort"

$err = Join-Path $OutDir 'app-stderr.log'
$out = Join-Path $OutDir 'app-stdout.log'
$p = Start-Process -FilePath $AppExe -WorkingDirectory (Split-Path $AppExe) `
    -RedirectStandardError $err -RedirectStandardOutput $out -PassThru
$p.Id | Set-Content (Join-Path $OutDir 'app-pid.txt') -Encoding ASCII
Write-Host "[launch-app] pid=$($p.Id) cdp port=$CdpPort"

# Wait for the CDP endpoint (WebView2 browser process up + debugger socket).
$deadline = (Get-Date).AddSeconds(60)
$ready = $false
$ver = $null
while ((Get-Date) -lt $deadline) {
    if ($p.HasExited) { throw "app exited early (code $($p.ExitCode)); stderr so far: $(Get-Content $err -Raw -ErrorAction SilentlyContinue)" }
    try {
        $ver = Invoke-RestMethod -Uri "http://127.0.0.1:$CdpPort/json/version" -TimeoutSec 2
        if ($ver.Browser) { $ready = $true; break }
    } catch { Start-Sleep -Milliseconds 500 }
}
if (-not $ready) { throw "CDP endpoint did not come up on port $CdpPort within 60s" }

[ordered]@{
    pid      = $p.Id
    cdpPort  = $CdpPort
    browser  = $ver.Browser
    userAgent = $ver.'User-Agent'
    readyUtc = (Get-Date).ToUniversalTime().ToString('o')
} | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $OutDir 'cdp-ready.json') -Encoding UTF8
Write-Host "[launch-app] CDP ready: $($ver.Browser)"

# The launched process's ACTUAL token elevation (OpenProcessToken +
# GetTokenInformation(TokenElevation) via P/Invoke - the same class of
# probe the engine's own posture op runs). Recorded as a fact; the
# elevated runner means this is expected true for the main instance, and
# false for the unelevated-launch.ps1 twin.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class TokenElevationProbe {
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern IntPtr GetCurrentProcess();
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern bool OpenProcessToken(IntPtr h, uint access, out IntPtr tok);
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern bool GetTokenInformation(IntPtr tok, int cls, out int info, int len, out int retLen);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(IntPtr h);
    public static string IsElevated(int pid) {
        IntPtr proc = pid < 0 ? GetCurrentProcess() : OpenProcess(0x0400, false, pid); // PROCESS_QUERY_INFORMATION
        if (proc == IntPtr.Zero) return "ERROR: OpenProcess " + Marshal.GetLastWin32Error();
        try {
            IntPtr tok;
            if (!OpenProcessToken(proc, 0x0008, out tok)) return "ERROR: OpenProcessToken " + Marshal.GetLastWin32Error(); // TOKEN_QUERY
            try {
                int elev, retLen;
                if (!GetTokenInformation(tok, 20, out elev, 4, out retLen)) return "ERROR: GetTokenInformation " + Marshal.GetLastWin32Error(); // TokenElevation
                return elev != 0 ? "elevated" : "not-elevated";
            } finally { CloseHandle(tok); }
        } finally { CloseHandle(proc); }
    }
}
'@
$tokenElevation = [TokenElevationProbe]::IsElevated($p.Id)
Write-Host "[launch-app] app token elevation: $tokenElevation"
[ordered]@{ pid = $p.Id; tokenElevation = $tokenElevation; probedUtc = (Get-Date).ToUniversalTime().ToString('o') } |
    ConvertTo-Json | Set-Content (Join-Path $OutDir 'app-token-elevation.json') -Encoding UTF8
