# modules/cli.md — rrecov CLI audit verdicts

Reference: the og has NO CLI (DRW.exe GUI launcher; WinPE runs DRW.exe via auto.bat).
The CLI is therefore a core-specific surface judged on its own contract + the safety
posture; og-parity applies only where the CLI exposes engine behavior already audited
in engine modules.

Core (audited `67193b6`): `src/app/rrecov.cpp` (1,498 LOC).

## Verdicts

- **C24 exit codes** — own contract, `M`-as-documented (N-020 drift corrected to the
  rrecov.cpp:17-19 docstring): scan 0 ok / 1 failure / 3 cancelled (session saved when
  --out was given); list 1 on enumeration failure; health 3 when every health source is
  unavailable; recover 2 partial (item errors / hash mismatch).
- **Commands** — `M`-as-designed (N-020 drift corrected): wmain dispatches exactly
  7 commands - list, scan, tree, recover, health, fix, bench. verify and query are
  API-only surfaces (`cr_ui_scan_verify_identity`, `cr_ui_query_run`); `tree --json`
  uses `query::run` internally but accepts no user query predicates. The `--json` tree
  output and recovery report are CI-validated.
- **F62/F63** — `M`: destination overlap refusal surfaced; repair consent = explicit
  command.
- **B10** — **N-009c** (--filter ASCII-only mangling).
- **Q69** — `M`: zero stub markers; identity verify (SESS-2) on resume.
- **Q65** — `M`-note: no unit tests for the CLI itself (bench scripts smoke it); the
  command surface is thin over tested engine calls — acceptable, noted as test debt.

## Findings index

| ID | Severity | One-line |
|---|---|---|
| N-009c | MINOR | --filter widen/narrow ASCII-only mangles non-ASCII |

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **Q68 — N-020 (MINOR):** documentation drift. The cli.md file itself has
  two stale claims:
  1. Line 14 lists "verify" and "query" as commands. `rrecov.cpp:1459-1499`
     wmain dispatches only 7 commands: list, scan, tree, recover, health,
     fix, bench. `verify` and `query` are API-only (`cr_ui_scan_verify_identity`,
     `cr_ui_query_run`). The CLI's `tree --json` uses `query::run` internally
     but doesn't accept user query predicates.
  2. Lines 12-13 conflate exit codes: "0 success / 1 usage / 2 scan error /
     3 recover error" — the actual codes per `rrecov.cpp:17-19` docstring
     are "scan: 0 ok / 1 failure / 3 cancelled; list: 1 on enumeration
     failure; health: 3 when all sources unavailable; recover: 2 partial".

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-020 (partial) | MINOR | **FIXED** (moved to findings/FIX/ session 22): cli.md commands list + exit codes corrected — the two rows at the top of this file carry the corrected text; verify/query marked API-only |
## Session-2 update (2026-10-25, core `e93decf`)

- **N-009c group gains a neighbor:** A-004's sink mismatch is reachable from any embedder
  that wires events post-construction the way the CLI/ABI docs describe — the CLI's own shim
  sets both sinks and is unaffected; CLI verdicts unchanged otherwise.
- RawFixer remains manual-CLI only (`rrecov.cpp:1388/1431`) — S-008's pre-scan wiring gap is
  CLI-visible (no `--auto-fix` story yet); note for the S-008 fix plan.
