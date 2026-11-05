# RELEASE — tag flow, artifacts, signing truth

## Flow

1. Land on `main` (all lanes green: contracts, ui, engine; e2e on
   the test-mode build).
2. Bump the root `VERSION` file and its lockstep consumers
   (`ui/package.json`, `ui/src-tauri/Cargo.toml`,
   `ui/src-tauri/tauri.conf.json`) — `build.ps1`'s gate enforces this.
3. Tag `v<VERSION>`. The `release` workflow refuses any tag that does not
   equal the VERSION file.
4. `release.yml` runs: contracts gate → `build.ps1 -Target all`
   (contracts → engine → ui+tauri → NSIS installer → portable zip →
   SHA256SUMS) → artifact presence verification → release upload with the
   truthful notes.

## Artifacts (per release)

- `RecoveryWizard-<version>-setup.exe` — the combined NSIS installer:
  app + engine kit, all-users install; complete uninstall of everything
  placed (sessions kept by default with explicit consent); update =
  in-place app upgrade (ProgramData preserved). The product is user-mode:
  no driver, no service, no certificate-store changes, nothing loaded
  into the OS (the driver-era `-testsigning.cer` asset is gone with it).
- `RecoveryWizard-<version>-portable.zip` — app + engine kit beside the
  exe, no install: run the executable as administrator for raw-disk
  work; sessions and logs are written beside it.
- `SHA256SUMS` — every artifact, published with the release.

MSIX is not produced (the assessment is `docs/MSIX-ASSESSMENT.md`);
the portable zip serves the no-installer need.

## Signing status (current, stated plainly)

**Development self-signed; production signing pending.**

- `build.ps1` embed-signs the engine kit (`cr_ui.dll`, `rrecov.exe`) and
  the installer with an **ephemeral self-signed development certificate**
  generated per build (nothing private committed or shipped;
  `installer/devsign.ps1` — actions new-cert/sign/verify; outputs
  `devsign.cer`, `devsign.pfx`, `devsign-thumbprint.txt` into the stage
  dir; design in `docs/DEV-SIGNING.md`). The signature is an integrity
  marker — proof the payload came from our build — and **not** a load
  prerequisite: user-mode binaries need no kernel-signing trust, and the
  installer imports NO certificate into ANY store.
- The Tauri application bundle is not signed yet (production gap,
  `docs/DEV-SIGNING.md` §5); SmartScreen may warn on the app's first run.

The release notes carry this section verbatim. When a production
certificate is provisioned (a real CA certificate for the app + engine
kit + installer), `devsign.ps1` gains a "prod" action that consumes it
from a GitHub environment secret instead of generating one; until then
every release note carries the truthful status: **development
self-signed; production signing pending.**

## Release notes outline (what the truthful notes must state)

- The product is a **user-mode application** with direct raw-disk access:
  no kernel driver, no service, no certificate changes; uninstall removes
  everything.
- **Raw-disk operations require running as administrator** — scan,
  recover, health, repair, lost-partition apply; the non-elevated app
  refuses truthfully (`ERR_ELEVATION_REQUIRED`, CLI exit 4) and offers a
  restart-as-administrator action. No degraded mode, no fallback.
- The **driver removal is the headline change** vs the 0.6.x development
  releases (consolidation session, commit 318b9655): earlier releases
  shipped a mandatory kernel-mode transport; this one installs nothing
  into the OS.
- The signing status section above, verbatim.

## Reproducibility

The same tag rebuilds the same artifacts (byte-identical modulo signing
timestamps): the vcpkg manifest, the VERSION gate, and
deterministic NSIS defines (`!error` guards on every required define).
`build.ps1` is the single build entry point for both CI and a clean local
checkout.

## Candidate check (T18 discipline)

Before a public tag: run an RC tag (`v<version>-rc1`), confirm
`release.yml` produces every artifact, and install/run/uninstall on a clean
Windows runner (evidence under `docs/INTEGRATION-VALIDATION/`). Only then
cut the final tag.
