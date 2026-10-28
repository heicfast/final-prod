# MINOR N-149 — the carve filter's hard-wired always-on set: the og's per-search filter rebuild NEVER removes the OLE/RIFF/ZIP analysers (exempt mask 0x1000801 over id−0x30); core's disable surface can switch all three off

**Severity:** MINOR (reachable only through the per-format disable surface; the og's shipped [Filter] config never targets the three, so the divergence is latent — but a host disabling "zip"/"avi"/"doc" carves differently than the og would)
**Module:** carve (the per-scan filter semantics)
**Parameters affected:** D16-adjacent (the filter model's predicate semantics), A31-adjacent (scan lifecycle — the per-search registry rebuild)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:4408-4432` — the per-search filter rebuild `FUN_180006b30`'s removal loop, verbatim (lead re-verified at filing): `uVar4 = *(int *)(_Memory[2] + 8) - 0x30, uVar4 < 0x19 && ((0x1000801U >> (uVar4 & 0x1f) & 1) != 0)` — the loop SKIPS (never removes) entries whose id−0x30 hits a set bit of the mask 0x1000801; the mask's set bits are 0, 11, 24 → ids **0x30 (OLE), 0x3b (RIFF), 0x48 (ZIP)**; the analyser ids pinned by the registry builders (OLE `:2337`, RIFF `:2304-2310`, ZIP `:2243`); the filter source: `[Filter]` from RawConfig.ini (`:13133-13200`) applied via the IAnalyserFilter at the rebuild (`:4408-4432`)
**Core references:** audited `461825d8`-lineage — `src/core/carve/cr_registry.cpp:1231-1254` (`carve_token_listed` — the M-024 disable surface, no exempt set: any listed format id can be disabled), `src/core/carve/cr_carve.cpp:976-993` (the per-scan disabled_formats map); the M-024 landing's semantics (registry rows + module map `modules/carve.md`'s M-024 row)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (S11-lead push-time reconciliation):** filed as N-142 by this lane; renumbered **N-149** — the parallel S11B lane's N-142 (the dead CrPreview wiring) pushed first and keeps the ID per the SA-9 rule.

**Prior audit check:** newly discovered by the S11-lead Lane A infrastructure walk (verdict-table row 6); NOT covered by M-024 (FIXED — the disable surface itself, whose og-side semantics this refines), N-031 (the only_common gate — a different config key), D-97 (the RawConfig posture), or I-031 (the parallel infra register — its seven entries do not include the exempt mask).

## Summary

When the og rebuilds its working analyser set per search and applies the user/config filter, three formats are hard-wired exempt from removal: OLE compound documents, RIFF containers, and ZIP — the mask 0x1000801 keeps their ids in the set regardless of what the [Filter] section says. Core's per-format disable surface (the M-024 landing) has no exempt set: a host that disables "zip" (or "avi" via RIFF, or "doc" via OLE) switches those formats off entirely, diverging from the og's always-on guarantee.

## What the reference does

The rebuild's removal loop walks the working list and removes entries the filter
rejects — EXCEPT entries whose analyser id hits the exempt mask (the decompile
quoted above: the loop's continue-condition includes the mask test, so exempt ids
are never visited by the removal). The og's shipped RawConfig.ini [Filter] (32
keys enabled, STL/VPJ disabled — none of the three) never exercises the exemption,
but the semantics are structural: OLE/RIFF/ZIP survive any filter.

## What our core does

`carve_token_listed` + the per-scan `disabled_formats` map treat every format
identically; no id is exempt.

## Gap

Falsifiable: disable the zip format through the core surface (the disabled_formats
path) and scan a fixture with a valid ZIP → core carves nothing for it. Apply the
equivalent og filter (a [Filter] key targeting id 0x48) → the og still carves the
ZIP (the exempt mask keeps it).

## Impact

Host-driven carve-configuration divergence only (the CLI/ABI disable surface);
the og's shipped config never targets the three, so no default-configuration
divergence. But a "custom scan" feature (the og UI's per-format toggles map onto
the same filter) would produce different scan results for ZIP/AVI/DOC families.

## Reproduction / verification

Decompiled evidence (Lane A walk; the mask arithmetic re-derived by the lead at
filing — 0x1000801's set bits at offsets 0/11/24 over id−0x30 = 0x30/0x3b/0x48,
and the three ids cross-checked against the registry-builder cites). Core-side
code-read. Full record: `evidence/s11-lead-deep-lanes.md` §Lane-A row 6.

## Fix plan (for the engineering team)

1. Add the exempt set to the disable surface: in `carve_token_listed` (or the
   `disabled_formats` map's fill path), refuse to disable `FMT_OLE`,
   `FMT_RIFF`-family ids, and `FMT_ZIP` — return the formats' always-on semantics
   with a comment citing the mask decode (`RAWSearchDLL_decompiled.c:4412-4418`).
2. One test: disable "zip" through the surface, scan a ZIP fixture → still carved
   (RED today); the same for one RIFF and one OLE format.
3. Module-row note in `modules/carve.md`'s M-024 row (the og-side exemption).
4. Acceptance: the three formats cannot be disabled through any surface; the test
   pins it.

## Tests to add

- `Carve.FilterExemptSetAlwaysOn` (zip/riff/ole stay carveable under a disable
  list that names them).

## Related findings

M-024 (the disable surface landing this refines); N-031/I-023 (the only_common
gate — the other config key the same rebuild reads); D-97 (the RawConfig
registered posture); I-031 (the parallel infra register — its rebuild entry
documents the clear/rebuild, not the exemption).

## Open questions

None — the mask, the ids, and the three formats' identities are all cited.
