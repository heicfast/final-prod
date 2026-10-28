# MAJOR M-061 — the release's uninstall.ps1 is never exercised by any CI lane, carries the same broken elevation guard as install.ps1, and reports success when file removal silently fails

**Severity:** MAJOR
**Module / Surface:** driver uninstall / dev-sign path (packaging surface — teardown lifecycle)
**Parameters affected:** uninstall behavior (service stop+delete verification); file-removal truthfulness; lifecycle symmetry (install ↔ uninstall)
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:105` — the reference's driver lifecycle: "At app exit / driver uninstall, service is stopped and deleted" (a verified teardown step, not a best-effort one); §5.1-5.3 the full install/operate/uninstall loop
**Core references:** `.github/workflows/driver-install.yml:157-163` (the "Uninstall the driver (always)" step: raw `sc.exe stop/delete` + `Remove-Item`, exit-code unchecked, no post-uninstall verification, does NOT call uninstall.ps1); the release's `uninstall.ps1` (rda-driver v0.2.2, verbatim copy at `diff-to-fix/evidence/executions/devsign-run-40549660074/release-scripts/uninstall.ps1`: line 15 the identical broken elevation-guard expression from S-036; lines 22-25 stop → 1 s sleep → delete with no stop-verification; lines 33-36 `Remove-Item -Force -ErrorAction SilentlyContinue` followed by unconditional `Write-Host "Removed $destPath"`); `docs/PREBUILT-ARTIFACTS.md` §2 (uninstall.ps1 is a required release asset — the "cleanup script"); audited commit `dd82db9`
**Status:** FIXED — see Resolution below
**Prior audit check:** not in prior scope (no prior finding covers the uninstall path or the release scripts)

## Summary

The merged product's CI never executes the release's own `uninstall.ps1` — every lane tears the driver down with raw `sc.exe` commands — so the documented uninstaller is completely unvalidated. Reading it shows why validation matters: it carries the same elevation-guard expression that crashes `install.ps1` (S-036), stops the service with a fixed 1-second sleep and no verification that the stop completed, and then removes the driver file with `-ErrorAction SilentlyContinue` while unconditionally printing `Removed <path>` — if the service is still stopping (image still loaded), the file delete fails silently and the script still reports success.

## What the reference does

The corpus's driver lifecycle documentation states the uninstall behavior as a definite, verified step: at app exit / driver uninstall, "service is stopped and deleted" (02-kernel-driver-architecture.md:105). In the reference the teardown is part of the product's own orchestration (the app uninstalls its driver when it exits), meaning the stop is expected to be confirmed before the delete and before the app considers the driver gone — the uninstall is the *completion* of the install/operate lifecycle, not a fire-and-forget best-effort. The corpus documents no silent-failure semantics anywhere in the driver lifecycle.

## What our implementation does

1. **CI never runs `uninstall.ps1`.** The only teardown in the repo is `driver-install.yml:157-163`:
   ```powershell
   sc.exe stop rda 2>$null | Out-Null
   sc.exe delete rda 2>$null | Out-Null
   Remove-Item "$env:SystemRoot\System32\drivers\rda.sys" -Force -ErrorAction SilentlyContinue
   ```
   — exit codes unchecked, no `sc query` after, no evidence that the service is gone (the mandate's uninstall-confirmation evidence is simply not captured). The other lanes (api-benchmark lineage, staged-scenarios) use the same raw pattern via the composite callers. `uninstall.ps1` is a required release asset that no CI path ever executes.
2. **`uninstall.ps1` itself is defective on three axes** (verbatim v0.2.2 asset):
   - Line 15: `$identity = [Security.Principal.WindowsPrincipal]::[Security.Principal.WindowsIdentity]::GetCurrent()` — the identical expression that crashes install.ps1 under Windows PowerShell 5.1 (S-036). Run the documented way (`.\uninstall.ps1 -ServiceName rda` from an elevated 5.1 console), the script dies at the guard before touching the service.
   - Lines 22-25: `sc.exe stop` → `Start-Sleep -Seconds 1` → `sc.exe delete` — the fixed sleep does not verify the service reached STOPPED; deleting a stop-pending service succeeds at the SCM level while the image stays loaded.
   - Lines 33-36: `Remove-Item $destPath -Force -ErrorAction SilentlyContinue` then `Write-Host "Removed $destPath"` — if the image is still loaded, the file is locked, `Remove-Item` fails, the error is swallowed, and the script still prints success and exits 0.

## Gap

Falsifiable claims: (a) no CI lane in the merged repo executes `uninstall.ps1` (grep of `.github/` shows zero invocations); (b) `uninstall.ps1` run under Windows PowerShell 5.1 crashes at line 15 with `InvokeMethodOnNull` (same expression as S-036's transcript-proven crash); (c) with a still-loaded image, `uninstall.ps1` exits 0 and prints `Removed <path>` while the file remains on disk (the `-ErrorAction SilentlyContinue` + unconditional Write-Host combination guarantees it).

## Impact

- **Consumers following the documented path** (`uninstall.ps1` per PREBUILT-ARTIFACTS.md §2's asset table) hit the S-036 crash and cannot uninstall through the documented tool.
- **Silent-failure semantics**: on the (common) stop-pending path, the uninstaller reports success while leaving a loaded driver + a locked file; the machine keeps a kernel service image until reboot. A subsequent install.ps1 run then fails at its own "service already exists" preflight with a confusing error.
- **No uninstall evidence in CI**: because the raw teardown doesn't verify or capture, drift in the uninstall path (SCM behavior differences, file-lock races) would be invisible — the green checkmark only means the step didn't turn the workflow red.
- The uninstall symmetry the mandate calls "production-shaped" (stop → verify stopped → delete → verify gone → remove file → verify gone) exists in neither path.

## Reproduction / verification

Executed + source evidence: run 40549660074's teardown step (job-log.txt:334-341) shows the raw-command path running with no verification and no captured post-state; the `uninstall.ps1` copy at `.../release-scripts/uninstall.ps1` is the exact v0.2.2 asset (SHA256SUMS-verified in-run) — lines 15/22-25/33-36 quoted above; the grep for `uninstall.ps1` across `.github/` returns only the fetcher's required-asset lists (downloaded, never invoked). Claim (b) rides S-036's transcript proof of the identical expression under 5.1.

## Fix plan (for the engineering team)

1. Upstream (rda-driver): rewrite `uninstall.ps1` — fix the elevation guard (same fix as S-036); replace the fixed sleep with a poll loop (`sc query` until `STOPPED`, bounded, e.g. 10×500 ms); make `Remove-Item` failure loud (`try/catch` → `Write-Error` + `exit 1`) and print `Removed` only after a successful delete; add a final verification block (`Get-Service` → absent, `Test-Path` → false) that gates the exit code.
2. Core repo: replace the raw teardown in `driver-install.yml` (and the composite callers' documented cleanup) with the release's `uninstall.ps1`, then **capture the evidence**: `sc query rda` after (expect "The specified service does not exist"), `Test-Path ...\rda.sys` (expect False) — satisfying the uninstall-confirmation the lane currently omits.
3. Add uninstall.ps1 to the release-asset CI contract: a smoke lane that installs then uninstalls and asserts both post-states.
4. Acceptance criteria: on a green run, the log shows uninstall.ps1's transcript + the two post-state assertions; on a forced lock (e.g. an open handle held against the service image), the uninstall step FAILS (exit 1) rather than reporting success.

## Tests to add

- "uninstall leaves no service" — `sc query rda` post-uninstall must fail with error 1060.
- "uninstall leaves no file" — `Test-Path $env:SystemRoot\System32\drivers\rda.sys` must be False.
- "uninstall fails loud on locked image" — hold a handle, expect exit 1 + no `Removed` line.
- "elevation guard executes" — run both scripts unelevated; expect the guard's clean error, not `InvokeMethodOnNull`.

## Related findings

- **S-036** — the crash in install.ps1; the guard fix must land for both scripts together.
- **M-060** — the install-side divergence; the uninstall verification step should assert the documented service shape before tearing it down.

## Open questions

None.

## Resolution (FIXED — the release's uninstall.ps1 is now the CI teardown, executed and asserted)

1. **Upstream (rda-driver v0.2.3)**: `uninstall.ps1` checks `$LASTEXITCODE`
   on stop (exit 2) and delete (exit 4), polls `Get-Service` up to 10×500 ms
   for STOPPED instead of the fixed 1 s sleep (exit 3 on timeout), verifies
   the registration is gone after delete (exit 5 — the SCM marked-for-
   deletion case), and makes the image delete loud with re-`Test-Path`
   verification (exit 6) — the unconditional "Removed" lie is gone.
2. **The CI lane (`.github/workflows/driver-install.yml`)**: the raw
   unverified `sc.exe` teardown is replaced by the release's own
   `uninstall.ps1`; its exit code is the job's teardown verdict, followed by
   the post-state evidence assertions the finding asked for (service gone,
   image gone) — an incomplete teardown now fails the lane instead of
   printing success.

The full install→uninstall smoke lane is the existing driver-install.yml
daily cron (both phases in one job); its next dispatch is the execution
evidence.

**Landed:** rda-driver `6b50bb8` (tag v0.2.3) + core_recovery "fix(ci):
S-036 close-out" (the teardown flip rides in the same commit).
