# MINOR N-224 — the M-075 landing grew src/win/cr_fsck.cpp without growing the win-stub SDK surface: win_syntax_check RED on main (Wow64Disable/RevertWow64FsRedirection + ULONGLONG undeclared in the declaration-only stub)

**Severity:** MINOR (a Linux-lane gate red, not a code defect — the MSVC lane parses the real SDK fine; but per the never-push-a-red-commit rule a red gate on main is a board defect of the M-120/N-210 "pushed without running the local gate" class)
**Module:** CI / the M-055 win-layer parse gate (tools/win_syntax_check.sh + tests/win-stub/windows.h)
**Parameters affected:** Q68 (audit-trail/CI truth — the gate that is supposed to catch win-layer structural breaks was itself red), the CI-green deliverable of the fix sessions
**Corpus references:** none (a core-repo gate integrity defect)
**Core references:** found at core `ddab68b4` (this lane's open pin; first-hand): `ctest` on the linux-release build → `1679 - win_syntax_check ... Failed`; the direct parse `g++ -fsyntax-only -Itests/win-stub -Iinclude src/win/cr_fsck.cpp` → six errors, all `Wow64DisableWow64FsRedirection`/`Wow64RevertWow64FsRedirection`/`ULONGLONG` undeclared. The offending commit is the M-075 landing `d10ae333` (src/win/cr_fsck.cpp introduced with the WoW64-native spawn at :36-58). The gate's own header contract: `tests/win-stub/windows.h:14-16` — "When src/win starts consuming a new SDK symbol, the gate fails here until the stub grows - that failure is the gate working."
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The M-075 chkdsk-escalation landing (`d10ae333`) added `src/win/cr_fsck.cpp` whose `live_spawn` uses `Wow64DisableWow64FsRedirection`/`Wow64RevertWow64FsRedirection` (the og's WoW64-native guarantee) and `ULONGLONG` (the GetTickCount64 timeout bound). The M-055 Linux parse gate re-declares the exact SDK surface src/win consumes in `tests/win-stub/windows.h`; the landing did not grow the stub, so `win_syntax_check` — a ctest test in the default Linux suite — went RED on main at the landing push and stayed red through `ddab68b4` (six intervening pushes, all windows-side or trail-side, none re-running the full Linux ctest). The defect class is the N-210/M-120 recurrence: a lane pushing without running the local gate (`ci-local.sh` or at minimum the full ctest suite).

## Gap

Falsifiable: `ctest` on any linux-release build of `ddab68b4` fails test 1679 with the six errors quoted above. The M-075 commit message itself records "linux-release rebuild clean (0 warnings)" for the N-125 rider that preceded it in the same session — the fsck landing's own push skipped the suite that would have caught it.

## Fix

The stub grows the three symbols, SDK-faithful (`tests/win-stub/windows.h`):
- `typedef unsigned long long ULONGLONG;` (beside ULONG_PTR, the SDK's own width on x64)
- `inline BOOL Wow64DisableWow64FsRedirection(void**)` / `inline BOOL Wow64RevertWow64FsRedirection(void*)` — the SDK shapes (PVOID = void*), declaration-only bodies per the stub's charter.

No src/ change: the MSVC-side code is correct (it parses against the real SDK); the defect was the missing stub growth, exactly the failure mode the gate's header documents as "the gate working."

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S87 fix lane, 2026-10-30)
**Verified by:** `bash tools/win_syntax_check.sh` → 13/13 ok (cr_bitlocker, cr_bootfix, cr_destio, cr_elevate, cr_enumerate, **cr_fsck** ok, cr_health, cr_image_create, cr_image_win, cr_mountpoints, cr_transport, app/rrecov, app/cr_ui); full ctest suite green post-fix (the only other open failure, QueryPerf.MillionNodesFilterCountUnder200ms, is the known flaky-under-parallel-load perf pin — passes serially, 0.53 s, unchanged by this stub-only edit).
**Resolution notes:** RED first-hand at `ddab68b4` (ctest 1679 failed, six undeclared-symbol errors), GREEN with the three stub additions. The stub-body semantics are inert by design (declaration-only, never linked); the shapes match the SDK so future parses see the real arity. The recurrence class (a landing pushing without the local gate) is unchanged since N-210's own resolution note — the structural fix remains procedural: lanes must run `ci-local.sh` (or the full ctest) before every push; this lane records the sixth occurrence of the class in the finding trail so the pattern is visible at the next audit.
