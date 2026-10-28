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

**Verification procedure** (every consumer runs it, via
`.github/scripts/fetch-release.ps1` — the shared composite action of the
pre-consolidation era was deleted with its three workflows; the fetcher is
the one convention enforcer now):
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

**Install posture on runners:** the release's `rda.sys` arrives ALREADY
test-development-signed with the repo's **persistent dev certificate**
(`release.yml`'s "Test-development-sign the driver (persistent dev
certificate)" stage: GitHub secrets `DEV_SIGN_CERT_PFX_B64` +
`DEV_SIGN_CERT_PASSWORD`, CN=RRECOV-DEV-SIGN, 3-year self-signed); the
public cert ships as the `rda-test-sign.cer` release asset and is imported
into `LocalMachine\Root` + `LocalMachine\TrustedPublisher` **before**
install (`Install-ReleaseDriver.ps1:6-11`) — that is the entire trust story
on HVCI-off runners (windows-2022 = the execution-validated class), no
bcdedit/testsigning, no reboot, and the signature is verified
(`Get-AuthenticodeSignature` → Valid) before any kernel load attempt.
Install goes through the release's own `install.ps1 -ServiceName rda
-Source rda.sys` — the mechanism proven on windows-2022 by the historical
api-benchmark lane's real runs (40227375329, 40315255368; that lane was
deleted by the consolidation — the current provers are `test-benchmark.yml`'s
Install-ReleaseDriver.ps1 stage and `installer.yml`'s round trip, e.g. run
40549660074's trust-import → install → RUNNING → control round-trip →
uninstall transcript). The earlier job-local self-signed-cert mechanism
(`New-SelfSignedCertificate` per run) was superseded by the persistent
dev-cert model — recorded here per I-026's S11B cite-refresh addendum.
Signing posture is documented truthfully per release (DRIVER-RELEASE §7:
unsigned-by-design kernel artifacts, the dev-sign stage is release-pipeline
consumption-time signing for test/CI load, not a vendor signature).

**The v0.2.4 deployer contract (S-036/M-060/M-061/M-062/M-064 closed):** both
live fetch sites (`release.yml`'s driver tier and `installer.yml`) pin
`FloorVersion v0.2.4` — the versioned-layout era (the `%s%d` image names +
the versioned-aware uninstall the M-061 teardown contract asserts). The
installer's exit-code contract:
0 installed+started · 1 not elevated · 2 name exists · 3 copy failed ·
4 CreateService SCM error · 5 Parameters read-back mismatch · 6 StartService
failed · 7 not RUNNING. `uninstall.ps1`: 0 clean · 1 not elevated · 2 stop
error · 3 no STOPPED within 5 s · 4 delete error · 5 still registered ·
6 locked image — the CI teardown runs the script and asserts the post-state
(service gone, versioned image gone). The pre-consolidation composite's
manual `sc.exe` fallback no longer exists in any current path: the two live
consumers run the release's own `install.ps1` verbatim (the S-036 lesson —
the composite fallback was where the M-060 divergence lived).

**Operator posture (N-085):** these are operator-run scripts for dev/CI
machines (elevated console, visible window). The customer-shaped silent
installer (the og's product-orchestrated SCM deployment) is the packaging
milestone — S-038's PACKAGING.md contract, not this lane.

**Environment matrix (M-063):** the only execution-validated environment class
is GitHub's windows-2022 (HVCI/VBS off, job-local cert). On stock Win11 (HVCI
on) kernel loading requires attestation-signed drivers — use a VM with VBS
off or `bcdedit /set testsigning on` + reboot for dev boxes; Win7 SP1 is
build-gated (PE headers + import allowlist) but not install-validated —
treat it as UNVALIDATED until a runner-class dispatch covers it. The release
compatibility sentence is PE-header evidence, not install evidence.

## 3. Core releases (core_recovery)

Resolved from `heicfast/core_recovery`. Tag convention: `vX.Y.Z` (plain
semver — new convention from the combined-release era; the historical
`v*-parity` tags are grandfathered for lookup but never emitted again).
**Required assets** (from the first combined release, `v0.5.0`):

| Asset | Role |
|---|---|
| `core-recovery-<tag>.zip` | core binaries + libs + include/ + docs + in-zip SHA256SUMS; carries `userdata-defaults.json` beside `rrecov.exe` (M-065: the og's install-tree EUUserData posture — 111 entries, the engine-init default user-sign set; absent file = the empty registry + one log line, never an error) |
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
| `release.yml` (driver-tier job) | the rda-driver release at the compatibility tier (`-FloorVersion v0.2.4`; source-build only when the tier release does not exist, stamped `built-from-source` in MANIFEST.json) | fetch-release.ps1 |
| `installer.yml` (S-038 gates) | the rda-driver release at the same tier (`-FloorVersion v0.2.4`), job-local test-signed | fetch-release.ps1 |
| `test-benchmark.yml` | the LATEST combined core release (which embeds the tier-mate `rda.sys` + `rda-test-sign.cer` + install/uninstall scripts) | fetch-release.ps1 + `.github/scripts/Install-ReleaseDriver.ps1` |

The historical consumers (`driver-install.yml`, `core-install.yml`,
`staged-scenarios.yml` and the `.github/actions/install-driver` composite)
were deleted by the 3-lane consolidation (566c483); their contracts live on
in the lanes above (the refusal contract in checks.yml's driverless probes;
the install chain in test-benchmark's Install-ReleaseDriver stage; the
pinned-matrix truth in benchmark-matrix.json).
