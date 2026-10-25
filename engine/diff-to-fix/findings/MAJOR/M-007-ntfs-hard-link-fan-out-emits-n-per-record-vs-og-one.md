# MAJOR M-007 — Hard-link fan-out: og emits ONE node per record, core emits N (result-set superset)

**Severity:** MAJOR
**Module:** ntfs
**Parameters affected:** B1 (detection correctness — result-set membership), B7 (hard-link/ADS/sparse), D11 (node fields), D15 (confidence grading — every hard-link child duplicates grading), A29 (merge/dedup — no per-record dedup), X52 (scalability — N hard-links → N nodes)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:5221-5273` (`FUN_180006db0` calls `FUN_1800448b0` exactly ONCE per record at `:5273`); `:54124-54162` (`FUN_1800448b0` fetches at most TWO `$FILE_NAME` attributes via `FUN_1800430b0(param_1, 0x30, 0, ...)` index 0 and `:54127` index 1, then picks ONE by namespace ranking via `FUN_180043330`); `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md` §3 (the "reference-counted property objects" prose — actually a misread of the viability+destroy block, see S-001 finding's errata section).
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:711-724` (collect distinct parents from the full `$FILE_NAME` list, built via `enum_attributes` → `parse_file_name` in `cr_attributes.cpp:84-89, 131-161`); `:796-841` (emit ONE node per distinct parent, cloning props for each parent beyond the first via `props->clone()` at `:807`, link name chosen per-parent at `:798-804`); `tests/unit/test_ntfs_volume.cpp:1314` (`EndToEndWalk` asserts `stats.hardlinks == 1u` on record 21 with 2 `$FILE_NAME` entries with parents 5 and 16 — the og would emit 1 node here, the core emits 2) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's `NTFSSearchDLL` scans each NTFS MFT record exactly once and
emits ONE result-tree node per record — picking the namespace-best
`$FILE_NAME` (POSIX > Win32-and-DOS > Win32 > DOS) and the namespace-best
parent. The core's `cr_mftwalk.cpp` collects ALL distinct parents from
the full `$FILE_NAME` list and emits ONE node PER distinct parent — so
an NTFS file with 3 hard-links (3 distinct `$FILE_NAME` entries with 3
distinct parents) appears 3 times in the core's result tree and once
in the og's. The core's behavior is MORE useful (users see the file
under every directory that links to it), but every comparative inventory
diverges mechanically.

Session-1's NTFS B7 verdict "M one node per distinct parent with shared
refcounted props (mirrors og LOCK/unlock AddRef/Release sharing)" is
REFUTED — the cited "LOCK/unlock at 21170-21205" lines are the
viability+destroy block (verified first-hand in S-001 finding's errata
section), not hard-link refcount management.

## What the reference does

The og's per-record builder `FUN_180006db0` (the "build props" function)
calls `FUN_1800448b0` exactly ONCE per record:

```c
// NTFSSearchDLL_decompiled.c:5221-5273 (FUN_180006db0)
//   ... parse $STANDARD_INFORMATION ...
//   ... parse $FILE_NAME list ...
lVar7 = FUN_1800448b0(param_2);                          // :5273  build props (ONE call)
//   ... emit into result tree ...

// NTFSSearchDLL_decompiled.c:54124-54162 (FUN_1800448b0)
//   Fetches at most TWO $FILE_NAME attributes:
//     fn0 = FUN_1800430b0(param_1, 0x30, 0, NULL, NULL);  // index 0 (first $FILE_NAME)
//     fn1 = FUN_1800430b0(param_1, 0x30, 1, NULL, NULL);  // index 1 (second $FILE_NAME)
//   Picks ONE by namespace ranking (FUN_180043330 returns the namespace):
//     if (ns0 == POSIX || (ns0 == WIN32_AND_DOS && ns1 != POSIX)) use fn0;
//     else if (ns1 == POSIX || ns1 == WIN32_AND_DOS) use fn1;
//     else use fn0;
//   Returns ONE name + ONE parent_ref + ONE set of times.
```

The og never iterates beyond index 1. A record with 3+ `$FILE_NAME`
entries (3+ hard-links) yields ONE emitted node carrying the first or
second name (whichever wins the namespace ranking). Subsequent hard-links
are silently dropped from the result tree.

The `CNTFSFileProperties` object DOES use `std::shared_ptr`-style
refcounting (`AddRef`/`Release` at `:21187` `(**(code **)(*plVar10 +
0x138))(plVar10, 1)` and `:21257`/`:21260`), but this is for
in-process lifetime management of the props object — not for emitting
multiple nodes per record. The og's result tree has exactly one node
per MFT record processed.

## What our core does

```cpp
// src/core/ntfs/cr_mftwalk.cpp:711-724 (audited 5d6da5f)
// Collect distinct parents from the FULL $FILE_NAME list (built by
// enum_attributes → parse_file_name in cr_attributes.cpp:84-89, 131-161,
// which iterates ALL $FILE_NAME entries, not just the first two).
std::vector<const FileFileName*> best_per_parent;
for (const auto* fn : file_names) {
    auto it = std::find_if(best_per_parent.begin(), best_per_parent.end(),
                           [&](const FileFileName* x) { return x->parent_record == fn->parent_record; });
    if (it == best_per_parent.end()) {
        best_per_parent.push_back(fn);          // first $FILE_NAME for this parent
    } else {
        // Per-parent namespace ranking: replace if higher
        if (rank(fn) > rank(*it)) *it = fn;
    }
}

// src/core/ntfs/cr_mftwalk.cpp:796-841 (audited 5d6da5f)
for (size_t i = 0; i < best_per_parent.size(); ++i) {
    const FileFileName* best = best_per_parent[i];
    FileProperties* sp;
    if (i == 0) {
        sp = props;                                 // first link: use the props we built
    } else {
        sp = props->clone();                        // subsequent links: clone (shared extents, copied name)
        sp->name = best->name;                      // per-parent name
        sp->parent_record = best->parent_record;
        ++stats.hardlinks;
    }
    // ... emit node into result tree under best->parent_record ...
}
```

The core iterates ALL distinct parents and emits one node per parent.
An NTFS file with 3 hard-links yields 3 nodes in the result tree (each
under its own parent directory), all sharing the same `FileProperties`
(extents, times, attributes) via `clone()`.

## Gap

The og emits 1 node per MFT record (the namespace-best name under the
namespace-best parent). The core emits N nodes (one per distinct parent,
each carrying the namespace-best name for THAT parent). For a file with
no hard-links (the common case, single `$FILE_NAME`), both sides emit 1
node — no divergence. For a file with N hard-links, the og emits 1 node
and the core emits N nodes — N-1 extra nodes in the core's result tree.

## Impact

- **Result-set membership:** the core's result set is a strict SUPERSET
  of the og's for any volume with hard-links. Comparative inventories
  diverge by N-1 per hard-linked file.
- **User experience:** the core is MORE useful — users see the file
  under every directory that links to it. The og's behavior is a
  limitation, not a deliberate choice.
- **Recovery correctness:** no impact — the file's content is correctly
  recovered under whichever parent the user picks. The hard-link
  relationship is preserved (the same `clone()`'d props point to the
  same extents).
- **Parity benchmark:** any og-vs-core file count comparison fails on
  volumes with hard-links.
- **Session size:** the core's `.crsf` is larger than the og's `.rsf`
  for the same volume (N nodes vs 1 per hard-linked file).

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `NTFSSearchDLL_decompiled.c:5221-5273` and `:54124-54162` (the
  `FUN_180006db0` → `FUN_1800448b0` chain — one call, two-`$FILE_NAME`
  fetch, one namespace-best pick).
- **Core evidence:** first-hand read of `cr_mftwalk.cpp:711-724` and
  `:796-841` (distinct-parent collection and per-parent emission
  loop). The `stats.hardlinks` counter at `:807` documents that the
  core is aware of the divergence.
- **Test evidence:** `tests/unit/test_ntfs_volume.cpp:1314` `EndToEndWalk`
  asserts `stats.hardlinks == 1u` on record 21 (2 `$FILE_NAME` entries
  with parents 5 and 16). The og would emit 1 node here; the core
  emits 2 (`hard@root.txt` under root, `hard.txt` under folder1). The
  test currently PASSES (the core emits 2 nodes, hardlinks counter
  increments by 1).
- **Executed comparison:** not run — would require building both
  implementations and comparing result-tree sizes on a hard-link-rich
  fixture. Tracked as a parity-bench scenario.

## Fix plan (for the engineering team)

The decision is: match the og's one-node-per-record behavior (parity)
or keep the per-parent fan-out (superset, more useful). Recommendation:
**parity by default, opt-in superset** via a walker flag (mirrors the
S-001 `destroy_fully_overwritten` flag pattern).

1. **Architectural change:** add `WalkerFlags::single_link_per_record`
   (default ON for parity, OFF for "show all hard-links" mode).
2. **Files to modify:**
   - `include/cr/ntfs/cr_mftwalk.h:30-58` — extend `WalkerFlags` with
     `single_link_per_record` (bit 0x40 or similar). Document the
     default and the opt-in.
   - `src/core/ntfs/cr_mftwalk.cpp:711-724` — when the flag is ON,
     pick the namespace-best `$FILE_NAME` from the full list (not
     per-parent) and emit ONE node. When OFF, keep the current
     per-parent fan-out.
   - `src/core/ntfs/cr_mftwalk.cpp:796-841` — gate the per-parent
     loop on the flag; when ON, emit only the first iteration.
3. **Algorithm/constant:** namespace ranking already implemented (the
   `rank()` function used in the per-parent collection). When the flag
   is ON, apply the ranking across ALL `$FILE_NAME` entries (not
   per-parent) to pick the single namespace-best name.
4. **Tests to add:**
   - `NtfsVolume.HardLinkSingleLinkMatchesOg` — fixture: a record
     with 3 `$FILE_NAME` entries (POSIX under root, Win32 under
     folder1, DOS under folder2). Assert the result tree has 1 node
     (the POSIX name under root) when `single_link_per_record` is ON.
     This test FAILS today (core emits 3 nodes).
   - `NtfsVolume.HardLinkFanOutOptIn` — same fixture; assert 3 nodes
     when the flag is OFF. This test PASSES today.
   - `NtfsVolume.HardLinkCounterZeroInSingleMode` — assert
     `stats.hardlinks == 0` when the flag is ON (the counter only
     increments in fan-out mode).
5. **Contract/doc updates:** `docs/AUDIT-NTFS.md` hard-link section —
   note the default parity mode and the opt-in superset.
6. **Acceptance criteria:**
   - The three tests above pass.
   - A parity-bench comparison on a hard-link-rich fixture shows
     identical result-tree sizes when the flag is ON.
   - The opt-in flag is documented and tested.

## Tests to add

- `NtfsVolume.HardLinkSingleLinkMatchesOg` (FAILS today)
- `NtfsVolume.HardLinkFanOutOptIn` (PASSES today — regression guard)
- `NtfsVolume.HardLinkCounterZeroInSingleMode`

## Related findings

- **S-001** (FIXED) — the destroy rule for fully-overwritten deleted
  files interacted with hard-link fan-out: if the primary link was
  destroyed, the secondary links were also dropped (they share props).
  The fix's `destroy_fully_overwritten` flag operates on the props
  level; M-007's fix operates on the emission level. They compose.
- **S2-A-2 / N-011** — ADS as child nodes is the parallel "core emits
  more than og" divergence: the og silently drops named `$DATA`, the
  core emits ADS as child nodes. Same parity-vs-superset shape.
- **S2-A-3 / M-008** — `$ATTRIBUTE_LIST` multi-hop chase is the inverse
  shape: the core chases deeper than the og (more content recovered,
  less parity).

## Open questions

- **OQ-S2A-2:** does the og's downstream UI/writer fan out hard-links
  from a single record? `NTFSLib.mo` has `CNtfsMultiFileRecord` with
  `GetMainFileRecord`/`GetChildRecordArray`/`Convert2AttributeArray` —
  but `NTFSSearchDLL` strings have NO `HardLink`/`MultiFileRecord`
  mentions. The scan engine itself doesn't fan out; the UI/writer
  might. Needs RE of the consumer side to confirm no scan-path
  fan-out exists.
- **Should the destroy rule fire per-record or per-link?** Currently
  the destroy rule (S-001 fix) fires per-record (if the props' reuse
  ratio is 1.0, the whole record is destroyed, including all
  per-parent clones). If the og fires per-link, the destroy rule
  would need to be re-scoped. Resolving OQ-S2A-2 would clarify.
