# MINOR N-128 — session-list CWD dir composes "./name" row paths: a caller's own CWD-relative save path never string-matches its row

**Severity:** MINOR
**Module:** session / ABI
**Parameters affected:** B10 (filename/path handling — the row path's canonical form), Q68 (the S7f contract's path-composition rule)
**Corpus references:** `DRWUI_decompiled.c:432259-432359` (the og's FindFirstFileW SaveScan enumeration — the og always composes its own fixed module-adjacent folder, never a caller-supplied CWD dir; the bare-relative case is core-side contract surface)
**Core references:** discovered this session while proving the M-102/M-083 ABI landings: `tests/ui_abi_check.c` invoked with NO argv (its default `cr_ui_abi_check.img`, CWD-relative) fails at :566 (`sl_found`) — the save path is `cr_ui_abi_check.img.crsf` while the "."-dir listing returns `./cr_ui_abi_check.img.crsf` (`src/ui/cr_ui.cpp` POSIX branch: `dir + sep + n`; the Windows branch composes `dir_w + sep + name` the same way). CI never sees the class: `tests/CMakeLists.txt:158` invokes the gate with `${CMAKE_CURRENT_BINARY_DIR}/cr_ui_abi_check.img` (an absolute path). Verified at the session-9 baseline `84a1832` via a clean worktree (the defect predates this session; not a regression of the W1a/N-116 landings).
**Status:** discovered during verification (session-10, this lane) — filed per the §8 duty

## Summary

A caller who saves `name.crsf` (CWD-relative) and then lists `.` gets rows whose paths read `./name.crsf` — the same FILE, but a string that never `strcmp`-matches the caller's own save path. The row contract (S7f: "byte-sorted caller-owned POD rows... path") intends the caller-comparable form; the CWD dir is the one composition where the join decorates instead of denotes.

## Gap

Falsifiable: save `"a.crsf"`, list `"."` → row path `"./a.crsf"` ≠ `"a.crsf"`. Any host keying sessions by their own save-path string (the C gate's exact pattern) misses its own row.

## Impact

Host-facing string comparability only; no listing-scope or persistence effect. The absolute-path callers (CI, the C++ suite) never observe it.

## Reproduction / verification

Executed: `cr_ui_abi_check` with the default argv at `84a1832` (clean worktree) → FAIL sl_found; with an absolute argv → OK. Root cause read in both listing branches.

## Fix plan

1. Both `list_crsf_files` branches: the dir exactly `"."` composes BARE names (`name`, never `./name`) — the canonical CWD-relative form; every other dir keeps the current join (the caller's separator convention preserved).
2. Pin it in the C gate (both a `.`-listing assertion and the default-path invocation) and the C++ suite.
3. Keep the ctest invocation as-is (absolute path — the other class stays covered).

## Tests to add

- The C gate's `.`-listing block (bare names + the default-path save's own row).
- `CrUi.SessionListCwdDirListsBarePaths` (C++).

## Related findings

- W1a (`eb16335` — the listing surface), N-116 (`9445627` — the time ordering), the M-102 landing (this session's context).
