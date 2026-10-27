# MINOR N-085 — the documented install path requires an elevated interactive PowerShell console (visible window, manual elevation, ExecutionPolicy bypass); the reference installs silently via SCM APIs from a native EXE

**Severity:** MINOR
**Module / Surface:** driver install / installer posture (production-shaped gap)
**Parameters affected:** installer interaction model (silent vs console); elevation UX; install orchestration ownership (product vs operator)
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:25,99-105` — the reference's driver is installed by `PartitionRecovery/setupdrvx64.exe` (PDB `mod.WinDiskAccessDriverSetup/setupempdrv03`), a native setup EXE invoked by the product, which copies the driver and "creates + starts the service (SCM APIs)"; §5 the whole lifecycle is product-orchestrated (install at app start, teardown at app exit) — no console, no operator-run scripts
**Core references:** `docs/PREBUILT-ARTIFACTS.md` §2 (the documented install: `install.ps1 -ServiceName rda -Source rda.sys` run by an operator/CI in an elevated shell); `.github/actions/install-driver/action.yml:76-78` (`powershell -ExecutionPolicy Bypass -File install.ps1` — console host, explicit policy bypass); rda-driver v0.2.2 `install.ps1:11-12` ("REQUIRES: elevated PowerShell"), `:35` (`$ErrorActionPreference = 'Stop'`), `:38` the guard; audited commit `dd82db9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope (installer posture was not a prior session's axis)

## Summary

The merged product's documented install path is an operator-run PowerShell script: a visible console host, an `ExecutionPolicy Bypass` invocation, and a manual elevation guard — every step the reference product's real installer does not surface to anyone. In the og, installation is a silent step the product performs for the user at app start (`setupdrvx64.exe`, SCM APIs). For the dev/CI posture this is an acceptable and normal shape; for the customer-shaped install the mandate asks about, it is a gap: nothing in the merged repo automates or wraps the install into a product-owned, prompt-free unit.

## What the reference does

Per the corpus, the reference product owns the driver lifecycle end to end: when DRW runs on a live Windows system, `setupdrvx64.exe` installs `eprdtdrv.sys` (copy to `System32\Drivers`, SCM create + start), and at app exit the service is stopped and deleted (02-kernel-driver-architecture.md:25,101-105). The user never sees a console, an execution-policy question, or an elevation prompt distinct from the product's own UAC elevation — the driver lifecycle is an implementation detail of the product session.

## What our implementation does

The documented path (`docs/PREBUILT-ARTIFACTS.md` §2) is `install.ps1` executed by hand (or by CI) in an elevated PowerShell; the composite invokes it as `powershell -ExecutionPolicy Bypass -File ...` (action.yml:76-78) and falls back to inline `sc.exe` (S-036/M-060). The script's own header states the contract: "REQUIRES: elevated PowerShell" (install.ps1:11-12). Interactive use means: a visible console window, a policy bypass, and an elevation the operator performs; there is no product-side orchestration (the core refuses work without the driver — e58bde1's driver-only era — but does not install it for the user). In CI this is invisible (no interactive desktop); on a real machine it is a visible, operator-driven install — the exact "visible PowerShell window / interactive prompt the merged product's real installer would not produce" posture the mandate names.

## Gap

Falsifiable claim: the merged product has no install path that executes without an operator-run elevated console (or an equivalent CI shell), while the reference's documented install is a product-invoked silent EXE using SCM APIs.

## Impact

- **Dev/CI posture: none** (the current shape is the norm for dev-sign validation and is working, per run 40549660074).
- **Customer-shaped install: the gap** — when the merged product grows a real installer, the PowerShell-script posture cannot ship as-is: it needs wrapping into a silent unit (installer EXE/MSI with a custom action or a service-based installer) with the product's own UAC story, and the scripts become internal implementation. Recording it now keeps the roadmap honest about what "production-shaped" still requires.
- No correctness impact; no corpus-parameter divergence (the copy/create/start semantics themselves match — M-060 covers the config divergence).

## Reproduction / verification

Source evidence (strength-3): corpus :25,101-105 (product-owned silent lifecycle) vs the quoted core-side invocation chain (PREBUILT §2 prose, action.yml:76-78, install.ps1:11-12 header contract). Executed confirmation (strength-1): run 40549660074's install step is a `powershell -File` console transcript (job-log.txt:205-249) — a shell-mediated install by construction.

## Fix plan (for the engineering team)

1. When the product installer milestone arrives: wrap the driver install into the product's installer (MSI custom action or bootstrapper EXE) running the same SCM sequence (copy → create → Parameters → start), silent, elevation via the installer's single UAC prompt; the release's install.ps1 remains the dev/CI path with a doc note making the split explicit.
2. Near-term (docs-only, this session's allowed scope boundary): add one line to PREBUILT-ARTIFACTS §2 — "operator-run script posture: dev/CI only; the customer install shape (silent, product-owned) is a packaging milestone, tracked as N-085."
3. Acceptance criteria: the doc note exists now; the later installer lane reuses the documented service shape (M-060's parity assertions) and requires no visible console.

## Tests to add

- (At installer time) an automated install on a clean VM asserting zero interactive prompts and a post-install `rrecov driver --json` ready state; until then, none — this is a posture record, not a code defect.

## Related findings

- **S-036 / M-060 / M-062 / M-061** — the script-level defects that also stand between the current posture and any customer-shaped reuse of these scripts.
- **M-063** — the environment matrix the installer milestone will have to cover.

## Open questions

None.
