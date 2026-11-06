# unelevated-launch.ps1 - launch the INSTALLED product with a genuinely
# UNELEVATED token so the user-mode refusal contract can be probed: the
# product must show the elevation banner and refuse raw-disk work — never
# run degraded, never pretend to work (the product-side twin of core's
# exit-4 elevation probes, INTEGRATION-PLAN section 6.3).
#
# Mechanism (v2, after run 42129256087): a FRESH STANDARD USER whose
# interactive logon is genuinely unelevated regardless of the host's UAC
# state. The first attempt used `runas /trustlevel:0x20000`, but on the
# windows-2022 runner image UAC token filtering is not in effect, so the
# restricted token never materialized: the app ran elevated, truthfully
# reported administrator, and correctly showed NO banner (the product was
# right; the launcher was wrong). CreateProcessWithLogonW via
# Start-Process -Credential of a non-admin user is the deterministic path.
#
# The launcher ASSERTS its own precondition: it records the spawned app
# process's TokenElevation (OpenProcessToken P/Invoke) and THROWS if the
# token is still elevated - the probe's banner assertion is only honest
# when the launch was genuinely unelevated.
#
# Usage: unelevated-launch.ps1 -OutDir <dir> [-CdpPort 9223]
# Outputs: unelevated-app-pid.txt, unelevated-token.json, unelevated-user.txt
#          (the workflow's finally-block deletes the user after the probe)

param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$CdpPort = 9223
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$appExe = 'C:\Program Files\HeicFast\Recovery Wizard\recovery-wizard-ui.exe'
if (-not (Test-Path $appExe)) { throw "app exe not found: $appExe" }

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class TokenElevationProbe2 {
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern bool OpenProcessToken(IntPtr h, uint access, out IntPtr tok);
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern bool GetTokenInformation(IntPtr tok, int cls, out int info, int len, out int retLen);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(IntPtr h);
    public static string IsElevated(int pid) {
        IntPtr proc = OpenProcess(0x1000, false, pid); // PROCESS_QUERY_LIMITED_INFORMATION
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

# --- The fresh standard user -------------------------------------------------
$user = 'rwprobe'
$pass = 'Rw!' + (Get-Random -Maximum 9999999).ToString() + 'a1'
$null = & net.exe user $user $pass /add /y 2>&1
if ($LASTEXITCODE -ne 0) { throw "net user add failed ($LASTEXITCODE)" }
$user | Set-Content (Join-Path $OutDir 'unelevated-user.txt') -Encoding ASCII
# A freshly created local user is a member of Users only (never
# Administrators) - record the groups for the evidence file.
$null = & net.exe user $user 2>&1
& whoami.exe /groups /user "$env:COMPUTERNAME\$user" 2>&1 |
    Out-String | Set-Content (Join-Path $OutDir 'unelevated-user-groups.txt')

# --- The CDP wrapper (the child does not inherit this shell's env) ----------
$cmd = Join-Path $OutDir 'unelevated-launch.cmd'
@"
@echo off
set WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=$CdpPort
start "" "$appExe"
"@ | Set-Content $cmd -Encoding ASCII

$before = @(Get-Process -Name 'recovery-wizard-ui' -ErrorAction SilentlyContinue)

$sec = ConvertTo-SecureString $pass -AsPlainText -Force
$cred = New-Object System.Management.Automation.PSCredential("$env:COMPUTERNAME\$user", $sec)
Start-Process -FilePath $env:ComSpec -ArgumentList '/c', "`"$cmd`"" -Credential $cred -WindowStyle Hidden
Write-Host "[unelevated-launch] spawned as $env:COMPUTERNAME\$user (standard user)"

# --- Wait for the CDP endpoint -----------------------------------------------
$deadline = (Get-Date).AddSeconds(90)
$ready = $false
while ((Get-Date) -lt $deadline) {
    try {
        $v = Invoke-RestMethod -Uri "http://127.0.0.1:$CdpPort/json/version" -TimeoutSec 2
        if ($v.Browser) { $ready = $true; break }
    } catch { Start-Sleep -Milliseconds 500 }
}
if (-not $ready) { throw "unelevated instance CDP did not come up on port $CdpPort within 90s" }

# --- Identify the app process + ASSERT its token is genuinely unelevated ----
$app = $null
foreach ($i in 1..20) {
    $app = @(Get-Process -Name 'recovery-wizard-ui' -ErrorAction SilentlyContinue |
        Where-Object { $before.ProcessId -notcontains $_.Id } |
        Sort-Object StartTime | Select-Object -Last 1)
    if ($app) { break }
    Start-Sleep -Milliseconds 500
}
if (-not $app) { throw 'could not identify the unelevated app process (CDP is up; pid tracking failed)' }

$token = [TokenElevationProbe2]::IsElevated($app[0].Id)
[ordered]@{
    pid             = $app[0].Id
    tokenElevation  = $token
    mechanism       = 'CreateProcessWithLogonW (standard user rwprobe)'
    user            = "$env:COMPUTERNAME\$user"
    probedUtc       = (Get-Date).ToUniversalTime().ToString('o')
} | ConvertTo-Json | Set-Content (Join-Path $OutDir 'unelevated-token.json') -Encoding UTF8
Write-Host "[unelevated-launch] app pid=$($app[0].Id) token=$token"
$app[0].Id | Set-Content (Join-Path $OutDir 'unelevated-app-pid.txt') -Encoding ASCII

if ($token -ne 'not-elevated') {
    throw "LAUNCH PRECONDITION FAILED: the spawned app token is '$token' - the banner assertion would be dishonest; refusing to continue"
}
Write-Host '[unelevated-launch] precondition holds: app token is not-elevated'
