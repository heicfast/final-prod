/* test_parallel.cpp - ThreadPool + parallel_for + parallel carve/remnants.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_parallel.h"
#include "cr/carve/cr_carve.h"
#include "cr/crypto/cr_sha256.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/tree/cr_node.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <set>
#include <vector>

namespace cr {
namespace {

TEST(Pool, RunsAllItemsExactlyOnce) {
    ThreadPool pool(4);
    std::atomic<u64> sum{0};
    std::atomic<u32> count{0};
    const u32 rc = parallel_for(pool, 1000, [&](u64 i) -> u32 {
        sum.fetch_add(i, std::memory_order_relaxed);
        count.fetch_add(1, std::memory_order_relaxed);
        return OK;
    });
    EXPECT_EQ(OK, rc);
    EXPECT_EQ(u64(1000) * 999 / 2, sum.load());
    EXPECT_EQ(1000u, count.load());
}

TEST(Pool, AbortStopsWork) {
    ThreadPool pool(4);
    std::atomic<u32> ran{0};
    const u32 rc = parallel_for(pool, 100000, [&](u64) -> u32 {
        ran.fetch_add(1, std::memory_order_relaxed);
        if (ran.load() >= 10)
            return ERR_READ_FAILED; /* triggers pool abort */
        return OK;
    });
    EXPECT_EQ(ERR_READ_FAILED, rc);
    EXPECT_GE(ran.load(), 10u);
    EXPECT_LT(ran.load(), 100000u); /* far short of all items */
}

TEST(Pool, ExternalCancelTokenAdapter) {
    ThreadPool pool(2);
    PoolCancelToken tok(&pool);
    EXPECT_FALSE(tok.cancelled());
    pool.abort();
    EXPECT_TRUE(tok.cancelled());
}

TEST(Pool, ExceptionsNeverCrossThreadEdge) {
    ThreadPool pool(4);
    std::atomic<u32> seen{0};
    const u32 rc = parallel_for(pool, 100, [&](u64 i) -> u32 {
        seen.fetch_add(1, std::memory_order_relaxed);
        if (i == 3)
            throw std::bad_alloc();
        return OK;
    });
    EXPECT_EQ(ERR_INTERNAL, rc);
    EXPECT_GE(seen.load(), 4u);
}

TEST(Pool, WaitIdleAndPending) {
    ThreadPool pool(2);
    EXPECT_FALSE(pool.busy());
    EXPECT_EQ(0u, pool.pending());
    EXPECT_EQ(OK, parallel_for(pool, 64, [](u64) -> u32 { return OK; }));
    EXPECT_FALSE(pool.busy());
    EXPECT_EQ(0u, pool.pending());
}

/* ---------------------------------------------------------------------------
 * Parallel carve: byte-identical results to single-threaded.
 * ------------------------------------------------------------------------- */
class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> data) : data_(std::move(data)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = data_.size();
        g_.sector_count = data_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> data_;
};

/* Build a buffer with 12 planted valid JPEGs + noise, spanning >1 window. */
static std::vector<u8> build_carve_fixture() {
    std::vector<u8> d(3ull << 20, 0xEE); /* 3 MiB = 3 windows */
    /* grammar-complete JPEG (SOI, APP0, COM, SOF0, SOS, entropy, EOI) -
     * the same shape the carve suite proves the validator accepts */
    auto put16 = [&d](u64 at, u16 x) {
        d[at] = u8(x >> 8);
        d[at + 1] = u8(x);
    };
    auto plant_jpeg = [&](u64 at) {
        u64 p = at;
        d[p] = 0xFF;
        d[p + 1] = 0xD8;
        d[p + 2] = 0xFF;
        d[p + 3] = 0xE0; /* SOI + APP0 */
        put16(p + 4, 16);
        memcpy(d.data() + p + 6, "JFIF", 4);
        p += 20;
        d[p] = 0xFF;
        d[p + 1] = 0xFE;
        put16(p + 2, 4600); /* COM */
        memset(d.data() + p + 4, 'A', 4596);
        p += 2 + 4600;
        d[p] = 0xFF;
        d[p + 1] = 0xC0; /* SOF0 */
        put16(p + 2, 17);
        d[p + 4] = 8;
        put16(p + 5, 64);
        put16(p + 7, 48);
        d[p + 9] = 1;
        d[p + 10] = 1;
        d[p + 11] = 0x22;
        d[p + 12] = 0;
        p += 19;
        d[p] = 0xFF;
        d[p + 1] = 0xDA; /* SOS */
        put16(p + 2, 12);
        memset(d.data() + p + 4, 0, 10);
        p += 14;
        memset(d.data() + p, 0x11, 512); /* entropy */
        p += 512;
        d[p] = 0xFF;
        d[p + 1] = 0xD9; /* EOI */
    };
    /* plants span ~5.2 KiB each: straddlers cross a window boundary with
     * no other plant inside their span (overlapping plants would corrupt
     * each other's marker walks) */
    plant_jpeg(0);
    plant_jpeg((1 << 20) - 2000); /* straddles window 0/1 boundary */
    plant_jpeg((3 << 20) / 2);    /* mid window 1 */
    plant_jpeg((2 << 20) - 2000); /* straddles window 1/2 boundary */
    plant_jpeg((5 << 20) / 2);    /* mid window 2, clear of the straddler */
    return d;
}

TEST(ParallelCarve, MatchesSingleThreaded) {
    auto run = [](u32 threads) -> std::pair<u32, std::vector<std::pair<u64, u64>>> {
        tree::ResultTree tree;
        auto* root = tree.add_root(u"carved");
        carve::CarveFlags cf;
        cf.threads = threads;
        cf.hash_small_files = false;
        carve::CarveStats st;
        auto dev = std::make_unique<MemDevice>(build_carve_fixture());
        const u32 rc = carve::carve_slice(dev.get(), cf, nullptr, nullptr, nullptr, tree, root,
                                          nullptr, nullptr, st);
        std::vector<std::pair<u64, u64>> extents;
        for (auto* r : tree.roots())
            for (tree::FileNode* c = r->first_child(); c; c = c->next_sibling())
                if (c->props() && !c->props()->extents.empty())
                    extents.push_back(
                        {c->props()->extents[0].offset, c->props()->extents[0].length});
        std::sort(extents.begin(), extents.end());
        return {rc, extents};
    };
    const auto single = run(1);
    const auto multi = run(4);
    ASSERT_EQ(OK, single.first);
    ASSERT_EQ(OK, multi.first);
    EXPECT_EQ(single.second.size(), multi.second.size());
    EXPECT_EQ(single.second, multi.second);
    EXPECT_GE(single.second.size(), 5u); /* all plants found */
}

/* ---------------------------------------------------------------------------
 * Parallel remnant sweep: same records found as sequential.
 * ------------------------------------------------------------------------- */

static std::vector<u8> build_remnant_fixture() {
    /* 2 MiB of noise + 3 valid FILE records at arbitrary 512-aligned spots */
    std::vector<u8> d(2ull << 20, 0x55);
    auto plant = [&](u64 at) {
        u8* rec = d.data() + at;
        /* header: magic, usa_off 0x30, usa_cnt 3 (2 sectors), attrs at 0x38 */
        memcpy(rec, "FILE", 4);
        u16 usa_off = 0x30, usa_cnt = 3, attr_off = 0x38;
        u32 alloc = 1024;
        memcpy(rec + 4, &usa_off, 2);     /* update_sequence_offset u16 @4 */
        memcpy(rec + 6, &usa_cnt, 2);     /* update_sequence_size u16 @6 */
        memcpy(rec + 0x14, &attr_off, 2); /* attribute_offset u16 @0x14 */
        memcpy(rec + 0x1C, &alloc, 4);    /* bytes_allocated u32 @0x1C */
        u16 usn = 0x0BB8;
        u16 repl[2] = {0xAAAA, 0xBBBB};
        memcpy(rec + 0x30, &usn, 2);
        memcpy(rec + 0x32, &repl[0], 2);
        memcpy(rec + 0x34, &repl[1], 2);
        memcpy(rec + 510, &usn, 2);  /* sector 0 tail */
        memcpy(rec + 1022, &usn, 2); /* sector 1 tail */
        /* $FILE_NAME resident attr at 0x38: value_off 0x18, name "fl<N>" */
        u32 atype = 0x30;
        u32 vlen_f = 66 + 10;
        u32 alen = (0x18 + vlen_f + 7) / 8 * 8; /* value_off + value_len, 8-aligned */
        memcpy(rec + 0x38, &atype, 4);
        memcpy(rec + 0x38 + 4, &alen, 4);
        rec[0x38 + 8] = 0; /* resident */
        rec[0x38 + 9] = 0; /* name_length: none */
        u16 voff = 0x18;
        memcpy(rec + 0x38 + 0x14, &voff, 2);   /* value_offset u16 @0x14 */
        memcpy(rec + 0x38 + 0x10, &vlen_f, 4); /* value_length u32 @0x10 */
        u8* v = rec + 0x38 + 0x18;
        u64 parent = 5;
        memcpy(v + 0, &parent, 8);
        u64 t = 132000000000000000ULL;
        memcpy(v + 8, &t, 8);
        memcpy(v + 16, &t, 8);
        memcpy(v + 24, &t, 8);
        memcpy(v + 32, &t, 8);
        u64 rs = 100, as_ = 128;
        memcpy(v + 40, &as_, 8);
        memcpy(v + 48, &rs, 8);
        v[64] = 5; /* name length */
        v[65] = 0; /* POSIX namespace */
        memcpy(v + 66, u"fl", 4);
        v[68] = u8('0' + (at >> 12) % 10);
        v[69] = u8('x');
        /* end marker after the attribute */
        u32 end = 0xFFFFFFFF;
        memcpy(rec + 0x38 + alen, &end, 4);
    };
    plant(0x100000);
    plant(0x180400);
    plant(0x30000);
    return d;
}

TEST(ParallelRemnants, SameRecordsAsSequential) {
    auto run = [](u32 threads) -> u64 {
        tree::ResultTree tree;
        auto* root = tree.add_root(u"lost");
        ntfs::WalkStats ws;
        auto dev = std::make_unique<MemDevice>(build_remnant_fixture());
        const u32 rc = ntfs::scan_mft_remnants(dev.get(), 512, 512, 1024, 512, nullptr, nullptr,
                                               nullptr, tree, root, ws, threads);
        EXPECT_EQ(OK, rc);
        return ws.emitted;
    };
    const u64 seq = run(1);
    const u64 par = run(4);
    EXPECT_EQ(seq, par);
    EXPECT_GE(seq, 3u);
}

} // namespace
} // namespace cr

namespace cr {

/* ---- N-017 (diff-to-fix): the factored cancel watcher ------------------- */

TEST(Pool, CancelPoolWatcherDrainsQueueWithinOneTick) {
    /* N-017: a QUEUED item must never start after a cancel. 64 long items
     * (150 ms each) over 4 workers; cancel at 50 ms: with the watcher the
     * abort lands within one 50 ms tick, so at most ~workers + 1 items
     * ever START (each worker's in-flight item finishes to its own end);
     * without the abort the workers keep pulling items for the full
     * 64 x 150 / 4 = 2.4 s. */
    const cr::u64 kItems = 64;
    std::atomic<cr::u64> started{0};
    std::atomic<bool> fire{false};
    std::thread canceller([&fire]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        fire.store(true, std::memory_order_release);
    });
    struct FlagToken : public CancelToken {
        std::atomic<bool>* f_ = nullptr;
        bool cancelled() const override { return f_->load(std::memory_order_acquire); }
    } token;
    token.f_ = &fire;
    {
        ThreadPool pool(4);
        CancelPoolWatcher watcher(pool, &token);
        const auto t0 = std::chrono::steady_clock::now();
        u32 rc = parallel_for(pool, kItems, [&](u64) -> u32 {
            started.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            return OK;
        });
        watcher.done();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - t0)
                                 .count();
        EXPECT_EQ(u32(ERR_CANCELLED), rc) << "the aborted pool must report cancellation";
        EXPECT_LE(started.load(), 6u) << "queued items must not start after the cancel tick";
        EXPECT_LT(elapsed, 1500) << "the scan must not run the full 2.4 s queue";
    }
    canceller.join();
}

TEST(Pool, CancelPoolWatcherNullTokenIsInert) {
    /* no token: the watcher is a no-op (never aborts, joins cleanly) */
    ThreadPool pool(2);
    CancelPoolWatcher watcher(pool, nullptr);
    std::atomic<u64> ran{0};
    EXPECT_EQ(u32(OK), parallel_for(pool, 16, [&](u64) -> u32 {
                  ran.fetch_add(1, std::memory_order_relaxed);
                  return OK;
              }));
    watcher.done();
    EXPECT_EQ(ran.load(), 16u);
}

} // namespace cr
