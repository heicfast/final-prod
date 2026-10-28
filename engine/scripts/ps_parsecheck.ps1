# ps_parsecheck.ps1 - parse-only validation of every bench PowerShell script.
# Catches syntax errors BEFORE a 90-job matrix dispatches (the F4 lesson:
# script defects surfaced only at minute 4 of each Windows job).
# Runs on any runner with pwsh (ubuntu images ship it).
$files = @(
  "bench/volume/volume.ps1", "bench/seed/generate_seed.ps1",
  "bench/scenarios/apply-scenario.ps1", "bench/ours/run-ours.ps1",
  "bench/ours/run-behavior.ps1", "bench/ours/verify-driver.ps1",
  "bench/ours/run-alignbench.ps1", "scripts/bench_seed.ps1",
  "bench/og/run-og.ps1", "bench/og/og-driver.ps1",
  "installer/osgate.ps1", "installer/build-installer.ps1", "installer/run-gates.ps1")
$failed = 0
foreach ($f in $files) {
  if (-not (Test-Path $f)) { Write-Host "MISSING  $f"; $failed++; continue }
  $errs = $null
  [void][System.Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $PWD $f), [ref]$null, [ref]$errs)
  if ($errs.Count) {
    Write-Host "PARSE FAIL ${f}: $($errs[0].Extent.StartLineNumber): $($errs[0].Message)"
    $failed++
  } else {
    Write-Host "PARSE OK   $f"
  }
}
if ($failed) { exit 1 }
Write-Host "all $($files.Count) bench scripts parse clean"
