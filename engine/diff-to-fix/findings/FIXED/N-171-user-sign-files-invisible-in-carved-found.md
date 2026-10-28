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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-171+N-188 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `CarveStats.UserSignFilesCountInCarvedFound` (RED pre-fix: the DEADBEEF user-sign fixture carved one 0xFFFE0000-tagged record with sign_hits == 1 and validated == 0 — the whole channel invisible in the carved_found feed) + `CarveStats.UserSignCarvedFoundSessionRoundTrip` (RED pre-fix: the orchestrator-level scan reported carved_found == 0 with the record in the tree; post-fix: 1 at runtime AND re-read 1 from the persisted session). Both GREEN post-fix; the full suite 1236/1236 (linux-release).
**Resolution notes:** The tick landed exactly per the fix plan's preferred step 1: the user-sign emission path now ticks `validated` right after `sign_hits` (the record IS validated — the sign matched), keeping every layer's downstream per-stage counters uniform; the alternative assignment change (cr_scan.cpp) was not needed. The session-persisted undercount closes itself through the same counter (the REC_SESSION_STATE writer reads stats.carved_found). During the verification, the SAME defect class was proven on the stock raw-sign layer (an executed probe: the sample-format fixture carved with sign_hits=1/validated=0) — filed as **N-188** and fixed in the same landing. S-055/S-056 (the same block's probe-order and filter defects) were NOT touched — the emission-site tick is order-independent and rides cleanly under whichever probe order the S-055 fix lands.
