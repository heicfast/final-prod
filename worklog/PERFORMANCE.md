# PERFORMANCE — the 2026-11-18 optimization ledger (every fix: before/after, same harness)

> The session's DIAGNOSIS (worklog/DIAGNOSIS.md) ranked four layers; this
> ledger records every optimization this session lands, each with its
> before/after measurement on the SAME fixture/harness, environment
> recorded. The standing baselines live in worklog/BASELINE.md; the
> prior sessions' perf history is in engine/docs/PERFORMANCE-BASELINE.md
> and engine/worklog/PERFORMANCE.md + PERF-LEVERS.md (the lever ledger
> this session opens).

## Environment

- Component microbench: this Linux container (2 vCPU, 4 GiB, overlayfs,
  gcc 14.2 -O2, page-cache-warm 256 MiB fixture at /tmp/bench_dev.img) —
  harness: scripts outside the repo (bench_cachedreader.cpp, built
  against the REAL repo cr_cached.cpp both sides). RELATIVE component
  numbers (lock/serialization/alloc costs), NOT device-throughput truth.
- End-to-end + device-class truth: GitHub-hosted windows-2022 runners
  (the repo's engine-ci / bench lanes) — every wall number cites its run.

## Fix 1 — CachedReader: fill-outside-the-lock + sharded registry + pooled
page-aligned buffers (DIAGNOSIS Layer 2 + Layer 4; PERF-LEVERS F1)

**What changed** (engine/src/core/storage/cr_cached.cpp,
engine/include/cr/storage/cr_cached.h):

1. The old `read_at` held ONE std::mutex across the whole loop INCLUDING
   the transport fill — every worker's device round trip serialized
   behind every other's. Now: the registry is SHARDED (32 lanes; one per
   block at the scan pipe's 32-block configuration), and the fill runs
   OUTSIDE every registry lock with per-block in-flight claims —
   concurrent claimants of the same block wait on that block's own
   condition variable (the og's consumer model, DRWUI:962764).
2. Hit path: lock-free state probe (VALID is terminal) + copy under the
   shard lock (no refcount churn); hot counters per shard (no global
   cache-line ping-pong); block headers cache-line aligned.
3. Buffers: page-aligned POOL (VirtualAlloc/mmap) instead of per-miss
   std::vector resize (~4.6 µs per 256 KiB miss, 0.8x the copy itself);
   shared block ownership makes eviction safe under a reader mid-copy.

**Before/after — the same component microbench, same 256 MiB
page-cache-warm fixture, SAME DAY (2026-11-18), the harness's
per-thread-destination fix applied to BOTH sides** (the harness's
original shared destination buffer was false-sharing one cache line
across every worker — a measurement artifact capping all runs; found
and fixed before any number was recorded as BEFORE). The BEFORE side is
the pre-rework cr_cached.{h,cpp} extracted verbatim from commit
883107bf and recompiled against the same harness (raw outputs:
bench_before/bench_old_fixed/bench_final in this session's transcript;
scripts preserved under /home/z/my-project/scripts/):

| Measurement (harness section) | BEFORE | AFTER | Delta |
|---|---:|---:|---|
| [b] hit-path aggregate scaling N=2 | 0.54x | 0.96x | serialization removed |
| [b] hit-path aggregate scaling N=4 | 0.17x | 0.82x | 4.8x |
| [b] hit-path aggregate scaling N=8 | 0.12x | 0.82x | 6.8x |
| [b] hit-path N=1 absolute | 4.16 M ops/s | 5.00 M ops/s | +20% (lock-free probe, no refcount) |
| [b2] miss-path N=1 (fill+copy) | 23,266 ops/s | 33,876 ops/s | +46% (copy/publish outside the global lock) |
| [b2] miss-path N=4 aggregate | 17,763 ops/s (BELOW N=1: the mutex actively throttled) | 49,392 ops/s | +2.8x, and now SCALES above N=1 |
| [a] sequential 256 KiB stream overhead vs raw pread | 8.0% | 5.5% | -2.5 points (pooled buffers) |
| [c] LRU lookup per read @ 32 blocks | 107.1 ns | 54.2 ns | -49% (1 entry/shard) |
| [d] per-miss buffer alloc | ~4.6 µs (0.8x the copy itself) | pooled (O(1) freelist) | churn removed |

The N=4/8 aggregate is now bounded by the container's 2-vCPU memcpy
ceiling (verified with a lock-free memcpy control at the same pattern:
1.6x at N=2, flat beyond — the cache adds no serialization on top).

**Correctness gates**: full unit suite 1637/1637 green (was 1634 + 3 new
concurrency pins: ConcurrentReadersByteIdentical,
DistinctBlocksFillConcurrently — two fills provably INSIDE the transport
at once (the first fill parks until the second arrives),
OneFillServesAllConcurrentClaimants — one transport read serves 3
concurrent claimants). Single-threaded hit/miss accounting unchanged
(the existing HitMissAndBridging pins pass verbatim). The audit-row-38
contract (failed blocks never cached) preserved and pinned.

**Scope note**: this fix removes the CACHE-layer serialization on any
hardware. On current CI-runner transport handles (not yet
FILE_FLAG_OVERLAPPED) the OS serializes concurrent fills at the file
object — the fix's full device-level concurrency payoff pairs with the
transport overlapped ring (Fix 2, in flight); the component numbers
above are cache-layer truth independent of that.

