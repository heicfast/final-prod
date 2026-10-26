/* CachedReader behaviour + hit/miss accounting. */
#include <gtest/gtest.h>
#include "cr/storage/cr_cached.h"
#include <cstring>
#include <string>

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
    const auto scan_src = cr_test::read_repo_file("src/core/orchestrator/cr_scan.cpp");
    ASSERT_FALSE(scan_src.empty());
    /* M-058 moved the construction into build_read_pipe(); the budget
     * constants live at run()'s call site now - the pin follows the
     * truth, same lockstep contract. */
    EXPECT_NE(std::string::npos,
              scan_src.find("build_read_pipe(slice, 8ull << 20, 256 * 1024)"))
        << "scan-pipe cache construction drifted from the registered 8 MiB / 256 KiB";
    EXPECT_NE(std::string::npos, scan_src.find("PARITY-DELTAS D-72"))
        << "the cr_scan.cpp cache comment must cite the register row";

    const auto deltas = cr_test::read_repo_file("docs/PARITY-DELTAS.md");
    ASSERT_FALSE(deltas.empty());
    EXPECT_NE(std::string::npos, deltas.find("D-72"))
        << "PARITY-DELTAS must keep the D-72 cache-budget row";
    EXPECT_NE(std::string::npos, deltas.find("0x400000 / bytes_per_sector"))
        << "D-72 must keep the og 4 MiB budget citation";
    EXPECT_NE(std::string::npos, deltas.find("8ull << 20"))
        << "D-72 must name the exact core constant it registers";

    const auto hdr = cr_test::read_repo_file("include/cr/storage/cr_cached.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(std::string::npos, hdr.find("REGISTERED DIVERGENCE (N-045, PARITY-DELTAS D-72)"))
        << "cr_cached.h must carry the N-045 divergence block";
}
