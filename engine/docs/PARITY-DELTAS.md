# PARITY-DELTAS — deliberate-deviations registry (AUTHORITATIVE)

<!-- N-191/N-207 (the S57 lane): the duplicate-D renumber pass. Earlier-push-wins on D-seats:
     N-143 keeps D-118 (M-125's later row -> D-127); N-163 keeps D-135 (N-177 -> D-128, N-090 -> D-129);
     S-060 keeps D-136 (N-165 -> D-130); S-050 keeps D-137 (N-169 -> D-131). The duplicate-number
     invariant is gate-checked (tools/doc_truth_gate.py check 5).
     The gate's first run surfaced four more born-collided seats: M-082 keeps D-107
     (the S-047 TS null-packet row -> D-132); S-044 keeps D-105 (the embedded-fast-path
     row -> D-133); N-105 keeps D-106 (the missing-member row -> D-138); the overlap-
     mapping row -> D-139. -->
> **Status: promoted from the parity-audit draft (docs/parity/PARITY-DELTAS-draft.md)
> after auditor review.** Every entry carries WHAT OG DOES (corpus cite), WHAT WE DO
> (file:line), WHY (rationale class), RISK, and VERDICT (ACCEPTED / NEEDS-WORK).
> A delta that is not in this registry is a bug, not a delta. `rda-wizard` carries the
> UI-scoped mirror (its docs/PARITY-DELTAS.md); this file is the single source of truth.
>
> **Parity-audit session 1 closures (2026-10-23):**
> - F-A1-3 (fixup validation scope) resolved as **PARITY, not a delta**: we now match the
>   og's first-two-tails validation (FUN_1800411f0). The 4Kn sector-size parameterization
>   (og hardcodes 0x200) remains a registered delta below (F1 family).
> - F-B-8 (abort-path error-map merge) closed by the concurrent UI-readiness work —
>   the stamp() lambda now merges the retry error map on every exit path.
> - G-18 gang-block clause and G-19 cross-disk LDM scoping: stale texts corrected in
>   rda-wizard/docs/CORE-CONTRACT.md (gang blocks are assembled; the resolver landed).
> - F-C-1 (§13 list-format transcription error) fixed in rda-wizard bd83be3.
> - Auditor wave-1 engine fixes that matched og behavior are PARITY (not deltas):
>   FAT12 sentinels, NTFS viability probe size, HFS+ B-tree layout, NVMe page-2 offsets,
>   carve stats double-count, undo-apply disk guard.

## A. Format / session

<!-- DFX30-T4 delta-ledger renumbering (2026-10-26): same-window parallel landings
     collided on D-75/D-78/D-79/D-80. Resolution: first-in-file keeps the number;
     logging seam D-75->D-81, CSMI N-032 D-78->D-82, Btrfs M-020 D-79->D-83,
     enumeration two-stack D-79->D-84, enumeration scope D-80->D-85; the pwddict
     D-78 row tombstoned (superseded by the D-80 convergence record). Live
     references updated repo-wide; worklog history verbatim. -->

<!-- REVIEW-SESSION ledger reconciliation (2026-10-26): the parallel-lane waves left
     12 same-number collisions (D-52/53/56/57/59/72/73/75/78/80/81/82 at 2-4 file
     positions, distinct topics sharing numbers). Resolved per the errata convention
     (first-landing-wins keeps the number; later same-window collisions renumber):
     D-91 HFS+ journal harvest+S-021 span fix (was D-52), D-92 ext4/Btrfs M-010
     lossless names (was D-53), D-93 HFS+ journal differential (was D-56), D-94
     FAT/exFAT open gates (was D-56), D-95 S-007 carve common set (was D-57), D-96
     session-7 INFO batch (was D-59), D-97 M-024 carve default-off (was D-72),
     D-98 S-029 APPLY safety registers (was D-72), D-99 M-043 backup fallback (was
     D-73), D-100 N-033 carve-registry consumption (was D-80), D-101 M-025
     stateless carve validators (was D-81), D-102 M-018 FilterFolder policy (was
     D-82). Dropped as superseded by documented renumbers: the D-78 CSMI copy
     (live at D-82), the D-78 pwddict copy (tombstoned per the D-80 convergence
     record), one D-78 N-075 union duplicate, the D-75 logging-seam copy (live at
     D-81). Live references updated repo-wide (7 sites); worklog history and the
     finding files' session texts stay verbatim (append-only convention). -->

<!-- DFX30-T4 delta-ledger repair (2026-10-26): the same-window parallel landings
     collided on D-75/D-78/D-79/D-80 AND committed nested conflict markers into this
     file (merge 220ebe lineage). Repair: markers stripped keeping both sides,
     duplicate-topic rows deduped (first kept), the superseded pwddict register
     tombstoned (replaced by the N-064 convergence record), colliding numbers
     renumbered first-come-wins (fresh: D-81+). Live references updated repo-wide;
     worklog history verbatim. -->

### D-01 — CRSF `.crsf` session format vs og `.rsf` (semantics parity, not byte identity)

- **WHAT OG DOES:** SaveScan `.rsf`: 16-byte EaseUS magic `EA F4 B3 1B…`, 0x24-byte
  record headers chaining zlib-chunked data — chunk framing u16 (bit 15 = deflate,
  bits 0–11 = size−1, ≤ 4 KiB, level-5, stored-when-not-smaller, raw-payload
  fallback on inflate failure). **No checksums over record chains** (the constant
  0xD739 at +0x014 is "likely header CRC/size", not a chain integrity mechanism);
  no container versioning; reader falls back per chunk but has no record-level
  skip-forward. Evidence: corpus `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md`
  §2–§3 (lines 29, 83–101), validated by `scripts/rsf_parser.py` + `rsf_dump_records.py`.
- **WHAT WE DO:** Own container `CRSFMT01` + record magic `CRSR`, explicit version
  u32, **CRC32 per record (decompressed payload)**, 512-byte skip-forward
  re-indexing on corrupt headers, five typed record roles. Chunk framing is the
  proven .rsf scheme verbatim. Evidence: `core docs/crsf-format.md` (whole file);
  `src/core/session/cr_crsf.cpp`; tests `tests/unit/test_session.cpp:1,107`
  (round-trip + F10 skip-forward); commit 32b31b5 (row 33 fix: session-id hi dword).
- **WHY:** correctness-fix (F10 — torn tails / silent empty records in og) +
  licensing (own magic/format identity; we do not emit byte-identical `.rsf`
  files carrying EaseUS's format magic).
- **RISK:** No interop with og sessions by design — `.rsf` import absent and
  classed [absent] as G-11 (wizard `docs/CORE-CONTRACT.md` §14). User-visible:
  our sessions cannot be loaded by og and vice-versa. Benchmark og leg works
  around it by *reading* og's auto-saved `.rsf` (extraction only, byte-exact
  validated — core `docs/benchmarking.md` §3).
- **VERDICT: ACCEPTED.** Both sides cited; the format is documented, tested,
  and the no-import decision is a tracked gap (G-11).
  *Minor gap (non-blocking):* the licensing half of the rationale (why we did not
  clone `.rsf` byte-for-byte) is implied (clean-room posture, README) but never
  stated in one sentence in `crsf-format.md` — recommend adding a "why not .rsf"
  paragraph at finalization.

### D-02 — fs_kind canonical factory map + legacy remap (F16)

- **WHAT OG DOES:** Two coexisting numbering maps — the factory/FS-type map
  {0–2 FAT12/16/32, 3 NTFS, 6 RAW, 7 exFAT, 8 HFS+, 9 EXT4, 10 ReFS, 11 Btrfs,
  13 ZFS} and the `.rsf` content map {0 NTFS, 1 FAT, 3 RAW, 4 exFAT, 5 HFS,
  6 ReFS, 7 Btrfs, 8 EXT4, 9 generic, 0xC ZFS}. Evidence: corpus
  `SearchDLL/engine-matrix--factory-contract-verification.md` (factory contract)
  + `SaveScan/rsf-format--scan-session.md` §5 (content map); summarized in core
  `AUDIT.md` row 45.
- **WHAT WE DO:** Single canonical `FsKindCode` (factory map), session load remaps
  legacy v1 value 12→13; legacy Btrfs-7 reads back as exFAT — documented
  limitation for v0.1 sessions only. Evidence: `include/cr/cr_types.h:57–80`
  (comment cites audit row 45), `src/core/session/cr_records.cpp:198`
  (`legacy content-map ZFS -> factory 13`), `docs/error-fix-registry.md` F16.
- **WHY:** correctness-fix (our own v0.1 collision bug, og-side ambiguity removed).
- **RISK:** Wire-visible only in our own sessions (self-inflicted v0.1 issue).
- **VERDICT: ACCEPTED** (both sides cited; this is a defect WE introduced and
  fixed, with og-side map evidence recorded).

### D-03 — Session auto-save naming + report export

- **WHAT OG DOES:** auto-saves at scan completion as
  `SaveScan/auto_<volume label>_<MM_DD_HH_MM_SS>.rsf`; user-visible session list
  `LastScanRecordView`. Evidence: corpus `SaveScan/rsf-format--scan-session.md`
  (sample names, §“446” auto-scan loop) + `architecture/04-recovery-workflow.md` §8.
- **WHAT WE DO:** Auto-save mirrors the og *behavior* (session saved at scan
  completion) with our own naming scheme; human-readable report export is
  UI-generated (G-09). Evidence: wizard `AUDIT-UI.md` ADR-10.
- **WHY:** licensing (no og filename patterns required) + scope.
- **RISK:** none material.
- **VERDICT: ACCEPTED** (behavior parity documented; naming delta deliberate).
  *Note:* the concrete naming scheme is not yet written down anywhere — speccing
  it belongs to the wizard's session-service work (see §I U-8 for the related
  `.rrc`/`.crsf` drift found in wizard README/worklog).

---

## B. Engine semantics

### D-04 — Byte-anchored geometry everywhere; 4Kn first-class (F1)

- **WHAT OG DOES:** `eprdtdrv.sys` READ/WRITE complete with
  `Information = Length << 9 // SECTOR = 512 BYTES (hardcoded)`; "user-mode must
  adapt to 4Kn/512e drives (sector-size normalization happens above the driver)".
  Evidence: corpus `PartitionRecovery/eprdtdrv-sys--driver-internals.md:68,73`;
  mirrored in `epmntdrv-internals-v2--completion-engine-identity-ioctls.md` (+0x38
  io_info "sector count << 9") and driver repo `docs/error-fix-registry.md` F1.
- **WHAT WE DO:** Per-target logical/physical sector size captured at open;
  byte-anchored offsets; PartitionSlice validates every access in bytes; FAT32
  backup-boot fallback reads at 6×logical-sector (4Kn-aware, AUDIT row 9 fix);
  ZFS uberblock ring auto-probes ashift shifts 10–13. Evidence: core
  `docs/requirements.md` R1.2; `src/core/fat/cr_fat_quick.cpp:275`
  ("violates the locked fix F1 'no fixed 512 anywhere'"); AUDIT.md rows 9/21
  (Round 1); tests `test_fixup.cpp` ("4Kn record … fix honored"),
  `test_fat_volume.cpp`, `test_zfs_volume.cpp`.
- **WHY:** correctness-fix.
- **RISK:** User-visible improvement on 4Kn media; none on 512-byte media.
- **VERDICT: ACCEPTED** (both sides cited; F1 markers in code+tests).

### D-05 — ZFS deleted-file story: newest-txg live state only — PREMISE REFUTED (S-034, DFX30 2026-10-26)

- **WHAT OG DOES (re-decoded first-hand):** walks EVERY uberblock slot in the
  label-0 ring (quick-searcher run loop FUN_180006b90 :5685-5841: per-slot
  `txg = *(u64*)(slot+0x10)` -> FUN_180003150 "addBlock" with a per-txg
  `VdevLoc::UberData` registry (FUN_180017310) and a per-state tree walk
  emitting ZplNodes via a `bool(shared_ptr<ZplNode>)` visitor; "from_txg: 0,
  birth_txg: <txg>" on the success path, unbounded descent). Files deleted
  within the ring's txg window ARE og-visible.
- **WHAT WE DO:** newest-txg live state only; deleted content left to RAW
  carving. Evidence: `src/core/zfs/cr_zfs_engine.cpp` (best_txg ring walk;
  the header comment — corrected at S-034's filing — previously asserted the
  refuted premise).
- **WHY THE ORIGINAL D-05 WAS WRONG:** its corpus citation
  (`zfs-engine--deep-dive.md` §2 "selects the highest-transaction-group valid
  uberblock") over-read a NEIGHBOR-txg comparison inside the ring scan
  (`plVar12[-0x7e] <= plVar12[2]` = slot-1KiB-back vs current) as a
  newest-only SELECTION; the caller-level semantics (which the deep-dive did
  not decode) iterate every slot per txg.
- **RISK:** Deleted-ZFS recall strictly BELOW og inside the ring window.
- **RESOLUTION (DFX33b, 2026-10-26 — S-034 FIXED):** the three decode items
  are answered first-hand (see the finding's Resolution): (1) cross-txg
  emission — the quick searcher's collector FUN_180006b40 is a plain
  push_back with NO engine-side dedup; every state's ZplNode reaches the
  host, and the walk's only gate (FUN_18001d9a0 :26210) is
  `from_txg < birth_txg || from_txg == 0` — always true at the top level
  (addBlock descends with from_txg=0). (2) the per-txg descent re-walks
  each state's own dataset/ZPL tree (the MOS snapshot list is per-state
  content, not re-emitted separately). (3) `last_txg` is display-only (the
  previous iteration's txg in the progress log string; no walk bound).
  The core now walks every valid ring state (all four labels' rings —
  the established geometry superset; txg-deduped, newest-first): the
  first state whose MOS chain opens is the LIVE tree; each older state
  is a RECALL walk emitting only entries whose cross-state identity
  (dataset, parent dir, entry name, object id, dnode bp0 birth) no newer
  state claimed — deleted-in-window content surfaces as Deleted nodes
  in place, unchanged directories are re-walked against their existing
  node (deleted descendants surface under them), and modified-in-window
  entries recall the prior version as a Deleted sibling. Torn states are
  skipped like the og's FUN_180016c50 tree-open failure. The identity
  dedup is a deliberate improvement over the og's raw N-per-state
  emission (the og dedups host-side; a recovery tree cannot hold N
  duplicate paths) — registered here as the S-034 design note.
- **VERDICT: FIXED — parity restored inside the ring window** (findings/
  FIXED/S-034-zfs-ring-txg-history-not-walked.md; tests
  Zfs.RingWindowDeletedFileRecalled / RingWindowUnchangedEntriesEmitOnce /
  RingWindowModifiedFilePriorVersionRecalled / RingTornNewestStateFallsBackToOlder
  — all four fail-before RED on the pre-fix engine).
### D-06 — Btrfs deleted recovery: newest-generation tree only (row 20) — CLOSED

- **WHAT OG DOES:** Superblock selection = max(generation) across mirrors; only the
  newest-generation tree is walked; deleted files surface via RAW carving.
  Evidence: corpus `SearchDLL/refs-btrfs-zfs-engine-algorithms-v2.md` §2
  (newest-generation-wins, FUN @ corpus line 10290) + `btrfs-engine--deep-dive.md`
  (no old-generation walk present).
- **WHAT WE DO:** Same policy, and since W5 the promised header note exists:
  `include/cr/btrfs/cr_btrfs_engine.h` carries the deleted-file-story paragraph
  mirroring the ZFS (D-05) wording, citing AUDIT.md row 20 + the corpus §2.
- **WHY:** scope (parity with og).
- **RISK:** none — behavior matches og; documentation gap closed.
- **VERDICT: CLOSED (ACCEPTED).** W2 additionally landed BTR-2/3/4 (chunk-root
  uuid from the node header, inline-extent content, compressed extents) — all
  decompile-cited engine parity, not deltas.

### D-07 — ext4 jbd2 journal salvage: ALL blocks surfaced, og-exact — CLOSED (was: revoked-blocks divergence)

- **WHAT OG DOES:** Feeds EVERY block in [s_first, s_maxlen) to the directory
  parser after gating only on the jsb magic 0xC03B3998 + blocktype {3, 4} —
  **no descriptor/commit/revoke semantics at all**. Evidence: EXT4SearchDLL
  decompile FUN_18000e5b0 + FUN_18000e890 (:12159–12368), pinned during the
  deep-parity audit (AUDIT-LINUX-FS E-2, plan ruling D-G).
- **WHAT WE DO:** Exactly that, since W2-10 (commit 435bc5e): the journal walk
  covers all blocks in range with no commit gating; uncommitted-tail directory
  blocks surface as lost candidates. Evidence: `src/core/ext4/cr_ext4_engine.cpp`
  ("D-07 RE-PINNED (AUDIT-LINUX-FS E-2 …): the og gates on the jsb magic +
  blocktype {3,4} then parses EVERY block … with NO descriptor/commit/revoke
  semantics"; regression test JournalSalvage surfaces the uncommitted tail).
- **WHY:** parity (the prior commit-gated salvage was an unregistered
  divergence that DROPPED entries og surfaces).
- **RISK:** none — this widened salvage to og-exact.
- **VERDICT: CLOSED (PARITY).** The old "deliberately surface revoked blocks"
  divergence is obsolete: og has no revoke handling to diverge from. The prior
  NEEDS-WORK action (pin og revoke handling) resolved the opposite way the draft
  expected — the decompile showed no revoke semantics exist engine-side.

### D-08 — LDM recovery-grade divergences (clamp-not-abort family)

- **WHAT OG DOES / GROUND TRUTH:** kernel `block/partitions/ldm.c` aborts on
  uncommitted transactions (ldm.c:461–465 vblk-size bound), disk/database overlap,
  config overrun; libldm aborts on composite dup-fragments; og `LdmManager`
  defaults zero `vblk_size` to 0x80 (corpus `LdmManager` decode OG:59525–59528,
  cited in our code). Evidence chain: core worklog S5-LDM (kernel + flatcap +
  libldm triangulated), AUDIT.md row 30, Round 3.
- **WHAT WE DO:** Damaged records counted in `LdmStats` and skipped — never
  silently, never aborting the walk: config-region clamp
  (`src/core/table/cr_ldm.cpp:1093–1098`), degraded flag for uncommitted
  transactions (`:1149–1162`), dsk4 floor-vs-equality divergence (`:626`,
  "documented divergence - AUDIT.md row 30"), og 0x80 default kept
  (`:195–200`).
- **WHY:** correctness-fix class (recovery stance vs mount stance) — a recovery
  scanner must not abort.
- **RISK:** We may surface volumes the kernel would reject — flagged `degraded`,
  never silent.
- **VERDICT: ACCEPTED** (both sides cited at the code sites; upstream-vs-og
  distinction explicitly recorded).

### D-09 — ZFS scoping: vdev handling parity (BE pools refused; G-18)

- **WHAT OG DOES (pinned since M-019 + the OQ-T6/OQ-Z3 decodes):**
  vdev-TYPE-BLIND (zero vdev-type/geometry strings in ZfsSearchDLL —
  session-2 scan), and its DVA read path NEVER resolves across vdevs:
  FUN_1800168b0's registry loop falls back to the FIRST registry entry on
  any vdev-id miss (first-hand :19541-19560 — single-vdev behavior with
  first-entry fallback; mirrors read through vdev 0 "by accident"; RAID-Z
  parity math is impossible in the build). Evidence:
  `findings/FIXED/M-019` Resolution + the OQ-Z3 re-verification in
  `tracked-open/OQ-Z-E-FLT-session3.md`.
- **WHAT WE DO:** Single-vdev pools only; mirror/raidz detected and reported as
  unsupported (`stats.unsupported_vdevs`, `src/core/zfs/cr_zfs_engine.cpp:1144–1145`);
  BE pools refused with a clear error (`cr_zfs_engine.cpp:28–30`); header
  documents scope (`include/cr/zfs/cr_zfs_engine.h:1–32`). Wizard surfaces it
  honestly (G-18, `CORE-CONTRACT.md` §14).
- **WHY:** scope.
- **RISK:** User-visible capability limit; stated honestly per G-18.
- **VERDICT: ACCEPTED (og story pinned, M-019 parity landed).** Both former
  NEEDS-WORK actions closed: (1) the og-side multi-vdev story is pinned
  (first-entry fallback, type-blind — above) and the core MATCHES it since
  M-019 (foreign-vdev blocks planned at the fallback position,
  `foreign_vdev_reads` counted, never refused — the "single-vdev only +
  unsupported_vdevs" text above is the pre-M-019 posture); (2) G-18's
  wording "gang blocks counted damaged" is **stale** — `read_gang` is
  implemented (`cr_zfs_engine.cpp:486–491`, AUDIT Round 1: gang header =
  3 blkptrs) —
  update G-18/CORE-CONTRACT to "gang blocks assembled; mirror/raidz report-only".

### D-10 — NTFS remnant scan: FILE0 shifted-header salvage, full-record improvement

- **WHAT OG DOES:** `'FILE'+0x30` probe at +0x58/+0x60, record parsed from the
  shifted start into a zeroed buffer, copying only `record_size − delta` bytes
  (a buffer artifact), **without applying USA fixup**. Evidence: corpus
  `NTFSSearchDLL` FUN_18002a780 (verified, AUDIT.md row 6d + §6 correction:
  lives in the $LogFile RCRD walk chain FUN_180029dd0→18002a0f0→18002a780,
  decompile lines 34152–34166).
- **WHAT WE DO:** Ported to `scan_remnant_buffer`; **improvement over og**: a full
  record is read when the block buffer reaches past the shift (og's truncation was
  a buffer artifact); at the buffer edge we keep the og's truncated form; fixup
  attempted, parse proceeds on fixup failure (matches og's fixup-optional salvage).
  Evidence: `src/core/ntfs/cr_mftwalk.cpp:958–994`; commit 32b31b5 (row 6d);
  worklog S5-NTFS.
- **WHY:** correctness-fix (recovers record tails og loses).
- **RISK:** Strictly more salvage than og; graded Lost/MetadataOnly — safe.
- **VERDICT: ACCEPTED** (both sides cited incl. the og buffer-artifact analysis).

### D-11 — NTFS $UsnJrnl harvest + dedup refinements

- **WHAT OG DOES:** Complete scan cross-checks `$Extend/$UsnJrnl:$J` for
  recently-deleted files. Evidence: corpus `SearchDLL/ntfs-engine--deep-dive.md`
  (walk table: "USN journal cross-check (`$UsnJrnl` strcmp sites at
  9070/19761/19868/20282)").
- **WHAT WE DO:** `$J` runs captured during the metafile pass, 64 MiB-bounded
  stream read, USN_RECORD_V2 **and V3** (128-bit refs, true shifted layout),
  FILE_DELETE-only signal (CREATE=0x100 corrected — 0x10000 is HARD_LINK_CHANGE),
  dedup vs structured (parent,name) pairs via hash set + in-journal dedup,
  8-byte resync on damaged records. Evidence: `src/core/ntfs/cr_mftwalk.cpp:745–930`;
  AUDIT.md row 7 + Round 3 corrections; worklog S5-NTFS / S5-TAKEOVER.
- **WHY:** parity (og capability) + correctness refinements.
- **RISK:** Journal-evidenced names surface as MetadataOnly/Deleted under
  root_lost — matches og intent, exceeds og robustness.
- **VERDICT: ACCEPTED** (both sides cited; our refinements each carry evidence).

### D-12 — $Recycle.Bin $I/$R mapping (incl. Win10 $I pathlen format-dependence)

- **WHAT OG DOES:** `$RECYCLE.BIN` / `$I…` / `$R…` recognized; recycle-bin items
  surfaced through `IRecycleBinNode2UI` / `CRecycleBinRecoveryController`
  "including the $I…/$R… info/restore" mapping. Evidence: corpus
  `SearchDLL/fat-exfat-engine-internals.md` §4 (byte-exact strings); UI side
  `architecture/04-recovery-workflow.md` (Recycle Bin row),
  `07-user-flow-uiux.md` (dedicated flow).
- **WHAT WE DO:** Known-metafiles-only NTFS filter (no blanket `$`-skip), $I
  parsed with **format-dependent pathlen**: Vista..Win8 fixed 544-byte form vs
  Win10+ variable form where @0x18 is repurposed as a deletion-sequence counter
  (external forensic literature + runner benchmark evidence); original name/size/
  time from $I, content remapped from $R; $I metadata never surfaces.
  Evidence: `src/core/ntfs/cr_mftwalk.cpp:149–204`; commits d0e41a0 (Gen-3a),
  16c6c60 ($I pathlen FORMAT-DEPENDENT).
- **WHY:** parity (og capability) + correctness-fix (our blanket-$-skip bug) +
  external-source correctness (Win10 $I form).
- **RISK:** Low; strictly better recycle-bin fidelity.
- **VERDICT: ACCEPTED**, with one note: the Win10-sequence-counter handling is
  evidenced by external research + our benchmark, **not** by the corpus (og's own
  Win10-$I handling is unrecorded) — if the og comparison leg shows og mishandling
  Win10 recycle deletions, add the og-side observation here (feeds §I U-7).

### D-13 — Carving deltas: RAR5 addition, PST hardening, MP3/ZIP/PDF semantics

Split verdict — three sub-claims:

- **D-13a RAR5 analyser (ACCEPTED).** Og: `raw_signs.json` (139 extensions) has
  **no rar entry** and the og validators "reject byte 6=0x01 entirely" (corpus
  `SearchDLL/rawsearch-carving--deep-dive.md` registry census; commit 94ce57d
  cites both). Ours: clean-room RAR5 vint walk (8-byte sig, ENDARC-terminated,
  confidence 880/700) — `src/core/carve/cr_analysers.cpp:760+`, commit 94ce57d,
  tests. A superset capability, documented as "clean-room improvement".
- **D-13b PST/OST hardening (ACCEPTED).** Og floor 0x100000 for damaged headers
  (FUN_180041eb0 — AUDIT §2 "REJECTED as a defect (it IS corpus parity)"); ours
  keeps the floor and adds the `wMagicClient == 'MS'` gate ("hardening beyond og",
  commit 94ce57d). Both sides cited.
- **D-13c ZIP EOCD selection + PDF bound + shared-magic precedence (CLOSED by W2
  with our own decompile-derived tests).** The og-side semantics the draft asked to
  pin are now implemented to the corpus-artifact contract and pinned by tests
  built from that contract: ZIP 3-anchor salvage (LFH + CD `PK\x01\x02` + EOCD
  `PK\x05\x06`) with CD-ownership reconciliation and the og zero-padding
  extension (5.24 MB cap; corpus `rawsearchdll--zip-analyser-carving.md`); the
  nearest-EOCD-that-owns-the-CD selection replaces the old first-wins rule and
  rejects a later archive's geometry-consistent EOCD (commit 7577f74, tests
  planted to the real CD layout). PDF remains last-%%EOF bounded by the next
  %PDF- hard boundary (rawsearch deep-dive 41230–41233); shared-magic families
  still take the highest-confidence sign (the og table is ambiguous there by
  construction). Residual: the multi-EOCD tie-break inside a single corrupted
  archive remains ours-by-necessity (the corpus documents parse-and-continue,
  not the selection rule) — tracked as the narrow remainder of this entry.

### D-14 — Engine hardening family F4–F9 (fixup mandatory, viability grading, depth/cycle limits, validator gate, merge dedup)

- **WHAT OG DOES / GROUND TRUTH (per fix):**
  - F4 fixup optional in some paths / torn records silently parsed — corpus
    `ntfs-engine--deep-dive.md` (fixup sites; "fixup-aware" strategies; the
    viability walk's tolerance documented in `ntfs-engine-algorithms-v2--deleted-viability.md`).
  - F5 destroy-only-fully-reused, no graded confidence — corpus
    `ntfs-engine-algorithms-v2--deleted-viability.md` §1 (pseudocode: `props.Release()
    ← ALL clusters re-allocated → destroyed`; "destroys such candidates when every
    cluster is re-used"; the intact==total→destroy asymmetry).
  - F6 $ATTRIBUTE_LIST chase unbounded — corpus `ntfs-engine--deep-dive.md`
    (FUN_18001a3c0/a540 fragmented-MFT handler; no depth bound recorded).
  - F7 FAT chains RB-tree-guarded but EBR chains trusted — corpus
    `fat-exfat-engine-algorithms-v2.md` (chain walker §2 visited-range RB-tree) vs
    `Partition.mo` EBR handling.
  - F8 header-only raw_signs entries inflate counts — corpus
    `rawsearch-carving--deep-dive.md` (raw_signs.json layer: header/tail signs +
    confidence only, no grammar mandate) + `05-scan-engine-architecture.md` §6.
  - F9 merge relies on UI tree attach (no content dedup) — corpus
    `05-scan-engine-architecture.md` §7.3 ("structured nodes win; carved nodes
    attach as Lost/Other deleted files").
- **WHAT WE DO:** F4 mandatory USA gate before any parse (`cr_mftwalk.cpp`
  "fixup mandatory"; `cr_fixup.cpp`); F5 four-tier confidence grading surfaced in
  node metadata + sessions (`cr_mftwalk.cpp:477–499`); F6 FRAG_DEPTH_LIMIT 64
  with cycle guards (32b31b5); F7 visited-set guards on every chain incl. EBR /
  GPT alternates / attrlist (`cr_chain.cpp`, `cr_table_parse.cpp` "cycle guard
  (fix F7)", tests `test_crc32_gpt.cpp:192`, `test_fat_volume.cpp:385`); F8
  load-time validator gate (`cr_registry.cpp`, test `test_carve.cpp:315`
  RegistryF8Gate); F9 merge dedup by (first-cluster, size-class, hash) before
  tree attach (`cr_carve.cpp`, test "THE F9 invariant: pass 2 adds zero nodes").
- **WHY:** correctness-fix family.
- **RISK:** Confidence grades are user-visible additions (og shows binary
  found/destroyed); all other deltas are strict improvements.
- **VERDICT: ACCEPTED** — every F-entry has a corpus cite (above), an our-side
  code cite, and at least one test carrying the F-marker (F-marker census:
  `grep -r "fix F"` over src/ + tests/).

### D-15 — Error-code taxonomy: 0x5003–0x5009 families beyond og's 0x5002

- **WHAT OG DOES:** "engine error codes are HRESULT-style `0x5002xxxx` (e.g.
  0x5002200000004 open-fail, 0x5002c0000004 read-fail)". Evidence: corpus
  `architecture/05-scan-engine-architecture.md` §4 (line 62). No other families
  appear anywhere in the corpus engine artifacts.
- **WHAT WE DO:** `0x5002` open/transport (reference class) **extended** with
  0x5003 format/parse, 0x5004 session, 0x5005 writer/policy, 0x5006 FVE,
  0x5007 health, 0x5008 repair, 0x5009 internal. Evidence:
  `include/cr/cr_error.h:1–60` ("Reproduces the reference engine convention …
  while extending it"); `docs/architecture.md` §5 ("the reference engine
  convention, extended").
- **WHY:** modernization/correctness (distinguishable failure domains).
- **RISK:** Codes are internal + CLI/log surface only; no wire contract with og.
- **VERDICT: ACCEPTED** (og side cited; "og 0x5002-only" is an absence claim —
  acceptable as the corpus is exhaustive on engine codes — note recorded).

### D-16 — BitLocker: detect + legitimate unlock only, no decryption (F13)

- **WHAT OG DOES:** Full FVEK/XTS content decrypt in-product: DRWUI imports
  OpenSSL `EVP_aes_{128,256}_xts`; BitLockerLib/DRWService implement FVE parsing
  and volume decrypt. Evidence: corpus
  `DRWService-exe--bitlocker-service.md:44,88` ("Volume content decrypt:
  XTS-AES via OpenSSL"); `bitlockerhelp.mo` FVE/FVEK parsing cross-links.
- **WHAT WE DO:** Detection (FVE metadata signatures, corpus-cited
  `cr_bitlocker.cpp:78`), legitimate Windows unlock only (PowerShell
  `Unlock-BitLocker`, manage-bde fallback, FVEAutoUnlock registry BEK blobs →
  temp .BEK + `-KeyFilePath`), locked volumes reported locked; **no decryption
  code at all**. Evidence: `docs/requirements.md` R8; `src/win/cr_bitlocker.cpp`
  header ("fix F13: no crypto in-process"); AUDIT row 42 + Wave-2 fix.
- **WHY:** licensing/legal (directive) + safety.
- **RISK:** User-visible: og can scan BitLocker volumes after unlock *and*
  decrypt them in-process; we require the OS unlock and scan through it. Locked
  volumes are honestly reported.
- **VERDICT: ACCEPTED** (both sides cited; directive documented in R8/F13).

### D-17 — SafeWriter: hard destination-overlap refusal in code (F12)

- **WHAT OG DOES (as evidenced):** destination picker *warns* ("We highly
  recommend saving the found data to a different drive or a network drive" —
  corpus `DRWUI-exe--main-ui-architecture.md`, `ChooseDestinationPathDialog`);
  `CRecoveryMgr::ExportThread` opens the destination volume raw handle for
  "writability + same-volume detection" (`architecture/04-recovery-workflow.md`
  §7.1–7.3). **The exact og refusal semantics are not pinned**: whether the
  same-volume detection refuses, warns, or only steers defaults (volume vs disk
  granularity, source-set awareness) is not decoded anywhere in the corpus.
- **WHAT WE DO:** Hard code-level refusal when the destination volume/disk
  overlaps the source set of any queued node (R7.2): `cr_safewriter.cpp:76–89`
  (ERR_DEST_OVERLAP), CLI warn + refuse (`rrecov.cpp:643–691`), test
  `test_orchestrator.cpp:332` ("hard refusal (F12)"), plus the UI hard gate
  (ADR-7, see D-24).
- **WHY:** safety (writing recovery output onto the source destroys evidence).
- **RISK:** User-visible: og apparently allows destination-on-source with a
  warning; we refuse. Intentional.
- **VERDICT: NEEDS-WORK (evidence precision).** Our side is fully cited; the
  og-side claim "destination validation is UI-level / og can write onto the
  source disk" overstates what the corpus proves (an engine-side same-volume
  check EXISTS in ExportThread). Action: decode `CRecoveryMgr::ExportThread`'s
  same-volume branch (trace @354990–355155, `DRWUI-subsystems--assistant-grading-preview.md`
  §3) to establish refuse-vs-warn + granularity, then correct F12's wording and
  this entry. Keep the hard refusal regardless (safety stance stands on its own).

### D-18 — Cancellation first-class + pool-parallel scans (F3)

- **WHAT OG DOES:** Cancel callback consulted inside every engine loop head
  (`if controller.callback.vt[0x10]() != 0: break` — corpus
  `ntfs-engine-algorithms-v2--deleted-viability.md` §1; same pattern in
  `ext4-hfs-engine-algorithms-v2.md`); UI-side `CStopScanThread` cooperative stop
  (`DRWUI-exe--thread-architecture.md` §1). **No IRP cancellation in the driver**
  (driver F4: KeWaitForSingleObject with no cancel routine — driver
  `docs/error-fix-registry.md` F4). Blocked reads therefore hang the stage.
  Engine-internal threading exists in places (FAT `QuickSearchWorker` — corpus
  `searchdll-family--architecture.md` cross-links; 45 named DRWUI threads).
- **WHAT WE DO:** `CancelToken` checked in every loop head AND propagated into
  pending reads (overlapped/CancelIoEx; RDA path closes handle on abort — F3);
  `cr_parallel` worker pool (thread-safe tree, parallel carve/remnants/lost
  candidates/LDM volumes) + TSan CI. Evidence: `docs/architecture.md` §4 + §1;
  F3 in `docs/error-fix-registry.md`; driver F4 (cancellable forward-wait);
  commit 94fa821, f63d471 (TSan).
- **WHY:** correctness-fix (F3) + modernization (parallelism).
- **RISK:** None user-visible beyond faster scans + responsive cancel.
- **VERDICT: ACCEPTED** — with a citation-hygiene note: roadmap v0.2's parenthetical
  "mirrors the corpus model: engines single-threaded per unit, the host fans units
  out" is a simplification (corpus shows engine-internal worker threads too);
  reword to "extends the corpus's host-fans-out model with a first-class pool" or
  cite the specific corpus pages (feeds §I U-14).

---

## C. Safety (product-level)

### D-20 — UI hard destination gate (ADR-7, defense in depth)

- **WHAT OG DOES:** Destination funnel is monetization-first: the destination
  dialog is "only reachable with a sub-selection under the free limit — the mass-
  recover path is intercepted by monetization first" (corpus
  `UIUX/30-interaction-driver-ground-truth.md`); free 2 GB gate family
  (`CRecoverLimitDlg`).
- **WHAT WE DO:** UI computes disk identity via `rrecov list` + volume→disk
  resolution and refuses up-front before queuing, regardless of CLI behavior.
  Evidence: wizard `AUDIT-UI.md` ADR-7; implemented in
  `src/RdaWizard.Core/Wizard/WizardStateMachine.cs:58,133,146`,
  `RecoveryViewModel.cs:12`, `Services/Interfaces.cs:90`,
  `Integration/Parsers/OutputParsers.cs:161`.
- **WHY:** safety (prompt's hard-gate requirement on top of core F12).
- **RISK:** Stricter than og (og lets you proceed with a warning; see D-17 for
  the og-side caveat).
- **VERDICT: ACCEPTED** (both sides cited; layered on D-17's core gate).

---

## D. Architecture / build

### D-21 — In-process engine registry; no engine DLLs (F14)

- **WHAT OG DOES:** 9 `*SearchDLL.dll` engine binaries in the build tree
  (corpus `architecture/01-build-tree-intelligence.md` §tree: "DRWUI.exe, … ALL
  *SearchDLL.dll"); the IFileProperties class family is statically linked into
  **every** engine DLL ("the engines construct live props objects themselves" —
  corpus `fat-exfat-engine-algorithms-v2.md:82–90`; corroborated by
  `remaining-engines--ext4-hfs-refs-btrfs-zfs.md:90–92`: RTTI overlap proves the
  per-DLL static "engine SDK").
- **WHAT WE DO:** Single C++20 in-process registry with `crCreate*`-style
  factories; engines consume one shared refcounted node library. Evidence: core
  `README.md` architecture diagram ("no engine DLLs — in-process registry");
  F14 registry entry.
- **WHY:** modernization/build hygiene (kills the per-DLL duplication F14).
- **RISK:** None user-visible (og's DLL-per-engine is a build-tree property).
- **VERDICT: ACCEPTED** (both sides cited).

### D-22 — rda-driver: three reference drivers merged into one; wire preserved; defects fixed (read-only notes)

- **WHAT OG DOES:** 7 driver binaries for 4 roles; the raw-access core exists as
  **4 signed copies of the same compiled code** (eprdtdrv / ebrntdrv / epmdkdrv /
  eprdtdrv_win7older, all `epmntdrv.pdb`); separate EUDCPEPM hook + EUEDKEPM
  access drivers. Evidence: corpus `architecture/02-kernel-driver-architecture.md`
  §1 (line-cited).
- **WHAT WE DO:** One WDM driver `rda.sys` merging all three roles under
  service-name-derived identities (drop-in `eprdtdrv` service name supported);
  wire contracts preserved byte-semantically (open grammar, 15-command session
  protocol, `Information = Length × 512` **deliberately preserved** — F11
  "intentionally identical", verified against Device.mo rescaling). Fixed
  reference defects F1–F15 each with decompile evidence; the registry was itself
  corrected by line-by-line re-verification (audit doc G6: the old F13
  "reference leaks the top ref" claim was WRONG and reworded — reference drops
  the top ref; our stricter ownership kept as a documented divergence).
  Evidence: driver `README.md`, `docs/error-fix-registry.md` (F1–F15 + "Documented
  benign divergences": GET_VERSION product string, 4 stub session op codes,
  opaque handles), `docs/audit/2026-10-22-decompile-reverification.md` (G1–G28),
  `docs/audit/2026-10-22-security-review.md`.
- **WHY:** modernization + correctness-fix + security (F2 SDDL — the one
  wire-visible change: non-admin opens that previously could succeed now fail,
  documented in the registry's "Wire-visible?" column).
- **RISK:** Wire-visible items enumerated in the registry (F2/F3-class ACL change;
  F10 direct-method IOCTLs now work — strictly broader).
- **VERDICT: ACCEPTED** — the driver repo is the best-evidenced delta set of the
  three (every row carries decompile cites; contradictions were re-verified and
  corrected rather than copied).
  *Nuance note:* core F2's phrasing "PID token treated as a security boundary"
  overstates the corpus, which calls the handshake "a lightweight anti-abuse
  bind" (`epmntdrv-trio--naming-pid-handshake.md` §3). Substance unchanged
  (any process can compute its own token); recommend softening the wording at
  finalization.

### D-23 — Win7-inclusive posture with machine-verified import allowlist

- **WHAT OG DOES:** `eprdtdrv.sys` built 2022 with a modern toolchain, PE OS 6.1,
  41 Win7-era ntoskrnl imports, ships Win7→Win11 ("win7older" = same bytes,
  differently signed). Evidence: corpus `architecture/02-kernel-driver-architecture.md`
  §1 + driver `docs/research-digest.md` §A / `docs/win7-compat.md`.
- **WHAT WE DO:** Same approach as a checked build property: NTDDI pinned
  0x06010000, PE `/osversion:6.1 /subsystem:native`, CI gate `tools/verify_pe.py`
  (imports ⊆ 75-function allowlist = reference-binary union + 9 audited XP-era
  additions, each with a stated reason); honest non-claims documented (signature
  policy per-OS; x64 only; hosted CI compiles but cannot load).
  Evidence: driver `docs/win7-compat.md` (whole).
- **WHY:** modernization (single binary, latest WDK) + parity with og's own
  deployment trick.
- **RISK:** None; the allowlist converts an assumption into a gate.
- **VERDICT: ACCEPTED** (both sides cited).

### D-24 — Wizard architecture: CLI-subprocess now, native shim later (ADR-1); truthful-degradation ADRs 2–6

- **WHAT OG DOES:** Qt5 DRWUI links engines in-process through the factory/DLL
  dispatcher; browse-while-scanning with live tree; embedded preview/office
  viewers; crash supervisor via 2 MB shared-memory handshake. Evidence: corpus
  `05-uiux-architecture.md`, `DRWUI-exe--main-ui-architecture.md`,
  `04-userflow-state-machine.md` (synthesized in wizard `docs/UI-MAP.md` §1–14).
- **WHAT WE DO:** UI drives `rrecov.exe` as a monitored subprocess behind
  `IEngineHost` (never links C++ classes; every flow 1:1 a CLI line); ADR-2/3:
  results populate at stage boundaries, no Pause button (dead buttons forbidden),
  Cancel-with-warning; ADR-4 truthful-coarse progress (no fake ETA); ADR-5
  driver-absent = truthful direct-path label; ADR-6 UI pre-creates destination
  tree. Every gap mapped to core issue G-01…G-23 with a degradation row.
  Evidence: wizard `AUDIT-UI.md` §1 (ADR-1…ADR-6) + §3 truth table;
  `docs/CORE-CONTRACT.md` §14.
- **WHY:** scope/safety (crash isolation, truthfulness mandate) + core-capability
  reality (no C ABI — G-01).
- **RISK:** User-visible vs og: no mid-scan recovery, no live tree, no pause —
  each stated truthfully and tracked (G-02/G-03/G-07).
- **VERDICT: ACCEPTED** (both sides cited; degradations enumerated, not hidden).
  *Update (G-01 landed, 2a93c45):* the "native shim later" half of the ADR-1
  hedge now exists — `cr_ui.dll`/`libcr_ui.so`, the og's in-process
  factory-binding architecture (see D-45). The wizard may swap
  IScanService/ISessionService/IRecoveryService to in-process bindings
  behind the same Core interfaces; the CLI-subprocess mode remains valid
  (crash isolation) and stays the parity-tested default until the swap
  lands.
  *Consistency note:* DESIGN-SYSTEM §12 lists "browse-while-scanning (DYN-scan
  §3)" among behaviors "kept from DRW because they are honest and good", while
  ADR-2 defers live results to stage boundaries — the "kept" claim is aspirational
  until core G-02 lands; recommend reconciling the two docs (feeds §I U-2).

---

## E. UI / UX

### D-25 — WizardStepper 3-step rail (ADR-12)

- **WHAT OG DOES:** No stepper; `DrwMainViewManagerWizard` is a QStackedWidget
  with 4 top-level pages (DrwHomePage/DrwLostDiskPage/PeripheralHomeView/
  CDetailPage, landing index 2; instant page switch). Evidence: corpus
  `05-uiux-architecture.md` (view-manager row, ctor decode artifact 04);
  DESIGN-SYSTEM §7 cites ARCH05 §3 for the instant QStackedWidget switch.
- **WHAT WE DO:** 3-step rail (Select → Scan → Recover), documented as "a
  deliberate, user-first adaptation for non-technical users — not presented as
  parity". Evidence: wizard `AUDIT-UI.md` ADR-12; `docs/DESIGN-SYSTEM.md` §8.1
  (WizardStepper `[ADAPTATION]`).
- **WHY:** modernization/UX.
- **RISK:** Navigation-model difference, deliberate.
- **VERDICT: ACCEPTED** (both sides cited; explicitly not claimed as parity).

### D-26 — Omitted surfaces (cloud/NAS/RAID/WeChat/WinPE-builder/AI/monetization/telemetry/OEM/multi-language)

- **WHAT OG DOES (per item, cited in UI-MAP §13):** cloud recovery + OAuth
  (P15), NAS/Linux chains + EuMrx redirector + SSH agent (P10/P11), RAID
  reconstruction (P13), WeChat/SBackup bridges (P16/P17), WinPE media builder
  (P14), ChronoSnap backup product (P02e/P40), AI assistant/recommend/dedup
  layer, monetization surfaces (retention nag, buy-guide tracking window, VIP
  chat, anti-piracy dialog, embedded-IE checkout), 195-event telemetry + Aliyun
  upload + Firebase + DRWReport, holiday skinning/FreezeAnimalDialog, `.qm`
  multi-language matrix, Apower/Cisdem/Foxit OEM variants, "Add Driver" flow.
- **WHAT WE DO:** All omitted by policy; local structured logging only; EN-first
  with string-table discipline; single identity; driver-status surface instead of
  driver-add. Evidence: wizard `docs/UI-MAP.md` §13 (Consciously OMIT 1–13),
  `AUDIT-UI.md` §4; DESIGN-SYSTEM §12 (with per-item corpus cites A01/A21/A34/
  A35/ARCH05).
- **WHY:** scope + licensing + no-dark-patterns policy.
- **RISK:** Feature-set reduction vs og — all enumerated, none silent.
- **VERDICT: ACCEPTED** (every omission carries its corpus cite in UI-MAP §13).

### D-27 — Design-system adaptations (a11y, contrast, grid, motion)

- **WHAT OG DOES:** `outline:none` in QSS (L18) — suppressed focus outlines;
  `#30B373` on white ≈2.7:1 contrast; 24×24 icon buttons; 20,802-line
  QSS-in-stylesheet layout; instant page switches; 12 px font floor; PNG frame
  sequences for scan animation. Evidence: corpus QSS census
  (`UIUX/21-qss-property-census.md`, `05-styling-theming-layout.md`) — all cited
  per token in DESIGN-SYSTEM.
- **WHAT WE DO:** Visible focus rings, WCAG-adjusted status colors, ≥32×32 hit
  targets, 4 px grid, 300 ms transitions + reduce-motion support, WPF resource
  dictionaries keeping the token discipline. Every such change is marked
  `[ADAPTATION]` with its evidence inline. Evidence: wizard
  `docs/DESIGN-SYSTEM.md` (≈30 `[ADAPTATION]` markers, §3–§8).
- **WHY:** modernization/a11y.
- **RISK:** Visual identity differs from og by design.
- **VERDICT: ACCEPTED** (the `[ADAPTATION]` discipline is exactly what this
  registry wants elsewhere).

### D-28 — Preview subsystem: temp-materialization + managed decoders (ADR-8)

- **WHAT OG DOES:** `VAPreviewListener` local-socket protocol (602-byte
  MediaInfo header + ≤1 MiB BMP thumbnail, 10 s bounded wait) to a media-preview
  agent; embedded per-format windows (Image/Video/RtfTxt/Archive);
  `EUOfficeViewer.exe` handoff for office; free-tier caps as pressure: "Files
  larger than 100MB are not available for preview", "15s preview only" (video
  QTimer `timeout()`), "Trial version is preview-only". Evidence: corpus
  `DRWUI-subsystems--assistant-grading-preview.md` §4;
  `UIUX/10-preview-subsystem.md` §2–§3; `UIUX/18-preview-architecture.md`.
- **WHAT WE DO:** Preview workers over content obtained through the sanctioned
  recovery path (temp-materialization via `recover --filter`, logged + hashed),
  managed decoders with hard size/time caps (15 s / 100 MB), every failure
  surfaces a diagnostic; no source-media touching. Evidence: wizard
  `AUDIT-UI.md` ADR-8 (tracked core gap G-10).
- **WHY:** scope (core has no preview API — G-10) + safety (bounded workers).
- **RISK:** Preview fidelity differs from og (no in-panel office rendering until
  the shim/native reader lands); temp-materialization costs destination space.
- **VERDICT: REFRESHED (W5).** Core-side G-10 is CLOSED: `PreviewReader` +
  per-format extractors landed (commit 35f4652) with fuzz coverage, and the
  preview short-window bug class found by CI was fixed with regression tests
  (f93f38e: strict/lenient Window::fetch split, WAV fmt-chunk window, DOCX
  raw-deflate windowBits, malformed-caps advance, 3000-round deterministic
  stress). Remaining actions are wizard-side: reconcile ADR-8's cap rationale
  (worker-safety vs og free-tier pressure, same numbers) and add the
  `UIUX/10-preview-subsystem.md` §3 cite. U-2 stays tracked there.

### D-29 — Filters client-side over the loaded tree (ADR-9)

- **WHAT OG DOES:** FileFilter.xml classification — 8 categories / 1,489
  extensions — applied by `CSetClassifyThread`; advanced filter pipeline
  `CAdvanceFileFilterMgr` → `CSetFilterThread` (7 sub-condition groups) in the
  DRWUI process; **pre-set filters are pushed INTO the scan controller**
  (`CDoScanPartition::PushFolderFilter`) constraining incremental tree insertion.
  Evidence: corpus `architecture/05-scan-engine-architecture.md` §7.4–7.6;
  `UIUX/31-filterbar-family-decode.md`; UI-MAP §6 (filter-bar family).
- **WHAT WE DO:** Core-side query API LANDED (G-12 closed, commit 365c263):
  `cr::query::run` — composable filter (category mask, size/date ranges,
  confidence floor, extension list, substring, deleted-only, origin mask),
  stable sort (name/size/modified/confidence/path), offset+limit paging,
  count-only; name-dependent predicates configured-last so the count path never
  allocates strings; 1M-node filter+count 31–36 ms UNDER ASan. The wizard's
  client-side ResultFilter remains for its own tree projection. Evidence:
  `include/cr/query/cr_query.h`; perf gate in-suite; randomized-vs-brute-force
  reference tests.
- **WHY:** scope originally; now closed core-side. Filter push-down into scan
  insertion (og `PushFolderFilter`) remains absent by design — our query runs
  over the immutable session snapshot.
- **RISK:** Filter push-down absent — large-tree cold filter differs from og's
  constrained incremental insertion; documented, measured fast enough.
- **VERDICT: REFRESHED (ACCEPTED).** Both sides cited; core gap closed; the
  taxonomy-fidelity issue remains split out as D-30 (wizard-side).

### D-30 — File-category taxonomy: 7 categories vs og's 8 — comment over-claims parity

- **WHAT OG DOES:** 8 classify tabs (Audio/Document/Photo/Video/Archives/Emails/
  Databases/Bookmarks) mapping FileFilter.xml's 8 categories / 1,489 extensions;
  filter dropdown adds Others/Unsaved File/Custom. Evidence: corpus
  `05-scan-engine-architecture.md` §7.4; UI-MAP §6 ("Type/classify tree").
- **WHAT WE DO:** `FileCategory` = 7 (Photos/Videos/Documents/Audio/Archives/
  Email/Other) with ~90 hand-listed extensions; code comment claims
  "**og parity**: the category taxonomy". Evidence:
  `src/RdaWizard.Core/Models/ResultNode.cs:129–171`.
- **WHY:** scope (v1 subset) — but the comment mislabels it as parity.
- **RISK:** Category counts and bucketing differ from og (Databases/Bookmarks
  missing; "Photos" vs og "Graphics"; far fewer extensions classified).
- **VERDICT: NEEDS-WORK.** Either (a) generate the category sets from the
  corpus's FileFilter.xml extension lists (1,489 ext) and add the missing
  categories, or (b) keep the v1 subset and fix the comment to "v1 subset of the
  og taxonomy (FileFilter.xml: 8 categories / 1,489 extensions) — Databases and
  Bookmarks fold into Other until v2". Choose deliberately, don't mislabel.

---

## F. Licensing / assets

### D-31 — MIT license; original strings/icons/fonts; no proprietary assets

- **WHAT OG DOES:** ~7,400 RCC images/SVGs, DIN-Black/DINCond-Black OTFs,
  EaseUS logo/first-aid-kit icon/mascot/isometric art, custom `.cur` cursors —
  all proprietary. Evidence: corpus `UIUX/01-visual-inventory.md` (A01 §1–§2,
  cited in DESIGN-SYSTEM §12.1).
- **WHAT WE DO:** MIT in all three repos; v1 icons = Fluent System Icons (MIT)
  for line/chrome + homegrown 2-letter ext tiles (bespoke art v2); "Never
  extract or redraw DRW's shipped SVG/PNG assets"; Segoe UI replaces DIN; all
  strings original. Evidence: wizard `docs/DESIGN-SYSTEM.md` §6.2 ("v1 sourcing
  `[ADAPTATION]`") + §12.1; `AUDIT-UI.md` ADR-11; READMEs ("no proprietary
  strings, icons, or assets are copied").
- **WHY:** licensing.
- **RISK:** None (clean-room discipline).
- **VERDICT: ACCEPTED** (both sides cited; policy documented at the point of use).

### D-32 — No monetization/license gates, no telemetry, no Aliyun/EuShare surfaces

- **WHAT OG DOES:** 12 free/pro gates (UI-MAP), "The free version restores up to
  2GB" family, retention nag every 30–60 s (`RetentionActiveDlg`, A35 §12.15),
  buy-guide window physically tracking the Recover CTA (`sigMoveBuyGuideWnd`,
  A34 §2.3), 195 telemetry events + Aliyun upload + Firebase remote config +
  DRWReport crash uploader, EuShare/onedrive cloud upload paths. Evidence: corpus
  `UIUX/06-dialog-telemetry-inventory.md`, `17-monetization-growth-surfaces.md`,
  `30-interaction-driver-ground-truth.md`, A35 (all cited in UI-MAP §13.8–9 and
  DESIGN-SYSTEM §12).
- **WHAT WE DO:** "monetization gates → none (every recovered byte is real)";
  passive upgrade surfaces only; local structured logging (Serilog) only; no
  cloud upload paths. Evidence: wizard `AUDIT-UI.md` §4 (Replaced list);
  `UI-MAP.md` §13.8–9.
- **WHY:** licensing/policy (no dark patterns; not funnel-first).
- **RISK:** Commercial-model difference — deliberate, product-defining.
- **VERDICT: ACCEPTED** (both sides cited).

---

## G. Benchmarking

### D-33 — Comparison methodology: two independent legs, same methodology; og driven through its real UI

- **WHAT OG DOES (as a measurable product):** DRW.exe UI-driven scanning with
  auto-saved `.rsf` sessions (SaveScan), driver eprdtdrv under native service
  name. Evidence: corpus rsf doc (auto-scan loop), `02-kernel-driver-architecture.md`.
- **WHAT WE DO:** `comparison-ours.yml` (CLI leg) + `comparison-easeus.yml`
  (og leg: DRW.exe driven through its real UI via mouse injection — the
  corpus-proven recipe — with window-title polling, screenshots, `.rsf`
  harvesting and byte-exact-validated extraction) — split at user direction so a
  UI-automation hiccup can never void the CLI leg; same windows-2022 env, same
  seeded 8 GiB GPT VHD via identical `bench_seed.ps1` DEFAULT parameters
  (identical manifests by construction), both raw-disk drivers side by side
  (non-fatal on the leg that doesn't need them), single scorer
  `compare_report.py`, wipe excluded from both denominators, keep-control gate.
  Evidence: core `docs/benchmarking.md` §3; commits c6d32c1, ac881cd; workflows
  `.github/workflows/comparison-{ours,easeus}.yml` (present on disk).
- **WHY:** scope/measurement honesty (per-volume capped scans for unambiguous
  attribution; two-mode regression matrix incl. deleted-partition scenario).
- **RISK:** None; methodology deltas (caps, per-volume split, scenario set) are
  measurement-design choices documented in place.
- **VERDICT: ACCEPTED** (methodology fully documented; og-side driving recipe is
  corpus-derived).

### D-34 — Driver benchmark: cross-driver byte-equality proof

- **WHAT OG DOES:** Ships the reference binaries (WHQL-counter-signed eprdtdrv
  loads on stock Windows). Evidence: driver `docs/benchmarking.md` §2.
- **WHAT WE DO:** Both drivers installed side by side under distinct service
  identities; `rdabench` matrix (open/seq/rand/info/verify/fuzz); SHA-256 per
  1 MiB range cross-driver equality asserted — any mismatch fails the job;
  reference binaries fetched at run time from the corpus, never committed.
  Evidence: driver `docs/benchmarking.md` (whole), `docs/benchmarks/
  2026-10-22-runner-comparison.md`.
- **WHY:** correctness verification (wire parity proven, not assumed).
- **RISK:** None.
- **VERDICT: ACCEPTED** (both sides cited).

---

## H. Error-fix registry integrity (cross-cutting)

### D-35 — F15 (capability report before every scan): registry claim not backed by code

- **WHAT OG DOES:** quick-scan config default ON with no user-visible capability
  report (`GetPrivateProfileIntW(L"search", L"ln", 1, ini)` — defaults ON,
  corpus `SearchDLL/ntfs-engine--deep-dive.md` line 7163 of the decompile).
- **WHAT WE DO (claimed):** "Capability/limits reported by the orchestrator
  (stages, formats, geometry) before every scan; session records persist them"
  — `docs/error-fix-registry.md` F15. **No F15 marker exists in src/ or tests/;
  no capability report found in the scan path** (`rrecov.cpp` scan logs config
  only); the "session records persist them" half is contradicted by G-14
  (VolumeInfo label/geometry/offset are zero).
- **WHY:** would be correctness-fix/scope-honesty if implemented.
- **RISK:** Registry-vs-code drift: a claimed fix that doesn't exist is worse
  than an open defect — it corrupts the audit trail.
- **VERDICT: CLOSED (W3/W5).** The citation drift is fixed: ground-truth
  contracts §3 now carries the precise key (`GetPrivateProfileIntW(L"search",
  L"ln", 1)` — commit b505799, U-11 closed). The claim is reworded to the
  truth: session records persist the capability surface (stage masks +
  VolumeInfo geometry/stages — landed with G-14, which the UI-readiness
  session closed), and the scan start line reports the configured stages to
  `--log` (threads/quick/deep/carve/whole_disk). There is NO stdout capability
  preamble before scans and none is claimed anymore; a user-visible capability
  line belongs to the wizard surface (CORE-CONTRACT §13 telemetry), not the CLI.
  See `docs/error-fix-registry.md` F15 (reworded in the same W5 docs wave).

---

## I. Session-5 parity-refactor waves (D-36..D-44, W1–W4 + parallel-agent waves)

### D-36 — Single-pass quick+deep (ARCH-3): one walk where og runs two generations

- **WHAT OG DOES:** Quick scan = `EUCreate<FS>QuickSearcher`; Advanced = a
  SEPARATE complete-searcher generation (**`FUN_140581500`** (DRWUI:917015-917040)
  is the creation site — I-044 citation correction; the earlier-cited
  `FUN_140580720` (DRWUI:916443+) is the run phase — builds
  `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher` + `EUCreateRAWSearcher`;
  the ZFS leg is `scanzfs`-gated and the shipped product has the key absent
  from config/version.ini, so ZFS recognition is compiled out at runtime);
  quick results carry into Advanced
  (dynamic-ground-truth: "Advanced Scanning 9% … 1.11M files"). The
  quick→deep auto-advance itself is an ENGINE-side state machine
  (`CDoScanPartition::DoScan` pushes states 0/1 unconditionally,
  DRWUI:567875-567892; I-044 decode) — the core's default-on stage model is
  the correct parity translation.
- **WHAT WE DO:** Single-pass-with-deep-settings: the quick stage runs the
  engine pass with deep flags when deep is configured ("single walk" design,
  `src/core/orchestrator/cr_scan.cpp`); the deep stage adds the remnant sweep +
  lost discovery; carve is a third weighted stage. Both passes' semantics are
  preserved via flags + extras.
- **WHY:** performance (half the walking; output-equivalent for the same
  configured surface) — plan ruling ARCH-3/D-3x.
- **RISK:** Output-equivalence holds for our stage model; a wizard UX that
  needs quick-then-deep STAGING sees one combined pass instead. Registered so
  the staging question is explicit, not silent.
- **VERDICT: ACCEPTED** (registered this session; previously an unregistered
  divergence — "a delta not in this registry is a bug").

### D-37 — Transport fallback superset (D-H/FB-T02): grammar-open failure falls back to direct

- **WHAT OG DOES:** Device.mo duality — `\\.\PhysicalDriveN` direct vs
  `\\.\EPRDTDRV` grammar; the og gates the fallback on the RDA probe only.
- **WHAT WE DO:** `open_disk()` falls back to the direct path when the grammar
  open FAILS (broader condition than og's probe-only gate).
- **WHY:** robustness — an unopenable grammar path must not kill a scan of an
  unlocked volume; direct access is sufficient there (plan ruling D-H).
- **RISK:** A volume og would refuse (driver present but grammar broken) is
  scanned direct by us. Bounded: the fallback only fires on open failure, not
  on probe mismatch.
- **VERDICT: ACCEPTED (remark).**

### D-38 — Skip-mode zero-fill retry mechanism (D-I/FB-O03): transport-wide bisection + ReadErrorMap

- **WHAT OG DOES:** Concept is og behavior (EXT4 CatchBuffer zero-fill;
  BadSectorTipsDlg product copy "unreadable sectors are replaced with
  zeros"), but the og mechanism is per-engine and UI-mediated.
- **WHAT WE DO:** Transport-wide bisection retry (`RetryingDevice`) down to
  2× sector granularity, persistent `ReadErrorMap` through stats+session,
  per-item `salvaged` reporting in the writer (W2-18, parallel agent:
  Outcome::Salvaged schema v1).
- **WHY:** correctness + honesty (zero-filled content must be VISIBLE as
  salvaged, not passed off as clean).
- **RISK:** none — strictly more information than og.
- **VERDICT: ACCEPTED.**

### D-39 — Mirrored-FAT salvage (D-E/FAT-2): FAT1 BAD + FAT2 valid chains are salvaged

- **WHAT OG DOES:** CDoubleFATTable consults the mirror per-entry — but drops
  chains whose primary entry is BAD (FAT1 wins on conflict).
- **WHAT WE DO:** `FatTable::next()` consults FAT2 whenever primary says
  BAD/FREE and the mirror is sane — salvage-over-purity (our R2 stance).
- **WHY:** recovery value (a chain og drops is content we recover); evidence:
  FATSearchDLL decompile divergence noted in AUDIT-FAT.
- **RISK:** We may surface chains whose primary-FAT damage is real (contents
  partially stale) — confidence grading carries the risk.
- **VERDICT: ACCEPTED.** FAT12 BAD-cluster canonicalization (D-F/FAT-4: we stop
  the chain where og follows raw 4087) rides the same rationale.

### D-40 — Carved-node naming strings (CARVE-5)

- **WHAT OG DOES:** RAWSearcher naming callback produces og-specific lost-file
  labels; the exact strings are UI-census material, not engine-decoded.
- **WHAT WE DO:** Own naming scheme for carved nodes (extension-based, `carved`
  origin, note strings like "indexless-LFH-chain").
- **WHY:** licensing + honesty (our evidence notes travel with the node).
- **RISK:** cosmetic divergence only.
- **VERDICT: ACCEPTED.**

### D-41 — Report JSON (ours-only, FB-R02-adjacent): machine-readable recovery report

- **WHAT OG DOES:** report is UI-rendered; no JSON report surface located in
  the corpus.
- **WHAT WE DO:** `report::build_report` JSON with per-item outcomes
  (Done/Refused/Error/Salvaged), totals reconciliation, hash verification
  results; CI-validated schema (`tools/validate_report.py`).
- **WHY:** testability + the wizard contract (G-09).
- **RISK:** none.
- **VERDICT: ACCEPTED (ours-only feature, documented).**

### D-42 — ATA-passthrough health delta (HEALTH-2): ATA_PASS_THROUGH vs og's SAT wrapper

- **WHAT OG DOES:** SMART via a SAT wrapper layer (corpus DRWUI health path).
- **WHAT WE DO:** `ATA_PASS_THROUGH_EX` direct with 36-byte-capped sense
  handling; seek-penalty SSD detection vs og's WMI MediaType; documented in
  `include/cr/win/cr_health.h` (W3 header note).
- **WHY:** API availability + explicitness.
- **RISK:** Device coverage may differ on exotic SAT layers.
- **VERDICT: ACCEPTED.**

### D-43 — Cache-coalesced MFT reads (D-D/NTFS-2): 64-KiB coalescing at the cache layer

- **WHAT OG DOES:** 64-KiB bootstrap read buffer feeding the MFT batch walk
  (corpus ntfs deep-dive §2).
- **WHAT WE DO:** Engine reads per-record; the `CachedReader` (8 MiB LRU,
  **256-KiB blocks** since a4053c4 — D-51 registers the block-size tuning;
  the budget+algorithm register is D-72) coalesces — device-read count
  equals the og's bootstrap buffer behavior (verified: read-count parity
  in tests; this row's old "64-KiB blocks" text predates the tuning wave).
- **WHY:** layering (one cache serves every engine, not just NTFS).
- **RISK:** none measured.
- **VERDICT: ACCEPTED** (contract §2/§3 amended accordingly in W5).

### D-44 — Removed dead surface (W4 appendix) + W4-introduced hardening deltas

- **WHAT WE DO (this session, commit 0c6a1f5):** the W4 sweep removed ~420
  net lines of grep-verified dead/duplicate surface across all five audit
  registers — every deletion itemized in that commit body (NTFS INDX family,
  FAT dead helpers, per-engine byte readers → `cr_bits.h`, three private
  time folds → `cr_types.h`, unread scan-flag structs, dead accessors,
  `temp_data.txt`). No behavior change was intended; the three documented
  semantic unifications live in the commit (bootstrap walker 64-attr cap,
  unterminated-tail strictness, `MbrTable` value-4 reservation).
- **HARDENING DELTAS INTRODUCED (deliberate, evidence-backed):**
  - **FAT-8:** the backup-boot fallback now requires a FAT32-validating
    candidate (FAT12/16 have no backup boot; sector 6 there is FAT-area
    bytes). Og has no in-engine fallback at all (RawFixer `CheckBackupDBR`
    is pre-scan) — our fallback itself is the registered delta, now
    spec-scoped. Test: `Fat16HasNoBackupBootFallback`.
  - **lzjb source bound (GAP-P1-2, parallel agent 632edc4):** every source
    read bounded against psize — the kernel port is unchanged for valid
    streams; corrupt blocks return false instead of over-reading. Kernel
    truth deliberately hardened for on-disk input.
  - **Recycle token convention (GAP-P1-3, 632edc4):** $I/$R pairing key is
    the first-dot-stripped segment (real Windows: $R keeps the original
    extension, $I does not) + post-walk remap phase — og-parity behavior,
    registered here because the pairing key is OUR reconstruction (og's
    exact pairing code is not decompiled).
- **WHY:** the mandate — remove invented degradation and dead surface; keep
  and REGISTER proven safety hardening.
- **RISK:** documented per item above.
- **VERDICT: ACCEPTED (appendix).**

---

### D-45 — cr_ui C-ABI shim: the og's in-process UI-engine binding, flat-C adapted (G-01)

- **WHAT OG DOES:** DRWUI.exe binds the engines IN-PROCESS: LoadLibraryW of
  the ten *SearchDLL.dll engines, GetProcAddress of
  `EUCreate<FS>Searcher` / `EUCreate<FS>QuickSearcher` factories returning
  controller interface pointers (`ICompleteFileSystemSearchController`
  family); progress through UI models fed by engine event hooks; a separate
  cooperative abort event. Evidence: `searchdll-family--architecture.md`
  §1 (exports verified in all 9 DLLs), `DRWUI-exe--main-ui-architecture.md`
  §9.2 (decompiled loader FUN_14056c0d0, factory map FUN_140577d40, RAW
  callback registration via vtable+0x48); MOM module system for partition
  ops (`mom-module-system--architecture.md` §1.1). NO CLI subprocess and NO
  IPC on the UI-engine leg (pipes/sockets exist only for BitLocker FVE and
  media preview).
- **WHAT WE DO:** `cr_ui` shared library (include/cr/cr_ui.h, 2a93c45): the
  same factory-binding architecture adapted to a C ABI — opaque handles for
  the controller objects, `cr_ui_*` factories for the EUCreate* role, C
  progress callbacks for the engine hooks, token functions for the abort
  event. Every flow 1:1 with a CLI line (scan/tree/recover/health), so
  CLI-parity remains the tested contract. Zero-init-safe configs (0 =
  default, never "nothing matches" — the plan's ABI rule); u32 codes from
  the SAME cr::Err space (no invented families); two-call size pattern;
  thread-local last_error; catch-all per entry.
- **WHY:** the wizard's ADR-1 hedge ("CLI-subprocess now, native shim
  later") needed exactly this surface; the og's architecture is the
  authority for what the binding layer looks like.
- **RISK:** none claimed beyond the og: the shim is a binding, not an
  engine (no engine logic in src/ui); crash isolation is LOST vs the
  subprocess mode for in-process hosts — documented trade, both modes
  remain available.
- **VERDICT: MATCH (architecture) / EXCEEDS (surface breadth).** The og
  exports per-FS factories; we export one orchestrator-level surface —
  equivalent binding power (the orchestrator already dispatches engines),
  documented as the deliberate adaptation. Validated: 9-test gtest suite +
  plain-C full-lifecycle consumer, plain + ASan green.

---

### D-46 — HFS+ attributes surfaced as named-fork children (S-002 superset)

- **WHAT OG DOES:** the og reads the Attributes B-tree (CNID 8) for
  `com.apple.decmpfs` lookups (HFSPlusSearchDLL FUN_180009c10, string at
  0x18001f480, B-tree-8 key search, first-hand verified this session); whether
  it surfaces NON-decmpfs named attributes as recoverable nodes is not decoded
  (the audit's S-002 open question: "superset is acceptable if registered").
- **WHAT WE DO:** beyond the decmpfs parity implementation (both storage
  forms, codec-extent emission), every inline attribute with a non-empty
  value surfaces as a child node `..namedfork/<name>` carrying its raw value
  bytes as a plain extent (the `..namedfork/rsrc` convention extended).
  Gated by `HfsScanFlags::named_forks` (default on).
- **WHY:** named-fork content is user data on the volume; a recovery product
  that silently drops it loses recoverable bytes. The convention matches the
  resource-fork surface the core already ships.
- **RISK:** none claimed beyond the og: result sets gain nodes the og may not
  list; comparative benches must account for the superset (tag
  named-fork-originated nodes).
- **VERDICT: EXCEEDS (recovery-positive superset), registered per S-002's own
  instruction.**

---

### D-47 — exFAT entry-set checksum strictness: honest-mode LIVE sets only (N-003, scope corrected by M-013)

- **WHAT OG DOES (CORRECTED BY M-013's corpus decode):** the og DOES
  validate entry-set checksums — the dual ROL15 form (raw + InUse-restored,
  FUN_18001eba0 :25153-25198) wired into set completion
  (FUN_18001fef0 :26287-26292) and the parked-set reconstruction oracle
  (FUN_1800203a0). N-003's "no checksum loop anywhere" claim is REFUTED
  (its own session-2 note records the supersession). For DELETED sets the
  og's gate is unconditional (a failed set is dropped, possibly rebuilt by
  the oracle) — the core now matches this exactly (M-013). For LIVE sets
  the og's complete-set path applies per-entry validators WITHOUT a
  checksum test — a live file with a torn SetChecksum grades normally.
- **WHAT WE DO:** the deleted-set dual-form gate + the reconstruction
  oracle are DEFAULT, unconditional og parity (M-013). What
  `ExfatScanFlags::validate_entry_sets` still gates is the LIVE-set
  behavior: FALSE (default) = og parity (torn-checksum live sets grade
  Excellent); TRUE (the honest-recovery profile) = the deliberate
  strictness — live files are NEVER dropped, only downgraded to
  PartialRisk with a sets_damaged counter.
- **WHY:** a torn checksum on a live set is real corruption evidence the
  og ignores; strictness that never removes content is a
  recovery-product improvement. (The deleted-set side is now parity, not
  a divergence: the og itself rejects failed deleted sets.)
- **RISK:** none while disabled (exact og grading); enabled, grades differ
  from the og by design (the profile is explicit at the call site).
- **VERDICT: ACCEPTED (default OFF = og parity on live sets; the
  divergence exists only behind the explicit honest-mode flag; the
  deleted-set gate and oracle are parity, not divergence — M-013).**

---

### D-48 — Audit-acknowledged supersets (diff-to-fix I-003 / I-004 / I-005.4)

Rows registered here had no prior PARITY-DELTAS entry (per the I-batch fix
plans' "check before duplicating" instruction); every rationale was verified
against the corpus evidence this session and ACKNOWLEDGED in the finding
files.

- **D-48a — Btrfs mirror probing + working deep scan (I-003, ACCEPTED).**
  Og: `EUCreateBtrfsComplateSearcher` is a NULL stub and the SB scan aborts
  when the primary's magic fails. Ours: every mirror offset probed
  (kernel `btrfs_sb_offset()`/og .rdata parity), newest generation wins,
  full chunk-map/root-tree deep walk. Pins: `Btrfs.MirrorSelectionNewestGeneration`
  (primary smashed -> 64 MiB mirror recovers). Benches must NOT assume og
  deep-Btrfs output.
- **D-48b — ReFS version-gate superset (I-004, ACCEPTED).** Og: gate
  major==3 && minor>3 (+ a major==1 path). Ours: major 1..3 with full shape
  validation ("wider recovery scope, same structure walk") - ReFS 2.x and
  3.0-3.3 volumes are attempted where the og refuses; parity benches must not
  count their yield against the og.
- **D-48c — 64 KiB direct-I/O envelope (I-005.4, ACCEPTED; GAP-S6-13).**
  Og driver EPMNtDrv has a hard 512-byte sector contract; ours issues
  64 KiB-aligned direct reads with a zero-copy fast path (~3.5x on the
  slow-path shapes per the fix's measurements). A throughput superset, not a
  behavioral one.

---

### D-56 — NTFS pass-2 dump scan: og heap-overrun read + >512b claim-index ambiguity not replicated (S-012)

- **WHAT OG DOES:** the pass-2 GUID-dump record loop scans a chunk with bound
  `record_size × chunk_sectors` (FUN_18001acc0 :21125) while the read filled only
  `chunk_sectors × sector_bytes` — for record_size 0x400 on 512b media the loop walks
  2× past the filled buffer (uninitialized heap parsed as records). The claim-map
  builder writes `min(run_clusters, ceil(size/cluster))` bits at `engine[0x2c] × lcn`
  (FUN_180017190 :18323-410) while the viability probe indexes `engine[0x28] (=spc)
  × lcn` (:21220-22) — consistent only at sector==cluster scale; on multi-sector
  clusters the claim coverage and the probe disagree (under-destroys).
- **WHAT WE DO:** `guid_dump_scan_file` (src/core/ntfs/cr_mftwalk.cpp) scans exactly
  the bytes read (chunk = min(0x8000 sectors × sector_bytes, run remainder)) and the
  claim map is a merged cluster-interval set probed at the run's start cluster —
  internally consistent on every geometry; identical outcomes at 512b sectors.
- **WHY:** correctness-fix (two og defects deliberately not replicated).
- **RISK:** none on standard geometries; on 4Kn-cluster volumes the core destroys
  dump records the og's under-claiming map would keep — the conservative side.
- **VERDICT: ACCEPTED** (S-012 session-4 convergence addendum; corpus re-read
  first-hand; pinned by NtfsVolume.GuidDumpDestroyRules).

### D-57 — NTFS pass-2 membership: dual-source duplicates kept (og parity; finding premise corrected)

- **WHAT OG DOES:** pass-2 dump records carry SYNTHETIC record numbers (chunk sector
  base + offset/record_size, FUN_18001acc0 :21137-38) and the emit path
  (LAB_18001afe6 :21264-21292) has no pass-1 registry lookup — a file present BOTH as
  a deleted live-MFT record AND inside a GUID dump emits TWICE (first-hand verified;
  corrects the S-012 finding's "dedup by record#" premise).
- **WHAT WE DO:** the same — pseudo-number dedup applies WITHIN pass 2 only; no
  cross-pass dedup; downstream merge handles duplicates like the og's consumer side.
- **WHY:** parity (the og's second-source design intentionally surfaces both copies —
  the dump copy may carry the older, recoverable runlist).
- **RISK:** duplicate rows when a dump exists on-volume — exactly what og users see.
- **VERDICT: ACCEPTED** (S-012 session-4 convergence addendum).

## K. core-api session (API surface + driver utilization, 2026-10-26)

Every deliberate divergence this session introduced or closed, with evidence.
Items that are plain MATCHES (self-healing reopen, grammar/token open, F11
compensation, synchronous completion, driver-absent fallback, push/pull) are
NOT registered here — see docs/DRIVER-UTILIZATION.md's checklist.

- **D-50 — the unified 64 KiB read envelope on the DRIVER path too
  (supersedes D-48c's framing).** The og's driver (EPMNtDrv) has no
  alignment rule and forwards raw lengths — its user mode DOES issue
  single-sector (512 B) probe reads through the driver path
  (NTFSSearchDLL_decompiled.c:991, :20706), while its bulk chunking (4/7/2/1
  MiB) is 64 KiB-multiple by construction. Our transport now rounds EVERY
  issued read (both transports, one `io::direct_envelope`) to 64
  KiB-multiple lengths clamped at the device end — probe reads fetch 64 KiB
  instead of 512 B. Rationale: the mandate's non-negotiable property on
  every direct read through the driver; 64 KiB subsumes sector alignment
  (preserving the bench-run-40073093997 sector-rejection fix); the og's
  observable BULK I/O shape is matched exactly. Semantics unchanged
  (byte-granular IStorageDevice contract kept via the bounce path).
  Proven by `DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices`.
- **D-51 — engine cache blocks at 256 KiB (og batches 2-4 MiB).** The og's
  NTFS engine reads the MFT stream in 4096-sector (2 MiB) batches and its
  device layer chunks at 4 MiB; our engine pipe coalesces to 256 KiB
  (8 MiB LRU). 4,096 transitions/GiB vs the og's 256-512 — same class,
  4-8x coarser; chosen to bound random-miss over-fetch (FAT deleted-chain
  walks) while cutting 64 KiB blocks' 16,384/GiB. The alignbench sweep
  (PERFORMANCE-BASELINE §6) owns further tuning.
- **D-52 — carve-format registry as a unified two-layer enumeration.** The
  og exposes RAWSearchDLL registry views (EUGetAnalyserList/Map/Multimap —
  the registry enumeration pair; N-170 re-truth: GetRawUserInfoMap is NOT a
  registry seed, it is the per-scan carve-statistics OUTPUT map per E-B);
  ours exposes one POD
  enumeration over BOTH layers (grammar validators then raw-sign formats)
  with stable append-only ordering. Equivalent host capability, different
  shape; the og's RawConfig.ini per-format toggles are host-side policy on
  both (ABI-CONTRACT §7a).
- **D-53 — the og's carved-file NAMER callback is not an ABI export
  (API-SURFACE-AUDIT op #5, PARTIAL-by-design).** The og registers a
  std::function namer/filter at RAW vt+0x48 at discovery time; our carve
  synthesizes signature-derived names and exposes carve_format/confidence
  per node, so a host re-derives or renames post-query. The extension point
  itself is not exported (a C callback for tree-identity-affecting naming
  would thread into session persistence — not worth the ABI surface for a
  UI policy the reference uses for its own dedup display).
- **D-54 — SQLite session sidecar REJECTED-BY-REFERENCE (closes GAP-S6-12
  as a removal).** ABI-CONTRACT §8 previously documented a sidecar index
  the reference does not have (the og's durable store IS the .rsf
  container; corpus rsf-format--scan-session.md §2-§5) and that we never
  implemented. The false contract claim was cut (A-ABI-2); queries answer
  from the in-memory session tree exactly as shipped.
- **D-55 — category taxonomy stays 6-category/~90-ext vs the og's
  8-category/1,481-ext runtime XML (M-023, open, engine scope).** The API
  surface exposes the core's truthful taxonomy; the completion is tracked
  as M-023 (filter model), not silently over-claimed here.

## L. diff-to-fix closing session (all-tier remediation, 2026-10-26)

Deliberate divergences introduced or pinned while closing the audit roadmap.

### D-58 — ext4 meta_bg descriptor resolution: kernel/e2fsprogs-correct arithmetic, two og defects NOT adopted (S-026)

- **WHAT OG DOES:** seeds its meta-group stride from `s_desc_size`
  (EXT4SearchDLL :3368-3373; asm `divl 0xdf8(%rcx)` at 180004cb9) and reads
  the meta-group descriptor block at `m * blocks_per_group * block_size +
  base` with no `first_data_block` adjustment (:3582-3583, asm `imul` chain
  at 180004d8d-180004d9a). That is correct only when `desc_size^2 ==
  block_size` AND `first_data_block == 0` (the canonical 4KiB/64B-desc
  geometry); it misresolves 2KiB/32B and 4KiB/32B volumes and reads one
  block early on 1KiB-block volumes.
- **WHAT WE DO:** `meta_group_size = block_size / desc_size`
  (EXT2_DESC_PER_BLOCK); descriptor block for meta-group m at
  `first_data_block + (m * meta_group_size) * blocks_per_group` (+1 only for
  m == 0); meta-groups below `s_first_meta_bg` keep the classic front
  location `first_data_block + 1 + m`. Verified empirically this session on
  two real mke2fs 1.47.2 `-O meta_bg` images (1KiB/64B and 4KiB/64B;
  dumpe2fs + hexdump byte-matching descriptor contents).
- **WHY:** correctness — the og's arithmetic silently drops every group
  whose descriptor is not where it looks on non-canonical geometries; same
  defect family as the documented 1KiB front-GDT og-bug (I-014 E9).
- **RISK:** none — the adopted arithmetic is the e2fsprogs
  `ext2fs_descriptor_block_loc2` formula; on canonical 4KiB/64B meta_bg
  volumes both implementations resolve identically (the og's only working
  configuration).
- **VERDICT: ACCEPTED** (pinned by `Ext4Engine.MetaBg*` — 5 tests, 4
  fail-before proven on the engine-reverted build).

### D-59 — ext4 backup-SB rescue: canonical-position grid + exact s_block_group_nr rebasing (S-027)

- **WHAT OG DOES:** its engine-side walker (EXT4SearchDLL FUN_180001b00,
  CEXT4CompleteSearchController vtable slot 53 — the only reference, no
  textual caller) probes caller-supplied sector positions, validates with
  the primary's superblock validator (FUN_180004710), and rebases each
  sub-searcher at `found − 0x400` — a boot-area model that misplaces every
  structure by 1024 bytes for backup copies (group g's copy sits at the
  group's first block with no 1024 boot skip: same defect family as the
  og's 1KiB front-GDT bug, I-014 E9). It also aborts the whole walk on the
  first failed read (its CatchBuffer proxy pre-salvages).
- **WHAT WE DO:** probe a 1 MiB grid (m >= 8; one buffered 8 KiB read per
  window, ~0.8% of the slice, bounded at 4 TiB) covering every canonical
  geometry's group-boundary backup positions (m*1MiB + {0, 1024}); adopt
  the first valid backup; compute the EXACT volume base from the backup's
  own `s_block_group_nr` (SB+0x5A — verified on real mke2fs images:
  primary 0, group-1 copy 1, group-3 copy 3); GDT ladder front-GDT-first
  with the backup GDT (the block after the found copy — e2fsprogs
  "Backup superblock at B, Group descriptors at B+1", full table on
  classic volumes, verified) as fallback. Failed probe reads skip onward
  (the transport retry layer owns bad sectors). Emitted extents are
  base-adjusted so downstream recovery reads land correctly.
- **WHY:** correctness — the og's rebase reads every structure 1024
  bytes off on the rescue path; the grid replaces the og's unpinned
  caller-side sector range with the artifact-documented canonical
  positions (ext4-engine--deep-dive.md SS2: "backup superblock blocks:
  8193, 32768...").
- **RISK:** volume starts not aligned to the 1 MiB grid are covered only
  when the engine is dispatched on a volume-start slice (partition /
  lost-candidate materialization — the og's own sub-searcher dispatch
  shape); cylinder-aligned legacy starts on whole-disk scans fall to the
  carve layer (documented limitation).
- **VERDICT: ACCEPTED** (pinned by `Ext4Engine.Backup*` — 4 tests, 3
  fail-before proven on the engine-reverted build).

## J. UNDOCUMENTED DIVERGENCE CANDIDATES

Things our code/docs do differently with no (or stale/contradictory) rationale
recorded — for the engine/UI parity sub-agents to confirm and promote into
numbered entries (or close) in the final registry:

- **U-1 (feeds D-30):** `FileCategory` 7-category subset with ~90 extensions,
  comment claims "og parity: the category taxonomy" — og is 8 categories /
  1,489 extensions (`05-scan-engine-architecture.md` §7.4). Over-claim.
- **U-2:** AUDIT-UI ADR-8 preview caps (15 s / 100 MB, "matching the og's
  observed caps") vs DESIGN-SYSTEM §12.7 "preview is unlimited for recoverable
  formats" — unreconciled internal contradiction about our own behavior; og's
  caps are free-tier gates, not worker limits (`UIUX/10-preview-subsystem.md` §3).
- **U-3:** ADR-2/§12 tension: DESIGN-SYSTEM §12 "kept: browse-while-scanning"
  vs ADR-2 "results populate at scan completion/stage boundaries" (core G-02
  blocks the former today).
- **U-4 (feeds D-13c):** ZIP EOCD "first-geometry-consistent-wins" selection
  rule and PDF "last-%%EOF bounded by next %PDF-" — og-side selection semantics
  not pinned in corpus (`rawsearchdll--zip-analyser-carving.md` documents
  parse-and-continue + 5.24 MB padding cap only).
- **U-5 (feeds D-13c):** Raw-sign shared-magic family tie-break
  ("highest-confidence sign wins", `cr_carve.cpp:186`) — og tie-break behavior
  unrecorded; rationale lives only in worklog S5-WAVE3-CARVE.
- **U-6 (feeds D-09):** G-18 text stale: "gang blocks counted damaged" —
  `read_gang` implemented post-183dfcd (AUDIT Round 1); CORE-CONTRACT pinned at
  183dfcd predates it. Same staleness class: G-19 "LDM multi-disk members
  unresolved" — cross-disk GUID resolver landed in fffe597 (`rrecov.cpp`
  whole-disk LDM path); re-verify what remains true (RAID-5 single-column
  reconstruction only — likely still true).
- **U-7 (feeds D-12):** Win10 `$I` @0x24 sequence-counter handling — ours follows
  external forensic research; og's own handling of Win10 recycle `$I` files is
  unrecorded in the corpus (check the comparison-easeus leg artifacts for og's
  recycle-bin recall on the Win10-era runner).
- **U-8:** Session-extension drift: wizard `README.md` ("`.rrc` / CRSF") and
  wizard `worklog.md` (".rrc") vs `.crsf` everywhere else (CORE-CONTRACT ×11,
  src ×12, core CLI). One canonical extension, two stray references.
- **U-9:** Wizard README documents `docs/ARCHITECTURE.md`, `docs/TESTING.md`,
  `docs/ROADMAP.md` — none exist yet (docs dir has only the four T2 research
  docs). Doc-map drift, in-flight per worklog.
- **U-10 (feeds D-17):** og `CRecoveryMgr::ExportThread` "writability +
  same-volume detection" (04-recovery-workflow.md §7.3) — undecoded branch; the
  F12 claim ("destination validation is UI-level") may be wrong in detail.
- **U-11 (feeds D-35):** `ground-truth-contracts.md` §3 cites the og ini key as
  `search`/`fastsearch`; corpus decompile shows `GetPrivateProfileIntW(L"search",
  L"ln", 1, …)`. Citation precision fix.
- **U-12 (feeds D-18):** Roadmap v0.2 "mirrors the corpus model: engines
  single-threaded per unit, the host fans units out" — corpus shows engine-side
  worker threads too (FAT `QuickSearchWorker`, threaded quick search); reword or
  cite precisely.
- **U-13 (feeds D-06):** AUDIT row 20's promised btrfs engine-header parity note
  was never added (ZFS got its note; Btrfs didn't).
- **U-14:** WizardStepper state machine implements the ADR-12 rail, but no
  doc records the og-page → our-step mapping for the 4th og page
  (PeripheralHomeView/CDetailPage fold) — trivial, but the final registry should
  carry the mapping so the 3-vs-4 page difference is explicit.

---

## Verdict roll-up

| Group | Entries | ACCEPTED | NEEDS-WORK |
|---|---|---|---|
| A. Format/session | D-01…D-03 | 3 | 0 |
| B. Engine semantics | D-04…D-18 | 12 | 3 → **1** (D-06/D-07 CLOSED W5; D-13c CLOSED W2/W5; D-09 remains) |
| C. Safety | D-19, D-20 | 2 | 0 |
| D. Architecture/build | D-21…D-24 | 4 | 0 |
| E. UI/UX | D-25…D-30 | 4 | 2 → **1** (D-28 core-half closed W5, wizard-half tracked; D-30 remains) |
| F. Licensing/assets | D-31, D-32 | 2 | 0 |
| G. Benchmarking | D-33, D-34 | 2 | 0 |
| H. Registry integrity | D-35 | 0 → **1** (CLOSED W3/W5) | — |
| I. Session-5 waves | D-36…D-44 | 9 | 0 |
| **Total** | **44** | **37 (+3 sub-accepted in D-13)** | **2 (D-09, D-30)** |

**Closing actions — state after the session-5 parity-refactor waves (W1–W5):**

1. **D-06/U-13** — **CLOSED (W5):** btrfs deleted-recovery parity note added to
   `include/cr/btrfs/cr_btrfs_engine.h`.
2. **D-07** — **CLOSED (W2-10/W5):** og jbd2 pinned — NO revoke semantics
   engine-side; our walk-all now matches (E-2, ruling D-G).
3. **D-09/U-6** — **OPEN (capability gap, ruling D-L):** ZFS multi-vdev
   topology registered as an honest gap in CAPABILITY-MATRIX (single-vdev
   pools are full-parity; og multi-vdev extent is corpus-UNVERIFIED).
4. **D-13c/U-4/U-5** — **CLOSED (W2/W5):** ZIP 3-anchor + CD-ownership +
   padding extension implemented to the corpus contract with
   decompile-derived tests; narrow remainder tracked inside the entry.
5. **D-17/U-10** — **OPEN (evidence only):** our hard refusal stands (safety);
   ExportThread same-volume decode remains UNVERIFIED-og; F11/F12 wording
   corrected W2.
6. **D-28/U-2** — **CORE HALF CLOSED (W5):** G-10 preview landed + window-bug
   class fixed; ADR-8 rationale reconcile is wizard-side, U-2 tracked there.
7. **D-30/U-1** — **OPEN (wizard-side):** FileCategory over-claim — subset or
   extend to FileFilter.xml's 8/1,489 (also feeds the corpus errata: the
   1,481 recount).
8. **D-35/U-11** — **CLOSED (W3/W5):** `ln` citation precise; F15 reworded to
   the truth (session-persisted capability + log line; no stdout preamble
   claimed).
9. **U-8/U-9/U-12** — **W5 core-half:** roadmap threading claim reworded (W3);
   `.rrc`→`.crsf` + phantom-docs references remain wizard-side actions.

*Prepared by parity sub-agent 2-e; W1–W5 refresh by the principal-engineer
parity-refactor session. Cross-check against the engine-semantics,
orchestration/CLI, UI, and architecture parity reports before promoting to
`docs/PARITY-DELTAS.md`.*

### D-49 — exFAT timestamps: on-disk time carried as-UTC, tz bytes not applied (S-011 parity)

**Class:** parity-over-spec (engine semantics) · **Finding:** diff-to-fix S-011 (SEVERE, FIXED) · **Date:** 2026-10-26

exFAT stores each file's create/modify/access timestamps as a local time on
disk PLUS a per-timestamp `UtcOffset` byte (7-bit two's-complement
quarter-hours). The exFAT spec (ELM-Chan, Standard File Entry) says a reader
SHOULD apply the offset to get true UTC. The og's `exFATSearchDLL` does not:
it passes the raw date/time words to `DosDateTimeToFileTime` and never reads
the tz bytes (verified first-hand: zero TZ-conversion imports beyond
`DosDateTimeToFileTime` itself; six call sites at
`exFATSearchDLL_decompiled.c:5215-5235` / `:45174-45194` taking
`(date_word, time_word, &ft)` only; zero `*900` quarter-hour arithmetic in
the whole DLL). DosDateTimeToFileTime performs NO timezone
conversion — it interprets the MS-DOS wall clock as UTC (Wine conformance tests
assert `DosDateTimeToFileTime(1980-01-01, 00:00)` == `SystemTimeToFileTime` of
the same wall clock; ReactOS/Wine implementations are TZ-free; MS "File Times"
docs place FAT local-to-UTC conversion in the FS driver / `FindFirstFile`,
which the og bypasses by parsing raw dentries). The og's resulting FILETIME is
therefore byte-deterministic and HOST-INDEPENDENT — the on-disk local wall
clock re-interpreted as UTC on every host. (Correction 2026-10-26, diff-fix
verification pass: this record and the S-011 finding's Impact section
originally claimed host-dependence — refuted by the API-semantics evidence
above. Host TZ only enters at DRWUI display time, FileTimeToLocalFileTime,
DRWUI_decompiled.c:412990-413010 / :368323-368329.)

**Core decision (S-011 fix):** the core's `exfat_time_to_unix100` does NOT
apply the tz bytes either. It carries the on-disk time through the shared
FAT-family datetime core (`fat_datetime_to_unix_seconds`), host-independent,
on-disk-time-as-UTC — exactly what the core's FAT engine already does for
the same wire format (and holds a MATCH verdict for). This matches the og
byte-for-byte on EVERY host (both sides are host-independent local-as-UTC). The spec-correct UTC
interpretation was REMOVED by this fix (it diverged from the og's .rsf
bytes on every non-UTC file by exactly `tz_offset × 900 × 10⁷` 100ns
units). The wire tz fields remain parsed for completeness
(`cr_exfat_wire.h` carries the note).

Considered and deferred: a `--strict-utc` opt-in (the finding's open
question). The engine layer has no flag surface for it; a CLI/ABI grammar
addition for a mode no og comparison consumes is scope creep; this delta
record is the semantic cover. Revisit only if a consumer asks for
spec-correct timestamps.

**Pin tests:** `ExfatEngine.TimestampsMatchOgLocalAsUtc` (tz = 0x88),
`ExfatEngine.TimestampsZeroTzPassThrough` (tz = 0x00),
`ExfatEngine.TimestampsMatchExplorerDisplay` (tz = 0x20).

### D-51 — NTFS two-pass scan model (S-012, ACCEPTED as og parity)

- **WHAT OG DOES:** pass 1 walks the live $MFT and emits EVERY record (deleted
  included) with no viability destroy; a cluster claim map accumulates each
  emitted record's extents (files $DATA, dirs $I30; live AND deleted). Pass 2
  walks the $DATA of live `{*}{3808876b-c176-4e48-b7ae-04046e6cc752}`-named
  MFT-dump files as record buffers (stride 0x400/512) and only THERE applies
  destroy: `size <= 0x3ff`, all-run-STARTs claimed, `$`-prefix/system/unnamed;
  folders are exempt. Deleted size-0 non-dir records take a rescue detour
  ($MFTMirr resident harvest + same-name runlist adoption). Grading happens
  downstream, never in the walk (OQ-S2M-1).
- **WHAT WE DO (post-S-012):** exactly that architecture. `WalkerFlags::
  destroy_fully_overwritten` now defaults FALSE (the S-001 in-walk destroy was
  the mis-scope; TRUE remains as the opt-in honest-recovery inversion).
  `guid_dump_pass` and `size_zero_rescue` default ON. The in-walk
  `Confidence` grading is retained as the documented D15 superset (emitted
  membership matches; grades are additive).
- **NOTES:** claim map is cluster-granular (og: sector-granular bitmap; probe
  outcomes identical for cluster-aligned run starts). Dump records dedup by
  the pseudo record number (the og's exact lVar18 arithmetic:
  in-chunk-offset/record_size + chunk byte base, :21150-51) within pass 2
  and resolve parents against pass-1 emissions (unresolved -> lost root).
  The og's in-walk strict same-name merge is not replicated (its candidate
  map is empty at rescue time by construction); the relaxed post-walk pass
  is the dominant og path.
- **CORRECTION (diff-fix session 4, verified line-by-line):** the dump-walk
  STRIDE is the record size on every path — uVar8 after holes
  (LAB_18001b0cb :21293) and after a successful emit (:21151); uVar16 =
  max(0x400, rs) only after props-fail/skip/destroy (:21169/:21259),
  which coincides with the record size through the boot clamp. The first
  landing advanced holes by one sector (examining positions the og never
  visits) and bounded parse at max(sector, 0x400) (truncating 2/4 KiB
  records). Also N-040 (fixed the same commit): the pass-1 attribute
  filter is the og's (og-15 name table) AND ($SI SYSTEM+HIDDEN) gate -
  FUN_180040f20's table + FUN_180040ea0's $SI dword check - with NO
  $FILE_NAME-flags system filter anywhere in pass 1; the old
  `dos_flags & 4` gate is removed (the known-name list subsumes the og-15
  table; real metafiles always carry $SI SYSTEM|HIDDEN). Pinned by
  NtfsVolume.{GuidDumpStrideIsRecordAligned, GuidDumpLargeRecordParsedFully,
  MainWalkKeepsSystemFlaggedUserFile} on top of the original five pins.
- **STATUS:** ACCEPTED (og parity; diff-to-fix S-012 FIXED).

### D-93 — HFS+ journal differential harvest: the og's primary deleted surface (S-021/S-003) (renumbered from a same-window D-56 collision - the errata convention)

**Class:** parity (mechanism) + deliberate hardening · **Findings:** diff-to-fix S-021 (SEVERE, FIXED) + S-003 (SEVERE, FIXED — outcome parity; "replay" premise corrected) · **Date:** 2026-10-26

The og's HFS+ journal consumer is a differential deleted-record harvester
(CHFSPlusJournal::GetDeleteRecord, FUN_180011300 :13637-14252), NOT a
live-tree replay: journal transaction images install into a scratch
record, harvested catalog records emit into the kind-5 deleted branch, and
the live catalog walk runs unchanged afterwards (:10122). The core now
implements the same model (`journal_harvest_pass`): JournalInfoBlock →
jnl_header (LE, jnl.c) gates → ring walk with split reads → leaf-only
images classified by fork ranges → live-catalog differential (kept iff
not-found or fileID differs) → deleted-tier emission with extents-branch
fallbacks.

Deliberate differences (all hardening, none affect result membership):
- **Ring-walk bound:** one full ring of advances maximum — the og's loop
  terminates only on `offset == jhdr.start` or cancel and can cycle
  forever on a corrupt ring (observed defect, documented in the S-021
  evidence log).
- **Size caps:** journal ≤ 512 MiB, images/transaction sizes ≤ 16 MiB —
  the og allocates what the header claims.
- **Degradation:** a damaged journal (bad magic/endian/size cross-check)
  degrades to a no-harvest, never a scan failure.
- **Grading:** harvested records carry our display-tier bitmap grading
  (Excellent/PartialRisk/Overwritten) — the og emits grade 5 and grades
  downstream (OQ-S2M-1); same superset family as the freed-node sweep.
- **The transaction-magic constant** `(u32@jhdr+0x20 >> 4) − 1` is
  mechanically decoded (first-hand re-read this session) but its
  derivation is corpus-undecoded; fixtures encode the same formula. A
  wrong constant yields a silent zero-harvest (never wrong results) — the
  risk note stays in OQ-S2M-1's probe list.

**WHY:** correctness — the journal is the only place recent deletions
remain visible on HFS+ (the live catalog overwrites delete records in
place).
**RISK:** none beyond the tracked constant question.
**VERDICT: ACCEPTED** (decompile cited; fail-before/pass-after in
`diff-to-fix/evidence/executions/s021-failbefore-journal.md`; 423/423 ASan).

### D-50 — Lost-partition superblock probes: ext parity + HFS+/Btrfs/ZFS/ReFS superset anchors (S-006)

**Class:** parity (ext) + deliberate superset (HFS+/Btrfs/ZFS/ReFS) · **Finding:** diff-to-fix S-006 (SEVERE, FIXED) · **Date:** 2026-10-26

The og's PartitionRecoveryD deep scan wires exactly two parser families beyond
the Win32 set (ctor decodes: CBlockParseWin32 = {CFatxBlockParser,
CNTFSBlockParser, CExFatBlockParser, BitlockerParser}; CBlockParserLinux =
{CExtXBlockParser}). It has NO lost-scan parsers for HFS+, Btrfs, ZFS or ReFS.

**Core decision (S-006 fix):**
- **ext2/3/4 = parity.** `probe_ext` reproduces the og's decoded accept gates
  (magic 0xEF53 @SB+0x38, nonzero inode/block/group fields, s_first_ino>=3,
  the two-formula group-count cross-check, the first_data_block geometry rule,
  and s_block_group_nr==0 — only block-group-0 superblocks anchor a partition
  start, so the per-block-group SB copies do not flood candidates), plus a
  group-descriptor liveness probe. Size is SB-derived (real extent).
- **HFS+/Btrfs/ZFS/ReFS = documented superset.** The core's engines exist for
  all four and the materialize->probe_family->engine dispatch predates this
  change; the discovery anchors reuse the engines' own validators
  (hfsplus::validate_volume_header, btrfs::validate_superblock, the ZFS
  label-0 uberblock anchor, refs::detect_refs on a bounded 8 MiB window).
  ReFS is first-class per the audit mandate. This is the same superset family
  as the acknowledged I-003 (Btrfs deep scan).
- **Cost-bounding gate:** the two out-of-window anchors (Btrfs +64 KiB, ZFS
  +128 KiB) run only at 1 MiB grid alignment (the standard partition-start
  alignment; ~2 bounded reads per MiB). A lost Linux/Mac volume whose start
  is NOT 1 MiB-aligned is missed by these two superset probes (ext/HFS+ are
  stride-1 in-window and unconditioned). The alignment was chosen over
  stride-1 out-of-window reads to keep discovery throughput uncompromised;
  the og's own deep-scan grid constants remain corpus-undecoded (S-006
  remainder notes).
- **ZFS/Btrfs/ReFS candidate sizes** carry the flagged `size_from_device_end`
  fallback (label/MSB geometry is engine-side); the GAP-S6-6 dedup rule keeps
  fallback-sized candidates from claiming containment over peers.

**Scoped remainder:** the og's CFatxBlockParser (Xbox FATX) is not implemented
— its accept criteria are not corpus-decoded (FUN_180006250 ->
FUN_180004070/FUN_180004610 chain entered but unresolved). No invented gates.

**Pin tests:** `LostDiscovery.Ext4SuperblockAnchorsDeletedPartition`,
`Ext4BlockGroupCopyNotACandidateStart`, `HfsPlusVolumeHeaderAnchorsDeletedPartition`,
`BtrfsSuperblockAnchorsDeletedPartition`, `ZfsLabelAnchorsDeletedPartition`,
`RefsBootAnchorsDeletedPartition`, `SuperblockNoiseRejected`.

### D-94 — FAT/exFAT engine-open + lost-scan gates: og parity with three registered core deltas (N-010b/N-010c/N-022/N-034) (renumbered from a same-window D-56 collision - the errata convention)

**Class:** parity + registered superset (engine semantics) · **Findings:** diff-to-fix N-010a..f (N-023 group), N-022, N-034 (MINOR, FIXED 2026-10-26; renumbered D-51 -> D-56 to clear the numbering collision with the NTFS two-pass entry and the section-K bullet family) · **Date:** 2026-10-26

The FAT batch aligns the core's boot/dentry gates with the og's two-layer
architecture, verified first-hand this session:

- **Engine open** (`fat::boot_passes_open_gates`, mirrors FATSearchDLL
  `FUN_18002f970` + the quick-search 0xE bail at `FUN_180021000:26318`):
  FS-name string ("FAT16   " +0x36 / "FAT32   " +0x52, `_strnicmp`
  case-insensitive), numFATs ∈ {1,2}, bps == device sector size, FAT16
  root_entries ≥ 224 + root×32 % bps == 0, spf ≠ 0. A failing volume
  declines the engine (routes to other engines / raw carving) exactly like
  the og.
- **Lost scan** (`fat::lost_boot_flavor`, mirrors PartitionRecoveryD
  `FUN_180004610` + gates `FUN_1800048f0/4a40/4b70`): 512-multiple bps,
  og cluster-count arithmetic (total32-preferred, floor root sectors),
  bracket-ordered gate walk with the FAT12 small-FAT probe (spf16 ∈ [1,200])
  and the case-SENSITIVE "FAT32   " memcmp.
- **Flavor boundary**: 65524 data clusters is FAT32 (og `FUN_180007e50`
  brackets [0xFF5, 0xFFF4) / [0xFFF4, …)); the core's old `< 65525` read
  the boundary volume as FAT16.
- **exFAT stream reserved fields** (N-010e): the og's byte-OR form
  (`FUN_180003090:1843-1846`, single-byte loads) — reserved2 passes when
  either byte is 0, reserved3 when any byte is 0, reserved1 strictly 0.

**Registered core deltas (preserved deliberately):**

1. **FAT12 engine-open exemption.** The og's engine-open validator has NO
   FAT12 branch — its quick engine declines every FAT12 volume (flavor 0xE);
   only the deep scan generates FAT12 candidates. The core keeps structural
   FAT12 recovery at engine open (the I-002-family superset): FAT12 volumes
   pass the geometry gates (numFATs/bps) but no string gate (there is no og
   string gate to mirror). Future sessions must NOT "fix" this by declining
   FAT12 — that would delete a documented recovery surface.
2. **Date-sanity gate default OFF (N-010c).** The og's dentry date-sanity
   rejection (`FUN_1800365a0:44298-44315`) is config-gated (singleton
   +0x40); its default runtime value is corpus-undecoded. The core exposes
   `FatScanFlags::strict_date_sanity` (default OFF, preserving the
   N-021-pinned zero-FileTime emission flow that the og's
   `DosDateTimeToFileTime` zero-out at :32135 proves happens for
   gate-passing dentries).
3. **Mirror-BAD salvage opt-in (N-022).** Default path is og parity
   (mirror consulted on primary FREE only — `CDoubleFATTable::GetNext`
   `FUN_180041de0:54216-18`); `FatScanFlags::mirror_bad_salvage` keeps the
   old torn-pair salvage as an explicit recovery mode, never the default.
4. **Mid-chain FREE/BAD content shape (N-010f, registered no-chase).** The
   og's directory walk continues at cluster+1 on mid-chain FREE only when
   the range is in the claim/bitmap RB-trees (`FUN_18000f950:11075-11116`)
   and aborts the whole directory read on canonical BAD
   (`FUN_18000fb80:11231-11243`); the core keeps pre-BAD clusters and marks
   the chain broken. Informational-grade difference on damaged volumes;
   registered, not chased (the finding's own recommendation).
5. **Underflow guard in the lost ladder.** The og computes the deep-scan
   cluster count in wrapping uint arithmetic (an underflowing corrupt BPB
   wraps to ~4 G and lands in the FAT32 bracket by accident); the core
   rejects underflowing BPBs outright — same outcome for every sane input,
   no wraparound path.

**Correction to the audit (recorded):** N-010b/N-034's "we accept any pow2
SPC incl. 256+" sub-claim is a false positive — SPC is a u8 BPB field, so
the old any-pow2 check already accepted exactly {1,2,4,8,16,32,64,128} =
the og's set over the representable domain. The explicit-set code and the
`FatEngine.SpcDomainMatchesOgSet` domain-walk pin (all 256 values) keep the
gate stated and pinned. N-010d (exFAT tz) was already fixed by S-011
(D-49); N-023's resolution marks it FIXED-ALREADY.

**Pin tests:** `FatEngine.FlavorBoundary65524IsFat32`,
`FatEngine.BootStringGateDeclinesTornStringVolume`,
`FatEngine.BootStringCaseInsensitiveAccepted`,
`FatEngine.Fat16LowRootEntriesDeclined`,
`FatEngine.Fat16RootEntriesNotSectorMultipleDeclined`,
`FatEngine.NumFatsThreeDeclined`, `FatEngine.SpcDomainMatchesOgSet`,
`FatEngine.DateSanityRejectedInStrictMode`,
`FatEngine.BadPrimaryServedWithoutMirrorConsult`,
`FatEngine.PrimaryFreeStillConsultsMirror`,
`LostFatGates.*` (7 tests), `LostDiscovery.FatBootWithOgGatesBecomesCandidate`,
`ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr`,
`ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged`,
`ExfatEngine.StreamReserved1ByteStrictlyZero`.

### D-52 — NTFS boot-ladder, attribute source, USN gate: og parity with registered deltas (N-011/N-012/N-024/N-039)

**Class:** parity + registered superset (engine semantics) · **Findings:** diff-to-fix N-011, N-012, N-024 (N-011a..e), N-039 (MINOR, FIXED 2026-10-26) · **Date:** 2026-10-26

- **Boot ladder (N-011b):** volume open now mirrors NTFSLib
  `CNtfsVolume::LoadNtfsVolume` :19292-19344 exactly: bps 512-multiple
  (pow2 dropped; 256 now rejected like the og), spc signed-byte decode
  (direct < 0x81 including non-pow2, shift form above), cpr==0 / 0-result /
  sub-sector record REJECTED (the M-001 fallback ladder belongs to the walk
  function), index-block byte 0 rejected, no 0x400 clamp at open, no 0xAA55
  check. **Registered core hardening kept** (the og lacks all four): the
  4096 bps cap (spec max), the P1-4 shift-UB exponent guard (1 MiB record
  cap — the og computes a 1 GiB record for cpr=-30), the P1-3 declared-size
  sanity, and the bps-vs-device-sector match with the 512e escape.
- **Attributes source (N-012):** serialized from `$STANDARD_INFORMATION`
  dos_flags (:5351-5368, per-bit RO/hidden/system/archive + 0x400 OFFLINE),
  `$FILE_NAME` fallback when `$SI` is absent, mask widened 0xFF → 0x7FF.
- **Short name (N-011a):** new `FileProperties::short_name` (the og's
  GetShortName* field), captured from a distinct DOS-namespace `$FILE_NAME`,
  persisted as session record nflags bit 2 (versioned optional — old readers
  skip, old files read back empty).
- **USN harvest (N-039):** the 0x20000 FILE_DELETE gate stays (the og's
  0x200 mask is COMPRESSION_CHANGE — an og defect, do NOT copy); >64 MiB
  `$J` streams now mine the TAIL (circular journal — newest records last).
  The og's 512-byte record cap and V2-only parsing stay core supersets
  (long names and V3 records parse natively here).
- **Registered supersets (N-011c/d):** the og pass-2 0x400/512 stride quirk
  (512-byte-record MFT dumps skip every other record) is NOT replicated;
  the core's uniform stride is the correct superset. The core's namespace
  ranking (POSIX > WIN32_AND_DOS > WIN32 > DOS) vs the og's pick-one is a
  deliberate recovery improvement.
- **ADS (N-011):** `WalkerFlags::emit_ads` (default ON superset, OFF = og
  parity drop) landed with 367561d; this session adds the missing parity
  pin (`NtfsVolume.AdsDroppedInParityMode`).

**Pin tests:** `NtfsVolume.AttributesFromSIWhenAvailable`,
`AttributesFallBackToFileNameWhenSIAbsent`, `AttributesPreserveOfflineBit`,
`ShortNameFieldRoundtrips` (incl. session roundtrip), `BootRecordSizeZeroRejected`,
`BootNoPow2RecordSizeAcceptedAtDecodedValue`, `BootNegativeExponentOverflowRejected`,
`AdsDroppedInParityMode`, `AdsEmittedByDefaultSuperset`,
`UsnHarvestTrueDeleteReason0x80020000`, `UsnHarvestRejectsCompressionChangeOnly`,
`UsnHarvestCapMinesTail`.

### D-53 — HFS+ sink filter, fork selection, prdf folderization, block-size bound (N-028/N-038/N-005)

**Class:** parity + registered superset (engine semantics) · **Findings:** diff-to-fix N-028 (N-015a/b/c), N-038, N-005 (MINOR, FIXED 2026-10-26) · **Date:** 2026-10-26

- **Sink filter (N-015a):** the og's result sink (FUN_1800057d0 :3714-3718)
  destroys dot-prefixed names and "HFS+ Private Data" before emission. The
  core filters both at the live/deleted walk AND the freed-node orphan sweep
  (the og's single result callback covers every source).
- **Fork selection (N-038):** record ownerFlags (+0x29) bit 0x20 selects the
  resource fork as the node's content and marks the decmpfs completion path
  (FUN_180009940 :7665/:7677-78) — implemented in both record sources; the
  empty-data-fork decmpfs substitution stays as the fallback predicate for
  bit-clear records. The documented-field NAME of +0x29 stays unresolved
  (ownerFlags vs UF_COMPRESSED per the finding's open question); the
  mechanical predicate is decompile-quoted and now implemented.
- **'prdf' folderization (N-015c):** FinderInfo fileType (+0x30) "prdf" →
  the record emits as a folder, forks not parsed (:7687-89).
- **Block size (N-005):** upper bound relaxed 64 KiB → 16 MiB (the og has no
  upper bound beyond the 512-multiple mask; large-allocation-block volumes
  open on the og). **Registered hardening kept:** the ≥ 512 floor
  (blockSize 0 is geometrically unusable) and a NEW device-size product
  sanity (block × total ≤ 2× device — the og lacks it; torn VH pairs would
  feed absurd offsets).
- **Registered superset kept (N-015b):** `..namedfork/rsrc` child nodes for
  unmarked files remain a core superset (the og reads the data fork only
  outside the UF_COMPRESSED branch) — parity benches filter rsrc children.

**Pin tests:** `HfsPlus.DotfileAndPrivateDataFiltered`, `PrdfRecordFolderized`,
`RecordBit29SelectsResourceFork`, `LargeBlockSizeAccepted`.

### D-54 — Orchestrator/health/writer quick wins (N-016/N-017/N-009a/N-009b)

**Class:** parity/quality · **Findings:** diff-to-fix N-016, N-017, N-009a, N-009b (MINOR, FIXED 2026-10-26) · **Date:** 2026-10-26

- **N-016:** the run()-exit stamp's `emit_boundary_events_` double call
  (56cce87 merge artifact) removed — FilesFound fires once per boundary
  (pinned: exactly 3 for the quick+deep fixture).
- **N-017:** the carve-style 50 ms cancel watcher is factored into
  `cr::CancelPoolWatcher` (cr_parallel.h) and wired at BOTH fan-outs (carve
  windows + the orchestrator's parallel candidate scan) — a queued
  candidate/work item never starts after a cancel; running jobs finish to
  their own poll point. The carve behavior is unchanged (same semantics,
  one implementation).
- **N-009a:** NVMe `percentage_used` read at u8 width (spec; the reserved
  byte at +6 no longer leaks into the value).
- **N-009b:** writer `done_bytes` counts only non-Error outcomes (progress
  honesty — the og's ExportThread counts written bytes).

**Pin tests:** `ScanEvents.SingleBoundaryAtTeardown` (count == 3),
`Pool.CancelPoolWatcherDrainsQueueWithinOneTick`, `Pool.CancelPoolWatcherNullTokenIsInert`,
`NvmeHealth.PercentageUsedIsU8Width`, `WriterDirs.ProgressExcludesErrorItems`.

### D-91 — HFS+ journal deleted-record harvest (S-021) + the record-span convention fix (A-005) (renumbered from a same-window D-52 collision - the errata convention)

**Session:** diff-to-fix remediation (S-021 + A-005). **Corpus:**
HFSPlusSearchDLL CHFSPlusJournal (`hfsplusjournal.cpp` trace strings)
:13637-14252 + FUN_18000c8f0 :10017-10127 + FUN_180010720 :13049-13108.

- **A-005 (correctness, og-exact):** the HFS+ B-tree record-span convention
  corrected to the on-disk format (TN1150, and the og's executable journal-image
  validator): records pack UP from offset 14, [end-2] = record 0's offset = 14,
  deeper slots ascend, deepest = free-space start. Record i =
  [slot(i+1), slot(i+2)). The pre-fix core (and its fixtures) were inverted —
  on real volumes only record 0 per node was emitted.
- **Journal harvest (og-exact mechanics):** JIB gates (flags bit 0; base/size;
  the VH journaled flag 0x2000), jhdr gates ('JNLx' LE magic, endian, size
  cross-check), the ring walk (u16 header test `(blhdr_size>>4)-1`; block lists
  + serial images with wrap; termination at the walk origin), the og's
  node-image validator, leaf-only harvest, fork classification by VOLUME
  address ranges, the catalog differential (keep absent-key or different-fileID
  records; skip cnid == parentCNID), extents side map for empty-fork records.
- **Deliberate divergences:**
  - **Corruption step-cap** on the ring walk (journal_size/512 + 16 probes): the
    og relies on the ring's own termination; a hostile/edited header could loop
    the og forever. Bounded here — the harvest degrades to a partial ring, never
    hangs.
  - **Differential via the collected live keys** instead of per-record live-tree
    searches: identical result set, one pass instead of O(records × log tree).
  - **Freed-node sweep dedup against harvested cnids** (the sweep is a core
    superset — I-014; both surfaces finding the same record emit once).
  - `HfsScanFlags::journal_harvest = false` off switch; harvested records with
    content carry `Confidence::Unknown` (the og emits ungraded).
  - Attributes/allocation/startup journal images are walked (validated,
    counted) but not harvested — the og uses their branches for other
    consumers; the deleted-record surface is catalog+extents only.

**Pin tests:** `HfsPlus.JournalHarvestEmitsDeletedRecords`,
`HfsPlus.JournalWrapHandled`, `HfsPlus.NonLeafImagesSkipped`,
`HfsPlus.JournalHarvestDisabledByFlag`, `HfsPlus.LeafNodeLayoutMatchesOnDiskFormat`
(the A-005 convention guard).

### D-96 — Audit-acknowledged supersets and registers, session-7 INFO tier (I-006..I-020 batch) (renumbered from a same-window D-59 collision - the errata convention)

Rows registered here had no prior PARITY-DELTAS entry; every rationale was
re-verified against corpus `03ad46c` and the running core this session
before acknowledgment. Future work must PRESERVE every row in this entry
unless the row itself is first re-audited against the corpus.

- **D-59a — ext4 nanosecond timestamps (I-006, ACCEPTED).** Og
  EXT4SearchDLL is second-only (`_gmtime32_s` + `wMilliseconds = 0` at
  :5494-5515; 3rd/4th FILETIME slots zeroed at :6255-6258). Core preserves
  nsec at 100ns (`decode_extra` + `unix_to_filetime_ns`). Bench comparisons
  diverge mechanically on sub-second units — expected, not a regression.
- **D-59b — 4-of-4 timestamps on ext4/HFS+/Btrfs (I-007, ACCEPTED).** Og
  reads 2/4 (ext4), 3/4 (HFS+ — no access_date), 3/4 (Btrfs — otime never
  converted; `+0x94` zeroed only). Core reads all four on every engine,
  including both HFS+ record sources (catalog walk + S-021 journal
  emission).
- **D-59c — carve byte-granular probing (I-009, ACCEPTED + pinned).** Og
  steps whole sectors (RAWSearchDLL :9496); core byte-steps
  (scan_one_window). A header starting mid-sector is invisible to the og.
  Pin: `CarveEngine.CarvesMidSectorStart` (PNG at sector+137 must carve).
  The M-025 read-model rework must not adopt sector stepping.
  I-031 clause (the S11 infra walk): the og's engine setup additionally
  REFUSES sector sizes not a multiple of 512 outright
  (FUN_18000c430 :9265-9268 — no carve at all on e.g. 520-byte-sector
  devices); the core is byte-granular and carves regardless — part of the
  same registered geometry superset, recorded so the refusal is not
  re-imported as "missing" behavior.
- **D-59d — LDM RAID-5 dormant rotations (I-008, ACCEPTED).** Og's
  selector (+0x2a8) is constructor-hard-zeroed; only left-symmetric is
  constructible. Core implements exactly the enabled mode (algebraic
  identity verified). Do NOT implement the dead rotations 1/2/3.
- **D-59e — scope/superset registers (I-010, ACCEPTED).** ZFS label-field
  superset (name/ashift/asize/offset/guid vs og vdev_tree/id only); ext4
  BIGALLOC refusal (safety choice, og has no gate); ReFS one-record shape
  confirmed post-S-020 (no ADS children — matches og's structural no-ADS).
- **D-59f — ReFS host gate (I-013, ACCEPTED).** Og refuses hosts < Win8
  (GetVersion, FUN_1800010d0); core is host-agnostic (the only
  "GetVersion" in the tree is the RDA-driver IOCTL). Non-Windows bench
  lanes are not og-divergent by this row.
- **D-59g — ext4/HFS+ superset registers (I-014, ACCEPTED).** ext4
  deleted-inode dtime sweep default-on (no og counterpart); fragmented
  journal walked via the extent map (the og's physical+1 walk is an og
  defect — do not replicate); HFS+ bitmap-grading rows consumed by the
  S-021 landing (D-56).
- **D-59h — preview architecture (I-015, ACCEPTED).** Og = UI-layer
  socket/thumbnail protocol (602-byte header, 1 MiB BMP, 10 s wait, ports
  20171/20172, 100 MB cap, 15 s free-tier video); core = in-process
  bounded reader (4 MiB read / 256 MiB decode). Different layers; parity
  tests must not byte-compare payloads across them.
- **D-59i — engine factory contract (I-016, ACCEPTED).** Og = per-DLL
  searcher factories + vtable host binding + per-FS enable gates; core =
  flat C ABI + in-process FsFamily switch. FS-code spaces are DIFFERENT
  (og {3,9,0xB,0xD,6} vs core {1,4,7,8}) — every parity table translates,
  never equates.
- **D-59j — slice-layer supersets (I-017, ACCEPTED).** Byte units with
  enforced alignment; eager ctor validation; rich per-call errors +
  persistent ReadErrorMap; NO sticky error, NO null-buffer guard (the
  og's ctor validates nothing — that is why it needs them; our ctor
  boundary already rejects). The one behavioral divergence (tail reads)
  is M-032, not this row.
- **D-59k — EPM tool operations excluded (I-019, ACCEPTED).** EditLdm2*
  write-side, Ntfs2FAT conversion, partition merge/resize deals, FatFormat,
  IMAPI2 burn — tool ops, not recovery; zero code in the core (grep-
  verified). S-029 (apply path) is deliberately NOT covered by this
  exclusion.

### D-60 — Session-5 scope/superset registers (I-021/I-022, session-7 verification)

Verified against corpus 03ad46c and the running core before acknowledgment.

- **D-60a — Elevation (I-021-1, ACCEPTED).** Og: DRW.exe + DRWUI.exe =
  requireAdministrator, DRWService.exe = asInvoker (manifest strings
  re-verified per binary); the og has no non-elevated path. Core:
  `cr_elevate` probe + graceful degradation. Do NOT demand elevation.
- **D-60b — EuSendMessage (I-021-2, SCOPE).** WM_COPYDATA{999} to
  "DRW_Installer" — installer-only; zero engine contract.
- **D-60c — Destination repair toggle (I-021-3, LAYER).** UI-layer toggle;
  the core's repair hook stays pre-scan (S-008 scope).
- **D-60d — CRecoveryFilter layering (I-021-4, LAYER).** Og pushes filter
  state into the write phase; core filters at query, writer takes the
  filtered set. Equivalent semantics.
- **D-60e — PENetwork SMB destinations (I-021-5, LANE).** WinPE-lane
  validation item (N-056 test gap), not an engine gap.
- **D-60f — DRWCfg.bin (I-021-6, SCOPE).** UI-layer AES config cache; the
  engine reads RawConfig/FileFilter/raw_signs only. (N-170 re-truth: the
  ENGINE never reads EUUserData — its sole og consumer is DRWUI's
  CHKRepairFileSign, the CHK repair chain, per CORRECTIONS.md #9/E-A.)
- **D-60g — Session robustness (I-022-1, ACCEPTED).** Per-record CRC32 of
  the decompressed payload, F10 resync + torn-tail salvage, decompression
  bomb caps (cr_crsf.cpp) — the og's loader is per-field only. Keep.
- **D-60h — Btrfs compressed extents (I-022-2, ACCEPTED).** Structured
  {codec, psize} vs the og's opaque dual-shape buffer.
- **D-60i — ZFS runlist (I-022-3, ACCEPTED).** Decoded absolute extents +
  allocated_size vs the og's polymorphic unvalidated shape.
- **D-60j — Naming wire (I-022-4, ACCEPTED).** Single UTF-16LE field vs the
  og's per-class length mix. Documented in the session contract.
- **D-60k — changed/allocated props (I-022-5, ACCEPTED).** Core superset
  fields with no og carrier.
- **D-60l — Core-only carve anchors (I-022-6, ACCEPTED).** ISO9660, RAR5,
  M4A, ZIP-CD/naked-MOV (S-007's register).

### D-95 — Carve common formats: the og's hardcoded dual-registry set (S-007 primary) (renumbered from a same-window D-57 collision - the errata convention)

**Class:** parity + two registered calibration deltas · **Finding:** diff-to-fix S-007 (SEVERE, primary set FIXED; niche tail scoped in the finding) · **Date:** 2026-10-26

The og hard-codes the common user formats in the dual magic registry
(GIF8, BM, the MPEG-audio sync family, AAC ADTS, `{\rt`, OLE D0CF11E0,
MPEG start codes, AC10 — corpus registrations cited in the finding). The
core's registry had none of them; `raw_signs.json` never covered them.

**Core decision (S-007 fix):** 27 appended registry entries with 9
grammar validators, corpus-exact keys, reimplemented from the format
grammar (OGG behavior as the model). Two deliberate calibration deltas,
both registered:

- **The headerless audio gate is 10 consecutive frames** (MP3/AAC), not
  the og's 2 (:37152-92). The og's gate is calibrated for
  sector-aligned probing (mid-stream noise never reaches its grid); the
  core's scan is byte-granular (the I-009 documented superset), so the
  finer grid needs the stricter frame gate to hold the false-positive
  rate. Net vs og: streams under 10 frames that happen to start
  sector-aligned are og-carved and core-rejected (a ~0.26 s audio
  floor); mid-sector streams of any length are core-only.
- **`suppress_overlap` on the audio family:** every frame of a
  headerless stream re-matches the 2-byte sync, so the byte-granular
  scan would emit overlapping per-frame candidates (a 12-frame stream =
  3 nodes). The og's BruteForce walker runs once per stream start; the
  flag reproduces that at the core's grid (a later candidate starting
  strictly inside an emitted same-format stream is the same stream).
  Other formats keep the exact-start dedup only — nested cross-format
  files (zip-in-docx) stay carvable.

**Scoped remainder:** the ~25 niche og-only formats (the corrected table
in the finding file) + the OLE CLSID name classification (single-ext
per-signature shape) + the DWG section-locator walk (version gate +
bounded extent now) + the bench seed-matrix extension (Windows runner).

**Pin tests:** `Carve.S007CommonFormatsCarve`, `Carve.S007ValidatorsRejectNoise`,
`Carve.S007TruncatedSalvageSemantics`.

### D-92 — ext4/Btrfs lossless invalid-UTF-8 name mapping (M-010) (renumbered from a same-window D-53 collision - the errata convention)

**Session:** diff-to-fix remediation (M-010). **Corpus:** EXT4SearchDLL /
BtrfsSearchDLL imports (zero conversion APIs — re-verified); ZfsSearchDLL
FUN_18001c630 :24995+ (CP_UTF8, flags 0 — replacement).

- The og's ext4/Btrfs engines pass raw name bytes through UNCONVERTED into the
  session; the core's internal representation is UTF-16, so raw bytes must be
  ENCODED reversibly: every byte that cannot start or continue a valid UTF-8
  sequence maps to **U+E000 + byte** (private-use plane; bijective). This is a
  *fidelity* superset over the og's observable behavior (the og keeps the bytes;
  the core keeps a reversible encoding of them — `lossless_u16_to_utf8`
  recovers the original byte string).
- **ZFS stays U+FFFD** (the strict `utf8_to_u16`): the og's ZFS engine converts
  via `MultiByteToWideChar(CP_UTF8, 0)` and bakes replacements — the engine
  asymmetry IS the parity. Pinned by `Text.ZfsDecoderStaysReplacement` and the
  unchanged `Zfs.NonAsciiNamesDecodedNotByteLifted`.
- Downstream layers (session/report/CLI) must NOT sanitize private-use units —
  they round-trip arbitrary UTF-16 today; the round-trip tests guard this.

**Pin tests:** `Text.InvalidByteLosslessMapping`,
`Text.LosslessRoundTripRecoversRawBytes`, `Btrfs.Latin1NameRoundTripsRawBytes`,
the updated `Ext4Engine`/`Btrfs` `NonAsciiNamesDecodedNotByteLifted` (lossless
assertions).

### D-61 — RawFixer 4Kn sector-parametric buffers + sector-correct restore plans (A-005)

**Class:** parity/correctness fix (no deliberate divergence) · **Finding:** diff-to-fix A-005 (ACUTE, FIXED) · **Date:** 2026-10-26

The og's repair chain is sector-parametric end to end (corpus
`rawfixer-v2--method-inventory.md`; the FIX-3 wave already made the READ
LENGTHS device-sector-sized). The pre-A-005 core kept four 512-byte stack
buffers receiving `sector`-byte reads — a 3,584-byte stack overflow per
read on 4Kn hardware (ASan-proven: the pre-fix tree dies with
`stack-buffer-overflow` in memcpy on the first 4Kn analyze). Fix: the
four analyze() boot-sector buffers are heap vectors sized by the device
sector; the exFAT via-backup restore plan is sector-sized (the old
512-of-4096 restore left the rest of the boot sector garbage);
`plan_exfat_checksum_after` mirrors the FULL restored sector (the old
fixed-512 mirror checksummed a half-stale region), with the restore
length threaded through so the deep-scan 512-byte captures (M-046's
scope) stay bounded. The FAT backup-branch buffer is sector-sized the
same way. Pinned by `RawFixer.NtfsPrimaryDamaged4Kn`,
`RawFixer.ExfatViaBackup4Kn`, `RawFixer.AnalyzeAllBranches4KnAsan`
(the fail-before is the ASan abort itself).

### D-62 — linuxfs minors register (N-006; session-7 linuxfs-minors batch)

- **D-62a — Btrfs non-pow2 nodesize rejection (N-006, ACCEPTED HARDENING).**
  The og's band [0x86, 0x10000) has NO pow2 requirement
  (BtrfsSearchDLL :10302-10304), so 24 KiB nodesize (0x6000 % 0x1000 == 0)
  is og-legal. Real mkfs.btrfs only writes pow2 nodesizes - non-pow2 is
  corrupt-SB territory, and a forged value would size node-buffer
  allocations from an attacker-influenced field. The core's pow2 gate
  stands; refusal routes the volume to other engines/carving (strictly
  better than the og's garbage attempt). Pin:
  `Btrfs.NonPow2NodesizeRejectedRegistered`. Do not loosen for parity.

- **D-63 — health failing-bit disk-grade superset (S-024 upgrade, ACCEPTED).**
  The og DiskHealth grader (FUN_0040cdf0) never reads the SMART flags
  byte; a FAILED-NOW bit (flags bit 3) does not affect its verdicts.
  Our engine keeps the per-attr display grades og-exact (no flags
  consumption) but forces the DISK grade to Bad when any attribute's
  flags report failure-now — the one deliberate divergence of the
  health surface (a drive that self-reports "failing now" must not be
  shown Good). Pinned by Health.FailingBitForcesDiskBadButAttrGradesFollowOg;
  the Unknown path is untouched (the superset does not fire without
  data). Registered in the S-024 Resolution addendum with the decode
  evidence (evidence/decompiled-quotes/s6-health-grading-decode.md §6).

### D-63/D-64 — carve-magic quartet registers (N-051..N-054; session-7 batch)

- **D-63a — IIU-salvage fallback (N-053, ACCEPTED SUPERSET).** The og binds
  the IIU\0 key to the TIFF chain only (census :53; no Panasonic row) and
  carves nothing when the TIFF grammar rejects. The core's FMT_RW2 row now
  fires after FMT_TIFF_EP rejects: IIU + a plausible IFD offset (u16@4 != 0)
  salvages at confidence 600 with the honest note "IIU-salvage (TIFF/EP
  rejected; Panasonic RAW fallback)". Pin:
  Carve.IiuRw2FallbackWhenTiffEpRejects.
- **D-64a — JPEG 3-byte prefix (N-054b, ACCEPTED SUPERSET).** The og keys
  JPEG on four exact dwords (FF D8 FF E0/E1/DB/EE, census :54); the core
  matches the 3-byte prefix - FF D8 FF xx with xx outside the og's four
  carves here and is not dispatched by the og. I-009 family; validate_jpeg's
  grammar gates the false-positive surface. Do not tighten for parity. Pin:
  Carve.JpegFourthByteOutsideOgSet.

- **D-64 — image-container scope (S-030, ACCEPTED; CONVERGED across two
  parallel landings — b30354d's flat/VMDK/VHD core, then the convergence
  upgrade adding VHDX + the differencing families + the qemu-fixture
  compatibility proof layer).** The og's import factory dispatches
  edisk/vhdx/vmdk/vhd + the flat fallback for everything else including
  .dmg (DRWUI FUN_1405b3410 — note: the factory has NO dmg case; the og
  opens .dmg as raw flat bytes, compressed or not, and the scan reads
  what a raw read yields; parity is the same flat fallback, NOT Apple
  container parsing). The core now implements flat raw, VMDK
  (monolithicSparse + descriptor extents FLAT/SPARSE/ZERO + parent
  chains via parentFileNameHint, depth-capped), VHD (fixed + dynamic +
  differencing via W2ku/W2ru locators), and VHDX dynamic (header pair
  CRC-32C + region table + metadata + chunk-ratio BAT) with a READ-ONLY
  in-memory log replay overlay (qemu vhdx-log.c walk semantics — a
  recovery tool never rewrites its input; qemu itself refuses read-only
  opens with a valid log sequence). One exclusion and two remainders,
  honestly registered: **edisk** (EaseUS Todo-Backup proprietary,
  undecoded in the corpus; ERR_UNSUPPORTED); **compressed VMDK**
  (streamOptimized/streamMonolithic + the ESX VMFS extent families;
  refused precisely, never mis-read as flat); **VHDX differencing**
  (parent-locator metadata + sector-bitmap blocks; refused precisely).
  Pins: `Image.VmdkSparseExtentReads`,
  `Image.VmdkDifferencingChainReadsParent`,
  `Image.VmdkStreamOptimizedRefused`, `Image.VhdDynamicDisk`,
  `Image.VhdDifferencingReadsParentOnClearBitmap`,
  `Image.VhdxDynamicReads`, `Image.VhdxLogOverlayServesUnflushedWrites`,
  `Image.VhdxDifferencingRefusedPrecisely`,
  `Image.EdiskRefusedAsRegisteredExclusion`, and the qemu-img-10.0.13
  compatibility fixtures (`Image.QemuVmdkCompat`, `Image.QemuVhdCompat`,
  `Image.QemuVhdxCompat` over tests/data/images).

## M. diff-to-fix session 10 (2026-10-26)

- **D-68 — the lost-partition apply path (S-029, IMPLEMENTED).** The
  og's apply is a consent-gated product flow (RecoveryModule
  `CreateRecoveryPartitionObject` → PartitionRecovery FUN_1800566d0
  "rebuild disk layout": DELETE_DRIVE_LAYOUT + raw sector-0 rewrite +
  SET_DRIVE_LAYOUT, UPDATE_PROPERTIES with settle). The core implements
  the SEMANTIC through the rawfixer seams: `apply::plan_apply` (read-only,
  containment-refusing) + `apply::execute_apply` (consent callback →
  revalidation → transactional sector-0 write → read-back verify →
  rollback) + the shared CRUNDO01 undo, surfaced as `rrecov apply --disk
  N --session F --candidate N [--apply]`. Deliberate v1 boundaries,
  registered: (a) **MBR only** — GPT layouts are refused cleanly (the
  og's GPT variant FUN_1800392f0 incl. GUID regen is v2 scope); (b) the
  write lands through the raw sector-0 path (one of the og's two
  equivalent legs) rather than the kernel IOCTL chain — same resulting
  table, verifiable, undoable; the win layer adds the og's
  IOCTL_DISK_UPDATE_PROPERTIES + 3×Sleep(1000) settle after the write;
  (c) the entry is written **not-bootable** (the og's decoded flow
  carries no boot-flag decision for the recovered entry; auto-activating
  a boot flag would be a guess); (d) **boot-chain repair (M-040) is not
  chained** — the og's apply composites BootIni_Init + Fixup.mo BCD
  repair; the core's apply exposes the boundary truthfully through the
  plan's `boot_repair_available` field and refuses to pretend.
- **D-69 — the apply MBR type-byte map (convention, corpus-undecoded).**
  The og's own family→type map for the recovered entry is not decoded in
  the corpus for this path. The core uses the industry conventions:
  NTFS/exFAT/ReFS/BitLocker→0x07, FAT→0x06, ext2/Btrfs→0x83,
  HFS+→0xAF, ZFS→0xBF, unclassified→0x07 (generic data). Any future
  corpus decode that differs supersedes this map (registered in the
  S-029 Resolution when it happens).

- **D-66 — raw-sign confidence consumption (N-030, ACCEPTED SUPERSET).**
  The og's raw_signs.json parser (FUN_18001e510, decompiled
  :23522-23765) splits each sign string and consumes ONLY the offset
  and hex body — the third field ("0,44564456,0.96" → 0.96) is DEAD
  DATA in RAWSearchDLL; overlapping hits are resolved by sign-table
  order and the probe's own logic. The core scales the field (0.96 →
  960) and uses it for winner selection at shared positions plus the
  (header+tail)/2 + 100 tail-pair blend — a deliberate ranking
  superset over dead data. Do NOT "fix" to table-order in a parity
  pass without first decoding the og's own emitted grade
  (FUN_18000d670's result fill — tracked in N-030's Resolution).
  Pinned by `CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic` (the
  best-confidence winner at the flp/fst shared position) and
  `CarveEngine.RawSignTailPairWithinFormat` (the blend arithmetic).

## S. diff-to-fix session 15 (MAJOR lane, 2026-10-26)

### D-70 — FAT engine bps ceiling 32768 (M-045)

The og's classifier accepts ANY 512-multiple bytes-per-sector with no
upper bound (FUN_180004610 :2542, `(bps & 0x1ff) == 0` — the u16 field
tops at 65535, so 33280..65280 are og-classifiable). The core's
`validate_boot` adopts the 512-multiple rule (probe coherence with
`lost_boot_flavor`) but bounds bps at **32768**: no real format writes
past 32768 (512 through 32768 covers every FAT spec geometry and every
known formatter), and the bound only cuts absurd-volume noise the og's
own engine would fail on anyway. Documented deliberate bound; pinned by
`FatEngine.FatBps32768CeilingAndNonMultipleRejected`.

### D-71 — ext4 valid-entry advance by rec_len (M-037)

The og's dir-block walk advances VALID entries by the padded-name size
`(name_len+11)&~3` (FUN_180005820's emit path) and relies on zero-fill
tails / inode-0 termination for slack regions. The core keeps advancing
by `rec_len` (the structurally-validated field), which handles
non-zeroed tail slack and merged deleted-entry regions without
terminating the walk — a strict superset on real damaged volumes. The
SKIP path (invalid entries) matches the og exactly (the 4-byte resync
slide). Pinned by `Ext4Engine.CorruptRecLenSkipsEntryNotBlock` and the
inode-0 superset pin `Ext4Engine.InodeZeroHoleContinues`.

### D-65 — Hard-link emission: the opt-in per-parent fan-out (M-007, REGISTERED SUPERSET — off by default)

The og emits ONE node per MFT record (FUN_180006db0 :5273; FUN_1800448b0
fetches two $FILE_NAME entries and picks one by namespace ranking -
FUN_180043330). The core's default now matches this exactly
(`WalkerFlags::single_link_per_record = true`, M-007). The pre-M-007
behavior - one node per distinct parent, each with its own link name -
remains available as an EXPLICIT opt-out (`single_link_per_record =
false`) for "show all hard-links" scans. It is a registered deliberate
divergence when enabled: the result set is a strict superset of the og's
(N-1 extra nodes per hard-linked record), per-file grading is duplicated
across the fan-out nodes, and the hardlinks WalkStats counter is nonzero
only in this mode (the og's scan has no such counter). Pinned by
`NtfsVolume.HardLinkFanOutOptIn` (per-link names, per-parent nodes) and
`NtfsVolume.HardLinkSingleLinkMatchesOg` (the parity default).

### D-73 — Attrlist chase depth: the multi-hop BFS default (M-008, REGISTERED SUPERSET — the default)

The og's $ATTRIBUTE_LIST chase is depth-1 (FUN_180041df0 iterates the
pass-1 child-record list, never recursing into child records' own lists).
The core's default keeps the multi-hop BFS (depth 64, cycle guard): for a
chain base -> child(nested list) -> grandchild($DATA), the og recovers the
child's fragment and loses the grandchild's; the core merges both. This is
a deliberate recovery superset — the og's depth-1 is a limitation, not a
choice (the finding's own recommendation). Opt-in parity for benchmark
comparisons: `WalkerFlags::single_hop_attrlist = true` (the chase stops
after the first level). Note the corrected model (S-012's decode): the og
chases DELETED records too in pass 1 — the deleted-gate variant some early
notes assumed does NOT exist on either side. Pinned by
`NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1` (parity) and
`NestedAttrlistChase` (the default), plus the deleted-chain depth test.

## T. diff-to-fix session 20 (DFX20 MAJOR lane, 2026-10-26)

**D-75 — BitLocker in-stack decrypt: FVEK-input key surface (M-033's
documented scope; CONFIRMED og-parity by the s31 decode — see the update
below).** The og acquires FVE keys through its full protector
chain (user password/recovery-key/BEK → stretch → VMK → FVEK, inside
BitLockerLib) and the DRWService auto-unlock pipe; the corpus pins that
chain's existence and API symbols but NOT its byte interop (the FVE
metadata datum parse sits behind opaque C++ object graphs in
ParserBitLocker — verified first-hand; see OQ-S20-1, resolved s31). The
core's landed surface therefore takes the FVEK directly — the
`BitLockerManager::GetFullVolumeEncryptionKey` output bytes (64/128 hex
chars) — via `--bitlocker-fvek` (CLI) and `cr_ui_open_volume_with_key`
(ABI). Everything else matches the og layer-for-layer: AES-XTS
(pinned byte-exact against OpenSSL EVP, the og's own cipher interface),
per-sector LE-index tweaks, decrypt below the engines, locked-without-key
refusal preserved. Deliberate, documented. Do NOT remove the refusal
path or the weak-key rejection when extending the key surface.

**s31 update (OQ-S20-1 resolved — the premise corrected, the scope
CONFIRMED as og-parity):** the decode (evidence
diff-to-fix/evidence/decompiled-quotes/s31-oq-s20-1-bitlocker-derivation-scope.md)
proves the og's BitLockerLib does NOT implement the recovery-password
stretch (no 0x100000-round loop; the verified SHA-256 layer has three
non-iterating consumers; the KDF framework defaults 10000 with only the
vendor "BITLOCKER"-labeled KDF) and DELEGATES user-key unlock to Windows
(PSUnlockBitLocker `Unlock-BitLocker -MountPoint`, manage-bde fallback).
The og's in-stack surface = metadata parse + protector enumeration + the
64-byte FVEK extract + auto-unlock keys + XTS decrypt — the core's landed
surface matches it layer-for-layer. The stretch/AES-CCM/BEK port (a
dislocker-class capability) and a Windows unlock shim are registered
IMPROVEMENT options, not parity debt: (a) in-stack FVE metadata parser
(protector enumeration for diagnostics), (b) the standard KDF + CCM VMK
unwrap + BEK parse (Windows-free unlock — valuable where even the og's
delegation fails), (c) the Unlock-BitLocker delegation shim on Windows.
### D-74 — Slice tail-read refusal vs the og's clamp+zero-fill (M-032, ACCEPTED SAFETY DIVERGENCE)

The og's virtual-volume layer (HFSPlusSearchDLL FUN_180009290 :7219-7264,
DRWUI twin :1044939-1044979) CLAMPS a tail-crossing read: the in-bounds
prefix is read from the parent and the past-end tail is zero-filled, and
the call succeeds. Our PartitionSlice refuses the whole read with
ERR_RANGE_OUT_OF_BOUNDS. This is a deliberate safety divergence: a refused
read is honest; a zero-filled read can fabricate structure near a volume's
computed end (ragged candidates). The engine audit (M-032's decision gate,
session 11) found no reliance on the og contract — journal_ring_read bounds
every take within the ring; all other engine reads are explicitly bounded;
refusals degrade through the damage counters. The og's own tail memset is
in SECTOR units (count-n, :7258) — an under-zeroing defect NOT adopted.
Pinned by Slice.TailReadRefusalPinned + the updated Slice.BoundsEnforced.
If a future engine needs the og contract, the documented next step is the
opt-in clamped-read primitive from the finding's path (2) — added THEN,
not as dead code now.

(The original session-10 D-64 draft was folded into the converged entry above.)

**D-74 SUPERSESSION ADDENDUM (2026-10-29, the S47 MAJOR-first fix lane, M-119
landing `see git log --grep M-119`):** the read-half of this divergence is
SUPERSEDED - `PartitionSlice::read_at` now implements the og's ladder
(straddle clamps + full-tail zero-fill + success; offset >= length and count >
length remain the parameter errors BOTH sides refuse). What changed versus the
session-11 decision gate: (1) M-119's real-run evidence - `DRWUI.log:638`
shows the clamp firing during the og's own lost-partition probing (the og's
scan flows ISSUE past-end reads and proceed on the zero-filled result), and
the S42 Ldq decode found 393 out-of-parameter refusals in ONE captured run -
the og's layers probe boundaries routinely; (2) the M-119 fix-time probe
re-confirmed the ENGINE-side device's clamp first-hand
(HFSPlusSearchDLL FUN_180009290 :7219-7264 - the gate, the parent prefix
read, the tail memset); (3) the whole-volume refusal on table-length overrun
(the CLI/UI leg) was a divergence this entry never claimed to cover and the
exact damage class recovery tools are bought for. KEPT from this entry: the
parameter-error refusals (the safety core), and the under-zeroed-tail memset
non-adoption (the og zeroes sector-count BYTES; the core zeroes the FULL tail
- the D-117 class). The create-half is new parity, not divergence:
`PartitionSlice::create` clamps an over-long declared length to the device
end with the delta surfaced (`was_clamped`/`clamp_delta`). Pins:
`Slice.TailReadClampsAndZeroFills` (replaces TailReadRefusalPinned) +
`Slice.CreateClampsOverlongDeclaredLength` + `Slice.CreateStartBeyondDeviceStillRefused`
+ `Slice.ExactFitNotClamped` + `Slice.OrchestratorTargetClampScansRaggedTable`.

### D-72 — scan cache: 8 MiB reactive LRU vs og 4 MiB prefetch read-ahead (N-045, REGISTERED DIVERGENCE — budget + algorithm)

The og's `CStorageDeviceWithCache` sizes its cache at **4 MiB**
(DRWUI_decompiled.c:919080-:919092 — cache block count =
`0x400000 / bytes_per_sector`, fallback `0x2000` blocks; constructor
:962250-:962292 with a 1000-entry cache count; one instance per
physical disk, shared across all partition scans) and runs it as a
**prefetch read-ahead engine** — a dedicated prefetch thread, a 10-run
look-ahead window, 1000-sector default runs, CRITICAL_SECTION locking
(:962250-:962597). The core's scan pipe builds an **8 MiB** reactive
true-LRU (`CachedReader::create(..., 8ull << 20, 256 * 1024)` at
cr_scan.cpp; 256 KiB block size separately registered as D-51) with no
prefetch thread. Both divergences are deliberate: the 2x budget rides
the 256 KiB block curve measured by the alignbench transport sweep
(PERFORMANCE-BASELINE section 5), and the sequential latency-hiding the
og obtains from its prefetch thread is provided one layer down by the
transport's large 64 KiB-multiple reads. Aligning the budget to 4 MiB or
introducing a prefetch thread is a benchmark-gated tuning decision
(docs/benchmarking.md), not a parity fix. Placement parity confirmed:
both caches sit below the slice layer and are shared across candidate
scans. Pinned by `Cached.ScanCacheBudgetIsTheRegisteredDivergence`
(doc-truth pin: source constant + this row + the cr_cached.h divergence
block must stay in lockstep).

### D-99 — NTFS scan-path volume-end backup fallback (M-043, session 14; renumbered from a D-72 collision with the scan-cache register — the errata convention)

**Deliberate recovery SUPERSET — not og parity (corpus-proven).** The og
has NO scan-path backup-boot fallback: the analyser's backup step
(FUN_180005470 :4309-4327, decode read first-hand) is a memcmp storing the
verified location at +0x510 as the REPAIR input, and NTFSSearchDLL carries
zero backup reads. The og refuses torn-primary DBRs on the scan path. The
core opens from the volume-end backup instead: read-only, flagged
`VolumeParams::boot_from_backup`; repairing the primary stays
consent-gated rawfixer territory (S-008). Bench comparisons must expect
core-only finds on torn-DBR NTFS fixtures. Registered by DFX14 while
reconciling the concurrent M-043 landing (1468393 + this leg).

*(D-numbering erratum, session 14: this section was first registered as
D-72 in this leg's push; the parallel leg's scan-cache register (N-045)
landed as D-72 in the same window. The FAT-entry precedent applies — the
later landing renumbers; this one is now D-73. All references updated.)*

### D-97 — carve default-off formats are host policy, not engine constants (M-024) (renumbered from a same-window D-72 collision - the errata convention)

The og's RawConfig.ini [Filter] ships STL and VPJ default-DISABLED — its
effective carve set differs from its compiled set. The core ships every
compiled format ENABLED and expresses default-off through
`CarveFlags::disabled_formats` (the host's policy layer — the CLI/UI can
mirror the og's shipped defaults by passing "stl vpj"). Rationale: an
engine constant would bake one vendor's config into the library; the og
itself separates engine (compiled analysers) from policy (the ini). Pinned
by `CarveEngine.AllFormatsEnabledByDefault`.

### D-68 — Error value space: family-prefix convention only; numeric values core-internal (M-049, ACCEPTED; context subset LANDED)

The og's engine errors are 12-hex-digit HRESULT-shaped codes
(0x5002200000004 open-fail, 0x5002c0000004 read-fail — the only two
corpus-documented values; the og's full table is not documented), carried
by an error-object architecture (CErrorManager/CErrorTrackManager, the
MOM UI bus). The core's 28-code family keeps the HRESULT-style family
PREFIX convention (0x5002 open/transport .. 0x5009) with core-internal
numeric values; the old header claim ("reproduces the reference engine
convention") was false as written — value equality was never true for any
code. Renumbering to the two known 12-digit values would break 26 others
arbitrarily (the og table is undocumented); the family-prefix match is
the honest extent of the parity. Pinned by Error.ErrTextCoversEveryCode +
the header truth.

The ENGINE-SIDE SUBSET of the og's error objects LANDED with M-049's
close-out (session 24): the error-range event family now carries the
module/operation/retry-state triple (`err_module`/`err_operation`/
`err_attempts` on `cr_ui_event`, CR_ERRMOD_*/CR_ERROP_* constants; the
og's module ids are string class-names, ours are core-internal numbers —
the D-68 value-space policy applied to the module dimension). Per-range
attribution is recorded at failure time (thread stamp + device-owner
fallback, cr_errmod.h); sessions persist the geometric trio unchanged
(a reloaded range is honestly unattributed). The MOM-bus
manager/track-manager layer stays out of core scope (UI-bus
architecture). Pinned by ScanEvents.ErrorRangeCarriesModuleOperation-
Attempts + CarveErrorRangeAttributesToCarveModule + ProbeBlockFailure-
AttributesToOrchestrator + the Retry/DevMap/Errmod unit trio.

### D-140 — carve validator chain: single modern validator per format, no legacy File*Analyser fallback generation (N-070, REGISTERED SIMPLIFICATION — deliberately not ported; renumbered from its born-collided D-75 seat at the N-207 repair: the M-033 BitLocker row keeps D-75)

The og's analyser registry wires every magic key to a two-generation chain:
a modern (namespaced) analyser plus — where one exists — a legacy
first-generation `File*Analyser` fallback; a candidate is discarded only
when BOTH validators fail (rawsearchdll--analyser-registry-census.md:43,
:45-117 the chain roster). The core has exactly one validator per registry
row; a rejection is final at the grammar layer. Decision (explicit, per
the finding's own fix-plan option): the legacy validator BODIES are not
decoded in the corpus — the census carries class names and chain wiring
only; the two legacy grammars that ARE fully decoded (the FileMKVAnalyser
EBML vint walk, analyser-granular-batch1 :43-61; the FileRARAnalyser fast
validator, :25-40) are already the core's validators (N-051 quartet).
Porting the undecoded remainder would be invented grammar — fabrication,
not parity. The N-070 half this registers away from is the second-chance
accept-rate; the overlap-scoring half (accumulated header+tail confidence
ranking) IS implemented (the finding's fix). Revisit precondition: a
future corpus decode of any legacy File*Analyser body makes its port
evidenced — until then this stays a registered simplification, pinned by
CarveEngine.OverlapScorePrefersHigherConfidence (which proves the scoring
half) and the absence of any fallback path in cr_registry.cpp.

### D-76 — carved-file naming: RETIRED with S-049 (the og's universal contract decoded and landed)

**Class:** resolved (the deliberate-default registration narrowed to nothing) · **Finding:** diff-to-fix S-049 (SEVERE, FIXED) superseding N-063 · **Date:** 2026-10-29

**SUPERSEDED by executed evidence.** The three REAL og sessions
(`og-codebase/SaveScan/`, ~2M rows) decode the naming rule universally:
every unnamed carved file is named `<PREFIX>_<%06d>.<UPPERCASE-EXT>`
(DRWUI FUN_140c44770, :1878330-81, both caller sites :1877600/:1877724;
the per-format counter map :1877584-90) with PER-FORMAT counters (the
JPG counter runs 1..3938 continuous across its plain/dims/Model/Software
prefix variants; GZ 1..27,694; PNG 1..17,580) and the format's own
UPPERCASE extension as the default prefix (GZ_000001.GZ, PNG_000001.PNG
— every observed format). The N-063 premise ("formats with no decoded og
naming rule") is falsified: the rule IS decoded, universally. The
File%06u deliberate default (this row's divergence 1) is RETIRED —
`default_namer` now emits the og contract, and the retired default is
pinned absent by `CarveEngine.FileNnnnFallbackRemoved`.

The MP4/MOV "Lost Name File (<time>)_EaseUS" engine template
(RAWSearchDLL :33312/:33408 — the N-063 decode, real engine code) does
NOT fire for carved files in the shipped product: 57/57 real MP4/MOV
carve rows across the three sessions are MP4_%06d.MP4 /
MP4_<duration>_%06d.MP4 / MOV_* (MP4_14s, MP4_08m07s, MP4_01m, MOV_10s,
MOV_01m01s — the mvhd timescale/duration prefix). The DRWUI
tree-naming layer names every unnamed carve record through the %s_%06d
contract and the engine's SetName template does not survive it. The
template landing (N-063) is superseded ON THE CARVE-TREE APPLICATION;
its engine-side decode remains documented here and in the N-063 ledger
row. The template's two replicated quirks (no 1904-epoch conversion,
the v1 high-word truncation) retire with it.

The content-derived prefix families (all real-session-pinned, landed
with S-049): JPEG SOF dims `JPG_<W>x<H>` (the DRWUI caller's
[1,0x2000] sanity gate) / EXIF Model (`GT-S7562`) / EXIF Software
(`Adobe Photoshop 7.0`, `GIMP 2.6.8`, `Android SP1A...`); MP4/MOV mvhd
duration; MP3 ID3v2 title; PDF Title (25-char read cap — the
`LaTeX with hyperref packa` / ` TeX output 1998.06.28_17` truncation
pins) else Creator/Producer + CreationDate (`YYYY-MM-DD HH-MM-SS`, the
plain `D:YYYYMMDDHHMMSS` form only — matplotlib's dashed
`D:2022-08-17 15:05:58+02'00'` never produces a date in any session);
DOCX docProps created; ZIP first central-directory entry name with path
separators -> `_` (`META-INF/` -> `META-INF_`; dots preserved — the
N-063-era dot-translation example is corrected by the classes.dex /
027.jpg families); WAV LIST/INFO INAM title. The JPEG Model-vs-Software
precedence (Model first) and the PDF Creator-vs-Producer order are the
documented closest-pinned readings (no observed co-occurrence separates
them; recorded in S-049's Resolution as the open nuances).

**Carrier:** `CarveResult::name_prefix` (the semantics moved from
N-063's full-name `embedded_name`; emit precedence: namer callback >
the og-contract name with the analyser prefix over the per-format
counter).

**Pin tests:** `CarveEngine.OgNamingUniversalExtCounter`,
`OgNamingJpgSofDimensions`, `OgNamingJpgExifModelAndSoftware`,
`OgNamingCounterPast99999Unpadded`, `OgNamingMp3Id3Title`,
`OgNamingPdfCreatorAndDatePrefixes`, `OgNamingDocxCreatedTimestamp`,
`OgNamingZipFirstCentralDirectoryEntry`, `OgNamingWavInamTitle`,
`Mp4NamedByDurationPrefix`, `MovNamedByDurationPrefix`,
`MovV1MvhdDurationRead`, `FileNnnnFallbackRemoved`.

### D-98 — Lost-partition APPLY: the safety registers around the og's table writer (S-029) (renumbered from a same-window D-72 collision - the errata convention)

The S-029 apply module reproduces the og's write-path architecture
(PartitionRecovery FUN_1800566d0/FUN_180038b10/FUN_1800392f0: the legacy
0x7C100-delete -> sector-0 rewrite -> 0x70140-refresh -> 0x7C010-set-layout
sequence, the modern 0x7C058 0x18-byte record, the 3x Sleep(1000)
UPDATE_PROPERTIES retry tail, GPT_GenerateSignature on the GPT rebuild) with
four registered deliberate deltas — each a safety superset over a reference
weakness, none a capability loss:

- **D-98a — Consent at the core (A1).** The og's apply is a UI flow the
  product calls directly; the core module REQUIRES a nonzero consent
  callback before any write (NULL/zero = ERR_APPLY_CONSENT_DENIED, zero
  writes, not even a volume lock). Pinned by `LostApply.RefusedWithoutConsent`.
- **D-98b — Single-entry preservation (A7).** The og's legacy path
  zero-wipes ALL FOUR entries as part of its rebuild; the core's
  SingleEntry scope (the default) preserves the other three entries
  byte-for-byte — applying one lost partition must never destroy three
  live ones. The og's exact wipe semantics remain available as the
  explicit RebuildTable scope. Pinned by `LostApply.WritesEntryAndRefreshes`.
- **D-98c — Refresh-tail honesty (vs the og's silent swallow).** The corpus
  tail loop ignores a failing 3x UPDATE_PROPERTIES retry
  (PR:47015-47025 - the function returns with no error); the core reports
  ERR_APPLY_REFRESH_FAILED with the table written+verified and the undo log
  valid — a stale OS mount view must be visible to the operator, never
  silently absorbed. Pinned by `LostApply.RefreshFailureIsReportedNotSwallowed`.
- **D-98d — The table-change race guard (A6).** The plan snapshots sector
  0; execute refuses (ERR_APPLY_TABLE_CHANGED, zero writes) if the table
  moved between plan and execute — the og has no such guard. Pinned by
  `LostApply.DetectsTableRace`.

Corpus correction carried by the same fix (recorded in the finding's
Resolution): GPT_GenerateSignature regenerates the 32-BIT DISK SIGNATURE
(MBR +440 / DRIVE_LAYOUT_INFORMATION.Mbr.Signature), not the 128-bit GPT
disk GUID — every call site passes a 4-byte stack slot as one register
argument (PR:32805 local_24 ulong; RM:46338; CM:118357; UL:37590), and the
GPT header GUID is untouched by FUN_1800392f0. The core stamps the
regenerated 32-bit signature on the GPT rebuild exactly as the corpus does.

### D-79 — salvaged (no-EOI) JPEGs graded PartialRisk, not stamped identically to complete files (S-018 re-file, s31; REGISTERED DIVERGENCE)

The og's legacy FileJPGAnalyser tier accepts tail-truncated JPEGs at its
scan limit (h*w*3 from the candidate start, or the 200,000,000 global cap
/ 10) and stamps them with the SAME format code (0x23) and no reduced
grade as complete files (FUN_18004c480 :57958-59, RAWSearchDLL_decompiled.c;
evidence diff-to-fix/evidence/decompiled-quotes/s31-oq-s3-jpeg-fallback-decode.md).
The core mirrors the salvage's accept/sizing exactly but grades the result
PartialRisk (validator confidence 600 < the 850 Excellent band) and notes
it ("fallback-salvage(no-EOI,...)") — a recovered file whose tail is
destroyed IS objectively degraded, and hiding that from the user would be
the og's UX weakness, not a parity virtue. Pinned by
Carve.JpegTruncatedTailSalvagedAtSofEstimate (grade assertion).

### D-78 — progress delivery: push events + telemetry pull, not the og's poll-dominant timer farm (N-075, REGISTERED DIVERGENCE; renumbered from a D-77 collision with the N-009d ACL register landed by the parallel lane in the same window — the errata convention)

The og's progress model is predominantly PULL-polled: 38 `timeout()`
timer consumers call `onGet*Progress` on the UI thread, and the only
progress-push signal is `sigPercentChange` (CUISink's — corpus
UIUX/25-connect-wiring-graph.md :175-192); scan/finish events push.
The core inverts this deliberately: progress is a coalesced monotonic
PUSH (`CR_EVT_SCAN_PROGRESS` + the cr_ui_progress bridge with
single-flight delivery), and the pull surface is the telemetry
snapshot (`cr_ui_scan_telemetry`, any thread, any time) — the host
that wants og-style polling simply calls telemetry on its own timer,
so the information content is fully covered by the two surfaces the
core already had.

I-031 carve-layer boundary note (the S11 infra walk): the og's carve
ENGINE emits no progress at all — its only observer interaction is the
per-chunk continue-check (RAWSearchDLL :9439-9441); DRWUI computes carve
progress host-side off its own counters. The core's per-1-MiB-window
progress callback is therefore the registered superset on the engine
side too (the push surface the og defers to its UI), not a divergence
to be flattened away. The og's reference-out QUERY signals
(`sigIsBusy(bool&)` ×14, `sigIsStopping(bool&)` ×8, :125-144) are now
surfaced as the synchronous ABI ops `cr_ui_scan_busy` /
`cr_ui_scan_stopping` (N-075's fix) — synchronous by construction,
the stronger contract vs the og's Qt-connection-dependent answer
edge. The og's typed `IUIProgressInfo` per-item recovery-progress
interface (UIUX/20-moc-surface-sink-architecture.md :145) maps onto
the writer's report/queue item-state surfaces (documented in
ABI-CONTRACT §5a) rather than a parallel ABI interface. Pinned by
`CrUi.ScanBusyStoppingQueries` + the plain-C edges in
`cr_ui_abi_check`.

### D-77 — recovered files inherit destination ACL defaults; no source-descriptor re-application (N-009d, REGISTERED DIVERGENCE — capability not ported)

**Class:** registered divergence (writer security posture) · **Finding:**
diff-to-fix N-009d (MINOR, resolved by registration) · **Date:** 2026-10-26

**WHAT OG DOES:** the pending-operation engine's copy paths preserve
source security descriptors — epmpublic carries CAccessControlList /
CAccessEntry / CFileDaclProc / CFileSecurityInfo classes that capture the
source stream's ACL and re-apply it on the recovered copy (corpus
`re-workspace/artifacts/PartitionRecovery/pending-operation-engine.md`;
the same surface also carries IOCTL_DISK_DELETE_DRIVE_LAYOUT for its
destructive ops). On NTFS sources the descriptor stream is the $SDH/$SII
security-index pair; on FAT-family sources there is no descriptor to
carry and the og's classes no-op.

**WHAT WE DO:** `cr_safewriter` / `cr_destio` create every recovered
file as a NEW file that inherits the destination volume's default DACL
(grep SDDL/SetSecurityInfo/ACL across src/ + include/: zero hits). No
descriptor is captured on the source side and none is applied on the
destination side.

**WHY:** (1) For a recovery product the recovered file IS a new file —
the destination's owner policy should govern it, not the (often corrupt,
often attacker-influenced) source metadata; re-applying a source DACL
verbatim can re-create an unreadable file at the destination (a
recovery-negative outcome the og's users hit with inherited-deny ACEs).
(2) The NTFS $SDH/$SII decode surface does not exist in the core engine
today; porting it would be a feature-sized effort whose only consumer
would be this flag. (3) The finding's own fix text marks the port
"optional" and asks for the default-OFF decision to be registered here.
The risk is accepted: ACL fidelity is lost on cross-ACL-tree recovery,
content is never lost.

**RISK:** user-observable only when recovering NTFS→NTFS with
non-default source ACLs and expecting them to travel: they do not.
Recovery content, names, timestamps and hashing are unaffected.

**VERDICT:** ACCEPTED (default: inherit-destination). If a future host
needs descriptor transport, the recorded next step is: NTFS $SDH/$SII
index decode in the engine → descriptor carrier on
FileProperties → a WriterConfig `preserve_dacl` flag (default OFF)
applying it via SetSecurityInfo on the win destio layer only — never on
the POSIX test shims.

### D-83 — Btrfs RAID0/RAID10 chunk reads: kernel stripe math vs the og's read failure (M-020, REGISTERED SUPERSET)

The og's logical→physical node reader (BtrfsSearchDLL FUN_1800042a0,
:2498-2583, verified first-hand session 33) serves ONLY the 2-copy class
(SINGLE/DUP/RAID1 → the rotating mirror). RAID0 (class 8) and RAID10
(class 0x40) fall through to LAB_1800043fe — the read FAILS; the og
recovers nothing from striped chunks. The pre-fix core instead read
stripe 0 of every chunk unconditionally — on striped chunks that yields
WRONG BYTES (crossed data), which is worse than nothing for content
correctness.

The core now implements the kernel stripe math for both profiles
(RAID0: `idx = stripe_nr % n`, physical = stripes[idx].offset +
(stripe_nr/n)·stripe_len + in-stripe; RAID10: mirror-set selection with
og-style member rotation), splits raw extents at stripe boundaries so
spanning files map per-stripe, and refuses+counts what cannot be mapped
correctly (compressed extents spanning stripes, classes &0x80/&0x100,
zero stripe_len on striped chunks — the og refuses the latter classes
too). This is a deliberate recovery-positive superset: content the og
cannot read is recovered with correct bytes, and nothing is ever emitted
with wrong bytes. Fixtures: `Btrfs.Raid0StripeMathSplitsAtStripeBoundaries`
(the spanning split + the in-stripe offset), `Btrfs.Raid10GeometryGate`
(the og's parse-time gate stays og-exact — the superset is read-side
only).

### D-101 — Carve validators are stateless single-shot; no pending-object/ContinueScan streaming state (M-025, REGISTERED ARCHITECTURAL DIVERGENCE) (renumbered from a same-window D-81 collision - the errata convention)

The og's block scan keeps open-ended formats (MovAnalyzer/MTS and family)
as pending CRawFileInfo objects re-fed every sector through analyser slot
4 (ContinueScan, RAWSearchDLL :9541-9605, :10114-10144) until the
analyser reports done — O(1) per-sector work per open file against
buffer-resident data. The core's validators are stateless single-shot
functions (CarveResult (*)(IStorageDevice*, hit, cap)) that walk their
grammar from the hit; there is no cross-window per-candidate state.

Why kept: (1) the stateless contract is what the existing 46-test carve
suite pins and what every ported analyser to date (S-007's MPEG family,
S-017's MOV salvage) has been written against — a pending-object
migration would touch every validator for no measured coverage gain
(byte-granular dispatch finds mid-sector starts the og never sees,
I-009); (2) the read-model half of the finding — the cost profile — is
now served by the window buffer (M-025's WindowDevice: candidates cost
zero device re-reads when their reads fit the window, 13x fewer reads
measured), which was the dominant structural cost; (3) the og's
continuation machinery exists to serve ITS buffer-resident model — with
validators reading through the window context the same coverage is
achieved without per-sector state.

Boundary: any future port of an og analyser that genuinely requires
incremental continuation (streaming formats whose end cannot be probed
by a bounded forward walk) must either keep that model in its own layer
or extend the validator contract — this register is the flag S-017's
fix-plan step 3 cites.

I-031 boundary addenda (the S11 infra walk, both decoded first-hand):
(a) the og's pending list is capped at 16 size-unknown candidates
(FUN_18000c820 :9573-9591 — `0x10 < count + 1` force-finalizes the
OLDEST at its best-known size, emits it, and notifies the host); the cap
bounds the og's memory AND truncates the least-recent file on overflow —
any future streaming-analyser port behind the M-021 seam must decide its
own cap policy explicitly, not inherit one silently. (b) the og's Stop()
with an idle device force-finalizes and EMITS every pending partial file
(FUN_18000ce30 :9658-9693 — partial content reaches the user); the
core's cancel discards un-emitted candidates — inherent to this
registered stateless architecture, a visible difference on
cancel-during-carve the host layer must not paper over.

**DFX32 supplement (the convergent second landing, session 32):** the
WindowDevice/WindowView wrapper delivers the og's buffer-residency benefit
(measured on scripts/bench_m025.cpp — 64 MiB sign-quiet fixture, 1280
grammar-validated candidates: device read_at calls 20,544 -> 2,080 = 9.88x
fewer; device bytes 2.20 GB -> 1.06 GB = 2.08x fewer; no wall regression on
the memcpy device — the real-transport gain is ~18.5K fewer read round trips
per 64 MiB). The bench lesson is part of the record: an LCG-random fill was
abandoned for this measurement — random FF-Ex sync bytes send the raw-sign
tail walk into the gigabytes, measuring the tail-scan budget instead of the
window-reuse delta (the sign-quiet fill isolates the fix's effect; the
mixed-content unit fixture carries the realism duty). The geometry change
(4-8 MiB ping-pong) was evaluated on both lanes and NOT taken on either:
dispatch cost is byte-granularity-dominated (the I-009 superset) and the
window size only affects the re-read margin the view already removed.

### D-84 — disk enumeration: the two-stack merge (PhysicalDrive loop + cfgmgr32 disk interfaces), not the og's five-stack fan-in (N-071, REGISTERED DIVERGENCE — partial port with scope rationale)

**Class:** registered divergence (enumeration breadth) · **Finding:**
diff-to-fix N-071 (MINOR, fixed with the two-stack merge; residual
registered here) · **Date:** 2026-10-26

**WHAT OG DOES:** the device discovery fan-in spans five stacks — WMI
(MSFT_* namespaces), SetupAPI device interfaces, VDS (dynamic-disk
packs), Storage Spaces, and Function Discovery (mom-module-system
architecture :57-59; the QA boot census shows `CDiskEnumerator :
Storage space init` + `GetUsedPhysicalDisks` + `Vds loadservice` running
alongside the plain `disk enumerate` — vendor-logs :13-29) — merged into
the device list the UI presents, with hot-plug notification.

**WHAT WE DO:** a two-stack merge: the numbered `\\.\PhysicalDrive0..N`
loop (geometry/alignment/descriptor/table parse) + the cfgmgr32
`GUID_DEVINTERFACE_DISK` present-instance pass (every interface opened,
queried with the same IOCTL family, merged by
`merge_disk_enumeration` — dedup by disk number, then by instance
path; N-071's landing). The WMI/VDS/Storage-Spaces/FunctionDiscovery
stacks are not ported; hot-plug eventing is not ported (the CLI/UI
re-enumerates on demand).

**WHY:** (1) Every block device Windows exposes for user-mode READ
surfaces as `\\.\PhysicalDriveN` or a disk-interface instance — the two
stacks cover the readable-device space the recovery engine consumes;
the og's extra stacks feed its UI metadata (WMI friendly names, VDS
dynamic-volume CONFIG — which our LDM engine parses from the disk
itself — and Storage Spaces pool topology for display). (2) The
finding's own open question records that Storage Spaces members DO
surface as PhysicalDrive in practice (the miss-case is
UNVERIFIED-BY-EXECUTION without a Storage Spaces host; the QA census
shows the stacks RUNNING, not a disk the numbered namespace lacks). (3)
FunctionDiscovery and hot-plug notify are product-shell concerns (the
og's DeviceManager.mo is a UI-facing module); the engine's contract is
a complete device LIST on demand, which the merge provides. The
cfgmgr32 pair (`CM_Get_Device_Interface_List_SizeW`/
`CM_Get_Device_Interface_ListW`) is Win2000-era — the Win7-inclusive
import gate holds (cfgmgr32 added to the strict allowlist with the
function pair).

**RISK:** a device reachable ONLY through VDS/Storage-Spaces stacks and
absent from both the PhysicalDrive namespace AND the disk-interface
list would be missed — no such device class is documented in the corpus
(the QA host's own Storage-Spaces census ran WITHOUT producing an
extra device). The merge is the engine scope; the UI shell may layer
its own stacks later.
Y-DELTAS — deliberate-deviations registry (AUTHORITATIVE)

> **Status: promoted from the parity-audit draft (docs/parity/PARITY-DELTAS-draft.md)
> after auditor review.** Every entry carries WHAT OG DOES (corpus cite), WHAT WE DO
> (file:line), WHY (rationale class), RISK, and VERDICT (ACCEPTED / NEEDS-WORK).
> A delta that is not in this registry is a bug, not a delta. `rda-wizard` carries the
> UI-scoped mirror (its docs/PARITY-DELTAS.md); this file is the single source of truth.
>
> **Parity-audit session 1 closures (2026-10-23):**
> - F-A1-3 (fixup validation scope) resolved as **PARITY, not a delta**: we now match the
>   og's first-two-tails validation (FUN_1800411f0). The 4Kn sector-size parameterization
>   (og hardcodes 0x200) remains a registered delta below (F1 family).
> - F-B-8 (abort-path error-map merge) closed by the concurrent UI-readiness work —
>   the stamp() lambda now merges the retry error map on every exit path.
> - G-18 gang-block clause and G-19 cross-disk LDM scoping: stale texts corrected in
>   rda-wizard/docs/CORE-CONTRACT.md (gang blocks are assembled; the resolver landed).
> - F-C-1 (§13 list-format transcription error) fixed in rda-wizard bd83be3.
> - Auditor wave-1 engine fixes that matched og behavior are PARITY (not deltas):
>   FAT12 sentinels, NTFS viability probe size, HFS+ B-tree layout, NVMe page-2 offsets,
>   carve stats double-count, undo-apply disk guard.

## A. Format / session

### D-81 — the logging seam: one host sink, no cross-process transport (N-048, REGISTERED DIVERGENCE)

The og runs a three-layer logging architecture: Log.mo's EPMLOG.log
(64 KiB cross-process shared staging + named mutex + 10 MB
truncate-rollover, line format `[%Y-%m-%d %H:%M:%S:%ms] (T:%d/P:%d)
msg`), PubLog's 2 MB ring for the UI-family binaries, and Logger.dll's
async queue + worker with keep-second-half compaction
(Log_decompiled.c :159-176/:193/:604-621/:699, PubLog_decompiled.c
:119-135, Logger_decompiled.c :1630-1696/:1962-2015). The core is a
SINGLE-PROCESS library: there is no second process to share a mapping
with, no UI-family binary set for the ring, and an async worker queue
buys nothing at the core's line volumes (diagnostic brackets, not
traces). The seam is therefore the SINK, not the transport
(`cr::log::set_sink` — default NULL = silent, zero formatting
overhead; serialized synchronous deliveries; the host owns every
policy the og baked into its transports: file placement, rollover,
compaction, async). The og's per-line timestamp/process decoration is
its transports' job; the core emits the message text. Pinned by
`Orchestrator.LogSilentByDefault` + `Orchestrator.LogSinkReceivesScanBrackets`.

### D-82 — CSMI SMART transport: SCSI-address routing, no RAID census; LSILOGIC sibling and SMART auto-enable not ported (N-032, REGISTERED DIVERGENCE — transport scope)

**Class:** registered divergence (health transport scope) · **Finding:**
diff-to-fix N-032 (MINOR, FIXED with this registration) · **Date:** 2026-10-26

**WHAT OG DOES:** the DiskHealth companion's CAtaSmart engine reaches
SMART through a multi-transport accessor set. The CSMI route
(GetSmartAttributeCsmi) is IOCTL_SCSI_MINIPORT CC-0x19 "CSMISAS"
(wire-decoded this session, DiskHealth_decompiled.c FUN_00425450), but
the og routes every request through a companion-side RAID census
(GET_DRIVER_INFO / GET_RAID_INFO / GET_RAID_CONFIG / GET_PHY_INFO walks,
incl. an AMD "rcraid" family) that fills phy/port/target-SAS-address
from the array inventory. A second, LSILOGIC-signature sibling transport
(FUN_00426040) serves LSI controllers. When the first CSMI SMART read
fails, the og auto-enables SMART on the disk (command 0xB0 feature 0xD8
— a device WRITE, dispatch case 0x16 :4427-4438) and retries.

**WHAT WE DO:** the CSMI channel (src/win/cr_health.cpp
CsmiMiniportChannel) issues the og-exact CC-0x19 request built by the
pure seam (include/cr/health/cr_csmi.h — every load-bearing byte pinned
by Health.CsmiRequestMatchesOgWireFormat), routed through
IOCTL_SCSI_GET_ADDRESS on the physical-drive handle (the og's own
address probe, FUN_00417750 :12254): PortNumber opens the \\.\Scsi%u:
device, PathId/TargetId carry the routing. No RAID census, no LSILOGIC
sibling, no SMART auto-enable.

**WHY:** (1) The census is companion device-manager machinery (its
inputs are the array inventory, not a single disk handle) — the finding's
own fix plan prescribes the IOCTL_SCSI_GET_ADDRESS-based pattern.
(2) The auto-enable is a device write; this module is report-only by
standing policy (cr_health.h header contract). A disabled-SMART disk
reads health-unavailable rather than us flipping device state during a
read-only advisory pass. (3) The LSILOGIC route is a separate vendor
family the finding does not name (its acceptance is the Intel-RST CSMI
transport).

**RISK:** on RST arrays whose member routing genuinely requires the
census-sourced SAS address, our CC-0x19 request may be rejected where
the og's succeeds — the failure is clean (ERR_HEALTH_UNAVAILABLE, the
final fallback the finding mandates). Real-hardware validation on an
Intel RST host is pending for both implementations' claims (the finding
records this as inherent: UNVERIFIED-BY-EXECUTION on this hardware).
Recovery correctness is unaffected (health is advisory).

### D-19 — Recovery queue: collision naming, atomic writes, hash verify, resume (F11)

- **WHAT OG DOES:** Writes recovered files directly to destination via normal
  file APIs after `CreateDirectoryW` path reconstruction. Collision policy
  DECODED (N-074/N-065; DRWUI_decompiled.c:357116-357131, re-verified
  first-hand): the og retries colliding names as `%s_%d` / `%s_%d%s`
  (FID_conflict_sprintf_s, 0x410-wide buffers) with a 10,000-attempt cap
  — the earlier "no collision policy in any artifact" claim (AUDIT-UI
  §5.7) is superseded by the decompile walk.
- **WHAT WE DO:** Collision policy ALIGNED TO THE OG (N-065, s23):
  `name_1.ext` / `name_2.ext` (`%s_%d` extensionless / `%s_%d%s` with
  extension, counter from 1), `rename_attempts` default 10,000 (the og's
  cap), exhaustion FAILS the item with `ERR_DEST_NAME_EXHAUSTED` (the
  og's cap-break at DRWUI :357131) — the old `name (N).ext` shape, the
  64 cap and the og-foreign `_dup` fallback are removed. Kept supersets
  the og lacks (all registered): atomic temp+rename via DestIo, SHA-256
  verify with per-item mismatch accounting, resumable queue
  (`.part-<id>` suffix, resumed counting), zero-fill Skip policy, the
  corpus name sanitizer (`cr_safewriter.cpp`). mkdir acceptance is the
  SHCreateDirectoryExW 0/0xB7 contract in semantic form (WinDestIo's
  exist-as-directory tolerance IS the API's ERROR_ALREADY_EXISTS-on-
  existing behavior — the finding's ACCESS_DENIED premise mis-modeled
  the API; the component walk + tolerance replaced a stricter form that
  broke drive-letter destinations on the MSVC lane, test #227). Evidence:
  F11 registry; N-065's Resolution + first-hand decompile cites; tests
  (`CollisionNameUnderscoreFormat`, `CollisionExhaustionFailsTheItem`,
  `MkdirSemanticsMatchShCreateDirectoryEx`) per `docs/testing.md`.
- **WHY:** safety/correctness-fix (supersets) + og parity (collision).
- **RISK:** None negative.
- **VERDICT: ACCEPTED** (collision half now og-MATCH by N-065; the
  atomic-write/hash-verify/resume supersets remain registered).

### D-80 — the password-dictionary data posture: the corpus Dict converted as generated data (raw_signs precedent), user dictionaries loadable (N-064 convergence record — two parallel landings, first-landing-wins)

**Class:** convergence record (replaces this lane's original D-78
registration, renumbered per the errata convention — D-78 collided with
the N-075 progress register and the N-032 CSMI register in the same
window) · **Finding:** diff-to-fix N-064 (MINOR, FIXED — twice, convergent
landings `4c475ba` (cr::dict, DFX31, first) and `817740c` (cr::pwddict,
DFX34)) · **Date:** 2026-10-26

**THE CONVERGENCE:** two parallel lanes landed N-064 independently.
The DFX34 lane shipped an authored seed list + a loader for the og's
dictionary FORMAT, refusing to copy the og's 228-entry asset (its D-78).
The DFX31 lane (first-landing-wins) ships the corpus Dict converted to a
generated compiled table (scripts/gen_dict.py — the same
corpus-data-to-generated-source posture as raw_signs.json's 139-format
table) and absorbs the loader capability. Canonical state after the
reconciliation commit: **cr::dict** (the generated byte-faithful 228-entry
table + the parse grammar + the consent-gated attempt loop),
`--bitlocker-dict [path]` (bare = the shipped table; with a path = the
shipped table plus a user dictionary in the og's exact file format,
loaded through cr::dict::parse), the cr_ui_unlock_volume_with_dictionary
ABI export. The cr::pwddict module, its seed list, its CLI flag and its
tests are dropped cleanly; both Resolutions stay in the finding file
(append-only audit trail).

**WHY THE CANONICAL POSTURE (byte-faithful conversion, not an authored
seed):** (1) The repo's established precedent converts corpus data files
documented as ground truth into generated source — raw_signs.json (139
formats) since the carving wave. (2) The Dict entries are facts, not
creative expression: 228 common passwords (numeric runs, names, words —
"12345678", "iloveyou") that appear in every public leaked-credential
corpus; there is no EaseUS-authored expression to avoid copying. (3)
True parity requires the same list: an authored seed of "universal
patterns" approximates the capability; the converted table IS the
capability (the corpus byte-verifies the file — config-data-reference
:20). (4) The loader keeps the flexibility: a host can still supply any
og-format dictionary.

**RISK:** none beyond the capability itself; the F61 consent gate is
structural in both landings and unchanged in the union.

**VERDICT:** ACCEPTED (registered divergence: authored seed +
format-compatible loader vs the og's copied asset; mechanism parity
complete; D-78).

<!-- D-number errata (session-28 merge): this same-window landing
     produced duplicate D-78/D-79 registrations from two parallel lanes
     (the N-064 pwddict register and the N-032/M-020 pair below). Per
     the standing errata convention both stand; future sessions cite
     them by FINDING (N-064 / N-032 / M-020), not by bare D-number. -->

### D-102 — the FilterFolder junk-filter policy: applied at emission, sessions re-derive on resume; comparator mode undecoded (M-018, REGISTERED POLICY) (renumbered from a same-window D-82 collision - the errata convention)

The og pushes FilterFolder.ini's hidden sets per scan-partition at scan
setup (PushFolderFilter) and its presentation hides OS droppings,
metadata noise, localized "New Folder" names, System Volume Information
and BitLocker recovery artifacts. The corpus does NOT decode: (a) the
comparator (case mode; exact vs prefix — whether "$UsnJrnl:$J" matches
the "$UsnJrnl" entry), or (b) whether recover-all also skips the hidden
set (the artifact scopes the filter to "hide ... from results").

The core's registered choices: (1) matching is exact u16 equality with
ASCII-case-fold (the only sane Windows-name semantics — DESKTOP.INI is
desktop.ini; the fold is a no-op for the CJK entries); (2) the filter
applies at EMISSION — query rows, CLI tree and recover-all planning all
skip flagged nodes, so sessions, reports and recovery agree (the finding's
own open-question recommendation); (3) sessions do not persist the flag —
the pass re-runs idempotently on resume (names are stable); (4) children
of a filtered node are NOT filtered (the og hides entries, not their
contents). Junk counts surface as ScanStats.junk_filtered.

### D-103 — the base raw-signature table is compiled in (generated source), not an install-tree data file loaded at engine init (N-086, REGISTERED DIVERGENCE — posture, content-identical)

The og's RAWSearchDLL opens `raw_signs.json` from the install tree at
engine init (deep-dive:54-59 — ifstream → size → whole-file read →
tinyxml2 parse), making the 139-extension header/tail table field-
mutable without touching the binary. The core's equivalent is
`cr_rawsigns_data.cpp` — generated by `scripts/gen_rawsigns.py` from
the corpus reference and compiled into `libcr_core` (behavior on the
reference file is byte-identical; the generator is the single
KiB→byte conversion point and refuses zero-max entries).

The registered choice: **(a) compiled-in — reproducibility over field
mutability.** Rationale: (1) the runtime-extension surface the og's
mutability actually serves is covered better by the user-signature
path (cr_usersigns: the shipped 111-entry default set + `--user-signs`
replacement + the cr_ui ABI — M-048/M-065 lineage), which is a
stronger contract than an unsigned JSON swap; (2) a reproducible
binary carrying its own signature table cannot be silently
downgraded/tampered in the field (the SHA256SUMS conventions cover the
artifact, not a mutable install-tree file); (3) no corpus evidence
shows the og's field-mutability being exercised by any update channel
(the file is byte-stable across the corpus installs). If a field-
updatable signs pack ever becomes a product requirement, option (b)
(ship the table as a data asset with a SHA-pinned fallback to the
compiled copy) is the tracked design — the finding's own plan records
its test pair (`RawSignsDataFileOverridesCompiled` +
`RawSignsCorruptFileFallsBack`).

### D-100 — carve registry consumption: direct compile-time read, no per-scan cross-DLL re-clone (N-033, REGISTERED DIVERGENCE — structural, content-identical) (renumbered from a same-window D-80 collision - the errata convention)

The og's FS SearchDLLs import RAWSearchDLL.dll at runtime and clone its
master analyser registry into their own state — `LoadLibraryA
("RAWSearchDLL.dll")` + `GetProcAddress("EUGetAnalyserList"/"Map"/
"Multimap")` + a full std::map/multimap/list tree-copy with leftmost/
rightmost fixups (FATSearchDLL_decompiled.c :13872-13977, into members
+0xb8/+0xd0/+0xe0), and the SAME sequence again per scan (:29290-29361)
— because the og is a multi-DLL process and the registry lives in
another module. The core is one static library: `carve::registry()` is a
compile-time immutable table read directly at every consumption site (the
F8 gate + the scan loop), so there is nothing to clone and nothing that
can drift between the scan's start and its per-engine fetches — the same
content by construction, one less failure surface (a torn LoadLibrary or
a version-skewed RAWSearchDLL is an og-only hazard). The capability
question the clone serves ("which formats can we carve") is answered by
the ABI enumeration (`cr_ui_carve_format_count/info`, ABI-CONTRACT §7a)
and its CLI projection (`rrecov list-carve-formats`, landed with this
register). Pinned by `CrUi.CarveFormatRegistryEnumeration`.

### D-85 — device enumeration scope: the PhysicalDrive namespace is the engine's device truth; no WMI/SetupAPI/VDS/StorageSpaces/FunctionDiscovery fan-in (N-071, REGISTERED DIVERGENCE — scope)

**Class:** registered divergence (enumeration scope) · **Finding:**
diff-to-fix N-071 (MINOR, resolved by registration + the busy-disk hardening) · **Date:** 2026-10-26

**WHAT OG DOES:** the MOM device manager fans device discovery across
five stacks — WMI (MSFT_* namespaces), SetupAPI device interfaces, VDS
(dynamic disks), Storage Spaces, and Function Discovery — merged into
the device list the UI presents (corpus
`re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md`
:57-59; QA-session forensics census :13-29).

**WHAT WE DO:** `cr::win::enumerate_disks` walks `\\.\PhysicalDrive0..N`
(gaps skipped; geometry/descriptor IOCTLs per disk; tables through the
pure core) and `enumerate_volumes` walks the drive-letter namespace
(FIXED/REMOVABLE) with volume-disk extents. No second discovery stack.

**WHY:** (1) The five-stack fan-in is MOM device-manager/UI machinery
its consumer is the product's device-selection dialog, not the recovery
engine; every stack it merges still resolves to the same
PhysicalDrive/volume objects the engine opens. (2) The finding's own
open question — whether Storage Spaces pool members genuinely fail to
surface as `\\.\PhysicalDriveN` — is UNVERIFIED-BY-EXECUTION (no Storage
Spaces host in any CI fleet of this project; the corpus sentence is a
design description, not an observed miss). Building a SetupAPI merge
pass on an unobserved miss-case would be speculative surface with no
executable proof available in this environment — the opposite of this
project's no-hallucinated-parity rule. (3) The load-bearing behavioral
difference — a busy disk held by another process (backup/AV/imaging)
must not vanish from the device list, which the og's stacks get for
free because they enumerate without opening — IS ported: query_disk now
retries a 0-access metadata-only open on ERROR_ACCESS_DENIED /
ERROR_SHARING_VIOLATION (the FILE_ANY_ACCESS geometry/descriptor IOCTLs
answer on that handle; the disk is listed with `busy=true`, scans
re-open when the holder releases). A disk that cannot even be
metadata-opened stays absent — the honest limit.

**RISK:** devices visible only through the exotic stacks (none observed
in this project's fleet) would not be listed. REVISIT TRIGGER: any
field report of a disk visible in Windows Disk Management but absent
from `rrecov list` / `cr_ui_enumerate_disks` reopens this row with a
live probe (the SetupAPI disk-interface pass is the designed extension
point — enumerate_via_setupapi() merging dedup'd by device instance
path per the finding's fix plan option b).

### D-87 — the damage-audit check phase: three bounded divergences from the og's FileSystemCheck (M-041, REGISTERED; renumbered from a D-76 collision with the parallel lanes' N-063 register — the errata convention)

**CONTEXT:** M-041 landed the og's chkdsk-grade audit layer
(`cr/check/cr_fscheck.h/.cpp`, corpus ghidra-mo/FileSystemCheck_decompiled.c,
every load-bearing gate decompile-cited in the header). Three points could
not be decoded to byte-parity and are registered here rather than guessed.

**(a) The $Bitmap slack term is not implemented.** The og's structural
check (checkntfs.cpp :3866-3888, inside FUN_180005590) fires 0x100b when
`bitmap_bits < mft_record_count` OR when
`mft_record_size × (CCheckNTFSVolume+0x30) × 8 <= bitmap_bits − mft_record_count`.
The first half is implemented exactly. The second half's constant lives in
an object-field layout the decompile does not resolve (+0x30 is not read
anywhere else in the module): every well-formed volume stores exactly
one bit per cluster, and no fixed slack constant can both fire on
corruption and stay silent on healthy volumes under the field's plausible
readings — the term is layout-undecoded and deliberately NOT approximated.

**(b) CheckIssue carries superset context the og's callback does not.** The
og's FAT-path 0x1003 (orphan sweep) reports `(code, 0)`; the NTFS
cross-check reports `(code, LCN)` and the record number only in its log
line. The core's `CheckIssue` carries the orphan CLUSTER, the NTFS LCN,
the claiming record number, and a path hint for the dir audit. Hosts
consuming the report get strictly more context; the numeric codes and
their meanings are unchanged.

**(c) A partial $Bitmap run load fails the load (0x100b), never a
zero-tail audit.** The og's LoadVolumeBitmap allocates the declared
$DATA size and reads the runs; a short/failed read leaves the og's log
line and a bitmap whose tail content depends on its allocator. The core
refuses to cross-check against a partially-loaded bitmap (the zero-filled
tail would read as "every cluster free" and fire false 0x1003 verdicts) —
the structural 0x100b is reported instead. The og's
"Call LoadVolumeBitmap() failed!" path is the closest analogue.

**RISK:** none user-observable beyond the og's own coverage; (a) may miss
a corruption class the og's slack term catches (undecodable which), (b) is
additive, (c) is stricter-than-og on a pathological shape. REVISIT
TRIGGER: any future decode of CCheckNTFSVolume's field layout (+0x30)
reopens (a) with the exact constant.

### D-86 — the logging seam: one host sink, no cross-process transport (N-048, REGISTERED DIVERGENCE)

The og runs a three-layer logging architecture: Log.mo's EPMLOG.log
(64 KiB cross-process shared staging + named mutex + 10 MB
truncate-rollover, line format `[%Y-%m-%d %H:%M:%S:%ms] (T:%d/P:%d)
msg`), PubLog's 2 MB ring for the UI-family binaries, and Logger.dll's
async queue + worker with keep-second-half compaction
(Log_decompiled.c :159-176/:193/:604-621/:699, PubLog_decompiled.c
:119-135, Logger_decompiled.c :1630-1696/:1962-2015). The core is a
SINGLE-PROCESS library: there is no second process to share a mapping
with, no UI-family binary set for the ring, and an async worker queue
buys nothing at the core's line volumes (diagnostic brackets, not
traces). The seam is therefore the SINK, not the transport
(`cr::log::set_sink` — default NULL = silent, zero formatting
overhead; serialized synchronous deliveries; the host owns every
policy the og baked into its transports: file placement, rollover,
compaction, async). The og's per-line timestamp/process decoration is
its transports' job; the core emits the message text. Pinned by
`Orchestrator.LogSilentByDefault` + `Orchestrator.LogSinkReceivesScanBrackets`.

### D-88 — NAS/remote scanning: excluded from the core's device scope; the EuMrx redirector is host-layer surface (M-034, REGISTERED DIVERGENCE — scope)

**Class:** registered divergence (scope exclusion) · **Finding:**
diff-to-fix M-034 (MAJOR, resolved by registration — the disposition
the finding demanded) · **Date:** 2026-10-26

**WHAT OG DOES:** the product scans NAS shares through a remote-device
layer — `CRemoteDevice`/`CDoScanRemoteDevice` (rtti_classes.json,
DRWUI census) wrapping an EuMrx redirector that mounts shares as
drive paths (`X:\EuShare.com\…`, architecture/05-scan-engine-
architecture.md §8; DRWUI-exe--main-ui-architecture.md :214-215
documents the EuMrx/NAS scan flow).

**WHAT WE DO:** the five local-device storage layers only (device,
slice, cached, retry, devmap — physical, slice-bounded, cached,
retrying, and image views). No remote transport, no EuMrx equivalent,
no share-mounting. A HOST may layer an SMB-backed `IStorageDevice` —
the device contract (cr_device.h) is intentionally transport-agnostic,
and the win layer's volume path (`\\.\C:`) reads mapped network drives
through the OS redirector for file-level recovery.

**WHY:** (1) The EuMrx interop is og-internal plumbing — the finding's
own fix plan records that "the device contract is the parity surface",
not the redirector. (2) The corpus proves the scan-path plumbing only
(RTTI + architecture docs); whether the og's NAS scan actually
recovers deleted files from shares — server-side undelete semantics
differ fundamentally from raw block access — is a product-level
question ~~the corpus does not answer~~ **[RE-ADJUDICATED 2026-10-28, N-119:
the corpus NOW answers it — the EuShare artifacts decode the semantics:
the SSH-root agent pushes to `/tmp/EaseUS` on the NAS box and runs the
ON-BOX BITMAP-BASED SCAN, serving results back
(`re-workspace/artifacts/EuShare/eumrx-nas-architecture.md:10-21` — the
3-layer model). Deleted-file yield on shares therefore rides the agent's
on-box block access, not a client-side raw-SMB path. The exclusion below
now rests on the host-layer/product-scope rationale alone — leg (2) of
the WHY is retired.]** (the finding's own open question).
(3) A network stack is not pure-core surface: the core deliberately
owns no OS facilities below the device contract, and SMB client
behavior (credentials, timeouts, offline caching) is host policy. (4)
The capability was never previously dispositioned anywhere — this row
is the registration the finding existed to force.

**RISK:** users needing NAS recovery must go through the host layer's
mapped-drive path (file-level, whatever the OS redirector exposes) —
no raw-block carving over SMB in OUR core; the og's own answer is the
on-box agent model (the EuShare decode above), which is a product
deployment shape, not an engine contract. REVISIT TRIGGER: ~~a corpus
decode of the EuMrx scan's actual recovery semantics (deleted-file
yield on shares) would reopen the row~~ **[FIRED and adjudicated
2026-10-28 (N-119): the EuShare decode answered the semantics; the
exclusion STANDS on the scope rationale — re-filed only if the product
ever wants an on-box agent architecture.]**

### D-89 — the Btrfs re-scan cache: our own format, caller-supplied directory, OFF by default (N-072, REGISTERED DIVERGENCE)

**OG:** BtrfsSearchDLL persists a relative-path `BtrfsCache.txt` beside the
engine DLL to speed re-scans of the same volume — subvolume/chunk trees
cached (artifacts/SearchDLL/remaining-engines artifact :66-67). STRING
EVIDENCE ONLY: the og's cache format, keying and invalidation rule are not
decoded anywhere in the corpus.

**CORE (N-072):** the mechanism is ported with three deliberate deltas, each
required by the finding's own fix plan: (1) the format is OUR OWN — a
versioned binary blob ("CRBTC001", zlib CRC32, fsid + generation strict
match-or-discard, hostile-blob caps, 1 MiB size bound) documented in
`cr_btrfs_cache.h`; no og format exists to match, so none is claimed. (2)
The placement is a CALLER-SUPPLIED cache directory (the og's beside-the-DLL
write breaks under Program Files / readonly media / multi-user contexts —
the finding's own hazard note). (3) The cache is OFF by default (the og
writes unconditionally); a host opts in per scan with a directory + IO seam.
What is cached: the derived chunk map + the chunk-root/root-tree bytenrs —
exactly the re-scan's repeated derivations; the trees themselves are NEVER
cached (every scan re-reads and re-validates them — recovery-honest).

**RISK:** a warm scan adopts the cached chunk map when fsid AND generation
match; a volume whose generation advanced (any live write) invalidates
cleanly (re-derive + rewrite). A corrupt/foreign/truncated blob is
discarded, never adopted.

**VERDICT:** ACCEPTED (registered divergence: own format + placement +
default-off vs the og's undecoded beside-the-DLL write; the capability gap
N-072 filed is closed).

### D-90 — boot.ini ARC-path repair skipped; post-op ChkDsk.exe delegation is UI-layer (M-040 stages c/d, REGISTERED DISPOSITIONS)

**OG (M-040 corpus set):** Fixup.mo carries a boot.ini ARC parser/rewriter
(:2605+, :2999-3227) with \ArcName resolution (:2067-2193), and MainModule's
undo chain runs `ChkDsk.exe %C: /F /X` synchronously (MM:83206) beside the
BCD restore.

**CORE:** boot.ini repair is NOT ported: the target envelope is Win7 SP1
onward (the Win7-inclusive import gate), and Win7 boots through BCD — a
legacy ARC-path repair has no recovery-relevant consumer in the envelope
(the finding's own fix-plan note, adopted verbatim as the registered
rationale). The chkdsk delegation stays OUT OF CORE: spawning an external
binary is the host's job (the og itself spawns it from the host-side undo
chain); the core's contract ends at the byte-exact undo record the host
acts on. Stage (a) — the BCD offset rewriter — IS ported (cr/bcdfix, the
consent-gated plan/execute/undo core); stage (b) — the UEFI NVRAM updater
— is the recorded next step on the finding.


**N-122 addendum — the NVRAM Boot#### create/delete halves are deliberately
excluded (the v1-scope note now a registered row):** the og's EFIBoot module
carries a create/patch/delete state machine for Boot#### entries
(EFIBoot_decompiled.c :251-693, :3405-3480, :4130-4273 — the same decode
M-040's corpus block cites): a system whose Boot#### variable for the target
OS was DESTROYED gets a created entry (the next free slot, the og's entry
template, BootOrder updated); delete = the walk-then-remove path. The core's
nvramfix PATCHES existing entries only and its walk STOPS on a missing
Boot#### variable (pinned by NvramFix.MissingBootVariableStopsTheWalk —
the honest stop, not a silent skip). The exclusion rationale: the create
path's entry-template details (the og's exact friendly-description string,
the :3405-3480 slot-allocation walk) need one dedicated decode pass before
implementation; the patch-only v1 covers the dominant corrupted-entry case;
the missing-entry case fails loudly with a precise diagnostic the host can
act on. **Next concrete step:** decode FUN at :3405-3480 (the create walk)
+ the template strings, then land create/delete with the walk's
stop-on-missing replaced by create-on-missing (the existing test's
expectation flips with it). Registered per N-122's fix-plan step 1.
**N-121 — the ESP auto-mount step (CUpdateGptSystemBootPartition's
MountGptPartition/UnmountGptPartition) is a REGISTERED host contract, not a
core implementation.** The og's boot-repair chain finds and mounts the EFI
System Partition itself so the boot-config update reaches
\EFI\MICROSOFT\BOOT\BCD on any recovered system (mo-module-map.md:56-57;
the InitLayout/Mount/Unmount trace strings, DeviceManager.mo :321-332). The
core's `rrecov apply --boot-fix-bcd <file>` requires the HOST to supply the
BCD path — a recovered system whose ESP has no drive letter (the common
case after lost-partition recovery) cannot complete the repair through the
core until the host mounts the ESP. REGISTERED CONTRACT: the host discovers
the ESP by type GUID C12A7328-F81F-11D2-BA4B-00A0C93EC93B among the
recovered/applied partitions, mounts it (the elevation seam), passes the
path, and unmounts after (the og's own UnmountGptPartition implies the
pairing — exception-safe: unmount even on failure). RATIONALE: the corpus
evidence for the mount semantics is trace-string-level (the class name +
method strings; no artifact decodes the mount behavior — the finding's own
open question), so the core implementation needs the DeviceManager
:49956 decode pass first; the host contract covers the flow today with the
requirement documented where the CLI's help and ABI-CONTRACT describe the
boot-fix stages. NEXT STEP: decode DeviceManager_decompiled.c:49956 (the
CUpdateGptSystemBootPartition behavioral pass), then land the discovery +
mount/unmount pair in the win layer with the exception-safe pairing test
(a GPT fixture with an unlettered ESP: BootFix.EspAutoMountReachesBcd /
BootFix.EspUnmountedAfterFix — the finding's test plan). VERDICT: ACCEPTED
(documented host contract + registered follow-up; the reachability gap is
no longer silent — the CLI help and the boot-repair module row state the
requirement).
**RISK:** a pre-Vista boot.ini-only system (outside the envelope) would
lack the legacy repair — out of posture by construction. A host wanting
the og's exact undo chain spawns chkdsk itself from the undo record.

**VERDICT:** ACCEPTED (registered dispositions: (c) skippable per the
Win7-boots-BCD envelope; (d) UI-layer responsibility; stages (a) landed +
(b) tracked on M-040).

## U. Driver-only session (2026-10-27) — the architecture divergences

Evidence base: REFERENCE-DRIVER-BEHAVIOR.md (the corpus's own picture of the
og's driver posture), CORE-DISK-ACCESS.md (the pre-change fallback map),
PLAN-DRIVER-ONLY.md (the removal plan), DRIVER-REQUIRED.md (the new contract).
The user mandate for this session is the product decision behind every row.

**D-DRV-1 — the direct-access fallback is REMOVED (data path is driver-only).**
REFERENCE: the og's duality is environment-gated, not failure-gated
(Device.mo FUN_1800577e0; a failed EPRDTDRV open is a silent NULL device — no
site retries the other transport), and our core had ADDED a failure-gated
fallback superset on top (CORE-DISK-ACCESS F1-F12). CORE: the superset is
deleted; every driver-requiring open refuses with its ERR_DRIVER_* class
(0x500B family); the CLI refuses with exit 4 + per-condition stderr before any
other output. RATIONALE: the product does not market a non-driver path, and
silent degradation was the direct cause of the "product installed, nothing
works" failure the user experienced. RISK: a user who could previously scan an
unlocked, healthy volume without the driver now gets a refusal — the refusal
is actionable (install/start/elevate/update/reinstall) and the driver ships in
the installer. VERDICT: ACCEPTED (deliberate divergence; mandate §2).

**D-DRV-2 — the WinPE gate is REMOVED (driver required in WinPE too).**
REFERENCE: the og opens \\.\PhysicalDrive%d directly inside WinPE by design
(cr_winenv.h pins; M-030). CORE: no PE exemption — the driver is the
prerequisite everywhere; rda-driver ships scripts/winpe/regdrv-rda.reg so PE
media can register it (DRIVER-RELEASE §6). RATIONALE: a driverless PE mode is
exactly the silent-degradation posture the product is eliminating; WinPE
signing is relaxed for registered drivers. VERDICT: ACCEPTED (deliberate
divergence; mandate §2 "no user-visible phase proceeds without the driver").

**D-DRV-3 — the driver-status surface is ADDED (product scope, not parity).**
REFERENCE: the og exposes NO driver-status surface to its UI — the
service-start result is discarded (PartitionRecovery_decompiled.c:2611), no
version negotiation, no per-facet queries (REFERENCE-DRIVER-BEHAVIOR §3;
REFERENCE-API-GAP R-84 + corpus-silence list). CORE: present/loaded/started/
authorized/healthy/version facets via win::query_driver_status +
cr_ui_driver_status + the `rrecov driver` verb (exit 0/4). RATIONALE: the
session mandate requires the per-facet surface; a product that refuses must
also say WHY, which the og never did. This is a truthfulness superset in the
D-24/ADR-5 family. VERDICT: ACCEPTED (product-scope divergence, mandate §3).

**D-DRV-4 — the core does NOT demand-start the driver service.**
REFERENCE: the og calls StartServiceA at every enumeration refresh and treats
ERROR_SERVICE_ALREADY_RUNNING as success (PartitionRecovery_decompiled.c:
2620-2622). CORE: a stopped service is a NotStarted refusal naming `sc start
RDA` / the installer. RATIONALE: the installer owns service lifecycle
(DRIVER-REQUIRED §5); surprise service starts from a user-mode core are a
privilege and state-management hazard the product explicitly declines.
VERDICT: ACCEPTED (deliberate divergence; mandate §2's refusal contract).

**D-121 — the drive-disconnect retry window lives in the HOST, not the engine (N-125).**
REFERENCE: the og detects mid-scan target removal and offers a
reconnect-then-continue flow ("Drive Disconnected - Unable to continue
scanning. Reconnect your drive and try again." Retry/Cancel; UIUX/08:39-47;
the same triad for NAS/Linux/Cloud offline) - the dialog is the UI layer's;
no decoded engine error code drives it (the finding's own open question).
CORE: the transport classifies the device-gone Win32 GLE family
(DEV_NOT_EXIST/NOT_READY/MEDIA_CHANGED/NO_MEDIA_IN_DRIVE/DEVICE_REMOVED)
as ERR_DEVICE_DISCONNECTED 0x50020007 - a distinct code, never a
read-failure storm - and the documented reattach flow is the
session-checkpoint resume (the machinery exists): the host detects the
code, waits for reattach in its own loop, then `resume` continues from
the checkpoint; the CLI's scan verb maps it to exit 5 with the resume
instruction. RATIONALE: the engine retry window (the og's dialog loop)
is UI-layer responsibility in the core's layering; a distinct code + the
resume machinery covers the flow without an in-engine wait state.
VERDICT: ACCEPTED (the lighter engine posture is the documented contract;
media errors stay ERR_READ_FAILED - the classification is precise).

**D-DRV-6 — GET_DISK_CONTEXT_MODE (0x60002408) is deliberately NOT issued (N-096).**
REFERENCE: the og's user-mode client issues exactly one identity IOCTL — the
per-open GET_DISK_CONTEXT_MODE query (Device_decompiled.c:52598; a 4-byte-out
mode probe: 0 = the chosen layer's driver is \Driver\Disk, 1 otherwise) —
and keeps the result as an informational per-handle record; no decoded branch
consumes it. CORE: the transport opens the grammar path and reads (cr_rda.h
carries all three identity constants at wire parity; zero client-side
issuance — the ARCH-DEAD-2 register's finding). RATIONALE: the og's use is
informational only (no behavioral dependency decoded anywhere in the
corpus); the core's transport already resolves the layer stack through
geometry probing; issuing the query would add a per-open round trip whose
result nothing consumes. The deliberate lighter posture is registered here
per N-096's fix-plan option 2, and the DRIVER-UTILIZATION row's wording is
downgraded from MATCH to the wire-matched/issuance-omitted truth.
VERDICT: ACCEPTED (client-side non-issuance of an informational-only query;
wire parity intact — the constants and their semantics are documented in
cr_rda.h for any future consumer).

**D-DRV-5 — GET_INFO/GET_VERSION are now issued (the wire contract goes live).**
REFERENCE: the og issues only GetDiskContextMode (0x60002408) — the extended
RDA IOCTLs postdate the reference binaries (DRIVER-CONTRACT §5; the core's own
cr_rda.h documented them as documentation-only, "no core code issues them",
N-041). CORE: query_driver_status issues GET_INFO (0x6002440) + GET_VERSION
(0x6002448) on the global control-device handle; the version-compatibility
contract is major-match/minor-informative. RATIONALE: the status surface's
version/healthy facets need them; the driver serves both on global handles.
VERDICT: ACCEPTED (wire-contract activation, our own driver's documented
surface — no og behavior is contradicted because the og never had them).

**D-DRV-6 — the read-truth completion gate (fake scans refuse; the og
fake-completes).**
REFERENCE: on a fully unreadable volume the og's engines log read errors and the
scan still "completes" with an empty result (the CErrorManager surfaces errors
to the UI, but the scan verdict is completion, not refusal); our pre-fix core
did the same class of thing WORSE: Skip-mode zero-fill made every failing read
look like an empty region, the Unknown-family cascade (blind probe -> no
engines -> instant stage completes -> carve over zero-filled windows) ended in
"scan complete: 0 file(s)" + exit 0 with the error truth only in the log. CORE
(this fix): ScanStats.bytes_unreadable (the error map's coverage) +
probe_unreadable (family=Unknown BECAUSE the header was unreadable, not
because a filesystem was absent); a scan whose coverage spans (essentially)
the whole slice with nothing found REFUSES with ERR_READ_FAILED (exit 1, the
coverage named in the CLI output); partial damage keeps the exact og-parity
salvage semantics with the coverage surfaced ALWAYS (CLI summary line + JSON
fields + ABI stats fields + log). RATIONALE: the user's report - "the scan
never works and starts from 40% and shows no progress, it looks like fake
scanning" - is precisely this class: the quick stage's 40% weight lands
instantly with zero real work when the family is Unknown, and the product
appeared alive while reading nothing. The mandate's rule: the product is
either working or explicitly refusing. VERDICT: ACCEPTED (deliberate
divergence; truthfulness superset in the D-24/ADR-5 family).

### D-104 — the user-signature carve channel + the shipped default set are a product superset: the og's scan engine has NO user-sign input (OQ-SA-3 re-resolution, M-065 37c48c6, REGISTERED SUPERSET — engine behavior)
**S-055/S-056 landing notes (2026-10-29, session-51 severe-first lane):** the
channel's in-engine ordering is now the one D-104's text always described —
grammar claims first, the built-in raw-sign table second, the user-sign layer
LAST (built-in-first at collisions; the 8 default-set overlap formats
amr/cap/class/cpl/flac/msf/pdb/wpl return bounded built-in candidates). The
format toggle gates the user layer like every other carve source (the og's
FUN_180006b30 removal-loop semantics: a disabled extension's signs never fire;
other extensions' signs still can). One further core-side rule with NO og
analog (the og has no user layer to mirror): a grammar-REJECTED start is not
reclaimed by the user-sign arm — the strongest layer's negative verdict
settles the position for the weaker header-only layer (no-reclaim), while the
built-in table still probes grammar-rejected positions (the og's layers claim
by emission only). Rejection tracking is memory-bounded at 2^20 entries
(kMaxTrackedRejections) — past the bound the supplementary suppression
degrades gracefully on garbage-saturated media; emitted_at and the extent-dedup
contracts are unaffected.


**Class:** deliberate superset (carve engine behavior) · **Findings:** OQ-SA-3
(re-resolved, scan-engine lane, 2026-10-28), M-065 (landed 37c48c6 before the
re-resolution; superset registration retro-active) · **Date:** 2026-10-28

**WHAT OG DOES:** the og's RAW scan engine has no user-signature input at
all. Its only runtime-loaded signature data is the install-tree
`raw_signs.json` (139 ext, read-only ifstream at engine init,
RAWSearchDLL :23836-23850) and `config/RawConfig.ini` (:13133). The shipped
`EUUserData` (111 entries) never reaches the engine: DRWUI reads it
(`%s/EUUserData`, :1005387) into `CHKRepairFileSign` (RTTI-confirmed repair
chain class; MatchedFile/NotMatchedFile checker :721520-721680) — file
signature checking for the repair flow. The `GetRawUserInfoMap` export
returns the singleton's per-scan carve-statistics map (insert-or-increment
by the analysers' detection paths, FUN_18000ed00 :11376-11525; cleared at
every searcher init, FUN_18000beb0 :9044-9051; read one-way post-scan by
DRWUI's "Trigger_Raw"/"FileSystem" report builder :227538-:227625) —
output, not input.

**WHAT CORE DOES:** the user-signature surface (cr_usersigns, M-048; caps
S-039) installs user-taught signs into the carve registry — capability-
creating by construction — and since 37c48c6 the release ships the og's own
111-entry/190-sign DB as `data/userdata-defaults.json`. Load posture (I-036's
precision registration): the CLI app loads the executable-adjacent default at
startup when no explicit `--user-signs` is given (rrecov.cpp, the single load
site); the LIBRARY default is off — an engine, orchestrator, or cr_ui ABI host
gets user signs only via the explicit `cr_ui_register_user_signs` op. D-104's
prior "loaded at engine init" phrasing overstated the surface.

**WHY REGISTERED AS A SUPERSET, NOT A PARITY GAP:** the landed behavior
exceeds the og's scan engine (the og's engine cannot consume user signs at
all). The original M-065 framing ("the og consults the DB at scan setup")
was based on the pre-re-resolution reading of the :227538 consumer and is
superseded (see the OQ-SA-3 re-resolution + CORPUS-ERRATA E-A/E-B). The
shipped default set is retained as a deliberate product enhancement: it
uses the og's own shipped data through the core's own superset channel, it
is opt-out-able (explicit `--user-signs` replaces it entirely), and the
carve pass remains gated by the same format-toggle/occupancy/bitmap rules
as every other carve source. The og-parity statement for the scan engine is:
default-off user signs; the core's statement is: default-on with the og's
own data — both recorded here so no parity table re-derives the wrong
"matching behavior" claim.

**FOLLOW-UPS:** the og's true EUUserData consumer (CHK repair chain) is out
of the core's product scope (file-repair lane; handoff logged); the corpus
artifacts carry the correction (CORPUS-ERRATA E-A/E-B + the corpus's
CORRECTIONS.md #9).

## V. The parity-close fix session (2026-10-29)

### D-105 — the auto-save enable surface is a config DIRECTORY field, not the og's config-singleton byte flag (S-044 resolution rider, REGISTERED DIVERGENCE — functionally equivalent enable semantics)

**WHAT OG DOES:** the auto-save entry gate is a one-byte flag on a global
config singleton (`FUN_1400b5270` :165951-5 = `return *(u8*)(param_1+0x10)`,
0 = enabled; read at FUN_1402857c0 :442822-24). The save PATH is composed at
each call site; no directory field exists in the scan context (the
`"SaveResultPath = %S"` string is a trace log).

**WHAT CORE DOES:** `cr_ui_scan_config::autosave_dir_utf8` (cr_ui.h) carries
the destination directory; empty = off (zero-init). The trigger-site gates
landed with S-044 (results-found / not-import / not-already-saved) are
behavioral parity and are NOT part of this delta.

**WHY REGISTERED:** the enable/destination semantics are functionally
equivalent (off by default, one destination per run, host-settable), but the
SHAPE differs (composed-at-call-site vs carried-in-config). Rewiring the
core to a byte flag + composed path would change the public ABI surface for
zero behavioral gain; the directory form is also strictly more expressive
for the CLI (`--autosave-dir`). Verdict: ACCEPTED.

### D-106 — the auto-save Folder-scan naming variant (`Folder` suffix) is unimplementable until a folder-scan mode exists (S-044/N-105 deferral, REGISTERED GAP — surface absent on both engine sides)

**WHAT OG DOES:** the auto-save volume-name builder appends `L"Folder"`
when `vtable+0x110` is set (:442883-886) — folder scans (the og's
folder-recovery mode) name their auto sessions `auto_<vol>Folder_<ts>`.

**WHAT CORE DOES:** the core has no folder-scan mode (device/image targets
only), so the naming variant has no trigger surface. N-105 owns the row.

**WHY REGISTERED:** recorded so a future folder-scan landing knows the
naming contract. Verdict: NEEDS-WORK (only when folder-scan lands).

### D-107 — the offline-OS profile is a core-only C++ API, not an ABI export (M-082 resolution rider, REGISTERED LAYERING DECISION)

**WHAT OG DOES:** DeviceManager.mo (live-loaded by name) mounts a candidate
partition's `<root>\Windows\System32\config\{SOFTWARE,SYSTEM}` through its
in-house HMG classes and fills the 0x418-byte CPartitionBootInfo record —
the 8 CurrentVersion identity fields, the drive-letter-gated SystemRoot,
the `\MountedDevices\DosDevices\<x>:` 12-byte system-partition location,
the `\Select\Current` ControlSet resolution (`%03d` rendering,
DAT_18007d37c), and the ProductOptions classification
(DeviceManager_decompiled.c :62034-62330).

**WHAT CORE DOES:** `reghive::offlineos::profile_os()` (cr_offlineos.h,
M-082) reproduces the walk read-only over the S-014 regf parser — same
fields, same gates, same Select-awareness (NOT the FVE harvest's
hardcoded 001). It is a C++ API only: no cr_ui export yet.

**WHY REGISTERED:** the M-082 fix plan's explicit decision — "Expose as a
small C++ API first; ABI only when a host-facing flow consumes it." The
recorded follow-up consumer is boot-fix auto-locate (cr_bootfix taking the
BCD path from the profile's MountedDevices location instead of requiring
the caller's path); the ABI surface lands with that flow, not before.
Verdict: ACCEPTED (layering-faithful staging, not a silent gap — the
capability decision the finding's step 1 demanded is YES and is recorded
here).

### D-132 — the TS null-packet posture (renumbered from its born-collided D-107 seat at the N-207 repair: the M-082 rider keeps D-107): the core tolerates null-packet-headed streams on both TS rows; the og's CM2TS probe rejects them (S-047 post-landing addendum, REGISTERED SUPERSET — recall over strictness)

**WHAT OG DOES:** the CM2TS probe (FUN_180032670 :38556-38619) rejects candidates whose first two packets carry all-0xFF payload qwords (`param_2[1] == -1` or the second packet's +8 qword == -1) — even though the og's own size walk (FUN_180032460 :38472-75) marks and TOLERATES all-0xFF null packets mid-run, and the CMTS chain (the chunk handler FUN_18002e430) has no payload gate at all.

**WHAT CORE DOES:** both FMT_TS and FMT_M2TS implement the walk's tolerance uniformly — null packets anywhere in the run (including the head) keep the run intact; the "+null" evidence marker records their presence.

**WHY REGISTERED AS A SUPERSET:** broadcast and camcorder TS streams commonly begin with null-padding packets; the og's own walk semantics accept them mid-run, so the probe's head-reject loses real files the CMTS chain would carve. The core's uniform tolerance is strictly more recalling on the same evidence strength (0x47-synced strided packets ≥ 0x5000). Verdict: ACCEPTED.

### D-108 — the session-load progress model: stage permille + a 0xFFFFFFFF overall sentinel, not the og's fixed 40→65 % overall window (N-091/W1b registration, REGISTERED DIVERGENCE — recomputable)

**WHAT OG DOES:** the restore mirror computes the overall bar as
`40 + 50·done/(total+done)` over the load window (rsf-format--scan-session.md
:344-376 — the 40→65 % sub-window of the overall restore progress; per-child
`IsCancelled` polls).

**WHAT CORE DOES:** `CR_EVT_SESSION_LOAD_PROGRESS` (W1b, ce6bcb0) carries the
load's OWN stage_permille (0-1000) with `overall_permille = 0xFFFFFFFF` as the
"not applicable" sentinel, PLUS `bytes_done`/`bytes_total` fields.

**WHY REGISTERED:** a host implementing the og's bar from the stage permille
alone cannot reproduce the fixed 40-65 % window — but the byte fields permit
exact host-side recomputation (`40 + 50·bytes_done/(bytes_total+bytes_done)`),
which is the documented composition route. Rewiring the event to the og's
fixed window would bake a UI layout decision into the engine event. Verdict:
ACCEPTED (the recomputation route is documented; ABI-CONTRACT §7g carries the
fields).

### D-109 — thumbnails decode IN-ENGINE (a superset of the og's UI-side pipeline) (N-091 registration, I-015 lineage, REGISTERED SUPERSET — engine capability)

**WHAT OG DOES:** all thumbnail work is UI-process — GDI+
`GdipGetImageThumbnail` at 256 px on the still-image family, the MediaGraph
agent for video key-frames, EURawImg/LibRaw for RAW, heif.dll embedded-first
for HEIC; the engines never decode an image (I-015/D-59h's register).

**WHAT CORE DOES:** `cr_thumbnail` (05e6786) decodes in-process through the
bounded PreviewReader (JPEG/PNG/BMP/GIF/TIFF native + PSD/PNM/HDR via stb;
dims-first bomb guard; named refusals for the deferred families; EXIF
orientation applied per S-043; both-axis scaling per I-028) — an engine-side
capability the og does not have. The cr_ui ABI exposure is the open M-102
handoff; the format matrix + cache-scope decisions live in
modules/preview.md + cr_thumbnail.h's contract.

**WHY REGISTERED:** the in-process doctrine deliberately places the decode
where the PreviewReader already bounds every read; a UI-side split would fork
the bounded-read guarantee. Verdict: ACCEPTED SUPERSET (registered here per
N-091 — previously scattered across OGI C.2 + preview.md only).

### D-110 — the auto-session-name sanitizer is a deliberate superset: backslash + the C0 control range beyond the og's 8 forbidden characters (N-091 addendum registration, REGISTERED SUPERSET)

**WHAT OG DOES:** `FUN_140286930` replaces exactly 8 characters in the volume
label for the auto-session filename — `/ * : ? " < > |` — with no backslash
and no control-range handling (rsf-format--scan-session.md:450-451).

**WHAT CORE DOES:** `auto_session_name` (cr_crsf.cpp) additionally replaces
backslash and every C0 control byte — the full Windows name-forbidden set.

**WHY REGISTERED:** a label containing a backslash or a control byte produces
a DIFFERENT (but always VALID) core filename than the og's; the og's 8-char
set can yield filenames Windows itself rejects. Verdict: ACCEPTED (strictly
more valid outputs; the naming grammar's other halves — `auto_` prefix,
timestamp shape — are byte-parity per N-061).

*(D-numbering note: the two D-107 rows above — the M-082 layering rider and
the S-047 TS-null-packet posture — are a push-window numbering collision
between two parallel lanes; both rows' content stands, the next lane that
adds D-rows should treat the sequence as D-108+ and, if reconciling, renumber
the second D-107 to a free slot with a history note. This session's rows are
D-108/D-109/D-110.)*


### D-111 — the text-preview precedence for a whitelisted extension whose CONTENT matches a rich magic (M-108 resolution rider, REGISTERED DIVERGENCE — corpus-silent edge)

**WHAT OG DOES:** the preview controller resolves the previewer "by file type"
(UIUX/10 §1); the TxtViewFiles membership flag (DRWUI:535858/:535937) routes a
whitelisted extension to the text viewer. The resolution ORDER when a
whitelisted extension's CONTENT matches a rich previewer's magic (a `.txt`
file whose bytes start `PK`) is not decoded in the corpus — the two flag
consumers set the text-viewable bit, but the panel's preference between
text-view and a content-decoded rich view is UIUX-silent.

**WHAT WE DO:** `extract_metadata` (src/core/preview/cr_preview.cpp) keeps the
magic-keyed rich previews (JPEG/PNG/.../PDF/ZIP) AHEAD of the M-108 extension
gate — a `PK`-headed `.txt` reports `kind=archive/docx`, not text.

**WHY:** the core's preview surface is content-keyed for the rich formats
(the S-0xx/M-0xx landings); the finding's fix plan lands the whitelist at the
fall-through (both executed-probe classes resolved); the og's order for the
overlap case is undecoded, so the conservative architecture (rich previews
first) stands until a corpus decode settles it.

**RISK:** low — the overlap class is pathological (a text-named archive); no
executed evidence shows the og preferring text there.

**VERDICT:** ACCEPTED (one decompile probe of the preview controller's
resolution order, if the corpus ever decodes it, flips this to parity work).

## V. Closing-fix session (2026-10-29) — the thumbnail orientation rule (S-043 rider)

**D-133 — the embedded-fast-path orientation semantics.** (renumbered from its born-collided D-105 seat at the N-207 repair: the S-044 rider keeps D-105) The og's EXIF
orientation behavior is corpus-decisive for the full-decode path only:
`FUN_1402fcc30` reads GDI+ property 0x112 and RotateFlips the decoded
bitmap immediately before `GdipGetImageThumbnail` (call order :652307 →
:652309; evidence/decompiled-quotes/s8-thumbnail-pipeline.md §1) — the
core does exactly that for JPEG (APP1/IFD0 tag 0x112, the landed base) and
TIFF (tag 274, the same datum GDI+ surfaces as 0x112 — the closing-fix
rider), before the 256-px pass.

The core's embedded-EXIF fast path (the M-080/D-96 budget strategy for
oversized inputs — the og full-decodes those via GDI+) has NO corpus
statement about orientation: the og has no such path. Its rule is
spec-derived and registered here: an IFD1 thumbnail is served AS STORED
(the CIPA DC-008 convention records embedded thumbnails upright —
applying IFD0's tag to them would double-rotate spec-conformant files, a
regression the og never exhibits), and an IFD1-DECLARED 0x112 is honored
as the marker of a not-already-oriented thumb (the rare non-conformant
writer). Both behaviors are pinned by test
(`Thumbnail.EmbeddedFastPathServesStoredOrientationByDefault` /
`Thumbnail.EmbeddedFastPathHonorsIfd1DeclaredOrientation`). Residual
known gap: a file whose IFD1 thumb is stored unrotated WITHOUT declaring
0x112 would serve sideways where the og's full decode would rotate —
indistinguishable from the stored bytes without a semantic the file does
not carry; the spec-conformant case (the overwhelming norm) matches.

## W. The session-11 fix lane (2026-10-29) — S-052's registered divergences

**D-138 — the missing-member recovery policy.** (renumbered from its born-collided D-106 seat at the N-207 repair: the N-105 deferral keeps D-106) The og's recovery-side
behavior for an LDM member disk that is ABSENT at recovery time is not
decoded in the corpus (the scan-side semantics are: reconstruction where
the layout allows, loud failure where it does not). The core's recovery
applies its own documented bad-span policy consistently: an unresolvable
basis (or a member region that fails to read) zero-fills the affected
spans, flags the item Salvaged (`had_unreadable`), and the default
hash-verify reports the mismatch — the item is never silently dropped
and never carries wrong-disk bytes. Rationale: consistency with the
existing unreadable-span semantics (FB-R02) beats inventing an
undecoded og behavior; the honest failure is visible in the report.
**VERDICT:** ACCEPTED (a corpus decode of the og's recovery-side
missing-member behavior, if ever found, re-opens this as parity work).

**D-139 — the overlap mapping's conservative families.** (renumbered from its born-collided D-107 seat at the N-207 repair) The S-052
destination-overlap refusal maps LDM-basis extents to physical member
spans before comparing: EXACT for spanned/striped (only the data chunks'
spans are read), CONSERVATIVE for mirrored (every copy's spans — copies
are tried in read order, any may serve the bytes) and RAID-5 (every
column of the covered rows — reconstruction XORs the survivors). The og
has no decoded equivalent check (the R7.2/F12 refusal is the core's own
documented hard-safety invariant, a deliberate improvement over the
reference). The conservatism trades rare false refusals for never
missing a genuine overlap — the safe error direction for a data-loss
guard. Comparison basis: spans on the scan device (the same same-disk
convention the absolute-basis check documents; spans on other member
disks are disjoint hardware).
**VERDICT:** ACCEPTED (safety posture, documented).

### D-112 — the image-stem label fallback is a last-resort only (N-133 resolution rider, REGISTERED DIVERGENCE — the corrected precedence)

M-079's image-target leg derived the session/auto-save label from the image
FILENAME STEM on the refuted premise that "the og has no image-scan naming
semantics." N-133 refuted the premise first-hand (the og's engines decode the
on-disk label: NTFS $Volume NTFSSearchDLL:647-676 + FUN_18003f3a0
:50963-50980; ext4 s_volume_name, algorithms-v2 SS2 step 5 — image scans
included). The corrected precedence landed: **--label > the engine-decoded
on-disk label > the image-stem fallback** — the orchestrator's engine pass
replaces the DERIVED stem (ScanConfig.volume_label_derived, set only by the
stem leg) with the engine decode (NTFS $Volume / ext4 s_volume_name / the FAT
+0x2B boot field / the exFAT root 0x83 dentry — probe_slice_volume_label),
and the CLI/UI auto-save naming consults it (stats.volume_label).

The REMAINING divergence, registered here: a label-LESS image keeps the
filename-stem label where the og's session would carry no volume label (its
auto-name falls back to the timestamp-only shape). Rationale: the stem keeps
the user's mental model of which image produced the session (a recovery-tool
convenience the og lacks for images); the engine label always wins when the
volume carries one. Evidence: the N-133 Resolution + the tests
VolumeLabel.CallerValueWins / Ext4FromSuperblock / NtfsFromVolumeRecord /
FatFromBootSector. A second correction recorded in the Resolution: exFAT's
label lives in the root-dir 0x83 dentry (EntryType@0, CharacterCount@1,
label UTF-16@4 — the spec form the M-079 spec-repair pinned), not a boot
field (the finding's "+0x80" reading was the inference; the corpus deep-dive
documents the dentry).

### D-113 — the display-config data layers are core query tables, not UI presentation (N-132 resolution rider, REGISTERED LAYERING DECISION)

The og's FilterFolder.ini SumaryFileType (the 11 extension-unification
groups behind "MP4 family: N files") and PriorityDisplayType.xml (the
5-category/35-extension alias ordering behind the Type tab's
extension order) are consumed in DRWUI's presentation layer. The core
lands both verbatim as query-layer data + public lookups
(`query::summary_family_for_extension` / `query::type_display_order` /
`type_display_category_name`, cr_config_display_data.cpp) — the ABI
host's presentation consults the same tables the og's UI reads, the
core itself renders nothing (the CLI has no Type tab). This is the
D-107 lineage (the core-only C++ API layering decision). Evidence: the
N-132 Resolution; tests ConfigDisplay.*.


### D-114 — the dormant `only_common` vendor QA mode is deliberately not ported (I-031/S11-R registration, REGISTERED DIVERGENCE — a vendor QA mode)

The og's RAWSearchDLL carries a dormant config-driven mode: when
config/config.ini's `only_common` key is set (the SHIPPED og config does
not set it), the registration walk registers ONLY
`RawFormat_xjq::CommonRawAnalyser` (id 0xa8 = 168,
FUN_180004fb0 :3013-3056) and the dispatch's map/multimap fast paths are
disabled (FUN_18000cf60 :9842-9893 falls to the list layer) — a vendor
QA mode for exercising the single-analyser path. The core has no analog
(the compiled-in signs table always registers the full set; the
disable surface is M-024's disabled_formats, the RawConfig toggles'
port). Rationale: the mode's purpose (isolated QA of one analyser) is
served by the core's per-format disable surface and the test suite's
per-analyser fixtures; porting a config key that ships unset would add
a dead surface the og itself never exercises in production. Evidence:
I-031's S11-R addendum (both sites read first-hand); corpus
RAWSearchDLL_decompiled.c :3013-3056, :9842-9893.

### D-115 — carve OOM: the graceful error code + one-line diagnostic, not the og's log-then-die (N-139 resolution rider, REGISTERED DIVERGENCE — a library must not kill the host)

The og's carve engine treats allocation failure as a hard, loudly-diagnosed
terminal event: the DLL's global operator new (`RAWSearchDLL:958-983`) logs
a five-line PUBLOG memory dump (the failed size + the full
physical/pagefile status via GlobalMemoryStatusEx) and then
`RaiseException(0xe0000001, EXCEPTION_NONCONTINUABLE, ...)` — process
death with a forensic trail; DRWUI carries the identical policy
(`:94179`), so log-then-die is the product's OOM contract, with no
handler anywhere. The core deliberately diverges: every carve allocation
failure converts to `ERR_NO_MEMORY` (the two OOM-realistic buffers — the
scan window and the tail-scan buffer — carry the failed size and window
index in a one-line `cr::log` Error diagnostic on the D-81/D-86 seam; a
top-level guard converts any remaining `bad_alloc` so the engine's
error-code contract holds on every path, worker threads included). A
library embedded in a host UI must not kill the process the og's
standalone product could; the diagnostic preserves the og's
field-diagnosability intent at the core's logging seam. Evidence: the
N-139 Resolution + `Carve.OomWindowBufferReturnsCode` /
`Carve.OomTailBufferReturnsCode` / `Carve.OomNoExceptionEscapesEngine`.

### D-116 — the GPT deliberate strictness: entry_size==128 refusal + the 1 MiB entries-read cap + the salvage entries_lba bounds check (N-164 registration, REGISTERED DIVERGENCE — validation strength over the og's mis-walk)

The og's GPT reader accepts whatever entry_size the (possibly corrupt) header
declares: it allocates `entry_count × 0x80` with 64-bit saturation
(Partition_decompiled.c:4530-4541), reads `entry_size × entry_count` bytes as a
32-bit product, and its consumers stride a HARDCODED 0x80 (:4703) — a non-128
entry_size produces an og MIS-WALK of the entry array rather than a refusal,
and there is no entry-count cap. The core refuses (`entry_size != 128 →
ERR_BAD_PARAMETER`, cr_table_parse.cpp:38 and the salvage twin), caps the
entries read at 1 MiB (~8192 entries), and bounds-checks the salvage path's
entries_start_lba — deliberate strictness registered here per the registry's
own rule (an unregistered delta is a bug, not a delta): a mis-sized array on a
corrupt header is refuse-with-code territory for a library whose callers
branch on rc, not silent mis-parse territory. Rationale: the og's hardcoded
stride means its acceptance of non-128 arrays yields WRONG data, never
recovery-relevant salvage. Evidence: N-164's corpus cites (re-read first-hand
this session: the saturation alloc at :4530-4541, the 32-bit product read at
:4541, the 0x80 stride at :4703); the core's guards at cr_table_parse.cpp
:38/:40/:76-81/:100.

### D-117 — the user-sign availability bound is the decoded-byte length, not the og's 2× hex-string guard (N-168 registration, REGISTERED SUPERSET — the og's guard is an og defect)

The og's sign matcher refuses to test any file whose length is below
`offset + hexStringLength` — twice the bytes it actually reads (the guard at
DRWUI_decompiled.c:1003371 and :1003545 compares against the hex TEXT length
while the matcher at the same sites uses `hexstrlen >> 1`, the decoded byte
count). A 16-byte sign at offset 0 therefore demands a ≥32-byte file; fragments
in [offset+bytes, offset+2·bytes) are never tested. The core's
`cr_usersigns.cpp` bound is the correct decoded-byte length
(`s.offset + s.len > avail`), and it matches those fragments — a deliberate
superset: adopting the og's 2× guard would LOSE matches on short fragments (the
CHK-repair identify flow's exact population); the og's guard is an og defect,
not a parity contract. Evidence: N-168's cites (both og sites re-read
first-hand this session); the pin test `UserSigns.ShortFragmentMatchesInCore`
(the 5-byte fragment vs the 4-byte sign: matched in core, never tested by the
og; the below-byte-bound reject still holds).

### D-118 — no generic id-8 demote on special-reject/plain-accept (N-143 registration, REGISTERED DIVERGENCE — the honest-refusal doctrine over the og's salvage stamp)

The og's analyser dispatch is two-tier (RAWSearchDLL :9877-9883, re-read
first-hand): a candidate analyser implementing ISpecialRawFileAnalyser gets its
full context-aware validate first; on its reject the plain probe
(vtable+0x10) runs, and a plain-accept after a special-reject stamps the
GENERIC format id 8 and continues the per-position chain (:10020-10039 — the
`result+0x18 == 0 → 8` stamp) so a later analyser can still claim the position.
FileZIPAnalyser carries both vtables (:2235-2238) — corrupt ZIP/RIFF-family
archives are the practical demote population. The core deliberately declines
the id-8 generic fallback class: a signature-true/grammar-false hit produces
NO candidate (the N-068 sanity gates and every validator's honest refusal),
never a generic-format salvage row — the same doctrine D-75 registers for the
per-key chain/fallback family. Rationale: the id-8 row carries no format
semantics (no validator, no extension family, no recovery affordances); a
generic candidate's only content is "bytes matched a magic" — the core's
false-positive discipline (the F8 gates, the N-070 score contest) exists to
keep exactly that class out of the results. Affected fixture class: corrupt
ZIP/RIFF archives (pinned: `Carve.SpecialRejectPlainAcceptRefusedByName` —
the corrupt candidate refused by name; the valid control beside it carves).

### D-119 — the FAT cluster→sector math is full u64; the og's 32-bit wrapping product deliberately not adopted (N-162 registration, REGISTERED SUPERSET — the numeric-hygiene family's FAT member)

The og computes `(cluster−2) × sectors_per_cluster` as a 32-bit product at four
verified sites (FATSearchDLL :13194 — the explicit `(uint)` cast on the cluster
operand with the `(int)` spc, re-read first-hand this session; :11239 — the same
32-bit operand promotion without the cast; :28172 and :39068-39080 — the
CalcPartition geometry twins; 64-bit sites also exist in the og, e.g. the :28045
runlist factory — the width inconsistency is og-internal). The product wraps on
data areas beyond 2^32 sectors — legal FAT32 territory (2 TiB at 512 B/sector;
spc=32 wraps from cluster 134,217,730; at the spec-max cluster 0x0FFFFFF4 with
spc=32 the og addresses 0xFFFFFE40 instead of 8,589,934,144 — a ~2 TiB
displacement). The core's `FatTable::cluster_offset` (cr_chain.cpp) is full u64
and correct — the deliberate non-adoption of an og arithmetic defect, this
family's FAT member (D-56 the NTFS pass-2 precedent, D-68 the MRW precedent).
Rationale: a recovery engine must address what the on-disk geometry actually
names; the og's wrap is silent corruption on large volumes, not a contract.
Evidence: N-162's cites; the pin test `FatEngine.ClusterOffsetBeyond2TiB` (the
spec-max cluster's exact u64 address, the wrapped form explicitly NOT equal).
## X. The S42 fix lane (2026-10-29) — the progress display layer (M-125)

### D-127 — the engine's linear permille vs the og's display remap (progress
semantics, M-125):** the core's engine telemetry reports a TRUTHFUL linear
weighted permille over the stage weights (cr_scan's `weighted_progress`,
40/35/25 — the engine never lies in its telemetry), where the og's DRWUI
renders a piecewise-remapped display percent (FUN_1401af820: raw<11 → ×2;
11..30 → (raw−11)·0.2+20; 30..90 → ×0.8; 90..98 → (raw−88)·2.7+72; >98 →
identity; the defensive 99.99 stall cap — dead for raw in [0,100], ported as
decoded) with the monotonic publish guard hiding the 98-boundary
discontinuity (the 2.7 segment tops at 99.0, the identity tail drops to
98.5). Rationale: the engine's truthful permille is the deliberate
improvement (a lying engine metric corrupts every consumer — benches,
sessions, hosts); the og's remap is a UX-layer construct. The display math
lands as the OPTIONAL query-layer helper family
(`query::progress_display_remap/_percent/_publish`) so hosts render
og-faithful UX without the engine lying. Evidence: DRWUI_decompiled.c
:314931-314986 + :315014-315024 (the four-function family, decoded
first-hand); the runtime anchors (displayed 9% at 75 s, 34% at ≈460 s,
dynamic-ground-truth :39,:90) reproduce under the curve. Tests:
ProgressDisplay.* (M-125).

### D-120 — the deleted-FAT name presentation: the og's '#' placeholder on the no-oracle branch (aligned), the LFN-checksum original-byte recovery as the deliberate superset (N-172 registration)

The og renders a deleted 8.3 entry's destroyed first byte (0xE5) as '#' in its name
builder (FATSearchDLL FUN_180036350 :44166-44172, re-read first-hand: the 0x23
constant and the -0x1b signed comparison) — "#EADME.DAT" — with NO recovery of the
destroyed byte anywhere in the DLL. The core's presentation is two-branch: with a
surviving LFN oracle, `lfn_deleted_checksum_recover` brute-forces the ORIGINAL byte
deterministically (a bijection over 256 candidates validated against the stored
checksum — GAP-S6-1, bench 40079065019; "readme.txt" where the og shows
"#EADME.TXT") — a deliberate superset, quality win, registered here; without an
oracle the placeholder is '#' (aligned with the og this session per the finding's
bench-comparability recommendation; the pre-N-172 core used '_'). Rationale: the
recovery is strictly richer name truth at zero risk (the checksum is a 256-space
oracle); the placeholder alignment keeps name-set benches comparable without
normalization. Evidence: N-172's cites; the pins FatEngine.DeletedFirstChar
PlaceholderIsHash + FatEngine.DeletedLfnChecksumRecoversOriginalByte + the re-pinned
#EL.TXT/#ONE.TXT/#OLD.TXT family.

### D-122 — scan-state ownership: single-owner + mutexed telemetry snapshots, not the og's unlocked master enum (I-043 registration, REGISTERED IMPROVEMENT — data-race-free by construction)

- **WHAT OG DOES:** the master scan-state variable — a 14-value enum
  (`notstarted…quickscanpaused/stopped/finish…advancedscanning…repairvideo_pause`)
  at the service singleton +0x78 (`DRWUI_decompiled.c:208243-296`) — is
  written by the scan flow and read cross-thread **without its own lock**.
  The og itself carries a parallel *locked* carrier beside it
  (`CUIScanProgressInfo::nStatus` +0x44 with its own mutex at +0x90) —
  i.e. the og's own mitigation acknowledges the race surface.
- **WHAT WE DO:** the authoritative scan state is single-owner (the
  controller owns it; no cross-thread unsynchronized reads exist by
  construction), and every cross-thread observation goes through mutexed
  telemetry snapshots (`src/core/orchestrator/cr_scan.cpp:319-376` publish
  paths; the parallel-scan test family + TSan CI pin it).
- **WHY:** thread-safety hardening (the same family as D-18's cancellation
  first-class): a recovery library must not expose racy state to hosts.
- **RISK:** none — the observable state surface (stage masks, progress,
  pause/stop/finish) is equivalent or richer; only the internal ownership
  shape differs.
- **VERDICT: ACCEPTED** (deliberate improvement, registered I-043; the
  telemetry mutex is already covered by the parallel-scan tests; no
  additional test owed).

### D-123 — og-defect non-adoption batch #2: ext4 i_size_high unconditional poll + _gmtime32_s post-2038 garbage (I-040 registration, REGISTERED IMPROVEMENT — the D-58/D-59/N-162 [D-119] class)

- **WHAT OG DOES (defect 1):** EXT4SearchDLL's size compute (FUN_180007510,
  `EXT4SearchDLL_decompiled.c:5455-5467`) CONCATs the +0x6c dword with the
  +0x04 dword **unconditionally for every inode** — but +0x6c (i_size_high)
  is meaningful only for regular files; for directories it is the legacy
  dir-ACL pointer, so legacy-ACL directories surface polluted garbage
  sizes (e.g. an ACL/xattr block number 0x3039 reads as a ~51 PiB size).
- **WHAT OG DOES (defect 2):** the SearchDLL family converts unix times
  through `_gmtime32_s` (EXT4SearchDLL.dll import,
  api-ms-win-crt-time-l1-1-0.dll) — the 32-bit time_t path produces
  garbage for timestamps past 2038-01-19.
- **WHAT WE DO:** (1) the clean conditional — the high half folds in only
  when `ifmt(mode) == 0x8000` (regular), directories keep their true
  32-bit size (`src/core/ext4/cr_ext4_engine.cpp:404-407`); (2) full-u64
  time paths end-to-end (`unix_to_filetime_ns`, cr_types.h — i64 seconds,
  no 32-bit cliff anywhere).
- **WHY:** both og behaviors are arithmetic defects, not parity surface —
  adopting them would corrupt recovered metadata.
- **RISK:** none — the core's outputs are strictly more correct.
- **VERDICT: ACCEPTED** (deliberate non-adoption, registered I-040;
  anti-down-fix pins: `Ext4Engine.LegacyAclDirSizeClean` (a dir inode with
  a nonzero +0x6c ACL pointer keeps its true 32-bit size) + the extended
  `Ext4Engine.UnixTimeConversion` (exact post-2038/2100 filetimes, ns
  variant) — future sessions must not "fix" the core down to either
  defect).

### D-124 — GPT backup-probe position parity + the dead-LBA1 backup recovery superset (M-132 registration, two sub-items)

- **WHAT OG DOES (sub-item a — probe position):** the analyzer's backup
  read is ALWAYS at the FIXED device-last LBA (`FUN_180006e20:4567` reads
  `param_1[1] - 1` — the device's own sector count, never the primary
  header's declared `backup_lba` field; re-read first-hand at
  `Partition_decompiled.c:4555-4575`).
- **WHAT OG DOES (sub-item b — the dead-LBA1 early return):** when the
  LBA1 READ ITSELF fails, the analyzer prints "Failed to read GPT
  HEADER.", frees, and returns with NO GPT partitions (:4517-4524) — only
  the separate `IsGPTDisk` detection layer (:4208-4218) classifies the
  disk as GPT for UI purposes.
- **WHAT WE DO:** (a) the probe is structurally at device-last FIRST
  (`cr_table_parse.cpp` — the rc_p==OK branch probes `dev_last` before
  the declared field; the declared `backup_lba` is a fallback only).
  Note: post-M-123 the position gate already REQUIRES backup_lba ==
  dev-last on every trusted primary, so the observable class "healthy
  primary declaring a nonstandard backup position" routes into the
  salvage/backup ladder exactly as the og's does — the probe-order change
  is architectural parity so the divergence cannot re-emerge if the gate
  is ever relaxed; (b) the core KEEPS the recovery stance the og lacks:
  a dead/unreadable/wiped LBA1 still routes (via the protective MBR) to
  the device-last backup probe and its entries are emitted.
- **WHY (b is kept):** the og's early return is a weakness on the exact
  damage class recovery tools exist for — a wiped LBA1 with an intact
  backup is recoverable, and the core recovers it.
- **RISK:** none — (a) is og-exact structure; (b) only ADDS recoverable
  output on a class where the og emits nothing.
- **VERDICT: ACCEPTED** (deliberate superset, registered M-132; pin:
  `Gpt.DeadLba1BackupRecoveryRegistered` — future sessions must not
  "fix" the backup recovery away as a parity bug).

**CONVERGENT LANDING NOTE (SA-9, this session):** the S47 INFO-first lane's
independent I-040 registration (pushed second; per earlier-push-wins this D-123
stands as registered) verified the same corpus cites first-hand and adds two
evidentiary/coverage supplements: the 4-site `_gmtime32_s` line census
(`EXT4SearchDLL_decompiled.c:5495/:5506/:6262/:6273`) and the regular-file
fold-direction pin `Ext4Engine.SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect`
(hi=1/lo=0xFFFFFFFF surfaces the full 8 GiB-1 size — the direction
`LegacyAclDirSizeClean` leaves unpinned) plus
`Ext4Engine.Post2038FileTimeConversionStays64Bit`. Both lanes' records kept.

### D-125 — user-sign hex-case semantics: case-insensitive byte-decode, not the og's %02X-only text compare (M-128 registration, REGISTERED IMPROVEMENT — the D-117 class)

- **WHAT OG DOES:** the EUUserData consumer converts candidate file bytes
  to UPPERCASE hexadecimal text (`%02X` — FUN_140604090 at
  `DRWUI_decompiled.c:1003619`; zero `%02x` anywhere in DRWUI.exe) and
  memcmps against the stored sign string (`:1003452`/`:1003555`). Any
  sign containing lowercase hex can never match: the og's own shipped
  EUUserData carries **11** such signs (10 pure-lowercase MPEG-PS
  `000001b3…`/`000001ba…` family + 1 mixed-case img sign — the census
  re-counted first-hand this session at `og-codebase/EUUserData`;
  corrects the finding's "12" per the corpus-wins rule) — dead
  configuration in the og.
- **WHAT WE DO:** `cr_usersigns.cpp` decodes both hex cases to BYTES and
  compares bytes (`hex_nibble` accepts 0-9/a-f/A-F, `match_at` memcmps
  decoded bytes) — all 190 signs of the shipped default set are live,
  including the 11 og-dead ones.
- **WHY:** the og's %02X-only text compare is a string-handling defect of
  its UI-layer identify flow (the vendor's own DB ships signs its matcher
  can never fire), not a semantic contract; comparing decoded bytes is
  the architecturally right channel (the core's user-sign channel is
  itself the D-104-registered superset — the og's scan engine has no
  user-sign channel at all). Folding case to match the defect would
  break good code to reproduce an og weakness.
- **RISK:** parity benches on the user-sign channel must normalize for
  D-104 (the channel) + this row (the 11-sign subset): core-only carve
  records in the MPEG-PS free-space class are expected and explained.
- **VERDICT: ACCEPTED** (deliberate superset, registered M-128; pins:
  `UserSigns.LowercaseHexSignLiveAndRegistered` (the og-dead class fires)
  + `UserSigns.UppercaseHexSignMatches` (the og-live form unchanged) +
  `UserSigns.DefaultSetEffectiveCount` (190 live, the og-dead mpeg family
  present) — future sessions must not case-fold the decoder down to the
  og's dead-configuration semantics).

### D-126 — EUUserData loader tolerance: skipped signs vs the og's dead entries (M-129 registration, one-line nuance)

- **WHAT OG DOES:** malformed sign strings (token count ≠ 2) are silently
  SKIPPED at load (`DRWUI:1005986`); odd-length/non-hex signs LOAD but the
  matcher's length/charset guard makes them permanently unmatchable
  (`:1003452`); unknown members are ignored (`_stricmp "signs"` only,
  `:1005818`).
- **WHAT WE DO:** the same tolerances (skip-and-continue) with ONE nuance:
  odd-length and non-hex signs are SKIPPED at load rather than carried as
  permanently-dead entries — equivalent-in-effect (a dead sign and a
  skipped sign both never fire), simpler in the byte-compare channel.
  The S-039 caps doctrine stays deliberately strict: over-cap shapes
  (offset > 1024, sign > 96 hex digits, > 12 signs/entry, > 80-char mime,
  > kMaxEntries) still reject the whole file — the og has no decoded caps
  analog, `UserSigns.HostileBlobStillAtomic` is the pin.
- **WHY:** forward compatibility and operational availability — a
  hand-edited or future-versioned EUUserData the og loads (partially)
  must load in the core (M-129's whole subject); the caps are the
  core's own memory-safety posture.
- **VERDICT: ACCEPTED** (registered M-129; pins: `UserSigns.MalformedSignSkippedNotFatal`
  + `UserSigns.OddHexSkipped` + `UserSigns.UnknownMemberIgnored` +
  `UserSigns.MemberNamesCaseInsensitive` + `UserSigns.AllMalformedSignsLoadStructurally`
  + `UserSigns.TornFileStillRejected` + `UserSigns.ShippedOgFileVerbatimLoads`).
### D-134 — the N-158 typed-id filter coupling: the row-level disabled_formats gate (REGISTERED CONSERVATIVE POSTURE — the og's exact [Filter] suppression mechanism is undecodable from the corpus)

The og ships `[Filter]` keys for exactly the five N-158 typed ids
(XMIND/MMAP/EDDX/WFP/DRP, `og-codebase/config/RawConfig.ini` :12/:15/:17/:18/:22,
all default 1), loaded by CNeedRawMgr::LoadConfig (RAWSearchDLL :13133-13200)
into a key→flag map. The map's consumers are decodable for the analyser-layer
ids (HIK/AEP registrations consult their tokens at :3823-3894/:4024-4074 —
key 0 removes the analyser), but for the five TYPED ids stamped inside the
ZIP analyser's LFH handler (:53025-53100) no consumer exists in the decompile:
the per-search filter rebuild's IAnalyserFilter callback (the removal loop
FUN_180006b30, :4408-4432) calls `(**(code **)(*param_2 + 8))(param_2)` — a
one-argument method with no id parameter — so whether an og key-off suppresses
the typed row, the whole analyser, or nothing at all is not decidable from
the corpus (the keys may even be dead config).

The core's registered posture: the conservative row-level gate the finding
itself prescribed — the five ids gate through `CarveFlags::disabled_formats`
(the M-024 surface, the core's documented [Filter] equivalent), suppressing
the TYPING itself (a disabled typed id makes the candidate fall through to
the generic ZIP row, never a mis-labeled office claim); the N-115 nine stay
exempt via carve_format_always_on (N-149's analyser-mask decode — those
families have no landed og disable evidence and the mask keeps the ZIP
analyser always-on). This divergence is bounded and honest: it documents a
gate the og's config file advertises but whose engine-side wiring the corpus
cannot prove. Registered by N-158's Resolution; revisit only if a future
corpus artifact decodes the IAnalyserFilter's id plumbing.

### D-135 — the sparse-FIRST $DATA acceptance (N-163 registration, REGISTERED DELIBERATE SUPERSET — a fully-sparse file is real content shape)

The og's runlist pre-validator (FUN_18001d5f0, NTFSSearchDLL :23175-23210)
REJECTS a $DATA whose first runlist header byte carries off-nibble==0
(:23184: `(*b & 0xf0) == 0 -> return 0`) — enforced at the five attribute
parse sites (:23412/:23506/:23533/:24337/:24643). A fully-sparse file
(an all-hole $DATA, e.g. an NTFS sparse file never written or a
hole-punch artifact) therefore yields NO extents in the og — the file is
unrecoverable-by-policy on the reference.

The core accepts and represents sparse-first runlists natively (the
RunList decode treats off_width==0 as a legal sparse run; S-001's
all-hole $DATA fixture recovers as zero-filled truth). Rationale: the
shape is real content (the zero-filled recovery is honest — the
Salvaged/FB-R02 family marks it), and rejecting a decodable structure
the og's own DECODER accepts (FUN_1800347d0 has no sparse gate — only
the pre-validator does) would sacrifice recoverable file identity for
reference-quirk parity. Registered by N-163's Resolution alongside the
decoder's truncate-and-keep tolerance landing (the og direction this
finding fixed); the S-001 all-hole recovery behavior is unchanged and
now decode-anchored.

### D-136 — the metadata-restoration POSIX translation (S-060 landing, REGISTERED DELIBERATE TRANSLATION — engine behavior)

**Class:** deliberate cross-platform translation · **Findings:** S-060 (landing
2026-10-29) · **Date:** 2026-10-29

**WHAT OG DOES:** after every committed item the og's export tail runs
`SetFileAttributesW(path, word & 0xfffd)` then `CreateFileW` + the mask-gated
`SetFileTime(created, access, write)` trio (DRWUI:874686-718), both failures
ReleaseTrace'd and non-fatal.

**WHAT CORE DOES:** the identical contract on Windows (WinDestIo implements
the og's shape verbatim). On POSIX hosts (FileDestIo — the portable ABI-check
recovery surface), the same contract translates as: the times via
`utimensat` (atime + mtime; a BIRTH time is not portably settable through
POSIX — the created slot maps to nothing on hosts without a birth-time API
and is skipped, never guessed), and the attributes via `chmod` carrying the
READONLY bit (0x1) as the owner-write bit (the one DOS attribute POSIX can
hold truthfully; hidden/system/archive have no portable mapping and are
dropped rather than approximated). The per-field enables mirror the engine's
emission — a zero slot is the og's honest no-date (the N-021/A-014 zero
FileTime) and passes as NULL exactly like the og's mask gates.

**WHY:** the og is a Windows-only product; the core's portable surface must
either fake Win32 semantics (wrong) or translate honestly (this). The
translation is reported per row ("restored" / "failed:<code>" /
"unsupported") — never a silent approximation.
### D-128 — the ext4 damaged-marker compute: clean 64-bit form over the og's sign-extended/truncated comparison (N-177 registration)

The og's truncation-damage verdict (EXT4SearchDLL FUN_180007510 :5455-5467)
computes `size = (i_size_high << 32 | i_size_lo) + ((i_size_high >> 0x1F) & 0x1FF)`
— a 9-bit sign-extension smear on the high dword (the same +0x6c-slot legacy-ACL
poll family D-123 documents for directories, applied here as an arithmetic
adjustment on files) — and then compares the two sides as TRUNCATED 32-bit words:
`u32(i_blocks_lo) < u32(size >> 9)`. The core's N-177 landing computes the clean
form: `u64(i_blocks_lo) * 512 < i_size`, files only. Divergences from the og:
(1) sizes above 4 GiB compare correctly (the og's u32 truncation can mis-mark
exactly-4-GiB-straddling files); (2) inodes whose +0x6c slot carries a legacy
ACL pointer pollution never smear the size (the D-123 family); (3) the marker
bit is the SAME numeric bit 0x200 so cross-tool consumption is unaffected.
Registered deliberate superset: the og's comparison defects are NOT adopted
(the I-040 anti-down-fix doctrine).

### D-130 — the FAT out-of-volume run-end stance: deferred per-read failure, not scan-time destruction (N-165 registration, REGISTERED DELIBERATE SUPERSET — truncated-tail volumes are first-class recovery targets)

The og's FAT runlist factories destroy a file's whole runlist at SCAN time
when a run's end passes the volume sector limit — "File overflows
partition!" via ReleaseTrace and the runlist object released
(FATSearchDLL :10873-10903 and the second factory :28045-28070, both
64-bit checks, re-read first-hand this session). The file then surfaces
with NO data runs: no recoverable content offered even where the device
still holds the chain's readable prefix.

The core's registered stance: extents emit unguarded
(cr_fat_quick.cpp:370-386) and validate_boot deliberately accepts volumes
whose BPB claims more sectors than the device holds (the truncated-tail
allowance, cr_chain.cpp:613-616) — a volume cut mid-FAT keeps every
overflow-tail file LISTED with its chain extents; the readable prefix
recovers, and the beyond-device tail fails per-read, surfacing through the
FB-R02 Salvaged family (the recovery report's partial-content disclosure)
instead of a scan-time destruction. Rationale: truncated-tail volumes are
recovery targets exactly when the tail is gone (repartitioned/re-sized
disks); destroying the runlist because its end is past the limit also
destroys the in-device prefix the og itself could have served. The
scan-time grade stays the chain's own (FAT-verified intact chains grade
Excellent; the truncation is a recovery-time disclosure, never a silent
drop). Pinned by FatEngine.TruncatedVolumeOverflowChainStance; do NOT
"fix" toward the og's destruction without re-registering this row.
### D-129 — the session-list surface's registered deltas (N-090: the scope superset, the save-time second resolution, the deviceType non-carriage; the label width CLOSED by the v6 tail)

**WHAT:** three deliberate differences on the W1a session-list surface,
each verified against the corpus first-hand this session: (a) the core
lists EVERY `*.crsf` in the directory where the og's home list keeps
only names containing BOTH `auto_` and `.rsf` (FUN_140273940 at
DRWUI_decompiled.c:432326-432333 — the substring pair filter; rsf-format
§5.5); (b) `save_unix` carries SECONDS where the og's record-2 carries a
local SYSTEMTIME with milliseconds (rsf-format §5.2 +0x6C); (c) the og's
record-2 `deviceType` u32 (2 → "Desktop", 3 → "Recycle Bin", else the
disk-info name at +0x120) has no row counterpart.
**WHY:** (a) the core is an engine ABI serving hosts that resume MANUAL
saves too — a manual save is as resumable as an auto-save, and hiding it
from the list serves no recovery purpose; a host wanting the og's exact
home view filters client-side by name (one substring pair). (b) the
newest-first comparator needs second resolution only — the og's own
auto-save FILE NAMES are second-granular
(`auto_<vol>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf`, rsf-format §5.5), and the
core's equal-second ties break by path bytes deterministically (N-116);
carrying ms would force a v6-format migration for a display nicety the
banner never exercises. (c) no core consumer exists for a UI display
classification of the disk; a host composing the og's view has its own
enumeration to draw from.
**RISK:** none for recovery behavior — all three are presentation-layer
on a host-facing summary surface. The LABEL WIDTH delta (the og's
`volumeName[100]` vs the core's 31-char card) is CLOSED by this same
landing: the v6 record tail + the `label_full[100]` ABI append carry
the complete label at the og's exact strncpy(…,99) width, with
`label[32]` preserved as the legacy prefix view.
**Registered by:** N-090's Resolution (the finding's fix-plan steps 1-4,
the deliberate-superset option taken at every choice point).

### D-137 — the file-repair product line: formal scope exclusion (S-050 disposition, REGISTERED SCOPE EXCLUSION — product boundary)

**Class:** scope exclusion (product line, not an engine capability) ·
**Findings:** S-050 (+ its three addenda: the video-engine build-order
amendment, the UIUX/32 row-contract decode, the per-chunk feed wiring) ·
**Date:** 2026-10-29

**WHAT OG DOES:** bundles a complete file-repair product line (Fixo
provenance; 9 binaries; photo/video/office/HWP/X3F/7z repair + enhancement),
remote-config-flagged (enable_imgrepair/enable_docrepair), integrated into
DRWUI's recovered-file flow via the CHK/CHKRepairFileSign eligibility chain
and a dedicated UI spine (row controllers, the serialized repair queue, the
TimeCritical thread fleet).

**WHAT CORE DOES:** implements the recovery engine + the ADJACENT eligibility
surface truthfully (the EUUserData user-signature channel, D-104, carries the
same signature database the og's CHK chain reads; the M-021 post-processor is
the per-candidate seam; the per-chunk feed is declined-and-registered in
D-104's landing notes) and registers the repair line itself as OUT OF SCOPE
in CAPABILITY-MATRIX §3 — a separate product line the og bundles, not a
parity obligation of the recovery engine.

**WHY:** the finding's own decision framework (fix-plan step 1: implement OR
register truthfully). The corpus documents the line as a distinct product
(the Fixo source tree, its own UI spine, its own output-location contract);
faking an engine-side repair verb would be the silent-approximation class
this register exists to prevent. A future implementation's decoded entry
points are preserved here: cr/repair beside rawfixer; the video engine's
datarun-selection seam (tree::FileNode::extents — the only repair engine
that consumes the core's own scan results); the sample-file consent model
(RawFixer's S1 pattern); the distinct repaired-output location; the
per-format grammar artifacts in `artifacts/FileRepair/` (8 files, all
first-cited by S-050).

### D-131 — the EUUserData duplicate-key resolution: the core MERGES every block (N-169 registration, REGISTERED DELIBERATE POSTURE — the og's parse-side semantics remain an honest OQ)

The og's own shipped EUUserData carries a GENUINE duplicate `asf` block (112
members; offsets 4770/16766; the SAME sign `0,3026B2758E66CF11A6D900AA0062CE6C`
with mimes `application/vnd.ms-asf` then `asf` — first-hand re-measured this
session). Whether the og's JSON layer keeps first, last, or both members is
NOT decodable from the corpus: the finding's decisive cite (FUN_140077dca0)
does not exist in either decompile (citation erratum, recorded), and the
member-iteration evidence (the child-chain walks + the per-child "signs"
lookup, RAWSearchDLL :24295-24333) supports — but does not prove — the
merge, since the parse-side member-append behavior was never attributed.

The core's registered posture: the loader walks EVERY top-level member —
duplicate keys never reject the file, every block's sign set installs, each
block's mime backfills its own signs (the MERGE). The SHIPPED default set
(data/userdata-defaults.json) is the DEDUPED 111/190 view keeping exactly
one asf block with the LAST block's mime ("asf" — last-wins dedup at the
asset level, matching the file's own trailing-block shape). Both censuses
are true: raw = 112 members/asf×2; shipped = 111/190 — future sessions must
stop reading them as census drift. Pinned by UserSigns.DuplicateKey
Semantics + ShippedCensusDedupTruth; the og's parse-side semantics stay the
recorded OQ (the follow-up needs the og's JSON member-append attribution).

## Y. The diff-to-fix closing-fix lane (2026-10-29) — the two INFORMATIONAL registers

### D-141 — og-defect non-adoption batch #3: ext4 unwritten-extent `ee_len` raw multiply (I-045 registration, REGISTERED IMPROVEMENT — the D-58/D-59/N-162 [D-119] class, batch #3 of the I-026/I-040/D-123 family)

- **WHAT OG DOES:** the extent decoder multiplies the RAW ee_len word by
  the block size with no unwritten-bit handling anywhere in the walk family
  (EXT4SearchDLL FUN_180005670 region :4044-4048, re-read first-hand this
  session: `local_58 = (ulonglong)CONCAT24(psVar5[-1], *(undefined4 *)
  psVar5) * (longlong)*(int *)(param_1 + 0x58)` and the twin
  `(uint)(ushort)psVar5[-2] * *(int *)(param_1 + 0x58)` product; no
  0x8000/32768 test exists anywhere in the og's extent decoder). An
  unwritten extent (ee_len bit 15 set — fallocate output, common on modern
  ext4) therefore yields an overflow-class garbage run length
  (e.g. ee_len 0x8005 at 1 KiB blocks → a "33,556,736-byte" extent).
- **WHAT WE DO:** the kernel-correct decode (`src/core/ext4/
  cr_ext4_engine.cpp` walk_extent_node :346-350): raw ee_len > 0x8000
  (EXT_INIT_MAX_LEN) → the length is `raw & 0x7FFF` (the unwritten
  marker stripped); raw == 0x8000 stays a WRITTEN 32768-block extent.
  On images containing unwritten extents the og computes garbage lengths
  and the core computes correct ones.
- **WHY:** the og behavior is an arithmetic defect, not parity surface —
  adopting it would corrupt recovered content extents.
- **RISK:** none — the core's outputs are strictly more correct.
- **VERDICT: ACCEPTED** (deliberate non-adoption, registered I-045;
  anti-down-fix pin: `Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect`
  (ee_len 0x8005 → a 5-block content extent, never the raw product) —
  future sessions must not "fix" the core down to the og's garbage math).

### D-142 — the lost-partition resume granularity: stage-granular checkpoints, not the og's scan-position-granular marker-restore (I-046 registration, REGISTERED DELIBERATE DIVERGENCE — the M-042 architecture this rides is FIXED and first-class)

- **WHAT OG DOES:** the lostpart.dat/partinfo.dat pair is a
  PER-PROCESS-GENERATION cache, not a cross-session knowledge base
  (PartitionRecoveryD :23189-23211 — the first-search-per-process latch
  deletes BOTH .dat files and rebuilds partinfo from the live table;
  within a process, found candidates append per-hit mid-scan (:20154-20238,
  the append at :20230), and a clone/search interrupt appends a
  header-only progress-marker record (payload zeroed, kind flag 0 —
  :20079-20082 → :23088-23121). At load, marker records max-merge their
  counter (pool+0xe4) and RESTORE THE SCAN-POSITION LBA (pool+0xe8) at
  sector granularity (:20748-20770) — cross-PROCESS restart resumes the
  lost-partition search at the persisted scan position.
- **WHAT WE DO:** the M-042 architecture (session-checkpoint persistence):
  no module-local .dat files; REC_PARTITION_CONTEXT per candidate with
  forced checkpoint on discovery (the per-hit append cadence equivalent),
  REC_DISK_LAYOUT for the whole-disk snapshot (the partinfo comparator
  equivalent), and resume revalidation through `revalidate_candidate`
  (cr_scan.cpp:1085-1090). The core's resume is STAGE-granular (the
  session's stage bits), not scan-position-granular.
- **WHY:** the stage-granular model is the registered M-042 architecture —
  deliberate, already landed and tested; the og's sector-granular restore
  is a fidelity refinement on a resume path the core already covers at
  stage granularity.
- **RISK:** a resumed lost-partition scan re-walks the current stage's
  sectors instead of resuming at the persisted LBA — bounded by the stage
  caps (no stuck/freeze; every walker bounded); no recovery-result
  divergence (candidates revalidate against the persisted records).
- **VERDICT: ACCEPTED** (deliberate divergence, registered I-046). If a
  future session wants scan-position-granular lost-partition resume, the
  W3 marker shape (header-only, `min(cur,end)` LBA, kind flag 0) and the
  :20748-20770 restore path are the decode inputs — cite I-046. Future
  documentation must stop describing the og's .dat pair as a persistent
  knowledge base (it is one process-generation old at most).

### D-143 — the NTFS walk-entry record-geometry gate declined: the core's wider acceptance band is deliberate (N-196 registration, REGISTERED DELIBERATE DIVERGENCE — accept-direction superset)

- **WHAT OG DOES:** the MFT walk entry refuses records whose device-sector
  count is odd or exceeds 16, in two layers (NTFSSearchDLL
  FUN_1800272b0): the primary branch computes `record_bytes /
  device_sector` and bails on `(count & 1) != 0 || (byte)count > 0x10`
  (:31620-31621); the mirror-fallback branch recomputes the 512-byte-
  rounded size and requires `(rounded & 1) == 0 && (byte)rounded < 0x11`
  to proceed (:31599-31600); both failing sets the walk's refusal code
  0x610016. A 3-sector 1536-byte record and a 17-sector 8704-byte record
  are both refused at the walk entry (odd and >16 respectively), even
  though the og ACCEPTS non-pow2 even sizes (3072/5120/6144 pass).
- **WHAT WE DO:** no walk-entry geometry gate exists between open and
  walk (`cr_bootstrap.cpp` — the P1-4 shift-UB guard caps the negative-
  exponent decode form at 1 MiB and the sub-sector/0-result rules reject
  degenerate sizes, but positive cpr decodes uncapped and no even/≤16
  device-sector rule applies). The core opens and walks volumes in the
  og's refusal band; per-record validation (magic + fixup + USA) is the
  safety mechanism on every stride.
- **WHY DELIBERATE:** refusing recoverable volumes is the worse direction
  per the board's calibration history (the D-119 numeric-hygiene family):
  a real formatter never emits an odd/>16-sector record size the way a
  corrupt DBR does, but a torn volume whose DBR survived with an unusual
  cpr still has recoverable records behind it — the core's per-record
  validation rejects garbage strides individually while keeping the
  volume walkable. The wider band is accept-direction only: no wrong-
  content risk (every record is still validated before decode).
- **EVIDENCE:** og gates re-read first-hand at the decompile lines above
  (the S-063 verification trail's own sites); core stance pinned by
  `NtfsVolume.OddSectorRecordWalkScans` (the 1536-byte odd-stride fixture
  opens, walks, and surfaces its user file — the og's 0x610016 class)
  and `NtfsVolume.OversizePositiveRecordOpensAndWalks` (the 17-sector
  8704-byte stride opens and walks, per-record validation carrying the
  safety) beside the adjacent P1-4 pin
  (`NtfsVolume.BootNegativeExponentOverflowRejected`).

### D-52 ADDENDUM — the spc shift-decode threshold: the og is internally inconsistent; the core's signed-byte form is the registered choice (N-197)

- **THE OGRE'S OWN THREE FORMS (all four sites re-read first-hand):**
  NTFSSearchDLL FUN_18003f5d0 (:51038-51049) shifts only when the high
  nibble is 0xF — bytes 0x80..0xEF are taken as DIRECT sector counts
  (0x85 -> 133 sectors/cluster), 0xF0 -> 1<<16, 0xFF -> 2. NTFSLib
  (:19307, `bVar2 < 0x81 -> raw else 1 << (-bVar2 & 0x1f)`) and
  NTFSFileSystemAnalyser FUN_1800048c0 (`0x80 < bVar1 -> shift`) shift
  at >= 0x81 — the majority form the corpus artifacts document.
  PartitionRecoveryD (:2323-2329) uses the RAW byte with no decode at
  all (the lost-scan's 4-record MFT read computes MFT positions from the
  raw count). The parent row's "spc signed-byte decode" wording cited
  LoadNtfsVolume as *the* og form — an overclaim this addendum corrects.
- **THE CORE'S REGISTERED CHOICE:** the signed-byte convention (the
  NTFSLib/analyser majority): positive -> direct count, negative ->
  `1 << ((-spc) & 31)`, extracted as the pure wire helper
  `sectors_per_cluster_from_dbr` (cr_ntfs_wire.h) and pinned by
  `NtfsVolume.BootSpcShiftDecodeConventionPinned` (0x01/0x40/0x7F direct;
  0xF6 -> 1024 the canonical real-world shift form; 0xFF -> 2; 0x85 ->
  2^27 under the chosen convention where the scan controller takes 133
  raw). On a 0x80..0xEF spc byte with positive cpr, the og's own scan
  controller computes different MFT addresses than the core — deep-edge
  (shift-form spc implies >= 256-sector clusters; rare even in synthetic
  images), wrong MFT addressing only on that geometry.
- **THE 0x80 BOUNDARY NUANCE:** every og layer takes byte 0x80 raw (128);
  the signed form shifts (exponent 128 & 31 = 0 -> 1). A nonsense byte
  either way (no formatter emits it); registered here rather than
  silently diverging.
