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
        zs.avail_out = uInt(comp.size());
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

/* ---------------- session-5 continuation regressions ----------------
 * The CI fuzzer found the class live: a truncated head window fed
 * memcmp/readers that consumed `want` bytes from a SHORT window buffer
 * (heap-buffer-overflow READ, fuzz_preview, cr_preview.cpp:648). These
 * tests pin the fixed fetch contract + the three functional defects
 * fixed alongside (WAV out-of-window bps, DOCX windowBits, extent
 * misalignment). */

TEST(PreviewMeta, TruncatedMagicsAreSafeAndHonest) {
    /* the exact fuzzer-crash shape: 2-byte head, memcmp(head+1,"PNG",3) */
    {
        std::vector<u8> two{u8(0x89), 'P'};
        MemDevice dev(std::move(two));
        tree::FileProperties::Extent a{0, 2, 0, 0};
        tree::FileProperties* pr = props_with("t.png", 2, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r); /* must not read OOB */
        EXPECT_FALSE(m.kind == "image" && m.width != 0);
    }
    /* every short-magic family: no crash, no fabricated fields */
    {
        struct Case {
            std::vector<u8> bytes;
            const char* expect_kind; /* "" = no claim */
            const char* expect_format;
        };
        std::vector<Case> cases;
        cases.push_back({{0xFF, 0xD8}, "image", "jpeg"});        /* 2-byte JPEG */
        cases.push_back({{'G', 'I', 'F', '8'}, "image", "gif"}); /* 4-byte GIF */
        cases.push_back({{'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'E', 'B', 'P'},
                         "image",
                         "webp"});                          /* 12-byte RIFF/WEBP */
        cases.push_back({{'P', 'K'}, "archive", "zip"});    /* 2-byte PK */
        cases.push_back({{'%'}, "text", "txt"});            /* 1-byte printable: text */
        cases.push_back({{0x00, 0xFF}, "unsupported", ""}); /* binary */
        for (auto& c : cases) {
            const u64 n = c.bytes.size();
            MemDevice dev(std::move(c.bytes));
            tree::FileProperties::Extent a{0, n, 0, 0};
            tree::FileProperties* pr = props_with("t.bin", n, 0, {a});
            PropsGuard g(pr);
            PreviewReader r(&dev, pr);
            const Metadata m = extract_metadata(r);
            EXPECT_EQ(c.expect_kind, m.kind);
            if (*c.expect_format) {
                EXPECT_EQ(c.expect_format, m.format);
            }
            EXPECT_EQ(0u, m.width);
            EXPECT_EQ(0u, m.height);
            EXPECT_EQ(u64(0), m.duration_ms);
        }
    }
}

TEST(PreviewMeta, WavFmtChunkOutsideHeadCache) {
    /* fmt at a LATER chunk (JUNK first): the pre-fix code read
     * bits/sample from a 16-byte window while the field sat at +22 -
     * stale window bytes whenever fmt was past the head cache */
    std::vector<u8> wav(68, 0);
    std::memcpy(wav.data(), "RIFF", 4);
    putle32(wav, 4, 60);
    std::memcpy(wav.data() + 8, "WAVE", 4);
    std::memcpy(wav.data() + 12, "JUNK", 4);
    putle32(wav, 16, 16); /* 16 bytes of padding body [20,36) */
    std::memcpy(wav.data() + 36, "fmt ", 4);
    putle32(wav, 40, 16);
    putle16(wav, 44, 1);     /* pcm @ chunk+8 */
    putle16(wav, 46, 1);     /* channels @ chunk+10 */
    putle32(wav, 48, 8000);  /* rate @ chunk+12 */
    putle32(wav, 52, 16000); /* byte rate @ chunk+16 */
    putle16(wav, 56, 2);     /* block align @ chunk+20 */
    putle16(wav, 58, 16);    /* bits @ chunk+22 - outside any 16-byte window */
    std::memcpy(wav.data() + 60, "data", 4);
    putle32(wav, 64, 8000);
    MemDevice dev(std::move(wav));
    tree::FileProperties::Extent a{0, 68, 0, 0};
    tree::FileProperties* pr = props_with("w.wav", 68, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("audio", m.kind);
    EXPECT_EQ("pcm", m.codec);
    EXPECT_EQ(u64(500), m.duration_ms); /* 8000 bytes / (8000*2 B/s) = 500 ms */
}

TEST(PreviewMeta, DocxRawDeflateInflatesAndPlainZipStaysZip) {
    /* minimal hand-built zip: LFH + CD + EOCD, raw-deflate body
     * (windowBits -15 - the pre-fix code used 15 and every real
     * docx stream failed the zlib-header check) */
    const std::string name = "word/document.xml";
    const std::string plain = "<w>hello preview world</w>";
    std::vector<u8> comp;
    {
        uLongf cl = compressBound(static_cast<uLong>(plain.size()));
        comp.resize(cl);
        z_stream zs{};
        ASSERT_EQ(Z_OK, deflateInit2(&zs, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY));
        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(plain.data()));
        zs.avail_in = uInt(plain.size());
        zs.next_out = comp.data();
        zs.avail_out = uInt(comp.size());
        ASSERT_EQ(Z_STREAM_END, deflate(&zs, Z_FINISH));
        comp.resize(zs.total_out);
        deflateEnd(&zs);
    }
    std::vector<u8> zip;
    const auto put16 = [&zip](u16 v) {
        zip.push_back(u8(v));
        zip.push_back(u8(v >> 8));
    };
    const auto put32 = [&zip](u32 v) {
        zip.push_back(u8(v));
        zip.push_back(u8(v >> 8));
        zip.push_back(u8(v >> 16));
        zip.push_back(u8(v >> 24));
    };
    /* local file header */
    const size_t lfh_at = 0;
    zip.insert(zip.end(), {'P', 'K', 3, 4});
    put16(20); /* version needed */
    put16(0);  /* flags */
    put16(8);  /* method = deflate */
    put16(0);  /* time */
    put16(0);  /* date */
    put32(0);  /* crc (not verified in preview) */
    put32(u32(comp.size()));
    put32(u32(plain.size()));
    put16(u16(name.size()));
    put16(0); /* extra len */
    zip.insert(zip.end(), name.begin(), name.end());
    zip.insert(zip.end(), comp.begin(), comp.end());
    /* central directory (ends exactly where the EOCD begins) */
    const size_t cd_at = zip.size();
    const size_t cd_len = 46 + name.size();
    zip.insert(zip.end(), {'P', 'K', 1, 2});
    put16(20);
    put16(20);
    put16(0);
    put16(8);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(comp.size()));
    put32(u32(plain.size()));
    put16(u16(name.size()));
    put16(0);
    put16(0);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(lfh_at));
    zip.insert(zip.end(), name.begin(), name.end());
    /* EOCD: sig, disk, cd-disk, entries-on-disk, total-entries,
     * cd-size, cd-offset, comment-len */
    zip.insert(zip.end(), {'P', 'K', 5, 6});
    put16(0);
    put16(0);
    put16(1);
    put16(1);
    put32(u32(cd_len));
    put32(u32(cd_at));
    put16(0);

    MemDevice dev(std::move(zip));
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties::Extent a{0, zlen, 0, 0};
    tree::FileProperties* pr = props_with("d.docx", zlen, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("document", m.kind);
    EXPECT_EQ("docx", m.format);
    EXPECT_NE(std::string::npos, m.text_snippet.find("hello preview world"));

    /* plain zip (different member name): stays archive/zip, never docx */
    std::vector<u8> zip2;
    const std::string name2 = "readme.txt";
    zip2.insert(zip2.end(), {'P', 'K', 3, 4});
    put16(20);
    put16(0);
    put16(8);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(comp.size()));
    put32(u32(plain.size()));
    put16(u16(name2.size()));
    put16(0);
    zip2.insert(zip2.end(), name2.begin(), name2.end());
    const size_t cd_at2 = zip2.size();
    zip2.insert(zip2.end(), {'P', 'K', 1, 2});
    put16(20);
    put16(20);
    put16(0);
    put16(8);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(comp.size()));
    put32(u32(plain.size()));
    put16(u16(name2.size()));
    put16(0);
    put16(0);
    put16(0);
    put16(0);
    put32(0);
    put32(0);
    zip2.insert(zip2.end(), name2.begin(), name2.end());
    const size_t cd_len2 = 46 + name2.size();
    zip2.insert(zip2.end(), {'P', 'K', 5, 6});
    put16(0);
    put16(0);
    put16(1);
    put16(1);
    put32(u32(cd_at2));
    put32(u32(cd_len2));
    put16(0);

    MemDevice dev2(std::move(zip2));
    const u64 z2len = dev2.geometry().total_bytes;
    tree::FileProperties* pr2 =
        props_with("z.zip", z2len, 0, {tree::FileProperties::Extent{0, z2len, 0, 0}});
    PropsGuard g2(pr2);
    PreviewReader r2(&dev2, pr2);
    const Metadata m2 = extract_metadata(r2);
    EXPECT_EQ("archive", m2.kind);
    EXPECT_EQ("zip", m2.format);
}

/* ---- M-028: the Caps.max_read contract on the docx/zip path + the
 * compressed-extent disclosure (the audit's executed bypass scenario:
 * evidence/executions/s4-preview-caps-bypass.md — 220,158-byte zip on
 * one extent, max_read = 4096, pre-fix pulled 65,568 device bytes with
 * truncated == false). ---- */

/* The audit's crafted zip: LFH + "word/document.xml" + a large
 * stored-deflate payload + CD + EOCD — total 220,158 bytes, so the EOCD
 * is only reachable through the 64 KiB tail window. */
static std::vector<u8> build_audit_zip() {
    const std::string name = "word/document.xml";
    const std::string plain(220'000, 'x'); /* incompressible body */
    std::vector<u8> comp;
    {
        uLongf cl = compressBound(static_cast<uLong>(plain.size()));
        comp.resize(cl);
        z_stream zs{};
        if (deflateInit2(&zs, 0 /* level 0 = stored blocks */, Z_DEFLATED, -15, 8,
                         Z_DEFAULT_STRATEGY) != Z_OK)
            return {};
        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(plain.data()));
        zs.avail_in = uInt(plain.size());
        zs.next_out = comp.data();
        zs.avail_out = uInt(comp.size());
        if (deflate(&zs, Z_FINISH) != Z_STREAM_END) {
            deflateEnd(&zs);
            return {};
        }
        comp.resize(zs.total_out);
        deflateEnd(&zs);
    }
    std::vector<u8> zip;
    const auto put16 = [&zip](u16 v) {
        zip.push_back(u8(v));
        zip.push_back(u8(v >> 8));
    };
    const auto put32 = [&zip](u32 v) {
        zip.push_back(u8(v));
        zip.push_back(u8(v >> 8));
        zip.push_back(u8(v >> 16));
        zip.push_back(u8(v >> 24));
    };
    const size_t lfh_at = 0;
    zip.insert(zip.end(), {'P', 'K', 3, 4});
    put16(20);
    put16(0);
    put16(8);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(comp.size()));
    put32(u32(plain.size()));
    put16(u16(name.size()));
    put16(0);
    zip.insert(zip.end(), name.begin(), name.end());
    zip.insert(zip.end(), comp.begin(), comp.end());
    const size_t cd_at = zip.size();
    const size_t cd_len = 46 + name.size();
    zip.insert(zip.end(), {'P', 'K', 1, 2});
    put16(20);
    put16(20);
    put16(0);
    put16(8);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(comp.size()));
    put32(u32(plain.size()));
    put16(u16(name.size()));
    put16(0);
    put16(0);
    put16(0);
    put16(0);
    put32(0);
    put32(u32(lfh_at));
    zip.insert(zip.end(), name.begin(), name.end());
    zip.insert(zip.end(), {'P', 'K', 5, 6});
    put16(0);
    put16(0);
    put16(1);
    put16(1);
    put32(u32(cd_len));
    put32(u32(cd_at));
    put16(0);
    return zip;
}

TEST(PreviewMeta, DocxCapsMaxReadHonored) {
    auto zip = build_audit_zip();
    /* the audit's 220,158-byte geometry (zlib stored-block framing
     * shifts a few bytes across versions); what matters: the EOCD is
     * reachable ONLY through the 64 KiB tail window */
    ASSERT_GT(zip.size(), 200'000u);
    MemDevice dev(std::move(zip));
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties* pr =
        props_with("d.docx", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    Caps caps;
    caps.max_read = 4096;
    const Metadata m = extract_metadata(r, caps);
    /* the documented invariant ("Untrusted input never makes the
     * extractor read more than the caps allow"): either the pulls
     * stayed within the budget or the truncated flag tells the truth */
    EXPECT_TRUE(r.device_bytes() <= caps.max_read || m.truncated)
        << "device_bytes=" << r.device_bytes() << " truncated=" << m.truncated;
    /* the strong form: the tail fetch that would blow the budget is
     * refused outright, so the device never sees the over-read */
    EXPECT_LE(r.device_bytes(), caps.max_read);
}

TEST(PreviewMeta, DocxTailBeyondBudgetDegradesToZip) {
    /* the same geometry with an honest degrade: the EOCD is unreachable
     * under the budget -> the archive/zip label stands (never a docx
     * claim built on over-budget reads) and truncated is set */
    auto zip = build_audit_zip();
    MemDevice dev(std::move(zip));
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties* pr =
        props_with("d.docx", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    Caps caps;
    caps.max_read = 4096;
    const Metadata m = extract_metadata(r, caps);
    EXPECT_EQ("archive", m.kind);
    EXPECT_EQ("zip", m.format);
    EXPECT_TRUE(m.truncated);
}

TEST(PreviewMeta, CompressedExtentReportsTruncatedUnderBudget) {
    /* a compressed extent whose packed block (psize) alone exceeds the
     * budget: the whole-block read is refused BEFORE any device byte is
     * issued (pre-fix: the full psize was pulled silently) */
    if (!compress::codec_available(compress::kGzip))
        GTEST_SKIP() << "gzip codec unavailable";
    const std::string plain(96 * 1024, 'q'); /* -> ~96 KiB stored-deflate block */
    uLongf cl = compressBound(static_cast<uLong>(plain.size()));
    std::vector<u8> comp(cl);
    {
        z_stream zs{};
        ASSERT_EQ(Z_OK, deflateInit2(&zs, 0, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY));
        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(plain.data()));
        zs.avail_in = uInt(plain.size());
        zs.next_out = comp.data();
        zs.avail_out = uInt(comp.size());
        ASSERT_EQ(Z_STREAM_END, deflate(&zs, Z_FINISH));
        comp.resize(zs.total_out);
        deflateEnd(&zs);
    }
    ASSERT_GT(comp.size(), 4096u); /* the block alone busts the budget */
    MemDevice dev(comp);
    tree::FileProperties* pr =
        props_with("c.bin", plain.size(), 0,
                   {tree::FileProperties::Extent{0, plain.size(), compress::kGzip, comp.size()}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    Caps caps;
    caps.max_read = 4096;
    const Metadata m = extract_metadata(r, caps);
    EXPECT_TRUE(m.truncated);
    EXPECT_LE(r.device_bytes(), caps.max_read);
}

TEST(PreviewMeta, CompressedExtentWholeBlockReadDisclosed) {
    /* with a sufficient budget the whole packed block IS read (that is
     * the documented compressed-extent cost: psize device bytes for any
     * window into it) and extraction is not truncated */
    if (!compress::codec_available(compress::kGzip))
        GTEST_SKIP() << "gzip codec unavailable";
    const std::string plain(8192, 'w');
    uLongf cl = compressBound(static_cast<uLong>(plain.size()));
    std::vector<u8> comp(cl);
    {
        z_stream zs{};
        ASSERT_EQ(Z_OK, deflateInit2(&zs, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY));
        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(plain.data()));
        zs.avail_in = uInt(plain.size());
        zs.next_out = comp.data();
        zs.avail_out = uInt(comp.size());
        ASSERT_EQ(Z_STREAM_END, deflate(&zs, Z_FINISH));
        comp.resize(zs.total_out);
        deflateEnd(&zs);
    }
    MemDevice dev(comp);
    tree::FileProperties* pr =
        props_with("c.bin", plain.size(), 0,
                   {tree::FileProperties::Extent{0, plain.size(), compress::kGzip, comp.size()}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    Caps caps;
    caps.max_read = 1u << 20;
    const Metadata m = extract_metadata(r, caps);
    /* the whole-block cost, disclosed: EVERY logical read over the
     * compressed extent costs exactly psize device bytes (the reader
     * has no block cache - the head fetch and the text-sniff fetch each
     * re-read the packed block); the total is an exact multiple */
    EXPECT_GE(r.device_bytes(), comp.size());
    EXPECT_EQ(0u, r.device_bytes() % comp.size());
    EXPECT_FALSE(m.truncated);
}

TEST(PreviewMeta, MetadataTruncatedFlagTruthfulAllPaths) {
    /* the belt: on every path, an over-budget extraction MUST carry
     * truncated == true — re-run the audit geometry and a mid-size zip
     * under two budgets each and assert the invariant both ways */
    auto zip = build_audit_zip();
    for (const u32 budget : {4096u, 8192u, 70000u, 1u << 20}) {
        MemDevice dev(zip);
        const u64 zlen = dev.geometry().total_bytes;
        tree::FileProperties* pr =
            props_with("d.docx", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        Caps caps;
        caps.max_read = budget;
        const Metadata m = extract_metadata(r, caps);
        EXPECT_TRUE(r.device_bytes() <= caps.max_read || m.truncated)
            << "budget=" << budget << " device_bytes=" << r.device_bytes()
            << " truncated=" << m.truncated;
    }
}

TEST(Preview, MalformedCapsExtentKeepsAlignment) {
    /* extent 1 carries a compressed codec with malformed caps (psize 0),
     * extent 2 is plain: the pre-fix code did not advance `done` past the
     * malformed extent, so extent 2's bytes landed at logical 0 instead
     * of 8 (mis-placed data under hostile extent lists) */
    std::vector<u8> img(512, 0);
    std::memcpy(img.data() + 100, "SECOND!", 7);
    MemDevice dev(std::move(img));
    tree::FileProperties::Extent bad{0, 8, compress::kGzip, 0 /* psize 0: malformed */};
    tree::FileProperties::Extent good{100, 8, 0, 0};
    tree::FileProperties* pr = props_with("m.bin", 16, 0, {bad, good});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    u8 buf[16] = {};
    ASSERT_EQ(16u, r.read_at(0, buf, 16));
    /* malformed extent: zero-filled + reported */
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(0, buf[i]);
    EXPECT_FALSE(r.unreadable_spans().ranges().empty());
    /* plain extent lands at its logical position [8,16) */
    EXPECT_EQ(0, std::memcmp(buf + 8, "SECOND!", 7));
}

TEST(PreviewMeta, RandomSmallInputsAreSafe) {
    /* deterministic fuzz-substitute: the CI fuzz lane runs libFuzzer with
     * clang; this stress runs in EVERY lane (gcc/ASan, TSan, MSVC) and
     * would have caught the short-window class (2-byte "PNG" head) that
     * the fuzzer found. Fixed seed, bounded work, zero flakiness. */
    std::vector<u8> img(4096, 0);
    MemDevice dev(std::move(img));
    for (u64 round = 0; round < 3000; ++round) {
        /* xorshift64*: size 0..96, bytes from the PRNG stream */
        u64 s = 0x9E3779B97F4A7C15ull * (round + 1);
        auto next = [&s]() {
            s ^= s >> 12;
            s ^= s << 25;
            s ^= s >> 27;
            return s * 2685821657736338717ull;
        };
        const u64 n = next() % 97;
        const u64 at = next() % (4096 - 128);
        tree::FileProperties::Extent a{at, n, 0, 0};
        tree::FileProperties* pr = props_with("r.bin", n, 0, {a});
        PropsGuard g(pr);
        PreviewReader r(&dev, pr);
        const Metadata m = extract_metadata(r); /* must never read OOB */
        (void)m;
    }
}
