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
  geometry, GPT header shape); **S-006** (no ext/HFS+/Btrfs/ZFS/ReFS probes — og has
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
| S-006 | SEVERE | No ext4/HFS+/Btrfs/ZFS/ReFS lost-partition probes (og has Linux/FATX parsers) |
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
