# modules/lost-partition.md — lost-partition module audit verdicts

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

- **A30 candidate scoring** — **FIXED (DFX20 / M-017, findings/FIXED/)**: the og's
  content-driven dSuggested + AdjustPartitionInfoOrder are ported (cr_scoring.{h,cpp});
  the fan-out counts recovered content per candidate and ranks by the og's ln-normalized
  per-FS weights (+ per-FS bonus defaults; + the +500 table match; denominator = the
  PRE-+500 pass-1 max - a decode correction the fix pinned by fitting the QA-log rows),
  then curates (containment proper-subset drop per the round-1 log arbitration, 0.1/0.3
  FAT tie rules, size-match promotion to result #2 with the follower-score copy -
  re-decoded first-hand at LAB_140510a0e; stable descending sort). The UI curation cap
  is deliberately NOT ported (the og moves overflow to a secondary list; the core would
  lose data). score = the og-shaped permille; REC_PARTITION_CONTEXT v3 round-trips the
  counts. Executed fail-before (ranking disabled -> the orchestrator test RED); 7 tests;
  suite 706/706. The structural evidence points remain on the evidence list (the
  tie-break input); the old UNVERIFIED verdict is superseded - OQ-T9 was already
  resolved by the session-3 legs' decode, and this session's port closes the loop.

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
- **A29-adjacent → M-017:** AdjustPartitionInfoOrder post-hoc rules (0.1/0.3 gap keeps,
  ~16 MiB over-large duplicate re-score + splice-to-front, FAT multi-child keep) — no core
  counterpart.
- **S-006 stays open** (probe families); its scoring half is now decoded — implement M-017
  together with the probe-family wiring so candidate content probes feed the og formula.
- **OQ-T9 residue:** bonus (+0x68) setter in the SearchDLLs unpinned → OQ-S2M-2.

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