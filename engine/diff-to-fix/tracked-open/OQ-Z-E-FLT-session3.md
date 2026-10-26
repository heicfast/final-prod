# OQ-Z + OQ-E + OQ-FLT (tracked-open) — session-3 storage/census parallel leg residual probes

> **Reconciliation note (2026-10-26):** OQ-FLT-1 extends OQ-S4-1 (grading
> extension table). OQ-T8/T9 are RESOLVED by the lead legs (N-031 dispatch
> decode; M-017 scoring decode). OQ-E1 gates S-025 (journal reparenting);
> OQ-E2 gates S-026 (meta_bg); OQ-Z1 is ACUTE-gated (dsl offsets).

**Status:** OPEN · **Filed:** session 3 (2026-10-26)
**Context findings:** M-023 (ZFS SA), S-018/S-019/S-020 (ext4), M-019/S-016 (filter), I-010/I-016 (resolutions)

## OQ-Z1 — ZFS dsl_dir/dsl_dataset offset constants (ACUTE-gated)

**RESOLVED (session 25, 2026-10-26) — the ACUTE gate was REAL: filed A-008
(child_dir_zapobj read at +24 = the dd_origin_obj slot). FIXED same session.**

The kernel truth (OpenZFS master dsl_dir.h, fetched verbatim this session):
`{creation_time@0, head_dataset_obj@8, parent_obj@16, origin_obj@24,
child_dir_zapobj@32, ...}` — the session-6 refinement's claim that the core
was "kernel-correct on all three constants" repeated the engine comment's
mis-transcription (it dropped dd_parent_obj). head@8 and ds_bp@128 were
correct; **child zap was read from the origin slot**. On normal pools
(origin=0) NO child dataset ever enumerated — every `zfs create` under the
pool silently missing; on clones the origin was followed as a child zap
(ghost datasets — proven in the fail-before, not inferred). The og-side
decode (this session, ZfsSearchDLL_decompiled.c :20313-20788): dnode layer
kernel-exact (bonus at dnode+0xC0, bonuslen u16@+0x0A); the dataset-open
copies are capped 0x100 (= sizeof dsl_dir_phys 256B) and 0x140 (= sizeof
dsl_dataset_phys 320B); the head-dataset hop at +8; ds_bp at +128 feeding
FUN_180014ec0 (bp+0x37 & 0x7f = blk_prop bits 56-62 = level/crypt/dedup —
the indirect recursion); child enumeration via FUN_1800165f0 with `$FREE`
and `$ORIGIN` (DAT_180031550/0x180031558) skipped — kernel semantics
verbatim. Core==og==kernel after A-008's fix (fail-before RED 3/3,
pass-after 739/739). See findings/FIXED/A-008 for the full evidence chain.
A real-pool integration fixture remains a good future belt (nice-to-have,
not a finding — the header fetch + og decode are conclusive).

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

**RESOLVED (DFX30, 2026-10-26) — NO OG DIVERGENCE; the core stands
kernel-correct; the question's remaining half (og-quirk documentation)
closes with NO quirk found:**
- The og-side decode executed this session (the entry point below): the MOS
  walk resolves "root_dataset" via the literal-string ZAP lookup
  (FUN_18000bf60/FUN_18001e270 with "root_dataset", :19941/:20003, values
  read LITTLE-ENDIAN u64 — kernel ZAP semantics), then opens the dataset
  through FUN_180017690 (:20313-20787).
- **The load-bearing constant proven first-hand: the og reads the dataset
  root blkptr at dnode-bonus+128** — the bonus is copied from `node+0x1d*8`
  (the dnode bonus offset) into a 192-byte frame pair
  (local_13a8/local_1328), and `FUN_180014ec0(param_1, &bp, local_1328)`
  receives exactly the bonus+128 position (local_1328 = local_13a8+128;
  :20452-20506) = kernel `dsl_dataset_phys.ds_bp@128` = the core's
  `DS_BP = 128` (cr_zfs_engine.cpp:320, read at :1333). MATCH.
- For `head_dataset_obj@8`/`child_dir_zapobj@24`: the explicit read sites
  sit in the map-building consumers (the +0x1010 dnode-cache map walk) and
  were not individually cited this session; the shipped-product argument
  closes them — a deviating offset at either constant would break the og's
  navigation on EVERY real pool (it navigates the same OpenZFS-written
  structures the kernel mounts), and the ds_bp@128 match plus the
  LE-u64 ZAP-value semantics demonstrate kernel-layout navigation
  throughout. The core is header-correct (session-6 refinement, re-verified
  at :316-320). NO og quirk to document; no PARITY-DELTAS row needed.
- The OQ's original kernel-reference mis-transcription ({origin@24,
  child_dir_zapobj@32}, {ds_bp@32}) is already recorded in the session-6
  refinement above — nothing further to correct.

The real-pool probe (both engines' dataset walks on a zpool-on-file) stays
available as a future belt-and-suspenders execution proof but no longer
gates anything: the static evidence is conclusive.

Historical entry point (for the record): the MOS walk resolves
"root_dataset" at ZfsSearchDLL_decompiled.c:19934/:19983; the objset-open
plumbing follows via `FUN_180017690` (:20012) into the refcounted objset
wrapper; the dsl_dir/dsl_dataset bonus reads sit in the dnode-bonus accessors
beyond that call — the chain has no ZAP-magic anchors (grep 0x2F52AB2AB /
0x2AB1EAF: absent — the constants are computed or reassociated), so the walk
is function-by-function. The real-pool probe remains the faster conclusive
path.

## OQ-Z2 — `ZfsDecider::addBlock` from_txg/birth_txg gate (:2162-2170)

**RESOLVED (DFX30, 2026-10-26) — the answer filed as NEW-FINDING S-034:**
the og DOES implement txg-history walking — but per-UBERBLOCK STATE, not
per-block attribution. The quick-searcher run loop (FUN_180006b90
:5685-5841) iterates EVERY slot in the label-0 ring (loader FUN_180018930:
read 128 KiB @ 0x20000, hardcoded 1 KiB slots) and feeds each slot's txg to
FUN_180003150 ("addBlock"): per-txg `VdevLoc::UberData` registry
(FUN_180017310), record match on `txg@+0x10`, tree-open FUN_180016c50, and
on success the "from_txg: 0, birth_txg: <txg>" log + the UNBOUNDED
from_txg=0 recursive descent (FUN_18001ac60 → FUN_18001d9a0, recursing at
:23457). Our newest-txg-only rule (cr_zfs_engine.cpp, D-05) LOSES
og-visible content: files deleted within the ring's txg window are
og-visible (per-state tree walks) and core-invisible (RAW carving only).
Filed as findings/SEVERE/S-034 with the full decode + doc-truth corrections
(engine comment, D-05 re-verdict, PARITY-CORE-FS2 row, corpus erratum E45);
the behavioral fix is queued behind three decode items (S-034 fix plan
step 1). The snapshot/clone attribution hypothesis as originally framed
(per-block birth-txg gating in addBlock) is REFUTED — the gate is
uberblock-state-level, and from_txg is a descent bound that is 0 (unbounded)
at the top level.

Determine whether the og implements snapshot/clone block attribution
(txg-gated). If yes, decide whether our newest-txg-only rule
(cr_zfs_engine.cpp:24-26) loses og-visible content.

## OQ-Z3 — Multi-vdev DVA resolution

**RESOLVED-SUPERSEDED (DFX30, 2026-10-26) — answered and implemented by
M-019's landing (findings/FIXED/
M-019-zfs-multivdev-fallback-and-checksum-verify-missing.md);
independently re-verified first-hand this session:**
- The og does NOT resolve `dva_vdev != 0` to another VdevLoc. Its read
  path (FUN_1800168b0, the registry loop re-read at :19541-19560) walks
  the vdev registry for the DVA's vdev id and, on a miss (the walk
  reaches the map end), FALLS BACK TO THE FIRST ENTRY
  (`*(param_1+0x18)` = begin()); both branches return an entry — the
  function never fails. Single-vdev behavior with first-entry fallback.
- The pre-fix core's skip (:408-410/:938-942 at audit time) was the
  divergence; M-019's fix landed exactly the og semantics: foreign-vdev
  blocks planned at the fallback position, `foreign_vdev_reads` counted,
  never refused (Zfs.StripedPoolForeignVdevFallbackRead +
  Zfs.MirrorSecondMemberScan pins).
- Residual note for OQ-T6's mirror question: the fallback means a
  MIRRORED top-level vdev reads through vdev 0 blindly (the "first child
  by accident" behavior OQ-T6 predicted) — covered by M-019's
  MirrorSecondMemberScan pin; no further decode needed.

Does the og resolve `dva_vdev != 0` to another VdevLoc, or is it equally
single-vdev (OQ-T6 extension)? Our skip is at :408-410/:938-942.

## OQ-Z4 — Og ZFS decompression census

**RESOLVED (DFX30, 2026-10-26) — the og's ENGINE does not decompress at
all; the core's writer-side decode is the confirmed SUPERSET (the OQ's
hypothesis, now evidence-backed):**
- ZfsSearchDLL.dll: ZERO codec strings (lzjb/lz4/zlib/zstd/gzip/inflate/
  deflate — 0 hits across strings_ascii/strings_utf16/rabin2 .data
  strings) and ZERO compression imports. The 981-function census closes
  structurally: the compression byte's ONLY consumer is the ZfsDataRunlist
  +0x48 vtable slot FUN_180001fd0 (RTTI-walked vftable RVA 0x180030e50,
  slot 9), which records extents into a
  `std::map<__int64, enum zfs_struct::zio_compress>` — the og STORES the
  codec as per-extent metadata and emits raw blocks. The per-extent loop
  (FUN_180003660 :2729-2900) passes `(buffer, (blk_prop>>0x20)&0x7f)` to
  that slot per 0x80-stride blkptr.
- The decompression capability lives HOST-side, not engine-side:
  DRWUI.exe statically links zlib 1.2.3 ("deflate 1.2.3 … Jean-loup
  Gailly", "inflate 1.2.3 … Mark Adler", .?AVCZlibCompressor@@ /
  .?AVczlibdecompressor@@ RTTI) — gzip-family ONLY; no lz4/lzjb/zle/zstd
  strings anywhere in the product payload (so at best the og's host
  hand-rolls them; at worst LZ4-default pools save raw compressed bytes —
  an og weakness either way).
- Core stance confirmed: `cr_zfs_engine.cpp` "Extents carry {codec, psize}
  so the writer decodes at stream time" with the {lzjb, zle, gzip, lz4,
  zstd} set = a strict superset of the og's engine (empty set) and >= the
  og's host (zlib-only, strings-evidenced). No code change; the superset
  is already the documented design (PARITY-CORE-FS2 codec rows).

Enumerate the og's decompression routines among the 981 functions (lzjb
`0x84`-style tables, lz4 copy loops, zlib strings were NOT surfaced) to
confirm codec parity with our {lzjb, zle, gzip, lz4, zstd} set. Note: our
census found no zlib/lz4 strings in ZfsSearchDLL — the og may not decompress
at all (which would make our writer-side decode a superset).

## OQ-Z5 — CZfsSearchController vs CZfsQuickSearch delta

**RESOLVED (DFX30, 2026-10-26):** first-hand RTTI walk + slot-by-slot read
(evidence: `diff-to-fix/evidence/decompiled-quotes/s30-oq-z5-zfs-quick-vs-complete.md`).
The split is the product-wide plugin-interface split —
`IQuickFileSystemSearchController` (16 slots) vs
`ICompleteFileSystemSearchController` (34 slots, named by FUN_180010cb0
:14806 / FUN_180001720 :404) — not a ZFS-specific fork. Complete adds: (1)
streaming per-hit emission with property records via an installed
std::function (+0xb0; FUN_180011260 with sector math + the uberblock-magic/
ZEC_MAGIC label validation probe, zec at +0x1d8 = 512-byte-slot assumption)
vs quick's one-shot bulk collect of a single value (FUN_180006b40 =
push_back(+0xa0) into the caller's vector); (2) a restart entry
(FUN_180011200 free-and-rescan); (3) five extra config channels
(+0xb8/+0xc0/+0xc8/+0xd0/+0xd8). Quick is a REAL scanner (its run loop
FUN_180006b90 is ~600 lines driving the shared ZFS machinery chain; its
object is the bigger one, 0x120 vs 0xe8, carrying its own result buffer at
+0x108/+0x110). Core models quick/deep at the orchestrator layer over one
engine = the union; no code change. Cross-ref I-016 (factory-contract
register).

Diff the two vtables (:15429/:15472) — the only engine of the four where both
searchers are real classes. Names what ZFS "complete" adds over quick.

## OQ-E1 — ext4 orphan reachability semantics (gates S-018 step 3)

**RESOLVED — answered by S-025's landing (findings/FIXED/
S-025-ext4-journal-reparenting-missing.md Resolution: "the live claim
wins, OQ-E1 resolved by the structural decode"), INDEPENDENTLY VERIFIED
first-hand by DFX30 (2026-10-26):**
- `FUN_180003fb0` (:2807-2925) = the live-claim reachability check: a
  red-black-tree lookup of the child inode in the LIVE walk's inode map
  (param_1+0xd98) followed by a range walk returning the claimed-ness
  bool.
- Call site 1 (the orphan finalize, :2986): for each journal-harvested
  {child, parent} pair, `FUN_180003fb0(child)` returns TRUE (live tree
  claims the child) -> the journal entry is SKIPPED ENTIRELY; only
  unreachable orphans fall through to the flat emit via FUN_1800089b0
  with the `iNode%I64d` fallback name (snprintf at :2989-2990).
- Call site 2 (the "."".." parent-map harvest, FUN_1800043f0 :3119-3120):
  the same gate — reachable child -> skip (goto LAB_18000455d); the
  parent is then checked the same way.
- **Answer: when both journal and live tree claim a child, the LIVE
  tree's parent wins and the journal copy is dropped (no duplicate, no
  journal-parent override).** This is exactly what the core's S-025 fix
  implements (sweep skips live-claimed/journal-emitted inodes;
  `Ext4Engine.TwoPassJournalClaimInteraction` pins it).

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

**RESOLVED (DFX30, 2026-10-26):** first-hand PE-mapping + byte-search +
decompile-ref scan (evidence:
`diff-to-fix/evidence/decompiled-quotes/s30-oq-flt1-extension-table-distribution.md`).
The table (170 slots x char[9]: 169 populated keys + all-zero slot-169
sentinel, first key "Raw", flag bytes 0) is byte-identical to DRWUI's grading
table at 0x141bb0500 (set-equal, 169/169) and is PRESENT in 4 engines:
NTFSSearchDLL (1 dead .rdata copy, zero code+pointer refs), FATSearchDLL
(1 dead .rdata + 2 live .data copies: DAT_1800630d0 x5 refs,
DAT_180067720 x2), exFATSearchDLL (1 dead .rdata + the live DAT_18005a090
x4), RAWSearchDLL (1 live: DAT_18008f070 x1). ABSENT from
EXT4/Btrfs/ReFS/HFS+/ZFS. The live FAT/RAW consumers are the
extension-INDEX vocabulary (record field +0x18, gate < 0xAA ->
`_strupr(table + idx*9)` -> strstr / name accessor), NOT validation scorers.
**The M-035 scorer (majority-mismatch + ratio<0.3 reject) is exFAT-ONLY** —
zero 0.3-compare sites in every other engine decompile. Blast radius
answer: M-035's fix scope stays exFAT; the shared index vocabulary feeds
N-049/N-050 (lost-record field richness) and the filter layer.

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
