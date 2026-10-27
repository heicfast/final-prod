# RELEASE — tag flow, artifacts, signing truth

## Flow

1. Land on `main` (all lanes green: contracts, ui, engine, driver; e2e on
   the test-mode build).
2. Bump the root `VERSION` file and its lockstep consumers
   (`ui/package.json`, `ui/src-tauri/Cargo.toml`,
   `ui/src-tauri/tauri.conf.json`) — `build.ps1`'s gate enforces this.
3. Tag `v<VERSION>`. The `release` workflow refuses any tag that does not
   equal the VERSION file.
4. `release.yml` runs: contracts gate → `build.ps1 -Target all` (driver →
   engine → ui+tauri → NSIS installer → portable zip → SHA256SUMS) →
   artifact presence verification → release upload with the truthful notes.

## Artifacts (per release)

- `RecoveryWizard-<version>-setup.exe` — the combined NSIS installer:
  app + engine kit + the REQUIRED RDA driver (surfaced, explained,
  `SectionIn RO` — there is no product without it; per-user installs do
  not exist); all-users install; complete uninstall including the imported
  development certificate (sessions kept by default with explicit
  consent); update = stop/delete/recreate the driver service + in-place
  app upgrade (ProgramData preserved).
- `RecoveryWizard-<version>-portable.zip` — app + engine + the STAGED
  (not installed) development-signed driver; the engine refuses raw-disk
  work until the driver runs (install via the staged scripts or use the
  installer).
- `RecoveryWizard-<version>-testsigning.cer` — the build's public
  development certificate (embedded in the installer's flow; published
  for transparency and manual provisioning).
- `SHA256SUMS` — every artifact, published with the release.

MSIX is not produced: a Windows kernel driver cannot be installed from an
AppContainer package, and the app requires raw-disk handles an AppContainer
cannot open. The truthful verdict is `docs/MSIX-ASSESSMENT.md`; the
portable zip serves the no-installer need.

## Signing status (current, stated plainly)

**Development TEST-SIGNED; production signing pending.**

- `build.ps1` embed-signs `rda.sys`, the engine kit (`cr_ui.dll`,
  `rrecov.exe`) and the installer with an **ephemeral self-signed
  development certificate** generated per build (nothing private committed
  or shipped; `installer/devsign.ps1`, design in `docs/DEV-SIGNING.md`).
- The installer imports the certificate into `LocalMachine` Root +
  Trusted Publishers before creating the driver service, so the driver
  loads out of the box on Secure-Boot-OFF machines. On Secure-Boot-ON
  machines, enable test signing (`bcdedit /set testsigning on`) and
  reboot, or disable Secure Boot; the registered demand-start driver then
  loads.
- The Tauri application bundle is not signed yet (production gap,
  `docs/DEV-SIGNING.md` §5); SmartScreen may warn on the app's first run.
- The msbuild driver output stays unsigned by design (`driver-ci.yml`
  asserts it): the signature is a packaging step, never a build step.

The release notes carry this section verbatim. When a production
certificate is provisioned: sign the driver via Hardware Dev Center /
attestation, sign the app + installer with signtool, and update this
document + the notes
## Reproducibility

The same tag rebuilds the same artifacts (byte-identical modulo signing
timestamps): pinned WDK NuGet, vcpkg manifest mode, the VERSION gate, and
deterministic NSIS defines (`!error` guards on every required define).
`build.ps1` is the single build entry point for both CI and a clean local
checkout.

## Candidate check (T18 discipline)

Before a public tag: run an RC tag (`v<version>-rc1`), confirm
`release.yml` produces every artifact, and install/run/uninstall on a clean
Windows runner (evidence under `docs/INTEGRATION-VALIDATION/`). Only then
cut the final tag.
