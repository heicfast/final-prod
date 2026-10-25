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
> GAP-W6-WIN7 (74f42c0; the gate's three designed iterations converged - rrecov 145 / cr_tests 250 / cr_ui.dll 125 imports all OK, MSVC suite 230/230, run 40018530391 at ccc1dca).
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
| GAP-CI-2 | **CLOSED** | fuzz lane: zero-crash campaigns green on every run since f93f38e (the CI-found preview short-window OOB was the last finding, fixed with regressions); codeql: green since bb6a647 (the C2039 was breaking its build, not the vcpkg env); windows: FULL green at ccc1dca (gate + 230/230 suite, run 40018530391). | runs 40013456484 / 40015445403 / 40018530391 |

## 2. P1 correctness (recovery-output-affecting)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-P1-1 | **CLOSED (464d8e5)** | NTFS compressed $DATA: `ATTR_FLAG_COMPRESSED` (cr_ntfs_wire.h:102) and `compression_unit` (NonresidentHeader +0x22) are parsed but **no engine path sets an extent codec** — LZNT1 (native compression) and XPRESS/LZX (compact.exe) streams are surfaced as raw compressed bytes → silent corruption on recovery. Writer already has the per-extent decode hook (`cr_safewriter.cpp:278-297` calls `compress::decompress_block`). | code grep: zero `kLzjb`-style codec tagging in `src/core/ntfs/`; `include/cr/compress/cr_decomp.h` has no LZNT1/XPRESS codecs; corpus: NTFSSearchDLL decompress path (NTFS compression engine, `RtlDecompressBuffer` family semantics) |
| GAP-P1-2 | **CLOSED (632edc4)** | lzjb port lacks the source bound: `lzjb_decompress` (cr_decomp.cpp) never checks `s` against `src+psize`; corrupt blocks over-read the source (control byte, literals, 2-byte match reads all unbounded). ASan/fuzz hazard on damaged disks — a recovery product must hard-bound every parse. | `src/core/compress/cr_decomp.cpp` lzjb_decompress body |
| GAP-P1-3 | **CLOSED (632edc4)** | Recycle $R remap is single-pass-ordered: `$R` records whose `$I` has a higher MFT record number are emitted under the raw `$R<rand>` name (map lookup at `cr_mftwalk.cpp:557-566` only sees earlier records). S5-CONT-2 bench on a real volume: **ours 0/26 vs og 26/26**. Og surfaces recycle deletions under original names. | `src/core/ntfs/cr_mftwalk.cpp:459-476` (capture) `:557-566` (consume); corpus PartitionRecoveryD/NTFS recycle mapping (UI-MAP recycle row) |
| GAP-P1-4 | **CLOSED (3267d26)** | Writer `salvaged` outcome (W2-18): zero-filled / hash-less items are reported `Done`; og distinguishes salvage-with-holes from clean recovery. Needs per-item `had_unreadable` flag → report `salvaged`. | PARITY-REFACTOR-PLAN W2-18 (FB-R02); `cr_safewriter.cpp:292-324` zero-fill path has no flag |
| GAP-P1-5 | **CLOSED (837a543)** | Transport elevation probe: raw-device open failures print guesses (`"(admin?)"` — rrecov.cpp:895,1021,1233,1319) instead of probing the process token. Og elevates via manifest/UAC (service-driven design). A definitive probe turns a guess into a diagnosis. | rrecov.cpp error strings; corpus architecture/02 (service elevation model) |

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
| GAP-G-01 | **CLOSED (2a93c45)** | `cr_ui` C-ABI shim DLL: no `include/cr/cr_ui.h`, no CMake target. Full design + acceptance in UI-READINESS-PLAN §#1 (opaque handles, `cr_ui_*` u32-returning exports, ABI version, thread-local last-error, C-consumer + Windows DLL smoke + Win7 import gate). The wizard's IScanService/ISessionService/IRecoveryService wait on this. **CLOSED (2a93c45)**: 57-export C11 surface per UI-READINESS-PLAN #1 (cr::Err space, zero-init-safe configs, device/session/snapshot/query/preview/recovery/report/health, CLI-parity flows incl. resume + subtree + [F12] arming), cr_ui_tests (9 gtests) + cr_ui_abi_check plain-C full-lifecycle consumer; 228/228 plain+ASan green, clang-format-18 clean. *Parallel-implementation note:* S5-CONT-4 landed an independent e79d23f variant (abi_size PODs, CR_UI_CANCELLED 0x30000001 outside the cr::Err space, zero-init-invalid scan config, JSON-for-structs surfaces); resolved on rebase to the spec-conformant implementation - their MSVC consumer-flag guard + Win7-no-filesystem posture absorbed. | include/cr/cr_ui.h; src/ui/cr_ui.cpp; tests/ui_abi_check.c |
| GAP-G-02 | **CLOSED (fc656d2)** | Mid-scan immutable snapshot (`cr::tree::Snapshot`, copy-on-write value snapshot under `ResultTree::mutex_`, SNAP_PARTIAL, TSan-provable) + recovery-from-snapshot value-enqueue + ADR-UI-SNAPSHOT. | UI-READINESS-PLAN.md:78-108 |
| GAP-G-02b | **CLOSED (fc656d2)** | Writer value-driven enqueue (SafeWriter from snapshot rows, race-free mid-scan recovery). | UI-READINESS-PLAN.md:97-100 |
| GAP-TREE-JSON | **CLOSED (code-verified)** | `rrecov tree --json`: scan/bench accept `--json`, tree does not. Projection via `query::run` rows; `--csv` grammar frozen byte-identical. | rrecov.cpp:654+ (cmd_tree has no --json) |
| GAP-CAPMATRIX | **CLOSED (046889d)** | `docs/CAPABILITY-MATRIX.md` does not exist (W5 requires per-capability DONE/PARTIAL/MISSING/EXCLUDED, ReFS/LDM first-class with honest remaining gaps). | `ls docs/CAPABILITY-MATRIX.md` |
| GAP-UI10-DOCS | **CLOSED (this wave)** | README capability summary ADDED (CAPABILITY-MATRIX link + honest-gap naming + UI-surface-landed block); ground-truth-contracts §11 extension ADDED (the UI-surface contracts); roadmap v0.3 row updated (core-side UI surface landed, wizard front-end is the rda-wizard repo); CORE-CONTRACT §13 tree --json grammar synced (rda-wizard, coordinated update citing core 1005d14 + the normative §11 table). | README.md; docs/ground-truth-contracts.md §11; docs/roadmap.md; rda-wizard docs/CORE-CONTRACT.md §13 |

## 5. Benchmark truth (scoring validity)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-BENCH-1 | **CLOSED (837a543)** | keep-gate: `keep_active_ok = keep["name_hit"]` counts hits **anywhere** (deleted duplicates included); the docstring contract is "must appear in the ACTIVE tree". `active_paths` is built but unused for the gate. | scripts/bench_score.py:106-108,133 |
| GAP-BENCH-2 | **CLOSED (837a543)** | precision accounting: `unmatched_recoveries -= 1` per manifest row — two manifest rows matching the same result row decrement twice (name+size collisions across scenarios are exactly what the seeded layout has); per-result matching needed. | bench_score.py:90,103-104,134-138 |
| GAP-BENCH-3 | **CLOSED (837a543)** | numeric enum schema: `rrecov tree --csv` emits numeric enums (Origin 0-3, DeletedState 0-3, Confidence 0-4 — cr_node.h:28-36) but bench_score.py parses words (`"deleted","true","1","yes"`) — works only by the accidental `"1"` collision; PartiallyOverwritten(2)/Damaged(3) misclassify as active. Freeze the numeric schema in the script + docs. | bench_score.py:86 vs rrecov.cpp:338-341 + cr_node.h:28-36 |
| GAP-BENCH-4 | **CLOSED (837a543)** | .rsf contamination guard: the og leg compares an .rsf that could contain entries from prior scans on the volume; guard = manifest-subset check + session-identity fields (ties GAP-W3-9a). | scripts/compare_og_rsf.py; comparison-easeus.yml |
| GAP-BENCH-5 | **CLOSED (837a543)** | driver service name: comparison-ours.yml installs our driver as service `rdatest`, but the transport default opens `\\.\RDA` (cr_transport.cpp:222,238,387-393) and the driver derives its device name from the service name (rda-driver rda_control.c:88-119) → the comparison leg **never exercises our driver** (silent direct-handle fallback). Fix: install under service `rda` (or explicit override) + assert the RDA path was actually used in scan JSON. | comparison-ours.yml:120-127; rda-driver/src/driver/rda_control.c:88-130 |

## 6. Docs (W5) and release (W6)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-W5-DELTAS | **CLOSED (046889d)** | PARITY-DELTAS D-36..D-44 + refresh D-06/07/13c/17, D-28/29/30, U-8/12; removed-dead-surface appendix (D-44). | docs/PARITY-DELTAS.md (no D-36+) |
| GAP-W5-CONTRACTS | **CLOSED (this wave)** | ground-truth-contracts: §3 fixup-gate/viability/0xFFF/nibble rows verified PRESENT (landed with 046889d); §2 cache-coalesced contract point ADDED (the reader is the batching point; NTFS-2/D-4x); §6 LDM history note verified present; §11 UI-binding-surface contracts ADDED (cr_ui ABI policy, zero-init configs, threading/progress single-flight, G-02 snapshot semantics, G-12 query, tree --csv/--json frozen schemas with the numeric enum table). | docs/ground-truth-contracts.md §2 + §11 |
| GAP-W5-REGISTRY | **CLOSED (verified stale)** | error-fix-registry: F11 already carries the WRITER-2 correction ("the earlier 'collision = overwrite' reading of this row was wrong" - og renames `name_N`, cap 10000, DRWUI:357110-357136) and F15 is already reworded (fastsearch ini precise key U-11 + the honest no-stdout-preamble note). No edit needed; register row was stale. | docs/error-fix-registry.md:17,21 |
| GAP-W5-BENCHREPORT | **CLOSED (e9e1176)** | `docs/BENCHMARK-REPORT.md` does not exist; must carry seeded-scenario numbers with MATCH/EXCEEDS/BELOW/NOT-COMPARABLE verdicts incl. the recycle 0/26 → 26/26 closure. | `ls docs/BENCHMARK-REPORT.md` |
| GAP-W5-ERRATA | **CLOSED (easeus-drw-research 03ad46c)** | Corpus errata feed: 0xFFF misread, FUN_180041970 mislabel, FAT12 BAD raw-follow, ext4 backup-SB overreach, FileFilter 1,481 recount. | PARITY-REFACTOR-PLAN §3 W5 |
| GAP-W5-TODO | **CLOSED (verified zero)** | TODO sweep: the two counted matches are POLICY SENTENCES, not markers - PARITY-REFACTOR-PLAN.md:61 ("no implementation this session ... no stub, no TODO") and UI-READINESS-PLAN.md:337 ("No stubs, no TODOs, no fake data"). Zero actual TODO/FIXME markers in docs/ and src/. | grep -n TODO docs/*.md (this session) |
| GAP-W6-WIN7 | **CLOSED (74f42c0 + bb6a647..ccc1dca; CI PROVEN)** | No `_WIN32_WINNT`/`NTDDI` pin in any CMake target (F-D-4); no user-mode import allowlist gate in CI (driver repo has stamp_osversion.py + verify_pe.py to port). | grep CMakeLists: 0 matches |
| GAP-W6-RELEASE | **CLOSED (v0.3.0-parity LIVE; release run 40031221660)** | Release workflow now stages the complete SDK per this row: rrecov.exe + cr_ui.dll + cr_ui.lib (import) + cr_core/cr_win static libs + include/ headers + docs + the APP-LOCAL RUNTIME (vcpkg z/lz4/zstd AND the VC143 CRT + UCRT - Win7 RTM ships no UCRT; without them the zip does not load on a clean Win7) + pdbs + SHA256SUMS, every artifact existence-checked, both PEs stamped + import-gated in-lane, release body = RELEASE-NOTES.md (references this list + PARITY-DELTAS + BENCHMARK-REPORT). Tag: v0.3.0-parity (the v0.2.1-parity convention + the WRAP recipe), cut at 2140eb4 after the FULL six-lane green at 849f0f9 (code-identical); the release workflow's live validation caught and fixed three real staging defects in sequence (the SDK-26100 UCRT relocation, the PowerShell array gotcha, the empty-dir name-sort) + one production-cleanliness defect (test-framework DLLs in the bundle). Final bundle: 117 entries, 3.94 MB - bin/ (rrecov.exe, cr_ui.dll, z/lz4/zstd, VC143 CRT, UCRT + api-sets), lib/ (cr_ui.lib + cr_core/cr_win statics), include/, docs/, SHA256SUMS over all 116 files; release body = RELEASE-NOTES.md (honest manifest). | .github/workflows/release.yml; RELEASE-NOTES.md |

## 7. Closure protocol

1. CI first: GAP-CI-1 unblocks main; every later push keeps all lanes green.
2. P1 correctness wave (GAP-P1-1..5) — recovery-output bugs before surface work.
3. W2/W3 leftovers (GAP-W2-9, GAP-W2-19, GAP-W3-9a/b, GAP-W3-10).
4. W4 dead-code sweep.
5. G-01 shim + G-02 snapshot + tree --json (issue #1/#2/#10).
6. Benchmark truth wave (GAP-BENCH-1..5) + BENCHMARK-REPORT.
7. Docs wave (W5) + Win7 gate + release (W6).

---

# Session 6 register (core-final, 2026-10-25) — the full-matrix engine findings + the missing product surfaces

> Scope of this section: every delta verified THIS session against the code at
> HEAD `064fcf4` (line-by-line), cross-checked against the corpus decompiles
> and, where the corpus was silent, against an INDEPENDENT PROBE (mtools
> FAT32 image, `probe/fat32.img` evidence chain in the session worklog). Rows
> carry the mandate's verdict vocabulary. Closure rule unchanged: a row closes
> only by a commit citing its GAP id with a fail-before/pass-after test.

## 6.A P0 engine correctness (recovery-output-affecting, bench-evidenced)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-S6-1 | **WRONG** | FAT32 deleted LFN names lost: `lfn_checksum(e->name)` runs over the 0xE5-marked 8.3 bytes, so deleted LFN validation ALWAYS fails and names fall back to `_`-mangled 8.3 (`SEED\TINY\_000.JPG`). PROBE (mtools FAT32 image, deleted file, raw dentry dump): deletion 0xE5's byte 0 of EVERY 32-byte entry — for LFN slots that is the SEQUENCE byte, the 13-char name chunks are FULLY INTACT; every slot carries the same checksum, and brute-forcing the destroyed 8.3 first byte against that checksum recovers BOTH the slot-set validation AND the original short-name first char deterministically (the checksum's byte-0 contribution is a bijection). | bench run 40079065019 fat32 cells (names); `src/core/fat/cr_fat_quick.cpp:174` + `cr_lfn.cpp:46`; corpus `fat-exfat-engine-internals.md` §3 (LFN assembly incl. deleted entries); probe image `../probe/fat32.img` (session worklog) |
| GAP-S6-2 | **WRONG** | FAT32 deleted content wrong (10143 'done', 46/10143 hash-true): `reconstruct_deleted_chain` stops at the first FREE FAT entry — a deleted (unlinked) chain yields a ONE-cluster extent, the writer zero-fills the rest at the full dentry size → plausible-size wrong-byte files. Fix: contiguous-candidate reconstruction bounded by `expect_clusters`, validated cluster-by-cluster against the FAT free-state (a reused candidate cluster cuts/truncates the run), graded PartialRisk (inferred chain — the honesty cap already distinguishes FAT-linked from inferred). | bench run 40079065019 (`46/10143 hash-true` via rematch); `src/core/fat/cr_chain.cpp:174-214`, `cr_fat_quick.cpp:203-251`; corpus `fat-exfat-engine-internals.md` §5 ("deleted-entry chain reconstruction + cross-FAT recovery + orphan cluster re-linking") |
| GAP-S6-3 | **WRONG** | ReFS real-volume refusal: the checkpoint pointer-list layout gate uses `boot.minor` (VBR minor; =7 on ReFS 3.7) where the OG gates on the CHECKPOINT BODY minor (`*(ushort*)(body+6) < 0xE`, decompile FUN_180012650). Our own comment ("minor>=14 (ReFS >= 3.7)") names the body field; the code reads the VBR field. On any volume whose body layout minor differs from the VBR minor (real Server-2022 3.7), we read the wrong descriptor list → `tables.empty()` → ERR_BAD_MAGIC in ~2 ms. Also: the og's body offset is DYNAMIC (0x20 + reference-size from the page header, FUN_18000fc80 `param_1[4] += ref.size`), ours hardcodes 0x50. | bench run 40079065019 refs cells (rc=0x50030001, 0 files, ~2 ms); `src/core/refs/cr_refs_engine.cpp:499` vs corpus `ReFSSearchDLL_decompiled.c` FUN_180012650 (`lVar7 = 0x58; if (*(ushort *)(param_2 + uVar14 + 6) < 0xe) lVar7 = 0x44;`) |
| GAP-S6-4 | **MISSING** | NTFS remnant scan is name-only (quick_format 0/10228): the remnant path extracts $FILE_NAME but never parses the record's $DATA runlists — every remnant node is MetadataOnly with no extents → recovery ERR_TRUNCATED (10013). The og rebuilds full files from the same remnants (51.5% smoke recall). Fix: parse the unnamed $DATA (resident → resident_data; non-resident → runlist extents, reusing `parse_data_runs` + the main walk's extent-building) and grade by extent presence (extents → better-than-MetadataOnly; the post-format $Bitmap is empty so no reuse grading is possible — PartialRisk is the honest class). | bench runs 40073093997 + 40079065019 quick_format cells; `cr_mftwalk.cpp:1109-1194` (remnant emission, no $DATA path); root-causes.json quick_format |
| GAP-S6-5 | **WRONG** (root-cause correction) | NTFS decay_80 collapse (96.4% → 0.2%): the registered hypothesis ("$Bitmap grading drops partially-reused content") is CONTRADICTED by code — extents emit regardless of grade (`cr_mftwalk.cpp:745-767`) and recover-all enqueues regardless of grade (`rrecov.cpp:884-908`). The cliff is consistent with PHYSICAL destruction: NTFS allocation-hint semantics place the decay fillers beyond the seed region first (decay_50 ≈ intact), and wrap into the freed seed clusters once the tail space is exhausted (decay_80 ≈ destroyed). exFAT holding 98.6% at decay_80 indicates ITS allocator did not reuse the seed clusters — a volume-allocation difference, not an engine defect. ACTION: add the physical-ceiling metric (hashmap-post-decay.csv ground truth: files whose full extent ranges still hash-match) to the bench scorer so recall is judged against what is physically recoverable; correct the root-causes entry. | `cr_mftwalk.cpp:652-691,745-767`; `rrecov.cpp:884-908`; bench/scenarios/apply-scenario.ps1 decay_ block; root-causes.json ntfs_decay_grading |
| GAP-S6-6 | **MISSING** | delete_partition dual-candidate discovery: the deep tree carried ONE candidate ('Candidate @ 32768') where the og found BOTH deleted NTFS partitions. Code reading bounds the defect to the discover pass (the orchestrator scans every candidate; parallel path verified) — prime suspect: the nested-same-fs dedup (`cr_candidates.cpp:260-273`) when the outer candidate's size came from the DEVICE-TAIL FALLBACK (`:96-98`, probe failure path) rather than a BPB — a fallback-sized extent spans to disk end and swallows every later peer. ACTION: local reproduction fixture (two deleted NTFS boot sectors at unequal offsets, one with a damaged BPB) before the fix; dedup must not treat fallback-sized extents as containment evidence. | bench run 40073093997 delete_partition artifacts; `cr_candidates.cpp:92-98, 247-251, 260-273`; root-causes.json delete_partition |
| GAP-S6-7 | **WRONG** (product-behavior) | MetadataOnly items enqueued by recover-all commit as full-size ZERO files (991 'salvaged' false positives vs manifest). The zero-fill Skip-policy is designed for PARTIAL spans of real files; whole no-extent items must not become full-size zeros. Fix: recover-all skips file nodes with NO extents and NO resident_data (report outcome 'nothing-to-recover'), preserving the partial-span zero-fill semantics for real files. | bench run 40073093997 report outcomes {salvaged 991}; root-causes.json permanent_delete |
| GAP-S6-8 | **MISSING** | Resume carve dedup: the resumed scan's tree carries EXTRA carved entries the uninterrupted scan suppressed — the carve dedup index is not restored/rebuilt across the resume boundary. Fix: rebuild the dedup baseline from the restored tree (extent-key set) before the resumed carve stage; regression = the exact probe flow (kill at 6 s → resume → diff vs uninterrupted). | runs 40073093997 + 40077233920 ours-behavior artifacts; root-causes.json behavior_probes |

## 6.B Product surfaces (mandate §3/§4 — verified absent at HEAD)

| ID | Status | Item | Evidence |
|---|---|---|---|
| GAP-S6-9 | **MISSING** | `docs/ABI-CONTRACT.md`: the cr_ui ABI is documented in-header + UI-READINESS-PLAN but there is no standalone contract document with ownership/threading/error-model/stability policy + the typed EVENT SCHEMA. | `ls docs/ABI-CONTRACT.md` (this session) |
| GAP-S6-10 | **MISSING** | Typed event stream: stage transitions, byte counts, error-map deltas and files-found counts are currently only available via the progress callback (stage/permille/bytes/error_ranges) and post-hoc telemetry — no typed event surface a consumer can subscribe to incrementally. Design target: the corpus's engine→UI pipe (`INotifyObserver::Notify` → CMessageProxy thread-marshaling → `doDispatchMessage(int, void*, int&)` fan-out; MOM-modules §4, UIUX-13 §3). | grep src/ for event surfaces; corpus MOM-modules--ui-bridge §4; UIUX/13-ui-core-wiring-state.md §3 |
| GAP-S6-11 | **MISSING** | Visualization data model: `AnalysisGridWidget`-equivalent geometry producer — cluster/sector allocation map (the og's `IBuildVolumeBitmap` shared interface), scan-coverage overlay, confidence grading overlay, bad-sector overlay. No `cr_ui_disk_map` surface exists; the engine-side bitmaps (NTFS `VolumeBitmap`, exFAT/ReFS allocators, `ReadErrorMap`) are internal-only. | grep disk_map/visualization: 0 hits; corpus UIUX/08 §1 (AnalysisGridWidget, 54 v-methods, sector-grid painting), SearchDLL family §2 (IBuildVolumeBitmap) |
| GAP-S6-12 | **MISSING** | SQLite persistence layer: session/tree/checkpoints/error-maps persist via the CRSF container only; nothing query-driven survives in a database. Mandate: SQLite WAL on every connection, busy_timeout 10-30 s, synchronous=NORMAL, bounded cache_size; paging/filtering query-driven, not materialized. Design: a session-index sidecar (results DB) alongside the CRSF container (the durable scan-session contract stays CRSF — D-01). | grep sqlite: 0 engine hits; vcpkg.json has no sqlite3 port |
| GAP-S6-13 | **MISSING** | 64 KiB-aligned direct I/O on Windows: `aligned_read_at` aligns to the LOGICAL SECTOR (512) only — the mandate's non-negotiable constraint (4-KiB-multiple-but-not-64-KiB lengths take the ~3.5× slow path on the direct-handle volume stack). Fix: 64 KiB envelope on the direct (non-RDA) path, clamped at device end, pass-through when already aligned; RDA path keeps sector alignment (kernel IRP path). | `src/win/cr_transport.cpp:133-166` (sector envelope); mandate §4 |
| GAP-S6-14 | **MISSING** | `docs/PERFORMANCE-PROFILE.md`: baseline measurements of the current build on the reference scenarios (the bench corpus numbers exist in runs/BENCHMARK-REPORT but no per-scenario profile doc at HEAD). | `ls docs/PERFORMANCE-PROFILE.md` (this session) |
| GAP-S6-15 | **PARTIAL** | LDM layout-relative extents: recovery from LDM-scanned volumes was misbased pre-F-1 and F-1's rebase fixed volume-slice cases, but LDM spans (RAID-0/concat across disks) need per-node multi-disk bases; recover from an LDM scan can still misbased. Registered follow-up from the F-1 session. | worklog F-1 session remainder; `cr_ldm.h` span model |

## 6.C Closure order (this session)

1. Gate docs first (GAP-S6-9, GAP-S6-14) — pushed before any code change.
2. P0 engine wave: GAP-S6-1, GAP-S6-2, GAP-S6-3, GAP-S6-4 (recall-critical).
3. Behavior wave: GAP-S6-6, GAP-S6-7, GAP-S6-8 (with local reproductions).
4. I/O constraint: GAP-S6-13 (64 KiB envelope + unit-tested computation).
5. Surfaces: GAP-S6-10 (event stream), GAP-S6-11 (disk map), GAP-S6-12 (SQLite layer) — each lands with ABI-CONTRACT updates in the same change.
6. GAP-S6-5 root-cause correction lands with the bench scorer change (physical-ceiling metric).
7. GAP-S6-15 closes with the LDM multi-disk extent basis + test.
