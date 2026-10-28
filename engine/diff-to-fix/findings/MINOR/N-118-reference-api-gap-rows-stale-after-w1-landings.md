# MINOR N-118 — REFERENCE-API-GAP's gap-check table is stale for the landed W1a/W1b/W1c rows and for R-31's components: R-52/R-54/R-56 still read PARTIAL (all three landed) and R-31's "missing" components are all dispositioned elsewhere

**Severity:** MINOR
**Module / Surface:** docs (the API-gap board)
**Parameters affected:** 46 (contract documentation), 68-adjacent
**Corpus references:** n/a — audit-trail doc-truth finding.
**Core references:** audited commit `ff879f9` — `docs/REFERENCE-API-GAP.md` — the (b) gap-check rows: R-52 (:231 area: "stage-boundary partial saves, not a full-session auto-write at completion") vs the landed W1c (cr_scan.h:282 autosave_dir + the completion leg, c95335f); R-54 ("Load is synchronous with no progress events and no load-cancel") vs the landed W1b (ce6bcb0); R-56 ("No cr_ui export and no CLI verb to enumerate/list past sessions") vs the landed W1a (cr_ui_session_list + the sessions verb, 78a6552/eb16335); R-31's listed components — RawConfig toggles (M-024 FIXED + D-97), usefullostfiles.xml (OOS-UI verified, PARITY-OG-CORE-INVENTORY:64), PriorityDisplayType (landed query/filefilter data layers, API-DELTA row 45) — all dispositioned, none missing.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** The W1 landings updated API-DELTA's §5.2 board but not the REFERENCE-API-GAP gap-check rows; R-31's disposition came in pieces across sessions.

## Summary

('The gap-check table — the document future sessions read to find work — now understates reality in the other direction: three rows say PARTIAL for capabilities that landed (R-52/54/56), and R-31 lists components that are all dispositioned. The board drift risks double-work (re-landing landed features) and false audits.',)

## What the reference does

n/a

## What our core does

As cited.

## Gap

Falsifiable: read the four rows against the landed exports/tests.

## Impact

Board truth; the session-5/6 citation-gap method starts from these boards.

## Reproduction / verification

Read the rows this session.

## Fix plan (for the engineering team)

1. Flip R-52/R-54/R-56 to PRESENT with the landing cites. 2. Rewrite R-31's delta cell to the three dispositions. 3. Sweep the remaining GAP/PARTIAL rows against the new findings M-073..M-102 (each row should now carry its finding ID — the M-073..M-102 formalization gives every open row an anchor).

## Tests to add

None (doc fix).

## Related findings

M-073..M-102 (the findings the rows map to); N-091/N-117 (the same landing-wave drift set).

## Open questions

None.
