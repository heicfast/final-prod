# MINOR N-139 — carve-engine OOM policy: the og logs a 5-line memory diagnostic then dies by non-continuable SEH; the core fails silently (two inconsistent paths, zero diagnostics)

**Severity:** MINOR (diagnosability divergence + an internal inconsistency in the core's own two OOM paths; no wrong recovery results — the core's graceful path is arguably the safer behavior, but the divergence is unregistered and the throwing paths escape as uncaught exceptions)
**Module:** carve (engine allocation policy) / orchestrator (error propagation)
**Parameters affected:** B23-adjacent (error codes — OOM is not a differentiated class), A35 (error recovery — the retry/fail propagation shape), P47-49-adjacent (memory behavior at the limit)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:958-983` (`FUN_1800020c0`, the DLL's global operator new — `malloc`; on NULL: `LogInfo("Failed to alloc 0x%08x bytes")`, `GlobalMemoryStatusEx` (with its own failure log), three more LogInfo lines carrying total/available physical + pagefile MB, then `RaiseException(0xe0000001, 0xc0000025 /*EXCEPTION_NONCONTINUABLE*/, 0, NULL)`); the controller itself allocates through it (`thunk_FUN_1800020c0(0x160)` at `:72128`); the product-wide policy: `ghidra-drwui/DRWUI_decompiled.c:94179` carries the identical RaiseException (DRWUI's own operator-new copy — log-then-die is the product's OOM contract; no handler exists)
**Core references:** audited `46b81573` — `src/core/carve/cr_carve.cpp` — the eight emission sites use `new (std::nothrow)` → `return ERR_NO_MEMORY` (`:434-437`, `:520`, `:542-545`, `:561`, `:643-646`, `:679`, `:824-827`, `:890`); BUT the window buffer `std::vector<u8> win(static_cast<size_t>(want), 0)` (`:919`) and the tail-scan buffer (`:757`) are THROWING allocations with no try/catch in `carve_slice` and none upstream (`grep -rn "catch|bad_alloc" src/app/` → zero); no OOM-specific diagnostic anywhere in the carve path (cr_carve.cpp includes no logging header; the only trace is the orchestrator's generic scan-end rc line, `cr_scan.cpp:1146-1151`)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11 RAWSearchDLL infrastructure walk (the trail's named wave-2 candidate, executed this session). CORPUS-ERRATA #102 covers the NTFS-side operator-new note only; no prior finding covers the carve OOM policy or the core's throwing-buffer inconsistency.

## Summary

The og's carve engine treats allocation failure as a hard, loudly-diagnosed
terminal event: a five-line PUBLOG memory dump (requested size + full
physical/pagefile status) followed by a non-continuable SEH exception —
process death with a forensic trail. The core's carve engine fails silently:
eight allocation sites return `ERR_NO_MEMORY` with no diagnostic, while its
two buffer allocations throw `std::bad_alloc` uncaught — so the core's own
two OOM paths disagree with each other (graceful rc vs. an exception escaping
the engine), and neither produces any record of the failed size or memory
state.

## What the reference does

`FUN_1800020c0` (`:958-983`, quoted in full in the evidence): malloc → on
NULL, five PUBLOG lines (failed size, memory-status query failure branch,
the four-field total/available physical+pagefile dump) →
`RaiseException(0xe0000001, 0xc0000025, 0, NULL)`. Every engine allocation
routes through it; no caller checks null. DRWUI carries the identical policy
(`:94179`) — the product contract is log-then-die; the crash reporter
(DRWReport/WER) owns the aftermath.

## What our core does

As cited: the nothrow sites fail gracefully but silently; the two buffer
sites throw with no handler anywhere between `carve_slice` and `main` — an
OOM there terminates via `std::terminate` with no carve-context record. The
core's logging seam (the cr_log host-sink, PARITY-DELTAS D-81/D-86) is not
wired into the carve path.

## Gap

Falsifiable: force OOM inside a carve scan (allocation-failure injection at
the window buffer vs. an emit site). og: 5 PUBLOG lines + exception
0xe0000001. Core: emit-site injection → silent `ERR_NO_MEMORY`; buffer
injection → uncaught `std::bad_alloc` (process death without diagnostics).
The core's two paths also disagree with each other — a contract gap
independent of the og comparison.

## Impact

Field diagnosability of OOM-class failures (large-image scans on memory-
constrained hosts are the exact population); API-contract cleanliness (an
exception escaping the engine violates the error-code contract every other
carve failure honors). No wrong recovery results either way.

## Reproduction / verification

Decompiled + source (strength 2/3): the og body re-read first-hand at
`:958-983`; the DRWUI policy-twin grepped (`0xe0000001` → only the identical
RaiseException, no handler); the core cites read at `46b81573`; the no-
upstream-catch grep executed. Full walk: `evidence/decompiled-quotes/
s11-rawsearchdll-infra-walk.md` §3 C1 + the lead verification log.

## Fix plan (for the engineering team)

1. Unify the core's two paths: wrap the two buffer allocations in
   `carve_slice` (cr_carve.cpp `:757`, `:919`) in try/catch
   (`std::bad_alloc` → the same `ctx.aborted.store(true); return
   ERR_NO_MEMORY;` the nothrow sites use) — the engine's error-code contract
   then holds on every OOM path.
2. Add the diagnostic the og has (sized to the core's logging seam): one
   log line at the failure site carrying the failed allocation size and the
   scan's window index (`cr_log` host-sink if wired, else the orchestrator's
   scan-end record gains a structured OOM note with the size).
3. Do NOT adopt the og's raise-exception policy (the core's graceful path is
   the better behavior for a library; register the deliberate divergence in
   PARITY-DELTAS beside D-81/D-86 with one line: "OOM → ERR_NO_MEMORY +
   one-line diagnostic; the og log-and-dies — deliberate").
4. Tests: `Carve.OomWindowBufferReturnsCode` (allocation-failure injection —
   a test hook or a `new` handler scoped to the test — expects
   `ERR_NO_MEMORY`, not an exception); `Carve.OomEmitSiteDiagnostic`
   (the log line carries the size).
5. Acceptance: an injected OOM at any carve allocation site yields
   `ERR_NO_MEMORY` + exactly one diagnostic line; no exception escapes the
   engine; the PARITY-DELTAS row exists.

## Tests to add

- `Carve.OomWindowBufferReturnsCode`
- `Carve.OomTailBufferReturnsCode`
- `Carve.OomEmitSiteDiagnostic`

## Related findings

- N-140 (the same walk's other MINOR — the stats surface)
- I-031 (the walk's INFO register: the pending-cap, Stop-flush, progress and
  sector-gate notes)
- D-81/D-86 (the logging seam this fix wires into)

## Open questions

None.
