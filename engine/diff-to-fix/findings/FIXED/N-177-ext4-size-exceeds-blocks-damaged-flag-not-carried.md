# MINOR N-177 — ext4 "size exceeds blocks" damaged flag (props FLAGS bit 0x200) is computed and persisted by the og; the core has no carrier

**Severity:** MINOR
**Module:** ext4 (grading metadata)
**Parameters affected:** node fields (11), report schema (13), confidence grading inputs (15 adjacent)
**Corpus references:** `EXT4SearchDLL_decompiled.c:5455-5467` (`FUN_180007510` computes `i_blocks_lo < size>>9`), `:6300-6303` (the caller sets `props FLAGS (+0x39) |= 0x200` for files); the .rsf serialization of FLAGS (ifileproperties-part2 §1: +0x39)
**Core references:** no size-vs-blocks comparison in `src/core/ext4/cr_ext4_engine.cpp` (grep-verified); `FileProperties::flags` carries only the internal bit-4 (`include/cr/tree/cr_node.h:85`); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og marks ext4 files whose i_size exceeds their block coverage (truncation damage)
with a persisted damaged bit (0x200 in the node FLAGS), serialized into the session and
consumed by grading/downstream. The core computes nothing equivalent — the damaged
class is invisible in core sessions.

## What the reference does

The two computes + the flag set + the rsf persistence (cites above).

## What our core does`

No comparison; no flag.

## Gap

A truncated ext4 file (i_size 100 KiB, i_blocks covering 4 KiB): og node carries the
0x200 marker; core node carries nothing.

## Impact

A grading/diagnostics input the og persists is absent — hosts cannot render the og's
damaged-file indication for ext4; parity sessions lose a signal. (The og's compute has
its own quirk — the +0x1F sign-extension on i_size_high — noted in the lane sheet; the
core should compute the CLEAN version and register the non-adoption.)

## Reproduction / verification

Corpus-derived (lane E first-hand: both sites + the rsf field). Core-derived (the grep +
the flags census).

## Fix plan

1. Compute `damaged = i_blocks_lo(+hi) * 512 < i_size` (clean 64-bit form; the og's
   sign-extension quirk NOT adopted — register beside D-58/D-59) in the ext4 engine's
   props assembly; set a new node flag bit (e.g. NODE_FLAG_SIZE_EXCEEDS_BLOCKS, next
   free bit after the S-052 bit-10 usage — verify the free bits first).
2. Files: `src/core/ext4/cr_ext4_engine.cpp`, `include/cr/tree/cr_node.h`, the session
   record's flags round-trip (`src/core/session/cr_records.cpp` — flags already
   persist).
3. Tests: the truncated fixture + the round-trip.
4. Acceptance: the damaged marker survives save/load; clean files carry no bit.

## Tests to add

- `Ext4.SizeExceedsBlocksSetsDamagedFlag` (fails today)
- `Ext4.DamagedFlagSessionRoundTrip`

## Related findings

- I-040 (the non-adoption register for the og's compute quirk)
- S-023 lineage (the ext4 grading superset inputs)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-177 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Ext4Engine.SizeExceedsBlocksSetsDamagedFlag` (the live-tree damaged file + the files-only dir control + the deleted-path damaged file; i_size 4096 vs coverage 1024 and the zeroed-coverage variant) + `Ext4Engine.DamagedFlagCleanFilesCarryNoBit` (the pristine fixture sets the bit on nothing) + `Ext4Engine.DamagedFlagSessionRoundTrip` (the bit survives serialize/deserialize through FileProperties::flags' existing put16/g16 pair). RED pre-fix: the tests do not compile without the flag constant (the API is part of the fix) and the flag compute is absent. Suite 1275 ran / 1233 pass / 42 env-skips / 0 fail (linux-release).
**Resolution notes:** The marker landed as `NODE_FLAG_SIZE_EXCEEDS_BLOCKS = 0x0200` — the SAME numeric bit as the og's node FLAGS 0x200 (EXT4SearchDLL :6300-6303, re-read first-hand; the verdict compute FUN_180007510 :5455-5467 re-read beside it). The compute is the CLEAN 64-bit form per the finding's own fix plan: `u64(blocks_512) * 512 < size`, files only, no sign-extension on i_size_high (the og's +0x1F quirk is the I-040/D-123 defect family — the non-adoption registered as PARITY-DELTAS D-135), no 32-bit truncation of either side (the og compares u32(u_blocks) < u32(size>>9)). Both emission paths compute it (the live-tree child walk and the deleted-inode "InodeNNNN" emission); the session round-trip needed NO code — FileProperties::flags already persists. The ext4 test fixture's `write_inode` now populates i_blocks_lo (real volumes always carry the coverage word; the fixture was unrealistic with it zeroed). The fixture-wide consequence is covered by DamagedFlagCleanFilesCarryNoBit.
