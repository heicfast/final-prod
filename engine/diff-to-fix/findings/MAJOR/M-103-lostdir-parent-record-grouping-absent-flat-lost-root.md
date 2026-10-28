# MAJOR M-103 — The lost-files branch loses the og's per-parent-record grouping: no "File Path Lost" root naming and no `LostDir<record>` synthetic folders (flat "Lost Files" reparent)

**Severity:** MAJOR
**Module:** ntfs / tree (orphan assembly; FAT/exFAT equivalent path noted)
**Parameters affected:** A26-adjacent (tree shape), B5 (orphan reconstruction behavior), B10-adjacent (branch naming), D12/D13 (session tree semantics), A42-adjacent (host-rendered shape)
**Corpus references:** EXECUTED — the three real og sessions: the `File Path Lost` root exists in every session (G: 13,420 rows; **H: 938,982 rows — 62.6% of the session**; I: 12,903), its children are 398 (H) synthetic folders named `LostDir<NNNN>` (`File Path Lost/LostDir7206/IMG-20210816-WAN0001.jpg`, verbatim in `evidence/executions/s10b-real-rsf-session-mining.md`), the files keep their real MFT $FILE_NAME remnant names with active/unknown deleted mixes (669,821 active in H — the post-format salvage population); decompile: `NTFSSearchDLL_decompiled.c:42041` (`FUN_180001e40(&local_78,"LostDir%d",...)`), `:48915`/`:48924` (the "LostDir" literal), `exFATSearchDLL_decompiled.c:5182` (the FAT-family twin, "LostDir%d"); artifacts: `ifileproperties-part1--ntfs-fat-exfat.md`, `ntfs-engine-algorithms-v2--deleted-viability.md`, `searchdll-family--architecture.md`; DRWUI `:319865`/`:321001` ("File Path Lost" folder creation) + `qt_strings_english.txt` 0x109f2
**Core references:** audited `62074bf` — `src/core/ntfs/cr_mftwalk.cpp:1066-1092` (phase B: orphans reparent FLAT to `root_lost`, `++stats.orphans`; no per-parent grouping, no synthetic folder), `src/core/orchestrator/cr_scan.cpp:202`/`:244` (the root is named `u"Lost Files"` — grep-verified: "File Path Lost" appears NOWHERE in `src/`, `include/`, `tests/`), `include/cr/ntfs/cr_mftwalk.h:14` (the documented "Lost Files" root role)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og groups orphaned MFT records into synthetic `LostDir<NNNN>` folders under a "File Path Lost" root, one folder per original parent record, preserving the sibling relationship among files that shared a deleted directory. Our core reparents every orphan flat under a root named "Lost Files": the sibling grouping is destroyed, and the branch name differs from the og's persisted session tree. In a real salvage population this branch is the majority of the session (938,982 of 1,500,395 rows in the H session), so the difference dominates the user's view of a deep scan.

## What the reference does

The engine-side decompile constructs the folder name `LostDir%d` (NTFSSearchDLL:42041; the exFAT twin at :5182), keyed by the orphaned file's parent record reference — files whose $FILE_NAME parent points at a dead/unresolvable record are grouped by that parent so ex-siblings stay together (real-session NNNN values 7206, 1707, 7507... are MFT record numbers). The files inside carry their real remnant names (`IMG-20210816-WAN0001.jpg`), sizes, and a mix of active/unknown deleted grading (the torn-write and post-format populations). DRWUI creates the "File Path Lost" tree folder for these rows (:319865, :321001), and the og's own help text names it as one of the two path-based special roots alongside "Reconstructed". The persisted sessions prove the full chain: real names, per-parent folders, the "File Path Lost" root — in all three real sessions.

## What our core does

Phase B of the MFT walk (`cr_mftwalk.cpp:1066-1092`) resolves each emitted node's parent by record: found → `tree.reparent(node, parent_node)`; not found → `++stats.orphans; tree.reparent(node, root_lost)` — every orphan lands DIRECTLY on the root, one flat pool, no intermediate. The root is created by the orchestrator as `u"Lost Files"` (`cr_scan.cpp:202/:244`); "File Path Lost" does not exist anywhere in the product code. The per-orphan parent-record value is available at the reparent site (`e.parent_record`) but is discarded after the failed lookup.

## Gap

Falsifiable: walk a fixture MFT with two dead-parent files (records X and Y both unresolvable, two children each). The og's session carries `File Path Lost/LostDir<X>/...` ×2 + `File Path Lost/LostDir<Y>/...` ×2 — four files, two sibling groups; our session carries four files flat under `Lost Files` with no grouping, and the branch name differs from the og's.

## Impact

Grouping information is destroyed, not just renamed: a user browsing a 900k-file salvage cannot navigate by original folder — the og's per-parent folders ARE the deleted-directory structure surviving the format, which is the single most useful navigation aid in a post-format recovery. Hosts written against the og's shape mis-walk our sessions (D12/D13). The magnitude is majority-of-session in real data (62.6% in the H session). Rated MAJOR (not SEVERE) because the files themselves, their names, sizes, and grading all survive — the loss is the grouping axis and the branch label; per the scale this is a significant structural/behavioral difference short of wrong recovery results.

## Reproduction / verification

Executed comparison (§6.1): the three real sessions parsed with the core's own script; the `File Path Lost` subtree enumerated (root rows, LostDir counts, real-name samples, the active/unknown distribution — verbatim in the evidence file). Decompile cites re-read at NTFSSearchDLL:42041/:48915/:48924 and exFATSearchDLL:5182. Core side read first-hand at the phase-B block; the name grep verified zero "File Path Lost" hits across the product tree.

## Fix plan (for the engineering team)

1. At the orphan reparent site (`cr_mftwalk.cpp` phase B), group by `e.parent_record`: create (lazily, one per distinct dead parent) a synthetic folder node under `root_lost`, name it `LostDir<parent-record-number>` (the og's `%d` — record number, unpadded), reparent the orphan to that folder instead of the root. Files whose parent record is 0/degenerate can go straight to the root (match the og's degenerate handling if a real-session sample ever shows one; none observed).
2. Rename the root default from `u"Lost Files"` to `u"File Path Lost"` (the persisted-session name; keep any existing config override honored the same way M-029 treats the carve root name — the og's DRWUI branch string is the contract).
3. Session round-trip: the synthetic folders persist as ordinary nodes (they are real nodes in the og's sessions — the 398 LostDir folders prove it) — no schema change needed beyond what the session writer already carries for folders.
4. FAT/exFAT parity: the exFAT twin (exFATSearchDLL:5182) implies the same grouping for FAT-family orphan sweeps; verify the FAT/exFAT quick-scan orphan paths group identically (the `cr_exfat_quick.cpp` orphan emission site) — same folder contract, same name.
5. Tests: `Ntfs.LostDirGroupsOrphansByParentRecord` (two dead parents → two LostDir folders with the right children); `Ntfs.LostDirNameIsRecordNumber` (`LostDir7206` pinned); `Ntfs.OrphanBranchNamedFilePathLost`; `Session.LostDirRoundTrip`; `Exfat.OrphanLostDirGrouping` (the twin); `Ntfs.FlatLostRootRemoved` (the flat shape asserted absent).

## Tests to add

- `Ntfs.LostDirGroupsOrphansByParentRecord` — fails today (flat), passes with grouping.
- `Ntfs.LostDirNameIsRecordNumber` — the og's naming pinned from NTFSSearchDLL:42041.
- `Ntfs.OrphanBranchNamedFilePathLost` — root-name parity.
- `Session.LostDirRoundTrip` — the folders persist and reload.
- `Exfat.OrphanLostDirGrouping` — the FAT-family twin.

## Related findings

- S-048/S-049 (the same evidence file; the sibling branch-shape and naming findings)
- M-101 (the other missing tree branches — Versions/Tags; this finding completes the real-session-grounded branch set)
- M-072 (the filter axes that target the branches)
- OQ-SA-1-FIXED (the exFAT orphan first-cluster contract at the same emission path)

## Open questions

None blocking. Whether the og ALSO uses LostDir for the "File Name Lost" population (no real-session sample shows that branch firing) — if a future session finds one, extend the same mechanism; do not invent it now.
