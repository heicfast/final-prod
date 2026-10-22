# core_recovery

**User-mode Windows data-recovery stack** — clean-room, production-grade rewrite of the
recovery engine layer (device/partition model, filesystem engines, lost-partition
recovery, raw carving, durable scan sessions, safe recovery writer), built on the
[`rda-driver`](https://github.com/heicfast/rda-driver) kernel transport.

> Research corpus: `easeus-drw-research` (reverse-engineering workspace of the reference
> product). Every on-disk structure, algorithm and heuristic implemented here is
> cross-verified against that corpus and documented in
> [docs/ground-truth-contracts.md](docs/ground-truth-contracts.md). Reference design
> defects are catalogued and fixed in [docs/error-fix-registry.md](docs/error-fix-registry.md).

## Scope (v0.1 — the base recovery wizard)

| # | Module | Reference source (RE corpus) |
|---|--------|------------------------------|
| 1 | Device / volume / partition layer | Device.mo, DeviceManager.mo, Partition.mo |
| 2 | NTFS engine (quick + complete + deleted) | NTFSSearchDLL, NTFSLib.mo, NTFSFileSystemAnalyser.mo |
| 3 | Lost / deleted partition recovery | PartitionRecoveryD.mo, Partition.mo |
| 4 | RAW carving (grammar-aware) | RAWSearchDLL, raw_signs.json |
| 5 | FAT32 engine (+FAT12/16 table model) | FATSearchDLL, FatLib.mo |
| 6 | exFAT engine | exFATSearchDLL |
| 7 | Scan orchestration + durable sessions (.crsf) | .rsf SaveScan system |
| 8 | Safe recovery writer | RecoveryModule / recovery manager |
| 9 | BitLocker-aware handling (legitimate unlock only) | BitLockerLib (behavior subset) |

Out of scope (by decision, see docs/roadmap.md): NAS redirector (EuMrx), filesystem
repair writers (RawFixer), RAID/VM/cloud, Linux/macOS filesystems, ext4/HFS+/ReFS/Btrfs/ZFS
engines, file-content repair.

## Architecture

```
rrecov CLI / (future wizard UI)
   │
   │  C++20, no engine DLLs — in-process registry (crCreate* factories)
   ▼
┌────────────────────────────────────────────────────────────────────┐
│ orchestration: ScanController (quick→deep→carve, merge, filter)    │
│ results: FileNode graph (dual-class: FileNode/FullNode)            │
│ sessions: CRSF container (chunked-zlib records, resumable)         │
├────────────────────────────────────────────────────────────────────┤
│ engines: ntfs / fat / exfat / lost-partition / carving             │
│ (IQuickFileSystemSearchController, ICompleteFileSystemSearchCont..)│
├────────────────────────────────────────────────────────────────────┤
│ core (pure, cross-platform): wire structures, runlists, fixups,    │
│ FAT chains, GPT/MBR/EBR, signature tables — unit-tested on Linux   │
├────────────────────────────────────────────────────────────────────┤
│ platform (Windows): enumeration, geometry, RDA transport + direct  │
│ handles, PartitionSlice bounded readers, error-map, BitLocker      │
└────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
            rda-driver (\\.\RDA\<token>\<disk>\<part>) or direct
            \\.\PhysicalDriveN / \\.\HarddiskVolumeN
```

## Status

See [docs/roadmap.md](docs/roadmap.md) and [worklog.md](worklog.md). CI runs the full
matrix on every push: Ubuntu (gcc, -Werror -fsanitize=address on pure core), Windows
(MSVC /W4 /WX), CodeQL, release pipeline.

## License

MIT — see [LICENSE](LICENSE).
