# CORE-GAP-LIST — final consolidation register (production-complete gate)

> **Purpose.** Single authoritative list of every remaining delta between
> `core_recovery` HEAD and the reference architecture (RE corpus
> `easeus-drw-research` + `AUDIT-*` / `PARITY-*` findings), as verified
> **directly against the code at HEAD `dc53e46`** (not inherited from prior
> worklog claims — several were stale and are marked below). Every row carries
> the verification evidence and the RE/reference citation where applicable.
> Statuses: MATCH / MISSING / WRONG / PARTIAL / STALE-CLAIM (prior register
> said open, code says landed).
>
> **Rule.** A row is only closed by a commit that cites its GAP id and adds
> a regression test (or doc-diff evidence for doc-only rows). No row is
> closed by re-classification without evidence.
>
> **Session-5 parity-refactor closures (2026-10-23, commits cite the ids):**
> GAP-W4 (0c6a1f5), GAP-W3-9a/9b (cd4bb41), GAP-W3-10 (0c6a1f5 + 34728b2 +
> 800e80a), GAP-W5-DELTAS/REGISTRY (046889d), GAP-CAPMATRIX (046889d),
> GAP-W5-BENCHREPORT (e9e1176), GAP-W5-ERRATA (easeus-drw-research 03ad46c),
> GAP-W6-WIN7 (74f42c0 - CI proof pending on the windows lane's next run).
> Closed by the parallel S5-CONT-2 wave: GAP-CI-1 (0a28a23), GAP-P1-2/P1-3
> (632edc4), GAP-P1-4/GAP-W2-9/GAP-W2-19 (3267d26).

## 0. Source registers consolidated

PARITY-AUDIT.md (auditor gate), AUDIT-ARCH/FALLBACKS/NTFS/FAT/LINUX-FS/
PIPELINE.md (principal-engineer audits), PARITY-REFACTOR-PLAN.md (W1..W6),
UI-READINESS-PLAN.md (G-01..G-23, issues #1–#10), worklog S5-CONT-1/S5-CONT-2
remainders, CI state at HEAD (runs 39984781013 / 39984822801).

## 1. CI (blocking — main is red)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-CI-1 | **WRONG** | Windows lane: 3/204 tests fail (`PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick`, `MaskIntegrityViolationRefusesResume`, `SessionRestoresVolumeInfoAndCandidates`) — `tmp_session_path()` hardcodes `/tmp/...`; MSVC `fopen` resolves that to `<current-drive>\tmp\` which does not exist on runners. Linux passes. | run 39984822801 job 120521156342 logs; `tests/unit/test_pause_resume.cpp:167-179` |
| GAP-CI-2 | PARTIAL | fuzz + codeql lanes in flight at HEAD; must be watched to completion and any failure fixed. | run 39984822801 |

## 2. P1 correctness (recovery-output-affecting)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-P1-1 | **MISSING** | NTFS compressed $DATA: `ATTR_FLAG_COMPRESSED` (cr_ntfs_wire.h:102) and `compression_unit` (NonresidentHeader +0x22) are parsed but **no engine path sets an extent codec** — LZNT1 (native compression) and XPRESS/LZX (compact.exe) streams are surfaced as raw compressed bytes → silent corruption on recovery. Writer already has the per-extent decode hook (`cr_safewriter.cpp:278-297` calls `compress::decompress_block`). | code grep: zero `kLzjb`-style codec tagging in `src/core/ntfs/`; `include/cr/compress/cr_decomp.h` has no LZNT1/XPRESS codecs; corpus: NTFSSearchDLL decompress path (NTFS compression engine, `RtlDecompressBuffer` family semantics) |
| GAP-P1-2 | **CLOSED (632edc4)** | lzjb port lacks the source bound: `lzjb_decompress` (cr_decomp.cpp) never checks `s` against `src+psize`; corrupt blocks over-read the source (control byte, literals, 2-byte match reads all unbounded). ASan/fuzz hazard on damaged disks — a recovery product must hard-bound every parse. | `src/core/compress/cr_decomp.cpp` lzjb_decompress body |
| GAP-P1-3 | **CLOSED (632edc4)** | Recycle $R remap is single-pass-ordered: `$R` records whose `$I` has a higher MFT record number are emitted under the raw `$R<rand>` name (map lookup at `cr_mftwalk.cpp:557-566` only sees earlier records). S5-CONT-2 bench on a real volume: **ours 0/26 vs og 26/26**. Og surfaces recycle deletions under original names. | `src/core/ntfs/cr_mftwalk.cpp:459-476` (capture) `:557-566` (consume); corpus PartitionRecoveryD/NTFS recycle mapping (UI-MAP recycle row) |
| GAP-P1-4 | **CLOSED (3267d26)** | Writer `salvaged` outcome (W2-18): zero-filled / hash-less items are reported `Done`; og distinguishes salvage-with-holes from clean recovery. Needs per-item `had_unreadable` flag → report `salvaged`. | PARITY-REFACTOR-PLAN W2-18 (FB-R02); `cr_safewriter.cpp:292-324` zero-fill path has no flag |
| GAP-P1-5 | MISSING | Transport elevation probe: raw-device open failures print guesses (`"(admin?)"` — rrecov.cpp:895,1021,1233,1319) instead of probing the process token. Og elevates via manifest/UAC (service-driven design). A definitive probe turns a guess into a diagnosis. | rrecov.cpp error strings; corpus architecture/02 (service elevation model) |

## 3. Refactor-wave leftovers (verified against code)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-W2-9 | **CLOSED (3267d26)** | FAT cancel polling: `src/core/fat/cr_chain.cpp` has zero cancel references; chain runs and dentry walks are not cancellable mid-pass (≤128-dentry latency contract). | grep: 0 matches in cr_chain.cpp / cr_fat_quick.cpp loop heads |
| GAP-W2-19 | **CLOSED (3267d26)** | GPT backup-salvage: works only when the **primary header parses** (`last_lba = prim.backup_lba`; cr_table_parse.cpp:206-243). Primary-damaged + protective-MBR path never derives last LBA from device size; backup-salvage loop also skips the type-GUID→FsKind mapping (all `FsKind::Other`). | `cr_table_parse.cpp:201-267` (verified line-by-line this session) |
| GAP-W3-4 | STALE-CLAIM | CLI Ctrl-C: **landed** in 0f88fd9 (`SetConsoleCtrlHandler` at rrecov.cpp:1352-1355, cooperative cancel of scan/recover/health). S5-CONT-2 listed it open. | rrecov.cpp:100,1352-1355 |
| GAP-W3-9a | **CLOSED (cd4bb41)** | SESS-2 identity card: `VolumeInfo` has no device identity (model/serial/label/boot-sector hash); restore side cannot verify the session is being resumed against the same volume. | `src/core/session/cr_records.cpp:296+` field list |
| GAP-W3-9b | **CLOSED (cd4bb41)** | SESS-4 compress-everything: `try_deflate = take >= 64` carve still present (cr_crsf.cpp:141); reference compresses every chunk, storing only when not smaller (the not-smaller rule is already implemented at :29). | `src/core/session/cr_crsf.cpp:20-36,140-141` |
| GAP-W3-10 | **CLOSED (0c6a1f5 + 34728b2 + 800e80a)** | Dedup wave: no `cr_bits.h`; 4 OVERLAPPED loops in cr_transport.cpp; cr_bootstrap triple attribute-walker; run_engine_pass/scan_candidate dispatch dup; byte-reader + time-conversion duplication. | grep counts this session |
| GAP-W4 | **CLOSED (0c6a1f5)** | Dead-code sweep per AUDIT-NTFS §4 (items 1-18+21), AUDIT-FAT §6 (1-20), AUDIT-LINUX-FS §6 (DC-1..13), AUDIT-PIPELINE §9 (1-9), ARCH register (`win::open_disk_view`, `status_text`, `trim_*`, `failed_bytes()`, `inner()`, `index_node` wrapper, unreachable cached branch, `round_up`, `Out.deleted`, `temp_data.txt`, `quick_done_` u32→bool, raw hex error literals, fs_kind literal style). Every deletion listed in the commit body; D-44 delta appendix entry. | AUDIT-*.md registers |
| GAP-CARVED-COUNTER | STALE-CLAIM | `carved_found` counter: **landed** (`cr_scan.cpp:875` `stats.carved_found = cs.validated`; session state v2 + CLI lines 582/631/636). S5-CONT-2 listed it open. | cr_scan.cpp:875,901; rrecov.cpp:582 |

## 4. UI-facing surface (issues #1, #2, #10)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-G-01 | **MISSING** | `cr_ui` C-ABI shim DLL: no `include/cr/cr_ui.h`, no CMake target. Full design + acceptance in UI-READINESS-PLAN §#1 (opaque handles, `cr_ui_*` u32-returning exports, ABI version, thread-local last-error, C-consumer + Windows DLL smoke + Win7 import gate). The wizard's IScanService/ISessionService/IRecoveryService wait on this. | `ls inc/cr_ui.h` → absent; UI-READINESS-PLAN.md:45-77 |
| GAP-G-02 | **MISSING** | Mid-scan immutable snapshot (`cr::tree::Snapshot`, copy-on-write value snapshot under `ResultTree::mutex_`, SNAP_PARTIAL, TSan-provable) + recovery-from-snapshot value-enqueue + ADR-UI-SNAPSHOT. | UI-READINESS-PLAN.md:78-108 |
| GAP-G-02b | MISSING | Writer value-driven enqueue (SafeWriter from snapshot rows, race-free mid-scan recovery). | UI-READINESS-PLAN.md:97-100 |
| GAP-TREE-JSON | **MISSING** | `rrecov tree --json`: scan/bench accept `--json`, tree does not. Projection via `query::run` rows; `--csv` grammar frozen byte-identical. | rrecov.cpp:654+ (cmd_tree has no --json) |
| GAP-CAPMATRIX | **CLOSED (046889d)** | `docs/CAPABILITY-MATRIX.md` does not exist (W5 requires per-capability DONE/PARTIAL/MISSING/EXCLUDED, ReFS/LDM first-class with honest remaining gaps). | `ls docs/CAPABILITY-MATRIX.md` |
| GAP-UI10-DOCS | PARTIAL | G-11 disposition + G-15..G-22 table exist inside UI-READINESS-PLAN but the README capability summary, ground-truth-contracts extension, and CORE-CONTRACT §13 sync for tree --json are not landed. | UI-READINESS-PLAN.md:290-303 |

## 5. Benchmark truth (scoring validity)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-BENCH-1 | **WRONG** | keep-gate: `keep_active_ok = keep["name_hit"]` counts hits **anywhere** (deleted duplicates included); the docstring contract is "must appear in the ACTIVE tree". `active_paths` is built but unused for the gate. | scripts/bench_score.py:106-108,133 |
| GAP-BENCH-2 | **WRONG** | precision accounting: `unmatched_recoveries -= 1` per manifest row — two manifest rows matching the same result row decrement twice (name+size collisions across scenarios are exactly what the seeded layout has); per-result matching needed. | bench_score.py:90,103-104,134-138 |
| GAP-BENCH-3 | **WRONG** | numeric enum schema: `rrecov tree --csv` emits numeric enums (Origin 0-3, DeletedState 0-3, Confidence 0-4 — cr_node.h:28-36) but bench_score.py parses words (`"deleted","true","1","yes"`) — works only by the accidental `"1"` collision; PartiallyOverwritten(2)/Damaged(3) misclassify as active. Freeze the numeric schema in the script + docs. | bench_score.py:86 vs rrecov.cpp:338-341 + cr_node.h:28-36 |
| GAP-BENCH-4 | MISSING | .rsf contamination guard: the og leg compares an .rsf that could contain entries from prior scans on the volume; guard = manifest-subset check + session-identity fields (ties GAP-W3-9a). | scripts/compare_og_rsf.py; comparison-easeus.yml |
| GAP-BENCH-5 | **WRONG** | driver service name: comparison-ours.yml installs our driver as service `rdatest`, but the transport default opens `\\.\RDA` (cr_transport.cpp:222,238,387-393) and the driver derives its device name from the service name (rda-driver rda_control.c:88-119) → the comparison leg **never exercises our driver** (silent direct-handle fallback). Fix: install under service `rda` (or explicit override) + assert the RDA path was actually used in scan JSON. | comparison-ours.yml:120-127; rda-driver/src/driver/rda_control.c:88-130 |

## 6. Docs (W5) and release (W6)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-W5-DELTAS | **CLOSED (046889d)** | PARITY-DELTAS D-36..D-44 + refresh D-06/07/13c/17, D-28/29/30, U-8/12; removed-dead-surface appendix (D-44). | docs/PARITY-DELTAS.md (no D-36+) |
| GAP-W5-CONTRACTS | PARTIAL | ground-truth-contracts §2/§3/§6 corrections (cache-coalesced reads, fixup gate wording, viability boundary, 0xFFF removal, LDM history note). | docs/ground-truth-contracts.md |
| GAP-W5-REGISTRY | PARTIAL | error-fix-registry F11 correction (og renames `name_N` cap 10000, not overwrite) + F15 reword. | docs/error-fix-registry.md:22 lines only |
| GAP-W5-BENCHREPORT | **CLOSED (e9e1176)** | `docs/BENCHMARK-REPORT.md` does not exist; must carry seeded-scenario numbers with MATCH/EXCEEDS/BELOW/NOT-COMPARABLE verdicts incl. the recycle 0/26 → 26/26 closure. | `ls docs/BENCHMARK-REPORT.md` |
| GAP-W5-ERRATA | **CLOSED (easeus-drw-research 03ad46c)** | Corpus errata feed: 0xFFF misread, FUN_180041970 mislabel, FAT12 BAD raw-follow, ext4 backup-SB overreach, FileFilter 1,481 recount. | PARITY-REFACTOR-PLAN §3 W5 |
| GAP-W5-TODO | PARTIAL | TODO/FIXME sweep: PARITY-REFACTOR-PLAN.md (1) + UI-READINESS-PLAN.md (1) carry TODOs; code is clean. Verified: `grep -c TODO docs/*` | grep this session |
| GAP-W6-WIN7 | **CLOSED (74f42c0; CI proof on next windows run)** | No `_WIN32_WINNT`/`NTDDI` pin in any CMake target (F-D-4); no user-mode import allowlist gate in CI (driver repo has stamp_osversion.py + verify_pe.py to port). | grep CMakeLists: 0 matches |
| GAP-W6-RELEASE | MISSING | Release tag (v0.3.0-core) with cr_ui.dll, static lib, rrecov.exe, headers, SHA256SUMS, notes referencing this list + PARITY-DELTAS + BENCHMARK-REPORT. | no tags beyond v0.2.x |

## 7. Closure protocol

1. CI first: GAP-CI-1 unblocks main; every later push keeps all lanes green.
2. P1 correctness wave (GAP-P1-1..5) — recovery-output bugs before surface work.
3. W2/W3 leftovers (GAP-W2-9, GAP-W2-19, GAP-W3-9a/b, GAP-W3-10).
4. W4 dead-code sweep.
5. G-01 shim + G-02 snapshot + tree --json (issue #1/#2/#10).
6. Benchmark truth wave (GAP-BENCH-1..5) + BENCHMARK-REPORT.
7. Docs wave (W5) + Win7 gate + release (W6).
