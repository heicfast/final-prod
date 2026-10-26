# MAJOR M-015 — FAT: og validates loaded FAT tables (media-byte canonical form, whole-table cross-link rejection, 98% watermark); core loads FATs unvalidated

**Severity:** MAJOR (wrong-chain recovery on corrupt media; wrong-FAT-copy selection)
**Module:** fat
**Parameters affected:** B1 (detection), A27 (fragment handling), B3 (false positives)
**Corpus references:** `FATSearchDLL_decompiled.c` — CFAT16 `FUN_180041b00 :54052` (`bVar10 = *piVar2 == -8;` — FAT[0] must be exactly 0xFFFFFFF8, media 0xF8 canonical), entry sweep `:54060+` (OOB/==1/self-loop invalid), claim-map cross-link walk `:54084-54114` (cross-link or chain covering the table → **entire table rejected**); CFAT32 `FUN_1800417c0 :53853-53919` (family-tolerant FAT[0]/FAT[1], 98% watermark `DAT_1800728d4 = entries*0.98` at `:44782`, gated by deep-check flag `+0x30` `:53867`)
**Core references:** `src/core/fat/cr_chain.cpp:76-96` (`FatTable::load` — loads bytes, no signature/cross-link validation) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When the og loads a FAT copy it validates it as a whole: FAT16 requires the canonical media signature 0xFFFFFFF8 at FAT[0]; entries are swept for OOB/self-loop/cross-link corruption and a table whose chains cross-link (or whose claimed chains cover the table region itself) is **rejected wholesale** — the factory then retries another FAT copy. The core loads whichever copy it reads with zero validation, so a corrupt FAT1 silently produces wrong chains where the og would have rejected it and scanned on the mirror/alternate copy instead.

## What the reference does

Quotes above: `FUN_180041b00` (CFAT16 validate) enforces the exact media word, sweeps entries, and walks the claim map; `FUN_1800417c0` (CFAT32) is family-tolerant on FAT[0]/[1] and applies a 98% used-watermark sanity bound; rejection → `FUN_180006140` retries alternate FAT regions (`:3817-3950`).

## What our core does

`FatTable::load` reads `table_bytes` and trusts them; flavor selection is geometry-only.

## Gap

Falsifiable claim: a FAT32 whose FAT1 contains a cross-linked pair (two chains referencing the same cluster) is wholesale-rejected by the og (which retries FAT2/alternates and, on success, produces clean chains); the core walks both chains from the corrupt copy, emitting conflicting files with wrong content.

## Impact

Corrupt-FAT volumes are common in the target damage classes; wholesale rejection + copy-retry is the og's main defense against garbage chains, and its absence both degrades correctness and inflates false positives.

## Reproduction / verification

Decompile-verified (agent S2-B; `evidence/decompiled-quotes/fat-exfat-session2.md`); core load path verified first-hand.

## Fix plan

1. Add `FatTable::validate(flavor)`: FAT16 → FAT[0] == 0xFFFFFFF8 (exact); sweep for OOB pointers, ==1, self-loops; build a claim map and reject on cross-links or chains covering the table region; FAT32 → family-tolerant FAT[0]/[1] + the 98% watermark bound under the deep-check flag.
2. On rejection, retry the next FAT copy / alternate region (ties into M-012/M-011 fallback work) and record `fat_copy_selected` in scan stats.

## Tests to add

- `Fat.CrossLinkedTableRejectedAndRetried` — FAT1 cross-link, FAT2 clean: chains come from FAT2 (core currently walks FAT1).
- `Fat.MediaByteCanonicalEnforced` (FAT16).

## Related findings

- M-011 / M-012 (windows and fallback — the same factory policy) · I-002 (FAT12 0xFF7 raw-follow, confirmed this session)

## Open questions

The deep-check flag at object +0x30 defaults unverified — treat the watermark as advisory-on-deep-scan until pinned (engineering team, one breakpoint).

## Resolution

**Status:** FIXED
**Fixed in commit:** `6dc1e32`
**Verified by:** FatEngine.CrossLinkedFat1RejectedAndFat2Used (fail-before FAILED: the corrupt FAT1 was walked, cross-linked chains and all; pass-after: FAT1 wholesale-rejected, chains resolve through the promoted FAT2, the corrupt links are gone, fat_copy_rejected set), FatEngine.MediaByteCanonicalEnforcedFat16 (fail-before FAILED; pass-after: zeroed media pair rejects the copy, the clean mirror carries the volume, both-bad refuses with ERR_BAD_MAGIC). Suite 463/463 Linux ASan.
**Resolution notes:** fat_table_validate ports FUN_180041b00 (:54052-54141, re-read first-hand): the exact media-word pair for FAT16 (u32@FAT[0] == 0xFFFFFFF8; FAT12/FAT32 family-tolerant per the og's own CFAT32), the entry sweep (==1, self-loops, OOB pointers in [n, BAD) - the band is STRICTLY below the BAD sentinel so BAD clusters are legal values, and FAT32's mask-24 alternate EOC family (N-002) ends chains rather than reading as links), and the claims-map cross-link walk (a cluster claimed by two different chain roots rejects the whole table). On FAT1 rejection the mirror is validated and promoted (the og factory's alternate-copy retry, :3817-3950); both-bad declines the table with ERR_BAD_MAGIC. The 98% watermark is computed as an advisory (used-link count) but never a reject - its deep-check gate default is unpinned (the finding's own open question). Fixture corrections in the same change: the four synthetic FAT16 volume builders (test_fat_volume, test_orchestrator, test_pause_resume, test_cr_ui, test_telemetry) never wrote the canonical media pair - real volumes always carry it, and the lazy fixtures were rejected by the (correct) og-parity validator; they now write FAT[0]=0xFFF8/FAT[1]=0xFFFF like every real FAT16.
