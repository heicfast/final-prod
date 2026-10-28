# MINOR N-145 — the cross-run apply-interruption marker absent: the og marks the target disk in `../res/rwConfig.ini [QuickPartition] pendingMarkedDisk` when a whole-disk repartition op is in flight (engine-side event 0x102d); core's CRUNDO01 transaction has no cross-run in-flight detection

**Severity:** MINOR (a crash/interruption posture difference: the og leaves a cross-run, cross-DLL marker so the next start can detect an interrupted apply; core's per-operation undo container is stronger per-op but leaves no trace if the process dies mid-apply)
**Module:** apply / orchestrator (the interruption posture)
**Parameters affected:** A34 (checkpoint — the cross-run interruption record), A35 (error recovery — the next-start detection), A36-adjacent (session lifecycle's crash semantics)
**Corpus references:** `ghidra-partition/RecoveryModule_decompiled.c:48164-48315` — the shared epmpublic marker utility: `QString(local_150, "../res/rwConfig.ini")` (`:48164`), group `"QuickPartition"` (`:48186`), key `"pendingMarkedDisk"` (`:48188`), the add-vs-remove split on `param_2` (`:48210-48244`), the clear-all variant `FUN_180064070` (`:48280-48340`); static-linked ×6 (MainModule `:111316-111412`, ToolsModule `:127746-127873`, PartitionRecovery `:34640-34767`, UILogic `:39330+`, CloneModule `:120285+`); the engine-side trigger: `MainModule_decompiled.c:123130-123144` — on **event 0x102d**, `__RTDynamicCast` to `CDiskRepartitionPendingOperation`, extract the disk (vtable+0xe0), mark/unmark; the same sink in `UILogic_decompiled.c:27690`
**Core references:** audited `461825d8`-lineage — `include/cr/apply/cr_apply.h:221-253` + `src/core/apply/cr_apply.cpp:348-579` (the CRUNDO01 transactional undo container + sector-0 revalidation); the whole-repo grep `pendingMarkedDisk|rwConfig|QuickPartition` → **zero hits** (Lane D agent, lead re-verified)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered by the S11-lead Lane D tree sweep (the EPM-heritage shared layer); NOT covered by S-029 (the apply transaction — whose per-op undo this complements), M-113/M-114 (the same sweep's other write-side finds), or any D-row.

## Summary

When the og's pending-op engine executes a whole-disk repartition (the disk-copy/repartition pair class), it writes the target disk into a pipe-separated list in a QSettings ini next to the process working directory, and removes it when the operation completes. The next product start can read the marker and detect an interrupted apply. Core's apply engine is a per-operation transactional container (CRUNDO01) with sector-0 revalidation — stronger guarantees while the process lives, but if the process dies mid-apply, nothing on disk records that an apply was in flight.

## What the reference does

The marker utility (cites above) maintains the pendingMarkedDisk list; the
engine-side event 0x102d (the repartition op's lifecycle) drives mark/unmark. The
ini is not shipped (created on first write) — a runtime artifact by design.

## What our core does

No cross-run marker of any kind; the undo container lives only for the
operation's lifetime.

## Gap

Falsifiable: kill the og's process mid-repartition → the next start's
rwConfig.ini still lists the disk (the marker survived); kill core mid-apply →
no artifact records the in-flight state (only the half-written disk itself).

## Impact

A crash-recovery posture difference: the og can warn/refuse/re-offer on an
interrupted destructive op; core's next run sees only the disk's half-applied
state. The core's per-op undo is arguably the better mechanism while alive — the
missing piece is purely the cross-run trace. Low practical weight (process death
mid-apply is rare, and the undo container's sector-0 revalidation catches many
residue cases), hence MINOR.

## Reproduction / verification

Decompiled evidence (Lane D sweep; the QSettings chain re-read by the lead at
filing — the path/group/key strings at :48164-:48188 match). Core-side absence
grep-proven. Full record: `evidence/s11-lead-deep-lanes.md` §Lane-D item D-3.

## Fix plan (for the engineering team)

1. Decide the posture: recommended = a first-class in-flight journal beside the
   undo container — at apply-begin, write a marker (the existing session/state
   directory is the natural home, NOT the CWD-relative ini the og used — that
   CWD-relative choice is an og defect worth declining); at apply-end/undo-end,
   remove it; at engine init, surface any surviving marker through the existing
   health/warning surface (the same channel S-029's residue checks use).
2. Files: `cr_apply.cpp` (the journal write/remove + the init probe), the apply
   header's contract block, one test.
3. Tests: begin-apply → marker exists; complete/undo → marker gone; a
   simulated crash (no cleanup) → the init probe reports the in-flight disk.
4. Acceptance: the three-state lifecycle pinned; the marker's location is the
   state directory (the CWD-relative decline registered in the D-row).

## Tests to add

- `Apply.InFlightJournalLifecycle` (begin/complete/crash-detect).

## Related findings

S-029 (the apply transaction this journals); M-114 (the same op-family's other
host-state behavior); M-113 (the same sweep's boot-surgery find).

## Open questions

Whether the og's next-start consumer actually reads the marker (the reader was
not decoded — the UILogic sink at :27690 suggests UI-side handling; the fix
lane's design does not depend on it, but the D-row should note the unread).
