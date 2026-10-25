/* hostile_smoke_s007.cpp - S-007 fuzz smoke: mutated common-format plants
 * driven through carve_slice (the full grammar + raw-sign pipeline) under
 * ASan. Standalone driver: the CI libFuzzer lane runs the real fuzz_carve
 * target; this is the in-session hostile-entry smoke (same pattern as the
 * S-012/S-013 smokes).
 *
 * Build: g++ -fsanitize=address -std=c++20 -I include hostile_smoke_s007.cpp
 *   <cr_core.a> -lz -llz4 -lzstd -lpthread -o smoke
 */
#include "cr/carve/cr_carve.h"
#include "cr/storage/cr_device.h"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace cr;
using namespace cr::carve;

namespace {

class MD final : public IStorageDevice {
public:
    explicit MD(const std::vector<u8>& b) : d_(b) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.sector_count = d_.size() / 512;
        g_.total_bytes = g_.sector_count * 512;
    }
    u32 read_at(u64 o, void* b, u32 l) override {
        if (o > d_.size() || l > d_.size() - o)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(b, d_.data() + o, l);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    std::vector<u8> d_;
    Geometry g_{};
};

void t16(std::vector<u8>& v, size_t o, u16 x) {
    v[o] = u8(x);
    v[o + 1] = u8(x >> 8);
}
void t32(std::vector<u8>& v, size_t o, u32 x) {
    t16(v, o, u16(x));
    t16(v, o + 2, u16(x >> 16));
}

/* seed image: every S-007 family planted (mirrors the gtest plants) */
std::vector<u8> seed() {
    std::vector<u8> img(256 * 1024, 0);
    /* GIF */
    {
        size_t at = 512;
        std::memcpy(img.data() + at, "GIF89a", 6);
        t16(img, at + 6, 16);
        t16(img, at + 8, 16);
        img[at + 10] = 0x80 | 2;
        size_t p = at + 13 + 24;
        img[p] = 0x2C;
        t16(img, p + 5, 16);
        t16(img, p + 7, 16);
        p += 10;
        img[p] = 8;
        ++p;
        img[p] = 32;
        for (int i = 0; i < 32; ++i)
            img[p + 1 + size_t(i)] = u8(i);
        p += 33;
        img[p] = 0;
        img[p + 1] = 0x3B;
    }
    /* BMP */
    {
        size_t at = 4096;
        img[at] = 'B';
        img[at + 1] = 'M';
        t32(img, at + 2, 14 + 40 + 48);
        t32(img, at + 10, 54);
        t32(img, at + 14, 40);
        t32(img, at + 18, 4);
        t32(img, at + 22, 4);
        t16(img, at + 26, 1);
        t16(img, at + 28, 24);
    }
    /* headerless MP3 (12 frames) */
    {
        size_t p = 8192;
        for (int f = 0; f < 12; ++f) {
            img[p] = 0xFF;
            img[p + 1] = 0xFB;
            img[p + 2] = 0x90;
            img[p + 3] = 0x00;
            p += 417;
        }
    }
    /* RTF */
    std::memcpy(img.data() + 20480, "{\\rtf1\\ansi{\\b hi}\\par}", 24);
    /* OLE */
    {
        size_t at = 24576;
        std::memcpy(img.data() + at, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8);
        t16(img, at + 24, 0x003E);
        t16(img, at + 26, 3);
        t16(img, at + 28, 0xFFFE);
        t16(img, at + 30, 9);
        t16(img, at + 32, 6);
        t32(img, at + 44, 1);
        t32(img, at + 48, 0);
        t32(img, at + 72, 0);
        t32(img, at + 76, 1);
        for (u32 i = 1; i < 109; ++i)
            t32(img, at + 76 + i * 4, 0xFFFFFFFF);
        t32(img, at + 1024 + 0, 0xFFFFFFFE);
        t32(img, at + 1024 + 4, 0xFFFFFFFD);
        std::memcpy(img.data() + at + 512, "Root Entry", 10);
    }
    /* MPEG-PS */
    {
        size_t p = 32768;
        img[p + 3] = 0xBA;
        img[p + 4] = 0x44;
        p += 14;
        img[p + 3] = 0xBB;
        t16(img, p + 4, 6);
        p += 12;
        img[p + 3] = 0xE0;
        t16(img, p + 4, 8);
        p += 14;
        img[p + 3] = 0xB9;
    }
    /* DWG */
    {
        size_t at = 65536;
        std::memcpy(img.data() + at, "AC1015", 6);
        std::memcpy(img.data() + at + 6, "\x0D\x0A\x0D\x0A\x00", 5);
    }
    /* AAC (12 frames) */
    {
        size_t p = 131072;
        for (int f = 0; f < 12; ++f) {
            img[p] = 0xFF;
            img[p + 1] = 0xF1;
            img[p + 2] = 0x50;
            img[p + 3] = 0x80;
            img[p + 4] = 25;
            img[p + 5] = 0x1F;
            img[p + 6] = 0xFC;
            p += 200;
        }
    }
    return img;
}

} // namespace

int main(int argc, char** argv) {
    const int iters = argc > 1 ? std::atoi(argv[1]) : 300;
    std::mt19937_64 rng(0x57007);
    auto base = seed();
    int carved_any = 0;
    for (int i = 0; i <= iters; ++i) {
        std::vector<u8> img = base;
        if (i > 0) {
            const int nmut = 1 + int(rng() % 24);
            for (int m = 0; m < nmut; ++m)
                img[size_t(rng() % img.size())] = uint8_t(rng());
            if (i % 3 == 0 && img.size() > 4096)
                img.resize(img.size() - 1 - size_t(rng() % 512));
        }
        MD dev(img);
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost");
        CarveFlags flags;
        CarveStats stats;
        (void)carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                          stats);
        if (stats.validated > 0)
            ++carved_any;
        if (i == 0)
            std::printf("seed sanity: validated=%llu rejected=%llu\n",
                        (unsigned long long)stats.validated, (unsigned long long)stats.rejected);
    }
    std::printf("hostile smoke: %d iters, carved on %d inputs\n", iters, carved_any);
    return carved_any > 0 ? 0 : 1;
}
