# MAJOR M-114 — MainModule's silent System-Restore purge is absent and undecided: the og deletes ALL restore points on BOTH drives of a copy/repartition operation (SRRemoveRestorePoint via WMI enumerate-all, 510-index fallback); zero core coverage, zero corpus documentation

**Severity:** MAJOR (an engine-adjacent destructive host-state operation the merged product must at least DECIDE about — matching it means deleting user restore points, not matching it means leaving state the og deliberately purges as unbootable-Windows prevention; either way the behavior gap is unregistered and the og's side was never even documented in the corpus)
**Module:** apply / orchestrator (the pending-op engine's host-environment side-effects)
**Parameters affected:** P60-adjacent (bad-sector/unsafe-condition handling class — a destructive host op), A35 (error recovery — the op's place in the apply chain), D46 (documentation of the contract)
**Corpus references:** `ghidra-partition/MainModule_decompiled.c:138378-138415` — `FUN_1800d7a60`: `LoadLibraryW(L"srclient.dll")` → `GetProcAddress("SRRemoveRestorePoint")` → `CoInitializeSecurity` → WMI connect `root\default`; `:138773` — the WQL `"Select * from SystemRestore"` enumeration; `:138812` — reading each result's `SequenceNumber`; `:138610-138634` — `FUN_1800d8320` calling `SRRemoveRestorePoint(seq)` per point; `:138415` — the 0x1fe (510) iteration bound; `:138418-138420` — the fallback loop driving indices 0..0x1fe directly; the caller `FUN_180060e50` at `:71496` and `:71511` — takes the operation's two device objects (`+0x588`/`+0x590`), resolves each one's drive letter (vtable+0x30, 'b'..'y' range check, `:71458-71476`), and invokes the purge for BOTH drives — i.e., around a disk-copy/repartition pair; MainModule-only (grep `srclient` across ToolsModule/CloneModule/RecoveryModule/PartitionRecovery: 0 each)
**Core references:** audited `461825d8`-lineage — `grep -rn "srclient|SRRemoveRestorePoint|SystemRestore|restore point" src/ include/ tests/` → **zero hits** (Lane D agent, lead re-verified); the apply chain `src/core/apply/cr_apply.cpp:348-579` + `include/cr/apply/cr_apply.h:221-253` (the CRUNDO01 transactional container — no host-environment side-effects at all, by design)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11-lead Lane D tree sweep; zero trail coverage (`grep srclient|SystemRestore|restore point` across docs/ + diff-to-fix/ → 0) and zero CORPUS-artifact coverage (not even string-listed in `artifacts/per-binary/MainModule.md` — the Lane D agent's grep). Not covered by S-029 (the apply transaction itself), M-113 (the boot surgery — a different write side), or any host-contract row.

## Summary

Around destructive disk operations (copy/repartition pairs), the og's pending-op engine silently deletes every System Restore point on both involved drives — a deliberate hygiene operation (stale restore points after disk rewrites are a known cause of unbootable Windows and of restore-to-corrupt failures). Core's apply engine is strictly transactional on the disk itself and performs no host-environment operations. Neither implementing nor declining this behavior has been registered anywhere — and the og's behavior was never even documented in the corpus's own per-binary artifacts, so the parity question has never been asked.

## What the reference does

The decompile chain (all cites above): on the apply path, for each of the two
device objects of the pending operation, resolve the drive letter, then via
srclient.dll's SRRemoveRestorePoint: enumerate every SystemRestore instance via
WQL and remove each by sequence number, with a 510-index fallback sweep if the WMI
path fails. The purge is silent (no user prompt visible in this chain) and
MainModule-scoped.

## What our core does

Nothing — by design, the apply engine touches only the target disk's sectors
(the CRUNDO01 transaction). No srclient usage, no WMI, no host-state operations.

## Gap

Falsifiable: run a disk-copy/repartition apply in the og with System Restore
enabled on both drives → all restore points on both drives are gone afterward.
Run core's apply → all restore points remain. The behaviors differ, and no D-row
or finding records the decision.

## Impact

Two-sided: (a) if the merged product does NOT match the og, users who re-partition
with core keep restore points that reference the pre-rewrite disk layout —
restoring from them can produce exactly the unbootable state the og's purge
prevents (a user-safety consequence, not merely cosmetic); (b) if the product DOES
match it, it silently deletes user data (restore points) — a consent posture
question that belongs to the host layer per the in-process doctrine. Either way
the divergence is real, engine-adjacent (the og reaches it from the pending-op
engine), and currently invisible to the parity board.

## Reproduction / verification

Decompiled evidence (Lane D sweep; the load-bearing chain re-read by the lead at
filing — the srclient/GetProcAddress pair at :138378-138415 and the WQL string at
:138773 match). Core-side absence grep-proven. Execution comparison not feasible
(needs a Windows host with System Restore enabled); the divergence is structural.
Full record: `evidence/s11-lead-deep-lanes.md` §Lane-D item D-2.

## Fix plan (for the engineering team)

1. Decide the posture first (this is a product decision, not a mechanical fix):
   the recommended shape is a HOST-side consent-gated operation (the RawFixer
   consent precedent) — the core exposes a plan item ("purge restore points on
   drives X, Y") in the apply plan's host-visible operations, and the host
   executes it with user consent; the core itself never calls srclient (matching
   the in-process doctrine and the SDDL/safety posture).
2. If registered as deliberate-divergence instead: a D-row (PARITY-DELTAS) with
   the safety rationale (silent deletion of user restore points without consent
   is a behavior the merged product may legitimately decline) + the user-safety
   note about stale points.
3. Either way: the corpus artifact gap — add the behavior to
   `artifacts/per-binary/MainModule.md` (the corpus is read-only for us; record in
   CORPUS-ERRATA for the corpus maintainers).
4. Tests: if the plan-item shape is chosen — a plan-surface test asserting the
   purge item appears for copy/repartition ops with the right drive letters and
   is absent for single-disk non-destructive ops.
5. Acceptance: the posture is registered (code or D-row); REFERENCE-API-GAP or
   PARITY-DELTAS carries the decision; no silent divergence remains.

## Tests to add

- `Apply.PlanExposesRestorePurgeItem` (if the plan-item shape is chosen).
- The D-row registration itself is the acceptance for the decline shape.

## Related findings

M-113 (the same sweep's other MainModule/RecoveryModule write-side find); S-029
(the apply transaction the plan item would ride); M-075 (chkdsk launch — the
adjacent host-tool invocation family and its consent posture); the RawFixer
consent precedent (modules/rawfixer.md).

## Open questions

Whether the og's purge is truly unconditional in the shipped product (a config
gate may exist upstream of the caller chain — the two call sites at :71496/:71511
are in the pending-op executor, but the operation-construction side was not
decoded this session). The fix lane's posture decision should decode the
construction site first.
