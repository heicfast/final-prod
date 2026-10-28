# MAJOR M-088 — FloorVersion pinned at two sites at two floors + the "ONE pin source" claim is false + the CI-shape docs describe the wiped workflow set

**Severity:** MAJOR
**Module / Surface:** CI / packaging (the driver-release consumption pins and the CI documentation layer)
**Parameters affected:** K75-adjacent (release/signing posture evidence), Q68 (documentation accuracy), Q70-adjacent (the CI gate layer's own truthfulness)
**Corpus references:** the corpus's driver-install ground truth as consolidated by the dev-sign session — `architecture/02-kernel-driver-architecture.md` §5 + `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md` (the reference's single-contract install posture; K-parameter map in `diff-to-fix/PARAMETERS.md:139-160`)
**Core references:** audited core `35adcb9`: `.github/workflows/release.yml:291` (`-FloorVersion v0.2.4`); `.github/workflows/installer.yml:118` (`-FloorVersion v0.2.3`); `docs/CI-CURRENT.md:59-61` ("Driver tier: fetched prebuilt from `heicfast/rda-driver` with `-FloorVersion v0.2.4` (**ONE pin source**; the versioned-layout era the teardown contract assumes)") and `:145` ("one pin source per dependency"); `docs/CI-CURRENT.md:1,15-19,127` + `README.md:125-132` + `RELEASE-NOTES.md:3-16` (the "3-lane" description omitting `installer.yml`; README's wholly stale Ubuntu/CodeQL/API-benchmark CI paragraph); `docs/PREBUILT-ARTIFACTS.md:36-37,54-59,63-64,73-74,81-82,127-128,148-153` (the deleted composite action + three deleted workflows cited as current); `docs/DRIVER-RELEASE.md:54,86,105-106,139,210`; `docs/DRIVER-CONTRACT.md:26`; `docs/CI-LOCAL.md:65-67`; `docs/CI-SCENARIOS.md:16,25-27`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered — the CI consolidation (566c483) and its aftermath left the pin sites and the doc layer divergent; no prior finding owns the "ONE pin source" falsehood or the stale-path cluster.

## Summary

Three interlocking documentation/pin defects on the driver-consumption surface: (1) the FloorVersion for `heicfast/rda-driver` is pinned at **two live sites at two different floors** — `release.yml:291` at v0.2.4 and `installer.yml:118` at v0.2.3 — while `docs/CI-CURRENT.md:59-61` asserts there is exactly **ONE pin source** (v0.2.4); (2) the CI documentation layer (`CI-CURRENT.md`, `README.md:125-132`, `RELEASE-NOTES.md` vNext) describes a **3-lane** windows-only CI that omits the fourth workflow `installer.yml`, and README's Status paragraph still describes the pre-consolidation shape (Ubuntu gcc/sanitizer/fuzz lanes, CodeQL, the API-benchmark lane — none exist); (3) `docs/PREBUILT-ARTIFACTS.md` §2/§5 still cite the **deleted** composite action (`.github/actions/install-driver/action.yml`) and the **deleted** workflows (`driver-install.yml`, `core-install.yml`, `staged-scenarios.yml`) as the consumption mechanism, with the same class of stale references in DRIVER-RELEASE/DRIVER-CONTRACT/CI-LOCAL/CI-SCENARIOS. Nothing currently fails at runtime (both floors ≤ the actual v0.2.4 release), but the trail's own "one pin source per dependency" lesson is violated by the tree, and an engineer reading PREBUILT-ARTIFACTS.md would look for an action that does not exist.

## What the reference does

The reference product's install posture is a single coherent contract (architecture/02 §5 — the K-parameter map): one driver-install mechanism, versioned coexistence, verified teardown. The audit's own board (PARAMETERS Group K) and the CI consolidation's design intent (566c483: "windows-only 3-lane consolidation") were supposed to leave exactly one floor pin and docs matching the tree.

## What our implementation does

- Two live FloorVersion pins: `release.yml:291` (v0.2.4, since 566c483) and `installer.yml:118` (v0.2.3, since its creation 3e6a8fa — never bumped).
- `docs/CI-CURRENT.md:59-61` claims ONE pin source; `:145` generalizes the lesson.
- The real driver consumers per the tree: `installer.yml`, `release.yml`, and `test-benchmark.yml` (indirect, via the combined core release's assets + `Install-ReleaseDriver.ps1`) — none of them the composite action or 3 workflows that PREBUILT-ARTIFACTS.md §2/§5 name; `.github/actions/` does not exist at all (verified: `.github/` holds only `workflows/`, `scripts/`, `scenarios/`).
- README.md:125-132 describes Ubuntu/MSVC sanitizer/fuzz/CodeQL/API-benchmark lanes — all deleted by the consolidation; CI-CURRENT.md's Files section and lane table omit `installer.yml`; RELEASE-NOTES.md vNext repeats the 3-lane claim.

## Gap

Falsifiable per item: (1) `grep -rn "FloorVersion" .github/workflows/` returns two pins with different values while CI-CURRENT.md:60 asserts one; (2) `ls .github/workflows/` returns 4 files while CI-CURRENT.md/README/RELEASE-NOTES describe 3 (and README's Status describes lanes that do not exist); (3) `test -d .github/actions` fails while PREBUILT-ARTIFACTS.md:37 cites the action path as the consumption mechanism, and its §5 Consumers table lists three nonexistent workflows while omitting the three real consumers.

## Impact

CI/packaging maintainability and onboarding: the pin split means a future driver release floor bump (e.g. requiring v0.3) must be made twice and can silently diverge — the exact drift class the "one pin source" lesson was written to prevent (a lane that reads only CI-CURRENT.md believes the floor is uniformly v0.2.4). The stale-path cluster sends engineers to nonexistent files (the composite action, driver-install.yml) and hides the real consumption seats. No runtime defect today (both pins ≤ the shipped v0.2.4), hence MAJOR not SEVERE.

## Reproduction / verification

All sites read first-hand this session at core `35adcb9`: the two workflow pins (quoted in Core references), CI-CURRENT.md:55-65 and :145, README.md:125-132, PREBUILT-ARTIFACTS.md §2/§5 (every stale quote captured), DRIVER-RELEASE.md/DRIVER-CONTRACT.md/CI-LOCAL.md/CI-SCENARIOS.md greps ("driver-install.yml|core-install.yml|staged-scenarios.yml|actions/install-driver"), `.github/` tree listing (no actions/ dir), RELEASE-NOTES.md vNext. Git archaeology: release.yml has pinned v0.2.4 since 566c483 itself; installer.yml since 3e6a8fa, never bumped.

## Fix plan (for the engineering team)

1. Decide the floor: if the installer lane accepts v0.2.4 semantics (it should — its teardown contract assumes the versioned-aware uninstall), bump `installer.yml:118` to `-FloorVersion v0.2.4`; otherwise split CI-CURRENT.md's claim honestly ("release lane v0.2.4 / installer lane v0.2.3 — deliberate, reasons"). One commit, both sites.
2. Rewrite `docs/CI-CURRENT.md`: 4-lane table incl. `installer.yml` (its trigger, the S-038 Inno build, its own AST gate), the true Files list, and the corrected pin-source sentence.
3. Rewrite `docs/PREBUILT-ARTIFACTS.md` §2/§5 to the real mechanism: release.yml's driver tier (floor + dev-sign posture), installer.yml (floor + job-local test-sign), test-benchmark.yml (indirect via the combined release + Install-ReleaseDriver.ps1); drop the composite-action text, the allow_fallback/timestamp_server composite parameters, and the "api-benchmark" citations; keep the §11/asset-shape content (still true).
4. Sweep the same class: DRIVER-RELEASE.md:54/86/105-106/139/210 (api-benchmark/comparative-bench citations + the v0.2.3 "current" floor), DRIVER-CONTRACT.md:26, CI-LOCAL.md (add the supersession banner like CI-SCENARIOS's; fix the cost table), CI-SCENARIOS.md:16 (seed-manifest.json path).
5. README.md:125-132 → the current 4-lane windows-only description; add the installer/packaging row to README's Documentation table (S-038's milestone is currently absent); RELEASE-NOTES.md vNext gains a correction line under the existing "3-lane" text (history stays, a "corrected: 4 lanes" note follows).
6. Acceptance: a doc-truth grep gate in checks.yml (optional but cheap): assert `grep -c "FloorVersion" .github/workflows/*.yml` equals the count CI-CURRENT.md declares; assert no doc cites `.github/actions/` or the three deleted workflow names as current.

## Tests to add

- `DocsCI-Shape-Truth` (the grep gate above — fails on README/CI-CURRENT/PREBUILT drift)
- `FloorVersion-Single-Source` (fails when the workflows' pin count ≠ the declared count or pins disagree)

## Related findings

- **M-086** (the driver FileVersion pin — same release-posture family)
- **N-095 / N-104** (the other doc-truth clusters — different sites; this file owns the CI-shape/pin cluster)
- **S-038** (the installer lane whose workflow the docs omit)

## Open questions

None.
