# SEVERE S-029 — Lost-partition APPLY path: the og writes the recovered partition into the table; the core is contractually read-only

**Severity:** SEVERE
**Module:** lost-partition (apply/write path)
**Parameters affected:** P1 (detection→application), P5 (orphan reconstruction — endgame), P42 (public surface — apply operation), P54 (module boundaries — write layer), P62 (destination semantics — the user's table), P69 (stub scan — "never repaired" contract)
**Corpus references:** `re-workspace/reversed/ghidra-partition/PartitionRecovery_decompiled.c` — `FUN_1800566d0` "rebuild disk layout" (:46887-47020: DeviceIoControl 0x7C100 DELETE_DRIVE_LAYOUT :46953; raw sector-0 read :46944; zero entries 0x1BE..0x1FD + 55AA write :46957-46962; **0x7C010 IOCTL_DISK_SET_DRIVE_LAYOUT, 4-entry DRIVE_LAYOUT_INFORMATION, 0xA8** :46995-47002; modern 0x7C058 0x18-byte SET_PARTITION_INFO_EX-style :47015), per-partition writer `FUN_180038b10` (:32548-32672), MBR-entry wipe `FUN_180039030` (:32700-32790), **GPT variant `FUN_1800392f0`** (:32797-32860: `GPT_GenerateSignature` new disk GUID :32816; signature set via interface vtable+0x80 :32825), protective entry `FUN_180039470` (:32863-32920), IOCTL_DISK_UPDATE_PROPERTIES 0x70140 with 3×Sleep(1000) retry (:46873-46880); `RecoveryModule` export table: **`CreateRecoveryPartitionObject`**, `CreateScanLostPartitionObject`, `CreateCheckPartitionObject`, `CreateEnumPartitionObject`, `CreateExplorerPartitionObject`; `IScanLostPartition` classes (:14083-14162); `MainModule_decompiled.c` `FUN_180076bb0` layout write (:82932) + `FUN_180075580` post-write boot fixup (:82322-82424); `artifacts/PartitionRecovery/pending-operation-engine.md` (op vocabulary incl. CSmartCreatePendingOperation); `artifacts/PartitionRecovery/mo-module-map.md` insight 6
**Core references:** audited `7cf6754` — `include/cr/lost/cr_lostpart.h:91-92` ("Materialize a candidate as a bounded virtual volume (R3.2: read-only, the user's table is never repaired)"); `src/core/lost/cr_candidates.cpp` (probe family — discovery only); `src/core/orchestrator/cr_scan.cpp:1227-1238/1293-1310` (candidates persisted/restored as virtual context, never applied); grep-verified zero hits for SET_DRIVE_LAYOUT/DELETE_DRIVE_LAYOUT/0x7C058/pending-op engine across src/ + include/
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's lost-partition flow is a two-step product feature: discover the lost partition (the decoded probe family, S-006/S-019 parity), then **APPLY it — write the recovered partition entry back into the user's MBR/GPT** via a dedicated write path (RecoveryModule's `CreateRecoveryPartitionObject` → PartitionRecovery.dll's rebuild-layout IOCTL chain), followed by boot-config repair. The core implements only discovery: its lost-partition contract is explicitly read-only ("the user's table is never repaired", cr_lostpart.h:91-92), and it has no table-write path at all. The endgame of lost-partition recovery — the operation the product is named after — is absent.

## What the reference does

RecoveryModule.dll (the recovery-scenario host) exposes the object factory
`CreateRecoveryPartitionObject` alongside `CreateScanLostPartitionObject` —
the scan and the apply are separate scenario objects in the og's API. The
apply path (decoded in PartitionRecovery.dll, which carries a duplicate of
the same engine) runs `FUN_1800566d0` "rebuild disk layout": it acquires the
block-device interface via `IObject::GetInterfaceEx` (PR:46844-46848), then
on the legacy path issues `IOCTL_DISK_DELETE_DRIVE_LAYOUT` (0x7C100,
PR:46953), re-reads sector 0 raw, zeroes all four MBR entries and re-writes
the 0x55AA signature (PR:46957-46962), and issues
`IOCTL_DISK_SET_DRIVE_LAYOUT` (0x7C010) with a 4-entry
DRIVE_LAYOUT_INFORMATION (0xA8 bytes, PR:46995-47002). On modern OSes it
takes the single-entry path `IOCTL 0x7C058` with a 0x18-byte
SET_PARTITION_INFO_EX-shaped record (PR:47015). The GPT variant
(`FUN_1800392f0`, PR:32797-32860) regenerates the disk GUID
(`GPT_GenerateSignature`, PR:32816) when rebuilding the table. Every write
is followed by `IOCTL_DISK_UPDATE_PROPERTIES` (0x70140) with a 3×Sleep(1000)
retry (PR:46873-46880). The same composite operation chains `BootIni_Init`
and the Fixup.mo BCD repair (PR:47290-47302) — apply and boot repair are one
unit in the og (see M-040).

## What our core does

The core surfaces `lost::Candidate` records (cr_candidates.cpp probe family,
S-006/S-019 parity), materializes a candidate as a **bounded virtual volume
for scanning** (`materialize()`, cr_lostpart.h:91-92 — "R3.2: read-only, the
user's table is never repaired"), persists/restores candidates through the
session format as virtual context (cr_scan.cpp:1227-1238, 1293-1310), and
stops there. There is no apply operation, no table-write IOCTL anywhere in
the tree, no pending-operation engine, and no write-layer module. The CLI can
scan a materialized candidate and recover files from it, but a user who wants
the partition itself restored (the og product's headline flow) cannot get
there from the core.

## Gap

The og exposes an apply-the-recovered-partition write path (RecoveryModule
`CreateRecoveryPartitionObject` → PRD IOCTL chain 0x7C100/0x7C010/0x7C058 +
GPT GUID regen + 0x70140 refresh); the core has no equivalent and its
contract forbids one. Falsifiable: no core code path issues
IOCTL_DISK_SET_DRIVE_LAYOUT/DELETE_DRIVE_LAYOUT or writes LBA 0 (grep-verified
zero hits).

## Impact

Users of the core (and any UI built on it) can discover lost partitions but
cannot complete the recovery the og product performs — restoring the
partition entry so the OS mounts the volume again. This is the endgame of
the lost-partition module and a user-observable capability gap of the
highest order for that flow. NOTE for triage: the read-only stance is a
deliberate safety posture (R3.2) but was never registered as an audited
deliberate-difference against the og's apply capability — this finding
forces the scope decision: either implement a consent-gated apply path
(mirroring the rawfixer consent model the core already has) or register the
exclusion as a documented deliberate delta in PARITY-DELTAS with the
product-level consequence stated.

## Reproduction / verification

Corpus-derived (decompiled evidence, lead-verified first-hand): the PR
IOCTL chain quoted above was re-read in the decompile at PR:46944-47017
(this session); the factory names are from the RecoveryModule export table
(PE dump). Core side grep-verified zero hits. Execution not possible
(Windows-only write path; no fixtures in scope). Evidence:
`diff-to-fix/evidence/decompiled-quotes/s5-mom-capabilities.md` §5.

## Fix plan (for the engineering team)

1. **Scope decision first** (one sprint): implement consent-gated apply, or
   register the documented exclusion. This finding assumes implementation.
2. Architecture: a new `apply` module (`src/core/apply/cr_apply.cpp`,
   `include/cr/apply/cr_apply.h`) behind the same consent ABI pattern as
   rawfixer (`cr_ui_repair_consent_fn`): `plan_apply(candidate, disk) ->
   ApplyPlan`, `execute_apply(plan, IWriteDevice)`, `undo_apply(plan)` —
   never called without an explicit consent callback.
3. Write path per the decoded og sequence: lock/dismount the disk's volumes
   (reuse the S-010 quiesce sequence), `IOCTL_DISK_DELETE_DRIVE_LAYOUT`,
   rewrite sector 0 (zero entries + 55AA on MBR; GPT: regenerate disk GUID
   only when rebuilding the header), `IOCTL_DISK_SET_DRIVE_LAYOUT_EX` /
   `SET_PARTITION_INFO_EX`, then `IOCTL_DISK_UPDATE_PROPERTIES` with retry.
   MBR-path first; GPT protective entry when converting layouts is OUT of
   scope for v1 (og `FUN_180039470`).
4. Boot-config side effects (BCD/boot.ini) are M-040's scope; gate the
   apply plan on "boot repair available/consented" as a plan field.
5. Undo: sector-0 backup + full-sector undo log (CRUNDO01 pattern) before
   the first write; restore + UPDATE_PROPERTIES on rollback.
6. Tests: `LostApplyWritesEntryAndRefreshes` (fixture disk image, assert
   sector-0 bytes + layout after), `LostApplyGptRegeneratesGuid`,
   `LostApplyRefusedWithoutConsent`, `LostApplyUndoRestoresSector0`,
   `LostApplyQuiescesVolumesFirst`. Windows CI leg required.
7. Contract/doc updates: cr_lostpart.h R3.2 comment superseded;
   PARITY-DELTAS gains the apply entry (implemented) — or the exclusion
   entry if step 1 chooses exclusion.

## Tests to add

See fix plan step 6 — the five named tests, all fail-before by absence.

## Related findings

M-040 (boot-chain repair — the og chains it into apply), M-042 (lostpart.dat
persistence — the og's applied-flag semantics), S-008/S-010 (the rawfixer
consent + quiesce patterns to reuse), I-019 (the EPM tool-op boundary —
where NOT to draw the apply line).

## Open questions

The og's exact undo semantics on apply failure (MM undo chain runs chkdsk +
Bcd_RestoreBcdFile, MM:83206-83212 — decoded; whether PartitionRecovery.dll's
apply path has its own sector-0 rollback beyond MainModule's op engine was
not decoded this session).

## Resolution (session 10, 2026-10-26)

**Status:** FIXED (v1 scope per the finding's own fix plan: MBR path, consent-gated, transactional, undoable; GPT variant + boot-chain composite out of scope and refused cleanly)
**Fixed in commit:** (this session's S-029 commit — see `git log` for "S-029")
**Verified by:** the 10-test `Apply.*` suite: `PlansEntryIntoFirstFreeSlot`, `ExecuteWritesEntryVerifiesAndUndoes` (incl. the byte-exact undo roundtrip), `RefusedWithoutConsent` (denied AND absent callbacks → `ERR_APPLY_CONSENT_DENIED` with zero writes), `StaleTableRefusedBeforeConsent` (`ERR_APPLY_STALE`, consent never reached), `QuiesceFailureIsACleanRefusal` (`ERR_VOLUME_LOCK_FAILED`, zero writes), `WriteFailureRollsBackEverything` (mid-transaction failure → byte-exact rollback), `ContainmentRefusals` (overlap / LBA 0 / past-end / full-table / GPT / zero-size), `SignatureRestorePlannedWhenAbsent`, `ChsClampsBeyondCylinder1023` (clamp + real-CHS math), `FamilyTypeMap`. Full suite **590/590** Linux ASan at the fix tree (577 baseline + 10 apply + 3 parallel-lane tests). The Windows-only surfaces (the `rrecov apply` CLI command, `win::refresh_disk_properties` with the og's IOCTL_DISK_UPDATE_PROPERTIES + 3×Sleep(1000) settle) compile on the MSVC CI lane — the pure-core plan/execute/undo logic is fully proven on Linux; the win lane is compile-checked + CI-executed (recorded, not claimed as locally executed).

**Resolution notes:** The fix implements the finding's fix plan steps 2-7 on the
rawfixer seams. New module `cr/apply` (`plan_apply` / `execute_apply` / the shared
`repair::undo`): the plan is READ-ONLY (slot pick = first zero entry; containment
refusals for overlap/LBA-0/past-disk/GPT/full-table/zero-size; the CHS pair at the
255/63 convention with the cylinder-1023 clamp; the 0x55AA restore planned only when
absent — the og rewrites it, PR:46957-46962; the family→type map D-69); execution is
A1-A4: quiesce via `IWriteDevice::lock_volume` (the S-010 sequence in the win
transport — busy volume = clean refusal), the A2 stale-table revalidation (the plan's
sector-0 snapshot vs the live disk — the og re-reads sector 0 raw, PR:46944), the A1
consent callback (denied OR absent = `ERR_APPLY_CONSENT_DENIED`, zero writes), then
the A4 transaction (snapshot → write → read-back verify → reverse-order rollback on
any failure). The undo is the shared CRUNDO01 container — `rrecov fix --undo-apply`
already replays it. CLI: `rrecov apply --disk N --session file.crsf --candidate N
[--apply] [--undo file]` — dry-run default, `--apply` is the explicit consent (the
`fix` posture), candidates come from the whole-disk scan's saved session (apply
transcribes a scored candidate, never invents geometry), and the post-write
`IOCTL_DISK_UPDATE_PROPERTIES` settle lands through the new
`win::refresh_disk_properties` (best-effort, surfaced, never fails the apply).
`cr_lostpart.h`'s R3.2 comment superseded (the materializer stays read-only BY
DESIGN as the recovery path; the write path is the separate consent-gated module).
PARITY-DELTAS D-68/D-69 register the v1 boundaries and the type-map convention.
Out of scope, refused cleanly, tracked: the GPT variant (FUN_1800392f0 GUID regen),
the boot-chain composite (M-040 — the plan's `boot_repair_available` field states
the boundary), and the cr_ui ABI surface for apply (the CLI is the v1 consumer; the
UI DLL surface follows when a UI consumer exists — registered as the follow-up).
