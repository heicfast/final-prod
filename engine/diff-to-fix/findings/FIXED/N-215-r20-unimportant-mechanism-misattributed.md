# MINOR N-215 — R-20's mechanism attribution is wrong: the og's "Unimportant files" bucket is fed by the usefullostfiles.xml extension gate, not the M-018 junk-name filter

**Severity:** MINOR
**Module:** docs (REFERENCE-API-GAP R-20) / query (display-tier organization)
**Parameters affected:** #46 (documentation of the contract), #16 (filter model — two different og layers conflated)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:320366-320439` (`FUN_1401b8380` — the usefullostfiles.xml reader: `<exe-dir>\config\usefullostfiles.xml`, TinyXml, 27-ext hardcoded fallback, cap 101, the singleton+0xa0 set), `:635764-635910` (`FUN_1403bb970` — the bucket builder: "Unimportant files" node, `strlen < 3` folder rule, the recursive any-useful-descendant walk `FUN_1403bbdc0` at `:636028-636041`, the `_strlwr` + set-membership file gate at `:635886-635899`), `:339014-339024` (the "Build tree finish." completion handler wiring); all read first-hand this session
**Core references:** audited `3799dee2`, re-verified LIVE at `271f5d06` — `docs/REFERENCE-API-GAP.md:214` (the R-20 row: `"Unimportant" ≈ M-018 junk filter`); `docs/PARITY-OG-CORE-INVENTORY.md:64` (the usefullostfiles OOS-UI disposition — accurate); `src/core/orchestrator/cr_filterfolder.h` (M-018's junk-name blocklist — the OTHER og layer)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og has TWO distinct junk-classification layers: (1) FilterFolder.ini's hidden-name blocklist (M-018, landed in core — hides OS droppings from the tree), and (2) usefullostfiles.xml's extension-membership gate, which at results-tree-build time demotes non-useful-extension files into the "Unimportant files" bucket under the lost branch. REFERENCE-API-GAP.md's R-20 row attributes the og's "Unimportant" branch to the M-018 junk filter — the wrong layer. A future implementer of the Unimportant branch would build the wrong predicate (name-based hide vs extension-based demotion).

## What the reference does

Verified first-hand this session:

1. The reader `FUN_1401b8380` (DRWUI:320366-320439) loads `config\usefullostfiles.xml` into a `std::set` at singleton+0xa0 (27 shipped extensions; an identical 27-entry hardcoded fallback on parse failure; a 101-entry cap).
2. The bucket builder `FUN_1403bb970` (DRWUI:635764-635910) runs at the "Build tree finish." completion handler (DRWUI:339014-339024) on the lost branch's subtree: it creates the flagged "Unimportant files" node (DRWUI:635824), then partitions the branch — folders with `strlen(name) < 3` go to the bucket (DRWUI:635843-635851); other folders go through `FUN_1403bbdc0`'s recursive any-descendant-has-useful-ext walk (DRWUI:636028-636041) and land in the bucket when NO descendant carries a useful extension; files are `_strlwr`'d and tested against the set (DRWUI:635886-635899) — in-set → attached to the branch root, out-of-set → the bucket. Nothing is dropped; it is a display-tier demotion.
3. FilterFolder.ini's blocklist (the M-018 surface) is a DIFFERENT mechanism: name-pool matching that hides entries entirely — consulted at scan time, not at tree-build time.

## What our core does

The R-20 row (`docs/REFERENCE-API-GAP.md:214`) states: `"Unimportant" ≈ M-018 junk filter`. The inventory's OOS-UI disposition (`PARITY-OG-CORE-INVENTORY.md:64`) correctly names usefullostfiles.xml as its own surface. The core implements M-018's blocklist; it has no usefullostfiles demotion (dispositioned OOS-UI, display-tier).

## Gap

Stated falsifiably: REFERENCE-API-GAP.md:214 claims the og's Unimportant branch is the M-018 junk filter's equivalent; the og's Unimportant branch is built from the usefullostfiles.xml extension set at tree-build time (DRWUI:635764-635910) and never consults FilterFolder.ini.

## Impact

Doc-truth only (the M-120 committed-truth class): a future lane or host integrator reading R-20 would implement the Unimportant branch with the junk-name predicate instead of the useful-ext predicate — two different data sources (FilterFolder.ini name pools vs usefullostfiles.xml extension list), different timing (scan-time hide vs completion-time demotion), and different user visibility (hidden vs bucketed). The behavioral gap itself is already dispositioned OOS-UI; this finding owns only the mis-attribution.

## Reproduction / verification

All four DRWUI bands quoted in the references read first-hand this session; the R-20 row read at `3799dee2` and re-verified unchanged at `271f5d06`. Evidence sheet: `evidence/s69-lane-sheets/S69-A-localdb.md` §B.1/§D.2.

## Fix plan (for the engineering team)

1. Re-truth `docs/REFERENCE-API-GAP.md:214`'s R-20 row: the og's "Unimportant" branch = the usefullostfiles.xml extension gate demoting non-useful files/folders into a bucket node at the completion handler (cite DRWUI:635764-635910); the M-018 junk filter is the separate scan-time hide layer. Keep the Versions/Tags-Extracted gap statement (M-101 owns it).
2. While in the row: note the OOS-UI disposition cross-link (PARITY-OG-CORE-INVENTORY.md:64) so the two documents cannot drift apart again.
3. Files: `docs/REFERENCE-API-GAP.md` (one row); no code change.
4. Acceptance: the row names the correct mechanism with the DRWUI cite; grep "Unimportant" over docs/ returns only the correct attribution.

## Tests to add

None — doc-truth. (If the Unimportant branch is ever implemented, the tests live with that feature: bucket-by-extension, folders-by-recursive-useful-descendant, the strlen<3 rule, the 27-ext set + fallback.)

## Related findings

M-018 (the junk-name blocklist — the layer R-20 wrongly credits), M-101 (the Versions/Tags-Extracted branches the R-20 row already tracks as missing), N-132 (the display-config data family — landed), M-073 (the scan-time folder filter — a different scan-time surface).

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-215 landing (the S81 MINOR-first lane)
**Verified by:** the corpus re-verification this session, first-hand: `DRWUI:320396` (`sprintf_s("%s\\config\\usefullostfiles.xml")` — the reader FUN_1401b8380's file path), `DRWUI:635824` (`FUN_1400563c0(local_100, "Unimportant files")` — the bucket node's creation in FUN_1403bb970), the `_strlwr` + membership gate in the same band — all three cited sites re-read directly. The post-fix grep: `rg "Unimportant" docs/` returns only the correct attribution (the usefullostfiles.xml gate) at every site — REFERENCE-API-GAP.md's R-20 detail row + its summary row, PARITY-OG-CORE-INVENTORY.md's cross-linked row.
**Resolution notes:** Both R-20 rows in REFERENCE-API-GAP.md re-truthed: the og's "Unimportant" branch is the usefullostfiles.xml extension gate at tree-build (the reader DRWUI:320366-320439; the bucket builder FUN_1403bb970 at the "Build tree finish." handler, DRWUI:635764-635910 — `_strlwr` + set-membership for files, `strlen<3` folders, the recursive any-useful-descendant walk), NOT the M-018 junk filter (the separate scan-time name-blocklist hide — different data source, timing, and visibility). The M-018 reference is removed from the R-20 "Ours" column (the junk filter is not the Unimportant counterpart; it stays landed as its own surface). The OOS-UI disposition cross-link added to PARITY-OG-CORE-INVENTORY.md's usefullostfiles.xml row (both documents now name the same mechanism with the same cites — they cannot drift apart again). No code change (the behavioral gap itself is dispositioned OOS-UI; this finding owned only the mis-attribution). The Versions-node/Tags-Extracted gap statement stands (M-101 owns it).
