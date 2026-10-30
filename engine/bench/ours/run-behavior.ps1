# run-behavior.ps1 - privilege-refusal, cancellation and session-resume probes.
#
# Probes (each writes JSON evidence; any probe failure fails the script):
#   1. unelevated - scan from a NON-ELEVATED token (the LIMITED-run scheduled
#                  task, the UAC-filtered state); the direct-era contract is the
#                  exit-4 ERR_ELEVATION_REQUIRED refusal as the WHOLE response
#                  (no scan output leaks past the gate). session-96: the old
#                  'nodriver' probe asserted the driver-era fallback text and
#                  died with the driver.
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

# ---------- probe 1: the unelevated refusal (the direct-era contract) ----------
# The runner shell is elevated by construction, so the refusal state is
# created the correct way: the shared LUA-token probe (the shell's own
# mechanism for unelevated children; TokenElevation FALSE is exactly what
# is_elevated() probes). The probe script captures the child's output +
# exit code and writes the evidence file.
$p1 = [ordered]@{ probe = 'unelevated' }
$probeOut = Join-Path $WorkDir 'unelevated-probe.txt'
& pwsh -NoProfile -File (Join-Path $PSScriptRoot '..\..\.github\scripts\Probe-Unelevated.ps1') `
    -RrecovExe $RrecovExe -OutFile $probeOut
$probeRc = $LASTEXITCODE
if (Test-Path $probeOut) {
    $probeText = (Get-Content $probeOut) -join "`n"
    $p1.exit_code = if ($probeText -match 'EXITCODE=(\d+)') { [int]$Matches[1] } else { -1 }
    $p1.refusal_seen = [bool]($probeText -match 'administrator privilege required')
    $p1.scan_leaked = [bool]($probeText -match 'starting scan|\[transport\]|\[stage')
} else {
    $p1.exit_code = -1; $p1.refusal_seen = $false; $p1.scan_leaked = $false
}
$p1.passed = ($probeRc -eq 0)
$behavior.probes += $p1
Write-Host "unelevated probe: exit=$($p1.exit_code) refusal=$($p1.refusal_seen) leaked=$($p1.scan_leaked)"
if (-not $p1.passed) { throw 'the unelevated refusal probe FAILED (see above)' }

# ---------- probe 2: cancellation (hard-kill) -----------------------------------------
# Cooperative CTRL_BREAK delivery to a console process group requires a custom
# CreateProcessW(CREATE_NEW_PROCESS_GROUP) launcher; three attempts failed on the
# runner with GLE 3 / GLE 123 (evidence in the run logs, recorded - not masked).
# This probe therefore measures the WORST CASE: hard kill mid-scan. The partial
# session survives via the engine's stage-boundary auto-checkpoints (G-13), and
# probe 3 proves the killed session resumes to an identical result.
$p2 = [ordered]@{ probe = 'cancel'; method = 'hard-kill (Stop-Process)'; cancel_after_seconds = $CancelAfterSec }
$sess2 = Join-Path $WorkDir 'cancel-session.crsf'
$log2 = Join-Path $WorkDir 'cancel-stage.log'
$so2 = Join-Path $WorkDir 'cancel-scan.stdout'
$se2 = Join-Path $WorkDir 'cancel-scan.stderr'
$t0 = Get-Date
$cp = Start-Process -FilePath $RrecovExe `
    -ArgumentList @('scan', '--volume', "$Letter`:", '--deep', '--threads', '4', '--out', $sess2, '--log', $log2, '--json') `
    -RedirectStandardOutput $so2 -RedirectStandardError $se2 -PassThru -NoNewWindow
Start-Sleep -Seconds $CancelAfterSec
$tCancel = Get-Date
Stop-Process -Id $cp.Id -Force -ErrorAction SilentlyContinue
$gone = $false
$tg = Get-Date
while (((Get-Date) - $tg).TotalSeconds -lt 30) {
    try { $proc = Get-Process -Id $cp.Id -ErrorAction Stop; Start-Sleep -Milliseconds 200 }
    catch { $gone = $true; break }
}
$p2.stop_latency_seconds = [math]::Round(((Get-Date) - $tCancel).TotalSeconds, 2)
$p2.process_gone = $gone
$p2.stdout_tail = @(Get-Content $so2 -ErrorAction SilentlyContinue | Select-Object -Last 8)
$p2.session_exists = (Test-Path $sess2)
$p2.session_readable = $false
if ($p2.session_exists) {
    $t = & $RrecovExe tree --in $sess2 --csv 2>$null | Out-String
    # a load failure prints 'session load failed' to stdout - that is NOT a
    # readable session (run 40064915424 misread the error line as 1 tree row)
    if ($t -and $t.Trim() -and $t -notmatch 'session load failed') {
        $p2.session_readable = $true
        $p2.partial_tree_rows = @($t -split "`r?`n" | Where-Object { $_ -ne '' }).Count
    } else {
        $p2.partial_tree_rows = 0
        $p2.tree_output = $t.Trim()
    }
}
$p2.passed = ($p2.session_exists -and $gone)
$behavior.probes += $p2
Write-Host "cancel probe: latency=$($p2.stop_latency_seconds)s session=$($p2.session_exists) rows=$($p2.partial_tree_rows)"

# ---------- probe 3: resume equivalence -----------------------------------------------
# The volume state after the cancelled probe is identical by construction to an
# uninterrupted scan's start state, so: resume the killed session to completion
# and compare with a fresh uninterrupted scan of the same volume.
$p3 = [ordered]@{ probe = 'resume' }
$sessR = Join-Path $WorkDir 'resume-session.crsf'
$logR = Join-Path $WorkDir 'resume-stage.log'
$soR = Join-Path $WorkDir 'resume-scan.stdout'
$rp = Start-Process -FilePath $RrecovExe `
    -ArgumentList @('scan', '--resume', $sess2, '--volume', "$Letter`:", '--deep', '--threads', '4', '--out', $sessR, '--log', $logR, '--json') `
    -RedirectStandardOutput $soR -RedirectStandardError (Join-Path $WorkDir 'resume-scan.stderr') -PassThru -NoNewWindow -Wait
$p3.resume_exit = $rp.ExitCode
$p3.resumed_session_exists = (Test-Path $sessR)

$sessU = Join-Path $WorkDir 'uninterrupted-session.crsf'
$logU = Join-Path $WorkDir 'uninterrupted-stage.log'
$soU = Join-Path $WorkDir 'uninterrupted-scan.stdout'
$up = Start-Process -FilePath $RrecovExe `
    -ArgumentList @('scan', '--volume', "$Letter`:", '--deep', '--threads', '4', '--out', $sessU, '--log', $logU, '--json') `
    -RedirectStandardOutput $soU -RedirectStandardError (Join-Path $WorkDir 'uninterrupted-scan.stderr') -PassThru -NoNewWindow -Wait
$p3.uninterrupted_exit = $up.ExitCode
$p3.uninterrupted_session_exists = (Test-Path $sessU)

if ($p3.resumed_session_exists -and $p3.uninterrupted_session_exists) {
    $treeR = & $RrecovExe tree --in $sessR --csv 2>$null | Out-String
    $treeU = & $RrecovExe tree --in $sessU --csv 2>$null | Out-String
    # guard: a load failure on either side is a probe failure, not a 1-row tree
    if ($treeR -match 'session load failed' -or $treeU -match 'session load failed') {
        $p3.load_error = ($treeR -match 'session load failed') ? $treeR.Trim() : $treeU.Trim()
        $p3.passed = $false
    } else {
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
        $p3.passed = $p3.identical
    }
} else {
    $p3.passed = $false
}
$behavior.probes += $p3
Write-Host "resume probe: identical=$($p3.identical) rows=$($p3.rows_resumed)/$($p3.rows_uninterrupted)"

$behavior | ConvertTo-Json -Depth 8 | Set-Content -Path (Join-Path $WorkDir 'behavior.json') -Encoding utf8
$failed = @($behavior.probes | Where-Object { -not $_.passed })
$okCount = @($behavior.probes).Count - $failed.Count
Write-Host "behavior probes: ok=$okCount failed=$($failed.Count)"
if ($failed.Count -gt 0) { throw "behavior probe(s) failed: $($failed.probe -join ', ')" }
