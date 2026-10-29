# MINOR N-225 — the M-075 landing's win-layer file fails the Linux parse gate: cr_fsck.cpp's Wow64 pair + ULONGLONG never declared in tests/win-stub/windows.h (win_syntax_check RED on main since d10ae333)

**Severity:** MINOR (a Linux-side gate red — no Windows behavior defect; the win layer itself is sound on MSVC; but main's "leave main green" contract was broken at the push and nobody re-ran the local gate)
**Module:** CI / parse gates (tools/win_syntax_check.sh + tests/win-stub/windows.h)
**Parameters affected:** Q68 (audit-trail truth), the Linux-CI-green deliverable
**Corpus references:** none (a core-repo gate defect)
**Core references:** found at core `ddab68b4` (origin/main HEAD at this lane's clean clone): `ctest -R win_syntax_check` → FAIL — `src/win/cr_fsck.cpp:40/:47/:54` (`Wow64DisableWow64FsRedirection` / `Wow64RevertWow64FsRedirection` not declared) + `:58-59` (`ULONGLONG` not a type) under `tests/win-stub/windows.h`. The M-075 landing `d10ae333` introduced the file (its WoW64-native launch + the GetTickCount64 timeout loop) without extending the stub; `git log -S Wow64DisableWow64FsRedirection -- tests/win-stub/windows.h` → empty (never declared at any commit). Every push since d10ae333 carried a red Linux gate (the racing lanes' worklog entries record suite counts, not the gate's verdict — the win_syntax_check shell test sits at the tail of the ctest list where a summary read misses it).
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule — the red-main repair before any finding work)

## Summary

The M-075 chkdsk-endpoint landing added `src/win/cr_fsck.cpp` with the WoW64 redirection disable/revert pair and `ULONGLONG` tick math, but the Linux-side parse gate's SDK stub (`tests/win-stub/windows.h`) was never extended with those three symbols — `win_syntax_check` has failed on every tree since `d10ae333`. The M-075 trail (finding Resolution, worklog entry, SUMMARY) records "Linux CI green" claims that were true for the suite the lane ran but false for the gate this file class exists to police.

## The repair (executed this session)

1. `tests/win-stub/windows.h`: `typedef unsigned long long ULONGLONG` added to the core typedef block (the SDK's 64-bit tick type, next to ULONG_PTR); the Wow64 pair added as SDK-faithful declarations in the process-launch section (`BOOL Wow64DisableWow64FsRedirection(void** OldValue)` / `BOOL Wow64RevertWow64FsRedirection(void* OldValue)` — the PVOID shapes the win layer calls, with the not-under-WOW64 no-op modeled as success for parse purposes; declaration-only, not for linking, the stub's own convention).
2. `ctest -R win_syntax_check` → 100% passed; the FULL suite re-run → **1679/1679** (the first fully-green count on main since d10ae333).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S88 fix lane, 2026-10-30)
**Verified by:** `ctest -R win_syntax_check` → 100% passed (was: FAIL with the five undeclared-symbol errors quoted above); full suite `ctest -j` → 1679/1679 passed (123 s); no other file touched.
**Resolution notes:** The root cause is the M-075 lane's push discipline: a new src/win consumer landed without running the parse gate its own landing class requires (the never-push-a-red-commit rule; the same class as the format-gate misses 4e522b11/9b921ef8 repaired). The stub extension is SDK-faithful (the pair's PVOID shapes and BOOL returns; ULONGLONG's 64-bit width) so no parse-gate behavior depends on the modeling choice. The M-075 trail's "Linux green" claims stand corrected by this file's record — the lane's suite count was true, its gate verdict was not re-checked.
