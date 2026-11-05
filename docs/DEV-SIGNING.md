# DEV-SIGNING — development self-signing for the user-mode product

> Session: 2026-10-27 (mandatory-driver + development-signing session);
> **rescoped 2026-11-05 (consolidation session)** — the kernel driver was
> REMOVED from the product entirely (commit 318b9655), which ended the
> kernel-signing era this document originally described: no `rda.sys`
> embed signature, no certificate-store imports, no test-signing
> requirement on any machine, no attestation path. The design below is
> the user-mode posture; the kernel-era policy research is preserved in
> the dated appendix (§6) as the audit record of why that era's
> installer behaved the way it did.

## 1. What is signed, and with what

| Artifact | Signature | Applied by |
|---|---|---|
| staged `cr_ui.dll` + `rrecov.exe` (the engine kit) | embedded (Authenticode, SHA-256) | `installer/devsign.ps1` via `build.ps1` Invoke-Engine |
| `RecoveryWizard-<version>-setup.exe` (the installer) | embedded | `installer/devsign.ps1` via `build.ps1` Invoke-Installer |
| the Tauri application bundle (inner app-setup + `recovery-wizard-ui.exe`) | **not signed** | the production-signing gap (§5); the bundle is produced inside `pnpm tauri build` and its signature awaits Tauri's bundler signing config with a real certificate |

The certificate is **ephemeral and self-signed**: `New-SelfSignedCertificate
-Type CodeSigningCert` (in `Cert:\CurrentUser\My` — works elevated AND
non-elevated), subject `CN=HeicFast Recovery Wizard Development
Signing`, generated per build into the stage dir. The tool outputs
`devsign.cer`, `devsign.pfx`, and `devsign-thumbprint.txt` beside the
staged payloads (`devsign.ps1 -Action new-cert | sign | verify`).
Nothing private is committed, and nothing private ships: the PFX and its
ephemeral password file are gitignored build outputs that exist only for
the duration of the build. Every rebuild carries a fresh certificate.

## 2. Why we sign at all (the user-mode rationale)

A self-signed development signature on user-mode binaries is an
**integrity marker, not a load prerequisite**:

- It proves the payload *came from our build* — byte-identical rebuilds
  can be cross-checked against the recorded thumbprint.
- It is NOT a publisher identity any other machine trusts: SmartScreen
  does not recognize it, and no ecosystem trust is claimed.
- **Nothing is imported into any store anywhere.** The installer performs
  zero certificate operations; the uninstaller performs zero certificate
  operations; the portable zip ships no certificate. (This is the
  deliberate inversion of the kernel era, where the driver's load-time
  code-integrity check forced the Root/TrustedPublisher imports — see
  the appendix.)
- User-mode code runs regardless of the signature under normal Windows
  policy — which is exactly why the marker is honest: it claims
  provenance, nothing more.

The engine's Win7 import gates run **before** signing (a signature
changes bytes, not imports).

## 3. How the product carries it

- `build.ps1` (engine + installer lanes) generates/reuses the stage
  certificate and signs the payloads (§1).
- **The installer** (`installer/recoverywizard.nsi`) embeds nothing
  signing-related: no `SEC_DRIVER`, no `certutil` import, no
  thumbprint registry value. Files are placed; that is all.
- **The portable zip** stages app + engine kit + README, with the
  truthful label: run the executable as administrator for raw-disk work;
  sessions and logs land beside it.
- **CI validation** (`validation-evidence.yml`) records the elevation
  facts (the driverless transport's prerequisite) and measures the real
  product path directly; `release.yml` verifies the expected signatures
  on the shipped artifacts (`devsign.ps1 -Action verify` throws on
  absence).

## 4. What this is NOT

- Not production signing: a self-signed certificate is not an identity
  any other machine trusts, and SmartScreen does not recognize it.
  Release notes state this.
- Not a test-signing requirement on ANY machine: no `bcdedit`, no
  Secure-Boot guidance, no per-machine provisioning step exists in the
  user-mode product. Machines need nothing beyond the app itself.
- Not a bypass of the engine's PE/import gates: those run on the
  unsigned build output; the signature is appended afterward.

## 5. The production path (pending, unchanged in structure)

When a real certificate is provisioned (a CA certificate — EV/OV — for
the app + engine kit + installer), the same call sites change payload:
`installer/devsign.ps1` gains a "prod" action that consumes the
provisioned certificate from a GitHub environment secret instead of
generating one, and the release notes state the real publisher. The
Tauri bundle signature lands via Tauri's `windows` bundler signing
config at that time. No Hardware Dev Center / attestation step exists in
this path anymore — there is no kernel binary to attest. Until then
every release note carries the truthful status: **development
self-signed; production signing pending.**

## 6. Appendix (historical, dated 2026-10-27): the kernel-signing era

Kept as the audit record of the mandatory-driver product this session
replaced (commit 318b9655). The facts below were true of THAT product
and its v0.6.x development releases; none of it applies to the current
tree.

### 6.1 What was signed then

`driver\bin\x64\Release\rda.sys` was embed-signed after the WDK build
(a *packaging* step, never a build step — the WDK project's `SignMode`
stayed Off and driver-ci asserted the unsigned msbuild output), the
public half was published as the `RecoveryWizard-<version>-testsigning.cer`
release asset, and the installer imported the certificate into
`LocalMachine\Root` + `TrustedPublisher` before creating the `RDA`
service (the uninstaller removed it by thumbprint). The load rationale
was kernel code integrity, not user-mode provenance.

### 6.2 Why the driver loaded (the policy matrix, primary sources)

A kernel-mode driver loads when Windows code integrity accepts its
signature; for a self-signed development certificate the acceptance
conditions were:

| Machine state | Test-signed driver loads? | Why |
|---|---|---|
| Test signing ON (`bcdedit /set testsigning on` + reboot) | **YES** | test-signing mode accepts any embed signature — no root trust needed |
| Secure Boot OFF, test signing OFF, cert in `LocalMachine\Root` + `TrustedPublisher` | **YES** | classic trusted-publisher acceptance |
| Secure Boot OFF, stores empty | no | the signature chains to nothing the machine trusts |
| Secure Boot ON, test signing OFF | **no** | Secure Boot requires a Microsoft signature (WHQL/attestation); local test certs are insufficient and test signing is blocked |

Primary sources (2026-10-27 research):

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

That entire matrix — and the Secure-Boot/`bcdedit` guidance the 0.6.x
release notes carried — became moot the moment the driver was removed:
user-mode binaries do not pass through kernel code integrity at all.
