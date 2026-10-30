# ci-failures — Session-84 captured logs (extended Session-95)

- `checks-281-lint-job.txt` — checks.yml run 281 (head 8fe3542f, 2026-10-29T17:14Z),
  job "Lint + parse gates" FAIL: 10 clang-format violations across
  src/ui/cr_ui.cpp:2427-2429/:3035-3036, src/win/cr_health.cpp:267-270,
  tests/win-stub/windows.h:308. RESOLVED by 9b921ef8 (format-only; the racing
  8fe3542f sweep had covered 14 other files but missed these three).
- `test-benchmark-6.txt` — test-benchmark.yml run 6 (head 468ce7a8), job
  "test-and-benchmark" FAIL, two root causes:
  1. Run-Benchmark.ps1:152 — `$profile` collides case-insensitively with the
     `[ValidateSet('smoke','full')] $Profile` param; the CRBIG volume's
     `follow-run-profile` sentinel assignment threw MetadataError. RESOLVED by
     8daf75dc (rename to `$seedProfile` at both sites).
  2. Run-Benchmark.ps1 carve-parity probe — `list-carve-formats` emits 240
     lines (count + one per format); array-LHS `-match` filters and never
     populates `$Matches` → `Cannot index into a null array` →
     "carve-registry parity failed: CLI= API=239" hard-gate failure.
     RESOLVED by 8daf75dc (the `-join "`n"` scalar fix — note: this fix rode
     the same commit as the profile fix through a staging misfire; the
     message describes only the profile half — recorded here and in worklog.md).
  A third latent cause is fixed separately: the image-create probe invokes
  `rrecov image --create`, a flag cmd_image never parsed (see the Session-84
  worklog and the image-create wiring commit).

## Session-95 captures — the 2026-10-30 Win7-posture-gate streak (8 red runs, 40965919891 → 40981611198)

Root cause class: the post-a972d29f remediation wave (the S-060/M-074/M-075/M-118/N-178 landings) introduced new Win32 consumers without extending the import baseline — plus one incomplete gate fix. Build (/W4 /WX) and ctest (1744/1744) were GREEN throughout; only step 7 "Win7 posture gate (stamp + import allowlist)" failed, on rrecov.exe (the FIRST PE — the loop exits before cr_tests.exe/cr_ui.dll are ever checked).

- `checks-40977711470-build-and-test.txt` — head c57986d6: the full 12-import failure set (4 advapi32: RegOpenKeyA/RegOpenKeyExA/RegEnumKeyA/RegQueryValueExA; kernel32: GetConsoleMode/ReadConsoleInputW/SetFileAttributesW/SetFileTime/GetVolumePathNameW/Wow64Disable+RevertWow64FsRedirection; DLL: SHLWAPI.dll). Partially RESOLVED by 2619e2e4 (the 4 advapi32 + console pair — 7 of 12).
- `checks-40979687718-build-and-test.txt` — head 2619e2e4: the residual 6 (SHLWAPI.dll unexpected + the 5 kernel32). NOT resolved by 3b203fe1: that commit added shlwapi.dll to ref_imports_um.json but not to verify_pe_um.py's ALLOWED_DLLS, so the DLL-name gate still fired first. Its commit message also mislocated the failure ("the third PE") — the log shows rrecov.exe.
- `checks-40981611198-build-and-test.txt` — head 3b203fe1: identical 6 failures (the half-fix proven inert). RESOLVED by this session's gate fix (see worklog.md Session-95): ALLOWED_DLLS gains "shlwapi.dll": "strict" + the 5 kernel32 entries land in ref_imports_um.json (all source-censused, all Win7-SP1-available: SetFileAttributesW/SetFileTime = cr_destio.cpp:154,177 + test_writer_dirs.cpp:189,203; GetVolumePathNameW = cr_transport.cpp:1076; the Wow64 pair = cr_fsck.cpp:40,47,54) + tools/test_verify_pe_um.py regression cases 6-7 pin the new surface.

## Session-95 captures — the layer behind the layer (runs 41024085633, 41026660604)

The cc6312ca run walked past the posture gate for the first time in 9 attempts (ctest, G-09 and the refusal probes all green) and tripped the NEXT masked defect:

- `checks-41024085633-build-and-test.txt` — head cc6312ca: step 10 "CLI junk-filter contract (M-071)" FAIL — but not at runtime: `ParserError` at line 15, `Variable reference is not valid. ':' was not followed by a valid variable name character`. Six `$LASTEXITCODE:` sites inside double-quoted strings (scope syntax = parse error). The step landed at 53eb9fff (2026-10-28 17:24), 2 hours AFTER the last green run a972d29f (15:19), and every subsequent run failed earlier at the posture gate — the step has NEVER parsed. RESOLVED by 7744acfe (`${LASTEXITCODE}:` at all six sites + the inline-run-block AST gate extension so the class can never ship silently again: the S-036 gate only parsed .ps1 files, never the inline YAML run blocks).
- run 41026660604 (head 7744acfe) — step 8 "doc-truth gate" FAIL: the census-honesty note added to CORE-API-CURRENT.md said "(78 exports then)" on a line with no historical-exemption marker — the A-ABI-3 count regex correctly refused a non-93 export claim. RESOLVED by d259febb (the note reworded to "a historical count (78 exports at that audit-time; the tree now derives 93)" — the gate's own per-line exemption convention, doc_truth_gate.py:360-362).
- run 41027072251 (head 4dd2234e, the workflow_dispatch-trigger addition) — the full stack re-proof: the first-ever EXECUTION of the M-071 step.
