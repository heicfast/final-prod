# RELEASE — the combined release convention and cadence

> The `release.yml` workflow is the **sole source of builds** (PLAN-CI-REBUILD
> §3.5; docs/PREBUILT-ARTIFACTS.md §4). Consumers — the CI workflows, the
> product installer, external consumers — depend on this single release point.

## Tag convention

`vX.Y.Z` — plain semver, pushed to `main`. The workflow triggers on
`v[0-9]+.[0-9]+.[0-9]+` (the historical `v*-parity` tags are grandfathered
for lookup but never emitted again). `workflow_dispatch` runs are DRILLS:
they execute validate → build → driver-tier → assemble and upload the
`combined-release-set` artifact, but publish no GitHub release
(`skip_validation` is the drill-only gate bypass — never for a real release).

## The pipeline (and why each stage exists)

1. **validate** — the FULL pre-merge validation suite (`ci.yml` via
   `workflow_call`: local-mirror, Linux/ASan, TSan, UBSan, fuzz, the Windows
   build + ABI lane). **No release without green** — a red validation blocks
   the release, without exception.
2. **build-core** — the ONE from-scratch build, from the tagged commit (the
   release is defined by the tag). MSVC /W4 /WX Release, full ctest, Win7
   posture gate on every shipped PE, the proven SDK+CLI bundle packaging
   (bin/lib/include/docs + vcpkg + VC/UCRT runtime + in-zip SHA256SUMS).
3. **driver-tier** — resolves the rda-driver release at the compatibility
   tier **by the prebuilt convention** (default + preferred: fetch +
   SHA256SUMS-verify the flat assets). ONLY when no consumable tier release
   exists does the fallback build the driver from source at the tier
   (checkout + MSBuild of rda.sln), stamped `built-from-source` — the
   mandate's sanctioned fallback, used never when the prebuilt path works.
4. **assemble** — `MANIFEST.json` (every artifact: name, version, SHA256,
   source tier, consumed-vs-built; the driver tier block; the signing
   posture) + top-level `SHA256SUMS` covering every asset.
5. **publish** (tag pushes only) — the GitHub release with the full asset
   set and notes that reference the manifest, the compatibility contract,
   and the applicable PARITY-DELTAS entries.

## Compatibility tier mapping

Core 1.x ↔ **driver wire major 1** (`kDriverWireMajorRequired`,
cr_driver_status.h). The driver tier resolves to the latest rda-driver
release (its wire major has been 1 since v0.2.0 — DRIVER-CONTRACT §5).
The core REFUSES a wrong-major driver with `ERR_DRIVER_VERSION_MISMATCH`
(`0x500B0004`) — the release notes state the contract; `rrecov driver
--json` reports it per deployment.

## Signing posture (this session, stated truthfully)

Driver artifacts: **test-signed at install time** (the consumer's job-local
certificate — the runner-proven mechanism; the shipped binaries themselves
are unsigned, DRIVER-RELEASE §7). Core binaries: **unsigned**. Both facts
are in every release's notes and in MANIFEST.json's `signing_posture`.

## Cadence policy (verbatim from the mandate)

**Release on every green milestone; the current release is the substrate
for CI; there is no external release schedule.** Every time the build and
validation are green after a set of changes, a new tag is pushed and the
release workflow produces a new release. There is no cap on the number of
releases — they are cheap (validation-consumed prebuilt dependencies where
possible) and they are exactly what the correctness workflows consume.
