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
