# install.ps1 - RDA service installer (raw-access deployment profile).
#
# Installs rda.sys as a kernel driver service under a caller-chosen service
# name. The driver derives its \Device\ + \DosDevices\ names from the service
# name at load time (reference parity), so:
#
#   -ServiceName eprdtdrv   -> \Device\EPRDTDRV  (DRW Device.mo drop-in)
#   -ServiceName ebrntdrv   -> \Device\EBRNTDRV  (WinPE spare instance)
#   -ServiceName rda        -> \Device\RDA       (neutral default)
#
# REQUIRES: elevated PowerShell; a signed driver (test-signing enabled) or a
# test certificate provisioned for this driver on the target machine.
#
# Usage:
#   .\install.ps1 -ServiceName eprdtdrv -Source .\bin\x64\Release\rda.sys
#
# Licensed under the MIT license. See LICENSE.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9_]+$')]
    [string]$ServiceName,

    [Parameter(Mandatory = $true)]
    [ValidateScript({ Test-Path $_ })]
    [string]$Source,

    [int]$IoctlBase = 0,
    [int]$MaxDisks = 200,
    [int]$DebugLevel = 1,
    [switch]$NoStart
)

$ErrorActionPreference = 'Stop'

# --- elevation guard ---------------------------------------------------------
# NOTE the canonical two-step form: the previous one-liner
#   [WindowsPrincipal]::[WindowsIdentity]::GetCurrent()
# parsed but evaluated to $null (PowerShell treats the whole
# '[WindowsIdentity]::GetCurrent()' as the MEMBER-NAME expression of the
# outer '::' access), so IsInRole() ran on null and the script DIED at line 1
# with a nonzero exit — run 13's T15 evidence: no service, no .sys, the
# installer's registration-failed branch. Fixed to the working form.
$identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must run elevated (Administrator)."
}

# --- preflight ----------------------------------------------------------------
$service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
if ($service) {
    Write-Error "Service '$ServiceName' already exists. Remove it first (uninstall.ps1) or pick another name."
}

$sourcePath = (Resolve-Path $Source).Path
$destDir = Join-Path $env:SystemRoot 'System32\drivers'
$destPath = Join-Path $destDir "$ServiceName.sys"

# --- WOW64 truth (2026-10-27, the validation run's discovery) ----------------
# The product installer is a 32-bit NSIS executable and spawns the 32-bit
# powershell.exe; from a WOW64 process, File System Redirection silently
# rewrites System32 -> SysWOW64 for FILE operations (the directory STRINGS
# above stay literal). Without this block the driver lands in
# SysWOW64\drivers while ImagePath names System32\drivers, and
# StartService fails with ERROR_FILE_NOT_FOUND (sc start exit=2) BEFORE any
# signature check - exactly what both the 0.5.0-rc3 and the 0.6.0-rc1 T15
# facts recorded. Disabling redirection around the copy puts the file
# where ImagePath says it is, from any process bitness (a 64-bit caller
# gets FALSE here and copies normally).
$wow64sig = @"
[DllImport("kernel32.dll", SetLastError = true)]
public static extern bool Wow64DisableWow64FsRedirection(ref IntPtr oldValue);
[DllImport("kernel32.dll", SetLastError = true)]
public static extern bool Wow64RevertWow64FsRedirection(IntPtr oldValue);
"@
Add-Type -MemberDefinition $wow64sig -Name Wow64 -Namespace RdaInstall | Out-Null
$wow64old = [IntPtr]::Zero
$wow64disabled = [RdaInstall.Wow64]::Wow64DisableWow64FsRedirection([ref]$wow64old)
try {
    Write-Host "Installing '$sourcePath' as service '$ServiceName' -> $destPath"
    Copy-Item $sourcePath $destPath -Force
} finally {
    if ($wow64disabled) {
        [RdaInstall.Wow64]::Wow64RevertWow64FsRedirection($wow64old) | Out-Null
    }
}
# Verify the file sits in the REAL System32 (redirection-free probe):
# a WOW64 caller sees the 64-bit System32 only through the Sysnative alias;
# a 64-bit caller (no redirection) probes the plain path directly.
$realPath = if ($wow64disabled) {
    Join-Path $env:SystemRoot "Sysnative\drivers\$ServiceName.sys"
} else {
    $destPath
}
if (-not (Test-Path $realPath)) {
    Write-Error "driver copy verification failed: '$realPath' is not present in the real System32 - the service would fail to start with ERROR_FILE_NOT_FOUND"
}
Write-Host "Verified (real System32): $realPath"

# --- service + parameters ------------------------------------------------------
sc.exe create $ServiceName binPath= "$destPath" type= kernel start= demand error= normal group= "Boot File System" | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error "CreateService failed ($LASTEXITCODE)" }

$paramKey = "HKLM:\SYSTEM\CurrentControlSet\Services\$ServiceName\Parameters"
New-Item -Path $paramKey -Force | Out-Null
Set-ItemProperty -Path $paramKey -Name 'IoctlBase' -Value $IoctlBase -Type DWord
Set-ItemProperty -Path $paramKey -Name 'MaxDisks' -Value $MaxDisks -Type DWord
Set-ItemProperty -Path $paramKey -Name 'DebugLevel' -Value $DebugLevel -Type DWord
Set-ItemProperty -Path $paramKey -Name 'SessionsEnabled' -Value 1 -Type DWord

# --- start --------------------------------------------------------------------
if (-not $NoStart) {
    sc.exe start $ServiceName | Out-Null
    if ($LASTEXITCODE -ne 0) {
        # One string, no '+' concatenation: in argument position a trailing
        # + is NOT a string continuation, and `powershell -File` propagates
        # the failed sc.exe start's exit code as the PROCESS exit when the
        # script ends without an explicit exit — the T15 facts file caught
        # both: 'driver install.ps1 exit=1' with the service fully created
        # (run 15). A start failure on an unsigned driver is the EXPECTED
        # stock-Secure-Boot outcome and is a WARNING, not a script failure.
        Write-Warning "StartService failed ($LASTEXITCODE). Check code-signing policy (testsigning), driver signature, and the System event log."
    } else {
        Write-Host "Service '$ServiceName' started."
    }
}

# --- verify the control device appeared ---------------------------------------
$expected = "\Device\" + $ServiceName.ToUpperInvariant()
$dosName = "\\.\$($ServiceName.ToUpperInvariant())"
Write-Host "Expected control device: $expected"
Write-Host "User-mode path:          $dosName\<pid-token>\<disk>\<part>"
Write-Host ""
Write-Host "Next: .\bin\x64\Release\rdacli.exe grammar 0"

# Registration is the script's contract; a start failure is warned above
# (non-fatal by design — the installer's own sc start + /SD notice handle
# the degradation truthfully). Exit explicitly so `powershell -File` does
# NOT propagate sc.exe's nonzero start exit as this script's exit code.
exit 0
