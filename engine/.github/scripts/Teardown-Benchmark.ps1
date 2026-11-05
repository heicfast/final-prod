# Teardown-Benchmark.ps1 - the always-run teardown for the test-benchmark lane.
#
# Contracts carried over (each from a recorded failure):
#   - the VHDs are detached via diskpart (select vdisk + detach - the same
#     primitive that attached them) and deleted; best-effort, recorded
#   - everything lands in evidence/99-teardown/ so a teardown problem is
#     diagnosable from the artifact alone
#
# session-96: the driver uninstall and certificate cleanup are REMOVED (the
# direct raw I/O replacement ships no driver, no service, and imports no
# certificates); the teardown is the VHD detach + delete only.
param(
    [Parameter(Mandatory = $true)] [string] $ReleaseDir,     # fetched release assets (kept for the caller's shape)
    [string] $DiskStatePath = '',                            # <workroot>\disk-state.json (absent = no disks were created)
    [string] $EvidenceDir = ''
)

$ErrorActionPreference = 'Continue'   # collect every teardown result; verdict at the end
if ($EvidenceDir) { New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null }
$log = if ($EvidenceDir) { Join-Path $EvidenceDir 'teardown.log' } else { "$env:TEMP\cr-teardown.log" }
function Log($msg) {
    $line = "[{0:o}] {1}" -f (Get-Date).ToUniversalTime(), $msg
    Write-Host $msg
    $line | Add-Content -Path $log -Encoding utf8
}
$exitCode = 0

# ---- 1. detach + delete the VHDs (best-effort, recorded) ---------------------------
try {
    if ($DiskStatePath -and (Test-Path $DiskStatePath)) {
        $state = Get-Content -LiteralPath $DiskStatePath -Raw | ConvertFrom-Json
        foreach ($d in $state.disks) {
            if (-not (Test-Path $d.vhd)) { Log "disk $($d.id): vhd already gone"; continue }
            $tmp = [System.IO.Path]::GetTempFileName()
            "select vdisk file=`"$($d.vhd)`"`r`ndetach vdisk" | Set-Content -Path $tmp -Encoding ascii
            $out = diskpart /s $tmp 2>&1
            $rc = $LASTEXITCODE
            Remove-Item $tmp -Force -ErrorAction SilentlyContinue
            if ($rc -ne 0) { Log "disk $($d.id): diskpart detach exit $rc (recorded) - output:"; $out | Select-Object -Last 6 | ForEach-Object { Log "  $_" } }
            else { Log "disk $($d.id): detached" }
            Start-Sleep -Seconds 2
            Remove-Item -LiteralPath $d.vhd -Force -ErrorAction SilentlyContinue
            if (Test-Path $d.vhd) { Log "disk $($d.id): vhd file still present after delete attempt (locked? recorded)" }
            else { Log "disk $($d.id): vhd file deleted" }
        }
    } else {
        Log 'no disk-state.json - no VHDs to detach (setup did not reach disk creation)'
    }
} catch {
    Log "vhd teardown note: $($_.Exception.Message)"
}

Log "teardown complete (exit $exitCode)"
exit $exitCode
