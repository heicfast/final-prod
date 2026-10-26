# CRSF - Core Recovery Session Format (v1)

Own documented format. The chunked-zlib framing is the scheme PROVEN by the
reference .rsf reversal (docs/ground-truth-contracts.md section 8); our
additions: explicit versioning, per-record CRC32, corruption skip-forward.

## Container

```
+0x00  magic[8]     "CRSFMT01"
+0x08  version u32  = 1
+0x0C  flags u32    bit 0 = ABS_EXTENTS (F-1: RESULT_TREE extents are
                    ABSOLUTE device offsets; absent = pre-F-1 slice-relative
                    basis, loaders normalize via VolumeInfo.slice_offset)
+0x10  record_count u32   (patched on finalize)
+0x14  unit_size u32      = 512 (skip-forward granularity)
+0x18  session_id lo u32
+0x1C  session_id hi u32
+0x20  reserved[32]  zero
= 64 bytes
```

## Record header (24 bytes), then the chunked data area

```
+0x00 magic[4] "CRSR"
+0x04 type u32     1 SESSION_STATE | 2 VOLUME_INFO | 3 PARTITION_CONTEXT
                    4 RESULT_TREE   | 5 ERROR_MAP | 6 ASSISTANT_STATS
                    7 ALLOC_MAP     | 8 DISK_LAYOUT (M-042)
+0x08 session_id u64
+0x10 chain u32    on-disk bytes after the header (patched on finalize)
+0x14 crc32 u32    CRC32 of the DECOMPRESSED payload
+0x18 reserved u32
```

## Chunk framing (payload <= 4096 per chunk)

```
u16 header:  bit 15 = deflate (zlib level 5)
             bits 0-11 = payload_size - 1
stored form is emitted when deflate does not strictly shrink (reference rule)
```

Reader behavior:
- inflate failure -> fall back to the raw payload for THAT chunk
- record CRC mismatch -> record delivered empty + BAD_CHECKSUM rc; the caller
  continues with later records
- broken record header during indexing -> skip forward in 512-byte steps
  until a well-formed "CRSR" header or EOF (resumable by construction)

## RESULT_TREE payload grammar (DFS, parents always earlier)

per node:
```
u32 parent_id (0xFFFFFFFF = top level; attaches under the synthetic
      "Restored" root on load)
u16 reserved
u8  marker (0 light, 1 full)
u8  origin, u8 deleted_state, u8 confidence
u64 record_ref, u32 fs_kind
u64 logical_size, u64 allocated_size
u64 created, modified, changed, accessed   (100ns since 1601)
u32 attributes, u16 flags
u16 name_len + name (UTF-16LE)
u32 extent_count + {u64 offset, u64 length}*
u32 carve_format, u16 carve_confidence
u8  hash_valid + 16B sha-128 id (when valid)
full nodes add: u16 role_len + role (UTF-16LE) + 4x u64 stats
```

## PARTITION_CONTEXT payload (M-042 v2)

```
v1 (25 bytes): u64 start_lba, u64 sector_count, u32 fs_kind,
               u32 score, u8 kind
v2 (41 bytes): v1 + u32 bytes_per_sector, u32 sectors_per_cluster,
               u32 hidden_sectors, u32 flags
               (flags bit 0 = size_from_device_end, bit 1 = geometry valid;
               the og's per-record CSearchedPartitionInfo payload,
               PartitionRecoveryD lostpart.dat +0x38 bps / +0x444 SPC)
the record LENGTH carries the version: v1 sessions (25 B) load with the
extras zeroed and stay re-validatable by classification only.
```

## DISK_LAYOUT payload (M-042)

```
u32 count, then per entry: u64 start_lba, u64 sector_count, u32 mbr_type
```

The og's partinfo.dat semantics: the live whole-disk table at save time.
On the next search the freshly enumerated layout is compared against it
("layout changed" - surfaced as a resume warning, never enforced); every
restored lost candidate is re-read at its start LBA against the CURRENT
disk and stale records are DROPPED before they reach the UI (the og's
PRD:20748-20842 search-start re-validation).
