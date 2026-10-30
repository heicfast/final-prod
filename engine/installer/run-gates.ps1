# run-gates.ps1 - the S-038 test gates, executable form (PACKAGING.md
# section 11). Runs on an elevated Windows host with the built setup.exe.
#
#   Gate 1 InstallerSilentRoundTrip: /VERYSILENT install (zero prompts) ->
#        ARP UninstallString + QuietUninstallString present and well-formed
#        (the quiet form ends " /SILENT") -> silent uninstall via the ARP
#        QuietUninstallString verbatim -> NO residue (files, ARP, version
#        key).
#   Gate 2 InstallerVersionKeyWritten: the version key's write+verify path -
#        HKLM\SOFTWARE\heicfast\core_recovery\Version reads back exactly the
#        version the installer was built with.
#   Gate 3 InstallerOSGateMatrix lives in tools/test_installer_osgate.py
#        (injectable version tuples + the ISS sync pin; no install needed).
#
# session-96: the kernel-driver component is REMOVED (the direct raw I/O
# replacement) - the installer ships the core only and never touches the
# service control manager; there is no driver component, no service gate,
# and no -SkipDriver switch (the engine IS the whole product).
#
# Usage:
#   pwsh -File installer\run-gates.ps1 -SetupExe dist\core-recovery-setup.exe `
#        -Version 1.0.0-gate.4242
#
# Exit: 0 = all gates green, 1 = any failure (loud, with the failing check).

param(
    [Parameter(Mandatory = $true)] [string] $SetupExe,
    [Parameter(Mandatory = $true)] [string] $Version
)

$ErrorActionPreference = 'Stop'
$AppId = '{2F4B9C31-7A08-4D6E-9B5C-3E1A8F0D4C72}'
$ArpKey = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${AppId}_is1"
$VersionKey = 'HKLM:\SOFTWARE\heicfast\core_recovery'

$script:Failures = @()
function Assert-Gate([bool] $Condition, [string] $Label, [string] $Detail = '') {
    if ($Condition) {
        Write-Host "  ok: $Label"
    } else {
        Write-Host "  FAIL: $Label $(if ($Detail) { "($Detail)" })"
        $script:Failures += $Label
    }
}

if (-not (Test-Path $SetupExe)) { Write-Host "::error::setup.exe not found: $SetupExe"; exit 1 }
$SetupExe = (Resolve-Path $SetupExe).Path

# ---- clean slate: a prior install would contaminate the residue checks ----
if (Test-Path $ArpKey) { Write-Host "::error::ARP entry already present - uninstall the prior install first"; exit 1 }
if (Test-Path $VersionKey) { Write-Host "::error::version key already present - clean it first"; exit 1 }

# ---- install: /VERYSILENT + /SUPPRESSMSGBOXES + /NORESTART (zero prompts) ----
Write-Host 'gate 1a: silent install (/VERYSILENT /SUPPRESSMSGBOXES /NORESTART, core only)'
$installArgs = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART')
$proc = Start-Process -FilePath $SetupExe -ArgumentList $installArgs -Wait -PassThru
Assert-Gate ($proc.ExitCode -eq 0) 'install exit code 0' "got $($proc.ExitCode)"

Start-Sleep -Seconds 2

# ---- gate 2: the version key (write + verify path proven by the read-back) ----
Write-Host 'gate 2: InstallerVersionKeyWritten'
Assert-Gate (Test-Path $VersionKey) 'version key exists' $VersionKey
if (Test-Path $VersionKey) {
    $ver = (Get-ItemProperty $VersionKey).Version
    Assert-Gate ($ver -ceq $Version) "version key reads back exactly '$Version'" "got '$ver'"
}

# ---- gate 1b: the ARP contract ----
Write-Host 'gate 1b: ARP entries present and well-formed'
Assert-Gate (Test-Path $ArpKey) 'ARP entry exists' $ArpKey
if (Test-Path $ArpKey) {
    $arp = Get-ItemProperty $ArpKey
    Assert-Gate ([bool]$arp.UninstallString) 'ARP UninstallString present' "$($arp.UninstallString)"
    Assert-Gate ($arp.QuietUninstallString -and $arp.QuietUninstallString.EndsWith(' /SILENT')) `
        'ARP QuietUninstallString present, ending " /SILENT"' "$($arp.QuietUninstallString)"
    Assert-Gate ($arp.DisplayName -ceq 'core_recovery') 'ARP DisplayName' "$($arp.DisplayName)"
    Assert-Gate ($arp.DisplayVersion -ceq $Version) "ARP DisplayVersion == $Version" "$($arp.DisplayVersion)"
    Assert-Gate ([bool]$arp.Publisher) 'ARP Publisher present' "$($arp.Publisher)"
    Assert-Gate ([bool]$arp.InstallLocation -and (Test-Path $arp.InstallLocation)) 'ARP InstallLocation exists on disk' "$($arp.InstallLocation)"
}

# ---- the silent uninstall: the ARP QuietUninstallString VERBATIM ----
Write-Host 'gate 1d: silent uninstall via the ARP QuietUninstallString verbatim'
$quiet = (Get-ItemProperty $ArpKey).QuietUninstallString
$installDir = (Get-ItemProperty $ArpKey).InstallLocation
# normalize doubled quotes (""path"" - the pre-canonicalization form) then
# split exe from args at the closing quote; $args is reserved in PowerShell
$quietNorm = $quiet -replace '""', '"'
$exe = $null
$quietArgs = ''
if ($quietNorm.StartsWith('"')) {
    $close = $quietNorm.IndexOf('"', 1)
    if ($close -gt 1) {
        $exe = $quietNorm.Substring(1, $close - 1)
        $quietArgs = $quietNorm.Substring($close + 1).Trim()
    }
}
if (-not $exe) {
    # unquoted form: split at the first space
    $sp = $quietNorm.IndexOf(' ')
    if ($sp -gt 0) {
        $exe = $quietNorm.Substring(0, $sp)
        $quietArgs = $quietNorm.Substring($sp + 1).Trim()
    } else {
        $exe = $quietNorm
    }
}
if (-not $exe -or -not (Test-Path $exe)) {
    Write-Host "::error::cannot resolve the uninstaller exe from QuietUninstallString: '$quiet'"
    exit 1
}
# the uninstaller re-spawns itself; wait on the process then poll the
# residue (the poll-bounded block below) instead of trusting the exit
$proc = Start-Process -FilePath $exe -ArgumentList $quietArgs -PassThru
try { $proc | Wait-Process -Timeout 180 -ErrorAction SilentlyContinue } catch { }

# ---- gate 1e: NO residue (poll-bounded: the uninstaller's second phase) ----
Write-Host 'gate 1e: no residue (files, ARP, version key)'
$deadline = (Get-Date).AddSeconds(90)
do {
    Start-Sleep -Seconds 3
    $arpGone = -not (Test-Path $ArpKey)
    $dirGone = ((-not $installDir) -or (-not (Test-Path $installDir)))
    $verGone = -not (Test-Path $VersionKey)
    $clean = $arpGone -and $dirGone -and $verGone
} while (-not $clean -and (Get-Date) -lt $deadline)

Assert-Gate $arpGone 'ARP entry removed'
Assert-Gate $dirGone "install dir removed ($installDir)"
Assert-Gate $verGone 'version key removed'
# session-96: nothing service-related may exist - the installer never
# touches the SCM, so a leftover 'RDA' service would be pre-existing
# contamination this gate now surfaces instead of ignoring
$svcLeftover = sc.exe query RDA 2>$null | Select-String -Pattern 'SERVICE_NAME'
Assert-Gate (-not $svcLeftover) "no kernel service installed by the product (sc query RDA must fail)" (sc.exe query RDA 2>$null | Out-String)

# ---- verdict ----
Write-Host ''
if ($script:Failures) {
    Write-Host "GATES FAILED: $($script:Failures.Count) check(s):"
    $script:Failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}
Write-Host 'ALL GATES GREEN (InstallerSilentRoundTrip + InstallerVersionKeyWritten)'
exit 0
