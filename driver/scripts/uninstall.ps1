# uninstall.ps1 - removes an RDA driver service.
#
# Usage: .\uninstall.ps1 -ServiceName eprdtdrv
#
# Licensed under the MIT license. See LICENSE.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ServiceName
)

$ErrorActionPreference = 'Stop'

# --- elevation guard (canonical form — see install.ps1's note: the old
# [WindowsPrincipal]::[WindowsIdentity]::GetCurrent() one-liner evaluated to
# $null and killed the script at line 1; the .nsi uninstaller's sc.exe
# fallback masked it, but the driver's own teardown never ran) ---
$identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must run elevated (Administrator)."
}

$service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
if ($service) {
    if ($service.Status -ne 'Stopped') {
        sc.exe stop $ServiceName | Out-Null
        Start-Sleep -Seconds 1
    }
    sc.exe delete $ServiceName | Out-Null
    Write-Host "Service '$ServiceName' deleted."
} else {
    Write-Warning "Service '$ServiceName' not found."
}

$destPath = Join-Path $env:SystemRoot "System32\drivers\$ServiceName.sys"
if (Test-Path $destPath) {
    Remove-Item $destPath -Force -ErrorAction SilentlyContinue
    Write-Host "Removed $destPath"
}
