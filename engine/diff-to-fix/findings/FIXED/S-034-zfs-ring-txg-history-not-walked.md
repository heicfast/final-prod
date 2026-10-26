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
**Fixed in commit:** this landing (DFX33b; the S-034 lane — engine `src/core/zfs/cr_zfs_engine.cpp`, header `include/cr/zfs/cr_zfs_engine.h`, tests `tests/unit/test_zfs_volume.cpp`)
**Verified by:** `Zfs.RingWindowDeletedFileRecalled`, `Zfs.RingWindowUnchangedEntriesEmitOnce`, `Zfs.RingWindowModifiedFilePriorVersionRecalled`, `Zfs.RingTornNewestStateFallsBackToOlder` — all four executed fail-before RED on the pre-fix engine (best_txg-only) and pass-after; full suite 903/903 green post-merge (linux-release + ctest incl. win_syntax_check + cr_ui_abi_check).
**Resolution notes:** The three decode items in Fix plan step 1 were answered first-hand from the corpus before implementing: **(1) cross-txg emission/dedup** — the quick searcher's collector (FUN_180006b40, the lambda body behind the `_Func_impl_no_alloc<lambda_3cb9c924…,bool,shared_ptr<ZplNode>>` vtable) is a plain `push_back` into the caller's vector with NO engine-side dedup, and the walk's only emission gate (FUN_18001d9a0 :26210) is `from_txg < birth_txg || from_txg == 0` — always true at the top level because addBlock descends with from_txg=0 (the literal 0 at FUN_180003150's FUN_18001ac60 call, propagated by both recursions at :23457/:26307); so every ring state's ZplNodes reach the host, and dedup is host-side tree semantics. **(2) MOS snapshot re-emission** — each addBlock opens the txg's own state from its matching ring record (record match `*(u64*)(rec+0x10) == txg`, tree-open FUN_180016c50 which itself re-ensures the registry when the record's txg differs from the current one) and descends that state's dataset/ZPL tree; there is no separate MOS snapshot-list re-emission — the per-state dataset hierarchy IS the state's content. **(3) `last_txg`** — display-only: the run loop builds the `"\n\ntxg = <txg>, last_txg = <last_txg>"` progress string from the PREVIOUS iteration's txg; no skip/bound consults it. The fix implements the decode: the engine collects every valid uberblock state from all four labels' rings (the established geometry superset; the og reads label-0's ring only), txg-dedupes (a closed txg's rootbp is 1:1 per vdev_label.c; the og's same-txg duplicate slots are the same state and its re-walk is result-neutral), sorts newest-first, and walks: the first state whose MOS chain opens is the LIVE tree; each older state is a RECALL walk emitting only entries whose cross-state identity — (dataset, parent dir object, entry name, object id, dnode bp0 birth), the finding's object-id+birth key extended with the entry path so hard links are not collapsed — no newer state claimed: deleted-in-window files surface as Deleted nodes in place (an unchanged directory is re-walked per state against its EXISTING node so deleted-in-window descendants surface under it), and modified-in-window entries recall the prior version as a Deleted sibling (the og emits every state's node; newest-wins is a tree-level consequence, not an engine-side drop). Torn states (MOS chain fails to open) are skipped exactly like the og's FUN_180016c50 tree-open failure — the ring walk continues, and if the newest state is torn the next one serves as live (pinned by the fallback test). Datasets are find-or-created by name (a dataset only an older state knows surfaces as Deleted). New telemetry: `ZfsScanStats::ring_states`. The identity dedup is a deliberate improvement over the og's raw N-per-state emission (a recovery result tree cannot hold N duplicate paths; the og's dedup lives host-side) — registered in PARITY-DELTAS D-05's resolution note. D-05's verdict updated to FIXED; the D-05/D-06-family merge-storm triplicates in PARITY-DELTAS.md were deduplicated in the same landing (content-identical rows dropped, divergent rows reconciled keeping the extended body — 99 D-rows remain, one per number). No safety hardening touched (BE-pool refusal and ashift-13 geometry probes stay green — the existing pins). Corpus citations: ZfsSearchDLL_decompiled.c :5312-5918 (run loop), :1967-2259 (addBlock), :20085-20340 (UberData registry), :26173-26320 (emission walk + gate), :5280-5311 (collector), :19758-19790 (tree-open re-ensure).
