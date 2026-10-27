# MAJOR M-024 — The og has a runtime per-format enable/disable surface (RawConfig.ini [Filter] + registry rebuild); the core registry is compile-time only

**Severity:** MAJOR
**Module:** carve (+ abi)
**Parameters affected:** B1 (detection set is host-configurable in the og), P42 (public surface shape — capability/filter interface), A25 (working-registry construction path)
**Corpus references:** `og-codebase/config/RawConfig.ini` `[Filter]` section (37 toggles; `HEIC=1 … STL=0 … VPJ=0 … 7Z=1 …` — STL and VPJ ship DISABLED); `GetRawUserInfoMap` = `FUN_180001350()+0x90` (RAWSearchDLL_decompiled.c:72213-72221); the searcher init fetches a filter interface from config (+0xe8 → vtable slot 0x90, :9180-9182) and `FUN_180006b30` (:4386-4434) rebuilds the three working containers with the filter applied (removal loop :4411-4431: analyser dropped when `id−0x30` hits mask 0x1000801 or the filter callback rejects); deep-dive artifact §4 ("gated per-extension at runtime via config/RawConfig.ini (37 extension toggles; loaded at line 13133)")
**Core references:** `src/core/carve/cr_registry.cpp:46-297` (static kTable; no filter), `src/core/carve/cr_carve.h:38-42` (`CarveFlags` = hash settings + threads only), `src/core/carve/cr_registry.cpp:300-305` (`registry()/registry_size()` internal accessors) — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og rebuilds its carve registry per search from a host-supplied filter: the
searcher's config interface provides an `IAnalyserFilter`, and `FUN_180006b30`
reconstructs the u32 map, u16 multimap and list minus disabled formats, driven by
`config/RawConfig.ini [Filter]` (37 named toggles; STL and VPJ disabled by default).
The core's signature table is a compile-time constant with no per-format
enable/disable surface in `CarveFlags` or any ABI — the host cannot turn formats
off, mirror the og's default-off set, or enumerate what is enabled.

## What the reference does

1. `EUCreateRAWSearcher` → CRAWSearchController; its init (`FUN_18000beb0`)
   fetches the analyser filter from the config object at searcher+0xe8 via vtable
   slot 0x90 (:9181) whenever that object is present.
2. `FUN_180006b30(registry, filter)` destroys and rebuilds the working containers
   (+0x00 list, +0x10 u32 map, +0x20 u16 multimap; teardown :4402-4404), then
   filters the analyser list (:4411-4431): each analyser whose
   `id − 0x30 < 0x19 && (0x1000801 >> (id−0x30)) & 1` — ids 0x30/0x3b/0x43/0x48 —
   or whose filter-callback (`(*(vt+8))(filter)`, :4418) says "disabled" is removed
   and destroyed.
3. The filter's data source is `RawConfig.ini [Filter]` (per-extension 0/1; the
   toggle names match the list-layer analysers 1:1: HEIC, VCF, WEBM, WEBP, M1V,
   M2T, M2TS, JP2, M4P, XMIND, RVT, MOBI, MMAP, POS, EDDX, WFP, VPJ, OSP, HFP,
   DRP, HIK, ODP, ODT, ODS, 7Z, INSP, INSV, GPX, CRM, AEP, FFF, DPX, MUSX, STL…).
   `GetRawUserInfoMap` exposes the user-extension→analyser bindings to the host.

## What our core does

`kTable` is a static array; `CarveFlags` (`cr_carve.h:38-42`) exposes only
`hash_small_files`, `hash_threshold`, `threads`. There is no API to disable a
format, no capability enumeration, and no config file consumed by the carve layer
(grep: no ini/json consumption in `src/core/carve/`).

## Gap

Falsifiable claim: for every format F in the og's [Filter] list, the og can be
configured (and ships defaults) to enable/disable F's carver at runtime, and the
host can enumerate the enabled set; the core can do neither for any format.

## Impact

- The og's shipped default-off formats (STL, VPJ) mean the og's *effective* carve
  set differs from its compiled set — parity comparisons must read the config, not
  just the binary (this finding's data feeds the S-007 + s3-rawsearch-inventory coverage tables).
- Hosts (CLI/UI) of the core cannot offer format filtering, cannot replicate og
  default-off behavior, and cannot enumerate carve capabilities (ties into N-033).

## Reproduction / verification

Corpus-doc + decompiled (strengths 3/2): RawConfig.ini quoted above;
`FUN_180006b30` :4386-4434 with the filter loop quoted; config-fetch call :9180-9182;
`GetRawUserInfoMap` :72213. Core: code-cited. Evidence file §6.

## Fix plan (for the engineering team)

1. Add `const char* disabled_formats` (comma/space-separated ext or name list) or a
   `const bool* format_enable[FMT_COUNT]` to `CarveFlags`; `scan_one_window` skips
   signatures whose slot is disabled (and the raw-signs layer skips formats whose
   name is listed).
2. Expose a capability enumeration in the ABI (see N-033): formats + names +
   enabled state (one function, stable order = table order).
3. Optionally consume a `cr-carve.ini`-style config for default-off parity
   (document any deliberate difference in PARITY-DELTAS).
4. Tests: `Carve_Format_Disabled_Skips_Signature` (JPEG disabled → plant_jpeg
   yields nothing), `Carve_Format_Disabled_RawSigns_Too`, `Carve_All_Formats_Enabled_Default`.

## Tests to add

(fix plan step 4 — fail before, pass after)

## Related findings

- N-033 (registry-export ABI — the enumeration half of this surface).
- S-007 + the s3-rawsearch-inventory coverage tables (they must account for default-off formats).

## Open questions

Which host component writes RawConfig.ini (DRWUI settings vs. installed default)
is not decoded; the file in `og-codebase/` is the installed default. Low priority.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-024 commit — see `git log` for "fix: M-024")
**Verified by:** `CarveEngine.FormatDisabledSkipsSignature` ("jpg, JPEG" listed → the
grammar layer never dispatches the JPEG signature; PNG unlisted still carves),
`CarveEngine.FormatDisabledRawSignsToo` (the "sample" format disabled → its header
sign never fires — sign_hits == 0; positive control: the sign fires unlisted), and
`CarveEngine.AllFormatsEnabledByDefault` (null/empty list = every format enabled;
the enumeration surface: registry order IS FormatId order; token matching is
exact-length, case-insensitive, comma/space/semicolon-separated — "jpe" does NOT
match "JPEG"/"jpg"). Fail-before is construction-level: the tests cannot compile
against the old CarveFlags (no disable field existed — the S-019 convention for
capability-absence findings). Full suite 631/631 Linux Release.
**Resolution notes:** the og's IAnalyserFilter registry-rebuild model
(FUN_180006b30 :4386-4434 — the removal loop destroys disabled analysers out of the
working containers) maps onto the core as: `CarveFlags::disabled_formats`
(comma/space/semicolon-separated extensions or display names, case-insensitive —
the og's [Filter] keys are name-form: HEIC, WEBM, 7Z...) + `carve_token_listed` /
`carve_format_listed` (cr_registry.cpp, the tokenizer) + per-layer skip maps built
once per scan (the grammar layer gates by trial slot BEFORE the first-byte fast
path; the raw-sign layer filters matches before the best-pick — the og's filter
removes the analyser so its signs never fire). The fix plan's step 2
(capability enumeration) is satisfied by the existing stable table order (one
entry per FormatId; registry()/registry_size() were already public in
cr_signatures.h) — no new ABI surface needed, N-033's cr_ui-export question stays
with N-033. Step 3 (consume a config file) is deliberately NOT wired: the og's
STL/VPJ default-off is HOST policy (the ini), not an engine constant — the core
ships all formats enabled and the host mirrors og defaults through the new flag;
registered as D-72 in PARITY-DELTAS. During verification this session's own first
implementation crashed on an OOB trial-slot array (kTrialCount=65 > a magic 64) —
caught by the new tests, fixed with a static_assert-bounded array (the defect
never landed on main).

---

## Addendum (dev-sign session, 2026-10-28 — CORRECTIONS #5 propagation)

The corpus errata (`easeus-drw-research/re-workspace/artifacts/CORRECTIONS.md`,
row 5) corrected the RawConfig.ini `[Filter]` toggle count: **34, not 37**
(recounted first-hand this session: 34 keys — HEIC, VCF, WEBM, WEBP, M1V, M2T,
M2TS, JP2, STL, M4P, XMIND, RVT, MOBI, MMAP, POS, EDDX, WFP, VPJ, OSP, HFP,
DRP, HIK, ODP, ODT, ODS, 7Z, INSP, INSV, GPX, CRM, AEP, FFF, DPX, MUSX).
The finding's mechanism is unaffected (the per-extension runtime filter
surface, the mask/callback rebuild, the STL/VPJ default-off observation all
stand); only the count was pre-correction RE output. The count references in
the living trail (CROSS-REF-MATRIX carve row, SUMMARY table, the s3 evidence
quote) carry the corrected figure with this addendum as the provenance. The
FIXED status is unchanged.
