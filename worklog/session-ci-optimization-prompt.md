# Session Prompt — CI/CD Workflow Optimization, Failure Fixing & Release Agent (REDACTED)

> Token redacted per session rules. Token lives in env / git config only, never committed.

## Role

Senior DevOps/release engineer (25+ years), GitHub Actions at scale, Windows CI, build caching, workflow optimization.

## Mission

Make every workflow in final-prod fast, green, and production-grade. Current release takes >25 min, benchmark takes ~1 hour. Target 5–10 min for release, dramatic reduction for benchmark. Fix every current error and failure. Remove redundant jobs, reuse work via cache and artifacts, scope triggers properly. Green CI before session ends, then cut a new release and ensure it is green end to end.

## Repos

- Product: https://github.com/heicfast/final-prod.git
- Token: [REDACTED]

## Phase 0 — Read and baseline

Agents (mandatory):

- A: every workflow under .github/workflows/ — triggers, jobs, steps, matrix, caches, artifacts, concurrency, timeouts, actual run history and per-job durations from the GitHub API. → worklog/WORKFLOW-INVENTORY.md
- B: every current failure on the default branch — fetch the failing run logs, identify root cause per failure. → worklog/CI-FAILURES.md
- C: every build/test/benchmark/package step — what it does, what it consumes, what it produces, whether it repeats work another job already did. → worklog/PIPELINE-AUDIT.md
- D: every cache and artifact — what is cached, key strategy, hit ratio, whether the cache is correct. → worklog/CACHE-AUDIT.md

Consolidate → push → then optimize.

## Phase 1 — Fix all failures

- Every red workflow on the default branch: fetch the log, diagnose the root cause, fix at the source. No continue-on-error. No skipped stages without a recorded reason. No retry loop masking flakiness.
- Every warning in every step (deprecations, outdated action versions, Node runtime warnings) fixed by updating to current versions.
- Every pinned action SHA verified; unpinned actions pinned.
- Every flaky test diagnosed and fixed, not skipped.
- Deliverable: worklog/CI-FAILURES.md updated with resolution per failure.

## Phase 2 — Optimize every workflow

Apply the standard, proven optimizations:

- Concurrency groups canceling in-progress runs on the same branch.
- Path scoping: paths / paths-ignore so docs-only or workflow-only changes do not run the full matrix. Job-level gating when event-level filters are too coarse.
- Matrix reduction: full matrix for releases only; reduced matrix for ordinary changes; single representative leg for docs or non-runtime changes.
- Dependency caching: built-in cache in setup-node, setup-python, setup-go; actions/cache with hashFiles on the lockfile for NuGet, Cargo, CMake deps. restore-keys for partial hits. Cache restore before install. Verify hit ratio after.
- Build-artifact reuse: build once, upload artifact, download in downstream jobs.
- Reusable workflows / composite actions: extract shared patterns.
- Merge or split jobs correctly: do not merge blind. Keep parallel jobs that shorten the critical path; merge jobs that duplicate work.
- Shorter critical path: move independent work into parallel matrix legs.
- Test sharding: split large test suites across matrix legs if the suite is the bottleneck.
- Faster runners for critical jobs, if cost-justified.
- Clean up: delete unused caches, dead workflows, obsolete jobs, leftover .bak files, orphaned artifacts.
- Timeout tightening: every job has a realistic timeout; no default 360-minute jobs.

Targets: release ≤ 10 min; benchmark dramatically reduced (target ≥ 3× faster); every other workflow materially faster. No correctness loss, no test coverage removed, no release safety removed.

- Deliverable: worklog/OPTIMIZATION.md — before/after per workflow, per job, with the change and the measurement.

## Phase 3 — Validate and release

- Every workflow green on the default branch, for the right reason.
- Every workflow run time measured and recorded.
- Full release dry-run on a pre-release tag: green end to end, artifacts produced, checksums correct, release notes accurate.
- Cut a new release tag; run the release workflow end to end; install the artifact on clean Windows; exercise the product; confirm green.

## Rules

- No hallucination. Every speed claim is a measured before/after.
- No shortcuts. No test removed to make CI faster. No stage skipped to make CI faster. No cache that hides a clean-build failure — at least one job per matrix is a clean build.
- No symptom patches. A red job is diagnosed to root cause.
- No regression in product correctness or release safety.
- Preserve audit trail. Token in env/git config only. Push after every coherent unit. Never force-push. main green.

## Validation gates

1. Every workflow on the default branch green for the right reason.
2. Release workflow ≤ 10 min (or documented Phase 2 target), measured.
3. Benchmark workflow materially faster, measured.
4. Every other workflow faster, measured.
5. Cache hit ratios recorded and healthy.
6. No continue-on-error, no skipped stage without rationale, no masked flake.
7. Release cut and green end to end; artifact validated on clean Windows.

## Deliverables

1. Phase 0 context sheets (4 files), pushed before any change.
2. worklog/CI-FAILURES.md with resolution per failure.
3. worklog/OPTIMIZATION.md with before/after per workflow and per job.
4. Every workflow updated: concurrency, path scoping, matrix reduction, caching, artifact reuse, reusable workflows, timeouts.
5. Every failure fixed at root cause.
6. Full green CI on the default branch.
7. Release cut, green end to end, artifact validated.
8. worklog.md — chronological handoff with every SHA, every workflow run link, every measurement.
9. All pushed with evidence-citing messages.

## TODOs

T1: Clone, record commit, save prompt (redacted), worklog entry.
T2: Spawn agents A–D, produce four context sheets, push.
T3: worklog/CI-FAILURES.md — every current failure with root cause, push.
T4–T6: Fix every failure, push per fix batch.
T7: Add concurrency to every workflow, push.
T8: Add path scoping and job-level gating, push.
T9: Reduce matrix where justified; keep full matrix for release, push.
T10: Add/repair caching for every dependency class; verify hit ratio, push.
T11: Extract artifact reuse — build once, download downstream, push.
T12: Extract reusable workflows / composite actions, push.
T13: Tighten timeouts; remove dead workflows, dead caches, .bak files, push.
T14: Measure every workflow before/after; record in worklog/OPTIMIZATION.md, push.
T15: Verify all workflows green on default branch, push.
T16: Release dry-run on pre-release tag, verify artifacts and time, push.
T17: Cut new release, run end to end, validate artifact on clean Windows, push.
T18: Update docs to current CI reality, push.
T19: Final verification — every gate proven, push.
T20: Handoff — complete worklog.md, final push.
