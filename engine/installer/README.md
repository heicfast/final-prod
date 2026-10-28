# installer/ — the app-installer surface (S-038)

Built strictly against [docs/PACKAGING.md](../docs/PACKAGING.md) — the
13-parameter contract with corpus citations. Nothing here is invented; every
parameter maps to a section of the contract (the mapping table is in the
ISS header comment).

## Files

| File | Role |
|---|---|
| `core-recovery.iss` | the Inno Setup script — the 13-parameter contract: silent flags (native), admin manifest, ARP entries (QuietUninstallString written+verified by `[Code]`), the frozen single-instance AppId, the version key (write+verify), the OS gate (§6), the driver deployer as an internal step (§10) |
| `osgate.ps1` | the OS-gate decision module — the shared decision table (the OSGATE-V1 marker is the sync pin with the ISS); injectable version tuples for the matrix gate |
| `build-installer.ps1` | stages the payload (bin/include/docs/data/LICENSE + the fetched driver release) and runs ISCC |
| `run-gates.ps1` | the executable test gates 1+2: silent round trip + version key + no-residue assertions |
| `payload/` | created by `build-installer.ps1` (gitignored) |

## Building

```powershell
# prerequisites: cmake build done (win-x64-release), a fetched+verified
# (and signed, for real deployments) rda-driver release in driver\
cmake --preset win-x64-release; cmake --build --preset win-x64-release
pwsh -File .github/scripts/fetch-release.ps1 -Repo heicfast/rda-driver -OutDir driver `
  -Required 'rda.sys,rda.inf,install.ps1,uninstall.ps1,SHA256SUMS' -FloorVersion v0.2.3

pwsh -File installer\build-installer.ps1 -Version 1.2.3
# -> dist\core-recovery-setup.exe
```

Requires Inno Setup 6.3+ (`x64compatible`); CI pins 6.4.3 via choco.

## The three test gates (PACKAGING.md §11)

| Gate | What | Where |
|---|---|---|
| 1 InstallerSilentRoundTrip | `/VERYSILENT` install (zero prompts, driver component on) → ARP `UninstallString` + `QuietUninstallString` (ends `" /SILENT"`) → uninstall via the quiet string verbatim → no residue (files/ARP/version key/service/driver images) | `run-gates.ps1` (CI: `installer.yml` job `silent-roundtrip`) |
| 2 InstallerVersionKeyWritten | `HKLM\SOFTWARE\heicfast\core_recovery\Version` reads back exactly the built version | same run (the read-back IS the verify path) |
| 3 InstallerOSGateMatrix | the gate admits the Win7-SP1→Server-2025 x64 envelope and refuses outside it (XP/Vista/Win7-RTM/x86/ARM64), via injectable tuples + the ISS↔module sync pin | `tools/test_installer_osgate.py` (CI: job `osgate-matrix`) |

## Deploying (what the CI does, and why)

The CI lane test-signs the fetched driver with a job-local certificate
before packing (the proven runner mechanism — hosted runners are
SecureBoot-off). A production deployment consumes properly signed driver
assets per [DRIVER-RELEASE](../docs/DRIVER-RELEASE.md) — the app installer
absorbs the driver step (§10); it never signs or reimplements SCM itself.

## Registered decisions (frozen in the contract doc)

- Technology: **Inno Setup** (PACKAGING.md §2 — the contract's default,
  evidence-pinned).
- Telemetry: **omitted** (§8 — no endpoint; no stub verbs).
- Multi-instance: **single instance** (§9 — one frozen AppId
  `{2F4B9C31-7A08-4D6E-9B5C-3E1A8F0D4C72}`, in-place upgrade).
- Version key path: `HKLM\SOFTWARE\heicfast\core_recovery\Version` (§7).
- Driver service: `RDA` (the core transport's `kDefaultDriverService`).
