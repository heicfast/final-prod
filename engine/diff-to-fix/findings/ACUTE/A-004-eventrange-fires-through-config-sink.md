# ACUTE A-004 — ErrorRange events fire through the config sink while the guard checks the live sink (latent null-call crash / mis-delivery)

**Severity:** ACUTE (crash-class latent defect on the core public-API surface)
**Module:** orchestration (event stream, GAP-S6-10)
**Parameters affected:** O38 (progress semantics), P42 (public surface shape), R56 (interface shapes)
**Corpus references:** N/A — core-internal ABI-contract bug against the project's own documented pattern (`include/cr/orchestrator/cr_scan.h` `set_event_sink` exists so a RESUMED flow can subscribe after the handle exists)
**Core references:** `src/core/orchestrator/cr_scan.cpp:309` (guard), `:340` (call), `:76-77` (config seed), `:126-131` (`set_event_sink`), introduced by commit `56cce87` — audited at core `e93decf` (re-verified against `34b3441` working tree)
**Status:** READ-ONLY FINDING — no fix applied. (Distinct from the parallel legs' N-016 double-call finding — this is the *wrong-sink* delivery; the two live in the same function.)

## Summary

The boundary-event emitter guards on the *live* sink (`event_fn_`, which `set_event_sink` overrides at `:130`) but the `ErrorRange` event inside it is invoked through the *config* sink (`cfg_.event_fn` at `:340`). An embedder that configures the sink via `set_event_sink` on a `ScanConfig` that had no event function (the documented post-construction subscription pattern) causes a **null function-pointer call mid-scan** on the first appended read-error range; if both sinks are set but different, ErrorRange silently mis-delivers to the wrong callback while every other event type goes to the right one.

## What the reference does

Not a parity item — the reference behavior is the core's own ABI contract: `ScanController::set_event_sink(event::EventFn fn, void* user)` is the public post-construction subscription point (`cr_scan.h`), and every other emitter in the file honors it:

```cpp
// cr_scan.cpp:265, :289, :346 (guards) — and :285 (delivery)
if (event_fn_ == nullptr)
    return;
...
event_fn_(e, event_user_);      // every other event type
```

The constructor seeds `event_fn_ = cfg_.event_fn` (`:76-77`, comment: "config seed, setter overrides"), so the intended model is: the live sink is the single delivery target; the config copy is only a seed.

## What our core does

`emit_boundary_events_` (`:308-343`) guards correctly on `event_fn_` (`:309`), emits FilesFound via the live path, but then hand-builds the `ErrorRange` event and delivers it through the config sink:

```cpp
// cr_scan.cpp:307-309 + 338-341
void ScanController::emit_boundary_events_(u32 stage) const {
    if (event_fn_ == nullptr)
        return;
    ...
    e.err_start_bytes = newest_start;
    e.err_len_bytes   = newest_len;
    cfg_.event_fn(e, cfg_.event_user);      // :340 — wrong sink
}
```

`cfg_.event_fn` is raw storage copied from the config at construction and never updated by `set_event_sink`.

## Gap

Falsifiable claim: with `ScanConfig{}` (null event fn) + `ScanController::set_event_sink(fn, user)`, the first boundary event carrying an error range calls a null function pointer (undefined behavior — in practice a crash in the scan thread); with config fn A + `set_event_sink(B)`, ErrorRange goes to A while ScanProgress/FilesFound/LostCandidate go to B.

## Impact

- **Stability:** crash inside the scan worker on the documented embedder pattern — ACUTE by the severity scale (crash), though it requires an embedder wiring style the in-tree `cr_ui` shim happens not to use (the shim sets both, `scan_set_event_fn_impl`, which is why the suite is green — 313/313 at the parallel leg's baseline, 282/282 verified in this leg's own run at `e93decf`).
- **API contract:** `set_event_sink`'s contract is silently violated for one event type.
- **Observability:** error-range telemetry is exactly the signal a resume/monitor UI needs most.

## Reproduction / verification

- Code-read verified first-hand (main auditor, `e93decf`, re-checked on `34b3441`): guard `:309` vs call `:340`, config seed `:76-77`, setter `:126-131`, all other emitters using `event_fn_` (`:265/:285/:289/:346`).
- Not executed (read-only mandate; the in-tree shim masks it). Fix is one line; the two tests below are the lab proof.

## Fix plan (for the engineering team)

1. `src/core/orchestrator/cr_scan.cpp:340` — replace `cfg_.event_fn(e, cfg_.event_user)` with `event_fn_(e, event_user_)` (or route the armed event through the existing delivery helper). Fix together with the parallel leg's N-016 (double call at `:577-578`) in one pass since both sit in the same function family.
2. Grep-audit every remaining `cfg_.event` read in the file for the same class (the ctor seeds at `:76/:90` are the only legitimate ones).
3. Consider unifying boundary-event delivery into one path for all boundary event types.

## Tests to add

- `ScanEvents.ErrorRangeGoesToLiveSink` — `ScanConfig{}` + `set_event_sink(recorder)` + forced read error + scan to a boundary: ErrorRange recorded, no crash (crashes before the fix).
- `ScanEvents.SinkSwitchDeliversAllTypes` — after `set_event_sink(B)`, ALL event types including ErrorRange arrive at B only, none at the config sink A.

## Related findings

- S-009 remainder (the typed event stream this bug lives in) · N-016 (parallel leg: the double-call in the same function) · N-027 (this leg's carve/writer minors group, same-commit neighbors)

## Open questions

None.
