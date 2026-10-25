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

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **F62 — N-013 (MINOR):** destination overlap check is OPTIONAL (gated on
  `have_dest_` which requires drive-letter path + disk-known source +
  non-image transport — `cr_ui.cpp:2057-2063`, `rrecov.cpp:869-873`) AND
  first-extent-only (`cr_transport.cpp:701-712` returns only
  `ext->Extents[0]` from `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`; the
  `VOLUME_DISK_EXTENTS ext[8]` buffer has room for 8 extents but only the
  first is read out). Image-source-to-same-disk and multi-extent
  spanned-volume destinations miss the check. The og ALWAYS opens `\\.\X:`
  for drive-letter destinations and uses the full multi-extent result
  (`RecoveryModule_decompiled.c:60381-60444`).
- **F60/O38/X48 — N-015 (MINOR):** writer does NOT pre-check destination
  free space. `WinDestIo::write()` (`cr_destio.cpp:24-42`) returns
  `ERR_WRITE_FAILED` on any `WriteFile` failure (including
  `ERROR_DISK_FULL = 112`); no ENOSPC special-case. The queue loop
  (`cr_safewriter.cpp:440-469`) continues to the next item after a disk-full
  failure — every subsequent item will ALSO fail with ENOSPC, pointlessly
  attempting `create_temp` + `write` + `delete` for each. The og's
  `RecoveryModule.dll` calls `GetDiskFreeSpaceExA` before writing
  (`RecoveryModule_decompiled.c:47033`).
- **N-009d refinement:** writer does NOT preserve timestamps or attributes.
  Confirmed og ALSO resets attributes to `FILE_ATTRIBUTE_NORMAL`
  (`RecoveryModule_decompiled.c:79677`) and does NOT call `SetFileTime`
  (grep empty in `RecoveryModule_decompiled.c`). Both og and core drop
  timestamps/attributes. The ACL drop IS the divergence (N-009d, already
  filed). Narrowed to: N-009d is the complete divergence; no additional
  timestamp/attribute gap. (The session-1 N-009d verdict was correct; this
  re-verification confirms the og's behavior matches the core's on
  timestamps/attributes, so the divergence is ACL-only.)

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-013 | MINOR | Destination overlap check optional (drive-letter only) + first-extent-only |
| N-015 | MINOR | No disk-full pre-check or fast-fail (wasted cycles after ENOSPC) |
| N-009d (refined) | MINOR | ACL drop is the only divergence (timestamps/attributes match og) |
