# CI-FAILURES.md — Default-branch failure baseline sheet

Phase-0 baseline capture (release-engineering session, subagent B). Date: 2026-11-26, capture window ~11:50–12:10 UTC. Default branch `main` at HEAD `126ea398ffa3123c2ff643b196deb1a7b030baab`.

Method: GitHub Actions REST API (`/actions/runs` for main + all recent, `/actions/workflows`, `/actions/runs/{id}/jobs` for job and step conclusions) plus downloaded run-log archives (`/actions/runs/{id}/logs`), unzipped under `/tmp/cilogs/<run id>/` (outside the repo, per constraint). Every error excerpt below is copied verbatim from those logs; the runner's leading timestamps and ANSI color codes are trimmed for readability (the message text itself is untouched; `[drive-ui ...]` timestamps are inside the tool's own messages and kept). Diagnosis only — no fixes applied by this session.

Scope: the CURRENT failure state of all 10 workflows on `main` as of 2026-11-26 — per-workflow latest run, and for every red/cancelled run the failing job, the failing step and the exact captured error. Every red run in the fetched window (Nov 18 15:20 – Nov 26 12:10) is accounted for either in §3–§5 or in the preserved 2026-11-18 session sheet in the Appendix. The 2026-11-18 sheet's "current reds" were all subsequently fixed (see §5).

Trigger topology (why the lanes differ in freshness; verified in `.github/workflows/*.yml`):
- push lanes on `main` with path filters: engine-ci (`engine/**`, `!**.md`), bridge-integration (`engine/**`, `ui/src-tauri/**`, `contracts/**`), ui-ci (`ui/**`, `contracts/**`, own yml), ui-e2e (`ui/**`, own yml; `concurrency: cancel-in-progress: true`), contracts-ci (`contracts/**`, `engine/include/**`, `ui/src/connector/**`)
- workflow_dispatch only: named-feature-fixture, benchmark-src, forensic-repro, validation-evidence
- tag pushes (`v*`): release

Consequence at HEAD 126ea398 (an engine-only commit: `engine/src/core/ntfs/cr_mftwalk.cpp` + `engine/tests/unit/test_ntfs_volume.cpp`): only engine-ci and bridge-integration triggered today; ui-ci/ui-e2e/contracts-ci last ran at their last triggering commit (20efb62a / 7c7b62b6 — both ancestors of HEAD, no ui/contracts changes since); the four dispatch lanes run only when dispatched.

## 1. Per-workflow current status (latest run on main, any event)

| Workflow | Latest run on main | Head | Event | Status | Started (UTC) | Duration | Current state |
|---|---|---|---|---|---|---|---|
| benchmark-src | [44359663812](https://github.com/heicfast/final-prod/actions/runs/44359663812) | `fc0e1e19` | workflow_dispatch | completed / **failure** | 11-18 17:31 | 201m32s | RED — dispatch-only, never re-run since |
| bridge-integration | [45814007296](https://github.com/heicfast/final-prod/actions/runs/45814007296) | `126ea398` (HEAD) | push | completed / success | 11-26 11:49 | 16m07s | GREEN at current HEAD |
| contracts-ci | [44375808948](https://github.com/heicfast/final-prod/actions/runs/44375808948) | `7c7b62b6` | push | completed / success | 11-18 19:00 | 12s | green at last trigger (no path-matching commits since) |
| engine-ci | [45814007501](https://github.com/heicfast/final-prod/actions/runs/45814007501) | `126ea398` (HEAD) | push | completed / success | 11-26 11:49 | 11m29s | GREEN at current HEAD |
| forensic-repro | [44369227822](https://github.com/heicfast/final-prod/actions/runs/44369227822) | `bd01ac19` | workflow_dispatch (release_tag `v0.9.5`) | completed / **failure** | 11-18 18:23 | 22m20s | RED — dispatch-only, never re-run since (current tag is v0.9.6) |
| named-feature-fixture | [45811533301](https://github.com/heicfast/final-prod/actions/runs/45811533301) | `d65a5d12` (HEAD~1) | workflow_dispatch | completed / success | 11-26 11:32 | 30m27s | GREEN — the F-01 closed-loop fix proof (§5.1) |
| release | [44391756880](https://github.com/heicfast/final-prod/actions/runs/44391756880) | `f4dcc3e8` (tag v0.9.6) | push (tag) | completed / success | 11-18 20:30 | 29m29s | green on the shipped v0.9.6 tag |
| ui-ci | [44475947005](https://github.com/heicfast/final-prod/actions/runs/44475947005) | `20efb62a` | push | completed / success | 11-19 06:57 | 4m32s | green at last trigger (all 16 steps; no ui/ changes since) |
| ui-e2e | [44475947192](https://github.com/heicfast/final-prod/actions/runs/44475947192) | `20efb62a` | push | completed / success | 11-19 06:57 | 6m54s | green at last trigger |
| validation-evidence | [44366316888](https://github.com/heicfast/final-prod/actions/runs/44366316888) | `dcee10aa` | workflow_dispatch | completed / success | 11-18 18:07 | 16m27s | green at last dispatch (T14 + T15 both success) |

All head SHAs in this sheet are ancestors of HEAD 126ea398 (verified with `git merge-base --is-ancestor`).

## 2. Runs in flight at session start — final state (all three went green)

The session opened ~11:50 UTC with three runs in flight; all three completed during this capture, all SUCCESS:

| Run | Workflow | Head | Final state (completed at UTC) | Jobs / steps |
|---|---|---|---|---|
| [45814007501](https://github.com/heicfast/final-prod/actions/runs/45814007501) | engine-ci | `126ea398` | success (12:00:53) | `Lint + parse gates` success (clang-format check, dead-surface lints, PS AST parse, workflow YAML parse, M-088 all green); `Build + test (MSVC Release, vcpkg cache)` success (/W4 /WX build, full ctest, Win7 posture gate, G-09, exit-4 probes, M-071 all green) |
| [45811533301](https://github.com/heicfast/final-prod/actions/runs/45811533301) | named-feature-fixture | `d65a5d12` | success (12:03:03) | `fixture` success; `engine-flow` success — `VERDICT: PASS` (excerpt in §5.1) |
| [45814007296](https://github.com/heicfast/final-prod/actions/runs/45814007296) | bridge-integration | `126ea398` | success (12:05:31) | `Build engine + link bridge + run integration tests` success (incl. the generated-ABI freshness gate) |

## 3. CURRENT RED #1 — benchmark-src (latest run is a failure; dispatch-only; not re-dispatched since 2026-11-18)

| Field | Value |
|---|---|
| Run | [44359663812](https://github.com/heicfast/final-prod/actions/runs/44359663812), workflow_dispatch, head_sha `fc0e1e19` (ancestor of HEAD), started 2026-11-18 17:31:42 UTC, 201m32s, conclusion failure |
| Failing job | `Build (release recipe) + the scenario matrix + the alignbench sweep` (job id 135036678696) |
| Failing step | step 8 `The scenario matrix (New-TestDisk + Run-Benchmark.ps1)` — steps 1–7 were green (engine kit build, API-harness compile, the full-ctest correctness gate, runner-context record) |
| Exact error lines | 12 hard-gate failures (1 probe + 1 recall floor + 10 perf walls), excerpt below |
| Suspected area | What the log itself shows: the failure surface is the engine read path — the cancel+resume probe resumed 0 files, permanent-delete recall collapsed to 0.27, and every IO wall is ~2x–350x over baseline (bench-open 221412 ns vs 19571.85 limit; api-recovery 17600.9 ms vs 54.12 limit). This is the same measured signature the 2026-11-18 sheet documented at a2561acd (§4b there: the IOCP ring starved non-window reads). CAVEAT: this run measured the `fc0e1e19` tree (Nov 18 17:31); the lane was never dispatched again, so whether it reproduces on the current HEAD is untested — the red is "latest known state", not a measured current-HEAD fact. |
| Resolution | (pending — needs a fresh benchmark-src dispatch on current main to re-measure) |

Excerpt (job log, scenario-matrix step; gates.md table as printed by the step):

```
api-lane complete: exit=0 queued=9 done=9 errors=0 hash_mismatch=0 (result: D:\cr-bench\work\probes\cancel-resume\api-result.json)
##[error]cancel+resume probe: cancelExit=0 cancelled=True resumeExit=0 resumedFiles=0
...
inventory[cli-S-DEL-NTFS]: files=10341 matched=10232 (path=0 namesize=10222 name=2 hash-only=8) hash_ok=2787 fp=109
metrics[ours/permanent_delete]: recall=0.2708 precision=0.2695 f1=0.2702 fp=109 -> D:\cr-bench\work\scenarios\S-DEL-NTFS\cli\metrics.json
##[error]scenario S-DEL-NTFS FAILED the 'floor' gate - cli: hash-recall 0.27084964761159 below floor 0.35 | api: hash-recall 0.27084964761159 below floor 0.35
...
## Performance gate (driver parity)
| id | metric | baseline | limit | observed | verdict |
|---|---|---|---|---|---|
| bench-seq-1m | mib_per_s >= | 536.92 | 520.81 | 217.94 | FAIL |
| bench-rand-1m | mean_us <= | 1993.22 | 2192.54 | 4338.44 | FAIL |
| bench-open | mean_ns <= | 17019 | 19571.85 | 221412 | FAIL |
| scan-quick-wall | wall_ms <= | 150 | 165 | 340 | FAIL |
| scan-deep-wall | wall_ms <= | 16505 | 18155.5 | 34820 | FAIL |
| scan-deepcarve-wall | wall_ms <= | 1501622 | 1546670.66 | 1756946 | FAIL |
| api-open | ms <= | 0.9 | 1 | 1.1 | FAIL |
| api-scan-run | ms <= | 1513221.7 | 1558618.35 | 1687596.8 | FAIL |
| api-save-session | ms <= | 10.8 | 11.88 | 14.7 | FAIL |
| api-recovery | ms <= | 49.2 | 54.12 | 17600.9 | FAIL |
##[error]12 hard gate failure(s) - see evidence/06-comparison/gates.md
##[error]the benchmark matrix failed
##[error]Process completed with exit code 1.
```

## 4. CURRENT RED #2 — forensic-repro (latest run is a failure; dispatch-only; not re-dispatched since 2026-11-18)

| Field | Value |
|---|---|
| Run | [44369227822](https://github.com/heicfast/final-prod/actions/runs/44369227822), workflow_dispatch with `release_tag: v0.9.5`, head_sha `bd01ac19` (tools checkout, ancestor of HEAD), started 2026-11-18 18:23:43 UTC, 22m20s, conclusion failure |
| Failing job | `Reproduce all reported symptoms (installed release)` (job id 135031165399) |
| Failing step | step 12 `Launch app + probes + drive the real UI - health, scan, every surface (S2 S3 S4)` — steps 1–11 were GREEN, i.e. the v0.9.5 installer ran clean (exit 0), the D-PRIV-01 manifest gate PASSED, the disk census matched (4 candidates — the D-ENUM-05 and installer-abort classes from the 11-18 sheet are gone in v0.9.5), and the S2b save/resume vehicle completed |
| Exact error lines | excerpt below |
| Suspected area | the gate message names the class itself: `D-SCAN-01 - the scan froze for 800 s (percent + found-count + sector all identical; the BF-22 stall class` — the S3 deep scan of the 64 GB SeedNTFS vdisk stalled with no progress for the full 800 s window while the UI kept ticking (`advancedPctDelta=1.8`, `liveBytesOk=true`). Product-side scan stall observed in the INSTALLED v0.9.5 release. The lane has not been dispatched since; v0.9.6 is the current tag, so the current-release state of this symptom is untested. |
| Resolution | (pending — needs a fresh forensic-repro dispatch against v0.9.6) |

Excerpt (job log, step 12; tool's own `[drive-ui ...]` timestamps kept):

```
$tag = "v0.9.5"
[install-probe] installer exited 2026-11-18T18:24:58.0772856Z after 5.6s (exit code 0)
[drive-ui 2026-11-18T18:25:17.952Z] health: 4 drive-selector candidates
[drive-ui 2026-11-18T18:25:59.168Z] S2b: drain banner (stopped early) visible: true
[drive-ui 2026-11-18T18:30:26.902Z] S2b: RESUMED scan COMPLETE
[drive-ui 2026-11-18T18:30:35.229Z] S3 probe: back to home, select the 64GB seeded vdisk, start scan
[drive-ui 2026-11-18T18:30:43.560Z] scan: ADVANCED stage first seen at 0s
[drive-ui 2026-11-18T18:45:53.317Z] D-SCAN-01 gates: maxFrozen=800s advancedPctDelta=1.8000000000000043 liveBytesOk=true
[drive-ui 2026-11-18T18:45:53.318Z] FATAL: Error: gate: D-SCAN-01 - the scan froze for 800 s (percent + found-count + sector all identical; the BF-22 stall class
    at main (file:///D:/a/final-prod/final-prod/tools/forensic/drive-ui.mjs:697:15)
##[error]Process completed with exit code 1.
```

## 5. Fixed during the capture window (context reds with exact captured errors)

All lanes below are green at their latest run; the reds are recorded because they are the last failure of each lane and the fixes are proven by the green runs cited.

### 5.1 named-feature-fixture — the F-01 closed-loop red, root-caused and fixed this morning

**The red** — [44475017206](https://github.com/heicfast/final-prod/actions/runs/44475017206), workflow_dispatch, head `1a1f6877`, 2026-11-19 06:49:41 UTC, conclusion failure. Job `engine-flow` (id 135393533600) failed at step 7 `Scan + reconstruct-deleted + byte-for-byte verify (I1-I5)` (steps 1–6 green: rrecov built, fixture downloaded; the `fixture` job was green). The engine itself recovered everything (19 files, 0 errors, 0 hash mismatches) — the VERIFIER failed, on exactly four rows, all of them deleted DIRECTORIES (they appear as `[deleted]` dir rows in the scan tree the same log prints):

```
recovered: 19 done, 0 refused, 0 error(s), 0 hash mismatch, 0 salvaged (zero-filled spans), 0 nothing-to-recover (metadata-only)
report: D:\a\_temp\nffx\engine-flow\recovery-report.json
{
  "ok": false,
  "failures": [
    "I2 missing output: Docs/Old",
    "I2 missing output: Docs/Old/sub",
    "I2 missing output: Photos/vacation",
    "I2 missing output: Photos/vacation/inner"
  ],
  "stats": {
    "manifest_rows": 26,
    "live_rows": 2,
    "deleted_rows": 24,
    "exact_rows": 21,
    "collision_rows": 2,
    "output_files": 19,
    "output_dirs": 17,
    "failures": 4
  }
}
VERDICT: FAIL
Exception: D:\a\_temp\b0cbdc9d-b229-4ad4-bfe9-3bbe605bf1b2.ps1:34
Line |
  34 |  … DE -ne 0) { throw 'VERIFIER FAILED - see verify-report.json (never ma …
     |                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     | VERIFIER FAILED - see verify-report.json (never masked)
##[error]Process completed with exit code 1.
```

**The fix** — commit `d65a5d12` (2026-11-26 11:32): "fix(nff-lane, run 44475017206): the F-01 closed-loop red root-caused to a verifier defect - deleted-DIRECTORY rows hit the I2 file-exactness branch" (touched `tools/validation/named-feature/verify-reconstruction.py` + `.github/workflows/named-feature-fixture.yml`).

**The proof** — [45811533301](https://github.com/heicfast/final-prod/actions/runs/45811533301) (dispatch at `d65a5d12`, today 11:32, completed success 12:03:03). Same scan, same 19 recovered files, verifier now clean (`exact_rows` 21 → 17 because deleted-DIRECTORY rows no longer run the file-exactness branch):

```
recovered: 19 done, 0 refused, 0 error(s), 0 hash mismatch, 0 salvaged (zero-filled spans), 0 nothing-to-recover (metadata-only)
report: D:\a\_temp\nffx\engine-flow\recovery-report.json
{
  "ok": true,
  "failures": [],
  "stats": {
    "manifest_rows": 26,
    "live_rows": 2,
    "deleted_rows": 24,
    "exact_rows": 17,
    "collision_rows": 2,
    "output_files": 19,
    "output_dirs": 17,
    "failures": 0
  }
}
VERDICT: PASS
F-01 ENGINE-FLOW VERDICT: PASS (I1-I5 green on the seeded ground truth)
```

**Prior reds on the same lane** (all Nov 18, all in the fixed chain; step + exact one-liner each):

| Run | Head | Failing job / step | Exact captured error |
|---|---|---|---|
| [44474398913](https://github.com/heicfast/final-prod/actions/runs/44474398913) | `be7966c5` | `fixture` step 3 `Build seeded fixture VHDX (generate, determinism proof, delete, verify self-test)` | `generate-fixture.ps1: Index operation failed; the array index evaluated to null.` → `Exception ... 50 | if ($LASTEXITCODE -ne 0) { throw "generate-fixture run 1 failed" }` → `generate-fixture run 1 failed` → `##[error]Process completed with exit code 1.` (fixed by `1a1f6877`: "null-safe magic lookup for magic-less rows") |
| [44391734922](https://github.com/heicfast/final-prod/actions/runs/44391734922) | `bed4d22c` | `engine-flow` step 7 (I1-I5) | verify-report `"failures": 27` — 19× `"I2 missing output: …"` (incl. `Docs/Old/sub/deep.bin`, `Photos/vacation/day1.jpg`, `Names/üñíçødé-名前- файл.txt`, …), `"I3 collision-set: expected 1..2 outputs under Collisions/ matching draft*.txt, found 0"`, 2× `"I4 unaccounted output file: _/fixture/…"`, 4× `"I2 deleted directory not reconstructed: …"` → `VERDICT: FAIL` (fixed by the be7966c5 "F01b: engine dot-transparency + fixture durability + fixture rooting" chain) |
| [44391634348](https://github.com/heicfast/final-prod/actions/runs/44391634348) | `2bab7c9a` | `engine-flow` step 7 (I1-I5) | same 27-failure verify-report class → `VERDICT: FAIL` |
| [44387954109](https://github.com/heicfast/final-prod/actions/runs/44387954109) | `590cd2b4` | `engine-flow` step 7 (I1-I5) | same I2-missing-output list class (`Docs/report.docx`, `Docs/Old`, `Photos/vacation`, …) → `VERDICT: FAIL` |
| [44380763554](https://github.com/heicfast/final-prod/actions/runs/44380763554) | `5e57d8b2` | `engine-flow` step 7 (I1-I5) | verifier crashed printing the Unicode fixture name: `UnicodeEncodeError: 'charmap' codec can't encode characters in position 647-648: character maps to <undefined>` (traceback: `verify-reconstruction.py", line 277, in main` → `cp1252.py", line 19, in encode`) → `VERIFIER FAILED - see verify-report.json (never masked)` |
| [44386982751](https://github.com/heicfast/final-prod/actions/runs/44386982751) | `5e824e44` | `fixture` step 3 | `throw "determinism FAILED: manifests differ beyond the _meta line"` → `##[error]Process completed with exit code 1.` |
| [44355793332](https://github.com/heicfast/final-prod/actions/runs/44355793332) | `a80899b7` | `fixture` step 3 | same `determinism FAILED: manifests differ beyond the _meta line` |
| [44354788432](https://github.com/heicfast/final-prod/actions/runs/44354788432) | `00abe98a` | `fixture` step 3 | same determinism throw |
| [44354166977](https://github.com/heicfast/final-prod/actions/runs/44354166977) | `aabc1ffa` | `fixture` step 3 | same determinism throw |
| [44375833304](https://github.com/heicfast/final-prod/actions/runs/44375833304) | `7c7b62b6` | `engine-flow` step 5 `Build rrecov (win-x64-release)` | the NOMINMAX compile class (see §5.4): `cr_rescue_win.cpp(193,24): error C2589` → `##[error]rrecov build failed (MSVC /W4 /WX)` |
| [44370686000](https://github.com/heicfast/final-prod/actions/runs/44370686000) | `cb2c8e0a` | `engine-flow` step 1 `Set up job` (infra, not product) | `##[error]Unable to resolve action `actions/download-artifact@631f0bb800df1fbd076b3b2d0b8c9c882bc4b1a7`, unable to find version `631f0bb800df1fbd076b3b2d0b8c9c882bc4b1a7`` (action-ref resolution failure — pinned SHA not found) |

### 5.2 ui-ci — three reds, then green at 20efb62a (44475947005, all 16 steps success)

| Run | Head | Failing step (job `ci`) | Exact captured error |
|---|---|---|---|
| [44475009006](https://github.com/heicfast/final-prod/actions/runs/44475009006) | `1a1f6877` | step 12 `Dead code (knip)` | `$ knip` → `Unused files (1)` → `tests/timing-budget-proof.ts` → `[ELIFECYCLE] Command failed with exit code 1.` → `##[error]Process completed with exit code 1.` — fixed by `20efb62a` ("tests/timing-budget-proof.ts is standalone evidence - knip-ignore it") |
| [44474393997](https://github.com/heicfast/final-prod/actions/runs/44474393997) | `be7966c5` | step 8 `Format check` | `$ prettier --check .` → `Checking formatting...` → `[warn] tests/timing-budget-proof.ts` → `[warn] Code style issues found in the above file. Run Prettier with --write to fix.` → `[ELIFECYCLE] Command failed with exit code 1.` — fixed by `1a1f6877` ("prettier on the timing-proof") |
| [44391744949](https://github.com/heicfast/final-prod/actions/runs/44391744949) | `f4dcc3e8` | step 14 `Unit + component tests (coverage thresholds 70/60/70/70)` | `❯ tests/reference-surfaces.test.tsx (8 tests | 1 failed)` → `× renders File Repair truthfully — no canned queue, the volume workbench is the surface (M31) 1026ms` → `Test Files  1 failed | 29 passed (30)` / `Tests  1 failed | 224 passed (225)` → `##[error]TestingLibraryElementError: Unable to find an element with the text: /no repairable problems were found/i. This could be because the text is broken up by multiple elements. In this case, you can provide a function for your text matcher to make your matcher more flexible.` — fixed by the M31 timing-budget change in `be7966c5` (which created `ui/tests/timing-budget-proof.ts`, +34 lines, and adjusted `ui/tests/reference-surfaces.test.tsx`; the 8c86a484 worklog session records the "ui-ci M31 timing budget" root-cause). The full unit+component step is green at 20efb62a. |

### 5.3 ui-e2e — one cancelled run (concurrency supersede, not a product red)

[44474393995](https://github.com/heicfast/final-prod/actions/runs/44474393995) (`be7966c5`, 2026-11-19 06:44:37) — job `e2e` CANCELLED at step 8 `Run e2e suite (preview server)`. The log shows the suite interrupted mid-wait, then the cancellation (ui-e2e has `concurrency: group … cancel-in-progress: true`; the next push `1a1f6877` at 06:49:37 superseded it, and that run [44475009083](https://github.com/heicfast/final-prod/actions/runs/44475009083) is success):

```
    Locator: getByText(/scan completed/i).first()
    Expected: visible
    Error: element(s) not found
    Call log:
      - Expect "toBeVisible" getByText(/scan completed/i).first() with timeout 90000ms
      - waiting for getByText(/scan completed/i).first()
      - Target page, context or browser has been closed
    > 24 |   await expect(page.getByText(/scan completed/i).first()).toBeVisible({ timeout: 90_000 });
        at startScanAndAwaitCompletion (D:\a\final-prod\final-prod\ui\e2e\lane-c-surfaces.spec.ts:24:59)
    Error: apiRequestContext._wrapApiCall: Target page, context or browser has been closed
  1 interrupted
    e2e\lane-c-surfaces.spec.ts:50:1 › recoverability: the completion celebration carries the engine verdict
##[error]The operation was canceled.
```

The lane is green at its latest run (44475947192, `20efb62a`).

### 5.4 engine-ci — Nov-18 evening reds (all fixed; lane GREEN at HEAD today, 45814007501)

| Run | Head | Failing job / step | Exact captured error |
|---|---|---|---|
| [44380469931](https://github.com/heicfast/final-prod/actions/runs/44380469931) | `5e57d8b2` | `Build + test (MSVC Release, vcpkg cache)` step 7 `Win7 posture gate (stamp + import allowlist)` | `FAIL: import 'SetFilePointerEx' (from KERNEL32.dll) is not in the Win7 allowlist` → `##[error]import allowlist failed: build\win-x64-release\src\app\Release\rrecov.exe` → `##[error]Process completed with exit code 1.` (the 9653e9d9 commit message names this as one of "7c7b62b6's three CI reds" repaired in the v0.9.6 bump session) |
| [44375809024](https://github.com/heicfast/final-prod/actions/runs/44375809024) | `7c7b62b6` | `Build + test (MSVC Release, vcpkg cache)` step 5 `Configure + build (win-x64-release, /W4 /WX)` | `D:\a\final-prod\final-prod\engine\src\win\cr_rescue_win.cpp(193,24): error C2589: '(': illegal token on right side of '::'` + `error C2059: syntax error: ')'` → `##[error]build failed (MSVC /W4 /WX - the errors above are the fix list)` (the NOMINMAX class — Windows min/max macros vs `std::min/max`) |
| [44372458287](https://github.com/heicfast/final-prod/actions/runs/44372458287) | `42e1e3ea` | `Lint + parse gates` step 8 `CI-shape doc-truth gate (M-088)` | lint-gate failure (Build+test job skipped — need-on-lint); superseded by the 7c7b62b6-era repairs; engine-ci green from 44380469931's successor runs through today's 45814007501 |
| [44370664371](https://github.com/heicfast/final-prod/actions/runs/44370664371) | `cb2c8e0a` | `Lint + parse gates` step 7 `Workflow YAML parse gate` | workflow-YAML parse failure of that era's edit (Build+test skipped); same repair chain |

### 5.5 bridge-integration — Nov-18 evening reds (all fixed; lane GREEN at HEAD today, 45814007296)

| Run | Head | Failing job / step | Exact captured error |
|---|---|---|---|
| [44375809197](https://github.com/heicfast/final-prod/actions/runs/44375809197) | `7c7b62b6` | `Build engine + link bridge + run integration tests` step 11 `[2/5] Engine build (win-x64-release, /W4 /WX)` | same `cr_rescue_win.cpp(193,24): error C2589` / `error C2059` pair → `##[error]build failed` → `##[error]Process completed with exit code 1.` |
| [44371227401](https://github.com/heicfast/final-prod/actions/runs/44371227401) | `8d61644e` | same job, step 9 `[1/5] Generated ABI files freshness gate` | `##[error]generated ABI layout files are STALE - re-run: python tools/gen_abi_layouts.py . && clang-format -i engine/tests/abi_layout_dump.c and commit the result` |
| [44370664098](https://github.com/heicfast/final-prod/actions/runs/44370664098) | `cb2c8e0a` | same job, step 9 `[1/5] Generated ABI files freshness gate` | same STALE error line |

(The many `cancelled` bridge-integration runs in the window — 44391712845, 44391157556, 44374871109, 44373274874, 44373058950, 44372458461, 44370249380, 44369205673, 44367523897, 44347614883 — are concurrency supersede cancellations from rapid successive pushes, not product failures. Last completed green before today: [44474394045](https://github.com/heicfast/final-prod/actions/runs/44474394045) at `be7966c5`.)

### 5.6 release — the v0.9.6 tag's first two pushes were red (version lockstep), the final tag push is green

| Run | Head (tag) | Failing job / step | Exact captured error |
|---|---|---|---|
| [44390728388](https://github.com/heicfast/final-prod/actions/runs/44390728388) | `9653e9d9` (v0.9.6) | `full-product` step 8 `Full product build (all lanes + installer + portable + checksums)` | `== final-prod build == version=0.9.6 target=all config=Release` → `[gate] VERSION consistency (0.9.6)` → `Exception: D:\a\final-prod\final-prod\build.ps1:40` → `ui/package.json version 0.9.5 != VERSION 0.9.6` |
| [44391177507](https://github.com/heicfast/final-prod/actions/runs/44391177507) | `2bab7c9a` (v0.9.6) | same job, same step | `Exception: D:\a\final-prod\final-prod\build.ps1:42` → `ui/src-tauri/Cargo.toml version != 0.9.6` (the same one-lockstep-row-at-a-time pattern as the 0.9.4 bump, 11-18 sheet §6a/§7) |

The v0.9.6 tag's final push [44391756880](https://github.com/heicfast/final-prod/actions/runs/44391756880) (`f4dcc3e8`) is green — `full-product` success, 29m29s. v0.9.6 is the current tag.

### 5.7 validation-evidence — two dispatch reds before the latest green dispatch

| Run | Head | Failing job / step | Exact captured error |
|---|---|---|---|
| [44363284154](https://github.com/heicfast/final-prod/actions/runs/44363284154) | `3809fbc5` | `T15 installer runtime matrix (clean runner)` step 7 `Full product build (all lanes; tests are the CI lanes' job)` (T14 was success) | NSIS: `File: "$PLUGINSDIR\engine-kit\*" -> no files found.` → `Error in script "D:\a\final-prod\final-prod\installer\recoverywizard.nsi" on line 351 -- aborting creation process` → `Exception: build.ps1:249` → `installer build failed`; cascade: `##[error]No files were found with the provided path: t15-evidence/. No artifacts will be uploaded.` |
| [44359489849](https://github.com/heicfast/final-prod/actions/runs/44359489849) | `87c04b7f` | `T15 installer runtime matrix (clean runner)` step 8 `Run the runtime matrix (install / update / repair / uninstall)` (T14 was success) | `[matrix] uninstall(silent)        CHECK-LEFTOVERS` (install/shortcuts/update/repair rows all `ok`) → `Exception: D:\a\final-prod\final-prod\tools\validation\installer-matrix.ps1:496` → `T15 matrix has failing rows — inspect the evidence files` |

Latest dispatch [44366316888](https://github.com/heicfast/final-prod/actions/runs/44366316888) (`dcee10aa`, 11-18 18:07) is green — both the T14 and T15 jobs success.

### 5.8 benchmark-src — the two reds before the current one (§3)

| Run | Head | Failing step | Exact captured error |
|---|---|---|---|
| [44346817200](https://github.com/heicfast/final-prod/actions/runs/44346817200) | `9ad991d6` | step 8 `The scenario matrix` | `##[error]cancel+resume probe: cancelExit=0 cancelled=True resumeExit=0 resumedFiles=0` → `##[error]scenario S-DECAY-NTFS FAILED the 'floor' gate - cli: hash-recall 0 below floor 0.85 | api: hash-recall 0 below floor 0.85` → `##[error]scenario S-MFT-NTFS threw: Exception calling "CorruptMft" with "4" argument(s): "Arithmetic operation resulted in an overflow."` → 5 perf-gate walls (bench-rand-1m 2533.2, bench-open 213135, scan-deep-wall 23940, api-save-session 75.3, api-recovery 18718.5 — all `<=` limits violated) → matrix failed |
| [44344598911](https://github.com/heicfast/final-prod/actions/runs/44344598911) | `a19afb56` | step 6 `Optional full ctest (the correctness gate before measuring)` | `CMake Error: Could not read presets from D:/a/final-prod/final-prod:` → `File not found: D:/a/final-prod/final-prod/CMakePresets.json` → `##[error]ctest red before the matrix - not measuring a red tree` (the ctest-preset working-directory class from the 11-18 sheet §3; FIXED between a19afb56 and fc0e1e19 — the fc0e1e19 run's ctest step is green) |

## 6. Currently red on main — summary

The latest run of exactly TWO lanes on `main` is a failure, and both are stale dispatch lanes (not re-run since 2026-11-18, reds recorded at pre-repair heads):

1. **benchmark-src** — run [44359663812](https://github.com/heicfast/final-prod/actions/runs/44359663812) (`fc0e1e19`, dispatch, 11-18 17:31): scenario-matrix step, `12 hard gate failure(s)` — top-level error lines: `##[error]cancel+resume probe: cancelExit=0 cancelled=True resumeExit=0 resumedFiles=0` and `##[error]scenario S-DEL-NTFS FAILED the 'floor' gate - cli: hash-recall 0.27084964761159 below floor 0.35 | api: hash-recall 0.27084964761159 below floor 0.35` plus ten perf-gate walls.
2. **forensic-repro** — run [44369227822](https://github.com/heicfast/final-prod/actions/runs/44369227822) (`bd01ac19`, dispatch against release v0.9.5, 11-18 18:23): UI-drive step, top-level error: `FATAL: Error: gate: D-SCAN-01 - the scan froze for 800 s (percent + found-count + sector all identical; the BF-22 stall class`.

Everything else is green at its latest run: **engine-ci** and **bridge-integration** at the current HEAD 126ea398 (today: 45814007501, 45814007296); **named-feature-fixture** at d65a5d12 (today: 45811533301 — the F-01 closed-loop fix proof); **ui-ci** and **ui-e2e** at 20efb62a; **contracts-ci** at 7c7b62b6; **release** on the v0.9.6 tag; **validation-evidence** at dcee10aa.

Operational reading for the release-engineering session: no push lane is red at HEAD; the two red lanes need a fresh workflow_dispatch each (benchmark-src on current main; forensic-repro against the current tag v0.9.6) to convert "last known state" into current facts.

## 7. Raw log archive (outside the repo)

Unzipped per-run job logs live under `/tmp/cilogs/<run id>/` (job logs named `0_<job>.txt`, `1_<job>.txt`, …). Captured for this sheet:

`/tmp/cilogs/{44475017206, 44359663812, 44369227822, 44474398913, 44391734922, 44475009006, 44474393997, 44391744949, 44474393995, 45811533301, 45814007296, 45814007501, 44380469931, 44375809024, 44375809197, 44371227401, 44370664098, 44391177507, 44390728388, 44363284154, 44359489849, 44359495858, 44346817200, 44344598911, 44391634348, 44387954109, 44380763554, 44386982751, 44355793332, 44354788432, 44354166977, 44375833304, 44370686000}/`

Job/step metadata JSON from the API: `/tmp/ci/jobs_<run id>.json`; run listings: `/tmp/ci/runs_main_p1.json`, `/tmp/ci/runs_all_p1.json`, `/tmp/ci/runs_all_p2.json`.

(The 2026-11-18 session's log archives at `/home/z/my-project/ci-logs/` are no longer present; its excerpts are preserved verbatim in the Appendix below.)

---

# Appendix — preserved history: the 2026-11-18 session sheet (verbatim)

The content below is the previous CI-FAILURES.md session sheet, captured 2026-11-18
(log evidence 15:46-17:31 UTC), preserved for the audit trail. It is HISTORICAL: every
"CURRENT RED" it names at head 9352c81d/fc0e1e19-era was subsequently fixed (see the
fix chains in §5 above — e.g. the abi_layout_dump.c formatting class went green with the
ABI freshness gate in today's bridge-integration run 45814007296; the libz spelling,
IOCP ring, D-ENUM-05, installer pre-flight, C4456 and version-lockstep classes all have
landed fixes cited in its own Resolution rows). Its run IDs, error excerpts and root-cause
analyses remain the reference for those runs. Only the heading levels were demoted to
nest under this appendix; the text is otherwise unchanged.

Session: CI/CD workflow optimization (subagent B). Date: 2026-11-18 (log evidence ~15:46–15:58 UTC).
Method: GitHub API run/job/step metadata + downloaded run logs + local repo cross-reference at each head_sha.
Scope: every failure on the default branch (`main`) as of 2026-11-18, per workflow, newest first; earlier same-class failures recorded for context. Diagnosis only — no fixes applied. `Resolution: FIXED on main by the concurrent lane / earlier commits (see per-run rows); no action at this HEAD — re-verified by this session's dispatches.` placeholders are for the lead agent.

Log archives: `/home/z/my-project/ci-logs/run<id>/` (outside the repo clone, per constraint).

---

### 1. engine-ci — CURRENT RED (head 9352c81d, both runs same failure)

| Field | Value |
|---|---|
| Runs | push [44339513142](https://github.com/heicfast/final-prod/actions/runs/44339513142) (43 s) + workflow_dispatch [44339552858](https://github.com/heicfast/final-prod/actions/runs/44339552858) (45 s), both head_sha `9352c81daffe329949023d99ad53f2e5c7d689c1`, event push/dispatch, conclusion failure |
| Failing job | `Lint + parse gates` (job id 134925283645 / 134925445094) |
| Failing step | `[1/2] clang-format check (src/ include/ tests/)` (step 4) |
| Exact error lines | `engine\tests\abi_layout_dump.c:23:18: error: code should be clang-formatted [-Wclang-format-violations]` — 20 violations printed, all in `abi_layout_dump.c`, then `##[error]files need formatting - fix: clang-format -i (all files under src/ include/ tests/)` (log lines 128–149 of run44339513142) |
| Root cause | Commit 9352c81d regenerated `engine/tests/abi_layout_dump.c` (the generated ABI layout dump) with `tools/gen_abi_layouts.py` but committed the file WITHOUT running the pinned `clang-format 18.1.8` pass that both gates require. The file collapsed from 843 formatted lines (9618d9ed) to 453 raw one-statement-per-line lines; the regenerated C dumper emits long single-line `fprintf(...)` rows that violate the repo's column limit. Locally reproduced with clang-format 18.1.8: 1176 violations on the committed file, 0 violations on the 9618d9ed/c049f83a versions. This is a process miss, not a code bug: the commit message even claims "layouts + abi.rs re-pinned same commit" but skipped the format half of the same-commit discipline. |
| Affected files | `engine/tests/abi_layout_dump.c` (generated by `tools/gen_abi_layouts.py`) |
| Proposed root-cause fix | Re-run the generator + pinned formatter and commit the result: `python tools/gen_abi_layouts.py . && clang-format -i engine/tests/abi_layout_dump.c` (clang-format 18.1.8) — exactly the remedy the gate's error message states. Optionally harden: run `clang-format -i` INSIDE `tools/gen_abi_layouts.py` (or a post-generation hook) so a generated file can never be committed unformatted again. |
| Resolution | (pending) |

Note: the second engine-ci job (`Build + test (MSVC Release, vcpkg cache)`) was `skipped` — the lane is need-on-lint, so the compile gate never ran; after the format fix the full MSVC build+ctest leg must be re-verified.

Prior same-class reds on main (context, all fixed by the commits cited):
- Runs [44261473448](https://github.com/heicfast/final-prod/actions/runs/44261473448) (18b918d5) and [44259547753](https://github.com/heicfast/final-prod/actions/runs/44259547753) (5213164d): same step, violations in `engine/src/core/storage/cr_cached.cpp` + `engine/src/win/cr_transport.cpp` (fix-1/fix-2 perf files committed unformatted). Fixed by 376462df ("clang-format 18.1.8 over the session's touched files") — green at 44268752214.
- Runs 44104298262 / 44099373023 / 44094503551 / 44084452349 / 44076508700 (Nov 17): `Win7 posture gate (stamp + import allowlist)` step — `FAIL: import 'ordinal_N' (from OLEAUT32.dll) is not in the Win7 allowlist` then `##[error]import allowlist failed: build\win-x64-release\src\app\Release\rrecov.exe`. Root cause: the engine's SMART/WMI path (oleaut32 COM) imports OLEAUT32 by ordinal; the allowlist learned the DLL-name layer (d24c1cab) then the ordinal layer (93389402). Fixed at 385d206e-era green runs.

### 2. bridge-integration — CURRENT RED (head 9352c81d, both runs same failure)

| Field | Value |
|---|---|
| Runs | push [44339513235](https://github.com/heicfast/final-prod/actions/runs/44339513235) (107 s) + workflow_dispatch [44339558061](https://github.com/heicfast/final-prod/actions/runs/44339558061) (110 s), both head_sha `9352c81daffe329949023d99ad53f2e5c7d689c1`, conclusion failure |
| Failing job | `Build engine + link bridge + run integration tests` (job id 134925284792 / 134925460014) |
| Failing step | `[1/5] Generated ABI files freshness gate` (step 9) |
| Exact error lines | `##[error]generated ABI layout files are STALE - re-run: python tools/gen_abi_layouts.py . && clang-format -i engine/tests/abi_layout_dump.c and commit the result`; the gate's `git diff --exit-code` printed a whole-file diff of `engine/tests/abi_layout_dump.c` (`@@ -20,434 +20,826 @@`: every long `fprintf` line split into two continuation lines — e.g. `-    fprintf(out, "\"%s\": %llu%s ", "error_ranges", (unsigned long long)offsetof(cr_ui_event, error_ranges), ",");` → `+    fprintf(out, "\"%s\": %llu%s ", "error_ranges",\n+            (unsigned long long)offsetof(cr_ui_event, error_ranges), ",");`) |
| Root cause | SAME root cause as engine-ci above (single defect, two gates): 9352c81d's regenerated `abi_layout_dump.c` is unformatted, so the freshness gate — which regenerates AND clang-formats, then demands a clean `git diff` — sees a diff. The generated *content* (offsets/struct table, 29 structs, 366 field paths) is correct; only the formatting is stale. The `.header.sha` and `abi_layouts.rs` sides of the gate are clean (diff shows only the `.c` file). |
| Affected files | `engine/tests/abi_layout_dump.c` |
| Proposed root-cause fix | Same one-commit fix as engine-ci §1: commit the generator output after `clang-format -i` (18.1.8). That single commit turns both engine-ci and bridge-integration green; no separate repair needed. |
| Resolution | (pending) |

Prior reds on main (context): runs 44261473417 / 44257909901 / 44256398461 / 44104298433 / 44102634559 / 44099373025 (Nov 17–18, older commits) — different, older causes already repaired on main (it-crate compile, layout drift classes); superseded by the fixes in e119cc71/b43ce273-era commits; green from 44244567592 (be3b8256) through 44334552899 (c049f83a).

### 3. benchmark-src — CURRENT RED (head 9352c81d)

| Field | Value |
|---|---|
| Run | workflow_dispatch [44339547294](https://github.com/heicfast/final-prod/actions/runs/44339547294) (689 s), head_sha `9352c81daffe329949023d99ad53f2e5c7d689c1`, conclusion failure |
| Failing job | `Build (release recipe) + the scenario matrix + the alignbench sweep` (job id 134925426642) |
| Failing step | `Optional full ctest (the correctness gate before measuring)` (step 6) |
| Exact error lines | `CMake Error: Could not read presets from D:/a/final-prod/final-prod:` / `File not found: D:/a/final-prod/final-prod/CMakePresets.json` → `##[error]ctest red before the matrix - not measuring a red tree` |
| Root cause | The workflow step runs `ctest --preset win-x64-release ...` from the repo ROOT, but the presets live in `engine/CMakePresets.json`. engine-ci's ctest step works because that job sets `defaults.run.working-directory: engine`; build.ps1 works because it `Push-Location`s into `engine/` before invoking cmake/ctest. benchmark-src's added step (84459c0f restored the lane) has no working-directory and no `Set-Location engine`, so ctest looks for `D:/a/final-prod/final-prod/CMakePresets.json` and fails before running a single test. Workflow-definition bug, not a product bug — the run never reached the matrix. |
| Affected files | `.github/workflows/benchmark-src.yml` (step "Optional full ctest", lines ~79–84) |
| Proposed root-cause fix | Point the step at the engine subtree: either `defaults: run: working-directory: engine` for that step (`working-directory: engine` on the step, or `Set-Location engine` before ctest), or run `ctest --preset win-x64-release` via `build.ps1`'s proven form. (Diagnosis only — not applied.) |
| Resolution | (pending) |

### 4. benchmark-src — earlier reds (same lane, three distinct causes; check head_sha each)

#### 4a. Run [44334634961](https://github.com/heicfast/final-prod/actions/runs/44334634961) (383 s, head c049f83a, 15:21 UTC)

| Field | Value |
|---|---|
| Failing job / step | same job; step 3 `Engine kit (build.ps1 engine lane; the release recipe)` |
| Exact error lines | `FAIL: unexpected import DLL 'libz.dll'` → `verify_pe_um: GATE FAILED` → `Exception: D:\a\final-prod\final-prod\build.ps1:96 … throw "engine PE verify failed (Win7 import gate): $pe"` |
| Root cause | The runner's vcpkg revision deployed the release zlib as `libz.dll` (this lane's toolchain) while `engine/tools/verify_pe_um.py`'s ALLOWED_DLLS spelled it `z.dll`/`zlib1.dll` — the d24c1cab spelling-drift class, one revision over. Already FIXED on main by b6e2997a, which added `"libz.dll": "name-only"` and `"libzd.dll": "name-only"` rows to ALLOWED_DLLS (visible in the current file at engine/tools/verify_pe_um.py lines 264–267). The current failure class is DIFFERENT (ctest preset path, §3 above). |
| Affected files | `engine/tools/verify_pe_um.py` (fix already landed in b6e2997a) |
| Resolution | (pending — likely "fixed by b6e2997a, verify with next dispatch") |

#### 4b. Run [44275941738](https://github.com/heicfast/final-prod/actions/runs/44275941738) (6811 s ≈ 1 h 53 m, head a2561acd)

| Field | Value |
|---|---|
| Failing job / step | same job; step 8 `The scenario matrix (New-TestDisk + Run-Benchmark.ps1)`; ctest step 6 was SKIPPED (run_tests input false) |
| Exact error lines | `##[error]cancel+resume probe: cancelExit=0 cancelled=True resumeExit=0 resumedFiles=0`; `##[error]scenario S-DEL-NTFS FAILED the 'floor' gate - cli: hash-recall 0.272122161315583 below floor 0.35`; five perf-gate walls: `bench-rand-1m: observed 9427.08 <= 2192.54`, `bench-open: observed 227869 <= 19571.85`, `scan-deep-wall: observed 26253 <= 18155.5`, `api-save-session: observed 13.7 <= 11.88`, `api-recovery: observed 5524.5 <= 54.12`; final `##[error]7 hard gate failure(s) - see evidence/06-comparison/gates.md` |
| Root cause | The IOCP read-ahead ring (18b918d5, fix-2) starved non-window reads: `AsyncRing::pick_victim_locked()` never returns INFLIGHT slots; in steady state all 8 slots are INFLIGHT, so every out-of-window read got `ERR_NO_MEMORY`, which `RetryingDevice::try_read` (cr_retry.cpp:49) treats as a hard error — killing cancel+resume (resumedFiles=0), recall (0.272), and random/recovery workloads (~1024x read amplification when a free slot did exist; api-recovery 102x over baseline). ALREADY FIXED on main by c049f83a ("root-cause repair of the benchmark-src regressions"): the companion sync handle serves every non-front miss; ERR_NO_MEMORY unreachable from the read path. engine-ci+bridge-integration were green at c049f83a (runs 44334569972/44334552871/44334578460) proving the repair; only benchmark-src's own lane then failed on the libz spelling (4a) and now the ctest preset path (§3). |
| Affected files | `engine/src/win/cr_transport.cpp` (AsyncRing; fix landed in c049f83a) |
| Resolution | (pending — fixed by c049f83a; the lane needs a green full-matrix run to close it) |

#### 4c. Run [44272577458](https://github.com/heicfast/final-prod/actions/runs/44272577458) (84459c0f, the lane's first run)

| Field | Value |
|---|---|
| Failing job / step | same job; step 5 `Compile the API harness` |
| Exact error lines | `##[error]kit missing cr_ui.dll` (the check `if (-not (Test-Path "$kit\cr_ui.dll"))` at the a2561acd-era workflow spelled the kit path `$kit\cr_ui.dll` but the harness link line used `$kit\lib\cr_ui.lib`; see log line 787) |
| Root cause | Kit-layout mismatch at lane creation: the workflow's harness step expected the FLAT engine kit (`build\out\stage-<ver>\engine\cr_ui.dll`) while the checked-out workflow version probed a `bin\`/`lib\` layout. Fixed by a2561acd ("the bench lane bridges build.ps1's flat kit into Run-Benchmark's release-zip bin layout"). |
| Resolution | (pending — superseded by a2561acd) |

### 5. forensic-repro — latest red (dispatch, head ffd1e0c2, 06:11 UTC)

| Field | Value |
|---|---|
| Run | [44249495727](https://github.com/heicfast/final-prod/actions/runs/44249495727), head_sha ffd1e0c2, conclusion failure (installs release v0.9.3 by `release_tag` input default) |
| Failing job / step | `Reproduce all reported symptoms (installed release)` — the one-step CDP drive; failing sub-gate S2b/S5 |
| Exact error lines | `[drive-ui] health: 2 drive-selector candidates` … `S2b: home, select the 8GB SeedEXFAT vdisk` → `S2b/S5 FAILED: locator.click: Timeout 15000ms exceeded. — waiting for getByRole('radio', { name: /SeedEXFAT/i }).first()` → `FATAL: Error: gate: S2b/S5 save-resume - locator.click: Timeout 15000ms exceeded.` (drive-ui.mjs:529). Runner facts: `[census] 4 physical disks recorded`; disk map shows disk2 (64 GB GPT, letter null, fs null) and disk3 (8 GB MBR exFAT, Y:, the SeedEXFAT fixture). |
| Root cause | D-ENUM-05 regression IN THE INSTALLED v0.9.3 PRODUCT, not in the workflow: the 0.9.3 device-kind axis (ba2b9347/D-ENUM-02) routed bus 14/15/16 (File-Backed Virtual) to `kind: 'virtual'`, and the landing page/health window filtered `kind !== 'virtual'` — so the two diskpart-attached VHDX fixtures (SeedNTFS 64 GB + SeedEXFAT 8 GB, both File-Backed bus) never rendered as radio cards; only the two SAS runner disks did (health found exactly 2 candidates while the OS census had 4). The `getByRole('radio', {name:/SeedEXFAT/i})` locator therefore could never resolve. Fixed on main in three layers by 96917f6c (bridge: virtual-bus disks fall to the removable-flag arm, `ui/src-tauri/src/commands/devices.rs`), 3c3e02c5 (consumer: `use-physical-devices` drops the silent kind filter), and the docs row; 96917f6c is in tag v0.9.4 but NOT in v0.9.3 — the red run installed v0.9.3 per the input default. The workflow itself is correct: it failed exactly as designed (no masking). |
| Affected files | product: `ui/src-tauri/src/commands/devices.rs` (+ consumer filters); tool: `tools/forensic/drive-ui.mjs` (S2b locator — symptom site) |
| Proposed root-cause fix | Already landed on main (96917f6c/3c3e02c5). To close the lane: re-dispatch forensic-repro with `release_tag: v0.9.4` (the workflow input default is still `v0.9.3` — consider updating the default input to the current tag so the lane stops re-testing the known-bad release). |
| Resolution | (pending) |

### 6. validation-evidence — latest reds (dispatch, Nov 18 morning)

#### 6a. Runs [44262457689](https://github.com/heicfast/final-prod/actions/runs/44262457689) (head 18b918d5), [44253108774](https://github.com/heicfast/final-prod/actions/runs/44253108774) (head 42ef613b)

| Field | Value |
|---|---|
| Failing jobs / steps | `T14 engine latency + bridge filter_memory measurements` step `Engine kit (build.ps1 lane; contracts gate runs inside)`; `T15 installer runtime matrix` step `Full product build (all lanes; tests are the CI lanes' job)` (both jobs fail independently; artifact-upload steps then fail with `No files were found with the provided path: t14-engine.jsonl` / `t15-evidence/.` — cascade, not a cause) |
| Exact error lines (44262457689) | `engine\src\win\cr_transport.cpp(542,45): error C2220: the following warning is treated as an error` … `warning C4456: declaration of 'g' hides previous local declaration` (see declaration of 'g' at 538,25) … `cr_transport.cpp(546,30): error C2440: '=': cannot convert from 'std::lock_guard<std::mutex>' to 'DWORD'` → `throw 'engine build failed'` (build.ps1:71). (44253108774 instead: `[gate] VERSION consistency (0.9.4)` → `throw "ui/package.json version 0.9.3 != VERSION 0.9.4"` at build.ps1:40.) |
| Root cause | (44262457689, 18b918d5): the fix-2 IOCP ring's completion loop declared `const DWORD g = ok ? 0 : GetLastError();` and then `std::lock_guard<std::mutex> g(s->mu);` — the second `g` shadowed the DWORD under /W4 /WX (C4456) and `s->gle = g` assigned the lock_guard (C2440). Fixed by 03c07984 (rename to `gle`/`lg`). (44253108774, 42ef613b): the 0.9.4 version lockstep was half-done — VERSION said 0.9.4 while ui/package.json and three other consumers said 0.9.3; build.ps1's consistency gate threw. Fixed by 9c9597e7 (completed the bump) — the v0.9.4 tag builds (release run 44256566315 green). Both failure classes are stale on main; the lane has simply not been re-dispatched since 07:53 (latest validation-evidence run is 44262457689). |
| Affected files | `engine/src/win/cr_transport.cpp` (fixed 03c07984); version consumers (fixed 9c9597e7) |
| Resolution | (pending — needs a fresh dispatch on current main to confirm green) |

#### 6b. Run [44255564017](https://github.com/heicfast/final-prod/actions/runs/44255564017) (head b43ce273)

T14 job step 8 `Run the ignored T14 tests (release, serialized, uncaptured)` failed — same C4456/C2440 compile class as 6a on the pre-03c07984 tree (18b918d5 was the parent content); superseded by 03c07984. Resolution: FIXED for the shipped line by v0.9.4 (96917f6c + 3c3e02c5, D-ENUM-05); the run installed v0.9.3 by its release_tag input..

### 7. release — latest red (head 3c3e02c5, 06:43 UTC)

| Field | Value |
|---|---|
| Run | [44253461220](https://github.com/heicfast/final-prod/actions/runs/44253461220), head_sha 3c3e02c5, conclusion failure |
| Failing job / step | `full-product`, step `Full product build (build.ps1 all lanes)` |
| Exact error lines | `== final-prod build == version=0.9.4 target=all config=Release` / `[gate] VERSION consistency (0.9.4)` → `Exception: build.ps1:40 … throw "ui/package.json version 0.9.3 != VERSION 0.9.4"` |
| Root cause | The same half-done 0.9.4 version lockstep as 6a: 96917f6c moved VERSION to 0.9.4 but left `ui/package.json` (and three other consumers) at 0.9.3; the tag v0.9.4 pushed on that tree was refused by build.ps1's consistency gate. Fixed by 9c9597e7 ("complete the 0.9.4 bump"), after which the v0.9.4 tag's release run [44256566315](https://github.com/heicfast/final-prod/actions/runs/44256566315) is green (07:08). No current failure. |
| Resolution | (pending — closed by 9c9597e7; re-tag already validated green) |

### 8. ui-ci — last red on main (head 07eb9ed4, 06:57 UTC; green since)

| Field | Value |
|---|---|
| Run | [44255202084](https://github.com/heicfast/final-prod/actions/runs/44255202084), head_sha 07eb9ed4, conclusion failure |
| Failing job / step | `ci`, the `cargo check` leg |
| Exact error lines | `error[E0433]: cannot find type `AtomicU32` in this scope` → `src\commands\imaging.rs:369:26  cancel: Arc::new(AtomicU32::new(0))  ^^^^^^^ use of undeclared type `AtomicU32`` (and line 106) |
| Root cause | Commit 07eb9ed4 (the imaging bridge half) used `AtomicU32` in `ui/src-tauri/src/commands/imaging.rs` without the import; ui-ci's `cargo check` (no link step) caught it. Fixed by b99fc523 (one-line import; current HEAD has `use std::sync::atomic::{AtomicBool, AtomicU32, AtomicU64, Ordering};` at imaging.rs:44). All later ui-ci runs green through 44339512896 (9352c81d). No current failure. |
| Resolution | (pending — fixed by b99fc523) |

### 9. ui-e2e / contracts-ci — no failures

- ui-e2e: last 30 runs all success (latest [44339513001](https://github.com/heicfast/final-prod/actions/runs/44339513001) at 9352c81d, 313 s). No failures in window.
- contracts-ci: last 30 runs all success (latest [44339512886](https://github.com/heicfast/final-prod/actions/runs/44339512886) at 9352c81d, 23 s). No failures in window.

### 10. Summary — what is actually broken NOW on 9352c81d

1. **One product-tree defect, two red workflows**: `engine/tests/abi_layout_dump.c` regenerated by 9352c81d without the pinned clang-format pass → engine-ci (lint gate) + bridge-integration (freshness gate) both red. Single fix: commit the formatted regeneration (and ideally make the generator format its own output).
2. **One workflow-definition bug**: benchmark-src's ctest step runs from the repo root with no working-directory, so `ctest --preset win-x64-release` cannot find `CMakePresets.json` (lives under `engine/`). Fix the step's working directory.
3. Everything else red in the last 30 runs per workflow (engine-ci Nov-17 Win7-ordinal class, benchmark-src libz spelling / ring miss-path / kit layout, forensic-repro D-ENUM-05 in the installed v0.9.3, validation-evidence C4456 + version lockstep, release version lockstep, ui-ci AtomicU32) is already fixed by landed commits (b6e2997a, c049f83a, a2561acd, 96917f6c+3c3e02c5, 03c07984, 9c9597e7, b99fc523) — the lanes either have not been re-dispatched (benchmark-src full matrix, forensic-repro on v0.9.4, validation-evidence) or already went green afterwards (engine-ci pre-9352c81d, bridge-integration pre-9352c81d, release, ui-ci, ui-e2e, contracts-ci).

Workflows with no runs at all: none — all nine workflows have run history (benchmark-src total_count 4, validation-evidence 28, forensic-repro 33, release 18, ui-ci 109, ui-e2e 94, contracts-ci 69, engine-ci 63, bridge-integration 45).

Log evidence index (local scratch, outside the clone): `/home/z/my-project/ci-logs/run{44339513142,44339552858,44339513235,44339558061,44339547294,44334634961,44275941738,44272577458,44249495727,44262457689,44253108774,44253461220,44255202084,44261473448,44259547753,44104298262,44099373023}/`.

### 11. forensic-repro — the v0.9.4-era reds (dispatch, heads 6a6e05cd / 201100aa / 87c04b7f, Nov 18 16:28–17:31 UTC)

| Field | Value |
|---|---|
| Runs | [44348181946](https://github.com/heicfast/final-prod/actions/runs/44348181946) (v0.9.4 input), [44359495858](https://github.com/heicfast/final-prod/actions/runs/44359495858) (87c04b7f) — plus the 235d3c3/201100a dispatches (phantom-invalid-era) |
| Failing job / step | `Reproduce all reported symptoms (installed release)` — the `Manifest gate - the installed exe requires elevation (D-PRIV-01)` step (the first install-consumer gate) |
| Exact error lines | `[install-probe] installer exited ... after 4.2s (exit code 2)` → `[manifest-gate] appManifest = app not installed` → `D-PRIV-01 gate: the installed exe's manifest is 'app not installed' - requireAdministrator expected` |
| Root cause | **A shipped-installer product bug introduced in v0.9.4, NOT a workflow defect.** The G-4 engine-kit pre-flight added to `installer/recoverywizard.nsi` probes `${ENGINE_KIT}\cr_ui.dll` at RUNTIME — but `ENGINE_KIT` is a COMPILE-TIME build-machine path (`D:\a\...\build\out\stage-0.9.4\engine`) that exists only where makensis compiled the installer. The `File` command bakes the kit payload into the exe, so the installer is complete — but the runtime `FileExists` on the build path aborts it (exit 2) on every other machine BEFORE the inner Tauri setup runs. Proof chain: the forensic evidence artifact (14447930604) shows `ProgramData/HeicFast/Recovery Wizard/logs/install.log` containing ONLY the `onInit` line (no `app-setup exit=` fact — the section aborted before the inner install), a 4.2–4.6 s runtime (too fast for the inner bundle), and `facts-postinstall.json` reporting `app not installed`. v0.9.3 installed clean (exit 0) — its nsi predates the pre-flight; v0.9.4's own t15 matrix stayed green because validation-evidence builds AND installs on the same runner, so the build path exists there. The last green forensic run (42255492260, v0.8.4) predates the assert-added d8fbef64. |
| Affected files | `installer/recoverywizard.nsi` (the pre-flight gates + the kit placement `File /x cr_ui.lib "${ENGINE_KIT}${PATHSEP}*"` line) |
| Root-cause fix | **5798cf39**: the kit payload is extracted to `$PLUGINSDIR\engine-kit` (SetOutPath + File) at the top of the section, the pre-flight gates read the payload (not the build path), and the `$INSTDIR` placement copies the staged payload — build-machine independence by construction. Same gates, same abort reasons, no test or stage removed. |
| Resolution | FIXED — commit 5798cf39; green proof: the v0.9.5 release run on tag (fresh compile of the fixed nsi) + a forensic-repro dispatch installing the shipped v0.9.5 setup.exe on a fresh runner (recorded in worklog.md when green). |

### 12. The 0.9.5-session self-inflicted reds (recorded truthfully)

| Field | Value |
|---|---|
| Runs | Every push to main between 201100aa and 241e8991 produced zero-job "failure" records for release/benchmark-src/validation-evidence (e.g. 44348177249, 44348179096, 44348174740, 44357202970) |
| Root cause | The session's own workflow edits: (a) `${{ runner.temp }}` used in a WORKFLOW-LEVEL `env:` (the runner context is not available there) → invalid-file, zero-job push failures; (b) a duplicated `timeout-minutes` key → the same class. The removal patch's regex failed silently (backslash-escaping), so the blocks survived two "fix" commits until cc201bff deleted them outright. |
| Resolution | FIXED — cc201bff (concurrent lane, env blocks removed at root) + 6a6e05cd (timeout dedup, verified per-job single key across all 9 files); v0.9.5 tag re-pointed at the clean head. The lesson is recorded here so the audit trail shows it: the two "fix" commits 235d3c32/6a6e05cd claimed the removal but the first did not verify the block was actually gone — every subsequent claim in this repo now requires the grep-verified absence in the commit message. |

## Resolution update (2026-11-26, fix-upgrade-release session)

- **F-01 closed-loop: RESOLVED.** Root cause = the verifier itself
  (d65a5d12: deleted-DIRECTORY rows wrongly hit the I2 file-exactness
  branch). Proven by named-feature-fixture [45811533301](https://github.com/heicfast/final-prod/actions/runs/45811533301)
  (d65a5d12, VERDICT: PASS, exact_rows 21→17) plus engine-ci 45814007501
  and bridge-integration 45814007296 green at HEAD 126ea398.
- **Current-truth conversion dispatched** (this session, ~12:05Z):
  benchmark-src smoke @ main(126ea398); forensic-repro @ v0.9.6. A
  sibling concurrent session then re-dispatched both lanes from its own
  head cd439313 (12:16Z): benchmark-src [45818119464](https://github.com/heicfast/final-prod/actions/runs/45818119464)
  still in flight (201-min class run); forensic-repro [45818123462](https://github.com/heicfast/final-prod/actions/runs/45818123462)
  **completed FAILURE** — job "Reproduce all reported symptoms (installed
  release)", step 12 "Launch app + probes + drive the real UI - health,
  scan, every surface (S2 S3 S4)" exit 1.
  Exact gate error captured from the run log:
  > FATAL: Error: gate: D-SCAN-01 - the scan froze for 825 s (percent + found-count + sector all identical; the BF-22 stall class)
  The UI-drive gate therefore STILL fails on the current tree — OPEN
  defect (root cause not yet diagnosed; coordinate with the sibling
  session's worklog/DEFECT-LEDGER.md to avoid double-fixing).
