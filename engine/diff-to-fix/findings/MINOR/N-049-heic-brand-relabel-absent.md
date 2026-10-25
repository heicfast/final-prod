# MINOR N-049 — HEIC analyser does not relabel HEIF-family brands to the "HEIF" display label

**Severity:** MINOR
**Module:** carve
**Parameters affected:** B10 (filename/label normalization), A28 (validator detail), D16 (type-model labeling)
**Corpus references:** `re-workspace/artifacts/SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md:67-80` (RawFormat_xjq::HEICAnalyser: "if brand == 0x68656963 ('heic'): accept {type_id = 0x7C}, label L'HEIC'; elif brand is HEIF-family: label L'HEIF' (**rewrites the brand slot to L'HEIF' for the display name**)"; Type-ID 0x7C)
**Core references:** `src/core/carve/cr_registry.cpp:71-80` (FMT_HEIC — fixed brand set {heic, heix, mif1, msf1}-family; single "HEIC" label), `src/core/carve/cr_analysers.cpp:487` (brand check) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's HEIC analyser distinguishes the `heic` brand (labeled L"HEIC") from the wider HEIF
family and rewrites the display label of sibling brands (mif1/heif etc.) to L"HEIF". The
core accepts the brand family but always labels "HEIC" — no relabel step exists.

## What the reference does

Quoted decode: brand 0x68656963 → type_id 0x7C, label L"HEIC"; HEIF-family brands → label
L"HEIF" (brand slot rewritten for display). HEICRawFileInfo carries dword@+0x18 = 0x7C.

## What our core does

FMT_HEIC validates the brand family and emits the fixed registry label "HEIC" for all of
them; no distinction between heic-proper and HEIF siblings.

## Gap

Falsifiable: a .mif1 HEIF file carved by both shows label "HEIF" in the og and "HEIC" in the
core.

## Impact

Type-labeling presentation only; recovery content identical. MINOR per the scale (naming/
labeling).

## Reproduction / verification

Corpus decode read first-hand; core registry row read first-hand.

## Fix plan

1. In the HEIC accept path, when the brand is not `heic`, emit the label "HEIF" (matching
   the og's display rewrite).
2. Keep type-id/carve_format unchanged (single format family both sides).

## Tests to add

- `Carve.HeifSiblingBrandLabeledHeif`

## Related findings

- N-048 (same artifact's decode family)

## Open questions

None.
