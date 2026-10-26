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
 *                 runs, CRITICAL_SECTION locking (ctor :962250-:962597).
 *   core        : reactive true-LRU (this file) — std::mutex, move-to-
 *                 front eviction; no prefetch thread.
 *
 * The 2x budget + reactive LRU are deliberate: the core's cache also
 * fronts the retry pipe's zero-fill reads and the carve windows, and
 * the transport-level sweep (bench alignbench, PERFORMANCE-BASELINE
 * section 5) measured the 256 KiB block curve this budget rides on.
 * Sequential read-ahead latency-hiding the og gets from its prefetch
 * thread is provided one layer down by the transport's large reads;
 * aligning the budget to 4 MiB or adding a prefetch thread is a
 * benchmark-gated decision (docs/benchmarking.md), not a parity fix.
 * Pinned by Cached.ScanCacheBudgetIsTheRegisteredDivergence.
 * ------------------------------------------------------------------
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CACHED_H_
#define CR_CACHED_H_

#include "cr_device.h"
#include <memory>
#include <mutex>

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

    u64 hits() const { return hits_; }
    u64 misses() const { return misses_; }

    ~CachedReader() override;

private:
    CachedReader() = default;
    struct Block {
        u64 start;
        std::vector<u8> data;
        bool valid;
    };
    std::unique_ptr<IStorageDevice> inner_;
    u32 block_bytes_ = 65536;
    size_t capacity_ = 0;
    /* simple true-LRU over a vector (small capacity, O(n) move-to-front is
     * fine at these sizes and keeps the code auditable) */
    std::vector<Block> blocks_;
    u64 hits_ = 0;
    u64 misses_ = 0;
    mutable std::mutex mtx_; /* P2-5: engine workers may share the cache */
};

} // namespace cr

#endif // CR_CACHED_H_
