# diff-to-fix/ — parity audit roadmap (core_recovery vs easeus-drw-research)

**This directory is the deliverable of a read-only parity audit.** No source
code was modified to produce it. It is the roadmap future sessions execute
against.

| | |
|---|---|
| **Audited core commit** | **session 5b (this leg): `7cf6754`-lineage rebased onto `c75f646`** (the S-019 fix leg verified first-hand in-source this session; the parallel fix legs' landings visible at rebase time: S-012 `c69a790`+`5f1a212`, S-022 `cdd340b`, S-020/M-022/N-037 `9af8014`, S-026 `0c1f58e`, S-021+S-003 `c75f646`, M-023 `d80a52e`, the N-batches and MSVC/bench legs). Session 4: `204137b` (lineage `63a9167` rebased onto `ed00020` — S-013 `d2d9b03` + S-014 `ed00020` included; suite 306/306 + C-ABI check green at that tree). Session 3 (all legs, close at `43149ff` + continuation): baselines `34b3441`→`e6d5b01`→`abe8e36`→`1f08c20`; mid-session landings verified by the audit legs: S-010 `3e34ed2`, M-009 `7fd00e0`, A-002 `7f2c83e`(+`abe26b8`), A-004+S-016 `c33236f`, S-017 `1cc08c3`, S-018 INVALID `2adfa29`, S-006 `abe8e36`, S-011, S-015 `021e2e6`, N-021 `6fc8c0e`, S-013 `d2d9b03`, S-014 `ed00020`. Sessions 1-2 baselines: `67193b6`/`5d6da5f`/`8574039`/`5f638a3`. Corpus `03ad46c` throughout. |
| **Corpus (ground truth) commit** | `03ad46c` (`easeus-drw-research` main, unmoved) |

**Session-3 storage/census parallel leg (integrated 2026-10-26, post-S4)** added
19 renumbered findings (S-024..S-028, M-031..M-039, N-045..N-047, I-017/I-018)
after reconciling its 42 local filings against the lead legs (23 superseded —
cross-referenced via addenda on S-020/M-017/M-022/M-023/M-020/M-026/N-030/N-036).
New surfaces: health grading (S-024), ext4 journal-reparenting/meta_bg/backup-SB
(S-025..S-027), Btrfs xattrs (S-028), the storage/slice layer (M-032..M-034 +
new `modules/storage.md`), engine-side FileFilter validation (M-035), NVMe
coverage (M-036), ZFS SA registry (M-038), occupancy bitmap (M-039). New tracked
probes: OQ-RS (ReFS) + OQ-Z-E-FLT (incl. the ACUTE-gated ZFS dsl-offset
question). Evidence: s3-virtual-volume-device / s3-filefilter-health /
s3-rawsigns-consumer-decode. Post-integration active count: 104 (inventory).

**Session 5b (2026-10-26, the MOM/product-capabilities continuation leg)** audited the
never-dispositioned og capability surfaces — `modules/mom-capabilities.md` is the new
module row (the PartitionRecovery `.mo` family + product DLLs: apply/write path,
boot-chain repair, disk-image scanning, check phase, volume-analysers, lostpart.dat
persistence, logging seam, EPM tool ops) — with findings **S-029, S-030, M-040..M-045,
N-048..N-050, I-019, I-020** (13 new; renumbered past the storage/census leg's
S-024..S-028/M-031..M-039/N-045..N-047/I-017/I-018 — no content overlap, pure ID
coordination); resolved OQ-S4-1 (the 169-entry grading extension table dumped from
`.data` → S-023's fix-plan step 3 unblocked) and OQ-S4-2 (the RAW naming hook =
single-site "filter_generalraw" category display-name mapping → M-029 §Gap (b)
corrected); dissolved S-006's FATX scoped remainder (no Xbox parser exists —
CFatxBlockParser is the FAT12/16/32 boot parser); verified S-019's fix in-source;
filed corpus errata rows 37-42; one new tracked-open OQ-S5-1 (CheckVersion's
consumer). Evidence: s5-mom-capabilities / s5-image-fscheck-persistence /
s5-oq-s4-1-extension-table-dump / s5-oq-s4-2-raw-naming-hook.

**Session 4 (2026-10-26) added the first-time module rows** — `modules/preview.md`,
`modules/query.md`, `modules/transport.md` (no prior session had audited these
surfaces) — with findings S-023, M-028..M-030, N-040..N-044, I-015/I-016; resolved
OQ-S2M-1 (grading locus → S-023) and M-022's OQ-1/OQ-3; filed corpus errata rows
29-36. Navigation: SUMMARY.md (deltas + counts + top risks) → CROSS-REF-MATRIX.md
(module × parameter verdicts) → modules/*.md (per-module detail) → findings/
(template-complete, evidence-cited) → tracked-open/ (precise open questions).
