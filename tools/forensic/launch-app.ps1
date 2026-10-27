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
