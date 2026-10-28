# MAJOR M-104 — table-matched known volumes surface as anonymous "Candidate @ <LBA>" roots: no volume display names, no FS-type markers, known-vs-lost indistinguishable, and the Found/Deleted roots stay empty — the og's real-session per-volume tree shape is unreachable through the whole-disk flow

**Severity:** MAJOR
**Module:** orchestration / tree (the whole-disk session shape) — modules/orchestration.md, modules/lost-partition.md, modules/session.md-adjacent
**Parameters affected:** A11-adjacent (node fields/roots), A42 (the tree shape the host renders), B8 (multi-volume boundaries), B10 (volume display naming inside the tree), D12/D13 (session tree semantics/shape), A56-adjacent (interface shape vs the og's persisted contract)
**Corpus references:** the real og sessions re-parsed first-hand this lane (evidence/executions/s10-wholedisk-scan-targets.md): the G session's depth-1 shape — `Local Disk(G:)` 121,775 rows + `System Reserved (NTFS)` 190 rows + `NTFS` (1-row FS-type marker) + `Reconstructed` 83,106 + `File Path Lost` 13,430; live and deleted files in ONE per-volume tree (deleted = per-row flag); `architecture/04-recovery-workflow.md` §3.4 (the insertion trace carries each target's name `%s` — decompile `DRWUI_decompiled.c:904076`/`:904198`); UIUX/07 + the landing-page decode (`Internal drives (2)`, `Windows(C:)` — display names are the og's target vocabulary)
**Core references:** audited `84a1832` — `src/core/orchestrator/cr_scan.cpp:1629-1636` (the candidate root construction: `role = u"Candidate @ " + snprintf(" %llu", c.start_lba)` — synthetic, identity-free), `:200-201`/`:242-243` (the Found/Deleted roots — empty on the whole-disk flow because the main pass dispatches Unknown), `:1728-1763` (`scan_candidate` passes the SAME `croot` as root_active/root_deleted/root_lost — active+deleted+orphan all land under the anonymous candidate root), `src/core/lost/cr_candidates.cpp:928-962` (the table-match exemption + the +500 "Found once partition" bonus — the og-parity scoring that rescues the files); EXECUTED: evidence/executions/s10-wholedisk-scan-targets.md RUN 1 + the full-tree dump (`Candidate @ 2048`/`Candidate @ 18531` roots, empty Found/Deleted, `_ELFILE.TXT del=1` under the candidate root)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered — S-048 owns the carve branch's volume grouping and M-103 the orphan grouping (the same real-session evidence family); no prior finding owns the VOLUME trees themselves (the known-volume presentation of the whole-disk flow). M-072/M-079 own the volume-label input mechanics this composes with.

## Summary

On a whole-disk scan with discovery enabled, our core does find and scan the disk's known in-table volumes — but only as a side effect of the lost-partition discovery's table-match bonus, surfacing them as synthetic `"Candidate @ <start-LBA>"` roots: no volume display name, no filesystem-type marker, no way to tell a known volume from a genuinely lost partition, and the main Found/Deleted roots stay empty. The og's whole-disk session tree gives every volume a display-name root (`Local Disk(G:)`, `System Reserved (NTFS)`) with live and deleted files in one tree, plus an FS-type marker row. The og's real-session shape — the shape its hosts, filters, and parity benches walk — is unreachable through our whole-disk flow.

## What the reference does

Every volume in a whole-disk session — known-in-table or discovered — becomes a tree root carrying its DISPLAY NAME: the real G session's depth-1 census (re-parsed first-hand) is `Local Disk(G:)` (121,775 rows: the volume's whole live+deleted tree, deleted as a per-row flag), `System Reserved (NTFS)` (190 rows: bootmgr, Boot, …), the one-row `NTFS` FS-type marker, plus the special roots (`Reconstructed` per S-048, `File Path Lost` per M-103). The scan-setup insertion trace carries each target's name (`"Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d"` — the `%s` is the target's name from the partition-info object, decompile :904198's 0xa8-stride loop). The og never presents a volume as an anonymous LBA.

## What our core does

The whole-disk flow's volume results come exclusively from the lost-partition discovery: each table-matched partition survives the M-017(a) containment drop via the `start_lba == p.start_lba` exemption, takes the +500 "Found once partition" bonus (cr_candidates.cpp:928-962 — og-parity scoring), and is scanned by `scan_candidate` under a root whose role is the synthetic string `"Candidate @ <start-LBA>"` (cr_scan.cpp:1629-1636). `scan_candidate` passes that one root as root_active AND root_deleted AND root_lost (:1752-1753), so the volume's live files, deleted files, and orphans all land under the anonymous root. The main `Found`/`Deleted` roots — created for the quick/deep pass (:200-201) — stay empty because the main pass still dispatches Unknown on the disk slice (S-051). Executed (RUN 1 + the tree dump): `Candidate @ 2048` / `Candidate @ 18531` with `DOC.TXT` + `_ELFILE.TXT del=1` under each; `Found`/`Deleted`/`Lost Files`/`Other Deleted Files (Carved)` all empty.

## Gap

Falsifiable (and executed): scan a two-volume disk with `--whole-disk`. Our tree: two anonymous `Candidate @ <LBA>` roots (indistinguishable from genuinely-lost partitions, no display names, no FS markers) + four empty standard roots. The og's tree: `Local Disk(G:)` + `System Reserved (NTFS)` display-name roots with the full live+deleted trees + the `NTFS` marker + the special roots. No field on our candidate roots or their children carries the volume's identity (label, letter, FS type, known-vs-lost).

## Impact

User-observable tree shape on every whole-disk scan: the user cannot tell which volume a result came from, cannot tell a known volume from a lost-partition find, and sees the main result roots empty while files hide under LBA-named nodes. Session trees we persist diverge structurally from the og's persisted sessions (the D12/D13 contract), so parity benches and host code written against the og's shape mis-walk our tree. The single-volume `ScanConfig.volume_label`/`VolumeSessionInfo` model (M-079's field) has no multi-volume expression — the session format records one volume identity for a tree that (in the og's reality) spans several.

## Reproduction / verification

Executed comparison (protocol §6.1): RUN 1 + the full-tree dump in evidence/executions/s10-wholedisk-scan-targets.md; the og side from the same file (the real G session's first-hand depth-1 census + the decompile insertion loop). Core code read first-hand at the cited lines.

## Fix plan (for the engineering team)

1. Architectural: give every scanned target a VOLUME ROOT with a display name instead of (or as the parent of) the candidate root — the table-matched targets take the volume's display name (label + letter when present, `System Reserved (NTFS)`-style bare label otherwise; the enumeration layer already reads labels via GetVolumeInformationW — the M-079 fill-site finding is the same plumbing), and genuinely-lost partitions take the og's lost-target naming (the insertion trace's `%s`; the corpus's lost-partition display decode is the naming source). The candidate's FS family (already probed by `scan_candidate`) becomes the FS-type marker semantics; keep the `Candidate @ LBA` string only as the last-resort name when nothing better exists (the og's trace shows named targets, not LBA strings).
2. Files: `src/core/orchestrator/cr_scan.cpp` (the candidate-root construction + the per-target root plumbing), `include/cr/orchestrator/cr_scan.h` (a per-target display-name input — composes with M-079's `volume_label` fill-site fix), `src/core/lost/cr_candidates.cpp` (carry `table_match` and any label the table/live probe can supply into the Candidate struct — the fields exist for scoring; expose them to the presentation).
3. Algorithm: the display-name resolution order: the live volume's label (when the enumeration layer can match the candidate's start LBA to a volume) → the partition's FS-type string → the LBA fallback. The Found/Deleted split question rides S-051's fix (per-volume targets with their own roots); do NOT keep double roots — one tree per volume, deleted as a flag (the og's shape, and our node model already carries `deleted_state`).
4. Tests: (a) the evidence fixture's whole-disk run asserts the two volume roots carry display names (VOLONE/VOLTWO labels in the fixture) and that `table_match` candidates are distinguishable from planted genuinely-lost partitions; (b) a lost-partition fixture (deleted partition, not in the table) asserts the lost naming, not "Candidate @ LBA"; (c) the session round-trip preserves the volume-root names; (d) the mixed case (one known volume + one lost partition on one disk — the names and the known/lost distinction both assert).
5. Contract/doc: `modules/orchestration.md` + `modules/lost-partition.md` rows; `docs/PARITY-DELTAS.md` (register the shape convergence); the S-048/M-103 fix plans should land AFTER or WITH this (they build per-volume branches under roots this finding names).
6. Acceptance: the executed fixture's tree shows `VOLONE (…)`-style display-name roots (not `Candidate @ 2048`); the og's real G-session depth-1 shape (display-name volume roots + FS marker + Reconstructed + File Path Lost) is reproducible end-to-end once S-048/M-103 land; `scan --disk --whole-disk` on the fixture asserts known-vs-lost distinction in the tree.

## Tests to add

- `Orchestrator.WholeDiskVolumeRootsCarryDisplayNames` — fails before (anonymous roots), passes after.
- `Orchestrator.WholeDiskKnownVsLostDistinguishable` (one known + one planted lost partition: different naming, both scannable).
- `Session.WholeDiskVolumeRootNamesRoundTrip`.
- `Orchestrator.CandidateFallbackNameOnlyWhenUnknown` (the LBA string is the last resort, not the default).

## Related findings

S-051 (the same flow's silent-empty half — the target builder this finding's presentation rides); S-048 (the carve branch's per-volume grouping — the same real-session evidence); M-103 (the orphan grouping); M-079 (the volume-label fill site — the display-name plumbing); M-101 (the results-tree branch family); N-090/M-072-adjacent (the session/query surfaces that consume the shape).

## Open questions

The og's exact lost-partition display-name template for the tree (the insertion trace proves a name; the rendered form — e.g. whether it carries size/FS — is a UIUX decode the fix session should pin; `DrwLostDiskPage` decode and the results-phase UIUX/09 rows are the sources). Whether the og's `NTFS` FS-type marker row is per-volume or per-session (the G session shows ONE `NTFS` row for two NTFS volumes — per-session by FS family, but a one-session sample; the H/I sessions are single-FS too — a corpus probe on multi-FS disks would settle it).

RESOLVED (session-10 record census, evidence addendum): the session persists ONE record-2 naming the primary volume; the second volume's identity lives ONLY in the tree (its display-name root) plus record-1's special-partition entry (System Reserved @ LBA 2048 in the G session — the resume-time re-attachment registry). The per-volume display-name roots this finding asks for ARE the og's persistence mechanism for multi-volume identity — no new session record is needed.

## Resolution

**Status:** FIXED
**Fixed in commit:** this landing (the same coherent change as S-051 — the presentation rides the target builder)
**Verified by:** `Orchestrator.WholeDiskVolumeRootsCarryDisplayNames` (VOLONE/VOLTWO display-name roots from the boot-sector labels; zero "Candidate @" roots; the volume's deleted file under the SAME root flagged `DeletedState::Deleted` — the og's one-tree-per-volume shape), `Orchestrator.WholeDiskKnownVsLostDistinguishable` (the in-table volume under its label root, the out-of-table volume as exactly one discovery candidate — both scannable, distinguishable), `Orchestrator.WholeDiskVolumeRootNamesRoundTripSession` (the display-name roots persist and reload through the session round-trip), `Orchestrator.WholeDiskMixedFamilies` (the NTFS target's tier-2 FS-marker name "NTFS @ <LBA>" — the anonymous form is never the default), and `Orchestrator.WholeDiskNoDoubleScan` (the known-vs-lost distinction includes the no-duplicate rule: no anonymous candidate roots for table volumes at all). Full suite 1105/1105.
**Resolution notes:** The finding's three-tier naming resolution landed at the orchestrator layer: (1) the on-disk volume label — the FAT families via the new `fat::read_volume_label` (validate_boot-gated, +0x2B/+0x47 by flavor, trailing-pad-trimmed, label-less volumes refused so tier 2 takes over — never a blank name); the NTFS/$Volume, ext4 and exFAT label decodes are N-133's landing and slot into the same tier unchanged; (2) the FS-type marker with the position (`"NTFS @ <LBA>"` — carries the family and the location, per the finding's own resolution order); (3) the anonymous LBA form only when the family itself is unknown — and an unknown-family table target gets NO root at all (skipped-honestly per S-051), so the anonymous form is de facto reserved for genuinely-lost discovery candidates, exactly the finding's acceptance. The known-vs-lost distinction is now expressible three ways: display-name roots for table volumes, "Candidate @ <LBA>" only for discovery finds, and the stats split (`table_volume_targets` vs `lost_partitions`). One-tree-per-volume confirmed: `scan_candidate` passes the volume root as root_active+root_deleted+root_lost and the deleted state rides the node flag — no Found/Deleted double roots for volume results (the finding's fix-plan item 3). Honest residuals, corpus-scoped, cross-referenced: the og's exact FS-type marker ROW (the one-row `NTFS` persistence artifact) stays unlanded — the finding's own open question records it as corpus-ambiguous (per-volume vs per-session, a one-session sample) and the FS-in-root-name carries the information meanwhile; the empty standard roots on whole-disk flows remain the ABI's persistent shape (they are the volume-scan contract, pinned by dozens of tests — the og-exact root-set narrowing belongs to the S-048/M-103 tree-shape wave, which now has the per-volume roots to build on). S-048's Reconstructed children and M-103's orphan grouping land on this mechanism next.

**DU-merge addendum:** the S40 lane's convergent S-051 landing carried a
GPT-entry-name tier for GPT partitions (their naming); the merged
`volume_target_name` order is now: (1) the FAT boot-sector label, (2) the
GPT entry name, (3) the `NTFS @ <LBA>` FS marker, (4) the anonymous form —
last resort only, reserved in practice for genuinely-lost discovery
candidates (an unknown-family table target gets no root and is skipped
honestly per S-051). The GPT tier is superseded per-family as the
N-133 label decodes land. The display-name, known-vs-lost and
round-trip tests all pass at the merged tree.
