# MINOR N-123 — modules/cli.md still documents "exactly 7 commands" while the dispatch has 11 verbs (sessions/bench/driver/list-carve-formats added since) — the N-020 count-drift recurrence class, plus the exit-4 row was never updated

**Severity:** MINOR
**Module / Surface:** CLI (modules/cli.md)
**Parameters affected:** 68 (documentation accuracy), A42-adjacent (the verb surface)
**Corpus references:** n/a — audit-trail doc-drift finding (the og has no CLI; the module row's job is core truth).
**Core references:** audited commit `ff879f9` — `diff-to-fix/modules/cli.md:14-16` ("exactly 7 commands" — the N-020-era correction) vs `src/app/rrecov.cpp:3025-3047` (the dispatch: list, list-carve-formats, sessions, scan, tree, recover, health, fix, apply, bench, driver = **11**); rrecov.cpp:35 (exit 4 = the driver prerequisite) absent from the module's exit-code row.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** N-020 fixed this class once (the 7-verb correction); every verb wave since (N-033, W1a, S-029, D-DRV-3, the bench lane) landed without updating the module row.

## Summary

The CLI module row undercounts the verb surface by four and misses an exit code; the three-site verb-lock discipline updated rrecov's header + API.md + CORE-API-CURRENT each time but never the audit-trail module.

## What the reference does

n/a

## What our core does

As cited.

## Gap

Falsifiable: read cli.md:14-16 vs rrecov.cpp:3025-3047.

## Impact

Roadmap truth for the CLI surface (the module row is what a future audit reads first).

## Reproduction / verification

Read both this session.

## Fix plan (for the engineering team)

1. Update the commands row to the 11-verb dispatch with each verb's disposition note (the N-033/W1a/S-029/D-DRV-3 cites). 2. Add exit 4 to the exit-code row. 3. Consider a test-side guard: a unit test asserting the module file's verb count matches the dispatch (the doc-truth pattern, catches the recurrence).

## Tests to add

Optional `Docs.CliModuleVerbCountMatches` (a parse-the-module-file test — the doc-truth pattern).

## Related findings

N-124 (the same recurrence class in transport.md); N-020 (the prior fix).

## Open questions

None.
