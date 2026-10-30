# Probe-Unelevated.ps1 - the unelevated-refusal probe (session-96: the
# direct-access elevation contract).
#
# The runner shell is elevated by construction, so the refusal state is
# created through a LIMITED-run scheduled task: the task executes as the
# runner user with RunLevel Limited - the UAC-filtered token, exactly the
# non-elevated state a desktop user's launched process runs under (what
# is_elevated() probes). The child (rrecov scan --disk N) must answer
# with the exit-4 ERR_ELEVATION_REQUIRED refusal as its WHOLE response
# (no scan output may leak past the gate).
#
# Two modes in one script (the task re-invokes this file with -Child):
#   parent (default): registers + starts the task, polls the output file,
#                     prints the verdict, exits 0/1
#   child  (-Child):  runs rrecov, captures stdout+stderr+exit code into
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

$ErrorActionPreference = 'Stop'

# ------------------------- the child: run + capture -------------------------
if ($Child) {
    $out = & $RrecovExe scan --disk $Disk 2>&1 | ForEach-Object { $_.ToString() }
    "EXITCODE=$LASTEXITCODE" | Set-Content -LiteralPath $OutFile -Encoding utf8
    $out | Add-Content -LiteralPath $OutFile -Encoding utf8
    exit 0
}

# ------------------------- the parent: task + poll ---------------------------
$taskName = "cr_unelev_probe_$([System.IO.Path]::GetRandomFileName() -replace '[^a-z0-9_]', '')"
Remove-Item -LiteralPath $OutFile -Force -ErrorAction SilentlyContinue

$self = $PSCommandPath
$action = New-ScheduledTaskAction -Execute 'pwsh.exe' `
    -Argument ("-NoProfile -File `"" + $self + "`" -Child -RrecovExe `"" + $RrecovExe +
               "`" -OutFile `"" + $OutFile + "`" -Disk $Disk")
# RunLevel Limited: the task runs with the UAC-filtered token of the
# runner user - the non-elevated state (TokenElevation FALSE)
$principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME -LogonType Interactive -RunLevel Limited
Register-ScheduledTask -TaskName $taskName -Action $action -Principal $principal -Force | Out-Null
try {
    Start-ScheduledTask -TaskName $taskName

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while (-not ((Test-Path -LiteralPath $OutFile) -and
                 ((Get-Content -LiteralPath $OutFile -ErrorAction SilentlyContinue) -match 'EXITCODE='))) {
        if ((Get-Date) -gt $deadline) {
            $info = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
            $state = if ($info) { $info.State } else { '<gone>' }
            Write-Host "::error::the unelevated probe never completed (${TimeoutSec}s deadline; task state $state)"
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
