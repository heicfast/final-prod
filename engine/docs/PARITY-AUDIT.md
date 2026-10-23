# PARITY-AUDIT.md — independent parity audit (gate document)

> **Status: AUDIT PHASE COMPLETE — this document is the gate. It was pushed BEFORE any
> parity fix.** Every fix that follows must reference a finding ID from this document.
> Authoritative copy lives in `core_recovery`; `rda-wizard/docs/PARITY-AUDIT.md` is the
> mirrored summary for the UI side.

- **Auditor:** independent parity-auditor agent (third party; not the core agent, not the
  UI agent). Mandate: `worklog/session-parity-prompt.md` (token redacted).
- **Pins at audit time:** core_recovery `2473c03e` + `d42bb00` (worklog bootstrap);
  rda-wizard `c2eaad6` + `c083bdd`; rda-driver `7e88e38` (v0.2.1);
  easeus-drw-research `18d6f14`.
- **Method:** 8 parallel read-only parity sub-agents (A1 NTFS/FAT/exFAT, A2
  ext4/HFS+/Btrfs/ZFS/ReFS, A3 carve/lost/LDM, A4 session/writer/health/repair, B
  orchestration+CLI+contract-drift, C UI, D architecture/structure, E deltas-registry
  draft) → full reports under `docs/parity/` (appendices, evidence-cited). The auditor
  then **personally re-verified every P0/P1 finding against the cited code and corpus
  lines** before promoting it to FIX-NOW (verification notes inline below). P2/P3
  findings are agent-reported with citations in the appendices; they are promoted to
  fixes only after the same spot-verification at fix time.
- **Verdict vocabulary:** MATCH / MISMATCH / MISSING / DELIBERATE-DELTA / UNVERIFIED.
  Any MISMATCH/MISSING that is not a registered DELTA is a bug to fix. UNVERIFIED is a
  legitimate verdict (missing RE evidence) and is tracked, never guessed.

## 1. Area verdict summary (audit grid roll-up)

| Area | Behavior | Constants/offsets | Data model | State machine | Wiring | Layering | Error surface | Threading | Capability claims | Overall |
|---|---|---|---|---|---|---|---|---|---|---|
| NTFS engine | MATCH* | MATCH (1 P1 gate bug) | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | **1 P1 to fix** |
| FAT engine | MISMATCH (FAT12 EOC) | MATCH | MATCH | MATCH (cancel gap P2) | MATCH | MATCH | MATCH | MATCH | MATCH | **1 P1 to fix** |
| exFAT engine | MATCH | MATCH (2 P2 gates) | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | clean (P2s) |
| ext4 engine | MATCH | MATCH (1 P3 guard) | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | clean |
| HFS+ engine | **MISMATCH** | **MISMATCH (2 P1)** | MATCH | n/a | MATCH | MATCH | MATCH | MATCH | MATCH | **2 P1 to fix** |
| Btrfs engine | MATCH | MATCH (chunk-tree uuid P2) | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | clean (P2s) |
| ZFS engine | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH (G-18 stale) | clean |
| ReFS engine | MATCH (gaps P2) | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | clean (P2s) |
| Carving | MISMATCH (4 P2 salvage gaps) | MATCH (rawsigns byte-exact) | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | P2 wave |
| Lost-partition | MATCH | MATCH | MATCH | n/a | MATCH | MATCH | MATCH | MATCH | honest | clean |
| LDM | MATCH | MATCH | MATCH | n/a | MATCH | MATCH | MATCH | MATCH | **contract stale (G-19)** | doc fix |
| Session CRSF | MATCH (semantic) | MATCH (chunk framing byte-verified vs rsf grammar) | MATCH (identity gaps P2) | MATCH | MATCH | MATCH | MATCH | MATCH | honest | clean |
| Writer | **MISMATCH (mkdir+abort P1)** | MATCH | MATCH | MISMATCH (abort-on-first-error) | MATCH | MATCH | MATCH | MATCH | honest | **1 P1 (owned: issue #5)** |
| Health/SMART | MATCH (ATA) | **MISMATCH (NVMe P1)** | MATCH | n/a | MATCH | MATCH | MATCH | n/a | honest | **1 P1 to fix** |
| RawFixer | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | n/a | honest | clean |
| Orchestrator | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | MATCH | clean |
| CLI (rrecov) | **MISMATCH (undo-apply P1)** | MATCH (exit codes) | MATCH | MATCH | MATCH | n/a | MATCH (1 P1) | n/a | honest | **1 P1 to fix** |
| UI Core/Integration | **MISMATCH (P0 parser)** | MATCH | MATCH | MATCH (13/30 edges) | MISMATCH (P1/P2s) | MATCH | MISMATCH (P2s) | MATCH | overclaims (P2) | **1 P0 + 3 P1** |
| Architecture/structure | MATCH | n/a | n/a | n/a | MATCH | MATCH (0 violations) | n/a | MATCH | hygiene (P1 symlink) | **1 P1 hygiene** |

\* NTFS behavior MATCH is conditional on the viability-gate fix (F-A1-2); the walker
itself (attrlist chase, ADS, UsnJrnl, remnants, $Recycle.Bin) audited MATCH with
byte-exact constants (shifted-header salvage is byte-exact vs `FUN_18002a780`).

## 2. Auditor-verified findings → FIX-NOW (fix wave 1)

Each was re-verified by the auditor against the actual code/corpus lines (not trusted
from the sub-agent summary).

| ID | Sev | Finding (verified) | Our evidence | OG/spec evidence | Fix |
|---|---|---|---|---|---|
| **F-C-1** | **P0** | UI `ListOutputParser` disk-line regexes require a bracketed table kind `[GPT\|MBR\|no-table]`; the engine prints the table kind as the final **unbracketed** token (`[removable\|fixed]` is the bracketed one). Real `rrecov list` output parses **zero disks** → Home page permanently empty. Root cause: `CORE-CONTRACT.md` §13 transcription error, inherited by the golden fixture — 99 green tests validate the bug. | `rda-wizard/src/RdaWizard.Integration/Parsers/OutputParsers.cs:14,18-19`; engine `core_recovery/src/app/rrecov.cpp:237-242` (verified at both 183dfcd and HEAD) | Engine code is the contract (CLI freeze §13 mis-transcribed) | Fix both regexes + fix §13 + fix golden fixture + add engine-output-shaped regression tests |
| **F-B-4** | **P1** | `rrecov fix --undo-apply log.undo` without `--disk` passes the guard `!have_disk && undo_apply_path.empty()` and replays undo sectors onto **PhysicalDrive0** (`disk` default-inits to 0). Data-safety bug; must land before the UI wires undo-apply. | `rrecov.cpp:1049` (guard), `:175` (`u32 disk = 0;`), `:1077` (open_disk_rw(disk)) | n/a (safety fix; og evidence not required) | Require `--disk` for undo-apply too (guard becomes `!have_disk`); contract-test |
| **F-A1-1** | **P1** | FAT12 `entry_at` returns raw 12-bit values with no BAD/EOC canonicalization (FAT16/32 branches do it) → every FAT12 chain terminates "broken", deleted FAT12 files capped at PartialRisk. No FAT12 test exists. | `src/core/fat/cr_chain.cpp:30-37` vs `:43-47`, `:55-59` | FAT spec (0xFF7 BAD / ≥0xFF8 EOC in the 12-bit table); corpus FatLib/FATSearchDLL parity | Canonicalize in the FAT12 branch; add FAT12 chain tests |
| **F-A1-2** | **P1** | NTFS deleted-viability probe uses `$FILE_NAME.real_size` (`best->real_size`); emission already prefers the $DATA-authoritative `data_real_size` (Gen-2 fix) but the probe does not → deleted files with $FILE_NAME size 0 fall into the <0x3FF fast-path and grade **Excellent without bitmap check**. | `src/core/ntfs/cr_mftwalk.cpp:480-483` (probe) vs `:531` (emission preference), `:410` (`best` = FileNameInfo) | corpus algorithms-v2 §1 viability rule (size gate + bitmap grading) | Probe uses `data_real_size ? data_real_size : best->real_size`; regression test |
| **F-A1-3** | **P1** | NTFS fixup validation checks ALL sector tails; og validates only the **first two** (`for(i=0; i<usa_count-1 && i<2; i++)`), apply still replaces all. We reject salvageable ≥2 KiB records the og salvages. | `src/core/ntfs/cr_fixup.cpp:39-43` | `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:52129-52135` (FUN_1800411f0, `local_18 < 2`), apply chain `:52452-52463` | Validate first min(2, sectors) tails; keep apply-all; keep sector-size parameterization (4Kn delta); tests + ground-truth-contracts §3 update |
| **F-A2-17** | **P1** | HFS+ B-tree node-kind model inverted: code says 0=leaf/2=header and `i16(node[NH_KIND])` **zero-extends** an unsigned byte so real leaf nodes (0xFF = -1) can never match; TN1150/og: −1=leaf, 0=index, 1=header, 2=map. Engine reads index nodes as leaf records on real volumes. | `src/core/hfsplus/cr_hfsplus_engine.cpp:55` (comment), `:306` (cast), call sites `:403` (want_kind 0) | TN1150 node kinds; og HFS+ gate `FUN_180010720` (kind ∈ {−1,0,1,2}) per appendix FS2 | Sign-correct read + TN1150 constants + fixture + test |
| **F-A2-18** | **P1** | HFS+ BTHeaderRec offsets 2 bytes early: our comment is self-inconsistent (`rootNode u32@0x02` followed by `leafRecords u32@0x04` — a 2-byte overlap is impossible); reads (e.g. `nodeSize` via `14+BTH_NODE_SIZE`) land 2 bytes early on real volumes; fixture mirrors the error (self-fulfilling). | `cr_hfsplus_engine.cpp:59-61`, `:390` | TN1150 BTHeaderRec layout; og decompile offsets per appendix FS2 | Re-derive offsets from corpus+TN1150 at fix time; fix reads + fixture + test |
| **F-A4-17** | **P1** | NVMe health log page 2: four 128-bit fields read at 64/80/96/112 (Host Reads / Host Writes / Controller Busy Time / Power Cycles) and assigned to power_cycles / power_on_hours / unsafe_shutdowns / media_errors. Spec offsets: **112/128/144/160**. | `src/win/cr_health.cpp:292-304` | NVMe 1.x health log page layout (spec); corpus silent → spec verdict (appendix DOM) | Correct offsets; extract pure page parser; unit test on Linux CI |
| **F-D-1** | **P1** | `fuzz/clang++` is a git-tracked **broken symlink** that breaks repo-wide rg/LS tooling (verified live during this audit). | `fuzz/clang++` (blob) | n/a (hygiene) | `git rm`; note in PARITY-DELTAS (it was a local helper accidentally committed) |

## 3. Agent-reported P1/P2 register → disposition (fix waves 2+)

Full citations live in `docs/parity/PARITY-*.md`. Dispositions: **FIX** (this session,
after spot-verification), **ISSUE** (filed with evidence; owner = the building agent to
avoid conflicts), **DOC** (documentation/contract correction), **DELTA** (goes to
`docs/PARITY-DELTAS.md`), **TRACK-UNVERIFIED**.

### Core
- **F-A4-9 (P1) writer mkdir + abort-on-first-error** → **ISSUE #5 comment + FIX of the
  queue-abort semantics if conflict-free** (og: `CreateDirectoryW` per level
  DRWUI:357842 + per-item failure collection). The UI-readiness agent owns G-08; the
  auditor contributes the og evidence via issue comment.
- **F-B-1/2/3 (P1) LDM contract drift** (`ldm_resolver` added, hook now unconditional,
  cross-disk registry; CORE-CONTRACT §5.1/§5.2/§12 + G-19 stale) → **DOC** (update
  CORE-CONTRACT in the coordinated pair with the UI repo).
- **F-A3-2 (P2) GPT backup-salvage dead code + doc-fiction header claim** → FIX (implement
  backup-header read at last LBA when LBA1 fails — og behavior) + header honesty.
- **F-A3-3 (P2) ZIP single-anchor fail-closed vs og 3-anchor + indexless salvage** → FIX.
- **F-A3-4 (P2) BE-TIFF/ORF dead validator branch** → FIX.
- **F-A3-5 (P2) PST unknown-version inverted vs og (accepts with 1 MiB floor)** → FIX.
- **F-A3-6 (P3) CarveStats double-count** → FIX.
- **F-A1-4..12 (P2 batch: fixup offset gate wording, PFLAG_IN_USE doc, 64 KiB dead
  constant, exFAT first-cluster gate, exFAT reserved-field nuance, contract §5 dentry
  mislabel, 0x3FF boundary (`>` vs `>=`), FAT cancel polling, deleted-name 0xE5 bytes)**
  → FIX the code-affecting ones (A1-7, A1-10, A1-11, A1-12); DOC for the contract/doc
  ones (A1-4, A1-5, A1-6, A1-9).
- **F-A2-2/5 (P2) ReFS og next-chains + unmet "MSB-chain tested" mandate clause; og
  version-gate correction (v3.4+, not "3.7-3.10"; cited FUN doesn't exist)** → FIX tests
  (multi-level tree, dual-checkpoint clock, backup SB) + comment honesty; next-chain
  following → ISSUE (scope call).
- **F-A2-7/8 (P2) ZFS G-18 stale (gang blocks now assembled), zero gang test coverage** →
  FIX tests + DOC (G-18 text in CORE-CONTRACT).
- **F-A2-12/13/14 (P2) Btrfs chunk_tree_uuid read from SB+0x60 (= log_root), inline-extent
  content discarded, no crc32c verify** → FIX uuid source (chunk-root located via
  chunk-tree walk/log area? evidence-based) + inline content + keep crc32c as tracked
  delta if og doesn't verify either (verify at fix time).
- **F-A4 P2 batch (session identity fields absent, collision naming delta, F11 registry
  text contradiction, health header 0x4D004 claim vs code, og TRIM-toggle delta)** →
  DOC/DELTA + ISSUE for session identity (ties to G-14).
- **F-B P2 batch (progress granularity vs og live counters, 3-level callback model, og
  Pause, abort-path error-map merge)** → ISSUE (ties to #3/G-04-06) + FIX the abort-path
  merge (small, conflict-free).
- **F-B P3 batch (swallowed lost::discover failures, dead PoolCancelToken, parallel
  deleted_found drop, §13 trailing spaces, 0..10000 comment, usage string D-6, "100.0%
  done" on failure, --filter case inconsistency, session-save progress)** → FIX the
  small ones; DOC the rest.
- **F-D-2/3 (P2) wizard CI absent + sln references 4 nonexistent projects** → ISSUE on
  rda-wizard (UI agent's active area) + comment evidence.
- **F-D-4 (P2) no _WIN32_WINNT pin in core despite Win7 posture claim** → ISSUE + FIX if
  trivial and conflict-free (CMake target_compile_definitions).
- **F-D-5/6/7 (P2) partition-write path absent undeclared; no logging seam; hardcoded
  classification 7 cats/~85 ext vs og 8/1489 external)** → DELTA/ISSUE (scope decisions).

### UI (rda-wizard)
- **F-C-2 (P1) ADR-4 live read-error ranges promised, unimplemented (log file vs
  stdout)** → ISSUE on rda-wizard + evidence (the `--log` tailer design).
- **F-C-3 (P1) recovery ignores user selection (recovers full session)** → ISSUE (UI
  agent's active area; request model cannot express subsets).
- **F-C-4 (P1) fixed 64 MiB space gate vs og P25 needed-space + continue-anyway** → ISSUE.
- **F-C-5..8 (P2) overlap preflight silent WMI-fail pass; frozen recovery progress; dead
  collision/folder controls; imported-session --disk -1** → ISSUE batch.
- **F-C-9 (P2) localization: infra exists, zero string tables** → ISSUE (in-flight
  expected; must not ship v1 without keys).
- **F-C-10 (P2) two-phase quick→advanced continuation promised by UI-MAP REPLACE-10,
  absent, undocumented as delta** → ISSUE + UI-MAP honesty fix.

## 4. Contract-drift register (CORE-CONTRACT.md vs core HEAD)

The UI contract was frozen at core `183dfcd`; HEAD is `2473c03e`. Verified drift:
- §5.1 `ScanConfig` gained `ldm_resolver` (F-B-1).
- §5.2 LDM hook condition changed (unconditional) (F-B-2).
- §12/G-19 cross-disk LDM registry exists (F-B-3).
- §13 list-output line: table kind **unbracketed** (F-C-1 root cause).
- §13 progress line: three trailing spaces omitted (F-B-12).
- §5.3 "0..10000 scale" claim false (F-B-13).
- `PhysicalDiskList`: **disproved** as drift — plain `vector` alias since the pin.
All §13 grammar/flags/exit-code/JSON/CSV/tree-format claims otherwise verified MATCH at
HEAD. → CORE-CONTRACT.md gets a coordinated update in the same push-pair as the fixes.

## 5. Deliberate-delta registry (draft → docs/PARITY-DELTAS.md)

35 entries drafted (27 ACCEPTED / 9 NEEDS-WORK) in `docs/parity/PARITY-DELTAS-draft.md`.
Highlights to carry into the final registry: CRSF-vs-.rsf semantics parity; 4Kn
byte-anchored geometry (F1); writer overlap refusal (F12 — og wording needs the
engine-side-branch correction from D-17); in-process engine registry (F14); no-telemetry
/ MIT / original strings; WizardStepper (ADR-12); og TRIM-toggle vs our report-only;
error-taxonomy extension. NEEDS-WORK items (F15 phantom fix, ADR-8 preview-cap framing
(og free-tier gates, not og parity), FileCategory over-claim, G-18/G-19 staleness) get
closed in this session's doc updates or filed as issues.

## 6. UNVERIFIED register (tracked, never guessed)

Key items (full lists in appendices): og USN-harvest specifics; og $I/$R recycle parse;
exFAT checksum loop details; og raw-sign hit handler internals; lost-part scoring
ground truth; og RAID-5 layout math; og ZIP EOCD selection rule; og writer hash
verification; og NVMe field mapping (spec verdict used); "corpus 0x19" constant; og
multi-session .rsf in the wild. Each is filed in the appendix reports with the precise
corpus locations that would close it.

## 7. Fix-wave plan (auditor-executed, coordinated)

- **Wave 1 (this session, conflict-checked):** F-B-4, F-C-1 (wizard+contract+fixture),
  F-A1-1, F-A1-2, F-A1-3, F-A2-17/18, F-A4-17, F-D-1 — each with regression tests,
  evidence-citing commits, coordinated CORE-CONTRACT updates.
- **Wave 2 (as time allows):** F-A3-2/3/4/5/6 carving batch; F-A1 P2 code items;
  F-B abort-path merge + P3 batch; F-A2-7/8 ZFS tests; F-A2-12/13 Btrfs; F-A2-2/5 ReFS
  tests; doc corrections (D-family, ground-truth-contracts §3/§5, error-fix-registry
  F11/F15 honesty).
- **Issue filings:** everything owned by the actively-building agents (writer G-08
  evidence, progress G-04/05/06 evidence, UI F-C-2/3/4, wizard CI/sln, Win7 pin).
- **Issue #1–#10 triage comments:** added after wave 1 with parity evidence links.

## 8. Appendices (full evidence)

`docs/parity/PARITY-CORE-FS1.md` · `PARITY-CORE-FS2.md` · `PARITY-CORE-REC.md` ·
`PARITY-CORE-DOM.md` · `PARITY-ORCH.md` · `PARITY-UI.md` · `PARITY-ARCH.md` ·
`PARITY-DELTAS-draft.md` — each with pins, coverage statements, per-finding citations
(our file:line + corpus file:line/§/address), UNVERIFIED lists, and delta candidates.
