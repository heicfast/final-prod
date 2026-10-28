/* test_thumbnail.cpp - cr_thumbnail acceptance: the bounded decode surface
 * (JPEG/PNG/BMP/GIF + native baseline TIFF variants), the dims-first bomb
 * guard (proved by device-byte accounting), the EXIF embedded-thumbnail
 * fast path, aspect-preserving no-upscale downscaling with EXACT quadrant
 * colors on lossless fixtures, budget/truncation/unsupported/unreadable
 * honesty paths, and the caps defaults pinned to the corpus anchors.
 *
 * Fixtures: tests/data/thumbnails (scripts/gen_thumbnail_fixtures.py).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/preview/cr_thumbnail.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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
        if (fail_from_ && off + len > fail_from_)
            return ERR_READ_FAILED;
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 bytes() const { return bytes_; }
    void fail_from(u64 off) { fail_from_ = off; }
    void reset_data(std::vector<u8> b) {
        data_ = std::move(b);
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }

private:
    std::vector<u8> data_;
    Geometry geom_;
    std::atomic<u64> reads_{0};
    std::atomic<u64> bytes_{0};
    std::atomic<u64> fail_from_{0};
};

std::vector<u8> load_fixture(const char* name) {
    FILE* f = fopen((std::string("data/thumbnails/") + name).c_str(), "rb");
    if (!f)
        return {};
    std::vector<u8> out;
    u8 buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        out.insert(out.end(), buf, buf + n);
    fclose(f);
    return out;
}

/* A node whose logical content is the fixture: resident (no device IO) or
 * extent-backed (device IO happens, accounting is observable). */
struct Node {
    MemDevice dev{std::vector<u8>(512, 0)};
    tree::FileProperties props;
    explicit Node(std::vector<u8> bytes, bool resident) {
        props.name = u"img";
        props.logical_size = bytes.size();
        if (resident) {
            props.resident_data = std::move(bytes);
        } else {
            /* place the image at device offset 4096 so nothing rides the
             * resident prefix */
            std::vector<u8> disk(4096 + bytes.size(), 0);
            std::memcpy(disk.data() + 4096, bytes.data(), bytes.size());
            dev.reset_data(std::move(disk));
            tree::FileProperties::Extent e{4096, bytes.size(), 0, 0};
            props.extents.push_back(e);
        }
    }
    PreviewReader reader() { return PreviewReader(&dev, &props); }
};

void expect_pixel(const Thumbnail& t, u32 x, u32 y, int r, int g, int b, int tol) {
    ASSERT_EQ(t.width * t.height * 4u, t.rgba.size());
    const u8* p = t.rgba.data() + (size_t(y) * t.width + x) * 4;
    EXPECT_NEAR(p[0], r, tol) << "r at " << x << "," << y;
    EXPECT_NEAR(p[1], g, tol) << "g at " << x << "," << y;
    EXPECT_NEAR(p[2], b, tol) << "b at " << x << "," << y;
    EXPECT_EQ(255, p[3]) << "alpha";
}

} // namespace

/* ---------------- surface ---------------- */

TEST(Thumbnail, StatusNamesAreStable) {
    EXPECT_STREQ("ok", thumbnail_status_name(ThumbStatus::Ok));
    EXPECT_STREQ("unsupported", thumbnail_status_name(ThumbStatus::Unsupported));
    EXPECT_STREQ("decode-failed", thumbnail_status_name(ThumbStatus::DecodeFailed));
    EXPECT_STREQ("oversized", thumbnail_status_name(ThumbStatus::Oversized));
    EXPECT_STREQ("budget-exceeded", thumbnail_status_name(ThumbStatus::BudgetExceeded));
    EXPECT_STREQ("unreadable-span", thumbnail_status_name(ThumbStatus::UnreadableSpan));
    EXPECT_STREQ("bad-parameter", thumbnail_status_name(ThumbStatus::BadParameter));
}

TEST(Thumbnail, FormatSupportIsTheDocumentedV1Matrix) {
    for (const char* f : {"jpeg", "png", "bmp", "gif", "tiff", "psd", "pnm", "hdr", "raw"})
        EXPECT_TRUE(thumbnail_format_supported(f)) << f; /* raw: S-045's TIFF-RAW half */
    /* documented phase-2 refusals */
    EXPECT_FALSE(thumbnail_format_supported("webp"));
    EXPECT_FALSE(thumbnail_format_supported("heic"));
    EXPECT_FALSE(thumbnail_format_supported("pdf"));
    EXPECT_FALSE(thumbnail_format_supported("mp4"));
    /* the metafile/icon family: named-and-refused (GDI+ natives the og
     * rasterizes; the core's matrix defers them) */
    EXPECT_FALSE(thumbnail_format_supported("wmf"));
    EXPECT_FALSE(thumbnail_format_supported("emf"));
    EXPECT_FALSE(thumbnail_format_supported("ico"));
}

TEST(Thumbnail, MetafileAndIconFamiliesAreNamedRefusals) {
    /* every form of the GDI+ metafile/icon family must be REFUSED WITH ITS
     * NAME, never "unknown" - the refusal matrix's own honesty contract:
     * placeable WMF (0xD7CDC69A key), standard WMF (type/header-size
     * header), EMF (the " EMF" signature dword at +40), ICO and CUR. */
    struct Case {
        const char* name;
        std::vector<u8> bytes;
    };
    std::vector<u8> placeable(64, 0x41);
    placeable[0] = 0x9A;
    placeable[1] = 0xC6;
    placeable[2] = 0xCD;
    placeable[3] = 0xD7;
    std::vector<u8> wmf_std(64, 0x41);
    wmf_std[0] = 0x02;
    wmf_std[1] = 0x00;
    wmf_std[2] = 0x09;
    wmf_std[3] = 0x00;
    std::vector<u8> emf(64, 0x41);
    emf[0] = 0x01; /* EMR_HEADER */
    emf[1] = emf[2] = emf[3] = 0x00;
    emf[40] = ' ';
    emf[41] = 'E';
    emf[42] = 'M';
    emf[43] = 'F';
    std::vector<u8> ico(64, 0x41);
    ico[0] = ico[1] = 0x00;
    ico[2] = 0x01;
    ico[3] = 0x00;
    std::vector<u8> cur(64, 0x41);
    cur[0] = cur[1] = 0x00;
    cur[2] = 0x02;
    cur[3] = 0x00;
    const Case cases[] = {
        {"wmf", placeable}, {"wmf", wmf_std}, {"emf", emf}, {"ico", ico}, {"ico", cur},
    };
    for (const Case& c : cases) {
        Node n(c.bytes, true);
        PreviewReader r = n.reader();
        Thumbnail t;
        ASSERT_EQ(ThumbStatus::Unsupported, make_thumbnail(r, ThumbnailCaps{}, t)) << c.name;
        EXPECT_EQ(c.name, t.source_format)
            << "the metafile/icon family must be refused BY NAME, not 'unknown'";
    }
}

TEST(Thumbnail, CapsDefaultsPinTheCorpusAnchors) {
    ThumbnailCaps c;
    EXPECT_EQ(256u, c.max_edge);          /* the og UI-side GDI+ convention */
    EXPECT_EQ(64ull << 20, c.max_pixels); /* 256 MiB worst-case RGBA decode */
    EXPECT_EQ(32u << 20, c.max_read);     /* M-080: the og GDI+ path has no input cap */
    EXPECT_EQ(1u << 22, c.max_work);      /* extract_metadata work scale */
    EXPECT_TRUE(c.allow_embedded);
}

/* ---------------- lossless decodes (exact quadrant colors) ---------------- */

TEST(Thumbnail, PngDecodesAndDownscalesExactly) {
    auto fx = load_fixture("quad64x48.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.png not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    EXPECT_EQ(16u, t.width);
    EXPECT_EQ(12u, t.height);
    EXPECT_EQ("png", t.source_format);
    EXPECT_FALSE(t.from_embedded);
    /* the truncated disclosure must be FALSE for a complete PNG: the
     * strict decoders refuse short streams (Ok proves completeness); the
     * post-parse EOF probe is stb API noise, never data truncation (the
     * M-102 ABI test's fail-before caught the poisoned signal) */
    EXPECT_EQ(0u, t.truncated);
    /* 64/16 = 4, 48/12 = 4: every 4x4 source block lands on one output
     * pixel, so the quadrant colors survive the box filter EXACTLY */
    expect_pixel(t, 2, 2, 255, 0, 0, 0);
    expect_pixel(t, 13, 2, 0, 255, 0, 0);
    expect_pixel(t, 2, 9, 0, 0, 255, 0);
    expect_pixel(t, 13, 9, 255, 255, 0, 0);
}

/* S-043: every thumbnail is EXIF-oriented - the og reads GDI+ property
 * 0x112 and applies the matching RotateFlip BEFORE the 256-px pass
 * (FUN_1402fcc30 :521694-725, call order :652307-309). The quadrant
 * colors pin each transform exactly (JPEG tolerance); orientation 1 is
 * identity and 9 (invalid) never rotates. Pre-fix: no orientation code
 * existed anywhere in the engine - every orientation-6/8/3 portrait
 * thumbnailed sideways. */
class ExifOrientationTest : public ::testing::TestWithParam<int> {};
TEST_P(ExifOrientationTest, AppliesOgRotateFlip) {
    const int orient = GetParam();
    char name[48];
    std::snprintf(name, sizeof(name), "exif_orient%d.jpg", orient);
    auto fx = load_fixture(name);
    if (fx.empty())
        GTEST_SKIP() << "fixture missing";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 256; /* no downscale: the raw transform is observable */
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    EXPECT_EQ("jpeg", t.source_format);
    /* the quadrant layout: TL=red TR=green BL=blue BR=yellow, 64x48 */
    const bool swapped = orient >= 5 && orient <= 8;
    EXPECT_EQ(swapped ? 48u : 64u, t.width) << "orient " << orient;
    EXPECT_EQ(swapped ? 64u : 48u, t.height) << "orient " << orient;
    /* where did the RED (top-left) quadrant land? */
    int rx = 2, ry = 2;
    switch (orient) {
    case 1:
        rx = 2;
        ry = 2;
        break; /* identity */
    case 2:
        rx = 61;
        ry = 2;
        break; /* flip X -> top-right */
    case 3:
        rx = 61;
        ry = 45;
        break; /* rotate 180 -> bottom-right */
    case 4:
        rx = 2;
        ry = 45;
        break; /* flip Y -> bottom-left */
    case 5:
        rx = 2;
        ry = 2;
        break; /* transpose -> top-left */
    case 6:
        rx = 45;
        ry = 2;
        break; /* rotate 90 CW -> top-right */
    case 7:
        rx = 45;
        ry = 61;
        break; /* transverse -> bottom-right */
    case 8:
        rx = 2;
        ry = 61;
        break; /* rotate 270 CW -> bottom-left */
    case 9:
        rx = 2;
        ry = 2;
        break; /* invalid: no transform */
    }
    if (orient == 1 || orient == 9 || orient == 5)
        expect_pixel(t, 2, 2, 255, 0, 0, 60); /* red stays TL */
    else
        expect_pixel(t, u32(rx), u32(ry), 255, 0, 0, 60); /* red per the map */
    /* the opposite corner (bottom-right of the ORIGINAL) is yellow */
    int yx = 61, yy = 45;
    switch (orient) {
    case 2:
        yx = 2;
        yy = 45;
        break;
    case 3:
        yx = 2;
        yy = 2;
        break;
    case 4:
        yx = 61;
        yy = 2;
        break;
    case 5:
        yx = 45;
        yy = 61;
        break;
    case 6:
        yx = 2;
        yy = 61;
        break;
    case 7:
        yx = 2;
        yy = 2;
        break;
    case 8:
        yx = 45;
        yy = 2;
        break;
    }
    if (orient != 1 && orient != 9)
        expect_pixel(t, u32(yx), u32(yy), 255, 255, 0, 60); /* yellow per the map */
}
INSTANTIATE_TEST_SUITE_P(ExifOrientation, ExifOrientationTest,
                         ::testing::Values(1, 2, 3, 4, 5, 6, 7, 8, 9));

/* M-080: the byte-heavy but pixel-benign middle class - a 4.07 MiB 12-MP
 * JPEG (the common modern-phone photo) fell between the pre-fix guards: no
 * fast path (12 MP < 64 MP) and no read budget (4.07 MiB > the old 4 MiB
 * default) -> BudgetExceeded, no thumbnail. The og's GDI+ path has no
 * input byte cap at all (FUN_1403d7840 :652377-419). Post-fix: the raised
 * 32 MiB default reads it and the 64 MP pixel guard still bounds decode
 * memory. */
TEST(Thumbnail, OrdinaryPhotoDefaultBudget) {
    auto fx = load_fixture("byteheavy12mp.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/byteheavy12mp.jpg not present";
    EXPECT_GT(fx.size(), size_t(4u << 20)) << "the fixture must exceed the OLD default";
    EXPECT_LT(fx.size(), size_t(8u << 20));
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c; /* zero-config: the SHIPPED defaults are the contract */
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t))
        << "the ordinary photo must thumbnail under the shipped defaults";
    EXPECT_EQ("jpeg", t.source_format);
    EXPECT_FALSE(t.from_embedded) << "the full decode wins (12 MP <= 64 MP)";
    EXPECT_EQ(256u, t.width); /* 4000 > 3000: the edge axis downscales */
    EXPECT_EQ(192u, t.height);
    /* the default pin: the raised cap is the ABI contract hosts inherit */
    ThumbnailCaps dflt;
    EXPECT_EQ(32u << 20, dflt.max_read) << "M-080: the raised I/O bound";
    EXPECT_EQ(64ull * 1024 * 1024, dflt.max_pixels) << "the pixel guard unchanged";
}

/* M-080: the embedded fast path fires for BYTE-oversized JPEGs too - it
 * reads only the embedded thumb (a few KiB), never the body. */
TEST(Thumbnail, EmbeddedFastPathOnByteOversize) {
    /* splice: a 5 MiB junk body + a real embedded thumb + huge SOF dims */
    auto fx = load_fixture("byteheavy12mp.jpg");
    auto thumb_fx = load_fixture("quad64x48.jpg");
    if (fx.empty() || thumb_fx.empty())
        GTEST_SKIP() << "fixtures not present";
    std::vector<u8> img;
    img.push_back(0xFF);
    img.push_back(0xD8);
    /* APP1/Exif with IFD0(orientation=1) + IFD1(thumb) - spliced inline */
    const u32 tlen = u32(thumb_fx.size());
    const u32 ifd0_off = 8;
    const u32 ifd1_off = ifd0_off + 2 + 12 + 4;
    const u32 thumb_off = ifd1_off + 2 + 24 + 4;
    std::vector<u8> tiff;
    auto p16 = [&](u16 v) {
        tiff.push_back(u8(v));
        tiff.push_back(u8(v >> 8));
    };
    auto p32 = [&](u32 v) {
        tiff.push_back(u8(v));
        tiff.push_back(u8(v >> 8));
        tiff.push_back(u8(v >> 16));
        tiff.push_back(u8(v >> 24));
    };
    tiff.push_back('I');
    tiff.push_back('I');
    p16(42);
    p32(ifd0_off);
    p16(1); /* IFD0: 1 entry */
    p16(0x0112);
    p16(3);
    p32(1);
    p16(1);
    p16(0);
    p32(ifd1_off);
    p16(2); /* IFD1: 2 entries */
    p16(0x0201);
    p16(4);
    p32(1);
    p32(thumb_off);
    p16(0x0202);
    p16(4);
    p32(1);
    p32(tlen);
    p32(0);
    tiff.insert(tiff.end(), thumb_fx.begin(), thumb_fx.end());
    const u32 seg_len = u32(6 + tiff.size() + 2);
    img.push_back(0xFF);
    img.push_back(0xE1);
    img.push_back(u8(seg_len >> 8));
    img.push_back(u8(seg_len));
    const char exif6[6] = {'E', 'x', 'i', 'f', 0, 0};
    img.insert(img.end(), exif6, exif6 + 6);
    img.insert(img.end(), tiff.begin(), tiff.end());
    /* SOF0 with benign 12-MP dims (4000x3000): pixel-benign, byte-heavy -
     * only the byte axis triggers the fast path */
    img.push_back(0xFF);
    img.push_back(0xC0);
    img.push_back(0);
    img.push_back(11); /* segment length */
    img.push_back(8);  /* precision */
    img.push_back(u8(3000 >> 8));
    img.push_back(u8(3000));
    img.push_back(u8(4000 >> 8));
    img.push_back(u8(4000));
    img.push_back(1); /* 1 component */
    img.push_back(1);
    img.push_back(0x11);
    img.push_back(0);
    /* a body just past a small max_read: the fast path must serve the
     * embedded thumb without reading the body */
    const size_t junk = size_t(256) * 1024;
    img.insert(img.end(), junk, 0x5A);
    img.push_back(0xFF);
    img.push_back(0xD9);

    Node n(img, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_read = 128u * 1024; /* the body exceeds it by 2x */
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t))
        << "byte-oversized JPEGs serve from the embedded thumb";
    EXPECT_TRUE(t.from_embedded);
    EXPECT_EQ("jpeg", t.source_format);
    EXPECT_EQ(64u, t.width);
    EXPECT_EQ(48u, t.height);
}

/* S-045 (the TIFF-RAW half): camera RAW files are TIFF-based containers
 * whose visible image is the full-size JPEG preview (the
 * JPEGInterchangeFormat pair 513/514) - the og routes them through the
 * EURawImg/LibRaw pipeline whose handoff IS the preview. Pre-fix: the
 * CR2 shape fell into the native TIFF decoder and failed (CFA
 * photometric has no baseline semantics). */
TEST(Thumbnail, RawTiffFamilyServesJpegPreview) {
    auto fx = load_fixture("tiff_raw_cr2.bin");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/tiff_raw_cr2.bin not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t))
        << "the CR2-shaped RAW must thumbnail from its JPEG preview";
    EXPECT_EQ("raw", t.source_format);
    EXPECT_EQ(16u, t.width);
    EXPECT_EQ(12u, t.height);
    expect_pixel(t, 2, 2, 255, 0, 0, 60); /* the quadrant preview decodes */
    expect_pixel(t, 13, 2, 0, 255, 0, 60);
    expect_pixel(t, 2, 9, 0, 0, 255, 60);
    expect_pixel(t, 13, 9, 255, 255, 0, 60);
    EXPECT_TRUE(thumbnail_format_supported("raw"));
    /* the format probe: the RAW family is supported since S-045 */
}

TEST(Thumbnail, JpegDecodesWithLossyTolerance) {
    auto fx = load_fixture("quad64x48.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.jpg not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    EXPECT_EQ(16u, t.width);
    EXPECT_EQ(12u, t.height);
    EXPECT_EQ("jpeg", t.source_format);
    expect_pixel(t, 2, 2, 255, 0, 0, 60);
    expect_pixel(t, 13, 2, 0, 255, 0, 60);
    expect_pixel(t, 2, 9, 0, 0, 255, 60);
    expect_pixel(t, 13, 9, 255, 255, 0, 60);
}

TEST(Thumbnail, BmpDecodesExactly) {
    auto fx = load_fixture("quad64x48.bmp");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.bmp not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    EXPECT_EQ("bmp", t.source_format);
    expect_pixel(t, 2, 2, 255, 0, 0, 0);
    expect_pixel(t, 13, 9, 255, 255, 0, 0);
}

TEST(Thumbnail, GifDecodesFirstFrameExactly) {
    auto fx = load_fixture("quad64x48.gif");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.gif not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    EXPECT_EQ("gif", t.source_format);
    expect_pixel(t, 2, 2, 255, 0, 0, 8);
    expect_pixel(t, 13, 2, 0, 255, 0, 8);
}

class ThumbnailTiff : public ::testing::TestWithParam<const char*> {};

TEST_P(ThumbnailTiff, BaselineVariantsDecodeExactly) {
    auto fx = load_fixture(GetParam());
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/" << GetParam() << " not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t)) << GetParam();
    EXPECT_EQ(16u, t.width);
    EXPECT_EQ(12u, t.height);
    EXPECT_EQ("tiff", t.source_format);
    if (std::strstr(GetParam(), "gray16")) {
        /* the four 16-bit quadrants 0x1F1F/0xE0E0/0x8080/0xFFFF narrow
         * (>>8) to 31/224/128/255 */
        expect_pixel(t, 2, 2, 31, 31, 31, 0);
        expect_pixel(t, 13, 2, 224, 224, 224, 0);
        expect_pixel(t, 2, 9, 128, 128, 128, 0);
        expect_pixel(t, 13, 9, 255, 255, 255, 0);
    } else {
        expect_pixel(t, 2, 2, 255, 0, 0, 8);
        expect_pixel(t, 13, 2, 0, 255, 0, 8);
        expect_pixel(t, 2, 9, 0, 0, 255, 8);
        expect_pixel(t, 13, 9, 255, 255, 0, 8);
    }
}

INSTANTIATE_TEST_SUITE_P(TiffVariants, ThumbnailTiff,
                         ::testing::Values("tiff_rgb_le.tif", "tiff_rgb_be.tif",
                                           "tiff_packbits.tif", "tiff_deflate.tif",
                                           "tiff_palette.tif", "tiff_gray16.tif",
                                           /* M-085: the classic TIFF LZW variant */
                                           "tiff_lzw.tif"));

/* ---------------- geometry policy ---------------- */

TEST(Thumbnail, AspectPreservedBothAxesNeverUpscaled) {
    auto fx = load_fixture("portrait32x64.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/portrait32x64.png not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 8;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    /* 32x64 with max_edge 8 -> 4x8 exactly (both axes bounded, portrait
     * height never exceeds the cap - the og's width-only quirk corrected) */
    EXPECT_EQ(4u, t.width);
    EXPECT_EQ(8u, t.height);
    expect_pixel(t, 1, 1, 255, 0, 0, 0);
    expect_pixel(t, 3, 6, 0, 0, 255, 0);
}

TEST(Thumbnail, SmallImageIsNotUpscaled) {
    auto fx = load_fixture("tiny8x6.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/tiny8x6.png not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_EQ(8u, t.width); /* below max_edge: original size kept */
    EXPECT_EQ(6u, t.height);
    expect_pixel(t, 2, 2, 10, 200, 30, 0);
}

/* ---------------- guards + honesty ---------------- */

TEST(Thumbnail, BombGuardRefusesBeforeDecode) {
    auto fx = load_fixture("bomb_dims.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/bomb_dims.png not present";
    Node n(fx, false); /* extent-backed: device accounting is observable */
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Oversized, make_thumbnail(r, ThumbnailCaps{}, t));
    /* dims-first proof: the 30000x30000 header was refused with less than
     * 1 MiB pulled from the device (a full decode would need ~3.4 GB) */
    EXPECT_LT(n.dev.bytes(), 1u << 20);
    EXPECT_TRUE(t.rgba.empty());
}

TEST(Thumbnail, OversizedJpegUsesExifEmbeddedThumbnail) {
    auto fx = load_fixture("huge_dims_exif_thumb.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/huge_dims_exif_thumb.jpg not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_TRUE(t.from_embedded);
    EXPECT_EQ("jpeg", t.source_format);
    /* the embedded 24x18 thumb decodes (below max_edge: no upscale) */
    EXPECT_EQ(24u, t.width);
    EXPECT_EQ(18u, t.height);
    expect_pixel(t, 4, 4, 9, 99, 199, 40);
    expect_pixel(t, 19, 13, 200, 100, 0, 40);
}

TEST(Thumbnail, SmallJpegWithEmbeddedThumbPrefersFullDecode) {
    auto fx = load_fixture("exif_thumb_small.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/exif_thumb_small.jpg not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_FALSE(t.from_embedded);
    EXPECT_EQ(48u, t.width); /* the 48x36 main image, not the 24x18 thumb */
    EXPECT_EQ(36u, t.height);
}

/* ---------------- S-043 riders (the closing-fix lane's additions over
 * the landed JPEG battery above): the TIFF tag-274 leg (the datum GDI+
 * surfaces as property 0x112 for TIFF inputs) and the embedded fast
 * path's D-105 orientation rule ---------------- */


TEST(Thumbnail, TiffOrientationApplied) {
    /* tag 274 = GDI+ property 0x112 for TIFF inputs; lossless fixture so
     * the corner colors are exact */
    auto fx = load_fixture("tiff_orient6.tif");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/tiff_orient6.tif not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_EQ(48u, t.width); /* the 90-degree cases swap the axes */
    EXPECT_EQ(64u, t.height);
    expect_pixel(t, 6, 4, 0, 0, 255, 30);  /* stored BL lands at TL */
    expect_pixel(t, 42, 4, 255, 0, 0, 30); /* stored TL lands at TR */
    expect_pixel(t, 6, 56, 255, 255, 0, 30);
    expect_pixel(t, 42, 56, 0, 255, 0, 30);
}

TEST(Thumbnail, TiffOrientationInvalidIgnored) {
    auto fx = load_fixture("tiff_orient_9invalid.tif");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/tiff_orient_9invalid.tif not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_EQ(64u, t.width);
    EXPECT_EQ(48u, t.height);
}

TEST(Thumbnail, EmbeddedFastPathServesStoredOrientationByDefault) {
    /* the CIPA DC-008 convention: an IFD1 thumbnail with no orientation
     * declaration of its own is stored upright - it serves as stored (no
     * double rotation; the pixels are the existing pin's) */
    auto fx = load_fixture("huge_dims_exif_thumb.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/huge_dims_exif_thumb.jpg not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_TRUE(t.from_embedded);
    EXPECT_EQ(24u, t.width);
    EXPECT_EQ(18u, t.height);
}

TEST(Thumbnail, EmbeddedFastPathHonorsIfd1DeclaredOrientation) {
    /* IFD1's own 0x112=6 is the marker of a NOT-already-oriented embedded
     * thumb: the fast path applies the transform (the finding's
     * conditional, made precise) */
    auto fx = load_fixture("huge_dims_exif_thumb_ifd1o6.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/huge_dims_exif_thumb_ifd1o6.jpg not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_TRUE(t.from_embedded);
    EXPECT_EQ(18u, t.width); /* the 90-degree case swaps the thumb axes */
    EXPECT_EQ(24u, t.height);
    /* stored thumb: left half (9,99,199), right half (200,100,0); after
     * rot90CW the left half lands on TOP */
    expect_pixel(t, 4, 4, 9, 99, 199, 45);
    expect_pixel(t, 13, 19, 200, 100, 0, 45);
}

TEST(Thumbnail, TruncatedPngFailsHonestly) {
    auto fx = load_fixture("truncated.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/truncated.png not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ASSERT_EQ(ThumbStatus::DecodeFailed, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_TRUE(t.truncated);
    EXPECT_TRUE(t.rgba.empty());
}

TEST(Thumbnail, TruncatedJpegIsDisclosedNotSilent) {
    auto fx = load_fixture("truncated.jpg");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/truncated.jpg not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    /* stb's JPEG decoder tolerates a short stream (zero-fill leniency);
     * the contract then REQUIRES the disclosure: Ok with truncated=true,
     * never a silent partial image */
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_TRUE(t.truncated);
    EXPECT_EQ(64u, t.width);
    EXPECT_EQ(48u, t.height);
    EXPECT_EQ("jpeg", t.source_format);
}

TEST(Thumbnail, BudgetExceededIsReported) {
    auto fx = load_fixture("quad64x48.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.png not present";
    Node n(fx, true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_read = 64; /* far below the 151-byte file */
    ASSERT_EQ(ThumbStatus::BudgetExceeded, make_thumbnail(r, c, t));
    EXPECT_TRUE(t.rgba.empty());
}

TEST(Thumbnail, UnsupportedFormatsAreNamed) {
    {
        auto fx = load_fixture("webp_magic.bin");
        if (fx.empty())
            GTEST_SKIP() << "fixture data/thumbnails/webp_magic.bin not present";
        Node n(fx, true);
        auto r = n.reader();
        Thumbnail t;
        ASSERT_EQ(ThumbStatus::Unsupported, make_thumbnail(r, ThumbnailCaps{}, t));
        EXPECT_EQ("webp", t.source_format); /* phase-2 refusal, named */
    }
    {
        auto fx = load_fixture("garbage.bin");
        if (fx.empty())
            GTEST_SKIP() << "fixture data/thumbnails/garbage.bin not present";
        Node n(fx, true);
        auto r = n.reader();
        Thumbnail t;
        ASSERT_EQ(ThumbStatus::Unsupported, make_thumbnail(r, ThumbnailCaps{}, t));
        EXPECT_EQ("unknown", t.source_format);
    }
}

TEST(Thumbnail, UnreadableSpanIsNeverThumbnailed) {
    auto fx = load_fixture("quad64x48.bmp");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.bmp not present";
    Node n(fx, false);            /* extent-backed at device offset 4096 */
    n.dev.fail_from(4096 + 5000); /* the pixel rows beyond 5 KiB fail */
    auto r = n.reader();
    Thumbnail t;
    /* the BMP header decodes, the zero-filled rows would decode "fine" -
     * the truth gate must refuse instead of shipping a lying thumbnail */
    ASSERT_EQ(ThumbStatus::UnreadableSpan, make_thumbnail(r, ThumbnailCaps{}, t));
    EXPECT_TRUE(t.rgba.empty());
}

TEST(Thumbnail, BadParametersAreRejected) {
    Node n(std::vector<u8>(64, 0xFF), true);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 0;
    EXPECT_EQ(ThumbStatus::BadParameter, make_thumbnail(r, c, t));
    c.max_edge = 9000;
    EXPECT_EQ(ThumbStatus::BadParameter, make_thumbnail(r, c, t));
    c.max_edge = 256;
    c.max_pixels = 0;
    EXPECT_EQ(ThumbStatus::BadParameter, make_thumbnail(r, c, t));
    c.max_pixels = 1;
    c.max_read = 0;
    EXPECT_EQ(ThumbStatus::BadParameter, make_thumbnail(r, c, t));
}

TEST(Thumbnail, EmptyInputIsUnsupported) {
    Node n(std::vector<u8>(), true);
    auto r = n.reader();
    Thumbnail t;
    EXPECT_EQ(ThumbStatus::Unsupported, make_thumbnail(r, ThumbnailCaps{}, t));
}

/* ---------------- resident vs extent-backed equivalence ---------------- */

TEST(Thumbnail, ExtentBackedDecodesLikeResident) {
    auto fx = load_fixture("quad64x48.png");
    if (fx.empty())
        GTEST_SKIP() << "fixture data/thumbnails/quad64x48.png not present";
    Node n(fx, false);
    auto r = n.reader();
    Thumbnail t;
    ThumbnailCaps c;
    c.max_edge = 16;
    ASSERT_EQ(ThumbStatus::Ok, make_thumbnail(r, c, t));
    expect_pixel(t, 2, 2, 255, 0, 0, 0);
    expect_pixel(t, 13, 9, 255, 255, 0, 0);
    EXPECT_GT(n.dev.bytes(), 0u); /* proof the bytes really crossed the device */
}
