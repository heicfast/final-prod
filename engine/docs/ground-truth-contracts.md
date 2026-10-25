# Ground-Truth Contracts (anti-hallucination anchor)

Every constant, structure and algorithm claim below is verified against the RE corpus
(`easeus-drw-research`) and carries its evidence pointer. If code and this file disagree,
this file wins; if the corpus and this file disagree, that is a documented correction.

## 1. Error-code family (engine convention)

HRESULT-style `0x5002xxxx` engine codes (corpus: `05-scan-engine-architecture.md` §4,
NTFS/HFS quick-search flow). `cr/core/cr_error.h` reproduces the family:
open-fail and read-fail classes; success = 0.

## 2. Volume IO stack (corpus: ntfs-engine-internals.md, fat-exfat v2 §2)

```
Engine reader (MFT batch / FAT chain run / carve window)
  -> CStorageDeviceWithCache equivalent (cr::CachedReader - bounded LRU)
     -> IStorageDevice (cr::IStorageDevice - ReadAt(off, buf, len))
        -> CPartitionVirtualVolumeDevice equivalent (cr::PartitionSlice - bounded view)
           -> transport: RDA driver path | direct Windows handle
```

Contract points (corpus-verified):
- MFT reads are cluster-strided, batched (reference: 64 KB buffers) — never per-record.
- **Cache-coalesced reads are the batching point**: per-record engine reads
  land on the 64-KiB-block `CachedReader` (LRU), so the DEVICE sees
  whole 64-KiB block fetches exactly like the og's bootstrap buffer —
  the coalescing contract lives in the reader, not in the callers
  (corpus ntfs-internals §2; our cr_scan pipe audit, NTFS-2/D-4x).
- FAT directory reads batch whole runs (one device call per contiguous chain run).
- The '.' self-entry detection `0x202020202020202E` + attr 0x10 anchors directory identity.
- Cancel callback consulted inside every loop head (responsive cancellation).

## 3. NTFS on-disk contract (corpus: ntfs deep-dive + internals + algorithms-v2)

| Constant | Value | Evidence |
|---|---|---|
| Record magic 'FILE' | 0x454C4946 | lines 21152/34153/34159/55194 |
| Min record parse size | 0x400 | walk clamp (21138) |
| Fixup min size gate | 0x400; og: low byte of the defensive offset != 0 (mod-256 test, FUN_180046160 :55187-55227); OURS: nonzero + even (hardening) | FUN_180041bb0 chain; AUDIT-NTFS NTFS-3 |
| Fixup validation scope | FIRST TWO sector tails only vs USA[0]; apply restores all | FUN_1800411f0 loop bound `local_18 < 2` (decompiled :52129); apply chain FUN_180041290 (:52452-52463). PARITY-AUDIT F-A1-3 |
| $ATTRIBUTE_LIST type | 0x20 | FUN_180041850 probe |
| $FILE_NAME type | 0x30 | 52190/53409 |
| $OBJECT_ID type | 0x40 | 51006 |
| $DATA type | 0x80 | 54487 (the "resident cap 0xFFF" claim was a corpus misread of an MSVC std::string destructor at Dec 450-477; ours correctly has no cap - AUDIT-NTFS NTFS-4) |
| Deleted branch | (flags & 0x10)==0 in walker; record in-use bit maps to props bit 4 | 21186 |
| Metafile filter | name[0]=='$' or flags&4 | 21170 |
| Viability gate | size > 0x3FF STRICT (0x3ff < size, Dec 21208); == 0x3FF takes the fast-path with NO bitmap check (NTFS-1) | algorithms-v2 §1 + decompile |
| Viability rule | all clusters re-allocated in $Bitmap -> destroy; partial -> keep | algorithms-v2 §1 |
| Runlist header | LOW nibble = length-field width, HIGH = offset-field width | Linux fs/ntfs3 run.c (`size_size = b & 0xF; offset_size = b >> 4`); CORRECTED 2026-10: the earlier 'high = length' reading misinterpreted FUN_180003440's order-agnostic size sum (`len += (b>>4) + 1 + (b&0xF)`) - both nibble orders produce the same total |
| Bitmap test | bit = base + index; buffer[pos>>3] & (1<<(pos&7)) | FUN_18002c2e0 |
| MFT batch | cache-coalesced 64 KiB reads: engine per-record reads through the 64-KiB-block CachedReader (device read count == og's 64 KB bootstrap buffer; NTFS-2/D-4x delta) | §2 + cr_scan.cpp pipe |
| fastsearch ini default | ON (`search`/`ln`, default 1 - U-11 citation precision) | line 7163 |
| MFTMirr cross-check | first 4 records' backup at volume midpoint | 52032/52102 |
| Information scale on IO | legacy x512 convention (driver contract) | rda-driver |

## 4. FAT on-disk contract (corpus: fat-exfat v1+v2)

| Constant | Value | Evidence |
|---|---|---|
| Canonical BAD cluster | 0xFFFFFF7 | IFATTable FAT16 Next |
| Canonical EOC | >= 0xFFFFFF8 | IFATTable FAT16 Next |
| cluster->sector | (c-2)*spc + first_data_sector | chain walker §2 |
| '.' entry signature | ".       " (8 chars) + space at +0xA + attr 0x10 | §2 |
| Deleted dentry marker | 0xE5 first byte | v1 §3 |
| Directory entry size | 0x20 bytes | v1/v2 |
| Cycle guard | visited-range RB-tree; repeat range stops walk | §2 |
| LFN | VFAT slots assembled before 8.3 entry | v1 §3 |

## 5. exFAT on-disk contract (corpus: fat-exfat v2 §3)

| Item | Value | Evidence |
|---|---|---|
| InUse flag | bit 0x80 of dentry type byte; deleted keeps type with 0x80 cleared | §3 |
| Volume-label triple | 0x03/0x83 + 0x20:0x81 + 0x40:0x82 | §3 table |
| Allocation bitmap dentry | 0x05/0x85 | §3 table |
| Stream-extension dentry | 0x40/0xC0; reserved zero at +2,+6/+7,+0x10..0x13; u32@+0x14 == 1 or >= 0xFFFFFF7 | §3 table |
| Filename dentry | 0x41/0xC1, UTF-16 fragments | §3 table |
| Triple reconstruction | walks 0x40-strided entries backward from stream dentry | §3 |

## 6. GPT / MBR (corpus: PartitionRecoveryD.mo, Partition.mo, LdmManager.md)

- GPT header signature "EFI PART" at LBA1; entry array CRC32 (header +0x58, entries
  +0x48 size, 128-byte entries standard, 128 entries default); backup header at last
  LBA; protective MBR type 0xEE.
- MBR: signature 0x55AA at +0x1FE; 4 x 16-byte entries at +0x1BE; extended partitions
  type 0x05/0x0F chains via EBR (each EBR has its own 0x55AA + 2 entries: own + next).
- LDM (dynamic disks): type 0x42 MBR entries / A2A0D0EB-... GPT data partitions;
  the LDM database (last MiB region: PRIVHEAD x2, TOCBLOCK, VMDB, VBLK array)
  is FULLY DECODED (first-class: VBLK graph -> volume layouts ->
  LdmVolumeDevice read paths incl. mirror failover + RAID-5 XOR; history:
  v0.2 shipped recognition only, the full decode landed with session 5+).

## 7. Carving registry (corpus: rawsearch deep-dive §2; raw_signs.json)

- Registry keyed by u32 LE on-disk magic; verified seeds:
  JPEG(Photoshop) `FF D8 FF EE`; PDF `25 50 44 46` + '-' version grammar;
  TIFF family `2A004D4D`/`2A4949`/`554949`/`0F524949`; PNG `89 50 4E 47`;
  RAR `52 61 72 21`; ZIP `50 4B 03 04`.
- raw_signs.json schema per extension: `{header:{signs:[off,hex,confidence]}, tail:{...},
  min, max}` — signs strings are `"<offset>,<hex>,<confidence>"`.
- JPEG grammar: SOF0 (0xC0) dimension check, precision 8 (corpus 39653).
- BruteForce family: formats without crisp tail walk forward until grammar breaks.
- Naming of carved files lives in the host, not the engine (corpus §4).

## 8. Session container (corpus: rsf-format--scan-session.md — fully reversed)

Proven scheme we re-implement in our own .crsf format:
- Container header: magic[16] + version + unit size + record count.
- Records: 0x24-style header (magic + session id + chain length), data follows.
- Chunk framing: u16 header — bit 15 = zlib-deflated, bits 0-11 = size-1, payload <= 4 KB
  (reference cap), stored form when compression does not shrink (explicit comparison),
  level-5 deflate. Reader falls back to raw payload on decompress failure.
- Record roles (ours, documented in docs/crsf-format.md): session-state, volume-info,
  partition-context, result-tree, read-error-map.

## 9. Node graph (corpus: rsf-format §5.4 — CFileNode/CFileSystemNode)

- Dual node class: lightweight file node (reference 0x93 bytes) vs full node (0x290)
  for roots/virtuals only — memory discipline for million-node trees.
- Content objects: refcounted IFileProperties family, `{flags, name, size, times,
  attributes, runlist}` field map (corpus serializer table).
- Marker byte per child on the wire (which node class) — we keep the pattern.

## 10. RDA transport (corpus: rda-driver ground-truth + Device.mo strings)

- Driver open grammar: `\\.\RDA\<PID-TOKEN>\<disk>\<part>`, `...\Volume\<n>`,
  2-segment `\<token>\<disk>`; token = b0|(b0+b1)<<8|(b1+b2)<<16|(b2+b3)<<24.
- READ contract: byte lengths/offsets; Information = length * 512 (legacy scale);
  clients divide by sector size (verified both sides).
- Direct fallbacks (reference duality): `\\.\PhysicalDriveN`, `\\.\HarddiskVolumeN`.

## 11. UI binding surface (our contracts, G-01/G-02/G-12 - the og-parity wire)

The reference wire is an IN-PROCESS C-export factory binding (DRWUI
LoadLibrary's each `*SearchDLL.dll`, GetProcAddress'es `EUCreate<FS>Searcher`
/ `EUCreate<FS>QuickSearcher` -> `ICompleteFileSystemSearchController`;
corpus searchdll-family sec.1 + DRWUI-exe sec.9.2). Our equivalent is the
`cr_ui` shim (cr_ui.dll / libcr_ui.so) - the wizard binds in-process, no IPC.

| Contract | Value | Evidence |
|---|---|---|
| ABI identity | `CR_UI_ABI_VERSION 2u` (v2: M-036 appended cr_ui_smart_report fields; v1 fields/offsets unchanged); every `cr_ui_*` returns u32 from the cr::Err space; thread-local last-error; catch-all guard per export; abi_size-versioned PODs | include/cr/cr_ui.h; src/ui/cr_ui.cpp |
| Config validity | configs are ZERO-INIT VALID (filters/paths/checkpoint optional) - the og factory pattern | cr_ui.h zero-init policy |
| run() threading | runs on the CALLER's thread (the reference UI calls its controllers from its own worker); pause/cancel/telemetry/checkpoint_count safe from ANY thread, any time | cr_ui.h threading block; ScanController telemetry lock |
| Progress callback | fired from scan worker threads, single-flight: serialized, strictly monotonic in overall_permille (stale out-of-order deliveries dropped; stage flips at a steady permille still emit) | scan_progress_bridge_ + CrScan::progress_mu |
| Snapshot (G-02) | `Snapshot::capture(tree, running)` - copy-on-write value capture under the tree mutex; `partial=1` iff captured mid-run; enumeration DFS-identical to the live serializer; recovery-from-snapshot touches zero live-tree state (writer value-enqueue) | cr_snapshot.h/cpp; ADR-UI-SNAPSHOT |
| Query (G-12) | `query::run` - name-substring / deleted-only filter, sort keys, offset+limit (limit 0 = all rows); stable emission order when SortKey::None | include/cr/query/cr_query.h |
| `tree --csv` (frozen) | one row per file: `"full\path",logical_size,origin,deleted_state,confidence,first_extent_offset` - NO header row | rrecov.cpp print_tree |
| `tree --json` (frozen, G-12 projection) | `{"total":<matched>,"rows":[{"path":"..","name":"..","size":N,"modified":N,"origin":N,"deleted":N,"confidence":N,"category":N}...]}` - all strings escape_json-escaped; `--csv` grammar frozen byte-identical alongside | rrecov.cpp print_tree_json |
| Enum values (numeric on BOTH csv and json) | Origin: Structured=0 Carved=1 Lost=2 Virtual=3; DeletedState: Active=0 Deleted=1 PartiallyOverwritten=2 Damaged=3; Confidence: Unknown=0 Excellent=1 PartialRisk=2 Overwritten=3 MetadataOnly=4; Category: None=0 Image=1 Video=2 Audio=3 Document=4 Archive=5 Other=6 Email=7 Bookmarks=8 (post-M-023: the Email/Bookmarks rows were missing from this line - doc truth fixed with M-029) | include/cr/tree/cr_node.h:28-36; include/cr/query/cr_query.h:32-40 |
| modified field | FILETIME 100-ns since 1601 (u64; 0 = unknown) - the engines' native unit (cr_fat_quick.cpp:256-259: unix*1e7 + 116444736000000000); the old "Unix epoch SECONDS" claim was FALSE (M-029 fix; a consumer trusting it mis-parsed every timestamp by 369 years) | query::Row; pinned by Query.ModifiedUnitIsFiletimeEverywhere |
