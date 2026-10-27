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

# --- WOW64 truth (2026-10-27): the uninstaller is spawned by the 32-bit
# NSIS uninstaller, so this script may run as a WOW64 process - in which
# case File System Redirection rewrites System32 -> SysWOW64 for file
# operations and the delete would MISS the real driver file (install.ps1
# has the full story). Disable redirection around the removal; verify
# against the Sysnative alias when it applies.
$destPath = Join-Path $env:SystemRoot "System32\drivers\$ServiceName.sys"
$wow64sig = @"
[DllImport("kernel32.dll", SetLastError = true)]
public static extern bool Wow64DisableWow64FsRedirection(ref IntPtr oldValue);
[DllImport("kernel32.dll", SetLastError = true)]
public static extern bool Wow64RevertWow64FsRedirection(IntPtr oldValue);
"@
Add-Type -MemberDefinition $wow64sig -Name Wow64 -Namespace RdaRemove | Out-Null
$wow64old = [IntPtr]::Zero
$wow64disabled = [RdaRemove.Wow64]::Wow64DisableWow64FsRedirection([ref]$wow64old)
try {
    if (Test-Path $destPath) {
        Remove-Item $destPath -Force -ErrorAction SilentlyContinue
        Write-Host "Removed $destPath"
    }
} finally {
    if ($wow64disabled) {
        [RdaRemove.Wow64]::Wow64RevertWow64FsRedirection($wow64old) | Out-Null
    }
}
$realPath = if ($wow64disabled) {
    Join-Path $env:SystemRoot "Sysnative\drivers\$ServiceName.sys"
} else {
    $destPath
}
if (Test-Path $realPath) {
    Write-Error "uninstall verification failed: '$realPath' still exists after removal"
}
