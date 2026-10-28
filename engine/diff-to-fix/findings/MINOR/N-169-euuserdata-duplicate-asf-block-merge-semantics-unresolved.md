# MINOR N-169 — duplicate-extension-key semantics in EUUserData: the shipped file carries a genuine duplicate `asf` block (112 blocks / 191 signs); the core MERGES both blocks — the og's own semantics undecoded

**Severity:** MINOR
**Module:** carve (user-sign data model)
**Parameters affected:** data model — sign set semantics (11 adjacent), merge and dedup strategy (29)
**Corpus references:** the shipped `og-codebase/EUUserData` — a genuine duplicate `asf` key at file offsets 4770/16766 (two objects with different mimes); the og's CJsonObject member-storage behavior (first-vs-last-vs-merge, `FUN_140077180`/`FUN_140077dca0`) is UNDECODED (OQ-1 in the lane sheet)
**Core references:** `src/core/carve/cr_usersigns.cpp:150-216` (MERGE: both blocks parse, both signs install, each block's mime backfills its own signs); the shipped default set is pre-deduped 111/190 (`data/userdata-defaults.json`, D-104; `DefaultSetShipsAndLoads` pins 190); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's own EUUserData contains two `asf` blocks. The core's loader merges both (191
signs from the raw file; the shipped defaults file is the deduped 111/190 view). Whether
the og's JSON layer keeps the first, last, or both blocks is undecoded — the semantics
of the og's own shipped data are ambiguous. The core's merge is a reasonable resolution
but is unpinned against the og.

## What the reference does

Undecoded (the CJsonObject member-storage functions named above are the deciding
artifacts). The duplicate is factual (file offsets cited).

## What our core does

Merge with per-block mime backfill (cites above).

## Gap

The og-side resolution is unknown; the core's is merge. If the og keeps last-only, the
core carries one extra sign set (the first block's) for `asf`.

## Impact

One format's sign set on the default path — negligible behavioral surface; the risk is
census drift (the 111/190 vs 112/191 counts confuse future sessions; I-033's anchors
record the deduped view).

## Reproduction / verification

Corpus-derived (the duplicate located at the cited offsets by lane C). Core-derived (the
merge path read directly; the 190-pin test cited).

## Fix plan

1. Decode the og's member-storage semantics (`FUN_140077180`/`FUN_140077dca0` — one
   focused read) and pin the core's loader to match (or register the merge as
   deliberate).
2. Files: `src/core/carve/cr_usersigns.cpp` (if change needed) + a census test.
3. Tests: `UserSigns.DuplicateKeySemantics` (whatever the decode decides).
4. Acceptance: the semantics are pinned by test; the census comment states the raw-file
   truth (112/191) vs the shipped-defaults truth (111/190).

## Tests to add

- `UserSigns.DuplicateKeySemantics` (first/last/merge — per the decode)

## Related findings

- M-129 (the same loader's strictness family)
- I-033 (the byte-identity anchors the census interacts with)

## Open questions

The og-side resolution (OQ-1, lane C sheet) — the two named functions settle it.
