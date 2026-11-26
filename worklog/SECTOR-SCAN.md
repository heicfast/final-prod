# SECTOR-SCAN — Phase 2 deliverable: the progress model rebuilt (F-B + F-C)

Session: sector-scan-rebuild (2026-11-26, the implementation continuation). Tree at
implementation: `126ea398` (main, clean). Fix plan: `worklog/session-sector-scan-rebuild/DIAGNOSIS.md`
(F-B stage 8, F-C stage 9, produced by this session's Phase 0/1 at `d69893ed`/`c54f2496`).

Every claim below is a corpus citation, a file:line in the current tree, or a captured
run/test id. Nothing is invented.

## 1. The model (what replaced the weights)

**Retired:** `StageWeights {quick=40, deep=35, carve=25}` + `weighted_progress` +
`phase_progress` (the product's own lineage, BF-22 — the reference corpus documents NO
per-phase percent weights for the scan bar; REFERENCE-SCAN §4.1, parity row 6: the
sector-linear "Reading sector: X/Y" model with remaining-time-from-throughput is the
only evidenced form, and the og's own captured ticker shows X beyond Y —
"Reading sector: 134316032/31413598", X 4.27x Y).

**The rebuilt form** (`engine/include/cr/orchestrator/cr_scan.h` + `cr_scan.cpp`):

1. **The byte plan (the percent's denominator).** `scan_byte_plan(run_carve, run_deep,
   whole_disk, fs_hint, slice_bytes)` = one target-span per configured BYTE sweep —
   the carve window sweep (cr_carve.cpp publishes `wi * WINDOW` over `ctx.total` =
   the slice), the NTFS remnant sweep (`scan_mft_remnants`, `progress_total` = the
   slice), and whole-disk lost discovery (the WindowScanner's whole-disk walk,
   `progress(base, total)` over `effective->size()`) — computed once at run start,
   post-probe, when the fs_hint the remnant gate will see is settled. Record-only
   passes (quick's walk, the engine dispatch, candidate scans) own NO sweep share:
   the honest shape that kills the 75.0% opening plateau (quick+deep are metadata
   passes finishing in seconds; the old weights handed them 75% of the bar).
2. **One cumulative sweep axis (X).** `tel_sweep_begin_()` brackets every byte
   sub-pass explicitly (carve / remnant / discovery — never inferred from
   reset-shaped publishes, so a parallel worker's out-of-order sample can never be
   mistaken for a new sweep); `tel_publish_` accumulates the active sweep's own
   monotonic cursor (max-guarded) onto the flushed base.
   `ScanTelemetrySnapshot::bytes_scanned` = flushed + active = X — ONE axis replacing
   the old split (carve's clamped `base+done` form vs deep's direct per-sub-pass
   assignment, P-DEEP-01/D4a). X CAN EXCEED the device size on multi-sweep scans —
   the og's own captured ticker shape (134316032/31413598); the percent divides by
   the PLAN, never by the device size.
3. **The percent.** `overall_from_plan(X, plan, fallback)` (pure, tested): X over the
   plan, clamped at 1000, monotonic-clamped into the snapshot; 1000 stamps only at
   true completion (the final stage end / post-stamp latch). Degenerate no-sweep
   configs (quick-only, record-only deep) fall back to the ACTIVE stage's own
   permille — the honest single-stage bar.
4. **The advanced-phase gauge (D-P6) on the byte plan.** `advanced_from_plan(X, plan,
   advanced_base_x, fallback)`: the phase restarts at 0 at the first non-quick stage
   begin (the latch) and rides X since that base over the same plan.
5. **Time-floor coalescing.** `emit_progress_event_` fires when the overall permille
   OR the active stage's own permille advances (record phases keep the event channel
   alive — quick's byte-plan overall is honestly static), throttled to one sample per
   **200 ms** (`kProgressMillisStep_`, ≤ ~5/s + stage flips) — replacing the 16 MiB
   byte-step trigger. The og's own shape is a polling grid ("38 named timeout()
   consumers", REFERENCE-SCAN §3.2); a per-window firehose is not.

**ABI:** `cr_ui_scan_config`'s `quick_weight/deep_weight/carve_weight` fields stay
(append-only W1c) and are now IGNORED — `cr_ui.cpp`'s mapping is deleted, the header
comment says so. No ABI struct changed; `abi_layout_dump.header.sha` is
comment-insensitive (verified: sha256 over `strip_comments(header)`), so no regen was
needed.

## 2. Why this is the reference-faithful model (citations)

| Property | Reference citation |
|---|---|
| Sector-linear progress, not phase weights | REFERENCE-SCAN §4.1 (dynamic-ground-truth--scan-phase-captured.md §7 behavior 2); parity row 6: "if 40/35/25 is kept, cite it as product-engine lineage" — it is now RETIRED instead |
| The ticker X can exceed Y (re-coverage) | dynamic-ground-truth--scan-phase-captured.md §7 + §2 status bar (verbatim "Reading sector: 134316032/31413598") |
| Quick is a small prefix (~2-3% of wall time) | qa-logs--session-forensics.md §2 line 30 (≈21 s quick on a 129 GB volume); the byte model gives metadata passes ~0% of the bar, matching the wall-time truth |
| Percent + counters observable moving throughout | parity row 4 (dynamic-ground-truth §7 behaviors 2 + status format) |
| Gauge restarts at the phase boundary | REFERENCE-SCAN §5 (strings @0001d7d6/@00007825) — preserved (D-P6), now byte-derived |
| Progress percentage is POLL-driven in the og | REFERENCE-SCAN §3.2 ("38 named timeout() consumers"; the only progress-push is sigPercentChange) — the 200 ms event floor keeps the push channel bounded while the UI's 500 ms polls stay the display rate |

## 3. Before / after (quantified)

**BEFORE (captured, pre-rebuild):**
- The UI opens at exactly **75.0%** then crawls **75.0 → 76.1% over 891 s**
  (forensic run 42255492260, v0.8.4; DIAGNOSIS stage 8 — quick(40)+deep(35) handed
  75% to seconds of metadata work while the carve sweep owned 25).
- The it-gate fixture pinned `overall_pm=750` at the quick+deep boundary
  (worklog/VALIDATION/it-gates/scan-timeline.csv:1; the pause-resume test asserted
  the 750 cap as a PINNED behavior).
- The ticker axis was split (carve clamped base+done, deep direct per-sub-pass —
  DIAGNOSIS stage 9: "can exceed Y, sit frozen, or sit empty").

**AFTER (this change, proven locally on Linux):**
- `OverallFromPlanIsByteDerived` (test_orchestrator.cpp): quick+deep DONE
  contributes 0 permille — X stays 0 until the carve sweep actually reads; the old
  form returned exactly 750 for that state.
- `PhaseModelCompletesAndQuickDeepOwnNoPercent` (test_pause_resume.cpp): the
  quick|deep degenerate run completes at 1000 (the record passes' own permille),
  never a 75% weighted cap.
- `CumulativeSweepAxisExceedsTheDeviceOnMultiSweepScans` (test_orchestrator.cpp):
  a whole-disk quick+deep+carve scan ends with `bytes_scanned == 2 x disk` (carve +
  discovery sweeps) — X past the device size, the og's captured ticker shape, with
  the percent still ending at exactly 1000.
- `ScanBytePlanCountsEveryConfiguredSweep` / `AdvancedFromPlanRestartsAtThePhaseBoundary`:
  the denominator counts only configured sweeps (NTFS partition scan = carve+remnant
  = 2 spans; FAT = carve alone; whole-disk = carve+discovery); the advanced gauge
  restarts at 0 and completes at 1000.
- `TelemetryCountersMoveDuringCarveStage` now rides a SlowDevice (30 ms/read) +
  8 MiB of carve slack + serial carve so the 200 ms coalescing floor is exercised
  deterministically: mid-carve progress events DO fire with live counts.
- Full local suite: **1821/1822 green** (`ctest`, linux-release build,
  -Wall -Wextra -Werror -Wshadow); the one red is `FatChain.CycleGuardPerf`
  (subprocess killed = the documented sandbox OOM exclusion — 3.4 GB fixture vs the
  4 GB sandbox; the Windows lane owns it; pre-existing, unchanged by this work).
- UI: tsc/eslint/prettier green; vitest **225/225**; contracts validate.mjs green
  (57 commands); doc_truth_gate green (exports=99, seam=12/71); the mock connector
  re-simulates the byte-plan model (its old per-stage weight table retired with the
  engine's).

## 4. Honest scope (what is NOT yet proven here)

- **Windows runner captures C2/C7/C8** (the REPRODUCTION capture plan: stall timeline
  after, per-second progress series, streaming cadence) ride the Windows CI lanes —
  dispatched with this push; run ids recorded in worklog.md as they land. The
  forensic-repro lane on a current release tag remains the end-to-end vehicle.
- **The saturation case:** a candidate-heavy whole-disk scan (lost partitions) runs
  candidate sweeps that are NOT in the fixed plan (their spans are unknown at run
  start); X can pass the plan there and the percent clamps at 1000 mid-candidates
  while the ticker (X), the phase gauge, currentPath and the found-counters keep
  moving — the honest bound chosen over a growing denominator (which would dip and
  freeze the bar at the clamp; D-SCAN-01's 180 s frozen-stretch gate would fire).
  Registered in BEHAVIOR-FINDINGS at close.
- **The degenerate fallback** (no-sweep configs) rides the ACTIVE stage's permille:
  a quick-only scan's global bar = quick's own record progress — honest for a
  metadata-only pass.
- `FatChain.CycleGuardPerf` cannot run in this sandbox (pre-existing, see above).

## 5. Files (the coherent unit)

Engine: `cr_scan.h` (model + plan + retirement), `cr_scan.cpp` (tel_sweep_begin_ /
tel_recompute_locked_ / the plan + bracket call sites in run_carve_stage + run_deep /
200 ms coalescing), `cr_ui.cpp` (weights mapping deleted), `cr_ui.h` +
`cr_query.h` (comment truths). Tests: test_orchestrator.cpp (3 new pure-model tests +
the X>Y multi-sweep pin + the SlowDevice carve-event pin), test_pause_resume.cpp,
test_telemetry.cpp (re-pinned), test_config_display.cpp (re-pinned). Docs trued:
CORE-API-CURRENT, PARAMETERS, PARITY-DELTAS (D-127), architecture,
parity/PARITY-ORCH (rows 10-12 — the divergence registered against the og's
no-weights truth), parity/PARITY-UI (F-C-13), FORENSIC-PROBE, INTEGRATION-PLAN,
PRODUCT-WIRING. UI: the mock connector's byte-plan simulation. No ABI layout change.
