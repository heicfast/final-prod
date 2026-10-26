# SEVERE S-025 — ext4 journal-salvaged entries are emitted flat; og reparents orphan dirs into the live tree (two-pass journal + ".""..".. parent map)

**Severity:** SEVERE
**Module:** ext4
**Parameters affected:** A5 (orphan reconstruction), A25 (engine algorithm steps — journal pass placement), B11 (node placement/parenting)
**Corpus references:** EXT4SearchDLL_decompiled.c — Complete driver `FUN_180003de0` :2722-2802: **journal pass BEFORE the root walk (:2773) AND again AFTER (:2778)**; orphan parent map from "."/".." pairs `FUN_1800043f0` :3077-3119; unreferenced-orphan fallback naming `iNode%I64d` :2986-2991; placeholder "Unknown" :6589; reachability check `FUN_180003fb0` :2807-2871.
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:822-850` (journal names emitted flat under root_lost, no parent resolution), single pass post-walk (:643-874, D-07/S-004 pins). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `S-018` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og runs the journal-salvage pass twice (before and after the live tree walk) and reparents salvaged directories into the recovered tree using a parent map built from "."/".." directory pairs; only truly unreferenced orphans fall back to `iNode%I64d` naming. Our engine runs one salvage pass after the walk and emits every salvaged entry flat under the "lost" root with no parent resolution. Users see salvaged directory contents dumped into Lost Files instead of under their original parents.

## What the reference does

`FUN_180003de0` (:2722-2802): journal pass 1 (:2773) → root inode-2 walk → journal pass 2 (:2778) → orphan finalize. Orphan dirs are keyed (child, parent) from "."/".." pairs (:3077-3119); a reachability check (:2807-2871) decides whether the live tree already claims the child; unreferenced ones get `iNode%I64d` (:2986-2991).

## What our core does

Single journal pass after the walk (:643-874); salvaged names emitted flat under root_lost (:822-850); no parent map, no reachability-based reparenting.

## Gap

Placement and pass-count divergence. Falsifiable: a volume with a deleted directory whose journal copy survives shows the dir under its original parent in the og output; ours shows its contents flat in "Lost Files".

## Impact

User-observable tree shape for salvaged content; findability of recovered files. No content loss — bytes are recovered either way (hence SEVERE, not ACUTE).

## Reproduction / verification

Corpus-derived (all cites first-hand). UNVERIFIED-BY-EXECUTION (needs a fixture with a journaled deleted dir — the jsb machinery exists in tests for S-004; extend it).

## Fix plan

1. Add the pre-walk journal pass (pass 1) so salvaged entries exist before the live walk can claim/parent them.
2. Build the "."/".." parent map during directory walks; key salvaged entries (child inode → parent inode).
3. Reparent salvaged dirs whose parent is reachable in the final tree; fall back to `Inode%u` naming (match og's `iNode%I64d` casing — see the E4 note in I-014 (parallel leg register)) only for unreferenced orphans.
4. Keep the S-004 every-block journal scan inside both passes.

## Tests to add

- `Ext4.JournalSalvagedDirReparented` — deleted dir with journal copy + live parent; assert the salvaged dir appears under its parent (fails today: flat).
- `Ext4.JournalUnreferencedOrphanNaming` — no live parent; assert `iNode%I64d`-style naming.
- `Ext4.TwoPassJournalClaimInteraction` — entry claimed by live tree in pass 2 not duplicated.

## Related findings

S-004 (FIXED — every-block scan; this finding is the placement/reparenting residual), I-014 (parallel leg register) (E4 superset note incl. the naming doc fix).

## Open questions

Which parent wins when both the journal and the live tree claim a child (og's `FUN_1800089b0`/`FUN_180003fb0` reachability semantics :6565-6630/:2807-2871) — decode before implementing step 3 (tracked OQ-E1).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for "fix(S-025)")
**Verified by:** 3 new tests: `Ext4Engine.JournalSalvagedDirReparented` (a journal snapshot of the LIVE parent's dir block naming a deleted dir + the deleted dir's own snapshot — the salvaged dir attaches UNDER its live parent, its child chains beneath it, nothing lands flat; the finding's falsifiable case), `Ext4Engine.JournalUnreferencedOrphanFlatWithRealName` (an unresolvable chain surfaces flat under Lost Files with the on-disk name — the finding's `iNode%I64d` expectation CORRECTED against the og's own trace decode: the iNode naming is the SWEEP surface's (our `Inode%u` at cr_ext4_engine.cpp, pre-existing); journal children keep real names), `Ext4Engine.TwoPassJournalClaimInteraction` (live-claimed entries never duplicate — `nested.bin` appears exactly once, journal_salvaged counts only the S-004 orphan remnants). FAIL-BEFORE PROVEN on the pre-fix worktree: the reparented test's `childdir`-under-`sub` assertion cannot pass pre-fix (the old code emitted every journal entry flat under root_lost — the S-004 flat emit, grep-verified); the claim-interaction test exposed a LATENT PRE-EXISTING BUG the old flat path hid: `visited_inodes` tracked only DIRECTORIES, so journal entries pointing at LIVE FILES re-emitted duplicates — the file-tracking fix (cr_ext4_engine.cpp walk child emit) is part of this change and pinned by the claim test. All 19 pre-existing ext4 tests stay green (the S-004 semantics — jsb gate, escape-tag restore, orphan flat surface — preserved through the restructure). Suite 473/473 Linux Release (470 + 3) + 473/473 Linux ASan detect_leaks=1; clang-format 18.1.8 clean.
**Resolution notes:** The og's two-pass structure implemented (FUN_180003de0 :2773/:2778): pass 1 (NEW, pre-walk) harvests every journal dir block into a parent map — blocks WITH "." link {owner inode → parent (from "..") + named entries}; blocks WITHOUT "." (torn/unlinked remnants, S-004's core case) contribute owner-less orphan entries. The live walk runs (unchanged except: FILE inodes now enter `visited_inodes` — dirs already did at walk_dir entry; the insert is files-only because inserting dirs there would no-op walk_dir's own cycle-guard insert and SKIP SUBTREES). Pass 2 (the old section-5 emit, restructured) runs the og's orphan finalize: a harvested child the live tree claimed is skipped (FUN_180003fb0 reachability — the live claim wins, OQ-E1 resolved by the structural decode: the reachability check exists precisely to keep live claims authoritative); otherwise it attaches under its owning dir's node — the LIVE parent via the record index (find_by_record) or a SALVAGED dir via the local fixpoint map (chains resolve round-by-round, bounded by dir count + 2); unresolvable chains surface flat under root_lost with their real names. The root inode's props now carry record_ref=2 (the record index answers the root chain — previously 0). S-004's gates are intact in the harvest: the jsb gates the whole pass, the range is the jsb's own s_first/s_maxlen, the ESCAPE restore is descriptor-tag driven (never a zero-byte heuristic — that would corrupt genuinely-zero blocks), every block in range feeds the dir parser. The finding's iNode%I64d expectation for orphan NAMING is corrected per the corpus-wins rule (the og's journal children keep on-disk names; the iNode naming is the deleted-inode sweep's fallback — already implemented).

### Convergence addendum (this session's independent verification + duplicate discard)

A second agent (this closing session) re-verified the same corpus chain
first-hand and built a convergent implementation before discovering the
parallel leg had landed `dcc41b8`. Per the S-012 convergence protocol the
LANDED implementation stands; the duplicate (topological pre-pass with
full-prop emission via a shared emit_inode_node, sweep skip-set,
live_nodes claim checks) is discarded. The converging leg's verification
notes are preserved here:

1. **Corpus decode agreement:** FUN_180003de0 :2773/:2778 (journal pass
   before AND after the root walk); FUN_1800043f0 :3077-3145 ("."/".. head
   -> child/parent pair into the map at +0xdc8, skipping already-known
   children; the "GotJournalDir id=%I64d,parentId:%I64d" trace);
   FUN_1800041e0 :2961-3016 (finalize: map walk, unreachable orphans named
   `iNode%I64d` via snprintf :2986-2991); FUN_180003fb0 (reachability =
   parent chain hitting a live node, the +0x72 bit-2 flag). Both legs
   decoded identical mechanics.
2. **One latent gap both legs found independently:** the pre-fix walk's
   visited set recorded only DIRECTORIES, so deleted-but-still-listed
   FILES double-emitted through the sweep. The landed fix records files
   too (the converging leg did the same via live_nodes).
3. **Superset note (not adopted by the landed fix, recorded for the next
   leg):** the og's journal traversal reads INODES for journal children
   (FUN_1800068b0 - the same walk function the live tree uses), so
   journal-known inodes can carry full extents/times/bitmap grading, not
   name-only PartialRisk nodes. The converging leg's discarded
   implementation proved this end-to-end (its reparented dir carried
   extents + Excellent grading, and the sweep skipped journal-covered
   inodes - no Inode10/Inode11 duplicates). Porting that emission path
   onto the landed structure is a bounded, test-covered follow-up; until
   then journal-covered inodes may also surface as sweep rows
   (name-superset, placement already correct).
