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
