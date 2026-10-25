# ACUTE A-003 — `rrecov fix --part N --apply` wrote the repair at DISK LBA 0 (slice-relative plan offsets never rebased onto the whole-disk write device)

> **ID note:** originally filed by the continuation leg as its local A-002;
> renumbered to A-003 when the diff-audit-s2 leg's A-002 (the A-001
> fix-induced LDM-GUID regression, `23adad1`) landed on main first — same
> collision-clearing convention the other legs used (M-007→M-010, N-010→N-021).

**Severity:** ACUTE (discovered divergence — silent destruction of the disk's
partition table on an explicit repair command; a second, undo-direction
instance in the S-008 scan path)
**Module:** cli (fix command), repair (offset-basis contract), orchestration
(S-008 pre-scan wiring)
**Parameters affected:** F63 (RawFixer consent/gating — the consented repair
lands at the WRONG location), R57 (wiring — slice basis vs parent basis)
**Corpus references:** N/A (core-internal defect — the og has no CLI; the
analogous reference contract is RawFixer operating on the volume's own device
context, where slice basis == device basis by construction)
**Core references:** `src/app/rrecov.cpp` cmd_fix (audited `67193b6` through
`1882847`: analyze ran on the `PartitionSlice` view but `execute` was handed
the whole-disk `open_disk_rw` device with no offset translation; the S-008
pre-scan path had the inverse instance — volume-scan undo logs carried
slice-relative offsets that `fix --undo-apply` replays against the whole
disk). Discovered by the continuation remediation leg during its S-008
verification pass; the parallel S-008 fix (`e0175b3`) left both instances
open. No unit coverage existed for the `--part` path.
**Status:** READ-ONLY FINDING — filed by the remediation session (§8: discovered
divergences get finding files, are announced in the commit, and are fixed in
the same session)

## Summary

`rrecov fix --disk 0 --part 2 --apply` analyzed partition 2 through a
`PartitionSlice` (child offset 0 == partition start), producing a repair plan
whose action offsets are SLICE-RELATIVE (the boot restore always plans offset
0). `execute()` was then called on the whole-`\\.\PhysicalDrive0` write device
— so the "repair" wrote partition 2's boot sector to **LBA 0 of the physical
disk**, destroying the MBR/GPT. The undo log (snapshotted before the
destructive write) allowed `--undo-apply` recovery in principle, but the write
itself was data-destroying and the dry-run display showed the misleading
slice-relative offsets.

The S-008 pre-scan repair carried the UNDO-DIRECTION instance: for volume
scans the parallel leg's writer adapters (`VolumeWriteSlice` /
`UiVolumeWriteSlice` / `ImageWriteDevice` at the image offset) mapped
slice-relative plan offsets onto the parent, so the executed repair landed
correctly — but the undo log recorded the SLICE-relative offsets, and the
documented `fix --undo-apply` contract ("absolute sector offsets for a
specific disk") replays the log against the whole-disk device: a volume-scan
undo would restore sectors at the WRONG disk offsets.

## Gap

Two falsifiable claims:
1. On a two-partition MBR disk where partition 2's boot sector is damaged and
   a valid backup exists, `rrecov fix --disk 0 --part 2 --apply` (pre-fix)
   overwrites the MBR signature at disk LBA 0 with partition 2's boot sector.
2. A volume scan's saved pre-scan-repair undo log (pre-fix) carries
   slice-relative offsets; `fix --undo-apply` replays them against the whole
   disk, restoring the original bytes at the wrong locations.

## Impact

- Silent partition-table destruction on the exact "repair" command a user runs
  to SAVE a damaged volume — the worst possible blast radius for a
  write-capable feature.
- The undo path (the operator's safety net) is equally misplaced for volume
  scans.
- Only reachable with explicit `--apply` (dry-run is the default) or a
  consented pre-scan repair — but the consent was given for repairing the
  VOLUME, never the disk header.

## Reproduction / verification

- Core-derived: `PartitionSlice::read_at` rebases child offsets to
  `start_ + offset` (`cr_slice.cpp:43-50`) — plans carry child-relative
  offsets; `open_disk_rw` opens `\\.\PhysicalDriveN` whole
  (`cr_transport.cpp`) — execute's offsets are interpreted disk-relative. No
  translation existed between the two. The unit suite's RamWriteDevice tests
  always analyzed and executed on the SAME device, which is why the basis
  mismatch never fired in CI.
- Executed: `RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0` is the
  machine proof — it analyzes through a partition slice in a disk image,
  applies the rebase, executes against the whole-disk device, and asserts the
  write landed at the partition-absolute offset AND that disk LBA 0 stayed
  byte-identical.

## Fix plan

1. `repair::rebase_plan(plan, base_bytes)` in the core (tested, reusable):
   shifts every action offset by the slice's base. The plan handed to the
   consent callback stays slice-relative (it describes the scanned volume);
   the rebase is applied after consent, immediately before execute.
2. One basis rule everywhere: write devices are always PARENT devices (whole
   disk / whole image file); plans are rebased to the parent basis before
   execute. The orchestrator's pre-scan stage rebases by
   `slice->absolute_base()`; `cmd_fix` rebases by the partition's base LBA
   (before the dry-run display, so the shown offsets are the true targets).
3. The S-008 writer adapters (`VolumeWriteSlice`, `UiVolumeWriteSlice`) are
   removed — with plans rebased, mapping the writer too would DOUBLE the
   offset. `ImageWriteDevice` opens the image at base 0 (parent basis).

## Tests to add

- `RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0` (fail-before at
  the defect: without the rebase the same execute writes at disk offset 0 —
  the MBR assertions fail).

## Related findings

- S-008 (the pre-scan wiring this finding's second instance lives in; both
  instances are fixed in the same design — one basis mechanism).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: A-003`)
**Verified by:** `RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0`
(slice-analyzed plan + rebase + whole-disk execute → write at
`base + slice_offset`, disk LBA 0 byte-identical, undo log disk-absolute);
the existing `Orchestrator.PreScanRepair*` and `CrUi.PreScanRepair*` suites
stay green (their devices are base-0, so the rebase is a no-op there — and
the new basis rule is what makes the volume-scan case correct).
**Resolution notes:** CONFIRMED (code-path proof; no corpus reference — the
reference never mixes a slice view with a whole-disk write device). The fix
establishes ONE offset-basis mechanism: `repair::rebase_plan` (core, unit
proven) applied by the orchestrator (`slice->absolute_base()`) and cmd_fix
(`part_base`, before the plan display, labeled "(disk-absolute offsets)").
The S-008 writer adapters are removed (`VolumeWriteSlice`,
`UiVolumeWriteSlice`); `ImageWriteDevice` opens at base 0; the CLI/UI hooks
return the unmapped parent writer. Undo logs — and `fix --undo-apply` — now
always carry parent-absolute offsets, matching the documented contract. The
consent callback still sees slice-relative offsets (the scanned volume's own
coordinates). Whole-disk `fix` and whole-disk scans are unchanged (base 0).
