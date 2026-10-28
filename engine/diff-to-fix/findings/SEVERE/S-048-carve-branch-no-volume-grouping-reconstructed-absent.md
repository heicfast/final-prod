# SEVERE S-048 — The carve branch has no per-volume grouping: the og's `Reconstructed/<volume-display-name>/` tree shape is absent (single flat root, no volume attribution)

**Severity:** SEVERE
**Module:** carve / tree (orchestrator's carve-root assembly)
**Parameters affected:** A26-adjacent (tree walk/shape), A42-adjacent (the tree shape the host renders), B8-adjacent (multi-volume boundaries), B10-adjacent (volume display naming inside the tree), D12/D13 (session tree semantics/shape), A56-adjacent (interface shape vs the og's persisted contract)
**Corpus references:** EXECUTED — the three real og sessions in `og-codebase/SaveScan/` (100% of 189,115 carved rows across all three sessions sit at depth 2 under per-volume parents: `Reconstructed/ (H:)` ×94,057, `Reconstructed/System Reserved` ×82,911, `Reconstructed/ (I:)` ×12,147 — full run record in `evidence/executions/s10b-real-rsf-session-mining.md`); decompile: `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:320992` + `:357713` + `:611618` + `:642786` (the type==7 branch creates the "Reconstructed" folder node), `:321001`/`:319865` ("File Path Lost"); Qt strings `re-workspace/reversed/DRWUI.exe/qt_strings_english.txt` 0x1e33e ("Reconstructed")
**Core references:** audited `62074bf` (product-code lineage through `26da616`) — `src/core/orchestrator/cr_scan.cpp:145-152` (`raw_root_name_()` → the single configured/default root name), `:205`/`:247` (ONE `root_carved_` global root, added per scan), `:1877` (`carve_slice(..., root_carved_, ...)` — every carved node attaches under the one root, no volume intermediate). Verified first-hand this session; no `Reconstructed` string anywhere in `src/` (grep 0 hits).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's results tree groups every carved file under a per-volume branch: a `Reconstructed` root whose children are the scanned volumes' display names, with each volume's carves under its own node. Our core attaches all carved nodes to a single global root named "Other Deleted Files (Carved)" (or the configured raw-category name) with no volume level at all. In a multi-volume or disk-level scan the og's user can see WHICH volume a carved file came from; our tree cannot express that, and session trees we persist diverge structurally from the og's persisted sessions on every scan that has carves.

## What the reference does

Executed evidence (the first mining of the real sessions — see the evidence file): all three real production sessions carry the `Reconstructed` root with per-volume children; the child name is the volume's display name — the H session's volume has an empty label so the child is ` (H:)`; the G session's carves sit under `Reconstructed/System Reserved` (the label of the volume whose unallocated space contained them, no drive-letter suffix since it has none). The DRWUI decompile creates the "Reconstructed" folder on the type==7 branch at four call sites (`:320992`, `:357713`, `:611618`, `:642786`), and the help string ('Refer to these files under the File Path, including "File Path Lost" and "Reconstructed"') documents both special roots as user-facing tree locations. The og's auto-saved sessions persist this shape (that is how it appears in the corpus's real .rsf payloads).

## What our core does

`ScanController` creates exactly one carve root: `root_carved_ = tree_.add_root(raw_root_name_())` (`cr_scan.cpp:205`, `:247`), where `raw_root_name_()` returns the configured `raw_category_name` or the default `u"Other Deleted Files (Carved)"` (`:145-152`). `carve_slice` receives that single root (`:1877`) and every carved node — from every volume/slice in the scan — attaches directly to it. There is no volume intermediate node, no volume-attribution field on carved nodes, and no `Reconstructed` string anywhere in the product code (grep-verified zero hits). The M-029 landing named the root after the og's `filter_generalraw` display-name mapping (the UI category), not the og's session-tree branch.

## Gap

Falsifiable: scan two volumes (or a whole disk) with carve enabled, then save the session. The og's tree contains `Reconstructed/<volume-A>/...` and `Reconstructed/<volume-B>/...` children (real-session-proven shape); our tree contains one flat root with all carves from both volumes mixed, and no node, field, or query axis can tell which volume a carved file was found in.

## Impact

User-observable on every multi-volume scan: volume attribution for carved results is lost (a user recovering from disk D: vs E: cannot tell where a carved photo came from). Host/API consumers see a different tree shape than the og persists — the session-tree contract (D12/D13) diverges, so parity benches and host code written against the og's shape mis-walk our tree. The C2 row of SCAN-ENGINE-DELTA ("structural roles map 1:1") is falsified for this branch by the real-payload evidence.

## Reproduction / verification

Executed comparison (protocol §6.1): the three real og sessions parsed with the core's own `scripts/compare_og_rsf.py`; every carved row's parent enumerated (the table above; verbatim parser output in the evidence file). Core side read first-hand at `cr_scan.cpp` (the root assembly) and grep-verified (no `Reconstructed`, no volume-child construction in the carve path). Decompile cites re-read at the quoted lines.

## Fix plan (for the engineering team)

1. Architecture: introduce a per-volume carve grouping — either (a) one `Reconstructed` root with per-volume child nodes (og's exact shape), or (b) keep the configurable category root but add the volume children under it. The og's shape is (a); the default name should be `Reconstructed` with the existing `raw_category_name` config retained as an override (the og's filter_generalraw display mapping stays a UI concern).
2. Files: `src/core/orchestrator/cr_scan.cpp` (the carve-root assembly + per-volume child creation at scan-start/volume-attach), `src/core/carve/cr_carve.cpp` (`carve_slice`'s `carve_root` parameter becomes the volume child), `include/cr/carve/cr_carve.h` (contract note), the session writer/reader round-trip for the intermediate nodes.
3. The volume child's display name: `<volume-label> (<drive-letter>:)` when a letter exists, the bare label otherwise — sourced from the existing volume-info path (M-079's label fill), with the real-session forms pinned (` (H:)` for empty-label-with-letter; `System Reserved` for label-without-letter).
4. Volume attribution datum: carry the originating volume id on carved nodes (node attr or the props record) so query/sort can regroup without re-walking.
5. Tests: `Carve.MultiVolumeReconstructedGrouping` (two slices → two volume children, correct attribution); `Carve.ReconstructedVolumeChildNaming` (label+letter, empty-label+letter, label-without-letter); `Session.ReconstructedShapeRoundTrip`; `Query.CarvedVolumeFilter` (the attribution axis).
6. Gate docs: correct SCAN-ENGINE-DELTA C2's "roles map 1:1" for the carve branch; CROSS-REF-MATRIX row for this finding.

## Tests to add

- `Carve.MultiVolumeReconstructedGrouping` — fails today (single flat root), passes when the volume children exist.
- `Carve.ReconstructedVolumeChildNaming` — pins the three real-session naming forms.
- `Session.ReconstructedShapeRoundTrip` — the intermediate nodes persist and reload.
- `Query.CarvedVolumeFilter` — volume id as a query axis over carved nodes.

## Related findings

- S-049 (the per-file naming contract mined from the same evidence)
- M-103 (the File Path Lost / LostDir grouping — the sibling divergence on the lost branch)
- N-063-FIXED (landed the root's category-name mapping; its remainder is resolved by S-049)
- M-029-FIXED (the raw_category_name surface — this finding narrows its scope to the UI category, not the tree root)

## Open questions

None material. The one nuance — whether the og's `Reconstructed` children also appear for network/imaged volumes with synthetic names — has no real-session sample; the fix lane should not block on it.
