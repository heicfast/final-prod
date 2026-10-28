# MINOR N-152 — The DXF "999" comment-header entry key absent: the og's u16 key 0x3939 → CDXFAnalyser and the probe's leading-comment acceptance; the core keys "  0\r" only

**Severity:** MINOR
**Module:** carve (RAWSearchDLL parity surface)
**Parameters affected:** P1 (detection correctness — one real-world DXF header form missed), P19 (magic bytes — the second entry key), P28 (carving validator logic — the leading-comment tolerance)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :2722-2733 (the u16-layer registry binding: key 0x3939 → a 0x18-byte `BruteForce::CDXFAnalyser` node, idx 0x19); :42240-42290 (FUN_1800376f0, the DXF probe: the main 16-byte pattern `"  0\r\nSECTION  \r\n"` → id 0x19; the variant branch `(u16)*p == 0x3939 && p[2] == '9'` accepting `"999\n"` or `"999\r\n"` then matching the shifted pattern); `re-workspace/artifacts/SearchDLL/raw-dual-registry--u16-magic-keys.md` §1 (the u16 key table); `diff-to-fix/evidence/decompiled-quotes/s3-rawsearch-inventory.md` (the "99 | DXF variant | S-007 DXF" row — marked MISSING at session 3, never separately filed because the S-007 DXF landing was read as the whole fix)
**Core references:** audited `c7fa527c`. `src/core/carve/cr_registry.cpp:928-931` (the single FMT_DXF row, magic `{' ',' ','0','\r'}`); `src/core/carve/cr_analysers.cpp:4296-4308` (`validate_dxf` — `memcmp(h, "  0\r\nSECTION", 8)` at offset 0, no leading-comment path, no second key)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og enters DXF carving through TWO keys: the u32 magic "  0\r" and the u16 magic "99" (0x3939) — the latter because real CAD-exported DXF files commonly begin with a `999` comment line ("999\r\n" / "999\n") before the SECTION header, a documented DXF convention (group code 999 = free-form comment). The og's probe explicitly accepts that leading form. The core registers only the "  0\r" key and its validator requires the SECTION header at offset 0 — every comment-led DXF file is silently missed.

## What the reference does

**Registry (:2722-2733):** after the u16-map insert machinery, a 0x18-byte node stamped `BruteForce::CDXFAnalyser::vftable`, idx `0x19` (25), is inserted under u16 key `0x3939` — i.e. any candidate whose first two bytes are "99" dispatches to the DXF analyser chain.

**Probe (FUN_1800376f0 :42240-42290):** the 16-byte window is checked against the canonical pattern `0x4345530a0d302020` + `0x20200a0d4e4f4954` = `"  0\r\nSEC" "TION  \r\n"` → stamps id 0x19. The variant branch: if the first u16 is 0x3939 and byte[2] is '9' (the "999" comment header), then byte[3] must be '\n' (a 0xE-byte shifted comparison) or bytes[3..4] must be '\r''\n' (0x10 bytes) — after the comment line, the SECTION pattern is matched at the shifted offset with LF-only line endings tolerated (`local_40 = 0x0a302020` = `"  0\n"`).

## What our core does

One key, one form: `FMT_DXF` is registered solely with `{' ',' ','0','\r'}` (cr_registry.cpp:928) and `validate_dxf` (cr_analysers.cpp:4296) rejects anything that does not begin exactly `"  0\r\nSECTION"`. A file starting `"999\r\n  0\r\nSECTION..."` never reaches the validator (no key) and would fail its first memcmp even if it did.

## Gap

Falsifiable: plant `"999\r\n  0\r\nSECTION\r\n  2\r\nHEADER\r\n..."` — the og carves it (u16 key 0x3939 → CDXFAnalyser → the probe's comment branch); the core produces no DXF row.

## Impact

Silent false negatives on one real-world DXF header form (comment-led exports — common from AutoCAD's own "999" metadata conventions and third-party CAD tools). The standard form (the overwhelming majority) works. This is the N-111-class residue: a variant entry key on an implemented format, discovered by the session-3 key-diff, folded under the S-007 DXF landing's umbrella assumption and never separately dispositioned.

## Reproduction / verification

Corpus-decompile-derived, first-hand this session: the u16 registry window read line-addressed (:2722-2733) and the probe's variant branch read at :42270-42284 (the 0x3939 check, the '9' byte check, the \n/\r\n acceptance, the shifted LF-tolerant pattern) — quoted in `evidence/decompiled-quotes/s41-registry-key-residue.md` §4. Core-side: the single-row registry grep + the validator body read (:4296-4308). UNVERIFIED-BY-EXECUTION (no Windows host this lane); decode evidence is the basis, per protocol tier 2.

## Fix plan (for the engineering team)

1. **Decode the shifted-pattern comparison fully** (FUN_1800376f0's `_Size = 0xe` / `0x10` memcmp windows at :42278-42290 — pin the exact accepted byte sequences after the comment line, including the LF-only variant).
2. **Register the second key**: add `FMT_DXF_999` (enum APPENDED) keyed `{'9','9'}` with the 2-byte magic, or extend the scanner's key derivation to route "99"-led candidates to validate_dxf.
3. **Extend `validate_dxf`**: accept the leading `"999\n"` / `"999\r\n"` comment (skip 4 or 5 bytes) before the SECTION check, with the LF-tolerant ending per the probe.
4. **Tests**: plant both comment-led fixtures (LF and CRLF) + the standard form + a hostile `"99x"` near-miss — RED before, GREEN after; the F8 gate's key-presence test extended.

## Tests to add

- `Carve.Dxf999CommentHeaderLfCarves`
- `Carve.Dxf999CommentHeaderCrLfCarves`
- `Carve.Dxf999RejectsNonCommentContinuation` (the "99x" near-miss)

## Related findings

**M-112** (this session's sibling from the same key-diff sweep — the DBX triple) · **N-111** (the JP2 variant-key precedent class, FIXED) · **S-007/M-107** (the DXF landing's umbrella and its stand-in residue records)

## Open questions

None — the two remaining decode details are pinned in the fix plan step 1 (the exact shifted-pattern windows), executable by the fixer in one decompile read.
