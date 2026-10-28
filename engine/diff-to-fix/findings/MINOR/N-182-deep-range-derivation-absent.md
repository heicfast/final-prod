# MINOR N-182 — the og's deep-scan range resolver absent: the deep pass can run a derived sub-range, a <300 MB small-volume rule, and neighbor-entry extension; the core reuses the quick slice verbatim

**Severity:** MINOR
**Module:** orchestration (scan-mode-semantics focus battery)
**Parameters affected:** #31 Scan lifecycle; #1 Detection correctness (sector coverage); #52 Scalability
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:575024-575200` (`FUN_14035a6d0` — the target resolver: the deep-only path at `param_2 != 0`, the device-type-6 gate, the `FUN_1400d03e0` sub-range derivation, the 300 MB `0x12C00000` small-volume flag at `:575053-575057`, the extension over neighbor partition entries of FS code 0xE/6); `:567902-567925` (the resolver called with mode 0 for quick and mode 1 for deep — the two "StartSector" trace lines); `architecture/04-recovery-workflow.md` §3 (the fallback = the same device)
**Core references:** `src/core/orchestrator/cr_scan.cpp:1181-1188` (the deep pass runs over the same `slice` the quick pass used — no range resolver exists); the registered D-36 single-walk delta (whose "same StartSector as quick" wording this finding qualifies) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og resolves the deep scan's sector target through the same resolver as quick but with mode 1, and the mode-1 path — for device type 6 — can derive a **sub-range target**, apply a **<300 MB small-volume rule**, and **extend over neighboring partition entries** of FS code 0xE/6. The common case is the same range as quick (the fallback), which is what the trail's D-36 wording recorded as an invariant; the resolver's special path makes it a *common case, not a universal one*. The core has no resolver: deep always runs over the identical slice.

## What the reference does

`FUN_14035a6d0` (`:575024-575200`, read via the s42 scan-mode worksheet and spot-verified at the resolver sites): quick mode (param_2==0) returns the device; deep mode (param_2!=0), **gated on device type 6** (vtable 0xa8), calls `FUN_1400d03e0` to derive a sub-range target, flags small volumes `< 0x12C00000` bytes (300 MB), and extends over neighbor entries whose FS code is 0xE or 6. The scan start traces both ("Quick scan StartSector=%I64d, TotalSector=%I64d" `:567911`; "Deep scan …" `:567918-925`).

## What our core does

`cr_scan.cpp:1181-1188` — `run_deep` operates on the same slice basis; no derivation, no small-volume rule, no neighbor extension anywhere in the orchestrator.

## Gap

Stated falsifiably: on the affected device shapes (type 6 with neighbors of FS 0xE/6, or sub-300 MB volumes), the og's deep pass covers a different sector range than its quick pass; the core's two passes always cover identical ranges.

## Impact

Bounded: rare device shapes; a missed tail (if the og's extension covers sectors the core never scans) or extra coverage in the og. The trail's D-36/Q8/A2 wording gains a qualifier. MINOR.

## Reproduction / verification

Decompiled evidence at the cited sites (the resolver read this session; spot-verified the mode-0/mode-1 call pair at `:567902-925` first-hand); core evidence at `cr_scan.cpp:1181-1188`. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-scanmode-state-machine.md` §2.

## Fix plan (for the engineering team)

1. Decide posture: either implement the resolver (device-type-6 gate + sub-range + 300 MB rule + neighbor extension — each constant decoded), or register the absence as a deliberate delta with the D-36 wording corrected ("same target in the common case; the og's deep resolver derives type-6 sub-ranges").
2. If implementing: the constants are `0x12C00000` (300 MB), neighbor FS codes {0xE, 6}, device-type gate 6; add the two StartSector trace lines for log parity.
3. Acceptance: a type-6 mock with a 0xE neighbor shows the deep pass covering the extended range (or the registered-delta row in PARITY-DELTAS with the corrected wording).

## Tests to add

- `Scan.DeepRangeResolverSmallVolume` / `Scan.DeepRangeNeighborExtension` (only if implemented)

## Related findings

I-044 (the scan-mode decode batch), D-36 (the registered delta this qualifies).

## Open questions

What device-type 6 *is* (the RTTI name was not chased this session — likely a removable/media class; one lookup settles it; recorded in the worksheet).
