# fetch-release.ps1 - resolve + verify a prebuilt release by the documented
# convention (docs/PREBUILT-ARTIFACTS.md). Used by every correctness workflow;
# NEVER builds from source and NEVER falls back to an older tag.
#
# Usage:
#   ./fetch-release.ps1 -Repo heicfast/rda-driver -OutDir driver `
#       [-Tag v0.2.2] [-Required rda.sys,rda.inf,install.ps1,uninstall.ps1,SHA256SUMS] `
#       [-FloorVersion v0.2.1]
#
# Behavior (the loud-failure contract):
#   - resolves the release: explicit -Tag, else /releases/latest (recorded in
#     <OutDir>\resolved-release.json for the run's env-manifest)
#   - downloads every asset
#   - verifies SHA256SUMS lines against downloaded assets by basename
#     (the driver repo's scripts/-prefix artifact is tolerated for matching;
#     unresolvable NON-required lines are REPORTED, never silently skipped)
#   - enforces every -Required asset is present and hash-verified
#   - when MANIFEST.json exists (combined releases): cross-checks manifest
#     entries (name + sha256) against the files
#   - enforces -FloorVersion (semver compare) when given
#   - any violation: non-zero exit naming the repo, tag, asset, remediation
param(
    [Parameter(Mandatory = $true)][string]$Repo,       # owner/name
    [Parameter(Mandatory = $true)][string]$OutDir,
    [string]$Tag = '',                                  # empty = latest
    [string[]]$Required = @(),
    [string]$FloorVersion = '',
    [string]$GithubToken = ''
)
$ErrorActionPreference = 'Stop'

# -Required tolerance: a comma-joined string binds as a ONE-element
# [string[]] (param conversion does not split) - normalize it to the array
# form; true arrays pass through untouched.
if ($Required.Count -eq 1 -and $Required[0] -match ',') {
    $Required = @($Required[0] -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
}

if (-not $GithubToken) { $GithubToken = $env:GITHUB_TOKEN }
$headers = @{}
if ($GithubToken) { $headers = @{ Authorization = "token $GithubToken" } }

function Fail($msg) {
    Write-Host "::error::$msg"
    Write-Host "PREBUILT-ARTIFACTS failure: $msg"
    exit 1
}

# ---- resolve the release (recorded once; consumers pin off this record) ----
if ($Tag) {
    $rel = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/tags/$Tag" -Headers $headers
} else {
    $rel = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest" -Headers $headers
}
$resolvedTag = $rel.tag_name
Write-Host "resolved release: $Repo @ $resolvedTag ($(if ($Tag) {'pinned'} else {'latest'}))"

# ---- floor enforcement (semver compare, suffix-tolerant: v0.2.1 / v0.4.1-parity) ----
if ($FloorVersion) {
    $cleanTag = ($FloorVersion -replace '^v', '') -replace '-.*$', ''
    $floorV = [version]$cleanTag
    $haveClean = ($resolvedTag -replace '^v', '') -replace '-.*$', ''
    try { $haveV = [version]$haveClean } catch { Fail "unparseable release tag '$resolvedTag' ($Repo)" }
    if ($haveV -lt $floorV) {
        Fail "resolved $Repo @ $resolvedTag is below the functional floor $FloorVersion (the bare-open error-87 class would classify Errored); publish/resolve a newer release"
    }
}

# ---- download every asset ----
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
if (-not $rel.assets -or $rel.assets.Count -eq 0) {
    Fail "release $Repo @ $resolvedTag has NO assets - the release workflow did not produce them; re-run it"
}
foreach ($asset in $rel.assets) {
    $dest = Join-Path $OutDir $asset.name
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $dest -Headers $headers
}
$names = @($rel.assets | ForEach-Object { $_.name })

# ---- SHA256SUMS verification (basename matching; scripts/-prefix tolerated) ----
$sumsPath = Join-Path $OutDir 'SHA256SUMS'
$verified = @{}   # basename -> $true (hash checked)
if (Test-Path $sumsPath) {
    foreach ($line in (Get-Content $sumsPath)) {
        $parts = $line -split '\s+', 2
        if ($parts.Count -ne 2) { continue }
        $want = $parts[0].Trim().TrimStart('*').ToUpperInvariant()
        $path = $parts[1].Trim()
        $base = [IO.Path]::GetFileName($path)
        $pp = Join-Path $OutDir $base
        if (-not (Test-Path $pp)) {
            if ($Required -contains $base) { Fail "SHA256SUMS lists required asset '$base' but it was not downloaded ($Repo @ $resolvedTag)" }
            Write-Host "  (sums line for absent non-required asset '$path' - reported, not skipped silently)"
            continue
        }
        $got = (Get-FileHash $pp -Algorithm SHA256).Hash.ToUpperInvariant()
        if ($got -ne $want) { Fail "SHA256 MISMATCH for '$base' ($Repo @ $resolvedTag): expected $want got $got" }
        $verified[$base] = $true
    }
} elseif ($Required -contains 'SHA256SUMS') {
    Fail "required asset SHA256SUMS missing from $Repo @ $resolvedTag (assets: $($names -join ', '))"
}

# ---- required-asset enforcement ----
# SHA256SUMS and MANIFEST.json are self-referential: they are computed OVER
# the asset set and cannot contain their own hash (the convention documents
# this - PREBUILT-ARTIFACTS.md section 3). Their EXISTENCE is still enforced
# (above / by the caller's -Required list); every OTHER required asset must
# carry a verified sums line.
$sumsExempt = @('SHA256SUMS', 'MANIFEST.json')
foreach ($req in $Required) {
    if ($sumsExempt -contains $req) { continue }
    if ($req -eq 'SHA256SUMS') { continue } # checked above
    if (-not ($names -contains $req)) {
        Fail "required asset '$req' MISSING from $Repo @ $resolvedTag (assets: $($names -join ', ')). For core releases predating the combined era this is expected - publish the first combined release (docs/RELEASE.md) and re-run"
    }
    if ((Test-Path $sumsPath) -and -not $verified.ContainsKey($req)) {
        Fail "required asset '$req' has no verified SHA256SUMS line ($Repo @ $resolvedTag)"
    }
}

# ---- MANIFEST.json cross-check (combined releases) ----
$manifestPath = Join-Path $OutDir 'MANIFEST.json'
if ($Required -contains 'MANIFEST.json') {
    if (-not (Test-Path $manifestPath)) {
        Fail "required asset MANIFEST.json missing from $Repo @ $resolvedTag (combined-release convention, docs/PREBUILT-ARTIFACTS.md section 3)"
    }
    $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
    foreach ($entry in $manifest.artifacts) {
        $pp = Join-Path $OutDir $entry.name
        if (-not (Test-Path $pp)) { Fail "MANIFEST lists '$($entry.name)' but the file is absent ($Repo @ $resolvedTag)" }
        $got = (Get-FileHash $pp -Algorithm SHA256).Hash.ToUpperInvariant()
        if ($got -ne ($entry.sha256.ToUpperInvariant())) {
            Fail "MANIFEST/asset SHA256 disagreement for '$($entry.name)' ($Repo @ $resolvedTag)"
        }
    }
    Write-Host ("manifest cross-check: {0} artifact(s) verified" -f @($manifest.artifacts).Count)
}

# ---- record the resolution (the env-manifest source) ----
$record = [ordered]@{
    repo        = $Repo
    tag         = $resolvedTag
    resolved_by = if ($Tag) { 'pinned' } else { 'latest' }
    assets      = $names
    verified    = @($verified.Keys)
    fetched_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$record | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $OutDir 'resolved-release.json')
Write-Host "PREBUILT-ARTIFACTS: $Repo @ $resolvedTag verified ($($verified.Count) hash-checked asset(s)) -> $OutDir"
exit 0
