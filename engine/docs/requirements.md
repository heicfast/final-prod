# Requirements — core_recovery v0.1 (locked)

Source: project directive (session 3). This file is the acceptance contract for the
base recovery wizard. Nothing here is optional; nothing here may be shipped as a stub.

## R1 — Device, volume and partition layer (foundation)

1. Physical-disk and removable-media enumeration (all Windows disk classes: SATA/NVMe/
   USB/SD via the storage stack).
2. Logical and physical sector sizes — 512, 512e and 4Kn handled correctly. The fixed
   512-byte assumption of the reference EPM driver (F1 in error-fix-registry) is NOT
   inherited; all offsets are byte-anchored, all sector math derives from probed geometry.
3. GPT: primary and backup headers, entry-array CRC32 validation, protective MBR.
4. MBR and extended partitions/EBR chains (with cycle guards).
5. `PartitionSlice` / virtual-volume reader: every filesystem engine receives a bounded
   `ReadAt(offset, buffer)` view and cannot read outside the candidate partition.
   Contract enforced in code, not convention.
6. Strong I/O errors: bad-sector tracking, bounded retry policy, skip/zero-fill only when
   explicitly selected by policy, read-error map preserved in the scan result/session.

## R2 — NTFS engine (highest algorithmic investment)

1. Bootstrap: validate NTFS boot sector (BPB sanity, geometry); locate $MFT; $MFTMirr
   fallback; decode cluster and record geometry.
2. MFT record parser: mandatory Update Sequence Array fixup validation; resident and
   non-resident attributes; runlist decoding incl. sparse runs; $ATTRIBUTE_LIST
   following with strict depth/cycle limit; exact Windows timestamps at 100 ns.
3. File reconstruction: path hierarchy from $FILE_NAME parent references and $I30
   indexes; multiple hard links; distinguish active / deleted / partially overwritten /
   structurally damaged records; $DATA streams including alternate data streams.
4. Recovery confidence: read $Bitmap; score whether recovered extents are still
   unallocated; grade excellent / partial-risk / overwritten / metadata-only. A deleted
   filename is never presented as fully recoverable unless its extents pass viability.
5. Post-format / severe-damage mode: scan for MFT remnants and valid record signatures;
   reconstruct detached records and orphan trees; raw carving as complement only.

## R3 — Lost / deleted partition recovery

1. Candidate-scoring engine scanning for: NTFS boot sectors + backup metadata; FAT/exFAT
   boot regions; GPT backup tables; MBR/EBR structures; plausible boundaries, sector
   geometry, filesystem signatures, metadata consistency.
2. Every result creates a virtual read-only partition candidate; then the normal
   filesystem engine runs against it. The user's partition table is never "repaired" in
   the initial product.
3. LDM/Dynamic disk recognition is additive, below normal GPT/MBR/NTFS priority.

## R4 — RAW carving (grammar-aware, not header-only)

1. Versioned, declarative signature registry (schema documented, validated at load).
2. Large sequential reads; bounded pipeline.
3. Header + footer + internal-grammar validation, never header-only detection.
4. Per-format size caps and corruption handling.
5. Fragmentation-aware recovery where the format supports indexes/tables.
6. Content hashing and deduplication.
7. Structured results merged with NTFS/FAT results — a carved JPEG never appears twice.
8. Initial format set: JPEG, PNG, HEIC, WebP, TIFF, CR2/NEF/ARW, MP4/MOV, AVI, MKV,
   PDF, DOCX/XLSX/PPTX, ZIP/7z/RAR, SQLite. PST/OST only after robust validation.

## R5 — FAT32 + exFAT engines

1. FAT32: deleted directory entries, LFN chain reconstruction, FAT-chain validation,
   backup boot sector, fragmented-file confidence.
2. exFAT: boot-region checksum validation, backup boot-region fallback, allocation
   bitmap, file entry-set checksum, no-FAT contiguous-file behavior.
3. FAT12/FAT16 table model exists (shared IFATTable contract) even if the quick/complete
   engines target FAT32 first — the chain abstraction is flavor-polymorphic by design.

## R6 — Scan orchestration and durable results

1. Quick scan -> deep scan -> carving as distinct, mergeable stages.
2. Weighted monotonic progress, cancellation, pause, resume.
3. Disk-backed checkpoints/results; no multi-million-node in-RAM-only state.
4. Canonical FileNode graph: source range, filesystem origin, path, attributes,
   confidence, preview metadata, recovery state.
5. Filtering by path/type/date/size/confidence.
6. Session container (.crsf): own documented format, .rsf-inspired chunked-zlib records,
   resumable, corruption-tolerant (stored-chunk fallback).
7. Safe preview workers with bounded parsing (out of scope for CLI v0.1 but the node
   graph carries preview metadata slots).

## R7 — Safe recovery writer

1. Recovery queue with collision naming, resume support, atomic destination writes,
   hashes for verification.
2. Hard refusal/warning when the destination overlaps the source disk or source volume.

## R8 — BitLocker-aware handling

1. Detect encrypted volumes (FVE metadata signatures).
2. Use legitimate Windows unlock paths only: PowerShell `Unlock-BitLocker`, manage-bde
   fallback, FVEAutoUnlock registry keys. Never attempt to bypass encryption.
3. Report locked volumes as locked; scanning proceeds only through legitimate unlock.

## Non-functional requirements

- C++20; MSVC /W4 /WX and gcc -Wall -Wextra -Werror both clean.
- Pure logic (wire structures, runlists, chains, table math, grammar classification)
  compiles and is unit-tested on Linux as well — dual-verification like the driver.
- Every constant that exists on disk (magics, type codes, limits) is asserted against
  the RE corpus and cited in docs/ground-truth-contracts.md.
- Reference design defects carry an F-number in docs/error-fix-registry.md and a test.

## v0.2 additions (scope corrections per the project decision)

The v0.1 'deferred' list included several LOCAL features that the v0.2
scope decision moved INTO the core (only REMOTE features stay deferred:
NAS/cloud/RAID/VM). Implemented in v0.2: RawFixer-class transactional
repair (cr::repair), SMART disk health (cr::win::health), the
ext4/HFS+/Btrfs/ZFS/ReFS engines, LDM recognition, PST/OST + RAR5
analysers, the extended carve set (30 formats), the RDA driver transport
with bad-sector isolation, parallel scanning, and the benchmark +
EaseUS-comparison workflows.
