# SA-8 evidence — the session-8 lead-lane context-sheet consolidation (2026-10-28)

**Audited core commit:** `ff879f94a3edd00f217612e45d77041f3e1100de` (ff879f9).
**Corpus commit:** `be0cfe95ecf22ac67e0f3a823dd003163b196d38` (be0cfe9 — one commit past the
trail's standing 03ad46c; the added commit (CORRECTIONS.md #9) touches only the corpus's own
corrections file, so every previously line-cited artifact is unmoved).
**Reconciliation (three push windows):** a parallel deep-audit lane (S-040..S-045,
M-071..M-085, N-090..N-106, I-027/I-028), a fix lane (the CRW/MRW resolution `9e5dedc`,
S-041 school-1 `b92ebef`, the SCAN-4 grammar table), and a third driver lane (M-086..M-089,
N-107/N-108) all landed concurrently. Merge: 28 lead-lane findings kept in the final
numbering (S-047, M-090..M-102, N-111..N-126, I-029/I-030); 13 duplicates merged via
addenda (S-041/S-042/N-091/N-100/N-101); 7 school-1 duplicate filings dropped (the formats
landed via S-041's rollup — CHM/ONE/MNY/EMF/SWF/3DS landed, FCPEvt closed-as-covered);
errata converged (one distinct row kept: E67, the DMG bound/walk).

**Method:** five parallel read legs, each producing a structured context sheet; the lead
agent then re-read every load-bearing site first-hand before any finding was filed.
Protocol: verification tier 2 (decompile-direct) + tier 4 (binary probes) where constants
were ambiguous; no executed comparisons were possible in this read-only container (no
Windows scan stack) — recorded honestly per finding.

---

## Leg 1 — V-API: the W1a/W1b/W1c landing verification

Read the corpus ground truth (rsf-format--scan-session.md in full; DRWUI_decompiled.c
first-hand at the enumeration/list/restore sites; the three shipped .rsf samples — record 2
of `auto_Local Disk (I_)10_14_23_13_21.rsf` decompressed and byte-matched by the leg)
against the landed core surfaces.

**Verified MATCH (the load-bearing halves):** W1a light-records-only (og :432791-432933 —
records 3/4 never opened in the list path; the remote erratum E64's record-1 correction
refines the phrasing, convergent) + the record-2 fields incl. the embedded save time; W1b
per-node progress + per-child IsCancelled + the partial-tree discard (og
:186960-186971 / :187082-187106; the remote E62 refines the autosave trigger — convergent);
W1c the completion-window trigger + the byte-exact naming (the no-separator layout
verified against the shipped samples).

**Filed (final numbering):** **N-116** (the og sorts rows by the embedded scan time — the
FILETIME comparator at :432112-432142 + the introsort at :432549 — core byte-sorts by
path; a distinct axis from the parallel N-090's scope/row-shape finding); the
registration-class deltas (the load-progress scale, the opt-in autosave posture, the
sanitizer superset) carried by the parallel N-091 + the lead addendum; **N-117/N-118**
(the plan's un-landed W1b shape + the ABI-CONTRACT line-cite drift; the stale R-52/54/56
gap rows).

**Key decompile quotes** (verified first-hand):
- `:186965` `local_a8 = done++; uVar8 = (local_a8 * 0x32) / (lVar12 + local_a8) + 0x28;`
- `:432112-432142` the row comparator: `SystemTimeToFileTime((SYSTEMTIME *)(row+0x20))`.
- `:432507-509` the open-failure path: no row is produced (the silent skip).

## Leg 2 — V-NTFS: the origin-LCN carrier + the batch-1 spot re-verification

The OQ-SA-1 decode chain HOLDS at NTFSSearchDLL_decompiled.c (:5126-27, :1987-88,
:2263-65, :41037-98, :22470-71; the stale name FUN_180033250 -> FUN_180003250 found
independently by the lead lane and the parallel N-100). **The resident provenance chain**
(the N-100 addendum now carries it): the og's resident wrapper carries {(u32)
walk-context record anchor, 1} — local_60, the 5th output of FUN_1800430b0 (:5066) ->
FUN_180042020 (:52709) <- the record ctor (:53315) — nonzero in raw-scan mode, NOT a
"zeroed raw block" as the cr_node.h:208-209 comment says.

**The batch-1 spot check — two landed rows refuted beyond the parallel waves' coverage:**
- **M-090** (DMG — the kept headline): og :49436-49440 (DMGRawFileInfo's OWN bound
  0xC800000, vs the global singleton 0xBEBC200 at :9049), :63806/:63843-63847 (the "koly"
  trailer walk, size = hit + 0x200, an 8-zero pre-gate) vs core's bzip2-EOS+10 walk +
  0xBEBC200 cap — carved DMGs truncated before the mandatory UDIF footer; the "koly"
  constant binary-probed at file offset 0x53222. Erratum E67.
- **M-091** (the MDB page-chain size walk, now decoded): og :52001-52039 — stride 0x1000
  from the first page, page-type word nonzero and <= 0x901, cap 0x6400000, size = the
  walked offset; core's landed row stands at the 64 MiB bounded extent with the now
  falsifiable "undecoded" comment. Complements the parallel M-084 (the ACCDB second
  signature, remote erratum E61).
- WMF (og :29829-29840): convergent with the parallel S-042; the lead addendum adds the
  exact size formula Size x 2 + 22 + the invented-inch-gate + fixture-regeneration points.
- CRW/MRW: convergent with the parallel M-081 (FIXED `9e5dedc`); the lead's constructor
  reads (FUN_18003c9f0 :46100-46125 = the CRW chain; :2156-2177 = OneAnalyser on the
  E4 52 5C 7B key; :2494-2510 = FileMRW on \x00MRM) are the arbitration record for the
  census corrections (remote errata E56/E57).

## Legs 3+4 — CARVE-B2a/b: the batch-2 decode (convergent with the parallel SCAN-4 table)

The lead lane's per-format decodes — CHM (:2488-2499, :17705-17740), MNY (:2901-2912,
:49956-49981), MPEG-TS (the 284->FH11 resolution; the sync scanners :35578-35677/
:38457-38536), EMF (:14204-14214, :3195-3209), DBF (absent — the census row was the EPS
family), 3DS (:2885-2888, :14535-14545), ONE (:2156-2177, :17160-17186), SWF (:3058-3074,
:29970-30259), JP2 (:3544-3559, :21812-21936), MOBI (:3600-3615, :29369-29485), HTML
(:2849-2879 incl. the FF FE chain, :14307-14380, :50865-50884), XML+SVG (:1806-1837,
:49622-49726), LZH (:3097-3111, :46937-46973), FCPEvt (:1963-1974, :20763-20841),
OfficeTagFromXML (:53202-53224, :53164-53194), CRW (:1746-1761 -> the CIFF chain),
the text school, the list-layer architecture (the three builders, the RawConfig token
gating, the only_common mode) — all converge with the SCAN-4 record and the remote errata
E56-E66. The kept school-2/3 fix units (S-047, M-092, M-093, M-096, M-098, M-099, N-111,
N-112, N-113, N-115) carry the per-format grammars the rollup's fix plan needs; the
school-1 formats' duplicate filings were dropped after `b92ebef` landed them.

## Leg 5 — GAP-SWEEP: the citation-gap three-pool analysis

No silently-unregistered in-scope surface remains in Cloud/, EuShare/, FileRepair/,
Installer/, Languages/, Licensing/, DiskHealth/, WinPE/, config-files/, per-binary/,
runtime-families/, PartitionRecovery/, MOM-modules/. Residue (final numbering):
**N-119** (D-88's stale premise — the EuShare artifact answers the row's own recorded
question, eumrx-nas-architecture.md:10-21) + **N-120** (the pointer/arithmetic drifts: the
I-010 D-26 over-claim; the two wrong-register SURFACE-INVENTORY pointers; the EuShare/
per-binary counts; the PARITY-CORE-REC off-by-one; the corpus-commit pin 03ad46c ->
be0cfe9) — the arithmetic overlap is carried by the parallel N-101 + the lead addendum.

## Leg 6 — CORE-SWEEP: the thin-module sweep + the thumbnail verification

The 10 thin module files swept; the thumbnail engine (05e6786) verified
MATCH-as-registered-superset (convergent with the parallel waves' thumbnail findings; the
lead lane's distinct additions: **M-100** the browse-inside gap, **M-102** the ABI-export
formalization). Residuals (final numbering): **N-121** (the ESP automount —
CUpdateGptSystemBootPartition, undispositioned), **N-122** (the NVRAM create/delete
exclusion), **N-123/N-124** (cli.md/transport.md staleness), **N-125** (the disconnect
states — R-16 never formalized before), **N-126** (the Unsaved File category),
**M-101** (the Versions/Tags tree branches), **I-029** (the DRIVER-CONTRACT code
provenance — 15 corpus-grounded codes + 4 core-wire), **I-030** (the module-map holes).

## Lead-agent first-hand verification log (the sites re-read by the lead, not just the legs)

- RAWSearchDLL :29829-29840 (WMF) — the S-042 gates quoted verbatim.
- RAWSearchDLL :1746-1761 + FUN_18003c9f0 :46100-46125 + :2494-2510 + :2156-2177 — the
  CRW/MRW/ONE key arbitration (the decisive constructor reads).
- RAWSearchDLL :17705-17740 (CHM), :46937-46960 (LZH), :14204-14215 (EMF), :52001-52039
  (the MDB walk), :49436-49440 + :63806 (the DMG bound/walk).
- core cr_analysers.cpp:3383-3411 (WMF-PL), :3053-3088 (MRW), :3488-3502 (MDB),
  cr_registry.cpp:709-740 (the three MRW rows).
- The og binary constant probes: RAWSearchDLL.dll greps — the HLP magics (0x3319/0x335e/
  0x343b), HEAPCCDR (0x6d4f6), \x00MRM (0x36f3), koly (0x53222), BZh1, wpg\0, Pain, Stuf,
  .snd, FOVb. (Post-merge note: the fix lane re-attributed the DBA5/94A6 rows to
  FileLowDocAnalyser in `9e5dedc` — the lead's binary greps confirmed the constants
  exist; the constructor read corrected the label. Convergent.)

## Gaps left open (recorded per finding, none blocking)

- G-1 (CHM/school-1 forms): the corpus has no .chm/.crw/.3ds sample (glob-verified) — the
  landed school-1 grammars are decode-first parity; a real-file corpus probe would
  strengthen (recorded in S-041's record).
- G-3 (list-layer reachability): the host-side list-enumeration call site is undecoded
  (M-093's school — affects og-yield comparisons, not the implementation).
- The XML generic marker (DAT_180093968) + the 0x1cd key semantics need .data hexdumps
  before M-093's generic-XML fallback lands.
