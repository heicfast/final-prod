# RELEASE — the combined release convention and cadence

> The `release.yml` workflow is the **sole source of builds** (PLAN-CI-REBUILD
> §3.5; docs/PREBUILT-ARTIFACTS.md §4). Consumers — the CI workflows, the
> product installer, external consumers — depend on this single release point.

## Tag convention

`vX.Y.Z` — plain semver, pushed to `main`. The workflow triggers on
`v[0-9]+.[0-9]+.[0-9]+` (the historical `v*-parity` tags are grandfathered
for lookup but never emitted again). `workflow_dispatch` runs are DRILLS:
they execute validate → build → assemble and upload the
`combined-release-set` artifact, but publish no GitHub release
(`skip_validation` is the drill-only gate bypass — never for a real release).

## The pipeline (and why each stage exists)

1. **validate** — the FULL pre-merge validation lane (`checks.yml` via
   `workflow_call`: the clang-format + dead-surface/architecture +
   PowerShell-AST + workflow-YAML + doc-truth lint gates, then the MSVC
   /W4 /WX Release build, the full ctest including the cr_ui ABI
   C-consumer, the Win7 posture gate on every shipped PE, the G-09
   report-schema validation, the unelevated-refusal probes and
   the M-071 CLI junk-gate contract). **No release without green** — a
   red validation blocks the release, without exception.
   (Historical: the pre-consolidation era ran this as the 7-job `ci.yml`
   with Linux/ASan/TSan/UBSan/fuzz lanes; those were dropped by the
   owner's windows-only directive — the sanitizer coverage keeps running
   locally via `./ci-local.sh --san|--fuzz`, docs/CI-CURRENT.md.)
2. **build-core** — the ONE from-scratch build, from the tagged commit (the
   release is defined by the tag). MSVC /W4 /WX Release, full ctest, Win7
   posture gate on every shipped PE, the proven SDK+CLI bundle packaging
   (bin/lib/include/docs + vcpkg + VC/UCRT runtime + in-zip SHA256SUMS).
3. **assemble** — `MANIFEST.json` (every artifact: name, version, SHA256,
   source tier, consumed-vs-built; the signing
   posture) + top-level `SHA256SUMS` covering every asset. Session-96: the
   asset set is CORE ONLY — `core-recovery-<tag>.zip` + `MANIFEST.json` +
   `SHA256SUMS`; no driver tier, nothing fetched from rda-driver.
4. **publish** (tag pushes only) — the GitHub release with the full asset
   set and notes that reference the manifest, the compatibility contract,
   and the applicable PARITY-DELTAS entries.

## Compatibility tier mapping

Session-96 removed the driver tier entirely, so there is no wire-major
mapping to carry: the core's single compatibility contract is the `cr_ui`
ABI version (v4 — the v3→v4 bump removed the driver-era `cr_ui_driver_status`
export with the kernel driver) and the Win7-SP1 PE envelope. The historical
driver-wire mapping (core 1.x ↔ driver wire major 1, the wrong-major
`ERR_DRIVER_VERSION_MISMATCH` refusal) is the driver-era record.

## Signing posture (stated truthfully)

Core binaries: **unsigned by design** (byte-comparable
builds) — the fact is in every release's notes and in MANIFEST.json's
`signing_posture`. The driver-era test-signing stage (CN=RRECOV-DEV-SIGN,
the `rda-test-sign.cer` asset) went with the driver (session-96): nothing
is signed, imported, or installed but the core. Access needs administrator
privilege, not a certificate.

## Cadence policy (verbatim from the mandate)

**Release on every green milestone; the current release is the substrate
for CI; there is no external release schedule.** Every time the build and
validation are green after a set of changes, a new tag is pushed and the
release workflow produces a new release. There is no cap on the number of
releases — they are cheap (validation-consumed prebuilt dependencies where
possible) and they are exactly what the correctness workflows consume.
