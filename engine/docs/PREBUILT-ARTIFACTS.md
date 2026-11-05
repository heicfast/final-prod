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

## 2. Driver releases (rda-driver) — REMOVED (session-96, historical record)

The driver-era kernel component is GONE: the release workflow builds the CORE ONLY and
nothing is resolved from `heicfast/rda-driver` (the doc_truth_gate's
FloorVersion check asserts no workflow carries a driver pin). The
driver-era convention — tag `vX.Y.Z`, required assets `rda.sys` /
`rda.inf` / `install.ps1` / `uninstall.ps1` / `SHA256SUMS`, functional
floor ≥ v0.2.1 (the bare-open error-87 fix), the persistent dev-cert
test-signing stage (CN=RRECOV-DEV-SIGN, the `rda-test-sign.cer` asset
imported into `LocalMachine\Root` + `TrustedPublisher` before install),
and the M-063 environment matrix (only GitHub's windows-2022,
HVCI/VBS-off, was execution-validated) — is the historical record of the
ended era; the surviving access-architecture record lives at
worklog/reference/og-access-architecture-record.md.

**Operator posture (N-085, historical):** the driver-era scripts were
operator-run scripts for dev/CI machines. The customer-shaped silent
installer is the packaging milestone — S-038's PACKAGING.md contract
(which now ships the core only; §10 there records the removal).

## 3. Core releases (core_recovery)

Resolved from `heicfast/core_recovery`. Tag convention: `vX.Y.Z` (plain
semver — new convention from the combined-release era; the historical
`v*-parity` tags are grandfathered for lookup but never emitted again).
**Required assets** (session-96: the release workflow builds the CORE ONLY
— no driver tier, nothing fetched from rda-driver):

| Asset | Role |
|---|---|
| `core-recovery-<tag>.zip` | core binaries + libs + include/ + docs + in-zip SHA256SUMS; carries `userdata-defaults.json` beside `rrecov.exe` (M-065: the og's install-tree EUUserData posture — 111 entries, the engine-init default user-sign set; absent file = the empty registry + one log line, never an error) |
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
   (the unelevated-refusal probe — exit 4 with `ERR_ELEVATION_REQUIRED` on a
   non-elevated token — doubles as the ABI regression pin).

For releases predating the combined era (e.g. `v0.4.1-parity`): the fetcher
fails loudly on the missing `MANIFEST.json`/`SHA256SUMS` assets with a
message naming the first combined release — consumers must resolve a
combined release, exactly the documented behavior (no silent fallback to
the legacy zip-only shape). The driver-era releases that additionally
shipped `rda.sys`/`install.ps1`/`uninstall.ps1` tier-mates are historical
(session-96 removed the tier).

## 4. Failure behavior (the loud-failure contract)

Every convention violation is a step failure with a diagnostic that names:
the resolved repo + tag, the missing/mismatching asset, and the remediation
(publish the release / re-run the release workflow). There
is no try-older-release, no skip-asset, no build-from-source fallback in any
correctness workflow. The single sanctioned source build lives in the release
workflow (the tag defines the build) and in the validation lane (whose
subject IS the source) — both documented by charter in CI-LOCAL.md.

## 5. Consumers

| Workflow | Consumes | Via |
|---|---|---|
| `release.yml` | nothing external — builds the core only from the tagged commit (session-96: the driver-tier job is gone) | the tag is the build |
| `installer.yml` (S-038 gates) | builds the installer's own subject from source (core only — no driver component, no SCM step; teardown asserts NO kernel service) | its own build |
| `test-benchmark.yml` | the LATEST core release (core-only asset set; no driver-install or verify — the elevated runner token IS the access model) | fetch-release.ps1 |

The historical consumers (`driver-install.yml`, `core-install.yml`,
`staged-scenarios.yml` and the `.github/actions/install-driver` composite)
were deleted by the 3-lane consolidation (566c483); their contracts live on
in the lanes above (the refusal contract in checks.yml's unelevated probes;
the pinned-matrix truth in benchmark-matrix.json). The driver-era
install/verify chain (`Install-ReleaseDriver.ps1`, verify-driver.ps1) went
with the driver itself (session-96).
