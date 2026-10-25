# MAJOR M-042 — Archive carve returns no inner-file inventory (og: ZIP/RAR analysers return the complete entry list; results tree expands archives)

**Severity:** MAJOR
**Module:** carve, query (results model)
**Parameters affected:** A25 (engine algorithm steps — analyser return model), D11 (node fields), P42 (public surface — results-tree shape)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md:38` ("Entry names accumulate into a list (local_68/local_a8 vectors, FUN_180007060 = merge) — **the analyser returns the complete inner file inventory**, which the UI later presents as the archive's contents"), `:64` ("Entry list = merged into result via FUN_1800173f0 — consumed by the results-tree builder (**ZIP files expand to their inner file list in the DRWUI tree**)"), `:37` (per-entry parser `FUN_180045690` reads name/extra/comment lens + sizes + local-header offset; "the final carve size is computed from the SUM of parsed entries, not guessed")
**Core references:** `include/cr/carve/cr_signatures.h:75-80` (`CarveResult` — end offset + size only; no entry-list field); `src/core/carve/cr_analysers.cpp` (validate_zip / validate_zip_cd walk entries for validation but discard names); `src/core/carve/cr_carve.cpp:141-184` (emission — no archive-children model); `include/cr/tree/cr_node.h` (no archive-expansion node kind)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ZIP analyser accumulates every inner entry name into a list and returns it as part of the carve result; the results tree expands ZIP files into their inner file lists (the RAR analyser does the same via the granular batch artifact). The core's carve result carries only the end offset and size — no entry inventory exists anywhere in the model, so the results tree can never show archive contents.

## What the reference does

`rawsearchdll--zip-analyser-carving.md:37-38,64`: the per-entry parser (`FUN_180045690`) decodes each ZIP fixed header (name len +0x1c, extra len +0x1e, comment len +0x20 for CD entries, compressed/uncompressed sizes, local-header offset), the entry names accumulate into vectors merged by `FUN_180007060`, and the merged list is handed to the results-tree builder — "ZIP files expand to their inner file list in the DRWUI tree". The total carve size is likewise computed from the SUM of parsed entries (not guessed from a tail-walk) (:37).

## What our core does

`CarveResult` (`cr_signatures.h:75-80`) is `{end, size}` — the validators walk ZIP/RAR structures for validation (validate_zip at cr_analysers.cpp walks the CD; validate_rar4/5 walk blocks) but discard entry names and per-entry fields. The carve emission creates a single leaf node per archive. There is no archive-children node model in the tree, no payload slot for an inner inventory, and no query surface for it.

## Gap

Falsifiable claim: for a carved ZIP containing `docs/a.txt`, `docs/b.docx`, `img.png`, the og's result carries the three-entry inventory (visible in its results tree); the core's result carries a single node named `zip_0.zip` with end/size and no inventory.

## Impact

Results-model capability gap, user-observable in the og's tree (archives expand) and absent in ours; downstream consumers (query row model, report) have no archive-content field to surface. Secondary precision effect: the og's size-from-sum-of-entries rule is a sharper size bound than a tail-walk for structurally-truncated archives.

## Reproduction / verification

Corpus-derived (artifact first-cited this session): quotes above with exact lines and function names. Core side verified in-source: `CarveResult` shape read at cr_signatures.h:75-80; validators read; no `archive`/`entries`/`inner` symbol anywhere in carve (grep-verified this session).

## Fix plan (for the engineering team)

1. Extend `CarveResult` with an optional inner-inventory payload: `struct CarveEntry { std::u16string name; u64 uncompressed; u64 compressed; }` + `std::vector<CarveEntry> entries;` (cap at a sane bound, e.g. 10k entries, to bound memory on hostile archives — the S-015 lesson).
2. Populate it in `validate_zip`/`validate_zip_cd` (names are already being parsed — keep them), `validate_rar4`/`validate_rar5` (file-head walks already decode names), and `validate_7z` where cheap.
3. Emission: attach the inventory to the node — either (a) a new node payload field `archive_entries` (simplest; query/report can then expose it), or (b) synthetic child nodes under the archive (matches the og's tree expansion but changes tree shape and recovery semantics — children must NOT be independently recoverable). Recommend (a) first.
4. Query/ABI: expose per-row `archive_entry_count` (+ names on a detail fetch) as a later append-only ABI addition; register in ABI-CONTRACT §9.
5. Tests: `Carve.ZipInnerInventory` (3-entry fixture → 3 names in payload), `Carve.Rar4InnerInventory`, `Carve.HostileZipEntryCap` (10k+ entries capped, no OOM), `Carve.InventorySizeMatchesSum` (the size-from-sum rule).
6. Acceptance: `--json` for a carved ZIP includes the entry list; the og-parity claim in CAPABILITY-MATRIX gains the archive-inventory row.

## Tests to add

- `Carve.ZipInnerInventory` — fails today (no payload)
- `Carve.Rar4InnerInventory`
- `Carve.HostileZipEntryCap`
- `Carve.InventorySizeMatchesSum`

## Related findings

S-007 (carve format coverage — same analyser-family layer; the family artifacts first-cited this session sharpen it); M-029 (query model — the Type-column/inventory surfacing is the query-side consumer); M-041 (emission-block family).

## Open questions

Whether the og's RAR inventory is depth-complete (nested RAR-in-ZIP) is not evidenced in the corpus artifact; the ZIP side is (FUN_180045690 walk). Recommend implementing ZIP depth-1 and RAR depth-1 and tracking nested-inventory as UNVERIFIED if the corpus is silent.
