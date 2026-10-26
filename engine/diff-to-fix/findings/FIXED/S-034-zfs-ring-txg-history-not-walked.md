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
**Fixed in commit:** (this session's S-034 commit — `git log` "fix(S-034)")
**Verified by:** `Zfs.RingWindowDeletedFileSurfaces` (fail-before RED EXECUTED on the pre-fix tree: `ring_states_walked 0 vs 2` + `tank/GONE.TXT must surface exactly once` — gone_count 0), `Zfs.RingUnchangedObjectEmittedOnce` (the cross-txg identity-dedup pin: hello.txt, present in both states with an identical dnode, emits exactly once), `Zfs.RingWalkSkippedWithoutDeletedRoot` (the pre-S-034 call shape — root_deleted=nullptr — stays supported, live walk unchanged, no crash); full suite **905/905** linux-gcc (ASan) incl. win_syntax_check; all 18 pre-existing ZFS tests green (BE refusal, ashift-13 geometry, foreign-vdev fallback, A-008 dataset enumeration pins unchanged).
**Resolution notes:** CONFIRMED-DIVERGENCE, independently re-verified first-hand this session before any change: the quick-searcher run loop (FUN_180006b90 :5685-5841) iterates every 1KiB ring slot, reads txg at slot+0x10, and calls FUN_180003150 per slot with the ZplNode visitor lambda (:5812-5841); addBlock's txg->UberData registry (FUN_180017310), the record-txg match at +0x10 (:2071), the tree-open (:2076), and the from_txg=0 unbounded descent (:2177) all re-read verbatim. The fix plan's three decode items were answered from the corpus BEFORE implementation: (1a) cross-txg emission has NO engine-side dedup — the ZplNodeFile ctor FUN_180012450 is a plain refcounted constructor with no registration map, and the visitor (lambda_3cb9c924, capturing the controller) bulk-collects; a file present in N states emits N ZplNodes — the core implements the equivalent presentation with a cross-state identity dedup (dataset head object + object id + the dnode's first block pointer's LOGICAL birth txg, blkptr+80), so the newest state's copy wins and older-only objects surface as Deleted (REGISTERED divergence: one emission instead of N — PARITY-DELTAS row this commit; a superseded incarnation of a re-created object id also surfaces as deleted history, matching the og's per-state content set); (1b) the per-txg walks re-walk each state's own dataset/snapshot hierarchy through the same descent — no separate MOS-snapshot-list step exists in the quick searcher, so the core's per-state dataset walk is the faithful model; (1c) `last_txg` is DISPLAY-ONLY — its sole occurrence in the entire binary is the progress-string append at :5703 (`", last_txg = "`), consumed by nothing. Core change (cr_zfs_engine.cpp): the geometry probe now collects EVERY valid uberblock into a txg-keyed map (the registry's semantics; a txg mirrored across labels walks once — a deliberate deviation from the og's duplicate walk of the same txg in every label, registered with the dedup row); the per-state walk (MOS -> dataset hierarchy -> per-dataset ZPL) is factored into `walk_state(ub, live)`; the newest state walks live (existing behavior preserved bit-for-bit: same stats fields, same progress stages), then every older state walks newest->oldest into the deleted tree: deleted-tree dataset/dir nodes are cached by name/(head,obj) so older states' content merges under one shadow chain, dirs that exist in a newer state are live containers (unmarked), dirs absent from all newer states are marked Deleted, files dedup by the full identity key and mark `DeletedState::Deleted`. New stats `ring_states_walked` + `ring_deleted_recovered` (append-only struct fields; the two internal consumers recompile cleanly). `root_deleted == nullptr` skips the old-state walks (the pre-S-034 call shape stays supported). The engine header's deleted-file story is rewritten from "REGISTERED GAP" to the fixed design. Fixture: `build_ring_pool()` — a second complete pool state at txg 76 sharing every unchanged block with the live state (the way a real pool shares unmodified blocks across txgs), root ZAP carrying the extra GONE.TXT (object 30); the first cut placed the state-2 root ZAP inside the 16K dnode-array footprint (B_ROOTDIR_ZAP2 0xB8000 within B_ZPL_DNODES2 0xB6000+0x4000) — the array copy clobbered the ZAP and the walk died at the first read_zap; placement fixed past the array end (0xBA000+), a fixture-authoring lesson recorded here for the audit trail. AUDIT.md row 26 and PARITY-CORE-FS2's row corrected with this landing. Safety hardenings preserved: BE-pool refusal (ERR_UNSUPPORTED), the 4Kn/ashift-13 geometry probe, the M-019 foreign-vdev fallback + checksum gates all re-verified green on the new walk paths (the per-state walks exercise them N times).
