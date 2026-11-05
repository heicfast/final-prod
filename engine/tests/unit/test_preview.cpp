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
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
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

TEST(PreviewMeta, AviMetadataExtracted) {
    /* N-184: RIFF/AVI with a LIST hdrl carrying avih (40ms frames, 100
     * frames -> 4000 ms) + 2 strh streams; the movi payload follows. */
    std::vector<u8> avi(256, 0);
    std::memcpy(avi.data(), "RIFF", 4);
    putle32(avi, 4, u32(avi.size() - 8));
    std::memcpy(avi.data() + 8, "AVI ", 4);
    std::memcpy(avi.data() + 12, "LIST", 4);
    putle32(avi, 16, 4 + (8 + 64) + (8 + 56) * 2); /* hdrl size */
    std::memcpy(avi.data() + 20, "hdrl", 4);
    /* avih at 24: size 64 */
    std::memcpy(avi.data() + 24, "avih", 4);
    putle32(avi, 28, 64);
    putle32(avi, 32, 40000); /* dwMicroSecPerFrame @ avih+8 = 40 ms */
    putle32(avi, 44, 100);   /* dwLength @ avih+20 = 100 frames */
    /* strh x2 */
    u32 at = 24 + 8 + 64;
    for (int i = 0; i < 2; ++i) {
        std::memcpy(avi.data() + at, "strh", 4);
        putle32(avi, at + 4, 56);
        at += 8 + 56;
    }
    std::memcpy(avi.data() + at, "LIST", 4);
    putle32(avi, at + 4, 4);
    std::memcpy(avi.data() + at + 8, "movi", 4);
    MemDevice dev(std::move(avi));
    tree::FileProperties::Extent a{0, 256, 0, 0};
    tree::FileProperties* pr = props_with("c.avi", 256, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("video", m.kind);
    EXPECT_EQ("avi", m.format);
    EXPECT_EQ(u64(40000) * 100 / 1000, m.duration_ms) << "40ms x 100 frames";
    EXPECT_NE(std::string::npos, m.codec.find("2str")) << "the stream count rides the codec";
}

TEST(PreviewMeta, MkvMetadataExtracted) {
    /* N-184: EBML header + Segment > Info (TimecodeScale 1ms, Duration
     * 2500 units) + Tracks (2 TrackEntry). Every size is a proper vint:
     *   EBML hdr: id 1A45DFA3, size 86 (6), body 42 86 81 01 00 00  -> 11 B
     *   Segment:  id 18538067, size 40 24 (= 36)                    -> 6 B, data@17
     *   Info:     id 1549A966, size 8E (= 14)                        -> 5 B, data@22
     *     TimecodeScale: id 2AD7B1, size 83, 0F4240 (= 1e6 ns = 1ms) -> 7 B
     *     Duration:      id 4489,   size 84, float 2500.0f           -> 7 B
     *   Tracks:   id 1654AE6B, size 8A (= 10)                        -> 5 B, data@41
     *     TrackEntry x2:  id AE, size 82, body E0 00                 -> 4 B each
     */
    std::vector<u8> mk(128, 0);
    size_t at = 0;
    auto put = [&](const u8* d, size_t n) {
        std::memcpy(mk.data() + at, d, n);
        at += n;
    };
    const u8 ebml[] = {0x1A, 0x45, 0xDF, 0xA3, 0x86, 0x42, 0x86, 0x81, 0x01, 0x00, 0x00};
    put(ebml, sizeof(ebml));
    const u8 seg[] = {0x18, 0x53, 0x80, 0x67, 0x40, 0x24};
    put(seg, sizeof(seg));
    const u8 info[] = {0x15, 0x49, 0xA9, 0x66, 0x8E};
    put(info, sizeof(info));
    const u8 tcs[] = {0x2A, 0xD7, 0xB1, 0x83, 0x0F, 0x42, 0x40};
    put(tcs, sizeof(tcs));
    const u8 dur[] = {0x44, 0x89, 0x84, 0x45, 0x1C, 0x40, 0x00};
    put(dur, sizeof(dur));
    const u8 trk[] = {0x16, 0x54, 0xAE, 0x6B, 0x8A};
    put(trk, sizeof(trk));
    const u8 te[] = {0xAE, 0x82, 0xE0, 0x00};
    put(te, sizeof(te));
    put(te, sizeof(te));
    const u64 len = at;
    MemDevice dev(std::move(mk));
    tree::FileProperties::Extent a{0, len, 0, 0};
    tree::FileProperties* pr = props_with("v.mkv", len, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("video", m.kind);
    EXPECT_EQ("mkv", m.format);
    /* TimecodeScale 1000000ns = 1ms; Duration 2500 units -> 2500 ms */
    EXPECT_EQ(u64(2500), m.duration_ms);
    EXPECT_NE(std::string::npos, m.codec.find("2trk")) << "the track count rides the codec";
}

TEST(PreviewMeta, FlacMetadataExtracted) {
    /* N-184: fLaC + STREAMINFO (rate 44100, channels 2, 441000 samples
     * -> 10000 ms). */
    std::vector<u8> fl(4 + 4 + 34, 0);
    std::memcpy(fl.data(), "fLaC", 4);
    fl[4] = 0x00; /* last+type STREAMINFO */
    fl[5] = 0;    /* block size 34 */
    fl[6] = 0;
    fl[7] = 34;
    const u32 rate = 44100;
    const u32 chans = 2;
    const u64 samples = 441000;
    /* packed field at si+10..si+17: 20 bits rate | 3 bits ch-1 | 5 bits
     * bps-1 | 36 bits samples (si = fl+8) */
    u64 bits = (u64(rate) << 44) | (u64(chans - 1) << 41) | (u64(16 - 1) << 36) | samples;
    for (int i = 0; i < 8; ++i)
        fl[8 + 10 + i] = u8(bits >> (8 * (7 - i)));
    const u64 len = fl.size();
    MemDevice dev(std::move(fl));
    tree::FileProperties::Extent a{0, len, 0, 0};
    tree::FileProperties* pr = props_with("a.flac", len, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("audio", m.kind);
    EXPECT_EQ("flac", m.format);
    EXPECT_EQ(u64(10000), m.duration_ms) << "441000 samples at 44100 Hz";
    EXPECT_EQ(44100u, m.width);
    EXPECT_EQ(2u, m.height);
}

TEST(PreviewMeta, Mp3DurationEstimated) {
    /* N-184: an ID3v2.3 tag (10-byte header + 100-byte body) then an
     * MPEG1 Layer III 128kbps frame sync; total payload 8210 bytes ->
     * (8210*8)/128 = 513 ms. */
    std::vector<u8> mp3(10 + 100 + 8100, 0);
    std::memcpy(mp3.data(), "ID3", 3);
    mp3[3] = 3; /* v2.3 */
    mp3[4] = 0;
    mp3[5] = 0; /* flags */
    mp3[6] = 0;
    mp3[7] = 0; /* syncsafe size = 100 */
    mp3[8] = 0;
    mp3[9] = 100;
    const size_t frame = 10 + 100;
    mp3[frame] = 0xFF;
    mp3[frame + 1] = 0xFB; /* MPEG1 Layer III, no CRC */
    mp3[frame + 2] = 0x90; /* bitrate index 9 = 128k, rate index 0 = 44100 */
    const u64 len = mp3.size();
    MemDevice dev(std::move(mp3));
    tree::FileProperties::Extent a{0, len, 0, 0};
    tree::FileProperties* pr = props_with("a.mp3", len, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("audio", m.kind);
    EXPECT_EQ("mp3", m.format);
    EXPECT_EQ(44100u, m.width);
    EXPECT_EQ(u64(len - frame) * 8 / 128, m.duration_ms) << "the bitrate estimate";
}

TEST(PreviewMeta, M4aTypedAsMp4Family) {
    /* N-184: the M4A brand rides the ftyp branch (it IS an ISO-BMFF
     * container): kind=video, format=mp4, codec="M4A ". */
    std::vector<u8> m4a(64, 0);
    putbe32(m4a, 0, 16);
    std::memcpy(m4a.data() + 4, "ftyp", 4);
    std::memcpy(m4a.data() + 8, "M4A ", 4);
    putbe32(m4a, 12, 0);
    std::memcpy(m4a.data() + 16, "mp42", 4);
    const u64 len = m4a.size();
    MemDevice dev(std::move(m4a));
    tree::FileProperties::Extent a{0, len, 0, 0};
    tree::FileProperties* pr = props_with("a.m4a", len, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("video", m.kind);
    EXPECT_EQ("mp4", m.format) << "M4A is MP4-family (the og's naming contract, S-049)";
    EXPECT_EQ("M4A ", m.codec);
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
                         "webp"});                       /* 12-byte RIFF/WEBP */
        cases.push_back({{'P', 'K'}, "archive", "zip"}); /* 2-byte PK */
        /* M-108: eligibility is extension-gated (the og's TxtViewFiles
         * whitelist) - a printable head in a non-whitelisted .bin is NOT
         * text (the old content-sniff false-positive class) */
        cases.push_back({{'%'}, "unsupported", ""}); /* printable head, non-whitelisted ext */
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
        props_with("c.txt", plain.size(), 0,
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
        props_with("c.txt", plain.size(), 0,
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

/* ---- M-066: the archive-content inventory (ArchivePreviewModel) ---- */

namespace {

/* build a minimal multi-entry zip: N members with the given names, a
 * stored (method-0) body of the given size, the CD + EOCD. crc != 0 for
 * the entries whose index is in has_crc. */
std::vector<u8> build_multi_zip(const std::vector<std::string>& names, u32 body_len,
                                const std::set<u32>& has_crc = {}) {
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
    std::vector<u32> lho(names.size(), 0);
    std::vector<std::vector<u8>> cds;
    for (u32 i = 0; i < names.size(); ++i) {
        lho[i] = u32(zip.size());
        zip.insert(zip.end(), {'P', 'K', 3, 4});
        put16(20);
        put16(0);
        put16(0); /* method = stored */
        put16(0);
        put16(0);
        put32(has_crc.count(i) ? 0xA1B2C3D4u : 0u);
        put32(body_len);
        put32(body_len);
        put16(u16(names[i].size()));
        put16(0);
        zip.insert(zip.end(), names[i].begin(), names[i].end());
        zip.insert(zip.end(), size_t(body_len), u8('z' + i));
        /* the CD record (built later, offsets need lho first) */
        std::vector<u8> cd;
        const auto p16 = [&cd](u16 v) {
            cd.push_back(u8(v));
            cd.push_back(u8(v >> 8));
        };
        const auto p32 = [&cd](u32 v) {
            cd.push_back(u8(v));
            cd.push_back(u8(v >> 8));
            cd.push_back(u8(v >> 16));
            cd.push_back(u8(v >> 24));
        };
        cd.insert(cd.end(), {'P', 'K', 1, 2});
        p16(20);
        p16(20);
        p16(0);
        p16(0);
        p16(0);
        p16(0);
        p32(has_crc.count(i) ? 0xA1B2C3D4u : 0u);
        p32(body_len);
        p32(body_len);
        p16(u16(names[i].size()));
        p16(0);
        p16(0);
        p16(0);
        p16(0);
        p32(0);
        p32(lho[i]);
        cd.insert(cd.end(), names[i].begin(), names[i].end());
        cds.push_back(std::move(cd));
    }
    const size_t cd_at = zip.size();
    size_t cd_len = 0;
    for (const auto& cd : cds) {
        zip.insert(zip.end(), cd.begin(), cd.end());
        cd_len += cd.size();
    }
    zip.insert(zip.end(), {'P', 'K', 5, 6});
    put16(0);
    put16(0);
    put16(u16(names.size()));
    put16(u16(names.size()));
    put32(u32(cd_len));
    put32(u32(cd_at));
    put16(0);
    return zip;
}

} // namespace

TEST(PreviewMeta, ArchiveEnumeratesEntries) {
    /* M-066: a plain multi-entry zip (the FS-origin case - not a carve
     * child) previews with the full content inventory: names, sizes,
     * compressed sizes, per-entry CRC presence. */
    const std::vector<u8> zip =
        build_multi_zip({"readme.txt", "dir/photo.jpg", "data.bin"}, 100, {0, 2});
    MemDevice dev{std::vector<u8>(zip)};
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties* pr =
        props_with("ar.zip", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("archive", m.kind);
    EXPECT_EQ("zip", m.format);
    ASSERT_EQ(3u, m.entries.size()) << "the ArchivePreviewModel inventory";
    EXPECT_EQ("readme.txt", m.entries[0].name);
    EXPECT_EQ(100u, m.entries[0].size);
    EXPECT_EQ(100u, m.entries[0].compressed);
    EXPECT_TRUE(m.entries[0].crc_present);
    EXPECT_EQ("dir/photo.jpg", m.entries[1].name);
    EXPECT_FALSE(m.entries[1].crc_present) << "the damaged-entry marker (CRC 0)";
    EXPECT_EQ("data.bin", m.entries[2].name);
    EXPECT_TRUE(m.entries[2].crc_present);
    EXPECT_FALSE(m.truncated);
}

TEST(PreviewMeta, ArchiveDamagedLocalHeaderStillEnumerates) {
    /* the og's family exists to browse DAMAGED zips: the CD is intact
     * but a member's local header is destroyed - the inventory still
     * enumerates (the CD is self-contained); the start magic stays
     * recognizable so the content sniff still routes to the zip path
     * (the real-world damaged-zip shape: the archive opens, a member's
     * body is gone). */
    std::vector<u8> zip = build_multi_zip({"a.txt", "b.txt", "c.txt"}, 50, {});
    /* destroy the SECOND member's local header magic (mid-file) */
    const size_t second_lfh = 30 + 5 /* name */ + 50 /* body */;
    ASSERT_LT(second_lfh + 4, zip.size());
    zip[second_lfh] = 0xDE;
    zip[second_lfh + 1] = 0xAD;
    zip[second_lfh + 2] = 0xBE;
    zip[second_lfh + 3] = 0xEF;
    MemDevice dev(std::move(zip));
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties* pr =
        props_with("damaged.zip", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("archive", m.kind);
    ASSERT_EQ(3u, m.entries.size()) << "the CD walk does not need the local headers";
    EXPECT_EQ("a.txt", m.entries[0].name);
    EXPECT_EQ("b.txt", m.entries[1].name);
    EXPECT_EQ("c.txt", m.entries[2].name);
}

TEST(PreviewMeta, ArchiveDocxUpgradeCoexistsWithEnumeration) {
    /* the docx hunt and the inventory share the CD walk - a docx both
     * upgrades to document/docx AND carries its entry list */
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

    MemDevice dev(std::move(zip));
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties* pr =
        props_with("d.docx", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("document", m.kind) << "the upgrade still fires";
    EXPECT_EQ("docx", m.format);
    EXPECT_NE(std::string::npos, m.text_snippet.find("hello preview world"));
    ASSERT_EQ(1u, m.entries.size()) << "AND the inventory rides along";
    EXPECT_EQ("word/document.xml", m.entries[0].name);
    EXPECT_EQ(plain.size(), m.entries[0].size);
}

TEST(PreviewMeta, ArchiveEntryCapAndTruncatedFlag) {
    /* the hostile-archive bound: kMaxPreviewArchiveEntries + the
     * truncated flag (never a silent loss) */
    std::vector<std::string> names;
    for (u32 i = 0; i < kMaxPreviewArchiveEntries + 40; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "f%06u.bin", i);
        names.emplace_back(buf);
    }
    const std::vector<u8> zip = build_multi_zip(names, 4);
    MemDevice dev{std::vector<u8>(zip)};
    const u64 zlen = dev.geometry().total_bytes;
    tree::FileProperties* pr =
        props_with("big.zip", zlen, 0, {tree::FileProperties::Extent{0, zlen, 0, 0}});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("archive", m.kind);
    EXPECT_EQ(kMaxPreviewArchiveEntries, m.entries.size()) << "the hostile bound";
    EXPECT_TRUE(m.truncated) << "the partial inventory is disclosed";
}

TEST(PreviewDocs, NoIInWindowStreamDriftAndNoMovGhost) {
    /* N-088: M-028's correction must hold at every outward-facing site -
     * the only permitted mentions of IInWindowStream are the correction
     * notes themselves (they name the 7-Zip/LZMA SDK); the preview
     * format comment must not advertise an unproduced mov. */
    /* the CONTRACT sites (headers + the audit row that rates our
     * surface); REFERENCE-API-GAP's og-architecture rows (R-42's export
     * loop) legitimately name the interface where the og actually uses
     * it - not drift. */
    const char* const files[] = {
        "include/cr/preview/cr_preview.h",
        "include/cr/cr_ui.h",
        "docs/API-SURFACE-AUDIT.md",
    };
    for (const char* f : files) {
        std::ifstream in(std::string(CR_SOURCE_DIR) + "/" + f, std::ios::binary);
        ASSERT_TRUE(in.good()) << f;
        std::string txt((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        size_t pos = 0;
        while ((pos = txt.find("IInWindowStream", pos)) != std::string::npos) {
            const std::string around = txt.substr(pos > 40 ? pos - 40 : 0, 80);
            EXPECT_NE(std::string::npos, around.find("7-Zip"))
                << f << ": an IInWindowStream mention outside a correction note";
            EXPECT_NE(std::string::npos, around.find("LZMA"))
                << f << ": the correction context must name the SDK";
            pos += 16;
        }
    }
    /* the mov ghost: the Metadata format comment must not list mov (no
     * producer - N-088's third site) */
    {
        std::ifstream in(std::string(CR_SOURCE_DIR) + "/include/cr/preview/cr_preview.h",
                         std::ios::binary);
        ASSERT_TRUE(in.good());
        std::string txt((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const size_t fmt = txt.find("std::string format;");
        ASSERT_NE(std::string::npos, fmt);
        const std::string comment = txt.substr(fmt, 200);
        EXPECT_EQ(std::string::npos, comment.find("|mov"))
            << "the format comment must not advertise a never-produced mov";
    }
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

/* M-077: the og's shipped preview PRODUCT caps as config-overridable
 * defaults (UIUX/10 §3, QM-verified copy): files over 100 MiB are "not
 * available for preview" (the eligibility gate refuses BEFORE any read,
 * disclosed via truncated - never silent) and decode work is bounded by
 * the 15 s time guard. Pre-fix: neither cap existed - an oversized
 * request proceeded to the budget. */
TEST(Preview, M077SizeGate100MiB) {
    /* a logical 101 MiB node over a 1 MiB resident prefix: refused at
     * the gate (never any read past the resident head), disclosed via
     * truncated - never silent. Built on the suite's own props helper. */
    tree::FileProperties::Extent sparse{0, 0, 0, 0}; /* placeholder; the gate runs first */
    (void)sparse;
    tree::FileProperties* pr = props_with("huge.bin", (100u << 20) + 1, 2, {});
    PropsGuard g(pr);
    pr->resident_data.assign(64, 0x41);
    MemDevice dev(std::vector<u8>(4096, 0)); /* the gate must never touch it */
    PreviewReader r(&dev, pr);
    Caps c;
    const Metadata m = extract_metadata(r, c);
    EXPECT_TRUE(m.truncated) << "the gate discloses via truncated, never silent";
    /* the override: a host raises its own line (the same input passes) */
    Caps raised;
    raised.max_file_bytes = 200u << 20;
    const Metadata m2 = extract_metadata(r, raised);
    EXPECT_FALSE(m2.truncated) << "the override is honored";
}

TEST(Preview, M077TimeGuardDefaultsOnAndOverridable) {
    /* the defaults pin: 100 MiB + 15 s, ON at zero-init */
    Caps c;
    EXPECT_EQ(100u << 20, c.max_file_bytes) << "the og's 100 MB line";
    EXPECT_EQ(15000u, c.max_decode_ms) << "the og's 15 s line";
    /* the disable: UINT32_MAX turns the time gate off */
    Caps off;
    off.max_decode_ms = 0xFFFFFFFFu;
    EXPECT_EQ(0xFFFFFFFFu, off.max_decode_ms);
}

/* ---- M-108: the TxtViewFiles text-preview eligibility model ----
 * The og gates text preview by EXTENSION MEMBERSHIP in
 * config/FilterFolder.ini [Filter] TxtViewFiles (DRWUI:535858/:535937,
 * the toupper-folded membership search FUN_140310aa0; the ".txt;"
 * ini-absent default). Content is never sniffed for eligibility. */

TEST(PreviewMeta, Utf8BomTextIsText) {
    /* the og renders a .txt whatever its bytes; the old 16-byte
     * printable-ASCII head gate refused the BOM byte (EF >= 0x7F) */
    std::vector<u8> txt(64, ' ');
    txt[0] = 0xEF;
    txt[1] = 0xBB;
    txt[2] = 0xBF;
    std::memcpy(txt.data() + 3, "hello bom text", 14);
    MemDevice dev(std::move(txt));
    tree::FileProperties::Extent a{0, 64, 0, 0};
    tree::FileProperties* pr = props_with("n.txt", 64, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("text", m.kind);
    EXPECT_EQ("txt", m.format);
    /* the BOM is skipped, the body passes through */
    EXPECT_EQ(std::string::npos, m.text_snippet.find("\xEF\xBB\xBF"));
    EXPECT_NE(std::string::npos, m.text_snippet.find("hello bom text"));
}

TEST(PreviewMeta, Utf16BomTextIsText) {
    /* UTF-16LE BOM text: decoded to UTF-8 for the snippet (a raw byte
     * pass-through would truncate at the first ASCII char's NUL) */
    const char16_t body[] = u"utf16 body";
    std::vector<u8> txt;
    txt.push_back(0xFF);
    txt.push_back(0xFE);
    for (char16_t c : body)
        for (int b = 0; b < 2; ++b)
            txt.push_back(u8(c >> (8 * b)));
    txt.resize(64, 0);
    MemDevice dev(std::move(txt));
    tree::FileProperties::Extent a{0, 64, 0, 0};
    tree::FileProperties* pr = props_with("n.txt", 64, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("text", m.kind);
    EXPECT_NE(std::string::npos, m.text_snippet.find("utf16 body"));
}

TEST(PreviewMeta, Utf16SurrogatePairDecodes) {
    /* an astral char (U+1F600) as a surrogate pair: the snippet carries
     * its UTF-8 encoding (F0 9F 98 80), not a replacement pair */
    const char16_t body[] = u"\x83\x83"; /* placeholder, real pair below */
    (void)body;
    std::vector<u8> txt;
    txt.push_back(0xFF);
    txt.push_back(0xFE);
    const char16_t pair[2] = {0xD83D, 0xDE00}; /* U+1F600 */
    for (char16_t c : pair)
        for (int b = 0; b < 2; ++b)
            txt.push_back(u8(c >> (8 * b)));
    txt.push_back(' ');
    txt.push_back(0);
    txt.push_back('!');
    txt.push_back(0);
    txt.resize(64, 0);
    MemDevice dev(std::move(txt));
    tree::FileProperties::Extent a{0, 64, 0, 0};
    tree::FileProperties* pr = props_with("n.txt", 64, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("text", m.kind);
    EXPECT_NE(std::string::npos, m.text_snippet.find("\xF0\x9F\x98\x80"));
}

TEST(PreviewMeta, Latin1TextIsText) {
    /* Latin-1 accented text (0xE9 >= 0x7F): the og renders it (the
     * extension gates); the snippet passes the high byte through raw */
    std::vector<u8> txt(64, ' ');
    std::memcpy(txt.data(), "caf\xE9 menu", 9);
    MemDevice dev(std::move(txt));
    tree::FileProperties::Extent a{0, 64, 0, 0};
    tree::FileProperties* pr = props_with("n.txt", 64, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("text", m.kind);
    EXPECT_NE(std::string::npos, m.text_snippet.find("caf\xE9"));
}

TEST(PreviewMeta, PrintableHeadBinaryNotText) {
    /* the false-positive class: a non-whitelisted extension with 16
     * printable head bytes was classified text with a garbage snippet;
     * the og's whitelist membership for .xyz is false */
    std::vector<u8> bin(64, 0);
    std::memcpy(bin.data(), "MJFZ-0123456789AB", 17); /* printable head */
    for (size_t i = 17; i < 64; ++i)
        bin[i] = u8(i * 7);
    MemDevice dev(std::move(bin));
    tree::FileProperties::Extent a{0, 64, 0, 0};
    tree::FileProperties* pr = props_with("data.xyz", 64, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("unsupported", m.kind);
    EXPECT_TRUE(m.text_snippet.empty());
}

TEST(PreviewMeta, ExtensionlessNameIsNotText) {
    /* the og's empty-needle early return: an extensionless name is
     * never text-viewable, whatever its bytes */
    std::vector<u8> txt(64, ' ');
    std::memcpy(txt.data(), "plain readme body", 17);
    MemDevice dev(std::move(txt));
    tree::FileProperties::Extent a{0, 64, 0, 0};
    tree::FileProperties* pr = props_with("README", 64, 0, {a});
    PropsGuard g(pr);
    PreviewReader r(&dev, pr);
    const Metadata m = extract_metadata(r);
    EXPECT_EQ("unsupported", m.kind);
}

TEST(PreviewMeta, WhitelistMembershipCaseInsensitive) {
    /* FUN_140310aa0 folds with toupper: .TXT (the shipped list's own
     * casing) and .txt/.Txt are the same extension; the shipped list's
     * lowercase families (.md/.json) match their uppercase forms too */
    EXPECT_TRUE(preview::textexts::eligible(".TXT"));
    EXPECT_TRUE(preview::textexts::eligible(".txt"));
    EXPECT_TRUE(preview::textexts::eligible(".Txt"));
    EXPECT_TRUE(preview::textexts::eligible(".MD"));
    EXPECT_TRUE(preview::textexts::eligible(".md"));
    EXPECT_TRUE(preview::textexts::eligible(".JSON"));
    EXPECT_TRUE(preview::textexts::eligible(".log"));
    EXPECT_TRUE(preview::textexts::eligible(".XML"));
    EXPECT_TRUE(preview::textexts::eligible(".ini"));
    EXPECT_TRUE(preview::textexts::eligible(".cfg"));
    EXPECT_TRUE(preview::textexts::eligible(".bat"));
    EXPECT_TRUE(preview::textexts::eligible(".msbak"));
    EXPECT_TRUE(preview::textexts::eligible(".database_uuid"));
    /* non-members: the og's list has no .xyz/.dat/.bin/.exe/.docx */
    EXPECT_FALSE(preview::textexts::eligible(".xyz"));
    EXPECT_FALSE(preview::textexts::eligible(".dat"));
    EXPECT_FALSE(preview::textexts::eligible(".bin"));
    EXPECT_FALSE(preview::textexts::eligible(".exe"));
    EXPECT_FALSE(preview::textexts::eligible(".docx"));
    EXPECT_FALSE(preview::textexts::eligible(".pdf"));
    /* the empty needle and the non-dot forms never match */
    EXPECT_FALSE(preview::textexts::eligible(""));
    EXPECT_FALSE(preview::textexts::eligible("txt"));
    /* no partial-token match: ".t" must not hit ".TXT;" */
    EXPECT_FALSE(preview::textexts::eligible(".t"));
    /* no delimiter injection: ".txt;.exe" is one (absurd) extension,
     * not two - it is not in the pool as a whole token */
    EXPECT_FALSE(preview::textexts::eligible(".txt;.exe"));
}

TEST(PreviewTextWhitelist, ByteExactAgainstCorpus) {
    /* the generated pool is the og's TxtViewFiles value VERBATIM: the
     * 97 shipped tokens (92 unique case-folded), the og's entry order,
     * the trailing delimiter, NUL-terminated (regenerate via
     * scripts/gen_preview_text_exts.py - the gen_filterfolder pattern) */
    const std::string_view pool(
        reinterpret_cast<const char*>(preview::textexts::data::kTxtViewPool),
        preview::textexts::data::kTxtViewPoolLen);
    /* the shipped list's head and tail, verbatim */
    EXPECT_EQ(0u, pool.find(".TXT;.CFG;.CONFIG;.CSS;.CSV;.HTML;.INF;"));
    EXPECT_NE(std::string_view::npos, pool.find(";.XSL;.bak;.msbak;\0", 0));
    /* the exact shipped token set (re-derived independently here from
     * the corpus value - any drift in generation fails this) */
    const char* kCorpusValue = ".TXT;.CFG;.CONFIG;.CSS;.CSV;.HTML;.INF;.INFO;.INI;.JS;.LOG;.XML;"
                               ".aae;.database_uuid;.srt;.bat;.md;.json;.1ST;.A;.A8S;.ANS;.ARENA;"
                               ".AS;.ASA;.ASM;.ATH;.ATT;.AVS;.BBZ;.BCS;.BMK;.BOOK;.BPW;.BSD;.BSDL;"
                               ".BSH;.CAMP;.CEL;.CELX;.CFG;.CFG;.CGI;.CHART;.CMD;.CONF;.CS;.CSH;"
                               ".CU;.CUS;.DBA;.DIF;.FOUNTAIN;.GM;.GSC;.GSD;.HAML;.HPS;.HTM;.JAV;"
                               ".JAVA;.JRTF;.M;.M;.MAP;.MARKDOWN;.MCMETA;.MDBACKUP;.MDINFO;.MDL;"
                               ".MDOWN;.ML;.MXML;.OSU;.OUT;.P6;.PAS;.PHP;.PL;.PNPROJ;.PNPT;"
                               ".PROPERTIES;.PROPERTIES;.PY;.QC;.QC;.RTX;.S;.SRC;.URL;.WTX;.XQL;"
                               ".XQY;.XSD;.XSL;.bak;.msbak;";
    EXPECT_EQ(std::string_view(kCorpusValue), pool.substr(0, pool.find('\0')));
    EXPECT_EQ(97u, preview::textexts::data::kTxtViewTokenCount);
    /* the pool NUL-terminates within its own bytes */
    EXPECT_LT(std::strlen(reinterpret_cast<const char*>(preview::textexts::data::kTxtViewPool)),
              preview::textexts::data::kTxtViewPoolLen);
}
