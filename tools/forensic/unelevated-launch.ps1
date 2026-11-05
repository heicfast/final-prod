# unelevated-launch.ps1 - launch the INSTALLED product under a restricted
# (Basic User) token so the user-mode refusal contract can be probed: the
# product must show the elevation banner and refuse raw-disk work — never
# run degraded, never pretend to work (the product-side twin of core's
# exit-4 elevation probes, INTEGRATION-PLAN section 6.3).
#
# Mechanics: `runas /trustlevel:0x20000` asks the Secondary Logon service
# to create the process with a Basic User token (no UAC prompt, no
# password). runas does not pass the caller's environment reliably, so the
# WebView2 CDP argument rides a generated .cmd wrapper instead.
#
# Usage: unelevated-launch.ps1 -OutDir <dir> [-CdpPort 9223]
# Outputs: unelevated-app-pid.txt (the app process, best-effort)

param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$CdpPort = 9223
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$appExe = 'C:\Program Files\HeicFast\Recovery Wizard\recovery-wizard-ui.exe'
if (-not (Test-Path $appExe)) { throw "app exe not found: $appExe" }

$cmd = Join-Path $OutDir 'unelevated-launch.cmd'
@"
@echo off
set WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--remote-debugging-port=$CdpPort
start "" "$appExe"
"@ | Set-Content $cmd -Encoding ASCII

# Baseline: the running (elevated) app instance's start time, so the
# restricted instance can be told apart afterwards.
$before = @(Get-Process -Name 'recovery-wizard-ui' -ErrorAction SilentlyContinue)

& runas.exe /trustlevel:0x20000 "cmd /c `"$cmd`""
if ($LASTEXITCODE -ne 0) { throw "runas /trustlevel failed: $LASTEXITCODE" }

# Wait for the CDP endpoint of the restricted instance.
$deadline = (Get-Date).AddSeconds(60)
$ready = $false
while ((Get-Date) -lt $deadline) {
    try {
        $v = Invoke-RestMethod -Uri "http://127.0.0.1:$CdpPort/json/version" -TimeoutSec 2
        if ($v.Browser) { $ready = $true; break }
    } catch { Start-Sleep -Milliseconds 500 }
}
if (-not $ready) { throw "restricted instance CDP did not come up on port $CdpPort within 60s" }

# Best-effort PID: the newest app process that was not running before.
$after = @(Get-Process -Name 'recovery-wizard-ui' -ErrorAction SilentlyContinue |
    Where-Object { $before.ProcessId -notcontains $_.Id } |
    Sort-Object StartTime | Select-Object -Last 1)
if ($after) {
    $after.Id | Set-Content (Join-Path $OutDir 'unelevated-app-pid.txt') -Encoding ASCII
    Write-Host "[unelevated-launch] restricted app pid=$($after.Id) cdp=$CdpPort"
} else {
    Write-Warning '[unelevated-launch] could not identify the restricted app pid (CDP is up; probe continues)'
}
