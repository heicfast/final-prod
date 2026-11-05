# MAJOR M-086 — rda-driver FileVersion pinned 1.0.0.0 defeats the -Layout Versioned upgrade path (every v0.2.x release collides at exit 2)

**Severity:** MAJOR
**Module / Surface:** Driver install lifecycle (rda-driver v0.2.4 install.ps1 -Layout Versioned)
**Parameters affected:** K76 (versioned coexistence & upgrade), K71 (installer executability & error semantics), Q70-adjacent
**Corpus references:** `architecture/02-kernel-driver-architecture.md` §5 (the `%s%d` versioned driver coexistence — eprdtdrv2.sys-style layouts); `artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §3 (the versioned copy+create+start contract); the og's own shipped tree (`og-codebase/PartitionRecovery/eprdtdrv.sys` + versioned siblings) as cited by M-064's trail
**Core references:** audited driver repo (heicfast/rda-driver, out of this repo's tree per the two-repo grant) `323e75a` (v0.2.4) — every path in this paragraph is a DRIVER-REPO path: `src/driver/rda_version.h:3-5` (`RDA_VERSION_MAJOR 1`, `RDA_VERSION_MINOR 0`, `RDA_VERSION_FILE_STRING "1.0.0.0"`); `src/driver/rda.rc:19-21` (FILEVERSION from those macros); `scripts/install.ps1:99-115` (dest name = `"$ServiceName$tag.sys"`, tag = FileMajorPart+FileMinorPart from the source image's version resource); `install.ps1:118-174` (the upgrade branch: `$isUpgrade` requires `$imagePath -ne $destPath` :130-131; same path → exit 2 :132-135); `.github/workflows/release.yml:23-24` (plain msbuild — no version-stamping step). Audited core `35adcb9` consumes the release via `release.yml:291` (`-FloorVersion v0.2.4`).
**Status:** PUSHED-FORWARD (driver-repo-resident; this session's two-repo grant excludes heicfast/rda-driver)
**Prior audit check:** newly discovered — the M-064 resolution (v0.2.4, `323e75a`) landed the `-Layout Versioned` machinery but its FileVersion source was never moved per release; the trail's own handoff note ("driver binary FileVersion stays pinned 1.0.0.0 while the release tag is v0.2.4 — same-build reinstalls hit exit 2 without -Uninstall", SCAN-ENGINE-DELTA driver-lane handoffs) recorded the symptom without a finding.

## Summary

The v0.2.4 `-Layout Versioned` scheme derives the versioned file tag (`<ServiceName><major><minor>.sys`, e.g. `rda10.sys`) from the driver binary's **FileVersion resource**, but the version header has been pinned at 1.0.0.0 since the repo's first release and the release workflow never stamps the tag into the RC. Every v0.2.x release therefore derives the same tag `10`, the same destination `rda10.sys`, and the upgrade branch's `$imagePath -ne $destPath` test fails — so **both same-build reinstalls AND genuine version upgrades (v0.2.3 → v0.2.4) hit exit 2** ("refuse; an image is already installed") unless the operator manually passes `-Uninstall` first. The M-064 "in-place upgrades, an in-use image never overwritten" contract is implemented but unreachable between releases.

## What the reference does

The og's driver family ships versioned siblings (`eprdtdrv.sys`, `eprdtdrv2.sys`-style `%s%d` layouts — architecture/02 §5; the M-064 corpus extraction) where a NEW version installs ALONGSIDE the old image and the service re-points, so an in-use image is never overwritten and upgrades proceed without an explicit uninstall. The version discriminator comes from the shipped binary itself (the versioned name is derived from the image's version resource — the M-064 fix contract, `install.ps1:99-115` comments).

## What our implementation does

- `rda_version.h:3-5` pins `RDA_VERSION_MAJOR 1` / `RDA_VERSION_MINOR 0` → every built `rda.sys` carries FileVersion 1.0.0.0 (`rda.rc:19-21`); the wire version is likewise 1.0 (`rda_ioctl.h:109-110`).
- `release.yml` builds with plain msbuild and never stamps the release tag (v0.2.4) into the version resource.
- `install.ps1:104-106` reads `$vi.FileMajorPart$vi.FileMinorPart` → always `"10"` → dest always `<ServiceName>10.sys`.
- The upgrade branch (`install.ps1:130-135`): installing v0.2.4 over an existing v0.2.3 install computes the SAME dest path ⇒ `$isUpgrade = $false` ⇒ the classic exit-2 refuse fires. The documented exit-code contract itself (0-9) is honored — but the semantics collapse "different release" into "same build".

## Gap

Falsifiable: `git log -p -- src/driver/rda_version.h` shows the version macros unchanged at 1/0 across all v0.2.x tags; `release.yml` has no stamping step; therefore for any two releases R1 ≠ R2 of v0.2.x, `install.ps1` computes identical dest names and refuses the upgrade with exit 2. The versioned-coexistence parameter (K76) is inert across releases — it only fires for a plain→versioned transition (e.g. an existing `rda.sys` → new `rda10.sys`).

## Impact

Install/upgrade path: customers and CI lanes moving between driver releases hit exit 2 and must run `-Uninstall` first — the exact friction M-064 was filed to remove. The core's `release.yml` driver tier (floor v0.2.4) and the installer lane both consume releases; a future v0.2.5 will collide with a deployed v0.2.4. Also masks the `%s%d` contract's test surface: the two-version coexistence execution drill (the driver worklog's own open item, `worklog.md:226-227`) has never run and would have exposed this immediately.

## Reproduction / verification

First-hand this session (driver `323e75a`): `rda_version.h` read (quoted above); `install.ps1:99-174` read line-by-line (the tag derivation, the isUpgrade test, the exit-2 branch); `release.yml` read (no version step). Logical chain verified: same FileVersion ⇒ same derived tag ⇒ same dest ⇒ isUpgrade false ⇒ exit 2. The driver worklog's queued "execution drills (the two-version coexistence on a real runner) ride CI's next dispatch" confirms no execution has yet contradicted this. (Windows execution is out of this container's reach — the finding is code-proven; UNVERIFIED-BY-EXECUTION is marked for the empirical exit-2 transcript only, not for the mechanism.)

## Fix plan (for the engineering team)

1. Pick one version source: either (a) bump `rda_version.h` per release (MAJOR.MINOR = 0.2 → tag `02`... note this yields `rda02.sys`-style names, fine) — plus a CI check that the header matches the tag; or (b) stamp the RC at build time in `release.yml` (read the tag, generate `rda_version.h` or pass `/D` overrides to the RC compile) — single-source-of-truth preferred per the repo's own "one pin source" lesson.
2. Files: `src/driver/rda_version.h`, `src/driver/rda.rc`, `.github/workflows/release.yml` (option b), `scripts/verify/` (add a release-asset assertion that FileVersion == tag).
3. Add the execution drill the worklog already queues: install v0.2.x → install v0.2.(x+1) → assert BOTH images exist on disk (`rdaX.sys` + `rdaY.sys`), the service runs the new one, exit 0; then uninstall sweeps both versioned leftovers (the existing `^$ServiceName\d+\.sys$` regex already handles this).
4. Contract/doc updates: DRIVER-RELEASE.md's "PE version resource 1.0.0.0" informational tier becomes the stamped-version tier; the driver README's versioned-layout section notes the tag source.
5. Acceptance: the drill above green on a windows runner; `verify_reference_binaries.py`-style assertion that no two consecutive releases share a FileVersion.

## Tests to add

- `Install-Upgrade-KeepsBothImages` (the two-version coexistence drill)
- `FileVersion-Matches-ReleaseTag` (release-asset gate)

## Related findings

- **M-064** (the versioned-layout contract this defeats — FIXED at v0.2.4, but the version source starves it)
- **M-088** (the FloorVersion pin split — same CI-release posture family)
- **N-107** (the driver-repo doc drift cluster, same lane)

## Open questions

None on the mechanism. Which version-source option (a) or (b) is an owner decision recorded in the fix plan.


## SA-9 addendum (2026-10-28 — the third lane's convergent decode + one prevention proposal)

The SA-9 lane (audited rda-driver `323e75a`) independently established the
same inertness proof: every v0.x release computes the identical
`<ServiceName>10.sys` destination (tag = FileMajorPart+FileMinorPart of the
pinned 1.0.0.0 resource), so the upgrade branch's
`($imagePath -ne $destPath)` condition is unsatisfiable between our own
releases and every reinstall exits 2 — the M-064 acceptance criterion
fails for every actual release pair.

Additional contribution to the fix plan: a **CI release gate asserting
tag == version resource** (the release workflow knows the tag; a mismatch
fails the release) so the pin cannot silently regress after the un-pin —
the same prevention shape as the v0.2.3 PowerShell AST parse gate (a
broken script broke every CI install; the gate now catches the class).
The lane's own finding file (dropped at reconciliation as convergent)
additionally walked the wire surfaces (GET_VERSION `(1<<32)|0`, GET_INFO
fields, the session protocol 0x00010100 — all 1.0-pinned) and framed the
follow-on decision: if the wire stays 1.0-by-design (matching EUDCPEPM's
protocol-version semantics), register that in DRIVER-CONTRACT.md so the
pin is deliberate on the wire side while the resource side moves.


## Resolution

**Status:** PUSHED-FORWARD (tracked-open OQ-S10-3)
**Verified by:** bf17e55-lineage tree: the core-side evidence re-verified — release.yml's driver tier consumes with -FloorVersion v0.2.4 (the floor is correct on the core side; the defect is the DRIVER repo's rda_version.h pin + unstamped release workflow + the install.ps1 derivation chain - all outside this session's two-repo grant). The finding stays ACTIVE on the board; the fix belongs to the driver-repo lane (the fix plan's steps 1-5 are all rda-driver files).
**Resolution notes:** The core-side consumption is verified sound; the entire fix surface (rda_version.h, rda.rc, the driver release.yml stamping, install.ps1's tag derivation, the two-version coexistence drill) lives in heicfast/rda-driver - out of scope for this session's TWO-repo grant (core + corpus only). Pushed forward as the driver-repo lane's next fix; no core code change is honest here.

## Session-89 disposition note (2026-10-30, the S89 fix lane — boundary re-affirmed; core-side half re-verified at the current tree)

This lane's grant carries the identical two-repo text as every session
since S10 ("Two repositories are in scope and only two: core_recovery
(modified) + easeus-drw-research (read-only)") — the entire fix surface
of this finding remains in heicfast/rda-driver and stays untouchable.
Core-side half re-verified first-hand at this lane's landing head
(`fb6842c3`-lineage): DRIVER-CONTRACT.md's truth rows stand (§4.4 "the
EUDCPEPM IoctlBase-normalized session protocol (19 codes)" + the
§4.2 extended-IOCTL wire-truth rows 0x600244x with the CTL_CODE_RDA
derivation note) — the integrator-facing corrections of exactly this
finding's drift class. No core-side change owed; the next concrete step
stands unchanged (OQ-S10-3's order: M-087 → M-089 → the N-107/N-127
prose sweeps → M-086 — one driver-repo lane takes all five).
