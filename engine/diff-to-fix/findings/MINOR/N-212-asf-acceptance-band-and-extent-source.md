# MINOR N-212 — ASF acceptance gates a torn count/size field the og never reads, and takes the extent from the walked objects where the og takes the FileProperties-declared size

**Severity:** MINOR
**Module / Surface:** carve (modules/carve.md) — the ASF/WMV validator's acceptance band and extent source
**Parameters affected:** A1 (detection correctness — the torn-header acceptance band), A25 (engine algorithm steps — the extent source), A27 (fragment/extent end)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:41948-41980` (the CAsfAnalyser/CAsfRawFileInfo ctor — the 0x188-byte object, the four walk GUIDs at +0x144/+0x154/+0x164/+0x174, NO count-field read), `:41927-41946` (FUN_180037030 — the tier-1: ONLY the 16-byte header GUID compare, raw asm verified), `:41994-42110` (FUN_180037210 — both walk phases), `:42113-42170` (FUN_1800374c0 — the 9-GUID top-level family table)
**Core references:** audited commit `9be37b68` (the S68 lane's L2-verified tree); `src/core/carve/cr_registry.cpp:304-315` (the FMT_ASF row), `src/core/carve/cr_analysers.cpp:2808-2859` (validate_asf — the count gate at :2820-2822, the size gate at :2817-2819, the 6-GUID walk at :2844-2857)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's ASF validator rejects two torn-header populations the og's analyser never gates on — a zero/garbage header object-count field (bytes +0x18..+0x1B) and a header-object size field (bytes +0x10..+0x17) outside [30, 2 GiB] — and it derives the carved extent from a walked top-level object grammar where the og derives it from the FileProperties child object's declared file-size field (bytes +0x28 of that child), maxed with the last walked top-level position and clamped by the global carve cap × 500. For intact files both implementations produce the same extent; for the corrupted-header population (the recovery product's own target) the core refuses recoverable candidates the og emits, and for padded/torn files the two take different extents.

## What the reference does

Verified first-hand, both walk phases decoded end-to-end:

1. **Tier-1 (FUN_180037030, raw asm at RVA 0x37030):** builds the 16-byte ASF header GUID on the stack (`30 26 B2 75 8E 66 CF 11 A6 D9 00 AA 00 62 CE 6C`) and compares the candidate's first 16 bytes against it — two `sub`/`jne` pairs over the two qwords, `sete al`. **That is the entire tier-1.** No count-field read, no size-field read (Ghidra's decompile of this function dropped the buffer parameter — the raw bytes `48 8b 0c 24 … 48 2b 0a 75 09 48 8b 4c 24 08 48 2b 4a 08 33 c0 48 85 c9 0f 94 c0` are unambiguous: `rcx=[rsp]; sub rcx,[rdx]; jne; mov rcx,[rsp+8]; sub rcx,[rdx+8]; xor eax,eax; test rcx,rcx; sete al`).

2. **The streaming walker (FUN_180037210, `(this, buffer, buffer_len)`):**
   - **Phase 1 — the header object's children** (from byte +0x1E to the header's declared size, capped at 0x400 children): each child's GUID is compared against the ctor's four precomputed GUIDs (`+0x164` = Stream_Properties `b7dc0791-a9b7-11cf-…`, `+0x174` = File_Properties `a1dcab8c-47a9-cf11-…`, plus two sub-GUIDs at `+0x144`/`+0x154` for the header-family subclassification). The **File_Properties child's bytes +0x28..+0x2F are extracted (AVX `vpextrq`) into `this+0x128` — the declared total file size**. Counters tick at `+0x138` (known), `+0x13c` (header-family), `+0x140` (unknown children — **counted and walked through, never a break**).
   - **Phase 2 — the top-level objects** (from the header end): each object is classified against FUN_1800374c0's 9-GUID family (header-obj/data/simple-index/index/media-index×2/file-props/stream-props/header-extension). Family objects are walked through by their size fields; **the first non-family object finalizes the extent**: `extent = max(FileProperties-declared-size [+0x128], last-top-level-position)`, then clamped by `min(extent, global_singleton(+0x70) × 500)` — the 0xBEBC200 (200 MB) global × 500 = 100 GB, non-binding in practice.
   - **Classification id:** unknown children > 0 → 6; header-family count == 1 → 8; known > 0 → 7; else 6.
   - **The header's declared object-count field (bytes +0x18..+0x1B) is never read anywhere in the family** (verified: no `+0x1c`/`[3]` indexed read in the ctor, tier-1, or walker).

## What our core does

`validate_asf` (cr_analysers.cpp:2808): compares the 16-byte GUID (og-exact, verified this lane), then:
- `:2817-2819` — **rejects `obj_size < 30 || obj_size > (2ull << 30)`** (the og has no such gate: a torn size field makes the og walk with a garbage cap — cap 0 emits a 30-byte salvage candidate; a garbage-large cap is bounded only by the 100 GB clamp);
- `:2820-2822` — **rejects `count == 0 || count > 128`** where count = `rd32(s, hit + 24)` (the og never reads this field);
- `:2844-2857` — walks top-level objects through exactly six GUIDs (header/data/simple-index/index/media-index×2), **breaking at any unknown GUID** (salvage-prefix semantics — matching the og's phase-2 stop-at-non-family) and at undersized/torn objects, `r.end = pos` (the last complete object).

## Gap

Falsifiable, three legs:
1. A candidate with a valid 16-byte ASF GUID and a torn count field (e.g. bytes +0x18..+0x1B zeroed or > 128) with intact structure otherwise: the og emits it (classification 6/7/8 per its counters); the core rejects it (`count` gate) — **recovery loss on the torn-header population**.
2. A candidate with a torn header-size field (< 30, e.g. zeroed): the og emits a 30-byte salvage candidate; the core rejects (`obj_size < 30`).
3. A padded or torn-tail ASF: the og's extent is the FileProperties-declared size (bytes +0x28 of that child) when the walk reaches a non-family object, potentially beyond or short of the walked prefix; the core's extent is the walked-object end. For intact files these coincide (the declared size is the true size); for a file whose declared size exceeds available data, the og clamps at 100 GB — i.e. effectively claims past the device end (the consumer clamps) — while the core carves the last complete object.

## Impact

User-observable on the corrupted-header population only (the M-146/D-119 calibration family's recurring class): ASF/WMV/WMA files with torn header fields — exactly the population a deep scan exists for — are refused by the core and emitted by the og. The extent leg affects padded/torn files' carved size. No crash, no security regression; the walk itself is bounded in both.

## Reproduction / verification

Corpus-derived, strength 2 (decompiled evidence, every claim first-hand this session):
- Tier-1 raw-asm decode (the 16-byte-only compare): RVA 0x37030, hex `4883ec28…c704243026b275c74424048e66cf11488b0c24c7442408a6d900aac744240c0062ce6c482b0a7509488b4c2408482b4a0833c04885c90f94c0`.
- No-count-read verification: grep over the ctor (`:41948-41980`), tier-1 (`:41927-41946`), and walker (`:41994-42110`) for `+0x1c`-indexed or `[3]`-indexed buffer reads — none.
- The FileProperties +0x28 extraction: `:42056-42059` (`uVar1 = vpextrq_avx(SUB3216(*(undefined1 (*) [32])(plVar5 + 4),0),1); *(undefined8 *)(param_1 + 0x128) = uVar1;` — plVar5[4]/[5] = object bytes +0x20/+0x28).
- The extent finalization + the ×500 clamp: `:42074-42086` (quoted in the lane sheet).
- The 9-GUID phase-2 family table: `:42113-42170` (FUN_1800374c0's nine-entry compare loop).
- Core side: the three gate sites re-read at HEAD (`cr_analysers.cpp:2817-2822`, `:2844-2857`).
- Executed comparison: not feasible this session (no og host); the corpus evidence is unambiguous.

## Fix plan (for the engineering team)

1. **Acceptance band (the recovery-loss legs):** drop the `count == 0 || count > 128` gate entirely (the og never reads the field) and replace the `obj_size < 30` rejection with the og's semantics — the size field only bounds the walk; a torn/zero size yields a 30-byte salvage emission. Keep a sanity clamp for the walk cap (the og's effective bound: the 100 GB product clamp; a 2 GiB row-level clamp is the natural core-scale stand-in if the row max stays 4 GiB).
2. **Extent source:** in the header-children walk, capture the FileProperties child's +0x28 declared size when present; on the phase-2 stop (first non-family top-level object or torn tail), set `r.end = max(declared_size, last_walked_position)` clamped by the row max and the device end — replacing the pure walked-object end.
3. Files: `src/core/carve/cr_analysers.cpp` (validate_asf), possibly `cr_registry.cpp` (row bounds unchanged).
4. Tests: `AsfTornCountFieldStillEmitted` (count==0 and count>128 fixtures — RED pre-fix, GREEN post), `AsfZeroHeaderSizeEmitsThirtyByteSalvage` (RED pre-fix), `AsfFilePropsDeclaredSizeDrivesExtent` (a padded file whose declared size exceeds the walked objects — extent = declared, RED pre-fix), `AsfTornTailStillSalvagesWalkedPrefix` (regression guard for the existing semantics), `AsfIntactFileExtentUnchanged` (the intact-file coincidence pin).
5. Contract: none (no public API change); the carve.md module note updates with the new extent semantics.
6. **Acceptance:** the three RED fixtures pass; the existing ASF suite stays green; the count gate's removal is grep-verifiable.

## Tests to add

As itemized in the fix plan (5 gtest cases, all named).

## Related findings

- **M-107** — the stand-in family's extent class (this finding's extent leg is the decoded-walk analogue for ASF, which M-107's eleven do not cover).
- **M-146 (FIXED)** — the probe-family acceptance-band calibration precedent (core-refuses-recoverable class).
- **D-119 / the M-090/M-091 decoded-walk precedents** — the truncate-vs-declared extent class.
- **S-007 (retired)** — the ASF grammar's original landing (the 6-GUID walk was its work; this finding refines the acceptance band and extent source against the now-fully-decoded og family).

## Open questions

1. The og's `+0x128` extraction reads FileProperties bytes +0x20..+0x2F via a 32-byte AVX load and extracts the high qword — whether the declared-size field is at +0x28 exactly (the ASF spec's `file_size` position) is 16-bit-confirmed by the load shape but was not spec-cross-checked; the fix should pin the offset with a real-file fixture before landing.
2. The two sub-GUIDs at `+0x144`/`+0x154` (`f8699e40-…`/`bc19efc0-…`, the 11cf legacy family) were not resolved to standard names this session — the classification counters they drive are not user-observable in the core's model (the core does not emit ids 6/7/8), so the miss is informational only.
