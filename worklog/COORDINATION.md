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

## Lane C — the API-surface lane (2026-11-11, post-v0.9.2 session)

A third session opens this cycle: **Lane C** — `API Surface Agent
<api-surface@heicfast.local>`. Its mandate (the user's own words): grow
the Tauri 2 API surface — more core capability exposed to the React 19
UI, better wiring, better compatibility — while the defect lane works
the reported breakages. Design sheet + evidence base:
`worklog/lane-c/API-EXPANSION.md` (the INTEGRATION-PLAN §4.7 DISABLE
rows 16-21/23/25 flip to SURFACE, each with the corpus citation that
contradicts its "no reference screen" rationale).

Lane C file ownership (additions to the seam table):

| Territory | Owner |
|---|---|
| `ui/src-tauri/src/commands/{imaging,diagnostics,maintenance}.rs` (new files) | Lane C |
| `worklog/lane-c/**` | Lane C |
| UI surfaces lane C adds (new component files under `ui/src/`) | Lane C |

Lane C seam crossings (announced here per the rule; all additive):
`commands/mod.rs` + `src-tauri/src/lib.rs` (registration lines only),
`contracts/{command-surface.json,event-mapping.md,error-codes.json,
type-maps.md}` (new rows), `ui/src/connector/**` (new domains/methods),
`ui/src/i18n/**` (new keys), `tests/connector-*.test.ts` (new cases).
Lane C does NOT touch the defect lane's active files — `engine/**`,
`bridge/{state,events,abi}.rs`, `commands/{scan,devices,health}.rs`,
and the scan/disks/health screens — this session. The two deferred
crossings (the `img-` scan-target arm in scan.rs/devices.rs, the TRIM
chip in devices.rs) wait for the defect lane's green and get their own
entry here before lane C takes them.

## Lane B process note (lane A, 2026-11-06)

Both of lane B's engine pushes (68460a4c, ce71143e) left main RED at
engine-ci's pinned clang-format gate (18.1.8). Lane A fixed both
mechanically (zero semantic change). Before any engine/ push, run the
local gate — it is the same check CI runs:

    cd engine && find src include tests \( -name '*.h' -o -name '*.cpp' -o -name '*.c' \) | xargs clang-format --dry-run -Werror
    # fix: clang-format -i <files>   (pip install clang-format==18.1.8)

Main must be green at every push — a red main blocks the release cut.
