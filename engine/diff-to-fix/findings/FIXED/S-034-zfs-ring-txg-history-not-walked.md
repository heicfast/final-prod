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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-034 commit — the commit carrying cr_zfs_engine.cpp's ring walk, the header fields, and the Zfs.* ring tests)
**Verified by:** Zfs.RingWalkSurfacesDeletedInWindowFile + Zfs.RingWalkRenameReemitsNewLocation + the extended Zfs.NewestTxgAcrossLabelsWins (all three EXECUTED RED on the pre-fix engine — engine.cpp reverted to HEAD with the new tests kept: gone.txt count 0 "in-window deletion lost", renamed.txt count 0, ring_states_walked 0 — and GREEN after the fix); the full suite 828 passed / 4 env-skips on linux-release, 832/832-clean under ASan and UBSan (both lanes exit 0).
**Resolution notes:** All three fix-plan decode items were resolved first-hand from the corpus before implementing (no guessed parity). (a) Cross-txg emission/dedup: the visitor (lambda_3cb9c924's `_Do_call` at 0x18000dd40 → FUN_180005940) keeps a scan-wide registry — per-parent-full-path buckets (FUN_180007f50 on the map at ctx+0x110; the path built RECURSIVELY by FUN_18001afb0 up the ZplNode chain), objid-keyed (the u64 from node vt+0x60; the set/map helpers FUN_180007db0/FUN_180007eb0); same (objid, path) → SKIP (children still descend under the first-seen node via the registered result-id linkage), same objid + different path → RE-EMIT under the new location with the registry entry OVERWRITTEN (FUN_180001d30 + `*puVar8 = id`), so a moved/renamed object surfaces under both its first-seen and its new location. (b) Per-txg walks re-walk the ENTIRE config as of each state (the descent re-reads that txg's dataset/snapshot hierarchy); datasets and folders dedup through the same (path, objid) registry — a dataset present in N ring states emits once (first-seen), an in-window-destroyed dataset surfaces from the older state's walk. (c) `last_txg` is progress-display only — the run loop's string formatting of the PREVIOUS slot's txg (the "\n\ntxg = %llu, last_txg = %llu" log at :5703-5724), not a walk bound. Two additional behaviors decoded during verification and implemented: the emit chain's regular-add path SUPPRESSES children of `.Trash-1000/info` (FUN_180004f00 :3708 builds L".Trash-1000/info", the normalized path-compare at :3716 short-circuits the add) while the `.Trash-1000/files` subtree is classified-not-suppressed (FUN_180004ad0 :3447 returns TRUE — the file still adds; verified at the disassembly level: `mov $0x1,%al` at 0x180004dbb on the equal path); and the run loop reads ONLY label-0's ring (FUN_180018220(dev,&buf,0x20000,0x20000) = 128 KiB @ +128 KiB) — the core keeps its 4-label + shift-probe geometry as the REGISTERED SUPERSET (D-04/I-017 lineage), walking label 0 first for og-parity storage order and the tail labels after, each UNIQUE txg once (the og's per-txg UberData registry keys txg). The core implementation (cr_zfs_engine.cpp): every valid ring slot collected in storage order into a RingSlot vector (txg-deduped), per-state `walk_state` (MOS → dataset hierarchy → ZPL) emitting through a scan-wide registry keyed (dataset-name bucket → objid → {dataset-relative path, first-seen node}), dataset nodes claimed per (name, head_obj) — a destroyed-and-recreated same-name dataset emits a second node, matching the per-objid registry; a state whose tree does not open is skipped (og addBlock's tree-open-failure no-op) and the scan fails only when NO state opens. In-window deletions emit as REGULAR results — the og has no per-engine deleted bit for ZFS (its ring walk IS the deleted-content surface), correcting the fix plan's "surface as Deleted" guess. New stats fields ring_states_total/ring_states_walked (trailing, aggregate-init safe). Two fixture corrections made during test construction, both verified against the fixture's own shapes: SA-bonus dnodes use bonustype 44 (DMU_OT_SA) with the bonus after blkptr 0; the per-txg "zfs: txg = %llu, last_txg = %llu" progression rides the N-048 log seam. The finding's AUDIT.md-row-26 citation is CORRECTED in passing: row 26 (the decompression/indirect/BPE row) does NOT carry the newest-only premise — the premise lived in the engine header comment (updated), D-05 (re-verdicted below), and modules/zfs.md B1/B5 (updated). PARITY-DELTAS D-05 re-verdicted CLOSED (all three ledger copies updated; the ledger's own triplication is filed as a separate NEW-FINDING). No finding-adjacent safety hardening was touched: BE-pool refusal and the fletcher4 gates stay exactly as landed (M-019); the checksum-drop/foreign-vdev counters now accumulate across ALL walked states.
