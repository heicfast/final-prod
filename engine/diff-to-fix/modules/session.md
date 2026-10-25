# modules/session.md — session module audit verdicts (CRSF vs .rsf)

Reference: the .rsf grammar per `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md`
(+ `ifileproperties-part1/2/3`), auto-save naming/sanitizer, CORRECTIONS folded.

Core (audited `67193b6`): `src/core/session/` (721 LOC) + `include/cr/session/` +
`docs/crsf-format.md` + `tests/unit/test_session.cpp`.

## Verdicts (D12 field-equivalence — own byte format allowed)

- **Chunk framing** — `M`: core replicates the og algebra exactly — header
  `(len−1)&0xFFF | 0x8000` when deflated, payload ≤ 4 KiB, zlib level 5,
  deflate-only-if-strictly-smaller, per-chunk raw fallback on inflate failure
  (`cr_crsf.cpp:20–62` vs rsf-format §3).
- **Record inventory** — `M` at field-equivalence: og record-2 (volume name[100],
  deviceType, SYSTEMTIME, stat1/stat2, flags) ≈ core `volume_info` (fs_kind, slice
  offset/bytes, sectors, label[32], scanned/elapsed, flags + v2 identity tail with
  model/serial/boot_hash — the og's identity role is served by the boot-sector copy in
  record-3; core's CRC-of-boot u32 + serial/model is an equivalent-strength identity
  card, SESS-2 verified at resume); og record-3 (partition struct + 5400B volume blob)
  ≈ core `partition_context` (+ slice placement for F-1 rebasing — richer); og record-1
  (12B state) ≈ core `session_state` (stage_done_mask, counters, error-map ranges —
  richer and resume-critical); og record-4 tree ≈ core tree records.
- **Tree grammar** — `M`-note: og persists per-FS IFileProperties field orders (ReFS
  u32 anchor, ZFS name-len-first, CRAW unconditional runlist, Btrfs +0xB5 switch,
  HFS+ named fork class); core persists a normalized node + codec/psize-tagged extents
  + resident_data blobs (v2 flags). Field-equivalent (every og field has a core
  counterpart; core adds codec/psize which the og's runlist hex form lacks — superset
  for compressed streams). The og's GetType()==0xB never-saved rule ≈ core's
  role-tagged roots (G-07 inverse: core saves roots explicitly — documented
  improvement for resume role recovery).
- **Deleted flag / FLAGS semantics** — `M`: og FLAGS 0x20=deleted / 0x10=has-children;
  core carries DeletedState (richer: Active/Deleted/PartiallyOverwritten/Damaged) +
  node flags — equivalence at the deleted bit, superset beyond.
- **Progress semantics in-session** — `M`-note: og record-4 embeds the save/restore
  progress formulas (0x28 + processed×0x32/(total×2), restore 40+50·done/(total+done));
  core's session_state carries stage bits + counters and the orchestrator computes
  progress from live telemetry — the resume-progress UX differs mechanically (core
  recomputes; og replays stored fractions). No user-visible gap identified beyond the
  visualization surface (S-009).
- **Auto-save naming** — `NA` for the engine (CLI/UI-level; core's save-on-cancel uses
  its own naming; the og's `auto_<volume>_<ts>.rsf` + `/ * : ? " < > |` sanitizer set is
  replicated in the writer's sanitize_name for recovered filenames — verified `M` there).
- **S36 lifecycle** — `M`: save/load/resume with identity verification, mask-⊆-configured
  integrity check, torn-tail clamp, skip-forward resync, legacy fs_kind-12 remap;
  configured-mask preservation (BENCH-F2, G-07).
- **Hardening beyond og** — `I` (grouped I-005.7): 64 MiB record bomb cap (P1-7),
  torn-tail salvage (BENCH-F1), UNIT-step resync (F10).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| — | — | No module-specific findings; hardening rows in I-005 |

## Test gaps (test debt, no finding)

64 MiB bomb-cap path, 65536-record guard, legacy remap, "Restored" synthetic root path.

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **S34/D12/S36 — N-014 (MINOR):** stage checkpoint is NOT atomic.
  `cr_ui.cpp:1131` opens via `_wfopen_s(&f, w.c_str(), L"wb")` which
  TRUNCATES the existing file to zero on open. `emit_checkpoint_()`
  (`cr_scan.cpp:418-433`) does `seek(0)` then writes a fresh container
  header + records + finalize. `Writer::finalize()` (`cr_crsf.cpp:172-200`)
  patches `record_count` and per-record `chain`/`crc` fields IN PLACE via
  seek+write; no `.tmp` + atomic rename. If the process is killed between
  `fopen("wb")` (truncate) and `finalize()` completing, the checkpoint file
  is torn. The previous good checkpoint is GONE (truncated). The og's
  pattern writes each auto-save to a FRESH timestamped file
  (`auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` per
  `rsf-format--scan-session.md §5.5`), so a torn new file leaves the
  previous auto-save intact. The torn-tail salvage (`cr_crsf.cpp:240-276`)
  mitigates the worst case (recovers the indexed prefix); the atomic-rename
  fix preserves the previous good state on top.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-014 | MINOR | Stage checkpoint not atomic (truncate-then-write; previous destroyed on crash) |
## Session-2 update (2026-10-25, core `e93decf`)

- **N-016b filed:** core `.crsf` stores the U+FFFD-substituted name where the og `.rsf` keeps
  the original on-disk bytes (substitution at scan vs display/restore) — add a versioned raw
  name field for true round-trips of invalid-UTF-8 names.
- **S-009 remainder:** the disk-map session record (downsampled copy, versioned) is still
  absent — resumed sessions recompute at fixed 64 KiB cells.
- **M-018 interplay:** the junk-filter policy choice determines what sessions persist (hide
  at emission vs presentation-only) — register the choice before the session format gains
  any filter-state fields.
- Session lifecycle/crash-resume verdicts unchanged (S2-F sweep re-confirmed the og's
  checkpoint cadence claims).
