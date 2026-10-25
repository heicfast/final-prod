# MAJOR M-021 — Carve: pre-scan video-repair wiring absent (og: RAWSearchDLL → EUVideoRepair.dll for damaged MOV/MP4)

**Severity:** MAJOR (missing user-visible post-processing feature in the carve pipeline)
**Module:** carve / orchestration wiring
**Parameters affected:** B2-adjacent (content post-processing), R54/R57 (wiring), P42-adjacent
**Corpus references:** `RAWSearchDLL_decompiled.c` — loader `FUN_18000beb0 :9210-9237` (`LoadLibraryA("…\\EUVideoRepair.dll")` → `GetProcAddress("GetIMovMp4Repair")` → interface at controller+0x158); carved MOV/MP4 feed `GetVideoInfo` (vtable+0x18) → repair check (vtable+0x48) at the deferred-analysis poll `FUN_18000d670 :10130-10136`
**Core references:** `grep -rn -i "videorepair\|GetIMovMp4Repair" src/ include/` — clean; no post-carve hook exists in `src/core/carve/` or `src/core/orchestrator/` — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's carve engine dynamically loads `EUVideoRepair.dll` at start-up and, for each carved MOV/MP4, runs a video-info/repair check through its COM-style interface before the candidate is finalized. The core has no equivalent: carved videos are emitted raw, with no post-processing hook. (Whether to *ship* a repair engine is a product decision — but the hook point, its wiring, and its absence are all undocumented in the core.)

## What the reference does

Quotes above: the loader binds `GetIMovMp4Repair` into the carve controller (interface slot +0x158), and the deferred-analysis poll invokes `GetVideoInfo`/repair-check on video candidates at `:10130-10136` — i.e. the hook is per-candidate, post-validate, pre-finalize.

## What our core does

No hook, no doc, no parity-delta registration.

## Gap

Falsifiable claim: a damaged carved .mov is post-processed (info/repair check invoked) in the og pipeline; the core emits it raw with no equivalent call site.

## Impact

User-visible on damaged-video recovery scenarios; also a template case for the general "carve post-processing hooks" architecture the og has and the core lacks.

## Reproduction / verification

Decompile-verified wiring chain (agent S2-D2; `evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`); core absence grep-verified first-hand. The og's repair *effect* on content is out of scope (FileRepair chain excluded per `docs/parity/PARITY-CORE-REC.md:164`) — this finding is about the **carve-pipeline hook**, not the repair engine.

## Fix plan

1. Decide and register: either (a) add a versioned post-carve hook interface (`CarvePostProcessor`) in the carve pipeline where a future/optional repair module can bind (mirroring the og's controller+0x158 shape), or (b) register the exclusion in `docs/parity/PARITY-DELTAS.md` under the repair-chain scope line (D-26) — today neither exists.
2. If (a): the hook must fire per video candidate after validation and before tree insertion (the og's deferred-poll position), with the interface failure (DLL absent) degrading to raw emission — matching the og's optional-load behavior.

## Tests to add

- `Carve.VideoPostProcessorHookInvoked` — registered test post-processor sees MOV/MP4 candidates (fails before: no hook).
- `Carve.HookDegradesWhenModuleAbsent` — no processor bound → raw emission unchanged.

## Related findings

- S-007 (carve formats — the candidates feeding this hook) · scope exclusions in I-010

## Open questions

None.
