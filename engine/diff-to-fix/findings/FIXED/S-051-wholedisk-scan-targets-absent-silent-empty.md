# SEVERE S-051 — the whole-disk scan target never consults the partition list: `cr_ui_open_disk` + scan (the CLI `scan --disk N` without `--whole-disk`) returns rc=0 with ZERO files on a healthy multi-volume disk — the og's CDoScanPartition flow reads the partition list and scans every known volume

**Severity:** SEVERE
**Module:** orchestration (scan-target assembly) — modules/orchestration.md, modules/lost-partition.md
**Parameters affected:** A25 (engine algorithm steps — the target-set construction that precedes every engine), A31 (scan lifecycle — the scan completes "successfully" with nothing), A42 (public surface shape — `cr_ui_open_disk` is a first-class ABI export), B8 (multi-volume boundaries), D37-adjacent (stage model)
**Corpus references:** `easeus-drw-research/architecture/04-recovery-workflow.md` §2 (the routing table: "A partition/disk | PartitionStartWnd | CDoScanPartition → CGenericRecoveryController") and §3.4 ("CGenericRecoveryController … reads the partition list, defaults sector size 512, then logs 'Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d' per lost partition — lost partitions are inserted as scan targets directly"); the insertion loop decompiled at `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:904076` and `:904198` (the 0xa8-stride partition-info iteration, each target carrying its name `%s`); the real multi-volume sessions (`og-codebase/SaveScan/auto_Local Disk (G_)*.rsf` — first-hand re-parse this lane, evidence/executions/s10-wholedisk-scan-targets.md); `re-workspace/artifacts/UIUX/07-home-phase-deep-dive.md` + `dynamic-ground-truth--landing-page-windows.md` (the device tree: `Internal drives (2)`, `Windows(C:)` — disk-level selection is a first-class product flow)
**Core references:** audited `84a1832` — `src/core/orchestrator/cr_scan.cpp:1034-1072` (the `fs_hint == Unknown` probe: reads LBA 0 of the SLICE — for a whole-disk view that is the MBR/GPT, never an FS signature), `:1424-1427` (the dispatch default: `rc = OK; /* unknown FS: quick yields nothing; deep/carve still run */`), run()'s `disk_layout_` parse (the table IS read at run start — but only to fill the session's `DiskLayoutEntry` records and the resume revalidation, never to build scan targets), `src/app/rrecov.cpp:10` (the CLI contract line "scan --disk N | --volume C: | --image file [--whole-disk]"), `include/cr/cr_ui.h:331` (`cr_ui_open_disk` — the ABI export). EXECUTED: evidence/executions/s10-wholedisk-scan-targets.md RUN 2 (rc=0, files_found=0, empty tree on the two-volume fixture).
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (push-time reconciliation):** filed as S-050 by this finding's lane; renumbered **S-051** — the SA-10B lane had concurrently filed S-050-file-repair-line-absent-and-unregistered (the earlier remote numbering wins, the session-9 collision convention). CONVERGENT: the remote SA-10 lead lane's close (`bb2cd8a`) performed the same S-050→S-051 repair independently; this lane's renumber and theirs are the same change. All cross-references updated.
**Prior audit check:** newly discovered — no prior finding owns the whole-disk scan-target construction (S-048 owns the carve branch's volume grouping; M-103 the orphan grouping; the lost-partition module owns discovery scoring; the "0 files exit 0" fake-verdict class was fixed for BLIND probes only — the readable-MBR case is this finding's).

## Summary

When the scan target is a whole disk (the CLI `--disk N`, the ABI `cr_ui_open_disk`), our orchestrator never builds scan targets from the disk's partition table: the family probe reads LBA 0, sees the MBR/GPT, returns Unknown, and the engine dispatch's default case completes "successfully" with zero files. The og's disk-level flow reads the partition list at scan setup and makes every known volume a scan target (lost partitions are inserted as additional named targets). On a healthy two-volume disk our product silently reports an empty successful scan where the og recovers both volumes' trees.

## What the reference does

The corpus routing table (architecture/04 §2) sends a disk-level selection to `CDoScanPartition → CGenericRecoveryController`, whose scan setup (§3.4, decompile :904076/:904198) "reads the partition list" and inserts lost partitions as additional scan targets — each target carrying its name (the `%s` in the insertion trace). The real production sessions prove the outcome: the G session's tree contains BOTH volumes of the disk as display-name roots (`Local Disk(G:)` 121,775 rows + `System Reserved (NTFS)` 190 rows — re-parsed first-hand this lane). The og has no "select the disk, get an empty successful scan" path: the partition list is the target-set basis of the disk-level flow.

## What our core does

`cr_ui_open_disk` (cr_ui.h:331) and the CLI's `--disk N` (rrecov.cpp) hand the orchestrator the whole-disk slice. `run()` reads the partition table — but only to fill the session's `DiskLayoutEntry` records and the whole-disk resume revalidation; no scan targets are built from it. The quick/deep engine pass dispatches on `cfg_.fs_hint`; when the host leaves it Unknown (the CLI always does), `probe_family` reads LBA 0 of the slice — the MBR — finds no FS signature, and returns Unknown; the dispatch default then yields `rc = OK` with nothing (cr_scan.cpp:1424-1427). The `probe_unreadable` truth-gate does not fire (the sector was readable). Executed proof (RUN 2): rc=0, files_found=0, deleted_found=0, an empty four-root tree, on a fixture whose two FAT16 volumes each contain a live and a deleted file.

## Gap

Falsifiable (and executed): scan a healthy MBR disk with two known volumes through the plain disk target (`cr_ui_open_disk` + default config, or `scan --disk N` without `--whole-disk`). Our core returns rc=0 with zero files; the og's flow scans both volumes (its real sessions persist both trees). The only rescue in our core is the lost-partition discovery (`--whole-disk`), which re-finds the in-table partitions as +500 table-match candidates — a different mechanism with a divergent presentation (M-104).

## Impact

Every host consumer that opens a disk (the ABI's own first-class export) and scans it with default config gets a silent empty verdict on any partitioned disk — the user is told the scan completed successfully and nothing was found, on a healthy disk whose files the reference product recovers. This is the "appears-alive-while-empty" failure class the trail's fake-scan fix targeted, reached by a different door (a readable non-FS LBA 0 instead of a blind read). Recovery correctness at the product-flow level; user-observable; no data is destroyed (re-running with `--volume` or `--whole-disk` recovers), which is what holds this at SEVERE rather than ACUTE.

## Reproduction / verification

Executed comparison (protocol §6.1): the fixture, probe source, verbatim outputs, and the real-session re-parse are in `evidence/executions/s10-wholedisk-scan-targets.md` (RUN 2 is this finding; RUN 1/RUN 3 bracket it). Corpus citations re-read first-hand (architecture/04 §2/§3.4; the decompile insertion loop; UIUX/07). Core code read first-hand at the cited lines.

## Fix plan (for the engineering team)

1. Architectural: make `run()` (or the quick-stage entry) build scan targets from the partition table when the slice basis is whole-disk (`slice->absolute_base() == 0` and `parse_disk_table` finds entries): one target per used, non-container partition — each a `SliceShim` at the partition's start LBA, dispatched through the existing `run_engine_dispatch` with its own per-target root. The lost-partition discovery stays the `--whole-disk` extra (the og's "additional" targets), and the LDM block keeps precedence (a dynamic disk's volumes come from the database, not the table).
2. Files: `src/core/orchestrator/cr_scan.cpp` (the target builder + the run_quick/run_deep dispatch loop), `include/cr/orchestrator/cr_scan.h` (any per-target telemetry fields), the CLI note at `src/app/rrecov.cpp` (`--whole-disk` help text: discovery only, the volumes are always scanned).
3. Algorithm: probe each table volume independently (per-target `probe_family` on the volume slice, not the disk slice); a volume whose probe fails is skipped honestly (count it in stats), never silently nor fatally; `fs_hint` (when set by the host) applies to the volume targets of that family only — the og's controller builds the full searcher set for the deep flow.
4. Tests: (a) the fixture from the evidence file as a unit test — a two-volume MBR disk scans to both volumes' files WITHOUT `whole_disk`; (b) a GPT twin; (c) a disk with one FAT + one NTFS volume (mixed families, per-target probing); (d) a volume with a damaged boot sector among healthy ones (skipped-honestly, counted); (e) the superfloppy regression (no table → the current single-slice behavior unchanged); (f) the CLI end-to-end `scan --disk` on the fixture.
5. Contract/doc: `docs/CORE-API-CURRENT.md` + `docs/API.md` (the `cr_ui_open_disk` scan semantics: always scans the table's volumes), the CLI help text, `modules/orchestration.md` row updates.
6. Acceptance: the executed probe's RUN 2 flips to files_found=4 / deleted_found=2 (both volumes, live+deleted) with rc=0; the og's real G-session shape (per-volume roots) becomes expressible (composes with M-104's naming); no regression in the existing whole-disk lost-partition suite (the candidates must not double-scan the now-targeted in-table volumes — the discovery's table-match set should be reconciled with the target set, scanning each volume exactly once).

## Tests to add

- `Orchestrator.WholeDiskScansTableVolumes` (the fixture: both volumes' files found without `whole_disk`) — fails before, passes after.
- `Orchestrator.WholeDiskGptTwin` / `Orchestrator.WholeDiskMixedFamilies`.
- `Orchestrator.WholeDiskDamagedVolumeSkippedHonestly` (stats surface the skip; rc stays OK).
- `Orchestrator.WholeDiskSuperfloppyUnchanged` (no table → single-slice dispatch, the current behavior).
- `Orchestrator.WholeDiskNoDoubleScan` (table volumes + discovery: each volume exactly one tree).

## Related findings

M-104 (the presentation half — the same flow's tree shape when discovery is on); S-048 / M-103 (the per-volume branch family: carve and orphan groupings); GAP-S6-6 (the discovery-side dedup class); M-073 (the og's per-folder scan-time filter rides the same CDoScanPartition flow).

## Open questions

The og's per-volume progress weighting when multiple volume targets run in one scan (the corpus's stage-weight decode is single-volume; the multi-volume weighting is a fix-time corpus probe — the qa-logs channel artifacts are the first source to check).

RESOLVED (session-10 record census, evidence addendum): the og persists ONE record-2 VOLUME_INFO per session — the two-volume G session's single record-2 names the PRIMARY volume ("Local Disk (G:)", the artifact's exact 147-byte layout confirmed first-hand); the multi-volume registry is record-1's special-partition list (the G session carries ONE entry: System Reserved @ start LBA 2048, fieldB=204,799; the single-volume H/I sessions carry entryCount=0). Our singular VolumeSessionInfo is record-level equivalent for the primary volume; the fix needs no session-model change.
