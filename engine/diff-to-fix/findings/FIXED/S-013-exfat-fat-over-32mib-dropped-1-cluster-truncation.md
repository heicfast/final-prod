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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit; see `git log` for the S-013 fix on main)
**Verified by:** fail-before/pass-after — with the pre-fix source (worktree stashed): `ExfatEngine.LargeFatFullChains` FAILED (doc.txt extent length 512 vs 1536 — the exact single-cluster truncation — and far.dat 512 vs 1024) and `ExfatEngine.LargeFatDirectoryWalk` FAILED (inner.txt, which lives in the directory's SECOND cluster, never found; fardir coverage 512). Post-fix both PASS: full chain extents (1536/1024), fardir walks both clusters (1024 coverage), inner.txt recovered, `st.fat_window_refills >= 1` (the far chain crossed windows). Full suite 322/322 Linux ASan. ASan smoke: 65 hostile iterations (mutated boot geometry, FAT window-1 bytes, random pokes, truncations) driven through the EXISTING fuzz_exfat harness entry (LLVMFuzzerTestOneInput) — no crashes/leaks (local clang/libFuzzer unavailable in this container; the CI fuzz lane runs the real target).
**Resolution notes:** ExfatFat gained the og's windowed mode (FUN_180021c60 :28039-28213: 1 MiB cached windows, `win + (cluster & 0x3ffff) * 4`, no total-size cap): the whole-FAT load remains the fast path for small volumes; EVERY other case (FAT > 32 MiB, FAT extending past device end, or a failed whole read) falls back to the windowed accessor so `have_fat` is NEVER false while fat_length != 0 — the silent one-cluster truncation (`contiguous_chain(first_cluster, 1)`) can no longer fire for any volume with a FAT. Per-window bounds and read failures degrade the affected entries to FREE, which feeds the existing EXFAT-2 assumed-contiguous salvage downstream — bounded degradation instead of silent content loss. Directory chains share the accessor via read_dir_chain's existing follow_exfat_chain path (fix plan step 3 was already satisfied structurally; the test pins it). `fat_window_refills` added to ExfatScanStats (fix plan step 4, drained at scan end). The test fixture taught two honest-assertion lessons recorded in the test comments: logical_size mirrors the STREAM entry on both sides of the fix (the truncation lives in EXTENT COVERAGE — the assertions pin extent length, not logical_size), and the entry walk terminates at the first zero entry, so multi-cluster directory fixtures must tile preceding clusters with valid entries. SetBuilder extended additively for multi-chunk name entries (spec 7.6.3; single-name callers byte-identical, secondary-count normalized in finish_set; deleted-set name flipping now covers every chunk). Safety hardenings: none near this change; EXFAT-2 salvage semantics untouched. Win7-inclusive gate unaffected (no new imports).
