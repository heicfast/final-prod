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
- FAT directory reads batch whole runs (one device call per contiguous chain run).
- The '.' self-entry detection `0x202020202020202E` + attr 0x10 anchors directory identity.
- Cancel callback consulted inside every loop head (responsive cancellation).

## 3. NTFS on-disk contract (corpus: ntfs deep-dive + internals + algorithms-v2)

| Constant | Value | Evidence |
|---|---|---|
| Record magic 'FILE' | 0x454C4946 | lines 21152/34153/34159/55194 |
| Min record parse size | 0x400 | walk clamp (21138) |
| Fixup min size gate | 0x400; offset must not be 0 mod 256 | FUN_180041bb0 chain |
| $ATTRIBUTE_LIST type | 0x20 | FUN_180041850 probe |
| $FILE_NAME type | 0x30 | 52190/53409 |
| $OBJECT_ID type | 0x40 | 51006 |
| $DATA type | 0x80 | 54487; resident cap logic 0xFFF |
| Deleted branch | (flags & 0x10)==0 in walker; record in-use bit maps to props bit 4 | 21186 |
| Metafile filter | name[0]=='$' or flags&4 | 21170 |
| Viability gate | size >= 0x3FF (files < 1 KiB fast-path) | algorithms-v2 §1 |
| Viability rule | all clusters re-allocated in $Bitmap -> destroy; partial -> keep | algorithms-v2 §1 |
| Runlist header | LOW nibble = length-field width, HIGH = offset-field width | Linux fs/ntfs3 run.c (`size_size = b & 0xF; offset_size = b >> 4`); CORRECTED 2026-10: the earlier 'high = length' reading misinterpreted FUN_180003440's order-agnostic size sum (`len += (b>>4) + 1 + (b&0xF)`) - both nibble orders produce the same total |
| Bitmap test | bit = base + index; buffer[pos>>3] & (1<<(pos&7)) | FUN_18002c2e0 |
| MFT batch | 64 KB read buffer (16,388 dwords) | §2 bootstrap |
| fastsearch ini default | ON (`search`/`fastsearch`, default 1) | line 7163 |
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
- LDM (dynamic disks): recognized (type 0x42), not decoded beyond reporting in v0.1.

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
