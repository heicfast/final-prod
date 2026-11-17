# DIAGNOSIS — session 2026-11-17 (SMART rebuild, scan pipeline repair, reference parity)

> **This session's Phase-0 is complete before this diagnosis**: five context
> sheets, committed and pushed before any code change — `worklog/PRODUCT-STATE.md`
> (agent A), `worklog/REFERENCE-SMART.md` (agent B), `worklog/REFERENCE-SCAN.md`
> (agent C), `worklog/REFERENCE-FOOTPRINT.md` (agent D), `worklog/VERIFICATION.md`
> (agent E). Every claim below cites those sheets; they cite the corpus and the
> source tree. The PRIOR session's diagnosis (2026-11-06) is preserved verbatim
> at the bottom of this file — most of its fix trains have since landed on main
> (ABI re-pin ba2b9347, scan-truth 693d4d60, surface-truth eb4bb5f3) and its
> strata are carried forward into §0 below.

## 0. Three defect strata, not one

**Stratum 1 — the user's installed build (v0.9.1, tag cdf66ff8, 2026-11-06).**
Every symptom the user reported is explained by defects that are FIXED on main
but were NEVER RELEASED (no tag after v0.9.1; 26 commits unreleased):
D-NVMe-01 (mis-packed NVMe query → only the first SATA disk had health),
D-ENUM-02 (shifted bus table), F-HLT-02 (UI-invented `healthScore` — the exact
"fake green percentage"), BF-31 (kind 'sd' never fired → hard-coded fake SD
card), D4f (the 605 s carve freeze), P-DEEP-01/P-HB-01/P-TICK-01 (deep stage
had no byte axis; heartbeat swallowed). PRODUCT-STATE §1.2, §7.1, §7.2.

**Stratum 2 — the current tree's residual breakage.** The ABI drift
(BF-ABI-01) is fixed in-tree (ba2b9347 + the bridge-integration layout gate),
BUT the integration lane itself is RED at HEAD: `it/tests/bridge_integration.rs:461`
assigns integer `0` to the `f64 frozen_run_s` (E0308) — commit e119cc71 fixed
only the declaration at :454. `ui-ci` clippy-strict is RED on the lib surface
(run 43108277889). `engine-ci` last ran (and failed) at 3415c71b — the
deliberate RED Phase-1 reproduction of the prior session; it has not run since
because no engine/** path was touched. Main is not green at HEAD. PRODUCT-STATE §5.

**Stratum 3 — registered divergences this session REVERSES (the structural
"works nothing like the reference").** Two rows of docs/PARITY-DELTAS.md that
recorded deliberate divergences are now mandated to match the reference:
- **D-P4**: ours = in-app `/health` page showing ONE selected disk at a time;
  the reference = a SEPARATE disk-health surface (DiskHealth.exe, Qt 5.15.2 QML,
  840×590 frameless, "EaseUS DiskHealth", drive selector "Which disk do you care
  about?", per-disk 4-state grade, ID/Attribute/Current/Threshold/RawValue/ST
  table, temperature card, tray-resident, launched from the home "Disk Health"
  card with `diskHealthIndex=<n>`). REFERENCE-SMART §1, §10; VERIFICATION V3/V4.
- **D-P6**: ours = one global 40/35/25-weighted curve (engine
  cr_scan.h:72-76 — location corrected by VERIFICATION V2; NOT contract.ts);
  the reference = per-phase 0-100% restart ("Quick Scanning %1, Files Found:"
  → "Advanced Scanning%1, Files Found:", gauge restarts at the phase boundary).
  REFERENCE-SCAN §1-§5; VERIFICATION V5 (byte-exact templates).
Plus two missing transports (below) and the unreleased-fix release gap.

## 1. Every reported defect → evidenced root cause → stratum → phase

| ID | Symptom (user words) | Root cause (evidenced) | Stratum | Fixed by |
|---|---|---|---|---|
| R1 | "reports only one disk" | D-NVMe-01: every real NVMe controller refused the mis-packed query → health only for the first SATA disk (cr_health.cpp:277-288); D-ENUM-02 misgrouping; ABI drift row-collapse (fixed ba2b9347) | 1+2 | RELEASE the train (Phase 4); the fixes exist in-tree |
| R2 | "fake green percentage with no attributes" | v0.9.1 UI computed `healthScore = max(88, 100-wear)` (F-HLT-02, health-page.tsx:100-107 pre-fix); NVMe disks have no ATA table and pre-NG-6/9 rendered empty; ex2 threshold axis unbound pre-ba2b9347 | 1 | RELEASE; current tree computes NO percentage anywhere (engine grades 4-state only, REFERENCE-SMART §4 parity) |
| R3 | "sometimes shows for the SD card" | kind 'sd' never fired pre-ba2b9347 (BF-31); SD page rendered a hard-coded fake card; reference: SD = no transport, stays in list, grades Unknown (gray) — never fake-green | 1 | RELEASE + Phase 1 truthful Unknown state in the new surface |
| R4 | "works nothing like the reference" (SMART) | D-P4 structural divergence (one-disk in-app page vs separate all-disks surface) + missing transports: SAT via \\.\SCSI%d: ABSENT, WMI MSStorageDriver_FailurePredict{Data,Thresholds} fallback ABSENT (comment-only cr_health.h:21), USB VID/PID quirk ladder ABSENT, SMART_GET_VERSION never called | 3 | Phase 1: separate disk-health surface + SAT + WMI fallback + truthful not-supported |
| R5 | "jumps directly to 30–75% advanced scanning" | exact math of the global 40/35/25 curve: quick (40%) + deep (35%) complete in seconds, so the first sample lands at 75.0% (carve boundary); "advanced" phase label spans deep+carve (contract.ts ScanPhase) | 3 (D-P6) | Phase 2: per-phase 0-100% restart, quick phase rendered with its own gauge |
| R6 | "never shows a quick-scan phase" | quick is one metadata pass finishing in seconds; no distinct quick-phase presentation exists; reference shows "Quick Scanning %1, Files Found:" with its own 0-100% gauge (strings @0001d7d6) | 3 | Phase 2 |
| R7 | "stalls in sector reading" / "very slow" | D4f 605 s freeze (registry mutex 2×/512B stride × 134M strides; fixed 0e87689d UNRELEASED) + pre-693d4d60 no deep byte axis + swallowed heartbeat; residual wall: BF-30 carve matcher 8.45 MiB/s | 1 (+perf) | RELEASE + Phase 2 bounded perf work on BF-30 |
| R8 | "produces no real results" | scan never passed the frozen carve window on the user's build; results surface only after stages emit; read-truth gate (post-fix) refuses fully-blind scans | 1 | RELEASE; Phase 2 validation on seeded fixture |
| R9 | "filters do not work" / "images, documents, archives are not found" | HYPOTHESIS (high confidence, needs runtime evidence): filters are bridge-side over the NodeMeta cache (results.rs, D-B3); carve populates those categories; the stalled scan left the cache empty — the filter is not dead, it has no data | 1 | RELEASE + Phase 2 per-filter validation against the seeded manifest |
| R10 | "deleted directory structure is not reconstructed" | engine DOES reconstruct (MFT parent-chain emit_parents, cr_mftwalk.cpp:1033-1098) — the user's scan never completed; reference model = 3 roots (existing / File Path Lost / Reconstructed) + viability gate 0x3ff strict + orphans grouped | 1 | RELEASE + Phase 2 deleted-tree validation vs expected tree |
| R11 | "product ships far fewer files than og" | 8 files / 8.2 MiB vs reference live tree 1,276 files / 494.7 MiB — registered consolidation (everything claimed ships inside cr_ui.dll) + REAL functional gaps: preview stack, repair workbench (G-1(b)), partition-apply UI, diskhealth companion content; rest is out-of-scope (WinPE 175.6 MiB, cloud/telemetry, 37 language packs, 200 UCRT shims) | registered | Phase 3: close functional gaps, document consolidations in FOOTPRINT-DELTA.md |

## 2. Verification corrections folded into the plan (agent E)

- Weights 40/35/25 live in `engine/include/cr/orchestrator/cr_scan.h:72-76` (+ bridge comment scan.rs:6) — PRODUCT-STATE's contract.ts:177-184 location was PARTIAL; that area holds ScanStage/ScanPhase types. (V2)
- Status-bar byte-exact templates: `Quick Scanning%1, Files Found:` / `Advanced Scanning%1, Files Found:` (no space before comma), `Reading Sector:` (capital S), `Estimated time remaining: %1s` (not "Remaining time"). Phase 2 must render these byte-exact. (V5)
- The reference's WMI fallback is `MSStorageDriver_FailurePredict{Data,Thresholds}` from root\WMI (VERIFIED in DiskHealth.exe UTF-16 strings) — NOT MSFT_PhysicalDisk. Phase 1 implements the reference's route. (V10-resolution)
- USB quirk ladder: 23 distinct `AddDisk - USB*` strings (not 28); the vendor NVMe route NAMES are verified but their IOCTL payloads are UNVERIFIED — we implement the generic STORAGE_QUERY NVMe route (as today) + SAT, and do NOT fabricate vendor routes. (V9a/V9b)
- RAWSearchDLL does NO device I/O (no CreateFile/DeviceIoControl imports) — carving rides the caller's buffers; our architecture (engine reads, carve analyses windows) matches. (V5)
- 840×590 verified via extracted main.qml (`width: blur === 0?840:856`). (V3/V4)

## 3. The plan (this session's phases)

**Phase 0 — DONE**: five sheets pushed (this commit train). No code change
before them (mandate honored).

**Phase 0.5 — make main green again (precondition for every gate):**
fix `frozen_run_s = 0;` → `0.0` (E0308), fix the ui-ci clippy findings (fetch
the run log — no cargo in this sandbox, CI is the only Rust gate), push, all
lanes green. Evidence: workflow run links.

**Phase 1 — SMART rebuild (T5/T6):**
1. Engine: add SAT transport (ATA pass-through via IOCTL_SCSI_PASS_THROUGH on
   \\.\SCSI%d:, the reference's SAT route) for USB-bridged disks; add the WMI
   fallback (MSStorageDriver_FailurePredict{Data,Thresholds}, root\WMI) as the
   last-resort attribute source; SMART_GET_VERSION support check on the legacy
   path; truthful per-disk "not supported" → grade Unknown with the reason.
   NO vendor NVMe routes (payloads unverified — recorded as DEFERRED in
   PARITY-DELTAS with precise scope). NO fabricated values anywhere.
2. Surface: the D-P4 REVERSAL — a separate disk-health window (Tauri
   WebviewWindow) matching the reference model: all-disks selector, per-disk
   4-state grade (Unknown/Good/Caution/Bad, no percentage), SMART table
   ID/Attribute/Current/Worst/Threshold/Raw/Status, temperature card, refresh
   (manual + interval), no-SMART disks stay listed with Unknown. Tray-residency
   and schtasks autostart: evaluated, likely DELIBERATE-DIVERGENCE (recorded
   with rationale) — the guardian/autostart behaviors are companion-app
   packaging, not health reading.
3. worklog/SMART-MATRIX.md: platform capability × product behavior per OS and
   transport (Win7/8.1/10/11 rows; NVMe protocol-query is Win10+; Win7 NVMe =
   truthful Unknown unless the vendor driver exposes it).

**Phase 2 — scan pipeline (T7–T12):** per-phase 0-100% progress (D-P6
reversal) with byte-exact status grammar; quick phase visibly rendered with
its own gauge; automatic quick→advanced transition (already ADR-008, keep);
filters validated per-dimension against the seeded manifest during AND after
scan; deleted-tree validation vs the expected tree on the seeded VHD fixture
(bridge-integration lane already attaches 2 VHDs and seeds — extend its
assertions); bounded BF-30 perf work (profile first, land safe wins, document
the wall honestly if structural).

**Phase 3 — preview + footprint (T13/T14):** close the preview gaps per
REFERENCE-FOOTPRINT §3.2 (bundled decode set, bounded workers, no console);
FOOTPRINT-DELTA.md reconciling every module; PARITY-DELTAS rows updated
(CLOSED / DELIBERATE-DIVERGENCE / DEFERRED with precise scope).

**Phase 4 — validation + release (T15–T20):** full CI green for the right
reason; forensic-repro run on the release candidate; release tag (0.9.3 per
the prior session's train — 0.9.2 was announced but never tagged; bump to
keep the train honest); installer exercised on a clean CI Windows machine;
worklog.md handoff with every SHA, run link, artifact, deferred item.

## 4. What is NOT broken (re-verified, do not re-break)

- Engine enumeration core (D3 keep-present, exact-size volume retry).
- The 4-state grading model already mirrors the og decoder (V8 verified
  byte-level: NVMe life=100−pct_used, >10 Good / =10 Caution / <10 Bad;
  critical_warning→Bad; attrs 194/184 never graded).
- Deleted-tree reconstruction exists engine-side (MFT parent chains).
- Privilege posture end-to-end (requireAdministrator manifest + D1 byte-scan).
- Contracts lane, ui-e2e lane green at HEAD.

---

# PRIOR SESSION'S DIAGNOSIS (2026-11-06, preserved verbatim)

# DIAGNOSIS — the consolidated Phase 0 picture (this session, T3)

> Six refreshed context sheets feed this document (REPO-STATE, UI-MAP,
> CORE-API, WIRING-AUDIT, REFERENCE-BEHAVIOR, INSTALLER-AND-PRIVILEGE).
> Every claim below is evidenced in those sheets with file:line. This is
> the entry-point diagnosis for Phase 1 reproduction.

## 0. The headline finding — TWO defect strata, not one

The user's machine and the current main tree carry DIFFERENT bugs that
produce the SAME symptoms:

**Stratum 1 — the user's installed build (v0.9.1, released Nov 6).** Its
symptom triad is fully explained by three defects that are FIXED on main
but were NEVER RELEASED (no v0.9.2 tag exists; the newest tag is v0.9.1
at cdf66ff8; RELEASE-RUN.md ends at v0.9.1):

- D-ENUM-02: the bus-type text table was shifted by one from bus 11 up —
  every SATA disk decoded `removable` via the mislabeled bus and landed in
  the wrong home-page group ("not all of my disks are visible" — they are
  present but misclassified/misgrouped, and the SD page can never match).
- D-NVMe-01: the NVMe health query's `pack(1)` buffer placed protocol
  data at offset 9 instead of 8 (`offsetof(STORAGE_PROPERTY_QUERY,
  AdditionalParameters)`) — every real NVMe controller refused the query,
  so health data existed only for the first (SATA) disk. "as I switch to
  other it shows nothing" — the exact D-NVMe-01 signature.
- D4f: the carve's user-sign probe took a registry mutex twice per
  512-byte stride over ~134M zero strides — a 605-second all-axes freeze
  ("stuck in sector scanning"). Fixed lock-free at 0e87689d, validated
  engine-side, never shipped.

**Stratum 2 — the current tree (post-eb4bb5f3, unreleased 0.9.2 state).**
The engine-side fixes above ARE in-tree, but commit eb4bb5f3 appended
fields to two frozen-grammar C structs (`cr_ui_disk_info.bus_type`,
`cr_ui_smart_report.power_cycles_nvme`, `cr_ui_smart_attr.threshold`) and
THE RUST BRIDGE WAS NEVER RE-PINNED. The bridge's abi.rs still asserts
"v4 carries NO bus_type" (abi.rs:184-186) — a comment that documents the
world BEFORE the append. This creates memory-corruption-class defects
that are WORSE than the user's current symptoms:

- `CrUiDiskInfo` is 296 B in Rust vs 304 B in the engine → the
  two-call enumeration (util.rs:156-171) allocates a `Vec` with the Rust
  stride while the engine writes with the C stride: an 8×N-byte heap
  overflow AND every row ≥1 decodes shifted — all rows collapse toward
  garbage `disk-0` duplicates (React key collapse drops them).
- `CrUiSmartReport` is 224 B in Rust vs 240 B in the engine → the
  engine's `memset(rep, 0, sizeof(*rep))` (cr_ui.cpp:4892) overwrites 16
  bytes of the caller's stack on EVERY health read (health.rs:32 local).
- `bus_type` is never consumed → `kind` can never be `'sd'` → the SD
  card page's `kind==='sd'` filter can never match (devices.rs:215).
- `cr_ui_health_read_ex2` (93rd export, the threshold axis + NVMe power
  cycles) exists but is unbound → the SMART threshold column is nulled.
- The startup ABI gate (lib.rs:98-107) only LOGS a version mismatch —
  a stale kit serves scrambled data without refusal.

**The systemic root cause behind Stratum 2:** no CI gate links the
bridge against the real engine. ui-ci runs `cargo check` (no link step);
the engine lane never touches Rust. A C header field append is
structurally invisible to every gate on the repo. This is the exact
class of seam the session prompt's "the thousand small seams" names.

## 1. The user's five reported defects — diagnosis table

| ID | Symptom (user words) | Stratum-1 cause (their build) | Stratum-2 cause (current tree) | Verdict |
|---|---|---|---|---|
| R1 | "on opening its not requesting or opening as administrator" | pre-0.9.0 build class (v0.8.x asInvoker) | manifest IS embedded (app.manifest:25 → build.rs:45-48 → D1 gate) | User's build predates 0.9.0 OR stale-bundle installer hazard (build.ps1:211-213 picks bundle by LastWriteTime) |
| R2 | "popup shells are appearing, probably cmd or powershell" | v0.8.x `_popen(fsutil)` per health read | 0 spawn sites lack CREATE_NO_WINDOW (all 3 audited: cr_health.cpp:393, cr_fsck.cpp:50, cr_bitlocker.cpp:64) | Same stale-build class; re-verify on clean install |
| R3 | "only one disk is appearing" (2 internal + 1 SD) | D-ENUM-02 SATA-mislabeled-removable + SD-never-'sd' | ABI stride drift → disk-0 row collapse (heap overflow) | BOTH strata must be fixed; release cut must carry both |
| R4 | "jumps directly to 75% advanced scanning, files found 21 (0 B), no sector progress, all stuck" | D4f 605s freeze + D4b-unshipped live bytes + deep phase has NO byte axis by design | bytes_scanned is carve-true only (cr_scan.cpp:510-518); bridge swallows the 5s heartbeat (events.rs:337-340) and only refreshes counters on file events | 75% is exact math (weights 40/35/25; quick+deep mid = 75%); "Advanced Scanning" spans deep+carve (contract.ts:171-176) |
| R5 | "restart as administrator... weird popup colour and background" | v0.8.x amber ElevationBanner (removed 0.9.0) | no elevation UI exists; dead `errors.elevationRequired` string (en.ts:543) + stale installer text (nsi:331) | Stale-build class; dead vocabulary to clean |

## 2. Beyond the reported five — the session's extended defect register

Refreshed from the sheets, ranked by user-visible severity:

- **BF-ABI-01 (NEW, this session): the bridge struct drift** — Stratum 2
  above. THE blocker: nothing may ship until abi.rs is re-pinned and a
  layout gate exists.
- **BF-ABI-02 (NEW): no real-bridge CI lane** — the missing gate that let
  BF-ABI-01 land. Fix: an integration job that builds the engine, links
  the bridge, and runs bridge-level tests against the real DLL
  (multi-disk where possible).
- **S-SD-01: SD cards are not a first-class surface** — kind 'sd' never
  emitted; the SD page renders a hard-coded fake card (en.ts:131-134,
  "58 GB/Disk2"). Reference: SD is a first-class sidebar page (atlas:143).
- **H-EX2-01: threshold column dead** — ex2 unbound; the reference's
  table renders ID/Attribute/Current/Worst/Threshold/Raw/Status
  (UIUX/19:26-28).
- **P-DEEP-01: deep phase has no byte axis** — "Reading sector: 0/N"
  frozen through quick+deep (the D4e coverage counter exists engine-side
  for the remnant sweep but does not stamp bytes_scanned for deep).
- **P-TICK-01 (BF-29): the "Searching:" ticker is dead** — currentPath
  always null (events.rs:193); reference shows a live path ticker
  (scan-phase-captured.md:56-107).
- **P-HB-01: the 5s heartbeat is swallowed** — events.rs:337-340
  (emit=false); the UI refreshes counters only on file events → any
  cadence gap reads as a hang.
- **F-REPAIR-01: the repair page's dropzone/Repair-All is local-state
  theater** — fake paths, no engine call (repair-page.tsx:121-124).
- **F-HLT-02: the health page synthesizes values** — wear '2%', '· 512B',
  hard-coded 512 B divisor, gauge computed UI-side from partial fields
  (health-page.tsx:100-106).
- **F-ACT-01: activation paste inserts a fake license key** — theater.
- **BF-30: the carve throughput wall** — 8.45 MiB/s (76-124 s/GiB):
  multi-TB scans take hours-to-days even with every D4 fix. Performance
  defect, engine-side.
- **BF-32: scan-emitter teardown race** — latent UAF in events.rs/state.rs
  (recorded with a two-phase fix design; deliberately deferred by the
  prior session).
- **I-STALE-01: the installer lane's bundle pick is LastWriteTime-based**
  (build.ps1:211-213) — can wrap a stale app exe.
- **I-CLEAN-01: app.log survives uninstall** (nsi:537-541).
- **CI-STALE-01: forensic-repro.yml default tag is v0.8.4**;
  engine-ci.yml:60 carries a false workflow_call claim.

## 3. What is NOT broken (re-verified this session)

- The engine's enumeration core is now genuinely robust (D3 keep-present,
  N-071 share-blocked, E-01 exact-size volume retry — CORE-API.md §D).
- All 43 manifest commands are implemented; all 10 events emit↔listen with
  payload parity (W-01 held).
- The privilege posture as built is correct end to end (manifest → D1
  byte-scan gate → forensic gate → OS-refusal twin).
- The shipped connector is REAL (connector/index.ts:23-26 defaults to
  tauri; mock is dev/test-only).

## 4. The plan (Phase 1 → 5)

1. **Reproduce Stratum 2 on a real runner** (fail-before evidence for the
   ABI drift: bridge integration harness against the real DLL, multi-disk
   via VHD attach).
2. **Fix train A (correctness blockers):** re-pin abi.rs (bus_type,
   power_cycles_nvme, threshold), bind ex2, map bus_type→kind (sd!),
   refuse-on-ABI-mismatch, rebuild.
3. **Fix train B (scan truth):** deep-phase bytes_scanned, heartbeat
   emission, current-path ticker, live counter refresh on poll.
4. **Fix train C (surface truth):** SD page real, health page real (no
   synthesized wear/sector-size), thresholds, honest no-SMART state,
   repair/activation theater removed or made real, dead strings cleaned.
5. **The new gate:** engine+bridge integration CI lane (link + run
   against the real DLL; ABI layout manifest cross-check; multi-disk
   enumeration pin).
6. **BF-30 investigation** (bounded: profile the carve, land safe wins,
   document the wall honestly if structural).
7. **Release 0.9.3** (0.9.2 was announced but never tagged; the version
   files already carry 0.9.2 — bump to 0.9.3 to keep the train honest),
   full forensic validation, clean-install exercise, uninstall check.

The register is deliberately larger than the user's five: "everything
seems broken" is the standard, and the fake/theater surfaces are exactly
the "script kiddie slop" the user refuses.
