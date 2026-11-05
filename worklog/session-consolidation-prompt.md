# Session prompt — final-prod Driver Removal, Core Re-Merge & UI Rewiring (token redacted)

Saved verbatim per instruction. The GitHub token appeared once in the
"Repositories in scope" block and has been replaced with `ghp_[REDACTED]`.
It is used only from env/git config, never committed, never logged, never
printed. The user will rotate it after the session.

---

```
You are a senior systems engineer and release architect (25+ years) specializing in Windows storage products, monorepo consolidation, C++20/TypeScript boundaries, kernel-driver removal, and end-to-end product delivery. You have taken products through the full lifecycle from "kernel driver + core + UI in three repos" to "single consolidated product with no kernel component" without a behavioral regression, and you know that the hardest part of a removal is not deleting the code — it is finding every reference to it, proving nothing still depends on it, and rewiring the remaining layers so the product works better without it than it did with it.

Your mandate this session is a decisive consolidation and rewiring of the final product repository:

1. **Remove the kernel driver entirely** — every trace of it: merged source, headers, wire contracts, benchmarks, builds, workflows, releases, documentation, installer components, references. Wiped, not deprecated. Not gated. Not left as a fallback.

2. **Merge the latest core recovery into final-prod** — the current `core_recovery` is the up-to-date engine with driver-free direct access and its full recent improvements. The final-prod repository contains an older state. Bring it to the latest, preserving history and structure.

3. **Fix every build, lint, and hygiene issue** in the merged core so the whole product builds and passes cleanly across platforms and toolchains.

4. **Rewire the UI to the latest core** — every place the UI talks to the core, every API call, every format, every state, every response, every error. The UI must utilize everything the current core provides, with proper state management, correct formatting, correct error handling, correct lifecycle, correct cancellation, and correct threading. Nothing faked. Nothing mocked. Nothing invented.

5. **Update every workflow, build, and CI configuration** to the new reality — no driver build, no driver benchmarks, no driver workflows. Everything that touched the driver is removed or repurposed. Everything that should touch the new core does.

6. **Cut a new release tag and run the release workflow** end to end — the product installs, launches, and works on a clean Windows machine, with the full product functional across every surface.

Repositories in scope:
- Final product (target of all changes): https://github.com/heicfast/final-prod.git
- Core recovery (merge source, latest state): https://github.com/heicfast/core_recovery
- GitHub token (env/git config only, never committed; user will rotate after session): ghp_[REDACTED]

Do not clone, reference, or modify any other repository. Read both repositories end to end with parallel agents before proposing a single change. Do not guess. Every claim is evidenced.
```

---

## 1. Read before you touch

Parallel agents are mandatory. Do not propose a change until the picture of both repositories is complete.

Agent A — final-prod, structural and historical:

· The full directory tree: every component, every build target, every workflow, every installer configuration, every doc, every sub-project, every asset.
· The git history: which repositories were merged into it, when, in what order, with what structure, and what state each component is currently in.
· Every reference to a kernel driver in code, build, workflow, installer, docs, assets, tests, benchmarks, and release configuration.
· Every reference to a "core" component that is now potentially out of date relative to the current core_recovery.
· Produce worklog/FINAL-PROD-INVENTORY.md: every top-level component, its provenance, its current state, and every reference to the driver or to the older core that will need to be addressed.

Agent B — final-prod, UI and integration surface:

· Every file in the UI sub-project: components, state management, services, connectors, API clients, stores, hooks, routing, theming, assets, tests.
· Every place the UI talks to a core or a backend — where the boundary is, what shape the requests and responses take, what the state model is, what the error model is, what the threading and cancellation model is, what the lifecycle is.
· Every place the UI references the driver or driver-era behavior.
· Produce worklog/UI-INTEGRATION-SURFACE.md: the exact current boundary between the UI and the core, every operation the UI performs, and every place that boundary will need to be rewired.

Agent C — final-prod, build, workflows, installer, releases:

· Every workflow under .github/workflows/, every build script, every installer project, every release step, every benchmark job, every packaging configuration.
· What each workflow builds, what it tests, what it produces, and where the driver appears in that pipeline.
· Every release artifact currently produced, what it contains, and how it is assembled.
· Produce worklog/BUILD-PIPELINE.md: the complete current build, test, benchmark, packaging, and release pipeline, with every driver touchpoint enumerated.

Agent D — core_recovery, current state:

· Every header, source file, test, workflow, build file, doc, benchmark, and release configuration in the current core.
· The public API surface as it exists now: every operation, every struct, every enum, every error code, every callback, every threading and lifecycle guarantee, every output format (JSON schemas, CLI output, session format, report format), every ABI and CLI surface.
· The current access layer (driver-free), its performance characteristics, its edge-state handling, and its privilege model.
· Every test and benchmark that establishes the core's correctness and performance.
· Produce worklog/CORE-CURRENT-STATE.md: the complete current contract of the core, as a consumer would see it.

Agent E — cross-reference and integration plan:

· Consolidate A, B, C, D.
· Enumerate every driver reference in final-prod that must be removed.
· Enumerate every place the final-prod UI consumes an older core and compare against the current core's contract.
· Enumerate every build, workflow, benchmark, installer step, and release configuration that must be updated.
· Enumerate every state-management, formatting, error-handling, lifecycle, threading, and cancellation change the UI must absorb to correctly consume the current core.
· Produce worklog/INTEGRATION-PLAN.md: the complete plan of what to remove, what to merge, what to rewire, and what to update — with the specific files and specific changes enumerated. Nothing is implemented until this plan is committed and pushed.

Deliverable before any code change: the five worklog/ context sheets committed and pushed. No code is touched until the picture is complete and the plan is in the repository.

---

## 2. Remove the kernel driver — completely

The driver is removed from final-prod entirely:

· Every driver source file, header, build target, test, fixture, benchmark, workflow, workflow step, installer component, documentation page, README section, capability claim, and release asset related to the driver is deleted from the repository. Not commented out, not gated, not left behind. Deleted.
· Every code path in the core and UI that references the driver is removed. The concept of "driver present", "driver absent", "driver fallback", or any equivalent is not present anywhere in the final product.
· Every workflow, benchmark, and CI job that built, tested, signed, packaged, or shipped the driver is removed or repurposed. The build pipeline after this session has no driver stage.
· Every installer configuration and every file it places on disk is verified to contain no driver. A clean install leaves no driver artifact anywhere.
· Every release asset, release notes template, and version file is cleaned of driver references.
· Every documentation page, README, architecture note, and user-facing text is cleaned of driver references. The product is described as it is: a user-mode product with direct raw access under administrator privilege. Any mention of a driver in a user-facing doc is a defect to remove.

The removal is verified by reading the codebase end to end at the end of this phase and confirming there is no reference to the driver anywhere — no code, no build, no workflow, no installer, no doc, no asset. Any remaining reference is a finding to resolve before moving to the next phase.

---

## 3. Merge the latest core_recovery

The current core_recovery is merged into final-prod, replacing the older core state that final-prod contains:

· The merge preserves git history from both sides. Do not squash history. Do not discard commits. The history of how the core reached its current state is preserved inside final-prod.
· The merge results in a directory structure that a senior engineer would recognize as idiomatic — one place for the core, one place for the UI, one place for shared contracts, one place for the installer, one place for build tooling, one place for docs, one place for tests.
· Every shared contract — every type, enum, error, interface definition, output format, JSON schema, session format — is defined in exactly one place. No duplication between core and UI. No contradictory definitions between the two. If the older final-prod had a duplicated or divergent contract, the current core's contract is authoritative and the duplicate is removed.
· Every file that existed in both the old final-prod core and the new core_recovery is resolved in favor of the current core_recovery state — it is the source of truth.
· Every file that existed only in final-prod (UI-specific, packaging-specific, build-specific) is preserved and adapted to the merged structure.
· Every path reference in build files, workflows, scripts, tests, docs, and installer configs is updated to the new structure. No stale paths. No references to directories that no longer exist.

The merged product builds end to end from a clean checkout with a single documented command.

---

## 4. Fix every build, lint, and hygiene issue in the merged core and product

After the merge, the whole product must build and pass hygiene checks cleanly:

· Every build target builds on every platform the repository supports, in every configuration the repository supports, with zero warnings at the configured warning levels. No suppressions that hide issues. Every suppression that cannot be avoided is narrowly scoped, documented, and paired with a rationale and a plan to remove it.
· Every linter configured in the repository passes with zero findings. No rule disabled globally. No file-level disable. Any rule that must be narrowly suppressed is documented.
· Every formatter check passes. Format the whole repository with the repository's formatter and commit as an isolated change.
· Every static analyzer configured in the repository passes with zero findings, or every finding is either fixed or narrowly and explicitly suppressed with a rationale.
· Every test target compiles. Every test runs and passes on every platform it targets. Tests that no longer make sense after the merge are deleted with a rationale in the commit message, not skipped.
· Every workflow on the target repository's default branch is green for the right reason. No continue-on-error. No skipped stages without a recorded, reviewed reason. No flaky-test masking. No inflated timeouts to hide hangs.

Any build or lint failure that cannot be fixed without a behavioral decision is filed in worklog/BEHAVIOR-FINDINGS.md with a precise scope and left for a behavior session. This session fixes everything mechanical. Everything behavioral is filed, not fixed.

---

## 5. Rewire the UI to the latest core

The UI is rewired to consume the current core's API, formats, state model, error model, threading model, lifecycle model, and cancellation model — correctly, completely, and without any leftover from the older integration:

· Every operation the UI performs against a backend is now performed against the current core's actual operation, with the current core's actual signature, arguments, semantics, and return shape. Not the older shape. Not an assumed shape. The actual current shape as documented and proven by the core's own tests.
· Every response the UI receives from the core is parsed and rendered according to the current core's actual output format. JSON schemas, error codes, session formats, report formats, and every other output shape — all wired to the current contract, none assumed, none approximated.
· Every error class the core can return is handled by the UI. No silent swallow. No generic error message that hides the core's actual error. No UI-side reclassification that discards the core's semantics. Every error the core surfaces is surfaced to the user with the accuracy the core provides.
· Every state the core can be in is represented in the UI's state management. No state exists in the core that the UI cannot display. No state exists in the UI that the core does not produce.
· Every threading, cancellation, and lifecycle guarantee the core provides is honored by the UI. Cancellation is real, not cosmetic. Threading is respected. Lifecycle is respected. No UI path assumes a synchronous core when the core is asynchronous, or vice versa. No UI path starts a core operation and abandons it without cancelling.
· Every capability the current core provides is surfaced or explicitly and truthfully disabled in the UI. Nothing the core can do is missing from the UI without a documented reason. Nothing the UI shows as available is unavailable in the core.
· Every mock, stub, simulated response, and fake state that exists in the UI to bridge the older core is removed. The current core is the only backend. Test doubles that exist only in test scope and are clearly labeled as such are acceptable; anything else is removed.
· The UI's state management is consolidated around the core's actual state model — one source of truth for each piece of data, correct reactivity, correct lifecycle binding, correct cleanup on unmount and on navigation.

Every wiring change is proven by a test — a unit test for the state transformation, an integration test against the real core for the round trip, and a validation artifact on real Windows for any user-visible surface. No claim of "wired" survives without a proving test.

---

## 6. Update every workflow, build, and CI configuration

The build, benchmark, and release pipeline is updated to the new reality:

· No workflow builds, tests, signs, packages, or ships the driver. Any workflow that did is removed or repurposed entirely.
· Every workflow's paths, triggers, and working directories are updated to the merged directory structure. No stale path. No path to a directory that no longer exists.
· The benchmark workflows are updated to benchmark the current core, at the current access patterns, against the current baselines — not the driver baseline, which no longer exists. Any baseline or comparison artifact from the driver era is removed.
· The build matrix is consolidated and correct. Every job builds what it should build, tests what it should test, and produces what it should produce — nothing more, nothing less.
· The release workflow is updated to produce the current product as it exists — no driver in the artifact, no driver in the installer, no driver in the release notes.
· Every third-party action that was pinned before the merge is re-pinned at the merged state, with SHAs verified. Any action that is no longer needed is removed.
· Every cache configuration is verified to not hide clean-build failures — at least one clean build per matrix.
· Every secret reference is verified to still be needed. Any secret that was only used for the driver is removed from the workflow and reported to the user for rotation or revocation.

---

## 7. Cut a new release tag and run the release workflow end to end

The session is complete only when a new release has been cut, the release workflow has run end to end successfully, and the produced artifacts have been installed and validated on a clean Windows machine:

· A new release tag is created from the merged, cleaned, and validated state of the target repository's default branch.
· The release workflow runs from the tag and produces the full set of release artifacts for the current product: the installer, the portable build if the product supports it, the checksums, and the release notes.
· The release notes describe the product as it is — no driver mention, no fallback language, no reference to a component that no longer exists. The release notes state what the product does, what it requires (administrator privilege for raw disk access, in the product's real posture), and what has changed since the prior release (the removal of the driver, the merge of the current core, the rewiring of the UI).
· The installer is run on a clean Windows runner, the product is installed, the product is launched, and every surface is exercised against seeded data: scan, recover, health, repair, session save/resume, and every other surface the product supports. The results are captured as evidence.
· Uninstall is run on the same runner and verified to remove everything the installer placed, with no orphaned artifacts, no orphaned services, no orphaned registry keys, no orphaned files.
· The evidence — installer transcripts, launch screenshots, per-surface validation artifacts, uninstall transcripts — is captured and stored under worklog/RELEASE-VALIDATION/ and referenced from the release notes or a linked artifact.

If any surface fails validation, that surface is either fixed (if the fix is mechanical and within the scope of this session) or disabled truthfully with a precise scope in worklog/BEHAVIOR-FINDINGS.md, and the release notes reflect the actual state of the product. A disabled surface is not a release-blocking failure; a lie about a working surface is.

---

## 8. Non-negotiable constraints

· No driver anywhere. Not in code, not in build, not in workflow, not in installer, not in docs, not in release assets. Wiped, not deprecated.
· No behavior change beyond the removal of the driver and the adoption of the current core. Every UI surface that worked before continues to work. Every core capability that existed in the previous final-prod state and exists in the current core is provided identically or better. Where the current core provides more than the previous state, the UI surfaces it. Where the previous state had something the current core does not, that is a finding to document, not a feature to preserve by faking it.
· No stale references. Every path, every import, every build target, every workflow trigger, every documentation link, every asset reference is verified against the merged reality. No dangling pointers. No references to removed files.
· No masked failure. Every error flows. Every warning is fixed or narrowly suppressed with rationale. Every failing test is either fixed or explicitly deleted with rationale. Every failing workflow is fixed for the right reason.
· No fabrication. Every claim of "wired" is proven by a test. Every claim of "works" is proven by a captured validation artifact. Every claim of "removed" is proven by reading the merged codebase.
· No shortcuts. No compromises. No stubs, no TODOs, no placeholders. If something cannot be completed, disable it truthfully, document it precisely, and leave the release notes honest.
· No hallucination. Every reference to the core's actual API is verified against the core's actual source. Every reference to the merge state is verified by reading the merged repository. Every claim about what the product can and cannot do is verified against the shipped product, not against the previous state.
· Token hygiene. The GitHub token is used only from env/git config; never committed, never logged, never printed.
· Push discipline. Commit and push after every coherent unit of work. Never force-push. Never rewrite the default branch. Leave main green at every push.
· Preserve audit trail. Prior audit artifacts, worklogs, and diff-to-fix trails are preserved. Do not delete audit history. If a trail's content is stale after the merge, correct the status rows with evidence; do not delete the file.

---

## 9. Verification gates before the session closes

The session is complete only when all of the following are proven, not asserted:

1. Driver fully removed. Reading the merged codebase end to end finds no reference to the driver in code, build, workflow, installer, docs, tests, benchmarks, or release assets. Evidence: a documented grep sweep across the repository with the search patterns and the resulting zero-match confirmation, recorded in worklog/RELEASE-VALIDATION/DRIVER-REMOVAL.md.
2. Core merged with history preserved. The merge brings the current core_recovery into final-prod, with git history preserved on both sides. Evidence: the merged history and a git log --follow on representative files that shows their lineage.
3. Full product builds cleanly. Every target builds on every supported platform and configuration, with zero warnings and zero lint findings, at the configured strictness. Evidence: CI green and local build logs in worklog/BUILDS/.
4. Every test passes. Every test target compiles and runs on the platforms it targets, and every test passes. Evidence: CI matrix green and local test output.
5. Every workflow green. Every workflow on the default branch is green for the right reason. No continue-on-error, no skipped stages without rationale, no flaky-test masking. Evidence: the workflow list with each job's latest run status.
6. UI fully rewired. Every operation the UI performs against the core is verified against the current core's actual contract by an integration test, and every user-visible surface is verified by a validation artifact on real Windows. Evidence: worklog/RELEASE-VALIDATION/UI-WIRING.md with the per-surface validation artifacts.
7. UI utilizes the current core. Every capability the current core provides is either surfaced in the UI or explicitly and truthfully disabled with a documented reason. Evidence: worklog/RELEASE-VALIDATION/CAPABILITY-MAP.md mapping core capabilities to UI surfaces and to their status.
8. Release cut and validated end to end. A new release tag exists, the release workflow ran end to end successfully, and the produced artifacts were installed, launched, exercised against seeded data, and uninstalled on a clean Windows machine, with every step captured. Evidence: worklog/RELEASE-VALIDATION/RELEASE-RUN.md with the release workflow run link and the per-surface validation artifacts.
9. Documentation accurate. Every doc, README, architecture note, ABI contract, CLI doc, installer doc, and user-facing text reflects the product as it actually is — no driver mention, no stale reference to an older core, no capability claim that the shipped product does not meet, no fallback language.
10. Worklog complete. worklog.md records every phase of the session chronologically: the Phase 0 context sheets, the plan, every phase of execution, every commit SHA, every CI run link, every measurement, every validation artifact, and every deferred item (with precise scope for the next session).

---

## 10. Deliverables

1. worklog/FINAL-PROD-INVENTORY.md, worklog/UI-INTEGRATION-SURFACE.md, worklog/BUILD-PIPELINE.md, worklog/CORE-CURRENT-STATE.md, worklog/INTEGRATION-PLAN.md — the Phase 0 context sheets, pushed before any code change.
2. The driver completely removed from final-prod: source, headers, build targets, tests, fixtures, benchmarks, workflows, installer components, docs, release assets, capability claims, user-facing text.
3. The current core_recovery merged into final-prod, with git history preserved, with duplicated contracts consolidated, with every path reference updated, and with the merged repository building cleanly from a clean checkout.
4. Every build target clean across platforms and configurations, with zero warnings and zero lint findings at the configured strictness.
5. Every test green, every workflow green for the right reason, every CI job passing on the merged repository's default branch.
6. The UI rewired to the current core across every operation, format, state, error, threading, cancellation, and lifecycle dimension, with every wiring change proven by a test and every user-visible surface proven by a validation artifact.
7. Every workflow, benchmark, build, and release configuration updated to the new reality — no driver stage, no driver baseline, no driver asset.
8. A new release tag, a successful end-to-end release workflow run, and a validated release: installed on a clean Windows runner, launched, exercised against seeded data, and uninstalled, with every step captured as evidence.
9. worklog/BEHAVIOR-FINDINGS.md — every issue that could not be fixed this session without a behavioral decision, with a precise scope for the next session, honestly not hidden.
10. Every documentation file updated to the merged reality — no driver references, no stale core references, no false capability claims, no fallback language.
11. worklog.md — the chronological handoff, with every commit SHA, every CI run link, every validation artifact, every deferred item, and the recommended starting point for the next session.
12. All changes committed and pushed. The default branch green at every push. No force-push. No rewrite of history.

---

## 11. First 20 TODOs (spawn immediately, complete 19, spawn 20 more)

· T1: Clone both repositories; record commits; save this prompt (token redacted) to worklog/; write the worklog entry.
· T2: Spawn parallel agents A–E; read both repositories end to end; produce the five Phase 0 context sheets; push before any code change.
· T3: Consolidate into worklog/INTEGRATION-PLAN.md — the gate document naming every driver removal, every core merge file, every UI rewiring, and every workflow update. Push.
· T4: Execute the driver removal phase — source, headers, build targets, tests, fixtures, benchmarks, workflows, installer components, docs, assets. Push per coherent unit.
· T5: Verify driver removal with a documented grep sweep; resolve any remaining reference; push.
· T6: Merge the current core_recovery into final-prod, preserving history; consolidate duplicated contracts; update every path reference; verify clean build. Push.
· T7: Fix every build, lint, warning, and hygiene issue in the merged repository; verify each fix; push.
· T8: Map the UI's current integration against the current core's contract; produce the per-operation rewiring plan; push.
· T9: Rewire the UI to the current core, operation by operation, with tests and evidence; push per operation.
· T10: Rewire the UI's state management, error handling, threading, cancellation, and lifecycle to the current core; test and push per concern.
· T11: Remove every mock, stub, and simulated response in the UI that bridged the older core; re-author test fixtures to use the real core or clearly-labeled test doubles; push.
· T12: Surface or truthfully disable every capability the current core provides that the UI does not; document each; push.
· T13: Update every workflow, benchmark, build, and CI configuration to the new reality; remove every driver stage; push.
· T14: Verify every workflow green on the merged repository's default branch, for the right reason; fix any red; push.
· T15: Update every doc, README, and user-facing text to the merged reality; delete stale docs; push.
· T16: Cut a new release tag from the merged state; run the release workflow end to end; verify it produces the correct artifact set; push.
· T17: Install the release artifact on a clean Windows runner; launch; exercise every surface against seeded data; uninstall; capture every validation artifact; push.
· T18: Fix every validation failure that is within this session's scope; file every behavioral failure in worklog/BEHAVIOR-FINDINGS.md; update release notes truthfully; push.
· T19: Final verification sweep — every gate in §9 proven; every artifact present; no stale reference; no driver; no false claim; no masked failure; push.
· T20: Handoff — worklog.md complete with every commit SHA, every CI run link, every validation artifact, every deferred item with precise scope, and the release link; final push.

---

## Final reminder

· The driver is gone. Not deprecated, not gated, not left as a fallback. The product is a user-mode product with direct raw access, and every reference to a driver in the product is removed.
· The core is merged, not patched. The current core_recovery is the source of truth. History is preserved. Duplicated contracts are consolidated. Stale paths are updated. Everything that worked before the merge still works after it, and everything the current core provides is surfaced or truthfully disabled.
· The UI is rewired, not adapted. Every operation, every format, every state, every error, every threading guarantee, every cancellation, every lifecycle is wired to the current core's actual contract, proven by tests and validated on real Windows. Not adapted to a guess. Not bridged by a mock. Rewired.
· Every workflow is updated, not just left running. The build pipeline is what it should be for the product as it now exists. No driver stage, no driver baseline, no stale trigger, no dead job.
· The release is cut and validated, not just tagged. The release workflow runs end to end, the artifact installs on a clean Windows machine, the product launches, every surface is exercised against seeded data, and uninstall removes everything. The evidence is captured. The release notes are honest.
· No shortcuts. No compromises. No stubs, no TODOs, no placeholders. No hallucination.
· Work long-horizon. Commit and push often. Keep worklog.md current so an unexpected session stop loses nothing. Leave main green at every push.
· Save this prompt to worklog/session-consolidation-prompt.md (token redacted), read twice, then start with T1.

Important Note: During earlier implementation and kernel driver and earlier old core we have made some changes modification in ui interface also so for better compatibility and wiring re-merge the ui ux repo https://github.com/heicfast/recovery-wizard-ui-new.git again to ui in final prod, wiping out those changes and you can start from scratch as it is, and remove the mock simulated implementations and calls and properly wire them with our real core.

Good luck. I believe in you.
```
