# SEVERE S-034 (NEW-FINDING, DFX30) — ZFS: the og walks EVERY uberblock in the ring (per-txg state recovery); the core reads the newest txg only — files deleted within the ring window are og-visible and core-invisible

**Severity:** SEVERE (missing og-visible recovery content — files the reference
returns are absent from our results on any ZFS pool with deletions inside the
ring's txg window)
**Module:** zfs
**Parameters affected:** B1 (detection correctness — result-set membership), B5 (deleted-file coverage)
**Corpus references:** `ZfsSearchDLL_decompiled.c` — the quick searcher's run
loop FUN_180006b90 (:5312-5918): ring-slot iteration `pvVar18 += 0x400` with
`txg = *(u64*)(slot+0x10)` and `FUN_180003150(ctx, txg, …)` per slot (:5685-5841,
the `\n\ntxg = %llu, last_txg = %llu` progress log built at :5703-5724); the
ring loader FUN_180018930 (:21323+, `FUN_180018220(dev, &buf, 0x20000, 0x20000)`
= read 128 KiB at offset 128 KiB = label-0's uberblock ring, split into 0x400
records); `FUN_180003150` ("addBlock", :1967-2259): per-txg UberData registry
`FUN_180017310` (txg→`std::_Ref_count_obj<VdevLoc::UberData>` map, :20116+),
record match `*(u64*)(rec+0x10) == txg` (:2071), tree-open `FUN_180016c50`
(:2076), success log `"from_txg: 0, birth_txg: <txg>"` (:2162-2170) and the
`from_txg=0` recursive descent `FUN_18001ac60(uberData, 0, cb)` (:2177, :23337,
recursing at :23457); the ZplNode visitor installed at :5812-5815
(`std::_Func_impl_no_alloc<lambda_3cb9c924…, bool, shared_ptr<ZplNode>>`).
**Core references:** `src/core/zfs/cr_zfs_engine.cpp:24-26` (the parity
comment claiming "the og reads the newest txg's live state only" — REFUTED by
the decode above), `:1236-1271` (the ring walk that keeps only `best_txg`);
`AUDIT.md` row 26 (the same refuted premise). Audited at DFX30 session
baseline `2554324`.
**Status:** NEW-FINDING filed by DFX30 during OQ-Z2 verification — first-hand
decompile evidence; fix not landed this push (the og's cross-txg emission and
dedup semantics must be decoded first — see Fix plan).

## Summary

The core's ZFS engine reads ONLY the newest uberblock's live state
(`best_txg`) and justifies it with a parity claim — "the og reads the newest
txg's live state only" — recorded in the engine header comment and AUDIT.md
row 26. First-hand decoding of the og's ZfsSearchDLL quick-searcher run loop
refutes that premise: the og iterates EVERY uberblock slot in the label-0
ring (storage order, 1 KiB slots, 128 per ring) and for each slot's txg
opens a per-txg `VdevLoc::UberData` from a txg-keyed registry, walks that
transaction state's tree, and emits `ZplNode`s through a
`bool(shared_ptr<ZplNode>)` visitor. Consequence: on any pool where files
were created and deleted within the txg window covered by the ring (up to
128 transaction groups), the og returns those files; the core does not — it
surfaces them only via RAW content carving (signature-dependent, no names,
no tree structure, no metadata).

## What the reference does (first-hand decode)

```
quick run loop (FUN_180006b90):
  ring = FUN_180018930(ctx)            // read 128 KiB @ label+0x20000, 1 KiB records
  for (slot = ring.begin; slot != ring.end; slot += 0x400) {
      poll device (Sleep(1000) until ready; abort on state 3/0)
      txg = slot->ub_txg                                     // u64 @ +0x10
      log "\n\ntxg = <txg>, last_txg = <last_txg>"
      FUN_180003150(ctx, txg, …, zplnode_visitor)            // "addBlock"
  }

addBlock (FUN_180003150):
  FUN_180017310(ctx, txg)              // ensure per-txg UberData in the txg→UberData map
  entry = ctx->map[+0x18].find(txg)    // absent → return (zeroed slots no-op)
  for (rec : FUN_180018930(entry), stride 0x400)
      if (rec->txg == txg) {
          out = FUN_180016c50(ctx, &out, rec)   // open this txg's tree (rootbp walk)
          if (out) {
              log "from_txg: 0, birth_txg: <txg>"
              FUN_18001ac60(out, /*from_txg=*/0, cb)   // recursive descent, unbounded
          }
      }
```

The `from_txg = 0` constant (the log's literal and the recursion argument at
:2177) is the top-level "no lower txg bound" — the descent
(FUN_18001ac60 → FUN_18001d9a0, recursing at :23457 with the same param)
propagates it through the dataset/snapshot hierarchy.

## What our core does

`cr_zfs_engine.cpp` reads the whole ring but keeps only the highest-txg
uberblock (`best_txg`, :1236-1271) and walks that single live state. The
header comment (:24-26) and AUDIT.md row 26 assert the og does the same —
the assertion is false.

## Gap

Falsifiable: on a pool with a file created at txg T-k and deleted before the
newest txg T (k ≤ ring depth), the og's result set contains the file (from
the txg T-k state walk); the core's does not. The core's RAW carve may
surface the content only if it matches a known signature.

## Impact

- Missing recovery results on ZFS (the ring window = recent deletions — a
  primary user scenario for a recovery product).
- The engine's stated parity rationale is wrong; the divergence is currently
  undocumented in PARITY-DELTAS (it was believed to be MATCH).

## Reproduction / verification

Corpus-derived (decompile citations above, all read first-hand this session;
brace-depth-annotated control-flow extraction for FUN_180003150). Execution
repro pending the fix (a seeded pool with in-window deletion — the existing
ZFS fixture builder can stamp a second uberblock at an older txg).

## Fix plan

1. Decode the remaining og semantics before implementing (no guessed parity):
   (a) cross-txg emission/dedup — does a file present in N ring txgs emit N
       ZplNodes or 1 (the `ZplNodeFile` ctor FUN_180012450's registration
       path decides; undecoded);
   (b) whether the og's per-txg walks also re-emit the MOS snapshot list
       (dsl_dataset snapshots ZAP) or only the ring states;
   (c) the `last_txg` bookkeeping's role (progress display vs a walk bound).
2. Core change: after the newest-txg walk, iterate the remaining VALID ring
   uberblocks (magic-checked; the ring is already read into memory by the
   geometry probe) oldest→newest or newest→oldest per (1a)'s dedup answer;
   walk each state's tree through the existing emission path with a
   cross-txg identity dedup (object-id + birth txg) so the newest state of
   each object wins and deleted-in-window objects surface as Deleted.
3. Tests: a fixture pool with two uberblocks (txg T-1 with file F, txg T
   without F) → F surfaces (Deleted) post-fix, absent pre-fix (fail-before);
   a dedup pin (file unchanged across txgs emits once); the BE-pool refusal
   and ashift-13 geometry pins must stay green.
4. Update AUDIT.md row 26 + the engine header comment (done at filing — see
   below) and register the divergence window in PARITY-DELTAS until the fix
   lands.

## Related findings

OQ-Z2 (this finding's origin — resolved by it), M-019 (FIXED — the
fletcher4/next-DVA retry machinery the per-txg walks will exercise),
OQ-T6.3 (uberblock geometry superset — the og's hardcoded 1 KiB slots are
the subset), M-023 (ZFS SA decode — shared by every per-txg walk).

## Open questions

The three decode items in Fix plan step 1.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-034 commit, DFX36 — SHA in worklog.md at push)
**Verified by:** `Zfs.RingHistorySurfacesDeletedInWindowFile` (fail-before EXECUTED RED on the pre-fix engine: "deleted-in-window file lost", hello.txt count 0 vs 1 — the fixture stamps a second ring state at txg+1 whose root directory no longer contains hello.txt), `Zfs.RingHistoryUnchangedFileEmitsOnce` (green by design on the pre-fix single-state walk; pins the registry against post-fix regressions — every path exactly once across states), `Zfs.RingHistoryDeleteAndRecreateEmitsBothIds` (fail-before EXECUTED RED: 2 vs 1 — state txg+1 maps hello.txt to a NEW object id 30; both the deleted original (id 10) and the replacement emit, the og's per-path id-set shape). Full suite **905/905** linux-release; the ZFS suite 19/19 under ASan (linux-gcc) and UBSan (linux-ubsan) — the registry's node-reuse map touches ownership-sensitive structures. The fail-before was executed on the stashed pre-fix engine with the new stats assertions temporarily removed (compile-RED without that trim — the ring_states_walked/ring_dedup_skips fields did not exist pre-fix).
**Resolution notes:** CONFIRMED-DIVERGENCE; the fix plan's three decode items were resolved first-hand from the corpus before implementing. **(a) Cross-txg emission/dedup — DECODED:** the visitor lambda (`_Func_impl_no_alloc<lambda_3cb9c92493e028cf5c8b2967fd6ee510>`, its `_Do_call` at vftable RVA 0x309b8 slot+0x10 = FUN_18000dd40, binary-verified in og-codebase/ZfsSearchDLL.dll) delegates every node to the collector FUN_180005940, which maintains a registry keyed by the node's FULL PATH (FUN_18001afb0 = the recursive parent-chain concatenation, parent path + separator + name) holding per-path object-id sets (the identity read at vtable+0x60): a FILE whose id is already registered under its path is DROPPED (one emission per file across the whole ring walk — first storage-order state wins); a FOLDER registered with the same id compares its stored display string (vtable+0x68) and re-emits only on change (no recovery-content effect — the path already carries the name; registered here as the decode residual, the core treats folders by the same (path,id) first-wins rule); emission assigns serial = ++counter and registers {id -> (serial, string)} — so delete+recreate in-window (a NEW id under the same path) emits BOTH. **(b) MOS snapshot re-emission — RESOLVED:** the per-txg tree-open FUN_180016c50 copies the ring record's ub_rootbp (rec+0x28..0xa8) and opens the objset through the same registry chain; the descent (FUN_18001ac60 -> FUN_18001d9a0) recurses type-2 nodes through the dataset/snapshot hierarchy with from_txg=0 unbounded — no separate MOS snapshot-list emission exists in the quick searcher; snapshots present across states dedup through the same (path,id) registry. **(c) last_txg — RESOLVED as cosmetic:** the progress log "\n\ntxg = <slot txg>, last_txg = <value>" formats the current slot's txg decimal and the CONSTANT string "0" (local_84 = 0x30 '0', never updated) — a display-only bookkeeping; addBlock receives only the current txg and the descent receives from_txg=0; no walk bound, no ordering constraint. **The fix:** the engine now collects every valid uberblock from the label rings (labels in order, storage order within each ring, geometry probe unchanged, BE-pool refusal unchanged) and walks EVERY distinct-txg state through the existing MOS -> dataset -> ZPL machinery; emission goes through a (full-path, object-id) first-wins registry with node reuse (later states re-walk every dataset/directory subtree — the og's descent continues through already-emitted type-2 nodes; the collector's registry drop never stops the walk — and attach NEW ids to the EXISTING parents, never duplicates). Dataset nodes dedup by (dataset path, dsl_dir object); their subtree walk continues per state. New telemetry: `ring_states_walked` (states whose MOS->dataset chain opened; a state that fails to open is skipped, never fatal — the og's addBlock returns quietly when the txg's record does not open) and `ring_dedup_skips` (registry drops). stats.txg stays the NEWEST txg across the ring (all existing txg pins preserved: EndToEndWalk, NewestTxgAcrossLabelsWins — both states walk, newest wins the stat, content dedups). **Deliberate deltas registered:** (1) distinct-txg state dedup — the og reads label-0's ring only; the core walks every DISTINCT txg found across all four labels (the OpenZFS syncer writes each uberblock to all four labels' rings at the same slot index, so distinct-txg-once == the og's walk on intact pools while a torn label-0 slot still recovers through labels 1-3 — the I-009 multi-label superset lineage); (2) in-window-deleted files emit as plain nodes under their dataset (the og's flat records carry NO decoded deleted-marker — membership parity is the B1/B5 demand; a future decode of a deleted flag in the record fields can add the marker as a follow-up). D-05 updated from REFUTED-PREMISE to FIXED. No safety hardening touched (BE refusal, 4Kn geometry probe, fletcher4 gate, M-019 fallback all pinned by the existing tests, all green).
