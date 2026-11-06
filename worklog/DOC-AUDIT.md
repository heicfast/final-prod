# DOC-AUDIT — every documentation surface vs the product as it actually is

Audit date: 2026-11-06 (the debug-and-release session, post D-PRIV-01/
D-CONS-01/D-ENUM-01-UI/D-UI-01/W-01/W-02/U-01..04 landing, pre-0.9.0 cut).
The register names every doc, its status, and what changed this session.

## Truth corrections landed this session

| Doc | Status | What was wrong → what is true now |
|---|---|---|
| README.md | TRUED | The privilege paragraph described the lazy refusal model; now states the launch-time requireAdministrator manifest (D-PRIV-01) |
| installer/recoverywizard.nsi (header comment) | TRUED | Claimed "the app itself runs elevated (requireAdministrator" while the exe had no manifest; now describes app.manifest + the v0.8.x hallucination note |
| installer/README.md | TRUED | The Finish-page claim (inherited elevation only) → the manifest posture; the restart-flow paragraph → the truthful-refusal posture |
| docs/INTEGRATION-PLAN.md §7.4 | ERRATUM ADDED | The sign-off was a decision never implemented through v0.8.x; the erratum records the landing (0.9.0) + the retired inner-setup description |
| docs/INSTALL-FOOTPRINT-CURRENT.md | STALE-BANNERED | The whole sheet froze at the v0.6.0-rc1 driver era (RDA, inner setup) — a staleness banner now warns and points to installer/README.md + worklog/INSTALLER-AND-PRIVILEGE.md; a full rewrite stays queued (the file describes a tree that no longer exists) |
| docs/ARCHITECTURE.md | TRUED | The banner+restart description → the manifest posture (D-PRIV-01) |
| docs/INSTALLER.md | TRUED | The privilege-page description → the manifest posture |
| docs/RELEASE.md | TRUED | The restart-action paragraph → the manifest posture |
| docs/MSIX-ASSESSMENT.md | TRUED | The restart-flow blocker phrasing → the manifest-incompatibility phrasing |
| docs/INTEGRATION-GAPS.md | TRUED | The "elevation UX = banner + restart wiring (P4)" entry → the manifest is the surface; banner retired |
| docs/PRODUCT-WIRING.md A.8 | TRUED | The historical driver-status row's replacement note → the manifest posture |
| docs/PARITY-DELTAS.md | EXTENDED | D-I3's history trued (the hallucination years); the session's deltas D-P1..D-P7 added with corpus citations |
| .github/workflows/release.yml (notes template) | TRUED | The shipped-notes privilege paragraph described the retired restart flow → the manifest posture |

## Verified accurate (no change needed)

| Doc | Note |
|---|---|
| docs/BUILD.md | build lanes + toolchain (unchanged by this session; build.ps1's D1 gate is lane B's addition) |
| docs/DEV-SIGNING.md | signing posture (user-mode self-signing; unchanged) |
| docs/TESTING.md | suite inventory (the session ADDED tests; counts in worklog.md, not restated here) |
| contracts/README.md + command-surface.json + error-codes.json | boundary truth (the W-01 additions are subscription surfaces; the bridge events were already documented in event-mapping.md) |
| engine/README.md, engine/AUDIT-*.md | engine-side (lane B's seam; their D2 notes ride their commits) |
| ui/README.md, ui/docs/, ui/worklog.md | the UI sub-project's own docs (untouched surfaces) |

## Queued (honest open items)

- docs/INSTALL-FOOTPRINT-CURRENT.md full rewrite to the 0.9.x tree (the
  banner points readers to the truth meanwhile).
- The "What changed in this release" notes body is composed AT CUT TIME
  for 0.9.0 (the template's stale 0.8.x bullet list is replaced then —
  tracked in the release checklist, not left to memory).
- worklog/DOC-AUDIT.md (this file) re-verified at the release tag.
