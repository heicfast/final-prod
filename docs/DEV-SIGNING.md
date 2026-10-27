# DEV-SIGNING — development test signing for the mandatory-driver product

> Session: 2026-10-27 (mandatory-driver + development-signing session).
> Authority: the user directive ("driver is mandatory; research and implement
> test development signing on both driver and installer"), the Microsoft
> driver-signing policy documents, and the primary-source research recorded
> below. The production-signing path is §5.

## 1. What is signed, and with what

| Artifact | Signature | Applied by |
|---|---|---|
| `driver\bin\x64\Release\rda.sys` | embedded (Authenticode, SHA-256) | `installer/devsign.ps1` via `build.ps1 Invoke-Driver` — **after** the WDK build and its PE gates |
| staged `cr_ui.dll` + `rrecov.exe` (the engine kit) | embedded | `installer/devsign.ps1` via `build.ps1 Invoke-Engine` |
| `RecoveryWizard-<version>-setup.exe` (the installer) | embedded | `installer/devsign.ps1` via `build.ps1 Invoke-Installer` |
| `RecoveryWizard-<version>-testsigning.cer` | — | the PUBLIC half of the certificate, published as a release asset |
| the Tauri application bundle (inner app-setup + `recovery-wizard-ui.exe`) | **not signed** | the production-signing gap (§5); the bundle is produced inside `pnpm tauri build` and its signature awaits Tauri's bundler signing config with a real certificate |

The certificate is **ephemeral and self-signed**: `New-SelfSignedCertificate
-Type CodeSigningCert`, subject `CN=HeicFast Recovery Wizard Development
Signing`, generated per build into `build\out\stage-<version>\`. Nothing
private is committed, and nothing private ships: the PFX
(`devsign.pfx`) and its ephemeral password file are gitignored build
outputs that exist only for the duration of the build. Every rebuild
carries a fresh certificate; that is the deliberate development posture —
a development signature proves the code *came from our build*, not that
the publisher is trustable, and it makes load-time verification possible
on machines that choose to trust it (below).

The WDK project's `SignMode` stays **Off**: the msbuild output remains
unsigned and `driver-ci.yml` keeps asserting that (the signature is a
*packaging* step, never a build step — the lane proves the signing
tooling with its own smoke leg). The engine's Win7 import gates run
**before** signing (a signature changes bytes, not imports).

## 2. Why the driver loads (the policy matrix)

A kernel-mode driver loads when Windows code integrity accepts its
signature. For a self-signed development certificate the acceptance
conditions are:

| Machine state | Test-signed driver loads? | Why |
|---|---|---|
| Test signing ON (`bcdedit /set testsigning on` + reboot) | **YES** | test-signing mode accepts any embed signature — no root trust needed |
| Secure Boot OFF, test signing OFF, cert in `LocalMachine\Root` + `TrustedPublisher` | **YES** | classic trusted-publisher acceptance |
| Secure Boot OFF, stores empty | no | the signature chains to nothing the machine trusts |
| Secure Boot ON, test signing OFF | **no** | Secure Boot requires a Microsoft signature (WHQL/attestation); local test certs are insufficient and test signing is blocked |

Primary sources for this table (2026-10-27 research):

- **GitHub-hosted Windows runners have test signing ON**: every Windows
  Packer template in `actions/runner-images` runs
  `bcdedit.exe /set TESTSIGNING ON` at image build time
  (`images/windows/templates/build.windows-2022.pkr.hcl`), and
  `images/windows/scripts/tests/WindowsFeatures.Tests.ps1` asserts
  `testsigning Yes` as an image validation gate (since PR #4098,
  September 2021). The older "hosted runners cannot load test drivers"
  folklore from issue #1497 (2020) is stale. Microsoft's `msquic` loads a
  test-signed kernel driver on hosted `windows-2022` today
  (`scripts/sign.ps1`: `signtool sign /f cert.pfx /fd SHA256 *.sys`).
- The cert-in-stores path is Microsoft's own documented test-certificate
  installation procedure ("Installing Test Certificates",
  windows-driver-docs): import the signing certificate into the Trusted
  Root and Trusted Publishers stores.

## 3. How the product carries it

- `build.ps1` (all lanes) generates/reuses the stage certificate and signs
  the payloads (§1). The staged driver dir carries the public
  `rda-testcert.cer` + `rda-testcert-thumbprint.txt`.
- **The installer** (`installer/recoverywizard.nsi`, `SEC_DRIVER`, a
  `SectionIn RO` required component) imports the certificate into
  `LocalMachine` `Root` + `TrustedPublisher` (`certutil -addstore`) BEFORE
  creating the service, records the thumbprint in the product registry key
  (`DriverCertThumbprint`), and the **uninstaller removes the certificate
  from both stores by thumbprint** — no orphaned trust anchors. The
  install facts file records both `certutil` exit codes and the thumbprint.
- **The portable zip** stages the signed driver + certificate with the
  truthful label: the product REQUIRES the running driver; the staged
  scripts install it on demand.
- **CI validation** (`validation-evidence.yml`) proves the whole chain on
  every dispatch: the T14 lane development-signs, imports, installs and
  STARTS the driver on the hosted runner (test signing ON) before the
  measurement harness runs — every measurement then rides the REAL
  `\\.\RDA` transport; the T15 matrix asserts the installed service is
  `Running`, the certificate is present in both stores, the staged
  `rda.sys` carries our signature, and that uninstall removes the
  certificate. `driver-ci.yml` proves the signing tooling per push (smoke
  leg on a copy of the built driver).

## 4. What this is NOT

- Not production signing: a self-signed certificate is not an identity any
  other machine trusts by default, SmartScreen does not recognize it, and
  Secure-Boot-ON production machines will refuse the driver. Release notes
  state all of this.
- Not a test-signing requirement on the CUSTOMER machine: Secure-Boot-off
  customer machines load the driver out of the box after the installer
  imports the certificate. Only Secure-Boot-on machines need the
  `bcdedit` step (stated in the installer's driver page, the release
  notes, and the About page's driver status note).
- Not a bypass of the Win7 import gates or the driver's own PE gates:
  those run on the unsigned msbuild output; the signature is appended
  afterward.

## 5. The production path (pending, unchanged in structure)

When a real certificate is provisioned (EV/OSSign for the app+installer;
Hardware Dev Center attestation for the driver — the `driver/signing/ossign`
mirror workflow is the prepared rail), the same call sites change payload:
`installer/devsign.ps1` gains a "prod" action that consumes the provisioned
certificate from a GitHub environment secret instead of generating one, the
`testsigning.cer` asset becomes the real publisher certificate, and the
installer's import step becomes unnecessary for Secure-Boot machines (the
Microsoft signature chains by itself). The Tauri bundle signature lands via
Tauri's `windows` bundler signing config at that time. Until then every
release note carries the truthful status: **development test-signed;
production signing pending.**
