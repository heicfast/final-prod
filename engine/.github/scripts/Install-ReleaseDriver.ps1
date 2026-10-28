# Install-ReleaseDriver.ps1 - establish trust for + install + verify the
# TEST-DEVELOPMENT-SIGNED rda.sys from a core_recovery combined release.
#
# Contract carried over verbatim from the proven composite action (and the
# findings that shaped it):
#   - the release's rda.sys arrives ALREADY signed with the repo's
#     persistent dev certificate; rda-test-sign.cer (the public cert) is
#     imported into LocalMachine\Root AND LocalMachine\TrustedPublisher
#     BEFORE install - that is the entire trust story on HVCI-off runners
#     (windows-2022 = the execution-validated class; PREBUILT-ARTIFACTS.md
#     section 2). No bcdedit/testsigning, no reboot.
#   - the SIGNATURE is verified (Get-AuthenticodeSignature -> Valid) after
#     the trust import and BEFORE any kernel load attempt - a signature
#     problem fails here in seconds with a clear message instead of inside
#     the service start with event-log archaeology.
#   - the release's own install.ps1 is the ONLY install path (S-036: a
#     broken documented installer must never hide behind a manual sc.exe
#     fallback) - and it is AST-parse-gated first (the v0.2.3 lesson).
#   - install.ps1 STARTS THE SERVICE ITSELF (v0.2.4): the start check is
#     IDEMPOTENT - query RUNNING first; "already running" is the
#     installer's own success shape (the M-068 double-start/1056 lesson);
#     any other start failure stays fatal and dumps the System event log.
#   - the service config is ASSERTED after install (M-060: group
#     'Boot File System' + the four Parameters values) - a divergence is a
#     silently misconfigured driver.
#   - the driver-status truth comes from the product itself: rrecov driver
#     --json must report state=ready with every facet green (present /
#     loaded / started / authorized / healthy) and exit 0.
#
# Everything lands in $EvidenceDir: install log, sc qc output, config
# read-back, the driver-status JSON, signature details, event-log excerpt
# on failure.
#
# Licensed under the MIT license. See LICENSE.
param(
    [Parameter(Mandatory = $true)] [string] $ReleaseDir,   # fetched release assets (rda.sys, install.ps1, uninstall.ps1, rda-test-sign.cer)
    [Parameter(Mandatory = $true)] [string] $ServiceName,  # 'rda'
    [Parameter(Mandatory = $true)] [string] $RrecovExe,
    [Parameter(Mandatory = $true)] [string] $EvidenceDir
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }
New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null
$installLog = Join-Path $EvidenceDir 'install.log'

function Log($msg) {
    $line = "[{0:o}] {1}" -f (Get-Date).ToUniversalTime(), $msg
    Write-Host $msg
    $line | Add-Content -Path $installLog -Encoding utf8
}

# ---- 0. elevation assertion (the whole flow is administrative by design) ----
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Log '::error::this script must run elevated (administrator) - the driver install, cert store writes and disk staging are all administrative operations'
    exit 1
}
Log "elevated as: $([Security.Principal.WindowsIdentity]::GetCurrent().Name)"

# ---- 1. trust: import the public dev certificate --------------------------------
$cerPath = Join-Path $ReleaseDir 'rda-test-sign.cer'
if (-not (Test-Path $cerPath)) { Log "::error::missing $cerPath - the release must carry the public dev-sign cert (new-pipeline releases only)"; exit 1 }
$cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($cerPath)
Log "dev certificate: $($cert.Subject) (thumbprint $($cert.Thumbprint), valid $($cert.NotBefore.ToString('u')) -> $($cert.NotAfter.ToString('u')))"
if ([DateTime]::UtcNow -lt $cert.NotBefore.ToUniversalTime() -or [DateTime]::UtcNow -gt $cert.NotAfter.ToUniversalTime()) {
    Log '::error::the dev certificate is NOT CURRENTLY VALID - re-generate and re-release (see docs/DRIVER-RELEASE.md section 5)'
    exit 1
}
$imported = @()
foreach ($store in 'Root', 'TrustedPublisher') {
    $inStore = Get-ChildItem "Cert:\LocalMachine\$store" -ErrorAction SilentlyContinue |
        Where-Object { $_.Thumbprint -eq $cert.Thumbprint }
    if (-not $inStore) {
        Import-Certificate -FilePath $cerPath -CertStoreLocation "Cert:\LocalMachine\$store" | Out-Null
        $imported += $store
    }
}
Log "certificate trusted: LocalMachine\Root + LocalMachine\TrustedPublisher (imported: $($imported -join ', '))"

# ---- 2. signature verification BEFORE any kernel load ----------------------------
$sysPath = Join-Path $ReleaseDir 'rda.sys'
$sig = Get-AuthenticodeSignature -FilePath $sysPath
$sig | Select-Object Status, StatusMessage, @{n = 'Signer'; e = { $_.SignerCertificate.Subject } }, @{n = 'NotAfter'; e = { $_.SignerCertificate.NotAfter } }, @{n = 'TimeStamper'; e = { $_.TimeStamperCertificate.Subject } } |
    ConvertTo-Json -Depth 3 | Set-Content (Join-Path $EvidenceDir 'signature.json') -Encoding utf8
if ($sig.Status -ne 'Valid') {
    Log "::error::rda.sys signature is '$($sig.Status)' after trust import: $($sig.StatusMessage) - the release shipped a signature this machine cannot validate"
    exit 1
}
if ($sig.SignerCertificate.Subject -notmatch 'RRECOV-DEV-SIGN') {
    Log "::error::rda.sys is signed by an unexpected subject: $($sig.SignerCertificate.Subject) (expected CN=RRECOV-DEV-SIGN)"
    exit 1
}
Log "signature VALID (signer: $($sig.SignerCertificate.Subject))"

# ---- 3. AST parse-gate the installers (the S-036 lesson) --------------------------
foreach ($s in @('install.ps1', 'uninstall.ps1')) {
    $p = Join-Path $ReleaseDir $s
    if (-not (Test-Path $p)) { Log "::error::missing $p in the release"; exit 1 }
    $errs = $null
    [System.Management.Automation.Language.Parser]::ParseFile($p, [ref]$null, [ref]$errs) | Out-Null
    if ($errs -and $errs.Count -gt 0) {
        Log "::error::PARSE ERROR in $s (the release shipped a broken script):"
        $errs | Select-Object -First 5 | ForEach-Object { Log ("  line {0}: {1}" -f $_.Extent.StartLineNumber, $_.Message) }
        exit 1
    }
}
Log 'install.ps1 + uninstall.ps1 parse clean (PS AST gate)'

# ---- 4. the release's own installer (the only path) -------------------------------
$absSys = (Resolve-Path $sysPath).Path
# exit codes (v0.2.3+ contract): 0 installed+started; 1 not elevated; 2 name exists;
# 3 copy failed; 4 CreateService SCM error; 5 Parameters read-back mismatch;
# 6 StartService failed (code-signing rejection carries the testsigning guidance);
# 7 not RUNNING; 8/9 upgrade path (v0.2.4)
# NOTE: the installer's stderr is captured WITHOUT the pwsh EAP=Stop throw
# trap (a redirected native stderr line would throw before the exit code
# could be diagnosed) - EAP goes to Continue around the invocation
$ErrorActionPreference = 'Continue'
$installerOutput = powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $ReleaseDir 'install.ps1') `
    -ServiceName $ServiceName -Source $absSys -IoctlBase 0 2>&1
$installExit = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
$installerOutput | Add-Content -Path $installLog -Encoding utf8
Log "install.ps1 exit: $installExit"
if ($installExit -ne 0) {
    Log "::error::install.ps1 failed (exit $installExit - see the exit-code map in the header of this script); output above"
    exit 1
}

# ---- 5. service config assertions (M-060: both values and group) ------------------
$qc = sc.exe qc $ServiceName 2>&1
$qc | Add-Content -Path $installLog -Encoding utf8
if (-not ($qc | Select-String 'Boot File System')) {
    Log "::error::service group is not 'Boot File System' (sc qc output above)"
    exit 1
}
$params = Get-ItemProperty "HKLM:\SYSTEM\CurrentControlSet\Services\$ServiceName\Parameters" -ErrorAction Stop
$params | ConvertTo-Json | Set-Content (Join-Path $EvidenceDir 'service-parameters.json') -Encoding utf8
if ($params.IoctlBase -ne 0 -or $params.MaxDisks -ne 200 -or $params.DebugLevel -ne 1 -or $params.SessionsEnabled -ne 1) {
    Log "::error::Parameters key incomplete: IoctlBase=$($params.IoctlBase) MaxDisks=$($params.MaxDisks) DebugLevel=$($params.DebugLevel) SessionsEnabled=$($params.SessionsEnabled)"
    exit 1
}
Log "service config verified: group 'Boot File System', IoctlBase=0 MaxDisks=200 DebugLevel=1 SessionsEnabled=1"

# ---- 6. idempotent start verification (M-068: install.ps1 starts it itself) -------
# (EAP=Continue around the sc.exe probes: a redirected native stderr line
# under EAP=Stop throws - the same trap class)
$ErrorActionPreference = 'Continue'
$running = sc.exe query $ServiceName | Select-String 'RUNNING'
if (-not $running) {
    sc.exe start $ServiceName
    if ($LASTEXITCODE -ne 0) {
        Log "start failed (exit $LASTEXITCODE) - System event log (last 30):"
        Get-WinEvent -LogName System -MaxEvents 30 | Format-List TimeCreated, ProviderName, Id, Message |
            Out-String | Add-Content -Path $installLog -Encoding utf8
        exit 1
    }
} else {
    Log "service '$ServiceName' already RUNNING (the v0.2.4 installer starts it) - start step skipped (M-068 idempotent-start contract)"
}

# ---- 7. the product's own driver-status truth (CLI surface) -----------------------
$statusJson = & $RrecovExe driver --json 2>&1
$statusExit = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
$statusJson | Set-Content (Join-Path $EvidenceDir 'driver-status.json') -Encoding utf8
Log "rrecov driver --json exit: $statusExit"
Log "driver status: $statusJson"
$ok = $false
try {
    $st = $statusJson | Where-Object { $_ -match '^\s*\{' } | Select-Object -Last 1 | ConvertFrom-Json
    if ($st) {
        $facets = @($st.present, $st.loaded, $st.started, $st.authorized, $st.healthy)
        $ok = ($st.state -eq 'ready') -and (($facets | Where-Object { $_ -ne 'yes' }).Count -eq 0)
        $st | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $EvidenceDir 'driver-status-parsed.json') -Encoding utf8
    }
} catch { }
if (-not $ok -or $statusExit -ne 0) {
    Log '::error::driver NOT ready per rrecov driver --json (expected exit 0, state=ready, every facet yes) - see driver-status.json'
    exit 1
}
Log 'DRIVER READY: state=ready, present/loaded/started/authorized/healthy all yes'

# ---- 8. environment record ---------------------------------------------------------
$sysVersion = (Get-Item $sysPath).VersionInfo
[ordered]@{
    service = $ServiceName
    driver_sys = $absSys
    driver_sha256 = (Get-FileHash $sysPath -Algorithm SHA256).Hash
    driver_file_version = $sysVersion.FileVersion
    driver_product_version = $sysVersion.ProductVersion
    signer = $sig.SignerCertificate.Subject
    cert_thumbprint = $cert.Thumbprint
    installed_utc = (Get-Date).ToUniversalTime().ToString('o')
} | ConvertTo-Json | Set-Content (Join-Path $EvidenceDir 'driver-env.json') -Encoding utf8

Log 'driver install complete + verified'
exit 0
