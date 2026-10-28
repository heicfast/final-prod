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
