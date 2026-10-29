# modules/cli.md — rrecov CLI audit verdicts

## S12 addendum (2026-10-29 — the session-2 deep-dive)

New active findings on this module: **A-013** (ACUTE — the --resume-without---out
truncate-before-read; the save-on-cancel flow prints the destructive command);
**M-131 FIXED-ALREADY (4f0fc160 — the parallel A-013 landing's CliCkSink covered the scope in full; verified at c98eb7a7)** (the checkpoint sink's truncate-in-place); **N-178** (no pause verb — the
ABI pair exists, the CLI never exposes it).


Reference: the og has NO CLI (DRW.exe GUI launcher; WinPE runs DRW.exe via auto.bat).
The CLI is therefore a core-specific surface judged on its own contract + the safety
posture; og-parity applies only where the CLI exposes engine behavior already audited
in engine modules.

Core (audited `67193b6`): `src/app/rrecov.cpp` (1,498 LOC).

## Verdicts

- **C24 exit codes** — own contract, `M`-as-documented (N-020 drift corrected to the
  rrecov.cpp docstring): scan 0 ok / 1 failure / 3 cancelled (session saved when
  --out was given); list 1 on enumeration failure; sessions 1 on enumeration failure
  (unreadable files are per-row data, never a failure); health 3 when every health
  source is unavailable; recover 2 partial (item errors / hash mismatch); apply/fix 2
  on open/plan/execute/undo failure; bench 2 on open/read failure or zero hits;
  **exit 4 = the driver prerequisite is not met (any driver-requiring verb — the
  refusal names the ERR_DRIVER_* class; docs/DRIVER-REQUIRED.md — the N-123 row,
  previously absent from this module's exit-code list)**.
- **Commands** — `M`-as-designed (N-020's 7-verb correction superseded by four verb
  waves — the N-123 recount): wmain dispatches exactly **11 commands** — list,
  list-carve-formats (N-033), sessions (W1a/R-56), scan, tree, recover, health, fix,
  apply, bench (the bench lane), driver (S-029/D-DRV-3). verify and query remain
  API-only surfaces (`cr_ui_scan_verify_identity`, `cr_ui_query_run`); `tree --json`
  uses `query::run` internally but accepts no user query predicates. The `--json` tree
  output and recovery report are CI-validated. The four locked doc sites (header
  comment, docs/API.md §1, CORE-API-CURRENT §c, the `-h` usage block — the lock grown
  to four sites by N-094) are enforced by `CrUi.CliHelpListsEveryDispatchedVerb`.
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


## Session-8 section (2026-10-28, audited `35adcb9`)

**N-094** (the `-h` usage block lists 10 verbs — `sessions` is invisible; the
three-site lock needs its fourth site; + the CORE-API-CURRENT:318 stale
dispatch cite). **M-071** (MAJOR — the M-018 regression: the junk-filter
gates were dropped from `print_tree` and the recover-all plan walk when
rrecov.cpp was reconstructed in the N-064 wave; git-proven). Verbs that would
land from this session's findings: `image` (S-040), `--filter-path-substr`/
`--state` (M-072), `--filter-folder` (M-073), `--chkdsk` (M-075), `--trim`
(M-076), `--turn-off` (M-078) — each must carry the four-site lock.


## Session-8 lead-lane wave (audited `ff879f9`)
- **N-123**: the "exactly 7 commands" row vs the 11-verb dispatch + the exit-4 row.

## Closing-fix lane addendum (2026-10-29 — the INFO-first fix lane)

**S-040 FIXED** — see the finding's Resolution in findings/FIXED/S-040:
the disk-image CREATE producer landed (portable copy loop + win-layer
disposition + ABI append + CLI verb 13). The module's P42/C24/F59/B8
cells flip to MATCH; R-93 PARTIAL->PRESENT in REFERENCE-API-GAP. The
flat-only boundary is registered as D-147.
