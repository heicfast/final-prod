# MAJOR M-071 — M-018 regression: the junk-filter gates were lost from the CLI tree print and the recover-all planning walk (rrecov.cpp reconstruction)

**Severity:** MAJOR
**Module:** writer / query / CLI
**Parameters affected:** D16 (filter model — the junk blocklist must gate every result surface), B2/Q65 (recovery correctness of the recover-all path — it writes the og's hidden junk set), Q68 (the stale code comment now misdocuments behavior)
**Corpus references:** `og-codebase/config/FilterFolder.ini` (the junk blocklist, three pools); `re-workspace/artifacts/UIUX/31-filterbar-family-decode.md:140-143` (the filter-folders push into the scan controller); the M-018 corpus chain (findings/FIXED/M-018-junk-filter-filterfolder-absent.md — FilterFolder.ini is applied to hide `desktop.ini`, System Volume Information, etc. from results and recovery)
**Core references:** audited `35adcb9`. Gate sites at HEAD: `rg -c "NODE_FLAG_JUNK_FILTERED" src/` → only `src/core/query/cr_query.cpp` (1 hit, :119-121 the query gate) — **`src/app/rrecov.cpp` has ZERO hits** (print_tree :819-860 and the recover-all plan walk :1949-1976 both walk raw nodes); stale comment `src/core/orchestrator/cr_scan.cpp:1200` ("query rows, the CLI tree and recover-all planning all skip flagged nodes") now false. Git-proven root cause: the M-018 fix commit `69507cb` added both rrecov.cpp gate lines; 15 minutes later the N-064 repair wave reconstructed rrecov.cpp from a pre-M-018 design (`cfa4c75` "rrecov.cpp reconstructed to the converged design", then `c228d2d` restored verbatim) — `rg -c NODE_FLAG_JUNK` per version: 2 (`69507cb`) → 0 (`cfa4c75`) → 0 (`c228d2d`) → 0 (HEAD).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

M-018's junk-filter fix has partially regressed. The filterfolder mechanism itself still works (lists load, `filterfolder::apply()` runs at scan completion `cr_scan.cpp:1202`, the flag is set, the query layer gates it), but the two CLI-side gates the M-018 resolution explicitly claimed — the CLI tree print and the recover-all planning walk in rrecov.cpp — were silently dropped when the N-064 wave reconstructed rrecov.cpp from a pre-M-018 design fifteen minutes after the fix landed. Recover-all today writes the og's hidden junk set (desktop.ini, SVI internals, …).

## What the reference does

The og applies FilterFolder.ini as a product-wide junk filter: the three pools (4 folder + 7 file + 14 fat-name entries) hide known-junk nodes from every result surface — the tree view, the recoverable set, and recovery itself. The M-018 finding documented this and its resolution claimed three gated surfaces: the query layer, the CLI tree print, and the recover-all planning walk.

## What our core does

The core loads the corpus lists (generated `cr_filterfolder_data.cpp`), sets `NODE_FLAG_JUNK_FILTERED` at scan completion (`cr_scan.cpp:1202`, stat `junk_filtered`), and gates exactly one surface: `query::passes()` (cr_query.cpp:119-121 — "M-018: … never a result row"). The CLI tree print (`rrecov.cpp:819-860`) walks `first_child()/next_sibling()` with only `deleted_only`/`name_matches` filters; the recover-all plan lambda (`rrecov.cpp:1949-1976`) enqueues every node via `writer.enqueue(c, path)` with no junk check; `SafeWriter::enqueue` (cr_safewriter.cpp:102) does not check either. The comment at cr_scan.cpp:1200 still claims all three surfaces are gated — it is now false. The shipped tests pass because they only assert the query surface (`test_orchestrator.cpp:220/:283`).

## Gap

`NODE_FLAG_JUNK_FILTERED` is checked by the query layer but not by the CLI `tree` print or the `recover --all` planning walk — junk-flagged nodes print in the CLI tree and are written by recover-all. Falsifiable: a fixture with `desktop.ini`/SVI junk scanned then `rrecov recover` (all) writes the junk files; the og hides them from recovery.

## Impact

User-observable: recover-all produces junk files the reference product never writes (a recovery-set divergence, though not a data-loss class — it recovers *more*, not less, and junk files are small). The false code comment misleads future maintainers. The M-018 FIXED record is partially untruthful at HEAD — a board-truth defect this session's spot-check exists to catch.

## Reproduction / verification

- Executed-class verification (static, first-hand this session): grep census of `NODE_FLAG_JUNK_FILTERED` across `src/` (only cr_query.cpp); read of `print_tree` and the plan lambda; git archaeology `git show 69507cb` vs `git show cfa4c75` (the two added gate lines present then absent).
- The regression window: `69507cb` (fix) → `cfa4c75`/`c228d2d` (N-064 reconstruction) → every later rrecov-touching commit (W1a sessions verb, W1c autosave) built on the gate-less file.

## Fix plan (for the engineering team)

1. Re-add the two gates in `src/app/rrecov.cpp`, in the exact M-018 form: `if ((c->props()->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0) continue;` — one in the `print_tree` walk (:819-860), one in the recover-all `plan` lambda (:1949-1976). Optionally also gate `SafeWriter::enqueue` as defense-in-depth (cr_safewriter.cpp:102) — decide once, document.
2. Fix the stale comment `cr_scan.cpp:1200` to name the surfaces that are actually gated post-fix.
3. Tests (the regression-proof pair the original fix lacked): `test_rrecov_plan.py`-class CLI test — scan a fixture with planted `desktop.ini` + an SVI-style name, assert (a) `rrecov tree` omits them, (b) `rrecov recover` (all) writes zero of them while still writing the planted good files; a `JunkGatePresent` static assertion is not possible, so pin it behaviorally at the CLI level.
4. Add the rrecov.cpp gate lines to the M-018 FIXED record's "surfaces" list as re-verified post-fix.

## Tests to add

- `RecoverAllSkipsJunkFlaggedNodes` (CLI-level: planted junk never written).
- `TreePrintSkipsJunkFlaggedNodes` (CLI-level: planted junk never printed).
- `RecoverExplicitSelectionStillAllowsJunk` (explicitly-selected junk can still be written — the og's filter is a results/recovery-set default, not a hard block; pin the semantics chosen).

## Related findings

- M-018 (FIXED but regressed in two of three claimed surfaces — this finding is the regression record).
- S-007/M-029 (the carve root naming — unrelated but shares the results-visibility concern).
- The N-064 landing (the reconstruction that dropped the gates).

## Open questions

Whether `SafeWriter::enqueue` should carry the gate (defense-in-depth) or the plan walk alone (current design intent) — an engineering call; the fix plan works either way.
