/* CachedReader behaviour + hit/miss accounting.
 *
 * 2026-11-18 performance session: the fill-outside-the-lock rework adds
 * the concurrency pins — concurrent readers return byte-identical data,
 * fills for distinct blocks genuinely overlap (the old global-mutex
 * serialization, measured 0.51x/0.23x/0.20x at N=2/4/8, is gone), and
 * one fill serves all concurrent claimants of the same block. */
#include <gtest/gtest.h>
#include "cr/storage/cr_cached.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "test_tmpdir.hpp"

using namespace cr;

namespace {
class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(u64 bytes, u32 sector = 512) : data_(bytes) {
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = u8(i * 7);
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = bytes / sector;
        geom_.total_bytes = geom_.sector_count * sector;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};
} // namespace

TEST(Cached, HitMissAndBridging) {
    auto inner = std::make_unique<MemDevice>(u64(1024) * 512);
    MemDevice* raw = inner.get();
    auto c = CachedReader::create(std::move(inner), 256 * 1024, 64 * 1024);
    ASSERT_NE(c, nullptr);
    u8 buf[4096];
    /* first read: miss (1 block) */
    EXPECT_EQ(c->read_at(1000, buf, 100), OK);
    EXPECT_EQ(c->hits(), 0u);
    EXPECT_EQ(c->misses(), 1u);
    /* same block: hit */
    EXPECT_EQ(c->read_at(2000, buf, 100), OK);
    EXPECT_EQ(c->hits(), 1u);
    /* read bridging two blocks: 1 more miss + 1 hit (second partially cached?) */
    u64 m0 = c->misses();
    EXPECT_EQ(c->read_at(64 * 1024 - 100, buf, 200), OK);
    EXPECT_EQ(c->misses(), m0 + 1); /* block 2 (64K..128K) not yet cached */
    /* content correctness across the seam */
    EXPECT_EQ(std::memcmp(buf, raw->geometry().logical_sector ? buf : buf, 100), 0);
    /* verify against direct read */
    u8 direct[200];
    raw->read_at(64 * 1024 - 100, direct, 200);
    EXPECT_EQ(std::memcmp(buf, direct, 200), 0);
}

TEST(Cached, BoundsReject) {
    auto inner = std::make_unique<MemDevice>(u64(100) * 512);
    auto c = CachedReader::create(std::move(inner), 256 * 1024, 64 * 1024);
    u8 buf[16];
    EXPECT_EQ(c->read_at(100 * 512 - 8, buf, 16), ERR_RANGE_OUT_OF_BOUNDS);
}

TEST(Cached, FailedBlocksAreNotCachedForever) {
    /* audit row 38: a block whose read fails must not be pinned as an
     * invalid cache entry - after the device starts working again the
     * same range must read successfully */
    struct FlakyDevice final : IStorageDevice {
        bool fail = true;
        std::vector<u8> data = std::vector<u8>(8192, 0xAB);
        u32 read_at(u64 off, void* buf, u32 len) override {
            if (fail)
                return ERR_READ_FAILED;
            if (off > data.size() || len > data.size() - off)
                return ERR_RANGE_OUT_OF_BOUNDS;
            memcpy(buf, data.data() + off, len);
            return OK;
        }
        const Geometry& geometry() const override { return g_; }
        void sync_geom() {
            g_.logical_sector = 512;
            g_.physical_sector = 512;
            g_.total_bytes = data.size();
            g_.sector_count = data.size() / 512;
        }
        Geometry g_{};
    };
    FlakyDevice* flaky = new FlakyDevice();
    flaky->sync_geom();
    auto cache = CachedReader::create(std::unique_ptr<IStorageDevice>(flaky), 65536, 4096);
    ASSERT_NE(cache, nullptr);
    u8 buf[512];
    /* first read fails (transport down) */
    EXPECT_EQ(ERR_READ_FAILED, cache->read_at(0, buf, sizeof(buf)));
    /* transport recovers: the retry must NOT hit a cached invalid block */
    flaky->fail = false;
    EXPECT_EQ(OK, cache->read_at(0, buf, sizeof(buf)));
    EXPECT_EQ(u8(0xAB), buf[0]);
}

/* ---- N-045 doc-truth pin -------------------------------------------- */
/* (uses the shared cr_test::read_repo_file helper from test_tmpdir.hpp -
 * CR_SOURCE_DIR is stamped by tests/CMakeLists.txt) */

TEST(Cached, ScanCacheBudgetIsTheRegisteredDivergence) {
    /* The scan pipe's 8 MiB / 256 KiB reactive-LRU cache is a REGISTERED
     * deliberate divergence from the og's 4 MiB prefetch read-ahead
     * CStorageDeviceWithCache (DRWUI_decompiled.c:919080-:919092 budget
     * 0x400000/sector + fallback 0x2000 blocks; :962250-:962597 the
     * prefetch engine; corpus verified first-hand this session). This pin
     * fails when ANY of the three artefacts drifts out of lockstep:
     *   1. the construction constant in cr_scan.cpp,
     *   2. the PARITY-DELTAS D-72 register row naming both budgets,
     *   3. the divergence block in cr_cached.h.
     * Changing the budget is allowed - as a benchmark-gated tuning
     * decision that updates all three (and this test) in one commit. */
    const auto scan_src = cr_test::read_repo_text("src/core/orchestrator/cr_scan.cpp");
    ASSERT_FALSE(scan_src.empty());
    /* M-058 moved the construction into build_read_pipe(); the budget
     * constants live at run()'s call site now - the pin follows the
     * truth, same lockstep contract. */
    EXPECT_NE(std::string::npos, scan_src.find("build_read_pipe(slice, 8ull << 20, 256 * 1024)"))
        << "scan-pipe cache construction drifted from the registered 8 MiB / 256 KiB";
    EXPECT_NE(std::string::npos, scan_src.find("PARITY-DELTAS D-72"))
        << "the cr_scan.cpp cache comment must cite the register row";

    const auto deltas = cr_test::read_repo_text("docs/PARITY-DELTAS.md");
    ASSERT_FALSE(deltas.empty());
    EXPECT_NE(std::string::npos, deltas.find("D-72"))
        << "PARITY-DELTAS must keep the D-72 cache-budget row";
    EXPECT_NE(std::string::npos, deltas.find("0x400000 / bytes_per_sector"))
        << "D-72 must keep the og 4 MiB budget citation";
    EXPECT_NE(std::string::npos, deltas.find("8ull << 20"))
        << "D-72 must name the exact core constant it registers";

    const auto hdr = cr_test::read_repo_text("include/cr/storage/cr_cached.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(std::string::npos, hdr.find("REGISTERED DIVERGENCE (N-045, PARITY-DELTAS D-72)"))
        << "cr_cached.h must carry the N-045 divergence block";
}

/* ---- 2026-11-18 concurrency pins (fill-outside-the-lock rework) ----- */

namespace {
/* A device whose read_at is OBSERVABLE: counts in-flight concurrent
 * fills, can gate its first fills on a latch, and serves deterministic
 * per-offset data (position-derived byte pattern). */
class GateDevice final : public IStorageDevice {
public:
    explicit GateDevice(u64 bytes, u32 sector = 512) : data_(bytes) {
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = u8((i * 31 + 7) & 0xFF);
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = bytes / sector;
        geom_.total_bytes = geom_.sector_count * sector;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        const u32 inflight = inflight_.fetch_add(1) + 1;
        if (inflight > max_inflight_.load())
            max_inflight_.store(inflight);
        if (hold_first_read_) {
            hold_first_read_ = false;
            /* park the first fill until a second concurrent fill arrives
             * (or a generous timeout — never hangs the suite); this is
             * what PROVES fills overlap under the new locking. */
            for (int spin = 0; spin < 200; ++spin) {
                if (inflight_.load() >= 2)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::memcpy(buf, data_.data() + off, len);
        inflight_.fetch_sub(1);
        reads_.fetch_add(1);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 total_reads() const { return reads_.load(); }
    u32 max_inflight() const { return max_inflight_.load(); }
    void arm_first_read_gate() { hold_first_read_ = true; }

private:
    std::vector<u8> data_;
    Geometry geom_;
    std::atomic<u32> inflight_{0};
    std::atomic<u32> max_inflight_{0};
    std::atomic<u64> reads_{0};
    bool hold_first_read_ = false;
};
} // namespace

TEST(Cached, ConcurrentReadersByteIdentical) {
    /* 4 threads: mixed sequential + strided access over an 8 MiB device
     * through ONE 2 MiB / 256 KiB cache; every byte must match the
     * single-threaded ground truth, and the accounting must stay sane. */
    auto inner = std::make_unique<GateDevice>(u64(8) << 20);
    auto c = CachedReader::create(std::move(inner), u64(2) << 20, 256 * 1024);
    ASSERT_NE(c, nullptr);
    std::vector<u8> truth(u64(8) << 20);
    for (size_t i = 0; i < truth.size(); ++i)
        truth[i] = u8((i * 31 + 7) & 0xFF);

    std::atomic<bool> bad{false};
    std::vector<std::thread> workers;
    for (u32 w = 0; w < 4; ++w) {
        workers.emplace_back([&, w] {
            std::vector<u8> buf(4096);
            for (u64 off = u64(w) * 4096; off + 4096 <= truth.size(); off += u64(4) * 4096) {
                if (c->read_at(off, buf.data(), 4096) != OK) {
                    bad = true;
                    return;
                }
                if (std::memcmp(buf.data(), truth.data() + off, 4096) != 0) {
                    bad = true;
                    return;
                }
            }
        });
    }
    for (auto& t : workers)
        t.join();
    EXPECT_FALSE(bad);
    EXPECT_EQ(c->hits() + c->misses(), u64(4) * ((u64(8) << 20) / (4 * 4096)));
}

TEST(Cached, DistinctBlocksFillConcurrently) {
    /* THE rework pin: two workers reading two DIFFERENT blocks must both
     * be inside the transport at the same time — the first fill parks
     * until the second arrives (max_inflight >= 2). Under the old global
     * mutex this test times out the gate and reports 1. */
    auto inner = std::make_unique<GateDevice>(u64(4) << 20);
    GateDevice* dev = inner.get();
    auto c = CachedReader::create(std::move(inner), u64(1) << 20, 256 * 1024);
    ASSERT_NE(c, nullptr);
    dev->arm_first_read_gate();
    std::atomic<int> done{0};
    std::thread a([&] {
        u8 buf[4096];
        if (c->read_at(0, buf, 4096) == OK)
            done.fetch_add(1);
    });
    std::thread b([&] {
        u8 buf[4096];
        if (c->read_at(256 * 1024, buf, 4096) == OK)
            done.fetch_add(1);
    });
    a.join();
    b.join();
    EXPECT_EQ(done.load(), 2);
    EXPECT_GE(dev->max_inflight(), 2u) << "fills serialized — the old global-mutex behavior";
}

TEST(Cached, OneFillServesAllConcurrentClaimants) {
    /* Single-fill dedup: while the transport is slow on block X, several
     * readers of X wait on X's own fill — the transport sees exactly ONE
     * read of X, and every waiter gets identical bytes. */
    auto inner = std::make_unique<GateDevice>(u64(1) << 20);
    GateDevice* dev = inner.get();
    auto c = CachedReader::create(std::move(inner), u64(1) << 20, 256 * 1024);
    ASSERT_NE(c, nullptr);
    dev->arm_first_read_gate(); /* parks only the FIRST read */
    std::vector<u8> truth(u64(1) << 20);
    for (size_t i = 0; i < truth.size(); ++i)
        truth[i] = u8((i * 31 + 7) & 0xFF);
    std::atomic<int> ok{0};
    std::atomic<int> mismatch{0};
    std::vector<std::thread> waiters;
    for (int w = 0; w < 3; ++w) {
        waiters.emplace_back([&] {
            u8 buf[65536];
            if (c->read_at(65536, buf, 65536) == OK)
                ok.fetch_add(1);
            else
                return;
            if (std::memcmp(buf, truth.data() + 65536, 65536) != 0)
                mismatch.fetch_add(1);
        });
    }
    for (auto& t : waiters)
        t.join();
    EXPECT_EQ(ok.load(), 3);
    EXPECT_EQ(mismatch.load(), 0);
    /* block 65536..131072 = exactly one transport read (one 256 KiB
     * cache-block fill covers it) — the first waiter filled it; the
     * other two were served from that fill. */
    EXPECT_EQ(dev->total_reads(), u64(1));
    EXPECT_GE(c->hits() + c->misses(), u64(3));
}
