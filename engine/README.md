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

Out of scope (by decision, see docs/roadmap.md — REMOTE/network recovery only):
NAS redirector (EuMrx-class), cloud recovery (cloud_session/EuICloud), and any other
remote/network feature. Everything LOCAL is in scope: all filesystem engines
(NTFS/FAT/exFAT/ext4/HFS+/Btrfs/ZFS/ReFS), the full LDM dynamic-disk layer, RawFixer-class
repair, and grammar-aware carving.

## Architecture

```
rrecov CLI ──────────────── cr_ui C-ABI shim (cr_ui.dll / libcr_ui.so, G-01)
   │                            │  wizard / any C host: opaque handles +
   │  C++20, in-process core     │  cr_ui_* factories + C callbacks — the og's
   │  (engines are never DLLs;   │  in-process EUCreate*Searcher factory
   │   the shim binds, not hosts)│  binding (corpus DRWUI-exe sec. 9.2), flat-C
   ▼                            ▼
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

Per-capability truth (DONE / PARTIAL / MISSING with evidence per row) lives in
[docs/CAPABILITY-MATRIX.md](docs/CAPABILITY-MATRIX.md) — ReFS and LDM are
first-class (full decode + reconstruction); the honest remaining gaps
(HFS+ journal replay, Btrfs free-space/tree-log recovery, ZFS multi-vdev
reports, NTFS containers/security-attribute surfacing) are registered there,
not hidden.

The UI binding surface is landed: `cr_ui.dll` / `libcr_ui.so` (G-01, the og's
in-process C-export factory pattern), mid-scan immutable snapshots (G-02),
query/filter/sort/paging (G-12), auto-checkpointing (G-13), `rrecov tree
--json` — the wizard binds in-process via
[include/cr/cr_ui.h](include/cr/cr_ui.h) with a plain-C consumer
(`tests/ui_abi_check.c`) proving the full lifecycle on every CI lane.

See [docs/roadmap.md](docs/roadmap.md) and [worklog.md](worklog.md). CI runs the full
matrix on every push: Ubuntu (gcc, -Werror -fsanitize=address on pure core), Windows
(MSVC /W4 /WX + the Win7 SP1 loader/import gate on every shipped PE), CodeQL, release
pipeline.

## License

MIT — see [LICENSE](LICENSE).
