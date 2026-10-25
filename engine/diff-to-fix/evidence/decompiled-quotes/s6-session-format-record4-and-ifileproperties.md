# Evidence — session 6: session-format record-4 grammar + IFileProperties payloads (first consumption of the never-cited SaveScan artifacts)

**Session:** 6 deep-continuation (2026-10-26) · **Audited core commit:** `022c81d`-lineage · **Corpus:** `03ad46c`
**Cited by:** S-031 (convergence addendum) + M-051 (parallel leg, same census table), N-066, N-059 (convergence addendum), OQ-S6-3. Quotes verbatim from the SaveScan artifacts at `03ad46c`.

## 1. The carved/RAW IFileProperties payload (12,147-record QA census)

`re-workspace/artifacts/SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md:74-90`:

> 12,147 records in the I_ sample — every carved file of the QA disk.
>
> | Serialized | In-memory | Size | Semantics [CENSUS] |
> |---|---|---|---|
> | u8 len + bytes | +0x12 ptr, +0x11 u8 | var | carved file name (assigned by analyser: `shot0017.jpg`, `trainstation-2.jpg` …) |
> | u8 len + bytes | std::string ptr@+0x26, cap@+0x3E, len@+0x83 | var | **formatted timestamp string** — written by `FUN_140c41aa0` via `"%d-%d-%d %d-%d-%d"` (wYear, wMonth, … from a FILETIME arg → SYSTEMTIME). Sample values observed as short strings; the SOURCE FILETIME is not persisted (correction, §5) |
> | qword | +0x1A | 8 | **file size** (e.g. shot0017.jpg = 83,104; max 4.29e9) |
> | u16 | +0x22 | 2 | FLAGS — **constant 0x0020 for all 12,147 records** (every carved file flagged deleted) |
> | byte | +0x25 | 1 | marker (=1 always in sample) |
> | u32 | +0x56 | 4 | analyser checksum/validation — range 1..0xFFFFFFFF incl. **0xFFFFFFFF = invalid/unknown marker** (12,030/12,147 nonzero) |
> | u8 len + bytes | ptr@+0x5A, cap@+0x72, len@+0x82 | var | second string (usually empty; analyser note family) |
> | qword | +0x8C | 8 | header offset / carve anchor A (1,245/12,147 nonzero; 2..1,331,712) |
> | qword | +0x94 | 8 | tail offset / carve anchor B (261/12,147 nonzero; 3,273..757,417) |
> | u32 | +0xF4 | 4 | boolean (2,431/12,147 = 1) |
> | u32 | +0xF8 | 4 | **type-registry ID** (35 for the shot*.jpg carve set — same ID space as NTFS +0x60) |
> | DataRunList | +0x84 ptr (**mutex @+0x9C**) | — | runlist serialized WITHOUT a hasRun flag — the reader consumes a runlist unconditionally; carved files always carry ≥1 run (runlen 6..31,797 bytes) |

## 2. Record-3 device identity card (the 512-byte boot snapshot)

`re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:228-242`:

> +0x004  char[?]  "ST500DM0"             (disk model, padded field)
> +0x108  char[?]  "02-1BD142"            (disk serial)
> +0x206  char[?]  "1.02"                 (firmware revision)
> +0x30A  char[16] "0000000000000000"     (WWN/serial-2 field)
> +0x414  char[?]  "USB Device"           (interface description)
> +0x518  byte[512] MBR/BOOT SECTOR SNAPSHOT (33 C0 8E D0 … 55 AA — full first-sector
>         image incl. partition table; 452 contiguous code bytes observed + 0x55AA magic)
> +…     (remaining ~4.3KB: further fixed fields, zeros in sample)
>
> → **Record 3 is the "device identity card" of the scan**: enough hardware + boot-sector context to (a) verify on restore that the same volume is being re-attached, and (b) re-materialize the partition context without re-enumerating the disk.

## 3. Record-4 tree-blob grammar (per-FS factories)

`re-workspace/artifacts/SaveScan/rsf-record4--tree-node-grammar.md:16,26`:

> **Role**: Companion decode for the record-4 tree blob — group framing, node-type factory, extent encoding, and progress mapping; the .rsf blob that `Load Scan Session` re-materializes without rescanning.
>
> 2. **Data (per-FS) nodes** — the filesystem-specific record for each file/folder (name, size, times, location data). Serialized by `FUN_1400d6fa0` as `[4B type][type-specific body via data-vt+0x98]`.

Plus the save-progress mapping (`:98-100`):

> percent = 0x28 + (processed * 0x32) / (total * 2) — climbs 40% → 65% across record 4

## 4. Core side (verification snapshot, audited commit)

- `src/core/carve/cr_carve.cpp:141-184` — the emission block sets name/fs_kind/logical_size/carve_format/carve_confidence/extents/(optional)sha128; **no deleted state, no times, no validation payload**; `Origin::Carved` at :182 is provenance-only.
- `include/cr/tree/cr_node.h:38,153` — `DeletedState::Active` default; `deleted_ : 4 = DeletedState::Active`.
- `src/core/session/cr_records.cpp:133,139-142,195,206-209,273` — the session persists deleted-state + 4 times + reload path (the gap is upstream at emission).
- `include/cr/session/cr_records.h:49-64` — `VolumeInfo`: model[48], serial[32], `u32 boot_hash` (CRC32 of the first 512 bytes) — no firmware/WWN/interface/snapshot.
- `include/cr/tree/cr_node.h:57-92` — `FileProperties` field set (no type-ID slot; no per-FS secondary payload slot).

## 5. MATCH register (the solid equivalences, for the record)

The same read confirmed the core's session model MATCHES the og on: chunked-zlib framing, marker-byte light/full nodes, name/size/3-4 times/attributes/flags/deleted-marker, extents in all forms (NTFS dataruns, HFS+ forks, ZFS polymorphic, Btrfs dual/compressed encoding ↔ core codec/psize+resident_data), RAW name/format-id/anchor-A — plus EXTRA-IN-CORE: CRC32+skip-forward container, Confidence grading, allocated_size, changed-time, stage-mask resume counters. These are recorded in the session module row (updated this session) and need no findings.
