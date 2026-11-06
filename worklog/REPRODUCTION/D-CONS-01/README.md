# D-CONS-01 — visible console shells when opening disk health

**Reproduced on:** the shipped v0.8.4 release (forensic run
[42255492260](https://github.com/heicfast/final-prod/actions/runs/42255492260));
the mechanism is code-level and fires on EVERY health read.

## Expected

No user-facing operation may spawn a visible console window. Every
subprocess invisible, bounded, silent.

## Observed

- `engine/src/win/cr_health.cpp:306` (at the v0.8.4 commit):
  `_popen("fsutil behavior query disabledeletenotify 2>nul", "r")` —
  the CRT pipe-spawn launches `cmd.exe`; in the GUI-subsystem product
  process (cr_ui.dll is loaded in-process by the Tauri shell) `_popen`
  ALLOCATES A CONSOLE WINDOW on the user's desktop. This is the user's
  "on opening disk health some kind of popup shells are appearing,
  probably cmd or powershell".
- Reached on EVERY health read: `cr_ui_health_read` calls
  `query_trim_state` unconditionally (`cr_ui.cpp:4948`).
- It is a REGRESSION: the 0.6.x era fixed this natively (NG-8); the
  engine re-merge (283a5d90) swapped in core_recovery's `_popen`
  version (REPO-STATE §the regression).
- Runner-capture caveat (recorded honestly): the probe's child-process
  census samples at 2 s intervals — a millisecond-scale console flash
  is not statistically catchable by sampling, and the probe's window
  census covers the INSTALL window only. The code path + CRT console
  semantics + the user's report are the evidence; the census-gap is
  itself a probe finding (the re-vehicled gate moves to a build-time
  tree-wide `_popen` guard, which cannot miss).

## Break point

The health TRIM query's process vehicle — `_popen` in a GUI process.

## Fix (landed)

Lane B's D2 (commit 68460a4c): fsutil.exe now runs DIRECTLY via
CreatePipe + CreateProcessW with `CREATE_NO_WINDOW` + `SW_HIDE` +
piped stdout/stderr and a bounded 10 s wait that terminates a hung
child — the og's own hidden pattern (DRWUI_decompiled.c:178726-178733),
one step cleaner (no shell process at all). Same command, same
first-'=' parse, zero windows. Plus the tree-wide no-`_popen`/
no-`system(` build-time guard (`engine/tests/unit/test_no_console_spawn.cpp`)
so the regression class can never re-land silently. The chkdsk
(cr_fsck.cpp:50-51) and PowerShell/manage-bde (cr_bitlocker.cpp:64-65)
spawns were already `CREATE_NO_WINDOW` (verified census,
INSTALLER-AND-PRIVILEGE).
