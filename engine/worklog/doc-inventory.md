# Documentation inventory — Session-95 Phase 0 (baseline 3b203fe1)

Agent D (Task T2-D) — documentation inventory + first-pass accuracy flags. Read-only
analysis; this sheet is the only file this task writes.
Repo: `/home/z/my-project/core_recovery`, branch `main`, HEAD `3b203fe19e6016b8302d8cac8b22a48cc551bef2`
(2026-10-30, verified with `git rev-parse HEAD`). Tracked `.md` files at HEAD:
**668** (`git ls-files '*.md'`). Individual rows below: 12 root + 44 `docs/` top-level +
8 `docs/parity/` + 1 `docs/adr/` + 10 `diff-to-fix/` summary docs = **75 rows**, one
summary row for `worklog/`, and grouped rows for the `diff-to-fix/` subdirs
(modules/findings/tracked-open/evidence — 444 further files, grouped per the brief).

Supersedes the Session-84 Phase-0 sheet (committed `dae6181a`, baseline `e8dcaba8`,
622 files). **245 commits landed between the two baselines** (v0.5.x → v0.6.1, the
M-107/M-160 carve families, the S84–S94 remediation lanes, the v0.6.0/v0.6.1 release
prep). Every count and every named heavy hitter below was re-derived at `3b203fe1`.

## Method

1. **Census** — `git ls-files '*.md'` → 668 files; for every root/docs/diff-to-fix
   top-level file: `git log -1 --format='%h %ad' --date=short -- <path>` + the first
   H1/intro phrase (own words) for the scope column.
2. **Heavy-hitter verification** (the 13 named in the brief) — README vs
   CMakePresets.json + src/app/rrecov.cpp; RELEASE.md/RELEASE-NOTES.md vs the
   v0.6.1 stamps (`f621b37f`); CI-CURRENT.md vs `.github/workflows/*`; PARITY-DELTAS
   tail + repo-wide conflict-marker sweep; CAPABILITY-MATRIX rows; error-fix-registry
   F13; ground-truth probes; UI-READINESS-PLAN baseline line; BUILD-SUPPRESSIONS vs
   the pragma fence; ABI-CONTRACT / PREBUILT-ARTIFACTS / DRIVER-REQUIRED contract
   spot-checks.
3. **Repo-owned truth gate** — `python3 tools/doc_truth_gate.py` at HEAD: **GREEN**
   (exports=93, error codes=44, event types=12; PARITY-DELTAS D-numbering unique,
   158 rows; FloorVersion exactly 2 pins at v0.2.4; finding-ID uniqueness 8 active /
   215 retired / 443 ledger-resolved, zero collisions; header-contract phrases clean;
   deleted-lane citations clean; script citations resolve).
4. **Sweeps** — `git grep -E '^(<{7}|={7}|>{7})( |$)' -- '*.md'`; CR_UI_API /
   include-header / TEST-macro re-counts; `git rev-parse` resolution of ledger-cited SHAs.
5. **Flags (first pass — input for the doc-audit, not the verdict)** —
   **ACCURATE-LOOKING** (probes agree at HEAD, or a correctly pinned/dated historical
   trail), **POSSIBLY-STALE** (a specific claim drifted, or a landed surface is
   missing), **STALE** (documents a removed surface, or contradicts the live register
   without a supersession banner). Maps onto the prior sheet's MATCHES-CODE /
   SUSPECT / STALE vocabulary.

---

## A. Root documents (12 rows)

| Path | Stated scope (own words) | Last commit | Flag — reason |
|---|---|---|---|
| README.md | "User-mode Windows data-recovery stack" — overview, quick start, architecture, status, doc index | cb03b543 2026-10-29 | **POSSIBLY-STALE** — build presets (`win-x64-release`/`linux-release`), 139-format table, 9 engines, 4-lane CI and GAP-P1-1-CLOSED phrasing all verify; but :157 "worklog.md was retired 2026-10-28" is false (worklog.md's last commit is the S89 close `fed4e663`, 10-29) and :121-122 lists "ReFS container-map translation (N-036)" as a remaining gap while FIXED-LEDGER.md:222 says N-036 FIXED (see F-D4) |
| RELEASE-NOTES.md | "core_recovery release notes" | d80316dc 2026-10-29 | **ACCURATE-LOOKING** — the **v0.6.1 section IS prepared** (:16, scope = v0.6.0 + the run-27 release-blocking repairs; landed `524a786e` merging both racing sides), matching the v0.6.1 stamps at vcpkg.json:3 + CMakeLists.txt:4 (`f621b37f`); carries the honest A-F1 telemetry-residue preamble |
| SECURITY.md | "Security policy" — BitLocker legitimate-unlock-only, consent, transport boundary, Win7 floor | c4d8dcee 2026-10-29 | **POSSIBLY-STALE** — :14-18 "no attack surface against locked volumes … unlocking a locked volume is out of scope by design and by policy" is in tension with the landed M-033 user-FVEK decrypt (README:47-49), the N-064 dictionary unlock (API.md:374) and the M-078 `unlock` verb (rrecov.cpp:4051); the doc names none of them |
| CONTRIBUTING.md | "Contributing to core_recovery" — local gate, build lanes, test/gate inventory, commit style | c6fb5299 2026-10-29 | **POSSIBLY-STALE** — CI paragraph (:68-76) now correctly describes the 4-lane set, but :82 "(currently zero entries)" for BUILD-SUPPRESSIONS is stale (S-1 exists; the Session-84 repair reached BUILD-SUPPRESSIONS.md and CI-LOCAL.md but not this file) |
| AUDIT.md | "Session 5 Verification & Reconciliation Audit" | 148cf8bf 2026-10-23 | **ACCURATE-LOOKING** — dated/pinned audit trail; 9 cited landing commits verified by the prior inventory; unchanged since |
| AUDIT-ARCH.md | "Deep Architecture-Parity Audit (Refactor-A)" | d8f68083 2026-10-25 | **ACCURATE-LOOKING** — pinned trail; its Dimension-2 "core includes no OS headers → PASS(0)" is superseded by the now-registered cr_lastscan.cpp:12 exception (architecture.md:48-49) — trail preserved, not retro-edited |
| AUDIT-FALLBACKS.md | "Refactor-B fallback/degraded-path census" | ffd9dc84 2026-10-23 | **ACCURATE-LOOKING** — pinned trail (FB-T06/T07 subjects deleted by N-041, recorded) |
| AUDIT-FAT.md | "Refactor-D: FAT12/16/32 + exFAT deep-parity audit" | ffd9dc84 2026-10-23 | **ACCURATE-LOOKING** — pinned trail |
| AUDIT-LINUX-FS.md | "ext4 / HFS+ / Btrfs / ZFS / ReFS deep-parity audit" | 5de2e2b0 2026-10-29 | **ACCURATE-LOOKING** — pinned trail |
| AUDIT-NTFS.md | "Refactor-C: NTFS deep-parity audit (read-only)" | c741160f 2026-10-25 | **ACCURATE-LOOKING** — pinned trail |
| AUDIT-PIPELINE.md | "Refactor-F pipeline-parity audit" | a4bd4052 2026-10-28 | **ACCURATE-LOOKING** — pinned trail; its 139/199/62 raw-sign counts re-confirmed (cr_rawsigns_data.cpp:14 `kFormatCount = 139`) |
| worklog.md | the chronological session log (no H1; 258 `## Session` headings) | fed4e663 2026-10-29 | **POSSIBLY-STALE** — content is a faithful trail through the S89 close and its conflict markers are repaired (repo sweep clean); "stale" only relative to README's false retirement claim — the file still receives commits |

## B. docs/ (44 top-level + 8 parity + 1 adr)

### B.1 Heavy-hitter verification notes (citations)

- **README vs build/CLI**: `win-x64-release` and `linux-release` presets exist
  (CMakePresets.json; full set: base, linux-gcc, linux-release, win-x64-debug,
  win-x64-release, linux-ubsan). Quick-start verbs list/scan/tree/recover/health all
  dispatch in src/app/rrecov.cpp. Nine engine dirs under include/cr (ntfs fat exfat
  ext4 hfsplus btrfs zfs refs ldm) ✓. 139-format signature table ✓
  (src/core/carve/cr_rawsigns_data.cpp:14). 4-lane CI paragraph ✓ (checks/release/
  test-benchmark/installer.yml all exist; triggers match CI-CURRENT.md).
- **RELEASE-NOTES vs stamps**: v0.6.1 section at RELEASE-NOTES.md:16; version stamps
  at vcpkg.json:3 ("version-string": "0.6.1") and CMakeLists.txt:4 (VERSION 0.6.1),
  landed `f621b37f` 2026-10-29 22:26 ("the v0.6.1 stamps"); notes lane `524a786e`.
  The tag itself was never cut (session-coherence-prompt.md rider records this).
- **docs/RELEASE.md**: pipeline step 1 (RELEASE.md:18-19) says validate = "the FULL
  pre-merge validation suite (`ci.yml` via workflow_call: local-mirror, Linux/ASan,
  TSan, UBSan, fuzz, the Windows build + ABI lane)" — **no `ci.yml` exists; the
  2026-10-28 windows-only consolidation deleted those lanes**; release.yml:76
  actually calls `./.github/workflows/checks.yml` on windows-2022. The doc's last
  commit (8943993c, 10-27) predates the consolidation and both the v0.5.x and v0.6.x
  releases.
- **docs/CI-CURRENT.md**: all four lanes verified (names, triggers, runners, the
  workflow_call chain release→checks at checks.yml:50); the previously-stale fixture
  filename is now correct (component 11 cites tools/gen_junk_gate_fixture.py, which
  exists); FloorVersion discipline gate-confirmed (2 pins, v0.2.4).
- **docs/PARITY-DELTAS.md**: repo-wide conflict-marker sweep is CLEAN (only the
  sanctioned fenced quote in findings/FIXED/M-120-…md:26-38 remains); the S82 tail
  now registers cleanly — D-160/D-161/D-162 renumbered I-048 registers at :4707/:4722/:4741,
  D-163/D-164/D-165/D-166 through :4855+; gate reports "D-numbering unique (158
  rows)" (152 `### D-` seats + `##`/bold-form rows; the one `### D-52 ADDENDUM`
  at :4349 is the deliberate continuation form the gate's N-207 rule excludes — the
  prior sheet's "duplicate D-52" no longer reproduces).
- **docs/CAPABILITY-MATRIX.md**: GAP-P1-1 phrasing now aligns with CORE-GAP-LIST
  (row :33 "LZNT1 itself landed: GAP-P1-1 CLOSED per CORE-GAP-LIST; the XPRESS/LZX
  half is out-of-scope-as-shared, tracked here, not claimed"); CI row :21 current;
  but the **duplicated "Lost-partition discovery" row persists** (:56 and :58,
  near-verbatim) and :49 keeps "container map N-036 open" against FIXED-LEDGER:222.
- **docs/roadmap.md**: v0.2 rows still cite `benchmark.yml`/`comparison.yml` as DONE
  (deleted by the 10-28 consolidation; benchmarking.md:28-29 records the deletion);
  the doc stops at "v0.3 candidates" while v0.4.1-parity → v0.6.1 all shipped.
- **docs/error-fix-registry.md**: F13 re-worded post-M-033 — "the pre-M-033 'no
  decryption code at all' wording was stale (Session-84 corrected it)" (:19).
- **docs/ground-truth-contracts.md**: `probe_family` recognition seam present
  (src/core/orchestrator/cr_scan.cpp); anchor framing intact.
- **docs/UI-READINESS-PLAN.md**: :37 still carries "rrecov scan passes CancelToken*
  = nullptr and never saves on cancel" — the retired-phrase class (S-057/N-211
  stop-window auto-save; doc_truth_gate's header-contract-phrase check exists to keep
  this wording out of headers; the plan doc carries no supersession banner).
- **docs/BUILD-SUPPRESSIONS.md**: "Current state: exactly ONE registered entry
  (S-1 …)" (:5-7) — matches the single pragma fence at
  src/core/preview/cr_thumbnail.cpp (Session-84 corrected the old ZERO-entries banner).
- **docs/ABI-CONTRACT.md**: §6/§7/§7a/§9 structure verified against cr_ui.h symbols
  (carve_format_count/info :465-466, session_list, driver advisories) — but the
  M-078 export `cr_ui_turn_off_bitlocker` (include/cr/cr_ui.h:1653, landed `fcd3ba10`)
  is absent from the contract.
- **docs/PREBUILT-ARTIFACTS.md**: latest-release resolution rule matches
  test-benchmark.yml's consume-latest design (CI-CURRENT lane 3); asset conventions
  consistent with DRIVER-RELEASE §3.
- **docs/DRIVER-REQUIRED.md**: exit-4 refusal taxonomy matches the CI probes
  (CI-CURRENT.md component 10: `driver --json` exits 4 with "state":"absent").

### B.2 docs/ top-level (44 rows)

| Path | Stated scope (own words) | Last commit | Flag — reason |
|---|---|---|---|
| docs/ABI-CONTRACT.md | "the `cr_ui` C ABI: ownership, threading, errors, events, stability" | c4d8dcee 2026-10-29 | **POSSIBLY-STALE** — contract sections verify, but the M-078 `cr_ui_turn_off_bitlocker` export (cr_ui.h:1653, `fcd3ba10`) is undocumented |
| docs/API.md | "API Reference — core_recovery" | cb03b543 2026-10-29 | **POSSIBLY-STALE** — 93-export count correct (96 CR_UI_API lines − 3 #defines, gate-green; :101-103) and exit-5 row present (:72); but the CLI grammar (:27-51) lists 13 verbs and the 14th `unlock` (M-078, rrecov.cpp:4051) is missing, as is the turn_off export row |
| docs/API-SURFACE-AUDIT.md | "the reference's API operations vs. the core's C ABI" | fcd3ba10 2026-10-29 | **POSSIBLY-STALE** — headline 93 ✓ (gate), but :32-33 says "95 CR_UI_API lines - 3 #defines"; actual is **96** (the stale-intermediate-arithmetic class recurred; API.md carries the correct 96−3) |
| docs/BENCHMARK-REPORT.md | "core_recovery vs the og reference" | 545c838b 2026-10-23 | **ACCURATE-LOOKING** — pinned comparative report (historical) |
| docs/BUILD-SUPPRESSIONS.md | "every warning/error suppression, with rationale and scope" | 25cdae33 2026-10-29 | **ACCURATE-LOOKING** — "exactly ONE registered entry (S-1)" matches the code (:5-7; the zero-entries banner was Session-84-corrected) |
| docs/CAPABILITY-MATRIX.md | "per-capability status vs the reference" (single source of truth) | cb03b543 2026-10-29 | **POSSIBLY-STALE** — GAP-P1-1 and CI rows current, but :56/:58 duplicate "Lost-partition discovery" rows persist and :49 "container map N-036 open" contradicts FIXED-LEDGER.md:222 (see F-D4); DONE/PARTIAL rows need doc-audit verification |
| docs/CI-CURRENT.md | "the 4-lane Windows-only CI (the 2026-10-28 consolidation + the S-038 installer lane)" | c6fb5299 2026-10-29 | **ACCURATE-LOOKING** — all 4 lanes, triggers, runners and the workflow_call chain verified against .github/workflows/; fixture filename now correct |
| docs/CI-LOCAL.md | "the local validation entry point and its CI mirror" | c6fb5299 2026-10-29 | **ACCURATE-LOOKING** — "carries the ONE registered row (S-1 …)" (:45-48; Session-84-corrected); driver-only lint primitives verified comment-only |
| docs/CI-SCENARIOS.md | "the staged-scenario truth matrix" | d10ae333 2026-10-29 | **ACCURATE-LOOKING** — carries its own 2026-10-28 supersession banner pointing at benchmark-matrix.json + the test-benchmark lane |
| docs/CONNECTION-GUIDE.md | "binding, driving, and connecting devices" | 22e96576 2026-10-26 | **STALE** — :4 and :77-78 still present the **direct disk leg** (`\\.\PhysicalDriveN`, "used in WinPE (the environment gate, M-030) and when the driver is absent") as a live connection mode; the driver-only architecture removed the fallback and the WinPE gate (`e58bde10`; ABI-CONTRACT's own tail: "the driver-only architecture removed the RDA→direct fallback and the WinPE gate entirely") |
| docs/CORE-API-CURRENT.md | "the current public API surface" | c4d8dcee 2026-10-29 | **POSSIBLY-STALE** — :13 "C++ public headers 82 under include/cr/**" vs actual **85** (`git ls-files 'include/cr/**/*.h'`; unguarded by the gate, drifted 79→91→85 across waves); the 13-verb enumeration (:16-17) misses `unlock`; 93 exports / 44+OK codes / 12 events all gate-green ✓ |
| docs/CORE-DISK-ACCESS.md | "the current disk access path (v0.4.1-parity tree, HEAD 22e9657)" | c169332e 2026-10-28 | **POSSIBLY-STALE** — self-pinned pre-driver-only fallback map; kept as the evidence chain input (DRIVER-REQUIRED cites it), but "current" in the title is historical |
| docs/CORE-GAP-LIST.md | "final consolidation register (production-complete gate)" | f4e500de 2026-10-28 | **ACCURATE-LOOKING** — 42 CLOSED rows, no OPEN rows found; GAP-P1-1 CLOSED (464d8e5) verified (`lznt1_payload_decode` in src/core/compress/cr_decomp.cpp) |
| docs/DRIVER-CONTRACT.md | "RDA Driver — User-Mode Contract" | 0ff6cc92 2026-10-28 | **ACCURATE-LOOKING** — wire-major-1 / ERR_DRIVER_VERSION_MISMATCH contract consistent with RELEASE.md §mapping and cr_driver_status.h |
| docs/DRIVER-RELEASE.md | "RDA Driver — Release Process & Prebuilt Artifact Shape" | 41d88297 2026-10-28 | **ACCURATE-LOOKING** — asset-shape conventions consistent with PREBUILT-ARTIFACTS.md |
| docs/DRIVER-REQUIRED.md | "the driver-only architecture contract" | 13fe7258 2026-10-29 | **ACCURATE-LOOKING** — exit-4 refusal taxonomy matches the CI-owned probes |
| docs/DRIVER-UTILIZATION.md | "the reference's driver usage vs. the core's, per interaction" | dbbcc6bc 2026-10-28 | **ACCURATE-LOOKING** — corpus-side transcription (trail) |
| docs/FORMATS.md | "Formats Guide — what core_recovery reads and writes" | 1f9f41cf 2026-10-28 | **ACCURATE-LOOKING** — engine-family rows and 139-sign table consistent; :28 carries the N-036 "tracked" phrasing (see F-D4) |
| docs/PACKAGING.md | "the app-installer contract (the packaging milestone's design input)" | c4d8dcee 2026-10-29 | **ACCURATE-LOOKING** — installer.yml (S-038 gates) exists and matches |
| docs/PARAMETERS.md | "Parameters Reference — every tunable, with defaults and bounds" | 21e69075 2026-10-29 | **POSSIBLY-STALE** (mild) — `--no-rda` now documented REMOVED (:61-62), M-073 `--filter-folder`/M-154 `--autosave-dir`/N-178 `--pause-stdin`/archive-list/image/driver all present; only the M-078 `unlock --turn-off` verb is missing |
| docs/PARITY-AUDIT.md | "independent parity audit (gate document)" | 722cb177 2026-10-23 | **ACCURATE-LOOKING** — historical gate document (pinned) |
| docs/PARITY-DELTAS.md | "deliberate-deviations registry (AUTHORITATIVE)" | c4d8dcee 2026-10-29 | **ACCURATE-LOOKING** — conflict markers repaired (sweep clean), D-numbering gate-unique (158 rows), the S82 tail D-160..D-166 registers cleanly; the registry rule ("a delta not in this registry is a bug") intact |
| docs/PARITY-OG-CORE-INVENTORY.md | "og-codebase → core_recovery classification (Agent 3)" | 59a431d3 2026-10-29 | **ACCURATE-LOOKING** — census trail |
| docs/PARITY-REFACTOR-PLAN.md | "deep architecture parity & refactor session" | d8f68083 2026-10-25 | **ACCURATE-LOOKING** — executed plan (historical) |
| docs/PERFORMANCE-BASELINE.md | "measured numbers at session start" | 41d88297 2026-10-28 | **ACCURATE-LOOKING** — pinned measurements |
| docs/PERFORMANCE-PROFILE.md | "baseline measurements at HEAD `064fcf4` (session 6, 2026-10-25)" | d0a102ce 2026-10-28 | **ACCURATE-LOOKING** — pinned measurements |
| docs/PLAN-API-EXPANSION.md | "close the reference-documented API gaps" | d10ae333 2026-10-29 | **POSSIBLY-STALE** — carries inline landed-notes (e.g. :81 thumbnails) but the status table needs a doc-audit re-walk against the M-075/M-078-era exports |
| docs/PLAN-CI-REBUILD.md | "prebuilt-artifact CI, combined release, local-first validation" | 41d88297 2026-10-28 | **ACCURATE-LOOKING** — executed plan; CI-CURRENT.md is the live record |
| docs/PLAN-DRIVER-ONLY.md | "remove the direct-access fallback; the driver becomes a hard prerequisite" | c169332e 2026-10-28 | **ACCURATE-LOOKING** — executed (landed `e58bde10`); historical plan |
| docs/PREBUILT-ARTIFACTS.md | "the deterministic latest-release convention" | 0ff6cc92 2026-10-28 | **ACCURATE-LOOKING** — resolution rule matches the workflows' consume-latest design |
| docs/REFERENCE-API-GAP.md | "the og engine→UI operation inventory vs. our core" | 37530342 2026-10-29 | **ACCURATE-LOOKING** — corpus-side transcription |
| docs/REFERENCE-DRIVER-BEHAVIOR.md | "REFERENCE DRIVER BEHAVIOR — EaseUS Data Recovery Wizard (og corpus)" | 995ba5dc 2026-10-28 | **ACCURATE-LOOKING** — corpus transcription (trail) |
| docs/RELEASE.md | "the combined release convention and cadence" | 8943993c 2026-10-27 | **STALE** — :18-19 describes validate as "`ci.yml` via workflow_call: local-mirror, Linux/ASan, TSan, UBSan, fuzz" — those lanes were deleted by the 10-28 consolidation and `ci.yml` no longer exists (release.yml:76 calls checks.yml, windows-2022); last commit predates v0.5.x/v0.6.x entirely |
| docs/UI-READINESS-PLAN.md | "UI-Readiness Plan — closing G-01…G-13 (issues #1–#10)" | 1f08c20d 2026-10-25 | **STALE** (in part) — :37 "never saves on cancel" is the retired S-057/N-211 phrase (stop-window auto-save landed); no supersession banner on the design-baseline section |
| docs/architecture.md | "Architecture" | 25cdae33 2026-10-29 | **ACCURATE-LOOKING** — :48-49 now registers the one guarded OS-header exception (cr_lastscan.cpp:12) that broke the old absolute claim |
| docs/benchmarking.md | "Benchmarking (v0.2 harness)" | 41d88297 2026-10-28 | **ACCURATE-LOOKING** — :28-29 banner records that benchmark.yml/comparison-*.yml/comparative-bench.yml/api-benchmark.yml are DELETED |
| docs/build.md | "Build" | 444d7ba8 2026-10-26 | **ACCURATE-LOOKING** — all five cited presets exist in CMakePresets.json |
| docs/crsf-format.md | "CRSF - Core Recovery Session Format (v1)" | 8b76017c 2026-10-28 | **ACCURATE-LOOKING** — container format spec |
| docs/error-fix-registry.md | "reference design defects corrected in this rewrite" | 25cdae33 2026-10-29 | **ACCURATE-LOOKING** — F13 re-worded for M-033 (:19, Session-84 correction noted in-place) |
| docs/ground-truth-contracts.md | "Ground-Truth Contracts (anti-hallucination anchor)" | e58bde10 2026-10-27 | **ACCURATE-LOOKING** — anchor contract; probe_family seam present in src/core/orchestrator/cr_scan.cpp |
| docs/recovery-report-schema.md | "Recovery Report JSON Schema (v1)" | adb26797 2026-10-29 | **ACCURATE-LOOKING** — tools/validate_report.py exists; G-09 lane validates |
| docs/requirements.md | "Requirements — core_recovery v0.1 (locked)" | 0db53c88 2026-10-22 | **ACCURATE-LOOKING** — locked historical baseline by design |
| docs/roadmap.md | "Roadmap" | 2dd6e011 2026-10-23 | **STALE** — v0.2 rows cite benchmark.yml/comparison.yml (deleted 10-28); stops at v0.3 candidates while v0.4.1→v0.6.1 shipped; no pointer to the consolidation |
| docs/testing.md | "Testing" | adf91248 2026-10-29 | **POSSIBLY-STALE** (mild, self-annotated) — :4 claims 1,717 TEST/TEST_F macros / 72 files (Session-84 recount); actual at HEAD **1,761 / 73** (the M-107/M-160/N-231 batteries landed after) |

### B.3 docs/parity/ (8) + docs/adr/ (1) — pinned audit family

| Path | Stated scope | Last commit | Flag — reason |
|---|---|---|---|
| docs/parity/PARITY-ARCH.md | "Architecture & Structure Parity Audit (Task 2-d)" | 995ba5dc 2026-10-28 | **ACCURATE-LOOKING** — pinned trail (corrections recorded in AUDIT-ARCH, per the append-only convention) |
| docs/parity/PARITY-CORE-DOM.md | "Durable Sessions (CRSF vs .rsf), Recovery Writer, Health" | 0f88fd92 2026-10-23 | **ACCURATE-LOOKING** — pinned trail |
| docs/parity/PARITY-CORE-FS1.md | "NTFS / FAT(12/16/32) / exFAT engine audit (Task 2-a1)" | 1d5cd00e 2026-10-23 | **ACCURATE-LOOKING** — pinned trail |
| docs/parity/PARITY-CORE-FS2.md | "Filesystem Engines Audit (ReFS / ZFS / Btrfs / ext4)" | d08b6d2c 2026-10-26 | **ACCURATE-LOOKING** — pinned trail; still carries the "Date: 2025" typo + foreign working-tree pin the prior sheet noted |
| docs/parity/PARITY-CORE-REC.md | "Parity-A3 Audit: Raw Carving / Lost Partitions / LDM" | 995ba5dc 2026-10-28 | **ACCURATE-LOOKING** — pinned trail |
| docs/parity/PARITY-DELTAS-draft.md | "Deliberate-Deviations Registry (DRAFT v0.1)" | 1d5cd00e 2026-10-23 | **ACCURATE-LOOKING** — explicitly superseded by docs/PARITY-DELTAS.md |
| docs/parity/PARITY-ORCH.md | "Parity-B: scan orchestration, staged model, progress, cancel" | 6b75b40e 2026-10-28 | **ACCURATE-LOOKING** — pinned trail |
| docs/parity/PARITY-UI.md | "Pages / State / Wiring Parity Audit (Task 2-c)" | 995ba5dc 2026-10-28 | **ACCURATE-LOOKING** — pinned trail |
| docs/adr/ADR-UI-SNAPSHOT.md | "mid-scan result access via immutable value snapshots" | fc656d2d 2026-10-23 | **ACCURATE-LOOKING** — ACCEPTED; include/cr/tree/cr_snapshot.h exists |

## C. diff-to-fix/ — summary docs (10 rows) + grouped subdirs

Index class: **living index** (must track HEAD: append-newest-first boards/indexes) vs
**audit trail (preserved)** (pinned, dated, deliberately not retro-edited).

| Path | Stated scope | Last commit | Index class | Flag — reason |
|---|---|---|---|---|
| diff-to-fix/README.md | "parity audit roadmap (core_recovery vs easeus-drw-research)" | 04fb5e88 2026-10-29 | living index | **STALE** — top banner frozen at the S69 lane: :27-28 "32 active / 169 FIXED / 4 tracked-open. Next free IDs: M-155+, N-217+, I-049+, A-016+, D-150+" vs SUMMARY.md:26-37 current "5 active / 220 FIXED / 3 tracked-open … Next free IDs: S-069+, M-161+, N-235+, I-050+, A-017+, D-168+"; ~25 lanes (S70–S94) never appended — following its next-free line would re-create born-collisions |
| diff-to-fix/SUMMARY.md | "executive summary (the S94 fix lane on top; the prior lanes below, newest-first)" | fed4e663 2026-10-29 | living index | **ACCURATE-LOOKING** — S94 board current (0 ACUTE · 0 SEVERE · 3 MAJOR · 2 MINOR, all driver-repo-resident / 220 FIXED / 3 tracked-open) matches on-disk findings (3 MAJOR + 2 MINOR files; gate: 8 active incl. 3 tracked-open); minor: findings/FIXED/ holds 221 files vs the 220-FIXED board (combined-finding convention — reconciliation pass candidate) |
| diff-to-fix/SURFACE-INVENTORY.md | "the corpus-derived surface inventory (the audit's scope)" — gate document | ed14732f 2026-10-29 | audit trail (preserved) | **ACCURATE-LOOKING** — AUDITED/PARTIAL/NEVER-AUDITED statuses; corpus-side, self-consistent |
| diff-to-fix/CROSS-REF-MATRIX.md | "module × parameter verdict matrix (the S93 fix lane on top)" | aa203440 2026-10-29 | living index | **ACCURATE-LOOKING** — the old conflict block (lines 39-46 at e8dcaba8) is repaired; M-154 and M-151/N-182 rows now single-form; S93/S94 rows carry the N-230/N-232/N-233 repair records |
| diff-to-fix/FIXED-LEDGER.md | "the master index of resolved findings" | aa203440 2026-10-29 | living index | **POSSIBLY-STALE** — ID uniqueness gate-green (443 ledger-resolved), but :222's N-036 row cites fixing SHA `180011e90` which **does not resolve** (`git rev-parse --verify 180011e90^{commit}` fails) and a `findings/FIXED/N-036-*.md` path absent from the tree — the pre-rebase-phantom-SHA class its own header documents twice |
| diff-to-fix/SEVERITY-SCALE.md | "verdict definitions used by this audit" | 439bb802 2026-10-25 | audit trail (preserved) | **ACCURATE-LOOKING** — stable taxonomy (MATCH/DIVERGENCE-*/UNVERIFIED) |
| diff-to-fix/API-DELTA.md | "API INVENTORY / DELTA — core_recovery vs the research corpus (API scope)" | f392d27d 2026-10-29 | audit trail (preserved) | **ACCURATE-LOOKING** — pinned session census (79-export era at its `d1fba5d`→`025dee7` pin); historical by design |
| diff-to-fix/PARAMETERS.md | "the parity-audit parameter taxonomy" | 167dbe3e 2026-10-28 | audit trail (preserved) | **ACCURATE-LOOKING** — taxonomy pinned at session-8 `ff879f9`, declared commit-independent |
| diff-to-fix/SCAN-ENGINE-DELTA.md | "the Scan Engine Inventory / Delta Report (Agent 2, scan-engine scope)" | 89405af1 2026-10-29 | audit trail (preserved) | **ACCURATE-LOOKING** — pinned audit report with method record |
| diff-to-fix/CORPUS-ERRATA.md | "corpus errata candidates surfaced by diff-to-fix audit" | b2bc1970 2026-10-29 | audit trail (preserved) | **ACCURATE-LOOKING** — contribution list; read-only-on-corpus recorded |

Grouped subdirs (not individually inventoried, per the brief):

| Group | Files | Note |
|---|---|---|
| diff-to-fix/modules/*.md | 33 | per-module audit-verdict worksheets, last committed 10-25..10-30; uniform family; repo-wide marker sweep clean (the N-234 carve.md residue swept at `b61faf47`) |
| diff-to-fix/findings/FIXED/ | 221 | resolved finding files (purge convention: recoverable from git history; ledger is the index) |
| diff-to-fix/findings/MAJOR/ + MINOR/ | 3 + 2 | the five active findings — all driver-repo-resident per SUMMARY.md:29-33 |
| diff-to-fix/tracked-open/ | 3 | the tracked-open external steps (gate counts them with the active set: 8 total) |
| diff-to-fix/evidence/ (incl. decompiled-quotes/, executions/, *-lane-sheets/) | 212 (182 .md) | first-hand evidence captures; append-only trail |

## D. worklog/ (one summary row) + strays

| Path | Stated scope | Last commit | Flag — reason |
|---|---|---|---|
| worklog/ (146 .md + 3 run captures) | the session-prompt archive: **134 session-lineage prompt files** (session-2 … session-diff-fix-s94-prompt + the two coherence prompts), **the five Phase-0 context sheets** (build-surface / api-surface / cli-surface / doc-inventory / recent-commits — committed together at `dae6181a`) **plus the sibling sheets** (api-cli-mismatches, api-findings, doc-audit, DEV-SIGN, session-diff-fix-40-convergence), **ci-failures/** (README + 3 run captures) and **builds/** (README) | sheets 2026-10-27..29 (3ee39995, dae6181a, d80316dc, f937081e, fcb4d424; ci-failures 4c4956ef; builds 053534c9) | **POSSIBLY-STALE** (as a set) — the Phase-0 sheets are Session-84-era snapshots; the tree moved after them (M-078 `fcd3ba10`, the M-107/M-160 landings, v0.6.x), so api-surface/cli-surface/recent-commits/doc-audit are historical inputs now; the prompt files are archival records (preserved) |
| bench/README.md | "Comparative Benchmark Harness (ours vs og EaseUS DRW free)" | 68aa5b78 2026-10-29 | **ACCURATE-LOOKING** — harness doc (category B, skimmed) |
| installer/*.md (1) | installer notes | (per census) | **ACCURATE-LOOKING** — ancillary |
| tests/data/images/*.md (1) | fixture note | (per census) | **ACCURATE-LOOKING** — ancillary |

## Rollup (668 tracked .md at 3b203fe1)

| Area | .md files | Deep rows this sheet |
|---|---|---|
| root | 12 | 12 |
| docs/ (top) + docs/parity + docs/adr | 44 + 8 + 1 = 53 | 53 |
| diff-to-fix/ | 454 (10 summary + 33 modules + 226 findings + 3 tracked-open + 182 evidence) | 10 + 5 grouped |
| worklog/ | 146 (134 prompts + 10 sheets + 2 subdir READMEs) | 1 summary |
| bench / installer / tests-data | 3 | 3 |
| **Total** | **668** | **75 rows + worklog summary + groups** |

(Prior sheet: 622 at e8dcaba8. Net +46: the S70–S94 lane sheets/evidence/findings growth
minus the purge-side retirements.)

## Status-claim registry — docs the doc-audit phase must verify against code

These carry DONE / MATCH / CLOSED / PARTIAL / FIXED-class claims (the inventory flags
WHERE the claims live, not whether they hold):

1. **docs/CAPABILITY-MATRIX.md** — every DONE/PARTIAL row (evidence-cited per row;
   symbol probes passed this pass, full re-walk is the audit's job); the :56/:58
   duplicate row; the N-036 "open" phrasing.
2. **docs/CORE-GAP-LIST.md** — 42 CLOSED rows each citing a closing commit
   (GAP-P1-1/464d8e5 verified this pass; the rest un-re-derived).
3. **diff-to-fix/SUMMARY.md + CROSS-REF-MATRIX.md + FIXED-LEDGER.md** — the board
   counts and every FIXED/MATCH verdict row; the 221-files-vs-220-board delta; the
   N-036 phantom-SHA row (:222).
4. **RELEASE-NOTES.md** — v0.6.1/v0.6.0 scope claims (897 commits, M-107's
   eleven-member enumeration, "full Linux suite 1744/1744").
5. **docs/roadmap.md** — the v0.1/v0.2 DONE lists (incl. the removed CI lanes).
6. **docs/UI-READINESS-PLAN.md** — IMPLEMENTED/LANDED statuses for G-01…G-13.
7. **docs/error-fix-registry.md** — F1–F16 corrected-in-rewrite claims.
8. **AUDIT-*.md (6)** — resolution banners and RESOLVED verdicts (pinned, but cited
   as evidence by current docs).
9. **docs/API-SURFACE-AUDIT.md** — A-ABI-* EXECUTED/DONE claims + the :32 count formula.
10. **docs/PLAN-API-EXPANSION.md / PLAN-CI-REBUILD.md / PLAN-DRIVER-ONLY.md** —
    landed/executed annotations.
11. **docs/testing.md** — suite counts (1,717/72 claimed vs 1,761/73 actual).
12. **docs/CI-SCENARIOS.md** — matrix coverage statuses (superseded-bannered).
13. **docs/PARITY-DELTAS.md** — VERDICT ACCEPTED/NEEDS-WORK rows + the "single source
    of truth" rule (D-165/D-166 are the newest registrations to spot-check).

## First-pass findings (drift hotspots, ranked)

### F-D1 (STALE) — docs/RELEASE.md describes the deleted pre-consolidation validation pipeline
RELEASE.md:18-19 (validate = "`ci.yml` … local-mirror, Linux/ASan, TSan, UBSan, fuzz")
vs reality: no `ci.yml`; release.yml:76 calls checks.yml (windows-2022); the ubuntu
sanitizer/fuzz lanes are local-only since the 2026-10-28 consolidation
(CI-CURRENT.md supersession note). Last commit 8943993c 2026-10-27 — two release
generations old (v0.5.0–v0.5.2, v0.6.0, v0.6.1 all shipped after it).

### F-D2 (STALE) — diff-to-fix/README.md (the roadmap) frozen at the S69 banner
:27-28 "Board at filing: 32 active / 169 FIXED / 4 tracked-open. Next free IDs:
M-155+, N-217+, I-049+, A-016+, D-150+" vs SUMMARY.md:26-37 (5 active / 220 FIXED /
3 tracked-open; next free S-069+/M-161+/N-235+/I-050+/A-017+/D-168+). ~25 lanes
(S70–S94) never appended. Highest-risk single line in the trail: a new lane following
the README's next-free IDs would re-create the born-collision class the SA-9 renumber
protocol exists to prevent.

### F-D3 (POSSIBLY-STALE, cross-doc) — the M-078 surface is missing from the API census docs
`unlock --turn-off` (rrecov.cpp:4051; landed `fcd3ba10` 2026-10-29) is absent from
docs/API.md's CLI grammar (:27-51, 13 verbs listed; the dispatch has 14), from
docs/CORE-API-CURRENT.md's 13-verb enumeration (:16-17), and from docs/PARAMETERS.md;
`cr_ui_turn_off_bitlocker` (cr_ui.h:1653) is absent from docs/ABI-CONTRACT.md and
API.md's export tables — while the gate-held 93-export count INCLUDES it (96 CR_UI_API
lines − 3 #defines). PARITY-DELTAS D-165 (:4826) and RELEASE-NOTES (v0.6.1 section,
the turn-off doubled-quotes repair) know about M-078; the census docs raced the landing.

### F-D4 (POSSIBLY-STALE, cross-doc) — N-036 "open" vs "FIXED", with a phantom SHA
README.md:121-122, CAPABILITY-MATRIX.md:49 and FORMATS.md:28 present ReFS
container-map translation (N-036) as an open/tracked gap; FIXED-LEDGER.md:222 says
"N-036 | MINOR | | 180011e90 | findings/FIXED/N-036-…md" — that SHA does not resolve
(`git rev-parse --verify 180011e90^{commit}` → fatal) and the cited findings/FIXED
file is absent from the tree; CROSS-REF-MATRIX.md:689 says "N-036 FIXED s23 (the
container-map half…)" while its older lane row :816 says "PARTIALLY FIXED (s4)…
container-map population open". Whether the capability landed needs the code-side
verdict (src/core/refs/); the registers currently disagree.

### F-D5 (STALE) — docs/CONNECTION-GUIDE.md documents the removed direct-access leg
:4 ("device connections (driver / direct / image / encrypted)") and :77-78 (Direct
disk `\\.\PhysicalDriveN`, "used in WinPE (the environment gate, M-030) and when the
driver is absent") — the driver-only architecture removed the fallback and the WinPE
gate (`e58bde10`; stated verbatim in ABI-CONTRACT.md's tail). Last commit 22e96576
2026-10-26, one day before the driver-only landing.

### F-D6 (POSSIBLY-STALE) — API-SURFACE-AUDIT.md:32 count arithmetic
"93 exports (… 95 CR_UI_API lines - 3 #defines)" — actual CR_UI_API lines = 96
(include/cr/cr_ui.h; rg -c). The headline 93 is gate-correct; the intermediate "95"
is the stale-intermediate class that recurred once before (the "92" at e8dcaba8).
docs/API.md:101-103 carries the correct 96−3.

### F-D7 (POSSIBLY-STALE) — CORE-API-CURRENT.md:13 header count
"C++ public headers 82 under `include/cr/**`" vs actual **85**
(`git ls-files 'include/cr/**/*.h' | wc -l`). Unguarded by doc_truth_gate; the count
has drifted 79 → 91 → 85 across the dead-surface-deletion waves while the doc held 82.

### F-D8 (POSSIBLY-STALE) — CONTRIBUTING.md:82 "currently zero entries"
The last un-repaired repeat of the zero-suppressions claim (BUILD-SUPPRESSIONS.md:5-7
and CI-LOCAL.md:45-48 were Session-84-corrected to "ONE registered row (S-1)";
CONTRIBUTING.md was not).

### F-D9 (POSSIBLY-STALE) — README.md:157 "worklog.md was retired 2026-10-28"
worklog.md's last commit is `fed4e663` (the S89 session close, 2026-10-29) — the
retirement never took; the Documentation table misstates it (persists from the prior
inventory's F-D10).

### F-D10 (STALE, in part) — UI-READINESS-PLAN.md:37 retired baseline phrase
"never saves on cancel" — superseded by S-057/N-211 (stop-window auto-save); the
doc_truth_gate header-contract-phrase check keeps the phrase out of headers/rrecov.cpp
but this plan doc still carries it without a banner.

### F-D11 (STALE) — docs/roadmap.md
Cites the deleted benchmark.yml/comparison.yml lanes as v0.2 DONE rows; pre-dates
v0.4.1→v0.6.1 entirely; no consolidation pointer (persists from prior F-D9).

### F-D12 (editorial) — CAPABILITY-MATRIX.md duplicated row
The "Lost-partition discovery" row appears twice (:56, :58 — the second missing the
UDF/M-044 tail). Persists from the prior inventory's F-D12.

### F-D13 (POSSIBLY-STALE) — SECURITY.md:14-18 scope phrasing
"no attack surface against locked volumes … unlocking a locked volume is out of
scope by design and by policy" vs the landed M-033 user-FVEK path, the N-064
dictionary aid and the M-078 `unlock` verb — the policy doc should name the
consent-gated unlock surfaces it permits rather than deny the category.

### F-D14 (mild) — testing.md:4 suite-count drift
1,717 macros / 72 files (Session-84 recount) vs 1,761 / 73 at HEAD. The doc itself
warns the number moves every landing wave; the doc-audit should pin the current pair.

## Fixed since the prior inventory (e8dcaba8 → 3b203fe1) — verified repaired

- **Conflict markers repo-wide: REPAIRED.** The S82 `b2bc1970` corruption in
  PARITY-DELTAS/CROSS-REF-MATRIX/FIXED-LEDGER/I-048/worklog.md is gone; the only
  remaining marker-lines in any tracked .md are the sanctioned evidence quote inside
  findings/FIXED/M-120-…md:26-38 (deliberate, fenced). (Prior F-D1 closed.)
- **BUILD-SUPPRESSIONS.md self-contradiction: REPAIRED** (:5-7 "exactly ONE
  registered entry"; Session-84 note in-place). CI-LOCAL.md's "zero rows" echo
  repaired (:45-48). CONTRIBUTING.md's repeat is the one left (F-D8). (Prior F-D2
  mostly closed.)
- **docs/PARAMETERS.md: REPAIRED** — `--no-rda` documented as REMOVED (:61-62);
  --filter-folder/--autosave-dir/--pause-stdin/archive-list/image/driver all present.
  (Prior F-D3 closed.)
- **API.md verb-count/export arithmetic: REPAIRED** — 93 = 96−3 stated correctly
  (:101-103); exit-5 row present (:72). The grammar's missing `unlock` verb is the
  new residue (F-D3 above). (Prior F-D4 half-closed.)
- **error-fix-registry.md F13: REPAIRED** (:19). (Prior F-D6 closed.)
- **architecture.md no-OS-headers claim: REPAIRED** (:48-49 registers the guarded
  cr_lastscan.cpp:12 exception). (Prior F-D8 closed.)
- **GAP-P1-1 id-reuse tension: REPAIRED** in README:118-120 and CAPABILITY-MATRIX:33
  ("LZNT1 landed: GAP-P1-1 CLOSED per CORE-GAP-LIST; XPRESS/LZX half tracked, not
  claimed"). (Prior F-D13 closed.)
- **CI-CURRENT.md stale fixture filename: REPAIRED** (component 11 cites
  tools/gen_junk_gate_fixture.py, which exists). (Prior F-D7 closed.)
- **CONTRIBUTING.md CI-lane description: REPAIRED** (:68-76 now describes the 4-lane
  set and marks sanitizer/fuzz local-only).

## Verified-clean highlights (no action)

- `tools/doc_truth_gate.py` GREEN at HEAD: exports=93 / error codes=44 / event
  types=12; PARITY-DELTAS D-numbering unique (158 rows); FloorVersion 2 pins at
  v0.2.4 (installer.yml, release.yml); finding-ID uniqueness (8 active, 215 retired,
  443 ledger-resolved); deleted-lane citations and script citations all resolve;
  header-contract phrases clean.
- All four workflows exist and match CI-CURRENT.md's lane table (checks.yml:41
  "Checks" + workflow_call at :50; release.yml:76 calls checks.yml; test-benchmark
  and installer names/triggers as documented).
- README quick start verified: both presets exist; all cited verbs dispatch; nine
  engine families present; 139-format table pinned at cr_rawsigns_data.cpp:14.
- v0.6.1 stamps present (vcpkg.json:3, CMakeLists.txt:4, `f621b37f`) and the
  RELEASE-NOTES v0.6.1 section is prepared (RELEASE-NOTES.md:16, `524a786e`).
- GAP-P1-1 closure verified end-to-end (`lznt1_payload_decode` in
  src/core/compress/cr_decomp.cpp; CLOSED row at CORE-GAP-LIST.md:42).
- Symbol probes for the driver-only era all present: `cr_ui_driver_status`,
  `cr_ui_turn_off_bitlocker` (cr_ui.h:1653), `--bitlocker-fvek/--bitlocker-dict`,
  the `unlock` verb (rrecov.cpp:4051), tools/{doc_truth_gate,validate_report,
  gen_junk_gate_fixture,verify_pe_um}.py all exist.
- No committed conflict markers in any modules/*.md, docs/parity/*.md, or
  findings/evidence file (the M-120 fenced quote excepted).
