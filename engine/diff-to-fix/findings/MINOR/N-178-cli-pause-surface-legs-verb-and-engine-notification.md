# MINOR N-178 — the pause surface's CLI legs: no pause verb in the CLI (the og ships a pixel-verified Pause control), and no engine-visible pause notification (the og calls engine vtable+0x88)

**Severity:** MINOR
**Module:** cli / orchestration (pause)
**Parameters affected:** pause and resume (33), callback threading adjacent (41), CLI surface shape
**Corpus references:** Pause ❚❚ / Stop ■ bottom-bar controls, pixel-verified (`UIUX/35:175-176`; `dynamic-ground-truth--scan-phase-captured.md:99`); the paused UI state ("Scan Paused" + red strip, gauge frozen — `UIUX/35:182`; DRWUI:1098406-24, :1100349-65); the pause machinery `DRWUI_decompiled.c:314388-314490` — including the ENGINE notification `engine->vtable+0x88(1)` on pause (:314401-404) and `(0)` on resume (:314482-84) beside the `+0xf8` gate
**Core references:** `cr_ui_scan_pause/resume` exist and are tested (`src/ui/cr_ui.cpp:2790-2803`; test_pause_resume.cpp) — the ABI half is LANDED; the CLI has no pause flag (`src/app/rrecov.cpp:3149-3160` usage census; header :142-145 mentions pause as UI-host-only); core pause is a pure `PausableToken` condition-variable wait (`include/cr/storage/cr_device.h:229-234`) — engines observe nothing; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two legs of the pause surface: (1) the CLI exposes no pause — Ctrl-C (cancel) is the
only control, so a CLI user who needs to suspend (laptop sleep, disk contention) loses
the scan per the S-057/A-013 loss paths; (2) the og notifies the ENGINE of pause via a
vtable call; the core's pause is a token wait invisible to engines. No decoded og
engine-side pause behavior exists in the corpus (no pause-specific engine code found),
so leg 2 is a register-or-note, not a build.

## What the reference does

The pixel-verified controls + the machinery cites above; the engine-notification call
pair (:314401-04/:314482-84) is decoded; what the ENGINE does with it is not (no
corpus evidence).

## What our core does

The ABI pause/resume pair is landed and tested (the PausableToken is a verified
RE-parity of the og's event gate + GetTickCount compensation — lane D verified both
sides). The CLI never exposes it; engines observe nothing on pause.

## Gap

CLI leg: absent verb. Engine-notification leg: absent mechanism (with no decoded og
engine-side consumer to mirror).

## Impact

CLI users: suspend-and-resume is impossible without loss (compounded by S-057 — the
cancel path drops results). Engine leg: informational only until an og engine-side
pause consumer is decoded.

## Reproduction / verification

Corpus-derived (the controls/machinery/notification cites, all first-hand by lane D).
Core-derived (the usage census + the token wait read; the ABI tests cited).

## Fix plan

1. CLI leg: add a `--pause-stdin` mode (a keystroke or signal maps onto the existing
   PausableToken pause/resume; Ctrl-C stays cancel) — or document the ABI-only posture
   in the usage block if the team prefers.
2. Engine leg: register as an open modeling difference (PARITY-DELTAS or the module
   doc): "the og notifies engines of pause (vtable+0x88); the core's pause is a token
   wait — no decoded og engine-side consumer exists to mirror."
3. Files: `src/app/rrecov.cpp` (leg 1); docs (leg 2).
4. Tests: `Cli.PauseResumeRoundTrip` (leg 1, if built).
5. Acceptance: a CLI scan can be paused and resumed with compensated elapsed (the
   existing token semantics), losing nothing.

## Tests to add

- `Cli.PauseResumeRoundTrip` (leg 1, if built)

## Related findings

- S-057 / A-013 (the cancel-loss paths that make pause matter for the CLI)
- Lane D's verified MATCHes: pause/resume + elapsed compensation, cancel-keeps-results
  (the ABI half is done)

## Open questions

Leg 2's depth: which thread waits the og's `+0xf8` pause gate engine-side (no engine
corpus code found — likely UI/timer-side; settles whether an engine consumer exists at
all).
