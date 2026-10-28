# MAJOR M-068 — the composite's double-start: install.ps1 (v0.2.3+) starts the service, then the composite's own bare `sc.exe start` dies 1056 ("An instance of the service is already running") and fails every install — the M-062 close-out's "single-start contract" was only half-implemented and no run exercised the post-`5e7390b` composite until the SA-6 drill

**Severity:** MAJOR
**Module / Surface:** driver install / the install-driver composite (modules/driver-install.md)
**Parameters affected:** K71 (installer executability), the M-062 single-start contract
**Corpus references:** none needed (a core-side defect); the contract: the og's create+start is ONE lifecycle unit (`architecture/02-kernel-driver-architecture.md:101-103`)
**Core references:** `.github/actions/install-driver/action.yml:104-152` — the composite calls install.ps1 WITHOUT `-NoStart` (line 105-106), install.ps1 (v0.2.3+) starts the service by default (exit codes 6/7 are the start-failure contract), and then the composite's own bare `sc.exe start` (line 143) fails 1056 on the already-RUNNING service → `exit 1`. The M-062 Resolution (`5e7390b`) claimed "the composite verifies RUNNING via `sc query` instead of double-starting" — the RUNNING verification WAS added (line 150) but the bare `sc.exe start` was never removed: the claim is half-true against the code.
**Execution evidence:** staged-scenarios run 40672083772 (2026-10-28T10:26, the SA-6 drill dispatch, at `97cc3b2`): `Installing ... -> C:\Windows\System32\drivers\rda10.sys` (the v0.2.4 versioned layout — working) → `Service 'rda' started.` (install.ps1's start leg — working) → `[SC] StartService FAILED 1056: An instance of the service is already running.` (the composite's double-start) → `exit 1` (the setup job red, the scenario job skipped). The lane's four runs are ALL failures (three at the pre-v0.2.3 composite era + this one); no run ever exercised the post-`5e7390b` composite before this drill.
**Status:** filed and fixed in the same landing (this session's M-068 commit)

## Summary

The v0.2.3 deployer batch made install.ps1 start the service itself (the
M-062 fatal-start-failure contract) and the composite's rewrite added a
RUNNING check — but left the composite's own bare `sc.exe start` in place.
Every install through the rewritten composite therefore starts the service
twice; the second start fails 1056 and the composite exits 1. The defect
was latent because no CI run exercised the rewritten composite until the
SA-6 drill (the lane's earlier green run predates the rewrite and rode the
now-disabled manual fallback).

## What the reference does

Create + start is one lifecycle unit (the og's setupdrvx64: CreateService
→ StartService, one call chain, one owner).

## What our implementation does

Two owners start the service: install.ps1 (its contract leg, exit 6/7) and
the composite (the leftover bare start, exit 1) — the second always fails
on a healthy first.

## Gap

Falsifiable claim: any dispatch through the composite after `5e7390b` with
a v0.2.3+ driver release fails at the double-start with 1056 (executed:
run 40672083772).

## Impact

- Every prebuilt lane that installs the driver is red (staged-scenarios:
  all 4 runs failed; the post-rewrite composite has never produced a green
  install).
- The M-062 Resolution's single-start claim is false against the code —
  the audit trail needs the correction.

## Fix plan

1. Remove the composite's bare `sc.exe start` (lines 141-148); keep exactly
   ONE start owner (install.ps1, already running) and the RUNNING
   verification (the existing layer-1 evidence block): `sc.exe query ...
   Select-String 'RUNNING'` → not RUNNING = the failure path (query + the
   System event log + exit 1).
2. The `-NoStart` contract stays available for callers that want the
   composite to own the start (pass `-NoStart` to install.ps1 + start
   once) — not the default path.
3. Re-dispatch staged-scenarios: the setup must go green and the scenario
   job must run (the end-to-end drill this defect blocked).
4. The M-062 Resolution gains the correction note (the half-implemented
   claim, now completed by this fix).

## Tests to add

- The staged-scenarios green run itself (the execution proof; the lane is
  the test).

## Related findings

- **M-062** (FIXED at `5e7390b`) — whose single-start contract this
  completes; the Resolution correction rides this landing.
- **S-036/M-060/M-061/M-064** (FIXED) — the same composite's deployer
  batch.

## Open questions

None — the fix is the removal of one dead start + the merge with the
existing RUNNING check.
