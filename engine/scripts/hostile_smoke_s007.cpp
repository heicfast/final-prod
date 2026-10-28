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
    /* ---- S-007 niche batch 1 (SCAN-2f): the walk-heavy + multi-field
     * families - MID/OGG/MXF/TAR/TTF/ICO/WMF-std/AU - hostile mutations
     * below hammer their length fields and walk states ---- */
    {
        /* MID at 145408 */
        size_t at = 145408;
        std::memcpy(img.data() + at, "MThd", 4);
        img[at + 7] = 6; /* BE len 6 */
        img[at + 10] = 1; /* BE ntrks 1 */
        img[at + 13] = 1; /* BE division 1 */
        std::memcpy(img.data() + at + 14, "MTrk", 4);
        img[at + 21] = 16; /* BE track len 16 */
    }
    {
        /* OGG (BOS + EOS pages) at 151552 */
        size_t at = 151552;
        std::memcpy(img.data() + at, "OggS", 4);
        img[at + 5] = 0x02; /* BOS */
        img[at + 26] = 1; /* one segment */
        img[at + 27] = 30; /* segment length */
        std::memcpy(img.data() + at + 58, "OggS", 4);
        img[at + 58 + 5] = 0x04; /* EOS */
        img[at + 58 + 26] = 1;
        img[at + 58 + 27] = 8;
    }
    {
        /* MXF (two KLV items) at 155648 */
        size_t at = 155648;
        std::memcpy(img.data() + at, "\x06\x0E\x2B\x34", 4);
        img[at + 16] = 0x81; /* long-form BER, 1 length byte */
        img[at + 17] = 8;
        std::memcpy(img.data() + at + 25, "\x06\x0E\x2B\x34", 4);
        img[at + 41] = 0x84; /* long-form BER, 4 length bytes */
        img[at + 42] = 0x7F; /* hostile length bytes */
        img[at + 43] = 0xFF;
        img[at + 44] = 0xFF;
        img[at + 45] = 0x0A;
    }
    {
        /* TAR (valid checksum member + EOF block) at 160768 */
        size_t at = 160768;
        std::memset(img.data() + at, 0, 512);
        std::memcpy(img.data() + at, "f.txt", 5);
        std::memcpy(img.data() + at + 124, "00000000020", 11); /* size 16 */
        std::memcpy(img.data() + at + 257, "ustar", 5);
        for (int i = 0; i < 8; ++i)
            img[at + 148 + size_t(i)] = ' ';
        unsigned long long sum = 0;
        for (int i = 0; i < 512; ++i)
            sum += img[at + size_t(i)];
        std::snprintf(reinterpret_cast<char*>(img.data()) + at + 148, 8, "%06llo ",
                      sum);
    }
    {
        /* TTF at 167936 + ICO at 172032 */
        size_t at = 167936;
        std::memcpy(img.data() + at, "\x00\x01\x00\x00", 4);
        img[at + 5] = 1; /* BE numTables 1 */
        std::memcpy(img.data() + at + 12, "cmap", 4);
        img[at + 22] = 28; /* BE offset 28 */
        img[at + 26] = 64; /* BE length 64 */
        at = 172032;
        t16(img, at + 2, 1); /* ICO type */
        t16(img, at + 4, 1); /* count */
        img[at + 8] = 16; /* color count */
        t16(img, at + 10, 1); /* planes */
        t16(img, at + 12, 32); /* bpp */
        t32(img, at + 14, 64); /* bytes */
        t32(img, at + 18, 22); /* offset */
    }
    {
        /* WMF-std at 176128 + AU at 180224 */
        size_t at = 176128;
        std::memcpy(img.data() + at, "\x01\x00\x09\x00", 4);
        t16(img, at + 4, 0x0300);
        t32(img, at + 6, 64); /* size words */
        t16(img, at + 10, 4); /* objects */
        t32(img, at + 12, 10); /* max record */
        at = 180224;
        std::memcpy(img.data() + at, ".snd", 4);
        img[at + 7] = 24; /* BE data offset 24 */
        img[at + 11] = 256; /* BE size */
        img[at + 15] = 3; /* BE encoding */
        img[at + 19] = 172; /* BE rate 44100 = 0xAC44 */
        img[at + 20] = 0x44;
        img[at + 23] = 2; /* BE channels */
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
