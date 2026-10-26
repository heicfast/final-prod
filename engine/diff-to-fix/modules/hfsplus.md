# modules/hfsplus.md — HFS+ engine audit verdicts

Reference: HFSPlusSearchDLL per corpus artifacts `SearchDLL/hfsplus-engine--deep-dive.md`,
`ext4-hfs-engine-algorithms-v2.md` §3–4, `remaining-engines--ext4-hfs-refs-btrfs-zfs.md`
(HFS+ section).

Core (audited commit `67193b6`): `src/core/hfsplus/cr_hfsplus_engine.cpp` (680 LOC) +
`include/cr/hfsplus/` + `tests/unit/test_hfsplus_volume.cpp` (4 test areas — thin).

## Behavioral

- **B1 detection** — `M` on the verified gate: signature "H+"(0x482B)/"HX"(0x4858),
  `(blockSize & 0x1FF)==0`, `version < 0x11`, alloc/extents fork clamps ≤ totalBlocks
  (decompile :10005–10010 agent-B verified); **N-005** (extra 512..65536 bounds the og
  gate does not have at that site).
- **B2 recovery correctness** — `M` catalog leaf walk with cnid parent chains,
  sign-extended record kinds, extents-overflow follow for >8-extent forks (og
  CHFSPlusExtents parity); **S-002** (decmpfs content), **S-003** (journal recency).
- **B3** — `M` record-kind validation; freed-node sweep for deleted grading.
- **B4 deleted classification** — `M`-note: catalog records are overwritten on delete
  (unlike NTFS); both sides rely on freed-node/orphan sweeps + RAW; core's freed-node
  sweep + alloc-file grading implemented; og's exact cnid-sweep mechanics not decoded
  (B OQ-3) — no contradiction demonstrable.
- **B5 orphan** — `M` (reparent-to-lost for unresolved cnids).
- **B7 resource forks** — `M` `..namedfork/rsrc` convention (og deep-dive §4 parity);
  **S-002** for named attributes.
- **B9 timestamps** — `M` 1904-epoch fold (VH times → 100ns-1601).
- **B10 Unicode** — `M` HFS+ names are UTF-16BE on disk → decoded correctly (no byte-lift
  issue on this engine).

## Constants and formats

- **C19/C20** — `M` (VH field offsets, fork layout 80-byte, B-tree node header 14 bytes,
  records-from-end growth).
- **C21** — `M` FsKindCode::HfsPlus = 8 = og factory code 8.
- **C23** — `M`-note the og's 0x5017900000002 error style is an og-internal HRESULT
  family; core's `cr::Err` family is a documented equivalent (see abi module).

## Algorithms

- **A26** — `M` catalog B-tree leaf-chain walk (sibling lists, per-node cancel/EOF gates);
  the og's exact "leaf first-record math" (base+1+record_count+attr_count) is corpus-
  INFERRED field naming — core's btnode record-offset-array walk is the spec-correct
  equivalent; no observable contradiction found.
- **FastUnicodeCompare / collation** — `NA`-note: og collation for HFSX key search not
  decoded in corpus (B OQ-1/2); core does no key-search collation (linear leaf walk, no
  B-tree search) — a full-tree walk sidesteps collation; recorded as a note, not a
  divergence (both engines enumerate rather than search).

## State machine

- **S32** — `M` per-node cancel gates (og vt[0x10] per record).
- **S35** — `M` fork clamps; journal-failure → non-fatal (og parity unknown, safe).

## Performance

- **X51** — `M` per-node reads + allocation-bitmap pass; og CHFSPlusFileCache hot-node
  cache ≈ core's CachedReader (64 KiB LRU) — equivalent device traffic.

## Quality

- **Q65 tests** — THIN (4 areas). The two SEVERE findings each demand new test families;
  overall this is the least-tested engine relative to its feature obligations.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| S-002 | SEVERE | Attributes B-tree (decmpfs/named forks) not walked — **FIXED `c9e8860` (session-2 verified)** |
| S-003 | SEVERE | Journal not replayed (recent-state loss on crashed volumes) — mechanics decoded (evidence addendum `b85c084`), fix deferred |
| N-005 | MINOR | Block-size bounds stricter than og gate |

## Open notes (no corpus basis for a finding)

- HFS+ hard links (special catalog records): og handling not decoded; core surfaces them
  as ordinary records. Both-sides-undocumented — revisit if a corpus decode lands.

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **B9 extra timestamps — I-007 (INFO, superset):** core reads all 4
  HFS+ catalog dates (`create_date`, `mod_date`, `attr_date`, `access_date`)
  via `cr_hfsplus_engine.cpp:494-497` with `mac_to_filetime()`. Og reads
  only 3 (create/content_mod/attr_mod) at
  `HFSPlusSearchDLL_decompiled.c:8012, 8027, 8042`; the 4th HFS+ catalog
  date `access_date` at `+0xCC` is NEVER read. The constant `0x239eae080 =
  9561628800` matches core's `mac_to_filetime` exactly. Core superset.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| I-007 | INFO | Core reads 4 timestamps (incl. access_date), og reads 3 (access_date dropped) |
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/ext4-hfsplus-session2.md`):**
- **S-002 DIRECTION CORRECTED:** the og does NOT decompress decmpfs (no zlib import at all) —
  it surfaces compressed files with the TRUE uncompressed size (decmpfs header offset 8) and
  content = raw resource-fork bytes (type 4) or nothing (type 3). Fix plan replaced:
  metadata pass (UF_COMPRESSED flag 0x20 → rsrc fork data-run + size), not content
  decompression.
- **S-003 DIRECTION CORRECTED:** the journal is a **deleted-record salvage**
  (GetDeleteRecord: live-tree differential → deleted records + extents/attributes harvest
  maps, emitted with grade 5), NOT a replay/overlay. Magic 0x4a4e4c78, LE-only, JIB flags
  bit 0, jhdr.size == JIB.size.
- **B3/B5/B7 → N-028 filed** (dotfile/"HFS+ Private Data" sink filter; rsrc-fork superset
  `..namedfork/rsrc`; 'prdf' 0x66647270 folderization).
- **Matches confirmed:** VH gates/read offset, catalog leaf chain, extents-overflow key
  {10,cnid,startBlock}, collation fold, names (UTF-16→UTF-8 with '_' fallback — an og nuance
  the core lacks, cosmetic), 1904-epoch dates, N-005's extra bps bound is core-only (harmless).

### Session-2 addendum (post-reconciliation)
S-002's core half is FIXED (`c9e8860`); audit it against the og-parity boundary recorded in
the FIXED file's session-2 note (og: decmpfs SIZE from attributes tree + raw-rsrc content
for type-4, NO decompression anywhere, type-3 zero content — recovered-content beyond raw
rsrc is a core superset to register). S-003 remains OPEN with two models on the table
(differential salvage vs node-image overlay — see the S-003 session-2 note) plus the
info-block-invalid raw-node salvage fallback both legs confirmed. N-028 carries the sink/
fork minors.

---

## Session-3 parallel-leg addendum (S3-EHB residues; core `34b3441`, corpus `03ad46c`)

Complements S-003/N-028 above (parallel leg's inventories:
`evidence/decompiled-quotes/s3-hfsplus-inventory.md` — 0xFunction-trace method map,
volume header re-verification, catalog decode, journal GetDeleteRecord decode,
attributes-tree remaining sites, names, hard links/hot files/allocation, leaf walk):

- **S-021 (SEVERE):** the journal has a SECOND consumer besides replay — a
  **deleted-record harvester** (`CHFSPlusJournal::GetDeleteRecord` :13637-14252 +
  emit `FUN_18000c8f0` :10017-10127): ring walk → transaction block images →
  leaf-kind gate → tree-by-address-range (catalog→extents→attributes→allocation→
  startup) → emit as kind-5 deleted nodes. The og's PRIMARY HFS+ deleted surface.
  Core parses `journal_info_block` and never uses it (:525; grep "journal" = 1 hit) —
  only the freed-node sweep. Non-overlapping surfaces: implement on the same
  ring-walk substrate as S-003's replay.
- **N-038 (MINOR):** fork-selection predicate — og: record byte +0x29 bit 0x20
  (:7665, quoted) → resource fork + decmpfs mark (+0xce |= 4); core: data fork
  always + empty-fork decmpfs substitution (:693-733). Two disagreement populations
  (marked-not-compressed; compressed-with-data). Field-name UNVERIFIED (TN1150
  layout ambiguity vs decmpfs literature) — one real volume settles. Refines
  N-015b's branch note.
- **I-014 registrations (rows 4-6):** allocation-bitmap grading of freed nodes =
  probable core superset (og's GetDeleteRecord uses the allocation file only for
  address mapping :13815; grading not visible — UNVERIFIED residual, low impact);
  hard links MATCH-by-omission (no 0xF0000000/cnid-15 handling either side); hot
  files MATCH-by-omission (neither walks .hotfiles.btree; og's startup-file use is
  journal address mapping only); decmpfs modes 1/2 MATCH-by-omission.
- **Re-verified MATCH:** volume header gates; catalog leaf walk (A26 — sibling-chain/
  leaf-first geometry unchanged); record-kind dispatch by tree id 3/4/8; dedup map
  {parentCNID, cnid, name} (:2432-2450); names/Unicode B10 rows unchanged.
- **UNVERIFIED residual (new):** §5's decmpfs completion site reads only the size
  (`node->size = *(u64*)(value+8)` :7840-7843) with NO mode-3/4 branch — mode-3
  inline payloads may not be wired to content at this site (the S-002-fixed core
  DOES wire mode 3). Needs one mode-3 fixture against the current core to decide
  MATCH vs core-superset; next artifact: a mode-3 decmpfs xattr fixture + the og's
  marked-node content path re-read at :7880+.

## Minors-fix session update (2026-10-26)

**N-028 group FIXED/REGISTERED** — the og's sink filter (dotfiles + "HFS+
Private Data", FUN_1800057d0 :3714-18) applies at BOTH record sources (live
walk + freed-node sweep — the og's result callback is source-global);
'prdf' fileType folderization (:7687-89); the rsrc-child superset stays
registered (D-53).

**N-038 FIXED** — record ownerFlags (+0x29) bit 0x20 selects the resource
fork as content + the decmpfs completion path (FUN_180009940 :7665/:7677-78),
in both record sources; empty-data-fork substitution stays the fallback.

**N-005 FIXED** — block-size upper bound 64 KiB → 16 MiB (og has none beyond
the 512-multiple mask); NEW device-size product sanity at scan time; ≥512
floor registered as hardening.

**Tests:** +4 (3 fail-before discriminators + product-sanity case). Suite
392/392.


---

## Session-5 fix update (2026-10-26, S-021 + S-003 landed)

- **B4/B5/A25 → S-021 (SEVERE, FIXED):** the journal deleted-record
  harvest is implemented (`journal_harvest_pass` in
  `cr_hfsplus_engine.cpp`) — the og's PRIMARY deleted surface. Mechanism
  per the decompile (all spans re-read first-hand): JournalInfoBlock →
  jnl_header gates (LE; magic 0x4A4E4C78, endian 0x12345678, size
  cross-check) → ring walk (split reads, transaction test
  `first_u16 == (size_a>>4)-1`, 16-byte entries at +0x20, count-1 loop,
  per-entry cancel) → leaf-only images (kind −1/height 1) classified by
  fork ranges (catalog→extents→attributes→allocation→startup) →
  cnid==parent skip → live differential (kept iff not-found or fileID
  differs) → deleted-tier emission with bitmap grading, the N-028a sink
  filter, and extents-branch fallbacks. Tests: 4 (harvest+differential,
  different-fileID, bad gates, ring wrap); fail-before 3/4 on the stashed
  engine; 423/423 ASan.
- **B2/B6/A26 → S-003 (SEVERE, FIXED):** outcome parity via the same
  substrate — pre-crash recent state (files created/renamed before the
  crash, records living only in journal images) now surfaces as
  deleted-tier results, matching the og's kind-5 emission. The "replay"
  mechanism premise is CORRECTED in the audit trail (scratch install +
  unchanged live walk + the :13660-13800 "replay driver" being
  GetDeleteRecord's own walk — three converging first-hand reads). The
  header-invalid fallback salvage (:1690-1790) is tracked:
  `../tracked-open/OQ-S003-journal-fallback-salvage.md`.
- **Hardening registered:** PARITY-DELTAS D-56 (ring bound, size caps,
  no-harvest degradation, grading superset, the tx-magic OQ).

## Fix-session update (2026-10-26 — S-021 + A-005)

- **B4/B5/B2/A25 → S-021 FIXED:** the journal deleted-record harvest landed
  (`journal_harvest` in cr_hfsplus_engine.cpp): JIB gates (flags bit 0, base/size),
  the VH journaled flag (0x2000), jhdr gates ('JNLx' LE magic + endian + size
  cross-check), the ring walk (u16 header test `(blhdr_size>>4)-1`, block lists +
  serial images with wrap modulo journal size, termination at the walk origin +
  corruption step-cap), the og's node-image validator, leaf-only harvest, fork
  classification by VOLUME address ranges (catalog → extents), the catalog
  differential (absent-key or different-fileID keeps; cnid==parentCNID skips), the
  extents side map for empty-fork records, deleted-node emission, sweep dedup.
  Stats: journal_txns / journal_images / journal_records; `journal_harvest=false`
  off switch. Tests: JournalHarvestEmitsDeletedRecords, JournalWrapHandled,
  NonLeafImagesSkipped, JournalHarvestDisabledByFlag.
- **B1/B2/A25 → A-005 FIXED (ACUTE, discovered during S-021's verification):** the
  B-tree record-span convention was inverted vs the on-disk format (TN1150:
  records UP from 14, [end-2] = record 0's offset = 14, ascending offsets, deepest
  slot = free start; the og's journal validator gates exactly this). `for_each_record`
  now takes record i = [slot(i+1), slot(i+2)); `make_leaf_node` packs records UP.
  On real volumes the pre-fix reader emitted only record 0 per node. Guard test:
  `HfsPlus.LeafNodeLayoutMatchesOnDiskFormat`.
- **B OQ-3 resolved:** the og's cnid-sweep mechanics question — the deleted surface
  is the journal harvest (now implemented) + the core's freed-node sweep superset
  (I-014, deduped against the harvest).


## Closing-session verification pass (2026-10-26, df5 lineage)

The journal rows move to MATCH with the corrections (full decode record in
`../evidence/executions/s003-s021-verification-corrections.md`):

| Parameter | Verdict | Note |
|---|---|---|
| JournalInfoBlock layout | **MATCH** | TN1150-verbatim (flags@0 + device_signature[8]@+4..0x23 + offset@+0x24 + size@+0x2C; NO magic field — the og's FUN_18000db70 swap scope, asm-verified; the landed code's 'jnlx'@+4 gate was a real-volume zero-harvest, corrected) |
| Journal entry gates | **MATCH** | kHFSVolumeJournaledBit (og :13456) + kJIJournalInFSMask (og :13513) |
| jnl_header gates | **MATCH** | magic/endian/size-cross-check (LE jnl.c layout; start@+8/end@+0x10 with the walk from end) |
| Ring walk + tx marker | **MATCH** | probe/advance/entry mechanics og-exact; the (blhdr_size>>4)-1 marker DERIVED = block_list_header.max_blocks = (blhdr_size-16)/16 (S-021's open question closed) |
| Node-image validator | **MATCH** | asm-exact strict (mid-chain offset-table violation rejects; record-0 offset == 14; kind/height gates) |
| Attributes branch | **MATCH** | og :14096 differential + decmpfs SIZE-only fallback (payload freed og-side, FUN_180009c10 :7840-42) |
| Header-invalid fallback | **MATCH** | the salvage ladder (:1715-1760 + FUN_180003ea0 = scratch-node harvest, never a live-tree install); OQ-S003 RESOLVED |
| Extents-overflow record decode | **MATCH** | A-007: 12-byte key + 64-byte descriptors at key+12 (was off-by-two — silent fragmented-tail loss) |
| tx entry loop | **MATCH (og quirk)** | the og's count-1 loop replicated for parity (TN1150's replay is inclusive — the og defect registered in-code) |

## Session-22 N-047 decision: volume-node naming stays partition-layer (REGISTERED divergence)

The og's HFS+ engine emits its own named system node — an
`HFSPlusSystemNodeProperties` with the literal name "HFSPlus"
(HFSPlusSearchDLL_decompiled.c :8798-8813: `FUN_180006720(puVar2 + 3,
"HFSPlus", 7)` attached under the volume's node). The core deliberately
does NOT: volume-level naming belongs to the orchestrator/partition layer
(the root role the scan tree receives), so an engine-side "HFSPlus" node
would duplicate the partition node one level down (two named containers
for one volume — the merged-tree model the core's consumers query).
Decision per the finding's own fix-plan option 2: keep the architecture,
register the divergence here. The engine's per-FILE naming, attributes,
and extents are unaffected (all MATCH rows above). Revisit only if a
consumer needs engine-attested volume identity on the node itself.
