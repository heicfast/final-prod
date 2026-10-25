# MINOR N-053 — IIU magic: core binds it to RW2 ("Panasonic RAW"); the og binds it to the TIFF/EP chain (TiffAnalyzer)

**Severity:** MINOR
**Module:** carve
**Parameters affected:** C19 (magic tables — same key, different chain), B1 (detection — validator grammar differs), B10 (type labeling)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:53` ("`MM\\x00*` / `II*\\x00` / **`IIU\\x00`** | 0x2a004d4d, 0x2a4949, **0x554949** | TIFF (both byte orders + **TIFF/EP variant**) | **BruteForce::TiffAnalyzer**")
**Core references:** `src/core/carve/cr_registry.cpp:194-204` (FMT_RW2 magic {'I','I','U',0x00}, label "Panasonic RAW", validator validate_rw2) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The same 4-byte key `IIU\\x00` (u32 0x00554949) is bound by the og to the TIFF chain
(TiffAnalyzer, as the TIFF/EP variant — the same validator as II*/MM*) and by the core to
RW2 ("Panasonic RAW" with a dedicated validator). Real-world files exist for both readings
(Panasonic RW2 opens with IIU; TIFF/EP is the ISO variant family), so this is a genuine
cross-implementation labeling/grammar divergence on one key.

## What the reference does

Registry census row (quoted above): IIU\\x00 → BruteForce::TiffAnalyzer — the TIFF/EP
variant of the TIFF chain, validated by the TIFF grammar and labeled TIFF-family.

## What our core does

FMT_RW2 claims the key with validate_rw2 and the label "Panasonic RAW". TIFF-proper is
covered separately (FMT_TIFF II*\\x00, FMT_TIFF_BE MM*\\x00) — but an IIU-headed TIFF/EP
file goes down the RW2 path in the core and the TIFF path in the og.

## Gap

Falsifiable: an IIU-headed TIFF/EP file is validated by the TIFF grammar and labeled
TIFF/EP by the og; the core validates it with validate_rw2 and labels it "Panasonic RAW".
Accept/reject can differ (the grammars differ) and the label always differs.

## Impact

Type mislabeling on IIU files; possible accept/reject divergence where the two grammars
disagree on damaged files. Low volume; presentation-level mostly. MINOR.

## Reproduction / verification

Census row read first-hand; registry row read first-hand.

## Fix plan

1. Decide the binding policy: match the og (IIU → TIFF-chain validation + TIFF/EP label)
   and keep RW2 as a SECONDARY check after TIFF-EP fails (the og's two-generation chain
   pattern — modern then legacy), or register both with the og's collision semantics
   (multimap equal_range chain: TIFF first, RW2 fallback).
2. Label the accepted result by which grammar validated.

## Tests to add

- `Carve.IiuTiffEpValidatedByTiffGrammar`
- `Carve.IiuRw2FallbackWhenTiffEpRejects`

## Related findings

- N-031 (dispatch chains — the og's collision model is the mechanism the fix should mirror)
- S-007 (format coverage context)

## Open questions

Whether the og's TiffAnalyzer accepts RW2-shaped IIU files (the TIFF grammar walking RW2
headers) is corpus-UNDECODED (the TIFF walk is one of the undecoded grammars — see
tracked-open); the fallback-chain fix is safe under either answer.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's carve-magic-quartet batch
**Verified by:** `Carve.IiuTiffEpValidatedByTiffGrammar` (a structurally-valid IIU file carves as .tif/FMT_TIFF_EP, never the RW2 row) and `Carve.IiuRw2FallbackWhenTiffEpRejects` (garbage IFD: TIFF/EP rejects, the IIU-salvage fallback fires at lower confidence). Fail-before: the tests do not exist on the pre-fix tree; pre-fix every IIU file took FMT_RW2/"Panasonic RAW" (.rw2) - the mislabel the finding describes.
**Resolution notes:** the og binds IIU\0 to the TIFF chain (census :53, BruteForce::TiffAnalyzer - re-read first-hand; the og has NO Panasonic row). Implemented per the fix plan's collision-semantics option: new FMT_TIFF_EP row (appended; magic IIU\0, validator validate_tiff - the TIFF/EP grammar, label "TIFF/EP", ext .tif) tried BEFORE FMT_RW2 in kTrialOrder; FMT_RW2's validator gained the post-reject IIU-salvage path (IIU + plausible IFD offset, confidence 600) - a REGISTERED SUPERSET (D-63): the og carves nothing when the TIFF grammar rejects; we salvage corrupt TIFF/EP headers. The note tells the truth on every path.

### DFX12 convergence addendum (session 12)

Convergent with this leg's same-resolution implementation (new
FMT_TIFF_EP row claiming the IIU key with the TIFF grammar ahead of an
FMT_RW2 salvage fallback, registered superset — their D-63a numbering is
canonical; this leg's duplicate D-64 register dropped in the union).
Both legs' test pairs are name-identical
(Carve.IiuTiffEpValidatedByTiffGrammar / Carve.IiuRw2FallbackWhenTiffEpRejects);
this leg executed both RED on the stash-isolated pre-fix engine as the
second fail-before proof set.
