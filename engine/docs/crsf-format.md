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
                    9 FS_CHECK (M-041)
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

## VOLUME_INFO payload (v1..v5 — the record LENGTH carries the version)

```
v1 (80 B):  u32 fs_kind, u64 slice_offset, u64 slice_bytes,
            u32 logical_sector, u32 physical_sector,
            char label[32] (UTF-8, NUL padded),
            u64 scanned_bytes, u64 elapsed_ms,
            u32 flags (bits 0-2 = quick|deep|carve CONFIGURED)
v2 (+84):   char model[48], char serial[32], u32 boot_hash
            (the SESS-2 identity card; zeros = unverifiable)
v3 (+256):  char image[256] (S-030: the image target's file path;
            empty for disk/volume targets)
v4 (+608):  char firmware[32], char wwn[32], char interface_desc[32],
            u8 boot_snapshot[512] (N-066: the og record-3 identity axes
            + the first-sector image)
v5 (+8):    u64 save_unix (W1a/R-56: the embedded save time, unix
            seconds UTC — our mirror of the og record-2's SYSTEMTIME
            scanTime, rsf-format--scan-session.md section 5.2 +0x6C;
            the session list shows the recorded time; 0 = pre-v5 file)
```

A reader stops at the length it knows; every absent tail deserializes
as zeros with "not recorded" semantics (never a guess). The W1a
(R-56) session-list surface decodes ONLY this record + SESSION_STATE
per file — the heavy payloads are skipped by type before any payload
I/O (ScanController::read_summary + Reader::peek_record; the og's list
likewise reads record 2 per file, never the 36 MB record 4).

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

## FS_CHECK payload (M-041)

```
u32 error_count
per error: { u32 code, u64 param, u32 record_no,
             u32 path_len, u8 path_hint[path_len] }
u32 fat_copies_compared
u32 fat_entries_checked
u32 clusters_visited
u32 mft_records_examined
u32 family_detected        0/1
u8  family[]               NUL-terminated ASCII
(zero pad to an 8-byte multiple; the writer pads, the reader stops
 at the NUL)
```

The damage-audit check-phase report (the og's FileSystemCheck error set,
codes 0x1001..0x100E: 0x1001 volume-too-small, 0x1002 duplicate claim,
0x1003 orphan / free-but-allocated cluster, 0x1004 size-vs-chain, 0x1005
FAT-copy mismatch, 0x1006 FAT-table invalid, 0x1007 dir-tree audit,
0x1008 $MFT-name, 0x1009 $MFTMirr, 0x100A NTFS version gate, 0x100B
$Bitmap floor, 0x100D DBR invalid, 0x100E catastrophic). `param` carries
the cluster/LCN/version-word context where the og passes one;
`record_no` the claiming record; `path_hint` an ASCII fragment populated
by the dir-walk contexts (0x1007 per-entry, 0x1006's walk hits, 0x1004's
size-vs-chain). OPTIONAL record with the same skip-unknown-types rule:
sessions saved without the check stage simply lack it and the report
reads as empty (cr_crsf.h:70-75; serializer src/core/check/
cr_fscheck.cpp:1009-1044, deserializer :1046-1089 —
`check::serialize_fs_check`).
