# modules/ldm.md — partition table + LDM module audit verdicts (table.md covers MBR/GPT/EBR)

Reference: Partition.mo / LdmManager.mo / PartitionRecoveryD per corpus artifacts
`PartitionRecovery/mo-module-map.md`, `PartitionRecovery/rawfixer-v2--method-inventory.md`,
`SearchDLL/zfs-vdev--mftmirr--calcpartition.md` §2–3; og binaries themselves (A-001
binary evidence).

Core (audited commit `67193b6`): `src/core/table/` (cr_table_parse, cr_ldm, cr_crc32 —
1,900 LOC) + `include/cr/table/` + `tests/unit/test_crc32_gpt.cpp` (334 LOC) +
`test_ldm.cpp` (506 LOC) + `test_refs_ldm.cpp` (81 LOC).

## Behavioral

- **B1 detection** — **A-001** (six wrong GPT type GUIDs — executed proof: GPT dynamic
  disks fail LDM discovery; ESP/MSR/Linux misclassified); `M` on MBR parsing (0x42
  dynamic detection at fixed 6×512 metadata offset — works), GPT header/entry CRC
  validation, backup-GPT salvage at device end, GPT_ENTRIES_READ_CAP.
- **B2 recovery correctness** — `M` on MBR-dynamic path (PRIVHEAD/TOCBLOCK/VMDB parse,
  VBLK composites with kernel-quirk parity, fragment rules, dup-fragment → broken
  group); **OQ-T7** (RAID-5 rotation vs LdmManager.mo — left-symmetric assumed, og
  undecoded); re-anchoring per member (per-member PRIVHEAD lds) implemented.
- **B3** — `M` unknown VBLK types counted (not silent).
- **B8 multi-volume/cross-partition** — `M` spanned/mirrored/stripped/RAID-5 layouts
  resolved; unresolved kept as Broken (no silent drops); cross-disk member registry in
  CLI. **A-001** blocks the GPT side of this entirely.
- **B6 damaged-volume** — **M-002** (EBR torn-chain discards walked volumes);
  `M` on two-step GPT salvage + backup-at-end.

## Constants and formats

- **C19** — **A-001** (the GUID constants); `M` on MBR/EBR/GPT signatures, PRIVHEAD/
  TOCBLOCK/VMDB magics, VBLK types 0x32/0x33/0x34/0x44/0x35/0x45/0x51.
- **C20** — `M` (header/entry layouts, end-of-disk PRIVHEAD fallbacks
  last−2048+1856/+2047/last−1).

## Algorithms

- **A30 lost-partition inputs** — `M` (partition table feeds the lost scanner; A-001's
  misclassifications pollute these inputs — fixed by A-001).
- **A27** — `M` on spanned/mirror/stripe reads; **OQ-T7** RAID-5 rotation.

## State machine

- **S32 cancellation** — `NA`-note: no cancel tokens in this module (contrast: FS
  engines poll). LDM config parse is bounded (≤4 MiB) and fast; volume reads during
  recovery are cancellable at the writer. Recorded as note (a multi-terabyte
  end-of-disk PRIVHEAD probe ladder is the only long pole — bounded to 3 reads).

## Performance

- **X51** — `M` sector reads, ≤4 MiB LDM config, 1 MiB read_exact chunks.

## Safety

- **F59/F62** — read-only module (no writes) — trivially safe.

## Quality

- **Q65 tests** — strong on parse paths (20 tests); gaps pinned by A-001 (canonical-GUID
  fixtures!), M-002 (torn EBR), EBR entry-0-link variant, degraded LDM paths,
  end-of-disk PRIVHEAD fallbacks.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| A-001 | ACUTE | Six wrong GPT type GUIDs — GPT dynamic disks get zero LDM recovery (executed proof) |
| M-002 | MAJOR | Torn EBR chain discards already-walked logical volumes |
| OQ-T7 | tracked-open | RAID-5 rotation (left-symmetric assumed) vs LdmManager.mo |

## Post-baseline note

Audited baseline `67193b6` already includes concurrent fixes through `7797bd6`; no
table/LDM files were touched by the concurrent session's later commits at audit time.
