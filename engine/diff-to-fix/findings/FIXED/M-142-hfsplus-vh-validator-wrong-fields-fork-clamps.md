# MAJOR M-142 — HFS+ volume-header validator gates the WRONG FIELDS: the og gates freeBlocks ≤ totalBlocks && journalInfoBlock ≤ totalBlocks (the only byte-swapped u32s besides blockSize); the core gates allocation/extents FORK block counts — both acceptance directions diverge on torn headers, and the misreading lives in two corpus docs

**Severity:** MAJOR
**Module:** hfsplus (volume-header validation)
**Parameters affected:** B1 (detection correctness), B3 (false-positive rate), A25 (engine algorithm steps), Q68 (documentation accuracy — the corpus artifact and AUDIT-LINUX-FS carry the misreading)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c` FUN_18000c6e0 (:9900-10020 — the copy loop, the selective byte-swaps, and the gate conjunction), re-read first-hand with the full frame-offset derivation this session
**Core references:** audited `34d6cf10` — `src/core/hfsplus/cr_hfsplus_engine.cpp:574-578`: `if (alloc.total_blocks > total_blocks || ext.total_blocks > total_blocks) return false;` (allocationFile +0x7C / extentsFile +0xCC fork clamps; the comment claims "decompile-verified gate")
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's HFS+ volume-header validator copies the first 512 bytes of the VH, byte-swaps exactly six fields (sig, version, and the four u32s at VH+0x0C, +0x28, +0x2C, +0x30 = journalInfoBlock, blockSize, totalBlocks, freeBlocks), and gates `sig ∈ {H+, HX} && version < 0x11 && blockSize % 512 == 0 && freeBlocks ≤ totalBlocks && journalInfoBlock ≤ totalBlocks`. The fork fields at +0x70/+0xC0 are never swapped or compared. The core gates the allocation/extents fork `total_blocks` clamps instead — fields the og never touches — so each side accepts torn headers the other refuses, in both directions.

## What the reference does

FUN_18000c6e0 — the derivation, re-verified first-hand this session (every step):

1. The copy loop (`lVar12 = 4` iterations × 0x10 qwords = 512 bytes) copies `param_1[0..63]` — the first 512 bytes of the VH — to the frame starting at `&local_218` (frame offset −0x218).
2. The named locals' frame offsets map to VH fields: `local_218`(−0x218) = copy+0 = signature; `uStack_216` = +2 = version; `uStack_20c`(−0x20c) = +0x0C = **journalInfoBlock**; `uStack_1f0`(−0x1f0) = +0x28 = **blockSize**; `uStack_1ec`(−0x1ec) = +0x2C = **totalBlocks**; `local_1e8`(−0x1e8) = +0x30 = **freeBlocks** (the HFS+ VH layout: journalInfoBlock@0x0C, blockSize@0x28, totalBlocks@0x2C, freeBlocks@0x30 — the fork structures start at +0x70, far beyond the swapped band).
3. The swap block (the CONCAT13/CONCAT12 sequences at :9978-9998) bswaps exactly those four u32s (plus the 2-byte sig/version handling); the fork fields are never swapped — so they cannot be the compared values (big-endian garbage would never satisfy a ≤ totalBlocks test).
4. The gate conjunction (:10005-10006):
```c
if ((((local_218 == 0x482b) || (local_218 == 0x4858)) && ((uStack_1f0 & 0x1ff) == 0)) &&
   (((uStack_216 < 0x11 && (local_1e8 <= uStack_1ec)) && (uStack_20c <= uStack_1ec)))) {
    FUN_18000cf90((undefined1 *)local_108);   /* proceed to the next stage */
}
```
— H+/HX, version ≤ 16, blockSize 512-multiple, **freeBlocks ≤ totalBlocks, journalInfoBlock ≤ totalBlocks**.

## What our core does

`cr_hfsplus_engine.cpp:574-578` — the allocationFile fork's `total_blocks` and the extentsFile fork's `total_blocks` clamped against the VH `total_blocks`, with a comment asserting the fork-clamp reading is decompile-verified. The sig/version/blockSize gates are parity (the N-005-registered blockSize bounds are a documented superset); the clamp gates are the divergence.

## Gap

Both directions, both falsifiable on synthetic VHs:
- Fixture A (FP direction): freeBlocks = 0xFFFFFFFF, totalBlocks = 0x10000, valid forks/sig/version/blockSize — the og rejects (freeBlocks > totalBlocks); the core accepts (forks fine) and scans a header the reference product refuses.
- Fixture B (FN direction): allocationFile.total_blocks = 0xFFFFFFFF (torn fork), everything else valid — the og accepts (forks never gated) and scans; the core returns false → the whole volume is skipped.

## Impact

Acceptance-band divergence on torn/damaged volume headers — exactly the recovery-target population; direction B is lost recovery (whole-volume), direction A is false-positive acceptance the og guards. Propagation: the fork-clamp misreading is encoded in `artifacts/SearchDLL/hfsplus-engine--deep-dive.md` §1 (:23-24) and `AUDIT-LINUX-FS.md` §2.1 row 1 — both docs assert the fork gates and must be corrected when the code changes (the corpus artifact's claim fed the core's comment; the decompile is the ground truth and it disagrees).

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: the copy loop, the frame-offset→VH-field mapping, the selective swap band, and the gate conjunction all read directly (the derivation above is reproducible from :9900-10020). Core side: the clamp quoted at `34d6cf10`. No executed fixture this session — fixtures A/B are the fix plan's first tests.

## Fix plan (for the engineering team)

1. In `validate_volume_header`: replace the two fork clamps with `free_blocks <= total_blocks && journal_info_block <= total_blocks` (the og's actual gates).
2. Decide the fork stance deliberately: if a wider recovery posture is wanted, KEEP the fork clamps as additional registered hardening on top of (not instead of) the og gates — then fixture B still refuses, which must be registered as a deliberate superset-refusal; recommendation: og-exact (drop the fork clamps) — the og's acceptance band is the product behavior under audit.
3. Correct the two docs: `hfsplus-engine--deep-dive.md` §1 and `AUDIT-LINUX-FS.md` §2.1 row 1 (note in each: the decompile's swapped-field set proves the fork reading wrong — cite FUN_18000c6e0 and the frame offsets).
4. Fix the core comment at :574 ("decompile-verified" → the corrected cite).
5. Files: `src/core/hfsplus/cr_hfsplus_engine.cpp`; the two docs.
6. Acceptance criteria: fixture A rejected, fixture B scanned (or B explicitly refused-and-registered if step 2 keeps the clamps); the docs match the decompile.

## Tests to add

- `HfsPlus.VhFreeBlocksAboveTotalRejected` (fixture A) — RED today (accepted).
- `HfsPlus.VhTornAllocForkStillScans` (fixture B) — RED today (refused).
- `HfsPlus.VhJournalInfoBlockAboveTotalRejected`.
- `HfsPlus.VhValidHeaderUnchanged` (control).

## Related findings

- M-138/M-145/M-146/M-141 (the cross-engine acceptance-band family: every engine's open gates re-derived from the decompile, not from the artifacts — this finding is the proof the artifacts themselves can mislead).
- N-198 (the exFAT core-only bounds — the same "register the stricter stance" decision, exFAT side).

## Open questions

Whether the og's `FUN_18000cf90` next stage re-gates the forks (the successor function was not decoded this session) — if it does, the fork clamps belong THERE, not at the VH gate; the probe is one function read (`FUN_18000cf90` at its definition, checking for fork-field compares).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — the S64 lane's first landing)
**Verified by:** `HfsPlus.VhFreeBlocksAboveTotalRejected` (fixture A — RED pre-fix: accepted, GREEN post-fix: rejected), `HfsPlus.VhTornAllocForkStillValidates` (fixture B — RED pre-fix: refused, GREEN: validated), `HfsPlus.VhJournalInfoBlockAboveTotalRejected`, `HfsPlus.VhFreeBlocksEqualTotalAccepted` (both `<=` edges), the re-pinned `HfsPlus.VolumeHeaderValidation` (its fork-clamp leg inverted to the og's acceptance); HfsPlus family 29/29 green post-fix; full suite re-run green.
**Resolution notes:** CONFIRMED-DIVERGENCE, fixed og-exact per the fix plan's recommendation (step 2's og-exact option). `validate_volume_header` (src/core/hfsplus/cr_hfsplus_engine.cpp) now gates `free_blocks <= total_blocks && journal_info_block <= total_blocks` — the og's actual conjunction, re-verified first-hand at FUN_18000c6e0 (HFSPlusSearchDLL_decompiled.c:10005-10006) with the frame-offset derivation (local_218=sig, uStack_216=version, uStack_20c=+0x0C journalInfoBlock, uStack_1f0=+0x28 blockSize, uStack_1ec=+0x2C totalBlocks, local_1e8=+0x30 freeBlocks) and the CONCAT13/CONCAT12 swap band (:9978-9998). The finding's open question is RESOLVED first-hand: FUN_18000cf90 (:10335) is the 16-byte fork byte-swapper (calling FUN_18000cd50 on the extents at +0x10), driven by the VH normalizer FUN_18000d030 over all five forks (+0x70/+0xC0/+0x110/+0x160/+0x1B0) — no fork compares anywhere in the family, so the fork clamps do not belong at any later stage either. The two fork clamps are dropped (og-exact; no superset registered — the finding's own recommendation). Preserved hardening: the N-005 blockSize bounds (512 floor / 16 MiB cap) and the total_blocks==0 geometric refusal stay. Doc legs: AUDIT-LINUX-FS.md §2.1 row 1 corrected to the real gates; CORPUS-ERRATA Session-65 row records the corpus deep-dive §1's misreading (the corpus repo is read-only — the errata row is the recorded correction channel).
