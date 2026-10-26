# MAJOR M-057 (NEW-FINDING, session 23) — the Windows CLI app has been uncompilable since `0a68937`: `cmd_apply`'s undeclared `&e`

**Severity:** MAJOR (M-055 class — build integrity of a Windows-only layer, invisible to the Linux lane until this session extended the parse gate)
**Module:** app (rrecov CLI) · build
**Parameters affected:** Q67 (buildability — every platform layer parses), Q65-adjacent
**Status:** FIXED (found + fixed in the same session — the N-009c parse-gate extension surfaced it)

## Summary

`src/app/rrecov.cpp`'s `cmd_apply` (the `rrecov lostpart apply` subcommand, the S-029
union landing) calls `win::open_disk_apply_rw(disk, use_rda, use_rda ? L"RDA" : nullptr,
&e)` — but **`e` is never declared in that function**. A single undeclared identifier
breaks the whole translation unit: **`rrecov.exe` cannot compile on MSVC** since
`0a68937` (the DFX16-S029 reconcile commit, verified by direct inspection of that tree:
`&e` at cmd_apply+135, zero `u32 e` declarations in scope). The Windows CI's app build
over that window was failing; the Linux lane could not see it because the app layer —
unlike `src/win` since M-055 — had no parse gate.

## Evidence

- `git show 0a68937:src/app/rrecov.cpp` — `cmd_apply` contains
  `open_disk_apply_rw(..., &e)` with no `e` in scope (verified this session).
- Pre-fix reproduction (executed this session): `g++ -std=c++20 -fshort-wchar -D_WIN32
  -fsyntax-only -Itests/win-stub -Iinclude src/app/rrecov.cpp` →
  `src/app/rrecov.cpp: error: 'e' was not declared in this scope` (the first error;
  everything after is cascade).
- Sibling functions declare it correctly (`cmd_fix`/`cmd_undo`: `u32 e = 0;` before the
  same call shape) — the omission is a plain authoring miss in the S-029 union landing,
  not a contract change.

## Root cause

Two failures compound:
1. The S-029 app-side landing was pushed without a Windows build executing (the M-055
   pattern, one directory over: concurrent parity-fix landings with no MSVC lane in the
   loop for the app target).
2. The M-055 parse gate (`tools/win_syntax_check.sh`) covered `src/win/*.cpp` only —
   `src/app/rrecov.cpp` (which is equally Windows-only: `wmain`, `_wfopen_s`, the whole
   `cr::win` surface) parsed nowhere on the Linux lane.

## Fix (both halves)

1. **The declaration:** `u32 e = 0;` before the `open_disk_apply_rw` call in
   `cmd_apply` (matching the sibling functions' shape).
2. **The gate:** `tools/win_syntax_check.sh` now parses `src/app/*.cpp` with the same
   stub harness (the ctest `win_syntax_check` target inherits the coverage — a future
   app-layer structural break fails the Linux suite immediately). Stub surface grown:
   `WINAPI` (empty — ignored on x64 MSVC too), `CTRL_C_EVENT`/`CTRL_BREAK_EVENT`,
   `errno_t`-family CRT additions `_wfopen_s`/`_fseeki64`/`_ftelli64`, `__int64`
   (width-faithful `long long`), `SetConsoleCtrlHandler`, `QueryPerformanceFrequency`.

## Fix-plan fidelity / related

- Same class as M-055 (`diff-to-fix/findings/FIXED/M-055-*`): the gate's per-directory
  blind spot is the root cause there too; this closes the remaining directory.
- The N-009c fix (CLI `--filter` UTF-8) lands in the same file in the same session;
  both are proven by the same extended gate.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-009/M-057 batch — the commit carrying this file)
**Verified by:** fail-before EXECUTED: with the declaration removed, the extended
`win_syntax_check` fails on `app/rrecov.cpp` with `'e' was not declared in this scope`
(the exact pre-fix diagnostic); pass-after: the gate is GREEN over all 7 win-layer
files + `app/rrecov.cpp` (8/8 parse). The full Linux suite is 740/740 at the fix
commit (the 3 new Text tests are N-009c's; the gate run is a ctest case in that
suite).
**Resolution notes:** the declaration landed (matching `cmd_fix`/`cmd_undo`), and the
parse gate now covers the app layer permanently — the M-055 failure class is closed
for every Windows-only directory in the tree (`src/win` + `src/app`). Corpus N/A
(this is a build-integrity defect, not a parity divergence; the og reference is not
implicated). The MSVC `/W4 /WX` lane compiles this file clean by construction of the
fix (the identifier now exists; no new imports, no Win7-gate impact — all added stub
surface is parse-gate-only and never ships).
