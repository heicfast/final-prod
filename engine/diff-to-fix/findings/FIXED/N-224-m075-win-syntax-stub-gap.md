# MINOR N-224 — the M-075 landing's win-layer file broke win_syntax_check: cr_fsck.cpp's WoW64-redirection pair and ULONGLONG were never grown into the win-stub (the gate's "a new SDK symbol grows the stub" contract skipped by the landing)

**Severity:** MINOR (a red CI gate, not behavior: the landing's own message omitted any win_syntax_check claim — an honest omission, but the gate was red at origin/main and every push after it inherited the red)
**Module:** win layer (the chkdsk launcher's stub surface) / the win_syntax_check gate
**Parameters affected:** Q68 (the gate's truth), the stub-contract surfaces (the N-116 "a new SDK symbol grows the stub" convention)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:65` + the WinChkdsk decompile chain (M-075's own citations — the WoW64-aware path the launcher implements: `IsWow64Process` at FileSystemCheck_decompiled.c:7093, the `x64\WinChkdsk.exe` branch :7134)
**Core references:** origin/main `db468e29` — `src/win/cr_fsck.cpp:40` (`Wow64DisableWow64FsRedirection(&redir_old)`), `:47`/`:54` (`Wow64RevertWow64FsRedirection`), `:58-59` (`ULONGLONG start = GetTickCount64()`); the win-stub `tests/win-stub/windows.h` had none of the three; the win_syntax_check ctest FAILED first-hand at origin/main ("cr_fsck.cpp does not parse" — the six errors)
**Status:** FIXED (this session, filed+fixed — the discovered-divergence SOP)

## Summary

The M-075 landing (d10ae333) added the chkdsk launcher with the og's WoW64-aware native-system32 path — but the three SDK symbols the new file uses (`Wow64DisableWow64FsRedirection`, `Wow64RevertWow64FsRedirection`, `ULONGLONG`) were never added to the win-stub headers the win_syntax_check gate parses against. The gate went red at origin/main and every subsequent push inherited the red. The landing's own message omitted the win-gate claim (its verification list named the Linux suite, the ASan lane, doc_truth_gate, and clang-format) — an honest omission that left the gap invisible until this lane's full-ctest run caught it.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's S88-lane repair commit (SHA in the commit footer)
**Verified by:** win_syntax_check RED first-hand at origin/main ("cr_fsck.cpp does not parse" — `Wow64DisableWow64FsRedirection`/`Wow64RevertWow64FsRedirection` "not declared", `ULONGLONG` "does not name a type", six errors) → GREEN post-fix (`win_syntax_check 100% tests passed`, cr_fsck.cpp parses); the full suite 1676/1676
**Resolution notes:** The stub grown per its own contract (the N-116 convention the N-178 landing used): `ULONGLONG` typedef'd beside ULONG_PTR; the WoW64 pair as BOOL-returning no-op inlines with the native signatures' shapes (`void**`/`void*` — the stub's PVOID stand-in). The launcher's own logic is untouched — the symbols' presence on the stub lane is all the gate checks (the real declarations come from the Windows SDK on the MSVC lane, where the pair is a documented Win32 API). Files: tests/win-stub/windows.h.


## Convergence addendum (recorded at the rebase, 2026-10-30)

A racing lane (commit `9f5f0415`) landed the IDENTICAL stub growth in the
same window — the same `ULONGLONG` typedef beside ULONG_PTR and the same
BOOL no-op WoW64 pair — as part of its N-223 trail-repair rider. The two
repairs are byte-identical (verified at the rebase: `git diff` on
tests/win-stub/windows.h is empty); the racing landing keeps the repair
seat per SA-9, and this finding's file remains as the convergent twin's
record — two lanes, one gap, one fix. The Verified-by line's evidence
(the RED at origin/main, six parse errors) stands as this lane's
independent reproduction.
