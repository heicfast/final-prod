/*
 * cr_cached.h - bounded LRU read cache in front of a transport.
 *
 * Clean-room equivalent of CStorageDeviceWithCache (corpus:
 * ntfs-engine-internals.md): "in front to survive MFT re-reads economically".
 * Bounded by construction; hit rate reported for benchmarking.
 *
 * ------------------------------------------------------------------
 * REGISTERED DIVERGENCE (N-045, PARITY-DELTAS D-72) — do not "fix"
 * without updating the register + the doc-truth pin test:
 *
 *   og budget   : 4 MiB  (DRWUI_decompiled.c:919080-:919092 —
 *                 CStorageDeviceWithCache sizes its cache count as
 *                 0x400000 / bytes_per_sector, fallback 0x2000 blocks;
 *                 ctor :962250-:962292, cache count 1000, constructed
 *                 once per physical disk, shared across partition scans).
 *   core budget : 8 MiB at the scan pipe (cr_scan.cpp — 8ull << 20,
 *                 256 KiB blocks; block size separately registered
 *                 as D-51).
 *
 *   og algorithm: a PREFETCH read-ahead engine — dedicated prefetch
 *                 thread, 10-run look-ahead window, 1000-sector default
 *                 runs (overridden to 4 MiB runs by the scan factory,
 *                 DRWUI:919088-:919142), CRITICAL_SECTION locking
 *                 (ctor :962250-:962597).
 *   core        : reactive true-LRU — fill-outside-the-registry-lock
 *                 since the 2026-11-18 performance session (the cache
 *                 no longer serializes every reader: per-block in-flight
 *                 claims let N workers fill N blocks concurrently, and
 *                 waiters block on the CLAIMED block, not on the world;
 *                 buffers are pooled page-aligned, not allocated per
 *                 miss — PERF-LEVERS F1 opened with the measured
 *                 serialization factor 0.51x/0.23x/0.20x at N=2/4/8 as
 *                 the accepting evidence, worklog/BASELINE.md §4.2).
 *                 No prefetch thread at THIS layer: the read-ahead
 *                 runway is the transport's overlapped ring (the og's
 *                 model generalized one layer down, cr_transport.cpp).
 *
 * The 2x budget + reactive LRU remain deliberate: the core's cache also
 * fronts the retry pipe's zero-fill reads and the carve windows, and the
 * transport-level sweep (bench alignbench, PERFORMANCE-BASELINE
 * section 5) measured the 256 KiB block curve this budget rides on.
 * Aligning the budget to 4 MiB is a benchmark-gated decision
 * (docs/benchmarking.md), not a parity fix. Pinned by
 * Cached.ScanCacheBudgetIsTheRegisteredDivergence.
 * ------------------------------------------------------------------
 *
 * Concurrency contract (2026-11-18 rework):
 *   - read_at is thread-safe and BLOCKING per call (IStorageDevice
 *     contract, cr_device.h:123 — engine worker threads).
 *   - the registry lock guards lookup/LRU/evict/insert ONLY; the inner
 *     transport fill runs OUTSIDE it (cr_cached.cpp).
 *   - one fill per block: the first claimant fills; concurrent readers
 *     of the same block wait on that block's condition variable (the
 *     og's consumer waits on its run's event the same way,
 *     DRWUI:962764).
 *   - failed fills are not cached (audit row 38 preserved): the block
 *     is removed and waiters re-claim, so the next attempt retries the
 *     transport.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CACHED_H_
#define CR_CACHED_H_
#include "cr_device.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace cr {

class CachedReader final : public IStorageDevice {
public:
    /* cache_bytes is rounded down to a multiple of block_bytes (default 64 KiB
     * - the reference MFT batch size, corpus section 2). */
    static std::unique_ptr<CachedReader> create(std::unique_ptr<IStorageDevice> inner,
                                                u64 cache_bytes = 1 << 20, u32 block_bytes = 65536);

    u32 read_at(u64 offset, void* buffer, u32 length) override;
    const Geometry& geometry() const override { return inner_->geometry(); }
    bool is_static_media() const override { return inner_->is_static_media(); }

    u64 hits() const;   /* summed over shards */
    u64 misses() const; /* summed over shards */

    ~CachedReader() override;

private:
    CachedReader() = default;
    struct Block;     /* defined in cr_cached.cpp */
    class BufferPool; /* defined in cr_cached.cpp */
    struct Shard;     /* defined in cr_cached.cpp */

    static void lru_touch(std::vector<std::shared_ptr<Block>>& blocks, size_t found);
    void evict_overflow_locked(Shard& shard);

    std::unique_ptr<IStorageDevice> inner_;
    u32 block_bytes_ = 65536;
    size_t capacity_ = 0;
    /* Sharded registry (segmented LRU): block_index % kShardCount maps
     * to one shard with its own mutex + per-shard LRU vector (index 0 =
     * most-recently-used). The hit path contends only on its shard —
     * the 2026-11-18 measured hit-path aggregate was 0.45x/0.27x/0.25x
     * at N=2/4/8 under the single registry lock (worklog/BASELINE.md
     * §4.2 rerun); sharding removes the cross-thread serialization
     * while per-shard scans stay O(capacity/shards). Approximate global
     * LRU by construction (standard segmented-cache property); the
     * budget registered in D-72 is unchanged. Shared block ownership is
     * the eviction-safety story: a reader mid-copy keeps its block's
     * buffer alive even after the registry drops it. */
    std::vector<std::unique_ptr<Shard>> shards_;
    std::shared_ptr<BufferPool> pool_; /* page-aligned pooled block buffers */
};

} // namespace cr

#endif // CR_CACHED_H_
