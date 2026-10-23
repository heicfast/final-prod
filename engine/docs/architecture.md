# Architecture

> Status: v0.2 - the complete core engine layer (all local filesystems,
> repair, health, benchmarking). See docs/roadmap.md for what ships next.

## 1. Layering

```
rrecov CLI (src/app)                        [Windows]
   │
   ▼
orchestrator (cr::orchestrator)             staged pipeline, sessions, merge
   │ quick ──────────► deep ──────────► carve
   │  (live metadata)  (deleted+remnants+   (grammar carving, dedup F9)
   │                    lost partitions)
   ▼
engines (cr::ntfs / cr::fat / cr::exfat / cr::ext4 / cr::hfsplus /
   cr::btrfs / cr::zfs / cr::refs / cr::lost / cr::carve / cr::repair)
   │ pure logic - identical on Linux CI and Windows
   │ + cr::cr_parallel worker pool (thread-safe tree; parallel carve,
   │   remnant sweeps, lost-partition candidates)
   ▼
result graph (cr::tree) + sessions (cr::session)   + writer (cr::recovery)
   │ dual-class FileNode/FullNode                   CRSF chunked-zlib      DestIo
   │ shared-props hard links                         CRC + skip-forward     atomic+hash
   ▼
bounded storage views (cr::storage)
   PartitionSlice - engines CANNOT read outside their candidate partition
   CachedReader   - MFT/cluster re-read absorption (corpus CStorageDeviceWithCache)
   ReadErrorMap   - R1.6 persistent error tracking
   ▼
platform transports (cr::win)               [Windows]
   RDA driver client (\\.\RDA\<token>\<d>\<p> | Volume\<n>) - PID-token handshake
   direct handles (\\.\PhysicalDriveN / HarddiskVolumeN) - reference duality
   enumeration (geometry 4Kn/512e, GPT/MBR via pure core)
   BitLocker detect + legitimate unlock ONLY (fix F13)
   Win32 DestIo (temp + atomic rename)
```

## 2. Pure core / platform split

Everything under `src/core` compiles identically on gcc (Linux CI, ASan) and
MSVC (/W4 /WX) - no OS headers. Everything Windows-specific lives in
`src/win` and compiles only there. The engines are fed `IStorageDevice`
views; a fake in-memory device (the tests) and the Windows transports are
interchangeable - the same property that let the reference product ship its
"CellTest" harness.

## 3. Key contracts (details in docs/ground-truth-contracts.md)

- Open grammar + PID token: rda-driver wire contract (both sides verified).
- NTFS: USA fixup mandatory; 'FILE' magic; $ATTRIBUTE_LIST chase (depth
  limit); $Bitmap viability grading (fix F5); hard links via per-link props;
  ADS children; remnant sweep (R2.5).
- FAT: flavor by cluster count (spec thresholds); mirrored FAT choose-healthy;
  chain cycle guards; LFN checksum assembly; deleted-chain reconstruction
  with honest confidence caps.
- exFAT: spec-exact dentry layouts; boot-region checksum fill-repeated in
  sectors 11/23; entry-set checksum (16-bit, skip +2); NoFatChain; allocation
  bitmap grading.
- Carving: versioned registry, F8 gate (validator mandatory), grammar
  validators per format, specificity trial order, extent dedup.
- Sessions (CRSF): own format, .rsf-proven chunked-zlib framing, CRC per
  record, skip-forward on corruption.

## 4. Threading model

Scan stages run worker fanned-out where the engine supports it (fixed
cancel-checked loops; cancel wakes paused waiters). The stage weights
(40/35/25) drive monotonic progress. The tree is arena-allocated
with bulk destruction - million-node scans stay memory-disciplined.

**Pause/resume (G-07, D-3 corrected):** two cooperating layers.
(1) LIVE pause: `PausableToken::pause()` parks scan threads at their
loop-head `checkpoint()` calls (every engine polls), with the paused
duration excluded from elapsed time (RE parity); cancel wakes parked
waiters. (2) DURABLE resume: `ScanController::resume(session)` skips
stages completed per the truthful `stage_done_mask` (stamped from ACTUAL
completion at save time - never the configured mask), so a cancelled or
crashed scan re-enters exactly where it stopped. Save-on-cancel: a
cancelled run persists a resumable session when a path is known (the CLI
saves on cancel with `--out`, exits 3; Ctrl-C cancels cooperatively;
`scan --resume file.crsf` continues a saved session; `--out` also wires
the stage-boundary CheckpointSink).

## 5. Error taxonomy

`cr::Err` (0x5002xxxx transport / 0x5003xxxx format / 0x5004xxxx session /
0x5005xxxx writer / 0x5006xxxx FVE / 0x5007xxxx health / 0x5008xxxx repair /
0x5009xxxx internal) - the reference engine convention, extended. Every
failed read lands in the ReadErrorMap and travels with the scan result and
the session record.
