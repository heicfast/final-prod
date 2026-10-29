# CI-LOCAL — the local validation entry point and its CI mirror

> **CI-shape note (M-088, 2026-10-29):** the CI this document mirrors is
> the 4-lane windows-only set (checks / release / test-benchmark /
> installer — docs/CI-CURRENT.md). Where the tables below still name the
> pre-consolidation lanes (ci.yml's local-mirror job, driver-install /
> core-install / staged-scenarios), read them as the historical mapping
> the local gate was built against; the component-level contract (what
> the local gate proves) is unchanged.

> The never-push-a-red-commit rule (docs/CONTRIBUTING.md) needs ONE local
> entry point that runs every check CI runs, in the same order, with the
> same strictness. `./ci-local.sh` (Unix) and `.\ci-local.ps1` (Windows) are
> it. The mapping below is the auditable contract between the local gate and
> the CI jobs.

## The entry points

| | |
|---|---|
| `./ci-local.sh` | Unix: the full default gate; `--san` adds TSan/UBSan; `--fuzz` adds the smoke campaign; `--all` = everything the Linux CI matrix runs |
| `.\ci-local.ps1` | Windows: the full default gate (the exact script the CI `local-mirror` job runs); `-Fast` = the pre-commit subset (format/lint/secrets/commits only) |

Both exit 0 (green — push) or 1 (red — fix first; every failure prints the
exact files/issues and the fix command).

## Components → CI mirror

| # | Component (local) | Strictness | CI mirror job |
|---|---|---|---|
| 1 | clang-format `--dry-run -Werror` over src/include/tests | the repo `.clang-format` | `local-mirror` (and the old lint job's toolchain, now inside the mirror) |
| 2 | Lint: the N-041 dead-surface grep + the **driver-only architecture lint** (the removed transport primitives `rda_available` / `win_pe_environment` / `open_harddisk_volume` / `prefer_rda` must never return to production code) | exact greps, evidence-cited | `local-mirror` |
| 3 | Warnings-as-errors build — gcc `-Wall -Wextra -Werror -Wshadow` / MSVC `/W4 /WX` (global in `CMakeLists.txt`; a preset build IS the Werror build) | identical flags | `windows` (MSVC) / `linux` (ASan) |
| 4 | Unit + contract tests — the full ctest suite (engines, orchestrator incl. the scan-truth pins, cr_ui ABI lane, win-stub syntax gate) | same presets | `windows` / `linux` / `linux-tsan` / `linux-ubsan` |
| 5 | Repo truth self-tests — G-09 report schema validation + the Win7 PE gate self-test (`tools/test_verify_pe_um.py`) | identical scripts | `linux` |
| 6 | Dependency pinning — vcpkg.json manifest mode (the repo's pinning policy) | manifest present + valid JSON | `local-mirror` |
| 7 | Documentation link check — relative `.md` links in docs/ + README resolve | tracked files only | `local-mirror` |
| 8 | Secrets scan — the working diff vs `origin/main` for token/key patterns (`ghp_`, `github_pat_`, AKSK, private keys) | exact patterns | `local-mirror` (and `.githooks/pre-commit` on staged content) |
| 9 | Commit message convention — `type(scope): lowercase …` on `origin/main..HEAD` | the repo's evidenced convention | `local-mirror` |
| 10 | Hooks path — `core.hooksPath = .githooks` (pre-commit/pre-push active) | presence check | `local-mirror` (in-tree `--check`) |

Static analysis (clang-tidy / cppcheck) is **not configured in this repo
yet** — the gate reports it as an INFO line, never a silent pass; when a
configuration lands, it joins this table with the same local-then-CI
mirroring rule. Suppressions: docs/BUILD-SUPPRESSIONS.md is the registry — it carries
the ONE registered row (S-1, the vendored stb_image.h block); there are
no other suppressions anywhere (Session-84 corrected this page's stale
'zero rows' echo).

## The hooks

| Hook | Runs | Install |
|---|---|---|
| `.githooks/pre-commit` | the FAST subset: staged-secrets scan + staged-file format check (seconds) | `git config core.hooksPath .githooks` (one-time, documented) |
| `.githooks/pre-push` | the FULL gate (`ci-local.sh` / `ci-local.ps1` dispatch) | same |

CI's `local-mirror` job asserts the hook files and entry points exist
in-tree — the `--check` the mandate asks for. Bypassing with `--no-verify`
is a contributor violation (CONTRIBUTING.md).

## The mirror rule

The CI's **first** job (`local-mirror`) runs `ci-local.ps1` verbatim on the
runner image — CI does not start with "build everything"; it starts with
"reproduce the local gate". If the local gate passes locally but fails on
the runner, that is a runner-vs-developer environment difference to be
diagnosed and closed — never silently retried. The heavy lanes (Windows
build, sanitizers, fuzz, release, driver-install, scenarios) run after the
mirror gate.

## Cost documentation (which builds remain from source, and why)

| Lane | Builds from source? | Charter |
|---|---|---|
| `ci.yml` validation lane | YES (one MSVC Release config + the Linux/sanitizer lanes) | its subject IS the source being merged (Werror/sanitizer/unit truth) |
| `release.yml` | YES (the core, from the tag) | the release is defined by the tag — the sole source of builds |
| `test-benchmark.yml` / `installer.yml` (historically driver-install / core-install / staged-scenarios) | **NO** (test-benchmark consumes the prebuilt combined release; installer builds ONLY the installer's own subject from source — its charter) | they consume prebuilt releases by the documented convention (PREBUILT-ARTIFACTS.md) |
