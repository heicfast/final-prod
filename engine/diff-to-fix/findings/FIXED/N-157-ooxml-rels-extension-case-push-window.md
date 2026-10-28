# MINOR N-157 (renumbered twice at the parallel-window reconciliations — the remote's bookkeeping batch keeps N-154, the parallel S42 lane's HFS+ finding keeps N-155, per earlier-push-wins) — the N-115 landing's OoxmlRelsPathSplit asserted lowercase extensions against the S-049 naming contract landed 43 seconds earlier in the same push window: main arrived RED

**Severity:** MINOR (a push-window test-truth defect — no product-code divergence; the carve engine's behavior is correct on both findings' terms, the test expectation contradicts the pinned contract)
**Module / Surface:** carve (tests) — the N-115 office-typing landing's proof set
**Parameters affected:** the S-049 naming contract's test-side pin (carve.md naming rows)
**Corpus references:** the S-049 executed evidence — `evidence/executions/s10b-real-rsf-session-mining.md` (the three real og sessions: every carved name is `EXT_%06d.EXT` with the UPPERCASE extension — `DOCX_000001.DOCX` pinned by `CarveEngine.OgNamingUniversalExtCounter`; `2006-07-25 02-16-00_000001.DOCX` ×24 the docProps-created family); DRWUI `FUN_140c44770` @ `DRWUI_decompiled.c:1878330-81` (`L"%s_%06d"`, the 0xff buffer); the og's 9-byte-stride id→name table `DAT_18008f070` (`RAWSearchDLL_decompiled.c:8606`, FUN_18000b7a0 — the id-indexed name source the naming layer reads)
**Core references:** `67de3276` (the N-115 landing — its test asserted `".docx"/".xlsx"/".pptx"` lowercase) vs `d4b487c8` (the S-049 landing, 43 seconds earlier in the same window — `src/core/carve/cr_carve.cpp` `og_carve_name` emits the UPPERCASE extension, and its own test set pins `PSD_000001.PSD`/`MP4_06s_000001.MP4`/`DOCX_000001.DOCX`)
**Status:** discovered this session (the §8 discovered-divergence class — the session-42 opening verification pass); CONVERGENT with the parallel lanes' repairs — `b26017a7` re-pinned the expectation to the same uppercase forms, `d10e4aa2` re-resolved over the N-126 lane's case-insensitive comparator, `1fb7603b` re-repaired the two halves' disagreement back to the direct uppercase compare per the S-049 contract. Their repairs pushed first and stand; this file lands the record of the defect class, which no finding documented.

## Summary

At session-42 open, `main` (`67de3276`) is RED: `Carve.OoxmlRelsPathSplit` fails with a
-32 case-delta on the extension assertion. The N-115 landing's test expected the carved
name to end in lowercase `.docx`, but the S-049 naming contract — landed 43 seconds
earlier in the same push window — names every carve `<PREFIX>_<%06d>.<UPPERCASE-EXT>`
(the real-session pinned truth). The N-115 commit message's "Suite 1119/1119 … 0 fail"
claim is therefore false at the pushed tree: the proof-set claim and the pushed
expectation were never reconciled against the S-049 re-pin that its own message cites
("the GrammarValidationAndSpecificity duplicates expectation updated to the new truth"
updated a different test). The engine behavior is correct per both findings; the test
expectation is the defect.

## Reproduction

Executed (strength 1): `./build/tests/cr_tests --gtest_filter='Carve.OoxmlRelsPathSplit'`
at `67de3276` → FAILED (`nm.compare(nm.size()-5, 5, ".docx")` = -32, the
`.DOCX`-vs-`.docx` case delta, three cases). Full suite at open: 1134 tests, 1 failed,
42 env-skipped. Post-repair: 1092 passed / 0 failed / 42 skipped.

## Resolution

**Status:** FIXED
**Fixed in commit:** CONVERGENT — the parallel lanes' `b26017a7` + `d10e4aa2` + `1fb7603b` (their pushes preceded this lane's; the exact-case uppercase table this lane wrote is equivalent against the same S-049 contract — the direct uppercase compare stands at HEAD)
**Verified by:** `Carve.OoxmlRelsPathSplit` RED at `67de3276` (reproduced above, the
-32 case delta) → GREEN post-repair; the full suite 1092/1092 pass (42 env-skips
unchanged); `cr_ui_tests` 61/61.
**Resolution notes:** The test's three extension expectations re-pinned to the S-049
contract's UPPERCASE form (`.DOCX`/`.XLSX`/`.PPTX`) with the contract citation in the
test comment. No engine change — the engine's name output was already correct on both
findings' terms (verified against the S-049 executed evidence: the real og sessions
name every carve `EXT_%06d.EXT` uppercase; the og's id→name table is the name source).
The N-115 commit message's green-suite claim is recorded here as the push-window
test-truth class (the same class as the N-135/`b5da834` conventions record): a proof-set
claim pushed without a re-run over the parallel window's re-pins.
