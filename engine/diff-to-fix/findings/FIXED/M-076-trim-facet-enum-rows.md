# MAJOR M-076 — TRIM facet absent from enumeration rows (the og's per-row "TRIM ON/OFF" chip; core's data lives only in the health facet)

**Severity:** MAJOR
**Module:** health / storage-enumeration / ABI / CLI
**Parameters affected:** P42 (row shape/flags), X48 (latency — the opt-in design decision), Q68 (documentation of the opt-in contract)
**Corpus references:** `re-workspace/artifacts/UIUX/07-home-phase-deep-dive.md:17` ("**List rows** (`DrwHomePartListView` + `CPartitionListItem` …): per partition — icon …, drive letter + label, file system, capacity bar, and the **TRIM status chip: 'TRIM OFF' / 'TRIM ON'** (SSD-aware recovery-chance indicator), plus a per-row `BtnScan`"); `:46` (the whole-disk page carries the same chip); semantics: per-row SSD/TRIM state warning that deleted-data recovery chances are TRIM-dependent
**Core references:** audited `35adcb9`. `include/cr/cr_ui.h:444-455` — `cr_ui_disk_info` fields: `number, size_bytes, logical_sector, physical_sector, removable, layout, table_ok, model[128], serial[128]` — **no TRIM fields**. TRIM exists only in the health facet: `cr_ui.h:1299` (`uint32_t trim_known, trim_enabled` inside `cr_ui_smart_report`), filled at `src/ui/cr_ui.cpp:4051-4054` from `win::health::query_trim_state` (`src/win/cr_health.cpp:301-323`, the fsutil `behavior query disabledeletenotify` first-"= "-match, og-parity). CLI surfaces it only under `health` (rrecov.cpp:2158-2161). `rg -i trim src/win/cr_enumerate.cpp src/ui/cr_ui.cpp` → the enumeration path has zero TRIM logic. No `list --trim` flag.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's home-page rows carry a per-partition TRIM status chip — the SSD-aware recovery-chance indicator that tells the user, before scanning, whether deleted-data recovery is TRIM-compromised. The core's enumeration rows (`cr_ui_disk_info`) carry no TRIM fields; the datum is reachable only through a separate `cr_ui_health_read` call, so a host rendering the og's rows from `cr_ui_enumerate_disks` cannot show the chip.

## What the reference does

Every partition row on the og's landing page (and the whole-disk page, UIUX/07:46) renders the TRIM chip ("TRIM OFF" / "TRIM ON") as a per-row SSD-aware recovery-chance indicator (UIUX/07:17). The data source is the OS TRIM state (the fsutil `behavior query disabledeletenotify` query — the same primitive `win::health::query_trim_state` implements).

## What our core does

`cr_ui_disk_info` (cr_ui.h:444-455) carries geometry, removability, layout, table health, model, serial — no TRIM fields and no opt-in flag. The enumeration path (`cr_ui.cpp:1631-1671` → `cr_enumerate.cpp:139-159`) never consults `win::health::query_trim_state`; TRIM state is health-facet-only (`cr_ui_smart_report.trim_known/trim_enabled`), reachable via a second call per disk. The CLI's `list` verb has no TRIM output.

## Gap

A host rendering the og's home-page rows cannot show the per-row TRIM chip: `cr_ui_disk_info` lacks the fields, and the enumeration path never queries TRIM. Falsifiable: `cr_ui_enumerate_disks` output for an SSD with TRIM on contains no TRIM datum of any form.

## Impact

A product-behavior gap at the decision moment (before scanning an SSD, the user should see the recovery-chance warning). The data exists in the core (health facet) — this is a row-shape/wiring gap, not a capability gap. Latency is the design tension: the og queries per-row on its home page; the core's plan-of-record words an opt-in flag with a DEFERRED-if-slow measurement.

## Reproduction / verification

- Corpus-derived (UIUX/07:17 and :46 quoted).
- Core first-hand: cr_ui.h:444-455 field census read; the enumeration path greps (zero TRIM logic); the health-facet fill site read (cr_ui.cpp:4051-4054).

## Fix plan (for the engineering team)

1. `include/cr/cr_ui.h`: append `uint32_t trim_known, trim_on;` to `cr_ui_disk_info` (zero-init = unknown — pre-existing hosts unaffected), plus the opt-in mechanism: per the append-only policy, a `cr_ui_enumerate_disks_ex(..., uint32_t flags)` export (81→82) with `CR_ENUM_FLAG_TRIM` — the plan's "only when the caller opts in via a flag — enumeration latency is the og's own concern".
2. `src/win/cr_enumerate.cpp` + `src/ui/cr_ui.cpp`: when opted in, call `win::health::query_trim_state` per disk (cache-inhibited); record the latency measurement with the landing (the plan's DEFERRED-if-slow decision point).
3. CLI: `list --trim` (three-site lock + `-h`).
4. Tests: flag off → `trim_known == 0`; flag on → queried (the Windows lane pins the plumbing, the stub lane pins the marshalling); docs: ABI-CONTRACT §7 append + count updates.

## Tests to add

- `Enumerate.TrimFieldsZeroByDefault` (no opt-in → unknown).
- `Enumerate.TrimQueriedWhenOptedIn` (Windows lane; stub lane pins the field path).
- `List.TrimFlagOutput` (CLI row shows the chip datum).

## Related findings

- S-024/M-036/N-032/N-055 (FIXED — the health facet where the datum already lives).
- S-038 (FIXED — unrelated; same product-page context).

## Open questions

The per-row latency of the fsutil query across many disks (the DEFERRED-if-slow trigger) — measure at landing; if >tens of ms/disk aggregate, the plan's fallback is a parallel query or an explicit `cr_ui_disk_trim(disk)` accessor instead of row fields.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

cr_ui_disk_info now cr_ui.h:449-460; health facet cr_ui.h:1387, filled cr_ui.cpp:4251-4252; query_trim_state still cr_health.cpp:301 (exact); CLI TRIM still health-only rrecov.cpp:2254-2257.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's M-076 landing (session-42 main lane; cite-by-title: "fix(abi+cli): M-076 - the per-row TRIM chip lands - the opt-in _ex enumeration, the append-only fields, the list --trim projection")
**Verified by:** `CrUi.EnumerationExFlagGrammarRefusesUnknownBits` (runtime, every lane: unknown bits → 0x50030003 with the named diagnostic; known bits reach the platform truth) + `Enumerate.TrimChipOptInShipsInAbiAndCli` (the three-layer source pin: the header's flag/fields/contract sentences, the impl's flag-gated `win::health::query_trim_state` call, the CLI's three sites incl. the TRIM ? unknown form); suites 1111/1111 + 64/64 green post-landing; the Windows syntax gate parses rrecov.cpp + cr_ui.cpp clean (the MSVC lane's /W4 /WX build + the C consumer compile are the Windows CI halves).
**Resolution notes:** The finding's fix plan executed with one deliberate refinement, recorded here: the plan's step-2 wording said "call query_trim_state per disk" — the datum is OS-GLOBAL (fsutil behavior query disabledeletenotify answers for the whole OS, exactly as the og's home page renders the same chip state on every SSD row), so the landing issues ONE fresh query per invocation and stamps every row (N identical fsutil process spawns would only add latency without any semantic difference; the finding's own "What the reference does" section cites the OS TRIM state as the data source). Everything else per plan: (1) `cr_ui_disk_info` + `trim_known`/`trim_on` (append-only per the §9 stability policy, zero-init = unknown — pre-existing hosts unaffected); (2) `cr_ui_enumerate_disks_ex(..., flags)` with `CR_ENUM_FLAG_TRIM` — unknown bits refused BEFORE the platform check so the grammar is lane-uniform (a refinement over the plan: a host must not learn about an ignored flag from a platform-dependent success); (3) the CLI `list --trim` (three sites: the Commands header, the usage text, the parse+render, with the `[TRIM ?]`/`TRIM:ON`/`TRIM:OFF` forms — a failed query prints unknown, never a guessed value); (4) docs: API.md §2.2 row, ABI-CONTRACT §5's device-rows flow + the opt-in contract in the header. The latency note (X48): the opt-in design keeps the plain path free of the fsutil spawn — the caller decides, per the plan's own design tension; the one-query-per-invocation shape is the cheapest correct form of the opt-in.
