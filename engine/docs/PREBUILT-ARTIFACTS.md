# PREBUILT-ARTIFACTS — the deterministic latest-release convention

> The contract every correctness workflow enforces (PLAN-CI-REBUILD §2). The
> rule: **the correctness path never builds from source.** Workflows resolve
> "the latest prebuilt release" by the convention below, verify it, and fail
> LOUDLY when the convention is not met — a missing required asset is a
> diagnostic, never a silent fallback to an older tag or a source build.
> The combined release workflow ([RELEASE.md](RELEASE.md)) is the sole source
> of builds.

## 1. The resolution rule

"Latest release" = the release the GitHub API returns for
`GET /repos/<owner>/<repo>/releases/latest` — the most recent non-prerelease,
non-draft release by creation date. It is resolved ONCE per workflow run (the
`setup` job records it in `env-manifest.json`; every downstream job consumes
the RECORDED tag, so a release published mid-run cannot skew the matrix).
`workflow_dispatch` inputs may pin an explicit tag for drill/debug runs — the
pin is recorded in the manifest; the default is always `latest`.

## 2. Driver releases (rda-driver)

Resolved from `heicfast/rda-driver`. Tag convention: `vX.Y.Z` (semver).
**Required assets** (exact names — DRIVER-RELEASE.md §3):

| Asset | Role |
|---|---|
| `rda.sys` | the kernel driver binary |
| `rda.inf` | the driver INF |
| `install.ps1` | the service installer (sc create/start) |
| `uninstall.ps1` | the cleanup script |
| `SHA256SUMS` | uppercase SHA256 + path per line |

Optional consumers: `rdacli.exe`, `run-integration.ps1`.

**Verification procedure** (every consumer runs it, in the shared composite
action `.github/actions/install-driver/action.yml`):
1. Download every asset of the resolved release.
2. Parse `SHA256SUMS`: each line is `<UPPERCASE_SHA256>  <path>`; the path may
   carry a `scripts/` prefix that does not match the flat asset name (a
   driver-repo packaging artifact, DRIVER-RELEASE §3) — the verifier matches a
   line to an asset by BASENAME and FAILS if a required asset has no matching
   line or the hash mismatches. Unresolvable lines for NON-required assets are
   reported (stdout), never silently skipped.
3. Any missing required asset or hash mismatch → the step exits non-zero with
   the named artifact and the resolved tag. No older-release retry.

**Functional floor:** the resolved release must be ≥ v0.2.1 (the bare-open
error-87 fix; DRIVER-UTILIZATION.md:241-252). The floor check is enforced by
version comparison in the fetcher; a latest below the floor fails loudly with
the explanation (it would classify `Errored` through the core's status query
regardless — belt and suspenders).

**Install posture on runners:** test-signed with a job-local self-signed cert
(`New-SelfSignedCertificate -Type CodeSigningCert` → `LocalMachine\Root` +
`LocalMachine\TrustedPublisher` → `signtool sign /fd SHA256`), installed via
the release's own `install.ps1 -ServiceName rda -Source rda.sys`, started with
`sc.exe start rda` — the mechanism proven by the api-benchmark lane's real
runs (40227375329, 40315255368) on windows-2022, which requires no
test-signing boot mode. Signing posture is documented truthfully per release
(DRIVER-RELEASE §7: unsigned-by-design artifacts, CI test-signs for load).

## 3. Core releases (core_recovery)

Resolved from `heicfast/core_recovery`. Tag convention: `vX.Y.Z` (plain
semver — new convention from the combined-release era; the historical
`v*-parity` tags are grandfathered for lookup but never emitted again).
**Required assets** (from the first combined release, `v0.5.0`):

| Asset | Role |
|---|---|
| `core-recovery-<tag>.zip` | core binaries + libs + include/ + docs + in-zip SHA256SUMS |
| `rda.sys` | the tier-mate driver binary (single-point consumers) |
| `rda.inf` | the driver INF |
| `install.ps1` | the driver installer |
| `uninstall.ps1` | the driver uninstaller |
| `MANIFEST.json` | every asset: name, version, SHA256, source tier, consumed-vs-built |
| `SHA256SUMS` | top-level, covering every asset above |

**Verification procedure** (`.github/scripts/fetch-release.ps1 -Repo core_recovery`):
0. Self-referential note: `SHA256SUMS` and `MANIFEST.json` are computed OVER the
   asset set and therefore carry no hash of themselves — their EXISTENCE is
   enforced (the required list), and the manifest's own cross-check anchors
   every artifact it lists; no scheme can hash its own index.
1. Resolve the release (or honor an explicit pin), download all assets.
2. Top-level `SHA256SUMS` check for every asset — mismatch → exit non-zero,
   naming the asset and the resolved tag.
3. `MANIFEST.json` cross-check: every required asset appears in the manifest
   with a matching name and SHA256; a disagreement → exit non-zero.
4. In-zip check for the core zip (its own SHA256SUMS covers the payload).
5. `rrecov.exe` presence + a `--version`-class smoke is the STAGING evidence
   (the driver-refusal probe `rrecov driver` → exit 4 with the driver absent
   doubles as the ABI regression pin — CORE-INSTALL workflow).

For releases predating the combined era (e.g. `v0.4.1-parity`): the fetcher
fails loudly on the missing `rda.sys`/`MANIFEST.json`/`SHA256SUMS` assets with
a message naming the first combined release — consumers of the driver-tier
mate must resolve a combined release, exactly the documented behavior (no
silent fallback to the legacy zip-only shape).

## 4. Failure behavior (the loud-failure contract)

Every convention violation is a step failure with a diagnostic that names:
the resolved repo + tag, the missing/mismatching asset, and the remediation
(publish the combined release / re-run the driver release workflow). There is
no try-older-release, no skip-asset, no build-from-source fallback in any
correctness workflow. The single sanctioned source build lives in the release
workflow (the tag defines the build) and in the validation lane (whose
subject IS the source) — both documented by charter in CI-LOCAL.md.

## 5. Consumers

| Workflow | Consumes | Via |
|---|---|---|
| `driver-install.yml` | latest rda-driver release + latest core release (for the `rrecov driver` verification) | composite action + fetch-release.ps1 |
| `core-install.yml` | latest core release | fetch-release.ps1 |
| `staged-scenarios.yml` | BOTH, pinned by the setup job's recorded manifest | composite action + fetch-release.ps1 |
| `release.yml` (driver-tier job) | the rda-driver release at the compatibility tier (fetch; source-build only when the tier release does not exist, stamped `built-from-source` in MANIFEST.json) | fetch-release.ps1 |
