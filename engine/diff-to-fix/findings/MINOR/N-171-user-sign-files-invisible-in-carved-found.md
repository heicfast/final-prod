# MINOR N-171 — user-sign emissions never tick `validated`/`carved_found`: the headline carved count undercounts the entire default-on user-sign channel (runtime + session-persisted)

**Severity:** MINOR
**Module:** carve (stats) / session persistence
**Parameters affected:** report schema (13), scan statistics accuracy, session-persisted stats
**Corpus references:** n/a (the og has no user-sign channel — D-104/E-A); the og's own carve counting surface is the GetRawUserInfoMap per-scan stats map (FUN_18000ed00 insert-or-increment; DRWUI report reader :227538-625)
**Core references:** `src/core/carve/cr_carve.cpp:803` (`ctx.sign_hits.fetch_add(1)` — the only counter the user-sign path ticks), `src/core/orchestrator/cr_scan.cpp:2184` (`stats.carved_found = cs.validated` — grammar layer only), the session-persisted `carved_found` (`include/cr/session/cr_records.h:137`); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The user-sign emission path increments only `sign_hits`; `carved_found` is assigned from
`cs.validated` (the grammar layer's counter), so every user-sign file — the entire
default-on channel, 78 extensions carried by no stock source — is invisible in the
headline carved count, both in runtime stats and in the session record.

## What the reference does

No channel (the count analog is the og's per-format stats map, cited above).

## What our core does

The split-counter shape (cites above).

## Gap

`carved_found` excludes user-sign records. Falsifiable: default-set scan over a fixture
with N user-sign-only magics → stats.carved_found == 0 while the tree carries N records
tagged 0xFFFE0000.

## Impact

Stats/report honesty: hosts and the CLI print a carved count that silently omits a whole
channel; sessions persist the undercount (resume/preview re-reads it). Distinct from
N-140 (the per-extension surface).

## Reproduction / verification

Core-derived (lead auditor read :803 and :2184 directly; the records field cited). The
default-set channel test (`OgAbsentFromStockExtensionsCarried`) proves the non-empty
population.

## Fix plan

1. Tick `validated` on user-sign emission (the record IS validated — the sign matched;
   the og's own stats map counts analyser detections the same way), or add
   `cs.sign_hits` into the assignment: `stats.carved_found = cs.validated +
   cs.sign_hits` — prefer ticking `validated` so downstream per-stage counters stay
   uniform.
2. Files: `src/core/carve/cr_carve.cpp` (:803 gains `ctx.validated.fetch_add(1)`), or
   `cr_scan.cpp:2184` — one of the two, not both.
3. Tests: `CarveStats.UserSignFilesCountInCarvedFound` (fails today) + a session
   round-trip leg.
4. Acceptance: the default-set fixture's carved_found equals the tree's 0xFFFE0000-tag
   count plus grammar-layer carves.

## Tests to add

- `CarveStats.UserSignFilesCountInCarvedFound`
- `CarveStats.UserSignCarvedFoundSessionRoundTrip`

## Related findings

- N-140 (the per-extension surface — the aggregation axis)
- S-055/S-056 (the same block's emission defects — fix together to avoid double-count
  during reorder)

## Open questions

None.
