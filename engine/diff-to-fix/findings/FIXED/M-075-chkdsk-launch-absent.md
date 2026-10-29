# MAJOR M-075 — chkdsk launch absent: the og's check→WinChkdsk escalation endpoint has no core counterpart

**Severity:** MAJOR
**Module:** check-phase / win layer / ABI / CLI
**Parameters affected:** S35 (error-recovery ladder — the fix endpoint of the check escalation), P42 (new export), C23 (new error codes if any), Q68 (D-90's disposition text must be rewritten when this lands)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:65` ("| FileSystemCheck.mo | 9,964 | chkdsk-style checks (CCheckFAT, CCheckNTFSVolume, CFatChain, CFileAllocationTable, CHardDriveIO) |"); `re-workspace/artifacts/MOM-modules/FileSystemCheck.md:28` ("Notable string evidence — `WinChkdsk.exe`"); `re-workspace/artifacts/per-binary/WinChkdsk.md:28` ("**chkdsk wrapper.** Launches/monitors chkdsk (legacy mod)"); `re-workspace/artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md` §2 (WinChkdsk.dll — "Runs real chkdsk (`\System32\` + chkdsk invocation), config via `WinChkdsk.ini`, results via `LogMsg.txt`/`LogVss.txt`, Volume Snapshot handling, SectionFlags/GlobalSettings keys"); decompile: `re-workspace/reversed/ghidra-mo/FileSystemCheck_decompiled.c:7099` (`builtin_memcpy(local_f98,"WinChkdsk.exe",0xe)`), `:7134` (`"x64\\WinChkdsk.exe"` WoW64 branch), `:7217` (`CreateProcessA(...)`); guidance string `FileSystemCheck.mo/strings_ascii.txt:126` ("The volume have some errors,please use Chkdsk to check, and use Chkdsk /F to fix!")
**Core references:** audited `35adcb9`. The in-engine checker is present and corpus-exact (M-041 FIXED: `include/cr/check/cr_fscheck.h:79-98` — the 0x1001..0x100e taxonomy, `CHK_CHECK_EXCEPTION 0x100E` "use Chkdsk /F"; `src/core/check/cr_fscheck.cpp`; CLI `fix --check`, `pre_scan_check`). The external launch is absent: `rg -n "fsck|chkdsk"` over `src/`+`include/` hits only the in-engine checker; `cr_ui_fsck_volume` — 0 hits. The current disposition: `docs/PARITY-DELTAS.md:2977` ("The chkdsk delegation stays OUT OF CORE: spawning an external…") + D-90 + `docs/REFERENCE-API-GAP.md:284` (the gap itself is named).
**Status:** FIXED (this session; the S84 fix lane)

## Summary

The og's damage-audit escalation ladder ends in a real chkdsk launch: FileSystemCheck.mo detects damage in-engine, and WinChkdsk.exe (a console wrapper spawning `\System32\` chkdsk with 5 space-separated args, WoW64-aware path) performs the fix. The core has the in-engine detection half (M-041, corpus-exact) but no launch surface at all — the ladder's fix endpoint is unreachable, and the current "out of core" disposition is superseded by the plan-of-record's decision to add it (this finding files the gap so the roadmap owns it).

## What the reference does

The og's FileSystemCheck.mo runs in-engine chkdsk-style checks; on damage it launches `WinChkdsk.exe` (or `x64\WinChkdsk.exe` under WoW64 — `IsWow64Process` at :7093) via `CreateProcessA` with a 5-argument command line (FileSystemCheck_decompiled.c:7099-7217). WinChkdsk.exe itself wraps Windows chkdsk (`\System32\` + chkdsk invocation; config `WinChkdsk.ini`; results `LogMsg.txt`/`LogVss.txt`; VSS "Volume Snapshot" coordination; untfs.dll/ifsutil.dll imports; the legacy source path `mod.winchkdsk\main.cpp`). The user-facing guidance string: "The volume have some errors,please use Chkdsk to check, and use Chkdsk /F to fix!" (strings_ascii.txt:126).

## What our core does

`check::run_fscheck` (M-041) implements the read-only audit with the og's exact 0x1001..0x100e error taxonomy, wired as `pre_scan_check` and the `fix --check` CLI surface. The 0x100E catastrophic class says "use Chkdsk /F" — advice the product itself cannot act on: no `cr_ui_fsck_volume` export exists (81-export census), no CreateProcess chkdsk launcher anywhere in `src/`, no win-layer function. The PARITY-DELTAS row registers the exclusion as deliberate ("spawning an external…"), and REFERENCE-API-GAP.md:284 names the missing half.

## Gap

No code path in core_recovery launches Windows chkdsk on a volume. The og's escalation endpoint (check → RawFixer → WinChkdsk → RAW) is missing its third rung in the core. Falsifiable: a damaged volume that the og would offer to fix via WinChkdsk gets, from the core, only the 0x100E advisory string.

## Impact

Workflow gap users observe at exactly the worst moment (a damaged volume they're trying to recover from): the core detects the damage, prescribes `chkdsk /F`, and cannot perform or even orchestrate it. The in-engine audit quality is unaffected (M-041 verified).

## Reproduction / verification

- Corpus-derived: the four citations above, incl. the decompile's CreateProcessA site (:7217) and the WoW64 path branch (:7134) read in the corpus.
- Core absence first-hand: greps zero; the fscheck header read (taxonomy present, launch absent); PARITY-DELTAS' registration read.

## Fix plan (for the engineering team)

1. New launcher `src/win/cr_fsck_volume_win.cpp`: `CreateProcessW("chkdsk.exe <vol> /f")` elevated, synchronous with timeout; command construction as a testable seam (mirror `detail::build_unlock_command`, cr_bitlocker.cpp:320-339); refuse loudly (not silently skip) when the volume can't be checked. The 5-arg shape the og passes to WinChkdsk.exe is undecoded — building directly on `chkdsk.exe` (what WinChkdsk itself wraps) is the corpus-grounded form.
2. ABI: `cr_ui_fsck_volume(char letter, uint32_t flags, int32_t* exit_code)` + contract text.
3. CLI: `fix --chkdsk C:` + the three-site lock (+ `-h`).
4. **Reconcile D-90/PARITY-DELTAS:2977** — the "chkdsk stays out of core" registration must be rewritten to the new posture (the plan-of-record supersedes it); leaving both is a doc/code contradiction.
5. Tests: command-construction unit (parameter validation, rc mapping via the seam); a Windows scenario row (chkdsk on the staged volume exits 0); CI-SCENARIOS.md entry.

## Tests to add

- `FsckVolume.CommandConstruction` (letter + flags → the exact command line).
- `FsckVolume.RefusesInvalidVolume` (no silent skip).
- `FsckVolume.ExitCodeMapped` (chkdsk exit → the ABI's int32_t).
- Windows-lane scenario: staged volume → chkdsk → exit 0 → re-check reports clean.

## Related findings

- M-041 (FIXED — the in-engine checker this escalates from).
- S-038 (FIXED — the installer posture; unrelated but same "external binary" coordination class).
- D-90 (the registration this finding supersedes — its rewrite is fix-plan step 4).

## Open questions

The og's exact 5 arguments to WinChkdsk.exe are not decoded (the corpus names the count, not the values). If a future decode recovers them, reconcile against the direct-`chkdsk.exe` form chosen in step 1.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S84 fix lane, 2026-10-30)
**Verified by:** `FsckVolume.CommandConstruction` / `FsckVolume.RefusesInvalidVolume` / `FsckVolume.ExitCodeClass` (test_fscheck_launch.cpp — the pure grammar battery, both lanes, green first-hand on the Linux build) + the `FsckWin.*` six-test seam battery (test_fsck_win.cpp, the Windows lane — the wiring, the scan-only posture, the absent-volume loud refusal with NOTHING spawned, the invalid-letter refusal before any probe, the spawn-failure mapping, the raw verdict passthrough) + `nm -D libcr_ui.so` shows the `cr_ui_fsck_volume` export live + the doc_truth_gate green (exports=92, all five count sites refreshed — the A-ABI-3 class swept in the same change). The end-to-end scenario (staged FAT32 with a seeded 0x1005 → `fix --chkdsk C:` → exit code 1 → re-audit clean) is the CI-SCENARIOS row for the Windows runner.
**Resolution notes:** CONFIRMED-DIVERGENCE, re-verified at corpus be0cfe9 first (the four citations re-read: FileSystemCheck_decompiled.c:7099/:7134/:7217 — the WinChkdsk.exe memcpy, the x64 WoW64 branch behind IsWow64Process, the CreateProcessA site; per-binary/WinChkdsk.md:28 — the wrapper "Runs real chkdsk (\System32\ + chkdsk invocation)"; strings_ascii.txt:126 — the two-posture guidance string). The landing follows the finding's own fix plan: (1) the launcher with the command construction as a testable seam (the N-064 build_unlock_command precedent — the pure `build_chkdsk_command` in cr/check/cr_fsck_launch.* is cross-platform, the win layer owns only the spawn); (2) the ABI `cr_ui_fsck_volume(char letter, uint32_t flags, int32_t* exit_code)` exactly as sketched (raw exit code out; OK when chkdsk ran); (3) the CLI `fix --chkdsk C:` with the three-site lock (the header usage comment, the -h usage, the parser — no --disk needed: a volume-letter operation placed before the F-B-4 disk guard and outside the driver precondition, matching the og's wrapper which runs beside the engine, not under it); (4) D-90/PARITY-DELTAS rewritten to the superseded posture in the same change (the doc/code contradiction the finding called out is gone — REFERENCE-API-GAP R-65 now PRESENT, CAPABILITY-MATRIX row DONE (M-041 + M-075)); (5) the WoW64 guarantee realized as the Wow64DisableWow64FsRedirection around the CreateProcessW (the og's own branch's native-binary guarantee for the direct form). Registered engineering choices (not corpus-decoded, documented as such in the headers): the 15-minute synchronous bound (the og's monitoring loop has no decoded bound; on timeout the process is LEFT RUNNING — killing chkdsk mid-fix is never safe — and the caller sees STILL_ACTIVE 259), and the timeout-on-timeout exit posture. The og's exact 5 arguments to WinChkdsk.exe remain undecoded (the finding's open question stands) — the direct `chkdsk.exe <vol> /f` form is the corpus-grounded base the wrapper itself wraps; a future decode reconciles against it. No safety hardening touched (the loud refusal IS the hardening: absent volume → ERR_OPEN_FAILED with nothing spawned — pinned by FsckWin.RefusesAbsentVolumeLoudly's empty-recorder assertion).
