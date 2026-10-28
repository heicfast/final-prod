# MINOR N-159 — The failed-files recovery log is an HTML report with a per-file tree-classification column: R-58/API-DELTA row 21's "per-run text log" / "og format NOT IN CORPUS" claims refuted by first-hand decode; the classification axis is absent from the core's report

**Severity:** MINOR
**Module:** writer+report (the recovery-completion report surface)
**Parameters affected:** P13 (report schema — the failed-files report's row grammar and classification column), P11 (node fields consumed — the tree-classification mapping), P46 (documentation of the contract — R-58/API-DELTA row 21's og-side record is wrong in two claims), P68 (documentation accuracy)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (all first-hand this session): `:353589` (`ISaveFailedFiles::vftable` — the interface), `:642669-642686` (`FUN_1403c6da0` — `CSaveFailedFilesWithHtml` ctor: the 2000-byte content buffer at +0x50 via `thunk_FUN_140053400(2000)`, path string at +8, second string at +0x28, flags at +0x24/+0x48), `:642675`/`:642705` (`CSaveFailedFilesWithHtml::vftable`), `:360047` (`CloudRecover::CloudFaildFilesWithHtml::vftable` — the cloud twin), `:642729-642746` (`FUN_1403c6f50` — the per-failure collection slot +0x10: failed-count increment at +0x254, failed-bytes accumulation at +0x268 on the results singleton), `:642755-642822` (`FUN_1403c6fc0` — **the HTML row grammar with the tree-classification mapping**, quoted verbatim in the evidence file §2), `:642843-642880` (`FUN_1403c76d0` — the companion row builder), `:642885-642892` (`FUN_1403c7c90` — `ShellExecuteW(NULL, NULL, this+8, …)` opening the report in the default viewer), `:357392-357430` (`FUN_1401f0de0` — the completion-message construction: report path + existence check), `:1080747` (the user-visible copy `"(%1 files / %2 failed; for details, you may check the recovery log)"`). Vtable addresses: `re-workspace/reversed/ui-resources/drwwui_vtable_map.json` (ISaveFailedFiles 0x141886800, CSaveFailedFilesWithHtml 0x14188a078, CloudFaildFilesWithHtml 0x141886930). RTTI: `re-workspace/rtti_classes.json` (both class names).
**Core references:** audited `869da779`. The core's report surface: `src/core/recovery/cr_report.cpp` (`cr_ui_recovery_report_json` — JSON schema v1; per-item outcomes done/refused/error/hash_mismatch/resumed/pending/salvaged + totals incl. bytes; **no per-file tree-classification field**, no failed-bytes split — grep `classification|group|branch` over the report schema → 0 semantic hits), `docs/REFERENCE-API-GAP.md:104` (R-58: "Per-run **text log** … **no CSV/list-export verb is documented**"), `:272`/`:406` ("ours exceeds"), `diff-to-fix/API-DELTA.md` row 21 ("MATCH (clean-room equivalent; **og format NOT IN CORPUS**)"), `diff-to-fix/modules/writer.md` (D13 verdict — "the og has no JSON report (GUI funnel)").
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** N-155 → **N-159** (S42-B merge, 2026-10-29, SA-9: the parallel S42-main window's HFS+ zero-logical-size finding keeps N-155; this lane's filing renumbers past their N-157).
**Prior audit check:** the SURFACE is dispositioned (R-44/R-58/API-DELTA row 21/D13 — all citing UIUX/11 + DRWUI-exe--main-ui-architecture prose), but the class family (`ISaveFailedFiles`/`CSaveFailedFilesWithHtml`/`CloudFaildFilesWithHtml`) has ZERO citations anywhere in the trail (`rg 'SaveFailedFiles|WithHtml'` over diff-to-fix/ + docs/ → 0 hits, verified), and the two register claims the decode refutes were never re-checked against the decompile. Discovered by the S42 DRWUI never-walked-surface lane's agent-C pass; the load-bearing sites verified first-hand before filing.

## Summary

The og's "recovery log" — the failed-files report referenced by the completion dialog — is not a plain text log: it is an **HTML report** produced by a dedicated class family (`ISaveFailedFiles` → `CSaveFailedFilesWithHtml`, with a `CloudRecover::CloudFaildFilesWithHtml` twin), accumulated per failed file during the export loop (count + bytes counters on the results singleton), rendered with a per-file **tree-classification column** derived from the results-tree node (Lost Files / Reconstructed / Tags Extracted / Existing Files / Deleted / the custom raw-category name), and opened for the user via `ShellExecuteW`. The trail's register claims the og format is "NOT IN CORPUS" and describes a "per-run text log" — both refuted: the generator, the row grammar, and the classification mapping are fully decodable from the corpus decompile. The core's JSON report has no per-file classification axis: a user reading the core's report cannot tell WHICH KIND of file failed (lost vs reconstructed vs tags-extracted vs existing vs deleted).

## What the reference does

(All cites first-hand this session; quotes in `evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md` §2.)

1. **The sink is a singleton obtained at export completion** — `CRecoveryMgr::ExportThread`'s completion path obtains the sink via the singleton getter (`FUN_1403c6d20`, static storage `DAT_142643c60`), calls vtable slot +0x08 with the destination path, passes the sink into the per-target export loop (`FUN_1401ec0d0`), and calls slot +0x18 at completion (non-cloud only).
2. **Per-failure collection** — inside the export loop, each failed file (`FUN_14053a930`/`FUN_140539fc0` != 0) reaches `sink->vtbl[+0x10](node)` = `FUN_1403c6f50`, which increments the failed-count (`+0x254`) and accumulates failed bytes (`+0x268`) on the results singleton.
3. **The row grammar** — `FUN_1403c6fc0` builds each failed file's HTML row, classifying by node: group==1 → `"Lost Files"` (plus `"Reconstructed"` when node-kind 7, `"Tags Extracted"` when kind 6); group==0 → kind 4 `"Existing Files"`; kind 0 → `"Lost Files"` + `"Deleted"`; else the custom raw-category name (`FUN_14008a650`). The markup fragments are the `DAT_1414a58xx`–`DAT_1414a5axx` .rdata constants.
4. **Open-the-report action** — `FUN_1403c7c90` calls `ShellExecuteW(NULL, NULL, this+8, …)`; `FUN_1401401f0de0` fetches the path, checks existence, and appends it to the completion message behind `"(%1 files / %2 failed; for details, you may check the recovery log)"` (`:1080747`).

## What our core does

`cr_ui_recovery_report_json` emits a structured JSON report (schema v1): per-item outcomes (done/refused/error/hash_mismatch/resumed/pending/salvaged) and totals including bytes — a deliberate structured-export superset over the og's funnel, registered at D13. The report has no per-file tree-classification field: the classification axis the og's HTML rows carry (which branch of the results tree the failed file came from) is not represented in any core report surface (the node's group/kind is tree-internal; the report's rows carry the outcome only). No failed-bytes total split exists either (the totals carry aggregate bytes; the og separately tracks failed bytes).

## Gap

Falsifiable: recover a mixed selection where one file under `Lost Files/Reconstructed` and one under `Existing Files` both fail. The og's recovery log contains two HTML rows, each carrying its tree classification ("Lost Files Reconstructed" / "Existing Files") plus the per-class failed-count and failed-bytes counters. The core's JSON report contains two error rows with no classification field — the axis does not exist in the schema. Additionally, the trail's register claims are falsified as stated: `docs/REFERENCE-API-GAP.md:104` ("per-run text log") and `diff-to-fix/API-DELTA.md` row 21 ("og format NOT IN CORPUS") contradict the decodable generator + row grammar.

## Impact

User-observable for the report contract: the og's completion funnel tells the user WHICH KIND of file failed; the core's does not. For hosts building a UI on the core's report JSON, the classification axis must be re-derived from the tree (possible today via node ids — but not from the report alone). The register-truth half matters for the roadmap: R-58's "NOT IN CORPUS" told future lanes there was nothing to decode; the decode exists and yields an exact spec. Recovery correctness is unaffected (the files themselves carry the same outcomes).

## Reproduction / verification

Corpus-derived, cross-verified by execution of the reads this session:
1. `sed -n '642669,642700p'` + `sed -n '642755,642825p'` of `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — the ctor and row-grammar functions quoted verbatim in the evidence file §2 (the classification mapping, the 2000-byte buffer, the string appends).
2. `sed -n '388960,389045p'` — the adjacent SearchResult.txt dump (I-035's item) confirming the same results-container class's map layout.
3. The vtable addresses + RTTI names from `drwwui_vtable_map.json` + `rtti_classes.json`.
4. Core absence: the report schema in `src/core/recovery/cr_report.cpp` (no classification field) + the register claims at REFERENCE-API-GAP:104 / API-DELTA row 21 (read first-hand).

## Fix plan (for the engineering team)

1. Decide the shape (the D-107 lineage: og UI-surface semantics land as core data, not UI): add a per-item `tree_class` field to the report JSON rows — the og's six-valued classification (lost / lost-reconstructed / lost-tags-extracted / existing / deleted / raw-category-name) derived from the node's group/kind at report time, plus a `failed_bytes` total split.
2. Files to modify: `src/core/recovery/cr_report.cpp` (the row builder + the classification helper), `include/cr/recovery/cr_report.h` (the field), `docs/recovery-report-schema.md` (the schema doc + version bump per its own contract), the ABI if the JSON is consumed via `cr_ui_recovery_report_json` (schema version field, append-only).
3. Register repair (doc-only, do it in the same push): REFERENCE-API-GAP R-58's row → "HTML report (CSaveFailedFilesWithHtml; row grammar + tree-classification decode at the N-155 cites)"; API-DELTA row 21's "NOT IN CORPUS" → "decodable; the generator + grammar are in the decompile"; D13 addendum noting the classification axis.
4. Tests: a report test with failures across tree classes asserting the `tree_class` values + the failed-bytes split; a schema round-trip pin.
5. Acceptance: the mixed-failure fixture's report carries the six-way classification; the three register sites tell the truth; the schema doc's version is bumped.

## Tests to add

- `Report.FailedRowsCarryTreeClassification` (lost-reconstructed vs existing vs deleted rows)
- `Report.FailedBytesSplitFromTotal`
- `Report.RawCategoryNameClassification` (the custom-name branch)
- `Report.SchemaVersionBumpedForTreeClass` (the append-only pin)

## Related findings

M-101 (the results-tree branches the classification derives from — Versions/Tags-Extracted still active); S-049 (FIXED) and S-048 (FIXED by the parallel window — the Reconstructed tree landed; this finding's classification axis rides the now-landed tree shape); N-126 (the unsaved-file category — the same classify-at-report-time family); I-035 (the S42 decode register carrying the adjacent SearchResult.txt + HTML-fragment notes); N-150 (the doc-truth batch — the register repairs ride its scope).

## Open questions

The exact HTML markup grammar (the `DAT_1414a58xx` fragment constants' raw .rdata values — table structure, column headers, CSS) was not resolved this session; a raw .rdata read at the RVAs would pin it if a byte-exact template is wanted (not required for the classification-axis landing). The physical file-write site (no CreateFileW/fopen in the class's address cluster 642600-643100) sits in an unattributed vtable slot or an ExportThread helper — the fix lane does not need it, but a future decode pass could attribute it.
