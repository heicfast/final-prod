# SEVERE S-040 — WMF placeable carve validator contradicts the og on every gate (disjoint accept sets)

**Severity:** SEVERE
**Module / Surface:** Carve / raw-signature surfaces (RAWSearchDLL FileWMFAnalyser placeable branch)
**Parameters affected:** A28 (carving validator logic), C19 (magic bytes — key MATCHes; validator grammar diverges), B3 (false-positive/false-negative rate)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:29820-29845` (the placeable branch, first-hand re-read this session); `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c:38495-38515` (the same branch, retdec rendering, first-hand re-read); key registration `retdec_decompiled.c:5960` (`v2 = -0x65393229` = 0x9AC6CDD7, node idx 0x46)
**Core references:** `src/core/carve/cr_analysers.cpp:3379-3408` (`validate_wmf_placeable`, audited at core `35adcb9`); registry row `src/core/carve/cr_registry.cpp:826-835`
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered — and it **refutes a load-bearing claim inside S-007's niche batch-1 addendum** (`findings/FIXED/S-007-carve-missing-common-formats.md:222-223`: "WMF(placeable — the stored checksum is the XOR of words 0..8, the +18 word included in neither side)" — verified FALSE against both decompilers; a correction addendum lands on S-007 in the same wave)

## Summary

The core's placeable-WMF carve validator and the og's decompiled `FileWMFAnalyser` placeable branch implement **grammars that cannot both accept the same file**. The og reads the stored checksum at byte offset **+20** as the XOR of header **words 0..9** and **requires the dword at +16 to be zero**; the core reads the stored checksum at **+18** as the XOR of **words 0..8** and **requires the u16 at +16 (inch) to be 1..14400 (nonzero)**. The +16 gate alone makes the accept sets disjoint: every file the og accepts fails the core's inch gate, and every file the core accepts fails the og's zero-dword gate. The registry key (D7 CD C6 9A) and the standard-header magic check (bytes `01 00 09 00` at +22) match; everything between them does not.

## What the reference does

The og's placeable branch (both decompilers read first-hand this session; ghidra quoted verbatim):

```c
/* ghidra RAWSearchDLL_decompiled.c:29831-29843 — param_2 is ushort* over the hit buffer */
else if ((*(int *)param_2 == -0x65393229) && (param_2[2] == 0)) {
    if (((param_2[10] ==
          (ushort)(param_2[9] ^ param_2[7] ^ param_2[6] ^ param_2[5] ^ param_2[4] ^ param_2[3] ^
                   param_2[1] ^ *param_2 ^ param_2[2] ^ param_2[8])) &&
        (((param_2[2] == 0 && (*(int *)(param_2 + 8) == 0)) && (*(int *)(param_2 + 0xb) == 0x90001))
        )) && (param_2[0x13] == 0)) {
      *(ulonglong *)(param_3 + 0x10) = (ulonglong)(*(int *)(param_2 + 0xe) * 2 + 0x16);
      *(undefined4 *)(param_3 + 0x18) = 0x46;
```

retdec rendering (`retdec_decompiled.c:38495-38515`) agrees term-for-term: `v13 = *(int16_t *)(a2 + 20)` is compared against the XOR of the words at byte offsets 0,2,4,6,8,10,12,14,16,18 (words 0..9); `*(int32_t *)(a2 + 16) == 0`; `*(int32_t *)(a2 + 22) == 0x90001`; `*(int16_t *)(a2 + 38) == 0`; result extent `2 * *(int32_t *)(a2 + 28) + 22`.

Consolidated og gates (offsets are byte offsets into the hit):
1. dword@+0 == 0x9AC6CDD7 (the placeable key; registered at `retdec_decompiled.c:5960`, node idx 0x46)
2. word@+4 == 0 (the hmf slot)
3. dword@+16 == 0 (the words at +16 **and** +18 must both be zero)
4. word@+20 == XOR(words 0..9) — the stored checksum sits at **+20** and covers the ten words at offsets 0..18
5. dword@+22 == 0x00090001 (the standard WMF header's {Type=1, HeaderSize=9} pair immediately follows the 22-byte placeable header)
6. word@+38 == 0 (the standard header's NumberOfParameters slot)
7. extent = `2 × dword@+28 + 22` — an **exact** byte size (dword@+28 is the standard header's Size-in-words field), type id 0x46 (70)

Note the og's own semantics: because gates 2 and 3 force words 2, 8, 9 to zero, the effective checksum is the key dword XOR the BBox words — and the accepted layout is a 22-byte placeable header with the checksum word at +20, not the 20-byte canonical Aldus layout.

## What our implementation does

`src/core/carve/cr_analysers.cpp:3379-3408` (audited `35adcb9`, quoted verbatim):

```cpp
/* WMF (placeable): D7 CD C6 9A key 0x9ac6cdd7 (:5961) -> the WMF node
 * idx 70. The 22-byte placeable header ends with the XOR checksum of
 * its first 10 words - verifying it is the strongest possible gate; a
 * standard-WMF header must follow at +22. */
CarveResult validate_wmf_placeable(IStorageDevice* s, u64 hit, u64 cap) {
    CarveResult r;
    u8 h[22];
    if (!rd(s, hit, h, sizeof(h)))
        return r;
    if (std::memcmp(h, "\xD7\xCD\xC6\x9A", 4) != 0)
        return r;
    const u16 inch = u16_from_bytes_le(h + 16);
    if (inch == 0 || inch > 14400)
        return r;
    /* the placeable checksum: the stored word at +18 is the XOR of the
     * first 9 words (computed with the checksum field zero) */
    u16 x = 0;
    for (int i = 0; i < 9; ++i)
        x = u16(x ^ u16_from_bytes_le(h + 2 * i));
    const u16 stored = u16_from_bytes_le(h + 18);
    if (x != stored)
        return r; /* placeable-header checksum */
    u8 sh[4];
    if (!rd(s, hit + 22, sh, 4) || sh[0] != 0x01 || sh[1] != 0x00 || sh[2] != 0x09 || sh[3] != 0x00)
        return r; /* the standard header follows at +22 */
    r.ok = true;
    r.end = std::min(hit + u64(64 << 20), probe_limit(s, hit, cap));
    r.confidence = 750;
    r.note = "wmf-placeable-checksum-verified";
    return r;
}
```

The core: (a) requires u16@+16 ∈ [1,14400]; (b) verifies stored@**+18** == XOR(words 0..**8**); (c) does not gate word@+4, word@+38, or dword@+16's high half; (d) uses a bounded 64 MiB extent instead of the og's exact `2×dword@+28 + 22`.

## Gap

Stated falsifiably: for every byte sequence B, `og_accepts(B)` and `core_accepts(B)` are mutually exclusive on the +16 gate — the og requires bytes 16..19 == 00 00 00 00 while the core requires bytes 16..17 != 00 00. Additionally the checksum read/write positions differ (+20 vs +18) and the XOR word sets differ (words 0..9 vs words 0..8), and the extent rule differs (exact `2×Size@+28+22` vs bounded 64 MiB). The two implementations will never agree on a placeable-WMF candidate.

## Impact

Carve recall and precision for the placeable-WMF family: the core will not carve any placeable WMF the og carves (false negatives — user-visible lost files of a format the reference product recovers), and it will carve files under a checksum/field convention the og rejects (false positives relative to the reference). This landed in S-007 niche batch 1 (`e018bf4`) with a commit-message and addendum claim of "XOR-checksum VERIFIED" — the verification was performed against a misread of the analyser; the shipped test (`tests/unit/test_carve.cpp` WMF placeable case) plants the core's own convention, so it passes while disagreeing with the reference. Downstream: any consumer comparing carve results against the og on WMF-bearing media sees divergence; the finding also blocks honest S-007 batch-1 closure for this format.

## Reproduction / verification

First-hand decompile re-read this session at both sites (quotes preserved in `diff-to-fix/evidence/decompiled-quotes/s8-wmf-placeable-validator.md`): ghidra `RAWSearchDLL_decompiled.c:29820-29845` and retdec `retdec_decompiled.c:38495-38515` — the two renderings agree term-for-term on every gate. Core source read at `35adcb9` (`cr_analysers.cpp:3379-3408`). The standard-WMF branch of the same og analyser (key `01 00 09 00`, size `2×dword@+6`, NumParams@+16==0) DOES match the core's `validate_wmf_std` — the divergence is confined to the placeable branch. The og's registry key and node id were re-verified at `retdec_decompiled.c:5960` (value −0x65393229 = 0x9AC6CDD7, idx 0x46).

## Fix plan (for the engineering team)

1. No architectural change — one validator function.
2. Modify `validate_wmf_placeable` in `src/core/carve/cr_analysers.cpp` (and only there; the registry row `cr_registry.cpp:826-835` already keys correctly).
3. Replace the grammar with the og's: read a 40-byte window; gate dword@+0 == 0x9AC6CDD7, word@+4 == 0, dword@+16 == 0, stored u16@+20 == XOR(u16 words at byte offsets 0,2,...,18), dword@+22 == 0x00090001, word@+38 == 0; extent = `hit + 2 × u32@+28 + 22` (exact, mirroring the og; keep `probe_limit` only as the slab-fit bound, as the ICO walk does). Keep confidence/note fields updated to name the og-exact convention.
4. Tests: (i) update the WMF placeable plant in `tests/unit/test_carve.cpp` to the og grammar (checksum at +20 over words 0..9, zeros at +16..19, hmf 0, std header at +22, NumParams 0, exact size dword consistent with the planted extent); (ii) add a negative test: a file matching the CORE's current convention (inch 1440@+16, checksum@+18 over words 0..8) must now be REJECTED; (iii) an extent test: a plant whose Size@+28 disagrees with the slab must be bounded/refused per the extent rule; (iv) fail-before proof: neutralize the checksum gate → the positive test goes RED.
5. Docs: correct the S-007 batch-1 addendum line (already landing as this session's correction addendum) and the function-header comment (which currently describes "the XOR checksum of its first 10 words" while the code XORs 9 — after the fix the comment becomes true).
6. Acceptance: byte-level fixture where `validate_wmf_placeable` and the og grammar agree on accept AND on the returned extent; the two conventions' plants cross-rejected; full carve suite green.

## Tests to add

- `S040WmfPlaceableOgGrammarAccepted` (og-convention plant carves, extent exact)
- `S040WmfPlaceableCoreConventionRejected` (the pre-fix convention is rejected)
- `S040WmfPlaceableChecksumAt20` (checksum-position regression pin)
- `S040WmfPlaceableZeroDwordGate` (dword@+16 nonzero → reject)

## Related findings

- **S-007** (the parent carve-coverage finding; its batch-1 addendum's WMF claim is refuted by this finding — correction addendum lands with this wave)
- **S-041** (the niche batch-2 remainder — same surface, next work item; the WMF fix rides the same validator file)
- **N-091** (the stale product-doc constants row — unrelated family, same wave)

## Open questions

None. Both decompilers agree; the core code is unambiguous; the contradiction is total.
