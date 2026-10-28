# MAJOR M-126 — the og's ETA model (rate-EMA remaining time + time-budget variant + "3m00s" placeholder + 1 Hz local countdown) is decoded; the core has no ETA surface anywhere

**Severity:** MAJOR
**Module:** orchestration (ETA) / query surface
**Parameters affected:** ETA inputs (39), progress semantics (38) adjacent
**Corpus references:** `DRWUI_decompiled.c:314835-314887` (`FUN_1401af410` — the ETA/rate computation on every progress message), `:1527650-1527768` (the label + placeholder + the 1 Hz countdown slot `FUN_1409cb530`), `:1783107` (meta-slot 7 wiring); runtime proof "Remaining time: 07m43s" at 34% (`dynamic-ground-truth--scan-phase-captured.md:95,102`)
**Core references:** grep over src/ + include/ for remaining_time/estimate/eta: zero scan-ETA surfaces; `src/app/rrecov.cpp:129-132` (CLI prints elapsed seconds only); telemetry inputs exist (`cr_scan.h:490-512` — overall_permille, elapsed_ms, per-stage bytes); `modules/orchestration.md:23-24` (the O39 disposition whose premise this corrects); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og computes a remaining-time estimate on every progress message: whole-run
`eta = (total−done)·elapsed/done`, an EMA-smoothed rate (0.2 instant / 0.8 history), a
type-gated time-budget variant (×0.6), a literal "3m00s" placeholder while the label is
empty, and a 1 Hz local countdown that ticks the label down between computed refreshes.
The core computes no ETA at any layer — engine, CLI, or cr_ui — and O39's disposition
("the og's Remaining time formatting is UI-side. No engine gap") rests on a wrong
premise: the decode shows a computed model, not formatting.

## What the reference does

`FUN_1401af410` (DRWUI:314835-314887), decoded by lane B:

```
elapsed_ms = GetTickCount() - startTick        // whole-run
eta_s      = ((total - done) * elapsed_ms / done) / 1000
rate       = (total - done) / eta_s
smoothed   = (2*rate + 8*prev_smoothed) / 10   // EMA 0.2/0.8
if type == 0: eta_field = eta_s
else: projected = total * (elapsed_ms/1000) / done
      eta_field  = (budget(+0x120) - budget*(elapsed_ms/1000)/projected) * 0.6
```

Display: "Remaining time: %1" (DRWUI:1527656, :1527752); empty-label placeholder the
literal "3m00s" (:1527664-7669); the 1 Hz slot (:1527550-1527768) parses "Xm Ys",
subtracts a second, re-renders — the label counts down locally between computed
refreshes. Runtime: "Remaining time: 07m43s" displayed at 34% (dynamic-ground-truth :95).

## What our core does

No ETA computation anywhere (verified by grep over src/ + include/: zero hits for an
ETA/remaining-time surface). The CLI prints elapsed seconds only (rrecov.cpp:129-132).
The telemetry exposes overall_permille + elapsed_ms + per-stage bytes (cr_scan.h:490-512)
— sufficient INPUTS for a host-side linear ETA, but nobody implements the og's model
(EMA, time-budget variant, ×0.6), and the decode exists nowhere in the tree.
`modules/orchestration.md:23-24` (O39) dispositioned the surface as UI-side formatting
with "no engine gap" — the premise is wrong (it is a computed model in the host layer).

## Gap

The og exposes a user-visible remaining-time estimate driven by a decoded model; the core
exposes no equivalent surface (nor a helper a host could call to reproduce it).

## Impact

A whole user-visible surface is absent. Hosts cannot reproduce the og's ETA UX from core
events without reimplementing an undocumented model (the EMA dampening and the time-budget
variant are invisible from the inputs alone). Comparative UX benches (time-to-estimate
stability) have no core counterpart.

## Reproduction / verification

Corpus-derived (lane B first-hand decode of FUN_1401af410 + the label/countdown sites,
cites above; runtime anchor dynamic-ground-truth :95,102). Core-derived (grep + the
telemetry contract read directly). PARITY-DELTAS: no ETA row exists; O39 is the
disposition being corrected.

## Fix plan (for the engineering team)

1. Land an ETA helper beside the progress surface: `eta_seconds_remaining(elapsed_ms,
   done, total, prev_rate)` implementing the og's model verbatim (whole-run base ETA,
   EMA 0.2/0.8, the type-gated time-budget variant ×0.6) as a pure function in the query
   layer, so hosts render og-faithful estimates.
2. Files: `src/core/query/` (new helper) + `include/cr/orchestrator/cr_scan.h` (telemetry
   carries smoothed_rate so hosts need no state); rrecov optionally prints
   "remaining: XmYs" using it; orchestration.md O39 rewritten with the decode.
3. Tests: the worked example below + EMA dampening pin + placeholder semantics.
4. Acceptance criteria: the helper's output matches the runtime anchor class (a 40/100
   progress at 400 s → 600 s remaining, EMA-damped); O39's replacement cites the
   decompile.

## Tests to add

- `Eta.WholeRunModel` (100 GB volume, 40 GB in 400 s → 600 s)
- `Eta.EmaDamping` (a 10× rate burst moves smoothed ≈22%, per the 0.2/0.8 weights)
- `Eta.TimeBudgetVariant` (the ×0.6 leg, type-gated)
- `Eta.PlaceholderAndCountdownFormat` ("3m00s"; "Xm Ys" parse/decrement)

## Related findings

- M-125 (the progress remap from the same message family)
- N-166 (live counters cadence)

## Open questions

None — the model is fully decoded from FUN_1401af410 + display sites.
