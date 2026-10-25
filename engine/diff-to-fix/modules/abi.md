# modules/abi.md — cr_ui C ABI audit verdicts

Reference: og's equivalent surfaces — the plugin contract (EUCreate<FS>Searcher/
QuickSearcher factories, RAWSearcher extras incl. EUGetAnalyserList/Map/Multimap +
GetRawUserInfoMap), MOM module system (GetModule/CreateObject), IObject refcounting,
.rsf DRWUI-side deserializer (CRefsResidentDataRunList re-materialized BY NAME — the
only per-FS runlist class the og's loader re-creates by name).

Core (audited `67193b6` + GAP-S6-7 append in tree): `src/ui/cr_ui.cpp` (2,265 LOC, 57 C11
exports, ABI_VERSION 1, zero-init safe, thread-local last-error) +
`tests/ui_abi_check.c` + `test_cr_ui.cpp` + `docs/ABI-CONTRACT.md`.

## Verdicts

- **P42 surface shape** — `M`-equivalent: scan/recover/session/preview/telemetry/
  recovery-stats operation groups cover the og's engine-driving surface; missing
  operation group = disk map (**S-009**). The og's analyser-list/map introspection
  exports (query the carve registry) have no core counterpart — minor (carve registry
  is queryable internally; expose if a UI needs the format list).
- **P43 ownership/lifetimes** — `M`: refcounted handles with documented ownership
  table (ABI-CONTRACT §conformance); og IObject AddRef/Release discipline equivalent.
- **P44 async model** — `M`: single-flight monotonic progress + cancel tokens; the og's
  event-bus/polling split is an implementation difference with equivalent guarantees.
- **P45 ABI versioning** — `M` (exceeds): ABI_VERSION + since-1.1 appended-field
  policy (GAP-S6-7 recovery_stats append is the worked example); the og has no explicit
  ABI versioning (COM-style implicit).
- **P46 documentation** — `M`: ABI-CONTRACT.md is normative and current (written by the
  concurrent session; spot-checked against code).
- **C22/C23** — `M`-note: the og's IOCTL contract (0x60002400/04/08, PID-token) is a
  driver-layer contract the core does not need (direct handles; see I-005.4/5).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| S-009 | SEVERE | Disk-map operation group absent (ABI-CONTRACT §7 to-build) |
