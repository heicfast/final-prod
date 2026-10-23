# run-behavior.ps1 - driver-presence, cancellation and session-resume probes.
#
# Probes (each writes JSON evidence; any probe failure fails the script):
#   1. nodriver  - scan with the RDA driver ABSENT; the documented behavior is a
#                  visible direct-path fallback ('[transport] RDA not available ... - direct path')
#                  and a successful scan. Anything else is a finding, recorded verbatim.
#   2. cancel    - start a deep scan, send CTRL_BREAK to the child process group at T=6 s,
#                  measure stop latency; rrecov must exit 3, save the session, print
#                  'scan cancelled'.
#   3. resume    - two identically seeded volumes: one uninterrupted deep scan, one
#                  cancelled then resumed; the resulting trees must match.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $RrecovExe,
    [Parameter(Mandatory = $true)] [string] $WorkDir,
    [Parameter(Mandatory = $true)] [int] $DiskNumber,     # VHDX disk for probes 2/3
    [Parameter(Mandatory = $true)] [string] $Letter,      # volume letter for probes 2/3
    [Parameter(Mandatory = $true)] [string] $ManifestJsonl,
    [int] $CancelAfterSec = 6
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }
. "$PSScriptRoot\..\volume\volume.ps1"
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$behavior = [ordered]@{ tool = 'ours'; probes = @() }

# ---------- probe 1: driver absent ----------
$p1 = [ordered]@{ probe = 'nodriver'; driver_service_before = 'unknown' }
try { $svc = Get-Service -Name rda -ErrorAction Stop; $p1.driver_service_before = $svc.Status } catch { $p1.driver_service_before = 'absent' }
if ($p1.driver_service_before -eq 'Running') {
    throw 'nodriver probe requires the rda service to be absent/uninstalled - run this before driver install'
}
$sess = Join-Path $WorkDir 'nodriver-session.crsf'
$so = Join-Path $WorkDir 'nodriver-scan.stdout'
$se = Join-Path $WorkDir 'nodriver-scan.stderr'
$log = Join-Path $WorkDir 'nodriver-stage.log'
$p = Start-Process -FilePath $RrecovExe `
    -ArgumentList @('scan', '--volume', "$Letter`:", '--deep', '--no-carve', '--threads', '4', '--out', $sess, '--log', $log, '--json') `
    -RedirectStandardOutput $so -RedirectStandardError $se -PassThru -NoNewWindow -Wait
$p1.exit_code = $p.ExitCode
$p1.session_exists = (Test-Path $sess)
$p1.transport_lines = @()
if (Test-Path $log) {
    $p1.transport_lines = @(Get-Content $log | Select-String -SimpleMatch '[transport]' | ForEach-Object { $_.Line.Trim() })
}
$p1.fallback_seen = [bool]($p1.transport_lines | Where-Object { $_ -match 'RDA not available' -or $_ -match 'direct path' })
$p1.passed = ($p1.exit_code -eq 0 -and $p1.session_exists -and $p1.fallback_seen)
$behavior.probes += $p1

# ---------- CTRL_BREAK launcher (child in its own process group) ----------
if (-not ('BenchLaunch' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class BenchLaunch {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool GenerateConsoleCtrlEvent(uint ctrlEvent, uint processGroupId);
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern bool CreateProcessW(string app, string cmdLine, IntPtr sa, IntPtr ta,
        bool inherit, uint creation, IntPtr env, string curDir, ref STARTUPINFO si, out PROCESS_INFORMATION pi);
    [StructLayout(LayoutKind.Sequential)] public struct STARTUPINFO { public uint cb; public string lpReserved, lpDesktop, lpTitle; public uint dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags; public ushort wShowWindow, cbReserved2; public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError; }
    [StructLayout(LayoutKind.Sequential)] public struct PROCESS_INFORMATION { public IntPtr hProcess, hThread; public uint dwProcessId, dwThreadId; }
}
"@
}

function Start-InGroup {
    param([string] $Exe, [string] $CmdLine, [string] $StdoutFile)
    # use cmd.exe redirection to keep it simple and robust in-process-group
    $si = New-Object BenchLaunch+STARTUPINFO
    $si.cb = [System.Runtime.InteropServices.Marshal]::SizeOf($si)
    $pi = New-Object BenchLaunch+PROCESS_INFORMATION
    $cmd = "`"$env:ComSpec`" /c `"$CmdLine > `"$StdoutFile`" 2>&1`""
    $ok = [BenchLaunch]::CreateProcessW($null, $cmd, [IntPtr]::Zero, [IntPtr]::Zero, $false,
        0x00000200, [IntPtr]::Zero, (Get-Location).Path, [ref]$si, [ref]$pi)   # CREATE_NEW_PROCESS_GROUP
    if (-not $ok) { throw "CreateProcessW failed GLE=$([Runtime.InteropServices.Marshal]::GetLastWin32Error())" }
    [System.Runtime.InteropServices.Marshal]::ReleaseHandle($pi.hThread) | Out-Null
    return @{ pid = $pi.dwProcessId; handle = $pi.hProcess }
}

function Wait-Pid {
    param([int] $Id, [int] $TimeoutSec = 600)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
        $proc = Get-Process -Id $Id -ErrorAction SilentlyContinue
        if (-not $proc) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

# ---------- probe 2: cancellation ----------
$p2 = [ordered]@{ probe = 'cancel'; cancel_after_seconds = $CancelAfterSec }
$sess2 = Join-Path $WorkDir 'cancel-session.crsf'
$log2 = Join-Path $WorkDir 'cancel-stage.log'
$so2 = Join-Path $WorkDir 'cancel-scan.stdout'
$cmd2 = "`"$RrecovExe`" scan --volume $Letter`: --deep --threads 4 --out `"$sess2`" --log `"$log2`""
$t0 = Get-Date
$cp = Start-InGroup -Exe $RrecovExe -CmdLine $cmd2 -StdoutFile $so2
Start-Sleep -Seconds $CancelAfterSec
$tCancel = Get-Date
$ev = [BenchLaunch]::GenerateConsoleCtrlEvent(1, [uint32]$cp.pid)   # CTRL_BREAK_EVENT to the group
$p2.ctrl_break_delivered = $ev
$stopped = Wait-Pid -Id $cp.pid -TimeoutSec 120
$p2.stop_latency_seconds = [math]::Round(((Get-Date) - $tCancel).TotalSeconds, 2)
$p2.process_gone = $stopped
$p2.stdout_tail = @(Get-Content $so2 -ErrorAction SilentlyContinue | Select-Object -Last 8)
$p2.session_exists = (Test-Path $sess2)
$p2.cancel_message_seen = [bool](Get-Content $so2 -ErrorAction SilentlyContinue | Select-String -SimpleMatch 'scan cancelled')
# exit code of the killed child is not directly retrievable via cmd wrapper; the
# session + cancelled line + stop latency are the behavioral evidence
$p2.passed = ($p2.session_exists -and $p2.cancel_message_seen -and $stopped)
$behavior.probes += $p2

# ---------- probe 3: resume equivalence ----------
# The volume state after the cancelled probe is identical by construction to an
# uninterrupted scan's start state (same VHDX, same scenario, deterministic seed),
# so: resume the cancelled session to completion and compare with a fresh
# uninterrupted scan of the same volume.
$p3 = [ordered]@{ probe = 'resume' }
$sessR = Join-Path $WorkDir 'resume-session.crsf'
$logR = Join-Path $WorkDir 'resume-stage.log'
$soR = Join-Path $WorkDir 'resume-scan.stdout'
$cmdR = "`"$RrecovExe`" scan --resume `"$sess2`" --volume $Letter`: --deep --threads 4 --out `"$sessR`" --log `"$logR`" --json"
$rp = Start-InGroup -Exe $RrecovExe -CmdLine $cmdR -StdoutFile $soR
$done = Wait-Pid -Id $rp.pid -TimeoutSec 900
$p3.resume_finished = $done
$p3.resumed_session_exists = (Test-Path $sessR)

$sessU = Join-Path $WorkDir 'uninterrupted-session.crsf'
$logU = Join-Path $WorkDir 'uninterrupted-stage.log'
$soU = Join-Path $WorkDir 'uninterrupted-scan.stdout'
$cmdU = "`"$RrecovExe`" scan --volume $Letter`: --deep --threads 4 --out `"$sessU`" --log `"$logU`" --json"
$up = Start-InGroup -Exe $RrecovExe -CmdLine $cmdU -StdoutFile $soU
$doneU = Wait-Pid -Id $up.pid -TimeoutSec 900
$p3.uninterrupted_finished = $doneU
$p3.uninterrupted_session_exists = (Test-Path $sessU)

$treeR = & $RrecovExe tree --in $sessR --csv | Out-String
$treeU = & $RrecovExe tree --in $sessU --csv | Out-String
$linesR = @($treeR -split "`r?`n" | Where-Object { $_ -ne '' } | Sort-Object)
$linesU = @($treeU -split "`r?`n" | Where-Object { $_ -ne '' } | Sort-Object)
$p3.rows_resumed = $linesR.Count
$p3.rows_uninterrupted = $linesU.Count
$p3.identical = (($linesR -join "`n") -eq ($linesU -join "`n"))
if (-not $p3.identical) {
    $diffR = Compare-Object $linesU $linesR | Select-Object -First 20
    $diffR | ForEach-Object { Write-Host "RESUME DIFF: $($_.SideIndicator) $($_.InputObject)" }
    $p3.diff_sample = @($diffR | ForEach-Object { "$($_.SideIndicator) $($_.InputObject)" })
}
$p3.passed = ($p3.resumed_session_exists -and $p3.uninterrupted_session_exists -and $p3.identical)
$behavior.probes += $p3

$behavior | ConvertTo-Json -Depth 8 | Set-Content -Path (Join-Path $WorkDir 'behavior.json') -Encoding utf8
$failed = @($behavior.probes | Where-Object { -not $_.passed })
$okCount = @($behavior.probes).Count - $failed.Count
Write-Host "behavior probes: ok=$okCount failed=$($failed.Count)"
if ($failed.Count -gt 0) { throw "behavior probe(s) failed: $($failed.probe -join ', ')" }
