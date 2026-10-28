# SEVERE S-040 — WMF placeable validator reads a wrong header layout and rejects every real placeable WMF

**Severity:** SEVERE
**Module / Surface:** carve (the RAW-signature layer — WMF analyser)
**Parameters affected:** B1 (detection correctness), A28 (validator logic), C19 (magic/grammar tables)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :29794-29840 (FileWMFAnalyser ctor `FUN_180026480` + validation `FUN_1800264d0`); the canonical placeable-WMF 22-byte header (Key/HWmf/BoundingBox/Inch/Reserved/Checksum) per the format's published layout
**Core references:** `src/core/carve/cr_analysers.cpp:3379-3411` (validate_wmf_placeable), `src/core/carve/cr_registry.cpp` (FMT_WMF_PL row); `tests/unit/test_carve.cpp:2257-2274` (plant_wmf_placeable) — audited commit `35adcb9`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered (the validator landed in the S-007 batch-1 fix `e018bf4`, whose commit message claims "WMF placeable (D7 CD C6 9A, XOR-checksum VERIFIED - the stored word is the XOR of words 0..8)" — the claim mis-states the og; no prior finding covers it)

## Summary

Our placeable-WMF validator reads the 22-byte placeable header with the wrong
field offsets: it treats bytes +16..17 as `Inch` (that is the first half of the
4-byte **Reserved** field, which canonical files carry as zero) and bytes
+18..19 as the checksum (the canonical checksum sits at **+20**, and the
canonical check XORs **10** words, not 9). Because every real placeable WMF has
Reserved == 0, our `inch == 0` gate rejects every real file at read time. The
unit-test plant was written with the same wrong convention, so the suite
verifies our bug rather than the format. The og additionally gates on HWmf == 0
and the following standard header's parameter count, and computes the extent
from the standard header's size field — none of which we replicate.

## What the reference does

`FUN_1800264d0` (ghidra :29805-29840, `param_2` is a `ushort*` so `param_2[N]`
= byte offset `2N`) accepts a placeable WMF when ALL of:

1. u32@+0 == 0x9AC6CDD7 (key `D7 CD C6 9A`);
2. u16@+4 == 0 (HWmf);
3. u16@+20 == XOR(u16 words 0..9) — the checksum over bytes 0..19;
4. u32@+16 == 0 (the 4-byte Reserved field);
5. u32@+22 == 0x0090001 (a standard WMF header `01 00 09 00` follows);
6. u16@+38 == 0 (that standard header's numOfParameters);
and then stamps extent = u32@+28 * 2 + 22 (the standard header's size-in-words
doubled, plus the placeable header length) and format id 0x46 (70).

The og performs **no `Inch` validation at all** — there is no 14400 bound and
no inch-nonzero check anywhere in the function. The standard-header path of the
same function accepts `01 00 09 00` + u16@+16 == 0 with extent = u32@+6 * 2
and performs **no version, object-count, or max-record checks** (all of which
our `validate_wmf_std` adds). Full verbatim quotes and the offset decoding are
in `diff-to-fix/evidence/decompiled-quotes/s9-wmf-placeable-gate.md`.

## What our implementation does

`validate_wmf_placeable` (`cr_analysers.cpp:3383-3411`) reads only 22+4 bytes
and checks:

- the key (correct);
- `inch = u16@+16`, rejecting `inch == 0 || inch > 14400` — **the invented
  gate that rejects every canonical file** (bytes +16..17 are Reserved, always
  0; the real Inch lives at +14);
- a checksum that XORs words 0..8 (bytes 0..17) and compares against +18 —
  **wrong position and wrong width** (canonical: words 0..9 vs +20);
- the standard header at +22 (correct);
- extent = a 64 MiB bounded probe with confidence 750 — **not** the og's
  `size@+28 * 2 + 22` self-describing extent.

It never checks HWmf (+4) or the standard header's numOfParameters (+38).
`validate_wmf_std` (`:3417-3445`) adds version ∈ {0x0100, 0x0300}, objects
≤ 256, maxrec ≤ words gates that the og does not have — a stricter-than-og
subset that can reject og-accepted files (e.g. version 0x0200 streams or
files with more than 256 objects).

The test plant (`test_carve.cpp:2257-2274`) writes `put16(at+16, 1440)` ("inch")
and the XOR-of-10-words value at +18 — i.e. it manufactures the exact
non-canonical byte sequence our validator expects. The og would reject the
plant (Reserved != 0; checksum@+20 = 0). The suite therefore cannot detect the
defect, and the bad-checksum rejection test (`:2679`, `:2747`) only proves our
own wrong checksum rule fires.

## Gap

As a falsifiable claim: for every canonical placeable WMF file (Reserved@+16
== 0, checksum@+20, Inch@+14), `validate_wmf_placeable` returns not-ok at the
`inch == 0` gate. Formally: `u16_from_bytes_le(h+16) == 0` holds for all
spec-conforming files, and the validator rejects precisely when that value is
0 — so the acceptance set of our validator and the set of real placeable WMFs
are disjoint.

## Impact

- **Correctness / user-observable:** placeable WMFs (`.wmf` files with the
  22-byte placeable header — the common form produced by graphics exporters
  and Office clip-art libraries) are silently missing from RAW carve results.
  A user recovering a disk with WMF assets gets no WMF files back where the
  reference product returns them. Files are still discoverable through the
  standard-WMF row only when the placeable header is absent.
- **Blast radius:** one format family (WMF placeable), every scan, every
  filesystem's free-space carve.
- **Test blind spot:** the plant mirrors the bug, so no current or future test
  can catch a regression or verify a fix unless the plant is rewritten to the
  canonical layout.

## Reproduction / verification

1. Corpus: read `RAWSearchDLL_decompiled.c` :29805-29840 and decode the
   `ushort*` offsets (done; quoted verbatim in
   `evidence/decompiled-quotes/s9-wmf-placeable-gate.md`).
2. Product: read `cr_analysers.cpp:3383-3411` (the +16/+18/9-word reads) and
   `test_carve.cpp:2257-2274` (the plant's `put16(at+16, 1440)` +
   `put16(at+18, x)`).
3. Logical proof: canonical placeable header ⇒ Reserved(+16..19) == 0 ⇒ our
   `inch` reads 0 ⇒ reject at `cr_analysers.cpp:3391`.
4. (Optional executed check for the fixing session: carve a real-world
   placeable `.wmf` plant written to the canonical layout — 22-byte header
   with Inch@+14, Reserved=0, checksum@+20 = XOR(words 0..9), std header
   +22, size@+28 — our engine will not emit a WMF_PL row; the og's gate
   accepts it by construction.)

## Fix plan (for the engineering team)

1. No architectural change — validator-local fix.
2. Files to modify: `src/core/carve/cr_analysers.cpp`
   (`validate_wmf_placeable`, optionally `validate_wmf_std`),
   `tests/unit/test_carve.cpp` (`plant_wmf_placeable`, the bad-checksum
   plant at :2679, and the rejection expectation at :2747).
3. The exact change:
   - Replace the `inch` gate with the og's gates in the og's order:
     a. `u16@+4 == 0` (HWmf);
     b. checksum: `u16@+20 == XOR(words 0..9)` (bytes 0..19);
     c. `u32@+16 == 0` (Reserved);
     d. std header at +22 (`01 00 09 00`);
     e. `u16@+38 == 0` (std numOfParameters).
   - Extent: `hit + u32@+28 * 2 + 22` (bounded by `probe_limit`), replacing
     the 64 MiB bounded probe; keep confidence in the 700-750 band.
   - Remove the invented 14400/Inch check entirely (the og has none).
   - For `validate_wmf_std`: decide scope — either match the og exactly
     (drop version/objects/maxrec gates, keep `u16@+16 == 0`, extent =
     `u32@+6 * 2`) or keep the stricter gates as a documented superset; the
     og-exact form is the parity target, the stricter form needs a
     PARITY-DELTAS row if kept.
4. Rewrite `plant_wmf_placeable` to the canonical layout: Key, HWmf=0,
   BoundingBox, **Inch=1440 at +14**, **Reserved=0 at +16..19**,
   **checksum at +20 = XOR(words 0..9)**, std header at +22 (18 bytes,
   numOfParams@+16-of-std = 0, size@+28). Keep a second, adversarial plant
   for each rejected field (bad checksum at +20; nonzero Reserved; nonzero
   HWmf; nonzero std params).
5. Tests to add (below) must fail before the fix and pass after.
6. Contract/doc updates: none (no ABI change); note the correction in the
   S-007 FIXED file's addendum (its "words 0..8" claim) so the trail stays
   truthful.

## Tests to add

- `S040WmfPlaceableCanonicalPlant` — a canonical-layout plant carves as
  FMT_WMF_PL with extent = size@+28*2 + 22 (fails before: no row emitted).
- `S040WmfPlaceableBadChecksum` — checksum@+20 corrupted -> rejected.
- `S040WmfPlaceableNonzeroReserved` — Reserved@+16 != 0 -> rejected.
- `S040WmfPlaceableNonzeroHwmf` — HWmf@+4 != 0 -> rejected.
- `S040WmfPlaceableStdParamsNonzero` — std numOfParams@+38 != 0 -> rejected.
- `S040WmfStdOgExactGates` (if the og-exact std form is chosen) — version
  0x0200 / objects 300 / maxrec > words accepted as the og does.
- Fuzz: mutate every field of the canonical plant (byte-wise) and assert
  no crash + rejection whenever a gated field changes.

## Related findings

- S-042 (batch-2 formats absent) — same surface, different rows; the WMF
  row exists but is defective.
- N-092 (MID/GZ cap drift) — the same batch-1 landing's cap discipline.
- S-007 (FIXED) — the landing that introduced the validator; this finding
  corrects its WMF claims.

## Open questions

None. The og's gate is fully decoded at the source level; the canonical
layout is unambiguous.
