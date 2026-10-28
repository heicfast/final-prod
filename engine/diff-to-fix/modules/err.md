# modules/err.md — the error-code registry + the logging seam audit verdicts

Reference: the og's error surface (the DRWUI/PartitionRecovery error strings
+ the corpus's registry-key-residue findings) — the error taxonomy the og's
dialogs branch on; the logging seam (the og's CLog / module trace layer).

Core (audited `0d7b1d9`-lineage, re-verified 2026-10-29): `src/core/err/
cr_error.cpp` (109 LOC — the registry + `err_text`), `cr_errmod.cpp`
(28 LOC — the module-code mapping), `cr_log.cpp` (96 LOC — the host sink
seam + the site registry) + `include/cr/cr_errmod.h` + `include/cr/cr_log.h`.
Tests: the error taxonomy is pinned by the CLI contract suite (exit-code
mappings) + the ABI error-code census (38 distinct 0x5-codes, the
doc_truth_gate's count invariant — N-150's landing).

This row closes I-030's remaining module-doc gap (N-154 item 3): the err
module's verdicts were carried by the error-fix-registry.md doc and the
retired findings; transcribed here verdict-for-verdict.

## Verdicts

- **Error-code registry completeness** — `M` (MATCH-class after M-049):
  the session-11 header truth + `err_text` landed every corpus-carried
  code; the registry doc (`docs/error-fix-registry.md`) is the navigable
  index. The count is now CI-guarded (the doc_truth_gate count invariants).
- **Module-code mapping (cr_errmod)** — `M`: the og's module tags mirrored;
  no corpus carrier unmapped.
- **The logging seam (cr_log)** — `M` with REGISTERED DELTAS: the host-sink
  default-silent posture (N-048 FIXED: the og's CLog traces are a dev
  build's surface — the core's default-silent seam is the registered
  improvement); the memory-monitor seam (N-062 FIXED: check_memory_pressure
  + the water-mark sampling, LeakSanitizer-clean — the N-154 comment-truth
  repair re-cited the Site-leak fix to its true landing); the OOM
  diagnostics family (N-139 FIXED: the og-shaped one-line diagnostics at
  the carve seam).
- **Q46/Q68 (doc truth of the registry)** — `M`: M-049's session-24 close
  (the register complete) + N-154 item 1 (the in-source cite truth).

## Findings index

| ID | Severity | One-line | State |
|---|---|---|---|
| M-049 | MAJOR | the error registry's session-11 header truth + err_text | FIXED |
| N-048 | MINOR | the engine logging seam (host sink, default silent) | FIXED |
| N-062 | MINOR | the memory monitor on the logging seam | FIXED |
| N-139 | MINOR | the carve OOM policy diagnostics (the cr_log seam) | FIXED |
| N-188 (m119 window) | MINOR | the committed-red app-parse finding (rrecov parse) | FIXED |

## Test debt

The taxonomy's exit-code mappings are CLI-contract-tested; `err_text` itself
is asserted only for non-nullness at the ABI gate (the og's exact strings
are a registered non-goal — the og's own strings are user-facing prose, the
core's are engine diagnostics; the D-row family in PARITY-DELTAS).
