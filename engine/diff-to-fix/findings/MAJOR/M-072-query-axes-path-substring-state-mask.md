# MAJOR M-072 — Query axes missing: path_substring and state_mask (the og's 7-group AdvanceFilterCondition, core has 5)

**Severity:** MAJOR
**Module:** query / ABI / CLI
**Parameters affected:** D16 (filter model — the predicate set), P42 (public surface shape), P46 (contract text for appended ABI fields)
**Corpus references:** `re-workspace/artifacts/UIUX/31-filterbar-family-decode.md:122` ("**AdvanceFilterCondition = 0x48 bytes with 7 sub-condition groups** (enable-bits at dword offsets 0,4,8,32,60,64 + one more) — matching the QSS/config decode of 7 filter dimensions (type/time/size/state/tag/path/search)"); `:142` (the filter is pushed INTO the scan controller — see M-073); `re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md:49` (the search-group "Optional Type" dropdown semantics)
**Core references:** audited `35adcb9`. `include/cr/query/cr_query.h:57-75` — present axes: `category_mask` (type), `size_min/max`, `modified_from/to`, `confidence_min`, `extensions`, `name_substring` (:65, search), `path_prefix` (:72, M-029), `deleted_only` (:73, a bool — not a mask), `origin_mask` (:74). ABI twin `include/cr/cr_ui.h:986-1003` (same field set; `path_prefix` :994, `deleted_only` :997). Predicate `src/core/query/cr_query.cpp:133` (`deleted_only`), `:160` (`starts_with_ci` — prefix only). Absence verified: `rg -n "path_substring|state_mask" --glob '*.{h,hpp,cpp,cc}'` → 0 code hits. CLI: `tree --filter s [--deleted-only]` only (rrecov.cpp:1736-1737).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's AdvanceFilterCondition carries 7 filter-dimension groups (type/time/size/state/tag/path/search). The core's query surface implements the type/time/size axes plus search-by-name and a path *prefix* (M-029), but the path axis has no substring form and the state dimension is a single `deleted_only` boolean rather than the state-mask the og's group encodes. Two of seven groups are absent.

## What the reference does

The og's filter structure is 0x48 bytes with 7 enable-bit-guarded sub-condition groups: type/time/size/state/tag/path/search (UIUX/31:122). The path group filters by path (the corpus's decoded core-side counterpart is prefix-based per M-029's evidence, with the UI box offering substring-style matching on the same axis); the state group selects by file state (the "Lost files only" toggle at UIUX/31:17 being the one-decoded control on that dimension, alongside the 5 special branches — Tags Extracted / File Name Lost / File Path Lost / Versions / Unimportant, architecture/05:22). The exact state-bucket encodings are NOT decoded in the corpus — the nearest decoded surfaces are the toggle and the branch taxonomy.

## What our core does

`query::Filter` (cr_query.h:57-75) exposes: `category_mask`, `size_min/max`, `modified_from/to`, `confidence_min`, `extensions`, `name_substring`, `path_prefix` (prefix, ASCII-case-insensitive, cr_query.cpp:160), `deleted_only` (bool), `origin_mask`. The ABI mirrors it exactly (cr_ui.h:986-1003). No `path_substring`, no `state_mask` (grep-verified zero hits). The CLI exposes `--filter` (name substring) and `--deleted-only` only.

## Gap

A host cannot match result rows by a path substring (only prefix), and cannot select a combination of state buckets (existing-only, deleted-without-carved, lost+carved, …) — the `deleted_only` boolean offers exactly one state predicate where the og's group offers a dimension.

## Impact

API-contract gap on a present surface (6 of 7 groups land as 5). Hosts implementing the og's filter bar cannot express two of its dimensions against the core's query API. No recovery-correctness effect — a filtering expressiveness gap users of the API observe.

## Reproduction / verification

- Corpus-derived (UIUX/31:122 quoted; the 7-group census).
- Core absence first-hand: the grep above (0 code hits); cr_query.h/cr_ui.h field enumerations read; cr_query.cpp predicate walk read (:133/:160 — no substring over paths, no state mask).

## Fix plan (for the engineering team)

1. `include/cr/query/cr_query.h`: `Filter` gains `std::string path_substring;` (ASCII-case-insensitive over the materialized full path — the Row path) and `u32 state_mask = 0xF;` with `CR_QUERY_STATE_EXISTING/DELETED/LOST/CARVED` bits mapped from `tree::DeletedState` × `tree::Origin` (cr_node.h:47-48). Keep `deleted_only` as sugar for the DELETED bit (back-compat). Document the caveat: the og's own state-bucket values are undecoded (the mapping is core-derived onto the corpus dimension — registration note in PARITY-DELTAS).
2. `src/core/query/cr_query.cpp`: state predicate in `passes()`; path-substring predicate — materialize full paths only when the axis is active (the `path_prefix` precedent at :165-185; keep the 1M-node <200 ms budget, cr_query.h:13-15).
3. ABI append: `cr_ui_query` tail `const char* path_substring; uint32_t state_mask;` (zero-init = no substring / all states; append-only per ABI-CONTRACT §9, no bump).
4. `src/ui/cr_ui.cpp` `query_run_impl` (:3236) passthrough; `src/app/rrecov.cpp` `tree --filter-path-substr S --state MASK` + the three-site lock.
5. Tests: per-axis + combined; the C-consumer gate; docs ABI-CONTRACT §7 + API.md + CORE-API-CURRENT.

## Tests to add

- `Query.PathSubstringMatches` / `Query.PathSubstringCaseInsensitive` / `Query.PathSubstringNoPathMaterializationWhenInactive` (perf pin).
- `Query.StateMaskExistingOnly` / `Query.StateMaskDeletedWithoutCarved` / `Query.StateMaskCompositions`.
- `Query.DeletedOnlySugarEqualsDeletedBit` (back-compat pin).
- ABI: appended-field zero-init semantics in `ui_abi_check.c`.

## Related findings

- M-029 (FIXED — path_prefix; this finding is the remainder of the same og group).
- D-53 (registered — tag axis NOT-EXPOSED as UI state; unchanged).
- M-073 (the same og structure's scan-time half).

## Open questions

The og's state-group bucket values are not decoded in the corpus — the core's four-bit mapping is an interpretation. If a future corpus decode recovers the exact buckets, re-pin the mapping.
