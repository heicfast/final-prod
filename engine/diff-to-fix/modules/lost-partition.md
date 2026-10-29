# modules/lost-partition.md — lost-partition module audit verdicts

## S12 addendum (2026-10-29 — the session-2 deep-dive — the table/GPT/EBR surfaces)

New active findings on this module: ~~**M-123**~~ **FIXED (S47 MAJOR-first fix
lane, `10620f04`)** — FUN_1800078d0's full gate set landed in `load_gpt_side`
(revision 0x10000, header_size==92, reserved==0, the mirrored position pair,
usable-range bounds, entries outside the usable range — structural gates before
the CRC, og order; N-164's deliberate strictness kept; salvage path unchanged by
design; 9 tests, 8 RED pre-fix; P1 detection / P20 struct-validation → MATCH);
~~**M-124**~~ **FIXED (S47 MAJOR-first fix lane, `dcb123dd`)** — the EBR link type
0x85 joined the Extended arm of `mbr_type_to_kind` (Partition_decompiled.c:1006's
`-0x7b` compare feeds the same link path as 0x05/0x0F): whole Linux-extended chains
walk, the bogus Other-volume at the link target gone (P1 detection / P26 tree-walk →
MATCH; tests Ebr.LinkType085WalksChain / Ebr.LinkType085SameBasisAs05 /
Mbr.Extended085NotEmittedAsVolume, all RED pre-fix);
~~**M-132**~~ **FIXED (S47 MAJOR-first fix lane, `d746a75b`)** — the backup
probe at device-last FIRST (FUN_180006e20:4567's fixed position; the declared
backup_lba a registered fallback), the dead-LBA1 recovery kept + registered
(PARITY-DELTAS D-124a/b; the pin test Gpt.DeadLba1BackupRecoveryRegistered);
**N-161 FIXED (Session-44 minors lane — the first≤last span gate + the named-invalid
counter, RED→GREEN proven; findings/FIXED/N-161)** (GPT entry span underflow);
**N-164 FIXED** (the deliberate GPT strictness registered as PARITY-DELTAS D-116).
Verified-MATCH: EBR base/rel chain math, MBR LBA fields,
GPT type-GUIDs, the salvage ladder's two-step order, LDM span/stripe/RAID-5 math
(post-S-052, algebraically identical to FUN_180086a90).


Reference: PartitionRecoveryD.mo + Partition.mo + CalcPartition classes per corpus
artifacts `PartitionRecovery/mo-module-map.md`, `SearchDLL/zfs-vdev--mftmirr--calcpartition.md`
§3, `PartitionRecovery/pending-operation-engine.md`; og binaries (driver layer out of
scope for this module — see rawfixer).

Core (audited commit `67193b6` + concurrent `03ddcd2` GAP-S6-6 fix in tree):
`src/core/lost/cr_candidates.cpp` (301 LOC) + `include/cr/lost/` +
`tests/unit/test_lostpart.cpp` (188 LOC — thin).

## Behavioral

- **B1 detection** — `M` on the four implemented families (boot-sig 0x55AA + OEM
  dispatch, evidence-point probes with $MFT liveness for NTFS, FAT-area probe, exFAT
  geometry, GPT header shape); ~~**S-006**~~ **FIXED (fix session): ext probes =
  the og's decoded CExtX accept gates (parity); HFS+/Btrfs/ZFS/ReFS anchors =
  documented superset (D-50; the og has
  CBlockParserLinux/CExtXBlockParser/CFatxBlockParser).
- **B3** — `M`-note: FAT catch-all relies on BPB strictness (noisy by design, og's
  CalcPartition likewise heuristics-driven; scoring internals undecoded — OQ-T9).
- **B6** — `M` device-tail size fallback (audit row 31b) with the GAP-S6-6 containment
  fix (fallback-sized outers can no longer swallow peers — the dual-partition bench
  shape now surfaces both; fix landed in-audit-window with fail-before/pass-after
  tests).
- **B8** — `M` candidates feed the orchestrator's parallel engine dispatch.

## Algorithms

- **A30 candidate scoring** — og's CalcPartition scoring walk is corpus-undecoded
  (B-partitionrecovery OQ-3); core's evidence-points model (15/25/30 NTFS etc.) is a
  documented concretization. Verdict: **UNVERIFIED-against-og model**, no contradiction
  demonstrable → OQ-T9. Overlap penalties (−20/−10) and dedup (first-nested-wins →
  post-fix probe-size-only containment) are core-side models.
- **A25** — `M` two-pass windowed scan (64 KiB windows, 1-sector stride).

## State machine

- **S32** — `M` cancel checkpoint + progress per window.
- **S33/S34** — `NA` (module is a single scan pass; resume handled at orchestrator stage
  level).

## Performance

- **X47/X51** — `M` 64 KiB windowed sequential reads; per-probe boot reads + $MFT/FAT
  liveness probes.

## Quality

- **Q65 tests** — THIN (2 areas). Missing: stride > 1, window read failures, probe
  fixup-failure paths, cancel mid-scan, and the whole S-006 family set.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-006 | SEVERE | No ext4/HFS+/Btrfs/ZFS/ReFS lost-partition probes — **FIXED** (ext parity + superset anchors; FATX scoped remainder; M-017 scoring = own finding) |
| OQ-T9 | tracked-open | CalcPartition scoring internals undecoded |

## Post-baseline note

`03ddcd2` (GAP-S6-6, concurrent session) landed during this audit and is INCLUDED in the
audited tree: nested-dedup containment now requires probe-derived sizes. The S-006 probe
set is unchanged by that fix.

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new lost-partition-specific findings in session 2. The session-1 verdicts
stand. Session 2's parallel agents re-verified the architecture parameters
(R54-R58) and safety parameters (F60-F64) cross-cuttingly; no new
lost-partition-specific divergences surfaced.
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/ldm-raid5-and-scoring.md`):**
- **OQ-T9 RESOLVED — DIVERGENT → M-017 (new MAJOR):** og scoring decoded and verified against
  all 187 CalcPartition.log rows: count = files + 50·folders (NTFS-family; FS==8 → f+10·d;
  FAT → (f+d)·50) + per-FS bonus + **+500** on partition-table match; score =
  **ln(count)/ln(maxCount)** (per-scan relative normalization); count==1 keeps numerator 1.0
  (quirk). Core = absolute structural evidence points — ordering diverges on almost any
  multi-candidate fixture.
- **M-017 FIXED (s19):** the og's content-driven scoring landed end to end — weighted
  count (f+50·d NTFS family / f+10·d HFS+ / (f+d)·50 FAT) + the observed per-FS bonus
  (NTFS 60/30, FAT 20 — s31 decode: generator outputs of the og's per-engine
  +0x68 composer ladders; the core's flat values are the verified-against-187-rows
  mapping for its single-probe architecture; OQ-S2M-2 RESOLVED) + the +500 table-match (the old overlap
  penalties' polarity inversion repaired), ln(count)/ln(max) in permille with the og's
  edge cases, the containment-in-known drop (table-matched candidates exempt), the
  stable descending sort, and the :842776-93 iVar16 size-match promotion re-decoded
  first-hand. Candidate.score is permille; structural points tie-break
  (Candidate.structural_points). Resolution in findings/FIXED/M-017.
- **A29-adjacent → M-017:** AdjustPartitionInfoOrder post-hoc rules (0.1/0.3 gap keeps,
  ~16 MiB over-large duplicate re-score + splice-to-front, FAT multi-child keep) — no core
  counterpart.
- **S-006 stays open** (probe families); its scoring half is now decoded — implement M-017
  together with the probe-family wiring so candidate content probes feed the og formula.
- **OQ-T9 residue CLOSED (s31):** the +0x68 setter decoded — per-engine scan-path
  composers (FAT/exFAT two-factor ladders boot+fs ∈ {10,45,50}; NTFS flag-base +
  boot-validation ± + stitching +30/+50); "FAT32 20" = 10+10 minimum tier; see
  findings/FIXED/OQ-S2M-2 + evidence s31-oq-s2m-2.

---

## Session-3 parallel-leg addendum (S3-LOST; core `34b3441`, corpus `03ad46c`)

Complements the OQ-T9 resolution above (same session, parallel leg; full decode:
`evidence/decompiled-quotes/s3-calcpartition-scoring.md` + executed RTTI vtable
dumps in `evidence/executions/s3-calcpartition-vtables/`):

- **OQ-T9 premise corrected (M-017 addendum):** the `*CalcPartition` classes are
  **geometry calculators, not scorers** — executed RTTI COL walk on the og
  binaries: `Fat32CalcPartition::vftable @ 0x180053680`, `ExFatCalcPartition::vftable
  @ 0x18004b9f0`, slot 4 returns `*(u32*)(this+0x18)` = sectors-per-cluster (proved
  by `(cluster−2)*SPC + start` at FATSearchDLL:39076-39083); only FAT/exFAT
  engines instantiate them. The scoring lives in DRWUI (`FUN_14050fda0`) as the
  leg above decoded.
- **AdjustPartitionInfoOrder completed (M-017 addendum):** containment-in-known
  drop → FS-grouped dedup → top-2 tie rules (≤0.1 keep; ≤0.3 keep when runner-up
  is FAT — NTFS-over-FAT priority) → FAT multi-vector drop → **±16 MiB size-match
  promotion to result #2** → surfaced-result **cap 2/4/5/6**; **stable** descending
  merge sort (the core's sort is not stable — parity needs std::stable_sort or an
  explicit tie key).
- **S-019 (SEVERE):** S-006 under-listed the og — the lost-partition prober also
  probes **BitLocker "-FVE-FS-"** partitions (executed string-probe + decompile
  evidence); core has no such probe family. Distinct from S-014 (the win-layer
  unlock-path finding) — S-019 is the discovery-side probe. Implement together
  with S-006's families.
- **N-034 (MINOR):** FAT lost-candidate gate constants — numFATs ∈ {1,2} on open,
  FAT12-only "small FAT" probe (spf ≤ 200 sectors), root_entries×32 % bps == 0 for
  FAT16; complements the N-010b engine-side gates (same og constants, different
  consumer: `src/core/lost/` validate_boot).
- **N-035 (MINOR):** candidate extents are **data-area-anchored** in the og
  (`CSearchedPartitionInfo` carries StartSector + DataAreaStart; EndSec =
  DataAreaStart + totalSector) vs the core's whole-volume extent — the
  containment-drop and ±16 MiB rules consume different extents (up to hundreds of
  MiB delta on large FAT32). Fix M-017 + N-035 together.
- **I-010 row appended** (probe surface post-S-006 = deliberate superset, register
  + pin).


## Session-4 remediation update

**S-019 FIXED** (s4 session): the B1/B3 verdicts now `MATCH` for the BitLocker family —
`probe_bitlocker` implements the og's exact classifier gate (memcmp OEM@+3 "-FVE-FS-",
PartitionRecoveryD_decompiled.c:2778-2785, type 0xB at :2604-2606), dispatched BEFORE the
FAT catch-all (the BPB-less FVE VBR died there pre-fix). boot-sig +15 / fve-shape +25
(grade-only — the og's gate is shape-free, pinned by the noise test); size = marked
device-tail fallback (FVE VBR geometry decode remains the finding's open question).
Downstream: FsFamily::BitLocker + probe_family OEM branch (encrypted content never hits
the ext4-magic probe); engine-dispatch default surfaces the candidate as a locked volume.
3 fail-before/pass-after tests (`LostDiscovery.LostBitlocker*`); suite 365/365 ASan.
See `findings/FIXED/S-019-*.md`.
## Minors-fix session update (2026-10-26)

**N-034 FIXED** — `fat::lost_boot_flavor` implements the og's PartitionRecoveryD
deep-scan ladder (FUN_180004610 + gates FUN_1800048f0/4a40/4b70, all constants
re-verified first-hand): 0x55AA, 512-multiple bps, og cluster-count arithmetic
(total32 preferred, floor root sectors), bracket-ordered gate walk with the
FAT12 spf16 ∈ [1,200] small-FAT probe and the case-SENSITIVE "FAT32   " memcmp.
`probe_fat` now requires a gate pass — the old any-valid-BPB acceptance
generated candidates the og's deep scan never surfaces (noise into M-017's
scoring). The engine-open {1,2} numFATs gate declines deep-accepted triple-FAT
candidates — the og's exact layering, pinned by
`LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates`. The finding's SPC
sub-claim was corrected (u8 domain false positive — see modules/fat.md).

---

## Session-5 addendum (2026-10-26, audited `7cf6754`)

New rows on this module's surface (full detail in
`modules/mom-capabilities.md`):

- **APPLY path (the recovery endgame)** — `S-029` **FIXED (s10)**: the og
  writes the recovered partition into the table (RecoveryModule
  `CreateRecoveryPartitionObject` → PartitionRecovery.dll IOCTL chain
  0x7C100/0x7C010/0x7C058 + GPT GUID regen); the core implemented the
  scope decision as a consent-gated apply module (cr/apply:
  plan/execute/undo on the rawfixer seams, A1-A4 safety standard, MBR v1,
  GPT + boot-chain refused cleanly, PARITY-DELTAS D-68/D-69) surfaced as
  `rrecov apply --disk N --session F --candidate N [--apply]`. 10 tests;
  suite 590/590. The materializer stays read-only BY DESIGN (the
  content-recovery path never needed writes).
- **S-006's FATX scoped remainder** — RESOLVED: no Xbox FATX parser exists
  in the og; `CFatxBlockParser` is the FAT12/16/32 boot parser ("Fatx" =
  FAT-x), and its accept gates diverge from the core's on six specific
  points → `M-045` (the gate deltas now carry the surface).
- **lostpart.dat/partinfo.dat persistence semantics** — **FIXED (DFX20 / M-042, findings/FIXED/)**: the candidates' session records now carry the og's per-record geometry payload (probe-captured bps/SPC/hidden + tail-fallback flag); revalidate_candidate re-probes the persisted start LBA through the current disk at resume (both the listing and the run-start surfaces) and drops records the disk no longer confirms; the whole-disk layout snapshot round-trips as REC_DISK_LAYOUT and surfaces the layout_changed signal. The core's per-hit persistence rides the session checkpoint (forced on discovery), not a module-local .dat — the sink-less residue is registered with the finding. 6 tests; suite 685/685.

- **lostpart.dat/partinfo.dat persistence semantics** — `M-042` (the
  session-module's resume re-validation row) + `N-049` (record richness).
## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| I-020 | INFO | PartitionRecoveryD MOM module: two-generator-set architecture (CGeneratorSetCompleteSearch ×2 / CGeneratorSetRegSearch ×2) + lostpart.dat/partinfo.dat persistence — capability register; semantics tracked in OQ-S6-1 |
| M-017 addendum | — | CalcPartition scorer-class evidence (Fat32CalcPartition ×39 / ExFatCalcPartition ×54 RTTI refs) appended; decode order for the fix leg: OQ-S6-1 first |

---

## Session-16 verdict update (S-029 FIXED, 2026-10-26)

**APPLY path — `S-029` FIXED.** The recovery endgame is live: a dedicated
apply module (`cr/apply/cr_apply.h` / `cr_apply.cpp`) with the rawfixer
transaction pattern extended to the table writer — `plan_apply` (read-only;
clean refusals for live-entry overlap, full table, GPT single-entry scope
(v1), zero/beyond-capacity/32-bit-LBA-range extents), `execute_apply`
(core-carried consent callback, default-deny; S-010 quiesce; A6
table-change race guard; CRUNDO01 snapshot; verified write-back; the corpus
device-control sequence — legacy 0x7C100-delete → sector-0 rewrite →
0x70140 → 0x7C010 set-layout vs modern 0x7C058 0x18-byte record, then the
3x Sleep(1000) refresh retry tail), and `undo_apply` (byte-exact restore +
refresh replay). The cr_ui surface (`cr_ui_apply_lost_partition` /
`cr_ui_apply_undo_last` / `cr_ui_apply_save_undo`, ABI-since 1.3) carries
the consent ABI; the Windows RW transport implements the real IOCTL chain
(kernel32-only imports — the Win7-inclusive gate untouched). Safety
supersets over the og's writer registered as PARITY-DELTAS D-72a..d
(consent, single-entry preservation vs the four-entry wipe, refresh-tail
honesty, race guard). Corpus correction recorded in the finding's
Resolution: GPT_GenerateSignature regenerates the 32-bit disk signature
(MBR +440), not the GPT disk GUID. Remaining on this module: M-017 (the
scoring model) is DFX19's claimed lane; the GPT entry-array writer
(FUN_180039470 protective conversion) stays v2 scope per the fix plan.

## Session-27 verdict update (M-035 FIXED, 2026-10-26)

**M-035 — FIXED:** the engine-side content-consistency validation (the og's
exFAT-only scorer, FUN_180025390) is now `cr::lost` scorer machinery
(cr_consistency.h/.cpp): per walked candidate file — considered (non-directory),
filterable (checkable extension vocabulary = the carve registry's families +
TIFF/JPEG specials, the OQ-FLT-1 shared-vocabulary decision; camera-RAW families
without a core signature stay uncheckable — bounded vocabulary registered),
matched (first registry signature at the file's data start, registry order =
carve dispatch precedence; classifier = substring strstr + the MOV/MP4/JPG
family specials). Reject rule decompile-exact: considered/2 <= filterable &&
ratio < 0.3 && filterable != 0 (STRICT < — the finding text's `<= 0.3`
corrected from the `<= 0.3 && != 0.3` decode); out-of-bounds data start
invalidates outright. Wiring: scan_candidate scores after the engine dispatch;
a reject detaches the candidate's subtree (the new
ResultTree::detach_subtree — nodes/roots/indexes removed, props refs released
once per node, root links severed), zeroes the counts, bumps
ScanStats::lost_content_rejected, logs the verdict (N-048 seam). The candidate
stays listed (per-hit persistence, M-042) — the reject drops its FILES, the
CalcPartition observable. ExFAT-only (OQ-FLT-1); uniform at the candidate seam
(lost fan-out + LDM volume scans; the og's LDM-path wiring corpus-UNVERIFIED,
documented). 15 tests incl. two end-to-end whole-disk scans; fail-before
executed; suite 808/808.


## Session-11D fix lane (2026-10-29 — the S-051 landing's discovery reconciliation)

- **The no-double-scan rule (S-051's acceptance)**: the deep stage's
  lost-partition discovery now drops candidates whose start LBA matches a
  table volume the primary pass ALREADY scanned as a partition-list target
  (the orchestrator records the scanned set; `run_deep` filters before
  materialization). A table entry that was skipped honestly (dead probe —
  no filesystem found) is NOT recorded, so its discovery candidate
  survives: the og's lost partitions are ADDITIONAL targets on damaged
  volumes (architecture/04 §3.4), never duplicates of healthy ones. The
  discovery's own unit contract is unchanged (`LostDiscovery.TableMatchBonus500`
  and the containment-exemption tests still pin the +500 semantics — the
  filter lives in the orchestrator, one site, above `lost::discover`).
---

## Session-12 continuation row (2026-10-29, audited `c7fa527c`, corpus `be0cfe9`)

**M-118 (MAJOR — NEW; renumbered from this lane's M-112 filing at the post-rebase ID reconciliation — the parallel lane's M-112 landed first and keeps the number):** the candidate-SOURCE model — the og's whole-disk assembly
consults THREE sources ("add lost partition from mbr / from filesystem / from
register", DRWUI.log:637-643 + FUN_140588820 at DRWUI_decompiled.c:921820-921890):
the table walk (FUN_140589070 — S-051's planned per-table-volume targets), the
filesystem-probe scan (FUN_140588ba0 — the core's existing discovery), and the
**register walk** (FUN_1405895d0 / PartitionRecoveryD:8930-8965 — the Explorer
`MountPoints2\CPC\Volume` history: HKCU, ≤1000 subkeys, the REG_BINARY "Data" blob's
'#'-delimited 32-hex-char placement field decoded in FUN_18000f5b0 with the
sector-alignment gate). The register channel has no core counterpart (grep-verified)
and uniquely recovers partitions whose on-disk remnants are destroyed — the exact
lost-partition use case. B1 verdict: **DIVERGENCE-MAJOR** pending the channel seam
(the fix plan's CandidateSource feed + the win-layer walk; S-051's fix plan gains
the channel merge). Evidence: `evidence/decompiled-quotes/s12-lostpart-register-channel.md`.

## S42 continuation lane (2026-10-29 — the partition-recovery focus battery; core `b26017a7`)

- **A30/D12/D13 (the surfaced shape) — FIXED (M-137, `e7030bf2`)**: the og names
  unnamed candidates `"Lost Partition %d(%s)"` via the per-scan counter
  (FUN_1403b9330, DRWUI:634817-838 — **resolving M-104's deferred open question 1**),
  relabels leftovers "Unallocated" (:525588-98), presents Disk/Basic/Dynamic/MBR/GPT/FS
  rows (UIUX/08 §2), caps insertion at 10 targets and registers each candidate for a
  "Quick" search (:904229-240), and falls back to 512-byte sectors on a failed
  geometry query (:904185-87). The core: `Candidate @ <LBA>` roots
  (cr_scan.cpp:1933-1939), no cap, always-deep candidates, ERR_BAD_PARAMETER
  refusal on zero sector size, LBA/size/fs-only listing surfaces. **FIXED at
  `e7030bf2`**: the template + counter + label-keep rules landed (the FAT12/16
  boot label captured by the probe), the presentation trio
  (disk_layout/disk_dynamic/disk_table_ok) stamped + persisted per-record
  (PartitionContext v3 tail) + surfaced in rrecov's listing and the
  cr_ui_apply_candidate ABI tail, the ten-target insertion cap
  (lost_candidates_inserted), and the FallbackGeomDevice 512-byte shim
  replacing the refusal. The "Quick" staging maps onto the core's default-on
  single-walk stage model per the C1/I-044 decode (D-36). Worksheet:
  `../evidence/decompiled-quotes/s42-lostpart-surface-batch.md`.


## S54 addendum (2026-10-29 — the fifth continuation lane's focus deep dives)

New active findings on this module: **N-187** (the NTFS candidate acceptance is
weaker in the core: the og requires boot-pair OR a validated 4-record MFT read
with a mirror retry before accepting; the core scores-but-never-rejects).
Verified-MATCH this session: the EBR/GPT partition-math families (re-walked in the
sector-math axis — no new divergence beyond the filed M-123/M-124/N-161 set), the
candidate scoring evidence set, and the lost-scan FAT bracket ladder.

## S55 addendum (2026-10-29 — the sixth continuation lane: the local-database deep dive)

New active findings on this module: **N-203** (the resume round-trip drops the
candidate's data-area extent — the og's 0x448 record replays the full
CSearchedPartitionInfo image through the {start,end,disk} dedup key with the
data-area-anchored EndSec; the core's PartitionContext v3 carries neither
data_start_lba nor data_sector_count, the load restores neither, and
revalidate_candidate discards the probe scratch that re-derives them — resumed
candidates enter the dedup/containment/leader rules with [0,0) windows),
**I-046** (the INFORMATIONAL lifecycle register: lostpart.dat/partinfo.dat are
NOT a cross-session knowledge base — the first search of each process deletes
both files and rebuilds partinfo from the live table [the generation latch];
per-hit mid-scan appends; interrupt header-only progress-markers carrying the
min(cur,end) scan-position LBA; the marker-restore path resumes the search at
sector granularity across process restarts — a capability class the core's
stage-granular session model deliberately does not replicate, per M-042's
registered architecture). New first-hand decode evidence:
`../evidence/s55-lane-sheets/S55-D1-localdb.md` (both record layouts with
offset tables; the shipped samples hex-verified — the lostpart end-LBA
validates arithmetically against the partinfo live entry).

## Closing-fix lane addendum (2026-10-29 — the INFO-first fix lane)

**I-046 FIXED (ACKNOWLEDGED)** — the lifecycle register landed as
PARITY-DELTAS **D-142**: the og's lostpart.dat/partinfo.dat pair is
registered as a per-process-generation cache (W1 delete latch, per-hit
mid-scan appends, W3 header-only interrupt markers with the
scan-position-granular marker-restore resume), and the core's M-042
stage-granular checkpoint architecture is registered as the deliberate
divergence. The resume-granularity delta is now on the record (D-142);
the W3 marker shape + the :20748-20770 restore path are the preserved
decode inputs for any future scan-position-granular resume. The module's
persistence-semantics row above (M-042's landing) is the architecture
D-142 registers against.


## S66 lane (2026-10-29): N-203 landed

**N-203 FIXED** (joint with modules/session.md) — the lost candidate's
data-area extent survives the session round-trip: the v4 PartitionContext
tail carries `data_start_lba`/`data_sector_count`, the resume re-validation
enriches pre-v4 records from the live probe's scratch (the og's record
replays with the extent by construction — the core's equivalent repair).
The finding's dedup-consumer scenario is architecturally unreachable as
literally stated (a deep-rerun resume REPLACES the restored list; the fresh
pass self-dedups on its own anchored extents); the RestoredWindowMatchesFreshDiscovery
pin holds the restored window to the fresh-derived equivalent.

## S70 lane (2026-10-29): N-200 landed

**N-200 FIXED** — the NTFS candidate acceptance now requires positive MFT
evidence exactly as the og's classifier (PartitionRecoveryD FUN_180011150
:9889-9906 → FUN_180011560 :10023-10075): the BPB-valid path's record-0
probe runs primary-first, then the og-exact $MFTMirr re-read; both legs
failing drops the candidate as noise (the og's 0xfffffff1 slot) — a forged
OEM with a plausible BPB no longer surfaces. Mirror-only acceptance carries
the `mft-mirror-probe` evidence label and the mirror-blind strong-probe
bonus (fs_bonus). The A30 scoring row's model is unchanged (15/25/30 + the
content model); what changed is the acceptance DIRECTION (score-and-keep →
require-evidence-or-drop), which the B3 false-positive row now carries as
MATCH. The boot-pair arm (FUN_180011260's field set) stays deferred on the
finding's own open question (its slot writes the same 0xfffffff1 as the
both-fail arm — backup-DBR dedup reading, getter decode pending). RED→GREEN:
ForgedOemNoMftEvidenceRejected / TornPrimaryMftMirrorEvidenceScored /
HealthyCandidateMftEvidenceUnchanged; suite 1425/1425.

## S77 fix lane — M-118 lands (2026-10-29)

**M-118 (MAJOR) — FIXED (findings/FIXED/, tests/unit/test_mountpoints2.cpp, 19
RED→GREEN tests):** the register channel — the og's third candidate source — is
landed end-to-end. The pure blob decoder `lost::decode_mountpoints2_placement`
(FUN_18000f5b0's exact gates: size > 0xe7, the data[0xc4]/data[0xe6] '#'
delimiters, the 0x20-char hex byte offset at +0xc6, bps alignment, /bps, the
sector-0 failure sentinel); the offline-hive walk
`reghive::mountpoints2_placements` (NTUSER.DAT through the regf parser — the
cross-platform surface); the live-HKCU walk `win::mountpoints2_volume_history`
(KEY_READ, the engine-side RegOpenKeyA shape; the DRWUI mirror's 0xf003f
over-privilege not adopted; the A-variant registry stubs added to the
parse-gate's win-stub); the discovery seam — `discover(register_starts)` probes
each remembered placement with the same format chain as the sweep
(FUN_14058a070's parser-chain semantics) and merges accepted placements
upstream of the pass-2 dedup (the real-log collapse: mbr=1 + filesystem=1 +
register=3 → one partition scanned once); the provenance model
(Provenance::Register on the Candidate, persisted as the session
PartitionContext's flags bit 2 — no wire bump); the wiring
(ScanConfig.register_history_lbas, the CLI's --whole-disk arm, the
orchestrator's discover call). The finding's fix-time probe is resolved by the
decode: the "Data" blob carries the START only — the total is derived
downstream from the filesystem the probe reads at the remembered start (the
DRWUI mirror's 0xc2/0xc6 selector picks the volume-LABEL offset, never a size).
**B1 verdict: MATCH** (the three-source assembly is complete: the table channel
= the +500 table_match feed, the filesystem channel = the sweep, the register
channel = this landing). Suite 1541/1541.
## Session-77 addendum (the S79 continuation audit lane; core `3799dee2`, re-verified at `0da3490b`, corpus `be0cfe9`)

- **M-149 (MAJOR, NEW):** the ghost-score math regressed from the M-017 landing — the +500
  table-match bonus is folded into the denominator (`cr_candidates.cpp:1091-1101` at the
  re-verified pin: `count += 500` precedes the `max_count` accumulation) where the og's max
  loop has no bonus walk (FUN_14050fda0 :842186-842240, first-hand); and `score_ratio`'s
  `count > 1 ? log : 1.0` maps count==0 to 1.0 where the og's own CalcPartition.log prints
  `0.000000` for the `files:0 folders:0` shape (row 6 of the last round; the 186-row numeric
  fit in `evidence/executions/s79-calcpartition-score-fit/`). Git archaeology: the corrected
  two-pass implementation (48a6c099) was dropped by the 9c36efba convergence merge per
  FIRST-LANDING-WINS — "the two decode divergences for the next pass" never got their pass.
  The standing "0.1/0.3 gap keeps … no core counterpart" note (M-017's known remainder)
  stays a separate open gap — not re-filed. N-200's landing (the mirror-retry probe gates)
  does not touch the scoring seam.