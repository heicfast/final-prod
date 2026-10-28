# MAJOR M-125 — progress display mathematics: the og's piecewise UX remap + 99.99% stall cap decoded — the core's linear weighted permille diverges with no registration, and O38's "M-equivalent" premise is superseded

**Severity:** MAJOR
**Module:** orchestration (progress semantics) / query surface
**Parameters affected:** progress semantics (38), ETA inputs (39, adjacent), API contract truthfulness
**Corpus references:** `DRWUI_decompiled.c:313925-313974` (`FUN_1401ad580` — the progress message entry), `:314835-314887` (`FUN_1401af410`), `:314931-314959` (`FUN_1401af820` — the remap), `:314964-314986` (`FUN_1401af950`), `:315014-315024` (`FUN_1401afa60` — monotonic publish); ground-truth anchors `dynamic-ground-truth--scan-phase-captured.md:39,90` (displayed 9% at 75 s, 34% at ≈460 s)
**Core references:** `src/core/orchestrator/cr_scan.cpp:129-144` (`weighted_progress`), `:319-376` (stage permille + telemetry publish); `modules/orchestration.md:19-22` (the superseded O38 verdict); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's displayed scan percent is not the raw scan completion: DRWUI computes
`raw = done*100/total` from sector-based engine messages, then (for type-1 messages)
applies a piecewise remap — raw<11 → ×2; 11..30 → (raw−11)·0.2+20; 30..90 → ×0.8;
90..98 → (raw−88)·2.7+72; >98 → raw — and caps the display at 99.99% until the
completion message forces 100. The core's `weighted_progress` is a linear weighted
permille over stage weights (40/35/25) with no remap and no stall cap. At the same scan
state the two surfaces disagree by design: raw 50% → og shows 40%, core reports 57.5%;
raw 5% → og shows 10%, core reports 2.0%. Neither the curve nor the cap is registered
anywhere, and orchestration.md's O38 "M-equivalent" verdict predates this decode.

## What the reference does

`FUN_1401af820` (DRWUI:314931-314959) — the remap, decoded by lane B this session from
the decompile:

```
REMAP(raw):
  raw < 11        -> raw * 2.0
  11 <= raw < 30  -> (raw - 11) * 0.2 + 20.0
  30 <= raw <= 90 -> raw * 0.8
  90 < raw <= 98  -> (raw - 88) * 2.7 + 72.0
  raw > 98        -> raw
  if percent > 100 and raw != 100: percent = 99.99
```

`FUN_1401af950` (:314964-314986): `raw = (done * 100) / total`; type 0 → 0, type 1 →
REMAP(raw), else → raw. `FUN_1401afa60` (:315014-315024): publish only if percent >
current (monotonic). The completion message (type 3) forces (100,100) (:313954-57).
Per-phase restart: the percent resets at the quick→advanced label flip (bottomarea
m_iScanType 1→2). Runtime anchors: displayed 9% at t=75 s and 34% at t≈460 s
(dynamic-ground-truth :39,90) — consistent with the curve's early doubling.

## What our core does

`cr_scan.cpp:129-144`:

```c
u32 weighted_progress(const StageWeights& w, u32 quick_pm, u32 deep_pm, u32 carve_pm) {
    ...
    const u64 num = u64(w.quick) * quick_pm + u64(w.deep) * deep_pm + u64(w.carve) * carve_pm;
    const u32 permille = u32(num / total);
    return permille > 1000 ? 1000 : permille;
}
```

A single monotonic overall permille, linear in stage completion, weights 40/35/25; per-stage
permille also exposed (cr_scan.h telemetry). No display remap, no 99.99 cap (the cap is
the true 1000‰ at completion), no per-phase display restart (the og's UX convention).
`modules/orchestration.md:19-22` (O38) claims the core's model is "M-equivalent" —
written before this decode, citing only the session-save 40→65% formula as the UI-side
construct.

## Gap

The og's observable progress curve (a decoded, deterministic function of scan state) has
no counterpart or registration in the core. Falsifiable: at raw deep-scan completion 50%
with quick done and carve unstarted, og displays 40%, core telemetry reports 575‰.

## Impact

Any host building og-faithful UX on the core's progress events renders a materially
different curve (5× slower early, ~30% lower mid-scan, no stall-cap convention at 99%).
The divergence also affects comparative benches (progress-vs-time plots are not
comparable across implementations without the remap). The stale O38 verdict misleads
parity sessions into treating the surfaces as equivalent.

## Reproduction / verification

Corpus-derived (lane B first-hand decode of the four DRWUI functions, cites above; the
piecewise constants read from the decompile bodies), cross-checked against the runtime
anchors (dynamic-ground-truth :39,90). Core-derived (lead auditor read cr_scan.cpp:129-144
directly — quote above). PARITY-DELTAS check: D-78 registers delivery (push+pull vs poll
farm), D-36/Q8/O38 register the stage model — no row carries the remap curve or the cap.

## Fix plan (for the engineering team)

1. Decide the contract: land the og's display math as an OPTIONAL query-layer helper —
   `progress_display_percent(raw_type1_percent)` implementing REMAP + the 99.99 cap +
   monotonic publish semantics — so hosts can render og-faithful UX without the engine
   lying in its telemetry (the core's truthful permille stays the primary surface).
2. Files: new `src/core/query/` helper (or extend cr_scan.h's query namespace) + tests;
   orchestration.md O38 rewritten with the decode; PARITY-DELTAS row registering the
   linear-permille core model as the deliberate engine-side truth and the remap as the
   og's display layer.
3. Constants verbatim from the decompile (the five segments + 99.99).
4. Tests: the worked examples below as table-driven pins.
5. Acceptance criteria: the helper reproduces the two runtime anchors (9%@75s-class and
   34%@460s-class states map to the recorded displays); O38's replacement cites the
   decompile.

## Tests to add

- `Progress.RemapCurveTable` (raw 0/5/10/11/20/29/30/50/90/98/99/100 → 0/10/20/20/22/23.8/24/40/72/99.4/99.99/100)
- `Progress.StallCapHoldsAt9999UntilCompletion`
- `Progress.MonotonicPublish` (a lower raw never lowers the display)
- `Progress.CorePermilleStaysLinear` (regression pin for the engine surface)

## Related findings

- M-126 (the ETA model decoded from the same function family)
- N-166 (the live counter cadence — the same progress-message family)

## Open questions

None — the curve is fully decoded; the open decision (adopt helper vs register-only) is
a product choice recorded in the fix plan.

## Resolution

**Status:** FIXED
**Fixed in commit:** `<this push>` (the S42 fix lane's M-125 landing)
**Verified by:** ProgressDisplay.RemapCurveTable · ProgressDisplay.StallCapIsTheDefensiveBranch · ProgressDisplay.PercentTypeDispatch · ProgressDisplay.MonotonicPublishGuard · ProgressDisplay.CorePermilleStaysLinear (5/5 GREEN; RED pre-fix proven by the stashed-engine rebuild — `progress_display_remap` not a member of `cr::query` pre-fix); full suite 1197/1197 green post-fix; clang-format 18.1.8 clean.
**Resolution notes:** The fix plan's step-1 contract landed exactly: the og's display math as an OPTIONAL query-layer helper family — `query::progress_display_remap` (FUN_1401af820 verbatim: the five segments with the corpus's constants 2.0/0.2+20.0/0.8/2.7+72.0/identity + the defensive 99.99 stall cap), `query::progress_display_percent` (FUN_1401af950: the type-0 reset, the type-1 remap with the outer 100.0 clamp, the other types raw), `query::progress_display_publish` (FUN_1401afa60: the monotonic guard) — with the engine's truthful linear weighted permille UNTOUCHED as the primary surface (the regression pin `ProgressDisplay.CorePermilleStaysLinear`). **Finding-text corrections, per the corpus-wins rule:** the finding's own test table carried four transcription errors against the decompile's constants — raw 20 → **21.8** (not "22"), raw 29 → **23.6** (not "23.8"), raw 98 → **99.0** (not "99.4"), raw 99 → **99.0** (not "99.99") — and the "caps the display at 99.99% until the completion message" framing overstates the cap: the 2.7 segment tops at 99.0 (raw 98) and the identity tail keeps raw ≤ 100, so the cap fires ONLY for the out-of-range raw > 100 (the og's dead defensive branch, ported as decoded and pinned live at raw 101/250). The decode ALSO surfaced the 98-boundary discontinuity the finding missed: the 2.7 segment ends at 99.0 (raw 98) and the identity tail DROPS to 98.5 (raw 98.5) — the og's curve is non-monotonic at the boundary, which is exactly what FUN_1401afa60's monotonic publish guard exists to hide (pinned by test). Trail: modules/orchestration.md O38 rewritten (MATCH, the decode + the helper surface); PARITY-DELTAS D-118 registers the linear-permille engine truth as the deliberate divergence (the engine never lies in its telemetry; the remap is the og's display layer). No safety hardening was touched by this fix.
