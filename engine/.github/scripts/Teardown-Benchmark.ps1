# Teardown-Benchmark.ps1 - the always-run teardown for the test-benchmark lane.
#
# Contracts carried over (each from a recorded failure):
#   - the driver is uninstalled with the RELEASE'S OWN uninstall.ps1 (never
#     raw sc.exe with a hardcoded image name - M-069: the versioned layout
#     ships rda10.sys, not rda.sys, and a fixed-name teardown missed it)
#   - the uninstall CONTRACT is asserted after it runs (M-061): service
#     registration GONE and the driver image file GONE (sweep
#     <ServiceName><digits>.sys leftovers); a leftover kernel service image
#     survives until reboot and poisons the next install
#   - the VHDs are detached via diskpart (select vdisk + detach - the same
#     primitive that attached them) and deleted; best-effort, recorded
#   - the imported dev certificate is removed from Root/TrustedPublisher
#     (best-effort, recorded)
#   - everything lands in evidence/99-teardown/ so a teardown problem is
#     diagnosable from the artifact alone
param(
    [Parameter(Mandatory = $true)] [string] $ReleaseDir,     # fetched release assets (uninstall.ps1)
    [Parameter(Mandatory = $true)] [string] $ServiceName,    # 'rda'
    [string] $DiskStatePath = '',                            # <WorkRoot>\disk-state.json (absent = no disks were created)
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

# ---- 1. driver uninstall (the release's own script, contract-asserted) -----------
try {
    $svc = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if ($svc) {
        $uninstallPs1 = Join-Path $ReleaseDir 'uninstall.ps1'
        if (-not (Test-Path $uninstallPs1)) {
            Log "::error::uninstall.ps1 missing from the release - cannot honor the M-061 teardown contract"
            $exitCode = 1
        } else {
            $out = powershell -NoProfile -ExecutionPolicy Bypass -File $uninstallPs1 -ServiceName $ServiceName 2>&1
            $rc = $LASTEXITCODE
            $out | Add-Content -Path $log -Encoding utf8
            Log "uninstall.ps1 exit: $rc (0 clean; 1 not elevated; 2 stop error; 3 no STOPPED in 5s; 4 delete error; 5 still registered; 6 locked image)"
            if ($rc -ne 0) { Log "::error::uninstall.ps1 failed (exit $rc)"; $exitCode = 1 }
            # post-state assertions (M-061): registration gone + image gone (versioned sweep)
            if (Get-Service -Name $ServiceName -ErrorAction SilentlyContinue) {
                Log "::error::service '$ServiceName' STILL REGISTERED after uninstall"
                $exitCode = 1
            }
            $leftovers = Get-ChildItem "$env:SystemRoot\System32\drivers" -Filter "$ServiceName*.sys" -ErrorAction SilentlyContinue
            if ($leftovers) {
                Log "::error::driver image leftovers after uninstall: $($leftovers.Name -join ', ')"
                $exitCode = 1
            } else {
                Log 'teardown verified: service gone + image gone'
            }
        }
    } else {
        Log "service '$ServiceName' not present - driver uninstall skipped"
    }
} catch {
    Log "::error::driver teardown threw: $($_.Exception.Message)"
    $exitCode = 1
}

# ---- 2. remove the imported dev certificate (best-effort, recorded) ---------------
try {
    foreach ($store in 'Root', 'TrustedPublisher') {
        $found = Get-ChildItem "Cert:\LocalMachine\$store" -ErrorAction SilentlyContinue |
            Where-Object { $_.Subject -match 'RRECOV-DEV-SIGN' }
        foreach ($c in $found) {
            Remove-Item "Cert:\LocalMachine\$store\$($c.Thumbprint)" -ErrorAction SilentlyContinue
            Log "removed cert $($c.Thumbprint) from LocalMachine\$store"
        }
    }
} catch {
    Log "cert cleanup note: $($_.Exception.Message)"
}

# ---- 3. detach + delete the VHDs (best-effort, recorded) ---------------------------
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
