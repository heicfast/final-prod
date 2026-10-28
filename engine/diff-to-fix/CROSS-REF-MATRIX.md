# CROSS-REF-MATRIX.md — module × parameter verdict matrix (the SA-10B lane on top: the real-rsf mining findings; the prior sessions below)

**SA-10B changes (2026-10-29, the B lane — the real-.rsf session mining; evidence: evidence/executions/s10b-real-rsf-session-mining.md):**
- **carve row, new cells**: tree-shape/naming parameters — **S-048** (the
  `Reconstructed/<volume>` per-volume grouping absent; B8/A26/A42/D12/D13
  cells), **S-049** (the `<PREFIX>_%06d.EXT` naming contract + the
  content-derived prefixes EXIF-Model/Software, ID3-title, PDF/DOCX-timestamp,
  ZIP-entry, SOF-dims — B10/A28/D12 cells; N-063's remainder resolved).
- **ntfs/tree rows, new cells**: orphan-grouping parameters — **M-103**
  (`LostDir<record>` per-parent folders + the "File Path Lost" branch name;
  B5/B10/D12/A26 cells; the C2 "roles map 1:1" claim falsified for the lost
  branch).
- **mom-capabilities/product-frame row, new cell**: **S-050** (the file-repair
  line — 9 binaries, 8 artifacts, unimplemented AND untruthfully registered;
  A42/Q68/I-010 cells).
- Executed MATCH recorded: the raw_signs.json regen sync (139/199/62
  byte-identical; the carve row's C19-C21 cells).
- **The FIXED purge (L-A)**: findings/FIXED/ retired to history; all matrix
  cells citing FIXED IDs resolve via FIXED-LEDGER.md.

---
# (prior header) CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

**Dev-sign session changes (2026-10-27/28, SA lane — Phase 1, audited `dd82db9`):**
NEW module row **driver-install/dev-sign** (the packaging surface the corpus
documents via 02-kernel-driver-architecture.md §5 + regdrv.reg — prior
sessions never audited install/uninstall/signing posture):
executability **S-036** (installer crash — the cell is the live divergence),
service-configuration **M-060** (group/Parameters/binPath divergence),
uninstall-behavior **M-061**, install-error-semantics **M-062**,
environment-matrix/HVCI **M-063**, signature-lifetime **N-084**,
installer-interaction-model **N-085**, signing-posture **I-026** (deliberate,
MATCH-equivalent). The corpus's other installer parameters — copy destination
(`System32\Drivers`), service shape (Type=1/Start=3/ErrorControl=1),
product-orchestrated lifecycle — are **MATCH** (install.ps1/composite comply;
see the per-cell detail at the bottom). Evidence:
evidence/executions/devsign-run-40549660074/.

**Session-27 changes (2026-10-26, DFX27 — the MAJOR-head lane):** **M-035
FIXED** (engine-side FileFilter validation — the exFAT-only content-consistency
scorer, reject rule decompile-exact with the strict-< correction; scan_candidate
wiring + ResultTree::detach_subtree; lost-partition A30/B3 and query B16 rows
below; findings/FIXED/M-035). **M-058 NEW-FINDING FIXED** (build-integrity/
memory-safety: run()'s cache-failure fallback UAF — build_read_pipe extracted,
ASan fail-before executed; the orchestrator row's R1.6 pipe note). M-049 landed
first from the parallel DFX24 lane (convergence honored).

**Session-30 changes (2026-10-26, DFX30 — tracked-open tail lane):** OQ-FLT-1
RESOLVED (the 169-entry extension table = a 4-engine shared vocabulary, live in
FAT/exFAT/RAW, dead copy in NTFS, absent in 5 engines; the M-035 scorer is
exFAT-only — its Open-questions cell answered); OQ-Z5 RESOLVED (ZFS quick/
complete = the IQuick/ICompleteFileSystemSearchController plugin split, 16 vs
34 vtable slots — core orchestrator is the union, no action); OQ-Z2 RESOLVED
with NEW-FINDING **S-034** (SEVERE — the og walks EVERY uberblock in the
label-0 ring per txg through a per-txg VdevLoc::UberData registry; the core's
newest-txg-only premise REFUTED; D-05 re-verdicted; corpus erratum E45). The
zfs B/D cells: S-034 FIXED (DFX37) — MATCH with the registered emission-dedup divergence.

**Session-31 change (2026-10-26, DFX31 — MINOR tail-first lane):** **N-064**
FIXED — the Dict password-dictionary unlock aid: the generated 228-entry
table + the pure-core parse/attempt loop (cr::dict) + the win-layer
consent action (unlock_with_dictionary, the PowerShell/manage-bde
-Password plumbing) + the CLI `--bitlocker-dict` consent flag + the
cr_ui_unlock_volume_with_dictionary additive ABI export. F61 consent gate
structural; health/bitlocker F61/B1 cells -> MATCH. **N-071** FIXED —
the two-stack disk enumeration (cfgmgr32 disk-interface pass merged into
the numbered loop; residual stacks registered as D-79); storage B8/P42/A54
-> MATCH-with-D-79. **N-059** FIXED — the per-FS session extras
(Btrfs/ZFS/HFS+ secondary payload carriers + engine emission + the
nflags bit-6 append-only block); session D11/D12 cells -> MATCH; the
N-019/M-020 convergence break repaired in the same landing. **N-066**
FIXED — the identity card's record-3 parity (v4 tail: firmware/WWN/
interface + the boot snapshot; session D12/B9 -> MATCH) + **N-080**
(NEW-FINDING) — the descriptor offset transposition fixed (storage D11
axis -> MATCH).

**Session-30b changes (2026-10-26, DFX30-MAJOR-tail lane — this lane):**
**M-041 FIXED** — the og's FileSystemCheck parity module
(`include/cr/check/cr_fscheck.h` + `src/core/check/cr_fscheck.cpp`): the full
error taxonomy 0x1001..0x100e, the FAT pass in og phase order (DBR ceilings
341.3333333/0x100/>>(2), dead-zone + duplicate-ref scans, pairwise copy
compare, dir-tree audit, orphan sweep with the host decision), the NTFS pass
($MFT/$MFTMirr proofs, version gate {1.2, 3.0, 3.1}, $Bitmap structural,
the $MFT::$BITMAP-gated MFT×$Bitmap cross-check 0x1002/0x1003), the 8-event
callback surface, the never-abort wiring (ScanConfig::pre_scan_check after
repair), REC_FS_CHECK, the cr_ui ABI appends (union with the parallel
M-049 error-context append) + CLI `scan --check`. New module
row: `check-phase` (modules/check-phase.md); deliberate deltas registered as
D-87 (the undecoded slack term, the param superset, the partial-load refusal).
15 fail-before tests; suites 758/758 release + ASan. **N-079 CONVERGENT**:
this lane's leak repair (eb3cb6d) reconciled to the parallel wave's canonical
deque-pool implementation (first-landing-wins; findings/FIXED/N-079 is the
parallel lane's file — this lane's convergence record rides the worklog).

**Session-25 changes (2026-10-26, DFX25 — INFO tier closed into FIXED/, tracked-open resolution wave):**
the 22 acknowledged INFORMATIONAL findings (I-001..I-024) moved from
`findings/INFORMATIONAL/` to `findings/FIXED/` — the directory is now empty
and the `ack` cells below denote acknowledged-and-filed-in-FIXED deliberate
divergences (each file carries its ACKNOWLEDGED Resolution; pin tests green on
`716d121`, 737/737). No verdict changed — `ack` was and remains MATCH-by-
deliberate-divergence; only the filing location moved per the user directive.
Tracked-open dispositions recorded per-OQ in the session's worklog entries.
**Session-24 changes (2026-10-26, DFX24 — the disjoint MAJOR lane; DFX23 owns the MINOR queue
in the same window):** **M-049 FIXED** — the error-model row below (err+abi): the session-11
half (header truth + err_text coverage) + this session's close-out of the finding's registered
step 2, the engine-side error-context subset (the og's per-module error registrars, Error.md
census): the error-range event family carries the module/operation/retry-state triple
(`err_module`/`err_operation`/`err_attempts` on `cr_ui_event`, CR_ERRMOD_*/CR_ERROP_* ABI
constants pinned by static asserts), attributed per-range at failure time (cr::errmod thread
stamp + device-owner fallback for engine-internal pools); sessions keep the geometric trio
(wire-compat pinned both directions); D-68 updated. Fail-before RED executed at the retry-device
seam (5/6); suite 745/745.

**Session-22 changes (2026-10-26, DFX22 — build-integrity NEW-FINDING + the carve overlap/scoring wave):** **M-055**
(MAJOR) — the `src/win` platform layer was UNCOMPILABLE since `3b5397a`/`915bce3`/
`d8f6808` (four structural breaks; see the finding for the per-commit decomposition);
Q67 buildability now MATCH (win_syntax_check parse gate on the Linux lane — the
fail-before/pass-after proof is in the finding). The writer+report and quality rows
below carry the M-055 cells.

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
- Acknowledged (filed in FIXED/ since session 25, with pin tests): I-001..I-005.
- S-009 partially FIXED (disk-map visualization via GAP-S6-11); the combined
  S-009/M-006 finding file remains in SEVERE/ pending split (tracked in N-020).

| Module \ Group | B | D | C | A | S | O | P | X | R | F | Q |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ntfs | M-007, M-008 (S2); N-011 FIXED-ALREADY + N-012 FIXED (minors session 2026-10-26, D-52); I-001 ack; S-012 FIXED (two-pass: pass-1 membership + pass-2 dump walk + size-0 rescue; convergent dual-agent fix); ****M-043 FIXED (s10: backup-DBR fallback + MFT content proof — findings/FIXED/M-043)** (S5b: content-proof recognition + scanner backup-boot)** | M (S-001+S-012 fixed: membership keep), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M; N-021 (S2); **M-045 FIXED (s10: validate_boot any-512-multiple bps + numFATs + cluster-cap; classifier already N-034 — findings/FIXED/M-045)** (S5b: boot-gate family — resolves S-006's FATX remainder) | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M-013 FIXED (s16: the dual-form deleted-set checksum gate + the parked-set reconstruction oracle — FUN_18001eba0/FUN_1800203a0 decompile-exact; findings/FIXED/M-013), M-014 FIXED (s35: structural entry gates — FUN_180002f90/FUN_180029280/FUN_180003030 decompile-exact; findings/FIXED/M-014), M (S-011 + S-013 fixed), N-003 FIXED (premise recorded REFUTED by M-013), OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | **M-054 (S10 NEW-FINDING) FIXED — allocation-bitmap bit i vs cluster i+2 shift (grading + the S-009 producer wrong on real volumes; spec §7.1; producer pin test added)**; M (debt) |
| ntfs | **M-007 FIXED (session 11: single_link_per_record parity default + the repaired per-parent fan-out, D-65)**, M-008 (S2); N-011 FIXED-ALREADY + N-012 FIXED (minors session 2026-10-26, D-52); I-001 ack; S-012 FIXED (two-pass: pass-1 membership + pass-2 dump walk + size-0 rescue; convergent dual-agent fix); **~~M-043~~ **FIXED (s14, FIXED/M-043: content proof at probe; backup-fallback premise corpus-disproven)** (S5b: content-proof recognition + scanner backup-boot)** | M (S-001+S-012 fixed: membership keep), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M; N-021 (S2); **M-045 (S5b: boot-gate family — bps/SPC/label/numFATs/cap/band-retry; resolves S-006's FATX remainder)** | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M (S-011 + S-013 fixed), N-003 FIXED, OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | M (debt) |
| ext4 | S-004 FIXED, S-025 FIXED (two-pass reparenting, D-58/59 siblings), S-026 FIXED (meta_bg resolver, D-58), S-027 FIXED (backup-SB rescue, D-59); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks; S-027 fixed: backup-SB positions + s_block_group_nr) | S-004 FIXED, S-025 OPEN | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, **S-003 FIXED (s5 + closing-session verification: JIB layout corrected, gates, strict validator, fallback salvage)**, **S-021 FIXED (s5 + closing-session verification: attributes branch + marker derivation resolved)**, N-005 FIXED, N-038 FIXED, **A-007 FIXED**; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) **N-047 (S3) FIXED s22** (volume-node naming = registered partition-layer divergence, modules/hfsplus.md) |
| ext4 | S-004 FIXED, S-025 FIXED (journal reparenting: orphan map + iNode naming + claim interaction), S-026 FIXED (meta_bg resolver, D-58), S-027 FIXED (backup-SB rescue, D-59); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 meta-group desc blocks; S-027 backup-SB positions + s_block_group_nr) | S-004 FIXED, S-025 FIXED (two-pass realized as harvest+claim checks; parent map + topo anchors) | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, **S-003 FIXED (s5)**, **S-021 FIXED (s5 — journal harvest)**, N-005 FIXED, N-038 FIXED; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) || btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010 FIXED (lossless mapping)**, I-007 (S2) | M; **M-010 FIXED**, I-007 (S2); **S-028 FIXED** (xattrs: DOSATTRIB bits + user.* records + the compression-property tag) | M | M | M | NA | NA | M | M | M | M |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2); S-034 **FIXED (S33b: every ring state walked newest-first + identity dedup; findings/FIXED/S-034)** | S-034 **FIXED (S33b: deleted-in-window files surface as Deleted nodes — RingWindowDeletedFileRecalled et al.)** | OQ-T6 | M | M | M (S-034: deleted-in-window recall; beyond-window via carve) | NA | M | M | M | M || refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved); **S-020 FIXED (s4)**; **M-026/M-027 FIXED (s4: stale-page sweep + identity keep-one + recycle routing)** | M | M | **M-022 FIXED (s4)** | M | NA | NA | M | M | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-031 FIXED (carved nodes flag Deleted - og FLAGS=0x20 const); S-007 (primary FIXED s5), M-005 | M | S-007, **N-008 FIXED s11** (sector-strided salvage + no clean-archive inflation + ZIP64 chain; gate half via N-068); M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s10)** (consent-gated apply module + `rrecov apply`; findings/FIXED/S-029) | M | NA | NA | M | M | M | M **N-035 (S2) FIXED s22** (data-area-anchored containment/overlap extents; both extents on Candidate; fail-before executed) |

| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s10)** (consent-gated apply module + `rrecov apply`; findings/FIXED/S-029) | M | NA | NA | M | M | M | M |

| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s16: the apply/table-write path — consent-gated transactional writer, corpus IOCTL chain, undo; findings/FIXED/S-029)** | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | M | M (D12), **N-014 (S2) FIXED s22** (atomic staging + commit-by-rename) | **M-042 (S5b: lostpart.dat resume re-validation + layout snapshot missing)**; N-049 (S5b: record richness) | NA | M; N-014 (S2) | NA | NA | M | M | I-005.7 ack | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2) FIXED s7 (FIXED/ move s11)**, **N-015 (S2) FIXED s9 (`915bce3`; FIXED/ move s11)**, **M-055 (S22 NEW-FINDING) FIXED — the win layer those fixes landed into was uncompilable; four structural repairs + the win_syntax_check gate** , N-009d | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock; N-064 FIXED s31 — the Dict password-dictionary unlock aid, consent-gated) | NA | ~~N-009a~~ FIXED (u8 width, s2-wave; group closed s23); N-055 (S5) FIXED by the S-024 fix leg (0xD1 + 4-status grading) | NA | **M; S-024 (S3) FIXED — decoded-truth upgrade (s6)**: FUN_0040cdf0 exact (family-gated SSD-life, bad-count/caution/graded aggregation, temp never graded, NVMe family-0x13 path; analogs superseded — S-024 Resolution addendum + evidence s6-health-grading-decode.md) | NA | NA | NA | M | M | M (M-036 FIXED s6: NVMe full row set + units→bytes; ABI v2); **N-032 (S3) FIXED s30 (CSMI CC-0x19 transport wire-decoded + the pure three-transport chain seam; findings/FIXED/N-032; D-78)**; **N-042 (S4) FIXED s7 `3678cbf` (FIXED/ move s11)** |
| rawfixer | M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | M | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | ~~M-005~~ **FIXED (s14, FIXED/M-005)** | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4) FIXED** (FIXED/S-023: post-scan grading pass + RecoveryAssistant statistics + ABI regrade); ~~N-043~~ **FIXED (s18 DFX18: CR_EVT_HEARTBEAT — the honest 5 s liveness watchdog, standing truth not synthetic advance; TSan-clean)** | NA | M | M | M | M |
| visualization | S-009 (partial-FIXED-in-tree, finding file stale — N-020) | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M |

| ext4 | S-003 FIXED (HFS+ journal differential harvest, landed with S-021), S-004 FIXED, S-026 FIXED (meta_bg resolver, D-58); **M-010**, I-006 (S2), I-007 (S2); **M-037 FIXED (s10: resync-slide damage model — findings/FIXED/M-037)** | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks) | S-025 OPEN (theirs: journal reparenting), S-004 FIXED | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, S-003 FIXED, S-021 FIXED (s5 — journal harvest, the og's PRIMARY deleted surface), N-005 FIXED, N-038 FIXED; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) |
| btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010**, I-007 (S2) | M; **M-010**, I-007 (S2) | M | M | M | NA | NA | M | M | M | M **N-047 (S3) FIXED s22** (ROOT_REF subvol names + 0x80 file attribute base, corpus :3574-3577; both fail-before-proven) |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2); S-034 **FIXED (S33b: every ring state walked newest-first + identity dedup; findings/FIXED/S-034)** | S-034 **FIXED (S33b: deleted-in-window files surface as Deleted nodes — RingWindowDeletedFileRecalled et al.)** | OQ-T6 | M | M | M (S-034: deleted-in-window recall; beyond-window via carve) | NA | M | M | M | M || refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved — parity); **S-020 FIXED (s4: resident+all-rows+paged+granularity)** | M | M | **M-022 FIXED (s4: dual grammar + gen tag)** | M | NA | NA | M | **M-031 FIXED (s10: per-file progress ticks — findings/FIXED/M-031)** | M | M |

| zfs | OQ-T6, M (S-005 fixed); I-007 (S2) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | **N-007 FIXED s34 DFX34 (first-DWORD discovery + signature_partial grading flag, fail-before executed; findings/FIXED/)**, I-004 ack (D-48b preserved), **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved — parity); **S-020 FIXED (s4)**; **N-036 FIXED s23 (the container-map half: og formula + populated map, fail-before executed; findings/FIXED/N-036)** | M | M | **M-022 FIXED (s4: dual grammar + gen tag)** | M | NA | NA | M | **M-031 FIXED (s10: per-file progress ticks — findings/FIXED/M-031)** | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), **M-002 FIXED (s10: torn chain keeps walked prefix — findings/FIXED/M-002)** | M | M (A-002 FIXED; A-001 fixed), **M-046 (S5: LDM GPT-path 512-hardcodes)** | M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007, ~~M-005~~ **FIXED** (FIXED/M-005, s14: the allocation-bitmap gate in both dispatch layers + telemetry; bench 1.88x on 50% used), **S-031 (S5: carved nodes never marked deleted)**, **S-032 (S5: archive inner-entry inventories)** | **S-031 (S5)**, **M-051 FIXED (s9)** (checksum/anchor/note/flag carriers; nflags-bit4 round-trip) | S-007, N-008, **N-051/N-052/N-053/N-054 S12 FIXED (convergent landings: the parallel session-11/7 lane's b2f48c6 canonical + this session's N-076 add-on)**; M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005, S-032 (S5: multi-part stitching) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | **N-063 FIXED s17 (og per-analyser names; File%06u = registered default)**, **M-048 (S5: user-signature extensibility)** | M |

| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007 **FIXED (primary set + full audio family)**, M-005 | M | S-007 **FIXED** (niche ~25-format tail precisely scoped in the finding), **N-008 FIXED s11** (sector-strided salvage + no clean-archive inflation + ZIP64 chain; gate half via N-068); M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | M | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | **M-047 (S5: stats round-trip asymmetry)**, **M-050 (S5: node-model carriers)**, S-031-adj (S5) | M-047, M-050, **N-059 (S5: btrfs/zfs extras)**, ~~N-014~~ FIXED, ~~N-049~~ **FIXED-ALREADY s23 (M-042's v2 payload verified + SizeSourceFlagRoundTrip pin added; findings/FIXED/N-049)** | M | NA | M; N-014 (S2); **N-058 (S5: load validation deltas)** | NA | NA | M | ~~N-060~~ **FIXED (s18 DFX18)** | I-005.7 ack; ~~N-061~~ **FIXED (s18 DFX18)** | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2) FIXED s7 (FIXED/ move s11)**, **N-015 (S2) FIXED s9 (`915bce3`; FIXED/ move s11)**, ~~N-009d~~ **FIXED s23 (D-77 registered — inherit-destination ACL posture; findings/FIXED/N-009)**, **N-056 (S5) FIXED (s17)**, **M-055 (S22 NEW-FINDING) FIXED — win-layer buildability restored + gated** | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock; N-064 FIXED s31 — the Dict password-dictionary unlock aid, consent-gated) | NA | ~~N-009a~~ FIXED (u8 width, s2-wave; group closed s23); N-055 (S5) FIXED by the S-024 fix leg (0xD1 + 4-status grading) | NA | **M; S-024 (S3) FIXED — decoded-truth upgrade (s6)**: FUN_0040cdf0 exact (family-gated SSD-life, bad-count/caution/graded aggregation, temp never graded, NVMe family-0x13 path; analogs superseded — S-024 Resolution addendum + evidence s6-health-grading-decode.md) | NA | NA | NA | M | M | M + **N-032 (S3) FIXED s30 (CSMI CC-0x19 transport wire-decoded + the pure three-transport chain seam; findings/FIXED/N-032; D-78)**; **N-042 (S4) FIXED s7 `3678cbf` (FIXED/ move s11)** |
| rawfixer | **A-005 (S5 ACUTE: 4Kn stack-buffer overflow)**, M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | **M-046 (S5: 4Kn exFAT plan truncation)** | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | ~~M-005~~ **FIXED (s14, FIXED/M-005)** | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4: no post-scan grading engine / RecoveryAssistant — S5 addendum: OQ-S4-1 RESOLVED, table dumped)**; ~~N-043~~ **FIXED (s18 DFX18: CR_EVT_HEARTBEAT — the honest 5 s liveness watchdog, standing truth not synthetic advance; TSan-clean)** | NA | M | M | ~~N-061~~ **FIXED (s18 DFX18)**, ~~N-062~~ **FIXED (s18 DFX18: the memory monitor on the N-048 seam — check_memory_pressure + the platform sampler on the watchdog cadence + memory_warn_mb appended config; never scheduling-coupled; fail-before RED executed)** | M |
| visualization | **S-009 FIXED (S8 — the free overlay: engine bitmap truth produced + persisted + snapshot-carried; `kCellAllocFree` now real)**; ~~partial-FIXED-in-tree~~ | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M; **N-057 (S5: Ebr enum + budget-growth test debt remain; the bad-overlay coverage item paid by S-009)** |
| abi (cr_ui) | NA | M | ok | NA | M | M | **S-009 FIXED (S8: the free overlay rides the same `cr_ui_disk_map` ABI — no signature change; snapshots carry errors/free-map/view)** | M | M | M | ~~N-040~~ **FIXED (s18 DFX18: D4 closed — truthful era labels + §9 coherence; D1/D2/D3/D5 verified already-landed via §8/S-009/§6/M-029; three new pins incl. the fail-before-proven AbiVersionAnnotationsAreTruthful)**; N-041 (S4: dead code); I-016 (S4: factory register + FS-code spaces) |
| orchestrator (N-050 row) | **N-050 FIXED s23** (two-stage recognition: exFAT + FAT content proofs, contract documented in ground-truth-contracts §0) · ~~N-009c~~ **FIXED s23 (cr::text::wide_to_utf8 + the shared ASCII-ci contract; findings/FIXED/N-009)** | NA | M | NA | M | NA | M | NA | M | M | ok; N-020 (S2) FIXED; **N-044 (S4: --deep naming hazard)** |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2) FIXED s7 `b1f9210` (FIXED/ move s11)**, N-019 (S2), **N-020 (S2) FIXED (doc-accuracy batch; FIXED/ move s22)**; M-006 FIXED; **M-053 (S8 NEW-FINDING) FIXED — the C-ABI consumer check itself was dead since M-015 (stale fixture → BAD_MAGIC + NULL-deref crash on every run, both lanes); fixture fixed + precondition bail-out, gate live again**; **M-055 (S22 NEW-FINDING) FIXED — win-layer buildability: four structural breaks repaired + the win_syntax_check parse gate (Q67)**; **M-057 (S23 NEW-FINDING) FIXED — app-layer buildability: cmd_apply's undeclared `&e` (MSVC-uncompilable since `0a68937`) + the parse gate extended to src/app (Q67 now covers every Windows-only directory)** |
| preview (S4 NEW) | NA | NA | NA | NA | NA | NA | NA | NA | **M-028 (S4: caps bypass — executed proof; provenance misattributions) FIXED (s21 — read_cost-gated budgets + checked_read; findings/FIXED/M-028)**; M-024 landed s15 (CarveFlags::disabled_formats) | I-013 (S4: architecture register — og socket protocol vs in-process reader) |
| query (S4 NEW) | ~~M-029~~ **FIXED (s32 DFX32: bucket-ID translation + general-raw naming — the decode-blocked remainder closed with the DRWUI first-hand decode; Resolution in FIXED/M-029)** + ~~M-023~~ FIXED (FIXED/M-023: category source now the generated table) | NA | NA | NA | NA | NA | NA | NA | NA | ~~M-029~~, ~~N-040~~ FIXED | M-029 (S4); S-023 grading rows now carry grades + sort key 6 |
| transport (S4 NEW) | **M-030 — FIXED (session 23)** (WinPE gate + stale-handle ladder [prior leg] + terminal OFFLINE + restore chain + 200-disk bound; OQ-2/OQ-3 decoded first-hand; findings/FIXED/M-030) | M-030 **FIXED (s23)** (the PE probe + direct-first on both open paths) | M-030 (IOCTL numbers = own driver's, grammar shape M) | M-030 (chunking provenance corrected; OQ-S4-3 perf open) | M-030 **FIXED (s23)**; **S-030 FIXED (S8 — image scan targets: flat/VMDK/VHD through `cr/storage/cr_image`, CLI --image, session v3 identity tail; D-64 registers edisk/dmg exclusions + VHDX/parent-chain remainders)** | NA | NA | M-030 **FIXED (s23)** (cr_ui_last_transport_error, ABI-since 2.2) | M-030 **FIXED (s23)** | M | **N-041 (S4: dead RdaClient + volume_letter_to_disk)**; I-016 (S4: wire register) |
| mom-capabilities (S5b NEW) | **M-043 (S5b: NTFS content-proof + backup-boot) FIXED s15**; **M-044 (S5b: UDF family) FIXED (s21 — the og-exact AVDP→VDS probe + FsFamily::Udf 11; findings/FIXED/M-044)**; **M-045 (S5b: FAT gates) FIXED s15**; N-050 (S5b: boot-only recognition) | **M-042 (S5b: lostpart persistence semantics)**; N-049 (S5b: record richness) | I-020 (S5b: corpus-doc corrections register) | **S-029 FIXED (s10)** (apply module + CLI; PARITY-DELTAS D-68/D-69) | **M-040 FIXED (stages a+b — findings/FIXED/M-040: stage a cr/bcdfix s34 DFX34, stage b cr/nvramfix + the win layer da46b14 s37 DFX37 — the FUN_1800046f0 matcher + FUN_180004b40 full-position rewrite decoded first-hand, the dynamic firmware seam (the og's :3281-3290), the BCD file side + the BCD00000000 force-unload; boot.ini + chkdsk REGISTERED (D-90); the host wiring landed as `rrecov apply --boot-fix`)** — was: (S5b: BCD/NVRAM/boot.ini/chkdsk family)**; ~~M-041 (S5b: check phase + error taxonomy 0x1001-0x100e)~~ **FIXED (s30 DFX30 — the cr/check module: full taxonomy + FAT/NTFS passes + never-abort wiring + REC_FS_CHECK + ABI/CLI; 15 tests; findings/FIXED/M-041; module row: modules/check-phase.md)** | **S-030 (S5b: image scan targets) FIXED (s8 — findings/FIXED/S-030; module row: modules/imaging.md)** | ~~N-048~~ **FIXED (s18 DFX18: the cr::log seam — host sink default-NULL + START/END/stage/checkpoint/candidate/read-error sites; CLI --log wired as the EPMLOG-role sink; D-75 registers the single-process divergence; fail-before RED executed)** | **I-019 (S5b: EPM tool-ops scope register)**; OQ-S5-1 (CheckVersion consumer) | M-040 pending (S-029 decided: implemented v1 MBR — D-68) | NA |
| check-phase (S30 NEW) | **M-041 FIXED (s30 DFX30)** — the damage-audit phase: FAT pass (DBR ceilings, dead-zone/dup scans, pairwise copy compare, dir audit, orphan sweep) + NTFS pass ($MFT/$MFTMirr/version/$Bitmap/cross-check), taxonomy 0x1001-0x100e | NA | NA | NA | NA | **M-041 FIXED (s30)** — never-abort wiring, REC_FS_CHECK, ABI, CLI --check | NA | NA | NA | NA | M-040 (the escalation endpoints) stays open on its row |

**Session-5 fix note (2026-10-26):** S-021 + S-003 FIXED (HFS+ journal
differential harvest — PARITY-DELTAS D-56; the fallback residual tracked in
OQ-S003-journal-fallback-salvage.md).

Cross-cutting rows: **Q66 = M-006 FIXED** (fuzz CI re-enabled by parallel leg `e30b84d`;
codeql deferral documented) **+ M-053 (S8) FIXED** (the C-ABI consumer gate —
registered in ctest — had crashed on every run since M-015's fixture update missed
`tests/ui_abi_check.c`; root-caused with a deterministic plain-C reproducer, fixture
fixed, gate green on release + ASan). **Q67 = M + N-018 (S2)** (ASan/TSan CI green; UBSan NOT enabled —
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
with pin tests, filed in FIXED/ since session 25).

**Session-3 changes (this leg):** S-015 (rawsigns KiB units, SEVERE), M-022 (ReFS
file-value grammar), M-023 (filter model — replaces the old GAP-cats note), N-030
(rawsigns confidence dead in og — **FIXED S12 as the registered D-66 superset,
option (b) of its own fix plan; pins re-cited in the Resolution**), N-031 (carve
dispatch/stride deltas; OQ-T8 RESOLVED), N-032 (health transports), I-012 (GAP-ADS
resolved — parity). Fixed
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
| ntfs | B1/B4/D15/A26/B5 | **S-012 — FIXED (convergent multi-agent fix, session 4 + verification/correction)** — two-pass architecture landed: destroy default OFF (pass-1 membership, :18630), full pass-2 GUID-dump walk with claim map + og gates (:21152-21264), size-0 rescue (mirror resident + same-name merge); finding premises corrected (no cross-pass dedup og-side; $Boot = the claim gate's 6-byte compare); the verification leg's corrections on top: dump-walk stride = record size on every path (corpus uVar8/uVar16 decode), pseudo record# arithmetic, N-040 (pass-1 filter = og-15 table AND $SI SYSTEM+HIDDEN; the $FILE_NAME gate removed); Resolution + both addenda in findings/FIXED/S-012-*.md, N-040 in findings/FIXED/ |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024** (8.3 short-name field; two-sided boot ladder; pass-2 stride quirk superset; namespace-ranking superset; og error facility codes) |
| ntfs | B1/B4/D15/A26/B5 | **S-012 FIXED (diff-fix session)** — two-pass landed: pass-1 membership restored (destroy default off), GUID-dump pass 2 (claim-map viability, 0x3ff + blanket-$ + folder rules), size-0 rescue (mirror resident + same-name merge); Resolution in findings/FIXED/S-012-*.md |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024 — FIXED (minors session, 2026-10-26)** (short_name field + session persistence ✓; boot ladder = LoadNtfsVolume exactly ✓; stride quirk + namespace ranking registered D-52; error-code mapping registered in ABI-CONTRACT §4 — see findings/FIXED/N-024) |
| fat | B1/S35/X49 | **M-011 — FIXED (s4)** (per-flavor windows: FAT32 up to 300 MiB whole-loaded, FAT16 >= 12 sectors) |
| fat | A27/B2/S35 | **M-012 — FIXED (s4)** (mirror scrutiny: shifted-copy/cross-link reject + contiguity accept + forward BAD/EOC disbelieved; mirror-only scan on unreadable FAT1; the N-022 primary-FREE consult landed earlier) |
| fat | B1/B3/A27 | **M-015 — FIXED (s4)** (whole-table validation + copy retry: media pair, sweep, claims-map cross-links; watermark advisory) |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| exfat | B2/A27/X47 | **S-013 — FIXED (session 3)** (windowed-FAT accessor, og 1 MiB windows; have_fat never false; extent coverage pinned; windowed failures degrade to the EXFAT-2 salvage) |
| exfat | B3/B4/B5/D15 | **M-013** (og DOES validate entry-set checksums — dual-form incl. deleted sets + reconstruction oracle; refutes the N-003 fix's verified-absence premise; supersedes N-003) |
| exfat | B3/B4/B10 | **M-014 — FIXED (s35)** (structural gates: secondary_count ∈ [2,18]; create+modify+access timestamp field sanity, all-zero FAILS; empty streams never emitted; name flags/first-char gates; reserved1/2 zero — findings/FIXED/M-014) |
| hfsplus | B3/B5/B7/C20 | **N-028 — FIXED (minors session, 2026-10-26)** (sink filter at both record sources ✓; rsrc superset registered D-53; 'prdf' folderization ✓ — see findings/FIXED/N-028) + **N-038 FIXED** (ownerFlags 0x20 fork selection) + **N-005 FIXED** (block bound 16 MiB + product sanity) |
| btrfs | B1/B2/A26/B5 | ~~M-020~~ **FIXED (s33)** — full profile machinery: RAID10 geometry gate (og :2162-2168), class-0x20 rotating mirror og-exact (:2534-2540), RAID0/RAID10 stripe math as the registered superset (D-80; the og FAILS those classes — audit Summary decode error corrected in the Resolution), stripe-boundary extent splitting, default-subvolume DIR_ITEM pick (:3975-4060) with root-5 fallback rendering at the volume root; 4 fail-before + 2 updated expectations; findings/FIXED/M-020 |
| zfs | B11 (SA registry) | ~~M-038~~ **FIXED (s19)** — registry-driven SA decode; findings/FIXED/M-038 |
| zfs | B1/B2/A25/D15 | ~~M-019~~ **FIXED (s19)** — first-entry fallback for every DVA (foreign reads counted), fletcher4 {7,0} gate over raw bytes + 32-byte compare + next-DVA retry, L0 extents planned at the fallback position; fixtures now checksum-carrying; 2 fail-before + 1 pin; findings/FIXED/M-019 |
| refs | B1/C19/C20/A25 | ~~N-026~~ **FIXED (s28, mixed)** — boot gates aligned (bps ladder + cluster product {4K,64K}, og :13544-49) + 4-KiB open read; N-013a/N-013c INVALID on first-hand decode (0x50 body is by-construction — hardcoded 0x30 ref extension; the mid-volume formula is N-036's container key, og probes NO superblock backup); findings/FIXED/N-026 |
| refs | B1/C19 (N-007) | ~~N-007~~ **FIXED (s33)** — first-DWORD page-signature discovery (og :11692 `*param_2 == *(int*)pcVar4`), full-width memcmp as the grading signal (signature_partial); I-004 half stays ACKNOWLEDGED (D-48b); findings/FIXED/N-007-I-004 |
| table+ldm | B1/C20 | ~~M-016~~ **FIXED (s19)** — candidate list {2,0,1,2045,2046} = og probe order + tail superset behind; config+0 disk parses, +2 wins conflicts (committed_seq proof); 3 tests, findings/FIXED/M-016 |
| table+ldm | B1/A27/X49 | ~~N-025~~ **FIXED (s23)** — 1 MiB config gate (>= 0x801 -> ERR_BAD_CHECKSUM, og :59748-59759) + zero-interleave 64 KiB default on stripe/RAID-5 (og :47665/:68631-68641); 2 fail-before tests + boundary pin; findings/FIXED/N-025 |
| lost-partition | B3/A30/B16 (engine-side filter validation) | **M-035 — FIXED (s27 DFX27)** (the exFAT-only content-consistency scorer: considered/filterable/matched counters + the reject rule decompile-exact (strict < 0.3), substring classifier + MOV/MP4/JPG specials, out-of-bounds invalidation; scan_candidate wiring + detach_subtree; ScanStats::lost_content_rejected; 15 tests + fail-before; findings/FIXED/M-035) |
| lost-partition | A30 | ~~M-017~~ **FIXED (s19)** — the og's content model end to end: weighted count + bonus + 500 table-match (polarity inversion repaired), ln/ln permille, containment-drop, stable sort, the iVar16 size-match promotion re-decoded; findings/FIXED/M-017 |
| carve | B1/X47/X51 | **I-009** (byte-stepped probing = detection superset; block-shape constants) |
| carve | B2-adj/R54/R57 | **M-021** (video-repair pre-scan hook absent) |
| carve | B1/B2/C19/A28/Q65/P46 | **S-017 — FIXED (session 3)** (ftyp-family magics re-keyed on the box TYPE; F8 self-probe guards the unreachable-magic class; naked-moov salvage mirrors the og MovAnalyzer conservatively, tried last) |
| carve | B2/A28/B3 | **S-018 — INVALID (session 3)** (premise refuted first-hand: the og's modern JPEG analyser IS EOI-gated — 0xD9 appears as Ghidra's signed char `-0x27`, 5 sites; `uVar21 == 0 -> epilogue`; the audit's literal `0xd9` grep missed them; residual = legacy fallback sub-analysers + streamed continuation -> OQ-S3-JPEG) | carve | D13/B10 | ~~N-027~~ **FIXED (s28, mixed)** — naming split closed (one File%06u family, both layers through the namer chain); N-014a/c/d verified FIXED-ALREADY (N-016/M-024/N-008+N-068); findings/FIXED/N-027 |
| session | B10/D12 | **N-029 FIXED s17** (N-016a: maximal-subpart policy, DFX12 lane; N-016b: raw name bytes persisted [nflags bit 32] + og-parity restore naming from raw; findings/FIXED/) |
| health | F61/B1 | **S-014** (BitLocker auto-unlock: wrong live registry path, offline-hive path absent, unassociated enumeration — F61 was never scored before) |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| orchestrator/wide | B1/D16 | ~~M-018~~ **FIXED (s33)** — the FilterFolder pass: generated data (byte-exact corpus ini via scripts/gen_filterfolder.py, the file's `._.DS_Store`/`新建資料夾` over the finding's typos), NODE_FLAG_JUNK_FILTERED at post-walk pre-grading, query/CLI-tree/recover-all gates, junk_filtered stat, D-82 policy register — adjacent minors **N-016 + N-017 FIXED (minors session 2026-10-26)** (teardown double-emit removed, count pinned; CancelPoolWatcher factored + wired at the candidate fan-out — D-54) |
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
| carve | A42/A45 | ****M-024 FIXED (s10: disabled_formats surface — findings/FIXED/M-024)**** (no runtime format-filter surface vs og RawConfig.ini [Filter], 34 toggles (CORRECTIONS #5 — was miscounted 37; M-024 addendum), STL/VPJ default-off) + **N-033** (registry exports consumed by FS engines — no core ABI) |
| carve | P47–P53/X47/X50 | ~~M-025~~ **FIXED (s32 DFX32: the WindowView — validator probes/checksums/hashes/tail-walks/feeds served from the already-read window; MEASURED 9.88× fewer read_at calls, 2.08× fewer device bytes; the pending-object model registered as D-80, future stateful analysers port behind the M-021 seam; geometry change not taken — byte-granularity dominates, I-009 superset kept)** |
| carve | P47–P53/X47/X50 | ~~M-025~~ **FIXED (s33)** — the window-serving read context (WindowDevice): validators/checksums/hashes/tail-scans read through the window buffer (the og's already-in-buffer model); MEASURED 52→4 device reads on the 4-plant fixture (13x, zero per-candidate re-reads); geometry unchanged per the finding's bench-evidence rule; stateless-vs-pending registered D-81; findings/FIXED/M-025 |
| carve | A25/A28/C19 | ~~N-031~~ **FIXED (s28)** — the zero-sector fast-skip landed (all-zero 47-byte sign window skips the raw-sign probe, counted in carve_zero_strides); stride superset + only_common coverage documented; addendum's chain semantics already equivalent (pins green); findings/FIXED/N-031 |
| lost-partition | A30/B1 | **S-019** (BitLocker "-FVE-FS-" probe family missing — S-006 under-listed the og; distinct from S-014's unlock-path finding) |
| lost-partition | A30 | **M-017 addendum** (RTTI proof: *CalcPartition = geometry calculators, slot 4 = SPC; full AdjustPartitionInfoOrder: containment drop, FS-grouped dedup, 0.1/0.3 tie rules, ±16 MiB promotion, cap 2/4/5/6, stable merge sort) |
| lost-partition | C20/B1 | **N-034 — FIXED (minors session, 2026-10-26)** (full og deep-scan ladder `fat::lost_boot_flavor` + engine-open gates `boot_passes_open_gates`; SPC sub-claim corrected as false positive — u8 domain; see findings/FIXED/N-034) |
| lost-partition | A29/C20 | **N-035 (S2) FIXED s22** (data-area-anchored containment/overlap extents on Candidate — the og's DataAreaStart/totalSec pair; fail-before executed) |
| refs | B2/B7/A25 | **S-020 — FIXED (s4)** (resident materialization e490/ea30 pointer-form + all-rows walk + paged chase cf30 + granularity e020; 6 fail-before tests; Resolution in findings/FIXED/S-020) |
| refs | B4/B5 | **M-026 — FIXED (s4)** (stale-MSB-page sweep landed: sig + vol-sig + live-exclusion + force-deleted dispatch; row-flag/ID2 superset preserved; both deleted models active) |
| refs | B7/A29-adj | **M-027 — FIXED (s4)** (placement-signature identity keep-one + live-over-deleted; $R*/S-*/SVI post-main recycle flush; $I evidence read + $R remap) |
| refs | C20/A27 | **N-036 PARTIALLY FIXED (s4)** (recognizer half: 0x40+refsize accepted, 0x20 physical, block-0 skip; container-map population open pending real-volume probe) |
| refs | B3/A27 | **N-037 — FIXED (s4, folded into S-020)** (granularity discard vs allocated size; premise corrected on the :10213 re-read) |
| refs | F-gate/B1 | **I-013** (host-OS runtime gate: og refuses < Win8 hosts; core host-agnostic — deliberate portability superset) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| hfsplus | B4/B5/B2/A25 | **S-021 — FIXED (s5)** (`journal_harvest_pass`: ring walk + differential + deleted-tier emission; D-56; the tx-magic derivation stays OQ) |
| hfsplus | B2/A26 (extents-overflow decode) | **A-007 — FIXED (closing session; twice-renumbered A-005→A-006→A-007 after the rawfixer-4Kn collision)** (descriptors at key+12, TN1150-verbatim — the 12-byte key includes the keyLength field; both parse sites (live walk + S-021 journal branch) were off by two and silently dropped fragmented tails; first populated overflow fixture) |
| hfsplus | B4/B5/B2/A25 | **S-021 — FIXED (fix session, 2026-10-26)** (journal deleted-record harvest: JIB/jhdr gates, ring walk with the u16 header test, og node-image validator, fork classification by volume ranges, catalog differential + extents side map; field-map corrections in evidence/decompiled-quotes/s3-hfsplus-inventory.md §3a; **discovered ACUTE A-005** — the B-tree record-span inversion — fixed in the same commit) |
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

## Session-5 detail rows (2026-10-26, audited `7cf6754`)

| Module | Parameters | Finding |
|---|---|---|
| rawfixer | F60/Q67/Q65/A25 | **A-005 — ACUTE (S5)** (4Kn stack-buffer overflow: `u8 x[512]` buffers receive `sector`-byte reads at cr_rawfixer.cpp:558/574/590/639; transport reports 4096 on 4Kn; tests pin 512 — FIX-3 fixed offsets, not buffers) |
| carve+session+query | B4/D11/D12/B1 | **S-031 (S5)** (carved files never marked deleted: og FLAGS=0x0020 constant on all 12,147 sampled records; core's deleted_only filter and deleted_count exclude the entire carve set) |
| carve | B1/B2/D11/A27/A28 | **S-032 (S5)** (ZIP/RAR inner-entry inventories: og returns the complete inner file inventory + multi-part RAR stitching; core emits one flat node) |
| ldm+rawfixer | C20/B2/A25/F60 | ~~M-046 (S5)~~ **FIXED (s10)** — GPT path geometry-correct ×3; exFAT plans bps-sized; LDM 512-LBA unit family fixed; 3 fail-before tests; findings/FIXED/M-046 |
| session+tree | D12/D11/B8 | ~~**M-047 (S5)**~~ **M-047 FIXED (s9)** (FIXED/M-047: recursive refresh_stats + the persisted-image trust on load; partial loads recompute) |
| carve | B1/A28/D16 | **M-048 (S5)** (EUUserData user-extensible signature DB — 111 extensions, `"offset,hex"` signs + MIME — has no core counterpart) |
| err+abi | C23/D14/F35/Q46 | ~~**M-049 (S5)**~~ **M-049 FIXED (s24 DFX24)** (FIXED/M-049: session-11 header truth + err_text; session-24 closes the registered step 2 — the engine-side error-context subset on the error-range event family: err_module/err_operation/err_attempts + CR_ERRMOD_*/CR_ERROP_*, per-range attribution recorded at failure time via the cr::errmod thread stamp + device-owner fallback; 8 fail-before-proven tests; ABI pins; D-68 updated) |
| session+tree | D11/D12/B4 | **M-050 (S5)** (node-model round-trip gaps: ~15 og wrapper fields — hidden-root bit, grading bits 5-7, roles, fixup, sentinels — have no core carrier) |
| carve+session | D11/D12/B9 | **M-051 (S5)** (RAW provenance unpersisted: timestamp string, analyser checksum w/ 0xFFFFFFFF invalid marker, note, tail anchor, boolean) |
| carve | A28/B3/C19 | ~~N-051 (S5) MKV DocType discrimination + element-bounds gate absent~~ **FIXED (S12)** — FIXED via the parallel lane's b2f48c6 (canonical): bounded-buffer DocType walk (webm/matroska via 0x6D626577), 0x200 element-bounds gate; DFX12 convergence addendum: 0x67805318 = Segment ID 0x18538067 LE + independent fail-before run (8/10 RED pre-fix) |
| carve | B10/A28 | ~~N-052 (S5) HEIC brand relabel to L"HEIF" absent~~ **FIXED (S12)** — FIXED via b2f48c6 (canonical): FMT_HEIF registry row (append-only) — heic-proper keeps HEIC, siblings heix/mif1/msf1 relabel HEIF; DFX12's alternative one-type-id carrier preserved in history |
| carve | C19/B1/B10 | ~~N-053 (S5) IIU magic: og binds TIFF/EP chain, core binds RW2~~ **FIXED (S12)** — FIXED via b2f48c6 (canonical): FMT_TIFF_EP claims the IIU key with validate_tiff; FMT_RW2 = post-reject IIU-salvage (SUPERSET D-63a); label-by-grammar; convergent with DFX12's same resolution |
| carve | C19/B1/B3 | ~~N-054 (S5) RAF 4-byte-vs-16-byte strictness (miss) + JPEG 4-dword-vs-prefix (superset)~~ **FIXED (S12)** — FIXED via b2f48c6 (canonical): RAF keyed on the FUJI 4CC, 16-byte string PRIMARY + variant path (conf 700); JPEG 3-byte prefix REGISTERED (D-64a, og's 4 dwords documented); + N-076 (DFX12): the ASCII-version gate fix their landing still lacked |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| carve | B1/A28 | N-076 (S12 NEW-FINDING, discovered during N-054 verification) — validate_raf's version gate rejected every real-world RAF (ASCII version "0100"/"0201" at +16 misread as binary, LE value ≫ 0x2000 cap); the FMT_RAF row was dead for real files since introduction (zero fixture coverage). **FIXED in the same change** — ASCII-digits OR small-binary acceptance; Carve.RafRealAsciiVersionAccepted fail-before executed |
| writer | F62/Q65/A35 | **N-056 FIXED s17** (WinDestIo .part-N temp leak on failed rename — commit deletes + close gates on the rename state; UNC contracts tested; findings/FIXED/) |
| table+visualization | Q65/A54/Q69 | N-057 (S5) TableLayout::Ebr dead + diskmap overlay/budget test debt |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| session+btrfs+zfs | D11/D12 | N-059 (S5) btrfs secondary ids/generation + zfs secondary flags unpersisted |
| session+orchestrator | F32/F38/F41 | ~~N-060~~ **FIXED (s18 DFX18: per-node SerializeProgress hook + coalesced CR_EVT_SAVE_PROGRESS + the SAVE-SCOPED cr_ui_scan_save_cancel (the og's IsCancelled is the save object's state — save-on-scan-cancel intact) + atomic temp-then-rename saves; fail-before by construction; suite 716/716)** |
| orchestration+session | F35/Q46/D12 | ~~N-061~~ **FIXED (s18 DFX18: run-log = the N-048 seam + the CLI --log sink with the qa-logs semantic set at the core; auto-naming = session::auto_session_name_at, byte-exact vs the corpus sample; Session.AutoNamingConvention fail-before by construction; suite 733/733)** |
| orchestration | F35/P49 | ~~N-062~~ **FIXED (s18 DFX18: check_memory_pressure on the seam + the shim sampler on the 5 s watchdog cadence + memory_warn_mb (0 = OFF the conservative default); never scheduling-coupled)** |
| carve | B10/A28/Q68 | **N-063 FIXED s17** (default_namer "File%06u" was a core invention — now the registered deliberate default D-87; MP4/MOV per-analyser template decoded + replicated quirks; embedded_name carrier; findings/FIXED/) |
| health/bitlocker | F61/B1 | MATCH (N-064 FIXED s31: cr::dict + unlock_with_dictionary + --bitlocker-dict + cr_ui export; consent-gated, manage-bde plumbing, Office side out-of-scope per I-010) |
| health/bitlocker | F61/B1 | ~~N-064~~ **FIXED (s34 DFX34: pure cr/pwddict module — og-exact Dict format parse (comma-terminated CRLF, byte-verified first-hand), 20-entry authored seed = REGISTERED DIVERGENCE D-78 (the og's 228-entry asset never copied; loader is format-compatible), consent-gated sequencer (dedup, caps, attempt-log progress+length only) + win try_password_unlock (same unlock() plumbing as a typed password, no in-process crypto) + CLI --bitlocker-pwddict <path\|seed> explicit-consent action; scan path holds no reference (F61 preserved); 9 Pwddict tests + executed fail-before RED; suite 752/752; findings/FIXED/)** |
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

| Module | Parameter(s) | Verdict change (evidence in the finding + `evidence/decompiled-quotes/s6-*`) |
|---|---|---|
| fat | B4/B5/D11/A29 | **M-052 — FIXED (s24 DFX24 + s35 convergent)** (recycle-bin $I/$R pairing — the shared cr/recycle model wired into the dentry walk; four-root recognition, chain-read $I parse, original-name remap + post-walk fixup; findings/FIXED/M-052) |
| exfat | B1/A25/S35 | **N-067 FIXED s11** (upcase-table validation — og shape gates + checksum32 stream check, soft stats signal; findings/FIXED/); B4/B5/D11 **M-052 FIXED (s24 DFX24 + s35 convergent)** (the FAT-mirrored pairing — bin context through walk + oracle re-walks; findings/FIXED/M-052) |
| carve | A25/D11/P42 | **S-032 (converged)** (archive inner-file inventories missing) |
| carve | B1/C19 | **N-069 FIXED s11** (CAB mscf case key — FMT_CAB_LC dual row + two-spelling validator re-check; findings/FIXED/); B1/A28 **N-068 FIXED s11** (LFH sanity gates decompile-exact; EOCD half FIXED-ALREADY — the CD anchor + EOCD reconciliation already cover it; findings/FIXED/) |
| carve | A28/A29/D15/B3 | **N-070** (overlap scoring + two-generation fallback); **S-007 addendum** (bare-MPEG dispatch gap — 18+ u16 sync keys vs ID3-only; family roster with provenance) |

| carve | B1/C19 | **N-069 FIXED s11** (CAB mscf case key — FMT_CAB_LC dual row + two-spelling validator re-check; findings/FIXED/); B1/A28 **N-068** (ZIP EOCD anchor + LFH sanity gates) |
| carve | A28/A29/D15/B3 | **N-070 (S5) FIXED s22** (overlap scoring: accumulated base+verdict ranking implemented, fail-before CR2-vs-DNG; two-generation fallback REGISTERED as D-75 — legacy bodies undecoded); **M-056 (S22 NEW-FINDING) FIXED** (TIFF-family size basis mix — multi-strip TIFFs carved at the 256 MiB cap); **S-007 addendum** (bare-MPEG dispatch gap — 18+ u16 sync keys vs ID3-only; family roster with provenance) |
| session | D11/D12/B4/B9 | **S-031 (converged)** (carved files: deleted flag + timestamp + validator payload dropped at emission; sessions round-trip the misstatement) |
| session | D12/S36 | **N-066** (identity card: firmware/WWN/interface + 512-byte boot snapshot vs model/serial/CRC32); A35/D12/F35 **N-078 FIXED s17** (truncated carve_note silent-OK exit — the c.fail-less break; filed+fixed, renumbered past the parallel lane's N-077 leak finding; findings/FIXED/) |
| session/tree | D11/D12/P42 | **M-051+N-059 (converged)** (type-registry ID + per-FS secondary payload slots missing) |
| writer | A27/S35/P46 | ~~N-065~~ **FIXED-ALREADY (s34 DFX34 verification — parallel-lane landing: collision = the og's `%s_%d`/`%s_%d%s` two-branch shape, cap 10,000, exhaustion fails the item, `_dup` removed; mkdir = the 0/0xB7 contract in SEMANTIC form as a registered divergence — the literal gate broke drive-root chains, MSVC test #227; D-19's refuted claim superseded; proof tests verified green: CollisionNameUnderscoreFormat / CollisionExhaustionFailsTheItem / MkdirSemanticsMatchShCreateDirectoryEx; findings/FIXED/)** |
| health | P46/P68 | **S-024 addendum** (cr_health.h claims 0xD1/0xD8 never issued — doc-truth sub-item; capability gap stays S-024) |
| storage | B8/P42/A54 | MATCH-with-D-84 (N-071 FIXED s31: the two-stack merge — PhysicalDrive loop + cfgmgr32 GUID_DEVINTERFACE_DISK pass, pure merge_disk_enumeration dedup; residual stacks registered) |

| storage | B8/P42/A54 | **N-071 FIXED s30** (option (a) per the finding's recommendation: PARITY-DELTAS D-80 scope registration + the busy-disk 0-access metadata retry — a share-blocked disk stays listed; findings/FIXED/N-071) |
| storage | P47/P48 | ~~N-045~~ **MATCH (registered divergence)** (FIXED/N-045, session 18: 8 MiB/256 KiB reactive-LRU vs og 4 MiB prefetch engine REGISTERED as PARITY-DELTAS D-72 — budget + algorithm, corpus cites re-verified first-hand; doc-truth pin `Cached.ScanCacheBudgetIsTheRegisteredDivergence` keeps source constant + D-72 row + cr_cached.h divergence block in lockstep; no code change, the alignbench sweep owns the tuning decision) |
| storage | S35/Q43 | ~~N-046~~ **MATCH** (FIXED/N-046, session 18: `PartitionSlice::refresh_geometry()` — the og's RefreshSectorSize-from-parent surface (FUN_140643870) with revalidation hardening over the og's blind store; Slice.GeometryRefreshAfterMediaChange + Slice.GeometryRefreshRefusesInvalidatingChange; orchestrator auto-refresh excluded and documented — the corpus exposes no scan-path caller) |
| ext4 | A27/S35/B2 | ~~N-073~~ **FIXED-ALREADY** (FIXED/N-073, session 14: N-004's transport-level CatchBuffer landing covers the engine path — 3 engine-path pin tests; the S35 row in modules/ext4.md carries both N-004 and N-073 closed) |
| btrfs | P47/P48/S36 | **N-072 FIXED s34 DFX34** (the re-scan metadata cache: pure codec + engine wiring, OFF by default, caller-supplied dir, fsid+generation strict gate, warm scan skips the chunk derivation — nodes_read strictly lower; the og's undecoded format replaced by OUR registered one; findings/FIXED/); **OQ-S6-2** (RC4 PRGA target) |
| zfs | A25/S35 | **N-072 FIXED s34 DFX34** (the delay-execution label retry: bounded 250/500/1000 ms backoff, cancel-aware in 50 ms slices, live handles only via the new is_static_media discriminator — images never retry; label_retries counted; findings/FIXED/); INFO-register (lzjb/zle/gzip decode — no divergence claimable, evidence ledger only) |
| lost-partition | A30/A37/D12 | **I-024** (PartitionRecoveryD generator-sets + .dat persistence register) + **OQ-S6-1** (decode request); **M-017 addendum** (CalcPartition class evidence) |
| orchestrator | A38/A41/P42/A44 | ~~N-075~~ **MATCH** (FIXED/N-075, session 28: `cr_ui_scan_busy`/`cr_ui_scan_stopping` synchronous query exports — the og's sigIsBusy(bool&) x14 / sigIsStopping(bool&) x8 reference-out signals; pull-progress convention REGISTERED as D-78; typed IUIProgressInfo mapping documented in ABI-CONTRACT 5; pinned by `CrUi.ScanBusyStoppingQueries` + the plain-C edges); **I-023** (DRWCfg scope register) |
| visualization | P42/D-model | **S-009 addendum** (stale premise corrected — surface landed; residual: kCellAllocFree dead, VolumeBitmap discarded) |
| query | P42 (sort Type) — **MATCH** | M-029 addendum (path-group CLOSED session-6) + session-11: Type sort CLOSED (SortKey::Type, ABI code 7, iconIndex order) + the modified-unit doc lie fixed (FILETIME, pinned) + paging documented as the deliberate difference (no og page-size constant) + **session-32: the bucket-ID translation + general-raw naming CLOSED (query::apply_date_bucket/apply_size_bucket from the DRWUI FUN_140705c50/FUN_140706060/FUN_140254670/FUN_140255820/FUN_140256830 decode; cr_ui_query_apply_buckets added export; ScanConfig::raw_category_name) — the finding's every sub-item is now closed (FIXED/)** |
| abi (cr_ui) | P42 | ~~N-033~~ **MATCH** (FIXED/N-033: ABI sub-item landed s6 (cr_ui_carve_format_count/info) + the CLI sub-item closed s28 (`rrecov list-carve-formats`, FB-K01 exit discipline) + the per-scan refresh divergence REGISTERED as D-80; pinned by CarveFormatRegistryEnumeration + the parse gate) |
| quality (cross-cutting) | P46/P68 | ~~N-074~~ **FIXED (s18 DFX18 canonical `479b0c7` + convergent s22 DFX22 landing reconciled in the same-window merge — both lanes verified all five sub-items first-hand independently; pin Health.SmartCommandListMatchesImplementation fail-before-proven)**; **~~N-019~~ FIXED (s28: the 13-scenario Q65 gap — 4 new mutation-proven tests + 6 already-covered re-cited + 1 superseded by M-017's scoring + OQ-T6 pushed forward with new snapshot evidence; see FIXED/N-019)** |

New tracked-open: **OQ-S6-1** (PartitionRecoveryD generator-set semantics), **OQ-S6-2**
(Btrfs RC4 target), **OQ-S6-3** (CRAW +0xF4/anchor-B/checksum consumption) — 14 tracked-open
files total. Session-6 verified-fixed addenda: M-029 (partial), N-033 (partial), N-045 (cite),
M-017/M-030/S-007/S-009/S-024 (evidence sharpened).

## Session-9 detail rows (2026-10-26 — the diff-fix session-9 baseline-regression filing)

| Module | Parameter(s) | Verdict change |
|---|---|---|
| fat (test infrastructure / CI gate) | M-015 landing coverage | **M-053 — FIXED-ALREADY (s9; the repair itself is the parallel DFX7 lane's `b1f9210` inside N-018's defect set — this session's filing is the formal record: root-caused independently via worktree bisect at the regression point (parent green / landing SEGV), the formal finding filed per the discovered-divergence SOP after the parallel lane fixed the bytes without a finding. Engine code untouched (og-parity validator correct). `evidence/executions/s9-m053-abi-fixture-regression.md`) |

## Session-20 detail rows (2026-10-26 — DFX20, the unclaimed-MAJOR lane)

| Module | Parameter(s) | Verdict change |
|---|---|---|
| storage | S61 (BitLocker placement) | **M-033 FIXED (DFX20)** — the in-stack XTS-AES decrypting device now exists (`DecryptingDevice : IStorageDevice`, cr_decrypt_device.h; AES-128/256+XTS clean-room in cr_aes.cpp, pinned byte-exact against pycryptodome AND OpenSSL EVP 3.5 — the og's own cipher interface — plus FIPS-197 KATs; per-sector LE-index tweaks; the og's layer order physical -> slice -> [XTS] -> engine). Key surface = the og's GetFullVolumeEncryptionKey output shape: `--bitlocker-fvek` CLI + `cr_ui_open_volume_with_key` (additive ABI export, no version bump). Locked-without-key refusal PRESERVED (S61/F13; FB-B01). The recovery-password/BEK -> VMK -> FVEK unwrap chain stays out-of-corpus — tracked-open OQ-S20-1 with the decompile entry points (ParserBitLocker :27633, FUN_180042350/FUN_180044e90). 13 tests; suite 608/608. |
| abi (cr_ui) | P42 (export set) | **M-033 additive export** — `cr_ui_open_volume_with_key` (keyed locked-volume open, the in-stack decrypt opt-in); ABI policy: additive = compatible, no version bump; C-consumer check compiled clean against the new declaration. |
| session | D12/S36/P34/P35 | **M-042 FIXED (DFX20)** - resume re-validation landed: lost::revalidate_candidate at load (revalidate_disk) AND run() start (whole-disk-basis resumes); stale records DROPPED (PRD:20748-20842 semantics). REC_PARTITION_CONTEXT v2 append-only (bps/SPC/hidden/flags - the og's CSearchedPartitionInfo payload); REC_DISK_LAYOUT type 8 (partinfo equivalent - layout_changed surfaced at resume, never enforced); per-hit checkpoint forced when discovery produces candidates (sink-gated). Executed fail-before; 6 tests; suite 685/685. |
| lost-partition | A30/B1/S35 | **M-042 FIXED (DFX20)** - the discovery probes capture per-record geometry (probe_ntfs/probe_fat/probe_exfat fill bps/SPC/hidden); revalidate_candidate re-runs the SAME per-family probe for resume truth. |
| cli | C24/grammar | **M-033** — `scan --volume X: --bitlocker-fvek <hex>` (64/128 hex chars, pre-pass so option order is free); locked refusal message now points at the new path. |

## Session-25 detail rows (2026-10-26 — DFX25, the INFO+tracked-open lane)


| Module | Parameter(s) | Verdict change |
|---|---|---|
| zfs | B1/B8 (dataset hierarchy constants) | **OQ-Z1 RESOLVED → A-008 (ACUTE, NEW-FINDING) FIXED** — child_dir_zapobj was read from dsl_dir bonus+24 (the dd_origin_obj slot; kernel +32 per OpenZFS master dsl_dir.h fetched verbatim + the og's own decode: 256B dsl_dir copy / 320B dsl_dataset copy, head hop @+8, ds_bp@+128, child zap walk skipping $FREE/$ORIGIN). Normal pools: NO child dataset ever enumerated (every zfs create silently missing); clones: origin followed as a child zap (ghost datasets — proven in the executed fail-before). Fail-before RED 3/3 (EndToEndWalk, ChildDirZapReadsKernelOffset32, OriginObjNotTreatedAsChildZap); pass-after 16/16 Zfs; suite 739/739. Fixture de-self-referenced (kernel layout). findings/FIXED/A-008. |
| (info tier) | all | **22/22 acknowledged INFORMATIONAL findings moved INFORMATIONAL/ → FIXED/** (resolutions verified on `716d121`, pins green; two first-hand spot-checks re-executed). No verdict changes — `ack` cells now denote filed-in-FIXED. |

## Dev-sign session detail rows (2026-10-27/28, SA lane — driver-install/dev-sign module; corpus: 02-kernel-driver-architecture.md §5 + regdrv.reg)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| driver-install · installer executability | **MATCH (FIXED)** | **S-036** RESOLVED (upstream v0.2.3 + composite fail-loud flip + FloorVersion v0.2.3) |
| driver-install · service group | **MATCH (FIXED)** | **M-060** RESOLVED (fallback = install.ps1 twin, opt-in only; sc qc + Parameters assertions on both paths) |
| driver-install · Parameters registry (IoctlBase/MaxDisks/DebugLevel/SessionsEnabled) | **MATCH (FIXED)** | **M-060** RESOLVED (fallback = install.ps1 twin, opt-in only; sc qc + Parameters assertions on both paths) |
| driver-install · ImagePath form | **MATCH (FIXED)** | **M-060** RESOLVED (fallback = install.ps1 twin, opt-in only; sc qc + Parameters assertions on both paths) |
| driver-install · copy destination | MATCH | install.ps1:50-54/composite → `System32\drivers\<svc>.sys`; corpus §5.1 `%SystemRoot%\System32\Drivers\%s.sys` |
| driver-install · service shape (Type/Start/ErrorControl) | MATCH | `type= kernel start= demand error= normal` = regdrv.reg Type=1/Start=3/ErrorControl=1 (both paths) |
| driver-install · install-error semantics (start failure) | **MATCH (FIXED)** | **M-062** RESOLVED (start failure fatal exit 6 + RUNNING confirm exit 7) |
| driver-uninstall · teardown behavior | **MATCH (FIXED)** | **M-061** RESOLVED (uninstall.ps1 is the CI teardown, executed + post-state asserted) |
| dev-sign · environment matrix / HVCI constraint | **MATCH (FIXED)** | **M-063** RESOLVED (environments contract + windows-2025 matrix leg + release-body truth pass) |
| dev-sign · signature timestamp | **MATCH (FIXED)** | **N-084** RESOLVED (the /tr rationale documented + timestamp_server composite input) |
| dev-sign · installer interaction model | **MATCH (FIXED)** | **N-085** RESOLVED (operator-posture note; absorbed by S-038's packaging milestone) |
| dev-sign · signing posture (release artifact state) | DIVERGENCE-INFORMATIONAL | **I-026** — unsigned-by-design + consumption-time sign (execution-proven); og: vendor Authenticode (corpus :19) |
| dev-sign · load/round-trip behavior | MATCH | run 40549660074: RUNNING + control-device open + GET_INFO/GET_VERSION ready + grammar-path refusal class — the verified core of Phase 1 |

(No prior module file exists for this surface — a `modules/driver-install.md` lands with the Phase-2 wave that owns the packaging surfaces.)

## Dev-sign session detail rows — waves 1-2 (2026-10-28, audited `dd82db9`; corpus: the never-cited family/part1 artifacts)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| carve · ASF/WMV/WMA end-of-file determination (A28/B2) | MATCH (FIXED) | **A-010 FIXED** — the object walker (5 known top-level GUIDs + size gates + torn-tail device bound) walks to grammar break per the og's BruteForce contract (deep-dive:87-91); 4 fail-before tests in test_carve.cpp |
| carve · FH11 + CPT7/8/9 format coverage | **MATCH (FIXED)** | **S-037** RESOLVED — validate_fh11 (the og's FUN_180029f30 predicate) + validate_cpt (the FUN_180043630 gate + FUN_1800436e0 pointer-chase); 4 appended ids; fail-before-proven |
| carve · CPT format presence, 3 generations (A28/C19) | **MATCH (FIXED)** | **S-037** RESOLVED — three duplicate-key rows (the og's registry policy) sharing validate_cpt |
| carve · 131 family classes — presence/semantics vs the 6 family artifacts | 34 MATCH · 3 registered-delta · 64 S-007-remainder · 26 N/A · 1 UNDETERMINED | the wave's class-by-class table (agent sheet, scratch); residuals → **OQ-SA-2** |
| carve · signature-table packaging (R56/Q68) | **MATCH (REGISTERED DELIBERATE — D-103)** | **N-086** RESOLVED — the compiled-in posture registered as PARITY-DELTAS D-103 (option (a); content-identical, the usersigns surface covers runtime extension) | |
| session · FAT 8.3 name encoding (B10/D11) | **MATCH (FIXED)** | **N-087** RESOLVED — the full CP437 boundary decode (standard-verified table); 3 tests fail-before-proven | |
| session · IFileProperties 23-slot interface | 16 MATCH · 1 registered-residual · 1 UNDETERMINED · 5 N/A | the +0x20 getter = the N-050/N-059 type-registry residual (registered); +0x30 → OQ-SA-1 |
| session · NTFS/FAT/exFAT persisted fields | 28 of 32 MATCH (10/12, 9/10, 8/10) | carriers richer in places (4-state deleted, 4th timestamp, allocated_size); the 4 unmatched → **OQ-SA-1** (anchor datums + ids) |
| carve · RawConfig [Filter] count (doc truth) | CORRECTION PROPAGATED | CORRECTIONS #5: 34 not 37 — M-024 addendum; matrix/SUMMARY/evidence notes updated this session |

## Dev-sign session detail rows — waves 3-7 (2026-10-28, audited `dd82db9`; corpus: the Installer/ + config-files + UIUX/10 + pe-payload + MOM-stub materials)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| app-installer · K74 silent surface (13 params) | DIVERGENCE-SEVERE | **S-038** — Inno /SILENT//VERYSILENT//SUPPRESSMSGBOXES//NORESTART, ARP QuietUninstallString, unins%03d, admin-required — all absent (string tables verified) |
| app-installer · K77 registry + telemetry contract | DIVERGENCE-SEVERE | **S-038** — SetupUE 16-entry OS gate, `SOFTWARE\EASEUS\DRW\Version`, /SendInfo//SendInfoEx//UninstallStart//UninstallEnd |
| driver-install · K71/K72/K76 versioned/verified deployer contract | **MATCH (FIXED)** | **M-064** RESOLVED (v0.2.3 `6b50bb8` + v0.2.4 `323e75a`): write+verify + per-call SCM errors + the exit-code contract + the %s%d versioned coexistence (in-place upgrades; an in-use image never overwritten) + the -Uninstall verb + the versioned-aware teardown + the CI PS parse gate |
| carve · user-sign caps vs the og's own DB | **MATCH (FIXED)** | **S-039** RESOLVED — caps raised to the measured maxima (48/12/1024/80); the og's own DB loads 111/111; fail-before-proven (ParseOgShippedExtremes/HostileBlobStillAtomic/RegistrationRoundTripProceedsToScan); the ABI widths half converged as **M-067** (below) |
| carve · the user-sign ABI widths (the S-039 consumer surface) | **MATCH (FIXED)** | **M-067** RESOLVED — `cr_ui_user_sign` resized to the v3 caps (mime[81]/bytes[48]); `CR_UI_ABI_VERSION` 2 → 3 (the first struct-FIELD change); compile-RED fail-before proven (`-Werror=array-bounds`) |
| carve · default user-sign set | **SUPERSET (FIXED, D-104)** | **M-065** RESOLVED — data/userdata-defaults.json ships (the og's 111/190 verbatim) + the engine-init load + the release asset; OQ-SA-3 RE-RESOLVED (A2 scan-engine lane: the og's scan engine has NO user-sign channel — EUUserData feeds the CHK repair chain (CHKRepairFileSign); the landed behavior is a registered deliberate superset (D-104), not og parity; the FIX-6 "scan-setup trigger config" reading of the :227538 consumer is superseded — that consumer reads the per-scan carve-statistics map) |
| preview · archive-content enumeration (FS-origin) | **MATCH (FIXED)** | **M-066** RESOLVED — Metadata.entries (name/size/compressed/crc) collected on the existing CD walk, 512-entry cap + truncated flag; 4 tests fail-before-by-construction |
| preview · doc truth (M-028 propagation) | **MATCH (FIXED)** | **N-088** RESOLVED — all three sites corrected + the PreviewDocs grep guard |
| packaging · WinPE payload census (K78) | DISPOSITIONED-MATCH-at-plan | the PE capability boundary recorded as the boot-media builder's design input; roadmap.md:71 |
| MOM stubs · 17 never-cited per-module censuses | MATCH (zero new parameters) | wave-7 spot-check: 13 ROLLUP-COVERED + 4 INFRASTRUCTURE-ONLY; decode candidates noted (CWindowsFS ×3, DeviceManager offline-OS strings) |
| inventory truth · citation-count correction | CORRECTED | SURFACE-INVENTORY "22 of 32" → 17 of 30 (5 stubs cited in brace-group form at s5-session5-corpus-quotes.md:64) |
## Session-8 detail rows (2026-10-28, audited product `35adcb9`; corpus `be0cfe9` — the second-session deep audit)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| imaging · disk→image create flow (P42/C24/F59) | DIVERGENCE-SEVERE | **S-040** — reader-only core; no `cr_ui_image_create`, no verb, no writer; the safe-scan posture's producer absent |
| carve · S-007 batch-2 remainder formats (B1/A28/C19) | DIVERGENCE-SEVERE | **S-041** — ~24 formats (CHM MNY EMF ONE MPEG-TS 3DS SWF JP2 MOBI + the text school) absent; previously unowned |
| carve · CRW key attribution + validator grammar (B1/C19/A28) | DIVERGENCE-MAJOR | **M-081** — II/MM\x1A\x00 keys are Canon CIFF per the census; landed as "Minolta MRW" rows whose PRD gate rejects genuine CRW |
| query · path_substring + state_mask axes (D16/P42) | DIVERGENCE-MAJOR | **M-072** — 5 of 7 og filter groups; no substring, no state mask (R-22) |
| orchestration · scan-time folder filter (S31/D16) | DIVERGENCE-MAJOR | **M-073** — PushFolderFilter unmirrored; post-hoc query only (R-14) |
| writer · destination error classification (C23/D14) | DIVERGENCE-MAJOR | **M-074** — too-long/invalid/network collapse to ERR_DEST_UNWRITABLE (R-41) |
| check · chkdsk launch endpoint (S35/P42) | DIVERGENCE-MAJOR | **M-075** — WinChkdsk launch absent; D-90 disposition superseded by the plan (R-65) |
| health/enum · TRIM facet in rows (P42/X48) | DIVERGENCE-MAJOR | **M-076** — datum health-facet-only; no row fields/flag (R-63) |
| preview · 100 MB/15 s product caps (X48/X49) | DIVERGENCE-MAJOR | **M-077** — no eligibility gate, no time bound in the path (R-34) |
| health · BitLocker turn-off op (F61/P42) | DIVERGENCE-MAJOR | **M-078** — CBitlockerTurnOffOperation unmirrored; unlock-only surface (R-88) |
| session/CLI · auto-save volume label (D12/B10) | DIVERGENCE-MAJOR | **M-079** — no fill site; CLI auto-saves always `auto_<ts>` |
| preview(thumb) · default read budget vs ordinary photos (X47/X48) | DIVERGENCE-MAJOR | **M-080** — 6 MiB 12-MP JPEG → BudgetExceeded (fast path gated on pixels only) |
| mom-capabilities · DeviceManager offline-OS identification (A54-58) | DIVERGENCE-MAJOR + disposition re-scope | **M-082** — live-module machinery (IsBootVolume chain); CheckVersion verdict covered only the dead copy |
| writer/query/CLI · M-018 junk gates (D16/B2) | **MATCH (gate restored + extended)** | **M-071 FIXED** — the rrecov pair restored byte-exact from `69507cb` + the cr_ui session/snapshot bulk walks gated (the W-lane surfaces that postdate M-018); explicit-selection stays open (the og's default-not-block semantic, pinned by test) |
| session · list scope + row shape (D12/B10/P42) | DIVERGENCE-MINOR | **N-090** — no `auto_` filter; 1 s save time; label[32]; no device-type |
| docs · PARITY-DELTAS registration gaps (Q68) | DIVERGENCE-MINOR | **N-091** — load-progress model, thumbnail superset, auto-save posture unregistered |
| preview(thumb) · refusal matrix + cache (Q68/X47) | **MATCH** | **N-092 — FIXED (session 38)**: WMF/EMF/ICO named-and-refused (all five magic forms; `MetafileAndIconFamiliesAreNamedRefusals` RED→GREEN); the cache scope DECIDED and registered (no engine cache — host-side per the in-process doctrine; modules/preview.md + cr_thumbnail.h contract) |
| carve · batch-1 bounds MID/GZ (A28/C19) | DIVERGENCE-MINOR | **N-093** — 64 MiB/512 MiB vs og 10 MB/400 MB, undocumented |
| cli · `-h` usage text + stale cite (C24/Q68) | **MATCH** | **N-094 — FIXED (session 38)**: the sessions usage line landed + CORE-API-CURRENT re-pinned (3043-3063, count 11) + the verb lock grown to four sites with the `CrUi.CliHelpListsEveryDispatchedVerb` regression pin (both directions) |
| docs · five stale texts (Q68) | DIVERGENCE-MINOR | **N-095** — 0x60002400 label; 37-toggles tail; carve.md superseded verdict; transport.md grammar; ABI-CONTRACT cites |
| transport · identity IOCTL issuance (S35/P42) | DIVERGENCE-MINOR | **N-096** — og issues GET_DISK_CONTEXT_MODE per open; core never issues any |
| health/docs · auto.txt classification (F61/Q68) | DIVERGENCE-MINOR | **N-097** — it is the FVE auto-unlock key snapshot (plaintext persistence datum) |
| abi/docs · enum refresh contract sentence (Q68) | DIVERGENCE-MINOR | **N-098** — always-fresh behavior unverified-by-text (R-86's doc half) |
| config assets · data-asset ground truths (Q68/I-010) | INFORMATIONAL | **I-027** — quota contract, json_raw shield, Aliyun caps, schtasks autostart, filter_generalraw value |
| mom-capabilities · CWindowsFS live-FS adapter | OPEN QUESTION | **OQ-S8-1** — decoded; og-side consumer untraced |
| landing verification · W1a/W1b/W1c + thumbnail + carrier | MATCH at load-bearing points | worklog session-8 record (cancel-per-node; ERR_CANCELLED-never-flattened; cancelled-never-autosaves; 27/27 batch-1 magics; carrier all-parameters) |
| FIXED-truth spot-check · 10 sampled findings | 9 CONFIRMED · 1 PARTIAL | M-018 → **M-071**; the rest verified in-code (worklog record) |


## Session-8 wave-2 rows (audited `ff879f9` lineage, corpus `be0cfe9` — the continuation lane, IDs reconciled past the parallel deep-audit lane's set)

| Module · parameter (cell) | Verdict |
|---|---|
| carve · batch-2 format coverage (C19/A28/B1) | **S-041 + wave-2 addendum** — the parallel lane's owner finding gains the full decode: 7 text-school grammars, the 18-site config-gated zone, registry geometry (59+35), the BOM battery, FCPEvt end-to-end |
| carve · WMF placeable validator grammar (A28/C19) | ~~**S-042**~~ **MATCH** (fixed this session — both WMF branches re-derived from FUN_1800264d0: checksum@+20/XOR words 0..9/reserved-dword-zero/hmf/tail gates, exact Size*2+22 extent; std branch's invented gates removed; spec-derived fixtures; Resolution in findings/FIXED/S-042) |
| carve · MDB family second signature (C19/B1) | **M-084** — "Standard ACE DB" (ACCDB 0x4C) missing |
| carve · batch-1 evidence citations (C65-adj) | **N-099** — registry regions/ctor lines/byte-order corrected (truth pass; constants shipped correct except WMF; also corrects N-093's WPG-ctor cite per erratum E63) |
| carve · progressive result emission (A31/S38-adj) | **MATCH** — carve_slice streams nodes live during the pass (cr_scan.cpp:1877-88); og live-growth equivalent (UIUX/30:14) |
| preview/thumbnail · EXIF orientation (B2/A25) | **S-043** — og rotates per tag 0x112 pre-thumbnail; core does not |
| preview/thumbnail · input eligibility (B1/P48) | **M-080 + wave-2 addendum** — the parallel lane's budget finding gains the og's 200 MB eligibility gate decode (:422428), correcting its "no input cap" claim; severity discussion recorded (SEVERE recommended) |
| preview/thumbnail · format coverage (B1) | **S-045** — video-keyframe/RAW/HEIC/PDF pipelines absent (og ships them); complements N-092's unnamed-trio + cache gaps |
| preview/thumbnail · grading side-effect (D15) | **M-083** — og promotes to grade 2 on thumbnail success (:422505-13) |
| preview/thumbnail · TIFF LZW (B1/A28) | **M-085** — compression-5 refused (mainstream variant) |
| preview/thumbnail · superset posture (A25) | **I-028** — both-axis scaling, RGBA8, PSD/PNM/HDR, bomb guard, truth gates: deliberate, registered |
| preview/thumbnail · still-image families + bounded-read doctrine + never-upscale + honest refusals | **MATCH** |
| session · auto-save trigger gates (A31/D12) | ~~**S-044**~~ **MATCH** (fixed this session — results-found / not-import / not-already-saved gates landed on the ABI + CLI twins; the byte-flag config shape registered as D-105; findings/FIXED/S-044) |
| session · auto-save naming folder variant | **N-105** — "Folder" suffix absent (deferred; cross-ref M-079's volume-label gap) |
| session · crsf reader robustness (S35) | **MATCH** — per-chunk raw fallback == og's raw-memcpy-on-inflate-failure (md :98) |
| session · light summary read (D12) | **MATCH** — SESSION_STATE+VOLUME_INFO == the og's record-1-then-record-2 list read (phrasing corrected E64; complements N-090's scope/shape rows) |
| ntfs · session-wire field set (D11/D12) | **N-100** — og persists +0x60 (total clusters, sparse-zeroed) beside +0x64; core carries +0x64 only |
| ntfs · origin-LCN stamp chain (C20) | **MATCH** (semantics) — chain cites corrected (+0x53/+0x4f first hop; FUN_180003250) |
| orchestration · device enumeration sixth source | **N-106** — GetDriverInfo.dll dispositioned OUT-OF-SCOPE by lineage (complements M-082's DeviceManager decode) |
| audit-trail · SURFACE-INVENTORY arithmetic + classification rows | **N-101** |
| audit-trail · M-024 count propagation (finding file itself) | **N-102** (cross-ref N-095's PARITY-CORE-REC site) |
| audit-trail · thumbnail API rows + heif attribution | **N-103** |
| audit-trail · ldm.md dangling ref + CAPABILITY-MATRIX HFS+ row | **N-104** (cross-ref N-095's five other sites) |

### Session-8 third lane (2026-10-28 — the driver lane + the merge/repair wave; audited core `35adcb9`, driver `323e75a`, corpus `be0cfe9`)

| Module · parameter (cell) | Verdict |
|---|---|
| driver-install · versioned coexistence across releases (K76) | **M-086** — FileVersion pinned 1.0.0.0 + no release stamping → the -Layout Versioned tag is always "10" → genuine upgrades hit exit 2 (the M-064 contract's version source starves it) |
| driver-install · WinPE UpperFilters encoding (K78) | **M-087** — regdrv-rda.reg hex(7) is 5 ASCII bytes, not UTF-16LE multi-string (the D4 handoff, now owned); og's regdrv.reg:35-36 shows the correct form |
| ci/packaging · FloorVersion pin count + CI-shape docs (K75/Q68) | **M-088** — release.yml v0.2.4 vs installer.yml v0.2.3 while CI-CURRENT.md claims ONE pin source; PREBUILT-ARTIFACTS/DRIVER-RELEASE/DRIVER-CONTRACT/CI-LOCAL/CI-SCENARIOS/README:125-132/RELEASE-NOTES describe the wiped composite-action + 3-workflow set (real consumers: installer/release/test-benchmark) |
| driver wire · extended-IOCTL fuzz coverage (Q66/C22) | **M-089** — rdabench.c:390 probes the phantom-zero 0x6000244x codes; the live 0x6002440/44/48 handlers receive zero fuzz iterations |
| driver wire · session-code count prose (C22/Q68) | **N-107** — 14 current-tense "15/18-command" sites vs the code's 19 (the D3 handoff, full inventory; core's DRIVER-CONTRACT.md already says 19) |
| driver wire · identity IOCTLs + Information×512 + LBA scaling + NAS/Cloud exclusion | **MATCH** — re-verified first-hand this session (rda_ioctl.h:63-65/146, rda_readwrite.c:269, rda_session.c:1122, rda_disk.c:191 vs eprdtdrv-sys--driver-internals.md:68,81-84; binary dword probe: all 19 codes in EUDCPEPM.sys) |
| docs · engine-constants row set (Q68) | **N-108** — docs/PARAMETERS.md:78-79 still carry 4-8 MiB / 8-signs / 16-bytes / offset-512 (code: 1 MiB+4 KiB / 12 / 48 / 1024 / mime-80); the SCAN-ENGINE-DELTA "fixed this session" claim corrected in place |
| audit-trail · A-007 evidence cites | **REPAIRED** — the dangling a006-extents-off-by-two.md cites re-pointed at the real a007-extents-off-by-two.md (dated note in the finding) |
| audit-trail · corpus errata | **E65/E66** — the census :99 0x11c gloss (284 = FreeHand 11, MTS/M2TS list-only) + the retdec qword-truncation tooling caveat (the CHM blocker's real cause) |
| carve · batch-2 remainder + WMF grammar | **MERGED** — this lane's duplicate S-041/S-042 filings merged into the continuation lane's owner files as convergence addenda (two lanes, two decompilers, one conclusion on WMF; the CHM/FH11 asm decode donated to S-041) |


## Session-8 lead-lane rows (FINAL numbering past the third lane, audited core `ff879f9`, corpus `be0cfe9` — 2026-10-28)

| Module | Lead-lane findings (final IDs; the parallel lanes' rows are in their own sections) |
|---|---|
| carve | **M-090** (DMG koly walk + DMG-specific cap — erratum E67), **M-091** (the MDB page-chain walk decoded), **S-047** (MPEG-TS/M2TS), **M-092/M-093/M-096/M-098/M-099** (HTML / XML+SVG / MOBI / HIK / AEP), **N-111..N-113, N-115** (JP2 / LZH / text school / OfficeTagFromXML) — school-2/3 children of S-041 |
| session + ABI | **N-116** (list row ordering), **N-117/N-118** (W1 doc drifts), **I-029** (driver-code provenance) |
| query | **M-101** (Versions/Tags branches), **N-126** (Unsaved File) |
| preview | **M-100** (browse-inside), **M-102** (thumbnail ABI) |
| boot repair | **N-121** (ESP automount), **N-122** (NVRAM create/delete) |
| CLI / transport | **N-123** (N-124 FIXED session 10 — the transport.md SUPERSEDED annotation landed in 2025554; see evidence/s10-reverification-sweep.md) |
| orchestration / docs / structure | **N-125** (disconnect states), **N-119** (D-88 premise), **N-120** (pointer drifts), **I-030** (module-map holes) |
**Session-9 changes (2026-10-28, SA-9 lane — audited core product code `35adcb9` / rda-driver `323e75a` / corpus `be0cfe9`; two-repo scope; RECONCILED at push time into the session-8 board — 7 convergent filings merged as addenda, 1 unique finding kept):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| transport · EPMNtDrv v2 identity IOCTLs + async completion + \Volume\{N} chain | **MATCH** | the never-audited surface verified by this lane: our driver carries 0x60002400 GetProcName (in==4 u32 PID, out>0x207), 0x60002404 GetFileObjName, 0x60002408 GetDiskContextMode (out==4, bound handles only), method-aware pass-through, ×512 Information — the C-sheet corpus decode vs the E-sheet code read, cross-checked (evidence: the SA-9 context sheets; SURFACE-INVENTORY rows flipped) |
| transport · EPMNtDrv trio naming + PID handshake | **MATCH** | the never-audited surface verified by this lane: the PID-token byte-sum transform re-derived at CREATE, mismatch → STATUS_INVALID_PARAMETER; \Device\<UPPERCASED service name> + EPMNTDRV fallback; DO_DIRECT_IO; benchmark-proven 64/64 |
| driver-install · WinPE filter Start posture | DIVERGENCE-MAJOR (parameter added to M-087) | **M-087 + SA-9 addendum** — the demand-start (Start=3) service + active UpperFilters cannot attach at volume-stack build; the og's filter service is Start=0 BOOT_START (EUDCPEPM in regdrv.reg) — the parameter their M-087 lacked |
| transport · driver doc-vs-code truthfulness (admin gate / S12 / 512-byte prose / unreachable default / 57th check) | DIVERGENCE-MINOR | **N-127** (the lane's unique survivor — renumbered from the collided N-091 at reconciliation) |
| V-lane · W1a/W1b/W1c session surfaces (list/progress+cancel/auto-save) | **MATCH** | the landings' corpus claims verified read-only (the D-sheet): v5 save_unix tail = the og's SYSTEMTIME mirror; per-node cancel semantics; completion-only auto-save (the og's own results-found/not-saved gates independently decoded by the parallel lane as S-044 — convergent) |
| V-lane · NTFS origin-LCN carrier | **MATCH** | both stamp sites + clone + nflags bit-8 nonzero-gate verified against the +0x64 decode (part1:85 erratum E68) |
| V-lane · thumbnail engine | **MATCH (documented superset)** | stb+native TIFF+EXIF fast path; the 20171/20172 IPC + UI caps n/a-by-layer; 256px constant not corpus-pinned (the parallel lane's thumbnail findings S-043/S-045/M-080/M-083/M-102 own the deeper gaps) |
| V-lane · installer S-038 steps 2-3 | **MATCH (the registered OS-gate divergence)** | the 13-parameter contract verified landed; step-4-accepted at run 40684304129 |
| G-lane · gap-hunt candidates | **MATCH (covered)** | Device.md CDiskDriveLayoutReadWriter = the lost-partition apply sequence (modules/lost-partition.md:205-218); RegLib (mom-capabilities:94-97 + health.md:63); Win32Bcd writer family (boot-repair.md); NTFSLib (ntfs.md:357); VideoViewer (I-015); CWindowsFS/offline-OS stay census stubs (no corpus behavior parameters — the parallel lane's M-082/N-106 own the adjacent dispositions) |
| carve · WMF placeable gate | MATCH (fixed this session) | **S-042 FIXED** (the std-WMF stricter-gates observation landed too — the og's key+params-only gates; evidence/decompiled-quotes/s9-wmf-placeable-gate.md; findings/FIXED/S-042) |
| carve · CRW/MRW key attribution | DIVERGENCE-SEVERE → **FIXED** | **M-081** (the parallel lane's filing, FIXED at `9e5dedc` — the CRW re-attribution landed; this lane's instruction-level registration quotes merged as the SA-9 convergence addendum, evidence/decompiled-quotes/s9-crw-mrw-registrations.md) |
| carve · batch-2 format set | DIVERGENCE-SEVERE | **S-041** (the parent) + the per-format children (S-047 MPEG-TS, M-092 HTML, M-093 XML/SVG, M-096 MOBI, M-098 HIK, M-099 AEP, N-111 JP2, N-112 LZH, N-113 text school, N-115 OfficeTag) + school 1 LANDED (`b92ebef` CHM/ONE/MNY/EMF/SWF/3DS) — this lane's consolidated decode table merged as the SA-9 addendum (evidence/decompiled-quotes/s9-batch2-registrations.md, the instruction-level CHM disassembly s9-chm-gate-asm.md corroborating E66) |
| driver-install · FileVersion pin | DIVERGENCE-MAJOR | **M-086** (the parallel lane's filing — this lane's convergent decode merged as the SA-9 addendum: the CI tag==resource gate proposal, evidence in the finding) |
| driver-install · regdrv-rda.reg hex(7) | DIVERGENCE-MAJOR | **M-087** (the parallel lane's filing + this lane's Start-posture parameter addendum) |
| transport · session-code count prose | DIVERGENCE-MINOR | **N-107** (the parallel lane's filing — 14 sites, supersedes this lane's 12-site list) |
| carve · MID/GZ/PSP/SIT batch-1 caps | DIVERGENCE-MINOR | **N-093** (the parallel lane's broader filing — PSP/SIT included; this lane's MID/GZ ctor quotes corroborate, evidence/decompiled-quotes/s9-mid-gz-bounds.md) |
