/*
 * bench_m005.cpp - local wall-clock bench for the M-005 carve gate
 * (the allocation-bitmap gating, RAWSearchDLL FUN_18000c820 :9446-9460).
 *
 * Fixture: 16 MiB, deterministic LCG pseudo-random fill (see the lesson
 * below - representative mixed content: most positions reject on the
 * first byte, some engage the grammar; exactly the load real disks put
 * on the dispatch layer). 4 KiB clusters, alternating clusters marked
 * used (50% used). The read cost (memcpy from the in-memory device) is
 * paid identically by both legs; the delta is the dispatch suppression.
 * 3 measured runs per leg + 1 warmup each (median reported).
 *
 * BENCH LESSON (recorded in M-005's Resolution): an earlier 0xFF-filled
 * fixture was abandoned - it is adversarial, not representative. The
 * grammar registry carries FMT_MPGA_FF with magic {0xFF, 0xFF} (the og's
 * raw-sign table carries the same FF sync family), so an all-0xFF image
 * makes EVERY position a candidate and the validation path dominates.
 * Real disks are mixed-content; pseudo-random fill models that.
 *
 * Build (from the repo root, linux-release tree):
 *   g++ -O2 -std=c++20 -I include scripts/bench_m005.cpp \
 *       build/linux-release/src/core/libcr_core.a -lpthread \
 *       -o build/bench_m005 && ./build/bench_m005
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_carve.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_freemap.h"
#include "cr/tree/cr_node.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::carve;

namespace {

class MemDev final : public IStorageDevice {
public:
    explicit MemDev(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
};

double one_run(IStorageDevice* dev, const storage::FreeMap* fm, u64& skipped_out) {
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    flags.free_map = fm;
    CarveStats st;
    const auto t0 = std::chrono::steady_clock::now();
    u32 rc =
        carve_slice(dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st);
    const auto t1 = std::chrono::steady_clock::now();
    if (rc != OK) {
        std::fprintf(stderr, "carve_slice failed: 0x%x\n", rc);
        std::exit(1);
    }
    skipped_out = st.bitmap_skipped_bytes;
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

} // namespace

int main() {
    constexpr u64 IMG = 16ull << 20; /* 16 MiB */
    constexpr u32 CB = 4096;
    constexpr int RUNS = 3;
    const size_t img_bytes = size_t(IMG);
    std::vector<u8> img(img_bytes);
    u64 lcg = 0x9E3779B97F4A7C15ULL;
    for (auto& b : img) { /* deterministic mixed-content fill */
        lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
        b = u8(lcg >> 56);
    }
    MemDev dev(std::move(img));

    /* 50% used: odd clusters used, even clusters free */
    storage::FreeMap fm;
    fm.base_byte = 0;
    fm.cluster_bytes = CB;
    fm.cluster_count = IMG / CB;
    fm.bits.assign(size_t(fm.cluster_count / 8), 0xFF);
    for (u64 k = 1; k < fm.cluster_count; k += 2)
        fm.bits[size_t(k / 8)] &= u8(~(1u << (k % 8)));

    /* warmup + measured runs per leg */
    u64 skipped = 0;
    (void)one_run(&dev, nullptr, skipped);
    double ug[RUNS], gt[RUNS];
    for (int i = 0; i < RUNS; ++i)
        ug[i] = one_run(&dev, nullptr, skipped);
    (void)one_run(&dev, &fm, skipped);
    for (int i = 0; i < RUNS; ++i)
        gt[i] = one_run(&dev, &fm, skipped);

    auto med = [&](double* v) {
        for (int i = 1; i < RUNS; ++i) { /* insertion sort */
            double x = v[i];
            int j = i - 1;
            while (j >= 0 && v[j] > x) {
                v[j + 1] = v[j];
                --j;
            }
            v[j + 1] = x;
        }
        return v[RUNS / 2];
    };
    const double m_ungated = med(ug), m_gated = med(gt);
    std::printf("M-005 gate bench (16 MiB LCG-random fixture, 4 KiB clusters, 50%% used, "
                "1 thread, median of %d):\n", RUNS);
    std::printf("  ungated: %.1f ms  (candidates offered: every position)\n", m_ungated);
    std::printf("  gated:   %.1f ms  (bitmap_skipped_bytes: %llu, %.1f%% of the image)\n",
                m_gated, (unsigned long long)skipped, 100.0 * double(skipped) / double(IMG));
    std::printf("  speedup: %.2fx\n", m_ungated / m_gated);
    return 0;
}
