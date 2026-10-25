# MINOR N-074 — Doc-drift group #2: five stale/incorrect claims (CAPABILITY-MATRIX LZNT1, CORE-GAP-LIST diskmap row, PARITY-DELTAS D-43 block size, D-19 collision "unevidenced", cr_health.h SMART command list)

**Severity:** MINOR
**Module:** quality (cross-cutting documentation truth)
**Parameters affected:** P46 (documentation of the contract), P68 (documentation accuracy), P69 (doc drift)
**Corpus references:** (per sub-item below — all first-hand corpus/code verifications this session; the corpus side is the *absence* of the claimed states or the newly-cited evidence)
**Core references:** `docs/CAPABILITY-MATRIX.md:22` (claims NTFS LZNT1 decompression "not implemented"), `docs/CORE-GAP-LIST.md:135` (GAP-S6-11 marked MISSING though the disk-map surface landed), `docs/PARITY-DELTAS.md` D-43 (claims 64 KiB cache blocks; the call site is 256 KiB since a4053c4), D-19 (:422-431, "collision handling not evidenced … no collision policy in any artifact" — refuted by DRWUI_decompiled.c:357116-357131, see N-048), `include/cr/win/cr_health.h:8` (claims the SMART ladder issues 0xD1 READ_THRESHOLDS / 0xD8 RETURN_STATUS; `src/win/cr_health.cpp:167-187` issues only 0xEC + 0xB0/0xD0)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Five documentation-truth defects, each verified against the tree this session: a capability claim contradicted by shipped code (LZNT1 IS implemented), a gap-list row stale after the fix landed (disk map), a parity-delta parameter stale after a later wave (cache block size), a parity-delta premise refuted by newly-cited decompile evidence (collision policy), and a header comment claiming SMART commands the implementation never issues.

## What the reference does / What our core does

1. **CAPABILITY-MATRIX.md:22** — the row says "codecs NOT implemented … **MISSING (GAP-P1-1)**" for "NTFS compressed $DATA (LZNT1/XPRESS/LZX)"; the LZNT1 half is stale: the decompressor ships (`src/core/compress/cr_decomp.cpp`, the LZNT1 ladder at the corpus anchor NTFSSearchDLL:2916 — re-verified this session) and the MFT walk inflates compressed $DATA through it (`cr_mftwalk.cpp:7,927-935` — `expand_compressed_data` → codec-tagged extents; the writer inflates). The precise fix: split the row — LZNT1 implemented; XPRESS/LZX a SHARED limitation with the og (both sides LZNT1-only, per the MATCH-by-omission rows).
2. **CORE-GAP-LIST.md:135** — GAP-S6-11 (visualization data model) marked MISSING; `include/cr/tree/cr_diskmap.h` + `src/core/tree/cr_diskmap.cpp` + the `cr_ui_disk_map` ABI export landed (the C consumer exercises it, tests/ui_abi_check.c:578-612). The row should be CLOSED-with-residual (the free overlay — S-009's addendum).
3. **PARITY-DELTAS D-43** — describes 64 KiB cache blocks; the live call site is `CachedReader(8 MiB, 256 KiB)` at `src/core/orchestrator/cr_scan.cpp:647` since a4053c4 (D-51 already registers the 256 KiB reality; D-43's text was left behind).
4. **PARITY-DELTAS D-19** — "collision handling not evidenced as rename-policy … no collision policy in any artifact"; DRWUI_decompiled.c:357116-357131 evidences the exact policy (`%s_%d` forms, 10,000-attempt cap) — see N-065 for the full decode.
5. **cr_health.h:8-9** — the DELTA comment lists "0xD1 READ_THRESHOLDS / 0xD8 RETURN_STATUS" in the issued command set. **Refined at the merged tree (post-S-024-fix):** 0xD1 is NOW genuinely issued (`cr_health.cpp:192-200`, landed with the S-024 fix `74f9def`) — that half is resolved; **0xD8 RETURN_STATUS remains claimed-but-unissued** (grep of cr_health.cpp: 0 hits). The residual fix: drop 0xD8 from the header list or issue it.

## Gap

Falsifiable: each cited doc line asserts a state the code or corpus contradicts, as detailed above.

## Impact

Consumer-facing trust: the ABI-adjacent docs (CAPABILITY-MATRIX, CORE-GAP-LIST, PARITY-DELTAS) are the parity contract's face; the N-040 family established that stale contract docs cost engineering time (engineers acting on "not implemented" skip shipped features; auditors chase closed gaps).

## Reproduction / verification

All five verified first-hand this session: code reads + greps at the audited commit (each cited above with file:line); the D-19 refutation cites the corpus decompile lines (preserved in evidence/decompiled-quotes/s6-writer-collision-and-mkdir.md).

## Fix plan (for the engineering team)

1. CAPABILITY-MATRIX.md:22 — flip the LZNT1 row to implemented (cite cr_decomp.cpp + the MFT-walk inflation site).
2. CORE-GAP-LIST.md:135 — close GAP-S6-11 with the residual pointer (S-009 addendum: free overlay unproducible).
3. PARITY-DELTAS D-43 — re-word to the 256 KiB block reality (or collapse into D-51 which already registers it).
4. PARITY-DELTAS D-19 — rewrite per N-065 (evidenced `%s_%d`/cap-10000 policy + the SHCreateDirectoryExW 0/0xB7 gate; keep our atomic-write/verify supersets).
5. cr_health.h:8 — correct the command list to what ships (0xEC, 0xB0/D0 + legacy fallback), and note threshold reads arrive with S-024's fix.
6. Tests: none for prose docs; for the header, add a comment-truth pin is overkill — the N-040-family doc-lint (if adopted) covers it.
7. Acceptance: five greps return the corrected texts.

## Tests to add

- None beyond the doc-lint family (N-040's fix plan).

## Related findings

N-020 (documentation-drift group #1 — this is group #2, same family); N-040 (ABI doc truthfulness — the header sub-item belongs to that family); N-065 (the D-19 evidence); S-009 addendum (the GAP-S6-11 residual); S-024 (the thresholds that will make the health header true).

## Open questions

None.

## Resolution

**Status:** FIXED (all five sub-items corrected on the current tree; sub-item 5 additionally pinned by an executable doc-truth test)
**Fixed in commit:** this session's N-074 doc-truth batch
**Verified by:** each sub-item re-verified first-hand before editing (the FreeMap producer present at cr_scan.cpp:896-931 + cr_records.h serialize_free_map + cr_carve.h's gate; the live call site `CachedReader::create(std::move(retry), 8ull << 20, 256 * 1024)` at cr_scan.cpp:732; the collision decompile re-read directly — DRWUI_decompiled.c:357110-357131 `FID_conflict_sprintf_s(..., L"%s_%d", ...)` + the `10000 < attempts` cap; XPRESS/LZX: zero corpus hits, both sides LZNT1-only; grep of cr_health.cpp for 0xD8: 0 hits while 0xD1 IS issued at :192-200). The health-header sub-item carries `Health.SmartCommandListHeaderIsTruthful` (fail-before executed: pre-fix header text restored → the pin FAILs on the 0xD8-in-issued-list check; fixed text → green). The remaining sub-items are prose docs — the finding's own plan records "none beyond the doc-lint family" for them.
**Resolution notes:** (1) CAPABILITY-MATRIX row SPLIT: LZNT1 DONE (cr_decomp.cpp lznt1_payload_decode + cr_mftwalk.cpp:944 expand_compressed_data cited in-row); XPRESS/LZX MATCH-by-omission (no corpus surface — shared limitation, honest). (2) GAP-S6-11 CLOSED with the S-009 pointer set (cr_ui_disk_map ABI + the FreeMap producer + REC_ALLOC_MAP persistence all shipped; the row's "internal-only" claim predates S-009 wholesale). (3) D-43 re-worded to the 256-KiB block reality with the a4053c4 attribution and the D-51/D-72 cross-references (the "64-KiB blocks" text was pre-a4053c4). (4) D-19's "no collision policy in any artifact" premise REFUTED with the decompile (verified this session at the cited lines — the %s_%d forms + 10,000 cap); the AUDIT-UI §5.7 "unevidenced" stance marked superseded, N-065 cross-referenced for the full decode. (5) cr_health.h command list corrected to the issued set (0xD0/0xD1[S-024]/0xEC + legacy 0x7C088) with the N-074 note explaining why 0xD8 was removed rather than invented (status-poll command, outside the health read path, absent from the corpus); the win-layer parse gate re-run green on the edited header. Note: N-065 (the collision-policy MINOR whose evidence D-19 now cites) remains open as its own finding — this resolution consumes its evidence for the doc truth only; the engine-side collision-naming parity (name_N vs %s_%d) is N-065's own scope, untouched here.
