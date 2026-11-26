# Assert-Clean.ps1 - the pre-commit hygiene gate for e2e results.
#
# Before ANY results tree is committed to the repo (and before every push from
# this suite), two invariants are asserted:
#   1. NO credential material: the results tree is grepped for the token
#      pattern (ghp_..., github_pat_..., AKIA...). The session token lives in
#      env/git-config only - a leak into a committed artifact is the one
#      unrecoverable mistake this suite can make.
#   2. NO placeholder residue: 'TODO', 'XXX', 'FIXME', 'NaN', 'undefined',
#      '待补充' must not appear in any .json/.md/.csv evidence file - a
#      placeholder in evidence means the capture was fabricated or broken.
#
# Exit 1 with the offending file list on any hit. Zero tolerance: hits are
# never auto-cleaned (masking); the producer must be fixed.
#
# Licensed under the MIT license. See LICENSE.

param(
    [Parameter(Mandatory = $true)] [string] $Root   # the tree to assert (results dir or repo)
)

$ErrorActionPreference = 'Stop'

$tokenPattern = 'ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16}'
$placeholderPattern = 'TODO|FIXME|XXX(?!X)|待补充|\bNaN\b|\bundefined\b'

$hits = @()
foreach ($f in @(Get-ChildItem -LiteralPath $Root -Recurse -File -Include *.json, *.md, *.csv, *.jsonl, *.txt, *.ps1, *.yml -ErrorAction SilentlyContinue)) {
    $text = $null
    try { $text = Get-Content -LiteralPath $f.FullName -Raw -ErrorAction Stop } catch { continue }
    if (-not $text) { continue }
    if ($text -match $tokenPattern) { $hits += "TOKEN: $($f.FullName)" }
    if ($text -match $placeholderPattern) { $hits += "PLACEHOLDER: $($f.FullName)" }
}

if ($hits.Count -gt 0) {
    Write-Host '::error::Assert-Clean FAILED - the tree is not committable:' -ForegroundColor Red
    $hits | Select-Object -First 40 | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "Assert-Clean: $Root clean (no token material, no placeholder residue)"
exit 0
