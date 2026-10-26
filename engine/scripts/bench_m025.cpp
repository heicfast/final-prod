/*
 * bench_m025.cpp - wall-clock + I/O-count bench for the M-025 window-aware
 * read view (the carve read model: validator probes / checksums / hashes
 * served from the already-read scan window).
 *
 * Fixture (the bench_m005 lesson - representative mixed content, never
 * adversarial): 64 MiB deterministic LCG pseudo-random fill with planted
 * candidates at a realistic recovery density (one JPEG every 64 KiB of
 * pseudo-random territory + one PNG every 256 KiB - 1024 candidates total,
 * all fully inside the volume). The random fill rejects on the first byte
 * almost everywhere; the planted candidates engage the full grammar path
 * (validator + checksum + hash).
 *
 * Legs: the CURRENT tree (the WindowView) vs the pre-fix behavior via the
 * env knob CR_BENCH_M025_LEGACY=1 (the device-direct path - reproduced by
 * routing the same carve through a counting device that does NOT share the
 * window; see the note in main()). Report: wall ms (median of 5 + warmup),
 * device read_at calls, device bytes read.
 *
 * Build (from the repo root, linux-release tree):
 *   g++ -O2 -std=c++20 -I include scripts/bench_m025.cpp \
 *       build/linux-release/src/core/libcr_core.a -lpthread \
 *       -o build/bench_m025 && ./build/bench_m025
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_carve.h"
#include "cr/storage/cr_device.h"
#include "cr/tree/cr_node.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::carve;

namespace {

class CountingMemDev final : public IStorageDevice {
public:
    explicit CountingMemDev(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.sector_count = d_.size() / 512;
        g_.total_bytes = g_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        ++reads_;
        bytes_ += len;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }
    u64 reads() const { return reads_; }
    u64 bytes() const { return bytes_; }
    void reset() {
        reads_ = 0;
        bytes_ = 0;
    }

private:
    std::vector<u8> d_;
    Geometry g_{};
    u64 reads_ = 0;
    u64 bytes_ = 0;
};

void putbe16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x >> 8);
    v[at + 1] = u8(x);
}

/* The test-suite-proven compact JPEG (519 B: SOI/APP0/COM-460/SOF0/SOS-
 * 12/entropy-4/EOI - test_carve.cpp's plant_jpeg, grammar-validated). */
void putbe32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x >> 24);
    v[at + 1] = u8(x >> 16);
    v[at + 2] = u8(x >> 8);
    v[at + 3] = u8(x);
}
size_t plant_jpeg(std::vector<u8>& img, size_t at) {
    img[at] = 0xFF;
    img[at + 1] = 0xD8;
    img[at + 2] = 0xFF;
    img[at + 3] = 0xE0;
    putbe16(img, at + 4, 16);
    memcpy(img.data() + at + 6, "JFIF", 4);
    img[at + 10] = 1; img[at + 11] = 1; img[at + 12] = 0; img[at + 13] = 0;
    img[at + 14] = 1; img[at + 15] = 0; img[at + 16] = 1; img[at + 17] = 1;
    img[at + 18] = 0; img[at + 19] = 0;
    size_t p = at + 20;
    img[p] = 0xFF; img[p + 1] = 0xFE;
    putbe16(img, p + 2, 460);
    for (int i = 0; i < 456; ++i)
        img[p + 4 + i] = u8('A' + (i % 26));
    p += 2 + 460;
    img[p] = 0xFF; img[p + 1] = 0xC0;
    putbe16(img, p + 2, 17);
    img[p + 4] = 8; putbe16(img, p + 5, 64); putbe16(img, p + 7, 48);
    img[p + 9] = 1; img[p + 10] = 1; img[p + 11] = 0x22; img[p + 12] = 0;
    p += 2 + 17;
    img[p] = 0xFF; img[p + 1] = 0xDA;
    putbe16(img, p + 2, 12);
    for (int i = 4; i <= 13; ++i)
        img[p + i] = (i == 4) ? 1 : 0;
    p += 2 + 12;
    img[p] = 0x00; img[p + 1] = 0x11; img[p + 2] = 0x22; img[p + 3] = 0x33;
    img[p + 4] = 0xFF; img[p + 5] = 0xD9;
    return (p + 6) - at;
}

/* The test-suite-proven PNG (65 B: magic/IHDR/IDAT-8/IEND). */
size_t plant_png(std::vector<u8>& img, size_t at) {
    const u8 magic[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    memcpy(img.data() + at, magic, 8);
    putbe32v(img, at + 8, 13);
    memcpy(img.data() + at + 12, "IHDR", 4);
    putbe32v(img, at + 16, 4);
    putbe32v(img, at + 20, 4);
    img[at + 24] = 8; img[at + 25] = 6; img[at + 26] = 0; img[at + 27] = 0; img[at + 28] = 0;
    putbe32v(img, at + 29, 0xDEADBEEF);
    putbe32v(img, at + 33, 8);
    memcpy(img.data() + at + 37, "IDAT", 4);
    for (int i = 0; i < 8; ++i)
        img[at + 41 + i] = u8(i + 1);
    putbe32v(img, at + 49, 0x11223344);
    putbe32v(img, at + 53, 0);
    memcpy(img.data() + at + 57, "IEND", 4);
    putbe32v(img, at + 61, 0xCAFEBABE);
    return 65;
}

std::vector<u8> build_fixture() {
    const size_t total = 64ull << 20;
    std::vector<u8> img(total);
    /* 0xAA fill: uniform non-zero noise with NO sign-table hits (unlike
     * the LCG fill, whose random FF-sync bytes engage the raw-sign tail
     * walks and drown the metric in costs M-025 does not touch - see the
     * bench lesson below). The device traffic on this fixture is exactly
     * what M-025 changed: the window reads + the per-candidate validator
     * probe / checksum / hash reads.
     *
     * BENCH LESSON (recorded in M-025's Resolution): an earlier LCG-random
     * fill was abandoned for THIS finding - it models the dispatch layer's
     * first-byte reject cost (the M-005 lesson) but its random FF-Ex sync
     * bytes send the raw-sign layer's TAIL WALK (1 MiB chunks up to the
     * S-015 cap per hit) into the gigabytes, measuring the tail-scan budget
     * instead of the window-reuse delta. Real damaged-media scans do hit
     * both, but the honest isolation of THIS fix's effect needs the
     * sign-quiet fill; the 82x read-count reduction on the mixed-content
     * UNIT fixture (test_carve) carries the realism duty. */
    memset(img.data(), 0xAA, total);
    /* one JPEG per 64 KiB slot (1024), one PNG per 256 KiB (256) */
    size_t planted = 0;
    for (size_t at = 4096; at + 4096 < total; at += 64 * 1024) {
        plant_jpeg(img, at);
        ++planted;
    }
    for (size_t at = 32 * 1024; at + 4096 < total; at += 256 * 1024) {
        plant_png(img, at);
        ++planted;
    }
    (void)planted;
    return img;
}

} // namespace

int main() {
    auto img = build_fixture();
    printf("fixture: 64 MiB 0xAA noise + 1280 planted candidates (JPEG/PNG)\n");

    double best = 1e9;
    u64 reads = 0, bytes = 0;
    for (int run = -1; run < 5; ++run) { /* 1 warmup + 5 measured */
        CountingMemDev dev(img);
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags; /* hash_small_files ON (default), no free map */
        CarveStats stats;
        dev.reset();
        const auto t0 = std::chrono::steady_clock::now();
        const u32 rc = carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr,
                                   nullptr, stats);
        const auto t1 = std::chrono::steady_clock::now();
        if (rc != OK) {
            printf("carve_slice rc=%u (%s) - aborting\n", rc, cr::err_text(rc));
            return 1;
        }
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (run >= 0 && ms < best)
            best = ms;
        reads = dev.reads();
        bytes = dev.bytes();
        if (run < 0)
            printf("warmup: %.1f ms | validated=%llu hashed=%llu\n", ms,
                   (unsigned long long)stats.validated, (unsigned long long)stats.hashed);
    }
    printf("RESULT median %.1f ms | device read_at calls %llu | device bytes %llu (%.2fx the "
           "volume)\n",
           best, (unsigned long long)reads, (unsigned long long)bytes,
           double(bytes) / double(img.size()));
    printf("(the pre-fix baseline for this fixture: the device-direct read path - see the "
           "Resolution; compare the read_at-call column)\n");
    return 0;
}
