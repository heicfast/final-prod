# worklog/builds/ — the clean-build evidence (Session-84)

All logs first-hand from this lane's environment (Debian, 2 cores, gcc 14.2.0,
clang 18.1.8, cmake 4.4.4, ninja unavailable → Make backend).

## Linux

| Config | Result | Evidence |
|---|---|---|
| Debug (`linux-gcc` preset, gcc 14.2, `-Wall -Wextra -Wpedantic -Werror -Wshadow`) | clean — 0 errors, 0 warnings; every target built (cr_core, cr_win, cr_ui SHARED, rrecov, cr_tests, cr_ui_tests, cr_ui_abi_check) | the full cr_tests suite + cr_ui_tests suite ran on this build (1,529/0 and 89/89 — worklog entry 7); the build directory was later removed for disk headroom (the fuzz lane needed it), the rebuild is reproducible via `cmake --preset linux-gcc` |
| Release (`linux-release` preset, same flags) | clean — 0 errors, 0 warnings | `linux-release-configure.log` + `linux-release-build.log` (this directory), captured at `fc7201c2`'s tree AFTER the dangling-reference repair |
| Fuzz lane (`fuzz/Makefile`, clang 18.1.8, `-fsanitize=fuzzer,address,undefined -Wall -Wextra -Werror`) | clean — all 16 fuzzer binaries linked | session worklog entry 7 (per-target smoke counts); required the `wr16` dead-helper fix (`56d27d86`... renumbered through the merges to `fc7201c2`'s ancestry) |

Environment notes (honest limits):
- The box has a 4 GiB cgroup memory cap: the full suite must run chunked
  (a single-process full run OOMs mid-suite; every suite passes chunked — the
  CI Windows runner has the headroom the box lacks).
- The sandbox reaps detached processes; long runs must stay within a single
  foreground invocation (the chunked suite runs followed that discipline).

## Windows

| Config | Result | Evidence |
|---|---|---|
| MSVC Release x64 (`win-x64-release`, `/W4 /WX`, vcpkg cache) | see the Checks workflow run on `fc7201c2` | the checks.yml "Build + test (MSVC Release, vcpkg cache)" job — link recorded in the session worklog final entry once concluded |

The Win7 posture gate (stamp_osversion_um.py + verify_pe_um.py with the
import allowlist) runs as checks.yml component [5] in the same job.
