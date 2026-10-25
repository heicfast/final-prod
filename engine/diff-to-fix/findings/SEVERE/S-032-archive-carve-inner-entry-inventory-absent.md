# SEVERE S-032 — Archive carve emits one flat node: the og reconstructs ZIP/RAR inner-entry inventories for the results tree

**Severity:** SEVERE
**Module:** carve
**Parameters affected:** B1 (detection correctness — archive contents), B2 (recovery correctness granularity), D11 (node fields — no inner-entry nodes), A27 (fragment handling — multi-part RAR stitching), A28 (carving validator logic — entry-table reconstruction)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--zip-analyser-carving.md:29-38` (the carving loop, per-entry parser FUN_180045690, entry-name merge FUN_180007060: "Entry names accumulate into a list … **the analyser returns the complete inner file inventory, which the UI later presents as the archive's contents**"), `:13-21` (all three ZIP anchors accepted: LFH / CD entry / EOCD); `analyser-granular-batch1--pst-rar-mkv-heic.md:36-41` (RAR4 walk FUN_180012d00 carries a **272-byte filename buffer** — "the archive-entry iterator extracts entry names (**multi-part archive stitching + entry-table reconstruction for the results tree**)"; RAR5 signature note)
**Core references:** `src/core/carve/cr_analysers.cpp` (validate_zip / validate_rar4 — size reconstruction only), `src/core/carve/cr_carve.cpp:142-186` (one FileNode per archive hit; no inner-entry emission), `src/core/carve/cr_registry.cpp` (FMT_ZIP / FMT_RAR4 / FMT_RAR5 rows) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ZIP and RAR analysers do not stop at "found an archive, computed its size" — they walk
the internal entry structures and **return the complete inner file inventory** (names, sizes,
offsets), which the results tree presents as the archive's contents, and the RAR walker
additionally stitches multi-part archives. Our core's carve emits exactly one flat node per
archive hit. Users who carved an archive cannot see or target its contents in our results the
way og users can.

## What the reference does

**ZIP** (`rawsearchdll--zip-analyser-carving.md:23-38`): the carving loop accepts any of the
three ZIP structures as the anchor (LFH `PK\x03\x04` with `version_needed ≤ 10` and
extra/flags ∈ [1, 0x102]; Central-Directory entry `PK\x01\x02` — the *preferred* anchor;
EOCD `PK\x05\x06` for truncated archives), then per structure:

```
if structure == PK0102 (CD entry):  parse entry (FUN_180045690)
elif structure == PK0304 (LFH):     sanity-check fields; parse entry
elif structure == PK0506 (EOCD):    parse; continue
entries.append(name, sizes, offsets)      # name len +0x1c, extra +0x1e, comment +0x20 (CD)
candidate += entry_size                   # exact parsed size
```

"Entry names accumulate into a list (local_68/local_a8 vectors, FUN_180007060 = merge) — the
analyser returns the complete inner file inventory, which the UI later presents as the
archive's contents" (`:38`). The final carve size is computed from the SUM of parsed entries
(`:37`).

**RAR** (`analyser-granular-batch1:36-41`): the RAR4 fast validator (version bounds
`[+0x1B]=7`, `[+0x22]=7`, flags `[+0x23]=0x40000000`, `[+0x24 u16]=0x101`, type-ID 0x3A) walks
with FUN_180012d00 carrying a 272-byte filename buffer — "the archive-entry iterator extracts
entry names (multi-part archive stitching + entry-table reconstruction for the results tree)".
RAR5 (`Rar!\x1A\x07\x01\x00`) rides the 4CC/fallback tables separately.

## What our core does

`cr_analysers.cpp`'s `validate_zip` / `validate_rar4` compute a validated size (the ZIP
validator walks structures to bound the archive; the RAR validators bound via headers), and
`cr_carve.cpp:142-186` emits **one** `FileNode` with `props->carve_format = sig.id`,
`logical_size = file_len`, a single extent. Nothing reads entry names, sizes, or offsets; no
child nodes are created; no multi-part stitching exists anywhere in the carve module.

## Gap

Falsifiable claim: carve a region containing a ZIP with N inner files (or a multi-part RAR
set); the og's results tree shows the archive with its N-entry inventory (and stitches the RAR
parts); the core's results show one opaque node with no inner listing.

## Impact

Users recovering archived data (one of the most common deep-scan scenarios — deleted archives
in unallocated space) lose all pre-recovery visibility into archive contents: they cannot
browse, filter, or selectively recover inner files, and cannot see whether the archive holds
what they want before recovering it. Multi-part RAR sets recovered as independent fragments
lose the og's stitching. This is a user-observable missing capability in the carve results
model, on par with the S-023 grading surface.

## Reproduction / verification

- Corpus: both artifacts quoted above, read first-hand this session (ZIP loop + per-entry
  parser + merge function + "the UI later presents as the archive's contents"; RAR 272-byte
  walk buffer + "entry-table reconstruction for the results tree").
- Core: carve emit path read first-hand (single-node emission, no entry parsing beyond size
  bounding); negative grep for any entry-name/inner-file surface in `src/core/carve/`.
- Executed comparison not run this session; the fix leg's fail-before test is
  `Carve.ZipInnerInventoryEmitted`.

## Fix plan (for the engineering team)

1. Extend `validate_zip` to optionally return the entry list it already walks (name, packed/
   unpacked size, offset per entry; cap the list at a sane bound, e.g. 10,000 entries, with
   an honesty flag when truncated).
2. Extend `validate_rar4`/`validate_rar5` walks to extract entry names (272-byte buffer in
   the og = max name length) and detect multi-part sets (volume flags + naming), stitching
   part sizes into one logical node with per-part extents.
3. Add `CarveInventory { entries: vector<{name, packed, unpacked, offset}>, truncated }` to
   the carve result and stash it on `FileProperties` (new optional field — session schema
   append-only).
4. In `cr_carve.cpp`, emit the archive node with the inventory attached (NOT as child nodes —
   the og presents it in the results tree via the UI layer reading the inventory; keep the
   same shape so query/report can project it).
5. Expose the inventory through the tree --json / report surfaces (append-only field).
6. Session round-trip: serialize the inventory (bounded; drop-with-flag on overflow to keep
   sessions small).

## Tests to add

- `Carve.ZipInnerInventoryEmitted` (synthetic ZIP with 3 named entries → inventory of 3)
- `Carve.ZipCdAnchorInventory` (archive whose first surviving structure is a CD entry)
- `Carve.Rar4EntryNamesAndMultipartStitching` (2-part RAR set → one node, stitched extents)
- `Carve.InventorySessionRoundTrip` (save → load → inventory intact)
- `Carve.InventoryTruncationHonestyFlag` (over-cap archive → truncated=true)

## Related findings

- S-031 (carved deleted flag — same emit path; land together)
- M-051 (RAW provenance fields — same CRAWFileProperties grammar carries the og's archive
  note fields; the inventory work should sequence with the provenance carrier)
- S-007 (missing formats — the entry-inventory work per format should follow the og's
  analyser roster)

## Open questions

The og's exact presentation of the inventory in the results tree (expandable children vs
preview pane) is UI-layer; the engine contract (analyser returns the inventory) is what this
finding pins. The corpus's UIUX artifacts (09-results-phase) describe presentation only at
the window level — no contradiction found.

---

## Session-6 convergence addendum (2026-10-26 — independent corroboration + three deltas)

This session independently filed the same divergence (local M-042, dropped in the
ID reconciliation) from the same artifact, and adds:

1. **The results-tree consumer citation** (the demand side):
   `rawsearchdll--zip-analyser-carving.md:64` — "Entry list = merged into result
   via FUN_1800173f0 — consumed by the results-tree builder (**ZIP files expand
   to their inner file list in the DRWUI tree**)". The inventory is not merely
   returned; the og's tree BUILDS with it.
2. **The size-from-sum rule**: the per-entry parser accumulates exact total size
   into `local_f0` — "the final carve size is computed from the SUM of parsed
   entries, not guessed" (zip-carve:37) — a sharper size bound than tail-walk
   heuristics for structurally-truncated archives; worth folding into this
   finding's fix (the inventory walk already computes it).
3. **Two fix-plan additions:** a hostile-entry cap (10k entries, the S-015
   lesson — hostile archives must not OOM the payload), and the
   nested-inventory open question (ZIP-in-ZIP depth: corpus-evidenced depth 1
   only — implement depth 1, track deeper as UNVERIFIED).

Core-side re-read at `022c81d`-lineage: `CarveResult` remains {end,size} —
unchanged; this finding stands as filed.
