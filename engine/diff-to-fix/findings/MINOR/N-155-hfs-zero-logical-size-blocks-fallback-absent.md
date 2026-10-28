# MINOR N-155 — The og's zero-logical-size fallback (dataFork.totalBlocks × 4096) is absent in the core's HFS+ node emission

**Severity:** MINOR
**Module:** hfsplus
**Parameters affected:** B2 (recovery correctness — size field), D11 (node fields — logical size semantics)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c` FUN_180009940 :7666-7671 (the bit-clear path: `size = *(u64*)(param_6 + 0x2c)` [ushort units → byte +0x58, `dataFork.logicalSize`] with the fallback `size = *(uint*)(param_6 + 0x32) << 0xc` [ushort units → byte +0x64, `dataFork.totalBlocks`, ×4096]); the decode table at `diff-to-fix/evidence/decompiled-quotes/s42-hfs-forkfield-resolution.md` §1.1
**Core references:** audited `869da779`. `src/core/hfsplus/cr_hfsplus_engine.cpp:102-110` (`ForkData` — `total_blocks` parsed at :115 but never consumed for sizing), `:1455` (`props->logical_size = content_fork.logical_size` — no zero-size fallback), `:1448-1451` (the N-038 fork-selection landing this rides)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When an HFS+ file record's data-fork `logicalSize` is zero but `totalBlocks` is nonzero, the og sizes the node as `totalBlocks << 12` (blocks × 4096); the core emits size 0. The divergence surfaced from the N-038 field-name resolution this session (the byte-exact record mapping made the fallback legible for the first time). It matters mainly on the journal/deleted-record path, where stale record versions with zeroed size fields and intact block counts occur.

## What the reference does

`FUN_180009940` (the og's catalog-record-to-node emitter, both record sources — the live `CHFSPlusCatalog::ReadData` copies and the journal-walk copies), the ownerFlags-bit-clear (normal file) branch:

```c
/* HFSPlusSearchDLL_decompiled.c :7666-7671 — param_6 is short*, so +0x2c/+0x32 are USHORT offsets */
plVar10 = (longlong *)(param_6 + 0x2c);        /* byte +0x58 = dataFork.logicalSize */
lVar6 = *plVar10;
if (lVar6 == 0) {
    lVar6 = (ulonglong)*(uint *)(param_6 + 0x32) << 0xc;   /* byte +0x64 = dataFork.totalBlocks, ×4096 */
}
LAB_180009acf:
param_1[0x17] = lVar6;                          /* node->size */
lVar6 = FUN_180009f40(..., (longlong)plVar10);  /* the extents resolver, fed the data fork */
```

The og's node size = `dataFork.logicalSize`, and when that is zero, `dataFork.totalBlocks × 4096`. Note the og **hardcodes 4096** — the volume's actual allocation-block size (`volumeHeader.blockSize`, already parsed by the og elsewhere) is not consulted, so on non-4096-blocksize volumes the fallback itself mis-sizes (an og quirk, not a contract to preserve — but the *observable* on 4096-blocksize volumes, the overwhelmingly common case, is exact).

The resource-fork path (ownerFlags bit 0x20 set, the N-038 landing) has **no** fallback: `resourceFork.logicalSize == 0` skips both the size store and the extents resolver entirely (the decmpfs completion path then supplies the size from the xattr header — evidence file §2).

## What our core does

`src/core/hfsplus/cr_hfsplus_engine.cpp`:

```cpp
/* :102-110 */ struct ForkData { u64 logical_size = 0; u32 total_blocks = 0; ... };
/* :115      */ f.total_blocks = be32(p + FORK_TOTAL_BLOCKS);          /* parsed, never used for sizing */
/* :1448     */ const bool rsrc_marked = !is_dir && (r.owner_flags & 0x20) != 0;
/* :1451     */ const ForkData& content_fork = rsrc_marked ? r.rsrc_fork : r.data_fork;
/* :1455     */ props->logical_size = is_dir ? 0 : content_fork.logical_size;   /* no fallback */
```

A record with `logicalSize == 0 && totalBlocks > 0` emits `logical_size = 0` in the tree. The decmpfs path can still override the size for compressed files (:1471+), but the plain zero-size-data-fork-with-blocks case stands at 0.

## Gap

For HFS+ file records with `dataFork.logicalSize == 0 && dataFork.totalBlocks > 0`, the og emits node size `totalBlocks × 4096`; the core emits 0. Falsifiable: construct a catalog file record with `logicalSize=0, totalBlocks=N>0` — the og's emitted node size is `N×4096`, the core's is 0.

## Impact

Recovery correctness, size field only (extents are unaffected — both sides resolve the data fork's extents the same way). The affected population is narrow on live volumes (a healthy zero-length file has zero blocks) but real on the journal/deleted path: stale record versions with a zeroed size field and an intact block count — exactly the records the deleted-file surface recovers — get size 0 in the core's tree (sorting, filtering by size, and recovered-file sizing all see 0). User-observable in the results list's Size column and in the recovered file's allocated size for that population.

## Reproduction / verification

Corpus-derived (decompile, first-hand this session): the exact quote at :7666-7671 with the ushort-units resolution (evidence file §1.1 — the ten-field byte-exact TN1150 mapping that makes `+0x32` = byte +0x64 = `dataFork.totalBlocks` unambiguous; corroborated by `FUN_18000f7b0`'s own `FileSize @ ushort[0x2C]` debug label). Core side: the code quote above (grep `total_blocks` in `cr_hfsplus_engine.cpp` — parsed at :115, no consumer in the emission path). Not executed (no mode-3 fixture build this session; the fix plan's tests provide the fail-before).

## Fix plan (for the engineering team)

1. In `cr_hfsplus_engine.cpp`'s node emission, after selecting `content_fork`: when `!is_dir && !rsrc_marked && content_fork.logical_size == 0 && content_fork.total_blocks > 0`, set `props->logical_size = u64(content_fork.total_blocks) << 12`.
2. Apply the fallback ONLY on the data-fork path (the og's rsrc path has none — evidence §2).
3. Do NOT consult the volume's real block size — the og's observable is the hardcoded 4096; replicate the observable, and register the og's block-size quirk in the test's comment (a volume with blockSize 512 and a 0-size/16-block record sizes 65536 in BOTH implementations — that is the parity target).
4. Tests (fail-before/pass-after): a synthetic catalog with (a) `logicalSize=0, totalBlocks=16` → size 65536; (b) `logicalSize=0, totalBlocks=0` → size 0 (empty file unchanged); (c) `logicalSize=0x1000, totalBlocks=1` → size 0x1000 (logical wins); (d) rsrc-marked with `rsrc.logicalSize=0, rsrc.totalBlocks=16` → size 0 (no fallback on the rsrc path); (e) the deleted/journal-path record shape (d) via the freed-node sweep if the fixture harness supports it.
5. Acceptance: the five tests green; `ForkData::total_blocks` gains its first consumer; no other size paths change (the decmpfs override still wins for compressed files).

## Tests to add

`HfsFork.ZeroLogicalSizeFallsBackToBlocks` (a), `HfsFork.ZeroSizeZeroBlocksStaysZero` (b), `HfsFork.LogicalSizeWins` (c), `HfsFork.RsrcMarkedNoFallback` (d), `HfsFork.DeletedRecordZeroSizeFallback` (e, journal path).

## Related findings

**N-038** (the fork-selection landing this rides — its field-name residual resolved this session, evidence file §1) · **S-002** (the decmpfs completion superset — evidence §2 closes its residual) · **I-014** (the sibling HFS+ residual, still scoped open — evidence §4)

## Open questions

None. (The og's hardcoded-4096-vs-real-blockSize divergence is an og quirk recorded in the fix plan, not an open question — the parity target is the observable.)
