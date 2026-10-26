# A-008 — ZFS dsl_dir child_dir_zapobj read at bonus+24 (dd_origin_obj slot): child datasets silently missing on kernel-layout pools

**Severity:** ACUTE (NEW-FINDING, filed during the DFX25 tracked-open OQ-Z1 resolution — the OQ's original ACUTE-gate was real)
**Module:** zfs · **Filed:** session 25 (2026-10-26) · **Core at filing:** `716d121`-lineage (post `03608d8`)
**Corpus:** `03ad46c` (unmoved)
**Origin:** OQ-Z1 (`tracked-open/OQ-Z-E-FLT-session3.md`) — "dsl_dir/dsl_dataset offset constants (ACUTE-gated)"

## Summary

`cr_zfs_engine.cpp` read `dd_child_dir_zapobj` from dsl_dir bonus **+24**
(`constexpr u32 DD_CHILD_DIR_ZAPOBJ = 24`). The OpenZFS `dsl_dir_phys`
layout (include/sys/dsl_dir.h, verified against master this session) is:

```
dd_creation_time@0, dd_head_dataset_obj@8, dd_parent_obj@16,
dd_origin_obj@24, dd_child_dir_zapobj@32, dd_used_bytes@40, ...
dd_props_zapobj@80, dd_deleg_zapobj@88, dd_flags@96,
dd_used_breakdown@104 (5), dd_clones@144, dd_pad[13] -> 256 bytes
```

Offset +24 is **dd_origin_obj**, not the child zap. The engine's own comment
("origin_obj@16, child_dir_zapobj@24 — dsl_dir.h") was a mis-transcription
that dropped `dd_parent_obj`; OQ-Z1's session-6 "re-verification" repeated
it. The test fixture stamped the same wrong offset
(`put64(bonus + 24, 3) /* child_dir_zapobj */`), so the suite was
self-confirming and green while real pools miswalked.

## Impact (user-visible, silent)

- **Normal pools** (the common case): `dd_origin_obj = 0` for non-clone
  datasets → `child_zap != 0` is false → **no child dataset is ever
  enumerated**. Every `zfs create pool/fs` / subvolume below the root is
  silently absent from recovery results. The root dataset alone surfaces.
- **Clones**: `dd_origin_obj != 0` → the origin's dsl_dir object id is fed
  to `read_zap` as if it were the child-dir zap — a wrong-object read that
  either fails the zap magic (children dropped silently) or walks an
  unrelated object.

This is the exact "all dataset roots misread (ACUTE)" scenario OQ-Z1
originally gated on, dismissed by a mis-transcribed kernel-reference quote.

## Evidence

1. **Kernel ground truth (fetched verbatim from OpenZFS master
   include/sys/dsl_dir.h this session):** field order
   creation_time/head_dataset_obj/parent_obj/origin_obj/child_dir_zapobj →
   child zap at **+32**; struct padded to 256 bytes (matches the og's
   0x100-cap bonus copy). dsl_dataset.h: `ds_bp@128`, padded to 320 bytes
   (matches the og's 0x140-cap copy). spa.h blk_prop: comp@bits 32-38,
   checksum@40-47, type@48-55, level@bits 56-60.
2. **Og decode (ZfsSearchDLL_decompiled.c, read first-hand this session):**
   - `FUN_180017690` (open objset by dsl-dir object, :20313): reads the
     dnode via `FUN_180018db0` (:20415) — the out-struct carries the dnode
     at +0x10 with bonus at dnode+0xC0 and bonuslen (u16) at dnode+0x0A
     (the alias pair `local_14e8`/`local_159e`), both OpenZFS-exact;
   - the first bonus copy is capped **0x100 = 256 = sizeof(dsl_dir_phys)**
     and the second (found by an object-id tree search = the
     dd_head_dataset_obj hop) is capped **0x140 = 320 =
     sizeof(dsl_dataset_phys)**;
   - `FUN_180014ec0` (:194, called with the second copy **+128** =
     `ds_bp@128`) is the blkptr→stream builder: its `*(byte*)(bp+0x37) &
     0x7f` gate is blk_prop bits 56-62 (level/crypt/dedup — the
     indirect-block recursion, 0x80-stride child iteration);
   - the child-dataset enumeration (inside `FUN_180017690`'s type-2 branch,
     :20600-20660): a second dnode read (`FUN_180018db0` :20598) whose
     dnode feeds `FUN_1800165f0` (:20602) — the child-dir ZAP reader —
     with the entry walk skipping **`$FREE`** and a 7-byte name
     (`DAT_180031558` = `$ORIGIN`) and recursing `FUN_180017690` per entry
     value (= the child dsl_dir object id). That is the kernel's
     dd_child_dir_zapobj semantics verbatim (the two reserved names a
     kernel child zap carries).
   - Conclusion: the og implements the kernel chain kernel-exactly —
     dsl_dir(256B) → head_dataset_obj@8 → dsl_dataset(320B) → ds_bp@128 →
     objset; children via the child zap with $FREE/$ORIGIN skipped.
3. **Core source at filing:** `cr_zfs_engine.cpp:312`
   `DD_CHILD_DIR_ZAPOBJ = 24` (reads dd_origin_obj); fixture
   `test_zfs_volume.cpp:521` `put64(bonus + 24, 3)` with the same
   mis-transcribed comment.
4. **Executed fail-before (RED 3/3 on the pre-fix engine, this session):**
   fixture moved to kernel layout (child zap stamped ONLY at +32; nested
   head dataset made resolvable) — `Zfs.EndToEndWalk`,
   `Zfs.ChildDirZapReadsKernelOffset32` (sub dataset missing),
   `Zfs.OriginObjNotTreatedAsChildZap` (origin@24 pointed at the objdir
   zap: the pre-fix engine enumerated a GHOST `root_dataset` dataset from
   the wrong slot — the clone-shape failure mode proven, not inferred).

## Fix

- `DD_CHILD_DIR_ZAPOBJ = 32` (+ the full kernel field-list comment,
  `cr_zfs_engine.cpp:308-320`).
- Fixture: kernel layout (child zap @+32; origin @+24 stamped explicitly);
  the nested head dataset (object 6) now carries a valid bp → the same ZPL
  block so the child walk is observable end-to-end; `restamp_zpl_array`
  extended to re-stamp object 6's bp (it shares the ZPL block).
- Tests: `Zfs.ChildDirZapReadsKernelOffset32` (children enumerate from +32)
  and `Zfs.OriginObjNotTreatedAsChildZap` (origin never followed as a
  child zap; no ghost datasets) — both fail-before executed, both green
  after; `Zfs.EndToEndWalk` now asserts `sub`/`sub/hello.txt`.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's A-008 commit)
**Verified by:** fail-before RED 3/3 executed on the pre-fix tree
(`Zfs.EndToEndWalk`, `Zfs.ChildDirZapReadsKernelOffset32`,
`Zfs.OriginObjNotTreatedAsChildZap` — the third proving the clone-shape
ghost-dataset mode); pass-after 16/16 Zfs suite; full suite **739/739**
linux-release. Kernel offsets verified against OpenZFS master
dsl_dir.h/dsl_dataset.h/spa.h fetched this session; og decode cited in
Evidence §2 (corpus `03ad46c`, ZfsSearchDLL_decompiled.c :20313-20788).
**Resolution notes:** the constant moved from the dd_origin_obj slot (+24)
to the kernel's dd_child_dir_zapobj slot (+32); the mis-transcribed
comment corrected with the full field list so the next audit reads the
true layout; the fixture de-self-referenced (it now encodes the kernel
layout, not the engine's). OQ-Z1 RESOLVED — the session-6 refinement's
"core is kernel-correct on all three constants" claim is corrected:
head@8 and ds_bp@128 were correct, child zap was not; the og is
kernel-exact on all three, so core==og==kernel after this fix. No safety
hardening was touched. The OQ's "real-pool probe" next step is no longer
needed for this question (both the kernel headers and the og decode are
conclusive), though a real-pool integration fixture remains a good future
belt (tracked in the OQ file's resolution note, not as a finding).
