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
| S-002 | SEVERE | Attributes B-tree (decmpfs/named forks) not walked |
| S-003 | SEVERE | Journal not replayed (recent-state loss on crashed volumes) |
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
