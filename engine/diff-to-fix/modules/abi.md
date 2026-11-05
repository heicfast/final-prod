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

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new abi-specific findings in session 2. The session-1 verdicts
stand. Session 2's parallel agents re-verified the architecture parameters
(R54-R58) and safety parameters (F60-F64) cross-cuttingly; no new
abi-specific divergences surfaced.
## Session-2 update (2026-10-25, core `e93decf`)

- **S-009 remainder lands here:** the disk-map ABI (`cr_ui_disk_map`, geometry + 4 overlays)
  exists and follows ABI-CONTRACT §7's shape, but §7's "state 2 free" no longer matches the
  ABI enum (`cr_ui.h:474-75`) and §"3 bad" semantics conflate FS-marked bad with retry-layer
  ranges — doc must be reconciled when the engine-bitmap wiring lands.
- **A-004 is an ABI-contract finding:** `set_event_sink` post-construction subscription is
  violated for ErrorRange events — the ABI surface shape (P42/R56) is correct, the delivery
  is not.
- N-011e: og error facility codes (0x2980005 family) vs core `cr::Err` — a consumer-facing
  mapping table belongs in ABI-CONTRACT (registration item).

---

## Session-18 update (2026-10-26, DFX18 — N-040 closed)

**N-040 (ABI/doc truthfulness, five defects) — FIXED** (four sub-defects
landed via parallel lanes and were verified this session; D4 closed here):

- **D1 SQLite sidecar** — ALREADY FIXED by the parallel §8 rewrite
  (REJECTED-BY-REFERENCE, closed GAP-S6-12; corpus rsf-format--scan-
  session.md §2-§5). Verified: zero sqlite/crsidx references in src/
  (the only "sqlite" strings are the carve format row and the filefilter
  category); the :114 save-flow claim is gone. Pinned forward by
  `CrUi.SessionSaveWritesCrsfOnly` (exactly one file next to a saved
  session — no .crsidx, ever).
- **D2 disk-map "2 free"** — ALREADY FIXED by S-009's landing (the ABI
  documents 0/1/2/3 with "2 free = engine bitmap truth"; the state is
  produced by storage::FreeMap and pinned by the DiskMap.* overlay
  tests).
- **D3 event-thread contract** — ALREADY FIXED in the doc (§6 line
  "Fired on engine worker threads or the thread running `run()`");
  pinned forward by `CrUi.EventThreadContractPinned` (doc-truth +
  header lockstep + deterministic serial-mode run-thread delivery +
  parallel-mode worker-delivery tolerance).
- **D4 "ABI-since 1.1/1.2" fake versions** — FIXED this session: all
  seven header annotations rewritten to the truthful era labels
  ("v1-era append with the change id; no 1.1/1.2 marker ever shipped");
  ABI-CONTRACT §9 made coherent (v2 = M-036's deliberate feature-batch
  bump; appends still require no bump; the sub-version history recorded).
  Pinned by `CrUi.AbiVersionAnnotationsAreTruthful` (fail-before RED
  executed on the stash-isolated pre-fix tree).
- **D5 `modified` unit lie** — ALREADY FIXED by M-029's landing
  (ground-truth-contracts now carries "FILETIME 100-ns since 1601" with
  the M-029 correction note; `Query.ModifiedUnitIsFiletimeEverywhere`
  is the pin — re-cited as this finding's TreeJson.ModifiedFieldIsFiletime
  spec, satisfied).

| N-075 query-signal exports | **MATCH** (cr_ui_scan_busy / cr_ui_scan_stopping — v2-era append, no bump; the og's reference-out signals as synchronous ops; D-78 registers the pull-progress convention; recovery-progress mapping documented in ABI-CONTRACT 5) | `CrUi.ScanBusyStoppingQueries`, plain-C edges in `cr_ui_abi_check` |


## Session-8 section (2026-10-28, audited `35adcb9`)

Export census re-verified: 81 exports, `CR_UI_ABI_VERSION 3`, 11 verbs. The
session's ABI-visible findings: **S-040** (`cr_ui_image_create` absent),
**M-072/M-073** (query/scan-config field appends), **M-075**
(`cr_ui_fsck_volume` absent), **M-076** (row-field/`_ex` flag append),
**M-077** (preview caps), **M-078** (`cr_ui_turn_off_bitlocker` absent),
**N-098** (the enumeration always-fresh contract sentence), **N-095** item 5
(ABI-CONTRACT §7f/§7g stale cr_ui.h cites — blocks now at 853-895/897-916).
Append-only discipline applies to every fix (§9).


## Session-8 lead-lane wave (audited `ff879f9`)
- **I-029**: DRIVER-CONTRACT's "19 EUDCPEPM codes, byte-for-byte" overstates the corpus
  grounding for 4 codes (0x800C2AC0 + the RRWR trio — zero corpus hits, grep-verified).

## Session-42 addendum (2026-10-29, audited `869da779` — the fix-wave-stale extension of N-150)

The S42 core-currency pass re-verified the API gate docs first-hand: 8 API-DELTA
rows are newly-underclaiming (features landed by M-072/M-074/M-077/N-098/M-023/
M-017/S-038/OQ-SA-1 while the rows still say absent — cr_ui.h:1026/:1028,
cr_error.h:67-69, cr_preview.h's shipped Caps defaults, cr_ui.h:476-482 all
verified landed) and 6 new defect sites exist: the phantom `CR_UI_ERR_FVE_LOCKED`
macro (cr_ui.h:328/:407/:418/:421 cite a never-#defined macro — the real name is
`ERR_FVE_LOCKED`, cr_error.h:72; the fixed cr_ui.h:916 class), the dead duplicate
`--label` handler (rrecov.cpp:1411 vs :1398, with DIVERGENT truncation semantics),
ABI-CONTRACT §7i's 32 MiB residue post-M-109's 200 MB re-pin, testing.md's
1,014-vs-1,199 census, the count sites (API.md "81", CORE-API-CURRENT "78"/"34"/"10"
vs the actual 83 exports / 42+OK codes / 12 events — CORE-API-CURRENT's own :38
banner carries the true 83), and bench/README's deleted-workflow citations. All
ride N-150's S42 addendum (the original fix plan, scope-extended); evidence: the
S42 decode pack §6.

## Closing-fix lane addendum (2026-10-29 — the INFO-first fix lane)

**S-040 FIXED** — see the finding's Resolution in findings/FIXED/S-040:
the disk-image CREATE producer landed (portable copy loop + win-layer
disposition + ABI append + CLI verb 13). The module's P42/C24/F59/B8
cells flip to MATCH; R-93 PARTIAL->PRESENT in REFERENCE-API-GAP. The
flat-only boundary is registered as D-147.
