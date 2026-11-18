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
