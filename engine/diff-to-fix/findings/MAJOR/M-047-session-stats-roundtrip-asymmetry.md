# MAJOR M-047 — Session stats round-trip asymmetry: core writes 4×u64 root stats then discards them on load; refresh is direct-children-only vs the og's persisted recursive accumulator

**Severity:** MAJOR
**Module:** session + tree
**Parameters affected:** D12 (session persistence format — persisted-then-discarded fields), D11 (node fields — recursive stats semantics), B8/Q68 (aggregate reporting accuracy after reload), A26 (tree walk — the og's stats are a cached recursive accumulator, the core's a lazy direct count)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:285` (CFileNode vtable +0x58 = FUN_140089c70 "recursive stats accumulator (+0x9C/+0xA8/+0xB4/+0xBC)"; +0x68 = CopyOut "0x28 B stat blob from +0x98"), `:318-320` (record-4 grammar: "40B stat_blob (image: … — reader stores the 40-byte image directly at +0x98)" — the reader TRUSTS the persisted image)
**Core references:** `src/core/session/cr_records.cpp:173-176` (writer persists stats().file_count/dir_count/total_logical/deleted_count), `:247-250` (reader: `(void)c.g64();` ×4 — the persisted stats are read and DISCARDED), `src/core/tree/cr_node.cpp:276-289` (`refresh_stats` counts DIRECT children only) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og persists each root's recursive stat accumulator as a 40-byte image and, on load, stores
it verbatim — folder totals after session reload equal the pre-save totals by construction.
Our core writes four u64 stats per root, then throws them away on load; when stats are later
refreshed, the refresh counts only DIRECT children, so any root whose files live in
subdirectories under-report file_count/total_logical/deleted_count after every reload.

## What the reference does

Per `rsf-format--scan-session.md`: the node carries a recursive stats accumulator
(vtable +0x58, fields at +0x9C/+0xA8/+0xB4/+0xBC) maintained during the scan; on save, a
40-byte stat blob is serialized as an image; on load, "the reader stores the 40-byte image
directly at +0x98" (`:318-320`) — no recomputation, full trust in the persisted recursive
totals.

## What our core does

- Writer (`cr_records.cpp:173-176`): `put64(out, f->stats().file_count); … dir_count …
  total_logical … deleted_count` — the stats ARE persisted.
- Reader (`cr_records.cpp:247-250`):

```cpp
(void)c.g64();
(void)c.g64();
(void)c.g64();
(void)c.g64(); /* stats */
```

  — consumed into nothing.
- `refresh_stats` (`cr_node.cpp:276-289`): iterates `first_child_ → next_sibling_` of the
  root ONLY — direct children. Files in nested directories never contribute to the root's
  totals through this path.

Net behavior: after save→load, root stats are zero until something calls `refresh_stats`,
and when it does, totals equal direct-children counts (under-reporting every nested
directory tree). Before any save (live scan), the same direct-only refresh already
under-reports whenever it is the stats source.

## Gap

Falsifiable claim: build a session whose root contains a directory with 10 files; save and
reload; the og's root totals show 10 files (persisted recursive image); the core's root
shows 0 until refresh, and after refresh shows 0 files at the ROOT level for nested content
(direct-children counting).

## Impact

Every aggregate the user sees after resuming a session (file counts, total sizes, deleted
counts per root and per the whole tree) is wrong in the core whenever content is nested —
the common case for real directory trees. Report/summary surfaces that consume stats inherit
the under-count. This is user-observable on the resume-session path, the exact path sessions
exist for.

## Reproduction / verification

- Corpus: rsf-format accumulator + "reader stores the image directly" citations read
  first-hand this session.
- Core: writer/reader asymmetry and the direct-only refresh both read first-hand (lines
  quoted above).
- Executed comparison: deferred to the fix leg (`Session.ReloadedRootStatsAreRecursive`
  fails before: root stats 0/under-count; passes after).

## Fix plan (for the engineering team)

1. Reader: store the four persisted u64s into the root's `stats_` instead of discarding them
   (`cr_records.cpp:247-250` — replace the `(void)` drops with assignments before
   `add_root` returns the node).
2. Make `refresh_stats` recursive (or add `refresh_stats_recursive` used on load/attach):
   depth-first accumulation of file_count/dir_count/total_logical/deleted_count through the
   subtree — matching the og's accumulator semantics (+0x58 walk).
3. Decide ONE source of truth: prefer "persisted image wins on load; refresh recomputes
   recursively on demand" — the og's model.
4. Add stats assertions to the existing session round-trip test family (nested fixture).

## Tests to add

- `Session.ReloadedRootStatsAreRecursive` (nested tree → save → load → root totals equal
  pre-save totals)
- `Session.StatsDeletedCountRecursive` (deleted files nested two levels count at the root)
- `Tree.RefreshStatsRecursiveAfterAttach` (attach subtree → refresh → totals include it)

## Related findings

- S-031 (deleted_count under-count compound — carved files not counted deleted AND stats
  direct-only; fixing both is required for correct deleted totals)
- M-050 (session node-model round-trip gaps — same reader path)
- N-058 (session load validation deltas — same loader)

## Open questions

None. (The og's 40-byte blob vs the core's 4×u64=32 bytes is a wire-format difference, not a
semantic one — the core may keep its own byte format per the session-format parameter as long
as the FIELDS are equivalent; they are, once read.)
