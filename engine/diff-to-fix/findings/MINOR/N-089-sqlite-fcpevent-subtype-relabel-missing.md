# MINOR N-089 — the FCP-event-database subtype relabel: the og's SQLite analyser relabels FCP event databases to type id 0x75 (the "tableactiveactive" schema marker); the core labels every carved SQLite file with the one generic format

**Severity:** MINOR
**Module / Surface:** carve / the SQLite validator's typing (modules/carve.md)
**Parameters affected:** B1-adjacent (result typing/labeling), D13-adjacent (the carve format label)
**Corpus references:** `ghidra-searchdll/RAWSearchDLL_decompiled.c` — the registry initializer at :1962-1971 (CFCPEvtAnalyser id 0x73 constructed, then bound to key `0x694c5153` — "SQLi", the first 4 bytes of "SQLite format 3"); `FUN_18002dfc1` (the 660-byte family walk, :3432-35520): a name read as a wide string (RBP-0x80) → `WideCharToMultiByte` → exact-length-0x11 gate → `memcmp(name, "tableactiveactive", 17)` — on match: `*(u32*)(this+0x18) = 0x75` (the id slot RELABELED 0x73 → **0x75**) and `*(u8*)(this+0x134) = 0` (the ctor's +0x134 flag cleared); the page-size/page-count math (`this+0x110` nonzero ∧ `this+0x118 % this+0x110 == 0`) and the 200,000,000-byte size cap at the walk's tail; `rabin2_strings_all.txt` :7734 ("SQLite format 3" at .rdata 0x664b0) and :7968 ("tableactiveactive", 17 bytes, at .rdata 0x6a0c0)
**Core references:** `src/core/carve/cr_analysers.cpp:1163` `validate_sqlite` (the generic detection: magic + page math + page count — one label for every SQLite file; no FCP-subtype marker)
**Status:** READ-ONLY FINDING — no fix applied (filed from the OQ-SA-2 probe resolution)
**Prior audit check:** not in prior scope — the OQ-SA-2 wave asked only the attribution question (SQLite vs FCP Events); the relabel subtype is the newly decoded residue

## Summary

The og's SQLite detection (the class named CFCPEvtAnalyser — Final Cut Pro event bundles ARE SQLite databases) carries a two-tier typing: every "SQLite format 3" file carves with type id **0x73**, and the deep walk additionally reads table names; when a name matches the 17-byte FCP schema marker `"tableactiveactive"` (exactly 17 chars, `memcmp`-verified), the analyser **relabels the file to type id 0x75** — the FCP-event-database subtype. The og's results therefore show FCP event libraries under their own type; the core shows every carved SQLite file under the one generic format. Detection parity is exact (the core's `validate_sqlite` implements the same magic + page math); the divergence is the subtype LABEL only.

## What the reference does

Registry: key `0x694C5153` ("SQLi") → `BruteForce::CFCPEvtAnalyser` (id 0x73). Deep walk (`FUN_18002dfc1`): page-size/page-count consistency (`max_size % page_size == 0`), a 200 MB size ceiling, a name-column walk (names arrive as UTF-16, converted), and the subtype gate — a name of EXACTLY 17 bytes equal to `"tableactiveactive"` flips the id to 0x75 and clears the +0x134 flag set by the FileInfo ctor (FUN_18002e360 sets `+0x134 = 1`).

## What our implementation does

`validate_sqlite` detects and sizes every SQLite file identically (magic + page-size gate 512..64 KiB pow-2 + page count + size = pages × page_size) and emits the one generic format label; no FCP-subtype marker exists anywhere in the carve layer.

## Gap

Falsifiable claim: an FCP event database (a SQLite file whose schema contains the "tableactiveactive" marker) carves in the og with type id 0x75 and in the core with the generic SQLite label — same file, same extent, different type shown.

## Impact

- Labeling-only: the file is found, sized, and recovered identically; only the Type column (and any type-based filtering) differs for this one subtype.
- FCP users (the video-recovery audience) see their event libraries generically typed.

## Reproduction / verification

Strength-1 decompile decode (this session, first-hand): the registry binding (the initializer at :1962-1971), the marker literal (rabin2 :7968), the exact-length gate + memcmp + the 0x75 stamp + the +0x134 flag clear (FUN_18002dfc1). The semantic identity of the marker (a table NAME literally "tableactiveactive" vs a `type||name` concatenation from sqlite_master) is the one undecoded layer — the mechanical gate is byte-exact either way.

## Fix plan (for the engineering team)

1. Decode the marker's source once (where the 17-char name is read from — the sqlite_master walk's name column): one bounded decompile read of the code preceding FUN_18002dfc1's split point.
2. In `validate_sqlite`: after the page math, scan page 1's schema region (bounded — sqlite_master lives in page 1) for the 17-byte marker `"tableactiveactive"`; on a hit set the carve note to the FCP-event subtype (the labeling surface — the note rides the report/query layers; no serialized format-id change needed for a label).
3. Test: a fixture SQLite image whose page-1 schema region carries the marker → the carved record's note/label distinguishes it; a marker-free SQLite file → the generic label (fail-before: no subtype exists today).

## Tests to add

- `Carve.SqliteFcpeventSubtypeRelabeled` (fails before).
- `Carve.SqliteGenericLabelUnchangedWithoutMarker`.

## Related findings

- **OQ-SA-2** (this session's resolution) — the probe that produced this finding (the CFCPEvt attribution leg).
- **S-039/M-065** (FIXED) — the same surface's other legs.

## Open questions

The marker's semantic identity (a literal table name vs a type+name concatenation) — bounded, one decompile read; does not change the mechanical gate or the fix shape.
