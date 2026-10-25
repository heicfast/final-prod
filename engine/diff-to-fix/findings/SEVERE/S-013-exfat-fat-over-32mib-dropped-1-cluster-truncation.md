# SEVERE S-013 — exFAT: FAT larger than 32 MiB is silently dropped; every FAT-chained file and directory truncates to ONE cluster

**Severity:** SEVERE (wrong recovery content for a whole volume class)
**Module:** exfat
**Parameters affected:** B2 (recovery correctness — content), B3 (false-positive shape), A27 (fragment handling), X47/X49 (throughput/memory side effects)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c` — FAT window accessor `FUN_180021c60 :28039-28213` (1 MiB cached windows: `operator_new(0x100000)`, entry read `*(int *)(win + (cluster & 0x3ffff) * 4)` at `:28158`, no size cap), boot validator `FUN_180021e80 :28180-28210`
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:499-505` (32 MiB load gate), `:366-370` (no-FAT truncation path), `src/core/exfat/cr_exfat_engine.h` (flags) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The exFAT quick scanner loads the whole FAT into memory only when it is ≤ 32 MiB. On a volume whose FAT exceeds that (any exFAT volume over roughly 34 GB formatted with 4 KiB clusters — `mkfs.exfat -c 4K` default class), `ctx.have_fat` stays false and every file and directory that relies on a FAT chain is reconstructed as a **single-cluster chain** — content beyond the first cluster is silently lost. The og reads the FAT through 1 MiB cached windows with no size cap and recovers full chains on the same volumes.

## What the reference does

`exFATSearchDLL_decompiled.c` `FUN_180021c60` opens the FAT as a windowed table: it allocates a 1 MiB window (`operator_new(0x100000)`) and resolves chain entries through it (`:28158`: `entry = *(int *)(win + (cluster & 0x3ffff) * 4)`), paging windows on demand across the whole FAT. There is **no total-size gate** on the FAT anywhere in the exFAT engine (the boot validator `FUN_180021e80` checks structure — MustBeZero fields, FAT count 1..2, `fat_length != 0`, heap fit — not a size cap). Free-cluster walk (`:28158-28163`) and chain following share the windowed accessor, so arbitrarily large FATs are handled with bounded memory.

## What our core does

```cpp
// cr_exfat_quick.cpp:499-505
if (p.fat_length != 0) {
    u64 fat_bytes = u64(p.fat_length) * p.bytes_per_sector;
    if (fat_bytes <= (32u << 20) &&
        u64(p.fat_offset) * p.bytes_per_sector + fat_bytes <= slice->size()) {
        ctx.fat.data.resize(size_t(fat_bytes)); ...
```

Above 32 MiB the FAT is silently not loaded (`have_fat == false`), and the entry-set walk then reconstructs chains without it:

```cpp
// cr_exfat_quick.cpp:366-370
} else {
    chain = contiguous_chain(first_cluster, 1);   // ONE cluster, unconditionally
}
```

A directory chain degrades the same way (`:360` uses `dir_clusters` only in the no-FAT-chain bit case). No warning, no counter distinguishing "chain truncated due to missing FAT" from genuinely-fragmented files, no scan failure — the volume appears to scan normally.

## Gap

Falsifiable claim: an exFAT volume with > 8,388,608 FAT entries (e.g. 4 KiB clusters on a ≥ 34 GB volume) recovers full cluster chains for fragmented and contiguous files alike in the og, while the core recovers exactly 1 cluster per FAT-chained file and directory (first-cluster content only).

## Impact

- **Recovery correctness:** silent content loss for an entire volume class — the default modern large-volume configuration. Multi-cluster documents, photos, and directory trees come back truncated with no grading signal, which is worse than an error: the user believes the data is all there was.
- **False confidence:** truncation is ungraded (`B3`): the result set reports these as ordinary recoverable files.
- **Scale:** exFAT exists precisely for large volumes; the gate disarms the engine on its home turf.

## Reproduction / verification

- Decompiled evidence (strength 2): windowed accessor with no cap — `FUN_180021c60 :28039-28213`, entry read `:28158` (agent S2-B quote sheet in `evidence/decompiled-quotes/fat-exfat-session2.md`).
- Core evidence verified first-hand at `e93decf` (`:499-505`, `:366-370`).
- Not executed (read-only session); the geometry arithmetic (34 GB @ 4 KiB clusters → FAT > 32 MiB) is exact: 8 Mi entries × 4 B.

## Fix plan (for the engineering team)

1. Replace the whole-FAT load with the og's windowed model: a `FatWindow` accessor owning a 1 MiB (or 64 K-entry) window cache over `fat_offset`, refilling on demand (`cluster & 0x3ffff` indexing per `:28158`), with the same no-cap trust of `fat_length`.
2. Keep a bounded whole-FAT fast path for small volumes if desired (≤ 32 MiB), but make the windowed path the only fallback — never `have_fat=false`.
3. Directory chains must use the same accessor (the og does; `:360`'s special case only applies to the genuine NoFatChain bit, which is already handled before the FAT branch).
4. Add a scan stat `fat_window_refills` for telemetry.

## Tests to add

- `ExFat.LargeFatFullChains` — synthetic volume with a 40 MiB FAT (10.5 M entries): a 100-cluster contiguous file and a 100-cluster fragmented file both recover full length (fails before — 1 cluster each — passes after).
- `ExFat.LargeFatDirectoryWalk` — deep directory tree beyond window 0: all entries enumerated.
- Fuzz: extend the exFAT structure-aware harness with `fat_length` values in the 32–300 MiB range (og's FAT32 family window; see M-011) to pin the window math against the analyzer.

## Related findings

- M-011 (the FAT-family table-size windows — the same class of size gates on the FAT engine, where the core is *stricter* than the og; here it is the exFAT engine being *lossier*) · N-023 (exFAT tz/validators group)

## Open questions

None.
