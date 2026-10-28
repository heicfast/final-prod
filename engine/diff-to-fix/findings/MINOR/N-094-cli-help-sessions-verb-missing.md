# MINOR N-094 — CLI truth drift: the `-h` usage text omits the `sessions` verb, and CORE-API-CURRENT cites a stale dispatch range

**Severity:** MINOR
**Module:** cli / docs
**Parameters affected:** C24 (CLI verb surface — the usage text), Q68 (documentation accuracy), Q65 (the three-site lock has a fourth site)
**Corpus references:** none needed (internal-consistency finding; the og reference is the CLI's own verb table) — the W1a corpus contract chain is `rsf-format--scan-session.md:11` + `UIUX/07-home-phase-deep-dive.md:21`
**Core references:** audited `35adcb9`. The dispatch has the verb (`src/app/rrecov.cpp:3031` — `sessions` case present; wmain table :3027-3048, 11 verbs). The three lock sites are correct (header comment :7, `docs/API.md:29`, `docs/CORE-API-CURRENT.md:318` "11 verbs"). **The `-h` usage print (`rrecov.cpp:2985-3022`) lists only 10 verbs — no `sessions` line** (read first-hand; the block's visible verbs: list, list-carve-formats, scan, tree, recover, health, driver, fix, apply, bench). `docs/CORE-API-CURRENT.md:318` cites "rrecov.cpp:2993-3018" for the dispatch; the dispatch is at :3027-3048 (insertion drift).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The W1a sessions verb landed with the three-site lock honored (header comment, API.md, CORE-API-CURRENT), but the usage text a user actually sees with `rrecov -h` was never updated — the verb is invisible in the product's own help. Additionally, CORE-API-CURRENT's dispatch citation has drifted 30+ lines. The lock needs a fourth site.

## What the reference does

n/a (the reference here is the CLI's own truth: 11 verbs dispatched).

## What our core does

11 verbs in code and in the three locked docs; 10 in the `-h` text; one stale cite.

## Gap

`rrecov -h` output omits `sessions`; a user cannot discover the feature from the help. CORE-API-CURRENT points at the wrong lines.

## Impact

Discoverability of the W1a feature and doc precision. No behavioral effect.

## Reproduction / verification

First-hand: the `-h` block read (rrecov.cpp:2985-3022 — the sessions line absent); the dispatch read (:3027-3048 — present); the three lock sites read (correct); the CORE-API-CURRENT cite read vs the actual line numbers.

## Fix plan (for the engineering team)

1. Add the `sessions [--dir D] [--json]` line to the `-h` block in the W1a form (one line).
2. Re-pin `CORE-API-CURRENT.md:318`'s dispatch cite to the current range.
3. Amend the lock: `CliVerbListsAreThreeSiteLocked` grows its fourth site — the `-h` usage block (the W1a session proved the three sites can move together while the fourth drifts; make the pin explicit in API-DELTA's rule box and, if a lint exists, the check).

## Tests to add

- `Cli.HelpListsEveryVerb` (parse `-h` output; assert the 11 dispatched verbs all appear — fails before, passes after; a regression guard for every future verb).

## Related findings

- W1a landing `eb16335` (the feature whose help line is missing).
- N-095 (this session — the doc-truth batch this belongs with).

## Open questions

None.
