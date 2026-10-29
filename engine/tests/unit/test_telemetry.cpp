/* test_telemetry.cpp - G-04/G-05/G-06 acceptance: uniform per-stage
 * progress across engines (incl. FAT incremental dentry ticks), monotonic
 * overall permille, per-stage elapsed, live bad-sector map mid-scan, and
 * thread-safe telemetry queries while the scan runs (TSan-covered in CI).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

using namespace cr;
using namespace cr::orchestrator;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
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

/* Device with a dead region: reads that intersect [bad_start, bad_start+bad_len)
 * fail (the retry pipe retries, bisects, records the range, zero-fills).
 * park_on_hit() parks reads that land FAR PAST the dead spot once it has
 * been hit: the quick pass's device traffic (boot/FAT/root + bisection
 * sub-reads) is all near the dead spot, so the park always engages in a
 * LATER stage read - after the pipe has already recorded the range.
 * That makes the live mid-scan state deterministically observable. */
class BadRegionDevice final : public IStorageDevice {
public:
    BadRegionDevice(std::vector<u8> b, u64 bad_start, u64 bad_len)
        : data_(std::move(b)), bad_start_(bad_start), bad_len_(bad_len) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    void park_on_hit() { park_ = true; }
    /* N-239: the far reads SLEEP instead of blocking - progress ticks
     * keep flowing while the scan works the far region (a parked thread
     * freezes the permille, and with it every later tick: no wait can
     * ever observe a delta the coalescer attaches after the freeze -
     * the release-27/28/357 Windows reds, all three shapes). */
    void slow_far_on_hit() { slow_ = true; }
    /* unpark releases BOTH modes: a released scan must finish fast
     * (slow-far's per-read sleep over thousands of sector-granular
     * tail reads is minutes - the 116 s joins first-hand) */
    void unpark() {
        park_ = false;
        slow_ = false;
    }
    bool hit() const { return hit_.load(std::memory_order_acquire); }
    u32 read_at(u64 off, void* buf, u32 len) override {
        const bool hits = off < bad_start_ + bad_len_ && bad_start_ < off + len;
        if (hits) {
            hit_.store(true, std::memory_order_release);
            return ERR_READ_FAILED;
        }
        const bool far = off > bad_start_ + bad_len_ + (1ull << 20);
        if (far && hit_.load(std::memory_order_acquire) && park_.load(std::memory_order_acquire)) {
            while (park_.load(std::memory_order_acquire))
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else if (off > bad_start_ + bad_len_ && hit_.load(std::memory_order_acquire) &&
                   slow_.load(std::memory_order_acquire)) {
            /* N-239: the slow region starts AT the dead spot (not at the
             * park threshold) - every post-spot read holds 100 ms, the
             * permille sweeps through hundreds of ticks after the JPEGs
             * carve: the widest observation window the design can give
             * the attach-vs-tick race. The discriminator: if the delta
             * still never rides a tick on Windows, the attach itself is
             * not happening there - the api-finding's evidence. */
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    u64 bad_start_, bad_len_;
    Geometry geom_;
    std::atomic<bool> park_{false};
    std::atomic<bool> hit_{false};
    std::atomic<bool> slow_{false};
};

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}
void putbe16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x >> 8);
    v[at + 1] = u8(x);
}

/* A grammar-complete JPEG (SOI/APP0/COM/SOF0/SOS/EOI, 519 bytes >= the
 * 512 min_size) - same shape as test_carve's plant_jpeg, so the carve
 * stage has something to find in these fixtures. */
size_t plant_jpeg(std::vector<u8>& img, size_t at) {
    img[at] = 0xFF;
    img[at + 1] = 0xD8; /* SOI */
    img[at + 2] = 0xFF;
    img[at + 3] = 0xE0; /* APP0 */
    putbe16(img, at + 4, 16);
    std::memcpy(img.data() + at + 6, "JFIF", 4);
    img[at + 10] = 1;
    img[at + 11] = 1;
    img[at + 12] = 0;
    img[at + 13] = 0;
    img[at + 14] = 1;
    img[at + 15] = 0;
    img[at + 16] = 1;
    img[at + 17] = 1;
    img[at + 18] = 0;
    img[at + 19] = 0; /* APP0 done: 2+2+16 */
    size_t p = at + 20;
    img[p] = 0xFF;
    img[p + 1] = 0xFE;        /* COM (comment) */
    putbe16(img, p + 2, 460); /* big payload */
    for (int i = 0; i < 456; ++i)
        img[p + 4 + i] = u8('A' + (i % 26));
    p += 2 + 460; /* SOF0 next */
    img[p] = 0xFF;
    img[p + 1] = 0xC0; /* SOF0 */
    putbe16(img, p + 2, 17);
    img[p + 4] = 8;
    putbe16(img, p + 5, 64);
    putbe16(img, p + 7, 48);
    img[p + 9] = 1;
    img[p + 10] = 1;
    img[p + 11] = 0x22;
    img[p + 12] = 0;
    p += 2 + 17;
    img[p] = 0xFF;
    img[p + 1] = 0xDA; /* SOS */
    putbe16(img, p + 2, 12);
    img[p + 4] = 1;
    p += 2 + 12;
    img[p] = 0x00;
    img[p + 1] = 0x11;
    img[p + 2] = 0x22;
    img[p + 3] = 0x33;
    img[p + 4] = 0xFF;
    img[p + 5] = 0xD9; /* EOI */
    return (p + 6) - at;
}

/* FAT16 fixture with a WIDE root directory (>64 dentries so the FAT walk
 * emits intermediate progress ticks, not just the end-of-directory call). */
std::vector<u8> build_fat16_wide(size_t root_files) {
    const u32 total_sectors = 5074 + u32(root_files / 8);
    std::vector<u8> img(u64(total_sectors) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 2);
    b[0x10] = 2;
    put16(img, 0x11, 512);
    put16(img, 0x13, u16(total_sectors));
    b[0x15] = 0xF8;
    put16(img, 0x16, 20);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    b[0x26] = 0x29;
    std::memcpy(b + 0x2B, "WIDEVOL    ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512, ROOT = 42 * 512;
    /* M-015: canonical media words (real-volume shape) */
    put16(img, FAT1 + 0 * 2, 0xFFF8);
    put16(img, FAT1 + 1 * 2, 0xFFFF);
    put16(img, FAT2 + 0 * 2, 0xFFF8);
    put16(img, FAT2 + 1 * 2, 0xFFFF);
    put16(img, FAT1 + 5 * 2, 0xFFFF);
    put16(img, FAT2 + 5 * 2, 0xFFFF);
    u8* root = img.data() + ROOT;
    for (size_t i = 0; i < root_files; ++i) {
        u8* e = root + i * 32;
        std::memset(e, 0, 32);
        char n11[32]; /* 8.3-safe: 11 chars used; 32 silences gcc
                         -Wformat-truncation (size_t worst case) */
        snprintf(n11, sizeof(n11), "F%06zu TXT", i);
        for (int k = 0; k < 11; ++k)
            e[k] = u8(n11[k]);
        e[11] = 0x20;                           /* plain file */
        put16(img, ROOT + i * 32 + 26, u16(2)); /* cluster 2 */
        /* M-130: a sized stream - the og's emission-walk shape gate
         * (FUN_1800365a0 :44344-48) rejects zero-size dentries that
         * carry a chain; the fixture's old cluster-2/size-0 shape is
         * the illegal class (fixtures derive from the dentry contract,
         * never from a lax implementation - the S-042 rule). */
        put32v(img, ROOT + i * 32 + 28, 1);
    }
    /* one carvable JPEG in the tail (beyond the last root slot) - gives
     * the carve stage something to validate in these fixtures */
    (void)plant_jpeg(img, size_t(img.size()) - 4 * 512);
    return img;
}

} // namespace

TEST(Telemetry, UniformProgressAcrossStagesMonotonic) {
    /* every engine now receives the bridge; the overall permille must be
     * monotonic non-decreasing and reach 1000 on completion. The capture
     * is mutex-guarded: the bridge invokes the callback from engine worker
     * threads AND the scan thread concurrently (CI TSan caught the plain
     * vector push_back racing — the callback contract requires consumers
     * to be thread-safe). */
    MemDevice dev(build_fat16_wide(200));
    tree::ResultTree tree;
    ScanConfig cfg;
    ScanController ctrl(tree, cfg);
    ScanStats stats;

    struct Capture {
        std::mutex m;
        std::vector<u32> permilles;
    } cap;
    auto cb = [](u32 overall, void* user) {
        auto* c = static_cast<Capture*>(user);
        std::lock_guard<std::mutex> lock(c->m);
        c->permilles.push_back(overall);
        return true;
    };
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, cb, &cap, stats));
    std::lock_guard<std::mutex> lock(cap.m);
    ASSERT_FALSE(cap.permilles.empty());
    u32 prev = 0;
    for (u32 p : cap.permilles) {
        EXPECT_GE(p, prev); /* monotonic (G-04) */
        EXPECT_LE(p, 1000u);
        prev = p;
    }
    EXPECT_EQ(1000u, cap.permilles.back());
}

TEST(Telemetry, FatWalkEmitsIncrementalTicks) {
    /* G-04 core fix, proven SYNCHRONOUSLY through the user callback (no
     * polling race): the walk emits a tick at every 64th dentry examined,
     * so the callback observes distinct overall permilles strictly below
     * the quick boundary (750 = weighted(1000,1000,0), covers_both) while
     * the walk is still running. The pre-fix engine emitted exactly one
     * end-of-directory tick - nothing below the boundary. */
    MemDevice dev(build_fat16_wide(200));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;

    struct Cap {
        std::mutex m;
        std::vector<u32> overall;
    } cap;
    auto cb = [](u32 overall, void* user) {
        auto* c = static_cast<Cap*>(user);
        std::lock_guard<std::mutex> lock(c->m);
        c->overall.push_back(overall);
        return true;
    };
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, cb, &cap, stats));
    {
        std::lock_guard<std::mutex> lock(cap.m);
        ASSERT_FALSE(cap.overall.empty());
        u32 prev = 0, below = 0, last_below = 0;
        for (u32 p : cap.overall) {
            EXPECT_GE(p, prev); /* monotonic across ticks AND boundaries */
            EXPECT_LE(p, 1000u);
            if (p < 750u && p != last_below) { /* distinct mid-walk values */
                ++below;
                last_below = p;
            }
            prev = p;
        }
        /* ticks at dentries 64/128/192 (root walk stops at the first 0x00
         * slot, i.e. after 200 entries) - at least three intermediates */
        EXPECT_GE(below, 3u);
        EXPECT_EQ(750u, prev); /* ends exactly at the quick->deep boundary */
    }
}

TEST(Telemetry, PerStageShapeAndCompletion) {
    MemDevice dev(build_fat16_wide(100));
    tree::ResultTree tree;
    ScanConfig cfg;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));

    const auto t = ctrl.telemetry();
    EXPECT_FALSE(t.running);
    EXPECT_EQ(1000u, t.overall_permille);
    EXPECT_TRUE(t.quick.completed);
    EXPECT_TRUE(t.deep.completed);
    EXPECT_TRUE(t.carve.completed);
    EXPECT_EQ(1000u, t.quick.permille);
    EXPECT_EQ(1000u, t.deep.permille);
    EXPECT_EQ(1000u, t.carve.permille);
    /* carve counts BYTES (G-05): bytes_done == units_done, and the whole
     * image was carved */
    EXPECT_EQ(t.carve.units_done, t.carve.bytes_done);
    EXPECT_GT(t.carve.bytes_done, u64(0));
    EXPECT_EQ(dev.size(), t.bytes_scanned);
    EXPECT_GT(t.files_found, u64(0));
    EXPECT_GT(t.carved_found, u64(0));
    EXPECT_GE(t.elapsed_ms, u64(0));
}

TEST(Telemetry, LiveErrorMapVisibleMidScan) {
    /* G-06, deterministic: the quick pass hits the dead spot (the retry
     * pipe records + zero-fills it), then the scan PARKS in a far later
     * read (carve stage) - with the error recorded BEFORE the park. The
     * live map is queryable while run() is parked. Dead spot inside
     * FAT2's region, away from the boot sector. */
    std::vector<u8> img = build_fat16_wide(100);
    const u64 bad = 30 * 512;
    BadRegionDevice dev(img, bad, 512);
    dev.park_on_hit();
    tree::ResultTree tree;
    ScanConfig cfg; /* carve enabled: it reads the far region and parks */
    ScanController ctrl(tree, cfg);
    ScanStats stats;

    std::thread scan([&] { (void)ctrl.run(&dev, nullptr, nullptr, nullptr, stats); });
    /* bounded wait: the dead spot was hit, then the scan parked in a
     * later-stage read (it cannot finish - the park holds it) */
    bool hit = false;
    for (int i = 0; i < 1500 && !hit; ++i) {
        hit = dev.hit();
        if (!hit)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_TRUE(hit);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); /* settle */

    /* mid-scan (G-06): running, and the live bad-sector map is visible */
    const auto t = ctrl.telemetry();
    ASSERT_TRUE(t.running);
    ASSERT_FALSE(t.errors.ranges().empty());
    bool found = false;
    for (const auto& r : t.errors.ranges())
        if (r.offset == bad)
            found = true;
    EXPECT_TRUE(found);

    dev.unpark();
    scan.join();
    /* end-of-run: the map also lands in stats (save-on-cancel contract) */
    EXPECT_FALSE(stats.errors.ranges().empty());
    found = false;
    for (const auto& r : stats.errors.ranges())
        if (r.offset == bad)
            found = true;
    EXPECT_TRUE(found);
}

TEST(Telemetry, QueryDuringScanIsThreadSafe) {
    /* concurrent telemetry() from two threads while the scan runs - the
     * TSan build proves the locking (G-04 acceptance). */
    MemDevice dev(build_fat16_wide(300));
    tree::ResultTree tree;
    ScanConfig cfg;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    std::atomic<bool> stop{false};
    std::thread q1([&] {
        while (!stop.load()) {
            (void)ctrl.telemetry();
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });
    std::thread q2([&] {
        while (!stop.load()) {
            (void)ctrl.telemetry();
            std::this_thread::sleep_for(std::chrono::microseconds(70));
        }
    });
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    stop.store(true);
    q1.join();
    q2.join();
}

/* ---- N-166: live counter refresh on progress samples ------------------ */

TEST(Telemetry, LiveAttachCounterTracksTheTree) {
    /* the O(1) attach counter: every add_file ticks, detach subtracts,
     * reset zeroes - the telemetry overlay's data source. */
    tree::ResultTree t;
    auto* r = t.add_root(u"Found");
    EXPECT_EQ(t.live_files(), 0u);
    for (int i = 0; i < 5; ++i) {
        auto* p = new tree::FileProperties();
        p->name = u"f";
        t.add_file(r, p);
        p->release();
    }
    EXPECT_EQ(t.live_files(), 5u);
    t.refresh_stats();
    EXPECT_EQ(t.live_files(), 5u); /* refresh does not disturb the counter */
    t.reset();
    EXPECT_EQ(t.live_files(), 0u);
}

TEST(Telemetry, CountersMoveDuringStage) {
    /* N-166: the og's results grid tracks the live tree (~2 s cadence);
     * the core's ScanProgress samples carry the attach-time delta - a
     * mid-carve event shows carved files BEFORE the stage completes.
     * Deterministic via the park device: early JPEGs carve, then the
     * far-region read parks the scan - the LAST pre-park progress event
     * must carry carved_found >= 1 (pre-fix: the frozen stage-boundary
     * snapshot shows 0 for the whole stage). */
    std::vector<u8> img = build_fat16_wide(200);
    /* three carvable JPEGs in the early data region (sectors 100-140,
     * inside the FIRST 1 MiB carve window, before the dead spot) */
    plant_jpeg(img, 100 * 512);
    plant_jpeg(img, 120 * 512);
    plant_jpeg(img, 140 * 512);
    /* the dead spot at sector 1800 (the WINDOW-0 tail, PAST the JPEGs):
     * the 1 MiB windowing (cr_carve.cpp WINDOW) makes the geometry
     * deterministic - window 0 [0,1M) contains the JPEGs and the dead
     * spot; its read-failure split carves the JPEGs from the good
     * sub-ranges (every sub-read stays below the park threshold, so no
     * split ordering can freeze the window before the attach), window 1
     * [1M,2M) is clean, and window 2 [2M,2.43M) starts BEYOND the park
     * threshold (bad+len+1MiB = ~1.88 MiB) - its read parks the scan.
     * The structured pass never reads sector 1800 (all 200 root files
     * point at cluster 2), so no structured-stage retries delay the
     * carve. The original shape (dead spot at sector 30, in the FAT)
     * made the structured pass retry with backoff - under load the
     * assertions fired before the carve stage even began (release-27:
     * last_carved=0 on the Windows runner; the same red at local -j4). */
    BadRegionDevice dev(img, 1800 * 512, 512);
    /* N-239: the far reads SLOW (never block) so ticks keep flowing
     * while the carve results attach to the live tree; the park is
     * ARMED BY THE OBSERVER at the first carved-carrying tick, ONE-
     * SHOT. The freeze can then never hide the property - the release
     * reds (27/28/357) were all the freeze racing the attach: the
     * upfront park froze the permille at the last pre-attach tick and
     * no later tick could ever fire while parked. A re-arming observer
     * re-parks post-unpark and hangs the join (first-hand); the armed
     * flag is mutex-guarded with the event vector. */
    dev.slow_far_on_hit();

    std::mutex m;
    std::vector<event::Event> evts;
    BadRegionDevice* dev_p = &dev;
    std::mutex* m_p = &m;
    std::vector<event::Event>* evts_p = &evts;
    tree::ResultTree tree;
    ScanConfig cfg;
    struct Obs {
        BadRegionDevice* dev;
        std::mutex* m;
        std::vector<event::Event>* evts;
        bool armed = false;
    } obs{dev_p, m_p, evts_p};
    cfg.event_fn = [](const event::Event& e, void* user) {
        auto* o = static_cast<Obs*>(user);
        {
            std::lock_guard<std::mutex> lock(*o->m);
            o->evts->push_back(e);
        }
        if (e.type == event::Type::ScanProgress && e.carved_found >= 1) {
            std::lock_guard<std::mutex> lock(*o->m);
            if (!o->armed) {
                o->armed = true;
                o->dev->park_on_hit();
            }
        }
    };
    cfg.event_user = &obs;
    ScanController ctrl(tree, cfg);
    ScanStats stats;

    std::thread scan([&] { (void)ctrl.run(&dev, nullptr, nullptr, nullptr, stats); });
    bool hit = false;
    for (int i = 0; i < 1500 && !hit; ++i) {
        hit = dev.hit();
        if (!hit)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_TRUE(hit);
    /* settle: wait for the carved-carrying tick (the observer arms the
     * ONE-SHOT park at that moment), then a brief settle for the freeze
     * to engage. If the delta never reaches a mid-stage tick, the scan
     * runs to completion and the running assert below fails - the
     * property stays guarded on every platform, without the cadence
     * race (N-235's condition-wait form over N-239's arming design).
     * The event vector is mutex-guarded: the observer records from the
     * emitter thread while this poll reads concurrently. */
    bool delta_seen = false;
    for (int i = 0; i < 15000 && !delta_seen; ++i) {
        {
            std::lock_guard<std::mutex> lock(m);
            for (const auto& e : evts)
                if (e.type == event::Type::ScanProgress && e.carved_found >= 1)
                    delta_seen = true;
        }
        if (!delta_seen)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    for (int i = 0; i < 150; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(ctrl.telemetry().running) << "the scan is parked mid-carve";

    /* the last ScanProgress sample carries the carved delta */
    u64 last_carved = 0;
    bool saw_progress = false;
    {
        std::lock_guard<std::mutex> lock(m);
        for (const auto& e : evts) {
            if (e.type == event::Type::ScanProgress) {
                saw_progress = true;
                last_carved = e.carved_found; /* last-wins per sample */
            }
        }
    }
    EXPECT_TRUE(delta_seen) << "a mid-stage tick carried the carved count "
                               "(the attach-time delta, not the frozen snapshot)";
    EXPECT_TRUE(saw_progress) << "the carve stage emitted progress samples";
    EXPECT_GE(last_carved, 1u) << "mid-stage progress carries the carved count";

    dev.unpark();
    scan.join();
    EXPECT_GE(stats.carved_found, 3u) << "the parked scan's eventual carve total";
}
