# MINOR N-225 — the convergent win-stub repair collision: three racing lanes each landed the M-075 stub growth, origin/main carried THREE definitions of the Wow64 pair + ULONGLONG, and win_syntax_check was RED on every src/win file (redefinition errors)

**Severity:** MINOR (a Linux-lane gate red on main — the N-210 push-without-gate class's seventh occurrence, this time committed BY the repair lanes themselves)
**Module:** CI / the M-055 win-layer parse gate (tools/win_syntax_check.sh + tests/win-stub/windows.h)
**Parameters affected:** Q68 (audit-trail/CI truth — the gate that catches win-layer structural breaks was itself structurally broken by its own repairs)
**Corpus references:** none (a core-repo gate integrity defect)
**Core references:** found at this lane's merge of origin/main `322dbdc0` over the local `74f29341` (2026-10-30; first-hand): `git show origin/main:tests/win-stub/windows.h | grep -n Wow64` → definitions at :302/:435/:642 (THREE inline pairs) + ULONGLONG typedefs at :28/:31/:641 (three). `g++ -fsyntax-only` over ANY src/win file → `redefinition of 'BOOL Wow64DisableWow64FsRedirection(void**)'` (:434 vs :301) — every one of the 13 gate files FAILed, i.e. win_syntax_check was 0/13 RED on origin/main. The three landing commits: `9f5f0415` (N-223-m075-win-stub-gap, the bottom block), `a2a93573` (N-224-m075-win-syntax-stub-gap, the :435 block + :28 typedef), `90b787d6` (the :301 block + :31 typedef) — three lanes converging on the same defect in the same window, each pushing without re-running the gate against the others' landings.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The M-075 cr_fsck.cpp stub gap (the racing lanes' N-223/N-224, and this lane's born-collided local N-224 — conceded per SA-9, earlier-push-wins) was repaired THREE TIMES in one window: three lanes each added the Wow64 pair + ULONGLONG typedef to `tests/win-stub/windows.h` at three different positions, none of them re-running `tools/win_syntax_check.sh` after the previous lane's landing had already merged. Identical typedefs are legal C++; identical inline FUNCTION definitions are not — `redefinition of 'BOOL Wow64DisableWow64FsRedirection(void**)'` fails every translation unit that includes the stub. The gate that exists to catch exactly this class of pushed-red went red on all 13 files: the repair lanes each fixed the defect and collectively re-created it.

## Gap

Falsifiable: at `322dbdc0`, `bash tools/win_syntax_check.sh` → 13 × `FAIL: <file> does not parse`, every error a redefinition at tests/win-stub/windows.h:434 vs :301 (first-hand, this lane's merge). `ctest -R win_syntax_check` on a fresh build of `322dbdc0` → red.

## Fix

The merge resolution dedupes to ONE definition set (the exact surface the M-055 charter prescribes): the `ULONGLONG` typedef beside `ULONG_PTR` (the :31 form, comment kept), the Wow64 pair as the single consolidated inline block after `GetTickCount64` (the :301 form — SDK shapes `void**`/`void*`, declaration-only bodies), and the two racing duplicate blocks removed. The `GetExitCodeProcess`/`WaitForSingleObject` neighbors the middle block carried are preserved (they were unique to that block's context).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S87 fix lane's merge over `322dbdc0`, 2026-10-30)
**Verified by:** `bash tools/win_syntax_check.sh` → 13/13 ok (zero FAIL lines); `grep -c 'Wow64DisableWow64FsRedirection(void\*\*)' tests/win-stub/windows.h` → 1; the merge commit's tree carries exactly one definition set. Full ctest re-run post-merge.
**Resolution notes:** RED first-hand on the merged origin/main tree (three definition sites), GREEN after the dedupe. The convergent-lane collision mechanism — N lanes repairing one defect in one window without re-running the shared gate between landings — is recorded here as the class's own entry so the next audit sees that the N-210 recurrence count reached the repair lanes themselves. The procedural fix is unchanged: the gate must run against the PULLED head immediately before every push, not against the pre-pull working tree. This lane's independently-derived stub growth (the born-collided local N-224, commit `74f29341`) is conceded to the racing filings per SA-9; the N-225 seat is the collision defect the racing window created and did not see.

## Session-89 disposition (2026-10-30, the S89 MINOR-first fix lane — the fixed-directory move + the re-verification at the lane head)

The finding's Resolution was FIXED at the S87 merge but the file remained in
`findings/MINOR/` — the same class of bookkeeping gap the S88 lane's N-223 move
closed. Moved to `findings/FIXED/` this session per the user's fixed-directory
directive. Re-verified at the lane head `7953c5e8` (first-hand): exactly ONE
`inline BOOL Wow64DisableWow64FsRedirection` definition (`tests/win-stub/windows.h:301`),
ONE `Wow64RevertWow64FsRedirection`, ONE `ULONGLONG` typedef (`:28`), and
`bash tools/win_syntax_check.sh` → 13/13 ok, zero FAIL lines; the full suite
1685/1685 green at the same tree (the S88-close suite grew 1683→1685 with the
racing window's landings). No code change owed — the move is the fix.
