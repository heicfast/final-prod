# MINOR N-054 — RAF/JPEG magic strictness deltas (both directions): og keys RAF on 4-byte "FUJI" (core requires 16-byte "FUJIFILMCCD-RAW "); og keys JPEG on 4 exact dwords (core accepts any FF D8 FF)

**Severity:** MINOR
**Module:** carve
**Parameters affected:** C19 (magic tables), B1 (detection — RAF variants missed), B3 (false positives — JPEG superset)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:55` ("`FUJI` | 0x494a5546 | Fujifilm RAF | camera-RAW chain"); `raw-dual-registry--u16-magic-keys.md:14` (u32 4CC key 0x494A5546 "FUJI"); JPEG 4-dword family per census:54 (`FF D8 FF E0/E1/DB/EE` → 4 exact keys)
**Core references:** `src/core/carve/cr_registry.cpp:185-195` (FMT_RAF magic {'F','U','J','I','F','I','L','M','C','C','D','-','R','A','W',' '} 16 bytes), `:48` (FMT_JPG magic {0xFF,0xD8,0xFF} 3 bytes — prefix match) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two magic-strictness deltas run in opposite directions: (a) the og accepts any
`FUJI`-prefixed RAF candidate (4-byte key; the camera-RAW chain validates further), while
the core requires the full 16-byte `FUJIFILMCCD-RAW ` string — RAF variants whose header
differs after byte 4 are missed; (b) the og keys JPEG on four exact dwords (FF D8 FF
E0/E1/DB/EE), while the core accepts any `FF D8 FF` prefix — a small superset (FF D8 FF xx
with xx outside the og's four) that the og's registry would not dispatch.

## What the reference does

Registry census rows quoted above: RAF = u32 key 0x494a5546 ("FUJI"); JPEG = 4 exact dword
keys. The camera-RAW chain (RAF) then validates with the format grammar.

## What our core does

FMT_RAF requires the full 16-byte string; FMT_JPG matches the 3-byte prefix.

## Gap

Falsifiable: (a) an RAF file whose bytes 4..15 differ from `FILMCCD-RAW ` (variant/older
RAF writers) is carved by the og and missed by the core; (b) a file starting FF D8 FF 99 is
carved by the core and not dispatched by the og's registry.

## Impact

RAF recall loss on header variants (small population); JPEG superset on the 4th byte (tiny
false-positive addition, mostly benign since validate_jpeg gates further). MINOR.

## Reproduction / verification

Census rows + registry rows read first-hand this session.

## Fix plan

1. FMT_RAF: key on the 4-byte `FUJI` prefix and move the 16-byte string into
   `validate_raf` as the primary (but not sole) acceptance path — accept og-style when the
   grammar validates even if bytes 4..15 differ.
2. FMT_JPG: keep the 3-byte prefix (the superset is a registered deliberate detection
   broadening — I-009 family); document the og's 4-dword set beside it, or tighten to match
   if strict parity is preferred (recommend: keep superset, register).

## Tests to add

- `Carve.RafVariantHeaderAccepted` (FUJI + non-standard tail, grammar-valid)
- `Carve.JpegFourthByteOutsideOgSet` (documents the superset)

## Related findings

- S-007 (magic coverage), I-009 (probing supersets register)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's carve-magic-quartet batch
**Verified by:** `Carve.RafVariantHeaderAccepted` (FUJI + non-canonical bytes 4..15 + sane version/section table carves at confidence 700 with the variant note) and `Carve.JpegFourthByteOutsideOgSet` (FF D8 FF 99 + valid JFIF body carves - the documented superset). Fail-before: the tests do not exist on the pre-fix tree; pre-fix a variant RAF header was MISSED entirely (the 16-byte string was the sole path).
**Resolution notes:** both directions per the fix plan: (a) FMT_RAF's registry key shrinks to the og's 4-byte FUJI u32 (census :55, 0x494a5546); validate_raf keeps the 16-byte FUJIFILMCCD-RAW string as the PRIMARY path (confidence 850) and adds the variant path (version sane + section table sane; confidence 700, note "FUJI variant header"). (b) FMT_JPG keeps the 3-byte prefix - a registered deliberate detection superset (D-64, I-009 family): the og's four exact dwords (FF D8 FF E0/E1/DB/EE, census :54) are documented beside the row; validate_jpeg's grammar gates the false-positive surface. Do not tighten for parity.

### DFX12 convergence addendum (session 12)

Convergent with this leg's same-resolution implementation (FUJI 4CC key,
canonical-string primary + variant path; JPEG 3-byte prefix kept and
registered — their D-64a numbering is canonical). **This leg's unique
discovery stands on top of BOTH landings: NEW-FINDING N-076** — the
canonical version gate (`ver > 0x2000`, binary read) rejected every
real-world RAF because the version at +16 is a 4-byte ASCII string
("0100"/"0201"; LE values ~8e8); both the canonical landing and this
leg's first pass carried the defect (the row was dead for real files
since introduction — zero fixture coverage is how it survived). Fixed in
the union follow-up: ASCII-digit OR small-binary acceptance, pinned by
Carve.RafRealAsciiVersionAccepted (fail-before executed RED on the
pre-fix engine).
