# CI-CURRENT — the 3-lane Windows-only CI (the 2026-10-28 consolidation)

**Supersession note:** this document describes the CURRENT lane set. The
previous 7-job ci.yml + 5-workflow era (local-mirror / linux ASan / TSan /
UBSan / windows / fuzz / codeql + core-install / driver-install /
staged-scenarios) is retrievable from git history. The ubuntu sanitizer and
fuzz lanes were dropped by the owner's windows-only directive — a
documented decision, not an accident. The pure core keeps that coverage
locally via `./ci-local.sh --san` / `--fuzz`; `ci-local.ps1` remains the
developer entry point and its 10 components remain the local gate
(docs/CI-LOCAL.md).

## The three lanes

| Lane | File | Trigger | Runner | Timeout | Purpose |
|------|------|---------|--------|---------|---------|
| **Checks** | `.github/workflows/checks.yml` | push/PR to main (docs-path-filtered, symmetric) + `workflow_call` | windows-2022 | lint 15 / build 90 | the pre-merge gate: format, lints, /W4 /WX build, full ctest, Win7 posture, G-09 schema, driverless refusal probes |
| **Release** | `.github/workflows/release.yml` | tag push `vX.Y.Z` + dispatch (publish/drill) | windows-2022 | 90/30/30 | the sole source of builds: validate (calls Checks) -> build-core + driver-tier -> assemble + publish; test-dev-signs rda.sys with the persistent dev cert |
| **Test & Benchmark** | `.github/workflows/test-benchmark.yml` | dispatch (release dispatches it on every publish) | windows-2022 | 120 smoke / 180 full | consumes the LATEST release: driver load+verify, simulated disks, the scenario matrix through CLI **and API**, gates + perf + structured evidence |

The chain: **commit -> Checks (green) -> Release (publish) -> Test &
Benchmark (automatic dispatch against the fresh tag)**.

## Checks lane components (the ci-local.ps1 mirror + CI-owned gates)

1. clang-format `--dry-run -Werror` over src/include/tests (component 1)
2. N-041 dead-surface lint + driver-only architecture lint (component 2)
3. vcpkg.json manifest presence (component 6)
4. PowerShell **AST parse gate** over every CI-consumed script (the S-036
   lesson — a PS parse-time crash in install.ps1 broke every install for an
   era behind a fallback)
5. Workflow **YAML parse gate** (the stray-brace class killed the old
   benchmark.yml across 5 dispatches)
6. MSVC /W4 /WX Release build (`win-x64-release` preset, vcpkg manifest)
7. Full ctest (all GoogleTest suites + the `cr_ui_abi_check` C consumer)
8. Win7 posture gate (stamp + verify allowlist on rrecov.exe, cr_tests.exe,
   cr_ui.dll — stamp BEFORE verify, always)
9. G-09 report-schema validation (`tools/validate_report.py` on the
   temp-dir sample written during ctest)
10. Driverless refusal-contract probes (this lane's owned regression): bare
    `rrecov` and `list` / `list-carve-formats` exit 0; `driver --json`
    exits **4** with `"state":"absent"`; `scan --disk 0` exits **4** with
    the driver-required refusal and NOTHING past the gate. Every probe
    that asserts a non-zero exit ends with the script's own explicit
    verdict (three dispatches historically died to a missing `exit 0`).

**Efficiency:** vcpkg binary caching keyed on `vcpkg.json` (the old lane
rebuilt zlib/gtest/lz4/zstd from source EVERY run — the largest fixed
cost), `paths-ignore` symmetric on push+PR, concurrency
cancel-in-progress, lint fast-fails before the build job spends compute.

## Release lane contract

- **No release without green**: `validate` calls checks.yml via
  `workflow_call` on the released commit; `skip_validation` is DRILL-ONLY.
- Identity: tag push publishes that commit; dispatch resolves the tag
  (explicit `version` input or auto next-patch from the latest release),
  asserts it does not already exist, builds the exact SHA.
- Driver tier: fetched prebuilt from `heicfast/rda-driver` with
  `-FloorVersion v0.2.4` (ONE pin source; the versioned-layout era the
  teardown contract assumes). Source-build fallback only when no tier
  release exists — stamped `built-from-source` in MANIFEST.json.
- **Signing posture (new)**: rda.sys is test-development-signed at release
  time with the repo's PERSISTENT dev certificate
  (`DEV_SIGN_CERT_PFX_B64` / `DEV_SIGN_CERT_PASSWORD` secrets,
  CN=RRECOV-DEV-SIGN, 3-year validity). The public cert ships as the
  release asset **`rda-test-sign.cer`** — any consumer establishes trust
  by importing it into `LocalMachine\Root` + `TrustedPublisher`; no secret
  needed at consume time. Core binaries stay unsigned (byte-comparable
  builds). The signature is verified in-lane (Get-AuthenticodeSignature ->
  Valid) before anything ships.
- Assets: `core-recovery-<tag>.zip` + `rda.sys` (signed) + `rda.inf` +
  `install.ps1` + `uninstall.ps1` + `rda-test-sign.cer` + `MANIFEST.json`
  + `SHA256SUMS` (self-exempt index files, as always).
- Publishing dispatches **test-benchmark.yml** with `core_tag=<tag>`.

## Test & Benchmark lane contract

Everything runs from the **prebuilt release** (fetch-release.ps1 enforces
SHA256SUMS + MANIFEST; never builds, never falls back). Sequence:

1. environment manifest + elevation assertion + work-drive probe (free
   space; never a hardcoded runner path)
2. release fetch (`rda-test-sign.cer` is a REQUIRED asset — old-pipeline
   releases fail loudly with the remediation)
3. core staging (app-local PATH) + CLI smoke probes
4. driver install (`.github/scripts/Install-ReleaseDriver.ps1`): cert
   trust -> signature assert -> AST-gated `install.ps1` -> sc qc +
   Parameters assertions (M-060) -> idempotent start (M-068) ->
   `rrecov driver --json` = ready with every facet green
5. API harness compiled against the release SDK (vswhere -> vcvars64 ->
   `cl /W4 /WX` against `include/` + `lib/cr_ui.lib`, exe beside
   `cr_ui.dll`); ABI/driver-status probes
6. `.github/scripts/Run-Benchmark.ps1` executes
   `.github/scenarios/benchmark-matrix.json`:
   - **disks**: 3 fixed VHDX (2 GiB MBR FAT32 / 5 GiB GPT NTFS+exFAT
     dual-partition / 8 GiB GPT NTFS), diskpart-only, ~15 GiB total
   - **corpus**: `bench/seed/generate_seed.ps1` (23 real-magic formats,
     CJK names, >260-char paths, ADS, hard links, sparse, fragmented,
     depth-6 nesting, many-small, keep-control group; deterministic PRNG)
   - **feature probes**: health, fix dry-run, user-signs, CLI-vs-API carve
     parity, cancel->save->resume session probe
   - **clean-volume benchmark**: per-mode scan timings, driver bench verb
     (info/seq/rand/open), API lane timings, process sampling
   - **scenarios**: permanent_delete (NTFS/exFAT/FAT32), recycle_bin,
     folder_delete (bin emptied), quick_format, delete_partition (+ the
     lost-partition APPLY probe), and the full-profile decay_50 /
     partial_overwrite / mft_corrupt — each: deterministic reseed ->
     apply-scenario -> CLI lane (run-ours.ps1) -> API lane (api_harness)
     -> inventory + metrics -> gate -> evidence -> payload cleanup
   - **gates**: per-scenario policy from the matrix — `floor` (hash-recall
     vs a floor derived from bench/baseline-candidate.json with headroom),
     `discovery` (lost partition found + recover completed),
     `observe` (known gaps recorded ungated — FAT32 delete, quick_format;
     see bench/root-causes.json). Ground truth is ALWAYS the manifest +
     deletion method, never a tool's own output; extra finds are recorded,
     never failures. The transport assertion is a hard rule: with the
     driver running, every scan must show `[transport] RDA driver path`.
7. teardown ALWAYS: the release's own uninstall.ps1 with the M-061
   post-state contract (service gone + versioned image gone), cert
   cleanup, VHD detach + delete
8. the structured evidence artifact (00-environment .. 99-teardown)
   uploads ALWAYS

## Files

- `.github/workflows/{checks,release,test-benchmark}.yml` — the lanes
- `.github/scripts/fetch-release.ps1` — the release convention enforcer
  (unchanged from the previous era)
- `.github/scripts/New-TestDisk.ps1` — matrix disks (diskpart-only)
- `.github/scripts/Install-ReleaseDriver.ps1` — trust + install + verify
- `.github/scripts/Run-Benchmark.ps1` — the orchestrator
- `.github/scripts/Teardown-Benchmark.ps1` — the always-run teardown
- `.github/scripts/api_harness.c` — the API-lane driver (cr_ui C ABI)
- `.github/scenarios/benchmark-matrix.json` — the versioned truth
- `bench/` — the seed/scenario/metrics tooling (unchanged; now wired into
  CI through the orchestrator instead of the wiped comparative lanes)

## The carried-over failure lessons (do not regress these)

Fail loud, never mask with a fallback (S-036) · idempotent driver start,
1056-after-install is success (M-068) · uninstall via the release's own
script + post-state asserts (M-061/M-069) · hashtable splats, never array
splats (M-069) · AST-parse every dispatched script · probe steps end with
their own exit 0 · one pin source per dependency · caller permissions
superset of the called workflow · probe runner layouts by content (VS/UCRT)
· app-local runtime DLLs + PATH prepend (0xc0000135) · explicit timeouts
everywhere · evidence artifacts `if: always()` with env-manifests · derive
truth from manifests, never tool output · record skips, never silence them.
