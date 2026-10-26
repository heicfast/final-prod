# MAJOR M-027 — ReFS object-identity keep-one and recycle-bin special-casing: core emits every name row independently

**Severity:** MAJOR
**Module:** refs
**Parameters affected:** B7 (hard-link handling), B5 (orphan reconstruction), D11 (node fields), A29 (merge and dedup strategy)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — FUN_18000d970 :9727–10079 (identity keep-one :9904–9966, loser destroy :10021–10027, recycle routing :9805–9876, dir node map :10015–10018), FUN_18000e930 :10542–10610 (pending-map duplicate scan), FUN_18000ef20 :10861–10972 (flush + recycle collection :10881–10911, 0x600 root tagging :10914–10933), FUN_18000ba90 :8374–8421 ($I resident read), FUN_1800041f0 :2141–2167 (volume label node)
**Core references:** `src/core/refs/cr_refs_engine.cpp` @ `34b3441` — per-row emission :805–810 (one result per ROW_FILE row), :811–847 (per-row dir links), no identity map, no recycle-bin routing anywhere in the file; `include/cr/refs/cr_refs_engine.h:24–29` (row model doc)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og resolves multiple directory rows pointing at the same object identity (hard links,
duplicate/stale name rows) through a keep-one decision — comparing name/priority fields and
destroying the loser — and routes Recycle-Bin-shaped results (`$R*`, `S-*`,
`System Volume Information`) into a dedicated collection flushed after the main map, reading
`$I` resident metadata alongside. Our core emits one result per `ROW_FILE` row with no
identity dedup (hard-linked objects are recovered N times, once per name row) and has no
Recycle-Bin special-casing.

## What the reference does

(ReFSSearchDLL_decompiled.c)

1. **Identity keep-one** (`FUN_18000d970:9904–9966`): when new file properties carry the
   identity flag (props+0x2c bit0), the og searches the identity map (ctx+0x10, keyed by the
   record's node id, :9905–9934). On a hit it compares the two candidates
   (`FUN_18000ac40` name/priority compare, :9939) and resolves on {+0x43 priority sign, +0x41,
   +0x2f, +0x42} (:9942–9966): exactly one survives; the loser is destroyed
   (:10021–10027 — `vtable+0x27` destroy). Directories additionally tracked by node id in the
   directory map at ctx+0xa8 (:10015–10018).
2. **Pending-duplicate scan** (`FUN_18000e930:10542–10610`): before accepting a new entry the
   pending map is walked for the same identity (returns "already present" → new candidate
   destroyed by the caller, `FUN_18000d970:9996–10019`, `FUN_18000ef20:10934–10945`).
3. **Recycle-bin routing** (`FUN_18000d970:9805–9876`): names beginning **"$R"** (:9824–9829),
   **"S-"** (:9837–9843 — per-SID `$RECYCLE.BIN` subdirectories), or equal to
   **"System Volume Information"** (:9844–9861) are inserted into the dedicated
   collection at ctx+0x40 (`FUN_18000f780` :9866) rather than the main identity map; the
   flush emits them after the main results (`FUN_18000ef20:10881–10911`).
4. **$I metadata read** (`FUN_18000ba90:8374–8421` → `FUN_18000ea30:10615–10856`): the
   `$I…` recycle-bin metadata files get their resident content materialized (original path /
   size / deletion time evidence used by the og family's recycle-bin restore).
5. Root/volume identity: single-char root names harvested from type-0x10 records
   (`FUN_18000cbd0:9213–9247`) tag the root row 0x600 at flush
   (`FUN_18000ef20:10914–10933`); the volume-label node is emitted by the quick path
   (`FUN_1800041f0:2141–2167`).

## What our core does

`cr_refs_engine.cpp:805–810` — every `ROW_FILE` row becomes a `tree::add_file` call under its
parent; there is no object-identity map, no keep-one, and no `$R`/`S-`/SVI routing. A
hard-linked object with N name rows is emitted N times, each with the full extent list
(recovered N times, N times the I/O). `$I` resident content is not read (see S-020). The
root fallback picks a not-a-child table (:888–897) — a different root-identity mechanism than
the og's 0x600-tagged special-name list, but functionally equivalent on single-root volumes.

## Gap

Falsifiable: a ReFS volume with a hard-linked file (two name rows, one object) yields one
result in the og (keep-one) and two duplicate results in ours; a `$RECYCLE.BIN` tree yields
og-special-cased `$R*`/`S-*` nodes (with `$I` metadata read) and plain nodes in ours.

## Impact

- Duplicate results for hard-linked objects: user sees the same file twice under two names
  (both recoverable — not data loss, but wrong result-set shape and doubled recovery I/O).
- Recycle-bin restore evidence (`$I` original paths) is absent, degrading any restore-rename
  feature parity the writer/UI layer assumes (og family reads it; NTFS side of our product
  already models recycle-bin semantics — see N-011's family).
- Merge/dedup strategy (A29) divergence compounds with M-026: stale-page records and live
  records for the same object are deduped by the og; ours would double-emit once the sweep
  exists.

## Reproduction / verification

Decompiled evidence (strength 2) — all citations above; inventory §6.6. Core read first-hand.
Deterministic from the core code: no identity map exists (grep: no such structure in
cr_refs_engine.cpp). No ReFS hard-link fixture exists in the corpus (vendor QA scanned none);
synthetic double-row fixture can be built from the core's own test scaffolding
(tests/unit/test_refs_volume.cpp) for the acceptance test below.

## Fix plan (for the engineering team)

1. Add an object-identity map to `scan_refs` keyed by the row's object identity (the thesis
   row model's object id in the file-row value; equivalent of the og's node id props+0x3a):
   `{identity → emitted node}`. On duplicate: keep-one by a deterministic rule mirroring the
   og (prefer non-deleted; then first-seen name; og priority fields decode: +0x43 sign,
   +0x41/+0x2f flags — implement "first wins with live-over-deleted preference" and document
   the simplification), destroy the loser before `tree::add_file`.
2. Route `$RECYCLE.BIN` content: rows whose parent chain is a recycle directory (names
   `$R*`, `S-*` under `$RECYCLE.BIN`, `System Volume Information`) — mark the nodes with a
   recycle-bin flag consumed by the writer/report layer (parity with the og's separate
   collection ordering: emit main results first, recycle collection after).
3. Read `$I` resident metadata (depends on S-020 step 5) and attach
   {original path, deletion size/time} to the sibling `$R` node.
4. Keep the 0x600 fixed root + not-a-child fallback (matches og behavior on single-root
   volumes; the og's special-name 0x600 tagging is an identity detail, not a behavior gap).
5. Acceptance: synthetic fixture with two name rows for one object → exactly one result;
   `$RECYCLE.BIN/S-1-5-19/$R123456.txt` + `$I123456.txt` → $R node flagged, $I evidence
   attached.

## Tests to add

- `Refs_HardLink_Single_Result` (two ROW_FILE rows, same object identity → one node).
- `Refs_Recycle_Routed_And_Flagged` ($R/$I pair → recycle flag + original-path evidence).
- `Refs_Recycle_Emitted_After_Main` (result ordering: main map before recycle collection).
- `Refs_Identity_Dedup_Live_Over_Deleted` (live + stale row for one object → live wins).

## Related findings

- S-020 (attribute layer — `$I` resident read is implemented there; this finding consumes it).
- M-026 (stale-page sweep — its records must flow through this finding's identity map).
- M-007 (NTFS hard-link fan-out — the NTFS-side analogue of the same B7/A29 divergence).

## Open questions

- The og's exact keep-one tie-break field semantics (+0x43/+0x41/+0x2f/+0x42) are shape-decoded
  but not name-decoded; the proposed deterministic rule is a documented simplification. Next
  artifact: `FUN_18000ac40` body (:7513) + a two-row fixture comparison if the bench stage
  produces one.

## Resolution

**Status:** FIXED (with one documented keying simplification the finding itself proposed)
**Fixed in commit:** `7feb79b`
**Verified by:** Refs.HardLinkRowsCollapseToOneResult (fail-before FAILED — two rows emitted; pass-after: identity_dedups 1, exactly one result), Refs.RecycleRoutedFlaggedAndDollarIRemap (fail-before FAILED; pass-after: "$R…" routed to the post-main flush, $I evidence read, the $R node remapped to its ORIGINAL name/size/time and marked Deleted; the $I row itself stays a main-map result), Refs.StaleSweepSkipsLivePages + StaleMsbPage (the sweep-found rows participate in the same identity map — live wins over stale). Suite 429/429.
**Resolution notes:** (1) Identity keep-one: a {first-extent offset, total extent bytes, logical size, created, modified} placement signature keys the map — the finding's own proposed rule ("first wins with live-over-deleted preference", documented simplification) because the og keys on the record's node id (props+0x3a; verified first-hand at the :9905-34 map lookup) which the core's synthetic row model does not carry; rows without extents skip dedup (no identity signal). Live-over-deleted promotion reparents + un-deletes the existing node when a live row arrives for a previously-deleted-emitted object. The og's tie-break fields (+0x43/+0x41/+0x2f/+0x42, re-read at :9929-66) remain shape-decoded; the simplification is recorded per the finding's fix plan. (2) Recycle routing verified first-hand (:9817-61 re-read): "$R*" / "S-*" / "System Volume Information" stage for the post-main flush (NOT through the identity map — the og inserts them into the dedicated collection before the identity check); flush order = main results first. (3) $I evidence: the "$I" rows' resident content (materialized by the S-020 landing) is read at scan time and parsed (the shell-family $I layout: version, ORIGINAL size, deletion time, UTF-16 path); the $R sibling pairs by token (marker-stripped, cut at '.') and is remapped to the original name/size/time + Deleted state — the core-family convention the NTFS engine already models; the og's SearchDLL leaves the rename to its UI layer, so the evidence READ is the parity part and the remap is the documented family-consistency addition. The fixture note: the NextChainFindsNewerTable fixture was corrected in the same change (two live files shared one cluster — physically impossible, and adversarial to identity dedup by construction; the fixture's purpose is chain selection).
