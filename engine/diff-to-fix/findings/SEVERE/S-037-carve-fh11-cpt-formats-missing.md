# SEVERE S-037 — two reference carve formats escaped every format list: FreeHand 11 (CFH11Analyser, key 0x11C) and Corel PhotoPaint (CPT7/8/9, three generations)

**Severity:** SEVERE
**Module / Surface:** carve / raw-signature engine (modules/carve.md)
**Parameters affected:** A28 (validator presence per format), C19 (magic keys 0x11C / 0x37545043-family), C21 (registry ids 0x1d for FH11), B1-adjacent (detection coverage)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:1952-1961` — the registry static-initializer: key `0x11c` → `BruteForce::CFH11Analyser::vftable`, registry id `0x1d` (FH11 = FreeHand 11; 0x11c LE = bytes `1C 01 00 00`, FreeHand's document version marker) · `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:93` — `CPT7`/`CPT8`/`CPT9` = `0x37545043, 0x38545043, 0x39545043` → **Corel PhotoPaint, 3 generations**, `RawSpeed::CPTAnalyser` · `:105` CFH11 in the @BruteForce namespace · `:107` CPT in the @RawSpeed namespace · `:128` "Three-generation PhotoPaint support (CPT7/CPT8/CPT9) — rare format, fully covered" · `rawsearchdll--family-bruteforce.md:4` (FH11 named in the family description) `:24` (`BruteForce::CFH11Analyser` roster)
**Core references:** audited commit `dd82db9` — `rg -i "fh11|freehand" src/ include/` → only `src/core/tree/cr_grading.cpp` ("FH11" in the grading extension table) and `src/core/query/cr_filefilter_data.cpp` (`{"fh11", Category::Image}`) — the extension is classified and graded but **no analyser, no signature, no carve path exists**; `rg -i "corel|cpt7|cpt8|cpt9" src/ include/` → **zero hits** (CPT absent entirely); `rg "1[Cc][ ]*01[ ]*00[ ]*00|0x0001011[Cc]" src/core/carve/` → zero hits; `cr_rawsigns_data.cpp` carries neither format
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered — S-007 (FIXED, "carve missing common formats") enumerated its format additions from the registry census but never included FH11 or CPT; the s3 evidence's FH11 "(N-026)" marker never became a finding (repo N-026 is an unrelated ReFS finding); no other finding names either format (the session's corpus inventory: family-bruteforce.md and family-rawspeed.md were NEVER-CITED)

## Summary

The reference carves FreeHand 11 documents (BruteForce::CFH11Analyser, registry key 0x11C, id 0x1d) and Corel PhotoPaint images across three format generations (CPT7/CPT8/CPT9 → RawSpeed::CPTAnalyser). The core has neither: FH11 appears only in the grading and filter extension tables (the extension is recognized in results that can never be produced), and CPT is absent from the entire tree. Both are user-visible recall gaps in the deep-scan carve pass — the exact class of gap S-007 closed for other formats.

## What the reference does

The registry static-initializer binds key `0x11c` (bytes `1C 01 00 00`) to `BruteForce::CFH11Analyser` with registry id `0x1d` (decompile:1952-1961, quoted in the citations) — a BruteForce-family structural walker for FreeHand 11 documents (the family contract: walk forward to grammar break, per the carving deep-dive). PhotoPaint is covered three ways: keys `CPT7`/`CPT8`/`CPT9` (`0x37545043`, `0x38545043`, `0x39545043`) all bind into the RawSpeed namespace's CPTAnalyser (census:93), which the census highlights as "rare format, fully covered" (:128) — three generations of the format deliberately carried.

## What our implementation does

Nothing, for either format. The verified greps (above) show: no FH11 signature or validator anywhere in the carve path — the extension string exists only in the grading table (`cr_grading.cpp`) and the filter category table (`cr_filefilter_data.cpp`), meaning a hypothetical FH11 result would be graded and filtered, but no scan stage can ever produce one; CPT has zero occurrences in the entire source tree. Neither format is in the generated raw-signs table (`cr_rawsigns_data.cpp`) nor in the grammar registry (`cr_registry.cpp`).

## Gap

Falsifiable claim: the core's carve engine cannot detect or recover FreeHand 11 (key 0x11C) or Corel PhotoPaint (CPT7/8/9) content that the reference's engine detects and recovers; the formats are absent from every core format list including S-007's.

## Impact

- **Recall (B1)**: FH11 (.fh11 FreeHand documents) and CPT (.cpt PhotoPaint images) content on damaged/lost volumes is silently unrecoverable through carving in the core — the reference recovers both.
- **Inconsistent surface**: FH11's presence in the grading/filter tables advertises a capability the engine cannot deliver (a results-only extension).
- Bounded: both are niche formats (the census itself calls CPT "rare"), hence SEVERE (missing user-observable capability), not ACUTE.

## Reproduction / verification

Decompiled evidence (strength-2): the registry initializer lines quoted above (decompile:1952-1961 for FH11's key/id binding; census:93/:107/:128 for CPT's three keys). Core evidence (strength-3): the greps quoted in Core references — zero carve-path occurrences; the two data-table occurrences quoted. The og's raw_signs.json carries neither format either (the C++ classes own them — census:121's division of labor), so the core's generated-signature absence is consistent with the corpus's architecture: these formats need grammar validators, not table signatures.

## Fix plan (for the engineering team)

1. Add a `validate_fh11` grammar validator: entry key = the 4-byte LE value `0x0000011C` at the hit; then the FreeHand document structure walk (the format's block/record sequence) per the BruteForce family contract (walk to grammar break); registry id parity is display-side only — the core's registry does not carry og ids (note in the row).
2. Add a `validate_cpt` validator for the three PhotoPaint keys `CPT7`/`CPT8`/`CPT9` (bytes `37 50 54 43` / `38 50 54 43` / `39 50 54 43`), with the per-generation container walk (the og's RawSpeed::CPTAnalyser semantics — decode from the decompile if the walk depth is nontrivial, else signature+bounded-size first pass with a tracked-open for the walk).
3. Register both in `cr_registry.cpp` with min/max sizes gated by the format realities (FH11 documents are small-to-medium; CPT images vary) and confidences consistent with the family.
4. Add "cpt" to the grading + filter tables (fh11 is already there).
5. Tests: fixtures for each key (a minimal FH11 header `1C 01 00 00` + records; minimal CPT7/8/9 headers) — detection + extent; a negative test (the `1C 01 00 00` byte sequence inside random data without structure rejects).
6. Acceptance criteria: the CAPABILITY-MATRIX carve row lists both formats; the detection tests are fail-before red.

## Tests to add

- `CarveFh11DetectedAndWalked` / `CarveFh11RejectsBareKeyBytes`.
- `CarveCpt7Detected`, `CarveCpt8Detected`, `CarveCpt9Detected` / `CarveCptRejectsTruncatedHeader`.

## Related findings

- **S-007** (FIXED) — the format-coverage family; this finding closes the two entries its enumeration missed.
- **A-010** — the same never-cited family artifacts' walk-semantics gap (ASF).

## Open questions

The exact FreeHand 11 post-key walk structure is not decoded in the census artifacts (only the class/key/id binding is); the fix plan's step 1 says "per the format's record sequence" — if the engineering pass needs the og's exact walk, the decompile's CFH11Analyser::Analyse (vtable-resolved from `.rdata`) is the probe; filing this as a finding does not depend on that decode (the absence is proven either way).
