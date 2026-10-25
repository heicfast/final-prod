# MINOR N-069 — CAB carve: lowercase `mscf` registry key missing (og registers both cases)

**Severity:** MINOR
**Module:** carve
**Parameters affected:** B1 (detection correctness — case-variant magic), C19 (magic bytes/signatures)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:94` (magic-key table row: "`MSCF` / `mscf` | 0x4643534d, 0x6663736d | Microsoft CAB (**both cases** — case-insensitive formats)"), `:127` ("**Case-insensitive formats get duplicate keys** (`MSCF`/`mscf`) rather than case-folding — deliberate registry bloat for correctness")
**Core references:** `src/core/carve/cr_registry.cpp:212` (`{FMT_CAB, "cab", {'M','S','C','F'}, 4, …}` — uppercase only; grep `mscf` across carve: 0 hits)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's analyser registry registers BOTH `MSCF` (0x4643534D) and `mscf` (0x6663736D) as CAB magic keys — a deliberate duplicate-key policy for case-insensitive formats. The core's CAB row anchors on uppercase `MSCF` only, so a cabinet whose header spells the magic lowercase is never probed.

## What the reference does

The registry census (first-cited this session) documents the dual-key row and the policy: case-insensitive formats get duplicate registry keys rather than case-folding at match time — "deliberate registry bloat for correctness".

## What our core does

FMT_CAB's magic is the exact 4 bytes `MSCF` (cr_registry.cpp:212); the magic scan is byte-exact, so `mscf` never matches.

## Gap

Falsifiable claim: a file starting `6D 73 63 66` (lowercase mscf) followed by a valid cabinet structure is carved by the og and ignored by the core.

## Impact

Very narrow recall gap (lowercase-magic cabinets are rare — some packed/installer-adjacent writers emit them; the og considered them worth a duplicate key). No false-positive implications.

## Reproduction / verification

Corpus-derived (census artifact first-cited this session; quotes above). Core side verified in-source at the audited commit.

## Fix plan (for the engineering team)

1. Register a second CAB anchor row `FMT_CAB_LC` on `{'m','s','c','f'}` pointing at the same `validate_cab` (the og's duplicate-key policy — no case-folding in the scanner).
2. Alternatively add a documented case-insensitive flag to the registry row schema — but the og's own choice was duplicate keys; matching it keeps the registry model aligned.
3. Test: `Carve.CabLowercaseMagic` (lowercase mscf cabinet fixture → carved).
4. Acceptance: both cases carve; registry enumeration (N-033's ABI) reports both rows deterministically.

## Tests to add

- `Carve.CabLowercaseMagic` — fails today

## Related findings

N-033 (analyser-registry ABI — the addendum this session notes the registry landed; this row pair extends it); S-007 (carve coverage family).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-069 commit)
**Verified by:** `Carve.CabLowercaseMagic` — fail-before executed on the pre-fix tree (lowercase `mscf` CFHEADER at 12288 in a grammar-valid image: `lower_node == nullptr`, `carved != 2` — the file was never probed, exactly the finding's claim); pass-after with the dual-key row + the validator's two-spelling re-check (both cases carve; the lowercase node carries `carve_format == FMT_CAB_LC`). Full suite 568/568 green (linux-gcc, RelWithDebInfo + ASan); lint clean.
**Resolution notes:** implemented the fix plan's option 1 (the og's duplicate-key policy): `FMT_CAB_LC` appended to the FormatId enum (serialized carve_format values stay stable, per the table's appended-id convention), a second registry row `{'m','s','c','f'}` sharing `validate_cab`, and the trial-order slot after FMT_CAB. **The fix plan missed one site and the fail-before run caught it:** `validate_cab` re-validates the magic internally (`memcmp(h, "MSCF", 4)`) — with the original validator, the lowercase key's hits died inside the validator even after the row anchored them. The re-check now accepts exactly the two registered spellings (`MSCF`/`mscf` — mixed case never anchors, no registry row keys it, matching the og's exact-dual-key policy, census :94 + :127). The registry F8 gate + self-probe pass unchanged (one row per FormatId, order = FormatId, table end). N-033's enumeration surface reports the row pair automatically (registry_size grew). No false-positive surface: the grammar gates (counts, cbCabinet bounds) apply to both spellings identically.
