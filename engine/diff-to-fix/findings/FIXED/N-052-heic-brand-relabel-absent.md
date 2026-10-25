# MINOR N-052 — HEIC analyser does not relabel HEIF-family brands to the "HEIF" display label

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

- N-051 (same artifact's decode family)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's carve-magic-quartet batch
**Verified by:** `Carve.HeifSiblingBrandLabeledHeif` - a heic-brand ftyp carves as FMT_HEIC (label "HEIC"), a mif1-brand ftyp carves as the NEW FMT_HEIF row (ext .heif, registry label "HEIF") - the og's brand-slot rewrite (analyser-granular-batch1 :67-80, re-read first-hand). Fail-before: the test does not exist on the pre-fix tree; pre-fix every family brand took FMT_HEIC/"HEIC".
**Resolution notes:** FMT_HEIF appended to the FormatId enum + registry table (append-only: serialized carve_format values and the ABI's format list stay stable - cr_ui_carve_format_info enumerates the registry, so the new row surfaces to consumers automatically). kTrialOrder places FMT_HEIF after FMT_HEIC: at an ftyp position, heic-proper validates on FMT_HEIC (which now shrinks to the heic brand only); the siblings (heix/mif1/msf1) fall through to FMT_HEIF. validate_heic/validate_heif carry the og's distinction in their notes.

### DFX12 convergence addendum (session 12)

The canonical landing (b2f48c6) relabels via a new FMT_HEIF registry row;
this leg had implemented the alternative carrier (a per-file
CarveResult::ext_override keeping ONE type-id for the family, per this
finding's fix-plan note "Keep type-id/carve_format unchanged (single
format family both sides)"). The first-landing registry-row approach is
canonical; the rejected one-type-id design is preserved in git history
(05307aa^ tree) should a future audit revisit the 0x7C single-family
question. Both legs' tests are name-identical
(Carve.HeifSiblingBrandLabeledHeif); this leg's fail-before run (RED on
the stash-isolated pre-fix engine) is the second proof set.
