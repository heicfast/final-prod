# CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

**Session-25 changes (2026-10-26, DFX25 — INFO tier closed into FIXED/, tracked-open resolution wave):**
the 22 acknowledged INFORMATIONAL findings (I-001..I-024) moved from
`findings/INFORMATIONAL/` to `findings/FIXED/` — the directory is now empty
and the `ack` cells below denote acknowledged-and-filed-in-FIXED deliberate
divergences (each file carries its ACKNOWLEDGED Resolution; pin tests green on
`716d121`, 737/737). No verdict changed — `ack` was and remains MATCH-by-
deliberate-divergence; only the filing location moved per the user directive.
Tracked-open dispositions recorded per-OQ in the session's worklog entries.

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
| exfat | M-013 FIXED (s16: the dual-form deleted-set checksum gate + the parked-set reconstruction oracle — FUN_18001eba0/FUN_1800203a0 decompile-exact; findings/FIXED/M-013), M-014 (structural gates), M (S-011 + S-013 fixed), N-003 FIXED (premise recorded REFUTED by M-013), OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | **M-054 (S10 NEW-FINDING) FIXED — allocation-bitmap bit i vs cluster i+2 shift (grading + the S-009 producer wrong on real volumes; spec §7.1; producer pin test added)**; M (debt) |
| ntfs | **M-007 FIXED (session 11: single_link_per_record parity default + the repaired per-parent fan-out, D-65)**, M-008 (S2); N-011 FIXED-ALREADY + N-012 FIXED (minors session 2026-10-26, D-52); I-001 ack; S-012 FIXED (two-pass: pass-1 membership + pass-2 dump walk + size-0 rescue; convergent dual-agent fix); **~~M-043~~ **FIXED (s14, FIXED/M-043: content proof at probe; backup-fallback premise corpus-disproven)** (S5b: content-proof recognition + scanner backup-boot)** | M (S-001+S-012 fixed: membership keep), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M; N-021 (S2); **M-045 (S5b: boot-gate family — bps/SPC/label/numFATs/cap/band-retry; resolves S-006's FATX remainder)** | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M (S-011 + S-013 fixed), N-003 FIXED, OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | M (debt) |
| ext4 | S-004 FIXED, S-025 FIXED (two-pass reparenting, D-58/59 siblings), S-026 FIXED (meta_bg resolver, D-58), S-027 FIXED (backup-SB rescue, D-59); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks; S-027 fixed: backup-SB positions + s_block_group_nr) | S-004 FIXED, S-025 OPEN | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, **S-003 FIXED (s5 + closing-session verification: JIB layout corrected, gates, strict validator, fallback salvage)**, **S-021 FIXED (s5 + closing-session verification: attributes branch + marker derivation resolved)**, N-005 FIXED, N-038 FIXED, **A-007 FIXED**; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) **N-047 (S3) FIXED s22** (volume-node naming = registered partition-layer divergence, modules/hfsplus.md) |
| ext4 | S-004 FIXED, S-025 FIXED (journal reparenting: orphan map + iNode naming + claim interaction), S-026 FIXED (meta_bg resolver, D-58), S-027 FIXED (backup-SB rescue, D-59); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 meta-group desc blocks; S-027 backup-SB positions + s_block_group_nr) | S-004 FIXED, S-025 FIXED (two-pass realized as harvest+claim checks; parent map + topo anchors) | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, **S-003 FIXED (s5)**, **S-021 FIXED (s5 — journal harvest)**, N-005 FIXED, N-038 FIXED; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) || btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010 FIXED (lossless mapping)**, I-007 (S2) | M; **M-010 FIXED**, I-007 (S2); **S-028 FIXED** (xattrs: DOSATTRIB bits + user.* records + the compression-property tag) | M | M | M | NA | NA | M | M | M | M |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved); **S-020 FIXED (s4)**; **M-026/M-027 FIXED (s4: stale-page sweep + identity keep-one + recycle routing)** | M | M | **M-022 FIXED (s4)** | M | NA | NA | M | M | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-031 FIXED (carved nodes flag Deleted - og FLAGS=0x20 const); S-007 (primary FIXED s5), M-005 | M | S-007, **N-008 FIXED s11** (sector-strided salvage + no clean-archive inflation + ZIP64 chain; gate half via N-068); M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s10)** (consent-gated apply module + `rrecov apply`; findings/FIXED/S-029) | M | NA | NA | M | M | M | M **N-035 (S2) FIXED s22** (data-area-anchored containment/overlap extents; both extents on Candidate; fail-before executed) |

| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s10)** (consent-gated apply module + `rrecov apply`; findings/FIXED/S-029) | M | NA | NA | M | M | M | M |

| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s16: the apply/table-write path — consent-gated transactional writer, corpus IOCTL chain, undo; findings/FIXED/S-029)** | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | M | M (D12), **N-014 (S2) FIXED s22** (atomic staging + commit-by-rename) | **M-042 (S5b: lostpart.dat resume re-validation + layout snapshot missing)**; N-049 (S5b: record richness) | NA | M; N-014 (S2) | NA | NA | M | M | I-005.7 ack | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2) FIXED s7 (FIXED/ move s11)**, **N-015 (S2) FIXED s9 (`915bce3`; FIXED/ move s11)**, **M-055 (S22 NEW-FINDING) FIXED — the win layer those fixes landed into was uncompilable; four structural repairs + the win_syntax_check gate** , N-009d | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock), **N-064 (S5: password-dictionary unlock aid absent)** | NA | N-009a; N-055 (S5) FIXED by the S-024 fix leg (0xD1 + 4-status grading) | NA | **M; S-024 (S3) FIXED — decoded-truth upgrade (s6)**: FUN_0040cdf0 exact (family-gated SSD-life, bad-count/caution/graded aggregation, temp never graded, NVMe family-0x13 path; analogs superseded — S-024 Resolution addendum + evidence s6-health-grading-decode.md) | NA | NA | NA | M | M | M (M-036 FIXED s6: NVMe full row set + units→bytes; ABI v2); **N-032 (S3: CSMI/USB-bridge transports)**; **N-042 (S4) FIXED s7 `3678cbf` (FIXED/ move s11)** |
| rawfixer | M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | M | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | ~~M-005~~ **FIXED (s14, FIXED/M-005)** | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4) FIXED** (FIXED/S-023: post-scan grading pass + RecoveryAssistant statistics + ABI regrade); ~~N-043~~ **FIXED (s18 DFX18: CR_EVT_HEARTBEAT — the honest 5 s liveness watchdog, standing truth not synthetic advance; TSan-clean)** | NA | M | M | M | M |
| visualization | S-009 (partial-FIXED-in-tree, finding file stale — N-020) | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M |

| ext4 | S-003 FIXED (HFS+ journal differential harvest, landed with S-021), S-004 FIXED, S-026 FIXED (meta_bg resolver, D-58); **M-010**, I-006 (S2), I-007 (S2); **M-037 FIXED (s10: resync-slide damage model — findings/FIXED/M-037)** | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks) | S-025 OPEN (theirs: journal reparenting), S-004 FIXED | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, S-003 FIXED, S-021 FIXED (s5 — journal harvest, the og's PRIMARY deleted surface), N-005 FIXED, N-038 FIXED; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) |
| btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010**, I-007 (S2) | M; **M-010**, I-007 (S2) | M | M | M | NA | NA | M | M | M | M **N-047 (S3) FIXED s22** (ROOT_REF subvol names + 0x80 file attribute base, corpus :3574-3577; both fail-before-proven) |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved — parity); **S-020 FIXED (s4: resident+all-rows+paged+granularity)** | M | M | **M-022 FIXED (s4: dual grammar + gen tag)** | M | NA | NA | M | **M-031 FIXED (s10: per-file progress ticks — findings/FIXED/M-031)** | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), **M-002 FIXED (s10: torn chain keeps walked prefix — findings/FIXED/M-002)** | M | M (A-002 FIXED; A-001 fixed), **M-046 (S5: LDM GPT-path 512-hardcodes)** | M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007, ~~M-005~~ **FIXED** (FIXED/M-005, s14: the allocation-bitmap gate in both dispatch layers + telemetry; bench 1.88x on 50% used), **S-031 (S5: carved nodes never marked deleted)**, **S-032 (S5: archive inner-entry inventories)** | **S-031 (S5)**, **M-051 FIXED (s9)** (checksum/anchor/note/flag carriers; nflags-bit4 round-trip) | S-007, N-008, **N-051/N-052/N-053/N-054 S12 FIXED (convergent landings: the parallel session-11/7 lane's b2f48c6 canonical + this session's N-076 add-on)**; M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005, S-032 (S5: multi-part stitching) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | **N-063 FIXED s17 (og per-analyser names; File%06u = registered default)**, **M-048 (S5: user-signature extensibility)** | M |

| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007 **FIXED (primary set + full audio family)**, M-005 | M | S-007 **FIXED** (niche ~25-format tail precisely scoped in the finding), **N-008 FIXED s11** (sector-strided salvage + no clean-archive inflation + ZIP64 chain; gate half via N-068); M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | M | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | **M-047 (S5: stats round-trip asymmetry)**, **M-050 (S5: node-model carriers)**, S-031-adj (S5) | M-047, M-050, **N-059 (S5: btrfs/zfs extras)**, N-014 (S2) | M | NA | M; N-014 (S2); **N-058 (S5: load validation deltas)** | NA | NA | M | ~~N-060~~ **FIXED (s18 DFX18)** | I-005.7 ack; ~~N-061~~ **FIXED (s18 DFX18)** | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2) FIXED s7 (FIXED/ move s11)**, **N-015 (S2) FIXED s9 (`915bce3`; FIXED/ move s11)**, N-009d, **N-056 (S5: WinDestIo temp leak on failed rename)**, **M-055 (S22 NEW-FINDING) FIXED — win-layer buildability restored + gated** | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock), **N-064 (S5: password-dictionary unlock aid absent)** | NA | N-009a; N-055 (S5) FIXED by the S-024 fix leg (0xD1 + 4-status grading) | NA | **M; S-024 (S3) FIXED — decoded-truth upgrade (s6)**: FUN_0040cdf0 exact (family-gated SSD-life, bad-count/caution/graded aggregation, temp never graded, NVMe family-0x13 path; analogs superseded — S-024 Resolution addendum + evidence s6-health-grading-decode.md) | NA | NA | NA | M | M | M + **N-032 (S3: CSMI/USB-bridge transports)**; **N-042 (S4) FIXED s7 `3678cbf` (FIXED/ move s11)** |
| rawfixer | **A-005 (S5 ACUTE: 4Kn stack-buffer overflow)**, M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | **M-046 (S5: 4Kn exFAT plan truncation)** | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | ~~M-005~~ **FIXED (s14, FIXED/M-005)** | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4: no post-scan grading engine / RecoveryAssistant — S5 addendum: OQ-S4-1 RESOLVED, table dumped)**; ~~N-043~~ **FIXED (s18 DFX18: CR_EVT_HEARTBEAT — the honest 5 s liveness watchdog, standing truth not synthetic advance; TSan-clean)** | NA | M | M | ~~N-061~~ **FIXED (s18 DFX18)**, ~~N-062~~ **FIXED (s18 DFX18: the memory monitor on the N-048 seam — check_memory_pressure + the platform sampler on the watchdog cadence + memory_warn_mb appended config; never scheduling-coupled; fail-before RED executed)** | M |
| visualization | **S-009 FIXED (S8 — the free overlay: engine bitmap truth produced + persisted + snapshot-carried; `kCellAllocFree` now real)**; ~~partial-FIXED-in-tree~~ | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M; **N-057 (S5: Ebr enum + budget-growth test debt remain; the bad-overlay coverage item paid by S-009)** |
| abi (cr_ui) | NA | M | ok | NA | M | M | **S-009 FIXED (S8: the free overlay rides the same `cr_ui_disk_map` ABI — no signature change; snapshots carry errors/free-map/view)** | M | M | M | ~~N-040~~ **FIXED (s18 DFX18: D4 closed — truthful era labels + §9 coherence; D1/D2/D3/D5 verified already-landed via §8/S-009/§6/M-029; three new pins incl. the fail-before-proven AbiVersionAnnotationsAreTruthful)**; N-041 (S4: dead code); I-016 (S4: factory register + FS-code spaces) |
| cli | N-009c | NA | M | NA | M | NA | M | NA | M | M | ok; N-020 (S2); **N-044 (S4: --deep naming hazard)** |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2) FIXED s7 `b1f9210` (FIXED/ move s11)**, N-019 (S2), **N-020 (S2) FIXED (doc-accuracy batch; FIXED/ move s22)**; M-006 FIXED; **M-053 (S8 NEW-FINDING) FIXED — the C-ABI consumer check itself was dead since M-015 (stale fixture → BAD_MAGIC + NULL-deref crash on every run, both lanes); fixture fixed + precondition bail-out, gate live again**; **M-055 (S22 NEW-FINDING) FIXED — win-layer buildability: four structural breaks repaired + the win_syntax_check parse gate (Q67)** |
| preview (S4 NEW) | NA | NA | NA | NA | NA | NA | NA | NA | **M-028 (S4: caps bypass — executed proof; provenance misattributions) FIXED (s21 — read_cost-gated budgets + checked_read; findings/FIXED/M-028)**; M-024 landed s15 (CarveFlags::disabled_formats) | I-013 (S4: architecture register — og socket protocol vs in-process reader) |
| query (S4 NEW) | **M-029 (S4: sort/paging/bucket-IDs/server-side push)** + ~~M-023~~ FIXED (FIXED/M-023: category source now the generated table) | NA | NA | NA | NA | NA | NA | NA | NA | **M-029, N-040 (frozen-doc unit lie)** | M-029 (S4); S-023 grading rows now carry grades + sort key 6 |
| transport (S4 NEW) | M-030 (S4: WinPE gate, stale-handle ladder, terminal OFFLINE, 32-disk cap) | M-030 | M-030 (IOCTL numbers = own driver's, grammar shape M) | M-030 (chunking provenance corrected; OQ-S4-3 perf open) | M-030; **S-030 FIXED (S8 — image scan targets: flat/VMDK/VHD through `cr/storage/cr_image`, CLI --image, session v3 identity tail; D-64 registers edisk/dmg exclusions + VHDX/parent-chain remainders)** | NA | NA | M-030 (silent RDA fallback) | M-030 | M | **N-041 (S4: dead RdaClient + volume_letter_to_disk)**; I-016 (S4: wire register) |
| mom-capabilities (S5b NEW) | **M-043 (S5b: NTFS content-proof + backup-boot) FIXED s15**; **M-044 (S5b: UDF family) FIXED (s21 — the og-exact AVDP→VDS probe + FsFamily::Udf 11; findings/FIXED/M-044)**; **M-045 (S5b: FAT gates) FIXED s15**; N-050 (S5b: boot-only recognition) | **M-042 (S5b: lostpart persistence semantics)**; N-049 (S5b: record richness) | I-020 (S5b: corpus-doc corrections register) | **S-029 FIXED (s10)** (apply module + CLI; PARITY-DELTAS D-68/D-69) | **M-040 (S5b: BCD/NVRAM/boot.ini/chkdsk family)**; **M-041 (S5b: check phase + error taxonomy 0x1001-0x100e)** | **S-030 (S5b: image scan targets) FIXED (s8 — findings/FIXED/S-030; module row: modules/imaging.md)** | ~~N-048~~ **FIXED (s18 DFX18: the cr::log seam — host sink default-NULL + START/END/stage/checkpoint/candidate/read-error sites; CLI --log wired as the EPMLOG-role sink; D-75 registers the single-process divergence; fail-before RED executed)** | **I-019 (S5b: EPM tool-ops scope register)**; OQ-S5-1 (CheckVersion consumer) | M-040 pending (S-029 decided: implemented v1 MBR — D-68) | NA |

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
| exfat | B3/B4/B10 | **M-014** (structural gates: secondary_count ∈ [2,18], timestamp sanity, empty files dropped, name-entry validation) |
| hfsplus | B3/B5/B7/C20 | **N-028 — FIXED (minors session, 2026-10-26)** (sink filter at both record sources ✓; rsrc superset registered D-53; 'prdf' folderization ✓ — see findings/FIXED/N-028) + **N-038 FIXED** (ownerFlags 0x20 fork selection) + **N-005 FIXED** (block bound 16 MiB + product sanity) |
| btrfs | B1/B2/A26/B5 | **M-020** (chunk-profile machinery: RAID10 gate, 2-copy rotating mirrors vs stripe-0-only; default-subvol pick) |
| zfs | B11 (SA registry) | ~~M-038~~ **FIXED (s19)** — registry-driven SA decode; findings/FIXED/M-038 |
| zfs | B1/B2/A25/D15 | ~~M-019~~ **FIXED (s19)** — first-entry fallback for every DVA (foreign reads counted), fletcher4 {7,0} gate over raw bytes + 32-byte compare + next-DVA retry, L0 extents planned at the fallback position; fixtures now checksum-carrying; 2 fail-before + 1 pin; findings/FIXED/M-019 |
| refs | B1/C19/C20/A25 | **N-026** (checkpoint dynamic body offset vs hardcode; two-sided boot gates; second-copy location START+MID vs START+TAIL) |
| table+ldm | B1/C20 | ~~M-016~~ **FIXED (s19)** — candidate list {2,0,1,2045,2046} = og probe order + tail superset behind; config+0 disk parses, +2 wins conflicts (committed_seq proof); 3 tests, findings/FIXED/M-016 |
| table+ldm | B1/A27/X49 | **N-025** (config-size gate; RAID-5 zero-interleave 64 KiB default vs core refusal) |
| lost-partition | A30 | ~~M-017~~ **FIXED (s19)** — the og's content model end to end: weighted count + bonus + 500 table-match (polarity inversion repaired), ln/ln permille, containment-drop, stable sort, the iVar16 size-match promotion re-decoded; findings/FIXED/M-017 |
| carve | B1/X47/X51 | **I-009** (byte-stepped probing = detection superset; block-shape constants) |
| carve | B2-adj/R54/R57 | **M-021** (video-repair pre-scan hook absent) |
| carve | B1/B2/C19/A28/Q65/P46 | **S-017 — FIXED (session 3)** (ftyp-family magics re-keyed on the box TYPE; F8 self-probe guards the unreachable-magic class; naked-moov salvage mirrors the og MovAnalyzer conservatively, tried last) |
| carve | B2/A28/B3 | **S-018 — INVALID (session 3)** (premise refuted first-hand: the og's modern JPEG analyser IS EOI-gated — 0xD9 appears as Ghidra's signed char `-0x27`, 5 sites; `uVar21 == 0 -> epilogue`; the audit's literal `0xd9` grep missed them; residual = legacy fallback sub-analysers + streamed continuation -> OQ-S3-JPEG) || carve | D13/B10 | **N-027** (carved-name split inside the core; runtime per-format config surface; ZIP padding inversion + two-tier version gates — refines N-008) |
| session | B10/D12 | **N-029 FIXED s17** (N-016a: maximal-subpart policy, DFX12 lane; N-016b: raw name bytes persisted [nflags bit 32] + og-parity restore naming from raw; findings/FIXED/) |
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
| carve | A42/A45 | ****M-024 FIXED (s10: disabled_formats surface — findings/FIXED/M-024)**** (no runtime format-filter surface vs og RawConfig.ini [Filter], 37 toggles, STL/VPJ default-off) + **N-033** (registry exports consumed by FS engines — no core ABI) |
| carve | P47–P53/X47/X50 | **M-025** (8 MiB ping-pong + sector dispatch + pending-object streaming vs 1 MiB byte-granularity stateless; M-021's hook depends on its state machine) |
| carve | A25/A28/C19 | **N-031 addendum** (u16 equal_range chains = insertion order; id-8 continuation records; TTF-vs-MDB u32 pre-emption; TIFF-family same-class relabeling — kTrialOrder equivalence verified per family) |
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
| err+abi | C23/D14/F35/Q46 | **M-049 (S5)** (error model: flat 28-code 8-hex enum with a false "reproduces the reference convention" header claim vs the og's 12-digit documented codes + error-object architecture) |
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
| carve | B10/A28/Q68 | **N-063 FIXED s17** (default_namer "File%06u" was a core invention — now the registered deliberate default D-76; MP4/MOV per-analyser template decoded + replicated quirks; embedded_name carrier; findings/FIXED/) |
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

| Module | Parameter(s) | Verdict change (evidence in the finding + `evidence/decompiled-quotes/s6-*`) |
|---|---|---|
| fat | B4/B5/D11/A29 | **M-052** (recycle-bin $I/$R pairing missing — og IRecycleBinNode2UI bridge; NTFS-side model exists in-repo) |
| exfat | B1/A25/S35 | **N-067 FIXED s11** (upcase-table validation — og shape gates + checksum32 stream check, soft stats signal; findings/FIXED/); B4/B5/D11 **M-052** (recycle mirror) |
| carve | A25/D11/P42 | **S-032 (converged)** (archive inner-file inventories missing) |
| carve | B1/C19 | **N-069 FIXED s11** (CAB mscf case key — FMT_CAB_LC dual row + two-spelling validator re-check; findings/FIXED/); B1/A28 **N-068 FIXED s11** (LFH sanity gates decompile-exact; EOCD half FIXED-ALREADY — the CD anchor + EOCD reconciliation already cover it; findings/FIXED/) |
| carve | A28/A29/D15/B3 | **N-070** (overlap scoring + two-generation fallback); **S-007 addendum** (bare-MPEG dispatch gap — 18+ u16 sync keys vs ID3-only; family roster with provenance) |

| carve | B1/C19 | **N-069 FIXED s11** (CAB mscf case key — FMT_CAB_LC dual row + two-spelling validator re-check; findings/FIXED/); B1/A28 **N-068** (ZIP EOCD anchor + LFH sanity gates) |
| carve | A28/A29/D15/B3 | **N-070 (S5) FIXED s22** (overlap scoring: accumulated base+verdict ranking implemented, fail-before CR2-vs-DNG; two-generation fallback REGISTERED as D-75 — legacy bodies undecoded); **M-056 (S22 NEW-FINDING) FIXED** (TIFF-family size basis mix — multi-strip TIFFs carved at the 256 MiB cap); **S-007 addendum** (bare-MPEG dispatch gap — 18+ u16 sync keys vs ID3-only; family roster with provenance) |
| session | D11/D12/B4/B9 | **S-031 (converged)** (carved files: deleted flag + timestamp + validator payload dropped at emission; sessions round-trip the misstatement) |
| session | D12/S36 | **N-066** (identity card: firmware/WWN/interface + 512-byte boot snapshot vs model/serial/CRC32); A35/D12/F35 **N-078 FIXED s17** (truncated carve_note silent-OK exit — the c.fail-less break; filed+fixed, renumbered past the parallel lane's N-077 leak finding; findings/FIXED/) |
| session/tree | D11/D12/P42 | **M-051+N-059 (converged)** (type-registry ID + per-FS secondary payload slots missing) |
| writer | A27/S35/P46 | **N-065** (collision `name_N`/cap 10000 vs ` (N)`/cap 64/`_dup`; mkdir 0/0xB7-only; **D-19 premise refuted** with decompile evidence) |
| health | P46/P68 | **S-024 addendum** (cr_health.h claims 0xD1/0xD8 never issued — doc-truth sub-item; capability gap stays S-024) |
| storage | B8/P42/A54 | **N-071** (enumeration breadth: 5-stack fan-in vs PhysicalDrive loop) |
| storage | P47/P48 | ~~N-045~~ **MATCH (registered divergence)** (FIXED/N-045, session 18: 8 MiB/256 KiB reactive-LRU vs og 4 MiB prefetch engine REGISTERED as PARITY-DELTAS D-72 — budget + algorithm, corpus cites re-verified first-hand; doc-truth pin `Cached.ScanCacheBudgetIsTheRegisteredDivergence` keeps source constant + D-72 row + cr_cached.h divergence block in lockstep; no code change, the alignbench sweep owns the tuning decision) |
| storage | S35/Q43 | ~~N-046~~ **MATCH** (FIXED/N-046, session 18: `PartitionSlice::refresh_geometry()` — the og's RefreshSectorSize-from-parent surface (FUN_140643870) with revalidation hardening over the og's blind store; Slice.GeometryRefreshAfterMediaChange + Slice.GeometryRefreshRefusesInvalidatingChange; orchestrator auto-refresh excluded and documented — the corpus exposes no scan-path caller) |
| ext4 | A27/S35/B2 | ~~N-073~~ **FIXED-ALREADY** (FIXED/N-073, session 14: N-004's transport-level CatchBuffer landing covers the engine path — 3 engine-path pin tests; the S35 row in modules/ext4.md carries both N-004 and N-073 closed) |
| btrfs | P47/P48/S36 | **N-072** (BtrfsCache.txt re-scan cache); **OQ-S6-2** (RC4 PRGA target) |
| zfs | A25/S35 | **N-072** (label import polling); INFO-register (lzjb/zle/gzip decode — no divergence claimable, evidence ledger only) |
| lost-partition | A30/A37/D12 | **I-024** (PartitionRecoveryD generator-sets + .dat persistence register) + **OQ-S6-1** (decode request); **M-017 addendum** (CalcPartition class evidence) |
| orchestrator | A38/A41/P42/A44 | **N-075** (event-bus query-signals + pull-progress convention + typed IUIProgressInfo); **I-023** (DRWCfg scope register) |
| visualization | P42/D-model | **S-009 addendum** (stale premise corrected — surface landed; residual: kCellAllocFree dead, VolumeBitmap discarded) |
| query | P42 (sort Type) | M-029 addendum (path-group CLOSED session-6) + **session-11: Type sort CLOSED (SortKey::Type, ABI code 7, iconIndex order) + the modified-unit doc lie fixed (FILETIME, pinned) + paging documented as the deliberate difference (no og page-size constant); REMAINING: bucket-ID translation + general-raw naming (decode-blocked, next steps recorded in the finding)** |
| abi (cr_ui) | P42 | **N-033 addendum** (carve-format registry landed — ABI sub-item CLOSED; CLI listing sub-item remains) |
| quality (cross-cutting) | P46/P68 | ~~N-074~~ **FIXED (s18 DFX18 canonical `479b0c7` + convergent s22 DFX22 landing reconciled in the same-window merge — both lanes verified all five sub-items first-hand independently; pin Health.SmartCommandListMatchesImplementation fail-before-proven)** |

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
