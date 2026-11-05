#Requires -Version 5.1
<#
.SYNOPSIS
    devsign.ps1 — the development SELF-SIGNING tool for the whole product.

.DESCRIPTION
    One place, three jobs (docs/DEV-SIGNING.md is the design document):

      new-cert     Generate the ephemeral self-signed code-signing certificate
                   for this build (no secrets anywhere: nothing committed, a
                   fresh certificate per build). Exports the PFX (with an
                   ephemeral random password file, both gitignored build
                   outputs) + the public .cer + the thumbprint file into the
                   stage directory. Idempotent: an existing stage certificate
                   is reused unless -Force.

      sign         Embed-sign files with the staged certificate
                   (signtool sign /fd SHA256). Verifies the signature landed
                   (Get-AuthenticodeSignature: the signer's subject must be
                   the certificate's subject; store-independent, so it is
                   deterministic on machines that do not trust the cert).

      verify       Assert one file carries our development signature.

    The product is USER-MODE ONLY (consolidation session): the files this
    tool signs are the engine kit (cr_ui.dll, rrecov.exe) and the installer
    exe. There is NO kernel component, NO service, and NO certificate-store
    import anywhere in the product — nothing needs a trust-anchor decision
    on the target machine (a self-signed Authenticode signature is an
    integrity marker, not a load prerequisite).

    Production signing (a real CA certificate: EV/OSSign) REPLACES this
    tool's outputs, not its call sites — docs/DEV-SIGNING.md §4 documents
    the production path.

.PARAMETER StageDir
    The build stage directory (build\out\stage-<version>). The certificate
    lives beside the staged payloads: devsign.pfx, devsign-pfx-password.txt,
    devsign.cer, devsign-thumbprint.txt.

.EXAMPLE
    .\installer\devsign.ps1 -Action new-cert -StageDir build\out\stage-0.7.0
    .\installer\devsign.ps1 -Action sign -StageDir build\out\stage-0.7.0 -File build\out\stage-0.7.0\engine\cr_ui.dll
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('new-cert', 'sign', 'verify')]
    [string]$Action,

    [string]$StageDir = '',
    [string[]]$File = @(),
    [string]$CerPath = '',
    [string]$Thumbprint = '',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$CertSubject = 'CN=HeicFast Recovery Wizard Development Signing'

# ---------------------------------------------------------------------------
# signtool discovery: the Windows Kits walk (msquic's scripts/sign.ps1
# pattern): newest kit first, x64 preferred, x86 fallback.
# ---------------------------------------------------------------------------
function Find-Signtool {
    $kitRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    if (-not (Test-Path $kitRoot)) {
        # CI images may live elsewhere; ask the well-known alternate root.
        $kitRoot = 'C:\Program Files (x86)\Windows Kits\10\bin'
    }
    if (Test-Path $kitRoot) {
        foreach ($kit in (Get-ChildItem $kitRoot -Directory | Sort-Object Name -Descending)) {
            foreach ($arch in @('x64', 'x86')) {
                $t = Join-Path $kit.FullName "$arch\signtool.exe"
                if (Test-Path $t) { return $t }
            }
        }
    }
    $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw 'signtool.exe not found (install the Windows SDK - the Windows Kits\10\bin walk failed)'
}

function Assert-Stage {
    if (-not $StageDir -or -not (Test-Path $StageDir)) {
        throw "-StageDir missing or does not exist: $StageDir"
    }
    return (Resolve-Path $StageDir).Path
}

function Get-StageCert {
    # returns pfx path + password (reads the ephemeral password file)
    $pfx = Join-Path $StageDir 'devsign.pfx'
    $pwFile = Join-Path $StageDir 'devsign-pfx-password.txt'
    if (-not (Test-Path $pfx) -or -not (Test-Path $pwFile)) {
        throw "stage certificate missing ($pfx) - run -Action new-cert first"
    }
    $pw = (Get-Content $pwFile -Raw).Trim()
    if (-not $pw) { throw "stage certificate password file is empty: $pwFile" }
    [pscustomobject]@{ Pfx = $pfx; Password = $pw }
}

# ---------------------------------------------------------------------------
switch ($Action) {

    'new-cert' {
        $StageDir = Assert-Stage
        $cer  = Join-Path $StageDir 'devsign.cer'
        $pfx  = Join-Path $StageDir 'devsign.pfx'
        $pwFile = Join-Path $StageDir 'devsign-pfx-password.txt'
        $tpFile  = Join-Path $StageDir 'devsign-thumbprint.txt'
        if ((Test-Path $pfx) -and (Test-Path $cer) -and -not $Force) {
            Write-Host "[devsign] stage certificate exists - reusing $cer (use -Force to regenerate)"
            return
        }
        Write-Host "[devsign] generating the ephemeral development signing certificate: $CertSubject"
        # CurrentUser\My works elevated AND non-elevated (LocalMachine\My needs admin).
        $store = 'Cert:\CurrentUser\My'
        $cert = New-SelfSignedCertificate `
            -Type CodeSigningCert `
            -Subject $CertSubject `
            -KeyAlgorithm RSA -KeyLength 2048 -HashAlgorithm SHA256 `
            -NotAfter (Get-Date).AddYears(3) `
            -CertStoreLocation $store
        if (-not $cert) { throw 'New-SelfSignedCertificate returned nothing' }
        # export the public part (.cer - DER, certutil's native input form)
        Export-Certificate -Cert $cert -FilePath $cer | Out-Null
        # export the private part (PFX) with an ephemeral random password
        $pw = [Convert]::ToBase64String(
            (1..32 | ForEach-Object { Get-Random -Maximum 256 } | ForEach-Object { [byte]$_ }))
        $securePw = ConvertTo-SecureString -String $pw -AsPlainText -Force
        Export-PfxCertificate -Cert $cert -FilePath $pfx -Password $securePw | Out-Null
        Set-Content -Path $pwFile -Value $pw -NoNewline -Encoding ascii
        Set-Content -Path $tpFile -Value $cert.Thumbprint -NoNewline -Encoding ascii
        # the store copy is no longer needed (signing reads the PFX); remove it
        Remove-Item -Path (Join-Path $store ($cert.Thumbprint)) -ErrorAction SilentlyContinue
        Write-Host "[devsign] certificate staged:"
        Write-Host "  cer       = $cer"
        Write-Host "  pfx       = $pfx (ephemeral password in devsign-pfx-password.txt - build output, never committed)"
        Write-Host "  thumbprint= $($cert.Thumbprint)"
    }

    'sign' {
        $StageDir = Assert-Stage
        if (-not $File -or $File.Count -eq 0) { throw '-File is required for sign' }
        $cert = Get-StageCert
        $signtool = Find-Signtool
        foreach ($f in @($File)) {
            if (-not (Test-Path $f)) { throw "sign: file not found: $f" }
            Write-Host "[devsign] signing $(Split-Path $f -Leaf)"
            & $signtool sign /f $cert.Pfx /p $cert.Password /fd SHA256 /v $f
            if ($LASTEXITCODE -ne 0) { throw "signtool sign failed ($LASTEXITCODE): $f" }
        }
        # verify the signature landed (store-independent: assert the signer,
        # not the trust decision - this machine may not trust its own new cert)
        foreach ($f in @($File)) {
            $sig = Get-AuthenticodeSignature -FilePath $f
            if (-not $sig.SignerCertificate) { throw "verify: no signature present on $f" }
            if ($sig.SignerCertificate.Subject -ne $CertSubject) {
                throw "verify: signer subject mismatch on $f - '$($sig.SignerCertificate.Subject)'"
            }
            Write-Host "[devsign] verified: $f signed by $($sig.SignerCertificate.Subject) ($($sig.Status))"
        }
    }

    'verify' {
        if (-not $File) { throw '-File is required for verify' }
        if (-not (Test-Path $File)) { throw "verify: file not found: $File" }
        $sig = Get-AuthenticodeSignature -FilePath $File
        if (-not $sig.SignerCertificate) {
            throw "verify: NO signature present on $File - the dev-sign step did not run"
        }
        Write-Host "[devsign] $File carries a signature: signer='$($sig.SignerCertificate.Subject)' status=$($sig.Status)"
    }

}
