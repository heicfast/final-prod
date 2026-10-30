# PLAN-CI-REBUILD — prebuilt-artifact CI, combined release, local-first validation

<!-- session-96: the driver era ended - the kernel driver was replaced by direct raw I/O; the driver-era rows below are historical records. -->

> **Superseded in part (2026-10-28 consolidation + M-088):** the lanes this
> plan specified in §3.2–§3.4 (`driver-install.yml`, `core-install.yml`,
> `staged-scenarios.yml`) were never created as separate workflows — the
> consolidation folded their contracts into the 4-lane set (checks /
> release / test-benchmark / installer; docs/CI-CURRENT.md). Read those
> sections as the historical design record; the contracts live on.

> Session: driver-only mandate §4–§6 (the driver-era mandate — historical). Current state: [CI-CURRENT.md](CI-CURRENT.md). Artifact shapes:
> the driver-era release contract (deleted with the driver, session-96; it specified tag `v*` and 7 flat
> assets), core release shape below.
> Non-negotiables inherited verbatim from the mandate: **no from-scratch build in the correctness path**
> (prebuilt releases by documented convention; the release workflow is the sole source of builds);
> **no release without green**; **no push without local green**; **no test on a mock** (real driver, real
> core, real seeded disk, real Windows runner).

---

## 1. Wipe list (what dies and why)

| Workflow | Verdict | Why |
|---|---|---|
| `.github/workflows/comparative-bench.yml` | **WIPED** | builds the full core PER MATRIX JOB (up to 45 jobs nightly, 93 total; CI-CURRENT §1) — the single largest waste; the og-vs-ours comparison lane it serves is superseded by the staged-scenario truth matrix |
| `.github/workflows/api-benchmark.yml` | **WIPED as a standalone benchmark** (its driver fetch/test-sign/install/verify chain is EXTRACTED into the shared composite action first) | full Release build per transport-touching push (63-70) to produce numbers, not correctness; the transport property tests live in test_directalign (unit lane); its driver-install mechanics are exactly what the new install workflows need — preserved, not lost |
| `.github/workflows/ci.yml` | **REBUILT** (pre-merge validation lane) | survives per mandate §4 ("preserve workflows needed for pre-merge validation") but is re-examined: ONE Windows build instead of Debug+Release double (107-119), the local-mirror gate added, zero-cache cost documented |
| `.github/workflows/release.yml` | **REBUILT** (combined release) | becomes the sole source of builds: core from tag + driver at tier + MANIFEST.json + SHA256SUMS |

The preserved cost reason, documented once: the validation lane builds from source **because its subject is
the source** (warnings-as-errors, sanitizers, unit truth on the exact tree being merged); every OTHER
workflow consumes releases. That is the whole rule.

## 2. The prebuilt-artifact convention (docs/PREBUILT-ARTIFACTS.md — the contract the workflows enforce)

**Driver (rda-driver):**
- Latest release = GitHub API `GET /repos/heicfast/rda-driver/releases/latest` (tag `vX.Y.Z`, semver).
- Required assets (exact names, from the deleted driver-era release contract §3): `rda.sys`, `rda.inf`, `install.ps1`,
  `uninstall.ps1`, `SHA256SUMS` (rdacli.exe/run-integration.ps1 optional consumers).
- Verification: SHA256SUMS per-line check with the documented `scripts/`-prefix tolerance
  (lines whose path does not resolve as a flat asset name are reported, not silently skipped —
  CI-CURRENT §3 documented the old fetcher's silent skip; the new one FAILS if a REQUIRED asset is
  missing or mismatches, and prints the missing list).
- Functional floor: >= v0.2.1 (the bare-open error-87 fix; the deleted driver-era utilization doc's § at 241-252); the convention
  resolves "latest", and a latest below the floor fails loudly (the status query would classify it Errored
  anyway — belt and suspenders).

**Core (core_recovery):**
- Latest release = `GET /repos/heicfast/core_recovery/releases/latest` (tag `vX.Y.Z` — NEW convention:
  plain semver; the historical `-parity` suffixes are grandfathered for lookup but never emitted again).
- Required assets: `core-recovery-<tag>.zip` (bin: rrecov.exe, cr_ui.dll + runtime; lib; include; docs;
  in-zip SHA256SUMS) — the existing shape (CI-CURRENT §4) — PLUS, from the first combined release:
  `rda.sys`, `rda.inf`, `install.ps1`, `uninstall.ps1` (the driver tier-mate, for single-point consumers
  like the installer), `MANIFEST.json`, `SHA256SUMS` (top-level, covering every asset).
- Verification: top-level SHA256SUMS; MANIFEST.json cross-check (asset names, versions, source tier,
  consumed-vs-built flag for driver binaries); then in-zip per-file check for the core zip.

**Failure behavior (mandate: fail with a diagnostic, never a silent fallback):** missing required asset,
hash mismatch, MANIFEST/asset disagreement, or latest-below-floor → the workflow step exits non-zero with
the named missing artifact and the resolved release tag. No try-older-release. No skip.

## 3. The new workflow set

### 3.1 `ci.yml` — pre-merge validation lane (the local mirror; builds from source by charter)

Jobs (all consume the repo, none consume releases; this is the ONLY from-source lane besides release):

1. `local-mirror` — Windows runner: checkouts and runs **`ci-local.ps1`** (the exact entry point
   developers run; PLAN §5). If the local gate cannot reproduce on the runner image, it fails —
   that is a runner-vs-developer environment difference to be diagnosed, not retried (mandate §6).
2. `linux-unit` / `linux-tsan` / `linux-ubsan` — the three sanitizer lanes as today (they validate the
   source; win-stub lane), each with ctest.
3. `fuzz-smoke` — the existing fuzz lane, unchanged in scope.
4. `windows-release-build` — ONE MSVC /W4 /WX Release build + full ctest (the double Debug+Release
   build dies; the validation subject is the source, one configuration suffices, the release workflow
   rebuilds from tag anyway).
5. `abi-check` — ui_abi_check.c against the built cr_ui.dll (symbol set pinned; catches accidental
   ABI drift before release).

Ordering: `local-mirror` first (fail-fast); heavy lanes after. Path filters on `pull_request` are added
(docs-only PRs skip the build lanes — documented).

### 3.2 `driver-install.yml` — the prebuilt driver lane (no build; HISTORICAL — folded into test-benchmark.yml)

windows-2022. Steps: resolve latest rda-driver release by convention (§2) → download + SHA256SUMS-verify →
test-sign with a job-local cert (the api-benchmark mechanism, verbatim: New-SelfSignedCertificate
CodeSigningCert → LocalMachine\Root + TrustedPublisher → signtool sign /fd SHA256 — proven on windows-2022
without test-signing mode by runs 40227375329/40315255368) → `install.ps1 -ServiceName rda -Source rda.sys`
→ `sc.exe start rda` → **verify via the core's driver-status surface**: fetch the latest CORE release by
convention (§2), stage rrecov.exe (+runtime from the zip), run `rrecov driver --json` and assert
`state == Ready` with all facets true and the version fields sane → uninstall (always) → upload evidence
(driver-info.json equivalent: the --json output + install log + resolved tag). The workflow produces no
artifact other than the evidence bundle.

### 3.3 `core-install.yml` — the prebuilt core lane (no build; HISTORICAL — folded into test-benchmark.yml)

windows-2022 (and `windows-latest` as a secondary matrix cell — documents where the runner supports it).
Steps: resolve latest core release by convention → download + SHA256SUMS + MANIFEST cross-check → unzip →
stage → smoke the staged binaries with NON-disk operations only (per the driver-era plan §1 classification):
`rrecov list`, `rrecov sessions`, `rrecov driver` (expected: exit 4 ABSENT — the driver is not installed in
this workflow; this doubles as the refusal-contract regression probe) → upload evidence (resolved tag,
SHA256 outputs, smoke logs). No artifact other than staged-binaries evidence.

### 3.4 `staged-scenarios.yml` — the truth matrix (no build; HISTORICAL — folded into test-benchmark.yml + benchmark-matrix.json)

windows-2022. Job graph: `setup` (installs driver per §3.2 mechanics + stages core per §3.3, uploads a
`env-manifest.json` recording runner image, toolchain versions, resolved tags, asset hashes) →
`scenario-*` matrix jobs (`needs: setup`, each re-fetches the SAME resolved tags — pinned by the setup
job's manifest so a mid-run release cannot skew the matrix).

Scenario set v1 (docs/CI-SCENARIOS.md carries the full matrix with per-scenario "what this proves"):

| ID | Filesystem | Staging | Deletion | Proves |
|---|---|---|---|---|
| S-DEL-NTFS | NTFS | VHD (diskpart create vdisk, fixed) + seeded manifest | `Remove-Item -Force` (bypasses recycle bin) on a recorded subset | deleted-file recovery truth on the primary FS: every expected file found, recovered, hash-verified |
| S-DEL-FAT32 | FAT32 | same | same | the FAT family's chain/LFN recovery |
| S-DEL-EXFAT | exFAT | same | same | exFAT dentry recovery |
| S-FMT-NTFS | NTFS | seeded, then `Format-Volume -Force` (quick) | quick-format | post-format recovery (the og's formatted-drive scenario, UIUX/08) |
| S-DRV-REFUSE | n/a | driver NOT installed (phase 1) / stopped (phase 3) | n/a | the refusal contract: exit 4 + exact stderr class per condition, zero scan output; then installed → Ready + scan proceeds (phase 2) |
| S-IMG-CREATE | NTFS | staged disk + `image --create` | n/a | the new image-create op round-trips: device hash == image hash |

Each scenario job: (1) create+attach VHD, (2) initialize/format, (3) seed files per the versioned seed
manifest (sizes: 0/1/512/513/64 KiB+1 bytes; categories: doc/jpg/png/mp4/pdf/zip/unknown; attributes:
readonly/hidden; names: ASCII/long/Unicode; depths: shallow/deep), recording SHA256 per file;
(4) run the scripted deletion method (the script is versioned in the repo; the expected-recoverable subset
is derived from manifest + deletion semantics — never from either tool's output);
(5) `rrecov scan --disk <N> --json` against the staged disk; (6) compare found vs. expected
(expected-vs-observed report, JSON); (7) `rrecov recover` to a fresh destination volume;
(8) hash-verify every recovered file vs. the manifest; (9) upload the evidence artifact
(logs, expected-vs-observed, hashes, seed manifest, env-manifest).

Evidence artifacts are the acceptance proof per scenario (mandate §4). The matrix is versioned in
docs/CI-SCENARIOS.md; scope = the filesystems and deletion modes the reference product supports and our
engines claim (CAPABILITY-MATRIX cross-ref).

### 3.5 `release.yml` — the combined release (the sole source of builds)

Trigger: tag push `vX.Y.Z` (semver; docs/RELEASE.md documents the convention + the
"release on every green milestone" cadence policy verbatim).

Jobs:

1. `validate` — the FULL pre-merge validation suite (§3.1's lanes, all of them) against the tagged commit.
   Red here = no release (mandate: no release without green).
2. `build-core` — needs validate: the one from-scratch MSVC Release build from the tag; assembles the
   core artifact set: `rrecov.exe`, `cr_ui.dll` (+ vcpkg/CRT runtime), `.lib`s, `include/`, docs,
   PDB symbols, in-zip SHA256SUMS → `core-recovery-<tag>.zip`.
3. `driver-tier` — needs validate: resolve the rda-driver release for the compatibility tier
   (tier mapping in docs/RELEASE.md: core 1.x ↔ driver wire major 1 → latest rda-driver `v*` release;
   the mapping is a documented table, not code). If present: download + verify (consumed path — the
   DEFAULT). If missing: **build the driver from source at the tier** (checkout heicfast/rda-driver at
   the tier tag, MSVC/WDK build via its own build docs) — the mandate's fallback, used only when the
   prebuilt tier release does not exist; every such use is stamped `built-from-source` in the manifest.
4. `assemble` — needs [build-core, driver-tier]: MANIFEST.json (every asset: name, version, SHA256,
   source tier, consumed-vs-built), top-level SHA256SUMS, release notes referencing MANIFEST.json +
   PARITY-DELTAS.md, signing posture stated (test-signed driver artifacts, unsigned core binaries —
   this session's posture, verbatim in the notes).
5. `publish` — needs assemble: `softprops/action-gh-release@v2` with the full asset set.
   The release IS the substrate CI consumes (§2); no external schedule gates it.

## 4. Local validation — `ci-local.ps1` / `ci-local.sh` + hooks (mandate §6)

Single entry points at repo root, running every CI check in the same order with the same strictness:

| # | Component | Windows (`ci-local.ps1`) | Unix (`ci-local.sh`) | CI mirror job |
|---|---|---|---|---|
| 1 | format check | clang-format --dry-run -Werror (repo .clang-format) | same | local-mirror |
| 2 | lint | (repo has no clang-tidy config yet — placeholder refuses to silently pass: reports "not configured" as INFO, not success) | same | local-mirror |
| 3 | warnings-as-errors build | cmake --preset win-x64-release + MSVC /W4 /WX | g++ -Wall -Wextra -Werror (the Linux lane's flag set from ci.yml) | windows-release-build / linux-unit |
| 4 | static analysis | (cppcheck absent from repo toolchain — same honest-not-silent posture as #2) | same | local-mirror |
| 5 | unit tests | ctest (win preset) | ctest (Linux lane incl. the win-stub path) | linux-unit |
| 6 | contract tests | the ctest suite includes the cr_ui ABI + contract tests (ui_abi_check) | same | abi-check |
| 7 | dependency pinning | vcpkg.json manifest mode = pinned baseline; the script fails on floating version detection (manifest-mode本身就是 pin; any classical-mode drift fails) | same | local-mirror |
| 8 | doc link check | relative links in docs/**.md resolve to tracked files | same | local-mirror |
| 9 | secrets scan | diff vs origin/main grepped for token patterns (`ghp_[A-Za-z0-9]{36}`, `github_pat_`, AKSK shapes, `-----BEGIN.*PRIVATE KEY-----`) | same | local-mirror |
| 10 | commit message check | conventional-commit regex (`type(scope): lowercase …` — the repo's evidenced convention, CI-CURRENT §8) on HEAD..origin/main | same | local-mirror |

Every failure prints the exact files/issues + the fix command. A fresh contributor runs it once and knows.

**Hooks:** `.githooks/pre-commit` (fast subset: #8/#9/#10 on staged content — seconds, not minutes),
`.githooks/pre-push` (full entry point). Install: `git config core.hooksPath .githooks` (documented in
docs/CONTRIBUTING.md as a one-time setup; CI's local-mirror asserts the hook files exist in-tree).
`--no-verify` is a contributor violation, stated verbatim in CONTRIBUTING.md.

**docs/CI-LOCAL.md** carries the component→CI-job mapping table (above) so the mirror is auditable.
**docs/BUILD-SUPPRESSIONS.md** starts EMPTY — no suppression exists; any future one needs a row
(rationale + scope + deferred-fix tracking).

## 5. Doc set this plan ships

`docs/PREBUILT-ARTIFACTS.md` (§2), `docs/CI-SCENARIOS.md` (§3.4 matrix), `docs/RELEASE.md`
(§3.5 conventions + cadence policy + tier mapping), `docs/CONTRIBUTING.md` (rewrite: local gate + hooks +
never-push-red), `docs/CI-LOCAL.md` (§4 mapping), `docs/BUILD-SUPPRESSIONS.md` (empty), plus updates to
`docs/benchmarking.md` (the wiped-workflow drift CI-CURRENT §7 documented), `docs/CAPABILITY-MATRIX.md`,
and README's CI section.

## 6. Acceptance criteria

1. `comparative-bench.yml` and standalone `api-benchmark.yml` deleted; their driver-install mechanics
   live in `.github/actions/install-driver/action.yml` (shared composite) — grep proves no correctness
   workflow builds the core or driver from source (only ci.yml lanes and release.yml do, each documented
   by charter).
2. (historical acceptance criteria; the 4-lane equivalents are green) driver-install.yml + core-install.yml + staged-scenarios.yml green on windows-2022, each consuming
   releases by the §2 convention, each uploading evidence artifacts; a missing-asset drill proves the
   loud-failure path (workflow_dispatch input to point at a bogus tag).
3. release.yml produces a combined release (MANIFEST.json + SHA256SUMS + core zip + driver assets +
   truthful notes) gated by full validation; at least one tag ships and the install workflows consume it.
4. `ci-local.ps1`/`ci-local.sh` pass on a fresh checkout (both lanes); pre-commit/pre-push hooks
   installed via the documented one-liner; CONTRIBUTING.md + CI-LOCAL.md current.
5. Every workflow records runner image + toolchain versions + consumed artifact tags in its evidence
   artifact (env-manifest pattern).
