# SEVERE S-029 — Carved files are never marked deleted: og flags every carved record deleted (FLAGS=0x0020); core's deleted-filter and deleted-count exclude the entire carve result set

**Severity:** SEVERE
**Module:** carve (producer) + session/query/tree (consumers)
**Parameters affected:** B4 (deleted-file classification — the largest single mis-classified population), B1 (detection correctness — presentation set), D11 (node fields — deleted state never set on carved nodes), D12 (session persistence — carved nodes round-trip as Active), A29 (merge/dedup — deleted-set membership feeds grouping)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md:82` — CRAWFileProperties (RAW record) field table: "FLAGS — **constant 0x0020 for all 12,147 records** (every carved file flagged deleted)"; `:77-90` the full RAW record grammar; `re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md` (carved files presented under the deleted set — "Other deleted files" grouping)
**Core references:** `src/core/carve/cr_carve.cpp:142-186` (props built and node attached — no deleted-state call), `include/cr/tree/cr_node.h:130` (`set_deleted_state`), `:38` (`DeletedState` enum), `src/core/query/cr_query.cpp:114` (`deleted_only` filter excludes Active), `src/core/tree/cr_node.cpp:287-288` (`refresh_stats` counts `deleted_count` only for non-Active) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

In the og, **every carved file is flagged deleted** — the RAW session record's FLAGS field is
the constant 0x0020 (deleted-family bit) across all 12,147 sampled records, because carving
recovers files that no longer exist in any filesystem structure. Our core's carve path never
calls `set_deleted_state`, so every carved node is `Active`. The consequence is not cosmetic:
the query layer's `deleted_only` filter (the product's primary "show me deleted files" view)
**excludes every carved file**, and the tree's `deleted_count` statistics miss them all.

## What the reference does

The og's CRAWFileProperties serialization (SaveScan part 3, `:77-90`) persists, per carved
file: the analyser-assigned name, a formatted timestamp string, the file size, and FLAGS —
**"constant 0x0020 for all 12,147 records (every carved file flagged deleted)"** (`:82`). The
0x20 bit is the same deleted-family flag used across the node formats (`:16`: "FLAGS (0x10
children / 0x20 deleted family)"). Downstream, the results UI presents carved files inside the
deleted set (the "Other deleted files" branch), and any deleted-scoped filter or counter
includes them.

## What our core does

`cr_carve.cpp:142-186` — the carve emit path builds `FileProperties`, sets
`fs_kind = Raw`, `logical_size`, `carve_format`, `carve_confidence`, the extent, optionally
the sha128, then:

```cpp
tree::FileNode* n = ctx.tree->add_file(ctx.carve_root, props);   // :174
...
n->set_origin(tree::Origin::Carved);                              // :180
n->set_confidence(cr.confidence >= 850 ? ... );                   // :181-182
```

`set_deleted_state` is never called — and a grep of `src/core/carve/` confirms no call site.
`DeletedState` defaults to `Active` (`cr_node.h:38`). Every engine EXCEPT carve calls it
(`cr_mftwalk`, `cr_fat_quick`, `cr_exfat_quick`, `cr_ext4_engine`, `cr_hfsplus_engine`,
`cr_refs_engine`, `cr_records`). The consumers then act on the wrong state:

- `cr_query.cpp:114`: `if (f.deleted_only && n->deleted_state() == tree::DeletedState::Active)`
  → skip — a deleted-only query returns zero carved files.
- `cr_node.cpp:287-288`: `if (c->deleted_state() != DeletedState::Active)
  ++root->stats_.deleted_count;` → carved files never counted as deleted.
- The session writer persists `deleted_` as-is (cr_records), so the divergence survives
  save/load round-trips.

## Gap

Falsifiable claim: scan a disk whose unallocated space contains valid carveable files; in the
og every carved result carries the deleted flag (FLAGS=0x20, 12,147/12,147 corpus census); in
the core every carved node has `deleted_state() == Active`, so (a) a `deleted_only` query
returns no carved files, and (b) `stats().deleted_count` excludes the entire carve result set.

## Impact

The "deleted files" view is the recovery product's core scenario — users who deleted files and
then scanned filter to the deleted set to find them. Carved files (the ONLY recoverable
content after metadata destruction, format, or partition loss) silently vanish from that view.
Statistics shown to the user under-count deleted files by the whole carve population. This is
a wrong-result divergence on the product's primary workflow, not a presentation nit.

## Reproduction / verification

- Corpus: `ifileproperties-part3--btrfs-zfs-raw-generic.md:82` quoted above (first-hand read
  this session; the 12,147/12,147 census is the artifact's own byte-level sample).
- Core: all three cite chains read first-hand this session (carve emit path, query filter,
  stats counter, and the negative grep over `src/core/carve/`).
- Executed comparison not run this session (read-only); the fix leg's fail-before test is
  `Carve.DeletedOnlyQueryIncludesCarvedFiles`.

## Fix plan (for the engineering team)

1. In `cr_carve.cpp`'s emit path (after `add_file`, alongside `set_origin`), add
   `n->set_deleted_state(tree::DeletedState::Deleted);` — matching the og's constant-0x20
   FLAGS for every carved record.
2. Keep the carve root's presentation name ("Other deleted files" branch) unchanged — the
   og groups by the same deleted family bit, so no separate grouping change is needed.
3. Verify no double-count: `refresh_stats` counts non-Active children once; carved nodes are
   direct children of the carve root, so `deleted_count` now includes them (this is the og's
   semantic — the count SHOULD include them).
4. Check the session round-trip: cr_records already serializes `deleted_`, so reloaded
   sessions keep the flag; add a round-trip assertion.
5. Audit the CLI/report surfaces that print deleted counts (`cr_report`) for the same
   under-count and update expectations.

## Tests to add

- `Carve.CarvedNodesAreDeleted` (every emitted carved node has `deleted_state() == Deleted`)
- `Carve.DeletedOnlyQueryIncludesCarvedFiles` (query with `deleted_only=true` over a carved
  fixture returns the carved files — fails before, passes after)
- `Carve.DeletedCountIncludesCarved` (root stats after carve reflect the carved population)
- `Carve.SessionRoundTripPreservesDeletedFlag` (save → load → flag still Deleted)

## Related findings

- S-023 (grading engine — the og's grade bits live at node+0x73 in the same FLAGS-adjacent
  byte family; a fix leg touching node state should sequence with this)
- M-044 (session node-model round-trip gaps — the deleted bit is one of the few flags the
  core DOES carry; this finding closes the producer-side hole)
- N-055 (session load validation deltas)

## Open questions

None. (Whether the og additionally distinguishes "carved-deleted" from "fs-deleted" in any
filter is a query-model question already covered by M-029's state-group decode; the flag
itself is constant 0x20.)

---

## Merged-tree re-verification (session-5 close, after the parallel fix legs' rebase)

The parallel legs' changes (clang-format sweep `591f40c` + the rawsigns regeneration)
shifted cr_carve.cpp and surfaced a SECOND emit site — the raw-sign tail-paired path at
`:330-338` (`add_file` → `set_origin(Carved)` → `set_confidence(PartialRisk |
MetadataOnly)`). Re-verified at the merged tree: **neither emit path calls
`set_deleted_state`** — the grep over `src/core/carve/` is still empty. The finding's
claim is therefore STRONGER at HEAD than at the audited `7cf6754`: both the grammar
layer (:173-182) and the raw-sign layer (:330-338) emit Active carved nodes. The fix
plan's step 1 must patch BOTH sites (or centralize the emit). Audited-`7cf6754` line
cites above remain historically accurate; the merged-tree anchors are :173/:330.
