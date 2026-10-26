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
  app + engine kit + optional RDA driver (surfaced, explained, consent-
  gated step); per-machine and per-user modes; complete uninstall (sessions
  kept by default with explicit consent); update = stop/delete/recreate the
  driver service + in-place app upgrade (ProgramData preserved).
- `RecoveryWizard-<version>-portable.zip` — app + engine beside the exe,
  driver STAGED not installed; run as administrator for raw-disk access.
- `SHA256SUMS` — every artifact, published with the release.

MSIX is not produced: a Windows kernel driver cannot be installed from an
AppContainer package, and the app requires raw-disk handles an AppContainer
cannot open. The truthful verdict is `docs/MSIX-ASSESSMENT.md`; the
portable zip serves the no-installer need.

## Signing status (current, stated plainly)

**Nothing is signed.**

- `rda.sys` ships unsigned by design (the CI asserts an empty certificate
  directory). Stock Secure-Boot Windows 10/11 will not load it: enable
  test signing (`bcdedit /set testsigning on`) or provision a signed
  driver (the installer accepts a signed drop-in later without reinstall).
- The app/engine binaries are unsigned until a code-signing certificate is
  provisioned; SmartScreen may warn on first run.

The release notes carry this section verbatim. When a certificate is
provisioned: sign the driver via Hardware Dev Center / attestation, sign
the app + installer with signtool, and update this document + the notes
template — the pipeline (release.yml) is structured so signing slots in
before the upload step.

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
