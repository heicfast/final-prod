# core_recovery

**User-mode Windows data-recovery stack** — clean-room, production-grade
rewrite of the recovery engine layer of the reference product (device /
partition model, nine filesystem engines, dynamic disks, lost-partition
recovery, grammar-aware carving, durable scan sessions, transactional
repair, the safe recovery writer), built on the
[`rda-driver`](https://github.com/heicfast/rda-driver) kernel transport.

> Research corpus: `easeus-drw-research` (reverse-engineering workspace of
> the reference product). Every on-disk structure, algorithm and heuristic
> implemented here is cross-verified against that corpus and documented in
> [docs/ground-truth-contracts.md](docs/ground-truth-contracts.md).
> Reference defects are catalogued and fixed in
> [docs/error-fix-registry.md](docs/error-fix-registry.md); deliberate
> divergences (with rationale) are registered in
> [docs/PARITY-DELTAS.md](docs/PARITY-DELTAS.md). The parity audit trail
> — every finding, its fix, and its proof — lives in
> [`diff-to-fix/`](diff-to-fix/README.md) (see `SUMMARY.md` for the
> current board).

## What it does

- **Filesystem engines** (live + deleted): NTFS 3.x (MFT walk, attribute
  lists, LZNT1 compression, ADS, hard links, USN harvest), FAT12/16/32,
  exFAT, ext2/3/4 (journal salvage, backup-SB rescue), HFS+ (attributes
  tree, journal deleted-record harvest), Btrfs (subvolumes, compressed
  extents, re-scan cache), ZFS (full uberblock-ring txg history, clones,
  snapshots, compressed pools), **ReFS (first-class: full MSB chain)**,
  and **LDM dynamic disks (first-class: RAID-0/1 assembly, cross-disk
  resolver)**.
- **Lost partitions**: whole-disk discovery with og-shaped candidate
  scoring, parallel candidate scans, consent-gated table apply + undo.
- **Raw carving**: structural grammar validators + a 139-format signature
  table + user-taught signatures; window-aware reads, cross-engine
  occupancy gating, overlap scoring, archive inner-entry inventories.
- **Damage audit & repair**: the read-only FileSystemCheck pass (og error
  taxonomy, never aborts a scan) and transactional RawFixer-class repair
  (analyze / dry-run / consent-gated apply / rollback), plus BCD + UEFI
  NVRAM boot-chain repair.
- **Durable sessions**: the `.crsf` container (CRC'd, chunked-zlib,
  versioned records), checkpoints, cancel/resume with device-identity
  revalidation.
- **Safe recovery writer**: queue, collision-aware naming, atomic
  temp+rename, hash verification, honest per-item truth.
- **Health**: ATA SMART (SAT + CSMI) and NVMe, the 4-status grading model.
- **BitLocker**: legitimate unlock only — live-registry/offline-hive
  auto-unlock, user-FVEK in-stack XTS-AES decryption, and the explicit
  password-dictionary aid. No bypasses, no derivation.
- **Disk images**: flat, VMDK, VHD, VHDX as first-class scan targets.

Out of scope by decision: remote/network recovery (NAS redirector,
cloud) — registered scope exclusion, not a silent gap.

## Quick start

```
# Windows (vcpkg manifest toolchain)
cmake --preset win-x64-release && cmake --build build/win-x64-release

rrecov list                                  # the device census
rrecov scan --disk 0 --whole-disk --out run.crsf --log run.log
rrecov scan --disk 0 --check                 # + the damage-audit report
rrecov tree  --in run.crsf --deleted-only
rrecov recover --in run.crsf --disk 1 --to D:\out --report report.json
rrecov health --disk 0
```

```
# Linux (pure core + tests; the win layer is parse-gated instead)
cmake --preset linux-release && cmake --build build/linux-release
ctest --test-dir build/linux-release --output-on-failure
```

Hosts bind in-process through `cr_ui.dll` / `libcr_ui.so` — the
[connection guide](docs/CONNECTION-GUIDE.md) walks the full lifecycle;
[docs/API.md](docs/API.md) is the surface reference and
[docs/PARAMETERS.md](docs/PARAMETERS.md) the tunables reference.

## Architecture

```
rrecov CLI ──────────────── cr_ui C-ABI shim (cr_ui.dll / libcr_ui.so)
   │                            │  wizard / any C host: opaque handles +
   │  C++20, in-process core     │  cr_ui_* factories + C callbacks — the
   │  (engines are never DLLs;   │  og's in-process EUCreate*Searcher
   │   the shim binds, not hosts)│  factory binding (DRWUI-exe §9.2)
   ▼                            ▼
┌────────────────────────────────────────────────────────────────────┐
│ orchestration: ScanController (quick → deep → carve; repair +      │
│   check pre-stages; merge, grading, occupancy gate, checkpoints)   │
│ results: FileNode graph · query (filter/sort/page) · preview       │
│ sessions: CRSF container (chunked-zlib records, resumable)          │
├────────────────────────────────────────────────────────────────────┤
│ engines: ntfs · fat · exfat · ext4 · hfsplus · btrfs · zfs · refs  │
│   · ldm (dynamic disks) · lost-partition · carve (grammar+signs)   │
│   · check (damage audit) · repair (rawfixer) · apply · bcdfix/     │
│   nvramfix (boot chain) · health · preview · query · writer        │
├────────────────────────────────────────────────────────────────────┤
│ core (pure, cross-platform): wire structures, runlists, fixups,    │
│   FAT chains, GPT/MBR/EBR/LDM tables, signature tables — unit-     │
│   tested on Linux, sanitizer-clean (ASan/UBSan/TSan)               │
├────────────────────────────────────────────────────────────────────┤
│ platform (Windows): enumeration, geometry, RDA grammar + direct    │
│   PhysicalDrive/HarddiskVolume handles (WinPE gate), retry pipe +  │
│   error map, BitLocker (unlock + XTS decrypt), images, quiesce     │
└────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
        rda-driver (\\.\RDA\<token>\<disk>\<part>) or direct handles
```

## Status

Per-capability truth with evidence per row lives in
[docs/CAPABILITY-MATRIX.md](docs/CAPABILITY-MATRIX.md). The honest
remaining gaps: NTFS XPRESS/LZX compressed $DATA (GAP-P1-1 — the og is
LZNT1-only too, no corpus surface exists), ReFS container-map
translation (N-036), ZFS multi-vdev topologies, and the HFS+ attributes
UNVERIFIED-in-corpus items — every one registered with evidence, none
hidden. The parity board (every finding fixed/invalid/superseded with
proofs) is [`diff-to-fix/SUMMARY.md`](diff-to-fix/SUMMARY.md).

CI runs the full matrix on every push: Ubuntu (gcc -Werror; ASan, UBSan,
TSan lanes; fuzz smoke; the win-layer parse gate), Windows (MSVC /W4 /WX,
the Win7 SP1 loader/import gate on every shipped PE, DLL smoke, C
consumer, seeded integration), CodeQL, the release pipeline, and the
driver-attached API benchmark lane.

## Documentation

| Document | Content |
|---|---|
| [docs/API.md](docs/API.md) | API reference — CLI, cr_ui ABI, C++ core, events, error model |
| [docs/CONNECTION-GUIDE.md](docs/CONNECTION-GUIDE.md) | binding, lifecycle, devices, BitLocker, threading, consent |
| [docs/FORMATS.md](docs/FORMATS.md) | formats guide — engines, carving, sessions, images, report |
| [docs/PARAMETERS.md](docs/PARAMETERS.md) | every tunable with defaults and bounds |
| [docs/CAPABILITY-MATRIX.md](docs/CAPABILITY-MATRIX.md) | per-capability status vs the reference |
| [docs/PARITY-DELTAS.md](docs/PARITY-DELTAS.md) | the deliberate-divergence register |
| [docs/architecture.md](docs/architecture.md) | layering rules and module boundaries |
| [docs/ABI-CONTRACT.md](docs/ABI-CONTRACT.md) | the ABI wire contract |
| [docs/testing.md](docs/testing.md) · [docs/build.md](docs/build.md) | test philosophy · build details |
| [diff-to-fix/README.md](diff-to-fix/README.md) | the parity audit trail (findings → fixes → proofs) |
| [worklog.md](worklog.md) | the chronological engineering log |

## License

MIT — see [LICENSE](LICENSE).
