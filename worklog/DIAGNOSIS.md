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
