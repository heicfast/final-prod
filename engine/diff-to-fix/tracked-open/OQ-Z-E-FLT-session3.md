# OQ-Z + OQ-E + OQ-FLT (tracked-open) — session-3 storage/census parallel leg residual probes

> **Reconciliation note (2026-10-26):** OQ-FLT-1 extends OQ-S4-1 (grading
> extension table). OQ-T8/T9 are RESOLVED by the lead legs (N-031 dispatch
> decode; M-017 scoring decode). OQ-E1 gates S-025 (journal reparenting);
> OQ-E2 gates S-026 (meta_bg); OQ-Z1 is ACUTE-gated (dsl offsets).

**Status:** OPEN · **Filed:** session 3 (2026-10-26)
**Context findings:** M-023 (ZFS SA), S-018/S-019/S-020 (ext4), M-019/S-016 (filter), I-010/I-016 (resolutions)

## OQ-Z1 — ZFS dsl_dir/dsl_dataset offset constants (ACUTE-gated)

Our constants: `head_dataset_obj@8, child_dir_zapobj@24, ds_bp@128`
(`src/core/zfs/cr_zfs_engine.cpp:235-237`). Kernel reference:
`dsl_dir_phys {head@8, parent@16, origin@24, child_dir_zapobj@32}` and
`dsl_dataset_phys {ds_bp@32}`. If OUR constants are wrong, all dataset roots
misread (ACUTE); if right, the og's offsets differ from the kernel and must be
documented for parity. **Probe:** a real single-disk pool image (zpool create
on a file, then compare both engines' dataset walks) + decode the og's
dsl_dir/dsl_dataset field reads in ZfsSearchDLL_decompiled.c around the MOS
walk (:19934/:19983 region).

## OQ-Z2 — `ZfsDecider::addBlock` from_txg/birth_txg gate (:2162-2170)

Determine whether the og implements snapshot/clone block attribution
(txg-gated). If yes, decide whether our newest-txg-only rule
(cr_zfs_engine.cpp:24-26) loses og-visible content.

## OQ-Z3 — Multi-vdev DVA resolution

Does the og resolve `dva_vdev != 0` to another VdevLoc, or is it equally
single-vdev (OQ-T6 extension)? Our skip is at :408-410/:938-942.

## OQ-Z4 — Og ZFS decompression census

Enumerate the og's decompression routines among the 981 functions (lzjb
`0x84`-style tables, lz4 copy loops, zlib strings were NOT surfaced) to
confirm codec parity with our {lzjb, zle, gzip, lz4, zstd} set. Note: our
census found no zlib/lz4 strings in ZfsSearchDLL — the og may not decompress
at all (which would make our writer-side decode a superset).

## OQ-Z5 — CZfsSearchController vs CZfsQuickSearch delta

Diff the two vtables (:15429/:15472) — the only engine of the four where both
searchers are real classes. Names what ZFS "complete" adds over quick.

## OQ-E1 — ext4 orphan reachability semantics (gates S-018 step 3)

Trace `FUN_1800089b0`/`FUN_180003fb0` (EXT4SearchDLL :6565-6630/:2807-2871):
which parent wins when both journal and live tree claim a child.

## OQ-E2 — ext4 meta_bg stride constant (gates S-019 step 2)

The og seeds the meta-group stride from s_desc_size (:3368-3373, `+0xdf8`)
which looks wrong vs the kernel's `block_size/desc_size * blocks_per_group`.
Build a `-O meta_bg` mkfs image and record which groups each side resolves.

## OQ-FLT-1 — Is the 170-entry extension table engine-wide or exFAT-only?

Byte-search the x9-byte-stride table pattern (DAT_18005a090) in every
SearchDLL binary (NTFS/FAT/EXT4/RAW/Btrfs/ReFS/HFS+/ZFS). Blast radius of
M-019 depends on the answer.

## OQ-FLT-2 — AdvanceFilterCondition field layout (gates N-028)

Decode `FUN_1406f91d0` + the 0x48-byte layout (7 enable-bit groups at dword
offsets 0/4/8/32/60/64 + one more) — exact per-group semantics, the tag
vocabulary's source, and the path matching mode.
