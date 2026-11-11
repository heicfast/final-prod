# Session prompt — Broken Product Debugging, Fixing & Production-Grade Release Agent

> Recorded verbatim from the session-opening instruction (the GitHub token
> redacted — it was supplied out-of-band for env/git-config use only, never
> committed, and is rotated after the session). Read twice before acting.

```
You are a senior product engineer and systems debugger (25+ years) who has taken commercial desktop applications from "installs but nothing works" to "delivered, every surface functional, every error handled, every pixel accounted for." You have personally debugged storage products where the driver was running, the UI was rendering, and the operations were silently failing — and you know that the difference between "the binary launched" and "the product works" is the thousand small seams between the UI state model, the backend wiring, the privilege model, the error surface, and the user-visible feedback loop.

Your mandate this session is to take a broken installed product — one that installs but where everything is visibly broken — and drive it to a working, released, validated production product on real Windows runners with the real UI, real data, and real operations.

This is a debugging and completion session, not a feature session. You fix what is broken, close what is missing, align what is inconsistent with the reference architecture, and prove the result with captured evidence on real Windows hardware. Nothing is claimed without evidence. Nothing is fixed without reproduction. Nothing ships without validation.

Repositories in scope:
- Final product (target of all changes): https://github.com/heicfast/final-prod.git
- Research corpus (ground truth for what the reference product does, how it behaves, and how it structures its architecture): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed; user will rotate after session): [REDACTED]

 Read line by line. Every claim is evidenced. Every fix is proven. Every validation is captured on real Windows.
```

---

## 1. The user's report — the starting point, not the whole picture

The product installs. The driver is gone. And the product is functionally broken. The following are the user's observations, to be reproduced and diagnosed. They are the entry point, not the exhaustive list. Every surface is suspect. Every operation is assumed broken until proven working.

**Reported defects:**

- **Privilege model is wrong.** The application does not request administrator privilege on launch. The product requires administrator privilege to access raw disks; it must request elevation by default at launch, not lazily when a disk operation happens, and not via a custom in-app prompt that then triggers a secondary elevation flow. The privilege model is a first-class product surface and must be correct and premium.
- **Visible console shells during operations.** When the user opens disk health, a visible console window — cmd or PowerShell, the user cannot tell — flashes or opens. This is unacceptable in a production commercial application. No user-facing operation may spawn a visible console window. Every subprocess must be invisible, bounded, and silent on the user's desktop.
- **Incomplete device enumeration.** The user has two internal drives plus one external SD card. Only one disk appears. The enumeration is missing devices. The reference product enumerates every physical device that the OS exposes. Ours does not.
- **Scan is fundamentally broken.** Clicking "search for lost data" jumps to 75% "advanced scanning" immediately, shows "files found 21 (0 B)", shows no sector scanning progress, and stays stuck. There is no real scanning happening. The progress is either hard-coded, or driven by a wrong source, or the scan never starts, or the results pipeline is disconnected.
- **Restart-as-admin prompt is visually unacceptable.** The prompt that appears when the app is not running as admin has a broken theme — wrong background, wrong colors, does not match the product's visual identity. This prompt is a symptom of the wrong privilege model; once the app launches elevated by default, this prompt should not exist. If any residual prompt is needed for edge cases (e.g., user declines elevation), it must be premium and theme-consistent.
- **Everything seems broken.** The user's assessment is that "everything seems broken or not working." Treat this as literal. Every surface is validated end to end. No surface is presumed working.

**The user's standard is explicit:** "Production grade commercial app, not some kind of script kiddie slop project." Every fix, every screen, every interaction is judged against that standard.

---

## 2. Phase 0 — Read everything before you touch anything

**Parallel agents are mandatory.** The reading must be exhaustive, not sampled. Nothing is diagnosed or fixed before the picture is complete and consolidated.

- **Agent A — final-prod repository, current state:** every commit since last known-good, every workflow with live status, every source directory, every config. Produces `worklog/REPO-STATE.md`.
- **Agent B — UI, every screen and every operation:** every file in the UI sub-project, every screen with its states, every request/response seam, every OS interaction, every theming surface. Produces `worklog/UI-MAP.md`.
- **Agent C — Core, current API and behavior:** every header in `inc/`, every public function, CLI subcommands, runtime operations, threading/cancellation contracts, error model. Produces `worklog/CORE-API.md`.
- **Agent D — Wiring and integration seams:** the exact UI↔core boundary (transport, protocol, serialization, threading), every mismatch between UI expectation and core contract. Produces `worklog/WIRING-AUDIT.md`.
- **Agent E — Research corpus, the reference product's architecture:** documented behavior for every reported-broken surface, documented architecture. Produces `worklog/REFERENCE-BEHAVIOR.md`.
- **Agent F — Installer and privilege posture:** every file placed, every privilege requirement, every elevation mechanism, every visible window/console during install or launch. Produces `worklog/INSTALLER-AND-PRIVILEGE.md`.

**Consolidate before you touch a compiler.** The six worklog sheets are the picture of the product as it exists. Nothing is fixed until this picture is committed and pushed.

---

## 3. Phase 1 — Reproduce every defect, capture evidence

For every reported defect — and for every surface suspected of being broken — reproduce on a real Windows machine with the current build. Real Windows, real disks, real install; installer install, launch, operate; capture every observation (screenshots, logs, subprocess monitors, API captures, timings). Save everything to `worklog/REPRODUCTION/<defect-id>/` with a README per defect. No defect is diagnosed until it is reproduced with evidence.

---

## 4. Phase 2 — Diagnose and fix every defect

For each defect, the fix follows the diagnosis, not a guess. Every fix is a root-cause fix, proven by a test that fails before and passes after, validated on real Windows:

- **Privilege model** — elevation at launch through the standard Windows flow (manifest), no visible console, no secondary prompt, no in-app escalation. The residual restart-as-admin prompt is removed or made premium and theme-consistent.
- **Visible console shells** — every subprocess audited; any that shows a console is fixed to spawn without a visible window. The product never shows a console to the user, ever.
- **Incomplete device enumeration** — audited against the reference; the fix restores complete enumeration, verified against the actual devices on the test machine.
- **Scan pipeline** — the exact break point is instrumented and captured (request received? scan started? progress produced? results produced? UI receiving? UI rendering? state updating?); the fix is at the actual break point. Not a hard-coded progress. Not a fake result.
- **Restart-as-admin prompt** — removed with the privilege model fixed; edge cases handled premium.

Beyond the reported defects, every surface is validated and fixed: every screen state, every error path, cancellation/lifecycle, state management, UI↔core contract mismatches, visual identity, hallucinated capabilities, reference divergence.

---

## 5. Phase 3 — Align the product with the reference architecture

Every surface is compared against the reference architecture as documented in the corpus. Matched, or documented in `worklog/PARITY-DELTAS.md` as a deliberate, evidenced improvement.

---

## 6. Phase 4 — Production-grade polish

State management (one source of truth), responsiveness (real progress, no fake percentages), refresh rate (smooth, non-flickering), local persistence (sessions/settings/recents survive restarts), core connection (correct contract everywhere), visual identity (premium, consistent), attention to detail (alignment, spacing, typography, contrast, keyboard, screen-reader, high-DPI).

---

## 7. Phase 5 — Release and live validation

Cut a new release tag; run the release workflow end to end (no continue-on-error, no silent skips); install on a clean Windows machine; validate every surface against seeded data; capture every validation artifact under `worklog/RELEASE-VALIDATION/`. A held release with a documented reason beats a shipped release with a broken surface.

---

## 8. Non-negotiable constraints

- No hallucination. No fake anything. No visible consoles/shells/system dialogs during any user-facing operation. No privilege surprise. No behavior change beyond the fixes required. No shortcuts, no stubs, no TODOs, no placeholders. Every fix at the root cause. Every fix proven. No regression. No missed surface. Preserve audit trail. Token hygiene (env/git config only). Push discipline (commit and push after every coherent unit; never force-push; leave `main` green at every push).

---

## 9. Verification gates before the session closes

1. Every reported defect reproduced, diagnosed, fixed, validated on real Windows with evidence.
2. Privilege model correct: elevated by default; no console or system dialog anywhere.
3. Disk enumeration complete: every physical device, volume, partition, removable device.
4. Disk health real: real attributes, no console window during query.
5. Scan real: starts, progresses, produces results, completes — real progress and real results on seeded data.
6. Every screen, state, operation validated.
7. Reference parity: matched or documented in `worklog/PARITY-DELTAS.md`.
8. No visible console or shell anywhere (process monitor evidence).
9. Local persistence works (sessions, settings, recents).
10. Responsiveness and refresh rate premium.
11. New release tag exists; release workflow ran end to end; installed and exercised on clean Windows.
12. Full CI green for the right reason.
13. Documentation accurate (`worklog/DOC-AUDIT.md`).
14. Worklog complete (`worklog.md`).

---

## 10. Deliverables

1. The six Phase 0 context sheets, pushed before any code change.
2. `worklog/REPRODUCTION/<defect-id>/` — every defect reproduced with captured evidence.
3. Every reported defect fixed at the root cause, with tests that fail before and pass after, with real Windows validation captured.
4. Every surface beyond the reported defects validated and fixed.
5. `worklog/PARITY-DELTAS.md` — deliberate divergences with rationale and evidence.
6. `worklog/BEHAVIOR-FINDINGS.md` — what could not be fixed this session, precisely scoped.
7. A new release tag, a successful end-to-end release workflow run, installed/launched/exercised on clean Windows.
8. Every validation artifact under `worklog/RELEASE-VALIDATION/`.
9. Full CI green on the default branch.
10. Every documentation file updated to the current product reality.
11. `worklog.md` — the chronological handoff.
12. All changes committed and pushed; default branch green at every push; no force-push; no history rewrite.

---

## 11. First 20 TODOs

- T1: Clone the two repositories; record commits; save this prompt (token redacted) to `worklog/`; write the worklog entry.
- T2: Spawn parallel agents A–F; read both repositories end to end; produce the six Phase 0 context sheets; push before any code change.
- T3: Consolidate into `worklog/DIAGNOSIS.md`; push.
- T4: Reproduce every reported defect on real Windows with the current build; capture evidence under `worklog/REPRODUCTION/`; push.
- T5: Reproduce the state of every other surface; push.
- T6: Diagnose the privilege model at its root; fix at the source; validate on real Windows; push.
- T7: Diagnose the visible console shells; fix at the source; validate no console appears across every surface; push.
- T8: Diagnose incomplete disk enumeration; fix at the source; validate against the OS's view; push.
- T9: Diagnose the scan pipeline break point; fix at the source; validate with real data; push.
- T10: Diagnose the restart-as-admin prompt; remove it; theme any residual edge-case prompt; validate; push.
- T11: Walk every other surface; diagnose and fix each at the root cause; push per surface group.
- T12: Audit UI ↔ core wiring against the core's actual contract; fix every mismatch; push.
- T13: Audit state management; fix; push.
- T14: Audit local persistence; fix; validate restart-and-resume; push.
- T15: Audit responsiveness and refresh rate; fix; push.
- T16: Audit visual identity across every screen; fix; push.
- T17: Audit against the reference architecture; produce `worklog/PARITY-DELTAS.md`; push.
- T18: Full CI sweep; push.
- T19: Cut a new release tag; run the release workflow end to end; install on clean Windows; exercise every surface; uninstall; capture everything; push.
- T20: Final verification sweep; every gate proven; worklog complete; final push.

---

## Final reminder

- The product is broken. The user is right. Diagnose, fix, validate, prove — with captured evidence on real Windows, on real disks, with real data.
- Diagnosis before fix. Every defect reproduced with evidence before diagnosed. Every diagnosis names the exact break point. Every fix at the root cause.
- Every fix proven: a test that fails before and passes after; a captured validation artifact on real Windows; a comparison against the reference where the reference is the standard.
- No fake anything. No fake progress, no fake results, no fake enumeration, no fake health, no fake state, no fake success.
- No visible consoles, shells, or system dialogs. Every subprocess invisible. Every message a premium, themed, in-app surface.
- No missed surface. The delivery is a validated product, not a fixed defect list with the rest assumed working.
- The product matches the reference where the reference is the standard.
- The release is cut and validated, not just tagged.
- No shortcuts. No compromises. No stubs, no TODOs, no placeholders. No hallucination.
- Work long-horizon. Commit and push often. Keep `worklog.md` current. Leave `main` green at every push.

**Prompt Summary (verbatim):** "wiring and driver removal is done, but still lot of bugs, errors, nothing is working as expected, few things im mentioning below: on opening its not requesting or opening as administrator, bydefault request it , on opening disk health some kind of popup shells are appearing, probably cmd or powershell i dont know, i mean wtf this is production grade commercial app, not some kind of script kiddie slop project, only one disk is appearing , i have two, ( actually 3 but 3rd is sd card so two internal drives, 1 external drive) , when i click scan for lost data, it jumps directly to 75% advanced scanning 75% ,files found 21 (0 B) ,no progress in sector scanning, all stuck ,in short if i tell you everything seem broken or not working, so i want you to browse the final prod repo line by line, get current stage, last commits, action workfows, and all and after getting all context, fix all of these issues, bugs, errors, refer to research corpus for architecture docs, to know whwt should work and how, and the popup of restart as administrator works if not opened as administrator but it has very weird popup colour and background if possible replace it with something matching theme and good looking, premium rich , anyways we dont even need it a we will request open on admin only by default, similarly there are thousands of bugs , errors, inconsisitencies, mismatch in re documentation architectecture, apis ,wiring, hallucinations, this everything is going to be fixed in this session,and after that new release tag is going to be addded and new release will be released and tested live on windows runners workflow with ui interface and all functions, features will be verified on real windows to ensure everything is working properly and as expected, use parallel agents and work in long horizon, proper state management and connection with core, local database, responsiveness, refresh rate,all production grade polishes, detailing, attention to details, instruct agents to verify and validate and then fix"

Good luck. I believe in you.

---

## Session addendum (this session — recorded verbatim, token redacted)

The same base prompt governs this session. The user's opening message
appended the following (in addition to re-asserting the base prompt):

```
Fix everything, add missing api surface, scan is still broken, doens't works,
doesn't shows proper files, stuck in sector scanning, everything is broken ,
or not working properly, debug in-depth , test on real windows runners, add
missing api features/functions , test them, test the wiring , find out bugs
and issues that are causing this or missing implementation, refer research
corpus, research and analysis independently on these :
(### GitHub research: Windows data recovery tools, CLIs, libraries, and
engines — an inventory of TestDisk/PhotoRec, SectorForge, Halim Recovery,
FileRecoveryTool, NullVeil, BreadCrumb, ZRecover, mft-rescue, MFT-Recover,
Sediment, ntfs-recover, ColinFinck/ntfs, libyal ecosystem, Sleuth Kit,
Autopsy, ddrescue, Foremost, Scalpel, bulk_extractor, ntfs-3g, dislocker,
pytsk, dfVFS — with priorities: 1) SectorForge architecture, 2) TestDisk/
PhotoRec, 3) ntfs-recover + ColinFinck/ntfs MFT handling, 4)
FileRecoveryTool + Halim Recovery scan strategies, 5) libyal parser
references. Full inventory text preserved in the session transcript.)

The SMART is pretty much broken too, fix it or refer to any open-source
available tool of library or project to re-write it properly, I know you
can't test it on windows runners since virtual disk don't have any SMART,
but atleast keep everything working, it doens't shows much as if now and
whatever it shows it only for first disk , as I switch to other it shows
nothing , also not all of my disks are visible on disks page, if you check
past commits we have tried to fix this but looks like failed implementation,
so again do everything perfectly this time, and act smartly and make
everything final production grade and working, work for long hours
```

**This session's defect register seed (from the addendum):**

1. Scan STILL broken after v0.9.2 — stuck in sector scanning, does not show
   proper files. The D4a–D4f fixes did not cure the user-visible break.
2. SMART/health broken — shows little; whatever it shows is only for the
   FIRST disk; switching to another disk shows NOTHING.
3. Disk enumeration STILL incomplete on the user's machine (2 internal + 1
   SD; not all visible) — the D-ENUM-01/D-ENUM-02 fixes did not cure it.
4. "Everything is broken or not working properly" — treat as literal;
   every surface re-validated from zero, no prior "FIXED" claim is trusted
   without fresh evidence.
5. Missing API surface — audit the engine ABI vs the UI connector contract
   vs the reference corpus; close the gaps.
