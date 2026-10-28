# diff-to-fix/ — parity audit roadmap (core_recovery vs easeus-drw-research)

**Merged product repository (audit + dev-sign target):**
**https://github.com/heicfast/core_recovery** — confirmed by inspection (core
engine `src/core`+`include/cr`, win layer `src/win`, driver-consuming transport,
CI, and this audit trail in one tree; the driver itself ships as a prebuilt
`rda.sys` release asset from `heicfast/rda-driver` per `docs/PREBUILT-ARTIFACTS.md`).
Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research.
Driver repo (in the audit scope when a session's grant covers both repos —
session 9's did): https://github.com/heicfast/rda-driver.

**Session 9 (2026-10-28, SA-9 lane — the wave-2 continuation executed; RECONCILED at push time):**
audited **core product code `35adcb9`** (the last code commit), **rda-driver
`323e75a` (v0.2.4)**, **corpus `be0cfe9`**. Two-repo scope: the session-8 T2
lanes (V/R/G/C) executed **plus the D-lane** (the driver-repo handoffs
D3/D4 + FileVersion). **Push-time reconciliation:** the parallel session-8
lanes had concurrently landed a 75-active-findings board (their S-040..S-047,
M-071..M-102, N-090..N-126 decompositions + the M-081 CRW fix `9e5dedc` and
the school-1 batch-2 code landing `b92ebef`) — this lane's 8 filings were
7-convergent/1-unique: **merged as addenda** into their S-042 (WMF — the
std-gates observation + canonical-layout evidence), FIXED M-081 (the CRW
third-lane corroboration), S-041 (the batch-2 consolidated decode table),
M-086 (the CI tag==resource gate), M-087 (the **Start-posture parameter** —
demand-start + active UpperFilters cannot attach at stack-build; the og's
filter is BOOT_START), N-107/N-093 (converged); **kept as N-127** (the
rda-driver doc-vs-code truthfulness batch: the claimed admin gate with no
code, the stale sector prose, the S12 contradiction, the unreachable
default, the 57th check). The lane's **unique MATCH verdicts**: the
never-audited EPMNtDrv v2 internals + trio naming/PID handshake (SURFACE-
INVENTORY rows flipped — the identity IOCTLs, the PID-token transform, the
naming scheme, DO_DIRECT_IO, ×512 all byte-exact) and the V-lane landing
verifications (W1a/W1b/W1c, origin-LCN, thumbnail, installer steps 2-3).
Erratum **E68** (part1:85's refuted "+0x64 secondary ID" label per the
SCAN-2f queue); evidence under `evidence/decompiled-quotes/s9-*` +
`evidence/executions/s9-*`. Board after the reconciliation: the session-8
board + N-127 (see SUMMARY for the current counts).

**This directory is the deliverable of a read-only parity audit.** No source
code was modified to produce it. It is the roadmap future sessions execute
against.

| | |
|---|---|
| **Audited core commit (session 8 — third lane: the DRIVER lane, 2026-10-28)** | **product code `35adcb9`** (rebased over the parallel session-8 lanes through `7748254`; all this lane's commits are trail-only) · **driver `323e75a`** (v0.2.4 — this lane's primary audit target; no other session-8 lane took the driver D-handoffs) · **corpus `be0cfe9`**. Filed: **M-086** (FileVersion 1.0.0.0 pin defeats -Layout Versioned — upgrades hit exit 2), **M-087** (regdrv-rda.reg hex(7) invalid — the WinPE UpperFilters never attaches; D4 owned), **M-088** (FloorVersion two-site/two-floor split + the "ONE pin source" false claim + the stale CI-shape doc cluster), **M-089** (rdabench phantom-zero probe list — the extended IOCTL handlers get zero fuzz coverage), **N-107** (14 stale 15/18-command sites vs 19), **N-108** (docs/PARAMETERS.md §3 stale constants + the SCAN-ENGINE-DELTA false "fixed" claim corrected). Merges: this lane's duplicate S-040/S-041 filings converged into the continuation lane's S-042/S-041 (WMF: two lanes, two decompilers, one conclusion; the CHM/FH11 asm decode donated; errata E65/E66). Repairs: A-007's dangling evidence cites. **Board after all session-8 legs: 47 active (6 SEVERE · 19 MAJOR · 19 MINOR · 3 INFO) + 249 FIXED + OQ-S4-3/OQ-S8-1 tracked-open.** |
| **Audited core commit (session 8 — the second-session deep audit, 2026-10-28)** | **product code `35adcb9`** (all session-8 commits are trail-only: diff-to-fix/ + worklog; zero product files touched). **Rebase note:** the product tree moved under the audit to `dd225da` (the parallel CI-REL lane's session-list empty-dir fix) — every load-bearing absence claim was **re-verified on the final rebased tree before push** (the junk-gate census, the volume_label fill-site census, the R-row absences, the batch-2 absences all re-run; only N-090's cr_ui.cpp line cites shifted, corrected in the finding). **Corpus `be0cfe9`** (one commit past `03ad46c`; absorption verified). Board at open: 0 active / 249 FIXED / 1 INFO / 1 tracked-open. Session scope (T1-determined): (a) verify the post-dev-sign landings' corpus claims read-only (W1a/W1b/W1c, the SCAN carrier, S-007 batch-1, thumbnail, installer steps 2-3, CI rebuild); (b) file the S-007 batch-2 remainder; (c) sweep the never-audited og-codebase assets; (d) re-verify the two NEVER-CITED driver docs + the MOM decode candidates; (e) spot-check the FIXED board's truth. **Board at close: 25 active (2 SEVERE · 12 MAJOR · 9 MINOR · I-027) + 249 FIXED + OQ-S4-3/OQ-S8-1 tracked-open.** See SUMMARY's session-8 delta. |
| **Audited core commit (dev-sign session, 2026-10-27/28 — CLOSED at `832b390`)** | **product code `3bda8e2`** (the session changed ZERO product files — the read-only sweep proves every changed path is under `diff-to-fix/` + `worklog*`; the dev-sign validation run 40549660074 executed the workflow content of `3bda8e2`). Corpus `03ad46c` (unmoved). Board at close: **18 active** (A-010 · S-036..S-039 · M-060..M-066 · N-084..N-088 · I-026) + 223 FIXED + 4 tracked-open. See SUMMARY's dev-sign session deltas + SURFACE-INVENTORY. The SA lane (this session) validated the dev-sign/install/uninstall path on windows-2022 and filed the installer-posture findings **S-036, M-060..M-063, N-084, N-085, I-026** (see `worklog/DEV-SIGN.md` + `evidence/executions/devsign-run-40549660074/`); the parallel S7 lane audits the driver-only-era parity delta. |
| **Audited core commit** | **session 5b (this leg): `7cf6754`-lineage rebased onto `c75f646`** (the S-019 fix leg verified first-hand in-source this session; the parallel fix legs' landings visible at rebase time: S-012 `c69a790`+`5f1a212`, S-022 `cdd340b`, S-020/M-022/N-037 `9af8014`, S-026 `0c1f58e`, S-021+S-003 `c75f646`, M-023 `d80a52e`, the N-batches and MSVC/bench legs). Session 4: `204137b` (lineage `63a9167` rebased onto `ed00020` — S-013 `d2d9b03` + S-014 `ed00020` included; suite 306/306 + C-ABI check green at that tree). Session 3 (all legs, close at `43149ff` + continuation): baselines `34b3441`→`e6d5b01`→`abe8e36`→`1f08c20`; mid-session landings verified by the audit legs: S-010 `3e34ed2`, M-009 `7fd00e0`, A-002 `7f2c83e`(+`abe26b8`), A-004+S-016 `c33236f`, S-017 `1cc08c3`, S-018 INVALID `2adfa29`, S-006 `abe8e36`, S-011, S-015 `021e2e6`, N-021 `6fc8c0e`, S-013 `d2d9b03`, S-014 `ed00020`. Sessions 1-2 baselines: `67193b6`/`5d6da5f`/`8574039`/`5f638a3`. Corpus `03ad46c` throughout. |

| **Audited core commit** | **session 5: `7cf6754`** (the S-019 landing, one commit past session-4 close `d870dd3`; suite 366/366 executed green at this tree — `evidence/executions/s5-baseline-suite-green.md`). **session 4: `204137b`** (this leg's wave-1 commit; lineage `63a9167` rebased onto `ed00020` — the fix legs' S-013 `d2d9b03` + S-014 `ed00020` included; suite 306/306 + C-ABI check executed green at this tree). Session 3 (all legs, close at `43149ff` + continuation): baselines `34b3441`→`e6d5b01`→`abe8e36`→`1f08c20`; mid-session landings verified by the audit legs: S-010 `3e34ed2`, M-009 `7fd00e0`, A-002 `7f2c83e`(+`abe26b8`), A-004+S-016 `c33236f`, S-017 `1cc08c3`, S-018 INVALID `2adfa29`, S-006 `abe8e36`, S-011, S-015 `021e2e6`, N-021 `6fc8c0e`, S-013 `d2d9b03`, S-014 `ed00020`. Sessions 1-2 baselines: `67193b6`/`5d6da5f`/`8574039`/`5f638a3`. Corpus `03ad46c` throughout. |
| **Corpus (ground truth) commit** | `be0cfe9` (both session-8 legs; CORRECTIONS #9 absorbed). **Session-8 wave-2 — the continuation lane (2026-10-28, audited `ff879f9` lineage, rebased over the parallel deep-audit lane)**: 16 new findings + 2 merges — **S-042** (WMF-placeable grammar contradicts the og — disjoint accept-sets), **S-043** (thumbnail EXIF orientation), **S-044** (auto-save gates missing), **S-045** (thumbnail format coverage video/RAW/HEIC/PDF), **M-083/M-084/M-085** (grade promotion; ACCDB signature; TIFF LZW), **N-099..N-106** (citation truth, +0x60 persistence, inventory arithmetic, M-024 propagation, API rows, doc truth, Folder suffix, GetDriverInfo), **I-028** (thumbnail superset register); the batch-2 decode and the thumbnail-budget evidence MERGED into the parallel lane's **S-041/M-080** as addenda; errata **E55-E64**. **Combined board after both session-8 legs: 41 active (6 SEVERE · 15 MAJOR · 17 MINOR · I-026/I-027/I-028) + 249 FIXED + OQ-S4-3/OQ-S8-1 tracked-open.** Earlier sessions: `03ad46c` throughout |

**Dev-sign validation session (2026-10-27/28, SA lane, audited `dd82db9`/workflow content `3bda8e2`)** —
Phase 1 of the merged-product validation: the documented dev-sign path
(`.github/actions/install-driver/action.yml` + `driver-install.yml`) was
execution-verified on a real GitHub windows-2022 runner (run 40549660074:
sign → install → RUNNING → 5-facet ready → control round-trip → uninstall),
`worklog/DEV-SIGN.md` records the reproduction, and the installer-posture
gaps became findings **S-036** (the release `install.ps1` crashes at its
elevation guard; only the undocumented composite fallback installs),
**M-060** (the fallback configures the service differently — invented group
name, no Parameters), **M-061** (release `uninstall.ps1` never exercised +
silent-failure semantics), **M-062** (install.ps1 start-failure exits 0),
**M-063** (single-environment validation; HVCI/SecureBoot constraint + OS
matrix undocumented), **N-084** (no timestamp server in the sign step),
**N-085** (elevated-console installer posture vs the og's silent
setupdrvx64.exe), **I-026** (unsigned-by-design posture — deliberate,
documented, now execution-proven). Evidence:
`evidence/executions/devsign-run-40549660074/`. `tools/dev-sign/` was NOT
created — the repo already documents a dev-sign path (the mandate's
condition for creating it).

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

**Session 5 (2026-10-26, audited `7cf6754`) — the citation-gap wave:** a systematic
inventory (corpus paths cited by findings vs the artifact tree; core file basenames vs
citations) drove a full read of every never-cited surface: the 6 RAWSearchDLL family
artifacts + per-engine-function-map, `SaveScan/rsf-record4` + `ifileproperties-part3`,
8 MOM censuses, 9 UIUX artifacts, the WinPE pair, config-files, and the core's
never-cited files (elevate/destio/snapshot/cached/decomp/err/diskmap). 24 new findings —
**A-005 (ACUTE: rawfixer 4Kn stack-buffer overflow)**, **S-031** (carved files never
marked deleted — the deleted view excludes the whole carve set), **S-032** (ZIP/RAR
inner-entry inventories), M-046..M-051, N-051..N-064, I-021/I-022; **OQ-S4-1 RESOLVED**
(the 169-entry grading table dumped byte-exact — S-023's fix is unblocked); OQ-S4-2
RESOLVED-with-residual; corpus errata 37-40. Navigation: CROSS-REF-MATRIX session-5
header + detail rows; per-module Session-5 sections in carve/session/rawfixer/health/
ldm/orchestration/writer/visualization/transport/query.
**Session 6 (2026-10-26, deep continuation)** executed the session-5 citation-gap
lanes (defined at `2001f7c`, never run): 23 never-cited corpus artifacts + 8
never-cited core files first-cited; 17 new findings (M-040..M-042, N-048..N-059,
I-019/I-020) + 9 addenda (S-007 family layer / bare-MPEG gap, S-009 free-overlay
residual, S-024 doc-truth, M-017/M-030/M-029/N-033/N-045) + 3 new tracked-open
OQs (S6-1 generator-set semantics, S6-2 Btrfs RC4 target, S6-3 CRAW reader).
Session-6 audited tree: **`022c81d`-lineage (landing `bf7df39`)**; corpus `03ad46c`
(unmoved). Verification discipline this session: every load-bearing citation
checked first-hand before filing (three stale agent claims filtered). Post-session
count: **115 active / 34 FIXED** (0 ACUTE · 11 SEVERE · 37 MAJOR · 49 MINOR ·
18 INFO), 14 tracked-open.


**Session-8 lead-lane wave (2026-10-28, audited core `ff879f9`, corpus `be0cfe9` — the FINAL
reconciliation over three concurrent lanes)** — 28 findings kept (S-047, M-090..M-102,
N-111..N-126, I-029/I-030), 13 duplicates merged via 5 addenda, 7 school-1 duplicate
filings dropped after `b92ebef` landed them, erratum E67. **Headline additions the parallel
waves did not have**: M-090 (the DMG koly-trailer walk/cap divergence — carved DMGs
truncated before the mandatory UDIF footer), M-091 (the MDB page-chain size-walk decode),
N-116 (the session-list time-sort divergence), the school-2/3 per-format grammar sheets,
and the fixture-regeneration requirement (tests derive fixtures from spec, never from the
implementation — the class behind S-042/M-081). Board after the merge: **75 active /
249 FIXED / 1 tracked-open**. Evidence: `evidence/sa8-context-sheets.md`.
