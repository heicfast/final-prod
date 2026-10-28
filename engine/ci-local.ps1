# ci-local.ps1 - the Windows local validation entry point (docs/CI-LOCAL.md).
#
# Runs every check the CI's local-mirror + windows-release-build jobs run, in
# the same order, with the same strictness, BEFORE a push (the
# never-push-a-red-commit rule, docs/CONTRIBUTING.md).
#
# Usage:
#   .\ci-local.ps1            # the full default gate
#   .\ci-local.ps1 -Fast      # format/lint/secrets/commits only (the
#                              # pre-commit subset - seconds, not minutes)
#
# Exit code: 0 = green (push), 1 = red (fix before pushing; each failure
# prints the exact files/issues and the fix command).

param([switch]$Fast)

$ErrorActionPreference = 'Continue'
Set-Location (Join-Path $PSScriptRoot '.')
$script:Failed = 0

function Fail($msg) { Write-Host "[FAIL] $msg" -ForegroundColor Red; $script:Failed = 1 }
function Ok($msg)   { Write-Host "[ ok ] $msg" -ForegroundColor Green }
function Info($msg) { Write-Host "[info] $msg" -ForegroundColor Yellow }

# ---------------------------------------------------------------- 1. format
Write-Host "== [1/10] clang-format check =="
$cf = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $cf) {
    $pip = Get-Command pip -ErrorAction SilentlyContinue
    # PINNED to the CI gate's version (checks.yml) - an unpinned latest
    # drifted from the lanes' 18.x tooling and kept the gate red on
    # 18-formatted code; the pin and the tree move together
    if ($pip) { Info "installing clang-format 18.1.8 via pip..."; pip install --quiet --disable-pip-version-check clang-format==18.1.8 2>$null | Out-Null; $cf = Get-Command clang-format -ErrorAction SilentlyContinue }
}
if ($cf) {
    $files = Get-ChildItem -Recurse -Include *.h,*.hpp,*.cpp,*.c -Path src,include,tests | ForEach-Object FullName
    $bad = $files | ForEach-Object { & clang-format --dry-run -Werror $_ 2>&1 } | Select-String 'error:' | Select-Object -First 20
    if ($bad) {
        Fail "files need formatting (first 20 violations):"
        $bad | ForEach-Object { Write-Host "  $_" }
        Write-Host "  fix: clang-format -i (all files under src/ include/ tests/)"
    } else { Ok "clang-format clean ($($files.Count) files)" }
} else {
    Fail "clang-format not installed. fix: pip install clang-format"
}

# ------------------------------------------------------------------ 2. lint
Write-Host "== [2/10] dead-surface + architecture lints =="
$n41 = Get-ChildItem -Recurse -Include *.cpp,*.h,*.c -Path src,include,tests,tools,bench,fuzz -ErrorAction SilentlyContinue |
    Select-String -Pattern 'RdaClient::','class RdaClient','volume_letter_to_disk' | Select-Object -First 5
if ($n41) { Fail "deleted dead-surface symbols found (N-041): $n41" } else { Ok "N-041 dead-surface lint clean" }

$drv = Get-ChildItem -Recurse -Include *.cpp,*.h,*.c -Path src,include -ErrorAction SilentlyContinue |
    Select-String -Pattern 'rda_available\s*\(','win_pe_environment\s*\(','open_harddisk_volume\s*\(','prefer_rda' |
    Where-Object { $_.Line -notmatch '^\s*(/\*|\*|//)' } | Select-Object -First 5
if ($drv) { Fail "removed fallback primitives in production code (driver-only architecture): $drv" }
else { Ok "driver-only architecture lint clean" }

Info "static analysis: not configured in this repo yet - tracked, not silenced (docs/CI-LOCAL.md section 4)"

if ($Fast) {
    Write-Host "( -Fast: skipping build/tests/schema/pins - run the full gate before pushing )"
    if ($script:Failed -eq 0) { Ok "FAST GATE: GREEN" } else { Fail "FAST GATE: RED" }
    exit $script:Failed
}

# ------------------------------------------------- 3. Werror build + tests
Write-Host "== [3/10] warnings-as-errors build (MSVC /W4 /WX; global in CMakeLists) =="
$env:VCPKG_ROOT = $env:VCPKG_INSTALLATION_ROOT
if (-not $env:VCPKG_ROOT) { $env:VCPKG_ROOT = "C:/vcpkg" }
cmake --preset win-x64-release
if ($LASTEXITCODE) { Fail "configure failed (win-x64-release preset)"; exit 1 }
cmake --build --preset win-x64-release
if ($LASTEXITCODE) { Fail "build failed (MSVC /W4 /WX - the errors above are the fix list)"; exit 1 }
Ok "win-x64-release build clean"

Write-Host "== [4/10] unit + contract tests (full ctest incl. cr_ui ABI + win_syntax_check) =="
ctest --preset win-x64-release -C Release --output-on-failure
if ($LASTEXITCODE) { Fail "ctest red (failures above)" } else { Ok "ctest green" }

# -------------------------------------------------- 5. Win7 posture gate
Write-Host "== [5/10] Win7 posture gate (stamp + import allowlist) =="
foreach ($pe in @('build\win-x64-release\src\app\Release\rrecov.exe',
                  'build\win-x64-release\tests\Release\cr_tests.exe',
                  'build\win-x64-release\src\ui\Release\cr_ui.dll')) {
    python tools/stamp_osversion_um.py $pe
    if ($LASTEXITCODE) { Fail "stamp failed: $pe"; continue }
    python tools/verify_pe_um.py $pe --allowlist tools/ref_imports_um.json
    if ($LASTEXITCODE) { Fail "import allowlist failed: $pe" }
}
if ($script:Failed -eq 0) { Ok "Win7 posture gate clean" }

# --------------------------------------------------------- 6. dependency pin
Write-Host "== [6/10] dependency pinning (vcpkg manifest mode) =="
if (Test-Path vcpkg.json) { Ok "vcpkg.json manifest present (manifest mode = the repo's pinning policy)" }
else { Fail "vcpkg.json missing" }

# ------------------------------------------------------------ 7. doc links
Write-Host "== [7/10] documentation link check =="
$linkBad = @()
Get-ChildItem docs -Filter *.md -Recurse | ForEach-Object {
    $dir = $_.DirectoryName
    foreach ($m in [regex]::Matches((Get-Content $_.FullName -Raw), '\]\(([^)h][^)]*\.md[^)]*)\)')) {
        $link = $m.Groups[1].Value -replace '#.*$',''
        if ($link -and -not (Test-Path (Join-Path $dir $link))) { $linkBad += "$($_.Name) -> $link" }
    }
}
if ($linkBad.Count) { Fail "broken relative doc links:"; $linkBad | ForEach-Object { Write-Host "  $_" } }
else { Ok "doc links resolve" }

# ----------------------------------------------------------- 8. secrets scan
Write-Host "== [8/10] secrets scan (working diff vs origin/main) =="
$base = git merge-base HEAD origin/main 2>$null; if (-not $base) { $base = 'HEAD' }
$diff = git diff $base 2>$null
$secretHits = $diff | Select-String -Pattern 'ghp_[A-Za-z0-9]{20,}','github_pat_[A-Za-z0-9_]{20,}','AKIA[0-9A-Z]{16}','-----BEGIN [A-Z ]*PRIVATE KEY-----' | Select-Object -First 5
if ($secretHits) { Fail "SECRETS in the diff:"; $secretHits | ForEach-Object { Write-Host "  $_" }; Write-Host "  fix: remove + rotate (docs/SECURITY.md)" }
else { Ok "no secrets in the diff" }

# ------------------------------------------------------ 9. commit message
Write-Host "== [9/10] commit message convention (type(scope): lowercase ...) =="
$msgBad = @()
git log --format='%H %s' origin/main..HEAD 2>$null | ForEach-Object {
    $rev, $subj = $_ -split ' ', 2
    if ($subj -notmatch '^(feat|fix|repair|docs|worklog|bench|ci|style|audit|merge|test|refactor|perf|chore|build|revert)(\([A-Za-z0-9._-]+\))?!?: .+') {
        $msgBad += "  non-conforming: $rev $subj"
    }
}
if ($msgBad.Count) { Fail "fix the commit subjects above (docs/CONTRIBUTING.md)"; $msgBad | ForEach-Object { Write-Host $_ } }
else { Ok "commit messages conform" }

# ------------------------------------------------------- 10. hook presence
Write-Host "== [10/10] git hooks path =="
if ((git config core.hooksPath 2>$null) -eq '.githooks') { Ok "core.hooksPath = .githooks (pre-commit/pre-push active)" }
else { Info "hooks not installed (recommended): git config core.hooksPath .githooks" }

Write-Host ""
if ($script:Failed -eq 0) { Ok "LOCAL GATE: GREEN - ready to push"; exit 0 }
else { Fail "LOCAL GATE: RED - fix the above before pushing (never push a red commit)"; exit 1 }
