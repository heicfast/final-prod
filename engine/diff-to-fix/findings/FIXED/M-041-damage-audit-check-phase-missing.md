# MAJOR M-041 — No damage-audit/check phase: the og's FileSystemCheck (chkdsk-grade FAT/NTFS audit) and its WinChkdsk delegation have no core counterpart

**Severity:** MAJOR
**Module:** check-phase (FileSystemCheck.mo) — new module row; extends rawfixer
**Parameters affected:** P1 (recovery correctness context — damage assessment), P3 (false-positive rate — cross-link/orphan detection), P5 (orphan reconstruction — orphan-cluster scan), P15 (confidence grading — the og's check errors feed repair dispatch), P25 (algorithm — per-check validators), P35 (error recovery — check-fail → RawFixer → WinChkdsk → RAW escalation), P42 (public surface — check/report ops)
**Corpus references:** `re-workspace/reversed/ghidra-mo/FileSystemCheck_decompiled.c` (read fully this session) — CCheckFAT::Check `FUN_1800015b0` (:462): DBR validators per type (:1315-1471: FAT12 cluster ceiling spf×341.3333 :1358-1360; FAT16 spf×0x100 :1415; FAT32 "FAT3"+'2'@+0x52 :1440, rootCluster range :1455, ceiling (bps×spf32)>>2 :1471; error 0x100d); FAT entry-hole check (:4386-4411) + **duplicate-reference scan** (:4337-4381) → 0x1006; **pairwise FAT-copy compare** (:689-781) → **0x1005**; dir-tree audit `FUN_180003360` (:1772: `.`/`..` :1893-1907, invalid first-cluster 0x1007 :1941-1948, **size≠chain 0x1004** :2050-2076, **dir cycle 0x1007** :2079-2097); post-walk **orphan scan** (:874-876 unvisited + FAT≠0 + ≠BAD → host decision → 0x1003 :949); CCheckNTFSVolume::Check `FUN_1800045a0` (:2757): $MFT $FILE_NAME resident + "$MFT" name (:2900-2916) → 0x1008, $MFTMirr proof (:2921-2971) → 0x1009, **NTFS version ∈ {0x102, 3, 0x301}** (:2994-3003, lead-verified) → 0x100a, $Bitmap structural (:3788-3942) → 0x100b, **MFT×$Bitmap cross-check** `FUN_180004d80` (:3061-3249: free-but-allocated → 0x1003 :3196-3209; duplicate allocation → 0x1002 :3211-3224), exception → "please use Chkdsk /F" 0x100e (:9295-9301); WinChkdsk child-process launcher `FUN_180009090` (:6956-7217); error taxonomy 0x1001-0x100e; 8-event host callback contract (SetRange/SetPos/OnError/stop-decision/status/path); `artifacts/PartitionRecovery/rawfixer-fs-repair.md:59-65` ("only volumes that resist RawFixer fall through to RAW carving")
**Core references:** audited `7cf6754` — `src/core/repair/cr_rawfixer.cpp` (repair-oriented only: `ntfs_check_mft` :177, `fat_check_tables_and_root` :273 with mirror arbitration :302-359, CheckRootDir readability :363-380); `src/core/fat/cr_chain.cpp:87-118` (FAT load with **no table validation** — known M-015 — and mirror choose-healthy :109-118 with no disagreement reporting); `src/core/ntfs/cr_bitmap.cpp:14-95` ($Bitmap used for **grading only**, never cross-checked against walked allocations); `src/core/orchestrator/cr_scan.cpp:546-566` (repair outcome never aborts scan — the escalation STANCE matches; the check PHASE itself is absent); no NTFS version gate anywhere (grep)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og runs a chkdsk-grade audit layer over a target volume before/alongside repair: CCheckFAT validates the DBR against per-type cluster-count ceilings, scans both FAT copies for entry holes and duplicate references, compares the copies entry-by-entry, walks the whole directory tree (cycles, cross-links, `.`/`..`, file-size-vs-chain), and sweeps for orphan clusters; CCheckNTFSVolume proves $MFT/$MFTMirr, gates the NTFS version (1.2/3.0/3.1), validates $Bitmap, and cross-checks every allocated MFT record's LCNs against the bitmap (free-but-allocated and double-allocation). Failures dispatch to RawFixer, then to a WinChkdsk child process, and only volumes that resist both fall through to RAW carving. The core has only repair-oriented checks inside rawfixer; it has no audit phase, no error taxonomy, no per-check reporting, and silently serves reads from the healthier FAT copy without ever reporting disagreement.

## What the reference does

See the corpus citations block — the full phase/validator/error-taxonomy
decode with every gate, threshold, and error code is in
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §B. The layer's
results feed the og's escalation ladder (structured scan → RawFixer →
WinChkdsk → RAW carving) and are user-visible as per-error reports
(0x1001 size … 0x100e exception) with a progress/status callback contract.

## What our core does

rawfixer's checks exist to BUILD A REPAIR PLAN (torn-copy restore, $MFT
restore), not to AUDIT: there is no FAT entry-hole/duplicate scan, no
pairwise-copy comparison report (the mirror arbitration picks per-entry
winners silently, cr_chain.cpp:109-118), no directory-tree audit, no
orphan-cluster sweep, no NTFS version gate, and $Bitmap is only consulted
for deleted-file grading — a doubly-allocated or bitmap-contradicting
volume walks silently. The pre-scan phase in cr_scan.cpp:546-566 wires
repair, not audit.

## Gap

Six concrete audit behaviors exist in the og with no core counterpart:
(1) FAT-copy disagreement REPORTING (0x1005) — core chooses silently;
(2) FAT entry-hole + duplicate-reference scans (0x1006);
(3) directory-tree audit: cycles/cross-links/size≠chain on LIVE files
(0x1002/0x1004/0x1007) — core grades size-vs-chain only for deleted files
(cr_fat_quick.cpp:80-90);
(4) orphan-cluster sweep with host decision (0x1003);
(5) NTFS version gate {1.2, 3.0, 3.1} (0x100a);
(6) MFT×$Bitmap cross-check (0x1002/0x1003).
Falsifiable per item — e.g., FAT1/FAT2 differing on one live chain yields
an og error report and a core clean scan with no advisory; an NTFS 3.x-
variant version word yields an og 0x100a and a core silent walk.

## Impact

Recovery results on damaged volumes: the og's audit feeds its damage
assessment and repair dispatch, and surfaces volume-level problems the core
hides. A doubly-allocated MFT region (cross-link) can yield two recovered
files with overlapping content in the core — the og flags it before the
walk. The check phase is also the og's trust signal for "structured results
are reliable here" — without it the core's confidence grading (S-023 lane)
loses an input the reference has. Not ACUTE: the core does not produce
WRONG file bytes from these gaps more often than the og's own structured
walk would (both walk the same structures); the divergence is the missing
audit/report/escalation layer.

## Reproduction / verification

Corpus-derived (full read of the 9,964-line module; the version gate and
DBR dispatch re-verified first-hand). Core-side verified by reading the
cited files. Not executable (Windows check phase + WinChkdsk child).
Evidence: `evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §B.

## Fix plan (for the engineering team)

1. New `check` module: `include/cr/check/cr_fscheck.h` +
   `src/core/check/cr_fscheck.cpp` — a read-only audit pass producing a
   `FsCheckReport { errors: vector<{code, cluster, path_hint}> }` with the
   og's error codes 0x1001-0x100e (numeric parity, P23).
2. FAT pass (order = og's phases): DBR ceilings → FAT load + hole/dup →
   pairwise copy compare → chain walk (loop/cross-link) → dir-tree audit →
   orphan sweep. Every threshold from the decode (341.3333 / 0x100 / >>2
   ceilings; BAD sentinels 0xff7/0xfff7/0xffffff7).
3. NTFS pass: $MFT/$MFTMirr proofs (reuse rawfixer's), version gate, $Bitmap
   structural, MFT×$Bitmap cross-check. Reuse the MFT walk's allocation
   visits — the cross-check is a bitmap fold over them, not a second walk.
4. Wire as an optional pre-scan stage after the repair stage (cr_scan.cpp
   stage model): report emitted as a new session record + summary stats
   field; NEVER aborts the scan (stance already matches).
5. Tests: `FatCopyDisagreementReported` (0x1005), `FatDuplicateRefDetected`
   (0x1006), `DirCycleDetected`, `LiveFileSizeNeChain` (0x1004),
   `OrphanClustersReported` (0x1003), `NtfsVersionGate` (0x100a — fixture
   with version word 0x400), `MftBitmapCrossLink` (0x1002),
   `BitmapFreeButAllocated` (0x1003), `CheckNeverAbortsScan`.

## Tests to add

The nine named tests; each fails-before by absence.

## Related findings

M-015 (FAT table validation — this finding extends it from "wrong-copy
selection" to "no audit layer"; keep both, different layers), M-043
(recognition probes — the analysers feed the same assessment lane),
M-040 (WinChkdsk delegation — the escalation endpoint), S-023 (grading —
the check report is an input the og's grading consumes).

## Open questions

Whether DRWUI changes scan strategy on specific check errors (the artifact
says structured→RawFixer→WinChkdsk→RAW; the exact error→strategy mapping
lives in the DRWUI check classes CCheckFatPartition/CCheckNtfsPartition
per rawfixer-fs-repair.md:15 — not decoded this session).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-041 commit, DFX30)
**Verified by:** the 15-test FsCheck suite (tests/unit/test_fscheck.cpp): FatCopyDisagreementReported (0x1005, param = the torn entry), FatDuplicateRefDetected (0x1006), DirDotPairMissingRejected + DirCycleDetected (0x1007), LiveFileSizeNeChain (0x1004, path hint), OrphanClustersReported (0x1003, param = the orphan cluster), CleanFatVolumeReportsNoErrors + DbrCeilingRejectsOversizedClusterCount (the clean/undetected pins), NtfsCleanVolumeReportsNoErrors, NtfsVersionGate (0x100A, param = the version word 0x400), MftRecordNotNamedMft (0x1008), MftBitmapCrossLink (0x1002, param = the doubly-claimed LCN, record context), BitmapFreeButAllocated (0x1003 NTFS form, param = the free-but-claimed LCN), CheckNeverAbortsScan (the orchestrator stage: scan returns OK with findings present, files still found), ReportSessionRoundTrip (the REC_FS_CHECK wire form). Each fails-before by absence (the module did not exist — the finding's own test-plan mode). Full suites: linux-release 758/758, linux-gcc ASan 758/758 (the same run carries the N-079 LSan repair), win_syntax_check green (the CLI/ABI files parse).
**Resolution notes:** New pure-core check module (`include/cr/check/cr_fscheck.h` + `src/core/check/cr_fscheck.cpp`) re-implemented from understanding of ghidra-mo/FileSystemCheck_decompiled.c (corpus 03ad46c; every gate re-verified first-hand this session — the DBR ceilings 341.3333333/0x100/>>(2) with the signature dispatch "FAT1"+digit@+0x36 / "FAT3"+'2'@+0x52, the dead-zone [cluster_count, 0xff6/0xfff6/0xffffff6) and duplicate-reference table scans, the entry-by-entry pairwise FAT-copy compare with the entry-0 media-signature mask (mismatch = silent stop, og parity), the FAT32 root-chain gate, the dir-tree audit ('.'/'..' pair for non-root dirs only, first-cluster == 1 or >= cluster space, the cluster-granular size-vs-chain compare (the og's sector-sum round-up decoded as ceil(size/cluster_bytes)), ancestor-array cycle detection, per-entry non-terminal errors), the orphan sweep (unvisited + FAT != 0/self/BAD -> host decision -> single terminal 0x1003), and the NTFS pass ($MFT record-0 resident-"$MFT" $FILE_NAME proof (0x1008, terminal), $MFTMirr slot-0 proof (0x1009, non-terminal), the version gate {0x102, 3, 0x301} from $Volume's $VOLUME_INFORMATION +8/+9 (0x100A with the version word as param), the $Bitmap structural floor bits >= MFT record count (0x100B), and the MFT x $Bitmap cross-check gated by $MFT::$BITMAP AND record flags&1 with per-LCN 0x1003 free-but-allocated / 0x1002 duplicate-claim — the og's own two-bitmap decode). The 8-event host callback surface (set_range/set_pos/on_error stop-decision/status/path + the orphan decision FUN_180002770) is modeled as optional function pointers. Wired as the optional pre-scan stage AFTER repair and BEFORE the engine pipe (ScanConfig::pre_scan_check, default OFF like pre_scan_repair — the og runs the check from its dedicated check/repair flows); surfaced via ScanStats.check_* + event::Type::FsCheck (wire value 10 — 8/9 are shim-side CR_EVT values) + the optional REC_FS_CHECK session record (round-trips with the load path restoring it) + the cr_ui ABI (cr_ui_scan_config.pre_scan_check, cr_ui_scan_stats check_ran/check_errors/check_detected, CR_EVT_FS_CHECK 10 with check_errors/check_family) + the CLI (`scan --check` with the full per-finding report). The og's termination semantics are preserved exactly: 0x100D/0x1006/0x1005 stop the FAT check (0x1001 reported, check continues); 0x1008 stops the NTFS check; 0x1009/0x100A continue; dir-audit errors are per-entry. Deliberate registered deltas (PARITY-DELTAS D-76): (a) the og's $Bitmap slack term (checkntfs.cpp :3869-3871, CCheckNTFSVolume+0x30) is layout-undecoded — only the firing half (bits < record count) is implemented, the slack constant is not guessed; (b) CheckIssue carries the FAT 0x1003 cluster and the claiming record number as superset context (the og's FAT 0x1003 passes param 0; the NTFS codes carry the LCN param as decoded); (c) a partial $Bitmap run load fails the load (0x100B path) rather than auditing against a zero-filled tail — the og's LoadVolumeBitmap failure shape. The safety stance (never abort the scan) is the orchestrator's pre-existing escalation posture, now pinned by test. Related: M-040 (the escalation ladder's boot-repair/WinChkdsk endpoints remain open), M-015 (the engine-layer FAT validation this audit layer complements), OQ-S5-1 (CheckVersion's consumer — unrelated to this phase).
