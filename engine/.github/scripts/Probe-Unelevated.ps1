# Probe-Unelevated.ps1 - the unelevated-refusal probe (session-96: the
# direct-access elevation contract).
#
# The runner shell is elevated by construction, so the refusal state is
# created through a LIMITED-run scheduled task: the task executes as the
# runner user with RunLevel Limited (LogonType S4U - runs without an
# interactive session, which the runner's user has none of) - the
# UAC-filtered token, exactly the non-elevated state a desktop user's
# launched process runs under (what is_elevated() probes). The child
# (rrecov scan --disk N) must answer with the exit-4 ERR_ELEVATION_REQUIRED
# refusal as its WHOLE response (no scan output may leak past the gate).
#
# Two modes in one file (the task re-invokes this script with -Child):
#   parent (default): registers + starts the task, polls the output file,
#                     prints the verdict, exits 0/1
#   child  (-Child):  runs rrecov through Start-Process redirection
#                     (never throws on stderr content - the refusal text
#                     rides stderr), captures exit code + output into
#                     <OutFile> (the evidence artifact)
#
# Exit: 0 = the refusal contract held; 1 = it did not (loud, with output).

param(
    [Parameter(Mandatory = $true)] [string] $RrecovExe,
    [Parameter(Mandatory = $true)] [string] $OutFile,
    [int] $Disk = 0,
    [switch] $Child,
    [int] $TimeoutSec = 90
)

# ------------------------- the child: run + capture -------------------------
# NO ErrorActionPreference=Stop here: the refusal text rides stderr, and a
# native-command stderr line under Stop terminates the script before the
# evidence file is written (the never-completed class this probe first hit).
if ($Child) {
    $so = "$OutFile.stdout"
    $se = "$OutFile.stderr"
    try {
        $p = Start-Process -FilePath $RrecovExe -ArgumentList @('scan', '--disk', "$Disk") `
            -RedirectStandardOutput $so -RedirectStandardError $se -PassThru -Wait
        "EXITCODE=$($p.ExitCode)" | Set-Content -LiteralPath $OutFile -Encoding utf8
    } catch {
        "EXITCODE=-1" | Set-Content -LiteralPath $OutFile -Encoding utf8
        $_.Exception.Message | Add-Content -LiteralPath $OutFile -Encoding utf8
    }
    if (Test-Path -LiteralPath $so) { Get-Content -LiteralPath $so | Add-Content -LiteralPath $OutFile -Encoding utf8 }
    if (Test-Path -LiteralPath $se) { Get-Content -LiteralPath $se | Add-Content -LiteralPath $OutFile -Encoding utf8 }
    exit 0
}

# ------------------------- the parent: task + poll ---------------------------
$ErrorActionPreference = 'Stop'

$taskName = "cr_unelev_probe_$(-join ((48..57) + (97..122) | Get-Random -Count 8 | ForEach-Object { [char]$_ }))"
Remove-Item -LiteralPath $OutFile, "$OutFile.stdout", "$OutFile.stderr" -Force -ErrorAction SilentlyContinue

$pwsh = (Get-Command pwsh).Source
$self = $PSCommandPath
$action = New-ScheduledTaskAction -Execute $pwsh `
    -Argument ("-NoProfile -ExecutionPolicy Bypass -File `"" + $self + "`" -Child -RrecovExe `"" + $RrecovExe +
               "`" -OutFile `"" + $OutFile + "`" -Disk $Disk")
# RunLevel Limited + S4U: the UAC-filtered token, no interactive session needed
$principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME -LogonType S4U -RunLevel Limited
Register-ScheduledTask -TaskName $taskName -Action $action -Principal $principal -Force | Out-Null
try {
    Start-ScheduledTask -TaskName $taskName

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while (-not ((Test-Path -LiteralPath $OutFile) -and
                 ((Get-Content -LiteralPath $OutFile -ErrorAction SilentlyContinue) -match 'EXITCODE='))) {
        if ((Get-Date) -gt $deadline) {
            $info = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
            $state = if ($info) { $info.State } else { '<gone>' }
            $last = try { (Get-ScheduledTaskInfo -TaskName $taskName).LastTaskResult } catch { '<unknown>' }
            Write-Host "::error::the unelevated probe never completed (${TimeoutSec}s deadline; task state $state; last result $last)"
            exit 1
        }
        Start-Sleep -Milliseconds 500
    }
} finally {
    Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue
}

$out = (Get-Content -LiteralPath $OutFile) -join "`n"
$failed = $false
if ($out -match 'EXITCODE=4' -and $out -match 'administrator privilege required') {
    Write-Host "PASS unelevated scan --disk ${Disk}: exit 4, the ERR_ELEVATION_REQUIRED refusal"
} else {
    Write-Host "::error::unelevated scan --disk ${Disk} did not refuse; output: $out"
    $failed = $true
}
if ($out -match 'starting scan|\[transport\]|\[stage') {
    Write-Host '::error::the refusal must be the whole response - scan output leaked past the privilege gate'
    $failed = $true
}
if ($failed) { exit 1 }
exit 0
