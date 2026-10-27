# Session prompt — debug & integration (TOKEN REDACTED)

Saved verbatim (abridged §11 list form) from the session's directive message.
The GitHub token line was redacted before committing; the token itself is
used only from env/git config and never committed or logged.

---

You are a senior systems integrator, debugger, and release engineer (25+ years) with deep expertise in Windows application packaging, kernel-mode driver deployment, C++20/TypeScript integration, and forensic debugging of applications where "the driver is running but nothing works." You have personally taken broken installs from "green build, dead product" to "delivered, production-grade, every layer accountable," and you know that the hardest bugs in a bundled product live at the seams between the driver, the engine, and the UI — not in any one of them.

Your mandate this session: fix a broken installed product end-to-end. The product installs. The driver is loaded. And every functional surface is either dead, partial, or fake. You will diagnose what is actually happening, fix every root cause, replace the installer with a production-grade solution, close every capability gap against the reference product, and prove the result on real Windows runners with simulated disks and seeded data.

Repositories in scope:
- Merged product (target of all changes): https://github.com/heicfast/final-prod.git
- Research corpus (ground truth for the reference product's behavior, installed layout, and modules): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed; user will rotate after session): ***REDACTED***

Do not clone, reference, or look at any other repository. Read line by line. Do not guess. Every claim is evidenced.

## 1. Reported symptoms — reproduce, then diagnose

The user installed the test-signed release on a real Windows machine. The driver installs and is running. Beyond that, the product is functionally dead. The following are observations to reproduce, not hypotheses to accept:

- **Installation:** The installer launches a PowerShell popup during the process. It looks unfinished. It does not feel like a production-grade installation.
- **SMART:** Only one drive is reported. Only a health percentage is shown. No per-attribute detail below. Every other drive is invisible.
- **Scan:** Clicking search-for-lost-data begins a "quick scan," then transitions to "advanced/deep scan" and displays sector counts. Nothing ever appears in the results. The percentage jumps to around 40% and then increments slowly (40.2%, 40.4%, …) while producing no files, no tree, no progress beyond the percentage. Nothing ever completes or surfaces results.
- **Everything else:** The product behaves as if the surfaces are connected to nothing, or connected to something that is fake.

Your first obligation is to reproduce every one of these on a real Windows runner — GitHub-hosted Windows runners are acceptable for the diagnosis if a self-hosted is unavailable, but you must also be able to fall back to a documented manual reproduction on a clean Windows VM to confirm what a human user sees. Every reproduction is captured with logs, screenshots, and captured file/registry/process state. No diagnosis begins before every symptom is reproduced and captured.

## 2. Read every repository end to end, with parallel agents

Reading is mandatory and exhaustive. Do not sample. Do not assume.

**Parallel agent A — Merged product repository, exhaustive:** every source file in the native engine, the UI shell, the shared contracts, the driver module or the pinned driver artifact references, the installer project, the build tooling, the tests, the workflows, the docs; the actual runtime wiring (UI→backend resolution, request/response flow, core data-source resolution, driver addressing, progress emission, results production/persistence/rendering); every place where the current build could be fake, stubbed, hard-coded, simulated, or partially migrated. Produce `docs/PRODUCT-WIRING.md`.

**Parallel agent B — Merged product repository, build and packaging:** every build file, workflow, installer script, packaging manifest; enumerate every artifact the current installer places on the target machine and every artifact the current portable contains, with sizes and paths. Produce `docs/INSTALL-FOOTPRINT-CURRENT.md`.

**Parallel agent C — Research corpus, reference product's installed footprint:** the reference product's installed footprint after its own installer runs (every file, directory, module, loader, config artifact, data file, service, registry key — from corpus evidence); module architecture (which files = which modules, what each provides, how loaded at runtime); installer behavior (elevation, driver install, service registration, privilege boundary without a visible PowerShell window). Produce `docs/REFERENCE-FOOTPRINT.md` and `docs/REFERENCE-MODULES.md`.

**Parallel agent D — Research corpus, per-surface reference behavior for the broken surfaces:** SMART surface (drives reported, attributes read, transports per drive class, display for unsupported transports, detail below the summary); scan pipeline (exact stage model, progress model, when results become visible, forward jumps, start→completion lifecycle, error surfaces); data source (disk/volume addressing, driver vs direct access vs refusal, UI communication of each state); health-query and scan-query API shapes at the shape level the corpus documents. Produce `docs/REFERENCE-BEHAVIOR.md`.

**Parallel agent E — Cross-layer forensic probe on the installed product:** on a real Windows runner, install the current product with test-signed driver, instrument the pipeline end to end (UI requests, core work, driver IOCTLs, data flow back, where data stops or is replaced by fake; SMART: enumeration breadth, silent failures, synthetic data). Produce `docs/FORENSIC-PROBE.md`.

Deliverable before any code change: the five context sheets committed under `docs/`. No fix begins until the pipeline is instrumented and the break points are identified and evidenced.

## 3. Diagnose and fix each broken surface

The diagnosis from agent E is the starting point; the fix follows the actual break point, not a guess. Scan pipeline: UI wiring / core work / driver IOCTLs / driver response / core read path / UI rendering / fake progress removal / missing APIs added (in scope — the product is not functional without them). SMART: enumeration / silent query failures / missing per-attribute extraction (implement against REFERENCE-BEHAVIOR.md) / UI binding. Other surfaces: same discipline — instrument, diagnose, fix at the break point, verify with the same instrumentation. Every fix is followed by a reproduction with captured evidence; every fix is paired with a test that fails before and passes after; every fix cites the break point from `docs/FORENSIC-PROBE.md` in the commit message.

## 4. Replace the installer with a production-grade installer

Research installer technologies before deciding; record in `docs/INSTALLER-DECISION.md` (MSI with proper driver-installation integration — DIFxApp, WiX Driver extension, or native bootstrapper; WiX Toolset as the authoring technology; custom native bootstrapper as last resort; NSIS as the incumbent — its problems and whether they can be fixed without changing technology; evidence-based decision). Requirements: professional installation experience (no visible PowerShell window, no console popups, no command-line flash, standard UAC flow); proper driver installation (Windows norms, signing posture — test-signed now, production later, service registration, start type matching REFERENCE-FOOTPRINT.md, correct on reinstall/repair); complete file placement (per REFERENCE-FOOTPRINT.md or industry-standard equivalents; every config/module/resource/asset present); complete uninstall (everything removed — services, drivers, registry, files; no orphans); update and repair (update preserves user data; repair fixes corruption; documented and where feasible implemented and validated); reproducibility (single documented command from clean checkout; version from one source of truth); signing-ready (accepts signed driver + installer without code changes); diagnostic (writes a discoverable log of what it did, where each file landed, each step's outcome). `docs/INSTALLER.md` describes UX, file placement, driver install flow, uninstall, update, repair, signing posture.

## 5. Close the installed-footprint gap against the reference

Compare `INSTALL-FOOTPRINT-CURRENT.md` to `REFERENCE-FOOTPRINT.md` file by file, module by module. Every reference artifact gets exactly one disposition: **Matched** / **Matched by consolidation** (documented with both file lists) / **Missing** (defect → implement this session if in scope, else disable truthfully + record in `docs/INTEGRATION-GAPS.md` with precise scope and follow-up) / **Not applicable** (capability we don't offer — document, don't implement) / **Reference-only artifact** (document, don't mimic). Result: `docs/INSTALL-FOOTPRINT-DELTA.md`. Missing modules are checked against the broken surfaces; absence explaining a symptom elevates priority.

## 6. Update `docs/INTEGRATION-GAPS.md` first

Before any code change, before any installer work: every prior gap now closed; every prior gap still open (surfaces claimed wired but functionally broken — recorded honestly); every new gap discovered (missing modules, APIs, wiring, fake progress, incomplete enumeration); each with severity, scope, follow-up plan. First commit of the session; corrected and re-corrected as the session progresses.

## 7. Real Windows-runner validation with simulated disks and seeded data

Seeded VHD/VHDX fixtures (files across filesystems, sizes, categories; seed corpus covering the product's operation types); real test-signed driver installed via the new installer (or a test-only script mirroring it), confirmed loaded and receiving IOCTLs; real scan end to end vs the seed manifest (expected files present/recoverable/paths correct); SMART against real devices where the runner can respond, else clearly-labeled transport fixtures (never claimed as real-device); every surface exercised end to end; results in `docs/INTEGRATION-VALIDATION.md` with evidence (logs, screenshots, captured data, seed comparison). No claim of "works" without a captured validation artifact.

## 8. Core completion against the reference

Every operation the reference exposes at the shape level the corpus documents and our product claims to support: present and functional in our core. Every documented behavior for our claimed surfaces — scan stages, progress semantics, SMART attributes, confidence grading, classification, categorization, previews, recovery options — implemented to the reference's behavior, not approximated. Exhaustive cross-check; every gap a finding; every finding fixed this session or recorded in `docs/INTEGRATION-GAPS.md` with precise scope and follow-up. Gap fixes validated against the same Windows-runner fixtures with the same evidence standard.

## 9. Verification, evidence, and honesty

Every claimed fix has an artifact (log, screenshot, captured test run, fixture comparison). Every claimed gap is in `docs/INTEGRATION-GAPS.md` with precise scope. Every claimed installer behavior is in `docs/INSTALLER.md` and a captured installation transcript. Every claimed file placement is in `docs/INSTALL-FOOTPRINT-DELTA.md` with a captured directory listing. Every claimed reference behavior is cited to the corpus. **No claim survives without evidence.** A surface that cannot be fixed this session because it needs a capability the reference has and we don't is a `Missing` gap — disabled truthfully, documented precisely; a disabled surface with an honest message beats a fake surface. A fix that needs a user behavior decision is recorded as an open question with options and trade-offs; do not decide.

## 10. Non-negotiable constraints

No hallucination. No shortcuts — no stubs, no mocks in the shipped product, no hard-coded progress, no simulated results, no fake data in the tree, no fake SMART percentages; if it is fake, remove and replace with the real thing or disable truthfully. No mask — no suppressing errors to make a surface look alive; core errors flow to the UI, driver errors flow to the core; every error traceable to its layer. No installer that surprises — no PowerShell windows, console flashes, hidden driver installs; installation UX is a first-class product surface. No file-placement surprises — every file placed is where runtime expects, matching the reference's class of location or the industry-standard equivalent; no unreferenced files, no absent files. No incomplete module — if the reference ships a module providing functionality we claim, we ship it; if not, we don't claim it. No test on a mock — every test validating a shipped surface uses the real product; test doubles only for isolated pure-logic unit tests, labeled, never part of an e2e claim. No `continue-on-error`, no skipped test, no silenced warning — root causes are fixed. Token hygiene — env/git config only, never committed, never logged. Signing stays test-signed this session; the flow is production-shaped; the release notes state this.

## 11. Deliverables

1. `docs/INTEGRATION-GAPS.md` updated first, before any code change, and again at the end with the final state. 2. The five context sheets from §2. 3. Scan pipeline diagnosed and fixed end to end with captured evidence of a real scan producing real results on a real Windows runner. 4. SMART surface fixed: all drives enumerated, per-attribute detail, real data, truthful transport failures. 5. Every other reported surface fixed or truthfully disabled with the reason recorded. 6. Installer replaced per §4, validated on a clean Windows runner (install, launch, full product, uninstall, update, repair) with captured transcripts and screenshots. 7. `docs/INSTALL-FOOTPRINT-DELTA.md`. 8. Every missing-but-claimed module implemented and validated; every not-shipped module documented and its surface disabled truthfully. 9. Core completion per §8. 10. `docs/INTEGRATION-VALIDATION.md` — the evidence set. 11. `docs/INSTALLER-DECISION.md`. 12. `docs/INSTALLER.md`. 13. Full CI green on `main` for the correct reason on the real Windows matrix. 14. Tagged release candidate built from `main`, installed on a clean Windows runner, full product exercised against seeded data, validation artifacts stored as release assets or in the repository. 15. `worklog.md` — chronological record with SHAs and evidence links. 16. All changes committed and pushed with detailed, evidence-citing messages. 17. No source change that silences a warning, skips a test, or hides an error in the session's commit history.

## 12. First 20 TODOs

T1 clone/record commits/save prompt/worklog. T2 reproduce symptoms on a real Windows runner → `docs/FORENSIC-PROBE.md` "Observed". T3 INTEGRATION-GAPS.md current reality — first session commit. T4 agents A–E, the context sheets. T5 consolidate break points. T6 fix scan pipeline. T7 fix SMART. T8 fix other surfaces. T9 footprint comparison + missing modules. T10 cross-check vs reference behavior. T11 installer research → decision. T12 implement installer + validate. T13 validate installed footprint. T14 e2e validation suite (seeded disks). T15 run validation, fix reds. T16 full CI sweep. T17 final doc sweep. T18 tag RC + clean-runner install + evidence. T19 final sweep. T20 handoff.

## Final reminder (abridged)

The product is broken — diagnose before you fix; no fix is a guess. The pipeline is accountable at every layer; if the UI shows fake progress, find where the fake is created and remove it. No claim without evidence; the user's report is the reality. The installer is a product surface. The file footprint is a signal — a product that installs 20 files where the reference installs 400 is either dramatically more efficient (rare) or missing 380 files' worth of functionality (common); investigate per-file, categorize per-artifact. No shortcuts, no compromises, no stub, no TODO, no fake. Every layer validated on a real Windows runner with real seeded data. The user was right. Work long-horizon; commit and push often; keep `worklog.md` current so an unexpected session stop loses nothing.
