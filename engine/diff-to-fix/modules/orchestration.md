# modules/orchestration.md — scan orchestrator audit verdicts

Reference: og stage model per `architecture/04-recovery-workflow.md` +
`DRWUI-exe--main-ui-architecture.md` §9.2 (quick → advanced composition: all full FS
searchers + RAW gated on settings+0x60), progress semantics (status strings, "Remaining
time", Reading sector X/Y), completion state machine (bottomarea nStatus=4,
m_iScanType 3→4), thread priority 7 workers, scan-abort manual-reset event.

Core (audited `67193b6` + concurrent fixes in tree): `src/core/orchestrator/cr_scan.cpp`
(1,126 LOC) + `tests/unit/test_orchestrator.cpp` + `test_pause_resume.cpp` +
`test_parallel.cpp`.

## Verdicts

- **O37 stage model** — `M`-equivalent: quick → deep (+remnant sweep + lost discovery) →
  carve; the og's quick/advanced split maps to core's quick/deep+carve (carve is a
  separate stage in core; in the og, RAWSearcher is part of the advanced set — S-006/M-005
  cover the composition differences).
- **O38 progress** — `M`-equivalent: weighted single-division permilles, per-stage
  monotonic, pause-compensated elapsed; og's 40→65% save formula and bottomarea
  semantics are UI-session constructs (session module note); engine-level callbacks
  bridge with veto (progress-false → cancel) — matches the og's cancel cadence.
- **O39 ETA** — `M`-note: core exposes telemetry for ETA computation (elapsed, rates);
  the og's "Remaining time" formatting is UI-side. No engine gap.
- **O40 threading** — `M`: engine pass single-threaded with intra-engine parallelism
  (remnant fan-out, carve workers, candidate scans — per-worker stats, F-B-11 deleted
  merge fix); og workers at priority 7 — core does not set thread priority (NA for a
  CLI/engine library; UI layer decision).
- **O41 callback threading** — `M`: documented bridge (callbacks fire from worker
  threads via engine_progress_bridge_; cr_ui documents the lock order).
- **S31–S35 state machine** — `M`: stage bits stamped only on success; pause/resume
  truth (G-07); checkpoint per stage boundary (G-13, fresh container, 'RKPP' tag);
  cancel reaches every path incl. discovery; retry→cache pipe with live error map.
- **S36 session lifecycle** — `M` (see session module).
- **R54–R58 architecture** — `M`: single dispatch table (ARCH-DUP-4), LDM probed
  unconditionally pre-hint, layering clean (AUDIT-ARCH 0 violations re-verified by
  agent C).
- **B8 multi-volume** — `M`: LDM volumes via LdmVolumeDevice under "Dynamic Volume"
  roots; lost candidates parallel-scanned with per-candidate rebase (F-1).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-005 | MAJOR | Carve stage scans whole slice (og: bitmap-gated free-space on volume scans) |
| S-008 | SEVERE | (wiring side) pre-scan repair absent |

## Post-baseline note

Concurrent fixes landed in-window and are IN the audited tree: GAP-S6-6 (candidate
dedup containment), GAP-S6-7 (metadata-only skip), GAP-S6-8 (carve absolute-extent
dedup), GAP-S6-13 (64 KiB transport envelope).
