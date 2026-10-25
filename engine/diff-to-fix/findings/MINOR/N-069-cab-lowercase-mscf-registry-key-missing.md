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
