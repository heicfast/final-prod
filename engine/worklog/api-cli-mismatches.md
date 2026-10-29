# worklog/api-cli-mismatches.md — the ABI↔CLI↔docs cross-check register (Session-84, T3)

As of `78c60e89` (2026-10-30). Method: the Phase-0 sheets (build/api/cli/doc-inventory,
landed at `dae6181a`) provide the surfaces; every load-bearing claim below was re-verified
first-hand at HEAD (the verification noted per row). Dispositions: **FIXED** (this session,
SHA cited) / **DOC** (Phase-5 doc reconciliation, T17) / **BEHAVIOR** (filed in
`worklog/api-findings.md` for the next behavior session — not fixed here, per the
no-behavior-change mandate).

## 0. The architecture truth (read this first)

The CLI does **not** link the C ABI. `rrecov` links `cr::core` + `cr::win` directly
(src/app/CMakeLists.txt:1-2), with the documented rationale at rrecov.cpp:859-867: linking
`cr_ui.dll` would give the process two copies of the engine (the static core in the exe +
the one inside the DLL), and a `--user-signs` install would land in the copy the exe's scan
never reads. The `cr_ui` ABI is the HOST surface, pinned by its own tests
(`cr_ui_tests`, `tests/ui_abi_check.c`, `.github/scripts/api_harness.c`).

Consequently "the CLI is wired to the ABI" is realized as two edges that must each hold:
(a) CLI subcommand → core C++ function (existence, args, error handling, exit codes), and
(b) `cr_ui` export → the same core surface (the ABI as a parallel projection). Both edges
were cross-checked. The sheets' per-subcommand call tables (cli-surface §2, api-surface §2)
carry the full evidence; this register lists only the DEFECTS and their dispositions.

## 1. Fixed in this session

| # | Defect | Evidence (both sides) | Fix |
|---|--------|----------------------|-----|
| F-1 | Phantom export: `cr_ui_session_resume_last` declared (cr_ui.h:927, contract :917-926) with zero implementation — 91 declared vs 90 defined; MSVC DLL build would LNK2001; Linux/GCC silently emitted nothing so local gates stayed green | cr_ui.h:927 vs `nm -D` empty pre-fix; api-surface sheet F1 HIGH | `a47419c7` — impl + 6-arm pin test (this lane); independently re-verified by the racing window's 4756bb9f merge ("cr_ui_tests 89/89, the new export test included") |
| F-2 | `image --create` documented at 5 sites but never parsed — `unknown image option: --create`, exit 1 | rrecov.cpp:2874 pre-fix vs usage :3858, docs/DRIVER-REQUIRED.md:38, CI-SCENARIOS.md:30, Run-Benchmark.ps1:184 | `32441353` (racing lane) — parser + `CrUi.CliImageCreateFlagParses` pin |
| F-3 | Exit code 5 (N-125 device disconnect, rrecov.cpp:2084-2095) undocumented in the verb's own docstring and API.md's exit table | rrecov.cpp:49-60 and docs/API.md:58-66 pre-fix vs :2080-2095 | `78c60e89` (this lane) — docstring scan clause + API.md `\| 5 \|` row |
| F-4 | FIXED-LEDGER M-117 collisions: N-219 rows citing 3 slugs (2 nonexistent), N-221 citing 2 (1 nonexistent) — doc_truth_gate 2×FAIL | FIXED-LEDGER.md rows 676/684/685 pre-fix; gate log in worklog entry 4 | `24edbb04` (this lane) — three stale rows retired, one slug per ID |
| F-5 | Lint-job format drift (3 files) + bench `$profile` ValidateSet collision + carve-parity `-join` bug | checks run 281/282 logs; test-benchmark-6 log (all in worklog/ci-failures/) | `9b921ef8`, `8daf75dc` (racing lane) |
| F-6 | Committed conflict markers in 6 tracked files at session open (the C-1/N-210 class) | opening sweep, worklog Session-84 entry 1/2 | swept across `15c59ee7`, `50a59f51`, `e2b850d4`, `4756bb9f`; at `78c60e89` only M-120's sanctioned evidence quotes remain (3 lines, verified) |

## 2. Open — DOC disposition (T17 batch)

| # | Claim | Reality | Sites |
|---|-------|---------|-------|
| D-1 | docs/PARAMETERS.md:18 lists `--part P` under `scan` | `--part` is a `fix`-verb flag only; scan rejects it (rrecov.cpp:1613-1614 vs :3536-3539) — verified | PARAMETERS.md:18 |
| D-2 | docs/PARAMETERS.md:50-51 lists bench `[--no-rda]` | removed by PLAN-DRIVER-ONLY R10; bench rejects it with an explicit code comment (rrecov.cpp:2930-2933, :2979-2980) — verified | PARAMETERS.md:50-51 |
| D-3 | `cr_ui_session_last_autosave_path` header contract documents `ERR_NOT_FOUND` | the impl returns `ERR_RANGE_OUT_OF_BOUNDS` (0x50020004) for the dead-pointer class and the pin asserts the hex (cr_ui.cpp:3646, test_cr_ui.cpp:5122) — verified; contract comment is stale | cr_ui.h (last_autosave_path block) |
| D-4 | Error-code count "45" in five doc sites + the gate | Real nonzero codes: 43 (+ OK = 44 values); the gate's regex also matches 2 og-code literals in the cr_error.h:6-7 comment (phantom matches) — reproduced with the gate's own regex | cr_error.h:25-114 vs API.md:95, CORE-API-CURRENT.md:14/15/43, API-SURFACE-AUDIT.md:32; tools/doc_truth_gate.py |
| D-5 | "92 CR_UI_API lines" | Tree has 94 (91 exports + 3 `#define`s) | API.md:96, API-SURFACE-AUDIT.md:32 |
| D-6 | docs/testing.md counts 1,404 tests / 65 files / 15 suites | Tree: 1,666 / 68 / 16 (doc-inventory spot-check) — re-derive at T17 before writing | docs/testing.md |
| D-7 | error-fix-registry F13 "no decryption at all" | M-033 landed FVEK/XTS decryption | docs/error-fix-registry.md |
| D-8 | FIXED-LEDGER header "300/328 rows" | 493 rows on disk at Phase 0 (re-count at T17) | diff-to-fix/FIXED-LEDGER.md |
| D-9 | SUMMARY.md claims 17/15 MAJOR | 8 MAJOR open at Phase 0 census (doc-inventory) — re-derive at T17 | diff-to-fix/SUMMARY.md |

## 3. Open — BEHAVIOR disposition (filed, not fixed — see api-findings.md)

| # | Defect | Sites |
|---|--------|-------|
| B-1 | Exit-code inconsistency: `recover` exits 1 on source-open failure where `bench`/`fix`/`apply` exit 2 on the same class | rrecov.cpp (per-subcommand open-failure returns; cli-surface sheet §9) |
| B-2 | Disconnect hint prints `` `rrecov resume %ls` `` — no `resume` subcommand exists (the mechanism is the session checkpoint + re-scan resume contract) | rrecov.cpp:2090 |
| B-3 | `-h` usage omits 7 parsed scan flags and exit 5 | rrecov.cpp:3858 block vs the scan parser |

## 4. Corpus parity (T5 seed)

PARITY-DELTAS.md carries 153 unique D-rows (gate-verified unique at HEAD). The
conflict-marker region that hid the D-155..159 vs D-160..162 seat clash has been swept by
the racing window; the surviving register is the authoritative divergence list. Byte-level
format comparisons against the corpus (report JSON schema, crsf, session sidecar) are the
T17/T18 verification surface; no NEW undocumented divergence was introduced by this
session's commits (resume_last implements the documented M-154 contract; image --create
makes the documented invocation reach the unchanged flow).
