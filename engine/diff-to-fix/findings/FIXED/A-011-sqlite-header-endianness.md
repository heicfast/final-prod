# ACUTE A-011 — validate_sqlite reads the page size and the page count LITTLE-ENDIAN: every real SQLite database is rejected by the grammar layer (a 4096-page-size DB reads as 16, fails the < 512 gate) and surfaces only through the raw-sign layer with degraded extents — wrong recovery extents for the whole format family

**Severity:** ACUTE
**Module / Surface:** carve / the SQLite grammar validator (modules/carve.md)
**Parameters affected:** A28 (validator semantics), C19-adjacent, B1 (detection coverage)
**Corpus references:** `ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c:45126-45166` (`function_18002e2b0` — the og's CFCPEvt Analyse entry, missing from the Ghidra export): the full 15-byte `"SQLite format 3"` memcmp, then the page-size SSE compose `v10 = v8 | (256 * v5 & 0xff00)` — a **BIG-ENDIAN compose** (the first header byte × 256 as the high byte) — and the encoding gate `v7 >> 24 < 4` (the BE u32 at header+56's high byte, i.e. the text-encoding field, valid 1..3); the SQLite file-format spec: **every multi-byte field in the 100-byte header is big-endian** — the page size at offset 16 is a 2-byte BE integer, the page count at offset 28 is a 4-byte BE integer, the text encoding at offset 56 is a 4-byte BE integer
**Core references:** `src/core/carve/cr_analysers.cpp:1163-1188` `validate_sqlite` — `rd16(s, hit + 16)` (the LE page-size read) and `rd32(s, hit + 28)` (the LE page-count read); NO grammar-layer SQLite test exists (both test fixtures — test_carve.cpp:964 and :3457 — are raw-SIGN layer tests; the grammar validator's fields were never exercised against real-format bytes, which is why the wrong endianness survived every suite)
**Status:** filed and fixed in the same landing (this session's A-011 commit)
**Prior audit check:** not in prior scope — the S-007/S-037 format waves enumerated registry presence; the SQLite validator's FIELD semantics were never byte-checked against the format spec or the og's compose

## Summary

The SQLite file format's header fields are big-endian; the core's validator
reads the page size and the page count with the LE helpers. A real database
with 4096-byte pages stores `10 00` at offset 16; the LE read yields 16 —
which fails the `page_size < 512` gate — so **every real SQLite database is
rejected by the grammar layer** and surfaces only through the raw-sign layer
(header/tail sign bounds, not the exact page math) — when it surfaces at all.
Every real carved SQLite file therefore carries a wrong extent (the sign
layer's bounded sizing instead of pages × page_size), which is incorrect
recovery output for the whole family.

## What the reference does

The og's Analyse (RetDec `function_18002e2b0`): the full 15-byte magic memcmp,
then the page-size extraction as an explicit big-endian compose
(`v8 | (256 * v5 & 0xff00)`), the nonzero gate, and the encoding gate
(`v7 >> 24 < 4` — the BE text-encoding field, valid 1..3) → the generic id
0x73. The deep walk then does the page-size/page-count consistency math and
the 200 MB ceiling.

## What our implementation does (at the audit base)

Reads the two fields LE, applies the (otherwise correct) sanity gates, and
rejects every real database on the first gate; no grammar-layer test exists,
and the two raw-sign-layer tests never exercise the validator's fields.

## Gap

Falsifiable claim: a byte-exact SQLite image (magic, BE page size 4096 at 16,
BE page count at 28, valid encoding at 56, content sized pages × page_size)
is NOT carved by the grammar layer at the audit base (the LE page-size read
= 16 < 512 → reject), while the og carves it with the exact extent.

## Impact

- Every real SQLite database (the FCP event-database family included — N-089's
  surface) gets grammar-rejected and sign-layer-sized: wrong extents, the
  50-MiB-class sign bounds instead of the true size, potential truncation on
  large DBs.
- The registry row's 850-confidence exact-size path is dead code on real
  files.

## Reproduction / verification

Strength-1: the format spec (BE fields) + the og's BE compose (RetDec
:45142-45156) vs the core's LE reads (code-cited); the fail-before test
below proves the rejection on a byte-exact real-format header.

## Fix plan

1. Read both fields big-endian: `rdbe16(s, hit + 16)` and
   `rdbe32(s, hit + 28)` (the helpers already exist and are used by the
   TIFF/JPEG validators).
2. Gate the text encoding (the og's `v7 >> 24 < 4`): read the BE u32 at
   +56; values > 3 reject (0 is invalid per the spec; 1/2/3 = UTF-8/16LE/16BE).
3. Tests with a byte-exact real-format fixture (`plant_sqlite`): the exact
   extent (`end = start + pages × page_size`) and the page-math note; a
   second fixture with an invalid encoding rejects; a third with the LE-only
   shape (the old wrongness) rejects (guards the endianness regression).
4. Fail-before: the exact-extent test is RED at the audit base.

## Tests to add

- `Carve.SqliteRealHeaderByteOrder` (fails before: the LE read rejects the real header).
- `Carve.SqliteInvalidEncodingRejects`; `Carve.SqliteLittleEndianShapeRejects` (the regression guard).

## Related findings

- **N-089** (this session) — the FCP subtype relabel rides this validator (its marker scan lands with this fix).
- **S-007/S-037** (FIXED) — the format-presence waves that never byte-checked this validator's fields.

## Open questions

None on the endianness (spec + og compose agree); the og's exact page-size
gate nuances (the `% 1024` term in the RetDec rendering) are a superset
question, not a correctness one.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's A-011+N-089 commit)
**Verified by:** `CarveEngine.SqliteRealHeaderByteOrder` (a byte-exact real-format header — BE page size 4096, BE page count, valid encoding — carves with the grammar-layer format id and the EXACT pages × page_size extent; fail-before RED: no grammar carve at the audit base), `CarveEngine.SqliteInvalidEncodingRejects` + `CarveEngine.SqliteLittleEndianShapeRejects` (the guards: an invalid encoding and the LE shape never reach the grammar layer — the raw-sign layer's sign hit is legitimate and asserted as the sign-tagged carve). **Fail-before EXECUTED RED** on the stash-reverted build (3 of 5 tests). Full suite green.
**Resolution notes:** Both fields read big-endian (`rdbe16`/`rdbe32`); the og's encoding gate added (the BE u32 at +56, valid 1..3 — FUN_18002e2b0's `v7 >> 24 < 4`); **a second latent bug fixed in the same never-exercised path**: the pre-fix `pages > 2ull << 30 / page_size` parsed as `2ull << (30 / page_size)` — for 4096-byte pages the limit was `2ull << 0` = 2, rejecting every real multi-page database (the operator-precedence bug hid behind the endianness bug; both were dead-path-hidden because NO grammar-layer SQLite test existed — both prior fixtures were raw-sign-layer tests). The N-089 marker scan rides the same validator (see below).

## Convergence record (Agent 2 scan-engine lane — the same two defects found independently in the same window)

The Agent 2 lane (this session's SCAN-2e work) discovered BOTH defects
independently while landing N-089's tests: its first spec-correct BE
fixtures went RED, and the debug trail isolated the LE reads and then
the `2ull << 30 / page_size` precedence parse (`2ull << (30/page_size)`
= 2 — the 2-page cap). The lane's draft fix (rdbe16/rdbe32 + the
parenthesized gate + three fixtures) was superseded by this landing
(the A-011 decode's extra og gates: the +56 text-encoding field, the
RetDec-export Analyse entry, the encoding-aware marker forms, the
4-page scan). Complementary evidence this lane's decode adds to the
record:

- The Ghidra-side walk cite: `FUN_18002dfc1` (RAWSearchDLL_decompiled.c
  :35404-35432, the Ghidra export of the same analyser family) — the
  converted-name gate reads the schema name as a wide string, converts
  it (`WideCharToMultiByte`, converted length gated < 0x65), and applies
  the exact-length-0x11 check before the memcmp — the og's name-side
  mechanics behind the RetDec-export read position.
- A second-stage fail-before proof: with the endianness/precedence fixes
  held and ONLY the subtype stamp neutralized, exactly the FCP-label
  test stays RED while the sizing/generic tests stay green — the
  isolation proof that the relabel is label-only (detection and sizing
  byte-identical with/without the marker), the property N-089's
  "labeling-only" severity rests on.

Both lanes' fixtures agree on the decisive fact: pre-fix, the grammar
layer's SQLite surface was dead code on every real database — no
spec-correct fixture existed anywhere in the tree before this window.
