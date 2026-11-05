# SEVERE S-065 — ext4/ext2/ext3 indirect-block ladder is unbounded: a single stale-pointer deleted inode triggers ~10⁹ reads and a ~17 GB extent vector (freeze/OOM); the og bounds the identical walk by the file's logical size

**Severity:** SEVERE
**Module:** ext4 (deleted-inode indirect-block recovery)
**Parameters affected:** X52 (scalability at large N), X49 (memory), A26 (tree walk bounds), S32 (cancellation — the walk is unresponsive while it runs), B3 (false-positive extents from stale trees)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` FUN_180005490 (:3949-3998 — the indirect translation loop, size-bounded, re-read first-hand) and FUN_1800068b0 (:5021-5024, the content loop's `lVar16 < size` bound)
**Core references:** audited `34d6cf10` — `src/core/ext4/cr_ext4_engine.cpp:445-475` (`read_ladder`, no size/budget bound), `:426` (the 65536 node budget that gates ONLY the extents path), `:1063-1071` (the deleted-inode sweep calling `read_inode` for every inode)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's indirect-block ladder (`read_ladder`) recurses levels 1..3 over every nonzero pointer in each indirection block with no file-size bound, no node budget, and no extent-count cap — the per-inode budget at :426 gates only the extents path. A single deleted ext2/3 inode whose stale triple-indirect pointer now targets reused data (e.g. a block full of 0xFFFFFFFF) drives ~1.07×10⁹ block reads and up to ~17 GB of pushed extents on a 4-KiB-block volume. The og's identical walk is bounded by the file's logical size (`param_4 <= lVar9` break, `lVar9 += block_size`) — it never walks a stale tree past EOF. One poison inode freezes the core's scan or exhausts memory; the og reads at most size/block_size blocks.

## What the reference does

FUN_180005490 (`EXT4SearchDLL_decompiled.c:3949-3998`), the og's indirect-block translator — re-read first-hand this session:

```c
while( true ) {
  …
  if ((iVar2 == 0) || (param_4 <= lVar9)) break;      /* :3952-3954 — STOP at the file's size */
  lVar4 = lVar9 / (longlong)*(int *)(param_1 + 0x58); /* logical block index */
  …                                                     /* translate logical→physical */
  lVar9 = lVar9 + iVar2;                               /* lVar9 += block_size each iteration */
  …                                                     /* contiguous physical merge into extents */
}
```

`param_4` is the file's logical size; the loop cannot advance past `size` bytes regardless of what the pointer blocks contain. The content loop (FUN_1800068b0 :5021-5024) carries the same `lVar16 < size` bound. The og also merges physically-contiguous blocks into extents as it goes (the `local_58`/`uStack_50` pair appended to the caller's vector only on physical discontinuity — the merge branch at :3986-3996), bounding the extent vector by the number of physical runs within the file's size, not by pointer count.

## What our core does

`read_ladder` (`cr_ext4_engine.cpp:445-475`):

```cpp
std::function<bool(u32, u32, std::vector<Extent>&)> read_ladder =
    [&](u32 indirection_block, u32 levels, std::vector<Extent>& out) -> bool {
    if (levels == 0 || indirection_block == 0) return true;
    std::vector<u8> blk(size_t(v.sb.block_size));
    if (v.dev->read_at(…) != OK) return false;
    for (u32 i = 0; i < per_block; ++i) {
        const u32 b = le32(blk.data() + u64(i) * 4);
        if (b == 0) continue;
        if (levels == 1) { … out.push_back({off, v.sb.block_size}); }
        else if (!read_ladder(b, levels - 1, out)) return false;
    }
    return true;
};
…
for (u32 lvl = 1; lvl <= 3; ++lvl) {
    const u32 b = le32(raw + I_BLOCK + (11 + lvl) * 4);
    if (b != 0) (void)read_ladder(b, lvl, iv.extents);
}
```

Every nonzero pointer at every level is read and pushed. `per_block` = block_size/4 = 1024 at 4 KiB. Level-3 worst case: 1 + 1024 + 1024² + 1024³ ≈ 1.07×10⁹ reads and the same order of extents pushed (~17 GB of `Extent` at 16 bytes each). The `budget = 65536` at :426 is consulted only by `walk_extent_node` (the extents path) — `read_ladder` never reads it. The deleted-inode sweep invokes `read_inode` for every inode number (:1063-1071), so any deleted non-extents inode with a stale indirect pointer reaches this code.

## Gap

Falsifiable: an ext2/3 image with one deleted regular-file inode, `EXTENTS_FL` unset, small `i_size`, and `i_block[14]` (triple-indirect) pointing at a block of 0xFFFFFFFF. The core issues ~10⁹ device reads (countable with a read-counting device stub) and grows `iv.extents` without bound; the og performs at most `ceil(size/block_size)` translations. The freeze is user-observable (no progress, no cancel delivery while the lambda recurses — the cancel token is not consulted inside `read_ladder`), and the OOM path aborts the scan.

## Impact

Deterministic freeze/OOM on corrupt or stale-pointer images — precisely the recovery threat model. The "no stuck or freeze" mandate's clearest core-only violation found this session: the core is the *less* safe side (the og cannot hang here by construction). Secondary impact: even non-poison volumes pay unbounded extents (a stale double-indirect pointing at a mostly-nonzero block yields thousands of garbage extents per inode → false-positive recovered content).

## Reproduction / verification

Core side: full read of `read_inode`/`read_ladder` at `34d6cf10` (quoted above); the budget's extents-only scope confirmed at :426-443. Og side: FUN_180005490's size-bounded loop and FUN_1800068b0's content bound re-read first-hand (quoted above). Not executed this session — the read-count reproduction is the fix plan's first test (a stub device makes it deterministic).

## Fix plan (for the engineering team)

1. Bound the ladder by the inode's logical size, like the og: walk only logical blocks `< ceil(i_size / block_size)` (the deleted-inode `i_size` semantics the engine already decodes); stop the recursion when the accumulated logical offset passes EOF.
2. Additionally cap per-inode extents (e.g. the same 65536 budget) and consult the cancel token inside the loop (the og's per-record cancel discipline).
3. Adopt the og's contiguous-physical merge while touching the code (it bounds the extent vector to physical-run count and is the og's observable output shape).
4. Files: `src/core/ext4/cr_ext4_engine.cpp` (`read_ladder` + the `for lvl` driver); tests in the ext4 engine test home.
5. Acceptance criteria: the poison-inode fixture completes in bounded time (≤ size/block_size + constant reads; assert the read count against a stub device); no allocation growth past the cap; cancel honored within one block read; all existing ext4 suites green.

## Tests to add

- `Ext4Engine.PoisonTripleIndirectBoundedBySize` (read-counting device asserts ≤ ceil(size/bs)+ε reads; RED today — the count explodes).
- `Ext4Engine.PoisonDoubleIndirectExtentCapped` (extent count ≤ the cap).
- `Ext4Engine.StaleIndirectCancelResponsive` (cancel token trips within one block read).
- `Ext4Engine.ContiguousPhysicalBlocksMergedToSingleExtent` (the og's merge shape).

## Related findings

- N-196 (the og's walk-entry record-geometry gate — the NTFS-family analog of "the og's walk refuses nonsense geometry"; ext4's own analog is this finding's size bound).
- M-127 (retired: the FAT/exFAT cycle-guard family — the same hang-safety parameter, FAT side).

## Open questions

None — the decode is complete on both sides. Implementation choice only: size-bound alone (og-exact) vs. size-bound + budget cap (defense-in-depth); the recommendation is both, with the budget registered as hardening.

## Resolution

**Status:** FIXED
**Fixed in commit:** `<this push>` (the S56 ACUTE-first lane, 2026-10-29)
**Verified by:** fail-before/pass-after on all four tests:
`Ext4Engine.PoisonDoubleIndirectBoundedBySize` (RED: 273 device reads vs the
control's 15 — the unbounded walk reproduced; GREEN: reads == control),
`Ext4Engine.PoisonTripleIndirectBoundedBySize` (RED: the capped counting
device hit its 3000-read cap — the ~65k-read pre-fix walk blown past it;
GREEN: reads == control), `Ext4Engine.StaleIndirectCancelResponsive` (RED:
257 reads of the poison block after the token flipped; GREEN: ≤ 1),
`Ext4Engine.ContiguousPhysicalBlocksMergedToSingleExtent` (RED: 4 separate
extents; GREEN: 1 merged {21*BS, 4*BS}). Full ext4 family 42/42; full suite
green post-landing.
**Resolution notes:** CONFIRMED-DIVERGENCE, corpus re-verified first-hand
before fixing: FUN_180005490's `if ((iVar2 == 0) || (param_4 <= lVar9))
break;` at :3952-3954 (the loop advances lVar9 by block_size and never
translates a logical block at/after EOF) and the contiguous-merge branch at
:3986-3996 both re-read in the corpus decompile; FUN_1800068b0's
`lVar16 < size` content bound confirmed. The fix is the finding's plan
(both options, per its own recommendation): (1) the ladder is bounded by
`ceil(i_size / block_size)` — the direct loop stops at `size_blocks` and
the ladder's `read_ladder(indirection_block, levels, logical_start)`
prunes any subtree whose logical start is at/after EOF, with the per-pointer
`logical >= size_blocks` break giving the og's linear-order stop (a
zero-size stale inode walks nothing, matching param_4 == 0); (2) the
per-inode `kLadderExtentBudget = 65536` caps the merged-extent vector
(registered as defense-in-depth beyond the og, not og-parity); (3) the
cancel token is consulted at every indirection-block boundary (the Volume
context gains `cancel`, set by scan_ext4); (4) the og's
physically-contiguous merge adopted in `emit_block` (the vector grows by
physical-run count). A parallel hot-window artifact repaired en route: the
test file's doubled namespace closure + a stray orphan brace (the M-133
class). No safety hardening touched — the budget IS the hardening.
