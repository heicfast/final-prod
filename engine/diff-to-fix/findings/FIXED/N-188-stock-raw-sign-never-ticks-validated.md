# MINOR N-188 — the stock raw-sign layer never ticks `validated`: carved_found excludes the og's own layer-1 sign-table channel (the N-171 class, stock side)

**Severity:** MINOR
**Module:** carve (stats)
**Parameters affected:** report schema (13), scan statistics accuracy
**Corpus references:** n/a — core-side stats-truth defect, the same frame as N-171 (the og's own counting surface is the GetRawUserInfoMap per-scan map, FUN_18000ed00 insert-or-increment, which counts every analyser detection; the core's `carved_found` headline is the count analog)
**Core references:** `src/core/carve/cr_carve.cpp` (the stock raw-sign walk: `sign_hits`/`sign_validated` tick at the CANDIDATE site, before the F9 dedup; the emission path adds the node with no `validated` tick), `src/core/orchestrator/cr_scan.cpp:2362` (`stats.carved_found = cs.validated` — the sole consumer), `include/cr/carve/cr_carve.h:52` (`u64 validated = 0; /* grammar-confirmed */` — the design comment that baked the exclusion in)
**Status:** FIXED (filed during N-171's fix verification — the §8 discovery protocol; fixed in the same session)

## Summary

`carved_found` is assigned from the carve engine's `validated` counter, which only the
grammar layer ticked (plus the user-sign layer after N-171). The stock raw-sign layer —
the og's own layer-1 sign table, 139 formats — emits records into the tree with
`sign_hits`/`sign_validated` ticks only, so a file carved by a sign-table-only format
(no grammar analyser: `sample`, `a`, `amr`, `class`, `evtx`, ...) is invisible in the
headline carved count, at runtime and in the persisted session record. Same defect
class as N-171, stock side; the audit's N-171 sweep stopped at the user-sign channel.

## What the reference does

The og's per-scan stats map counts every analyser detection (the raw-sign layer's
records are results the same way grammar records are); the og's headline count includes
its layer-1 raw carving output.

## What our core does

`validated` was documented "grammar-confirmed" and ticked only at the grammar layer's
emission (:822). The stock raw-sign emission adds the tree node without ticking it.
Empirically proven during N-171 verification: a fixture whose only content is the
`sample` sign (`#!/bin/sh`) at a 512-stride carves exactly one record (sign_hits == 1,
the tree carries the node) while `validated == 0` — carved_found reports 0.

## Gap

A sign-table-only format's carve never lands in carved_found. Falsifiable: the probe
above (executed, not hypothesized — the printed counters were 1/0/0).

## Impact

Stats/report honesty, same as N-171: hosts and the CLI print a carved count that
silently omits the stock sign-table channel; sessions persist the undercount. No wrong
bytes, no data loss. The pre-existing orchestrator test comment ("both the grammar and
raw-sign candidates carved", test_orchestrator.cpp:1417) shows the exclusion was never
deliberate — it passed only because its PSD fixture has a grammar analyser.

## Reproduction / verification

Executed probe (this session, linux-release): the `sample`-format fixture →
`sign_hits=1 validated=0 sign_validated=0`, tree carrying one carved node. The
user-sign twin (N-171's tests) proves the assignment seam (`carved_found =
cs.validated`) is the only consumer.

## Fix plan

1. Tick `validated` at the stock raw-sign EMISSION site (after the node is added,
   matching the grammar and user-sign layers' emission-time counting — NOT at the
   candidate site, so deduped candidates never inflate the count).
2. Update the `CarveStats::validated` doc comment: emitted carve records — grammar +
   raw-sign + user-sign layers.
3. Tests: `CarveStats.StockRawSignFilesCountInCarvedFound` (RED pre-fix: validated 0).

## Tests to add

- `CarveStats.StockRawSignFilesCountInCarvedFound`

## Related findings

- N-171 (the user-sign twin — fixed together, one session)
- N-140 (the per-extension stats surface — the aggregation axis)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-171+N-188 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `CarveStats.StockRawSignFilesCountInCarvedFound` (RED pre-fix: the executed probe printed sign_hits=1 / validated=0 with the tree carrying the carved node; GREEN post-fix: validated == 1) + the N-171 pair re-run green (`CarveStats.UserSignFilesCountInCarvedFound`, `CarveStats.UserSignCarvedFoundSessionRoundTrip`) + the full Carve/UserSigns/CarveEngine families green. Suite 1236/1236 (linux-release).
**Resolution notes:** The tick landed at the stock emission site (after `add_file`, before `emitted_at.insert`) — emission-time counting matching the grammar and user-sign layers, so F9-deduped candidates never inflate the counter (the candidate-site `sign_hits` semantics — "candidates", pre-dedup by design — are untouched). The `CarveStats::validated` doc comment updated from "grammar-confirmed" to the emitted-record truth (grammar + raw-sign + user-sign). No safety hardening touched; no parity claim registered (the og's own headline semantics stay the N-167 found-size aggregate's territory).
