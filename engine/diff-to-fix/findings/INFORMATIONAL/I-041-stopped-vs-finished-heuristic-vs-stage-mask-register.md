# INFORMATIONAL I-041 — stopped-vs-finished classification: the og infers "stopped" from progress<98% at the advanced-end transition and persists the verdict; the core derives it truthfully from the stage mask — a deliberate modeling improvement, registered

**Severity:** INFORMATIONAL
**Module:** orchestration / session (state model)
**Parameters affected:** scan lifecycle (31), session persistence semantics (36)
**Corpus references:** `DRWUI_decompiled.c:205797-205813` (`FUN_1400f1b10` — advanced end while state==5 AND progress (`+0x1b0`) < 0x62 (98) → state 7 "stopped", else 8 "finish"), `:314692-314712` (`ScanFinished` re-derives 7/8 from the stopped flag), `:837740-800` (the verdict restored on session load)
**Core references:** `src/core/orchestrator/cr_scan.cpp:2014-2016` ("a cancelled discovery must not stamp the stage done — G-07 integrity"), `:2583-2589` (resume skips only truthfully completed stages); audited at core `8d0d88b8
**Status:** READ-ONLY REGISTER — no fix applied

## Summary

The og's stopped/finished verdict is a heuristic (progress < 98% at the end transition)
persisted into the session; the core's stage-done bits are stamped only on truthful
stage completion. The mapping is explicit: og states 3/7 ↔ core cancelled-stage; og 4/8
↔ core stage-done bit. The core model is the honest replacement — registered so parity
tables stop looking for a "state 7" equivalent.

## What the reference does

The heuristic + persistence + restore chain (cites above, lane D first-hand).

## What our core does`

Truthful stage bits; cancelled stages never stamp.

## Gap

None (deliberate improvement).

## Impact

None behavioral; the register prevents future sessions from filing the missing
heuristic as a gap.

## Reproduction / verification

Both sides read first-hand (lane D; the lead auditor verified the 0x62 threshold site).

## Fix plan

1. Register the mapping in the orchestration module doc (one paragraph; doc lane).
2. No code change.

## Tests to add

- None.

## Related findings

- S-057 (the stop-window persistence this model feeds)
- N-178 (the pause legs of the same state machine)

## Open questions

None.
