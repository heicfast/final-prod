# MINOR N-140 — the per-extension carve-result aggregation surface (the og's GetRawUserInfoMap + post-scan report breakdown) has no core counterpart: CarveStats is scalar-only, the report carries category counts, and no API tallies carve_format over results

**Severity:** MINOR (a report/telemetry-content gap: the per-node data exists and a client could tally it — the missing piece is the engine-side aggregation surface the og serves pre-computed)
**Module:** carve / report / query (the scan-results statistics surface)
**Parameters affected:** D13 (report schema — the og's per-extension breakdown has no core field), A42 (public surface shape — the aggregation operation), D14-adjacent (the stats-map shape)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:72213-72221` (`GetRawUserInfoMap` export returns the user-info singleton's +0x90 stats map); `:11376-11531` (`FUN_18000ed00`, the insert-or-increment: existing key → `node[8] += 1` at `:11466`, new key → value 1 at `:11525-11526`; ~60 analyser call sites feed it — `:11571`, `:14336`, `:17856`, `:18418`, `:19626`, `:19969`, `:20222`, `:20794`, `:20892`, `:21226`, `:21574`, `:21926`, `:24942`, `:29402`, `:29919`, `:30331` … `:64079`); `:9047-9051` (cleared at every searcher init — per-scan lifetime); DRWUI's reader: `ghidra-drwui/DRWUI_decompiled.c:227538-227625` (the post-scan "Trigger_Raw"/"FileSystem" report builder renders the per-extension counts — the "found N .jpg / M .png" breakdown; CORRECTIONS.md #9's decode)
**Core references:** audited `46b81573` — `include/cr/carve/cr_carve.h:27-57` (`CarveStats`: 13 scalar counters — hits/validated/rejected/duplicates/hashed/bytes_scanned/occupancy_skipped/sign_hits/sign_validated/windows_skipped/bitmap_skipped_clusters/bitmap_skipped_bytes/zero_strides_skipped — no per-format map); `src/core/orchestrator/cr_scan.cpp:1873-1887` (the orchestrator copies 5 scalars into ScanStats); the report's 8 category counts; the per-node `carve_format` field EXISTS (`src/ui/cr_ui.cpp:3224`) and the format-REGISTRY enumeration exists (`cr_ui_carve_format_count`/`cr_ui_carve_format_info`, `cr_ui.cpp:3467-3490` — the static signature table, not scan results); no API aggregates results per format/extension (verified: no aggregation surface in engine/ABI/CLI)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** the og-side semantics are covered (CORRECTIONS #9, D-104, SCAN-ENGINE-DELTA F2) — this finding registers the CORE-side absence, which no prior finding owns. The original S11 candidate text's "grep carve_format: zero hits" scope error (it missed src/ui) was corrected by the lead before filing — the filed gap is the aggregation surface only.

## Summary

The og's carve engine maintains, per scan, a live `map<extension → count>`
fed by every analyser detection (~60 sites) and serves it to the host through
the `GetRawUserInfoMap` export; DRWUI renders it as the post-scan
per-extension breakdown. The core's equivalent statistics (`CarveStats`) are
13 scalar totals; the report carries 8 category counts; the per-node
`carve_format` field and the format-registry ABI exist, but nothing tallies
results per format — an ABI host wanting the og's breakdown must walk the
whole tree and count nodes itself.

## What the reference does

As cited: the singleton stats map at +0x90, insert-or-increment per
detection, per-scan clear, one-way post-scan read by the DRWUI report
builder.

## What our core does

As cited: scalars only at every layer (engine stats, ScanStats copy, report
categories). The per-format data is per-node (`carve_format` on the ABI row)
but never aggregated server-side.

## Gap

Falsifiable: after a scan that carves 405 JPG + 42 PNG, ask the ENGINE (not
the tree) for the per-format breakdown. og: the map holds {jpg:405, png:42}
and the report renders it. core: no API returns it; the report's scan
analysis shows category totals only.

## Impact

Report/telemetry parity (the og's post-scan screen shows the per-extension
breakdown — a core-side ABI host cannot reproduce it without a full tree
walk); no recovery-correctness impact.

## Reproduction / verification

Decompiled + source (strength 2/3): all cites re-read first-hand; the full
walk in `evidence/decompiled-quotes/s11-rawsearchdll-infra-walk.md` §3 C2 +
the lead verification log (including the scope correction).

## Fix plan (for the engineering team)

1. Extend `CarveStats` with a per-format tally: a fixed open-addressing table
   over the existing format-id space (the registry is bounded —
   `carve::registry_size() + rawsigns::kFormatCount`, the same bound
   `cr_ui_carve_format_count` uses), incremented at the same sites the
   scalar counters tick — no map allocation in the hot path.
2. Surface it: `ScanStats` gains the tally (or an accessor over it); the
   report's scan-analysis section gains a per-format array (id, count) in
   registry order; optionally a `cr_ui_scan_carve_stats(index)` ABI getter
   beside the existing format-registry pair so hosts render the breakdown
   without a tree walk.
3. Tests: `Carve.StatsPerFormatTally` (mixed-format fixture → the tally
   matches a manual tree-walk count); `Report.ScanAnalysisCarriesPerFormat`;
   `Session.StatsTallyRoundTrip` if persisted (recommended: persist — the
   og's map outlives the scan into the report).
4. Acceptance: the fixture's per-format counts are retrievable from the
   engine surface alone; the report renders them; no hot-path allocation.

## Tests to add

- `Carve.StatsPerFormatTally`
- `Report.ScanAnalysisCarriesPerFormat`
- `Session.StatsTallyRoundTrip`

## Related findings

- N-139 (the same walk's other MINOR)
- I-031 (the walk's INFO register)
- D-104 / SCAN-ENGINE-DELTA F2 (the og-side semantics registrations this
  finding's core-side absence completes)

## Open questions

None.
