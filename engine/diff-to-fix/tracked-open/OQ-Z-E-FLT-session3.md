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

**Session-6 refinement (2026-10-26) — the OQ's kernel-reference text is
mis-transcribed; the ACUTE-gate assessment changes:**

The core's constants were re-verified against the OpenZFS headers as cited in
the core's own comment (`cr_zfs_engine.cpp:231-237`): `dsl_dir_phys_s =
{creation_time@0, head_dataset_obj@8, origin_obj@16, child_dir_zapobj@24,
...}` and `dsl_dataset_phys` with `ds_bp@128` (after dir_obj/prev_snap_obj/
prev_snap_txg/next_snap_obj/snapnames_zapobj/num_children/creation_txg/
creation_time/deadlist_obj/used/compressed/uncompressed/unique/fsid_guid/
guid/flags — 16 u64s = 0x80 — then the 128-byte blkptr). The OQ's own text
claimed kernel `{origin@24, child_dir_zapobj@32}` and `{ds_bp@32}` — those do
NOT match the OpenZFS layouts (32 is `ds_snapnames_zapobj`, and dsl_dir's
child zap sits at 24). Consequence: **the core is kernel-correct on all three
constants**; the ACUTE risk would require the OG to deviate from OpenZFS AND
us to have copied that deviation — we did not (we are header-correct). The
remaining question is therefore og-quirk DOCUMENTATION, not our correctness:
if the og uses different offsets, it misreads pools the kernel mounts fine
(an og bug to register as informational), not the reverse.

The og-side decode entry point (next session): the MOS walk resolves
"root_dataset" at ZfsSearchDLL_decompiled.c:19934/:19983; the objset-open
plumbing follows via `FUN_180017690` (:20012) into the refcounted objset
wrapper; the dsl_dir/dsl_dataset bonus reads sit in the dnode-bonus accessors
beyond that call — the chain has no ZAP-magic anchors (grep 0x2F52AB2AB /
0x2AB1EAF: absent — the constants are computed or reassociated), so the walk
is function-by-function. The real-pool probe remains the faster conclusive
path.

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

**RESOLVED (closing session, 2026-10-26):** the real-image experiment was run
(e2fsprogs 1.47.2, `mke2fs -O meta_bg` at 1KiB/64B-desc and 4KiB/64B-desc;
dumpe2fs + hexdump byte-matching descriptor contents). The TRUE stride
(meta-group size) is `block_size / desc_size` (EXT2_DESC_PER_BLOCK), NOT
s_desc_size; the og's seed coincides only when desc_size^2 == block_size and
misresolves other geometries. Descriptor block for meta-group m sits at
`first_data_block + (m * meta_group_size) * blocks_per_group` (+1 only for
m == 0; meta-group starts carry NO backup SB — empirically confirmed).
Implemented in the S-026 fix with the og's two defects registered as
non-adopted (PARITY-DELTAS D-58). The question is closed; no residual.

## OQ-FLT-1 — Is the 170-entry extension table engine-wide or exFAT-only?

Byte-search the x9-byte-stride table pattern (DAT_18005a090) in every
SearchDLL binary (NTFS/FAT/EXT4/RAW/Btrfs/ReFS/HFS+/ZFS). Blast radius of
M-019 depends on the answer.

## OQ-FLT-2 — AdvanceFilterCondition field layout (gates N-028)

Decode `FUN_1406f91d0` + the 0x48-byte layout (7 enable-bit groups at dword
offsets 0/4/8/32/60/64 + one more) — exact per-group semantics, the tag
vocabulary's source, and the path matching mode.

---

## Session-14 addendum (DFX14 — pushed forward, unchanged)

The session-3 storage/census residuals stand as filed; no new evidence
this session.
