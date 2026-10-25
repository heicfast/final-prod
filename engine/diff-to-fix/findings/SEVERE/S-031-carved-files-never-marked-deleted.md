
---

## Session-6 fix verification (2026-10-26, first-hand at `517c56c`)

The parallel fix leg's `4001756` ("both carve emit sites patched") verified in-source:
`src/core/carve/cr_carve.cpp:213` (grammar layer, with the census citation in the
comment) and `:373` (raw-sign layer) both call `set_deleted_state(DeletedState::Deleted)`
— the two sites this finding's merged-tree re-verification and the session-6 convergence
addendum independently identified. The deleted-flag half of this finding is **FIXED**.

**Still open (the session-6 convergence addendum's item 2):** the scan-timestamp half —
the og persists a per-carved-file timestamp (FUN_140c41aa0, the formatted
`"%d-%d-%d %d-%d-%d"` scan-time string); `cr_carve.cpp` stamps no time field at either
site (grep-verified at the same tree). Recommend the fix leg take the timestamp as the
finding's residual scope (or split it to a MINOR with the round-trip test from the
addendum's fix-plan step 3).
