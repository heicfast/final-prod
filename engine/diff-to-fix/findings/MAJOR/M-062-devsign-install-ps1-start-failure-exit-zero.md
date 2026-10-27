# MAJOR M-062 — install.ps1 reports install SUCCESS (exit 0) when StartService fails, and its failure message is itself malformed

**Severity:** MAJOR
**Module / Surface:** driver install / dev-sign path (installer error semantics)
**Parameters affected:** install error semantics (start-failure reporting); installer exit codes
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:101-103` — the reference installer "creates + starts the service (SCM APIs)" as one verified lifecycle step; §5.3 teardown symmetry (a definite lifecycle, implying start success is part of the install contract)
**Core references:** rda-driver v0.2.2 `install.ps1:68-75` (verbatim copy at `diff-to-fix/evidence/executions/devsign-run-40549660074/release-scripts/install.ps1`):
```powershell
if (-not $NoStart) {
    sc.exe start $ServiceName | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "StartService failed ($LASTEXITCODE). Check code-signing policy " +
                      "(testsigning), driver signature, and the System event log."
    } else {
        Write-Host "Service '$ServiceName' started."
    }
}
```
consumed by `.github/actions/install-driver/action.yml:82-85` (invokes install.ps1 then checks `$LASTEXITCODE`); documented as the installer in `docs/PREBUILT-ARTIFACTS.md` §2 / `docs/DRIVER-RELEASE.md` §1; audited commit `dd82db9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope

## Summary

The documented installer treats a failed service start as a **warning and exits 0** — a consumer scripting `install.ps1` (as the composite does) sees success while no driver is loaded. The intended helpful message is also malformed: the `+` continuation is parsed in argument position, so the two string fragments do not concatenate as intended (the second fragment is passed as a separate argument / the expression does not join), and the printed warning is truncated or garbled relative to the author's intent. Wrong error semantics on the exact failure a dev-sign consumer is most likely to hit (code-signing rejection).

## What the reference does

The corpus's installer step is "creates + starts the service (SCM APIs)" — one lifecycle unit (02-kernel-driver-architecture.md:101-103). The corpus documents no warning-and-continue behavior for a failed start; the lifecycle's steps are stated as performed, and the teardown side ("service is stopped and deleted", :105) mirrors a definite model. A failed start in the reference model is a failed install, surfaced to the orchestrating product — not a yellow flag with a green exit code.

## What our implementation does

`install.ps1` lines 68-75 (quoted above): on `sc.exe start` failure the script emits `Write-Warning` and falls through; the script's final statement is the `Write-Host "Next: ..."` line, so the process exit code is 0. The composite (`action.yml:82-85`) checks `$LASTEXITCODE` of `install.ps1` and would take the fallback branch on non-zero — but this path never yields non-zero, so a signature-rejected driver slides through install.ps1 "successfully" and the composite's own subsequent `sc.exe start` (line 86) is what catches it. Any consumer invoking install.ps1 directly (the documented path) gets exit 0 + a truncated warning. Additionally, the `Write-Warning "..." + "..."` across two lines: in PowerShell argument mode the `+` binds as a separate token, so the intended concatenation does not occur as written — the message the user actually sees is just the first fragment (ending mid-sentence at "code-signing policy ").

## Gap

Falsifiable claim: with a driver whose signature is rejected (e.g. unsigned on an enforcing machine), `install.ps1` exits **0** while the service is created but not running, and its warning output is a single truncated fragment. (Currently masked by S-036 — the script never reaches line 68 on the documented invocation path — but the defect is in the released asset's start branch and becomes live the moment the guard is fixed.)

## Impact

- **Wrong error semantics on the highest-likelihood dev failure**: code-signing rejection is the canonical install failure for a dev-signed driver; reporting it as warning+success sends the consumer to diagnose a "missing driver" instead of a rejected signature.
- **Script-level success lies**: CI or automation keying on install.ps1's exit code (as the composite does) cannot trust it for start failures.
- The truncated message hides the two most useful diagnostics (testsigning hint, event-log pointer) — the fragment that prints ends before them.

## Reproduction / verification

Source evidence (strength-3, verbatim released asset quoted above + copied at `.../release-scripts/install.ps1`): the `if ($LASTEXITCODE -ne 0) { Write-Warning ... }` branch contains no `exit`/`Write-Error` and `$ErrorActionPreference = 'Stop'` does not act on `Write-Warning`; the script has no `exit <nonzero>` anywhere except via `Write-Error`. The composite's compensating check is at `action.yml:86-93` (`sc.exe start` + `if ($LASTEXITCODE -ne 0) { ... exit 1 }`) — proving the authors know a failed start must be fatal, but the shipped installer doesn't do it.

## Fix plan (for the engineering team)

1. In `install.ps1` (upstream, same v0.2.3 release as S-036/M-061): make the start-failure branch fatal — `Write-Error "StartService failed ($LASTEXITCODE) - check code-signing policy (testsigning), driver signature, and the System event log."; exit 1` (single string, no `+` continuation), keeping `-NoStart` as the explicit opt-out for deferred starts.
2. Print the `sc.exe start` transcript (drop the `| Out-Null`) so the SCM error text lands in the consumer's log.
3. Core side: once the composite trusts install.ps1's exit code (S-036 fix), remove the double-start (`install.ps1` starts; composite re-starts) — have the composite verify `RUNNING` via `sc query` instead of issuing a second start.
4. Acceptance criteria: unsigned driver on an enforcing runner → install.ps1 exits non-zero with the full one-line message containing "testsigning"; `-NoStart` run exits 0 with service created and not started.

## Tests to add

- "start-failure is fatal": install with a corrupted signature → exit non-zero, message contains both "testsigning" and "event log".
- "-NoStart defers": service created, state STOPPED, exit 0.
- "single-start contract": the composite's log shows exactly one `sc.exe start` invocation per install.

## Related findings

- **S-036** — the crash that currently masks this branch; fix together.
- **M-061** — the same wrong-semantics pattern on the uninstall side.

## Open questions

None.
