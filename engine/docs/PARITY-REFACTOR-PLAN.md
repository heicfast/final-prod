# PARITY-REFACTOR-PLAN — deep architecture parity & refactor session

<!-- session-96: the driver era ended - the kernel driver was replaced by direct raw I/O; the driver-era rows below are historical records. -->

> **Gate document. Written and pushed BEFORE any refactor code** (protocol step 7).
> Evidence base: `AUDIT-ARCH.md`, `AUDIT-FALLBACKS.md`, `AUDIT-NTFS.md`, `AUDIT-FAT.md`,
> `AUDIT-LINUX-FS.md`, `AUDIT-PIPELINE.md` (six parallel audits at pin `08b44ac` =
> code-identical to `be8875f`; corpus `18d6f14`). Every finding below carries its audit ID;
> every fix cites the audit's file:line + corpus evidence at commit time.
> Authority order: RE corpus > frozen contracts > prior agents' statements (this plan
> corrects several prior claims that failed re-verification — see §1).

## 0. Session verdict summary (from the six audits)

| Area | MATCH (or registered) | Open defects | Dead-code entries |
|---|---|---|---|
| Architecture/wiring | layering 0 violations; 18 boundaries MATCH/equivalent | 2×P1 (ARCH-1/2), 4×P2 | 26 |
| Fallbacks (56 conditionals) | 22 MATCH + 21 RETAIN-AS-SAFETY | 6 REMOVE + 2 exit-code lies | — |
| NTFS (47 params) | 37 MATCH + 6 registered = 91% | 2×P2 FIX, 10×P3 | 21 |
| FAT/exFAT (23 rows) | ~55% clean | 1×P1, 6×P2, 13×P3 | 20 |
| ext4/HFS+/Btrfs/ZFS/ReFS | 90/85/70/92/75% | ReFS 2×HIGH + Btrfs 4 gaps | 13 |
| Pipeline (carve/lost/sess/writer/health/fixer/LDM) | data pillars hold (raw-signs 139/199/62 byte-exact, CRSF chunk grammar, LDM full decode) | WRITER-1/2 + CARVE-1..4 P2s | 9 |

**ReFS is a full MSB-chain engine today** (chain, dual clock, ObjectID/allocator/parent-child,
deleted/ID2/orphans, orchestrator dispatch, fuzz) — confirmed first-class. **LDM is full
decode + reconstruction** — confirmed first-class. Remaining gaps are enumerated below, not
scope-escaped.

## 1. Prior claims that failed re-verification (corrections, not inherited)

1. ~~PARITY-ARCH.md:152~~ RESOLVED by N-041's deletion (session-12): the refuted "consumes exclusively through RdaClient
   OPEN_TARGET preferred") — **REFUTED**: production uses free-function grammar opens; the
   `crCreate*` wording is also wrong (0 matches).
2. docs/architecture.md:80-81 ("the CLI saves on cancel") — **FALSE** at HEAD
   (rrecov.cpp:503-506 returns before the save block).
3. PARITY-AUDIT "FAT otherwise near-MATCH" — **OVERTURNED** (FAT-2 mirror semantics,
   FAT-3 cancel, EXFAT-1..4 newly decoded from FUN_180041de0/180021c60/180003090).
4. F-A1-7 polarity was **inverted** in the artifact (og rejects fc==1 and ≥0xFFFFFFF7).
5. "resident $DATA cap 0xFFF" — corpus **misread** of an MSVC std::string destructor.
6. FUN_180041970 is a **base-record test**, not in-use (both artifacts mislabel).
7. og ext4 journal has **no** commit/revoke gating (walks [s_first, s_maxlen) raw) — D-07
   re-pinned; our commit gate is the asymmetry.
8. og Btrfs does **no** chunk-tree-uuid check; our SB+0x60 source is bogus (= log_root).
9. F11's og collision claim ("overwrite") is **wrong** — og renames `name_N`, cap 10000.
10. FileFilter.xml recount: 8 categories / **1,481** extensions (not 1,489); RawConfig.ini
    ships **34** toggles (not 37).

## 2. Decisions on contested items (principal-engineer rulings)

| # | Contested item | Decision | Rationale |
|---|---|---|---|
| D-A | `RdaClient` (dead in production, ~520 lines) | **SUPERSEDED — deleted (N-041, session-12)** | The KEEP ruling cited TRANSPORT-CONTRACT §8's "UI reads `info()/ioctl_base()/session_*` through it" - that document no longer exists in the tree and the claim is disproven by the exhaustive zero-caller grep (verified at audit time and again at deletion time: no consumer in src/, include/, tests/, tools/, bench/, fuzz/; cr_ui links the ENGINE, never src/win). N-041's wire-or-delete decision executed: the class + `volume_letter_to_disk` + the structured-open device plumbing deleted; wire structs stay documented in cr_rda.h; the grammar path is the sole production surface. FB-T06/FB-T07 moot (the slop paths died with the class). |
| D-B | Dead-but-documented grammar leg `open_rda_volume` | **KEEP** | Documented grammar branch (TRANSPORT-CONTRACT §2.1; og Device.mo has the `Volume\%d` duality). Mark header "wizard binding surface". |
| D-C | NTFS-1 viability boundary `>=0x3FF` vs og strict `>` | **MATCH OG** (strict `>`) | Unregistered divergence today = bug by the registry's own rule. One-line fix + boundary test at exactly 1023 bytes. |
| D-D | NTFS-2 64 KiB MFT batching | **AMEND CONTRACT + REGISTER DELTA** | Device-IO is already 64-KiB-coalesced by the CachedReader (8 MiB LRU, 64-KiB blocks) — identical device-read count to og's bootstrap buffer. Engine-level re-batching would add complexity for zero IO change. Delete the two dead constants; rewrite ground-truth-contracts §2/§3 to "cache-coalesced 64-KiB reads"; register D-4x. |
| D-E | FAT-2 mirrored-FAT semantics | **KEEP OURS + REGISTER DELTA + FIX COMMENT** | Ours salvages chains og drops (FAT1 BAD + FAT2 valid) — evidence-backed improvement for a recovery product (salvage-over-purity is already our R2 stance). Comment must stop attributing it to CDoubleFATTable. |
| D-F | FAT-4 FAT12 BAD canonicalization | **KEEP OURS + REGISTER DELTA** | Og follows BAD cluster 4087 raw (garbage-tolerant); we stop the chain. Spec-correct + safer. |
| D-G | ext4 journal commit gate (E-2) | **MATCH OG** | Og feeds every block in [s_first, s_maxlen) to the dir parser — more salvage, simpler code. Our commit gate drops uncommitted-tail dir blocks og surfaces. Replace gate with walk-all; update D-07 (the "revoked blocks surfaced" wording becomes "all journal blocks surfaced, og-exact"). |
| D-H | FB-T02 fallback condition superset (we fall back on grammar-open failure, og gates on probe only) | **KEEP + REGISTER REMARK** | An unopenable grammar path must not kill a scan; direct is sufficient for unlocked volumes. Register in D-22 remarks. |
| D-I | Skip-mode zero-fill retry policy (FB-O03) | **KEEP + REGISTER** (new D-entry) | Concept is og behavior (EXT4 CatchBuffer + BadSectorTipsDlg product copy); mechanism is ours (transport-wide bisection + ReadErrorMap). Register the mechanism delta. |
| D-J | FB-X01 zstd/lz4 build optionality | **MAKE REQUIRED** | og never runs without decompressors; ours must not either. vcpkg already lists them. Fuzz-lane pkg-config relaxation preserved. |
| D-K | ReFS next-chain + version-dependent checkpoint layout | **IMPLEMENT (both HIGH)** | Non-negotiable first-class mandate; decompile-pinned (FUN_18000fc80 next@+0x38; FUN_180012650 0x94 vs 0xA8 at minor≥14). |
| D-L | HFS-2/3 (attributes B-tree, journal replay), BTR-5/6, ZFS-2 (multi-vdev) | **REGISTER AS CAPABILITY GAPS, no implementation this session** | Honest scoping per "no stub, no TODO": gaps documented in CAPABILITY-MATRIX with acceptance criteria; not silently claimed. Session focuses on evidence-backed parity fixes. |
| D-M | `FsFamily::CarvedOnly` dead name | **DELETE NAME, RESERVE VALUE 5** | Serialized fs_kind contract (D-02) forbids renumbering. |
| D-N | Wire structs kept as layout documentation (NTFS INDX family, FAT Bpb*) | **DELETE INDX family + parse_index_root** (no walker exists, none planned — the walk model is attr-based); **KEEP FAT wire structs** only where static_asserted; delete the rest. INDX row 44 is UNVERIFIED-og — deleting speculative code beats carrying it. |

## 3. Fix waves (execution order; each = one or more commits with tests)

### Wave W1 — truth & slop removal (fallback audit REMOVEs) — all small, all tested

| Fix | Files | Acceptance test | Audit |
|---|---|---|---|
| W1-1 session_get_next_block: drain-signal-only success (0xC0000120→done, else ERR_READ_FAILED) | cr_transport.cpp:449-462 | unit: fake IOCTL failing with non-drain error ⇒ error return (win-layer — compile-verified; logic extracted testable where feasible) | FB-T07 |
| W1-2 BitLocker: unreadable boot sector ⇒ Status::Unknown; CLI refuses with unlock guidance | cr_bitlocker.cpp, rrecov.cpp | unit: locked-volume unreadable ⇒ Unknown + scan refused | FB-B01 |
| W1-3 `list` non-zero exit on enumeration failure; `health` exit 3 when ALL sources unavailable | rrecov.cpp | CLI contract test (exit codes) | FB-K01, FB-H04 |
| W1-4 zstd/lz4 REQUIRED for production builds (CMake FATAL_ERROR when absent; fuzz lane relaxed) | CMakeLists, vcpkg.json | build gate: configure fails without libs | FB-X01 |
| W1-5 RdaClient::open: GET_INFO failure ⇒ nullptr + wrong-driver error (no silent base-0) | ~~cr_transport.cpp:264-269~~ | MOOT — the class was deleted by N-041 (session-12); the slop path died with it | FB-T06 |
| W1-6 delete `skip_hashless` (cr_safewriter.h:86) + correct PARITY-CORE-DOM.md:287 | cr_safewriter.h | grep: 0 refs | FB-R03 |
| W1-7 delete `CarvedOnly` name, reserve value 5 | cr_scan.h:37 | existing session tests still green | FB-L03 |
| W1-8 carve skipped-window honesty: do NOT count unreadable window bytes as scanned; surface `windows_skipped_bytes` | cr_carve.cpp:299-302, cr_carve.h | unit: failing window ⇒ bytes_scanned excludes it; stat present | FB-C01 |
| W1-9 CLI save-on-cancel + checkpoint wiring + `--resume` (folds ARCH-2 CLI half + SESS-3) | rrecov.cpp, cr_scan.cpp | pause_resume-style CLI test: cancel mid-scan ⇒ session saved + resumable | ARCH-2, SESS-3 |
| W1-10 fix architecture.md:80-81 false claim (after W1-9 makes it true, reword to actual behavior) | docs/architecture.md | doc diff | ARCH-2 |

### Wave W2 — engine parity, P1/P2 (highest user impact first)

| Fix | Files | Acceptance test | Audit |
|---|---|---|---|
| W2-1 FAT deleted-LFN fragment reversal (deleted slots: reverse disk order before concat) | cr_lfn.cpp:21-51, cr_fat_quick.cpp:161-171 | fixture: 3-slot deleted LFN ⇒ original name | FAT-1 (P1) |
| W2-2 exFAT first-cluster gate: reject fc==1 \|\| fc≥0xFFFFFFF7 (live+deleted), count sets_damaged, keep entry | cr_exfat_quick.cpp:331 region | regression both values | EXFAT-1 |
| W2-3 exFAT EOC family ≥0xFFFFFFF8 | cr_exfat_quick.cpp:15-35 | 0xFFFFFFFA terminator test | EXFAT-3 |
| W2-4 exFAT entry==0 ⇒ contiguous continuation (cluster+1, flagged stat, cluster-count bound) | cr_exfat_quick.cpp:62-65 | FAT-zeroed salvage fixture | EXFAT-2 |
| W2-5 exFAT boot gates: MustBeZero sweep, fat_count∈[1,2], fat_length≠0, heap-fit bound | cr_exfat_dentry.cpp | boot fixtures per gate | EXFAT-4 |
| W2-6 NTFS viability boundary strict `>` (1023 bytes takes fast-path) | cr_mftwalk.h:39, .cpp:491 | 1023-byte deleted file fixture | NTFS-1 |
| W2-7 NTFS extension records: early-continue (base_record≠0), no damaged++ | cr_mftwalk.cpp:404-407 | NestedAttrlistChase + nameless child ⇒ damaged==0 | NTFS-8 |
| W2-8 NonresidentTail label swap (+0x30 file size / +0x38 initialized) + sparse-attrlist fixture | cr_ntfs_wire.h:126-135, cr_bootstrap.cpp:148 | attrlist with +0x30≠+0x38 ⇒ full read | NTFS-7 |
| W2-9 FAT cancel granularity: poll per chain-run + per dentry (≤128 dentries to cancel) | cr_chain.cpp, cr_fat_quick.cpp | cancel-token latency test | FAT-3 |
| W2-10 ext4 journal: walk ALL blocks [s_first, s_maxlen) (drop commit/commit-revoke gating) + D-07 rewrite | cr_ext4_engine.cpp:673-678 region | uncommitted-tail dir-block fixture surfaces entries | E-2 |
| W2-11 ReFS next-chain following (next@+0x38 traversal in read_page/refs table walk) | cr_refs_engine.cpp | fixture: older leaf at ref + newer leaf via chain ⇒ newer content emitted | REFS-1 (HIGH) |
| W2-12 ReFS version-dependent checkpoint pointer list (minor≥14 ⇒ list@0xA8, count@+0xA4) + checkpoint version gate | cr_refs_engine.cpp | minor-6 + minor-14 fixture pair | REFS-2 (HIGH) |
| W2-13 ReFS stale-pin purge: "3.7-3.10"→"3.4+ (major==3 && minor>3)", FUN_180013420→FUN_180012390, docs/formats.md ref removed | cr_refs_engine.h:38-39, .cpp:366-368 | grep-level assertions | REFS-3 |
| W2-14 Btrfs chunk_tree_uuid from first chunk-root node header (not SB+0x60) | cr_btrfs_engine.cpp | fixture: real uuid ≠ SB+0x60 garbage ⇒ candidate path selects chunk root directly | BTR-2 |
| W2-15 Btrfs inline-extent content → resident_data | cr_btrfs_engine.cpp | inline file ⇒ content matches | BTR-3 |
| W2-16 Btrfs compressed extents: FE_COMPRESSION read; extent {codec, psize, length} ZFS-style | cr_btrfs_engine.cpp | zlib-compressed fixture ⇒ decoded content via writer | BTR-4 |
| W2-17 Writer collect-and-continue: per-item errors collected, queue continues; cancel-only hard abort | cr_safewriter.cpp:399-400 | 3-item queue, item-2 unwritable ⇒ 1+3 Done, errors==1 | WRITER-1 (P2) |
| W2-18 Writer `had_unreadable` per-item flag + report outcome `salvaged` (zero-fill visible) | cr_safewriter.cpp:292-324, cr_report.cpp | zero-filled hash-less item reported salvaged, not Done | FB-R02 |
| W2-19 Carve: GPT backup-salvage reachable (last_lba from device when primary present-but-damaged; protective-MBR path probes last LBA) | cr_table_parse.cpp:207-264 | corrupt-primary + valid-backup ⇒ entries salvaged + gpt_backup_ok | CARVE-1 |
| W2-20 Carve ZIP: 3 anchors (LFH + CD PK\x01\x02 + EOCD PK\x05\x06) + indexless-LFH salvage + 5.24 MB padding cap | cr_analysers.cpp, cr_registry.cpp | indexless ZIP fixture; CD-first-survivor fixture | CARVE-2 |
| W2-21 Carve TIFF: BE-TIFF (MM\0\*) + ORF IIRS/MMOR registry entries (wire the dead BE branch) | cr_registry.cpp, cr_analysers.cpp | MM-TIFF + ORF fixtures carve | CARVE-3 |
| W2-22 Carve PST: unknown version accepted at 1 MiB floor | cr_analysers.cpp:733-734 | version 0x15 fixture accepted | CARVE-4 |
| W2-23 JPEG SOF precision-8 gate | cr_analysers.cpp:196-197 | precision≠8 rejected | CARVE-6 |
| W2-24 WAV vestigial fmt read deleted | cr_analysers.cpp:1105-1106 | grep | CARVE-11 |

### Wave W3 — orchestration/architecture wiring

| Fix | Files | Acceptance test | Audit |
|---|---|---|---|
| W3-1 parallel lost-candidate path: merge `deleted_found` (F-B-11); wire PoolCancelToken or delete it (F-B-10) | cr_scan.cpp:699-733 | same scan threads=1 vs 4 ⇒ identical deleted_found | ARCH-6 |
| W3-2 "100.0% done" on failure: only print success summary line on rc==OK | rrecov.cpp:492 | failed scan prints no "done" line | F-B-15 |
| W3-3 cr_device.h cancel comment: transports poll at loop heads — correct or scope the claim | cr_device.h:101-102 | doc truth | F-B-16 |
| W3-4 CLI Ctrl-C: CancelToken wired (scan + recover + health) | rrecov.cpp | contract test | ARCH-2 |
| W3-5 register D-36 single-pass quick+deep; D-37 transport-fallback-superset remark; D-38 Skip/retry mechanism; D-39 mirrored-FAT salvage; D-40 carved-naming strings; D-41 report JSON (ours-only); D-42 ATA-passthrough health delta; D-43 cache-coalesced MFT reads | docs/PARITY-DELTAS.md | registry diff | ARCH-3, D-H, D-I, D-E, CARVE-5, PIPE-report, HEALTH-2, D-D |
| W3-6 reword threading claim ×3 (roadmap, cr_parallel.h, cr_scan.cpp) | 3 files | grep "single-threaded" ⇒ new wording | ARCH-4 |
| W3-7 stale-language sweep: roadmap LDM row DONE, ground-truth-contracts §6 history note, cr_refs_detect.cpp:1 "detection probe only — full engine in cr_refs_engine" | 3 files | grep-level | ARCH-5, REFS-8, LDM-2 |
| W3-8 LDM ledger honesty (cr_ldm.h:26-28 WriteParityDisk claim reworded to libldm verdict; LDM-3 config-clamp comment fix) | cr_ldm.h, cr_ldm.cpp | doc diff | LDM-1/3 |
| W3-9 SESS-4: chunk compress-everything rule (drop ≥64 B carve, keep stored-when-not-smaller); SESS-2: device identity card fields (model/serial/label/boot-sector-hash) in VolumeInfo + restore-side verify hook | cr_crsf.cpp, cr_records.h/.cpp | round-trip test incl. 11-byte chunk; identity card assert | SESS-4/2 |
| W3-10 duplicate-code consolidation: 3 OVERLAPPED loops → one helper; cr_bootstrap 3 attribute walkers → enum_attributes; dispatch dedup (run_engine_pass/scan_candidate); byte-reader + time-conversion dedup (shared header) | cr_transport.cpp, cr_bootstrap.cpp, cr_scan.cpp, new cr_bits.h | build + full suite green; behavior-identical | ARCH-DUP-1/4, DC-12/13 |
| W3-11 RawFixer: FIX-2 rollback-rc split (clean rollback ≠ undo-failed), FIX-3 4Kn analyze offsets, FIX-1 header honesty (single derivation path) | cr_rawfixer.cpp/.h | 4Kn fixture + rollback test | FIX-1..3 |
| W3-12 health P3s: SSD story single-source (SeekPenalty + doc), TRIM first-match, HEALTH-2 delta note in header | cr_health.cpp/.h | parse tests | HEALTH-5/6/2 |

### Wave W4 — dead-code sweep (grep-verified registers)

- NTFS register items 1-18+21 (AUDIT-NTFS §4): delete constants/accessors/wire structs per
  D-N; consolidate walkers (W3-10).
- FAT/exFAT register items 1-20 (AUDIT-FAT §6): `is_dot_entry`, dead chain helpers,
  `set_raw`, dead accessors + divergence dead work, unused `keep_deleted` flags (harmonize:
  read them or remove), EXFAT duplicate constants, dead wire structs, duplicated date math.
- LINUX-FS register DC-1..13: dead constants (gaps now tracked in audit docs), byte-reader
  + time-conversion consolidation (W3-10).
- PIPELINE register 1-9: GPT dead store, `SignBucket`, `MbrTable`, `window_base()`,
  `fragmented`, `fs_probe` misuse (re-point to honest semantics), WAV vestigial (W2-24).
- ARCH register: `volume_letter_to_disk` (KEEP — writer overlap preflight candidate +
  documented), `win::open_disk_view` (delete — shadowed by CLI-local), `status_text`
  (delete), `trim_*` fields (delete), `failed_bytes()` (delete), `inner()` (delete),
  `index_node` public wrapper (delete), unreachable cached branch (delete), `round_up`
  (delete), `Out.deleted` (fix via W3-1), `temp_data.txt` (git rm), `quick_done_` u32→bool
  (ARCH-7), fs_kind literal→enum style (ARCH-DUP-7), raw hex error literals (ARCH-12).
- Every deletion listed in the commit body; PARITY-DELTAS gains a "removed dead surface"
  appendix entry (D-44).

### Wave W5 — docs, contracts, capability matrix, benchmark

| Item | Output |
|---|---|
| PARITY-DELTAS: new entries D-36..D-44; refresh D-28/29/30 (PIPE-1/2); close D-06 (btrfs note), D-07 (rewrite after W2-10), D-13c (after W2-20/21 — ZIP/EOCD/tie-break pinned by our own decompile-derived tests), D-17 (U-10 stays tracked), D-35 (F15 reword + `ln` citation fix), U-8/12 rewords | docs/PARITY-DELTAS.md |
| ground-truth-contracts: §3 (fixup gate wording, viability boundary, 0xFFF cap removal, nibble comment), §2 (cache-coalesced reads), §6 (LDM history note), §7 (unchanged — seeds verified) | docs/ground-truth-contracts.md |
| error-fix-registry: F11 og-behavior correction (rename not overwrite); F15 reword | docs/error-fix-registry.md |
| CAPABILITY-MATRIX: per-capability status, evidence-linked, ReFS/LDM first-class with explicit remaining gaps (next-chain done, containers/security/ADS pending), HFS+/Btrfs/ZFS gaps honest | docs/CAPABILITY-MATRIX.md |
| BENCHMARK-REPORT: seeded scenarios from docs/benchmarking.md methodology; our numbers vs corpus-documented og behavior where measurable; verdicts MATCH/EXCEEDS/BELOW/NOT-COMPARABLE | docs/BENCHMARK-REPORT.md |
| CORE-CONTRACT §13 coordination (rda-wizard): list/health exit codes, --resume, save-on-cancel, tree --json (if landed) | coordinated pair |
| Corpus errata feed (easeus-drw-research issues or doc notes): 0xFFF misread, FUN_180041970 mislabel, FAT12 BAD raw-follow, ext4 backup-SB overreach, FileFilter 1,481 recount | issue notes |

### Wave W6 — CI, fuzz, release

- Full suite green on Linux (gcc -Werror + ASan/UBSan/TSan); Windows lane compile gate
  (`/W4 /WX`) green; fuzz smoke on modified parsers (fat, exfat, ntfs, refs, btrfs, carve
  analysers); benchmark job if enabled.
- Win7 posture: add `_WIN32_WINNT=0x0601`/`NTDDI_VERSION` pins + import-allowlist CI gate
  (F-D-4 — machine-verified like the driver repo).
- Tag `v0.3.0-parity` with notes referencing PARITY-DELTAS + BENCHMARK-REPORT.

## 4. Acceptance gates for the session

1. Every wave's fixes land with regression tests that fail before / pass after (or
   doc-diff evidence for doc-only items).
2. `main` green after every push; pull-before-push every time (other agents active).
3. No new UNVERIFIED claims; existing UNVERIFIED registers carried forward unchanged or
   closed with new evidence.
4. PARITY-DELTAS contains an entry for every retained divergence introduced or discovered
   this session (D-36..D-44 + refreshed entries).
5. AUDIT-FALLBACKS.md statuses updated to FINAL at the end (REMOVE→done, MATCH→kept,
   RETAIN→registered).
6. ReFS/LDM first-class language sweep complete (code, docs, headers, capability strings).

## 5. Out-of-scope reminders (standing)

Cloud/NAS/remote recovery; contract breaks without coordinated updates; signing enable;
safety-hardening removal (destination overlap, BitLocker legitimate-unlock-only, RawFixer
consent-only, 4Kn, SDDL — all stay).
