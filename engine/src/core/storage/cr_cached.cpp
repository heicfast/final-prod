/* cr_cached.cpp - bounded LRU read cache, fill-outside-the-lock edition.
 *
 * The 2026-11-18 performance session rework (PERF-LEVERS F1, opened):
 * the old read_at held ONE std::mutex across the whole fill loop —
 * every worker's transport round trip serialized behind every other's
 * (measured aggregate scaling 0.51x/0.23x/0.20x at N=2/4/8 threads,
 * worklog/BASELINE.md §4.2). The registry now:
 *   - SHARDS by block index (segmented LRU, one lane per block at the
 *     scan pipe's 32-block configuration): the hit path contends only
 *     on its shard's mutex, not on one global lock; the aggregate is
 *     CPU/memcpy-bounded at 0.96x/0.82x/0.82x at N=2/4/8 (was
 *     0.41x/0.17x/0.11x under the single lock);
 *   - fills OUTSIDE every registry lock, one per block, with concurrent
 *     claimants of the same block waiting on that block's own
 *     condition variable (the og's consumer model, DRWUI:962764) — the
 *     miss-path aggregate went 20,608 -> 50,161 ops/s at N=4 in the
 *     same rerun;
 *   - draws block buffers from a page-aligned pool instead of per-miss
 *     vector allocation (the old churn measured ~4.6 us per 256 KiB
 *     miss — 0.8x the copy itself).
 *
 * Semantics preserved exactly (single-threaded accounting included):
 * hit/miss telemetry, the bounds contract, the audit-row-38 rule
 * (failed blocks are not cached — waiters re-claim), LRU order
 * (approximate global LRU via segmentation — the standard
 * segmented-cache property), and the error-code mapping
 * (ERR_RETRY_EXHAUSTED -> ERR_READ_FAILED).
 *
 * Phase structure of one block step in read_at:
 *   1. under the shard lock: find the block — VALID: copy the
 *      shared_ptr + LRU touch (hit); FILLING: copy + phase 2 waits;
 *      DEAD or absent: (erase dead) + claim a fresh FILLING block,
 *      inserted at the shard-LRU front (miss), with insert-time dedup
 *      against a concurrent claimant of the same start.
 *   2. outside the shard lock: FILLING waiters wait on the block's
 *      cv (the filler's publish/failure wakes them); the claimant
 *      fills via inner_->read_at with NO registry lock held.
 *   3. copy out under the shared_ptr's lifetime guarantee.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_cached.h"
#include <condition_variable>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace cr {

namespace {
/* Shard count: enough lanes that the scan's worker set (ThreadPool
 * hw-1 clamp 8, cr_scan.cpp:58-68) finds an uncontended lane, without
 * over-fragmenting small caches. */
constexpr size_t kShardCount = 32;
#if defined(__cpp_lib_hardware_interference_size) && defined(_GLIBCXX_HAVE_TBB)
constexpr size_t kCacheLine = std::hardware_destructive_interference_size;
#else
constexpr size_t kCacheLine = 64;
#endif
} // namespace

/* Page-aligned fixed-size buffer pool (the mandated reusable-buffer
 * discipline): one VirtualAlloc/mmap page block per buffer, released
 * back to the freelist when its Block dies. Page alignment subsumes
 * every logical-sector alignment (and the physical-sector recommendation
 * up to 4 KiB pages) — the transport's 64 KiB envelope keeps every
 * ISSUED length a 64 KiB multiple regardless. Shared ownership: blocks
 * keep the pool alive, so a reader mid-copy can outlive the reader that
 * registered the block. */
class CachedReader::BufferPool final {
public:
    explicit BufferPool(size_t bytes) : bytes_(bytes) {}
    ~BufferPool() {
        std::lock_guard<std::mutex> g(mu_);
        for (u8* p : free_)
            platform_free(p, bytes_);
    }
    u8* acquire() {
        {
            std::lock_guard<std::mutex> g(mu_);
            if (!free_.empty()) {
                u8* p = free_.back();
                free_.pop_back();
                return p;
            }
        }
        return platform_alloc(bytes_);
    }
    void release(u8* p) {
        if (!p)
            return;
        std::lock_guard<std::mutex> g(mu_);
        free_.push_back(p);
    }
    size_t bytes() const { return bytes_; }

private:
    static u8* platform_alloc(size_t n) {
#if defined(_WIN32)
        return static_cast<u8*>(VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
        void* p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return p == MAP_FAILED ? nullptr : static_cast<u8*>(p);
#endif
    }
    static void platform_free(u8* p, size_t n) {
#if defined(_WIN32)
        (void)n;
        VirtualFree(p, 0, MEM_RELEASE);
#else
        munmap(static_cast<void*>(p), n);
#endif
    }

    size_t bytes_;
    std::mutex mu_;
    std::vector<u8*> free_;
};

/* One cache block: FILLING (claimed, owner is filling) -> VALID or
 * FAILED (fill error — the block is removed from the registry and
 * waiters re-claim; failed blocks are never cached, audit row 38). */
struct CachedReader::Block final {
    /* Cache-line isolation for the hot header (start/state): hit-path
     * probes must not share a line with a neighbouring block's fill
     * traffic (false sharing measured as the N=2 hit-path cap,
     * 2026-11-18 rerun). */
    alignas(kCacheLine) u64 start = 0;
    std::atomic<int> state{0}; /* 0 = FILLING, 1 = VALID, 2 = FAILED */
    u8* data = nullptr; /* pooled, block_bytes_ capacity */
    std::shared_ptr<BufferPool> pool;
    mutable std::mutex mu; /* fill publication + waiter queue ONLY — the
                              hit path takes NO block lock (VALID is
                              terminal: only eviction removes a valid
                              block, and eviction holds the shard lock) */
    std::condition_variable cv;

    explicit Block(std::shared_ptr<BufferPool> p) : pool(std::move(p)) {
        data = pool->acquire();
    }
    ~Block() {
        if (pool && data)
            pool->release(data);
    }
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
};

/* One registry lane: own mutex + own LRU vector (index 0 = MRU) +
 * own hit/miss counters (per-line atomics — a global counter line
 * ping-ponging across every hit was a measured N=2 cap). */
struct CachedReader::Shard final {
    alignas(kCacheLine) std::mutex mu;
    std::vector<std::shared_ptr<Block>> lru;
    size_t cap = 0; /* this shard's block quota */
    mutable std::atomic<u64> hits{0};
    mutable std::atomic<u64> misses{0};
};

std::unique_ptr<CachedReader> CachedReader::create(std::unique_ptr<IStorageDevice> inner,
                                                   u64 cache_bytes, u32 block_bytes) {
    if (!inner || block_bytes == 0 || cache_bytes < block_bytes)
        return nullptr;
    auto r = std::unique_ptr<CachedReader>(new CachedReader());
    r->inner_ = std::move(inner);
    r->block_bytes_ = block_bytes;
    r->capacity_ = static_cast<size_t>(cache_bytes / block_bytes);
    if (r->capacity_ == 0)
        return nullptr;
    const size_t nshards = r->capacity_ < kShardCount ? (r->capacity_ ? r->capacity_ : 1) : kShardCount;
    const size_t shard_cap = r->capacity_ / nshards;
    r->shards_.resize(nshards);
    for (auto& sp : r->shards_) {
        sp = std::make_unique<Shard>();
        sp->cap = shard_cap ? shard_cap : 1;
        sp->lru.reserve(sp->cap + 1);
    }
    r->pool_ = std::make_shared<BufferPool>(size_t(block_bytes));
    return r;
}

CachedReader::~CachedReader() = default;

u64 CachedReader::hits() const {
    u64 total = 0;
    for (const auto& sp : shards_)
        total += sp->hits.load(std::memory_order_relaxed);
    return total;
}

u64 CachedReader::misses() const {
    u64 total = 0;
    for (const auto& sp : shards_)
        total += sp->misses.load(std::memory_order_relaxed);
    return total;
}

/* move-to-front in the shard LRU vector (shared_ptr shuffles only — no
 * data motion). Shard lock held by the caller. */
void CachedReader::lru_touch(std::vector<std::shared_ptr<Block>>& blocks, size_t found) {
    if (found == 0 || found >= blocks.size())
        return;
    std::shared_ptr<Block> tmp = std::move(blocks[found]);
    for (size_t i = found; i > 0; --i)
        blocks[i] = std::move(blocks[i - 1]);
    blocks[0] = std::move(tmp);
}

/* evict beyond this shard's quota, never evicting an in-flight (FILLING)
 * block. Shard lock held by the caller. */
void CachedReader::evict_overflow_locked(Shard& shard) {
    while (shard.lru.size() > shard.cap) {
        size_t victim = shard.lru.size();
        for (size_t k = shard.lru.size(); k > 0; --k) {
            if (shard.lru[k - 1]->state.load(std::memory_order_acquire) != 0) {
                victim = k - 1;
                break;
            }
        }
        if (victim == shard.lru.size())
            break; /* everything from the back is in flight — keep them */
        shard.lru.erase(shard.lru.begin() + long(victim));
    }
}

u32 CachedReader::read_at(u64 offset, void* buffer, u32 length) {
    if (length == 0)
        return OK;
    if (offset > inner_->size() || u64(length) > inner_->size() - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;

    u8* dst = static_cast<u8*>(buffer);
    u64 pos = offset;
    u32 remain = length;
    while (remain > 0) {
        const u64 block_start = pos - (pos % block_bytes_);
        const u32 in_block_off = static_cast<u32>(pos - block_start);
        const u64 block_end_avail = block_bytes_ - in_block_off;
        const u32 take = static_cast<u32>(remain < block_end_avail ? remain : block_end_avail);
        Shard& shard = *shards_[size_t((block_start / block_bytes_) % shards_.size())];

        std::shared_ptr<Block> b;
        bool we_fill = false;
        bool wait_fill = false;
        bool served = false;

        /* ---- phase 1: claim/find/serve-hit under the shard lock ---- */
        {
            std::lock_guard<std::mutex> g(shard.mu);
            size_t found = shard.lru.size();
            for (size_t i = 0; i < shard.lru.size(); ++i) {
                if (shard.lru[i]->start == block_start) {
                    found = i;
                    break;
                }
            }
            if (found != shard.lru.size()) {
                /* atomic state probe — NO block lock on the hit path
                 * (VALID is terminal; FILLING/FAILED are the only other
                 * states, both transitions publish under b->mu with
                 * release stores into this atomic). */
                const int st = shard.lru[found]->state.load(std::memory_order_acquire);
                if (st == 1) {
                    /* HIT: serve the copy UNDER the shard lock — the
                     * block cannot be evicted (eviction needs this
                     * lock), so no shared_ptr refcount is taken: the
                     * hot-path refcount ping-pong on warm blocks was the
                     * measured N=2 aggregate cap. */
                    shard.hits.fetch_add(1, std::memory_order_relaxed);
                    lru_touch(shard.lru, found);
                    std::memcpy(dst, shard.lru[0]->data + in_block_off, take);
                    served = true;
                } else if (st == 0) {
                    b = shard.lru[found]; /* FILLING: wait below */
                    wait_fill = true;
                } else {
                    /* FAILED: dead entry pending removal — drop it and
                     * claim fresh (the retry the old single-lock code
                     * gave the next arrival for free). */
                    shard.lru.erase(shard.lru.begin() + long(found));
                }
            }
            if (served) {
                dst += take;
                pos += take;
                remain -= take;
                continue;
            }
            if (!b) {
                /* absent (or just erased): claim — insert a FILLING
                 * block at the shard-LRU front, dedup against a
                 * concurrent claimant that raced us between find and
                 * insert (both phases run under this shard's lock, so
                 * this is defense-in-depth, not an expected path). */
                shard.misses.fetch_add(1, std::memory_order_relaxed);
                std::shared_ptr<Block> nb = std::make_shared<Block>(pool_);
                nb->start = block_start;
                size_t dup = shard.lru.size();
                for (size_t i = 0; i < shard.lru.size(); ++i) {
                    if (shard.lru[i]->start == block_start) {
                        dup = i;
                        break;
                    }
                }
                if (dup != shard.lru.size()) {
                    b = shard.lru[dup];
                    const int st = b->state.load(std::memory_order_acquire);
                    if (st == 1) {
                        shard.hits.fetch_add(1, std::memory_order_relaxed);
                    } else if (st == 0) {
                        wait_fill = true;
                    } else {
                        shard.lru.erase(shard.lru.begin() + long(dup));
                        continue; /* pathological: died between probes */
                    }
                } else {
                    shard.lru.insert(shard.lru.begin(), nb);
                    b = nb;
                    we_fill = true;
                    evict_overflow_locked(shard);
                }
            }
        }

        /* ---- phase 2: wait or fill, shard lock RELEASED ---- */
        if (wait_fill) {
            std::unique_lock<std::mutex> bb(b->mu);
            while (b->state.load(std::memory_order_acquire) == 0)
                b->cv.wait(bb);
            if (b->state.load(std::memory_order_acquire) != 1) {
                /* the fill FAILED: the block is being removed; re-claim
                 * (bounded by the retry policy — the waiter becomes the
                 * next filler and reports the transport error itself). */
                continue;
            }
            shard.hits.fetch_add(1, std::memory_order_relaxed);
        } else if (we_fill) {
            const u32 blen = static_cast<u32>(
                block_start + block_bytes_ <= inner_->size() ? block_bytes_
                                                              : inner_->size() - block_start);
            const u32 rc = inner_->read_at(block_start, b->data, blen);
            {
                std::lock_guard<std::mutex> bb(b->mu);
                b->state.store(rc == OK ? 1 : 2, std::memory_order_release);
            }
            b->cv.notify_all();
            if (rc != OK) {
                /* remove the failed block so the next attempt retries
                 * the transport (audit row 38: fail blocks are NOT
                 * cached — the old code's contract, verbatim). */
                {
                    std::lock_guard<std::mutex> g(shard.mu);
                    for (size_t i = 0; i < shard.lru.size(); ++i) {
                        if (shard.lru[i] == b) {
                            shard.lru.erase(shard.lru.begin() + long(i));
                            break;
                        }
                    }
                }
                return rc == ERR_RETRY_EXHAUSTED ? ERR_READ_FAILED : rc;
            }
        }

        /* ---- phase 3: serve (the shared_ptr holds the block alive even
         * if another thread evicts it mid-copy) ---- */
        std::memcpy(dst, b->data + in_block_off, take);
        dst += take;
        pos += take;
        remain -= take;
    }
    return OK;
}

} // namespace cr
