# PARITY-DELTAS — deliberate-deviations registry (AUTHORITATIVE)

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

### D-05 — ZFS deleted-file story: newest-txg live state only (parity decision)

- **WHAT OG DOES:** Reads the newest valid uberblock's live state; deleted content
  left to RAW carving (CoW "copy-on-everything" semantics). Evidence: corpus
  `SearchDLL/zfs-engine--deep-dive.md` §2 ("selects the highest-transaction-g
  valid uberblock — exactly how ZFS import finds the newest pool state").
- **WHAT WE DO:** Same policy, documented in the engine header.
  Evidence: `src/core/zfs/cr_zfs_engine.cpp:24–26` ("Deleted-file story (parity
  decision, AUDIT.md row 26): … the og reads the newest txg's live state only and
  lets RAW carving cover deleted content. We do the same."); AUDIT.md row 26.
- **WHY:** scope (parity with og; old-txg walk out of scope).
- **RISK:** Deleted-ZFS recall equals og (carving-only) — honest parity.
- **VERDICT: ACCEPTED** (both sides cited, documented at the engine).

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

### D-09 — ZFS scoping: single-vdev pools, BE pools refused (G-18)

- **WHAT OG DOES:** Corpus does NOT establish whether `ZfsSearchDLL` assembles
  multi-vdev (mirror/raidz) pools — it documents vdev-label scanning on the given
  device, highest-txg uberblock, MOS/DSL walks, gang/indirect block following
  (`zfs-engine--deep-dive.md` §2–§4). Multi-vdev assembly: **no evidence either
  way**.
- **WHAT WE DO:** Single-vdev pools only; mirror/raidz detected and reported as
  unsupported (`stats.unsupported_vdevs`, `src/core/zfs/cr_zfs_engine.cpp:1144–1145`);
  BE pools refused with a clear error (`cr_zfs_engine.cpp:28–30`); header
  documents scope (`include/cr/zfs/cr_zfs_engine.h:1–32`). Wizard surfaces it
  honestly (G-18, `CORE-CONTRACT.md` §14).
- **WHY:** scope.
- **RISK:** User-visible capability limit; stated honestly per G-18.
- **VERDICT: NEEDS-WORK (evidence, not behavior).** Two actions: (1) pin the
  og-side multi-vdev story from `ZfsSearchDLL` decompile (does it resolve DVAs
  across sibling vdevs?) and record it in this registry; (2) G-18's wording
  "gang blocks counted damaged" is **stale** — `read_gang` is implemented
  (`cr_zfs_engine.cpp:486–491`, AUDIT Round 1: gang header = 3 blkptrs) —
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

### D-19 — Recovery queue: collision naming, atomic writes, hash verify, resume (F11)

- **WHAT OG DOES:** Writes recovered files directly to destination via normal
  file APIs after `CreateDirectoryW` path reconstruction; collision handling not
  evidenced as rename-policy (corpus `architecture/04-recovery-workflow.md` §7.5;
  no collision policy in any artifact — AUDIT-UI §5.7 calls it "unevidenced in
  og").
- **WHAT WE DO:** Collision naming `name (N).ext` + corpus sanitizer
  (`cr_safewriter.cpp:29`), atomic temp+rename via DestIo, SHA-256 verify with
  per-item mismatch accounting, resumable queue (`.part-<id>` suffix, resumed
  counting), zero-fill Skip policy. Evidence: F11 registry; AUDIT row 35 + Wave-2
  fixes; tests (collisions, hashing, resume) per `docs/testing.md`.
- **WHY:** safety/correctness-fix.
- **RISK:** None negative.
- **VERDICT: ACCEPTED** (og-side absence documented as "unevidenced", which is
  itself the honest form of evidence for this delta; our side fully cited).

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
  SEPARATE complete-searcher generation (`FUN_140580720` builds
  `EUCreate{NTFS,FAT,EXFAT,ZFS,HFS,ReFS}Searcher` + `EUCreateRAWSearcher`,
  DRWUI_decompiled.c:916443+); quick results carry into Advanced
  (dynamic-ground-truth: "Advanced Scanning 9% … 1.11M files").
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
  64-KiB blocks) coalesces — device-read count equals the og's bootstrap
  buffer behavior (verified: read-count parity in tests).
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

### D-47 — exFAT entry-set checksum strictness: honest-mode only (N-003)

- **WHAT OG DOES:** the og exFAT engine validates dentry triples
  structurally only — a verified-absence search over all 1,299 decompiled
  functions found NO entry-set checksum loop anywhere (B-exfat 3.4); a live
  file with a torn SetChecksum grades normally.
- **WHAT WE DO:** `ExfatScanFlags::validate_entry_sets` defaults FALSE
  (og parity, N-003): torn-checksum live sets grade Excellent. TRUE (the
  honest-recovery profile) keeps the deliberate strictness — files are
  NEVER dropped, only downgraded to PartialRisk with a sets_damaged
  counter.
- **WHY:** a torn checksum is real corruption evidence the og ignores;
  strictness that never removes content is a recovery-product improvement.
- **RISK:** none while disabled (exact og grading); enabled, grades differ
  from the og by design (the profile is explicit at the call site).
- **VERDICT: ACCEPTED (default OFF = og parity; the divergence exists
  only behind the explicit honest-mode flag, registered per N-003).**

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
the whole DLL). The og's resulting FILETIME therefore depends on the
SCANNING HOST's timezone — a portability defect the og ships.

**Core decision (S-011 fix):** the core's `exfat_time_to_unix100` does NOT
apply the tz bytes either. It carries the on-disk time through the shared
FAT-family datetime core (`fat_datetime_to_unix_seconds`), host-independent,
on-disk-time-as-UTC — exactly what the core's FAT engine already does for
the same wire format (and holds a MATCH verdict for). This matches the og
byte-for-byte on a UTC+0 host and is host-independent everywhere (better
cross-host .rsf portability than the og). The spec-correct UTC
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
