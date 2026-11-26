# Session prompt — Comprehensive Fix, Upgrade & Release (2026-11-26)

Saved token-redacted at session start (T1). Redacted: the GitHub token
(kept in env/git config only, never committed — replaced below with
`[REDACTED]`).

---

Senior systems engineer (25+ years), build systems, CI/CD, dependency
management, vulnerability remediation, release engineering.

Make the product fully clean and release-ready: fix every build error,
compile warning, lint finding, deprecation, failed API call, bug, and
vulnerability. Upgrade every dependency, toolchain, and platform
component to the latest stable. Make every workflow green for the right
reason. Cut a new release with a new tag, validated end to end on a
clean Windows machine.

Repos:
- Product (target): https://github.com/heicfast/final-prod.git
- Token (env/git config only, never committed): [REDACTED]

Read the entire repository, every workflow, every build file, every
dependency manifest, and every current CI failure end to end before
touching anything. Every claim is evidenced. Every fix is proven. No
shortcut, no fake green.

## Phase 0 — Parallel read and baseline

Agents (mandatory):

- A: full repository state — every source module, build file, dependency
  manifest (Cargo.toml, package.json, pnpm-lock.yaml, CMakeLists.txt,
  .csproj, vcpkg.json, NuGet), every workflow under .github/workflows/,
  every doc. → worklog/REPO-STATE.md
- B: every current CI failure on the default branch — fetch the full run
  logs, capture the exact error per job and per step. → worklog/CI-FAILURES.md
- C: every build/compile warning and deprecation across every compiler,
  analyzer, and linter in the configured strictness. → worklog/WARNINGS.md
- D: every dependency's current version vs the latest stable, including
  the toolchain (Rust, Node, .NET, MSVC, CMake), action versions, and any
  vendored library. → worklog/DEPENDENCIES.md
- E: vulnerability scan — cargo audit, npm audit / pnpm audit, OWASP
  Dependency-Check, osv-scanner, CodeQL, and any other scanner the
  repository uses. Catalog every finding with severity. →
  worklog/VULNERABILITIES.md

Consolidate → push → only then fix.

## Phase 1 — Fix every error, warning, and deprecated usage

- Build errors: every target on every configured platform and
  configuration builds cleanly. Fix at the root cause. No suppressed
  errors.
- Compiler warnings: zero warnings at the configured strictness (MSVC
  /W4 /WX, Clang -Wall -Wextra -Wpedantic -Werror, Rust cargo clippy
  -D warnings, TypeScript strict + noEmit, ESLint --max-warnings 0).
  Every suppression is narrowly scoped, with a rationale, recorded in
  worklog/SUPPRESSIONS.md.
- Deprecated code: replace every deprecated API with its current
  replacement. If no replacement exists, narrowly suppress with
  rationale and document the plan.
- Lint: every configured linter passes with zero findings. Formatter
  applied as an isolated commit.
- Failed API calls: instrument and fix every runtime API call that
  fails. Every error path handled, propagated, and logged. No silent
  swallow.
- Known bugs: every reproduction captured, fixed at the root cause,
  proven with a regression test.
- Failed tests: every failing test diagnosed and either fixed (root
  cause) or deleted with a rationale in the commit message. Never
  skipped to hide a failure.

Every fix: commit message cites the defect, the file:line, the fix, the
verification.

## Phase 2 — Upgrade to latest stable

- Dependencies: every library upgraded to the latest stable. Read the
  changelog before upgrading — breaking changes must be adapted, not
  worked around. Pin exact versions.
- Toolchain: Rust, Node, .NET, MSVC, CMake, and every build tool
  upgraded to latest stable (verify against official releases).
- Actions: every GitHub Action upgraded to the latest major; pin SHAs
  for third-party actions.
- Runtime: WebView2, VC++ redistributable, and any other runtime bundled
  or required, at current stable.
- Platform targets: verify the upgrade does not break the supported
  Windows matrix. If a toolchain or library drops Win7 support (many
  have — .NET 5+, Rust MSVC, and modern compilers all dropped Win7), the
  product's stated support is updated truthfully and the installer
  validates accordingly.

Every upgrade: build green, test green, no behavioral regression.
Recorded in worklog/UPGRADES.md with before/after versions and every
adaptation.

## Phase 3 — Vulnerability remediation

- Every finding from worklog/VULNERABILITIES.md is fixed. Upgrade the
  vulnerable dependency, apply the patch, or replace the component.
- Where an upgrade is not feasible, document the compensating control in
  worklog/VULNERABILITIES.md with rationale and a plan to upgrade.
- Re-run all scanners after remediation — zero critical, zero high, and
  every remaining finding documented.

## Phase 4 — Workflows green

- Every workflow on the default branch green for the right reason.
- No continue-on-error. No skipped stage without a recorded, reviewed
  reason. No retry loop masking flakiness.
- Every previously-red job diagnosed and fixed at the root cause.
- Every workflow optimized: concurrency, path scoping, dependency
  caching, artifact reuse, tightened timeouts. Recorded in
  worklog/CI-OPTIMIZATION.md with before/after durations.
- Every job's timeout realistic; no 360-minute defaults.

## Phase 5 — Release

- Full release workflow dry-run on a pre-release tag: green end to end,
  artifacts produced, checksums correct, release notes accurate.
- Cut a new release tag from the default branch. The tag triggers the
  release workflow end to end.
- Install the release artifact on a clean Windows machine (GitHub
  Windows runner or self-hosted). Launch the product. Exercise every
  major surface against seeded data. Uninstall cleanly.
- Capture every validation artifact in worklog/VALIDATION/RELEASE/.
- The release notes state accurately what the release contains, what
  platforms it supports, and any deferred item with precise scope.

## Rules

- No hallucination. Every claim is a captured measurement, a run log, or
  a source citation.
- No shortcuts. No test removed. No stage skipped. No warning silenced
  without documented rationale. No dependency pinned back to dodge a
  security fix.
- No masked failure. Every error propagates. Every failure is diagnosed.
  No retry loop, no continue-on-error, no flake masking.
- Root-cause only. No symptom patches.
- No regression. Every previously-working surface works after this
  session.
- No fake green. A green workflow is green because the work is correct,
  not because the check was silenced.
- Preserve audit trail.
- Token in env/git config only.
- Push after every coherent unit. Never force-push. main green at every
  push.

## Validation gates

1. Every target builds cleanly on every configured platform and
   configuration with zero warnings. Evidence: CI green + local build
   logs.
2. Every test passes. Evidence: CI matrix green.
3. Every linter passes with zero findings. Evidence: CI lint job green.
4. Every deprecated usage replaced or documented with a removal plan.
   Evidence: worklog/SUPPRESSIONS.md.
5. Every dependency at latest stable. Evidence: worklog/UPGRADES.md.
6. Every vulnerability with a fix is fixed; every remaining is
   documented. Evidence: worklog/VULNERABILITIES.md.
7. Every workflow on the default branch green for the right reason.
   Evidence: run links.
8. Release cut, release workflow green end to end, artifact validated on
   clean Windows. Evidence: worklog/VALIDATION/RELEASE/.
9. Docs updated to current state — no stale claims, no outdated version
   numbers, no removed-feature references.
10. worklog.md complete with every SHA, every run link, every
    measurement.

## Deliverables

1. Five Phase 0 context sheets, pushed before any code change.
2. worklog/CI-FAILURES.md with resolution per failure.
3. worklog/WARNINGS.md, worklog/SUPPRESSIONS.md — every warning and
   deprecation with its resolution.
4. worklog/DEPENDENCIES.md, worklog/UPGRADES.md — every dependency's
   before/after, every adaptation.
5. worklog/VULNERABILITIES.md — every finding, every fix, every
   remaining with rationale.
6. worklog/CI-OPTIMIZATION.md — before/after per workflow.
7. Every workflow green on the default branch for the right reason.
8. New release tag, release workflow green end to end, artifact
   validated on clean Windows. Evidence:
   worklog/VALIDATION/RELEASE/.
9. Every doc updated to current reality.
10. worklog.md — chronological handoff with every SHA, every run link,
    every measurement, every deferred item with precise scope.
11. All pushed with evidence-citing messages.

## Final reminder

- Fix at root cause. Every error, warning, deprecation, vulnerability,
  and failing workflow has a real cause. Find it, fix it, prove it.
- Upgrade to latest stable, not latest. Every dependency, toolchain,
  and action at the current stable release. Read the changelog. Adapt
  breaking changes. Pin exact versions.
- Green is earned. A green workflow is green because the code is
  correct, not because the check was silenced. No continue-on-error. No
  skipped test. No silenced warning without rationale.
- No hallucination. No shortcuts. No fake anything. Production grade
  only.
- The release is validated, not just tagged. Install on clean Windows,
  exercise, uninstall, capture evidence.
- Work long-horizon. Commit and push often. main green at every push.
- Save to worklog/session-fix-upgrade-release-prompt.md (token
  redacted), read twice, start T1.
