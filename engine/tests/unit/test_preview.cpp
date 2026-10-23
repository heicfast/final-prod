/* test_preview.cpp - G-10 acceptance: the bounded preview reader and the
 * capped metadata extractors. Extent-accurate reads (resident prefix,
 * sparse gaps, EOF), bounded-read proof against a lying 1 TiB node,
 * zero-fill + truthful unreadable_spans on device failures, and metadata
 * ground truth for JPEG (dims + EXIF), PNG, GIF, BMP, WAV, MP4, PDF,
 * plain text, and unsupported binary.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/preview/cr_preview.h"
#include "cr/compress/cr_decomp.h"
#include <cstring>
#include <zlib.h>
#include <thread>

using namespace cr;
using namespace cr::preview;

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
        ++reads_;
        bytes_ += len;
        if (fail_all_)
            return ERR_READ_FAILED;
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 reads() const { return reads_; }
    u64 bytes() const { return bytes_; }
    void fail_all(bool on) { fail_all_ = on; }

private:
    std::vector<u8> data_;
    Geometry geom_;
    std::atomic<u64> reads_{0};
    std::atomic<u64> bytes_{0};
    std::atomic<bool> fail_all_{false};
};

tree::FileProperties* props_with(const char* name, u64 logical, u64 res_len,
                                 std::vector<tree::FileProperties::Extent> exts) {
    auto* p = new tree::FileProperties();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->logical_size = logical;
    if (res_len)
        p->resident_data.assign(size_t(res_len), 0xAA);
    for (auto& e : exts)
        p->extents.push_back(e);
    return p;
}

struct PropsGuard {
    tree::FileProperties* p;
    explicit PropsGuard(tree::FileProperties* q) : p(q) {}
    ~PropsGuard() { p->release(); }
};

void putbe16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x >> 8);
    v[at + 1] = u8(x);
}
void putbe32(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x >> 24);
    v[at + 1] = u8(x >> 16);
    v[at + 2] = u8(x >> 8);
    v[at + 3] = u8(x);
}
void putle16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void putle32(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}

} // namespace

/* ---------------- reader ---------------- */

TEST(Preview, ExtentAccurateReadsWithSparseTailAndEof) {
    /* extents are CONTIGUOUS after the resident prefix (the writer's
     * documented stream contract): resident "RR" [0,2), extent A "AAAA"
     * [2,6), extent B "BBBB" [6,10), sparse zero tail [10,26) */
    std::vector<u8> img(64, 0);
    std::memcpy(img.data() + 40, "AAAA", 4);
    std::memcpy(img.data() + 50, "BBBB", 4);
    MemDevice dev(std::move(img));
    tree::FileProperties::Extent a{40, 4, 0, 0}, b{50, 4, 0, 0};
    tree::FileProperties* pr = props_with("x.bin", 26, 2, {a, b});
    PropsGuard g(pr);
    pr->resident_data[0] = 'R';
    pr->resident_data[1] = 'R';

    PreviewReader r(&dev, pr);
    EXPECT_EQ(u64(26), r.size());
    u8 buf[64];
    ASSERT_EQ(26u, r.read_at(0, buf, 64)); /* clamped short at EOF */
    EXPECT_EQ(0, std::memcmp(buf, "RR", 2));
    EXPECT_EQ(0, std::memcmp(buf + 2, "AAAA", 4));
    EXPECT_EQ(0, std::memcmp(buf + 6, "BBBB", 4));
    for (u64 i = 10; i < 26; ++i)
        EXPECT_EQ(0, buf[i]) << "sparse tail byte " << i; /* zero tail */

    EXPECT_EQ(0u, r.read_at(26, buf, 8));  /* at EOF */
    EXPECT_EQ(4u, r.read_at(22, buf, 64)); /* tail short read */
    EXPECT_EQ(0, std::memcmp(buf, std::vector<u8>(2, 0).data(), 2));
    EXPECT_TRUE(r.unreadable_spans().ranges().empty());
}

TEST(Preview, BoundedReadCannotBeTricked) {
    /* a node CLAIMING 1 TiB against a 1 KiB extent list: the reader must
     * never read more from the device than the caller asked for */
    std::vector<u8> img(4096, 0);
    img[0] = 1;
    MemDevice dev(std::move(img));
    tree::FileProperties::Extent a{0, 1024, 0, 0};
    tree::FileProperties* pr = props_with("liar.bin", 1ull << 40, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);

    u8 buf[8192];
    const u32 got = r.read_at(0, buf, 8192);
    EXPECT_EQ(8192u, got);             /* filled from the extent + zeros */
    EXPECT_LE(dev.bytes(), u64(8192)); /* NEVER more than requested */
    EXPECT_LE(dev.reads(), u64(4));
    /* far offset: nothing to read, zero device traffic */
    const u64 before = dev.bytes();
    EXPECT_EQ(4096u, r.read_at((1ull << 39), buf, 4096));
    EXPECT_EQ(before, dev.bytes());
}

TEST(Preview, ReadFailuresZeroFilledAndReported) {
    std::vector<u8> img(64, 0);
    MemDevice dev(std::move(img));
    tree::FileProperties::Extent a{0, 16, 0, 0};
    tree::FileProperties* pr = props_with("dead.bin", 16, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    dev.fail_all(true);
    u8 buf[16];
    ASSERT_EQ(16u, r.read_at(0, buf, 16));
    for (u8 c : buf)
        EXPECT_EQ(0, c); /* zero-filled */
    ASSERT_EQ(1u, r.unreadable_spans().ranges().size());
    EXPECT_EQ(u64(0), r.unreadable_spans().ranges()[0].offset);
    EXPECT_EQ(u64(16), r.unreadable_spans().ranges()[0].length);
}

TEST(Preview, CompressedExtentDecodes) {
    /* gzip-1 codec via zlib raw deflate (kGzip = 3): 16 'Z' bytes */
    if (!compress::codec_available(compress::kGzip))
        GTEST_SKIP() << "gzip codec unavailable";
    std::vector<u8> plain(16, 'Z');
    /* raw deflate with zlib's compress2 + window bits -15 */
    uLongf comp_size = compressBound(16);
    std::vector<u8> comp(comp_size);
    {
        z_stream zs{};
        deflateInit2(&zs, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
        zs.next_in = plain.data();
        zs.avail_in = uInt(plain.size());
        zs.next_out = comp.data();
        zs.avail_out = uLongf(comp.size());
        deflate(&zs, Z_FINISH);
        comp.resize(zs.total_out);
        deflateEnd(&zs);
    }
    std::vector<u8> img(comp);
    img.resize(1024, 0);
    MemDevice dev(std::move(img));
    tree::FileProperties::Extent a{0, 16, compress::kGzip, comp.size()};
    tree::FileProperties* pr = props_with("z.bin", 16, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    u8 buf[16];
    ASSERT_EQ(16u, r.read_at(0, buf, 16));
    EXPECT_EQ(0, std::memcmp(buf, plain.data(), 16));
    EXPECT_TRUE(r.unreadable_spans().ranges().empty());
}

TEST(Preview, ConcurrentIndependentReaders) {
    /* two reader instances over one device (TSan build proves it) */
    std::vector<u8> img(64 * 1024, 0);
    for (size_t i = 0; i < img.size(); ++i)
        img[i] = u8(i);
    MemDevice dev(std::move(img));
    tree::FileProperties::Extent a{0, 64 * 1024, 0, 0};
    tree::FileProperties* pr = props_with("c.bin", 64 * 1024, 0, {a});
    PropsGuard g(pr);
    PreviewReader r1(&dev, pr), r2(&dev, pr);
    std::atomic<bool> ok{true};
    std::thread t1([&] {
        u8 buf[4096];
        for (u64 off = 0; off < 4096; off += 512)
            if (r1.read_at(off, buf, 512) != 512 || buf[0] != u8(off))
                ok = false;
    });
    std::thread t2([&] {
        u8 buf[4096];
        for (u64 off = 8192; off < 12288; off += 512)
            if (r2.read_at(off, buf, 512) != 512 || buf[0] != u8(off))
                ok = false;
    });
    t1.join();
    t2.join();
    EXPECT_TRUE(ok.load());
}

/* ---------------- metadata extractors ---------------- */

TEST(PreviewMeta, JpegDimensionsAndExif) {
    /* minimal JPEG: SOI + APP0(skip) + APP1/Exif(orientation+datetime) +
     * SOF0(64x48) + SOS */
    std::vector<u8> jpg;
    auto push = [&](const void* d, size_t n) {
        const u8* p = static_cast<const u8*>(d);
        jpg.insert(jpg.end(), p, p + n);
    };
    const u8 soi[] = {0xFF, 0xD8};
    push(soi, 2);
    /* APP0: len 16 */
    const u8 app0[] = {0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0, 1, 1, 0, 0, 1, 0, 1, 0, 0};
    push(app0, sizeof(app0));
    /* APP1/Exif: TIFF (II, IFD0 with Orientation=6, DateTime=20 chars) */
    std::vector<u8> tiff;
    tiff.insert(tiff.end(), {'I', 'I', 42, 0, 8, 0, 0, 0}); /* II, 42, IFD0@8 */
    tiff.push_back(2);                                      /* 2 entries */
    tiff.push_back(0);
    /* entry: tag 274, type 3, count 1, value 6 */
    const u8 e1[] = {0x12, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
    tiff.insert(tiff.end(), e1, e1 + 12);
    /* entry: tag 306, type 2, count 20, offset -> data at 8+2+12+12+2=36 */
    const u8 e2[] = {0x32, 0x01, 0x02, 0x00, 20, 0, 0, 0, 36, 0, 0, 0, 0x00 /*pad*/};
    tiff.insert(tiff.end(), e2, e2 + 12);
    tiff.push_back(0);
    tiff.push_back(0); /* next-IFD 0 */
    const char dt[] = "2024:01:02 03:04:05";
    tiff.insert(tiff.end(), dt, dt + 19);
    tiff.push_back(0);
    while (tiff.size() % 2)
        tiff.push_back(0);
    /* APP1 header: FF E1 len "Exif\0\0" + tiff */
    std::vector<u8> app1;
    app1.resize(4);
    app1[0] = 0xFF;
    app1[1] = 0xE1;
    putbe16(app1, 2, u16(2 + 6 + tiff.size())); /* segment length field
                                        includes itself */
    app1.insert(app1.end(), {'E', 'x', 'i', 'f', 0, 0});
    app1.insert(app1.end(), tiff.begin(), tiff.end());
    push(app1.data(), app1.size());
    /* SOF0: len 17, 8bpp, 48x64, 1 component */
    const u8 sof[] = {0xFF, 0xC0, 0x00, 0x11, 8, 0, 48, 0, 64, 1, 1, 0x22, 0, 0, 0, 0, 0, 0, 0};
    push(sof, sizeof(sof));
    const u8 sos[] = {0xFF, 0xDA, 0x00, 0x0C, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    push(sos, sizeof(sos));

    const u64 jpg_len = jpg.size();
    MemDevice dev(std::move(jpg));
    tree::FileProperties::Extent a{0, jpg_len, 0, 0};
    tree::FileProperties* pr = props_with("photo.jpg", jpg_len, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("image", m.kind);
    EXPECT_EQ("jpeg", m.format);
    EXPECT_EQ(64u, m.width);
    EXPECT_EQ(48u, m.height);
    EXPECT_EQ("6", m.orientation);
    EXPECT_EQ(std::string("2024:01:02 03:04:05"), m.datetime);
}

TEST(PreviewMeta, PngGifBmpDimensions) {
    /* PNG 800x600 */
    {
        std::vector<u8> png(64, 0);
        png[0] = 0x89;
        std::memcpy(png.data() + 1, "PNG\r\n\x1a\n", 7);
        putbe32(png, 8, 13);
        std::memcpy(png.data() + 12, "IHDR", 4);
        putbe32(png, 16, 800);
        putbe32(png, 20, 600);
        MemDevice dev(std::move(png));
        tree::FileProperties::Extent a{0, 64, 0, 0};
        tree::FileProperties* pr = props_with("i.png", 64, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("png", m.format);
        EXPECT_EQ("image", m.kind);
        EXPECT_EQ(800u, m.width);
        EXPECT_EQ(600u, m.height);
    }
    /* GIF 320x240 */
    {
        std::vector<u8> gif(32, 0);
        std::memcpy(gif.data(), "GIF89a", 6);
        putle16(gif, 6, 320);
        putle16(gif, 8, 240);
        MemDevice dev(std::move(gif));
        tree::FileProperties::Extent a{0, 32, 0, 0};
        tree::FileProperties* pr = props_with("a.gif", 32, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("gif", m.format);
        EXPECT_EQ(320u, m.width);
        EXPECT_EQ(240u, m.height);
    }
    /* BMP 1024x768 */
    {
        std::vector<u8> bmp(64, 0);
        bmp[0] = 'B';
        bmp[1] = 'M';
        putle32(bmp, 18, 1024);
        putle32(bmp, 22, 768);
        MemDevice dev(std::move(bmp));
        tree::FileProperties::Extent a{0, 64, 0, 0};
        tree::FileProperties* pr = props_with("b.bmp", 64, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("bmp", m.format);
        EXPECT_EQ(1024u, m.width);
        EXPECT_EQ(768u, m.height);
    }
}

TEST(PreviewMeta, WavDurationAndMp4Brand) {
    /* WAV: 8 kHz 16-bit mono, 8000 data bytes -> 500 ms */
    {
        std::vector<u8> wav(64, 0);
        std::memcpy(wav.data(), "RIFF", 4);
        putle32(wav, 4, 36 + 8000);
        std::memcpy(wav.data() + 8, "WAVE", 4);
        std::memcpy(wav.data() + 12, "fmt ", 4);
        putle32(wav, 16, 16);
        putle16(wav, 20, 1);     /* pcm */
        putle16(wav, 22, 1);     /* channels */
        putle32(wav, 24, 8000);  /* rate */
        putle32(wav, 28, 16000); /* byte rate */
        putle16(wav, 32, 2);     /* block align */
        putle16(wav, 34, 16);    /* bits */
        std::memcpy(wav.data() + 36, "data", 4);
        putle32(wav, 40, 8000);
        MemDevice dev(std::move(wav));
        tree::FileProperties::Extent a{0, 64, 0, 0};
        tree::FileProperties* pr = props_with("s.wav", 64, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("audio", m.kind);
        EXPECT_EQ("wav", m.format);
        EXPECT_EQ("pcm", m.codec);
        EXPECT_EQ(u64(500), m.duration_ms);
    }
    /* MP4: ftyp(isom) + moov/mvhd v0 (timescale 1000, dur 2500 -> 2500 ms) */
    {
        std::vector<u8> mp4(128, 0);
        putbe32(mp4, 0, 20);
        std::memcpy(mp4.data() + 4, "ftyp", 4);
        std::memcpy(mp4.data() + 8, "isom", 4);
        putbe32(mp4, 12, 0);
        std::memcpy(mp4.data() + 16, "mp41", 4);
        putbe32(mp4, 20, 8 + 8 + 100); /* moov size */
        std::memcpy(mp4.data() + 24, "moov", 4);
        putbe32(mp4, 28, 100); /* mvhd size */
        std::memcpy(mp4.data() + 32, "mvhd", 4);
        mp4[36] = 0;            /* version 0 */
        putbe32(mp4, 48, 1000); /* timescale @ mvhd+20 */
        putbe32(mp4, 52, 2500); /* duration @ mvhd+24 */
        const u64 mp4_len = mp4.size();
        MemDevice dev(std::move(mp4));
        tree::FileProperties::Extent a{0, mp4_len, 0, 0};
        tree::FileProperties* pr = props_with("v.mp4", mp4_len, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("video", m.kind);
        EXPECT_EQ("mp4", m.format);
        EXPECT_EQ("isom", m.codec);
        EXPECT_EQ(u64(2500), m.duration_ms);
    }
}

TEST(PreviewMeta, PdfTextAndPlainAndUnsupported) {
    /* PDF header + text */
    {
        std::vector<u8> pdf(128, 0);
        std::memcpy(pdf.data(), "%PDF-1.4 hello world", 21);
        MemDevice dev(std::move(pdf));
        tree::FileProperties::Extent a{0, 128, 0, 0};
        tree::FileProperties* pr = props_with("d.pdf", 128, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("document", m.kind);
        EXPECT_EQ("pdf", m.format);
        EXPECT_NE(std::string::npos, m.text_snippet.find("hello world"));
    }
    /* plain text */
    {
        std::vector<u8> txt(64, ' ');
        std::memcpy(txt.data(), "just a text file", 17);
        MemDevice dev(std::move(txt));
        tree::FileProperties::Extent a{0, 64, 0, 0};
        tree::FileProperties* pr = props_with("n.txt", 64, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("text", m.kind);
        EXPECT_EQ("txt", m.format);
        EXPECT_NE(std::string::npos, m.text_snippet.find("just a text"));
    }
    /* binary: unsupported */
    {
        std::vector<u8> bin(64, 0);
        bin[0] = 0x00;
        bin[1] = 0xFF;
        bin[2] = 0xFE;
        bin[3] = 0x99;
        MemDevice dev(std::move(bin));
        tree::FileProperties::Extent a{0, 64, 0, 0};
        tree::FileProperties* pr = props_with("x.dat", 64, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("unsupported", m.kind);
        EXPECT_TRUE(m.text_snippet.empty());
    }
    /* hostile EXIF: 0xFFFF entries - bounded, no crash */
    {
        std::vector<u8> jpg;
        jpg.push_back(0xFF);
        jpg.push_back(0xD8);
        jpg.push_back(0xFF);
        jpg.push_back(0xE1);
        jpg.resize(6); /* room for the length field */
        putbe16(jpg, 4, 40);
        jpg.insert(jpg.end(), {'E', 'x', 'i', 'f', 0, 0});
        jpg.insert(jpg.end(), {'I', 'I', 42, 0, 8, 0, 0, 0});
        jpg.push_back(0xFF);
        jpg.push_back(0xFF);
        jpg.resize(64, 0);
        MemDevice dev(std::move(jpg));
        tree::FileProperties::Extent a{0, 64, 0, 0};
        tree::FileProperties* pr = props_with("h.jpg", 64, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r);
        EXPECT_EQ("image", m.kind); /* dims absent, kind still honest */
        EXPECT_EQ(0u, m.width);
    }
}
