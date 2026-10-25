# CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

Legend: `M` = MATCH · `A-n`/`S-n`/`M-n`/`N-n`/`I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`ok` = MATCH-with-note (see module file). Full per-parameter expansion in each
`modules/<module>.md`.

**Session-4 changes (2026-10-26, audited `63a9167`):** three NEW module rows —
`preview`, `query`, `transport` (first-time audits of modules no prior session
covered) — with findings **S-023, M-028, M-029, M-030, N-040..N-044, I-015, I-016**
(renumbered past the parallel S3/S4 legs' own S-016..S-022 / M-024..M-027 /
N-033..N-039 / I-013/I-014 — cross-references inside the parallel legs' cells for
THEIR IDs are untouched); `orchestrator` gains S-023 (grading); `abi` gains
N-040/N-041/I-016; `cli` gains N-044; `health` gains N-042; OQ-S2M-1 RESOLVED
(S-023); OQ-S2M-2 partial (NTFS bonus setters decoded); M-022/M-023 amended
(OQ-1/OQ-3 resolved; counts corrected); corpus errata rows 29-36 filed (after the
S3-CARVE leg's 22-28). A-004 + S-016 verified FIXED in-tree (`c33236f`) — ACUTE
tier empty since that commit. The stale duplicate `carve` row left by the parallel
legs' own merge was removed (their updated row with the S-015/S-017-fixed
dispositions is the surviving one).

**Session 2 + parallel-leg combined changes:**
- New finding IDs (session 2): S-010, S-011, M-007, M-008, M-009, N-011, N-012,
  N-013, N-014, N-015, N-016, N-017, N-018, N-019, N-020, N-021 (was N-010,
  renumbered to avoid conflict with parallel-leg's N-010), I-006, I-007.
- New finding IDs (session-2 MAIN-AUDITOR leg, renumbered post-rebase to avoid the
  collisions above): **A-004, S-012, S-013, S-014, M-011..M-021, N-023..N-029,
  I-009, I-010, I-011**, plus OQ-S2M-1/OQ-S2M-2 (tracked-open). One candidate
  RETRACTED by the leg's own re-verification (exFAT fixer checksum predicate —
  core MATCH; see modules/exfat.md addendum).
- New finding ID (parallel leg): N-010 (FAT BAD sentinel) — FIXED in same commit.
- Resolved (moved to `findings/FIXED/`): A-001, **A-002 (fix-induced LDM GUID
  regression — restored to the real Microsoft values; test fixtures
  de-self-referenced)**, S-001, S-004, S-005, S-008,
  M-001, M-006 (combined with S-009 — split tracked in N-020), N-002, N-003,
  N-010-theirs, OQ-T1.
- Acknowledged (stay in INFORMATIONAL/ with pin tests): I-001..I-005.
- S-009 partially FIXED (disk-map visualization via GAP-S6-11); the combined
  S-009/M-006 finding file remains in SEVERE/ pending split (tracked in N-020).

| Module \ Group | B | D | C | A | S | O | P | X | R | F | Q |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ntfs | M-007, M-008 (S2); N-011 FIXED-ALREADY + N-012 FIXED (minors session 2026-10-26, D-52); I-001 ack; S-012 FIXED (two-pass: pass-1 membership + pass-2 dump walk + size-0 rescue; convergent dual-agent fix) | M (S-001+S-012 fixed: membership keep), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M (N-021 fixed) | M (N-021 fixed) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M (S-011 fixed), N-003 FIXED, OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M | M | M | M (debt) |
| fat | M; N-021 (S2) | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M (S-011 + S-013 fixed), N-003 FIXED, OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | M (debt) |
| ext4 | S-003 OPEN, S-004 FIXED, S-026 FIXED (meta_bg resolver, D-58); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks) | S-003 OPEN, S-004 FIXED, S-025 OPEN | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 OPEN, S-003 OPEN, N-005; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | THIN |
| btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010**, I-007 (S2) | M; **M-010**, I-007 (S2) | M | M | M | NA | NA | M | M | M | M |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved — parity); **S-020 FIXED (s4: resident+all-rows+paged+granularity)** | M | M | **M-022 FIXED (s4: dual grammar + gen tag)** | M | NA | NA | M | M | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007, M-005 | M | S-007, N-008; M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | M | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | M | M (D12), N-014 (S2) | M | NA | M; N-014 (S2) | NA | NA | M | M | I-005.7 ack | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2), N-015 (S2)**, N-009d | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock) | NA | N-009a | NA | M | NA | NA | NA | M | M | M + **N-032 (S3: CSMI/USB-bridge transports)**; **N-042 (S4: CLI temp fractional digit — renumbered past the parallel legs' N-033..N-039)** |
| rawfixer | M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | M | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | M-005 | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4: no post-scan grading engine / RecoveryAssistant)**; N-043 (S4: progress watchdog) | NA | M | M | M | M |
| visualization | S-009 (partial-FIXED-in-tree, finding file stale — N-020) | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M |
| abi (cr_ui) | NA | M | ok | NA | M | M | S-009 | M | M | M | **N-040 (S4: doc truthfulness), N-041 (S4: dead code), I-016 (S4: factory register + FS-code spaces)** |
| cli | N-009c | NA | M | NA | M | NA | M | NA | M | M | ok; N-020 (S2); **N-044 (S4: --deep naming hazard)** |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2), N-019 (S2), N-020 (S2)**; M-006 FIXED |
| preview (S4 NEW) | NA | NA | NA | NA | NA | NA | NA | NA | NA | **M-024 (S4: caps bypass — executed proof; provenance misattributions)** | I-013 (S4: architecture register — og socket protocol vs in-process reader) |
| query (S4 NEW) | **M-029 (S4: sort/paging/bucket-IDs/server-side push)** + ~~M-023~~ FIXED (FIXED/M-023: category source now the generated table) | NA | NA | NA | NA | NA | NA | NA | NA | **M-029, N-040 (frozen-doc unit lie)** | M-029 (S4); S-023-adjacent (no precomputed category map) |
| transport (S4 NEW) | M-030 (S4: WinPE gate, stale-handle ladder, terminal OFFLINE, 32-disk cap) | M-030 | M-030 (IOCTL numbers = own driver's, grammar shape M) | M-030 (chunking provenance corrected; OQ-S4-3 perf open) | M-030 | NA | NA | M-030 (silent RDA fallback) | M-030 | M | **N-041 (S4: dead RdaClient + volume_letter_to_disk)**; I-016 (S4: wire register) |

Cross-cutting rows: **Q66 = M-006 FIXED** (fuzz CI re-enabled by parallel leg `e30b84d`;
codeql deferral documented). **Q67 = M + N-018 (S2)** (ASan/TSan CI green; UBSan NOT enabled —
session 2 found this gap). **Q68 = N-020 (S2)** (session 2 found 6 stale doc claims;
previously marked M). **Q69 = M** (zero stub markers repo-wide, grep-verified). **Q70 = M**
(Win7 posture + import allowlist verified by CI PE-gate). **X47/X48/X49/X50 = tracked-open**
(og-execution not possible here; bench harness is the vehicle). **X51 = M post-GAP-S6-13 +
M-009 (S2)** (64 KiB envelope on the scan path; **session 2 found the repair-path `RwDevice`
bypasses it** — M-009). **X53 = M + N-017 (S2)** (cancel watcher 50 ms + per-record polls;
**session 2 found the orchestrator parallel candidate scan lacks the carve watcher** — N-017).

A cell reading `M (X-n fixed)` or `X-n FIXED` marks a finding the remediation session closed
— the finding file's Resolution section carries the fix commit and test evidence; the
underlying parameter is MATCH as of that fix. Cells with finding IDs marked **(S2)** are new
session-2 findings; the previous `M` verdict was refuted by first-hand decompile + core
source read. Cells marked `ack` are ACKNOWLEDGED informational findings (deliberate supersets
with pin tests, stay in INFORMATIONAL/).

**Session-3 changes (this leg):** S-015 (rawsigns KiB units, SEVERE), M-022 (ReFS
file-value grammar), M-023 (filter model — replaces the old GAP-cats note), N-030
(rawsigns confidence dead in og), N-031 (carve dispatch/stride deltas; OQ-T8
RESOLVED), N-032 (health transports), I-012 (GAP-ADS resolved — parity). Fixed
since the last matrix pass: S-010 (3e34ed2), M-009 (7fd00e0) — both first-hand
verified in FIXED/. CPartitionVirtualVolumeDevice (og virtual-volume device
layer) verified architecturally equivalent to the core IStorageDevice + slice
adapters (devirtualized field reads sector-size@+0x38/size via vt[3]/vt[5] —
HFSPlusSearchDLL :454/:550-572; evidence s3-refs-file-pipeline.md §7 context).

Group keys: B behavioral · D data model · C constants/formats · A algorithms ·
S state machine · O orchestration · P API · X performance · R architecture ·
F safety · Q quality (definitions in `PARAMETERS.md`).


---

# Main-auditor leg cells (session 2, third leg — decompile-exhaustive reads; core `e93decf` re-verified on `34b3441`)

| Module | Parameter(s) | Verdict change (evidence in the finding + `evidence/decompiled-quotes/*session2*`) |
|---|---|---|
| ntfs | B1/B4/D15/A26/B5 | **S-012 — FIXED (convergent dual-agent fix, session 4)** — two-pass architecture landed: destroy default OFF (pass-1 membership, :18630), full pass-2 GUID-dump walk with claim map + og gates (:21152-21264), size-0 rescue (mirror resident + same-name merge); one finding premise corrected (no cross-pass dedup og-side — see the Resolution's session-4 convergence addendum) |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024** (8.3 short-name field; two-sided boot ladder; pass-2 stride quirk superset; namespace-ranking superset; og error facility codes) |
| ntfs | B1/B4/D15/A26/B5 | **S-012 FIXED (diff-fix session)** — two-pass landed: pass-1 membership restored (destroy default off), GUID-dump pass 2 (claim-map viability, 0x3ff + blanket-$ + folder rules), size-0 rescue (mirror resident + same-name merge); Resolution in findings/FIXED/S-012-*.md |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024 — FIXED (minors session, 2026-10-26)** (short_name field + session persistence ✓; boot ladder = LoadNtfsVolume exactly ✓; stride quirk + namespace ranking registered D-52; error-code mapping registered in ABI-CONTRACT §4 — see findings/FIXED/N-024) |
| fat | B1/S35/X49 | **M-011** (table-size windows: og FAT32 128 KiB–300 MiB, FAT16 ≥12 sectors; core 16 MiB reject = scan-failure class) |
| fat | A27/B2/S35 | **M-012** (mirror policy: primary-wins-on-nonzero incl. BAD, scrutinized mirror answers, mirror-only mode; supersedes the N-022 sub-case) — **N-022 sub-case FIXED (minors session, 2026-10-26)**: mirror consult now primary-FREE-only by default (`FatScanFlags::mirror_bad_salvage` opt-in; see findings/FIXED/N-022) |
| fat | B1/B3/A27 | **M-015** (og validates loaded tables: media-word canonical, cross-link wholesale rejection, 98% watermark) |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| exfat | B2/A27/X47 | **S-013 — FIXED (session 3)** (windowed-FAT accessor, og 1 MiB windows; have_fat never false; extent coverage pinned; windowed failures degrade to the EXFAT-2 salvage) |
| exfat | B3/B4/B5/D15 | **M-013** (og DOES validate entry-set checksums — dual-form incl. deleted sets + reconstruction oracle; refutes the N-003 fix's verified-absence premise; supersedes N-003) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| hfsplus | B3/B5/B7/C20 | **N-028 — FIXED (minors session, 2026-10-26)** (sink filter at both record sources ✓; rsrc superset registered D-53; 'prdf' folderization ✓ — see findings/FIXED/N-028) + **N-038 FIXED** (ownerFlags 0x20 fork selection) + **N-005 FIXED** (block bound 16 MiB + product sanity) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B1/B2/A25/D15 | **M-019** (multi-vdev: og attempts with first-entry fallback, fletcher4 verify + next-DVA retry; core refuses and never verifies — engages the parallel leg's "honest superset" stance with quoted evidence) |
| refs | B1/C19/C20/A25 | **N-026** (checkpoint dynamic body offset vs hardcode; two-sided boot gates; second-copy location START+MID vs START+TAIL) |
| table+ldm | B1/C20 | **M-016** (TOCBLOCK candidates: og {+2,+0,+1} vs core {+1,+2,+2045,+2046} — config+0 missed, conflict preference +2 vs +1) |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| lost-partition | A30 | **M-017** (OQ-T9 resolved: og ln(count)/ln(max) content scoring, +500 table-match, count==1 quirk, AdjustPartitionInfoOrder rules — verified vs 187 log rows; core absolute points divergent) |
| carve | B1/X47/X51 | **I-009** (byte-stepped probing = detection superset; block-shape constants) |
| carve | B2-adj/R54/R57 | **M-021** (video-repair pre-scan hook absent) |
| carve | B1/B2/C19/A28/Q65/P46 | **S-017 — FIXED (session 3)** (ftyp-family magics re-keyed on the box TYPE; F8 self-probe guards the unreachable-magic class; naked-moov salvage mirrors the og MovAnalyzer conservatively, tried last) |
| carve | B2/A28/B3 | **S-018 — INVALID (session 3)** (premise refuted first-hand: the og's modern JPEG analyser IS EOI-gated — 0xD9 appears as Ghidra's signed char `-0x27`, 5 sites; `uVar21 == 0 -> epilogue`; the audit's literal `0xd9` grep missed them; residual = legacy fallback sub-analysers + streamed continuation -> OQ-S3-JPEG) || carve | D13/B10 | **N-027** (carved-name split inside the core; runtime per-format config surface; ZIP padding inversion + two-tier version gates — refines N-008) |
| session | B10/D12 | **N-029** (U+FFFD replacement-count policy + substitution timing in saved sessions; cross-refs parallel leg's M-010) |
| health | F61/B1 | **S-014** (BitLocker auto-unlock: wrong live registry path, offline-hive path absent, unassociated enumeration — F61 was never scored before) |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| orchestrator/wide | B1/D16 | **M-018** (FilterFolder junk-filter family absent and unregistered) — adjacent minors **N-016 + N-017 FIXED (minors session 2026-10-26)** (teardown double-emit removed, count pinned; CancelPoolWatcher factored + wired at the candidate fan-out — D-54) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |
| cross-cutting | Q68 (corpus docs) | **I-011** (8 corpus artifact claims corrected against the decompile; see CORPUS-ERRATA.md addendum) |
| cross-cutting | Q68/Q46 | **I-010** (scope exclusions + supersets registered: cloud/NAS/repair/WinPE/installer/licensing/i18n; ZFS label-field superset; ext4 primary-SB-only = og parity) |

**Retracted by this leg's own re-verification (recorded, not filed):** the exFAT
ChecksumDBR "no-skip" candidate — the og's fixer skips {106,107,112} via a
`std::find == end()` gate over the −1-terminated table at `DAT_180079a40`; the core's
spec-skip is a MATCH (the parallel leg's binary extraction was right; this leg's first
reading was wrong — corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`).

# S3-parallel-leg cells (session 3, second continuation leg — wave 1 + residues; core `34b3441`)

| Module | Parameter(s) | Verdict change (evidence: finding file + `evidence/decompiled-quotes/s3-*` / `evidence/executions/s3-*`) |
|---|---|---|
| carve | C19/B1/B2 | **S-017 — FIXED (`1cc08c3`, post-wave-2)** (ftyp re-keyed on box TYPE + F8 structural self-probe; 6 fail-before tests) |
| carve | A28/B2 | **S-018 — INVALID (`2adfa29`, post-wave-2; refutation independently re-verified by the S3 lead: the `-0x27` signed-char FF D9 sites gate the modern analyser)** — residuals in tracked-open OQ-S3-jpeg-fallback-and-continuation |
| carve | A42/A45 | **M-024** (no runtime format-filter surface vs og RawConfig.ini [Filter], 37 toggles, STL/VPJ default-off) + **N-033** (registry exports consumed by FS engines — no core ABI) |
| carve | P47–P53/X47/X50 | **M-025** (8 MiB ping-pong + sector dispatch + pending-object streaming vs 1 MiB byte-granularity stateless; M-021's hook depends on its state machine) |
| carve | A25/A28/C19 | **N-031 addendum** (u16 equal_range chains = insertion order; id-8 continuation records; TTF-vs-MDB u32 pre-emption; TIFF-family same-class relabeling — kTrialOrder equivalence verified per family) |
| lost-partition | A30/B1 | **S-019** (BitLocker "-FVE-FS-" probe family missing — S-006 under-listed the og; distinct from S-014's unlock-path finding) |
| lost-partition | A30 | **M-017 addendum** (RTTI proof: *CalcPartition = geometry calculators, slot 4 = SPC; full AdjustPartitionInfoOrder: containment drop, FS-grouped dedup, 0.1/0.3 tie rules, ±16 MiB promotion, cap 2/4/5/6, stable merge sort) |
| lost-partition | C20/B1 | **N-034 — FIXED (minors session, 2026-10-26)** (full og deep-scan ladder `fat::lost_boot_flavor` + engine-open gates `boot_passes_open_gates`; SPC sub-claim corrected as false positive — u8 domain; see findings/FIXED/N-034) |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| refs | B2/B7/A25 | **S-020 — FIXED (s4)** (resident materialization e490/ea30 pointer-form + all-rows walk + paged chase cf30 + granularity e020; 6 fail-before tests; Resolution in findings/FIXED/S-020) |
| refs | B4/B5 | **M-026** (deleted model: og stale-"MSB+"-page sweep FUN_180005330 vs core row-flag 0x4 + ID2 — different on-disk premise; CoW favors og) |
| refs | B7/A29-adj | **M-027** (object-identity keep-one + Recycle-Bin routing/$I resident read missing; core double-emits hard links) |
| refs | C20/A27 | **N-036 PARTIALLY FIXED (s4)** (recognizer half: 0x40+refsize accepted, 0x20 physical, block-0 skip; container-map population open pending real-volume probe) |
| refs | B3/A27 | **N-037 — FIXED (s4, folded into S-020)** (granularity discard vs allocated size; premise corrected on the :10213 re-read) |
| refs | F-gate/B1 | **I-013** (host-OS runtime gate: og refuses < Win8 hosts; core host-agnostic — deliberate portability superset) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| hfsplus | B4/B5/B2/A25 | **S-021** (journal deleted-record harvester GetDeleteRecord :13637-14252 + emit FUN_18000c8f0 — the og's PRIMARY deleted surface; core parses journal_info_block and never uses it) |
| hfsplus | B2/B7 | **N-038** (fork-selection predicate: og record byte +0x29 bit 0x20 → rsrc fork + decmpfs mark; core data-fork + empty-fork substitution; field-name UNVERIFIED) |
| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| ntfs | B4/B5/A26 | **S-022 FIXED (diff-fix session — INDX/$INDEX_ROOT harvest landed; findings/FIXED/S-022-*.md)** (orphaned-record sweep: INDX/MFT records outside live $MFT + $INDEX_ROOT entry harvest — second-source names for files with dead MFT records; core has NO INDX decode) |
| ntfs | B4/C19 | **N-039** ($UsnJrnl harvest gate: og reason & 0x200 = COMPRESSION_CHANGE, harvest-all flag dead code; core true FILE_DELETE — core correct side; bounds deltas both ways; dead-agent probe disposition) |
| ntfs | B4/B5/A26 | **S-022** (orphaned-record sweep: INDX/MFT records outside live $MFT + $INDEX_ROOT entry harvest — second-source names for files with dead MFT records; core has NO INDX decode) |
| ntfs | B4/C19 | **N-039 — FIXED (minors session, 2026-10-26)** (0x20000 gate kept — og's 0x200 registered as an og defect in D-52; >64 MiB $J mines the TAIL now; fixture comment corrected; 3 pin tests + tail test — see findings/FIXED/N-039) |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| cross-cutting | Q68 | errata **E22-E28** (registry census corrections: CEPS-not-dBase :1985, OneNote-not-LZH :2156-2177, CRW/PST/DBX/HLP attributions, 35/33 key count) + **S-007 correction addendum** (fix plan's DBF validator dropped; OneNote magic + list-layer LZH replace it) |

*(Renumber map from the pre-merge wave-1 IDs: S-012→S-017, S-013→S-018, S-014→S-019,
S-015→S-020, M-011→M-024, M-012→M-025, M-015→M-026, M-016→M-027, N-025→N-033,
N-027→N-034, N-028→N-035, N-030→N-036, N-031→N-037, I-011→I-013; merged as
addenda: dispatch→N-031, video-feed→M-021, scoring/merge→M-017, resident-bodies→M-022,
byte-pin→I-009, probe-row→I-010; dropped as dups: boot-geometry→N-013b, byte-granularity→I-009.)*
