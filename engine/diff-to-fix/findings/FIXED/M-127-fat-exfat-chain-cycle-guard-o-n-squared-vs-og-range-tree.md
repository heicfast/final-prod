# MAJOR M-127 — FAT/exFAT chain cycle-guard complexity: the core's O(n²) visited-vector scan vs the og's visited-range red-black tree — multi-minute stall appearance on large FAT32 volumes

**Severity:** MAJOR
**Module:** fat / exfat (chain walk)
**Parameters affected:** performance — CPU (50), scalability (52), cancellation responsiveness adjacent (53), no-stuck-or-freeze directive
**Corpus references:** the og's FAT chain walker inserts each cluster RUN into a red-black range tree (`range_tree.insert(cluster)` per run; repeated range → stop) — "a cycle-breaker so corrupt FAT loops (a cluster pointing backward) cannot hang the scan" (`re-workspace/artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md:43-56`); chain sanity arithmetic at the 0x10000 scale (`FATSearchDLL_decompiled.c:11839-11857`)
**Core references:** `src/core/fat/cr_chain.cpp:423-463` (`follow_chain` — vector + linear scan), `:465-494` (`reconstruct_deleted_chain` — same shape), `src/core/fat/cr_exfat_quick.cpp:91-133` (`follow_exfat_chain` — same); chain caps `cr_fat_quick.cpp:32,263` and `cr_exfat_quick.cpp:257,446,514,740` (0x10000 = 65,536 clusters); contrast `cr_btrfs_engine.cpp:389-395` (the btrfs walker already uses `std::set` + MAX_NODES); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's FAT and exFAT chain walkers push every cluster into a `std::vector<u64>` and
linearly scan it on EVERY step (`for (u64 v : visited) if (v == c)`); with the 65,536-cluster
chain cap, a maximal chain costs ~n²/2 ≈ 2.1×10⁹ comparisons before any I/O. The og
inserts cluster runs into a red-black range tree (O(log n) per run, with range-merge
semantics). On a FAT32 volume with many large files (e.g. 100 × 256 MB), the core burns
100+ seconds of pure cycle-check CPU where the og spends milliseconds — the quick stage
appears frozen (progress ticks only per dentry batch), directly hitting this session's
"no stuck or freeze" directive. Bounded, never infinite — cancel still works per chain
step — but user-observable as a stall.

## What the reference does

The og's chain walker (fat-exfat-engine-algorithms-v2.md:43-56, first-hand decode by the
lane): each contiguous cluster RUN is inserted into a red-black range tree; a repeated
range → chain stop. The structure gives O(log n) per run with run-coalescing (a 65,536-cluster
contiguous run is ONE tree node, not 65,536 entries). Chain sanity arithmetic runs at the
0x10000 scale (FATSearchDLL:11839-11857).

## What our core does

`cr_chain.cpp:423-463` (`follow_chain`) — every cluster: `visited.push_back(c)` + a linear
`std::find`-shaped scan; `reconstruct_deleted_chain` (:465-494) and the exFAT twin
(cr_exfat_quick.cpp:91-133) share the shape. Caps: 0x10000 clusters per chain
(cr_fat_quick.cpp:32,263; cr_exfat_quick.cpp:257,446,514,740). Worst case per maximal
chain: 65,536²/2 ≈ 2.1×10⁹ comparisons. The btrfs walker already uses the right structure
(`std::set` visited + MAX_NODES, cr_btrfs_engine.cpp:389-395) — this is FAT/exFAT-specific.

## Gap

Cycle-guard data-structure complexity: og O(runs·log runs) with run coalescing vs core
O(clusters²). Falsifiable perf claim: 100 maximal chains on FAT32 cost the core ≥100 s
of cycle-check CPU (measurable with perf counters, no I/O needed) vs the og's
milliseconds-class cost.

## Impact

- User-observable stall ("stuck" appearance) on large-FAT32-volume scans — progress
  freezes for minutes during the quick stage while CPU spins at 100% in the comparison
  loop.
- Scalability: cost scales quadratically with chain length, i.e. with file size — exactly
  the "behavior at large N" parameter. Cancel remains responsive (checked per chain step,
  cr_chain.cpp:433-437), so this is stall-class, not hang-class.
- Benchmark distortion: any throughput bench on chain-heavy fixtures measures the guard,
  not the I/O.

## Reproduction / verification

Core-derived (lane B read the three walkers + caps directly; the n² shape is visible in
the loop structure at cr_chain.cpp:429-447/:475-494). Corpus-derived (the range-tree
decode, fat-exfat-engine-algorithms-v2.md:43-56). Arithmetic: 100 chains × 2.1×10⁹
comparisons ≈ 10¹¹ comparisons ≈ 100+ s at ~1e9 cmp/s. A synthetic repro is cheap:
FAT32 image, 100 files × 65,536-cluster chains, time `follow_chain` (no device needed —
a memory FAT). PARITY-DELTAS: D-14/F7 register the guard EXISTENCE (hardening family),
Q3 registers the cap VALUE — the complexity divergence is in no registry.

## Fix plan (for the engineering team)

1. Replace the visited vector with run-coalescing containment checks: insert each
   CONTIGUOUS run {start,len} into an ordered map (or interval set), checking overlap on
   insert — the og's shape. Simpler equivalent that fixes the asymptote: a hashed set
   (`std::unordered_set<u64>`) is O(n) total for the visited check (still per-cluster,
   but linear) — the run-tree is strictly better (O(runs)) and matches the og; prefer it.
2. Files: `src/core/fat/cr_chain.cpp` (both walkers), `src/core/fat/cr_exfat_quick.cpp`
   (the exFAT twin). Factor one `VisitedRuns` helper (the btrfs precedent shows the
   pattern).
3. Keep the 0x10000 caps and per-step cancel checks exactly as-is (Q3/D-14 semantics).
4. Tests + bench: below.
5. Acceptance criteria: the synthetic 100×maximal-chain fixture's quick stage completes
   in <5 s (was 100+ s of CPU); cycle detection still triggers on the existing
   loop-fixture tests (backward-pointer, A→B→A, self-loop) — no semantic change.

## Tests to add

- `FatChain.CycleGuardPerf` (100 × 65,536-cluster chains under a perf budget — fails
  today by >20×)
- `FatChain.RunCoalescingDetectsOverlap` (semantic pin: a chain re-entering an earlier
  run stops exactly where the vector version stopped)
- `ExfatChain.CycleGuardPerf` (the twin)

## Related findings

- N-162 (the og's own 32-bit cluster math in the same walkers — independent)
- D-14/F7/Q3 (the registered guard/cap rows this rides on)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** `0a46063c` (the S47 MAJOR-first fix lane)
**Verified by:** FatChain.CycleGuardPerf (RED pre-fix: the 100 × 65,536-cluster fixture took 62.18 s on the pre-fix build — measured — vs the 10 s budget; post-fix 1.55 s, a 40× improvement with 6.4× budget headroom) · FatChain.RunCoalescingDetectsOverlap (the semantic pin: the A→B→A two-node loop cuts at 2 clusters, the 300..400-run backward-pointer re-entry cuts at exactly 101 clusters where the vector guard cut, the clean 201-cluster run walks whole — no semantic change) — the RED state proven by the stashed-walker rebuild; the existing cycle tests (FatEngine.ChainCycleIsCut and the family) stay green; the full suite 1296/1296.
**Resolution notes:** The fix plan's preferred option landed: `VisitedRuns` (include/cr/fat/cr_visited_runs.h) — the og's run-coalescing range-tree shape (each contiguous cluster run is ONE interval in an ordered map; tail-run extension is the O(log runs) hot path for the common contiguous walk; adjacent-run coalescing on both insert sides; visit() returns the cycle verdict exactly when the vector guard's linear scan would have hit). All three walkers rewired: follow_chain and reconstruct_deleted_chain (cr_chain.cpp) + follow_exfat_chain (cr_exfat_quick.cpp, where the FREE-continuation heuristic manufactures long contiguous runs — the run-tree's best case). The 0x10000 caps and the per-step cancel checks are untouched (Q3/D-14 register those — the fix plan's explicit keep). **Benchmark record (the fix plan's measure-don't-guess protocol):** baseline on the pre-fix build at the current tree: 62.18 s wall for the synthetic 100 × 65,536-cluster memory-FAT fixture (the finding's own reproduction design; lower than its 100 s arithmetic estimate because the vector scan is cache-friendly at ~1e9 effective cmp/s — the stall class confirmed, the constant honestly measured); post-fix: 1.55 s. No other parameter regressed: the semantic pin proves identical stop points, and the full suite is green. **Scope note (the self-loop):** the fix plan's test list included the self-loop shape, but the og's own whole-table validation (the M-015 sweep, FUN_180041b00) rejects v == i entries OUTRIGHT — a self-loop never reaches any walker through a validated table, so the sub-case is unreachable by construction; the guard's re-entry semantics (which cover it) are pinned by the backward-pointer case instead, and the test documents this. No safety hardening touched.
