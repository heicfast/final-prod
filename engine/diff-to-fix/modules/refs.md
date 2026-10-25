# modules/refs.md — ReFS engine audit verdicts

Reference: ReFSSearchDLL + RefsFileSystemAnalyser.mo per corpus artifacts
`SearchDLL/refs-engine--deep-dive.md`, `refs-btrfs-zfs-engine-algorithms-v2.md` §1,
`remaining-engines--ext4-hfs-refs-btrfs-zfs.md`, `PartitionRecovery/mo-module-map.md`.

Core (audited commit `67193b6` **including concurrent GAP-S6-3 fix `7797bd6`**):
`src/core/refs/cr_refs_engine.cpp` (938 LOC) + `cr_refs_detect.cpp` +
`include/cr/refs/` + `tests/unit/test_refs_volume.cpp` (11 areas) +
`test_refs_ldm.cpp` (ReFS-on-LDM smoke).

## Behavioral

- **B1 detection** — `M` page signatures ("SUPBMSB+ReFSFSRS" / "CHKPSUPBMSB+ReFSFSRS" /
  "MSB+ReFSFSRS"), page min size 0x20, ref parse at page+8/size−0x20, boot-sector
  identification page ("ReFS" @+3, "FSRS" u32@0x10, bps@0x20, spc@0x24, major@0x28,
  minor@0x29) — all verified against decompile; **N-007** (full-memcmp vs og
  first-DWORD); **I-004** (version-gate superset 1.x–3.x, documented); checkpoint-body
  minor gate + dynamic body offset fixed by concurrent session (GAP-S6-3, verified in
  this audit's read of `:359–364`).
- **B2 recovery correctness** — `M` checkpoint newer-clock selection, REFS-1
  next-chain, REFS-2 minor≥14 layout, allocator polarity calibration, runlist
  resolution, resident data (CRefsResidentDataRunList parity incl. the .rsf
  re-materialization contract — see session module).
- **B3** — `M` page-shape validation.
- **B4 deleted classification** — `M`-note: core's ReFS deleted grading has no
  Excellent tier (agent D observation); og's ReFS deleted-grading boundaries not decoded
  in corpus — no contradiction demonstrable; revisit if corpus decode lands.
- **B5 orphan** — `M` system-dir identity resolution; unnamed $DATA-only records noted
  (agent D) — naming fallback parity unknown, minor.
- **B7 ADS** — **known gap** (project's own CORE-GAP-LIST: ReFS ADS/containers/security
  not implemented; the og's .mo analyser family covers them — CRefsAppDataRunItem,
  mo-module-map). Filed as the project's GAP register row; audit verdict:
  DIVERGENCE-MAJOR tracked via the project's own register (re-verified open at audit
  baseline — see SUMMARY known-gaps cross-check).
- **B9 timestamps** — `M` raw 100ns-1601 stored.
- **B10 Unicode** — `M` UTF-16 names (no byte-lift issue).

## Constants and formats

- **C19/C20** — `M` (page signatures, ref fields flags+0x2C/size+0x30/next+0x38, page
  types 1/2/3).
- **C21** — `M` FsKindCode::Refs = 10 = og factory code 10.

## Algorithms

- **A25** — `M` CHKP→SUPB→MSB chain via the base-data manager model.
- **A26** — `M` system-directory B+ tree walks; 32-byte object IDs.

## State machine

- **S32** — `M` cancel polls per page cluster.
- **S35** — `M` damaged-page tolerance + checkpoint fallback ladder.

## Performance

- **X51** — `M` per-cluster page assembly, 256-page cache.

## Quality

- **Q65 tests** — strong (11 areas incl. REFS-1/2 chains + checkpoint layouts + the
  GAP-S6-3 regression tests added by the concurrent session).

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-007 | MINOR | Full-signature memcmp vs og first-DWORD (corrupt-page tolerance) |
| I-004 | INFO | Version-gate superset 1.x–3.x (documented; og gates 3.4+/1.x) |
| (project GAP) | MAJOR | ReFS ADS/containers/security not implemented (og .mo family covers) — re-verified open |

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new refs-specific findings in session 2. The session-1 verdicts
stand. Session 2's parallel agents re-verified the architecture parameters
(R54-R58) and safety parameters (F60-F64) cross-cuttingly; no new
refs-specific divergences surfaced.
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/btrfs-zfs-refs-session2.md`):**
- **N-026 filed (new):** checkpoint body offset — og dynamic (`*(uint*)(page+4)`) vs core
  hardcoded 0x50/0x56/0xA8; boot gates two-sided (og bps {256..4096} + cluster ∈ {4K,64K}
  ONLY vs core bps {512,4096} + pow2 cluster); second-copy location og START+MID vs core
  START+TAIL; volume-open read size (informational).
- **N-007/I-004 re-confirmed verbatim:** first-DWORD page dispatch (`:11701`); 3.4+ version
  gate; SUPB/CHKP constants byte-exact.
- **GAP-S6-3 (landed fix) verified og-parity:** body-minor gate source (`minor >= 0xE →
  0x58/0xA8` family) matches `:13744-56` — the fix's remaining edge is the dynamic body base
  (N-013a).
- **B4 og-boundary:** no deleted-grading classes exist in ReFSSearchDLL — core's
  medium-allocator deleted grading remains a core-only feature (registered).

## Session-3 deep pass (2026-10-25, core `34b3441`+rebase — the file-pipeline decode)

**New findings (evidence: `../evidence/decompiled-quotes/s3-refs-file-pipeline.md`):**
- **I-012 (INFORMATIONAL — mandate item RESOLVED):** GAP-ADS settled — the og does NOT
  recover ReFS named streams either (no stream RTTI; one properties block + one runlist +
  one emission per file row in FUN_18000ba90; .rsf RefsFileProperties has a single
  name/size/runlist — no stream field). Core's default-stream-only = MATCH. The prior
  module row "(project GAP) ReFS ADS ... — re-verified open" is superseded: it is a
  shared limitation, not a parity debt.
- **M-022 (MAJOR):** the file-value walk diverges — og reads the row type at key+0
  (`FUN_18000b820` = `*(u16*)key`) with id space {0xf8→flag3, 0x108→flag1, 0x178→old-layout
  data-run handler, modern: key-size 0x28 + type >= 0x29/0xb1, 100+"$I"→recycle special},
  iterates ALL value rows, and switches old/modern grammar on `param_1[10] >= 8` = the
  system-directory list count built from the DBR offset table. Core reads type at key+8
  (`attr_type_of`), matches only 0x80, breaks at the first data row, has no grammar gate
  and no $I handler. The type-space/offset reconciliation needs one real-volume dump
  (fix plan step 1).
- Convergence checks: directory-level row markers (0x10 / 0x30+subtype@+2) = the core's
  packed u32 markers ✓; root dir 0x600 ✓ (quick-path bootstrap `plVar4[10] = 0x600`).
- Not decoded to verdict (M-022 OQ-3): the quick-searcher's dot-extension prefilter
  (`memchr` '.' loop) — quick-search surface only.

---

## Session-3 parallel-leg addendum (S3-REFS; core `34b3441`, corpus `03ad46c`)

Complements M-022/I-012 above (same session, parallel leg; full inventory:
`evidence/decompiled-quotes/s3-refs-engine-inventory.md`, ~75% of ReFSSearchDLL's
20,270 lines read, all capability-relevant functions in full):

- **S-020 (SEVERE):** the og **materializes resident/inline content** —
  `FUN_18000e490` (:10410-10462) and `FUN_18000ea30` (:10723-10768) construct
  `CRefsResidentDataRunList` objects (one-entry runlist {offset, length}, cluster
  -granularity gate, val_len stored) — answering M-022's fix-plan step 4 about the
  special-handler bodies. The core never populates `resident_data` (grep-proven)
  and its `val_len ≥ 0x28` gate drops resident-sized rows → inline files recover
  as metadata-only. Fix requires M-022's dual-grammar/all-rows walk first.
- **M-026 (MAJOR):** deleted-recovery model — og sweeps **stale "MSB+" metadata
  pages** unreachable from the live checkpoint chain, force-marks their records
  deleted, emits through the normal pipeline (FUN_180005330 :2819-3054), and marks
  live runs in a used-space map; core harvests row-flag 0x4 + ID2 name-evidence
  rows + allocator-bitmap grading. Copy-on-write ReFS keeps deleted objects as
  stale page versions — the og's model matches the medium; escalation candidate
  if real volumes turn out not to set flag 0x4 (UNVERIFIED residual).
- **M-027 (MAJOR):** object-identity keep-one (name/priority compare, loser
  destroyed) + Recycle-Bin routing ($R*/S-*/SVI collection flushed after the main
  map, $I resident read) — core emits one result per ROW_FILE with no dedup
  (hard-linked objects recovered N times). Cross-ref M-022's $I call-site decode.
- **N-036 (MINOR):** datarun entry grammar — og requires flag **0x40** (block
  reference) and translates non-physical runs (flag 0x20 clear) through a
  per-volume container map (`base = map[((block/cluster)>>1) − 2]`); core requires
  flag **0x10**, strips a top-nibble tag, and its container map is **never
  populated** (grep-proven) — every run is treated as physical.
- **N-037 (MINOR):** runlist granularity validation — og computes the extents'
  power-of-two granularity band and discards the whole runlist on cluster-size
  mismatch (damaged-record FP guard); core accepts any in-bounds parse.
- **I-013 (INFO):** host-OS runtime gate — og's DLL refuses hosts < Win8 (6.2);
  core is host-agnostic (deliberate portability; parity benches on Linux are not
  og-divergent). On-disk gates remain I-004 + N-026/N-013b.
- **Convergence with the leg above:** my agent's row-grammar findings
  (directory rows 0x10 / 0x30+subtype; `0x00010030` packed markers ≡ og key+0
  type/key+2 subtype) independently match the M-022 convergence checks ✓; the
  named-streams parity (one name/size/runlist per row in .rsf, no stream field)
  matches I-012 ✓.
- **Newly-MATCH (matrix rows):** C21 (fs code 10 at a fresh site,
  FUN_180010390:12125), A26 (row grammar identity), X51 (I/O read pattern),
  B10 (name handling). Report-only concern resolved negative: core `scan_refs`
  is a full recovery engine (no detect-vs-recover mismatch finding).
- **UNVERIFIED residuals (7):** complete-open offset +0x170 semantics; container
  -map value composition (→ ghidra-mo RefsFileSystemAnalyser); row-flag-0x4
  reality on real volumes (decides M-026 escalation); run-flag polarity (decides
  N-036 fix-vs-INFO); FUN_1800089f0 consumer; keep-one tie-break fields; ReFS
  perf parameters X47–X50/X52 (needs a real ReFS volume — corpus has none).
