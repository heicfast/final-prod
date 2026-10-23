# core_recovery

**User-mode Windows data-recovery engine and SDK** — a clean-room C++20
reimplementation of the local recovery-core architecture documented in
[`heicfast/easeus-drw-research`](https://github.com/heicfast/easeus-drw-research),
using [`heicfast/rda-driver`](https://github.com/heicfast/rda-driver) for
locked/damaged-device access and a direct-handle transport where the reference
has that dual path.

Every implemented on-disk structure and heuristic is evidence-linked in
[docs/ground-truth-contracts.md](docs/ground-truth-contracts.md). Deliberate
differences are in [docs/PARITY-DELTAS.md](docs/PARITY-DELTAS.md); the live
release gate is [docs/CORE-GAP-LIST.md](docs/CORE-GAP-LIST.md).

## Core surface

- **Filesystems:** NTFS (including native LZNT1 compressed data), FAT12/16/32,
  exFAT, ext2/3/4, HFS+/HFSX, Btrfs, ZFS, and first-class ReFS MSB-chain
  reconstruction. LDM provides dynamic-disk database decoding and read-only
  simple/spanned/striped/mirrored/RAID-5 volume mappings.
- **Recovery pipeline:** lost-partition discovery and virtual volumes, staged
  quick/deep/carve scans, signature and grammar validators, merge/dedup,
  incremental read-error maps, weighted progress, ETA, pause/cancel and
  checkpoint/resume.
- **Durable state:** versioned CRSF sessions with per-record integrity,
  tolerant salvage loading, strict verification, and atomic byte-preserving
  archival.
- **Output:** immutable mid-scan snapshots, filtering/sorting/paging, bounded
  previews, safe writer with source/destination overlap refusal, empty-directory
  creation, collision naming, atomic commits, post-write hashes and JSON reports.
- **Platform:** ATA/NVMe health, TRIM/SSD interpretation, consent-gated
  transactional RawFixer with undo, BitLocker detection and legitimate Windows
  unlock only, 512e/4Kn geometry and Win7-inclusive import gating.
- **Interfaces:** `rrecov.exe`, static C++ libraries, and the versioned
  `cr_ui.dll` C ABI for scan/session/snapshot/query/preview/recovery services.

The exact per-capability status, including the bounded ZFS multi-vdev gap, is in
[docs/CAPABILITY-MATRIX.md](docs/CAPABILITY-MATRIX.md). No unimplemented surface
is represented as full parity.

## CLI

```text
rrecov list
rrecov scan --disk N | --volume C: [--deep] [--quick-only] [--no-carve]
            [--threads N] [--out file.crsf] [--resume file.crsf] [--json]
rrecov tree --in file.crsf [--filter s] [--deleted-only] [--csv|--json]
rrecov load --in file.crsf [--json]
rrecov verify --in file.crsf [--json]
rrecov save --in file.crsf --out file.crsf
rrecov recover --in file.crsf --disk N --to DIR [--report file.json]
rrecov health --disk N
rrecov repair --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
rrecov bench --disk N [--mode seq|rand|open|read|info] [--json]
```

`fix` remains a compatibility alias for `repair`. RawFixer defaults to analysis
and dry-run; writes require explicit `--apply`. `verify` is strict and rejects
any damaged record, while recovery-oriented loading can salvage intact records
from a damaged container. `save` validates before atomically replacing its
output and preserves unknown records for forward compatibility.

## Architecture

```text
rrecov.exe / cr_ui.dll / C++ callers
                  │
                  ▼
orchestrator ── result tree / snapshots ── query / preview / safe writer
                  │                           │
quick → deep → carve                       CRSF sessions
                  │
NTFS FAT exFAT ext4 HFS+ Btrfs ZFS ReFS LDM lost-partition carving RawFixer
                  │
bounded partition/volume devices + cache + retry/error map
                  │
RDA transport / direct Windows handles / image-file transport for the C ABI
```

Pure engines compile on Linux and Windows. Windows enumeration, transport,
health, destination IO and BitLocker integration are isolated in `src/win`.
The C ABI is implemented in `src/ui`; it is a native core deliverable, not a UI
mock or subprocess facade.

## Build and verification

CMake 3.25+, C++20, zlib, LZ4 and zstd are required. See
[docs/build.md](docs/build.md) and [docs/testing.md](docs/testing.md). CI runs
MSVC `/W4 /WX`, Linux ASan, UBSan and TSan, structure-aware fuzzers, CodeQL,
plain-C ABI lifecycle tests, Windows CLI contract tests and Win7 PE import
checks. Seeded-VHD and reference-comparison workflows supply recovery benchmarks;
results and honest comparability limits are recorded in
[docs/BENCHMARK-REPORT.md](docs/BENCHMARK-REPORT.md).

Remote/cloud/NAS recovery is excluded. Signing is disabled. The production
release workflow packages the executable, C ABI DLL, static/import libraries,
runtime dependencies, public headers, documentation and SHA-256 checksums.

## License

MIT — see [LICENSE](LICENSE). Reference behavior is reimplemented from
understanding; decompiled code and proprietary assets are not copied.
