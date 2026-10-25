# CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

**Session-5 changes (2026-10-26, audited `7cf6754`):** the never-cited-artifact wave —
new findings **A-005 (ACUTE — rawfixer 4Kn stack-buffer overflow; the first ACUTE since
A-004), S-031, S-032, M-046..M-051, N-051..N-064, I-021, I-022**; OQ-S4-1 RESOLVED (the
169-entry grading table dumped byte-exact — S-023 addendum + evidence file); OQ-S4-2
RESOLVED-with-residual (the vtable+0x48 hook is a per-scan filter-config fetch, not a
namer — N-063 + corpus errata row 37); M-029/N-043 addenda (filterbar decode + liveness
evidence); corpus errata rows 37-40; three evidence files (grading-table dump, session-5
quotes pack, baseline 366/366 execution). New module coverage: `err` gains M-049 (via the
cross-cutting err/error-model row below); `win/elevation` + `destio` registered in I-021.

Legend: `M` = MATCH · `A-n`/`S-n`/`M-n`/`N-n`/`I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`ok` = MATCH-with-note (see module file). Full per-parameter expansion in each
`modules/<module>.md`.

**Session-5b changes (2026-10-26, audited `7cf6754` rebased onto `c75f646`):** one NEW
module row — `mom-capabilities` (first-time coverage of the never-dispositioned
MOM/product surfaces: apply-write, boot-repair, imaging, check-phase, recognition,
session-persistence, logging, mom-tool-ops, mom-runtime) with findings
**S-029, S-030, M-040..M-045, N-048..N-050, I-019, I-020** (13 new — renumbered
past the storage/census leg's S-024..S-028/M-031..M-039/N-045..N-047/I-017/I-018;
no content overlap); existing rows gain: ntfs += M-043 (content-proof recognition
+ scanner backup-boot), fat += M-045 (boot-gate family — resolves S-006's FATX
remainder: no Xbox parser exists), session += M-042/N-049 (lostpart.dat resume
re-validation + record richness), lost-partition += S-029 (apply/table-write
path). OQ-S4-1 RESOLVED (the 169-entry grading extension table dumped — `.data`,
unsorted; S-023's fix-plan step 3 input complete); OQ-S4-2 RESOLVED (the RAW
naming hook = single-site "filter_generalraw" display-name mapping; M-029 §Gap
(b) corrected); S-019's fix verified in-source; corpus errata rows 37-42 filed;
one new tracked-open OQ-S5-1 (CheckVersion's consumer). The stale duplicate
fat/exfat row pair (lines 44-45 vs 46-47, an old parallel-leg merge artifact)
consolidated — the S-013/S-021-current rows survive (f210b7e precedent).

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
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |

| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B1/B2/A25/D15 | **M-019** (multi-vdev: og attempts with first-entry fallback, fletcher4 verify + next-DVA retry; core refuses and never verifies — engages the parallel leg's "honest superset" stance with quoted evidence) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2), N-015 (S2)**, N-009d, **N-056 (S5: WinDestIo temp leak on failed rename)** | M (debt) |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| rawfixer | F60/Q67/Q65/A25 | **A-005 — ACUTE (S5)** (4Kn stack-buffer overflow: `u8 x[512]` buffers receive `sector`-byte reads at cr_rawfixer.cpp:558/574/590/639; transport reports 4096 on 4Kn; tests pin 512 — FIX-3 fixed offsets, not buffers) |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |

| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B1/B2/A25/D15 | **M-019** (multi-vdev: og attempts with first-entry fallback, fletcher4 verify + next-DVA retry; core refuses and never verifies — engages the parallel leg's "honest superset" stance with quoted evidence) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |

| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2), N-015 (S2)**, N-009d, **N-056 (S5: WinDestIo temp leak on failed rename)** | M (debt) |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| rawfixer | F60/Q67/Q65/A25 | **A-005 — ACUTE (S5)** (4Kn stack-buffer overflow: `u8 x[512]` buffers receive `sector`-byte reads at cr_rawfixer.cpp:558/574/590/639; transport reports 4096 on 4Kn; tests pin 512 — FIX-3 fixed offsets, not buffers) |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |
| abi (cr_ui) | NA | M | ok | NA | M | M | S-009 | M | M | M | **N-040 (S4: doc truthfulness), N-041 (S4: dead code), I-016 (S4: factory register + FS-code spaces)** |
| cli | N-009c | NA | M | NA | M | NA | M | NA | M | M | ok; N-020 (S2); **N-044 (S4: --deep naming hazard)** |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2), N-019 (S2), N-020 (S2)**; M-006 FIXED |
| preview (S4 NEW) | NA | NA | NA | NA | NA | NA | NA | NA | NA | **M-024 (S4: caps bypass — executed proof; provenance misattributions)** | I-013 (S4: architecture register — og socket protocol vs in-process reader) |
| query (S4 NEW) | **M-029 (S4: sort/paging/bucket-IDs/server-side push)** + ~~M-023~~ FIXED (FIXED/M-023: category source now the generated table) | NA | NA | NA | NA | NA | NA | NA | NA | **M-029, N-040 (frozen-doc unit lie)** | M-029 (S4); S-023 grading rows now carry grades + sort key 6 |
| transport (S4 NEW) | M-030 (S4: WinPE gate, stale-handle ladder, terminal OFFLINE, 32-disk cap) | M-030 | M-030 (IOCTL numbers = own driver's, grammar shape M) | M-030 (chunking provenance corrected; OQ-S4-3 perf open) | M-030 | NA | NA | M-030 (silent RDA fallback) | M-030 | M | **N-041 (S4: dead RdaClient + volume_letter_to_disk)**; I-016 (S4: wire register) |
| mom-capabilities (S5b NEW) | **M-043 (S5b: NTFS content-proof + backup-boot)**; **M-044 (S5b: UDF family)**; **M-045 (S5b: FAT gates)**; N-050 (S5b: boot-only recognition) | **M-042 (S5b: lostpart persistence semantics)**; N-049 (S5b: record richness) | I-020 (S5b: corpus-doc corrections register) | **S-029 (S5b: apply/table write)** | **M-040 (S5b: BCD/NVRAM/boot.ini/chkdsk family)**; **M-041 (S5b: check phase + error taxonomy 0x1001-0x100e)** | **S-030 (S5b: image scan targets)** | **N-048 (S5b: logging seam)** | **I-019 (S5b: EPM tool-ops scope register)**; OQ-S5-1 (CheckVersion consumer) | NA (pending scope decisions S-029/M-040) | NA |

**Session-5 fix note (2026-10-26):** S-021 + S-003 FIXED (HFS+ journal
differential harvest — PARITY-DELTAS D-56; the fallback residual tracked in
OQ-S003-journal-fallback-salvage.md).

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

| Module | Parameters | Finding |
|---|---|---|
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B1/B2/A25/D15 | **M-019** (multi-vdev: og attempts with first-entry fallback, fletcher4 verify + next-DVA retry; core refuses and never verifies — engages the parallel leg's "honest superset" stance with quoted evidence) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| orchestrator/wide | B1/D16 | **M-018** (FilterFolder junk-filter family absent and unregistered) — adjacent minors **N-016 + N-017 FIXED (minors session 2026-10-26)** (teardown double-emit removed, count pinned; CancelPoolWatcher factored + wired at the candidate fan-out — D-54) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |
| cross-cutting | B1/F59/F61/F62/Q46 | I-021 (S5) scope/deliberate-delta registers (og always-elevated manifests, EuSendMessage installer-only, repair toggle, CRecoveryFilter layering, PENetwork SMB, DRWCfg.bin) |
| cross-cutting | B1/F59/F61/F62/Q46 | I-021 (S5) scope/deliberate-delta registers (og always-elevated manifests, EuSendMessage installer-only, repair toggle, CRecoveryFilter layering, PENetwork SMB, DRWCfg.bin) |

**Retracted by this leg's own re-verification (recorded, not filed):** the exFAT
ChecksumDBR "no-skip" candidate — the og's fixer skips {106,107,112} via a
`std::find == end()` gate over the −1-terminated table at `DAT_180079a40`; the core's
spec-skip is a MATCH (the parallel leg's binary extraction was right; this leg's first
reading was wrong — corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`).

# S3-parallel-leg cells (session 3, second continuation leg — wave 1 + residues; core `34b3441`)

| Module | Parameters | Finding |
|---|---|---|
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission) |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| cross-cutting | B1/F59/F61/F62/Q46 | I-021 (S5) scope/deliberate-delta registers (og always-elevated manifests, EuSendMessage installer-only, repair toggle, CRecoveryFilter layering, PENetwork SMB, DRWCfg.bin) |

*(Renumber map from the pre-merge wave-1 IDs: S-012→S-017, S-013→S-018, S-014→S-019,
S-015→S-020, M-011→M-024, M-012→M-025, M-015→M-026, M-016→M-027, N-025→N-033,
N-027→N-034, N-028→N-035, N-030→N-036, N-031→N-037, I-011→I-013; merged as
addenda: dispatch→N-031, video-feed→M-021, scoring/merge→M-017, resident-bodies→M-022,
byte-pin→I-009, probe-row→I-010; dropped as dups: boot-geometry→N-013b, byte-granularity→I-009.)*

## Session-5 detail rows (2026-10-26, audited `7cf6754`)

| Module | Parameters | Finding |
|---|---|---|
| rawfixer | F60/Q67/Q65/A25 | **A-005 — ACUTE (S5)** (4Kn stack-buffer overflow: `u8 x[512]` buffers receive `sector`-byte reads at cr_rawfixer.cpp:558/574/590/639; transport reports 4096 on 4Kn; tests pin 512 — FIX-3 fixed offsets, not buffers) |
| carve+session+query | B4/D11/D12/B1 | **S-031 (S5)** (carved files never marked deleted: og FLAGS=0x0020 constant on all 12,147 sampled records; core's deleted_only filter and deleted_count exclude the entire carve set) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| ldm+rawfixer | C20/B2/A25/F60 | **M-046 (S5)** (4Kn wrong-offset/truncated-plan: LDM GPT path hardcodes `*512` at 3 sites vs the geometry-correct main parser; exFAT plans write 512 of 4096 bytes + wrong checksum span) |
| session+tree | D11/D12/B4 | **M-050 (S5)** (node-model round-trip gaps: ~15 og wrapper fields — hidden-root bit, grading bits 5-7, roles, fixup, sentinels — have no core carrier) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| err+abi | C23/D14/F35/Q46 | **M-049 (S5)** (error model: flat 28-code 8-hex enum with a false "reproduces the reference convention" header claim vs the og's 12-digit documented codes + error-object architecture) |
| session+tree | D11/D12/B4 | **M-050 (S5)** (node-model round-trip gaps: ~15 og wrapper fields — hidden-root bit, grading bits 5-7, roles, fixup, sentinels — have no core carrier) |
| carve+session | D11/D12/B9 | **M-051 (S5)** (RAW provenance unpersisted: timestamp string, analyser checksum w/ 0xFFFFFFFF invalid marker, note, tail anchor, boolean) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| writer | F62/Q65/A35 | N-056 (S5) WinDestIo .part-N temp leak on failed rename + UNC untested |
| table+visualization | Q65/A54/Q69 | N-057 (S5) TableLayout::Ebr dead + diskmap overlay/budget test debt |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| session+btrfs+zfs | D11/D12 | N-059 (S5) btrfs secondary ids/generation + zfs secondary flags unpersisted |
| session+orchestrator | F32/F38/F41 | N-060 (S5) save_session progress/cancel contract absent (og 40%→65% per-node + IsCancelled) |
| orchestration+session | F35/Q46/D12 | N-061 (S5) no persistent engine run-log + no auto_<label><M><D><H><m><s> session auto-naming |
| orchestration | F35/P49 | N-062 (S5) no memory monitor (og CMemMonitorManager) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| health/bitlocker | F61/B1 | N-064 (S5) Dict password-dictionary unlock aid absent (228 entries, manage-bde -Password; consent-gated port required) |
| cross-cutting | B1/F59/F61/F62/Q46 | I-021 (S5) scope/deliberate-delta registers (og always-elevated manifests, EuSendMessage installer-only, repair toggle, CRecoveryFilter layering, PENetwork SMB, DRWCfg.bin) |
| session/carve/btrfs/zfs | D12/A27/Q67 | I-022 (S5) core-superset registers (CRC32/resync/bomb-caps, structured btrfs extents, decoded zfs runlists, UTF-16 unification, extra props, core-only formats) |
## Session-6 verdict changes (deep continuation, 2026-10-26, audited `022c81d`-lineage / corpus `03ad46c`)

The never-cited-artifact lanes executed (session-5's A-H lanes, defined at `2001f7c` but never
run, were this session's scope): the 6 RAWSearchDLL carving-family artifacts + registry
census/dual-registry; the SaveScan record-4/IFileProperties-part3 grammar; the per-engine
function map; the MOM/WinPE/config/UIUX-wiring 16-artifact set; and the core's 8 never-cited
files. Every load-bearing citation verified first-hand before filing (three stale agent claims
filtered by verification: FAT alternate-EOC — N-002's fix is live; NTFS BAAD — MATCH
not-BAAD both sides; FAT quick-scan threading — already registered D-30x).

| Module | Parameters | Finding |
|---|---|---|
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| carve | B10/A28/Q68 | N-063 (S5) default_namer "File%06u" is a core invention (og: per-analyser content names + "Lost Name File (...)_EaseUS"); OQ-S4-2 resolution |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| session/tree | D11/D12/P42 | **M-051+N-059 (converged)** (type-registry ID + per-FS secondary payload slots missing) |
| writer | F62/Q65/A35 | N-056 (S5) WinDestIo .part-N temp leak on failed rename + UNC untested |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| storage | B8/P42/A54 | **N-071** (enumeration breadth: 5-stack fan-in vs PhysicalDrive loop) |
| storage | P47/P48 | **N-045 addendum** (cache ALGORITHM: og prefetch read-ahead vs core reactive LRU; cite refreshed to :647/256 KiB) |
| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B1/B2/A25/D15 | **M-019** (multi-vdev: og attempts with first-entry fallback, fletcher4 verify + next-DVA retry; core refuses and never verifies — engages the parallel leg's "honest superset" stance with quoted evidence) |
| lost-partition | A29/C20 | **N-035** (candidate extents data-area-anchored vs whole-volume — M-017's containment/16-MiB rules consume different extents) |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |
| query | P42 (sort Type) | **M-029 addendum** (path_prefix landed and verified in-tree — path-group sub-item CLOSED; Type key/bucket IDs/paging/server-push remain) |
| abi (cr_ui) | NA | M | ok | NA | M | M | S-009 | M | M | M | **N-040 (S4: doc truthfulness), N-041 (S4: dead code), I-016 (S4: factory register + FS-code spaces)** |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2), N-019 (S2), N-020 (S2)**; M-006 FIXED |

New tracked-open: **OQ-S6-1** (PartitionRecoveryD generator-set semantics), **OQ-S6-2**
(Btrfs RC4 target), **OQ-S6-3** (CRAW +0xF4/anchor-B/checksum consumption) — 14 tracked-open
files total. Session-6 verified-fixed addenda: M-029 (partial), N-033 (partial), N-045 (cite),
M-017/M-030/S-007/S-009/S-024 (evidence sharpened).
