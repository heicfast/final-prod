# modules/writer.md — recovery writer + report audit verdicts

Reference: og ExportThread/recovery flow per `architecture/04-recovery-workflow.md` §7.3–7.4
(destination validation via `\\.\X:` raw open + same-volume detection, 0x5c→0x2f
normalization, per-file error collection "one unreadable file must not cancel the
remaining ten thousand" — WRITER-1 parity), ReportSchema contract.

Core (audited `67193b6` + concurrent GAP-S6-7 in tree): `src/core/recovery/` (779 LOC) +
`tests/unit/test_writer_dirs.cpp` + `test_report.cpp` + `tools/validate_report.py`.

## Verdicts

- **B2 recovery correctness** — `M`: resident-prefix + extents streaming; compressed
  extents decode with never-wrong-bytes policy (zero-fill + Salvaged flag); SHA-256
  always captured (G-09), verify against sha128 with mismatch-still-commit salvage.
- **F62 destination overlap refusal** — `M` (hard refusal on any intersecting extent,
  R7.2/F12; directories never refused G-08). Og equivalent: same-volume detection at
  destination validation.
- **D13 report schema** — `M`: schema_version 1, totals {queued, done, dirs, refused,
  errors, hash_mismatch, resumed, salvaged, pending, bytes} + per-item outcomes
  (done/refused/error/hash_mismatch/resumed/pending/salvaged) — the og has no JSON
  report (GUI funnel); the core's schema is its own contract, validated by CI
  (`validate_report.py`). GAP-S6-7 (concurrent) added the NothingToRecover skip policy
  (metadata-only items never zero-fill) + schema/ABI append — verified in tree.
- **Name sanitization** — `M`: `/ * : ? " < > |` → `_` (the og's volume-name sanitizer
  set — same rule family applied to recovered filenames).
- **Collision policy** — `M`-note: Rename suffix " (n)" before last dot; Skip/Overwrite
  idempotent. Og collision policy not decoded in corpus (UI-driven) — no contradiction.
- **O38 progress** — **N-009b** (error-item bytes counted as done).
- **F64 ACLs** — **N-009d** (not preserved; og preserves on copy paths).
- **S32 cancellation** — `M`: per-chunk checkpoints, ERR_CANCELLED aborts with temp
  closed; save-on-cancel at the CLI.
- **X49 memory** — `M`-note: whole-block compressed-extent decode spikes ≤ 256 MiB per
  extent (documented cap; a windowed decoder is a follow-up, not a parity gap — og has
  no compressed-extent writer at all).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| N-009b | MINOR | Progress counts error-item bytes as done |
| N-009d | MINOR | ACLs not preserved on recovery (og preserves on copy) |
