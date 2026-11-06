# unelevated-launch.ps1 - the D-PRIV-01 twin: an UNELEVATED start of the
# installed product MUST be refused BY THE OPERATING SYSTEM.
#
# Post-manifest posture (0.9.0): the exe embeds requireAdministrator, so
# the OS itself is the privilege gate - CreateProcessWithLogonW with a
# standard-user token fails with the elevation refusal (win32 740) and
# no app process ever exists. This launcher ASSERTS that refusal. (The
# pre-manifest twin asserted the retired asInvoker posture - banner +
# relaunch + no-mock - via a CDP-driven unelevated instance; that
# surface no longer exists and the probe rode with it into history.)
#
# The assertion is two-sided:
#   PASS  = the direct unelevated Start-Process throws the elevation
#           refusal AND no recovery-wizard-ui process appears.
#   FAIL  = the app STARTS with the unelevated token (the privilege
#           model is broken - requireAdministrator not in effect), or
#           the spawn fails with an unrelated error (diagnostics are
#           captured either way).
#
# Usage: unelevated-launch.ps1 -OutDir <dir> -ExpectRefusal
# Outputs: unelevated.json, unelevated-user.txt,
#          unelevated-user-groups.txt (+ failure diagnostics if any)

param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $true)][switch]$ExpectRefusal
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

$before = @(Get-Process -Name 'recovery-wizard-ui' -ErrorAction SilentlyContinue)

$sec = ConvertTo-SecureString $pass -AsPlainText -Force
$cred = New-Object System.Management.Automation.PSCredential("$env:COMPUTERNAME\$user", $sec)

# --- The refusal assertion ---------------------------------------------------
# Launch the APP EXE DIRECTLY (no cmd wrapper - an asInvoker cmd would
# start fine and mask the refusal one level down).
$spawnError = $null
try {
    Start-Process -FilePath $appExe -Credential $cred -WindowStyle Hidden -ErrorAction Stop
} catch {
    $spawnError = $_.Exception
}

# Grace window: if the spawn "succeeded", the process must appear here.
if (-not $spawnError) { Start-Sleep -Seconds 8 }
$alive = @(Get-Process -Name 'recovery-wizard-ui' -ErrorAction SilentlyContinue |
    Where-Object { $before.ProcessId -notcontains $_.Id })

$errText = if ($spawnError) { $spawnError.Message } else { '' }
$isRefusal = $errText -match 'elevat|740|administrator|requested operation'
$started = $alive.Count -gt 0

[ordered]@{
    posture           = 'D-PRIV-01: requireAdministrator - the OS refuses an unelevated start'
    spawnThrew        = [bool]$spawnError
    spawnError        = $errText
    refusedByOs       = [bool]($isRefusal -and -not $started)
    appProcessStarted = $started
    startedToken      = if ($started) { [TokenElevationProbe2]::IsElevated($alive[0].Id) } else { $null }
    mechanism         = 'CreateProcessWithLogonW (standard user rwprobe, direct exe)'
    user              = "$env:COMPUTERNAME\$user"
    probedUtc         = (Get-Date).ToUniversalTime().ToString('o')
} | ConvertTo-Json | Set-Content (Join-Path $OutDir 'unelevated.json') -Encoding UTF8
Write-Host "[unelevated-launch] spawnThrew=$([bool]$spawnError) refusedByOs=$($isRefusal -and -not $started) started=$started"
if ($errText) { Write-Host "[unelevated-launch] spawn error: $errText" }

if ($started) {
    # The app STARTED with an unelevated token - the privilege model is
    # broken. Record the token and refuse to pass.
    $alive | ForEach-Object { Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue }
    throw "D-PRIV-01 twin gate FAILED: the app STARTED unelevated (token: $($alive[0].Id) -> $([TokenElevationProbe2]::IsElevated($alive[0].Id))) - requireAdministrator is not in effect"
}
if ($isRefusal) {
    Write-Host '[unelevated-launch] the OS refused the unelevated start - the privilege gate holds'
    exit 0
}
# Not a refusal and not started: an unrelated failure - capture and fail.
$errText | Set-Content (Join-Path $OutDir 'unelevated-launch-failure.txt') -Encoding UTF8
throw "unelevated launch did not produce the expected elevation refusal (error: $errText; diagnostics in unelevated-launch-failure.txt)"
