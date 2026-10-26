# MINOR N-047 — Grouped minors: engine cosmetics from the session-3 census (Btrfs subvol naming, HFS+ volume node, file attribute bit 0x80)

**Severity:** MINOR
**Module:** btrfs, hfsplus, ext4 (cross-engine cosmetics)
**Parameters affected:** B11 (node fields/naming), B10 (filename/naming), B13 (report fields)
**Corpus references:** BtrfsSearchDLL_decompiled.c — per-subvol `CSearchedBtrfsPartitionInfo` (fs kind 0xb) named from SB data :471-474, :512; HFSPlusSearchDLL_decompiled.c — volume node named "HFSPlus" with `HFSPlusSystemNodeProperties` :8798-8813; Btrfs file attribute bits: files get **0x80** (FILE_ATTRIBUTE_NORMAL), dirs 0x10 :3574-3577.
**Core references:** `src/core/btrfs/cr_btrfs_engine.cpp:566-569, :583-598` (nested `subvol-<id>`, ROOT_REF names never parsed — deliberate per :588-590 comment); no per-volume node from the HFS+ engine; `cr_btrfs_engine.cpp:748` / `cr_ext4_engine.cpp:502` (files get attribute 0; dirs 0x10). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `N-029` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

Three cosmetic divergences: (1) Btrfs subvolume naming — og surfaces each subvolume as its own partition node with the SB-derived name; we nest `subvol-<id>` and never parse ROOT_REF names; (2) HFS+ volume root node — og emits a named volume node ("HFSPlus"); ours delegates naming to the partition layer; (3) file attribute bit — og sets FILE_ATTRIBUTE_NORMAL (0x80) on regular files (Btrfs verified; ext4-side value UNVERIFIED — only the 0x10 dir bit confirmed); ours sets 0.

## Fix plan

1. Btrfs: parse ROOT_REF names for subvolume display names (the nesting model can stay — the name is the gap).
2. HFS+: product decision on a per-volume engine node; if kept as-is, document the divergence in modules/hfsplus.md.
3. Attribute bit: set 0x80 on regular files in the Btrfs/ext4/HFS+/ZFS emit paths (or document 0 as deliberate; the og's 0x80 is the Windows-idiomatic value).

## Tests to add

- `Btrfs.SubvolumeNamedFromRootRef`, `Btrfs.FileAttributeNormalBit`, `HfsPlus.VolumeNodeNamingDocumented` (pin whichever decision lands).

## Related findings

S-028 (Btrfs same engine), I-014 (parallel leg register)/I-014 (parallel leg register) (the supersets ledger), N-020/N-040 (parallel leg) (doc family).

## Open questions

ext4-side og attribute value (only the dir bit is verified) — one string/decompile check during implementation.

## Resolution

**Status:** FIXED (sub-items 1 + 3 implemented; sub-item 2 registered as an architectural decision in modules/hfsplus.md per the finding's own fix-plan option)
**Fixed in commit:** this session's N-047 engine-cosmetics batch
**Verified by:** `Btrfs.SubvolumeNamedFromRootRef` + `Btrfs.FileAttributeNormalBit` (both fail-before executed on the stash-isolated pre-fix engine — 2/2 RED; pass-after 2/2 GREEN) + the adjusted `Btrfs.XattrDosAttribApplied` expectation (0x20 → 0xA0: the archive bit ORs onto the NEW 0x80 file base — the DOSATTRIB application itself unchanged and still asserted) + the full btrfs subset green + full suite green at the batch push.
**Resolution notes:** (1) ROOT_REF names parsed in the root-tree walk (kernel btrfs_root_ref layout: dirid@0, sequence@8, name_len@16, name@18; key.objectid = the child; buffered order-independently and applied post-walk); the subvol node takes the ROOT_REF name with `subvol-<id>` as the unnamed fallback — the nesting model stays (the single canonical tree is the core's architecture; the og's separate-roots rendering is the tree-builder side). (2) HFS+ volume node: registered as a deliberate architectural divergence in modules/hfsplus.md (partition-layer naming; an engine-side "HFSPlus" node would duplicate the volume container one level down) — the finding's "document if kept as-is" branch. (3) Attribute bit: Btrfs files now carry FILE_ATTRIBUTE_NORMAL 0x80 (corpus-verified first-hand this session at BtrfsSearchDLL_decompiled.c :3574-3577 — uStack_92 = 0x80 default, 0x10 when the type byte == 2); the ext4/HFS+/ZFS emit paths STAY 0 — the open question resolved by corpus search: none of those engines' attribute constants are decoded in the corpus (the ext4-side 0x80 hits are inode-size/block arithmetic, the ZFS-side allocation sizes — not attribute emissions), so setting 0x80 there would be invention; recorded as undecoded-not-divergent in the module docs. The Subvol struct's pre-existing (dead) name field is now live.
