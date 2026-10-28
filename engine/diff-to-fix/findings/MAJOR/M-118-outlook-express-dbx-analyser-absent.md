# MAJOR M-118 — Outlook Express Dbx carve analyser entirely absent (the og's 3-key registry + GUID probe + u32@0x7C size); the session-3 "MISSING (N-026)" residue that was never filed

**Severity:** MAJOR
**Module:** carve (RAWSearchDLL parity surface)
**Parameters affected:** P1 (detection correctness — a whole recoverable format silently missed), P3 (false-negative rate), P19 (magic bytes: three registry keys absent), P25 (engine algorithm steps — the Dbx validation grammar), P28 (carving validator logic), P65 (test coverage — no Dbx test exists)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :2192-2232 (the three registry bindings — node constructions + map inserts), :17509-17562 (FUN_180016740, the Dbx format-id probe), :17586-17600 (FUN_180016880, the DBXRawFileInfo ctor), :17622-17690 (FUN_180016950, the size/name pass); `re-workspace/artifacts/SearchDLL/rawsearchdll--family-rawspeed.md` (the RawSpeed roster: DbxAnalyser, 4 vftable refs — top code weight alongside TAR); `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md` §3 (keys 0x36464d4a/0xfe12adcf/0x2c3489f7), §5 (the email-recovery marketing maps to SQLite+dBase+MDB+**Dbx**+PST); `og-codebase/raw_signs.json` (no dbx entry — the JSON layer does not cover it either; the analyser is the only path)
**Core references:** audited `c7fa527c`. `src/core/carve/cr_registry.cpp` (all 122 FMT_ rows byte-matched — zero Dbx key), `include/cr/carve/cr_signatures.h` (the FormatId enum — no DBX member), `src/core/carve/cr_analysers.cpp` (no validate_dbx), `src/core/carve/cr_rawsigns_data.cpp` (no dbx extension). The trail's own record of the gap: `diff-to-fix/evidence/decompiled-quotes/s3-rawsearch-inventory.md:252-254` — the three keys marked **"MISSING (N-026)"**, and `diff-to-fix/evidence/s3-agent-logs/S3-CARVE.md:70` — "**Unfiled planned IDs: N-026** (per-layer format tables — folded into the inventory)". The N-026 ID was later consumed by an unrelated refs-geometry finding (FIXED-LEDGER :206), so the planned carve filing never happened and no successor finding covers it (S-041's umbrella retirement lists its children: M-092/M-093/M-096/M-098/M-099/N-111/N-112/N-113/N-115 — no Dbx).
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** M-117 → **M-118** (session-42, 2026-10-29). The S41 lane filed this finding as M-117 at `9cc6a03e` (19:20:03Z) believing the ID free; the earlier push `0f5d57fb` (19:17:34Z) had just renumbered the suppress_overlap quadratic-walk finding M-116 → M-117 (SA-9 earlier-push-wins) and retired it with its ledger row. Two active-file M-117s therefore coexisted — the retired suppress_overlap row and this file. Per the SA-9 rule the retired row keeps M-117; this finding renumbers to M-118 (verified free: zero trail consumption). The collision is recorded as N-156's item 7.

## Summary

The og ships a complete, dedicated Outlook Express Dbx carve analyser — `RawSpeed::DbxAnalyser` (format id 0x15=21, one of the highest-code-weight classes in the engine) — registered under THREE magic keys (`JMF6`, `CF AD 12 FE`, `F7 89 34 2C`), with a GUID-table identity probe and a header-field size computation. The core has none of it: no registry row, no validator, no raw_signs entry. Session 3 discovered exactly this gap, marked it "MISSING (N-026)" in its evidence, planned the filing — and never filed it; the ID was consumed by a parallel lane's refs finding. The absence has therefore sat unfiled through the entire campaign while every neighbouring format (PST, MDB, SQLite) landed.

## What the reference does

**Registry (RAWSearchDLL_decompiled.c :2192-2232, the static initializer):** three consecutive 0x18-byte chain nodes are constructed, each stamped `RawSpeed::DbxAnalyser::vftable` with `idx = 0x15` (21), and inserted under three magic keys:

| Key (LE u32) | Bytes | Meaning |
|---|---|---|
| 0x36464d4a | `4A 4D 46 36` = "JMF6" | the DBX variant magic (per the probe: "JMF6" + `03 00 01 00`) |
| 0xfe12adcf | `CF AD 12 FE` | the classic Outlook Express store-file magic |
| 0x2c3489f7 | `F7 89 34 2C` | the marked/folder variant (the probe's 16-byte pattern) |

**Identity probe (FUN_180016740 :17509-17562):** given a 16-byte window, stamps format id 0x15 when ANY of:
- the first 8 bytes equal `0x0100003_36464d4a` — i.e. "JMF6" followed by `03 00 01 00` (the version word);
- the first 16 bytes equal one of FOUR Outlook Express GUIDs (stack table :17529-17541): `{fe12adcf, 6f74fdc5|c6|c7, 11d1e366, c0004e9a}` (the OE store GUID family, three generations) or `{fe12adcf, 26fe9d30, 11d21a8f, 6000bfaa}` (the fourth variant);
- the first 16 bytes equal `{2c3489f7, 6deef0f7, d18922af, b0f0ff00}` (the marked pattern).

**FileInfo (FUN_180016880 :17586-17600):** a 0x120-byte `DBXRawFileInfo` (id 0x15; ctor defaults include slot 0x1b=0xe, slot 0x22=0x14, u16@+0x23=0x100 — the base-CRawFileInfo fields).

**Size/name pass (FUN_180016950 :17622-17690):** for the plain (CFAD12FE) form, the carve size is `u32 @ +0x7C` of the header (the DBX's own self-declared size field). For the marked (F789342C) form — gated by a bound check — the size becomes `(param_4 - base) * param_3 + FUN_180042b30(...)` (a block/sector computation) and a 272-byte NUL-terminated buffer is copied out as the embedded name (the DBX folder-name field).

**Downstream surface:** the og's extension table carries id 21 = "DBX" (`s5-grading-extension-table-dump.md:56`), so carved rows grade and categorise as mailbox stores; the census's own note (registry-census §5): "Database carving exists (SQLite SQLi + dBase + Access MDB + **Outlook Express Dbx** + PST) — the 'email/document recovery' marketing claims map to these concrete classes."

## What our core does

Nothing. Verified at `c7fa527c`:
- `rg -i "dbx" src/ include/ tests/` → 0 hits (the only "dBase" mention is a comment at `cr_analysers.cpp:4272` noting the *AI/EPS* row is NOT dBase — an erratum record, not coverage);
- the registry byte-match: all 122 FMT_ rows' magic arrays extracted and compared against all three keys in both endiannesses — zero matches (`evidence/decompiled-quotes/s41-registry-key-residue.md`);
- `cr_rawsigns_data.cpp` (the compiled-in raw_signs.json parity table) carries no dbx extension — and the og's own `raw_signs.json` (139 extensions, parsed this session) has no dbx entry either, so the JSON layer cannot backstop the absence in either implementation.

## Gap

Falsifiable: plant a fixture beginning `CF AD 12 FE` + a valid OE GUID at +0 and a plausible u32 size at +0x7C — the og's engine carves it as a DBX row (three registry keys + probe); the core's carve pass finds no row keyed on any of the three magics and skips the candidate entirely. The same holds for the JMF6 and F7-89-34-2C entry forms.

## Impact

Whole-format silent false negatives on a classic data-recovery target: Outlook Express shipped on every Windows XP/2000 machine and its `.dbx` mail stores are a first-tier recovery ask for that era's disks (the og's own marketing maps "email recovery" onto this class). The core recovers the sibling formats (PST ✓ FMT_PST, MDB ✓ FMT_MDB/FMT_ACCDB, SQLite ✓ FMT_SQLITE) but not the store format of the most widely deployed mail client of the corpus's target era. Every deep scan of a legacy disk silently omits recoverable mailbox content — the S-053 class (silent loss of recoverable content) at format granularity, held to MAJOR (not SEVERE) because it is a missing-format gap rather than wrong-results-on-scanned-formats, matching the M-098/M-099 precedent for absent carve analysers.

## Reproduction / verification

Corpus-decompile-derived, first-hand this session (S41 lane, T9 wave):
- the registry window re-read line-addressed at :2192-2232 (three nodes, three keys, idx 0x15 each) — `evidence/decompiled-quotes/s41-registry-key-residue.md` §1 carries the quoted decompile;
- the probe/ctor/size functions read at :17509/:17586/:17622 with the GUID table and the JMF6 version word decoded — §2 of the same evidence file;
- the core-side absence by the byte-level registry diff (the 57 MISSING-marked s3 rows re-verified against the current registry: 53 landed, the DBX triple + the DXF-999 key remain) — §3;
- the og's raw_signs.json parsed for dbx/jmf/outlook → no entries (the JSON backstop does not exist);
- the trail-side never-filed record read at `s3-rawsearch-inventory.md:252-254` + `S3-CARVE.md:70`.
UNVERIFIED-BY-EXECUTION: no og run was executed against a DBX fixture this session (no Windows host in this lane); the decode evidence is the verification basis, per protocol tier 2.

## Fix plan (for the engineering team)

1. **Decode the full grammar first** (the two remaining FUN bodies the fixer must walk before writing the validator): `FUN_180042b30` (the marked-form block computation the size pass calls) and the `DBXRawFileInfo` Analyse virtual (the vtable slot dispatch from :17586's ctor) — pin the exact size formula and the 272-byte name-field offset semantics.
2. **Add the registry rows** in `src/core/carve/cr_registry.cpp`: three rows (or one row + a key-alias mechanism consistent with the FMT_CAB/FMT_CAB_LC duplicate-key precedent). The LE u32 key 0xfe12adcf reads on disk as `CF AD 12 FE`, so the magic array is `{0xCF,0xAD,0x12,0xFE}`; `FMT_DBX_JMF6` keyed `{'J','M','F','6'}` (key 0x36464d4a); `FMT_DBX_MARKED` keyed `{0xF7,0x89,0x34,0x2C}` (key 0x2c3489f7). Enum values APPENDED (append-only contract).
3. **Write `validate_dbx`** in `src/core/carve/cr_analysers.cpp`: re-check the magic; validate the identity per the probe's table (the JMF6+`03 00 01 00` 8-byte form, the four OE GUIDs at +0, or the 16-byte marked pattern); size = `u32@+0x7C` for the plain forms, the block computation for the marked form (step 1's decode); confidence and bounds per the DBXRawFileInfo ctor defaults once decoded.
4. **Grade/ext surface:** the "dbx" extension maps to the og's ext-table id 21; category per the og's type table (the email/document bucket).
5. **Tests:** plant fixtures for all three entry magics (a plain CFAD12FE store with the OE GUID and size@0x7C; a JMF6+version form; a marked F789342C form), plus hostile variants (right magic, wrong GUID; GUID without size sanity) — RED before, GREEN after; add the registry-key presence assertions to the F8 gate's table test.
6. **Register the never-filed sweep:** after landing, the s3 inventory's "MISSING (N-026)" markers should be re-pointed at this finding (they currently cite an ID that was consumed by the refs finding).

## Tests to add

- `Carve.DbxPlainMagicCarves` (CFAD12FE + GUID + size@0x7C → carved with the declared size)
- `Carve.DbxJmf6VariantCarves`
- `Carve.DbxMarkedFormBlockSize`
- `Carve.DbxRejectsWrongGuid` (magic without a table GUID → no row)
- the F8 gate's registry-table test extended with the three keys

## Related findings

**M-098/M-099** (the absent-analyser precedent class) · **N-142** (the DXF-999 key — this session's sibling residue from the same key-diff sweep) · **N-026(ledger)** (the ID this finding was originally planned as — the collision record) · **S-053** (the silent-content-loss severity class)

## Open questions

The marked-form (F789342C) size computation depends on `FUN_180042b30`'s decode (fix-plan step 1) — the block arithmetic was not fully decoded this session; the fixer must pin it before writing the validator. The 272-byte name buffer's field offset (the DBX folder-name location) likewise rides the Analyse-virtual decode.
