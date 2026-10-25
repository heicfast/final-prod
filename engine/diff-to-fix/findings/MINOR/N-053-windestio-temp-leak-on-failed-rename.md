# MINOR N-053 — WinDestIo: failed rename leaks the .part-N temp file (commit nulls the handle before MoveFileExW; close()'s delete gate requires a live handle) + UNC path untested

**Severity:** MINOR
**Module:** writer (destination I/O)
**Parameters affected:** F62 (destination semantics — atomic-rename contract), Q65 (test coverage — no UNC/failed-rename test), A35 (error recovery — the leak survives the failure path)
**Corpus references:** `re-workspace/artifacts/UIUX/11-recovery-phase-funnel.md` (destination writing is staged temp-then-rename; the og's recovery funnel never leaves partial finals) — presentation anchor; the engine-side invariant is the atomic-rename contract both implementations follow
**Core references:** `src/win/cr_destio.cpp:44-71` (`commit()`: CloseHandle → `it->second.handle = nullptr` → MoveFileExW; on MoveFileExW failure returns ERR_WRITE_FAILED with the handle already null; `close()`: `if (!temp_path.empty() && it->second.handle) DeleteFileW(...)` — the null handle skips the delete) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`commit()` closes the handle and nulls it BEFORE the rename; if `MoveFileExW` fails, the
later `close()` skips its temp-file deletion because the gate requires a non-null handle —
the `.part-N` temp file is leaked on the destination volume. Separately, the UNC-path
(`\\\\?\\UNC\\...`) branch of path building has no test.

## What the reference does

Staged temp-then-atomic-rename on the recovery funnel (UIUX 11) — no partial finals and no
orphaned temps on failure in the og's flow.

## What our core does

Quoted chain above: the delete gate's handle condition can never be true after a failed
commit. The temp file remains on disk until the user manually cleans it. (The
ERROR_IO_PENDING branch in the overlapped write path was also reviewed this session — it
is reachable for overlapped handles and is NOT a defect; an earlier scratch-note claim to
that effect is withdrawn.)

## Gap

Falsifiable: open an item, write, call `commit()` with the final path unwritable (e.g.
read-only destination dir) → ERR_WRITE_FAILED; the `.part-N` file remains in the
destination directory afterwards.

## Impact

Orphaned temp files on the destination on rename failure — a hygiene/behavior defect users
see (half-completed recoveries leave litter); no data-loss (the final was never created).
MINOR.

## Reproduction / verification

Core source read first-hand this session (the exact gate quoted). Executed repro deferred
to the fix leg (Windows lane; the Linux CI cannot exercise WinDestIo).

## Fix plan

1. In `commit()`, on MoveFileExW failure: delete the temp file before returning (or defer
   the CloseHandle/null until after a successful rename and let `close()` do the delete —
   either order fixes the gate).
2. Add a Windows-lane test with an unwritable final path asserting the temp is removed.
3. Add a UNC-path construction test (destination on a network share path form).

## Tests to add

- `DestIo.FailedRenameCleansTemp` (Windows lane)
- `DestIo.UncPathConstruction`

## Related findings

- N-041 (dead-code family in the win layer — adjacent cleanup wave)

## Open questions

None.
