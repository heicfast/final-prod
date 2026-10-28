# MAJOR M-063 — dev-sign validation is a single-environment data point (GitHub windows-2022, HVCI off); the HVCI/SecureBoot constraint and any Win7/Win10/Win11 matrix are undocumented

**Severity:** MAJOR
**Module / Surface:** driver dev-sign posture / install environment coverage (packaging + signing surface)
**Parameters affected:** validated-environment matrix; code-signing enforcement classes (HVCI/VBS, Secure Boot, test-signing mode); compatibility claims truthfulness (Win7 SP1 → Win11)
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:19` — all six reference drivers are "Authenticode-signed (EaseUS = 'Chengdu Yiwo Tech Development Co., Ltd')" with per-target builds (`eprdtdrv.sys` production Win8+, `eprdtdrv_win7older.sys` legacy Win7-, WinPE variants preloaded where "no driver-signing enforcement issues exist" :26) — i.e. the reference solves signing-environment variance with a vendor signature chain and deployment-specific binaries, and its WinPE deployment is explicitly motivated by signature enforcement; `:25-27` the four-filename/context split
**Core references:** `.github/actions/install-driver/action.yml:44-49` (the job-local cert mechanism, comment "no test-signing boot mode required on windows-2022"); `docs/PREBUILT-ARTIFACTS.md` §2 "Install posture on runners" (documents the windows-2022 posture with no environment-constraint statement); `docs/DRIVER-RELEASE.md` §1 release-body claim "Win7 SP1 -> Win11 x64, single binary. Verified by the CI PE gate (import allowlist + OS 6.1 header contract)" — a PE-header claim standing in for an install/load claim; `:62-75` (signing.md scope honesty: OSSign ≠ Microsoft kernel signature, "Win10/11 retail loading still requires Hardware Dev Center attestation/HLK"; "WinPE is relaxed"); the validated run: 40549660074 (windows-2022 only); audited commit `dd82db9`
**Status:** FIXED — see Resolution below
**Prior audit check:** not in prior scope (the dev-sign CI era postdates the board close)

## Summary

The dev-sign mechanism is empirically proven on exactly **one** environment class: GitHub-hosted windows-2022 images (no HVCI, test-signing off). No lane, doc, or matrix states on which Windows environments the recipe works — and on a stock Windows 11 machine (HVCI/VBS enabled by default on modern hardware, Secure Boot on) a self-signed cert in Root+TrustedPublisher does **not** satisfy kernel-mode code-signing enforcement, so the documented dev-sign path fails at `sc.exe start` with signature rejection (0xC0000428-class). The repo's compatibility claim ("Win7 SP1 → Win11 x64, single binary") is a PE-header claim (OS 6.1 stamp + import allowlist); no install/load validation exists for Win7, Win10, or Win11.

## What the reference does

The reference product ships its driver family **Authenticode-signed by the vendor** ("Chengdu Yiwo Tech Development Co., Ltd" — corpus :19), which is why a single `eprdtdrv.sys` loads across its live-Windows targets, and it **splits deployment contexts explicitly**: `eprdtdrv.sys` (production Win8+), `eprdtdrv_win7older.sys` (legacy Win7-), and preloaded WinPE variants where "no driver-signing enforcement issues exist" (:26-27). In other words, the og treats signing-environment variance as a first-class deployment dimension — solved with a vendor chain plus context-specific binaries — and the corpus records this per-context posture explicitly.

## What our implementation does

The merged repo's dev-sign recipe (job-local self-signed cert → Root+TrustedPublisher → `signtool /fd SHA256`) is documented only with the parenthetical "no test-signing boot mode required on windows-2022" (action.yml:44-49, PREBUILT-ARTIFACTS.md §2). Nothing states the boundary conditions of that claim: that it depends on the runner's signing-enforcement posture (no HVCI/VBS, no Secure-Boot driver requirement), and that on stock Win10/11 retail machines — Win11 with HVCI on by default — the same recipe requires `bcdedit /set testsigning on` (with its desktop watermark and boot-configuration warning) or fails. The only cross-environment statement anywhere in the chain is signing.md's OSSign scope note (Win10/11 retail needs attestation/HLK; WinPE relaxed), which concerns the *disabled* OSSign pipeline, not the *live* CI test-sign path. The "Win7 SP1 → Win11" claim in the release body is backed by a PE gate (OS 6.1 header + imports), which says nothing about install/load behavior on those OSes; no Win7/Win10/Win11 install lane exists (the runner matrix is windows-2022 only across all five workflows).

## Gap

Falsifiable claims: (a) every dev-sign/install execution on record ran on GitHub windows-2022 (workflow `runs-on` audit: all lanes windows-2022); (b) no doc in the chain (PREBUILT-ARTIFACTS §2, DRIVER-RELEASE §7, action.yml comments) states the HVCI/SecureBoot constraint of the job-local-cert mechanism or the environments it was validated on; (c) the "Win7 SP1 → Win11" compatibility sentence cites the PE gate, not any install/load evidence.

## Impact

- **A developer on a stock Win11 box** follows the documented recipe, the driver refuses to load (signature enforcement), and nothing in the docs explains why the CI-proven mechanism doesn't reproduce — the exact "verify it works on real Windows environments" gap this validation exists to close.
- **Compatibility over-claim**: the release body's Win7→Win11 sentence reads as a load-behavior claim but is a header-contract claim; on Win7 SP1 (no SHA-256-only enforcement issues, but different SCM/ATS behaviors) and on Win10/Win11 retail, no evidence exists either way.
- **Test-signing guidance is absent**: the docs never mention `bcdedit /set testsigning on` (the standard dev-machine posture) or its warning — a dev-sign README's basic duty.
- The rda-driver INF path is additionally dead on any enforcement-enabled machine: `CatalogFile = rda.cat` but no `.cat` ships (unsigned-by-design), so `pnputil /add-driver` cannot succeed there — undocumented as well.

## Reproduction / verification

Executed (strength-1, the positive half): run 40549660074 — windows-2022, HVCI off class, load+round-trip green with the job-local cert. Source audit (strength-3) for the negative half: `rg -l "windows-" .github/workflows/` → every lane pins `windows-2022` only; `rg -i "hvci|secure.?boot|testsigning|device.?guard" docs/ .github/` → no hits for the CI mechanism's constraints (signing.md's OSSign note is the only adjacent text and explicitly about OSSign). The claim's provenance: DRIVER-RELEASE.md §1 release-body text quoting the PE gate.

## Fix plan (for the engineering team)

1. Write the environments contract into `docs/PREBUILT-ARTIFACTS.md` §2 (or a short `docs/DEV-SIGN.md`): a table of signing-enforcement classes × what the job-local-cert mechanism needs — (i) HVCI/VBS off + Secure Boot off or permissive → works as-is; (ii) HVCI on → requires testsigning (which itself is ignored under HVCI — on HVCI machines dev loading requires the driver to be attestation-signed, so state "use a VM with VBS off"); (iii) test-signing mode recipe + its boot-configuration warning; (iv) Win7: the OS 6.1 stamp means the loader accepts it with a test cert in Root/TrustedPublisher (cross-signed-era rules) — validate once on a Win7 VM to make the claim real.
2. Add a `windows-2025` (or `windows-latest`) lane to `driver-install.yml` as a second enforcement-posture data point, and a Win7/Win10 validation plan (self-hosted or VM lane; at minimum record them as UNVALIDATED in the doc table rather than implying coverage).
3. Reword the release-body compatibility sentence to distinguish the PE header contract (proven) from install/load validation (per-OS status), or attach the validation matrix once it exists.
4. Acceptance criteria: the doc table names every validated environment with its runner/run link and its enforcement posture; the compatibility sentence cites the table.

## Tests to add

- A second runner class (e.g. windows-2025) dispatch of the same lane, archived next to 40549660074.
- A documented negative test: HVCI-on VM → expected start failure with the docs' prescribed remediation path.
- (When a Win7/Win10 lane exists) the same three-phase assertions as driver-install.yml.

## Related findings

- **I-026** — the deliberate unsigned-by-design posture this constraint lives inside.
- **N-084** — the timestamp-server gap in the same sign step.

## Open questions

The exact signing-enforcement posture of the GitHub windows-2022 image (HVCI off is inferred from the successful self-signed load, which is itself the evidence; whether Secure Boot is on or off on those images is not load-bearing for this finding — the load succeeded, so whatever the posture is, it permits the mechanism; the finding's claim is about *other* environments being undocumented, not about the runner's exact flags).

## Resolution (FIXED — the environments contract documented + a second runner class + the release truth pass)

1. **The environments contract** is written into `docs/PREBUILT-ARTIFACTS.md`
   §2 (the M-063 fix plan's designated home): the enforcement-class table —
   windows-2022 (HVCI/VBS off, job-local cert: the ONLY execution-validated
   class), stock Win11 (HVCI on: attestation-signing required; the dev-box
   recipe is a VBS-off VM or `bcdedit /set testsigning on` + reboot), Win7
   SP1 (build-gated by the PE headers + import allowlist, **not
   install-validated** — treat as UNVALIDATED until a runner class covers
   it).
2. **A second runner class**: `driver-install.yml` is now a
   `[windows-2022, windows-2025]` matrix (fail-fast off; per-leg artifacts)
   — the lane's next dispatch is the second data point.
3. **The compatibility sentence**: the rda-driver release body (the claim's
   origin) now separates build-gate evidence (PE gate) from install/load
   evidence — "Win7 SP1 -> Win11 x64 ... BUILD compatibility verified by
   the CI PE gate ...; INSTALL/LOAD validated on windows-2022 (HVCI off,
   job-local cert)" (rda-driver `f569836`, re-tagged v0.2.3). The INF's
   dead CatalogFile line stays as-is (the INF is not the install path; the
   scripts are, and they are now the tested artifacts).

**Landed:** core_recovery "fix(ci): S-036 close-out + M-063 environments
matrix" + rda-driver `f569836`.
