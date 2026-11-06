# COORDINATION — the two-lane seam

Two sessions are working this repository concurrently on the user's
debug-and-release mandate. This file is the mechanical contract between
them. Both lanes: fetch-first before every push, small commits, never
force-push, never rewrite main, leave main green.

## Identity

- **Lane A** — `heicfast-release <release@heicfast.local>`. The session
  that opened with T1 at c8aef05a. Owns the product/shell surface.
- **Lane B** — `Session Agent <agent@session.local>`. Opened with the
  T1 coordination note at 108578a6. Owns the engine seam.

## File ownership (the seam — do not cross without announcing here first)

| Territory | Owner |
|---|---|
| `engine/**` | Lane B |
| `ui/src-tauri/src/bridge/**`, `ui/src-tauri/src/commands/{scan,devices,health,sessions,results,recovery,repair,lost_partition,preview}.rs` | Lane B (the ABI-facing wiring) |
| `contracts/**` | Lane B (ABI changes move in lockstep) |
| `ui/src/**` (frontend) | Lane A |
| `ui/src-tauri/{build.rs,tauri.conf.json,Cargo.toml}`, `ui/src-tauri/src/{lib.rs,main.rs,commands/system.rs,commands/mod.rs}` | Lane A |
| `installer/**`, `docs/**`, `tools/**`, `.github/**`, `worklog/**` (canonical sheets + this file) | Lane A |
| `worklog.md` | Shared — append-only entries, prefix every entry with the lane |

## Defect ownership (from the merged register in DIAGNOSIS.md)

- Lane B: D-CONS-01 (_popen → no-subprocess TRIM query), D-SCAN-01
  engine-side (D4a live bytes telemetry, D4b bytes_found ABI append,
  D4c intra-window carve checkpoints, D4d throughput verification),
  D-ENUM-01 engine-side (never drop a present disk interface), E-01
  (drive-strings buffer).
- Lane A: D-PRIV-01 (requireAdministrator manifest + banner retirement
  + docs truth + forensic twin redesign), D-ENUM-01 UI-side (device
  list reference parity), D-UI-01 (banner theming), W-01..W-03, U-01..U-04,
  probe re-vehicling (new-posture gates), CI sweep, release orchestration.

## Release protocol

Lane A cuts the release tag ONLY when both lanes' fixes are green on
main. If lane B has not landed the engine fixes by the time lane A's
surface is complete and verified, lane A announces the takeover here
(in a dated entry) and implements the engine-side fixes itself so the
session still delivers — the user's mandate outranks lane symmetry.

## Phase 1 (reproduction)

Lane A dispatches ONE forensic-repro run on the shipped v0.8.4 tag and
curates `worklog/REPRODUCTION/`. Lane B does not duplicate the dispatch;
it reads the artifacts from the shared run.

## Sheet provenance

The canonical Phase 0 sheets in `worklog/` are lane A's set; lane B's
set is preserved verbatim under `worklog/lane-b/` (identical defect
register, complementary evidence — lane B's unique findings are folded
into the canonical DIAGNOSIS.md with credit).
