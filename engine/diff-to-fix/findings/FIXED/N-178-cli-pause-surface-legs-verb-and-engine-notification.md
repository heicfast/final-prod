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

---

## Addendum (S54 lane, 2026-10-29): the og's engine-pause gate skips scan-type-1 runs

A further decoded constraint for the fix plan's engine-notification leg: the og's
engine-side pause gate (`DRWUI_decompiled.c:314405/:314476`) skips runs whose
`+0xD8 != 1` — i.e. **pause is not delivered to the engine for every scan type**
(the +0xD8 field's writer is an unresolved probe, recorded in the S54 evidence
sheet). The core pauses in ALL stages — a superset in the pause-delivery direction;
if the og's type-1 runs are the deep/complete scans, the og deliberately does NOT
pause engine work for that class (pause = UI-level only there). Fix plan impact:
when porting the engine notification, gate it on the scan-type field the core
already tracks (quick/deep/carve mask) and register the core's all-stage pause as
a deliberate superset OR align to the og's type gate once the +0xD8 writer is
decoded (one probe; see `evidence/s54-deepdive-orchestration.md`, UNRESOLVED #2).

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's push (the S80/S81 MINOR-first fix lane's N-178 landing)
**Verified by:** `CrUi.PauseStdinReaderMapsKeysOntoTheToken` (the source-contract pin, the repo's convention for Windows-only CLI surfaces per the N-116 precedent) — green; the existing pause family (`CrUi.PauseBlocksAndResumes` + test_pause_resume.cpp) green; `win_syntax_check` green over the new console surface (the SDK stubs grew INPUT_RECORD/KEY_EVENT_RECORD/GetConsoleMode/ReadConsoleInputW/CreateThread/GetStdHandle per the gate's own contract); full suite 1550 ran / 1508 passed / 0 FAILED.
**Resolution notes:** Both legs landed. (1) The CLI leg: `scan --pause-stdin` — a Win32 console-input reader thread that maps the keyboard onto the SAME PausableToken the ABI pair drives ('p'/SPACE pauses, 'r'/ENTER resumes, the transitions print, Ctrl-C stays cancel). The reader polls the console input buffer with a 200 ms WaitForSingleObject timeout — never a blocking read, so the process can always exit on cancel; it spawns only when a console is attached (GetConsoleMode probe — a redirected stdin makes the flag a documented no-op) and terminates on the token's cancel or at scan end. The usage block documents the flag and its no-op posture. (2) The engine leg: registered as D-154 in PARITY-DELTAS (the finding's step 2; RENUMBERED from the born-collided D-150 seat at the S84 repair — N-216's landing took D-150 first, the doc_truth_gate caught the triple claim) — "the og notifies engines of pause (vtable+0x88, DRWUI:314401-404/:314482-84); the core's pause is a token wait — no decoded og engine-side consumer exists to mirror" — the registration records the absent consumer evidence (zero pause-specific engine code in any SearchDLL decompile) and the revisit condition. The ABI half was already landed (cr_ui_scan_pause/resume + the RE-parity GetTickCount compensation); this landing completes the surface's third leg. The S54 addendum's condition ("gate the engine notification on the scan-type field WHEN PORTING") does not fire: the notification leg is registered (D-154), not ported — no decoded og engine-side consumer exists to mirror — and the core's all-stage pause (the addendum's noted superset) is the registered posture inside D-154's scope.
