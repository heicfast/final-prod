# MAJOR M-008 — $ATTRIBUTE_LIST chase scope + depth: core is multi-hop all-records, og is depth-1 in-use-only

**Severity:** MAJOR
**Module:** ntfs
**Parameters affected:** A25 (engine algorithm steps — $ATTRIBUTE_LIST chase), A27 (fragment handling — multi-hop child-record chase), B2 (recovery correctness — deleted/multi-hop fragmented files), B7 (hard-link/ADS/sparse — fragmented $DATA reassembly), A29 (merge/dedup — child-record fragment merge)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:53263-53283` (`FUN_1800430b0` — the in-record find-attribute function, with the `$ATTRIBUTE_LIST` fallback at `:53272`); `:52615-52687` (`FUN_180041df0` — the depth-1 child-record chase, no recursion); `:18423-18562` (`FUN_180017390` — in-use phase walk, passes non-NULL `param_6` at `:18462` so child-record list IS built); `:21095-21302` (`FUN_18001acc0` — deleted phase walk, passes NULL `param_6` at `:21161-21162` so child-record list is EMPTY)
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:475-572` (the multi-hop BFS chase — depth bound 64 via `FRAG_DEPTH_LIMIT` at `:23`, cycle guard via `visited` vector at `:489-501`, nested `$ATTRIBUTE_LIST` in child records queued for next BFS level at `:557-566`); `tests/unit/test_ntfs_volume.cpp:830-892` (`NestedAttrlistChase` exercises multi-hop chase for record 23 → 24 → 25, 8 clusters across two extents) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's `$ATTRIBUTE_LIST` chase is a depth-1 fallback that only fires
for in-use records (the deleted-phase walk passes NULL for the
child-record-list builder, so deleted records never get the chase).
The core's chase is a multi-hop BFS with depth bound 64 that fires for
ALL records (in-use and deleted). For a deleted NTFS file whose
`$ATTRIBUTE_LIST` points to extension records containing `$DATA`
fragments, the og recovers only the in-record `$DATA` (truncated
content); the core chases the list and recovers all fragments (full
content). For a multi-hop chain (record A's `$ATTRIBUTE_LIST` → record
B's `$ATTRIBUTE_LIST` → record C's `$DATA`), the og follows only one
level; the core follows the full chain.

The core's behavior is MORE capable than the og's — recovery of
deleted fragmented files is strictly better. But for og-parity
benchmarks, the core recovers more content than the og on the same
volume.

## What the reference does

The og's `$ATTRIBUTE_LIST` chase is the fallback in `FUN_1800430b0`
(the find-attribute function):

```c
// NTFSSearchDLL_decompiled.c:53263-53283 (FUN_1800430b0)
local_10 = FUN_180042020(param_1, ...);          // :53272  in-record find (FUN_180041850)
if (local_10 == 0) {
    // FALLBACK: chase $ATTRIBUTE_LIST child records (depth-1 only)
    local_10 = FUN_180041df0((longlong)param_1, local_18[0], ...);
}
return local_10;
```

`FUN_180041df0` (`:52615-52687`) iterates the child-record list at
`parse_ctx+0x20` (built by `FUN_1800425d0` only when `FUN_1800431d0`'s
`param_6` is non-NULL). For each child record, it calls:

```c
FUN_180041cf0(child_record, ...);          // parse child
FUN_1800414c0(child_record, target_type);  // count attributes of target type
FUN_180041850(child_record, target_type); // find attribute in child
```

It does NOT recursively chase `$ATTRIBUTE_LIST` in child records —
**depth-1 only**. If a child record itself has `$ATTRIBUTE_LIST`
pointing to a grandchild, the grandchild's attributes are NOT found.

The child-record list is built by `FUN_1800425d0` only when
`FUN_1800431d0`'s `param_6` is non-NULL:

- **In-use phase walk** `FUN_180017390` (`:18423-18562+`) at `:18462`
  passes `plVar9` (non-NULL) → child-record list IS built →
  `$ATTRIBUTE_LIST` chase WORKS for in-use records.
- **Deleted phase walk** `FUN_18001acc0` (`:21095-21302`) at
  `:21161-21162` passes `(undefined8 *)0x0` (NULL) → child-record list
  is EMPTY → `$ATTRIBUTE_LIST` chase does NOT work for deleted records.

So a deleted NTFS file with `$ATTRIBUTE_LIST` pointing to extension
records containing `$DATA` fragments recovers only the in-record
`$DATA` (the in-record fragments from the file's own MFT record),
not the extension fragments. The file is silently truncated.

## What our core does

```cpp
// src/core/ntfs/cr_mftwalk.cpp:475-572 (audited 5d6da5f)
// Multi-hop BFS chase — fires for ALL records (in-use and deleted).
const u32 FRAG_DEPTH_LIMIT = 64;            // :23

// :482  depth bound 64
for (int level = 0; level < int(FRAG_DEPTH_LIMIT) && !frontier.empty(); ++level) {
    // :489-501  cycle guard via `visited` vector
    for (const auto& entry : frontier) {
        if (visited.count(entry.record)) continue;
        visited.insert(entry.record);
        // :519-566  read child record, parse attributes, merge $DATA/$INDEX_ALLOCATION/$ATTRIBUTE_LIST
        //   - $DATA fragments merged into the parent's data view
        //   - $INDEX_ALLOCATION fragments merged into the parent's index view
        //   - $ATTRIBUTE_LIST entries in child records queued for next BFS level
        //     (next.insert(...) at :557-566)
    }
    frontier.swap(next);
    next.clear();
}
```

The core's chase fires for ALL records (no in-use gate). Multi-hop
BFS handles nested `$ATTRIBUTE_LIST` in child records. Cycle guard via
`visited` prevents infinite loops. Depth bound 64 is generous — real
NTFS files rarely need depth > 2.

`tests/unit/test_ntfs_volume.cpp:830-892` `NestedAttrlistChase`
exercises multi-hop chase for a regular file: record 23 → 24 → 25,
8 clusters across two extents. The core merges both fragments
(`bigfrag.txt` has 2 extents totaling `8 * CLUSTER` bytes). The og
would only see record 23's in-record fragment + possibly record 24's
first fragment (if processed via `FUN_180017390`), but NOT record 25's
grandchild fragment.

## Gap

Three concrete gaps:

1. **Deleted records:** the og's deleted-phase walk does NOT chase
   `$ATTRIBUTE_LIST` (`param_6` is NULL). The core's chase fires for
   deleted records too. Result: deleted fragmented files recover full
   content in the core, truncated content in the og.
2. **Multi-hop chains:** the og's chase is depth-1 (no recursion into
   child records' `$ATTRIBUTE_LIST`). The core's BFS handles arbitrary
   depth (up to 64). Result: multi-hop fragmented files recover full
   content in the core, partial content in the og.
3. **Cycle guard:** the og's `FUN_180041df0` does not appear to have
   a cycle guard — a malicious or corrupt `$ATTRIBUTE_LIST` pointing
   back to a visited record would loop. The core's `visited` set
   prevents this.

## Impact

- **Recovery correctness:** the core recovers full content for deleted
  fragmented files; the og silently truncates. The core is BETTER
  here — this is a SUPERSET divergence.
- **Parity benchmark:** any og-vs-core file-content comparison on a
  volume with deleted fragmented files diverges (the core's recovered
  file is larger than the og's).
- **User experience:** the core is MORE useful — users recover full
  content from files the og would have truncated.
- **Performance:** the core's chase does extra reads (one per child
  record per BFS level). For non-fragmented files (the common case),
  the chase is a no-op (no `$ATTRIBUTE_LIST` to chase). For
  fragmented files, the extra reads are bounded by the file's actual
  fragmentation (depth 1-3 in practice, not 64).
- **DoS surface:** a malicious volume with a deep `$ATTRIBUTE_LIST`
  chain could cause the core to do 64 reads per file. The og would
  do 1 read. Mitigated by the depth bound and the cycle guard.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `NTFSSearchDLL_decompiled.c:52615-52687` (`FUN_180041df0` — depth-1,
  no recursion), `:53263-53283` (`FUN_1800430b0` — the fallback),
  `:18423-18562+` (`FUN_180017390` — in-use phase, non-NULL `param_6`),
  `:21095-21302` (`FUN_18001acc0` — deleted phase, NULL `param_6`).
- **Core evidence:** first-hand read of `cr_mftwalk.cpp:475-572`
  (BFS chase, depth 64, cycle guard, no in-use gate).
- **Test evidence:** `tests/unit/test_ntfs_volume.cpp:830-892`
  `NestedAttrlistChase` exercises multi-hop chase for record 23 → 24
  → 25. Test currently PASSES (core chases the full chain).
- **Executed comparison:** not run — would require building both
  implementations and comparing recovered file sizes on a multi-hop
  fragmented fixture.

## Fix plan (for the engineering team)

The decision is: match the og's depth-1 in-use-only chase (parity) or
keep the multi-hop all-records chase (superset, more useful).
Recommendation: **superset by default, opt-in parity** — the og's
behavior is a limitation, and recovering less content from deleted
fragmented files is a real data-loss scenario the core should not
inherit. But the parity mode should be available for benchmark
comparisons.

1. **Architectural change:** add `WalkerFlags::single_hop_attrlist`
   (default OFF for superset, ON for parity). Document the default
   and the opt-in.
2. **Files to modify:**
   - `include/cr/ntfs/cr_mftwalk.h:30-58` — extend `WalkerFlags` with
     `single_hop_attrlist` (bit 0x80 or similar).
   - `src/core/ntfs/cr_mftwalk.cpp:475-572` — when the flag is ON,
     limit the BFS to depth 1 (the `level < 1` instead of
     `level < FRAG_DEPTH_LIMIT`). When OFF, keep the multi-hop BFS.
   - `src/core/ntfs/cr_mftwalk.cpp` (deleted-phase walk) — when the
     flag is ON, skip the chase entirely for deleted records (mirror
     the og's `param_6 = NULL`). When OFF, keep the chase.
3. **Algorithm/constant:** the depth bound stays at 64 for the
   superset mode; the parity mode uses depth 1 (or skips entirely for
   deleted records).
4. **Tests to add:**
   - `NtfsVolume.DeletedAttrlistChaseRecoversFullContent` — fixture:
     a deleted file with `$ATTRIBUTE_LIST` pointing to an extension
     record containing `$DATA` fragments. Assert the recovered file
     has full content (not truncated). This test PASSES today
     (core chases for deleted records).
   - `NtfsVolume.DeletedAttrlistChaseOgParityTruncates` — same
     fixture; assert the recovered file has only the in-record
     `$DATA` (truncated) when `single_hop_attrlist` is ON. This test
     FAILS today (core chases even in parity mode — the flag doesn't
     exist yet).
   - `NtfsVolume.MultiHopAttrlistChaseDepth3` — fixture: record A → B → C
     with `$DATA` in C. Assert the recovered file has C's content.
     This test PASSES today (BFS handles depth 3).
   - `NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1` — same fixture;
     assert the recovered file does NOT have C's content when
     `single_hop_attrlist` is ON. This test FAILS today.
5. **Contract/doc updates:** `docs/AUDIT-NTFS.md` `$ATTRIBUTE_LIST`
   section — note the default superset mode and the opt-in parity.
6. **Acceptance criteria:**
   - The four tests above pass.
   - The parity mode is documented as the "match og's behavior"
     option for benchmark comparisons.
   - The superset mode is documented as the "recover full content"
     default.

## Tests to add

- `NtfsVolume.DeletedAttrlistChaseRecoversFullContent` (PASSES today — regression guard)
- `NtfsVolume.DeletedAttrlistChaseOgParityTruncates` (FAILS today)
- `NtfsVolume.MultiHopAttrlistChaseDepth3` (PASSES today — regression guard)
- `NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1` (FAILS today)

## Related findings

- **S-001** (FIXED) — the destroy rule for fully-overwritten deleted
  files interacts with the chase: if the chase recovers more content,
  the destroy rule's "every probe cluster re-allocated" predicate has
  more clusters to check. The S-001 fix's `reused_ratio` correctly
  counts chased fragments (via `extents()`), so the destroy rule fires
  correctly.
- **M-007** — hard-link fan-out is the parallel "core emits more than
  og" divergence. Both are SUPERSET divergences where the og's
  behavior is a limitation, not a deliberate choice. The fix shape is
  the same: opt-in parity flag, default superset.

## Open questions

- **OQ-S2A-1:** does the og's in-use phase walk `FUN_180017390`
  process deleted records too (just with the chase disabled), or does
  it skip them entirely? The decompile at `:18423-18562+` is inside a
  loop at `:18939-18993`; that loop's record-selection logic needs
  full decoding. If `FUN_180017390` processes deleted records (just
  with the chase disabled), the og's behavior is: deleted records
  recover in-record `$DATA` only (truncated). If it skips them
  entirely, deleted records aren't recovered at all (the deleted-phase
  `FUN_18001acc0` would be the only path). Resolving this would
  sharpen the impact analysis.
- **OQ-S2A-4:** what's the og's effective depth bound for `$MFT`'s
  own `$ATTRIBUTE_LIST`? The `$MFT` fragmented handler
  (`FUN_18001a540` → `FUN_180042a80`) builds the child-record list and
  reads each child's `$DATA` runs, but doesn't appear to recursively
  chase. The core's bootstrap uses depth 64. Needs RE of
  `FUN_18001a540`'s full iteration structure.

## Resolution (session 11 — DFX11, the depth-model landing + premise correction)

**Status:** FIXED (with the finding's premise CORRECTED against the corpus —
the deleted-gate step was NOT implemented; see below)
**Fixed in commit:** this session's fix commit (`fix: M-008` — see git log)
**Verified by:** `NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1` (parity:
1 extent, 2048 bytes — record 24's fragment only, record 25's grandchild
fragment dropped), `NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1`
(both modes on a deleted 3-record chain: default 8 clusters / parity 4
clusters at exactly LCN 140), the pre-existing `NestedAttrlistChase` stays
green as the default-mode regression guard. Fail-before: walker-revert
(the depth gate reverted) — both parity tests fail (extents 2 vs 1,
4096 vs 2048 bytes). Suite 593/593 Linux Release.

**Resolution notes:** `WalkerFlags::single_hop_attrlist` (default FALSE =
the multi-hop BFS superset, per this finding's own recommendation: the
og's depth-1 is a limitation, not a choice; registered PARITY-DELTAS
D-66). TRUE = og parity for benchmark comparisons: the chase caps at ONE
level (children of the base record only; nested lists never queue) —
corpus FUN_180041df0 :52615-52687 (iterates the child list, no
recursion), re-verified first-hand this session.

**THE PREMISE CORRECTION (corpus wins):** this finding's gap #1 — "the
og's deleted-phase walk passes NULL param_6, so deleted records never
get the chase" — is STALE. It read FUN_18001acc0 (:21095+) as a
"deleted-phase walk"; the S-012 decode (landed after this finding was
filed) proved FUN_18001acc0 is the GUID-DUMP pass 2, which parses DUMP
BUFFERS, not MFT records — its NULL param_6 means the dump walk builds
no child list, which is correct for a buffer parser. The pass-1
per-record processor is FUN_180017390 (:18423-18562), which S-012
decoded as emitting EVERY record unconditionally — and its
FUN_1800431d0 call at :18461-18464 passes the engine's list builder
(plVar9, non-NULL), so pass 1 builds the child-record list for EVERY
record, deleted included. Verified first-hand in the corpus this
session (the :18461 plVar9 = FUN_180042ef0(...) acquire + the :18464
call). Consequence: the og chases deleted records at depth 1 too; the
fix-plan step "when the flag is ON, skip the chase entirely for deleted
records" was REJECTED — implementing it would have diverged from the
og, not matched it. The corrected parity semantics (depth-1 for ALL
records, deleted included) are pinned by the deleted-chain test. The
cycle guard stays (a hardening the og lacks; a hostile ring would loop
the og forever). OQ-S2A-1 is thereby RESOLVED (FUN_180017390 processes
deleted records with the chase enabled — the finding's own open
question, answered by the same evidence). OQ-S2A-4 ($MFT's own
bootstrap depth) remains open — the bootstrap's chase is a separate
surface (audit row 6a) untouched by this fix.
