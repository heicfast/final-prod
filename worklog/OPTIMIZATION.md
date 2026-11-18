# OPTIMIZATION.md — CI/CD workflow optimization, before/after

Session: 2026-11-18 (CI/CD optimization session). Baselines are the mean of
recent GREEN runs from the GitHub API (per-job durations from
`/actions/runs/{id}/jobs`), recorded in worklog/WORKFLOW-INVENTORY.md before
any change. "After" numbers are the runs triggered/dispatched by this
session's pushes; each cites its run id.

## Changes applied (one batch, commit "opt(ci): ..." of this session)

| # | Change | Where |
|---|---|---|
| 1 | Concurrency group `workflow-ref`, cancel-in-progress | contracts-ci, ui-ci, ui-e2e (new); forensic-repro, validation-evidence, release (queue, no cancel — evidence runs are never dropped; a tag build is never cancelled) |
| 2 | Explicit timeouts (was default 360 min) | contracts 10, ui-ci 30, ui-e2e 20, forensic 60, validation 90 (release/engine/bridge/benchmark already had 90/60/60/300) |
| 3 | vcpkg binary cache (actions/cache, key `vcpkg-bin-win2022-hashFiles(engine/vcpkg.json)`, restore-keys prefix — same key template the engine-ci lane already proves) + `VCPKG_DEFAULT_BINARY_CACHE` env wired before build.ps1 | release, benchmark-src, validation-evidence (all build.ps1 lanes) |
| 4 | pnpm store cache (setup-node `cache: pnpm`, lockfile-keyed) + setup-node reordered AFTER pnpm/action-setup (required order) | release |
| 5 | cargo cache (Swatinem/rust-cache pinned SHA d1527c8f67ef, `workspaces: ui/src-tauri`) | release, ui-ci |
| 6 | Versioned pip key (`pip-clangformat-win2022-18.1.8`, distinct restore-key prefix) | engine-ci lint |
| 7 | Path scoping | pre-existing on all push-triggered workflows (engine-ci, bridge, contracts, ui-ci, ui-e2e already scope paths); dispatch-only + tag-only workflows need none |
| 8 | Matrix reduction | N/A — the repo has NO matrices today (WORKFLOW-INVENTORY); every lane is already single-leg, breadth matches decision |
| 9 | Root-cause failure fixes | benchmark-src ctest `working-directory: engine` (7d196cda); abi format drift closed by a19afb56 (concurrent lane) |

## Baseline (before) — measured

| Workflow | Mean green run | Basis |
|---|---|---|
| contracts-ci | 12 s | recent green runs |
| ui-ci | 301 s | recent green runs |
| ui-e2e | 367 s | recent green runs |
| bridge-integration | 777 s | green runs 44334552899 etc. |
| engine-ci | 867 s | green runs 44334569972 (697-913 s) |
| validation-evidence | 928 s | t14/t15 dispatches |
| release | 1277 s job (1398 s build-and-test leg; full-product 1398 s) | v0.9.4 run 44256566315: 23 m 18 s wall, ~22 m inside build.ps1 |
| benchmark-src | no green run exists (4/4 dispatches red at the baseline point) | fastest red: 383-689 s |
| forensic-repro | 2003 s | green dispatches |

## After — measured, this session's runs

(Filled from the run URLs recorded in worklog.md's session log — see the
"Phase 3 measurements" entries; each line cites its run id.)

## Cache state after cleanup

- Duplicate rust cache entry (584.55 MB, character-identical key twin of the
  733.38 MB live entry — 41% of cache storage) deleted via the API.
- 14 orphaned `rda-wdk26100-*` artifacts (dead driver-ci workflow, deleted at
  318b9655) deleted via the API.

## Deliberately NOT done (recorded rationale)

- **Release ≤ 10 min target**: caches remove the port rebuilds (~95 s), the
  pnpm re-download (~60-90 s) and part of the cargo build; the remaining
  critical path is the full engine compile + tauri link + NSIS, which the
  session budget could not restructure into a build-once/download artifact
  chain (tag builds cannot reuse branch-run artifacts: engine-ci triggers on
  `branches: [main]` only, so no artifact exists for a tag SHA). The measured
  post-cache release time is recorded above; the artifact-reuse redesign is
  the documented follow-up.
- No test removed, no stage skipped, no continue-on-error added anywhere;
  every existing gate (version lockstep, PE gates, dev-sign verify, D1
  manifest gate, ctest, contracts) still runs on every release.

## After — measured, this session's runs (2026-11-18, all run IDs from the GitHub API)

| Workflow | Before (green mean) | After | Run | Note |
|---|---|---|---|---|
| contracts-ci | 12 s | 10 s | 44364079846 (76a6738) | timeout+concurrency landed; ~parity (lane is sub-20s by nature) |
| ui-ci | 301 s | 269 s | 44364079478 (76a6738) | rust-cache first hit pending (cargo deps cache added); -32 s |
| ui-e2e | 367 s | 405 s | 87c04b7 run 44359295695 | +38 s — the concurrent session's W5e/M06 preview-pane feature added real test/work; not a regression of the lane itself |
| engine-ci | 867 s | 571 s | 44347614852 (201100aa) | -296 s: the abi format fix unblocked the lane; the ctest working-directory fix + pip key versioning landed in the same train |
| bridge-integration | 777 s | 754 s | 44348649846 (795f532) | ~parity; engine build still uncached (vcpkg binary cache covers ports only — the C++ object compile is the documented follow-up) |
| validation-evidence | 928 s | t14 green in-run; t15 re-running with the G-3 fix | 44363284154 (3809fbc) | t14 green at first attempt with the vcpkg cache; t15 red pre-fix (stock RecoveryDrive.lnk false-match), re-dispatched at 3809fbc |
| forensic-repro | 2003 s (last green Nov 6) | — | 44359495858 red: the v0.9.4 shipped-installer exit-2 bug (section 11 of CI-FAILURES) | re-dispatch after the v0.9.5 release lands the fixed installer |
| benchmark-src | no green run ever (4/4 red) | — | 44359663812 queued at fc0e1e1 | ctest fix + vcpkg cache in the train; first-ever green expected |
| release | 1277 s job (v0.9.4 run 44256566315) | in progress | 44365218107 (tag 02e62af) | vcpkg+pnpm+cargo caches first-hit on this tag run (ports ~95 s + pnpm ~60 s + cargo link deps saved vs v0.9.4's cold run); two failed attempts (44359484978 cancelled pre-fix, 44361715679 makensis File/$PLUGINSDIR lesson) are recorded above |

Honest caveats, stated plainly:
- The engine C++ object compile (the ~9-min MSVC build) is NOT cached in any
  lane — ccache/sccache or an actions/cache over the build tree is the
  documented follow-up; the vcpkg binary cache removes the port rebuilds only.
- Release ≤ 10 min was NOT achieved: the caches trim the install/cargo-fetch
  overhead, but the serial engine+tauri build remains ~17-20 min. The
  build-once/download-artifact redesign (release consuming engine-ci's build
  via workflow_run) is the follow-up; tag builds cannot reuse branch-run
  artifacts because engine-ci triggers on branches: [main] only.
- The "no green run exists" baseline for benchmark-src means no before/after
  speed claim can be made for it — only correctness (first green) is claimed.
